#include <string>

#include "../../SoBridge.h"
#include "../../Log.h"

#ifdef __cplusplus
extern "C" {
#endif


int module_init() {
    return 0;
}

int module_exit() {
    return 0;
}


int so_handler(const msg_t* req, msg_t* rsp) {

    tzrpc::log_debug("get job1 running log...");
    return 0;
}


#ifdef __cplusplus
}
#endif