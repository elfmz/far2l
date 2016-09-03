
project(multiarc)

set(SOURCES
./src/arccmd.cpp
./src/formats/ace/ace.cpp
./src/formats/7z/7z.cpp
./src/formats/arc/arc.cpp
./src/formats/arj/arj.cpp
./src/formats/targz/targz.cpp
./src/formats/rar/rar.cpp
./src/formats/rar/unrar/extract.cpp
./src/formats/rar/unrar/uisilent.cpp
./src/formats/rar/unrar/strlist.cpp
./src/formats/rar/unrar/rs16.cpp
./src/formats/rar/unrar/dll.cpp
./src/formats/rar/unrar/isnt.cpp
./src/formats/rar/unrar/arccmt.cpp
./src/formats/rar/unrar/global.cpp
./src/formats/rar/unrar/blake2s_sse.cpp
./src/formats/rar/unrar/unicode.cpp
./src/formats/rar/unrar/list.cpp
./src/formats/rar/unrar/crypt3.cpp
./src/formats/rar/unrar/errhnd.cpp
./src/formats/rar/unrar/unpack30.cpp
./src/formats/rar/unrar/rs.cpp
./src/formats/rar/unrar/cmddata.cpp
./src/formats/rar/unrar/model.cpp
./src/formats/rar/unrar/smallfn.cpp
./src/formats/rar/unrar/rdwrfn.cpp
./src/formats/rar/unrar/filcreat.cpp
./src/formats/rar/unrar/unpack15.cpp
./src/formats/rar/unrar/uiconsole.cpp
./src/formats/rar/unrar/rarvm.cpp
./src/formats/rar/unrar/rar.cpp
./src/formats/rar/unrar/blake2sp.cpp
./src/formats/rar/unrar/file.cpp
./src/formats/rar/unrar/sha256.cpp
./src/formats/rar/unrar/rarvmtbl.cpp
./src/formats/rar/unrar/uicommon.cpp
./src/formats/rar/unrar/threadmisc.cpp
./src/formats/rar/unrar/hardlinks.cpp
./src/formats/rar/unrar/recvol3.cpp
./src/formats/rar/unrar/secpassword.cpp
./src/formats/rar/unrar/blake2s.cpp
./src/formats/rar/unrar/unpackinline.cpp
./src/formats/rar/unrar/recvol.cpp
./src/formats/rar/unrar/unpack50frag.cpp
./src/formats/rar/unrar/unpack50.cpp
./src/formats/rar/unrar/ui.cpp
./src/formats/rar/unrar/arcread.cpp
./src/formats/rar/unrar/coder.cpp
./src/formats/rar/unrar/win32stm.cpp
./src/formats/rar/unrar/encname.cpp
./src/formats/rar/unrar/unpack50mt.cpp
./src/formats/rar/unrar/timefn.cpp
./src/formats/rar/unrar/filestr.cpp
./src/formats/rar/unrar/uowners.cpp
./src/formats/rar/unrar/filefn.cpp
./src/formats/rar/unrar/find.cpp
./src/formats/rar/unrar/crc.cpp
./src/formats/rar/unrar/qopen.cpp
./src/formats/rar/unrar/ulinks.cpp
./src/formats/rar/unrar/crypt2.cpp
./src/formats/rar/unrar/archive.cpp
./src/formats/rar/unrar/crypt.cpp
./src/formats/rar/unrar/getbits.cpp
./src/formats/rar/unrar/headers.cpp
./src/formats/rar/unrar/consio.cpp
./src/formats/rar/unrar/pathfn.cpp
./src/formats/rar/unrar/rijndael.cpp
./src/formats/rar/unrar/rawread.cpp
./src/formats/rar/unrar/crypt1.cpp
./src/formats/rar/unrar/options.cpp
./src/formats/rar/unrar/scantree.cpp
./src/formats/rar/unrar/crypt5.cpp
./src/formats/rar/unrar/threadpool.cpp
./src/formats/rar/unrar/win32acl.cpp
./src/formats/rar/unrar/win32lnk.cpp
./src/formats/rar/unrar/rarpch.cpp
./src/formats/rar/unrar/system.cpp
./src/formats/rar/unrar/resource.cpp
./src/formats/rar/unrar/log.cpp
./src/formats/rar/unrar/extinfo.cpp
./src/formats/rar/unrar/unpack.cpp
./src/formats/rar/unrar/recvol5.cpp
./src/formats/rar/unrar/unpack20.cpp
./src/formats/rar/unrar/strfn.cpp
./src/formats/rar/unrar/sha1.cpp
./src/formats/rar/unrar/hash.cpp
./src/formats/rar/unrar/volume.cpp
./src/formats/rar/unrar/match.cpp
./src/formats/rar/unrar/suballoc.cpp
./src/formats/ha/ha.cpp
./src/formats/lzh/lzh.cpp
./src/formats/zip/zip.cpp
./src/formats/custom/custom.cpp
./src/formats/cab/cab.cpp
./src/global.cpp
./src/MultiArc.cpp
./src/ArcMix.cpp
./src/ArcProc.cpp
./src/arcread.cpp
./src/ma_emu.cpp
./src/arccfg.cpp
./src/libpcre/test/test.cpp
./src/arcget.cpp
./src/arcreg.cpp
./src/ArcPlg.cpp
./src/arcput.cpp
)

set(CMAKE_CXX_FLAGS "-Wall -std=c++11 -fPIC --shared -Wno-unused-function")
set(CMAKE_C_FLAGS "-Wall -std=c99 -fPIC --shared -Wno-unused-function")

include_directories(.)
include_directories(src)
include_directories(src/FStdLib)
include_directories(../WinPort)
include_directories(../far2l)
include_directories(../far2l/Include)

add_library (multiarc SHARED ${SOURCES})

target_link_libraries (multiarc far2l)

set_target_properties(multiarc
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/multiarc/bin"
    PREFIX ""
    SUFFIX ".far-plug-mb")

add_custom_command(TARGET multiarc POST_BUILD
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/multiarc")
