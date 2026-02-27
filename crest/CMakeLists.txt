
project(crest)

set(SOURCES
crest.c
config.cpp
)

add_library (crest MODULE ${SOURCES})

target_link_libraries(crest utils far2l)

target_compile_definitions(crest PRIVATE -DWINPORT_DIRECT -DUNICODE -DFAR_DONT_USE_INTERNALS)
set_property(TARGET crest PROPERTY C_STANDARD 11)

target_include_directories(crest PRIVATE ../far2l/far2sdk)
target_include_directories(crest PRIVATE ../WinPort)
target_include_directories(crest PRIVATE .)

set_target_properties(crest
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/crest/plug"
        PREFIX ""
        SUFFIX ".far-plug-wide")


add_custom_command(TARGET crest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/cresteng.lng "${INSTALL_DIR}/Plugins/crest/plug"
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/cresteng.hlf "${INSTALL_DIR}/Plugins/crest/plug"
)
