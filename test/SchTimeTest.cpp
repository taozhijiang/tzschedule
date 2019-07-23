#include <gmock/gmock.h>
#include <string>

using namespace ::testing;

#include <other/Log.h>
#include "JobInstance.h"

using namespace tzrpc;

TEST(SchTimeTest, SchTimeParseTest) {

    std::string test_sch = "* * *";
    SchTime schTm{};

    ASSERT_THAT(schTm.parse(test_sch), Eq(true));
//    roo::log_info("%s =>\n %s", test_sch.c_str(), schTm.str().c_str());

    test_sch = "0 3 4";
    ASSERT_THAT(schTm.parse(test_sch), Eq(true));
//    roo::log_info("%s =>\n %s", test_sch.c_str(), schTm.str().c_str());

    test_sch = "0 0 0";
    ASSERT_THAT(schTm.parse(test_sch), Eq(true));
//    roo::log_info("%s =>\n %s", test_sch.c_str(), schTm.str().c_str());


    roo::log_info("SchTimeTest finished.");
}



TEST(SchTimeTest, SchTimeNextTest) {

    // 在线 http://www.matools.com/crontab 进行校验
    // http://tool.chinaz.com/Tools/unixtime.aspx
    // '2018-04-30 00:00:00' ==> 1525017600
    time_t FROM = 1525017600L;

    std::string test_sch = "*/3 * *";
    SchTime schTm{};

    ASSERT_THAT(schTm.parse(test_sch), Eq(true));
    int next_i = schTm.next_interval(FROM);
    ASSERT_THAT(next_i, Eq(3));


    test_sch = "12,24 * *";
    ASSERT_THAT(schTm.parse(test_sch), Eq(true));
    next_i = schTm.next_interval(FROM);
    ASSERT_THAT(next_i, Eq(12));


    test_sch = "* */2 *";
    ASSERT_THAT(schTm.parse(test_sch), Eq(true));
    next_i = schTm.next_interval(FROM);
    ASSERT_THAT(next_i, Eq(1));

    test_sch = "* 2 *";
    ASSERT_THAT(schTm.parse(test_sch), Eq(true));
    next_i = schTm.next_interval(FROM);
    ASSERT_THAT(next_i, Eq(120));

    roo::log_info("SchTimeNextTest finished.");
}


TEST(SchTimeTest, SchTimeNextTest2) {

    // 在线 http://www.matools.com/crontab 进行校验
    // http://tool.chinaz.com/Tools/unixtime.aspx
    // '2018-04-30 00:00:00' ==> 1525017600
    time_t FROM = 1525017600L + 58;

    std::string test_sch = "*/3 * *";
    SchTime schTm{};

    ASSERT_THAT(schTm.parse(test_sch), Eq(true));
    int next_i = schTm.next_interval(FROM);
    ASSERT_THAT(next_i, Eq(2));  // next + 0


    test_sch = "12,24 * *";
    ASSERT_THAT(schTm.parse(test_sch), Eq(true));
    next_i = schTm.next_interval(FROM);
    ASSERT_THAT(next_i, Eq(14));

    test_sch = "* */2 *";
    ASSERT_THAT(schTm.parse(test_sch), Eq(true));
    next_i = schTm.next_interval(FROM);
    ASSERT_THAT(next_i, Eq(1));

    test_sch = "* 2 *";
    ASSERT_THAT(schTm.parse(test_sch), Eq(true));
    next_i = schTm.next_interval(FROM);
    ASSERT_THAT(next_i, Eq(2 * 60 - 58));

    roo::log_info("SchTimeNextTest2 finished.");
}


TEST(SchTimeTest, SchTimeNextTest3) {

    // 在线 http://www.matools.com/crontab 进行校验
    // http://tool.chinaz.com/Tools/unixtime.aspx
    // '2018-04-30 00:00:00' ==> 1525017600
    time_t FROM = 1525017600L + 24 * 60 * 60 - 2;

    std::string test_sch = "*/3 * *";
    SchTime schTm{};

    ASSERT_THAT(schTm.parse(test_sch), Eq(true));
    int next_i = schTm.next_interval(FROM);
    ASSERT_THAT(next_i, Eq(2));  // next + 0


    test_sch = "12,24 * *";
    ASSERT_THAT(schTm.parse(test_sch), Eq(true));
    next_i = schTm.next_interval(FROM);
    ASSERT_THAT(next_i, Eq(14));

    test_sch = "* */2 *";
    ASSERT_THAT(schTm.parse(test_sch), Eq(true));
    next_i = schTm.next_interval(FROM);
    ASSERT_THAT(next_i, Eq(2));

    test_sch = "* 2 *";
    ASSERT_THAT(schTm.parse(test_sch), Eq(true));
    next_i = schTm.next_interval(FROM);
    ASSERT_THAT(next_i, Eq(2 * 60 + 2));

    roo::log_info("SchTimeNextTest3 finished.");
}


