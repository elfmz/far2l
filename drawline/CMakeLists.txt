
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

# copy resource files
set(CURRENT_TARGET "drawline")
file(GLOB_RECURSE RESOURCE_FILES
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/configs
        "${CMAKE_CURRENT_SOURCE_DIR}/configs/*"
)
setup_target_auxiliary_files(${CURRENT_TARGET}
        RESOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/configs
        "${INSTALL_DIR}/Plugins/${CURRENT_TARGET}/"
)
add_dependencies(${CURRENT_TARGET} copy_aux_files_for_${CURRENT_TARGET})
