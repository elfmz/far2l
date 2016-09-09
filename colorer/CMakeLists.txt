
project(colorer)

set(SOURCES
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

add_definitions(-D_UNICODE -DUNICODE -D__unix__)

include_directories(src/shared)
include_directories(src/pcolorer2)
include_directories(../WinPort)
include_directories(../far2l)
include_directories(../far2l/Include)

add_library (colorer MODULE ${SOURCES})

find_package( ZLIB REQUIRED )
if ( ZLIB_FOUND )
    include_directories( ${ZLIB_INCLUDE_DIRS} )
    target_link_libraries( colorer ${ZLIB_LIBRARIES} )
endif( ZLIB_FOUND )

set_target_properties(colorer
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/colorer/bin"
    PREFIX ""
    SUFFIX ".far-plug-wide")

add_custom_command(TARGET colorer POST_BUILD
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/colorer"
)
