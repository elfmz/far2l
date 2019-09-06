
project(multiarc)

set(SOURCES
    src/MultiArc.cpp
    src/ArcPlg.cpp
    src/arccfg.cpp
    src/arcget.cpp
    src/arcput.cpp
    src/arcreg.cpp
    src/ArcMix.cpp
    src/ArcProc.cpp
    src/global.cpp
    src/arcread.cpp
    src/arccmd.cpp
    src/formats/rar/rar.cpp
    src/formats/ha/ha.cpp
    src/formats/custom/custom.cpp
    src/formats/arj/arj.cpp
    src/formats/ace/ace.cpp
    src/formats/arc/arc.cpp
    src/formats/zip/zip.cpp
    src/formats/lzh/lzh.cpp
    src/formats/cab/cab.cpp
    src/formats/targz/targz.cpp
    src/formats/7z/7z.cpp
    src/formats/7z/7zMain.c
    src/formats/rar/unrar/scantree.cpp
    src/formats/rar/unrar/sha1.cpp
    src/formats/rar/unrar/threadpool.cpp
    src/formats/rar/unrar/crc.cpp
    src/formats/rar/unrar/hash.cpp
    src/formats/rar/unrar/strlist.cpp
    src/formats/rar/unrar/rs16.cpp
    src/formats/rar/unrar/resource.cpp
    src/formats/rar/unrar/consio.cpp
    src/formats/rar/unrar/secpassword.cpp
    src/formats/rar/unrar/qopen.cpp
    src/formats/rar/unrar/recvol.cpp
    src/formats/rar/unrar/rijndael.cpp
    src/formats/rar/unrar/timefn.cpp
    src/formats/rar/unrar/encname.cpp
    src/formats/rar/unrar/rawread.cpp
    src/formats/rar/unrar/file.cpp
    src/formats/rar/unrar/strfn.cpp
    src/formats/rar/unrar/sha256.cpp
    src/formats/rar/unrar/filefn.cpp
    src/formats/rar/unrar/filcreat.cpp
    src/formats/rar/unrar/headers.cpp
    src/formats/rar/unrar/blake2s.cpp
    src/formats/rar/unrar/options.cpp
    src/formats/rar/unrar/volume.cpp
    src/formats/rar/unrar/isnt.cpp
    src/formats/rar/unrar/system.cpp
    src/formats/rar/unrar/crypt.cpp
    src/formats/rar/unrar/rar.cpp
    src/formats/rar/unrar/cmddata.cpp
    src/formats/rar/unrar/extinfo.cpp
    src/formats/rar/unrar/filestr.cpp
    src/formats/rar/unrar/ui.cpp
    src/formats/rar/unrar/list.cpp
    src/formats/rar/unrar/find.cpp
    src/formats/rar/unrar/smallfn.cpp
    src/formats/rar/unrar/unicode.cpp
    src/formats/rar/unrar/pathfn.cpp
    src/formats/rar/unrar/global.cpp
    src/formats/rar/unrar/rarvm.cpp
    src/formats/rar/unrar/getbits.cpp
    src/formats/rar/unrar/rs.cpp
    src/formats/rar/unrar/errhnd.cpp
    src/formats/rar/unrar/archive.cpp
    src/formats/rar/unrar/dll.cpp
    src/formats/rar/unrar/extract.cpp
    src/formats/rar/unrar/match.cpp
    src/formats/rar/unrar/unpack.cpp
    src/formats/rar/unrar/arcread.cpp
    src/formats/rar/unrar/rdwrfn.cpp
    src/formats/rar/unrar/rarpch.cpp
    src/formats/7z/C/Lzma86Dec.c
    src/formats/7z/C/7zDec.c
    src/formats/7z/C/Ppmd7Dec.c
    src/formats/7z/C/Sha1.c
    src/formats/7z/C/BraIA64.c
    src/formats/7z/C/Xz.c
    src/formats/7z/C/Lzma2Dec.c
    src/formats/7z/C/AesOpt.c
    src/formats/7z/C/BwtSort.c
    src/formats/7z/C/Bra86.c
    src/formats/7z/C/7zCrcOpt.c
    src/formats/7z/C/7zAlloc.c
    src/formats/7z/C/LzmaLib.c
    src/formats/7z/C/HuffEnc.c
    src/formats/7z/C/7zBuf.c
    src/formats/7z/C/XzDec.c
    src/formats/7z/C/7zBuf2.c
    src/formats/7z/C/Lzma86Enc.c
    src/formats/7z/C/Alloc.c
    src/formats/7z/C/XzCrc64.c
    src/formats/7z/C/XzEnc.c
    src/formats/7z/C/Ppmd8.c
    src/formats/7z/C/Aes.c
    src/formats/7z/C/7zCrc.c
    src/formats/7z/C/Bcj2.c
    src/formats/7z/C/Delta.c
    src/formats/7z/C/7zFile.c
    src/formats/7z/C/Bcj2Enc.c
    src/formats/7z/C/Ppmd8Enc.c
    src/formats/7z/C/LzmaEnc.c
    src/formats/7z/C/Ppmd7Enc.c
    src/formats/7z/C/7zStream.c
    src/formats/7z/C/Ppmd8Dec.c
    src/formats/7z/C/XzCrc64Opt.c
    src/formats/7z/C/LzmaDec.c
    src/formats/7z/C/XzIn.c
    src/formats/7z/C/CpuArch.c
    src/formats/7z/C/LzFind.c
    src/formats/7z/C/Ppmd7.c
    src/formats/7z/C/Lzma2Enc.c
    src/formats/7z/C/Sha256.c
    src/formats/7z/C/Sort.c
    src/formats/7z/C/7zArcIn.c
    src/formats/7z/C/Bra.c
    src/formats/7z/C/Blake2s.c

    src/libpcre/get.c
    src/libpcre/pcre.c
    src/libpcre/chartables.c
    src/libpcre/study.c

)

add_library (multiarc MODULE ${SOURCES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS "${CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS} -flat_namespace -undefined suppress")
endif()

target_link_libraries(multiarc utils far2l)

target_include_directories(multiarc PRIVATE src/formats/rar/unrar)
target_include_directories(multiarc PRIVATE src)
target_include_directories(multiarc PRIVATE src/libpcre)
target_include_directories(multiarc PRIVATE ../WinPort)
target_include_directories(multiarc PRIVATE ../far2l)
target_include_directories(multiarc PRIVATE ../far2l/Include)

target_compile_definitions(multiarc PRIVATE -DRARDLL -D_7ZIP_ST)

set_target_properties(multiarc
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/multiarc/plug"
    PREFIX ""
    SUFFIX ".far-plug-mb")

add_custom_command(TARGET multiarc POST_BUILD
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/multiarc")
