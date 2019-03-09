/*-
 * Copyright (c) 2018-2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __TZSERIAL_JOB_INSTANCE_H__
#define __TZSERIAL_JOB_INSTANCE_H__

#include <memory>

namespace tzrpc {

struct JobInstance {
    JobInstance(): {
    }



    std::string str() {
        std::stringstream ss;

        ss << "JobInstance: ";

        return ss.str();
    }

};

} // end namespace tzrpc


#endif // __TZSERIAL_JOB_INSTANCE_H__
