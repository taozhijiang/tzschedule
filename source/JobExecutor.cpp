/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */


#include <xtra_rhel.h>

#include "Timer.h"
#include "Status.h"
#include "ConfHelper.h"

#include "JobInstance.h"
#include "JobExecutor.h"

#include "TinyTask.h"

namespace tzrpc {


void JE_add_task_defer(std::shared_ptr<JobInstance>& ins) {
    JobExecutor::instance().defer_queue_.PUSH(ins);
}

void JE_add_task_async(std::shared_ptr<JobInstance>& ins) {
    JobExecutor::instance().async_queue_.PUSH(ins);
}


JobExecutor& JobExecutor::instance() {
    static JobExecutor helper;
    return helper;
}


bool JobExecutor::init(const libconfig::Config& conf) {

    // common params
    conf.lookupValue("schedule.thread_pool_size", conf_.thread_number_);
    conf.lookupValue("schedule.thread_pool_size_hard", conf_.thread_number_hard_);
    conf.lookupValue("schedule.thread_pool_step_queue_size", conf_.thread_step_queue_size_);

    conf.lookupValue("schedule.thread_pool_async_size", conf_.thread_number_async_);

    if (conf_.thread_number_hard_ < conf_.thread_number_) {
        conf_.thread_number_hard_ = conf_.thread_number_;
    }

    if (conf_.thread_number_ <= 0 || conf_.thread_number_ > 100 ||
        conf_.thread_number_hard_ > 100 ||
        conf_.thread_number_hard_ < conf_.thread_number_) {
        log_err("invalid thread_pool_size and hard setting: %d, %d",
                conf_.thread_number_, conf_.thread_number_hard_);
        return false;
    }

    if (conf_.thread_step_queue_size_ < 0) {
        log_err("invalid thread_pool_step_queue_size setting: %d",
                conf_.thread_step_queue_size_);
        return false;
    }

    if (conf_.thread_number_async_ <= 0) {
        log_err("invalid thread_pool_async_size setting: %d",
                conf_.thread_number_async_);
        return false;
    }

    // 检查是否需要创建thread_adjust定时任务，进行线程池的动态伸缩
    if (conf_.thread_number_hard_ > conf_.thread_number_ &&
        conf_.thread_step_queue_size_ > 0) {
        log_notice("we will support thread adjust with param hard %d, queue_step %d",
                   conf_.thread_number_hard_, conf_.thread_step_queue_size_);

        thread_adjust_timer_ = Timer::instance().add_better_timer(
                               std::bind(&JobExecutor::threads_adjust, this, std::placeholders::_1),
                               1 * 1000, true);
        if (!thread_adjust_timer_) {
            log_err("create thread adjust timer failed.");
            return false;
        }
    }

    // so_handlers
    // 进行动态任务的加载和初始化

    try {
        const libconfig::Setting& handlers = conf.lookup("schedule.so_handlers");

        for (int i = 0; i < handlers.getLength(); ++i) {
            const libconfig::Setting& handler = handlers[i];
            if (!handle_so_task_conf(handler)) {
                log_err("prase handle detail conf failed.");
                return false;
            }
        }

    } catch (const libconfig::SettingNotFoundException& nfex) {
        log_err("schedule.so_handlers not found!");
    } catch (std::exception& e) {
        log_err("execptions catched for %s", e.what());
    }


    // other initialize

    if (!threads_.init_threads(
            std::bind(&JobExecutor::job_executor_run, this, std::placeholders::_1), conf_.thread_number_)) {
        log_err("job_executor_run init task failed!");
        return false;
    }

    async_main_ = boost::thread(std::bind(&JobExecutor::job_executor_async_run, this));
    async_task_ = std::make_shared<TinyTask>(conf_.thread_number_async_);
    if (!async_task_ || !async_task_->init()) {
        log_err("create async_task failed.");
        return false;
    }

    Status::instance().register_status_callback(
        "JobExecutor",
        std::bind(&JobExecutor::module_status, this,
                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    ConfHelper::instance().register_runtime_callback(
        "JobExecutor",
        std::bind(&JobExecutor::module_runtime, this,
                  std::placeholders::_1));

    return true;
}


bool JobExecutor::handle_so_task_conf(const libconfig::Setting& setting) {

    std::string name;
    std::string desc;
    std::string sch_time;
    std::string exec_method;
    std::string so_path;
    bool status = true;

    setting.lookupValue("name", name);
    setting.lookupValue("desc", desc);
    setting.lookupValue("sch_time", sch_time);
    setting.lookupValue("exec_method", exec_method);
    setting.lookupValue("so_path", so_path);
    setting.lookupValue("enable", status);

    // 禁用的服务，初始化的时候不予加载
    if (!status) {
        log_err("Task %s marked disabled, skip it at init stage.", name.c_str());
        return true;
    }

    enum ExecuteMethod method = ExecuteMethod::kExecDefer;
    if (!exec_method.empty()) {
        if (exec_method == "defer") {
            method = ExecuteMethod::kExecDefer;
        } else if (exec_method == "async") {
            method = ExecuteMethod::kExecAsync;
        } else {
            log_err("invalid exec_method: %s", exec_method.c_str());
            return false;
        }
    }

    std::unique_lock<std::mutex> lock(lock_);
    if (tasks_.find(name) != tasks_.end()) {
        log_err("task %s already registered, reject it (duplicate configure?)", name.c_str());
        return false;
    }

    auto ins = std::make_shared<JobInstance>(name, desc, sch_time, so_path, method);
    if (!ins || !ins->init()) {
        log_err("init JobInstance failed, name: %s", name.c_str());
        return false;
    }

    tasks_[name] = ins;
    log_debug("register handler %s success.", name.c_str());
    return true;
}




void JobExecutor::threads_adjust(const boost::system::error_code& ec) {

    JobExecutorConf conf{};

    {
        std::lock_guard<std::mutex> lock(lock_);
        conf = conf_;
    }

    SAFE_ASSERT(conf.thread_step_queue_size_ > 0);
    if (!conf.thread_step_queue_size_) {
        return;
    }

    // 进行检查，看是否需要伸缩线程池
    int expect_thread = conf.thread_number_;

    int queueSize = defer_queue_.SIZE();
    if (queueSize > conf.thread_step_queue_size_) {
        expect_thread += queueSize / conf.thread_step_queue_size_;
    }
    if (expect_thread > conf.thread_number_hard_) {
        expect_thread = conf.thread_number_hard_;
    }

    if (expect_thread != conf.thread_number_) {
        log_notice("start thread number: %d, expect resize to %d",
                   conf.thread_number_, expect_thread);
    }

    // 如果当前运行的线程和实际的线程一样，就不会伸缩
    threads_.resize_threads(expect_thread);

    return;
}


void JobExecutor::job_executor_run(ThreadObjPtr ptr) {

    log_alert("JobExecutor thread %#lx about to loop ...", (long)pthread_self());

    while (true) {

        std::weak_ptr<JobInstance> job_instance{};

        if (unlikely(ptr->status_ == ThreadStatus::kTerminating)) {
            log_err("thread %#lx is about to terminating...", (long)pthread_self());
            break;
        }

        // 线程启动
        if (unlikely(ptr->status_ == ThreadStatus::kSuspend)) {
            ::usleep(1 * 1000 * 1000);
            continue;
        }

        if (!defer_queue_.POP(job_instance, 1000 /*1s*/)) {
            continue;
        }

        if (auto s_instance = job_instance.lock()) {

            // call it
            (*s_instance)();

        } else {
            log_debug("instance already release before, give up this task.");
        }
    }

    ptr->status_ = ThreadStatus::kDead;
    log_info("JobExecutor thread %#lx is about to terminate ... ", (long)pthread_self());

    return;

}



void JobExecutor::job_executor_async_run() {

    log_alert("JobExecutor async %#lx about to loop ...", (long)pthread_self());

    while (true) {

        std::weak_ptr<JobInstance> job_instance{};

        uint32_t available = async_task_->available();
        if (available == 0) {
            log_notice("current async_task full.");
            boost::this_thread::sleep_for(milliseconds(200));
            continue;
        }

        if (!async_queue_.POP(job_instance, 1000 /*1s*/)) {
            continue;
        }

        if (auto s_instance = job_instance.lock()) {
            auto func = std::bind(&JobInstance::operator(), s_instance);
            async_task_->add_additional_task(func);
        } else {
            log_debug("instance already release before, give up this task.");
        }

    }

    log_info("JobExecutor async %#lx is about to terminate ... ", (long)pthread_self());

    return;

}

int JobExecutor::module_status(std::string& module, std::string& name, std::string& val) {

    module = "tzrpc";
    name = "JobExecutor";

    std::stringstream ss;

    {
        std::unique_lock<std::mutex> lock(lock_);
        for (auto iter = tasks_.begin(); iter != tasks_.end(); ++iter) {
            ss << "E:" << iter->first.c_str() << std::endl;
            ss << "\t" << iter->second->str().c_str() << std::endl;
        }
    }

    val = ss.str();

    return 0;
}


int JobExecutor::module_runtime(const libconfig::Config& conf) {

    // 首先是JobExecutor全局信息(比如线程池等)的动态更新

    JobExecutorConf new_conf{};

    conf.lookupValue("schedule.thread_pool_size", new_conf.thread_number_);
    conf.lookupValue("schedule.thread_pool_size_hard", new_conf.thread_number_hard_);
    conf.lookupValue("schedule.thread_pool_step_queue_size", new_conf.thread_step_queue_size_);
    conf.lookupValue("schedule.thread_pool_async_size", new_conf.thread_number_async_);

    if (new_conf.thread_number_hard_ < new_conf.thread_number_) {
        new_conf.thread_number_hard_ = new_conf.thread_number_;
    }

    if (new_conf.thread_number_ <= 0 || new_conf.thread_number_ > 100 ||
        new_conf.thread_number_hard_ > 100 ||
        new_conf.thread_number_hard_ < new_conf.thread_number_) {
        log_err("invalid thread_pool_size and thread_pool_size_hard setting: %d, %d",
                new_conf.thread_number_, new_conf.thread_number_hard_);
    } else {
        if (new_conf.thread_number_ != conf_.thread_number_) {
            log_notice("update thread_pool_size from %d to %d",
                       conf_.thread_number_, new_conf.thread_number_);
            conf_.thread_number_ = new_conf.thread_number_;
        }

        if (new_conf.thread_number_hard_ != conf_.thread_number_hard_) {
            log_notice("update thread_pool_size_hard from %d to %d",
                       conf_.thread_number_hard_, new_conf.thread_number_hard_);
            conf_.thread_number_hard_ = new_conf.thread_number_hard_;
        }
    }

    if (conf_.thread_step_queue_size_ < 0) {
        log_err("invalid thread_pool_step_queue_size setting: %d",
                new_conf.thread_step_queue_size_);
    } else if (new_conf.thread_step_queue_size_ != conf_.thread_step_queue_size_) {
        log_notice("update thread_pool_step_queue_size from %d to %d",
                   conf_.thread_step_queue_size_, new_conf.thread_step_queue_size_);
        conf_.thread_step_queue_size_ = new_conf.thread_step_queue_size_;
    }


    if (new_conf.thread_number_async_ <= 0) {
        log_err("invalid thread_pool_async_size setting: %d",
                new_conf.thread_number_async_);
    } else if (new_conf.thread_number_async_ != conf_.thread_number_async_) {
        log_notice("update thread_pool_async_size from %d to %d",
                   conf_.thread_number_async_, new_conf.thread_number_async_);
        conf_.thread_number_async_ = new_conf.thread_number_async_;

        // 更新异步线程池的最大线程数
        async_task_->modify_spawn_size(conf_.thread_number_async_);
    }

    // 判定是否需要增加thread_adjust
    if (conf_.thread_number_hard_ > conf_.thread_number_ &&
        conf_.thread_step_queue_size_ > 0) {

        log_notice("we will support thread adjust with param hard %d, queue_step %d",
                   conf_.thread_number_hard_, conf_.thread_step_queue_size_);

        if (!thread_adjust_timer_) {
            thread_adjust_timer_ = Timer::instance().add_better_timer(
                                   std::bind(&JobExecutor::threads_adjust, this, std::placeholders::_1),
                                   1 * 1000, true);
            if (!thread_adjust_timer_) {
                log_err("create thread adjust timer failed.");
            }
        }
    } else {

        // 需要取消该线程的执行（洁癖）
        if (thread_adjust_timer_) {
            log_notice("we will close thread_adjust_timer_");
            thread_adjust_timer_->revoke_timer();
            thread_adjust_timer_.reset();
        }

    }


    // 然后针对so-handlers进行配置

    try {
        const libconfig::Setting& handlers = conf.lookup("schedule.so_handlers");

        for (int i = 0; i < handlers.getLength(); ++i) {
            const libconfig::Setting& handler = handlers[i];
            if (!handle_so_task_runtime_conf(handler)) {
                log_err("prase handle detail conf failed.");
                return false;
            }
        }

    } catch (const libconfig::SettingNotFoundException& nfex) {
        log_err("schedule.so_handlers not found!");
    } catch (std::exception& e) {
        log_err("execptions catched for %s", e.what());
    }


    return 0;
}

bool JobExecutor::add_builtin_task(const std::string& name, const std::string& desc,
                                   const std::string& time_str, const std::function<int()>& func,
                                   bool async) {

    if (name.empty() || time_str.empty() || func ) {
        log_err("param fast check failed.");
        return false;
    }

    enum ExecuteMethod method = ExecuteMethod::kExecDefer;
    if (async)
        method = ExecuteMethod::kExecAsync;

    std::unique_lock<std::mutex> lock(lock_);
    if (tasks_.find(name) != tasks_.end()) {
        log_err("task %s already registered, reject it (duplicate configure?)", name.c_str());
        return false;
    }

    auto ins = std::make_shared<JobInstance>(name, desc, time_str, func, method);
    if (!ins || !ins->init()) {
        log_err("init builtin JobInstance failed, name: %s", name.c_str());
        return false;
    }

    tasks_[name] = ins;
    log_debug("register handler %s success.", name.c_str());
    return true;
}

bool JobExecutor::remove_so_task(const std::string& name) {

    std::unique_lock<std::mutex> lock(lock_);

    auto handle = tasks_.find(name);
    if (handle == tasks_.end()) {
        log_err("task %s not registered, fast return", name.c_str());
        return true;
    }

    if (handle->second->is_builtin()) {
        log_err("remove builtin task, weird... hh");
        return false;
    }

    // tasks_ 持有一份，shared_from_this()持有一份
    // terminate中会取消timer，释放掉shared_from_this()
    handle->second->terminate();

    while (!handle->second.unique()) {
        log_notice("handle outside ref count: %d ...", static_cast<int>(handle->second.use_count() - 1));
        boost::this_thread::sleep_for(milliseconds(100));
    }

    tasks_.erase(name);
    handle = tasks_.end();

    log_notice("handler %s successful removed.", name.c_str());
    return true;
}



bool JobExecutor::handle_so_task_runtime_conf(const libconfig::Setting& setting) {

    std::string name;
    std::string desc;
    std::string sch_time;
    std::string exec_method;
    std::string so_path;
    bool status = true;

    setting.lookupValue("name", name);
    setting.lookupValue("desc", desc);
    setting.lookupValue("sch_time", sch_time);
    setting.lookupValue("exec_method", exec_method);
    setting.lookupValue("so_path", so_path);
    setting.lookupValue("enable", status);

    // 禁用的服务，一直阻塞，直到服务不再被占用，然后卸载
    if (!status) {
        log_err("task %s marked disabled, we will try to unload it", name.c_str());
        return remove_so_task(name);
    }

    // 检查是否存在，如果存在则直接跳过
    // 如果需要更新服务，需要先将其mark为delete的，然后再加载之

    enum ExecuteMethod method = ExecuteMethod::kExecDefer;
    if (!exec_method.empty()) {
        if (exec_method == "defer") {
            method = ExecuteMethod::kExecDefer;
        } else if (exec_method == "async") {
            method = ExecuteMethod::kExecAsync;
        } else {
            log_err("invalid exec_method: %s", exec_method.c_str());
            return false;
        }
    }

    std::unique_lock<std::mutex> lock(lock_);
    if (tasks_.find(name) != tasks_.end()) {
        log_err("task %s already registered, reject it (duplicate configure?)", name.c_str());
        return false;
    }

    auto ins = std::make_shared<JobInstance>(name, desc, sch_time, so_path, method);
    if (!ins || !ins->init()) {
        log_err("init JobInstance failed, name: %s", name.c_str());
        return false;
    }

    tasks_[name] = ins;
    log_debug("register handler %s success.", name.c_str());
    return true;
}



} // end namespace tzrpc
