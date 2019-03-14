/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */


#ifndef __TZSERIAL_THREAD_MNG_H__
#define __TZSERIAL_THREAD_MNG_H__


#include <xtra_rhel.h>

#include "EQueue.h"

namespace tzrpc {

typedef std::function<void()> TaskRunnable;

class ThreadGuard {

public:

    // TODO: create failed.
    explicit ThreadGuard(TaskRunnable& run, boost::thread_group& tgroup):
        tgroup_(tgroup) {
        thread_ = new boost::thread(run);
        tgroup_.add_thread(thread_);
    }

    ~ThreadGuard() {
        log_debug("thread exit...");
        tgroup_.remove_thread(thread_);
    }

    bool joinable() {
        return thread_->joinable();
    }

    void join() {
        thread_->join();
    }

    bool try_join_for(uint32_t msec) {
        return thread_->try_join_for(milliseconds(msec));
    }

private:
    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;

    boost::thread_group& tgroup_;
    boost::thread*       thread_;
};

class ThreadMng {

public:
    explicit ThreadMng(uint8_t max_spawn_task):
        max_spawn_task_(max_spawn_task) {
    }

    ~ThreadMng() {}

    // 禁止拷贝
    ThreadMng(const ThreadMng&) = delete;
    ThreadMng& operator=(const ThreadMng&) = delete;

    // 增加任务执行线程，但是不会超过限制数目
    bool try_add_task(TaskRunnable& run) {

        std::lock_guard<std::mutex> lock(lock_);
        if (threads_.size() >= max_spawn_task_)
            return false;

        std::shared_ptr<ThreadGuard> task( new ThreadGuard(run, thread_tgp_));
        threads_.emplace_back(std::move(task));
        return true;
    }

    // 阻塞，直到添加完成
    bool add_task(TaskRunnable& run) {

        std::unique_lock<std::mutex> lock(lock_);
        while (threads_.size() >= max_spawn_task_) {
            item_notify_.wait(lock);
        }

        // impossible
        if (threads_.size() >= max_spawn_task_)
            return false;

        std::shared_ptr<ThreadGuard> task( new ThreadGuard(run, thread_tgp_));
        threads_.emplace_back(std::move(task));
        return true;
    }


    int try_join() {

        std::unique_lock<std::mutex> lock(lock_);

        int size = 0;
        for (auto iter = threads_.begin(); iter != threads_.end(); /**/ ) {
            auto curr_iter = iter ++;
            if ((*curr_iter)->joinable()) {
                if ((*curr_iter)->try_join_for(1)) {
                    threads_.erase(curr_iter);
                    ++ size;
                }
            }
        }

        return size;
    }

    void join_all() {

        std::unique_lock<std::mutex> lock(lock_);

        for (auto it = threads_.begin(); it != threads_.end(); ++ it ) {
            if ((*it)->joinable())
                (*it)->join();
        }

        threads_.clear();
    }


    uint32_t current_size() {
        std::lock_guard<std::mutex> lock(lock_);
        return threads_.size();
    }

private:

    const uint32_t max_spawn_task_;

    std::mutex lock_;
    std::condition_variable item_notify_;

    std::vector<std::shared_ptr<ThreadGuard>> threads_;
    boost::thread_group thread_tgp_;

};



} // end namespace tzrpc

#endif // __TZSERIAL_THREAD_MNG_H__
