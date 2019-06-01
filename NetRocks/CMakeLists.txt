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
src/Host/IPC.cpp
src/Host/HostLocal.cpp
src/Host/HostRemote.cpp
src/Host/HostRemoteBroker.cpp
src/UI/DialogUtils.cpp
src/UI/Settings/SiteConnectionEditor.cpp
src/UI/Settings/ConfigureProtocolFile.cpp
src/UI/Settings/ConfigureProtocolSFTP.cpp
src/UI/Settings/ConfigureProtocolSMB.cpp
src/UI/Settings/ConfigureProtocolNFS.cpp
src/UI/Settings/ConfigureProtocolWebDAV.cpp
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
src/Protocol/SplitPathSpecification.cpp
src/Protocol/Protocol.cpp
src/Protocol/ProtocolFile.cpp
)
set (PROTOCOL_LIBS )
set (PROTOCOL_DEFS )

if (LIBSSH_FOUND)
	set(SOURCES
		${SOURCES}
		src/Protocol/ProtocolSFTP.cpp
	)
	set (PROTOCOL_LIBS ${PROTOCOL_LIBS} ${LIBSSH_LIBRARIES})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_SFTP")
endif (LIBSSH_FOUND)

if (LIBSMBCLIENT_FOUND)
	set(SOURCES
		${SOURCES}
		src/Protocol/ProtocolSMB.cpp
		src/Protocol/NMBEnum.cpp
	)
	set (PROTOCOL_LIBS ${PROTOCOL_LIBS} ${LIBSMBCLIENT_LIBRARIES})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_SMB")
endif (LIBSMBCLIENT_FOUND)

if (LIBNFS_FOUND)
	set(SOURCES
		${SOURCES}
		src/Protocol/ProtocolNFS.cpp
	)
	set (PROTOCOL_LIBS ${PROTOCOL_LIBS} ${LIBNFS_LIBRARIES})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_NFS")
endif (LIBNFS_FOUND)

if (LIBNEON_FOUND)
	set(SOURCES
		${SOURCES}
		src/Protocol/ProtocolWebDAV.cpp
	)
	set (PROTOCOL_LIBS ${PROTOCOL_LIBS} ${LIBNEON_LIBRARIES})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_WEBDAV")
endif (LIBNEON_FOUND)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FAR_HAS_NAMELESS_UNIONS")

add_library (NetRocks MODULE ${SOURCES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS "${CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS} -flat_namespace -undefined suppress")
endif()

#FIND_LIBRARY(LIBSSH_LIBRARY NAMES ssh)
#FIND_LIBRARY(LIBSMBCLIENT_LIBRARY NAMES smbclient)

target_link_libraries(NetRocks utils ${PROTOCOL_LIBS})

target_include_directories(NetRocks PRIVATE src)
target_include_directories(NetRocks PRIVATE ../WinPort)
target_include_directories(NetRocks PRIVATE ../far2l)
target_include_directories(NetRocks PRIVATE ../far2l/Include)

target_compile_definitions(NetRocks PRIVATE)

if (LIBSSH_FOUND)
	target_include_directories(NetRocks PRIVATE ${LIBSSH_INCLUDE_DIRS})
endif (LIBSSH_FOUND)

if (LIBSMBCLIENT_FOUND)
	target_include_directories(NetRocks PRIVATE ${LIBSMBCLIENT_INCLUDE_DIRS})
endif (LIBSMBCLIENT_FOUND)


set_target_properties(NetRocks
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/NetRocks/plug"
    PREFIX ""
    SUFFIX ".far-plug-wide")

add_custom_command(TARGET NetRocks POST_BUILD
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/NetRocks")
