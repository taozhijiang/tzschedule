/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */


#include <xtra_rhel.h>

#include "Timer.h"
#include "Status.h"

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
    conf.lookupValue("thread_pool_size", conf_.thread_number_);
    conf.lookupValue("thread_pool_size_hard", conf_.thread_number_hard_);
    conf.lookupValue("thread_pool_step_queue_size", conf_.thread_step_queue_size_);
    conf.lookupValue("thread_pool_async_size", conf_.thread_number_async_);

    if (conf_.thread_number_hard_ < conf_.thread_number_) {
        conf_.thread_number_hard_ = conf_.thread_number_;
    }

    if (conf_.thread_number_ <= 0 || conf_.thread_number_ > 100 ||
        conf_.thread_number_hard_ > 100 ||
        conf_.thread_number_hard_ < conf_.thread_number_) {
        log_err("invalid thread_pool_size setting: %d, %d",
                conf_.thread_number_, conf_.thread_number_hard_);
        return false;
    }

    if (conf_.thread_step_queue_size_ < 0) {
        log_err("invalid thread_step_queue_size setting: %d",
                conf_.thread_step_queue_size_);
        return false;
    }

    if (conf_.thread_number_async_ <= 0) {
        log_err("invalid thread_pool_async_size setting: %d",
                conf_.thread_number_async_);
        return false;
    }

    if (conf_.thread_number_hard_ > conf_.thread_number_ &&
        conf_.thread_step_queue_size_ > 0) {
        log_debug("we will support thread adjust with param hard %d, queue_step %d",
                  conf_.thread_number_hard_, conf_.thread_step_queue_size_);

        if (!Timer::instance().add_timer(std::bind(&JobExecutor::threads_adjust, this, std::placeholders::_1),
                                         1 * 1000, true)) {
            log_err("create thread adjust timer failed.");
            return false;
        }
    }

    // handlers
    try {
        const libconfig::Setting& handlers = conf.lookup("schedule.so_handlers");

        for (int i = 0; i < handlers.getLength(); ++i) {
            const libconfig::Setting& handler = handlers[i];
            if (!parse_handle_conf(handler)) {
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


    return true;
}




bool JobExecutor::parse_handle_conf(const libconfig::Setting& setting) {

    std::string name;
    std::string desc;
    std::string sch_time;
    std::string exec_method;
    std::string so_path;

    setting.lookupValue("name", name);
    setting.lookupValue("desc", desc);
    setting.lookupValue("sch_time", sch_time);
    setting.lookupValue("exec_method", exec_method);
    setting.lookupValue("so_path", so_path);

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
        log_err("Task %s already registered, reject it", name.c_str());
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



    // collect
    val = "NULL";

    return 0;
}


int JobExecutor::module_runtime(const libconfig::Config& conf) {

    // 首先是JobExecutor全局信息(比如线程池等)的动态更新
#if 0
    int ret = service_impl_->module_runtime(conf);

    // 如果返回0，表示配置文件已经正确解析了，同时ExecutorConf也重新初始化了
    if (ret == 0) {
        log_notice("update JobExecutor ...");
        std::lock_guard<std::mutex> lock(conf_lock_);
        conf_ = service_impl_->get_executor_conf();

    }

    return ret;
#endif


    // 然后针对so-handlers进行配置

    return 0;
}

} // end namespace tzrpc
