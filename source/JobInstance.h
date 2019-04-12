/*-
 * Copyright (c) 2018-2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __TZSERIAL_JOB_INSTANCE_H__
#define __TZSERIAL_JOB_INSTANCE_H__

#include <xtra_rhel.h>

#include <bitset>

#include "SoWrapper.h"

namespace tzrpc {

// https://crontab.guru
class SchTime {

public:
    SchTime() :
        sec_tp_(0), min_tp_(0), hour_tp_(0) {
    }

    // 根据指定的时间设置字符串，解析出下面的interval point成员
    bool parse(const std::string& sch_str);

    // 根据给定的时间，计算出下一个触发的时间间隔
    int32_t next_interval();
    int32_t next_interval(time_t from);

    std::string str();

private:

    template<std::size_t N>
    uint8_t find_ge_of(const std::bitset<N>& bits, int from);

    // 用来解析 时、分、秒的
    template<std::size_t N>
    bool parse_subtime(const std::string& sch_str, std::bitset<N>& store);


    // time_point
    std::bitset<60> sec_tp_;
    std::bitset<60> min_tp_;
    std::bitset<24> hour_tp_;
};


class SoWrapperFunc;
class TimerObject;

enum class ExecuteMethod : uint8_t {
    kExecDefer = 1,
    kExecAsync = 2,
    kExecBoundary,
};

enum class ExecuteStatus : uint8_t {
    kRunning = 1,
    kTerminating = 2,
    kDisabled = 3,
};

class JobInstance : public std::enable_shared_from_this<JobInstance> {

public:

    // 内置类型
    JobInstance(const std::string& name, const std::string& desc,
                const std::string& time_str, const std::function<int()>& func,
                enum ExecuteMethod method = ExecuteMethod::kExecDefer ):
        name_(name),
        desc_(desc),
        time_str_(time_str),
        exec_method_(method),
        so_path_(),
        builtin_func_(func),
        exec_status_(ExecuteStatus::kRunning),
        sch_timer_() {
    }

    // so动态类型
    JobInstance(const std::string& name, const std::string& desc,
                const std::string& time_str, const std::string& so_path,
                enum ExecuteMethod method = ExecuteMethod::kExecDefer ):
        name_(name),
        desc_(desc),
        time_str_(time_str),
        exec_method_(method),
        so_path_(so_path),
        exec_status_(ExecuteStatus::kRunning),
        sch_timer_() {
    }

    ~JobInstance();

    // 禁止拷贝
    JobInstance(const JobInstance&) = delete;
    JobInstance& operator=(const JobInstance&) = delete;


    bool init();
    void operator ()();
    bool next_trigger();
    void terminate();

    bool is_builtin() {
        return !!builtin_func_;
    }

    std::string str() {
        std::stringstream ss;

        ss << "JobInstance: " << name_ << std::endl
            << "desc: " << desc_ << ", "
            << "sch_time: " << time_str_ << ", "
            << "exec_method: " << static_cast<int32_t>(exec_method_) << ", "
            << "builtin: " << ( is_builtin()? "true" : "false" ) << ", "
            << "so_path: " << so_path_;

        return ss.str();
    }

private:
    // 原始配置参数
    const std::string name_;
    const std::string desc_;
    const std::string time_str_;

    enum  ExecuteMethod exec_method_; // defer async
    const std::string so_path_;
    std::unique_ptr<SoWrapperFunc> so_handler_;
    std::function<int()> builtin_func_;

    enum ExecuteStatus exec_status_;

    SchTime sch_timer_;              // 时间调度信息，解析后的结果
    std::shared_ptr<TimerObject> timer_;
};

} // end namespace tzrpc


#endif // __TZSERIAL_JOB_INSTANCE_H__
