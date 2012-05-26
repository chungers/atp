#
# Referenced inside source files as ifdef constants.

# Configure constants
configure_file(
  constants.h.in
  ${SRC_DIR}/constants.h
)


# Boost support for gzip
add_subdirectory(boost/iostreams)

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

# Historian
add_subdirectory(historian)

# Actual mains / programs
add_subdirectory(main)
