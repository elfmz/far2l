
project(align)

set(SOURCES
src/Align.cpp 
)

add_library (align MODULE ${SOURCES})

target_link_libraries(align utils far2l)

target_compile_definitions(align PRIVATE -DWINPORT_DIRECT -DUNICODE -DFAR_DONT_USE_INTERNALS)

target_include_directories(align PRIVATE .)
target_include_directories(align PRIVATE ../far2l/far2sdk)
target_include_directories(align PRIVATE ../WinPort)

set_target_properties(align 
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/align/plug"
        PREFIX ""
        SUFFIX ".far-plug-wide")


add_custom_command(TARGET align POST_BUILD
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/align"
)
