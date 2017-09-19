
project(FARStdlib)

find_package(PkgConfig REQUIRED)

set(SOURCES
src/fstd_scr.cpp
src/fstd_stdlibCS.cpp
src/fstd_asrt.cpp
src/fstd_Patt.cpp
src/fstd_OEM.cpp
src/fstd_SText.cpp
src/fstd_exSCHC.cpp
src/fstd_Reg.cpp
src/fstd_plg.cpp
src/fstd_Con.cpp
src/fstd_exSCPY.cpp
src/fstd_menu.cpp
src/fstd_exSCAT.cpp
src/fstd_log.cpp
src/fstd_mklog.cpp
src/fstd_String.cpp
src/fstd_err.cpp
src/fstd_mesg.cpp
src/fstd_SCol.cpp
src/fstd_Msg.cpp
src/fstd_exSCMP.cpp
src/fstd_exSPCH.cpp
src/fstd_crc32.cpp
src/fstd_exit.cpp
src/fstd_exSNCH.cpp
src/fstd_Dialog.cpp
src/fstd_per.cpp
src/fstd_Utils.cpp
src/fstd_ilist.cpp
src/fstd_Arg.cpp
src/fstd_FMsg.cpp
src/fstd_exSPS.cpp
src/fstd_FUtils.cpp
src/fstd_ClpS.cpp
)

add_library (FARStdlib ${SOURCES})

target_include_directories(FARStdlib PRIVATE ${GLIB2_INCLUDE_DIRS})

target_include_directories(FARStdlib PRIVATE include)
target_include_directories(FARStdlib PRIVATE ../WinPort)
target_include_directories(FARStdlib PRIVATE ../far2l)
target_include_directories(FARStdlib PRIVATE ../far2l/Include)
