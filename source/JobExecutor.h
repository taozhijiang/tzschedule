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


class TinyTask;
class JobInstance;


struct JobExecutorConf {

    int thread_number_;
    int thread_number_hard_;  // 允许最大的线程数目
    int thread_step_queue_size_;
    int thread_number_async_;

    JobExecutorConf() :
        thread_number_(1),
        thread_number_hard_(1),
        thread_step_queue_size_(0),
        thread_number_async_(10) {
    }

} __attribute__((aligned(4)));



void JE_add_task_defer(std::shared_ptr<JobInstance>& ins);
void JE_add_task_async(std::shared_ptr<JobInstance>& ins);

class JobExecutor {

    friend void JE_add_task_defer(std::shared_ptr<JobInstance>& ins);
    friend void JE_add_task_async(std::shared_ptr<JobInstance>& ins);

public:

    static JobExecutor& instance();

    bool init(const libconfig::Config& conf);
    int module_runtime(const libconfig::Config& conf);
    int module_status(std::string& module, std::string& name, std::string& val);

private:

    JobExecutorConf conf_;

    std::mutex lock_;
    std::map<std::string, std::shared_ptr<JobInstance>> tasks_;

    bool parse_handle_conf(const libconfig::Setting& setting);

    // 在线程池中依序列执行
    EQueue<std::weak_ptr<JobInstance>> defer_queue_;
    ThreadPool threads_;
    void job_executor_run(ThreadObjPtr ptr);  // main task loop


    // 每个任务开辟一个新的线程执行，主要是用于比较耗时的任务
    EQueue<std::weak_ptr<JobInstance>> async_queue_;
    boost::thread async_main_;
    std::shared_ptr<TinyTask> async_task_;
    void job_executor_async_run();  // main task loop

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

    JobExecutor() { }
    virtual ~JobExecutor() { }

    // 禁止拷贝
    JobExecutor(const JobExecutor&) = delete;
    JobExecutor& operator=(const JobExecutor&) = delete;

    // 根据rpc_queue_自动伸缩线程负载
    std::shared_ptr<TimerObject> thread_adjust_timer_;
    void threads_adjust(const boost::system::error_code& ec);
};



} // end namespace tzrpc


#endif // __TZSERIAL_JOB_EXECUTOR_H__
