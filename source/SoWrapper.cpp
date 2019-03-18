/*-
 * Copyright (c) 2018-2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#include "SoBridge.h"
#include "SlibLoader.h"
#include "SoWrapper.h"

namespace tzrpc {


bool SoWrapper::load_dl() {

    dl_ = std::make_shared<SLibLoader>(dl_path_);
    if (!dl_) {
        return false;
    }

    if (!dl_->init()) {
        log_err("init dl %s failed!", dl_->get_dl_path().c_str());
        return false;
    }

    return true;
}



bool SoWrapperFunc::init() {

    if (!load_dl()) {
        log_err("load dl failed!");
        return false;
    }
    if (!dl_->load_func<so_handler_t>("so_handler", &func_)) {
        log_err("Load so_handler_t func for %s failed.", dl_path_.c_str());
        return false;
    }
    return true;
}

int SoWrapperFunc::operator()() {

    if (!func_) {
        log_err("func not initialized.");
        return -1;
    }

    int ret = 0;

    try {
        ret = func_(NULL, NULL);
    } catch (const std::exception& e) {
        log_err("post func call std::exception detect: %s.", e.what());
    } catch (...) {
        log_err("get func call exception detect.");
    }

    return ret;
}


int SoWrapperFunc::operator()(const std::string& req) {

    if (!func_) {
        log_err("func not initialized.");
        return -1;
    }

    msg_t msg_req{};
    fill_msg(&msg_req, req.c_str(), req.size());

    int ret = -1;

    try {
        ret = func_(&msg_req, NULL);
    } catch (const std::exception& e) {
        log_err("post func call std::exception detect: %s.", e.what());
    } catch (...) {
        log_err("get func call exception detect.");
    }

    return ret;
}

int SoWrapperFunc::operator()(std::string& rsp) {

    if (!func_) {
        log_err("func not initialized.");
        return -1;
    }

    int ret = -1;
    msg_t msg_rsp{};

    try {
        ret = func_(NULL, &msg_rsp);
    } catch (const std::exception& e) {
        log_err("post func call std::exception detect: %s.", e.what());
    } catch (...) {
        log_err("get func call exception detect.");
    }

    rsp = std::string(msg_rsp.data, msg_rsp.len);
    return ret;
}


int SoWrapperFunc::operator()(const std::string& req, std::string& rsp) {

    if (!func_) {
        log_err("func not initialized.");
        return -1;
    }

    msg_t msg_req{};
    fill_msg(&msg_req, req.c_str(), req.size());

    int ret = -1;
    msg_t msg_rsp{};

    try {
        ret = func_(&msg_req, &msg_rsp);
    } catch (const std::exception& e) {
        log_err("post func call std::exception detect: %s.", e.what());
    } catch (...) {
        log_err("get func call exception detect.");
    }

    rsp = std::string(msg_rsp.data, msg_rsp.len);
    return ret;
}


} // end namespace tzrpc

