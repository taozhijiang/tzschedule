/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */


#ifndef __TZSERIAL_STR_UTIL_H__
#define __TZSERIAL_STR_UTIL_H__

#include <libconfig.h++>


#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "Log.h"

// 类静态函数可以直接将函数定义丢在头文件中

namespace tzrpc {

struct StrUtil {

    static const int kMaxBuffSize = 2*8190;
    static std::string str_format(const char * fmt, ...) {

        char buff[kMaxBuffSize + 1] = {0, };
        uint32_t n = 0;

        va_list ap;
        va_start(ap, fmt);
        n += vsnprintf(buff, kMaxBuffSize, fmt, ap);
        va_end(ap);
        buff[n] = '\0';

        return std::string(buff, n);
    }


    static size_t trim_whitespace(std::string& str) {

        size_t index = 0;
        size_t orig = str.size();

        // trim left whitespace
        for (index = 0; index < str.size() && isspace(str[index]); ++index)
            /* do nothing*/;
        str.erase(0, index);

        // trim right whitespace
        for (index = str.size(); index > 0 && isspace(str[index - 1]); --index)
            /* do nothing*/;
        str.erase(index);

        return orig - str.size();
    }


    template <typename T>
    static std::string convert_to_string(const T& arg) {
        try {
            return boost::lexical_cast<std::string>(arg);
        }
        catch(boost::bad_lexical_cast& e) {
            return "";
        }
    }

    static std::string trim_lowcase(std::string str) {  // copy
        return boost::algorithm::trim_copy(boost::to_lower_copy(str));
    }

};



class UriRegex: public boost::regex {
public:
    explicit UriRegex(const std::string& regexStr) :
        boost::regex(regexStr), str_(regexStr) {
    }

    std::string str() const {
        return str_;
    }

private:
    std::string str_;
};


} // end namespace tzrpc

#endif // __TZSERIAL_STR_UTIL_H__
