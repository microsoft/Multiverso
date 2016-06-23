cmake_minimum_required(VERSION 2.8)

PROJECT(WORDEMBEDDING)

set(MULTIVERSO_DIR ~/multiverso)

find_package(MPI REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -Wno-sign-compare -fno-omit-frame-pointer -fopenmp")

set(MULTIVERSO_INC ${MULTIVERSO_DIR}/include)
set(MULTIVERSO_LIB ${MULTIVERSO_DIR}/src)
set(MULTIVERSO_SRC ${MULTIVERSO_DIR}/src)

include_directories(${MULTIVERSO_INC})
include_directories(${PROJECT_SOURCE_DIR}/src)

link_directories(${MULTIVERSO_LIB})

set(SRCDIR ${PROJECT_SOURCE_DIR}/src)
aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC_ROOT)

set(SRC ${MULTIVERSO_SRC} ${SRC_ROOT})
message(${SRC})

add_executable(WORDEMBEDDING ${SRC})

target_link_libraries(WORDEMBEDDING multiverso ${MPI_CXX_LIBRARIES})
