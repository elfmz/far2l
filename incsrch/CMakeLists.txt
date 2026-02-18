
project(incsrch)

set(SOURCES
clip.c
control.c
incsrch.c
loc.c
search.c
util.c
config.cpp
)

add_library (incsrch MODULE ${SOURCES})

target_link_libraries(incsrch utils far2l)

target_compile_definitions(incsrch PRIVATE -DWINPORT_DIRECT -DUNICODE -DFAR_DONT_USE_INTERNALS)
set_property(TARGET incsrch PROPERTY C_STANDARD 11)

target_include_directories(incsrch PRIVATE ../far2l/far2sdk)
target_include_directories(incsrch PRIVATE ../WinPort)
target_include_directories(incsrch PRIVATE .)

set_target_properties(incsrch
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/incsrch/plug"
        PREFIX ""
        SUFFIX ".far-plug-wide")


add_custom_command(TARGET incsrch POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/ENG/isrceng.lng "${INSTALL_DIR}/Plugins/incsrch/plug"
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/UTF8/isrcrus.lng "${INSTALL_DIR}/Plugins/incsrch/plug"
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/UTF8/isrcbel.lng "${INSTALL_DIR}/Plugins/incsrch/plug"
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/ENG/incsrche.hlf "${INSTALL_DIR}/Plugins/incsrch/plug"
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/UTF8/incsrchr.hlf "${INSTALL_DIR}/Plugins/incsrch/plug"
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/key_macros.ini "${INSTALL_DIR}/Plugins/incsrch/plug"
)
