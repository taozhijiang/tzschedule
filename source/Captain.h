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


namespace tzrpc {



class Captain {
public:
    static Captain& instance();

public:
    bool init(const std::string& cfgFile);

    bool service_joinall();
    bool service_graceful();
    void service_terminate();

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
