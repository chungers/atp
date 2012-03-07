

proto_library(common_proto common.proto ${SRC_DIR})
proto_library(ib_proto ib.proto ${SRC_DIR})
proto_library(historian_proto historian.proto ${SRC_DIR})

add_dependencies(ib_proto common_proto)
add_dependencies(historian_proto ib_proto)
