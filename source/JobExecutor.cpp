/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */


#include <xtra_rhel.h>

#include "Timer.h"
#include "Status.h"

#include "JobExecutor.h"

namespace tzrpc {


bool JobExecutor::init() {

    if (!threads_.init_threads(
            std::bind(&JobExecutor::job_executor_run, this, std::placeholders::_1), 4)) {
        log_err("job_executor_run init task failed!");
        return false;
    }

    Status::instance().register_status_callback(
                "JobExecutor",
                std::bind(&JobExecutor::module_status, this,
                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));


    return true;
}

void JobExecutor::threads_adjust(const boost::system::error_code& ec) {
	
	// TODO
	
}


void JobExecutor::job_executor_run(ThreadObjPtr ptr) {

    log_alert("JobExecutor thread %#lx about to loop ...", (long)pthread_self());

	#if 0
    while (true) {

        std::shared_ptr<RpcInstance> rpc_instance {};

        if (unlikely(ptr->status_ == ThreadStatus::kTerminating)) {
            log_err("thread %#lx is about to terminating...", (long)pthread_self());
            break;
        }

        // 线程启动
        if (unlikely(ptr->status_ == ThreadStatus::kSuspend)) {
            ::usleep(1*1000*1000);
            continue;
        }

        if (!rpc_queue_.POP(rpc_instance, 1000 /*1s*/) || !rpc_instance) {
            continue;
        }

        // execute RPC handler
        service_impl_->handle_RPC(rpc_instance);
    }
	#endif

    ptr->status_ = ThreadStatus::kDead;
    log_info("io_service thread %#lx is about to terminate ... ", (long)pthread_self());

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
