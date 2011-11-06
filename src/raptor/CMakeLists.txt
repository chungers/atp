# raptor
#
message(STATUS "ATP R Extensions ============================================")

# Generate raptor R module MakeVars
configure_file(
  ./src/Makevars.tpl
  ./src/Makevars
)


set(R_PKG "raptor")
set(R_BUNDLE "${R_PKG}_1.0.tar.gz")
add_custom_target(bundle_raptor
COMMAND R CMD BUILD ${R_PKG}
WORKING_DIRECTORY ${SRC_DIR}
COMMENT "Building R package ${R_PKG}",
)
add_custom_command(
TARGET bundle_raptor
PRE_BUILD
COMMAND rm -f ${R_PKG}_*.tar.gz
COMMAND echo "Removed bundle."
WORKING_DIRECTORY ${SRC_DIR}
COMMENT "Clean up ${R_PKG}",
)

add_custom_target(compile_raptor
COMMAND R CMD INSTALL --no-inst --build ${R_PKG}
DEPENDS IBAPIConnector
WORKING_DIRECTORY ${SRC_DIR}
COMMENT "Compile R package ${R_PKG}",
)

add_custom_target(install_raptor
COMMAND R CMD INSTALL ${R_PKG}
WORKING_DIRECTORY ${SRC_DIR}
COMMENT "Install R package ${R_PKG}",
)

#add_dependencies(bundle_raptor raptor)

add_dependencies(bundle_raptor IBAPIConnector zmq_components)
add_dependencies(compile_raptor bundle_raptor)
add_dependencies(install_raptor compile_raptor)

