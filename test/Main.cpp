#include <iostream>
#include "gtest/gtest.h"

#include <xtra_rhel.h>

#include <syslog.h>

#include "Log.h"
#include "SslSetup.h"

#include "ConfHelper.h"
#include "Captain.h"

using namespace tzrpc;

int main(int argc, char** argv) {

    testing::InitGoogleTest(&argc, argv);

    // do initialize

    // SSL 环境设置
    if (!Ssl_thread_setup()) {
        log_err("SSL env setup error!");
        ::exit(1);
    }

    std::string cfgFile = "tzschedule.conf";
    if (!ConfHelper::instance().init(cfgFile)) {
        return -1;
    }

    auto conf_ptr = ConfHelper::instance().get_conf();
    if (!conf_ptr) {
        log_err("ConfHelper return null conf pointer, maybe your conf file ill!");
        return -1;
    }

    set_checkpoint_log_store_func(::syslog);
    if (!log_init(7)) {
        std::cerr << "init syslog failed!" << std::endl;
        return -1;
    }

    return RUN_ALL_TESTS();
}



namespace boost {
void assertion_failed(char const* expr, char const* function, char const* file, long line) {
    log_err("BAD!!! expr `%s` assert failed at %s(%ld): %s", expr, file, line, function);
}
} // end boost
