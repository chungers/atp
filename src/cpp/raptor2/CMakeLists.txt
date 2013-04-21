# raptor
#
message(STATUS "ATP R Extensions =============================================")

add_definitions('-fPIC')

# Generate raptor R module MakeVars
configure_file(
  ./src/Makevars.tpl
  ./src/Makevars
)

set(R_PKG "raptor2")
set(R_BUNDLE "${R_PKG}_1.0.tar.gz")
add_custom_target(build_raptor2
COMMAND R CMD build ${R_PKG}
WORKING_DIRECTORY ${SRC_DIR}
COMMENT "Building R package ${R_PKG}",
)

add_custom_target(install_raptor2
COMMAND R CMD INSTALL --library=${BUILD_DIR}/R --no-multiarch ${R_PKG}
WORKING_DIRECTORY ${SRC_DIR}
COMMENT "Install R package ${R_PKG}",
)

add_dependencies(build_raptor2
  atp_zmq
  atp_varz
  atp_historian
  atp_proto
)

add_dependencies(install_raptor2 build_raptor2)
