# raptor
#
message(STATUS "ATP R Extensions =============================================")

# Generate raptor R module MakeVars
configure_file(
  ./src/Makevars.tpl
  ./src/Makevars
)

set(R_PKG "raptor")
set(R_BUNDLE "${R_PKG}_1.0.tar.gz")
add_custom_target(build_raptor
COMMAND R CMD build ${R_PKG}
WORKING_DIRECTORY ${SRC_DIR}
COMMENT "Building R package ${R_PKG}",
)

add_custom_target(install_raptor
COMMAND R CMD INSTALL --no-multiarch ${R_PKG}
WORKING_DIRECTORY ${SRC_DIR}
COMMENT "Install R package ${R_PKG}",
)

add_dependencies(build_raptor
  IBAPIConnector
  zmq_components
  varz_components
  historian_components
  proto_components
)

add_dependencies(install_raptor build_raptor)

