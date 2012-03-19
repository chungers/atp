

message(STATUS "Raptor src ===================================================")

find_package(Protobuf REQUIRED)

protobuf_generate_cpp(RP_PROTO_SRCS RP_PROTO_HDRS
  common.proto ib.proto historian.proto)

include_directories(${PROTOBUF_INCLUDE_DIRS} ${SRC_DIR}/raptor/src)
add_library(raptor_protos ${RP_PROTO_SRCS} ${RP_PROTO_HDRS})
