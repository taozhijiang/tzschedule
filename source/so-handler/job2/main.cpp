#include <string>
#include <other/Log.h>

#include "../../SoBridge.h"
#include "../../JobInstance.h"

using namespace tzrpc;

#ifdef __cplusplus
extern "C"
{
#endif


int module_init() {
    return 0;
}

int module_exit() {
    return 0;
}

int so_handler(const msg_t* req, msg_t* rsp) {

    const JobInstance* ptr = reinterpret_cast<const JobInstance *>(req);
    roo::log_info("inline job2 running log, job desc: %s, thread %lx ...",
             ptr->str().c_str(), (long)pthread_self());

    return 0;
}


#ifdef __cplusplus
}
#endif
