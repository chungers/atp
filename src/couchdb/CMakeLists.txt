# CouchDB
#

message(STATUS "CouchDB ======================================================")

add_definitions('-fPIC')

##########################################
# CouchDB client library
set(atp_couchdb_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(atp_couchdb_srcs
  couchdb.cpp
)
set(atp_couchdb_libs
  boost_system
  boost_thread
  curl
  json_spirit
  gflags
  glog
)
cpp_library(atp_couchdb)

