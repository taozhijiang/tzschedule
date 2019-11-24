cmake_minimum_required (VERSION 2.8.11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x " )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Woverloaded-virtual -Wpointer-arith -Wwrite-strings -Werror=format -march=native " )
include("${PROJECT_SOURCE_DIR}/help.cmake")
define_colors()


include_directories(         
    ${PROJECT_SOURCE_DIR}/include/
    ${PROJECT_SOURCE_DIR}/source/
    ${PROJECT_SOURCE_DIR}/deps/
    ${PROJECT_SOURCE_DIR}/deps/xtra_rhel7.x/self/include/
    ${PROJECT_SOURCE_DIR}/deps/xtra_rhel7.x/boost_prefix/include/ 
    ${PROJECT_SOURCE_DIR}/deps/xtra_rhel7.x/google_prefix/include/ 
)

link_directories( /usr/lib64/mysql/
    ${PROJECT_SOURCE_DIR}/deps/xtra_rhel7.x/self/lib/
    ${PROJECT_SOURCE_DIR}/deps/xtra_rhel7.x/boost_prefix/lib
    ${PROJECT_SOURCE_DIR}/deps/xtra_rhel7.x/google_prefix/lib
    ../lib
)

set (TEST_HOST_LIB Argus)

set (EXTRA_LIBS ${EXTRA_LIBS} rt dl)
set (EXTRA_LIBS ${EXTRA_LIBS} crypto ssl pthread)

set (EXTRA_LIBS ${EXTRA_LIBS} config++)
set (EXTRA_LIBS ${EXTRA_LIBS} boost_system boost_thread boost_chrono boost_regex)
set (EXTRA_LIBS ${EXTRA_LIBS} gtest gmock gtest_main)

add_individual_test(SchTime)
add_individual_test(JobMng)


