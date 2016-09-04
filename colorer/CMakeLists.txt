
project(colorer)

set(SOURCES
src/zlib/uncompr.c
src/zlib/deflate.c
src/zlib/inftrees.c
src/zlib/trees.c
src/zlib/gzwrite.c
src/zlib/infback.c
src/zlib/crc32.c
src/zlib/gzclose.c
src/zlib/zutil.c
src/zlib/inflate.c
src/zlib/gzlib.c
src/zlib/gzread.c
src/zlib/adler32.c
src/zlib/compress.c
src/zlib/inffast.c
src/pcolorer2/FarEditor.cpp
src/pcolorer2/ChooseTypeMenu.cpp
src/pcolorer2/tools.cpp
src/pcolorer2/registry_wide.cpp
src/pcolorer2/FarHrcSettings.cpp
src/pcolorer2/FarEditorSet.cpp
src/pcolorer2/pcolorer.cpp
src/shared/common/Exception.cpp
src/shared/common/Logging.cpp
src/shared/common/MemoryChunks.cpp
src/shared/colorer/ParserFactory.cpp
src/shared/misc/malloc.c
src/shared/unicode/BitArray.cpp
src/shared/unicode/CharacterClass.cpp
src/shared/unicode/DString.cpp
src/shared/unicode/Encodings.cpp
src/shared/unicode/String.cpp
src/shared/unicode/SString.cpp
src/shared/unicode/UnicodeTools.cpp
src/shared/unicode/Character.cpp
src/shared/unicode/StringBuffer.cpp
src/shared/cregexp/cregexp.cpp
src/shared/xml/xmldom.cpp
src/zlib/contrib/minizip/mztools.c
src/zlib/contrib/minizip/zip.c
src/zlib/contrib/minizip/ioapi.c
src/zlib/contrib/minizip/unzip.c
src/shared/common/io/Writer.cpp
src/shared/common/io/FileInputSource.cpp
src/shared/common/io/HTTPInputSource.cpp
src/shared/common/io/SharedInputSource.cpp
src/shared/common/io/FileWriter.cpp
src/shared/common/io/StreamWriter.cpp
src/shared/common/io/InputSource.cpp
src/shared/common/io/JARInputSource.cpp
src/shared/colorer/parsers/HRCParserImpl.cpp
src/shared/colorer/parsers/TextParserImpl.cpp
src/shared/colorer/viewer/TextLinesStore.cpp
src/shared/colorer/viewer/TextConsoleViewer.cpp
src/shared/colorer/viewer/ConsoleTools.cpp
src/shared/colorer/editor/Outliner.cpp
src/shared/colorer/editor/BaseEditor.cpp
src/shared/colorer/handlers/FileErrorHandler.cpp
src/shared/colorer/handlers/TextHRDMapper.cpp
src/shared/colorer/handlers/LineRegionsSupport.cpp
src/shared/colorer/handlers/StyledHRDMapper.cpp
src/shared/colorer/handlers/LineRegionsCompactSupport.cpp
src/shared/colorer/handlers/RegionMapperImpl.cpp
src/shared/colorer/handlers/ErrorHandlerWriter.cpp
src/shared/colorer/parsers/helpers/HRCParserHelpers.cpp
src/shared/colorer/parsers/helpers/TextParserHelpers.cpp
)

add_definitions(-D_UNICODE)

include_directories(src/shared)
include_directories(src/zlib)
include_directories(src/pcolorer2)
include_directories(../WinPort)
include_directories(../far2l)
include_directories(../far2l/Include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -Wall -std=c++11 -fPIC --shared -Wno-unused-function")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -Wall -std=c99 -fPIC --shared -Wno-unused-function")

add_library (colorer SHARED ${SOURCES})

target_link_libraries (colorer far2l)

set_target_properties(colorer
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/Colorer/bin"
    PREFIX ""
    SUFFIX ".far-plug-mb")

add_custom_command(TARGET colorer POST_BUILD
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/Colorer"
)
