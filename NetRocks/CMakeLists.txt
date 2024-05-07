project(NetRocks)

set(SOURCES
src/Erroring.cpp
src/Globals.cpp
src/SitesConfig.cpp
src/NetRocks.cpp
src/PluginImpl.cpp
src/PluginPanelItems.cpp
src/PooledStrings.cpp
src/GetItems.cpp
src/BackgroundTasks.cpp
src/Location.cpp
src/ConnectionsPool.cpp
src/ImportFarFtpSites.cpp
src/Host/HostLocal.cpp
src/Host/HostRemote.cpp
src/Host/InitDeinitCmd.cpp
src/UI/DialogUtils.cpp
src/UI/Settings/ConfigurePlugin.cpp
src/UI/Settings/SiteConnectionEditor.cpp
src/UI/Settings/ExtraSiteSettings.cpp
src/UI/Settings/ProxySettings.cpp
src/UI/Settings/ConfigureProtocolSFTPSCP.cpp
src/UI/Settings/ConfigureProtocolFTP.cpp
src/UI/Settings/ConfigureProtocolSMB.cpp
src/UI/Settings/ConfigureProtocolNFS.cpp
src/UI/Settings/ConfigureProtocolWebDAV.cpp
src/UI/Settings/ConfigureProtocolSHELL.cpp
src/Protocol/SHELL/WayToShellConfig.cpp
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
src/Op/OpCheckDirectory.cpp
src/Op/OpChangeMode.cpp
src/Op/OpEnumDirectory.cpp
src/Op/OpXfer.cpp
src/Op/OpRemove.cpp
src/Op/OpMakeDirectory.cpp
src/Op/OpExecute.cpp
src/Op/OpGetLinkTarget.cpp
src/Protocol/SplitLocationSpecification.cpp
src/Protocol/Protocol.cpp
)


set(PROTOCOL_SOURCES
src/Erroring.cpp
src/Host/HostRemoteBroker.cpp
)

add_executable (NetRocks-FILE
    ${PROTOCOL_SOURCES}
    src/Protocol/File/ProtocolFile.cpp
    src/Host/HostLocal.cpp
)

add_executable (NetRocks-SHELL
    ${PROTOCOL_SOURCES}
    src/Protocol/SHELL/ProtocolSHELL.cpp
    src/Protocol/SHELL/ProtocolSHELL_ExecCmd.cpp
    src/Protocol/SHELL/WayToShellConfig.cpp
    src/Protocol/SHELL/WayToShell.cpp
    src/Protocol/SHELL/Request.cpp
    src/Protocol/SHELL/Parse.cpp
    src/Protocol/SHELL/RemoteSh.cpp
    src/Protocol/ShellParseUtils.cpp
)

# get rid of warning: dynamic exception specifications are deprecated in C++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")

target_link_libraries(NetRocks-FILE utils)
target_include_directories(NetRocks-FILE PRIVATE src)
set_target_properties(NetRocks-FILE
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/NetRocks/plug"
    PREFIX ""
    SUFFIX ".broker")
target_compile_options(NetRocks-FILE PUBLIC -DNETROCKS_PROTOCOL)

target_link_libraries(NetRocks-SHELL utils)
target_include_directories(NetRocks-SHELL PRIVATE src)
set_target_properties(NetRocks-SHELL
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/NetRocks/plug"
    PREFIX ""
    SUFFIX ".broker")
target_compile_options(NetRocks-SHELL PUBLIC -DNETROCKS_PROTOCOL)

add_custom_command(TARGET NetRocks-SHELL POST_BUILD
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/Protocol/SHELL/Helpers
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/Protocol/SHELL/Helpers "${INSTALL_DIR}/Plugins/NetRocks/plug/SHELL"
)


