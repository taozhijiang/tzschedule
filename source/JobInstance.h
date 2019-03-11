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


namespace tzrpc {

// https://crontab.guru
class SchTime {

public:
    SchTime():
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
    bool parse_subtime(const std::string& sch_str, std::bitset<N> & store);

    
    // time_point
    std::bitset<60> sec_tp_;
    std::bitset<60> min_tp_;
    std::bitset<24> hour_tp_;
};


class JobInstance {

public:

    JobInstance(const std::string& name, const std::string& desc,
                const std::string& sch_time, const std::string& so_path): 
        name_(name),
        desc_(desc),
        sch_time_(sch_time),
        so_path_(so_path) {
    }

    ~JobInstance() {
    }


    bool init();

    std::string str() {
        std::stringstream ss;

        ss << "JobInstance: " << name_ << std::endl;
        ss << "desc: " << desc_ << ", ";
        ss << "sch_time_: " << sch_time_ << ", ";
        ss << "so_path: " << so_path_;

        return ss.str();
    }

private:
    // 原始配置参数
    const std::string name_;
    const std::string desc_;
    const std::string sch_time_;
    const std::string so_path_;
};

} // end namespace tzrpc


#endif // __TZSERIAL_JOB_INSTANCE_H__
