/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */


#ifndef __TZSERIAL_JOB_EXECUTOR_H__
#define __TZSERIAL_JOB_EXECUTOR_H__

#include <xtra_rhel.h>

#include <other/Log.h>

#include <container/EQueue.h>
#include <concurrency/ThreadPool.h>
#include <concurrency/AsyncTask.h>

#include <scaffold/Setting.h>
#include <scaffold/Status.h>

#include "JobInstance.h"

#include <gtest/gtest_prod.h>

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

    FRIEND_TEST(ExecutorFriendTest, SoHandleTest);

    friend void JE_add_task_defer(std::shared_ptr<JobInstance>& ins);
    friend void JE_add_task_async(std::shared_ptr<JobInstance>& ins);

public:

    static JobExecutor& instance();

    // 可以用来创建内置的定时任务，不支持删除、更新等操作
    bool add_builtin_task(const std::string& name, const std::string& desc,
                          const std::string& time_str, const std::function<int(JobInstance *)>& func,
                          bool async = false);
    bool task_exists(const std::string& name);

    bool init(const libconfig::Config& conf);
    int module_runtime(const libconfig::Config& conf);
    int module_status(std::string& module, std::string& name, std::string& val);

private:

    JobExecutorConf conf_;

    std::mutex lock_;
    std::map<std::string, std::shared_ptr<JobInstance>> tasks_;

    // so task都是通过配置文件动态处理的，所以全部都是private
    bool handle_so_task_conf(const libconfig::Setting& setting);
    bool handle_so_task_runtime_conf(const libconfig::Setting& setting);

    // 无限阻塞，直到引用计数合法后，卸载对应so
    bool remove_so_task(const std::string& name);

    // 在线程池中依序列执行
    roo::EQueue<std::weak_ptr<JobInstance>> defer_queue_;
    roo::ThreadPool threads_;
    void job_executor_run(roo::ThreadObjPtr ptr);  // main task loop


    // 每个任务开辟一个新的线程执行，主要是用于比较耗时的任务
    roo::EQueue<std::weak_ptr<JobInstance>> async_queue_;
    boost::thread async_main_;
    std::shared_ptr<roo::AsyncTask> async_task_;
    void job_executor_async_run();  // main task loop

public:

    int threads_start() {

        roo::log_notice("about to start JobExecutor threads.");
        threads_.start_threads();
        return 0;
    }

    int threads_start_stop_graceful() {

        roo::log_notice("about to stop JobExecutor threads.");
        threads_.graceful_stop_threads();

        return 0;
    }

    int threads_join() {

        roo::log_notice("about to join JobExecutor threads.");
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
    std::shared_ptr<roo::TimerObject> thread_adjust_timer_;
    void threads_adjust(const boost::system::error_code& ec);
};



} // end namespace tzrpc


#endif // __TZSERIAL_JOB_EXECUTOR_H__
