/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#include <syslog.h>
#include <cstdlib>

#include <memory>
#include <string>
#include <map>


#include "Log.h"
#include "Timer.h"

#include "ConfHelper.h"
#include "JobExecutor.h"

#include "Captain.h"

namespace tzrpc {

// 在主线程中最先初始化，所以不考虑竞争条件问题
Captain& Captain::instance() {
    static Captain service;
    return service;
}

Captain::Captain():
    initialized_(false){
}


bool Captain::init(const std::string& cfgFile) {

    if (initialized_) {
        log_err("Manager already initlialized...");
        return false;
    }

    if (!Timer::instance().init()) {
        log_err("init Timer service failed, critical !!!!");
        return false;
    }

    if(!ConfHelper::instance().init(cfgFile)) {
        log_err("init ConfHelper (%s) failed, critical !!!!", cfgFile.c_str());
        return false;
    }

    auto conf_ptr = ConfHelper::instance().get_conf();
    if(!conf_ptr) {
        log_err("ConfHelper return null conf pointer, maybe your conf file ill!");
        return false;
    }

    int log_level = 0;
    conf_ptr->lookupValue("log_level", log_level);
    if (log_level <= 0 || log_level > 7) {
        log_notice("invalid log_level value, reset to default 7.");
        log_level = 7;
    }

    log_init(log_level);
    log_notice("initialized log with level: %d", log_level);

    if(!JobExecutor::instance().init(*conf_ptr)){
        log_err("JobExecutor init failed, critital.");
        return false;
    }

    JobExecutor::instance().threads_start();

    log_info("Captain makes all initialized...");
    initialized_ = true;

    return true;
}


bool Captain::service_graceful() {

    return true;
}

void Captain::service_terminate() {

    ::sleep(1);
    ::_exit(0);
}

bool Captain::service_joinall() {

    Timer::instance().threads_join();
    JobExecutor::instance().threads_join();
    return true;
}


} // end namespace tzrpc
