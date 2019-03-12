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

namespace tzrpc {


void JE_add_task(std::shared_ptr<JobInstance>& ins) {
    JobExecutor::instance().inline_queue_.PUSH(ins);
}

void JE_add_task_defer(std::shared_ptr<JobInstance>& ins) {
    JobExecutor::instance().defers_queue_.PUSH(ins);
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

    if (conf_.thread_number_hard_ < conf_.thread_number_) {
        conf_.thread_number_hard_ = conf_.thread_number_;
    }

    if (conf_.thread_number_ <= 0 || conf_.thread_number_ > 100 ||
        conf_.thread_number_hard_ > 100 ||
        conf_.thread_number_hard_ < conf_.thread_number_ )
    {
        log_err("invalid thread_pool_size setting: %d, %d",
                conf_.thread_number_, conf_.thread_number_hard_);
        return false;
    }

    if (conf_.thread_step_queue_size_ < 0) {
        log_err("invalid thread_step_queue_size setting: %d",
                conf_.thread_step_queue_size_);
        return false;
    }

    // handlers
    try
    {
        const libconfig::Setting& handlers = conf.lookup("schedule.so_handlers");

        for(int i = 0; i < handlers.getLength(); ++i) {
            const libconfig::Setting& handler = handlers[i];
            if (!parse_handle_conf(handler)) {
                log_err("prase handle detail conf failed.");
                return false;
            }
        }

    } catch (const libconfig::SettingNotFoundException &nfex) {
        log_err("schedule.so_handlers not found!");
    } catch (std::exception& e) {
        log_err("execptions catched for %s",  e.what());
    }


    // other initialize

    if (!threads_.init_threads(
            std::bind(&JobExecutor::job_executor_run, this, std::placeholders::_1), conf_.thread_number_)) {
        log_err("job_executor_run init task failed!");
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
    std::string so_path;

    setting.lookupValue("name", name);
    setting.lookupValue("desc", desc);
    setting.lookupValue("sch_time", sch_time);
    setting.lookupValue("so_path", so_path);

    std::unique_lock<std::mutex> lock(lock_);
    if (tasks_.find(name) != tasks_.end()) {
        log_err("Task %s already registered, reject it", name.c_str());
        return false;
    }

    auto ins = std::make_shared<JobInstance>(name, desc, sch_time, so_path);
    if (!ins || !ins->init()) {
        log_err("init JobInstance failed, name: %s", name.c_str());
        return false;
    }

    tasks_[name] = ins;
    log_debug("register handler %s success.", name.c_str());
    return true;
}




void JobExecutor::threads_adjust(const boost::system::error_code& ec) {
	
	// TODO
	
}


void JobExecutor::job_executor_run(ThreadObjPtr ptr) {

    log_alert("JobExecutor thread %#lx about to loop ...", (long)pthread_self());

    while (true) {

        std::weak_ptr<JobInstance> job_instance {};

        if (unlikely(ptr->status_ == ThreadStatus::kTerminating)) {
            log_err("thread %#lx is about to terminating...", (long)pthread_self());
            break;
        }

        // 线程启动
        if (unlikely(ptr->status_ == ThreadStatus::kSuspend)) {
            ::usleep(1*1000*1000);
            continue;
        }

        if (!inline_queue_.POP(job_instance, 1000 /*1s*/)) {
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


int JobExecutor::module_status(std::string& module, std::string& name, std::string& val) {

    module = "tzrpc";
    name = "JobExecutor";

    std::stringstream ss;

    

    // collect
    val = "NULL";

    return 0;
}


int JobExecutor::module_runtime(const libconfig::Config& conf) {

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

	return 0;
}

} // end namespace tzrpc
