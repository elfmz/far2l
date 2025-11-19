
project(multiarc)

set(SOURCES
    src/MultiArc.cpp
    src/ArcPlg.cpp
    src/arccfg.cpp
    src/arcget.cpp
    src/arcput.cpp
    src/ArcMix.cpp
    src/ArcProc.cpp
    src/global.cpp
    src/arcread.cpp
    src/arccmd.cpp
    src/formats/ha/ha.cpp
    src/formats/arj/arj.cpp
    src/formats/ace/ace.cpp
    src/formats/arc/arc.cpp
    src/formats/zip/zip.cpp
    src/formats/lzh/lzh.cpp

        src/formats/7z/C/7zAlloc.c
        src/formats/7z/C/7zArcIn.c
        src/formats/7z/C/7zBuf2.c
        src/formats/7z/C/7zBuf.c
        src/formats/7z/C/7zCrc.c
        src/formats/7z/C/7zCrcOpt.c
        src/formats/7z/C/7zDec.c
        src/formats/7z/C/7zFile.c
        src/formats/7z/C/7zStream.c
        src/formats/7z/C/Aes.c
        src/formats/7z/C/AesOpt.c
        src/formats/7z/C/Alloc.c
        src/formats/7z/C/Bcj2.c
        src/formats/7z/C/Bcj2Enc.c
        src/formats/7z/C/Blake2s.c
        src/formats/7z/C/Bra86.c
        src/formats/7z/C/Bra.c
        src/formats/7z/C/BraIA64.c
        src/formats/7z/C/BwtSort.c
        src/formats/7z/C/CpuArch.c
        src/formats/7z/C/Delta.c
        src/formats/7z/C/DllSecur.c
        src/formats/7z/C/HuffEnc.c
        src/formats/7z/C/LzFind.c
        src/formats/7z/C/LzFindMt.c
        src/formats/7z/C/LzFindOpt.c
        src/formats/7z/C/Lzma2Dec.c
        src/formats/7z/C/Lzma2DecMt.c
        src/formats/7z/C/Lzma2Enc.c
        src/formats/7z/C/Lzma86Dec.c
        src/formats/7z/C/Lzma86Enc.c
        src/formats/7z/C/LzmaDec.c
        src/formats/7z/C/LzmaEnc.c
        src/formats/7z/C/LzmaLib.c
        src/formats/7z/C/Md5.c
        src/formats/7z/C/MtCoder.c
        src/formats/7z/C/MtDec.c
        src/formats/7z/C/Ppmd7aDec.c
        src/formats/7z/C/Ppmd7.c
        src/formats/7z/C/Ppmd7Dec.c
        src/formats/7z/C/Ppmd7Enc.c
        src/formats/7z/C/Ppmd8.c
        src/formats/7z/C/Ppmd8Dec.c
        src/formats/7z/C/Ppmd8Enc.c
        src/formats/7z/C/Sha1.c
        src/formats/7z/C/Sha1Opt.c
        src/formats/7z/C/Sha256.c
        src/formats/7z/C/Sha256Opt.c
        src/formats/7z/C/Sha3.c
        src/formats/7z/C/Sha512.c
        src/formats/7z/C/Sha512Opt.c
        src/formats/7z/C/Sort.c
        src/formats/7z/C/SwapBytes.c
        src/formats/7z/C/Threads.c
        src/formats/7z/C/Xxh64.c
        src/formats/7z/C/Xz.c
        src/formats/7z/C/XzCrc64.c
        src/formats/7z/C/XzCrc64Opt.c
        src/formats/7z/C/XzDec.c
        src/formats/7z/C/XzEnc.c
        src/formats/7z/C/XzIn.c
        src/formats/7z/C/ZstdDec.c
        src/formats/7z/7z.cpp
        src/formats/7z/7zMain.cpp

        src/formats/ha/ha/acoder.c
        src/formats/ha/ha/archive.c
        src/formats/ha/ha/asc.c
        src/formats/ha/ha/cpy.c
        src/formats/ha/ha/error.c
        src/formats/ha/ha/ha.c
        src/formats/ha/ha/haio.c
        src/formats/ha/ha/hsc.c
        src/formats/ha/ha/info.c
        src/formats/ha/ha/machine.c
        src/formats/ha/ha/misc.c
        src/formats/ha/ha/swdict.c
)

