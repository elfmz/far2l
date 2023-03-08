
project(editorcomp)

set(SOURCES
    src/editorcomp.cpp src/editorcomp.h
    src/Editor.cpp src/Editor.h
    src/Editors.cpp src/Editors.h
    src/Settings.cpp src/Settings.h
)

add_library (editorcomp MODULE ${SOURCES})

target_link_libraries(editorcomp utils far2l)

target_compile_definitions(editorcomp PRIVATE -D_UNICODE -DUNICODE -D__unix__)

target_include_directories(editorcomp PRIVATE .)
target_include_directories(editorcomp PRIVATE ../far2l/far2sdk)
target_include_directories(editorcomp PRIVATE ../WinPort)

set_target_properties(editorcomp
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/editorcomp/plug"
        PREFIX ""
        SUFFIX ".far-plug-wide")

add_custom_command(TARGET editorcomp POST_BUILD
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/editorcomp"
)
