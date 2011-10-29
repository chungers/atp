#
# Referenced inside source files as ifdef constants.

# Configure constants
configure_file(
  constants.h.in
  ${SRC_DIR}/constants.h
)

# Interactive Brokers adapter
add_subdirectory(ib)

# ZMQ
add_subdirectory(zmq)

# ATP R extensions
add_subdirectory(raptor)
