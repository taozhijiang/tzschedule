/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#include <cstdlib>

#include <memory>
#include <string>
#include <map>


#include <other/Log.h>
#include <other/InsaneBind.h>

#include <concurrency/Timer.h>

#include <scaffold/Setting.h>
#include <scaffold/Status.h>

#include "JobExecutor.h"
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

static bool service_enable = true;


static int callback_for_serv(const std::string& dept, const std::string& serv,
                             const std::map<std::string, std::string>& property) {
    
    if (!service_enable)
        return 0;

    if(Captain::instance().zk_frame_->recipe_service_try_lock("bankpay", "argus_service", "master", 0)) {
        roo::log_info("owns lock ...");
        Captain::instance().running_ = true;
    } else {
        roo::log_info("lost lock ...");
        Captain::instance().running_ = false;
    }

    return 0;
}

static int callback_for_node(const std::string& dept, const std::string& serv, const std::string& node,
                             const std::map<std::string, std::string>& property) {

    auto iter = property.find("cfg_roo::log_level");
    if(iter != property.end()) {
        int32_t value = ::atoi(iter->second.c_str());
        if(value >=0 && value <=7 ) {
            roo::log_warning("roo::log_level setup to %d", value);
            tzrpc::roo::log_init(value);
        }
    }

    // 是否放弃锁竞争
    iter = property.find("enable");
    if( iter == property.end() || iter->second != "1") {
        // 放弃锁
        roo::log_warning("node %s not enabled, give up lock_master and disable running", node.c_str());
        Captain::instance().zk_frame_->recipe_service_unlock("bankpay", "argus_service", "master");
        service_enable = false;
        Captain::instance().running_ = false;
    } else {
        // 启用服务，并尝试锁竞争
        service_enable = true;
        callback_for_serv(dept, serv, {});
    }

    return 0;
}


#endif // WITH_ZOOKEEPER

bool Captain::init(const std::string& cfgFile) {

    if (initialized_) {
        roo::log_err("Manager already initlialized...");
        return false;
    }
    timer_ptr_ = std::make_shared<roo::Timer>();
    if (!timer_ptr_ || !timer_ptr_->init()) {
        roo::log_err("Create and init roo::Timer service failed.");
        return false;
    }

    setting_ptr_ = std::make_shared<roo::Setting>();
    if (!setting_ptr_ || !setting_ptr_->init(cfgFile)) {
        roo::log_err("Create and init roo::Setting with cfg %s failed.", cfgFile.c_str());
        return false;
    }

    auto setting_ptr = setting_ptr_->get_setting();
    if (!setting_ptr) {
        roo::log_err("roo::Setting return null pointer, maybe your conf file ill???");
        return false;
    }

    int log_level = 0;
    setting_ptr->lookupValue("log_level", log_level);
    if (log_level <= 0 || log_level > 7) {
        roo::log_warning("Invalid log_level %d, reset to default 7(DEBUG).", log_level);
        log_level = 7;
    }

    std::string log_path;
    setting_ptr->lookupValue("log_path", log_path);
    if (log_path.empty())
        log_path = "./log";

    roo::log_init(log_level, "", log_path, LOG_LOCAL6);
    roo::log_warning("Initialized roo::Log with level %d, path %s.", log_level, log_path.c_str());

    status_ptr_ = std::make_shared<roo::Status>();
    if (!status_ptr_) {
        roo::log_err("Create roo::Status failed.");
        return false;
    }

    if (!JobExecutor::instance().init(*setting_ptr)) {
        roo::log_err("JobExecutor init failed, critital error.");
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
    roo::log_info("we will using port: %d", instance_port);

    std::string zookeeper_idc;
    std::string zookeeper_host;
    conf_ptr->lookupValue("schedule.zookeeper_idc", zookeeper_idc);
    conf_ptr->lookupValue("schedule.zookeeper_host", zookeeper_host);

    if(zookeeper_idc.empty() || zookeeper_host.empty()) {
        roo::log_err("zookeeper conf not found!");
        return false;
    }

    zk_frame_ = std::make_shared<Clotho::zkFrame>(zookeeper_idc);
    if(!zk_frame_ || !zk_frame_->init(zookeeper_host)) {
        roo::log_err("initialize clotho support failed %s @ %s.", zookeeper_host.c_str(), zookeeper_idc.c_str());
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
        roo::log_err("register node failed.");
        return false;
    }
    
    if(zk_frame_->recipe_attach_node_property_cb("bankpay", "argus_service", instance_port, callback_for_node) != 0) {
        roo::log_err("register node_property callback failed.");
        return false;
    }

    if(zk_frame_->recipe_attach_serv_property_cb("bankpay", "argus_service", callback_for_serv)!= 0) {
        roo::log_err("register serv_property callback failed.");
        return false;
    }

#if 0
    // 周期性的检测
    if(!Timer::instance().add_timer(std::bind(&Clotho::zkFrame::periodicly_care, zk_frame_), 10, true)) {
        roo::log_err("add periodicly_care of zookeeper failed.");
        return false;
    }
#endif

    if(zk_frame_->recipe_service_try_lock("bankpay", "argus_service", "master", 0)) {
        running_ = true;
    } else {
        running_ = false;
    }


    roo::log_info("initialize with zookeeper successfully.");

#endif // WITH_ZOOKEEPER


    JobExecutor::instance().threads_start();

    roo::log_warning("Captain makes all initialized...");
    initialized_ = true;

    return true;
}


bool Captain::service_graceful() {

    JobExecutor::instance().threads_join();
    timer_ptr_->threads_join();
    return true;
}

void Captain::service_terminate() {

    ::sleep(1);
    ::_exit(0);
}

bool Captain::service_joinall() {

    JobExecutor::instance().threads_join();
    timer_ptr_->threads_join();
    return true;
}


void Captain::loop() {

}


} // end namespace tzrpc
