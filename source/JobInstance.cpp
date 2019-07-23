/*-
 * Copyright (c) 2018-2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */


#include <other/Log.h>
#include <string/StrUtil.h>

#include "SoWrapper.h"
#include "JobInstance.h"

#include "Captain.h"


namespace tzrpc {



// meta char:  * , - /
// 根据指定的时间设置字符串，解析出下面的interval point成员



// 用来解析 时、分、秒的
template<std::size_t N>
bool SchTime::parse_subtime(const std::string& sch_str, std::bitset<N>& store) {

    int max_val = static_cast<int>(N); // ignore warning
    std::vector<std::string> vec;
    boost::split(vec, sch_str, boost::is_any_of(","));

    for (size_t i = 0; i < vec.size(); ++i) {

        if (vec[i].find('*') != std::string::npos) {
            // *
            if (vec[i].size() == 1) {
                store.set();
                return true;
            }

            // */3
            auto it = vec[i].find('/');
            if (it == std::string::npos) {
                roo::log_err("invalid sch_str: %s, subitem: %s", sch_str.c_str(), vec[i].c_str());
                return false;
            }

            std::string time_istr = vec[i].substr(it + 1);
            // */ -> */1
            if (time_istr.empty()) {
                store.set();
                return true;
            } else {
                int time_i = ::atoi(time_istr.c_str());
                if (time_i > 0 && time_i < max_val) {
                    for (auto j = 0; j < max_val; j = j + time_i) {
                        store.set(j);
                    }
                } else {
                    roo::log_err("invalid sch_str: %s, subitem: %s", sch_str.c_str(), vec[i].c_str());
                    return false;
                }
            }

            continue;
        }

        if (vec[i].find('-') != std::string::npos) {

            std::vector<std::string> time_p;
            boost::split(time_p, vec[i], boost::is_any_of("-"));
            if (time_p.size() != 2 ||
                time_p[0].empty() || time_p[1].empty()) {
                roo::log_err("invalid sch_str: %s, subitem: %s", sch_str.c_str(), vec[i].c_str());
                return false;
            }

            int from = ::atoi(time_p[0].c_str());
            int to = ::atoi(time_p[1].c_str());

            if (from < 0 || to < 0 ||
                from > max_val || to > max_val ||
                from >= to) {
                roo::log_err("invalid sch_str: %s, subitem: %s", sch_str.c_str(), vec[i].c_str());
                return false;
            }

            while (from <= to) {
                store.set(from);
                ++from;
            }
        }


        if (!vec[i].empty()) {
            int from = ::atoi(vec[i].c_str());
            if (from < 0 || from > max_val) {
                roo::log_err("invalid sch_str: %s, subitem: %s", sch_str.c_str(), vec[i].c_str());
                return false;
            }

            //
            store.set(from);
        }
    }

    return true;
}




bool SchTime::parse(const std::string& sch_str) {

    // reset before value
    sec_tp_.reset();
    min_tp_.reset();
    hour_tp_.reset();

    std::vector<std::string> sub{};
    boost::split(sub, sch_str, boost::is_any_of(" \t\n"));

    // 删除连接处的可能空白字符
    for (auto it = sub.begin(); it != sub.end();) {
        roo::StrUtil::trim_whitespace(*it);
        if (it->empty()) {
            it = sub.erase(it);
        } else {
            ++it;
        }
    }

    if (sub.size() != 3) {
        roo::log_err("invalid sch_str: %s", sch_str.c_str());
        return false;
    }

    // sec
    if (!parse_subtime<60>(sub[0], sec_tp_)) {
        roo::log_err("parse sec part failed, full str: %s.", sch_str.c_str());
        return false;
    }

    // min
    if (!parse_subtime<60>(sub[1], min_tp_)) {
        roo::log_err("parse min part failed, full str: %s.", sch_str.c_str());
        return false;
    }

    // hour
    if (!parse_subtime<24>(sub[2], hour_tp_)) {
        roo::log_err("parse hour part failed, full str: %s.", sch_str.c_str());
        return false;
    }

    if (sec_tp_.none() || min_tp_.none() || hour_tp_.none()) {
        roo::log_err("integrity check failed...");
        return false;
    }

    return true;
}


std::string SchTime::str() const {
    std::stringstream ss;

    ss << "sec_:  " << sec_tp_ << std::endl;
    ss << "min_:  " << min_tp_ << std::endl;
    ss << "hour_: " << hour_tp_ << std::endl;

    return ss.str();
}


// greate & equal of from
template<std::size_t N>
uint8_t SchTime::find_ge_of(const std::bitset<N>& bits, int from) {

    // 从from开始向尾查找，如果查找不到就返回负数
    for (int i = from; i < N; ++i) {
        if (bits.test(i)) {
            return i;
        }
    }

    return 0;
}


int32_t SchTime::next_interval() {
    time_t now = ::time(NULL);
    return next_interval(now);
}

// 根据给定的时间，计算出下一个触发的时间间隔
int32_t SchTime::next_interval(time_t from) {

    struct tm tm_time;
    localtime_r(&from, &tm_time);
    int64_t next_tm = 0;

    int& next_sec = tm_time.tm_sec;
    int& next_min = tm_time.tm_min;
    int& next_hour = tm_time.tm_hour;

    try {

        do {

            if (!hour_tp_.test(next_hour)) {
                next_hour = find_ge_of(hour_tp_, next_hour + 1);
                if (next_hour == 0) {
                    next_hour = find_ge_of(hour_tp_, 0);
                }
                next_min = find_ge_of(min_tp_, 0);
                next_sec = find_ge_of(sec_tp_, 0);
            } else if (!min_tp_.test(next_min)) {
                next_min = find_ge_of(min_tp_, next_min + 1);
                if (next_min == 0) {
                    next_min = find_ge_of(min_tp_, 0);
                    next_hour = find_ge_of(hour_tp_, next_hour + 1);
                    if (next_hour == 0) {
                        next_hour = find_ge_of(hour_tp_, 0);
                    }
                }
                next_sec = find_ge_of(sec_tp_, 0);
            } else {
                next_sec = find_ge_of(sec_tp_, next_sec + 1);
                if (next_sec == 0) {
                    next_sec = find_ge_of(sec_tp_, 0);

                    next_min = find_ge_of(min_tp_, next_min + 1);
                    if (next_min == 0) {
                        next_min = find_ge_of(min_tp_, 0);
                        next_hour = find_ge_of(hour_tp_, next_hour + 1);
                        if (next_hour == 0) {
                            next_hour = find_ge_of(hour_tp_, 0);
                        }
                    }
                }
            }

            // 最终合法性校验
            if (next_sec < 0 || next_min < 0 || next_hour < 0 ||
                next_sec > 60 || next_min >= 60 || next_hour > 23) {
                roo::log_err("find next_interval failed...");
                roo::log_err("next_hour: %d, next_min: %d, next_sec:%d ",
                        next_hour, next_min, next_sec);
                return -1;
            }

            next_tm = ::mktime(&tm_time) - from;
            if (next_tm < 0) { // 日期溢出了
                roo::log_info("overflow day switch from %d-%d-%d %d:%d:%d",
                          tm_time.tm_year + 1900, tm_time.tm_mon, tm_time.tm_mday,
                          tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
                next_tm += 24 * 60 * 60;
                roo::log_info("new next_tm: %ld", next_tm);
            }

        } while (0);

    } catch (std::exception& e) {
        roo::log_err("invalid idx accessed: %s", e.what());
        return -1;
    }

    return static_cast<int32_t>(next_tm);
}





JobInstance::~JobInstance() {
    roo::log_info("Job destructed forever:\n%s", this->str().c_str());
}


// 对于内置类型，只需要进行next_trigger计算下一次执行时间
// 就可以了，对于so类型，则进行实际的so加载和初始化
bool JobInstance::init() {

    if (name_.empty() || time_str_.empty() || (!builtin_func_ && so_path_.empty()) ) {
        roo::log_err("param fast check failed.");
        return false;
    }

    if (!sch_timer_.parse(time_str_)) {
        roo::log_err("parse time setting failed %s.", time_str_.c_str());
        return false;
    }

    if (!builtin_func_) {
        so_handler_.reset(new SoWrapperFunc(so_path_));
        if (!so_handler_ || !so_handler_->init()) {
            roo::log_err("create and init so_handler failed.");
            return false;
        }
    }

    if (!next_trigger()) {
        roo::log_err("first init next_trigger failed.");
        return false;
    }

    roo::log_info("JobInstance initialized finished:\n%s", this->str().c_str());
    return true;
}



int JobInstance::operator()() {

    SAFE_ASSERT(builtin_func_ || so_handler_);

    do {

        if(!Captain::instance().running_)
            break;


        int code = 0;
        if (builtin_func_) {
            code = builtin_func_(this);
        } else if (so_handler_) {
            code = (*so_handler_)(this);
        } else {
            roo::log_err("job with empty func!");
            return -1;
        }

        if (code != 0) {
            roo::log_err("job func return %d, job desc: %s", code, this->str().c_str());
        }

    } while (0);


    // 如果设置了Terminate标识，则设置退出标志
    // 因为我们是so_handler执行完之后才会设置下一个schedule，所以不应当会有并发问题
    //
    // 如果用户对同一个so设置两个任务，可能会有问题，不要这么做
    //
    if (exec_status_ == ExecuteStatus::kTerminating) {
        roo::log_notice("marked job terminating, we will disabled it!");
        exec_status_ = ExecuteStatus::kDisabled;
        return 0;
    }

    next_trigger();
    
    return 0;
}


void JE_add_task_defer(std::shared_ptr<JobInstance>& ins);
void JE_add_task_async(std::shared_ptr<JobInstance>& ins);

bool JobInstance::next_trigger() {

    if (exec_status_ != ExecuteStatus::kRunning) {
        roo::log_notice("current exec_status is %d, not next...", static_cast<uint8_t>(exec_status_));
        return false;
    }

    int next_interval = sch_timer_.next_interval();
    if (next_interval <= 0) {
        roo::log_err("next_interval failed.");
        return false;
    }

    if (exec_method_ == ExecuteMethod::kExecDefer) {
        timer_ = Captain::instance().timer_ptr_->add_better_timer(
            std::bind(JE_add_task_defer, shared_from_this()), next_interval * 1000, false);
    } else if (exec_method_ == ExecuteMethod::kExecAsync) {
        timer_ = Captain::instance().timer_ptr_->add_better_timer(
            std::bind(JE_add_task_async, shared_from_this()), next_interval * 1000, false);
    } else {
        roo::log_err("unknown exec_method: %d", static_cast<int32_t>(exec_method_));
        return false;
    }

    roo::log_info("next trigger for %s success, with next_interval: %d secs.",
              name_.c_str(), next_interval);
    return true;
}


void JobInstance::terminate() {

    exec_status_ = ExecuteStatus::kTerminating;
    if (timer_) {
        timer_->revoke_timer();
        timer_.reset();
    }
}

} // end namespace tzrpc
