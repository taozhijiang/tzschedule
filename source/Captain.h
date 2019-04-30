/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __TZSERIAL_CAPTAIN_H__
#define __TZSERIAL_CAPTAIN_H__

#include <string>
#include <map>
#include <vector>
#include <memory>



#ifdef WITH_ZOOKEEPER

#include <clotho/zkFrame.h>
namespace Clotho {
class zkFrame;
}

#endif


namespace tzrpc {


class InsaneBind;


class Captain {
public:
    static Captain& instance();

public:
    bool init(const std::string& cfgFile);

    bool service_joinall();
    bool service_graceful();
    void service_terminate();

    void loop();

public:

    // 实例仍然会调度，这里控制是否执行函数

    volatile bool running_;
    
#ifdef WITH_ZOOKEEPER
    std::shared_ptr<InsaneBind> insane_bind_;
    std::shared_ptr<Clotho::zkFrame> zk_frame_;
#endif // WITH_ZOOKEEPER

private:
    Captain();

    ~Captain() {
        // Singleton should not destoried normally,
        // if happens, just terminate quickly
        ::exit(0);
    }


    bool initialized_;

public:
};

} // end namespace tzrpc


#endif // __TZSERIAL_CAPTAIN_H__
