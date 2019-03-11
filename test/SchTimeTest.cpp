#include <gmock/gmock.h>
#include <string>

using namespace ::testing;

#include "Log.h"
#include "JobInstance.h"

using namespace tzrpc;

TEST(SchTimeTest, SchTimeParseTest) {

	std::string test_sch = "* * *";
	SchTime schTm{};
	
	ASSERT_THAT(schTm.parse(test_sch), Eq(true));
//    log_debug("%s =>\n %s", test_sch.c_str(), schTm.str().c_str());

    test_sch = "0 3 4";
    ASSERT_THAT(schTm.parse(test_sch), Eq(true));
//    log_debug("%s =>\n %s", test_sch.c_str(), schTm.str().c_str());

    test_sch = "0 0 0";
    ASSERT_THAT(schTm.parse(test_sch), Eq(true));
//    log_debug("%s =>\n %s", test_sch.c_str(), schTm.str().c_str());


    log_debug("SchTimeTest finished.");
}



TEST(SchTimeTest, SchTimeNextTest) {

    std::string test_sch = "*/3 * *";
    SchTime schTm{};

    ASSERT_THAT(schTm.parse(test_sch), Eq(true));
    int next_i = schTm.next_interval();
    ASSERT_THAT(next_i, Eq(3));

    // 在线 http://www.matools.com/crontab 进行校验
    // http://tool.chinaz.com/Tools/unixtime.aspx
    // '2018-04-30 00:00:00' ==> 1525017600

    time_t FROM = 1525017600L;

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
    ASSERT_THAT(next_i, Eq(2*60*60));

    log_debug("SchTimeNextTest finished.");
}