if (LIBSSH_FOUND AND ((NOT DEFINED NR_SFTP) OR NR_SFTP))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_SFTP")
    add_executable (NetRocks-SFTP
        ${PROTOCOL_SOURCES}
        src/Protocol/SSH/SSHConnection.cpp
        src/Protocol/SSH/ProtocolSFTP.cpp
        src/Protocol/SSH/ProtocolSCP.cpp
        src/Protocol/FileStatsOverride.cpp
        src/Protocol/ShellParseUtils.cpp
    )
    target_link_libraries(NetRocks-SFTP utils ${LIBSSH_LIBRARIES})
    target_include_directories(NetRocks-SFTP PRIVATE src ${LIBSSH_INCLUDE_DIRS})
    set_target_properties(NetRocks-SFTP
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/NetRocks/plug"
        PREFIX ""
        SUFFIX ".broker")
endif ()

if (LIBSMBCLIENT_FOUND AND ((NOT DEFINED NR_SMB) OR NR_SMB))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_SMB")
    add_executable (NetRocks-SMB
        ${PROTOCOL_SOURCES}
        src/Protocol/SMB/ProtocolSMB.cpp
        src/Protocol/SMB/NMBEnum.cpp
    )
    target_link_libraries(NetRocks-SMB utils ${LIBSMBCLIENT_LIBRARIES})
    target_include_directories(NetRocks-SMB PRIVATE src ${LIBSMBCLIENT_INCLUDE_DIRS})
    set_target_properties(NetRocks-SMB
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/NetRocks/plug"
        PREFIX ""
        SUFFIX ".broker")
endif ()

if (LIBNFS_FOUND AND ((NOT DEFINED NR_NFS) OR NR_NFS))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_NFS")
    add_executable (NetRocks-NFS
        ${PROTOCOL_SOURCES}
        src/Protocol/NFS/ProtocolNFS.cpp
    )
    target_link_libraries(NetRocks-NFS utils ${LIBNFS_LIBRARIES})
    target_include_directories(NetRocks-NFS PRIVATE src ${LIBNFS_INCLUDE_DIRS})
    set_target_properties(NetRocks-NFS
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/NetRocks/plug"
        PREFIX ""
        SUFFIX ".broker")
endif ()


if (LIBNEON_FOUND AND ((NOT DEFINED NR_WEBDAV) OR NR_WEBDAV))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_WEBDAV")
    add_executable (NetRocks-WebDAV
        ${PROTOCOL_SOURCES}
        src/Protocol/WebDAV/ProtocolWebDAV.cpp
    )
    target_link_libraries(NetRocks-WebDAV utils ${LIBNEON_LIBRARIES})
    target_include_directories(NetRocks-WebDAV PRIVATE src ${LIBNEON_INCLUDE_DIRS})
    set_target_properties(NetRocks-WebDAV
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/NetRocks/plug"
        PREFIX ""
        SUFFIX ".broker")
endif ()

    add_executable (NetRocks-FTP
        ${PROTOCOL_SOURCES}
        src/Protocol/FTP/ProtocolFTP.cpp
        src/Protocol/FTP/FTPConnection.cpp
        src/Protocol/FTP/FTPParseMLST.cpp
        src/Protocol/FTP/FTPParseLIST.cpp
        src/Protocol/DirectoryEnumCache.cpp
    )
    target_link_libraries(NetRocks-FTP utils)
    target_include_directories(NetRocks-FTP PRIVATE src)
if (OPENSSL_FOUND AND ((NOT DEFINED NR_OPENSSL) OR NR_OPENSSL))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_OPENSSL")
    target_link_libraries(NetRocks-FTP ${OPENSSL_LIBRARIES})
    target_include_directories(NetRocks-FTP PRIVATE ${OPENSSL_INCLUDE_DIR})
endif ()
    set_target_properties(NetRocks-FTP
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/NetRocks/plug"
        PREFIX ""
        SUFFIX ".broker")


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FAR_HAS_NAMELESS_UNIONS")

add_library (NetRocks MODULE ${SOURCES})

target_link_libraries(NetRocks utils far2l)

target_include_directories(NetRocks PRIVATE src)
target_include_directories(NetRocks PRIVATE ../WinPort)
target_include_directories(NetRocks PRIVATE ../far2l/far2sdk)

target_compile_definitions(NetRocks PRIVATE)

set_target_properties(NetRocks
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/NetRocks/plug"
        PREFIX ""
        SUFFIX ".far-plug-wide")

add_custom_command(TARGET NetRocks POST_BUILD
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/NetRocks")
