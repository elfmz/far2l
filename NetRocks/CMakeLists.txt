project(NetRocks)

set(SOURCES
src/Erroring.cpp
src/Globals.cpp
src/SitesConfig.cpp
src/NetRocks.cpp
src/PluginImpl.cpp
src/PluginPanelItems.cpp
src/PooledStrings.cpp
src/GetSelectedItems.cpp
src/BackgroundTasks.cpp
src/Location.cpp
src/ConnectionsPool.cpp
src/Host/IPC.cpp
src/Host/HostLocal.cpp
src/Host/HostRemote.cpp
src/UI/DialogUtils.cpp
src/UI/Settings/ConfigurePlugin.cpp
src/UI/Settings/SiteConnectionEditor.cpp
src/UI/Settings/ConfigureProtocolFile.cpp
src/UI/Settings/ConfigureProtocolSFTPSCP.cpp
src/UI/Settings/ConfigureProtocolSMB.cpp
src/UI/Settings/ConfigureProtocolNFS.cpp
src/UI/Settings/ConfigureProtocolWebDAV.cpp
src/UI/Activities/Confirm.cpp
src/UI/Activities/ConfirmXfer.cpp
src/UI/Activities/ConfirmOverwrite.cpp
src/UI/Activities/ConfirmChangeMode.cpp
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
src/Op/OpChangeMode.cpp
src/Op/OpEnumDirectory.cpp
src/Op/OpXfer.cpp
src/Op/OpRemove.cpp
src/Op/OpMakeDirectory.cpp
src/Op/OpExecute.cpp
src/Protocol/SplitLocationSpecification.cpp
src/Protocol/Protocol.cpp
)


set(PROTOCOL_SOURCES
src/Erroring.cpp
src/Host/HostRemoteBroker.cpp
src/Host/IPC.cpp
)


add_executable (NetRocks-FILE
	${PROTOCOL_SOURCES}
	src/Protocol/ProtocolFile.cpp
	src/Host/HostLocal.cpp
)

target_link_libraries(NetRocks-FILE utils)
target_include_directories(NetRocks-FILE PRIVATE src)
set_target_properties(NetRocks-FILE
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/NetRocks/plug"
	PREFIX ""
	SUFFIX ".broker")
target_compile_options(NetRocks-FILE PUBLIC -DNETROCKS_PROTOCOL)

if (LIBSSH_FOUND)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_SFTP")
	add_executable (NetRocks-SFTP
		${PROTOCOL_SOURCES}
		src/Protocol/SSHConnection.cpp
		src/Protocol/ProtocolSFTP.cpp
		src/Protocol/ProtocolSCP.cpp
	)
	target_link_libraries(NetRocks-SFTP utils ${LIBSSH_LIBRARIES})
	target_include_directories(NetRocks-SFTP PRIVATE src ${LIBSSH_INCLUDE_DIRS})
	set_target_properties(NetRocks-SFTP
		PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/NetRocks/plug"
		PREFIX ""
		SUFFIX ".broker")
endif (LIBSSH_FOUND)

if (LIBSMBCLIENT_FOUND)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_SMB")
	add_executable (NetRocks-SMB
		${PROTOCOL_SOURCES}
		src/Protocol/ProtocolSMB.cpp
		src/Protocol/NMBEnum.cpp
	)
	target_link_libraries(NetRocks-SMB utils ${LIBSMBCLIENT_LIBRARIES})
	target_include_directories(NetRocks-SMB PRIVATE src ${LIBSMBCLIENT_INCLUDE_DIRS})
	set_target_properties(NetRocks-SMB
		PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/NetRocks/plug"
		PREFIX ""
		SUFFIX ".broker")
endif (LIBSMBCLIENT_FOUND)

if (LIBNFS_FOUND)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_NFS")
	add_executable (NetRocks-NFS
		${PROTOCOL_SOURCES}
		src/Protocol/ProtocolNFS.cpp
	)
	target_link_libraries(NetRocks-NFS utils ${LIBNFS_LIBRARIES})
	target_include_directories(NetRocks-NFS PRIVATE src ${LIBNFS_INCLUDE_DIRS})
	set_target_properties(NetRocks-NFS
		PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/NetRocks/plug"
		PREFIX ""
		SUFFIX ".broker")
endif (LIBNFS_FOUND)

if (LIBNEON_FOUND)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_WEBDAV")
	add_executable (NetRocks-WebDAV
		${PROTOCOL_SOURCES}
		src/Protocol/ProtocolWebDAV.cpp
	)
	target_link_libraries(NetRocks-WebDAV utils ${LIBNEON_LIBRARIES})
	target_include_directories(NetRocks-WebDAV PRIVATE src ${LIBNEON_INCLUDE_DIRS})
	set_target_properties(NetRocks-WebDAV
		PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/NetRocks/plug"
		PREFIX ""
		SUFFIX ".broker")
endif (LIBNEON_FOUND)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FAR_HAS_NAMELESS_UNIONS")

add_library (NetRocks MODULE ${SOURCES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS "${CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS} -flat_namespace -undefined suppress")
endif()

target_link_libraries(NetRocks utils far2l)

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
