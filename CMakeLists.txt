cmake_minimum_required (VERSION 2.8.11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x " )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Woverloaded-virtual -Wpointer-arith -Wwrite-strings -Werror=format -march=native " )

# 精简日志
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__FILENAME__=\"$(notdir $<)\" " )

####################
### 修改编译选项 ###

# if what to see debug in verbose mode
# cmake -DCMAKE_VERBOSE_MAKEFILE=true ../


set(CMAKE_BUILD_TYPE DEBUG)
#set(CMAKE_BUILD_TYPE RELEASE)
set(CMAKE_CXX_FLAGS_DEBUG   "$ENV{CXXFLAGS} -O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 ")

IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
ELSEIF(CMAKE_BUILD_TYPE MATCHES RELEASE)
    add_definitions(-DNP_DEBUG)
ELSE(CMAKE_BUILD_TYPE MATCHES DEBUG)
    message(FATAL_ERROR "Unknown BUILD_TYPE ... ")
ENDIF(CMAKE_BUILD_TYPE MATCHES DEBUG)


###################


set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)


set (INCLUDE_DIRS
        ${PROJECT_SOURCE_DIR}/include/
        ${PROJECT_SOURCE_DIR}/source/
        ${PROJECT_SOURCE_DIR}/deps/xtra_rhelz.x/include/
)

include_directories( ${INCLUDE_DIRS} )

add_subdirectory( source/ )

link_directories(
        /usr/lib64/mysql/
        ${PROJECT_SOURCE_DIR}/deps/xtra_rhelz.x/libs/
        ${PROJECT_SOURCE_DIR}/deps/xtra_rhelz.x/libs/boost/
)

# local temp generated file
exec_program( "export BUILD_VAR=`git log -1 --pretty=%H` && echo 'const char *build_commit_version = \"VCS: Commit:' $BUILD_VAR '\";' > build_version.cpp ")
exec_program( "export BUILD_VAR=`git symbolic-ref HEAD` && echo 'const char *build_commit_branch = \"VCS: Branch:' $BUILD_VAR '\";' >> build_version.cpp ")
exec_program( "export BUILD_VAR=`git log -1 --pretty=%cd` && echo 'const char *build_commit_date = \"VCS: Date:' $BUILD_VAR '\";' >> build_version.cpp ")
exec_program( "export BUILD_VAR=`git log -1 --pretty=\"%an %ae\"` && echo 'const char *build_commit_author = \"VCS: Author:' $BUILD_VAR '\";' >> build_version.cpp ")
exec_program( "export BUILD_VAR=`date +%F` && echo 'const char *build_time = \"Build At:' $BUILD_VAR '\";' >> build_version.cpp ")

add_executable( tzschedule main.cpp setup.cpp build_version.cpp )

# ld iconv ?

set (WHOLE_LIBS Schedule )

set (EXTRA_LIBS ${EXTRA_LIBS} ssl crypto config++)
set (EXTRA_LIBS ${EXTRA_LIBS} pthread)
set (EXTRA_LIBS ${EXTRA_LIBS} boost_system boost_thread boost_chrono boost_regex)



# 某些库需要静态全连接，否则主服务没使用但是so使用，会报符号没有找到
target_link_libraries( tzschedule -lrt -rdynamic -ldl
    -Wl,--whole-archive ${WHOLE_LIBS} -Wl,--no-whole-archive
    ${EXTRA_LIBS}
)

add_subdirectory( source/so-handler/job1 )
add_subdirectory( source/so-handler/job2 )

add_subdirectory( test )
