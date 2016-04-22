include_directories(${MULTIVERSO_HEADER_DIR})
include_directories(${THIRD_PARTY_INC})
include_directories(${MPI_CXX_INCLUDE_PATH})

ADD_DEFINITIONS(-DMULTIVERSO_USE_MPI)

set(MULTIVERSO_SRC actor.cpp communicator.cpp controller.cpp dashboard.cpp multiverso.cpp net.cpp node.cpp server.cpp table.cpp table/array_table.cpp table/matrix_table.cpp timer.cpp  updater/updater.cpp util/configure.cpp io/hdfs_stream.cpp io/io.cpp io/local_stream.cpp util/log.cpp util/net_util.cpp util/quantization_util.cpp worker.cpp zoo.cpp)

link_directories(${THIRD_PARTY_LIB})
if(USE_HDFS)
	link_directories(${HDFS_LIB})
	LINK_DIRECTORIES(${JVM_LIB})
endif(USE_HDFS)

#target_link_libraries(multiverso ${MPI_CXX_LIBARIES})
#target_link_libraries(mpi)

add_library(multiverso SHARED ${MULTIVERSO_SRC})

#set_property(TARGET multiverso PROPERTY CXX_STANDARD 11)

