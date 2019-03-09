/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */


#ifndef __TZSERIAL_JOB_EXECUTOR_H__
#define __TZSERIAL_JOB_EXECUTOR_H__

#include <xtra_rhel.h>

#include "Log.h"
#include "EQueue.h"

#include "ThreadPool.h"
#include "ConfHelper.h"

namespace tzrpc {

class JobInstance;

class JobExecutor {

public:

    static JobExecutor& instance();

    bool init();
    int module_runtime(const libconfig::Config& conf);
    int module_status(std::string& module, std::string& name, std::string& val);

private:
    EQueue<std::shared_ptr<JobInstance>> inline_queue_;
    EQueue<std::shared_ptr<JobInstance>> defers_queue_;

    ThreadPool threads_;
    void job_executor_run(ThreadObjPtr ptr);  // main task loop

public:

    int threads_start() {

        log_notice("about to start JobExecutor threads.");
        threads_.start_threads();
        return 0;
    }

    int threads_start_stop_graceful() {

        log_notice("about to stop JobExecutor threads.");
        threads_.graceful_stop_threads();

        return 0;
    }

    int threads_join() {

        log_notice("about to join JobExecutor threads.");
        threads_.join_threads();
        return 0;
    }

private:

    virtual ~JobExecutor(){}

    // 禁止拷贝
    JobExecutor(const JobExecutor&) = delete;
    JobExecutor& operator=(const JobExecutor&) = delete;
	
    // 根据rpc_queue_自动伸缩线程负载
    void threads_adjust(const boost::system::error_code& ec);
};

} // end namespace tzrpc


#endif // __TZSERIAL_JOB_EXECUTOR_H__