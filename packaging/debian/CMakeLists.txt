cmake_minimum_required (VERSION 3.2.2)

set(DEB_CONTROLS
    postinst
    prerm
)

set(BUILD_OPTS_FILE ${CMAKE_CURRENT_BINARY_DIR}/build.conf.m4)

message( STATUS "Generating DEB-package build dependencies" )

IF(NOT EXISTS ${BUILD_OPTS_FILE})
    MESSAGE( FATAL_ERROR "${BUILD_OPTS_FILE} not found" )
ENDIF(NOT EXISTS ${BUILD_OPTS_FILE})

add_custom_target(debian-package-controls ALL DEPENDS ${DEB_CONTROLS})

add_custom_command(
    OUTPUT postinst
    COMMAND ${M4} ${CMAKE_CURRENT_SOURCE_DIR}/postinst.m4 > postinst
    DEPENDS ${BUILD_OPTS_FILE} postinst.m4
    COMMENT postinst
)

add_custom_command(
    OUTPUT prerm
    COMMAND ${M4} ${CMAKE_CURRENT_SOURCE_DIR}/prerm.m4 > prerm
    DEPENDS ${BUILD_OPTS_FILE} prerm.m4
    COMMENT prerm
)
