
project(farftp)

set(SOURCES
src/AskOver.cpp
src/Cfg.cpp
src/CmdLine.cpp
src/cnDownload.cpp
src/cnInit.cpp
src/cnUpload.cpp
src/ConnectCmds.cpp
src/Connect.cpp
src/ConnectIO.cpp
src/ConnectMain.cpp
src/ConnectNB.cpp
src/ConnectSock.cpp
src/DeleteFile.cpp
src/EnumHost.cpp
src/Event.cpp
src/FAR.cpp
src/FGet.cpp
src/FileList.cpp
src/FPut.cpp
src/FtpAPI.cpp
src/FTPBlock.cpp
src/FTPConnect.cpp
src/Ftp.cpp
src/FtpDlg.cpp
src/FTPHost.cpp
#src/ftpu.cpp
src/fUtils.cpp
src/GetOpenInfo.cpp
src/HGet.cpp
src/HPut.cpp
src/JM.cpp
src/Key.cpp
src/MakeDir.cpp
src/Mem.cpp
src/Mix.cpp
src/Plugin.cpp
src/Queque.cpp
src/SetDir.cpp
src/Shortcut.cpp
src/sock.cpp
src/Url.cpp

)

set(CMAKE_CXX_FLAGS "-Wall -std=c++11 -fPIC --shared -Wno-unused-function")
set(CMAKE_C_FLAGS "-Wall -std=c99 -fPIC --shared -Wno-unused-function")

include_directories(.)
include_directories(src)
include_directories(src/FStdLib)
include_directories(../WinPort)
include_directories(../far2l)
include_directories(../far2l/Include)

add_library (farftp SHARED ${SOURCES})

target_link_libraries (farftp far2l)

set_target_properties(farftp
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/farftp/bin"
    PREFIX ""
    SUFFIX ".far-plug-mb")

add_custom_command(TARGET farftp POST_BUILD
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/farftp")
