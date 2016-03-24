include_directories(${MULTIVERSO_HEADER_DIR})
include_directories(${THIRD_PARTY_INC})
include_directories(${MPI_CXX_INCLUDE_PATH})

ADD_DEFINITIONS(-DMULTIVERSO_USE_MPI)
set(MULTIVERSO_SRC net.cpp actor.cpp communicator.cpp controller.cpp multiverso.cpp node.cpp server.cpp table.cpp util/log.cpp worker.cpp zoo.cpp util/io.cpp util/hdfs_file_sys.cpp util/local_file_sys.cpp )

link_directories(${THIRD_PARTY_LIB})
if(USE_HDFS)
	link_directories(${HDFS_LIB})
	LINK_DIRECTORIES(${JVM_LIB})
endif(USE_HDFS)

#target_link_libraries(multiverso ${MPI_CXX_LIBARIES})
#target_link_libraries(mpi)

add_library(multiverso ${MULTIVERSO_SRC})

#set_property(TARGET multiverso PROPERTY CXX_STANDARD 11)

