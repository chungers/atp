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



# Interactive Brokers adapter
add_subdirectory(ib)

# ZMQ
add_subdirectory(zmq)

# ATP R extensions
add_subdirectory(raptor)
