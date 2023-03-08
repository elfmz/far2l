
project(editcase)

set(SOURCES
src/Compare.cpp 
)

add_library (compare MODULE ${SOURCES})

target_link_libraries(compare utils far2l)

target_compile_definitions(compare PRIVATE -DWINPORT_DIRECT -DUNICODE -DFAR_DONT_USE_INTERNALS)

target_include_directories(compare PRIVATE .)
target_include_directories(compare PRIVATE ../WinPort)
target_include_directories(compare PRIVATE ../far2l/far2sdk)

set_target_properties(compare 
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/compare/plug"
        PREFIX ""
        SUFFIX ".far-plug-wide")


add_custom_command(TARGET compare POST_BUILD
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/compare"
)
