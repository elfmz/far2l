project(gitgutter)

set(SOURCES
src/gitgutter.cpp
)

add_library (gitgutter MODULE ${SOURCES})

target_link_libraries(gitgutter utils far2l)

target_compile_definitions(gitgutter PRIVATE -DWINPORT_DIRECT -DUNICODE -DFAR_USE_INTERNALS)

target_include_directories(gitgutter PRIVATE .)
target_include_directories(gitgutter PRIVATE ../far2l/far2sdk)
target_include_directories(gitgutter PRIVATE ../WinPort)
target_include_directories(gitgutter PRIVATE ../utils/include)

set_target_properties(gitgutter
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/gitgutter/plug"
        PREFIX ""
        SUFFIX ".far-plug-wide")

# copy resource files
set(CURRENT_TARGET "gitgutter")
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
