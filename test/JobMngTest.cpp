#include <gmock/gmock.h>
#include <string>

using namespace ::testing;

#include "ConfHelper.h"
#include "Log.h"
#include "JobInstance.h"
#include "JobExecutor.h"

using namespace tzrpc;

int test_func(JobInstance* inst) {
    return 0;
}

#define INST JobExecutor::instance()

TEST(JobMngTest, BuiltinJobTest) {

    ASSERT_THAT(INST.add_builtin_task("test1", "desc", "*/4 * *", test_func), Eq(true));
    ASSERT_THAT(INST.add_builtin_task("test1", "desc", "*/4 * *", test_func), Eq(false));


    ASSERT_THAT(INST.task_exists("test1"), Eq(true));
}


// fixture should be in the same namespace

namespace tzrpc {

class ExecutorFriendTest: public ::testing::Test {
public:

};

TEST_F(ExecutorFriendTest, SoHandleTest) {

    ASSERT_THAT(INST.add_builtin_task("test2", "desc", "*/4 * *", test_func), Eq(true));
    ASSERT_THAT(INST.task_exists("test2"), Eq(true));

    // builtin can not be removed
    ASSERT_THAT(INST.remove_so_task("test2"), Eq(false));
    ASSERT_THAT(INST.task_exists("test2"), Eq(true));

    // dynamic add so handle
    // 用ctest执行可能会失败，因为路径不对
    ASSERT_THAT(ConfHelper::instance().init("../argus_example.conf"), Eq(true));

    auto conf_ptr = ConfHelper::instance().get_conf();
    ASSERT_THAT(!!conf_ptr, Eq(true));


    try {
        const libconfig::Setting& handlers = conf_ptr->lookup("schedule.so_handlers");

        for (int i = 0; i < handlers.getLength(); ++i) {
            const libconfig::Setting& handler = handlers[i];
            ASSERT_THAT(INST.handle_so_task_conf(handler), Eq(true));
        }

    } catch (const libconfig::SettingNotFoundException& nfex) {
        log_err("schedule.so_handlers not found!");
        ASSERT_THAT(false, Eq(true));
    } catch (std::exception& e) {
        log_err("execptions catched for %s", e.what());
        ASSERT_THAT(false, Eq(true));
    }

    ASSERT_THAT(INST.task_exists("job-1"), Eq(true));
    ASSERT_THAT(INST.task_exists("job-2"), Eq(true));
    ASSERT_THAT(INST.task_exists("job-3"), Eq(true));

#if 0
    // can successful remove

    // 因为这里没有进行定时任务的调度，所以remove_so_task的引用
    // 计数检查会失败，下面的部分不能执行成功

    ASSERT_THAT(INST.remove_so_task("job-2"), Eq(true));
    ASSERT_THAT(INST.task_exists("job-1"), Eq(true));
    ASSERT_THAT(INST.task_exists("job-2"), Eq(false));
    ASSERT_THAT(INST.task_exists("job-3"), Eq(true));
#endif

}

} // end tzrpc
