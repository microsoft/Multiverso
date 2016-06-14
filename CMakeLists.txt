cmake_minimum_required(VERSION 2.8)

PROJECT(MULTIVERSO)

OPTION(USE_HDFS "won't use hdfs on default, set ON to enable" OFF)
OPTION(TEST "Build all tests." ON)

find_package(MPI REQUIRED)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")

if(USE_HDFS)
    ADD_DEFINITIONS(-DMULTIVERSO_USE_HDFS)
    # TODO: I think these paths should not be hard coded
    SET(JVM_LIB /usr/local/java/default/jre/lib/amd64/server)
    SET(HDFS_LIB /usr/local/hadoop/lib/native)
    LINK_DIRECTORIES(${HDFS_LIB})
    LINK_DIRECTORIES(${JVM_LIB})
endif(USE_HDFS)

include_directories(${PROJECT_SOURCE_DIR}/include)

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(Test)

# TODO: more header files should be installed. Only c_api.h is installed so far
install (FILES ${PROJECT_SOURCE_DIR}/include/multiverso/c_api.h DESTINATION include/multiverso)
