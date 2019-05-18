project(NetRocks)

set(SOURCES
src/Erroring.cpp
src/Globals.cpp
src/SitesConfig.cpp
src/NetRocks.cpp
src/PluginImpl.cpp
src/PluginPanelItems.cpp
src/PooledStrings.cpp
src/BackgroundTasks.cpp
src/Protocol/Protocol.cpp
src/Protocol/ProtocolFile.cpp
src/Protocol/ProtocolSFTP.cpp
src/Protocol/SplitPathSpecification.cpp
src/Host/IPC.cpp
src/Host/HostLocal.cpp
src/Host/HostRemote.cpp
src/Host/HostRemoteBroker.cpp
src/UI/DialogUtils.cpp
src/UI/Settings/SiteConnectionEditor.cpp
src/UI/Settings/ConfigureProtocolSFTP.cpp
src/UI/Settings/ConfigureProtocolFile.cpp
src/UI/Activities/Confirm.cpp
src/UI/Activities/ConfirmXfer.cpp
src/UI/Activities/ConfirmOverwrite.cpp
src/UI/Activities/SimpleOperationProgress.cpp
src/UI/Activities/ComplexOperationProgress.cpp
src/UI/Activities/AbortOperationRequest.cpp
src/UI/Activities/InteractiveLogin.cpp
src/UI/Activities/ConfirmNewServerIdentity.cpp
src/UI/Activities/WhatOnError.cpp
src/UI/Activities/BackgroundTasksUI.cpp
src/Op/Utils/ProgressStateUpdate.cpp
src/Op/Utils/Enumer.cpp
src/Op/Utils/IOBuffer.cpp
src/Op/OpBase.cpp
src/Op/OpConnect.cpp
src/Op/OpGetMode.cpp
src/Op/OpEnumDirectory.cpp
src/Op/OpXfer.cpp
src/Op/OpRemove.cpp
src/Op/OpMakeDirectory.cpp
)



set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FAR_HAS_NAMELESS_UNIONS")

add_library (NetRocks MODULE ${SOURCES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS "${CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS} -flat_namespace -undefined suppress")
endif()

FIND_LIBRARY(LIBSSH_LIBRARY NAMES ssh)

target_link_libraries(NetRocks utils ssh)

target_include_directories(NetRocks PRIVATE src)
target_include_directories(NetRocks PRIVATE ../WinPort)
target_include_directories(NetRocks PRIVATE ../far2l)
target_include_directories(NetRocks PRIVATE ../far2l/Include)

target_compile_definitions(NetRocks PRIVATE)


set_target_properties(NetRocks
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/NetRocks/plug"
    PREFIX ""
    SUFFIX ".far-plug-wide")

add_custom_command(TARGET NetRocks POST_BUILD
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/NetRocks")
