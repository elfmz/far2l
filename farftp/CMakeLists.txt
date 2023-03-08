
project(farftp)

set(SOURCES
lib/All.cpp
src/SetDir.cpp
src/cnInit.cpp
src/Cfg.cpp
src/DeleteFile.cpp
src/Key.cpp
src/HPut.cpp
src/FTPConnect.cpp
src/Mem.cpp
src/FAR.cpp
src/ConnectNB.cpp
src/ConnectCmds.cpp
src/sock.cpp
src/fUtils.cpp
src/FGet.cpp
src/EnumHost.cpp
src/ConnectMain.cpp
src/MakeDir.cpp
src/Ftp.cpp
src/Mix.cpp
src/FPut.cpp
src/ConnectIO.cpp
src/GetOpenInfo.cpp
src/Queque.cpp
src/Url.cpp
src/cnUpload.cpp
src/FileList.cpp
src/Plugin.cpp
src/Event.cpp
src/FtpDlg.cpp
src/Shortcut.cpp
src/ConnectSock.cpp
src/FtpAPI.cpp
src/FTPHost.cpp
src/CmdLine.cpp
src/cnDownload.cpp
src/JM.cpp
src/FTPBlock.cpp
src/HGet.cpp
src/AskOver.cpp
src/Connect.cpp
lib/DirList/pctcp.cpp
lib/DirList/vx.cpp
lib/DirList/os2.cpp
lib/DirList/vms.cpp
lib/DirList/unix.cpp
lib/DirList/os400.cpp
lib/DirList/dos.cpp
lib/DirList/netware.cpp
lib/DirList/skirdin.cpp
lib/DirList/eplf.cpp
lib/DirList/mvs.cpp
lib/DirList/Main.cpp
lib/DirList/tcpc.cpp
lib/DirList/cms.cpp
lib/Notify/Main.cpp
lib/Progress/cbFmt.cpp
lib/Progress/TraficCB.cpp
lib/Progress/Main.cpp
lib/Progress/Utils.cpp
)

add_library (farftp MODULE ${SOURCES})

target_link_libraries(farftp utils FARStdlib far2l)

target_include_directories(farftp PRIVATE .)
target_include_directories(farftp PRIVATE src)
target_include_directories(farftp PRIVATE ../FARStdlib/include)
target_include_directories(farftp PRIVATE ../WinPort)
target_include_directories(farftp PRIVATE ../far2l/far2sdk)

set_target_properties(farftp
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/farftp/plug"
        PREFIX ""
        SUFFIX ".far-plug-mb")

add_custom_command(TARGET farftp POST_BUILD
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/farftp")
