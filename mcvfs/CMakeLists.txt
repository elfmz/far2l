
project(mcvfs)

set(SOURCES
    src/mcvfs.cpp
    src/Globals.cpp
    src/PluginImpl.cpp

)

add_library (mcvfs MODULE ${SOURCES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS "${CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS} -flat_namespace -undefined suppress")
endif()

target_link_libraries(mcvfs utils FARStdlib)

target_include_directories(mcvfs PRIVATE src)
target_include_directories(mcvfs PRIVATE ../FARStdlib/include)
target_include_directories(mcvfs PRIVATE ../WinPort)
target_include_directories(mcvfs PRIVATE ../far2l)
target_include_directories(mcvfs PRIVATE ../far2l/Include)

target_compile_definitions(mcvfs PRIVATE)

set_target_properties(mcvfs
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/mcvfs/plug"
    PREFIX ""
    SUFFIX ".far-plug-mb")

add_custom_command(TARGET mcvfs POST_BUILD
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/mcvfs")
