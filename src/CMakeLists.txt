#
# Referenced inside source files as ifdef constants.

# Configure constants
configure_file(
  constants.h.in
  ${SRC_DIR}/constants.h
)


##########################################
# Firehose - gateway to zmq bridge
set(firehose_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(firehose_srcs
  firehose_main.cpp
)
set(firehose_libs
  IBAPIConnector
  boost_system
  gflags
  glog
)
cpp_executable(firehose)


##########################################
# Marketdata subscriber
set(marketdata_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(marketdata_srcs
  marketdata_main.cpp
)
set(marketdata_libs
  IBAPIConnector
  boost_system
  gflags
  glog
)
cpp_executable(marketdata)


# Protobuffers
add_subdirectory(proto)

# Interactive Brokers adapter
add_subdirectory(ib)

# VARZ
add_subdirectory(varz)

# ZMQ
add_subdirectory(zmq)

# ATP R extensions
add_subdirectory(raptor)
