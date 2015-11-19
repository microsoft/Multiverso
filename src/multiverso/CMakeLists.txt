INCLUDE_DIRECTORIES(${HEADERS_DIR})
INCLUDE_DIRECTORIES(${THIRD_PARTY_INC})

MESSAGE(STATUS "multiver  ${THIRD_PARTY_INC}")

SET(MULTIVERSO_SRC multiverso.cpp aggregator.cpp  endpoint_list.cpp   stop_watch.cpp zmq_util.cpp barrier.cpp lock.cpp parameter_loader.cpp  table.cpp communicator.cpp  log.cpp  row.cpp  table_iter.cpp data_block.cpp    mpi_util.cpp  row_iter.cpp trainer.cpp delta_pool.cpp  msg_pack.cpp  server.cpp vector_clock.cpp
)

LINK_DIRECTORIES(${THIRD_PARTY_LIB})

ADD_LIBRARY(multiverso ${MULTIVERSO_SRC})

TARGET_LINK_LIBRARIES(multiverso mpich mpl zmq pthread)

SET_PROPERTY(TARGET multiverso PROPERTY CXX_STANDARD 11)
