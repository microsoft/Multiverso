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

set(MULTIVERSO_DIR ${PROJECT_SOURCE_DIR})
ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(Test)
ADD_SUBDIRECTORY(Applications/WordEmbedding)
ADD_SUBDIRECTORY(Applications/LogisticRegression)

install (DIRECTORY ${PROJECT_SOURCE_DIR}/include/multiverso DESTINATION include)


# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
