
project(ImageViewer)

set(SOURCES
src/Plugin.cpp 
src/Settings.cpp 
src/Image.cpp 
src/ImageView.cpp 
src/ImageAtFull.cpp
src/ImageAtQV.cpp
src/ToolExec.cpp
)

add_library (ImageViewer MODULE ${SOURCES})

target_link_libraries(ImageViewer utils far2l)

target_compile_options(ImageViewer PRIVATE -ffast-math)
target_compile_definitions(ImageViewer PRIVATE -DUNICODE )

target_include_directories(ImageViewer PRIVATE .)
target_include_directories(ImageViewer PRIVATE ../WinPort)
target_include_directories(ImageViewer PRIVATE ../far2l/far2sdk)

set_target_properties(ImageViewer
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/ImageViewer/plug"
        PREFIX ""
        SUFFIX ".far-plug-wide")


# copy resource files
set(CURRENT_TARGET "ImageViewer")
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
