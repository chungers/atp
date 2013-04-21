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

# Common stuff
add_subdirectory(common)

# Interactive Brokers adapter
add_subdirectory(ib)

# VARZ
add_subdirectory(varz)

# ZMQ
add_subdirectory(zmq)

# CouchDB
add_subdirectory(couchdb)

# Indicator
# - technical indicators and other time series operations
add_subdirectory(indicator)

# Platform
# - composition of lower-level layers like zmq, ib, proto, etc.
add_subdirectory(platform)

# Services
# - Domain-specific layer built on Platform components.
add_subdirectory(service)

# Historian
add_subdirectory(historian)

# Actual mains / programs
add_subdirectory(main)
