
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

# copy resource files
set(CURRENT_TARGET "autowrap")
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
