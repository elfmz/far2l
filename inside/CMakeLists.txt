project(inside)

set(SOURCES
src/inside.cpp
src/Globals.cpp
src/Commands.cpp
src/ItemList.cpp
src/PluginImpl.cpp
src/Storage.cpp

src/plain/PluginImplPlain.cpp

)

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(SOURCES
        ${SOURCES}
        src/elf/Dumper.cpp
        src/elf/PluginImplELF.cpp
    )
endif()




add_library (inside MODULE ${SOURCES})

target_link_libraries(inside utils far2l)

target_include_directories(inside PRIVATE src)
target_include_directories(inside PRIVATE src/elf)
target_include_directories(inside PRIVATE src/plain)
target_include_directories(inside PRIVATE ../WinPort)
target_include_directories(inside PRIVATE ../far2l/far2sdk)

target_compile_definitions(inside PRIVATE)

set_target_properties(inside
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/inside/plug"
        PREFIX ""
        SUFFIX ".far-plug-mb")

# copy resource files
set(CURRENT_TARGET "inside")
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
