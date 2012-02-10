

proto_library(common_proto common.proto ${SRC_DIR}/historian)
proto_library(historian_proto historian.proto ${SRC_DIR}/historian)

add_dependencies(historian_proto common_proto)
