project(NetRoks)

set(SOURCES
src/Erroring.cpp
src/Globals.cpp
src/IPC.cpp
src/NetRoks.cpp
src/PluginImpl.cpp
src/PooledStrings.cpp
src/ProtocolSFTP.cpp
src/SiteConnection.cpp
src/SiteConnectionSlave.cpp

)

add_library (NetRoks MODULE ${SOURCES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS "${CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS} -flat_namespace -undefined suppress")
endif()

target_link_libraries(NetRoks utils FARStdlib)

target_include_directories(NetRoks PRIVATE src)
target_include_directories(NetRoks PRIVATE ../FARStdlib/include)
target_include_directories(NetRoks PRIVATE ../WinPort)
target_include_directories(NetRoks PRIVATE ../far2l)
target_include_directories(NetRoks PRIVATE ../far2l/Include)

target_compile_definitions(NetRoks PRIVATE)

set_target_properties(NetRoks
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/NetRoks/plug"
    PREFIX ""
    SUFFIX ".far-plug-mb")

add_custom_command(TARGET NetRoks POST_BUILD
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/NetRoks")
