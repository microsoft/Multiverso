include_directories(${MULTIVERSO_HEADER_DIR})
include_directories(${THIRD_PARTY_INC})
include_directories(${MPI_CXX_INCLUDE_PATH})

set(MULTIVERSO_SRC net.cpp actor.cpp communicator.cpp controller.cpp multiverso.cpp node.cpp server.cpp table.cpp util/log.cpp worker.cpp zoo.cpp )

link_directories(${THIRD_PARTY_LIB})

#target_link_libraries(multiverso ${MPI_CXX_LIBARIES})
#target_link_libraries(mpi)

add_library(multiverso ${MULTIVERSO_SRC})

#set_property(TARGET multiverso PROPERTY CXX_STANDARD 11)

