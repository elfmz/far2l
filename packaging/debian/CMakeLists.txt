cmake_minimum_required (VERSION 3.0.2)

set(DEB_CONTROLS
    postinst
    prerm
)

execute_process(
    COMMAND sh -c "cd ${CMAKE_CURRENT_SOURCE_DIR} && git status"
    RESULT_VARIABLE GIT_RV
)

if(GIT_RV EQUAL "0")
    set(DEB_CONTROLS
        ${DEB_CONTROLS}
        ${CMAKE_BINARY_DIR}/copyright
    )
    message( STATUS "Creating debian/copyright: ${CMAKE_BINARY_DIR}/copyright")
else()
    message( WARNING "${ColorRed}Skipping copyright generation cause git is unavailable${ColorNormal}")
endif()

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

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/copyright
    COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/copyright && ./generate.pl ${CMAKE_BINARY_DIR}/copyright
    COMMENT ${CMAKE_BINARY_DIR}/copyright
)
