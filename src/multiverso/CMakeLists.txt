SET(THIRD_PARTY ${MULTIVERSO_BINARY_DIR}/third_party)
SET(THIRD_PARTY_INC ${THIRD_PARTY}/include)
SET(THIRD_PARTY_LIB ${THIRD_PARTY}/lib)
SET(HEADERS_DIR ${MULTIVERSO_BINARY_DIR}/include/multiverso)
SET(EXECUTABLE_OUTPUT_PATH ${MULTIVERSO_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${MULTIVERSO_BINARY_DIR}/lib)

INCLUDE_DIRECTORIES(${HEADERS_DIR})
INCLUDE_DIRECTORIES(${THIRD_PARTY_INC})

Message(${MULTIVERSO_BINARY_DIR})

SET(MULTIVERSO_SRC multiverso.cpp aggregator.cpp  endpoint_list.cpp   stop_watch.cpp zmq_util.cpp barrier.cpp lock.cpp parameter_loader.cpp  table.cpp communicator.cpp  log.cpp  row.cpp  table_iter.cpp data_block.cpp    mpi_util.cpp  row_iter.cpp trainer.cpp delta_pool.cpp  msg_pack.cpp  server.cpp vector_clock.cpp
)

LINK_DIRECTORIES(${THIRD_PARTY_LIB})

ADD_LIBRARY(multiverso ${MULTIVERSO_SRC})

TARGET_LINK_LIBRARIES(multiverso mpich mpl zmq)

set_property(TARGET multiverso PROPERTY CXX_STANDARD 11)