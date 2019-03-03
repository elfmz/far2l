project(NetRocks)

set(SOURCES
src/Erroring.cpp
src/Globals.cpp
src/IPC.cpp
src/NetRocks.cpp
src/PluginImpl.cpp
src/PooledStrings.cpp
src/ProtocolSFTP.cpp
src/SiteConnection.cpp
src/SiteConnectionSlave.cpp

)

add_library (NetRocks MODULE ${SOURCES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS "${CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS} -flat_namespace -undefined suppress")
endif()

FIND_LIBRARY(LIBSSH_LIBRARY NAMES ssh)

target_link_libraries(NetRocks utils FARStdlib ssh)

target_include_directories(NetRocks PRIVATE src)
target_include_directories(NetRocks PRIVATE ../FARStdlib/include)
target_include_directories(NetRocks PRIVATE ../WinPort)
target_include_directories(NetRocks PRIVATE ../far2l)
target_include_directories(NetRocks PRIVATE ../far2l/Include)

target_compile_definitions(NetRocks PRIVATE)


set_target_properties(NetRocks
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/NetRocks/plug"
    PREFIX ""
    SUFFIX ".far-plug-mb")

add_custom_command(TARGET NetRocks POST_BUILD
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/NetRocks")
