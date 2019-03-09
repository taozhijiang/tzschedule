/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */


#ifndef __TZSERIAL_TINY_TASK_H__
#define __TZSERIAL_TINY_TASK_H__


// 本来想直接使用future来做的，但是现有生产环境太旧，std::future
// 和boost::future都不可用，这里模拟一个任务结构，采用one-task-per-thread的方式，
// 进行弹性的任务创建执行。

#include <memory>
#include <functional>

#include <boost/thread.hpp>

#include <Utils/EQueue.h>

namespace tzrpc {

typedef std::function<void()> TaskRunnable;

class TinyTask: public std::enable_shared_from_this<TinyTask> {
public:

    explicit TinyTask(uint8_t max_spawn_task):
       max_spawn_task_(max_spawn_task){
       }

    ~TinyTask() {}

    // 禁止拷贝
    TinyTask(const TinyTask&) = delete;
    TinyTask& operator=(const TinyTask&) = delete;

    bool init(){
        thread_run_.reset(new boost::thread(std::bind(&TinyTask::run, shared_from_this())));
        if (!thread_run_){
            printf("create run work thread failed! ");
            return false;
        }

        return true;
    }

    void add_additional_task(const TaskRunnable& func) {
        tasks_.PUSH(func);
    }


private:
    void run() {

        printf("TinyTask thread %#lx begin to run ...", (long)pthread_self());

        while (true) {

            std::vector<TaskRunnable> task;
            size_t count = tasks_.POP(task, max_spawn_task_, 1000);
            if( !count ){  // 空闲
                continue;
            }

            std::vector<boost::thread> thread_group;
            for(size_t i=0; i<task.size(); ++i) {
                thread_group.emplace_back(boost::thread(task[i]));
            }

            for(size_t i=0; i<task.size(); ++i) {
                if(thread_group[i].joinable())
                    thread_group[i].join();
            }

            printf("count %d task process done!", static_cast<int>(task.size()));
        }
    }

private:
    const uint32_t max_spawn_task_;

    std::shared_ptr<boost::thread> thread_run_;
    EQueue<TaskRunnable> tasks_;
};


} // end namespace tzrpc

#endif // __TZSERIAL_TINY_TASK_H__
