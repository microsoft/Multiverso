include_directories(${MULTIVERSO_HEADER_DIR})
include_directories(${THIRD_PARTY_INC})

set(MULTIVERSO_SRC actor.cpp communicator.cpp controller.cpp multiverso.cpp node.cpp server.cpp table.cpp util/log.cpp worker.cpp zoo.cpp)

link_directories(${THIRD_PARTY_LIB})

add_library(libmultiverso ${MULTIVERSO_SRC})

#target_link_libraries(mpich)

