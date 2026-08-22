
project(jumpword)

set(SOURCES
src/JumpWord.cpp 
)

add_library (jumpword MODULE ${SOURCES})

target_link_libraries(jumpword)

target_compile_definitions(jumpword PRIVATE -DWINPORT_DIRECT -DUNICODE -DFAR_DONT_USE_INTERNALS)

target_include_directories(jumpword PRIVATE .)
target_include_directories(jumpword PRIVATE ../far2l/far2sdk)
target_include_directories(jumpword PRIVATE ../WinPort)

set_target_properties(jumpword 
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/jumpword/plug"
    PREFIX ""
    SUFFIX ".far-plug-wide")

set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
  $<$<CONFIG:Debug>:_DEBUG>)

add_custom_command(TARGET jumpword POST_BUILD
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/jumpword"
)
