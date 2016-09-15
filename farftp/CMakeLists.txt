
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
src/FStdLib/FARStdlib/fstd_scr.cpp
src/FStdLib/FARStdlib/fstd_stdlibCS.cpp
src/FStdLib/FARStdlib/fstd_asrt.cpp
src/FStdLib/FARStdlib/fstd_Patt.cpp
src/FStdLib/FARStdlib/fstd_OEM.cpp
src/FStdLib/FARStdlib/fstd_SText.cpp
src/FStdLib/FARStdlib/fstd_exSCHC.cpp
src/FStdLib/FARStdlib/fstd_Reg.cpp
src/FStdLib/FARStdlib/fstd_plg.cpp
src/FStdLib/FARStdlib/fstd_Con.cpp
src/FStdLib/FARStdlib/fstd_exSCPY.cpp
src/FStdLib/FARStdlib/fstd_menu.cpp
src/FStdLib/FARStdlib/fstd_exSCAT.cpp
src/FStdLib/FARStdlib/fstd_log.cpp
src/FStdLib/FARStdlib/fstd_mklog.cpp
src/FStdLib/FARStdlib/fstd_String.cpp
src/FStdLib/FARStdlib/fstd_err.cpp
src/FStdLib/FARStdlib/fstd_mesg.cpp
src/FStdLib/FARStdlib/fstd_SCol.cpp
src/FStdLib/FARStdlib/fstd_Msg.cpp
src/FStdLib/FARStdlib/fstd_exSCMP.cpp
src/FStdLib/FARStdlib/fstd_exSPCH.cpp
src/FStdLib/FARStdlib/fstd_crc32.cpp
src/FStdLib/FARStdlib/fstd_exit.cpp
src/FStdLib/FARStdlib/fstd_exSNCH.cpp
src/FStdLib/FARStdlib/fstd_Dialog.cpp
src/FStdLib/FARStdlib/fstd_per.cpp
src/FStdLib/FARStdlib/fstd_Utils.cpp
src/FStdLib/FARStdlib/fstd_ilist.cpp
src/FStdLib/FARStdlib/fstd_Arg.cpp
src/FStdLib/FARStdlib/fstd_FMsg.cpp
src/FStdLib/FARStdlib/fstd_exSPS.cpp
src/FStdLib/FARStdlib/fstd_FUtils.cpp
)

add_library (farftp MODULE ${SOURCES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -flat_namespace -undefined suppress")
endif()

target_include_directories(farftp PRIVATE .)
target_include_directories(farftp PRIVATE src)
target_include_directories(farftp PRIVATE src/FStdLib)
target_include_directories(farftp PRIVATE ../WinPort)
target_include_directories(farftp PRIVATE ../far2l)
target_include_directories(farftp PRIVATE ../far2l/Include)

set_target_properties(farftp
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/farftp/bin"
    PREFIX ""
    SUFFIX ".far-plug-mb")

add_custom_command(TARGET farftp POST_BUILD
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/farftp")
