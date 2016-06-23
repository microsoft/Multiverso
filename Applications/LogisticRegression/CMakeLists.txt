cmake_minimum_required(VERSION 2.8)

PROJECT(LOGREG)

set(MULTIVERSO_DIR ~/multiverso)

find_package(MPI REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")

if(LOGLEVEL STREQUAL "DEBUG")
  add_definitions(-DLOGLEVEL_DEBUG)
elseif(LOGLEVEL STREQUAL "FATAL")
  add_definitions(-DLOGLEVEL_FATAL)
elseif(LOGLEVEL STREQUAL "ERROR")
  add_definitions(-DLOGLEVEL_ERROR)
else()
  add_definitions(-DLOGLEVEL_INFO)
endif()

set(MULTIVERSO_INC ${MULTIVERSO_DIR}/include)
set(MULTIVERSO_LIB ${MULTIVERSO_DIR}/build/src)
set(MULTIVERSO_SRC ${MULTIVERSO_DIR}/src)

include_directories(${MULTIVERSO_INC})
include_directories(${PROJECT_SOURCE_DIR}/src)

link_directories(${MULTIVERSO_LIB})

set(SRCDIR ${PROJECT_SOURCE_DIR}/src)
aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC_ROOT)
aux_source_directory(${SRCDIR}/model SRC_MODEL)
aux_source_directory(${SRCDIR}/objective SRC_OBJECTIVE)
aux_source_directory(${SRCDIR}/regular SRC_REGULAR)
aux_source_directory(${SRCDIR}/updater SRC_UPDATER)
aux_source_directory(${SRCDIR}/util SRC_UTIL)
set(SRC ${SRC_MODEL} ${SRC_OBJECTIVE} ${SRC_REGULAR} ${SRC_UPDATER} ${SRC_UTIL} ${MULTIVERSO_SRC}/table/array_table.cpp ${SRC_ROOT})
message(${SRC})

add_executable(LogReg ${SRC})

target_link_libraries(LogReg multiverso ${MPI_CXX_LIBRARIES})
