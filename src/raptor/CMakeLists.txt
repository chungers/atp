# raptor
#
message(STATUS "ATP R Extensions ============================================")

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
WORKING_DIRECTORY ${SRC_DIR}
COMMENT "Compile R package ${R_PKG}",
)

add_custom_target(raptor
COMMAND R CMD INSTALL ${R_PKG}
WORKING_DIRECTORY ${SRC_DIR}
COMMENT "Install R package ${R_PKG}",
)

