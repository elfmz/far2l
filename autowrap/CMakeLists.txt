
project(autowrap)

set(SOURCES
src/AutoWrap.cpp 
)

add_library (autowrap MODULE ${SOURCES})

target_link_libraries(autowrap utils far2l)

target_compile_definitions(autowrap PRIVATE -DWINPORT_DIRECT -DUNICODE -DFAR_DONT_USE_INTERNALS)

target_include_directories(autowrap PRIVATE .)
target_include_directories(autowrap PRIVATE ../far2l/far2sdk)
target_include_directories(autowrap PRIVATE ../WinPort)

set_target_properties(autowrap
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/autowrap/plug"
        PREFIX ""
        SUFFIX ".far-plug-wide")


add_custom_command(TARGET autowrap POST_BUILD
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/autowrap"
)