if("${UNRAR}" STREQUAL "lib")
    set(SOURCES
        ${SOURCES}
        src/formats/rar/rar.cpp
        src/formats/rar/rarmainstub.cpp
    )
elseif(UNRAR)
    set(RAR_FILES
        src/formats/rar/rar.cpp
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
        src/formats/rar/unrar/system.cpp
        src/formats/rar/unrar/crypt.cpp
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
        src/formats/rar/unrar/rar.cpp
        src/formats/rar/unrar/largepage.cpp
    )
    set(SOURCES ${SOURCES} ${RAR_FILES})
endif()

set(SOURCES
    ${SOURCES}
    src/formats/custom/custom.cpp
)

if(LibArchive_FOUND)
    set(SOURCES
        ${SOURCES}
        src/formats/libarch/libarch_utils.cpp
        src/formats/libarch/libarch_crutches.cpp
        src/formats/libarch/libarch_cmd_read.cpp
        src/formats/libarch/libarch_cmd_write.cpp
        src/formats/libarch/libarch.cpp
    )
else()
    set(SOURCES
        ${SOURCES}
        src/formats/targz/targz.cpp
        src/formats/cab/cab.cpp

    )
endif()

add_library (multiarc MODULE ${SOURCES})

target_compile_definitions(multiarc PRIVATE -D_7ZIP_ST -D_7ZIP_PPMD_SUPPPORT)

target_link_libraries(multiarc utils far2l)

if(NOT UNRAR)
    if (LibArchive_FOUND)
        message(STATUS "Using LibArchive for limited RAR archives support.")
    else()
        message(STATUS "${ColorRed}RAR archives support disabled due to UNRAR=${UNRAR} and lack of LibArchive.${ColorNormal}")
    endif()

elseif("${UNRAR}" STREQUAL "lib")
    message(STATUS "Using system unrar - will need libunrar and unrar tool.")
    find_package(Unrar REQUIRED)
    target_include_directories(multiarc PRIVATE ${UNRAR_INCLUDE_DIR})
    target_compile_definitions(multiarc PRIVATE -DRARDLL -DHAVE_UNRAR)
    target_link_libraries(multiarc ${UNRAR_LIBRARIES})
else()
    message(STATUS "Using bundled unrar sources")
    target_include_directories(multiarc PRIVATE src/formats/rar/unrar)
    target_compile_definitions(multiarc PRIVATE -DRARDLL -DHAVE_UNRAR)
    # hide rar warnings in release build
    if(NOT ${CMAKE_BUILD_TYPE} MATCHES Debug)
        set(UNRAR_FLAGS "-Wno-parentheses -Wno-switch -Wno-unused-variable -Wno-unused-but-set-variable -Wno-sign-compare -Wno-class-memaccess -Wno-catch-value -Wno-misleading-indentation -Wno-missing-braces")
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            set(UNRAR_FLAGS "${UNRAR_FLAGS} -Wno-unknown-warning-option")
        endif()
        set_source_files_properties(${RAR_FILES} PROPERTIES COMPILE_FLAGS "${UNRAR_FLAGS}")
    endif()
endif()

target_include_directories(multiarc PRIVATE src)
target_include_directories(multiarc PRIVATE ../WinPort)
target_include_directories(multiarc PRIVATE ../far2l/far2sdk)

if(LibArchive_FOUND)
    target_compile_definitions(multiarc PRIVATE -DHAVE_LIBARCHIVE)
    target_link_libraries(multiarc ${LibArchive_LIBRARIES})
    target_include_directories(multiarc PRIVATE ${LibArchive_INCLUDE_DIRS})
endif()

set_target_properties(multiarc
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/multiarc/plug"
        PREFIX ""
        SUFFIX ".far-plug-mb")

# copy resource files
set(CURRENT_TARGET "multiarc")
file(GLOB_RECURSE RESOURCE_FILES
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/configs
        "${CMAKE_CURRENT_SOURCE_DIR}/configs/*"
)
setup_target_auxiliary_files(${CURRENT_TARGET}
        RESOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/configs
        "${INSTALL_DIR}/Plugins/${CURRENT_TARGET}/"
)
add_dependencies(${CURRENT_TARGET} copy_aux_files_for_${CURRENT_TARGET})
