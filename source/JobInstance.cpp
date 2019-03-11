/*-
 * Copyright (c) 2018-2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */


#include "StrUtil.h"
#include "JobInstance.h"

#include "Log.h"

namespace tzrpc {



// meta char:  * , - / 
// 根据指定的时间设置字符串，解析出下面的interval point成员



// 用来解析 时、分、秒的
template<std::size_t N>
bool SchTime::parse_subtime(const std::string& sch_str, std::bitset<N> & store) {

    int max_val = static_cast<int>(N); // ignore warning
    std::vector<std::string> vec;
    boost::split(vec, sch_str, boost::is_any_of(","));

    for (size_t i=0; i<vec.size(); ++i) {

        if (vec[i].find('*') != std::string::npos) {
            // *
            if (vec[i].size() == 1) {
                store.set();
                return true;
            }

            // */3
            auto it = vec[i].find('/');
            if (it == std::string::npos) {
                log_err("invalid sch_str: %s, subitem: %s", sch_str.c_str(), vec[i].c_str());
                return false;
            }

            std::string time_istr = vec[i].substr(it + 1);
            // */ -> */1
            if (time_istr.empty()) {
                store.set();
                return true;
            } else {
                int time_i = ::atoi(time_istr.c_str());
                if (time_i > 0 && time_i < max_val ) {
                    for (auto j = 0; j<max_val; j=j+time_i) {
                        store.set(j);
                    }
                } else {
                    log_err("invalid sch_str: %s, subitem: %s", sch_str.c_str(), vec[i].c_str());
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
                log_err("invalid sch_str: %s, subitem: %s", sch_str.c_str(), vec[i].c_str());
                return false;
            }

            int from = ::atoi(time_p[0].c_str());
            int to = ::atoi(time_p[1].c_str());

            if (from < 0 || to < 0 ||
                from > max_val || to > max_val ||
                from >= to ) {
                log_err("invalid sch_str: %s, subitem: %s", sch_str.c_str(), vec[i].c_str());
                return false;
            }

            while (from <= to) {
                store.set(from);
                ++ from;
            }
        }


        if (!vec[i].empty()) {
            int from = ::atoi(vec[i].c_str());
            if (from < 0 || from > max_val) {
                log_err("invalid sch_str: %s, subitem: %s", sch_str.c_str(), vec[i].c_str());
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
    for (auto it=sub.begin(); it!=sub.end(); ) {
        StrUtil::trim_whitespace(*it);
        if (it->empty()) {
            it = sub.erase(it);
        } else {
            ++ it;
        }
    }

    if (sub.size() != 3) {
        log_err("invalid sch_str: %s", sch_str.c_str());
        return false;
    }

    // sec
    if (!parse_subtime<60>(sub[0], sec_tp_)) {
        log_err("parse sec part failed, full str: %s.", sch_str.c_str());
        return false;
    }

    // min
    if (!parse_subtime<60>(sub[1], min_tp_)) {
        log_err("parse min part failed, full str: %s.", sch_str.c_str());
        return false;
    }

    // hour
    if (!parse_subtime<24>(sub[2], hour_tp_)) {
        log_err("parse hour part failed, full str: %s.", sch_str.c_str());
        return false;
    }

    if ( sec_tp_.none() || min_tp_.none() || hour_tp_.none() )
    {
        log_err("integrity check failed...");
        return false;
    }

    return true;
}


std::string SchTime::str() {
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
    for (int i=from; i<N; ++i) {
        if (bits.test(i)) {
            return i;
        }
    }

    return -1;
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
    int& next_hour= tm_time.tm_hour;

try {

    do {

        next_sec ++;

        next_sec = find_ge_of(sec_tp_, next_sec);
        if (next_sec < 0) {
            next_sec = find_ge_of(sec_tp_, 0);
            next_min ++;
        }

        next_min = find_ge_of(min_tp_, next_min);
        if (next_min < 0) {
            next_min = find_ge_of(min_tp_, 0);
            next_hour++;
        }

        next_hour = find_ge_of(hour_tp_, next_hour);
        if (next_hour < 0) {
            next_hour = find_ge_of(hour_tp_, 0);
        }

        if (next_sec < 0 || next_min < 0 || next_hour < 0) {
            log_err("find next_interval failed...");
            return -1;
        }

        next_tm = ::mktime(&tm_time) - from;
        if (next_tm < 0) { // 日期溢出了
            log_debug("override day switch from %d-%d-%d %d:%d:%d",
                      tm_time.tm_year + 1970, tm_time.tm_mon, tm_time.tm_mday,
                      tm_time.tm_hour, tm_time.tm_sec, tm_time.tm_sec);
            next_tm += 24 * 60 * 60;
        }

    } while (0);

} catch (std::exception& e) {
    log_err("invalid idx accessed: %s", e.what());
    return -1;
}

    return static_cast<int32_t>(next_tm);
}




bool JobInstance::init() {
	
	if(name_.empty() || sch_time_.empty() || so_path_.empty()) {
		return false;
	}
	
	
	return true;
}


} // end namespace tzrpc
