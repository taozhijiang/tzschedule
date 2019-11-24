cmake_minimum_required (VERSION 2.8.11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 " )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Woverloaded-virtual -Wpointer-arith -Wwrite-strings -Werror=format -march=native " )
include("${PROJECT_SOURCE_DIR}/help.cmake")
define_colors()

####################
### 修改编译选项 ###

# if want to see debug in verbose mode
# cmake -DCMAKE_VERBOSE_MAKEFILE=true ../
# if want to see build in detail
# make -- VERBOSE=1

option(BUILD_DEBUG "Build Project for Debug Propurse..." OFF)
option(BUILD_WITH_ZOOKEEPER "Build Project for ZooKeeper distributed features..." OFF)

if(BUILD_DEBUG)
    set(CMAKE_BUILD_TYPE Debug)
    message(STATUS "${Red}build_type Debug flag: ${CMAKE_CXX_FLAGS_DEBUG}${ColourReset}")
else(BUILD_DEBUG)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
    #add_definitions(-DNP_DEBUG)
    message(STATUS "${Red}build_type RelWithDebInfo flag: ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}${ColourReset}")
endif(BUILD_DEBUG)

if(EXISTS "${PROJECT_SOURCE_DIR}/VERSION")
    file(READ "${PROJECT_SOURCE_DIR}/VERSION" PROGRAM_VERSION)
    string(STRIP "${PROGRAM_VERSION}" PROGRAM_VERSION)
endif()

configure_file (
    "include/version.h.in"
    "../include/version.h"
    @ONLY )

###################


set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

include_directories(
    ${PROJECT_SOURCE_DIR}/include/
    ${PROJECT_SOURCE_DIR}/source/
    ${PROJECT_SOURCE_DIR}/deps/xtra_rhel7.x/self/include/
    ${PROJECT_SOURCE_DIR}/deps/xtra_rhel7.x/boost_prefix/include/
    ${PROJECT_SOURCE_DIR}/deps/xtra_rhel7.x/google_prefix/include/
    ${PROJECT_SOURCE_DIR}/deps/roo
)

if(BUILD_WITH_ZOOKEEPER)
add_definitions(-DWITH_ZOOKEEPER)
add_subdirectory( deps/clotho )
endif()

add_subdirectory( source/ )
add_subdirectory( ${PROJECT_SOURCE_DIR}/deps/roo )

link_directories(
    /usr/lib64/mysql/
    ${PROJECT_SOURCE_DIR}/deps/xtra_rhel7.x/self/lib
    ${PROJECT_SOURCE_DIR}/deps/xtra_rhel7.x/boost_prefix/lib
    ${PROJECT_SOURCE_DIR}/deps/xtra_rhel7.x/google_prefix/lib
)

# local temp generated file
exec_program( "export BUILD_VAR=`git log -1 --pretty=%H` && echo 'const char *build_commit_version = \"VCS: Commit:' $BUILD_VAR '\";' > build_version.cpp ")
exec_program( "export BUILD_VAR=`git symbolic-ref HEAD` && echo 'const char *build_commit_branch = \"VCS: Branch:' $BUILD_VAR '\";' >> build_version.cpp ")
exec_program( "export BUILD_VAR=`git log -1 --pretty=%cd` && echo 'const char *build_commit_date = \"VCS: Date:' $BUILD_VAR '\";' >> build_version.cpp ")
exec_program( "export BUILD_VAR=`git log -1 --pretty=\"%an %ae\"` && echo 'const char *build_commit_author = \"VCS: Author:' $BUILD_VAR '\";' >> build_version.cpp ")
exec_program( "export BUILD_VAR=`date +%F` && echo 'const char *build_time = \"Build At:' $BUILD_VAR '\";' >> build_version.cpp ")

add_executable( argus_service main.cpp setup.cpp build_version.cpp )

# ld iconv ?

set (WHOLE_LIBS Argus )
set (EXTRA_LIBS ${EXTRA_LIBS} Roo )
set (EXTRA_LIBS ${EXTRA_LIBS} ssl crypto config++)
set (EXTRA_LIBS ${EXTRA_LIBS} pthread)
set (EXTRA_LIBS ${EXTRA_LIBS} boost_system boost_thread boost_chrono boost_regex)
set (EXTRA_LIBS ${EXTRA_LIBS} protoc protobuf )
set (EXTRA_LIBS ${EXTRA_LIBS} glogb )

if(BUILD_WITH_ZOOKEEPER)
set (EXTRA_LIBS ${EXTRA_LIBS} Clotho zookeeper_mt)
endif()

# 某些库需要静态全连接，否则主服务没使用但是so使用，会报符号没有找到
target_link_libraries( argus_service -lrt -rdynamic -ldl
    -Wl,--whole-archive ${WHOLE_LIBS} -Wl,--no-whole-archive
    ${EXTRA_LIBS}
)

if(BUILD_DEBUG)
    message(STATUS "${Red}we will build examples and unit_tests ...${ColourReset}")
    
    find_program(MEMORYCHECK_COMMAND NAMES valgrind)
    if(MEMORYCHECK_COMMAND)
        # ctest -T memcheck
        set(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full")
        include(CTest)
    endif()

    enable_testing()
    
    add_subdirectory( source/so-handler/job1 )
    add_subdirectory( source/so-handler/job2 )
    add_subdirectory( source/so-handler/jobasync )

    add_subdirectory( test )

endif()
