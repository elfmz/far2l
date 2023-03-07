
project(drawline)

set(SOURCES
src/DrawLine.cpp 
)

add_library (drawline MODULE ${SOURCES})

target_link_libraries(drawline utils far2l)

target_compile_definitions(drawline PRIVATE -DWINPORT_DIRECT -DUNICODE -DFAR_DONT_USE_INTERNALS)

target_include_directories(drawline PRIVATE .)
target_include_directories(drawline PRIVATE ../WinPort)
target_include_directories(drawline PRIVATE ../far2l/far2sdk)

set_target_properties(drawline 
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/drawline/plug"
        PREFIX ""
        SUFFIX ".far-plug-wide")


add_custom_command(TARGET drawline POST_BUILD
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/drawline"
)
