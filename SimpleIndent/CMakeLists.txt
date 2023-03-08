
project(SimpleIndent)

set(SOURCES
src/SimpleIndent.cpp 
)

add_library (SimpleIndent MODULE ${SOURCES})

target_link_libraries(SimpleIndent utils far2l)

target_compile_definitions(SimpleIndent PRIVATE -DWINPORT_DIRECT -DUNICODE -DFAR_DONT_USE_INTERNALS)

target_include_directories(SimpleIndent PRIVATE .)
target_include_directories(SimpleIndent PRIVATE ../WinPort)
target_include_directories(SimpleIndent PRIVATE ../far2l/far2sdk)

set_target_properties(SimpleIndent
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/SimpleIndent/plug"
        PREFIX ""
        SUFFIX ".far-plug-wide")

