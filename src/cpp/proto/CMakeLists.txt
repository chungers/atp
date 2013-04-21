
add_definitions('-fPIC')

proto_library(common_proto common.proto ${SRC_DIR})
proto_library(ib_proto ib.proto ${SRC_DIR})
proto_library(indicator_proto indicator.proto ${SRC_DIR})
proto_library(platform_proto platform.proto ${SRC_DIR})
proto_library(historian_proto historian.proto ${SRC_DIR})

add_dependencies(ib_proto common_proto)
add_dependencies(platform_proto common_proto)
add_dependencies(platform_proto indicator_proto)
add_dependencies(historian_proto ib_proto)


##########################################
# atp_proto
set(atp_proto_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(atp_proto_srcs
  ostream.cpp
  template_instantiations.cpp
)
set(atp_proto_libs
  atp_common
  historian_proto
  ib_proto
  common_proto
  indicator_proto
  platform_proto
  protobuf
  protobuf-lite
)
cpp_library(atp_proto)
