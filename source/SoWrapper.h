/*-
 * Copyright (c) 2018-2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __TZSERIAL_SO_WRAPPER_H__
#define __TZSERIAL_SO_WRAPPER_H__

// 所有的http uri 路由

#include <libconfig.h++>

#include <vector>
#include <string>
#include <memory>

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include "SoBridge.h"

namespace tzrpc {

class SLibLoader;

class SoWrapper {
public:
    explicit SoWrapper(const std::string& dl_path):
        dl_path_(dl_path),
        dl_({}) {
    }

    bool load_dl();

protected:
    std::string dl_path_;
    std::shared_ptr<SLibLoader> dl_;
};


// 默认的handle函数，后续如有需求，则再进行扩充
class SoWrapperFunc: public SoWrapper {

public:
    explicit SoWrapperFunc(const std::string& dl_path):
        SoWrapper(dl_path) {
    }

    bool init();

    int operator()();

    // 下面的暂时没用上
    int operator()(const std::string& req);
    int operator()(std::string& rsp);
    int operator()(const std::string& req, std::string& rsp);

private:
    so_handler_t func_;
};


} // end namespace tzrpc


#endif //__TZSERIAL_SO_WRAPPER_H__
