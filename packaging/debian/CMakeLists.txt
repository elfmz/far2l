cmake_minimum_required (VERSION 3.0.2)

find_package(Git QUIET)
execute_process(COMMAND "${GIT_EXECUTABLE}" status
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE GIT_RV
        )

if(GIT_RV EQUAL "0")
    set(DEB_CONTROLS ${CMAKE_BINARY_DIR}/copyright)
    message( STATUS "Creating debian/copyright: ${CMAKE_BINARY_DIR}/copyright")
else()
    message( WARNING "${ColorRed}Skipping copyright generation cause git is unavailable${ColorNormal}")
    return()
endif()

add_custom_target(debian-package-controls ALL DEPENDS ${DEB_CONTROLS})

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/copyright
    COMMAND ./generate.pl ${CMAKE_BINARY_DIR}/copyright
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/copyright"
    COMMENT ${CMAKE_BINARY_DIR}/copyright
)

include(GNUInstallDirs)
install(FILES "${CMAKE_BINARY_DIR}/copyright" DESTINATION ${CMAKE_INSTALL_DOCDIR} COMPONENT base)
