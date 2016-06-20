include_directories(${MPI_CXX_INCLUDE_PATH})

ADD_DEFINITIONS(-DMULTIVERSO_USE_MPI)

set(MULTIVERSO_SRC actor.cpp communicator.cpp controller.cpp dashboard.cpp multiverso.cpp net.cpp node.cpp server.cpp table.cpp table/array_table.cpp table/matrix_table.cpp table/sparse_matrix_table.cpp table/matrix.cpp timer.cpp  updater/updater.cpp util/configure.cpp io/hdfs_stream.cpp io/io.cpp io/local_stream.cpp util/log.cpp util/net_util.cpp util/quantization_util.cpp worker.cpp zoo.cpp c_api.cpp util/allocator.cpp table_factory.cpp blob.cpp)

add_library(multiverso SHARED ${MULTIVERSO_SRC})
target_link_libraries(multiverso ${MPI_LIBRARY})

install (TARGETS multiverso DESTINATION lib)
if (UNIX)
    install(CODE "execute_process(COMMAND ldconfig)")  # run ldconfig. Otherwise ld.so.cache won't be created.
endif()
