project(ImageViewer)

set(SOURCES
    ImageViewer.cpp
)

add_library (ImageViewer MODULE ${SOURCES})

target_link_libraries(ImageViewer utils far2l)

target_compile_definitions(ImageViewer PRIVATE -DWINPORT_DIRECT -DUNICODE)

target_include_directories(${PROJECT_NAME} PRIVATE ../far2l/far2sdk)
target_include_directories(${PROJECT_NAME} PRIVATE ../WinPort)

set_target_properties(ImageViewer
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/ImageViewer"
        PREFIX ""
        SUFFIX ".far-plug-wide")