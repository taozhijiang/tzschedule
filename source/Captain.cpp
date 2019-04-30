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

#include "InsaneBind.h"

#include "Captain.h"

namespace tzrpc {

// 在主线程中最先初始化，所以不考虑竞争条件问题
Captain& Captain::instance() {
    static Captain service;
    return service;
}

Captain::Captain() :
    running_(true),
    initialized_(false) {
}


#ifdef WITH_ZOOKEEPER

int callback_for_node(const std::string& dept, const std::string& serv, const std::string& node, 
                      const std::map<std::string, std::string>& property) {

    auto iter = property.find("cfg_log_level");
    if(iter != property.end()) {
        int32_t value = ::atoi(iter->second.c_str());
        if(value >=0 && value <=7 ) {
            log_info("log_level setup to %d", value);
            tzrpc::log_init(value);
        }
    }

    // 是否放弃锁竞争
    iter = property.find("enable");
    if( iter == property.end() || iter->second != "1") {
        Captain::instance().zk_frame_->recipe_service_unlock("bankpay", "argus_service", "master");
    }

    return 0;
}

int callback_for_serv(const std::string& dept, const std::string& serv,
                      const std::map<std::string, std::string>& property) {
    
    if(Captain::instance().zk_frame_->recipe_service_try_lock("bankpay", "argus_service", "master", 0)) {
        log_debug("owns lock ...");
        Captain::instance().running_ = true;
    } else {
        log_debug("lost lock ...");
        Captain::instance().running_ = false;
    }

    return 0;
}

#endif // WITH_ZOOKEEPER

bool Captain::init(const std::string& cfgFile) {

    if (initialized_) {
        log_err("Manager already initlialized...");
        return false;
    }

    if (!Timer::instance().init()) {
        log_err("init Timer service failed, critical !!!!");
        return false;
    }

    if (!ConfHelper::instance().init(cfgFile)) {
        log_err("init ConfHelper (%s) failed, critical !!!!", cfgFile.c_str());
        return false;
    }

    auto conf_ptr = ConfHelper::instance().get_conf();
    if (!conf_ptr) {
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

    if (!JobExecutor::instance().init(*conf_ptr)) {
        log_err("JobExecutor init failed, critital.");
        return false;
    }

    // add specified builtin task here


    
#ifdef WITH_ZOOKEEPER

    // fail will throw exception
    
    int         instance_port;
    conf_ptr->lookupValue("schedule.instance_port", instance_port);
    if(instance_port < 0)  instance_port = 0;

    insane_bind_ = std::make_shared<InsaneBind>(instance_port);
    instance_port =  insane_bind_->port();
    log_debug("we will using port: %d", instance_port);

    std::string zookeeper_idc;
    std::string zookeeper_host;
    conf_ptr->lookupValue("schedule.zookeeper_idc", zookeeper_idc);
    conf_ptr->lookupValue("schedule.zookeeper_host", zookeeper_host);

    if(zookeeper_idc.empty() || zookeeper_host.empty()) {
        log_err("zookeeper conf not found!");
        return false;
    }

    zk_frame_ = std::make_shared<Clotho::zkFrame>(zookeeper_idc);
    if(!zk_frame_ || !zk_frame_->init(zookeeper_host)) {
        log_err("initialize clotho support failed %s @ %s.", zookeeper_host.c_str(), zookeeper_idc.c_str());
        return false;
    }

    std::map<std::string, std::string> properties = {
        { "ppa", "ppa_val" },
    };


    Clotho::NodeType node("bankpay", 
                          "argus_service",
                          "0.0.0.0:" + std::to_string(static_cast<unsigned long long>(instance_port)), 
                          properties);
    if(zk_frame_->register_node(node, false) != 0) {
        log_err("register node failed.");
        return false;
    }
    
    if(zk_frame_->recipe_attach_node_property_cb("bankpay", "argus_service", instance_port, callback_for_node) != 0) {
        log_err("register node_property callback failed.");
        return false;
    }

    if(zk_frame_->recipe_attach_serv_property_cb("bankpay", "argus_service", callback_for_serv)!= 0) {
        log_err("register serv_property callback failed.");
        return false;
    }

#if 0
    // 周期性的检测
    if(!Timer::instance().add_timer(std::bind(&Clotho::zkFrame::periodicly_care, zk_frame_), 10, true)) {
        log_err("add periodicly_care of zookeeper failed.");
        return false;
    }
#endif

    if(zk_frame_->recipe_service_try_lock("bankpay", "argus_service", "master", 0)) {
        running_ = true;
    } else {
        running_ = false;
    }


    log_debug("initialize with zookeeper successfully.");

#endif // WITH_ZOOKEEPER


    JobExecutor::instance().threads_start();

    log_info("Captain makes all initialized...");
    initialized_ = true;

    return true;
}


bool Captain::service_graceful() {

    JobExecutor::instance().threads_join();
    Timer::instance().threads_join();
    return true;
}

void Captain::service_terminate() {

    ::sleep(1);
    ::_exit(0);
}

bool Captain::service_joinall() {

    JobExecutor::instance().threads_join();
    Timer::instance().threads_join();
    return true;
}


void Captain::loop() {

}


} // end namespace tzrpc
