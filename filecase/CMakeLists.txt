
project(filecase)

set(SOURCES
src/FileCase.cpp 
)

add_library (filecase MODULE ${SOURCES})

target_link_libraries(filecase utils far2l)

target_compile_definitions(filecase PRIVATE -DWINPORT_DIRECT -DUNICODE -DFAR_DONT_USE_INTERNALS)

target_include_directories(filecase PRIVATE .)
target_include_directories(filecase PRIVATE ../far2l/far2sdk)
target_include_directories(filecase PRIVATE ../WinPort)

set_target_properties(filecase 
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/filecase/plug"
        PREFIX ""
        SUFFIX ".far-plug-wide")


add_custom_command(TARGET filecase POST_BUILD
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/filecase"
)
