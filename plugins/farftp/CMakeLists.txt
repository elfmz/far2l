
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

fstd/fstd_scr.cpp
fstd/fstd_stdlibCS.cpp
fstd/fstd_asrt.cpp
fstd/fstd_Patt.cpp
fstd/fstd_OEM.cpp
fstd/fstd_SText.cpp
fstd/fstd_exSCHC.cpp
fstd/fstd_Reg.cpp
fstd/fstd_plg.cpp
fstd/fstd_Con.cpp
fstd/fstd_exSCPY.cpp
fstd/fstd_menu.cpp
fstd/fstd_exSCAT.cpp
fstd/fstd_log.cpp
fstd/fstd_mklog.cpp
fstd/fstd_String.cpp
fstd/fstd_err.cpp
fstd/fstd_mesg.cpp
fstd/fstd_SCol.cpp
fstd/fstd_Msg.cpp
fstd/fstd_exSCMP.cpp
fstd/fstd_exSPCH.cpp
fstd/fstd_crc32.cpp
fstd/fstd_exit.cpp
fstd/fstd_exSNCH.cpp
fstd/fstd_Dialog.cpp
fstd/fstd_per.cpp
fstd/fstd_Utils.cpp
fstd/fstd_ilist.cpp
fstd/fstd_Arg.cpp
fstd/fstd_FMsg.cpp
fstd/fstd_exSPS.cpp
fstd/fstd_FUtils.cpp
fstd/fstd_ClpS.cpp
)

add_library (farftp MODULE ${SOURCES})

target_link_libraries(farftp utils far2l)

target_include_directories(farftp PRIVATE .)
target_include_directories(farftp PRIVATE src)
target_include_directories(farftp PRIVATE fstd)
target_include_directories(farftp PRIVATE ${CMAKE_SOURCE_DIR}/WinPort)
target_include_directories(farftp PRIVATE ${CMAKE_SOURCE_DIR}/far2l/far2sdk)

set_target_properties(farftp
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/farftp/plug"
        PREFIX ""
        SUFFIX ".far-plug-mb")

add_custom_command(TARGET farftp POST_BUILD
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/farftp")
