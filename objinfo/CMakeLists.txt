
project(objinfo)

set(SOURCES
    src/objinfo.cpp
    src/Globals.cpp
    src/PluginImpl.cpp
    src/Dumper.cpp
    src/Storage.cpp

)

add_library (objinfo MODULE ${SOURCES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS "${CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS} -flat_namespace -undefined suppress")
endif()

target_link_libraries(objinfo utils FARStdlib)

target_include_directories(objinfo PRIVATE src)
target_include_directories(objinfo PRIVATE ../FARStdlib/include)
target_include_directories(objinfo PRIVATE ../WinPort)
target_include_directories(objinfo PRIVATE ../far2l)
target_include_directories(objinfo PRIVATE ../far2l/Include)

target_compile_definitions(objinfo PRIVATE)

set_target_properties(objinfo
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/objinfo/plug"
    PREFIX ""
    SUFFIX ".far-plug-mb")

add_custom_command(TARGET objinfo POST_BUILD
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/objinfo")
