
project(editorcomp)

set(SOURCES
    src/editorcomp.cpp
        src/Editor.cpp src/Editor.h src/Editors.cpp src/Editors.h src/Registry.cpp src/Registry.h)

add_library (editorcomp MODULE ${SOURCES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS "${CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS} -flat_namespace -undefined suppress")
endif()

target_link_libraries(editorcomp utils far2l)

target_compile_definitions(editorcomp PRIVATE -D_UNICODE -DUNICODE -D__unix__)

target_include_directories(editorcomp PRIVATE .)
target_include_directories(editorcomp PRIVATE ../far2l)
target_include_directories(editorcomp PRIVATE ../far2l/Include)
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
