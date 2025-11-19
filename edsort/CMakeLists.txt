
project(edsort)

set(SOURCES
src/edsort.cpp
src/fardialog.cpp
src/plugin.cpp
)

add_library (edsort MODULE ${SOURCES})

target_link_libraries(edsort utils far2l)

target_compile_definitions(edsort PRIVATE -DWINPORT_DIRECT -DUNICODE -DFAR_DONT_USE_INTERNALS)

target_include_directories(edsort PRIVATE .)
target_include_directories(edsort PRIVATE ../far2l/far2sdk)
target_include_directories(edsort PRIVATE ../utils/include)
target_include_directories(edsort PRIVATE ../WinPort)

set_target_properties(edsort
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/edsort/plug"
        PREFIX ""
        SUFFIX ".far-plug-wide")

# copy resource files
set(CURRENT_TARGET "edsort")
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
