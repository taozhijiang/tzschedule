/*-
 * Copyright (c) 2018-2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#include "SoBridge.h"
#include "SlibLoader.h"

#include "JobInstance.h"

#include "SoWrapper.h"


namespace tzrpc {

bool SoWrapper::load_dl() {

    dl_ = std::make_shared<SLibLoader>(dl_path_);
    if (!dl_) {
        return false;
    }

    if (!dl_->init()) {
        roo::log_err("init dl %s failed!", dl_->get_dl_path().c_str());
        return false;
    }

    return true;
}



bool SoWrapperFunc::init() {

    if (!load_dl()) {
        roo::log_err("load dl failed!");
        return false;
    }
    if (!dl_->load_func<so_handler_t>("so_handler", &func_)) {
        roo::log_err("Load so_handler_t func for %s failed.", dl_path_.c_str());
        return false;
    }
    return true;
}

int SoWrapperFunc::operator()(JobInstance* inst) {

    if (!func_) {
        roo::log_err("func not initialized.");
        return -1;
    }

    int ret = 0;

    try {
        ret = func_(reinterpret_cast<const msg_t*>(inst), NULL);
    } catch (const std::exception& e) {
        roo::log_err("post func call std::exception detect: %s.", e.what());
    } catch (...) {
        roo::log_err("get func call exception detect.");
    }

    return ret;
}



} // end namespace tzrpc

