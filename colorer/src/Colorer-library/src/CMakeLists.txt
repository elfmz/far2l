#====================================================
# colorer source
#====================================================
set(SRC_COLORER
    colorer/Common.h
    colorer/Exception.h
    colorer/FileType.h
    colorer/HrcLibrary.h
    colorer/LineSource.h
    colorer/ParserFactory.h
    colorer/Region.h
    colorer/RegionHandler.h
    colorer/Scheme.h
    colorer/TextParser.h
    colorer/base/BaseNames.h
    colorer/base/XmlTagDefs.h
    colorer/common/Exception.cpp
    colorer/common/Logger.cpp
    colorer/common/Logger.h
    colorer/cregexp/cregexp.cpp
    colorer/cregexp/cregexp.h
    colorer/editor/BaseEditor.cpp
    colorer/editor/BaseEditor.h
    colorer/editor/EditorListener.h
    colorer/editor/OutlineItem.h
    colorer/editor/Outliner.cpp
    colorer/editor/Outliner.h
    colorer/editor/PairMatch.h
    colorer/handlers/LineRegion.cpp
    colorer/handlers/LineRegion.h
    colorer/handlers/LineRegionsCompactSupport.cpp
    colorer/handlers/LineRegionsCompactSupport.h
    colorer/handlers/LineRegionsSupport.cpp
    colorer/handlers/LineRegionsSupport.h
    colorer/handlers/RegionDefine.h
    colorer/handlers/RegionMapper.cpp
    colorer/handlers/RegionMapper.h
    colorer/handlers/StyledHRDMapper.cpp
    colorer/handlers/StyledHRDMapper.h
    colorer/handlers/StyledRegion.cpp
    colorer/handlers/StyledRegion.h
    colorer/handlers/TextHRDMapper.cpp
    colorer/handlers/TextHRDMapper.h
    colorer/handlers/TextRegion.cpp
    colorer/handlers/TextRegion.h
    colorer/io/FileInputSource.cpp
    colorer/io/FileInputSource.h
    colorer/io/FileWriter.cpp
    colorer/io/FileWriter.h
    colorer/io/InputSource.cpp
    colorer/io/InputSource.h
    colorer/io/StreamWriter.cpp
    colorer/io/StreamWriter.h
    colorer/io/Writer.cpp
    colorer/io/Writer.h
    colorer/parsers/CatalogParser.cpp
    colorer/parsers/CatalogParser.h
    colorer/parsers/FileType.cpp
    colorer/parsers/FileTypeChooser.cpp
    colorer/parsers/FileTypeChooser.h
    colorer/parsers/FileTypeImpl.cpp
    colorer/parsers/FileTypeImpl.h
    colorer/parsers/HrcLibrary.cpp
    colorer/parsers/HrcLibraryImpl.cpp
    colorer/parsers/HrcLibraryImpl.h
    colorer/parsers/HrdNode.h
    colorer/parsers/KeywordList.cpp
    colorer/parsers/KeywordList.h
    colorer/parsers/ParserFactory.cpp
    colorer/parsers/ParserFactoryImpl.cpp
    colorer/parsers/ParserFactoryImpl.h
    colorer/parsers/SchemeImpl.h
    colorer/parsers/SchemeNode.cpp
    colorer/parsers/SchemeNode.h
    colorer/parsers/TextParser.cpp
    colorer/parsers/TextParserHelpers.cpp
    colorer/parsers/TextParserHelpers.h
    colorer/parsers/TextParserImpl.cpp
    colorer/parsers/TextParserImpl.h
    colorer/parsers/VirtualEntry.h
    colorer/utils/Environment.cpp
    colorer/utils/Environment.h
    colorer/utils/FileSystems.h
    colorer/version.h
    colorer/viewer/ParsedLineWriter.cpp
    colorer/viewer/ParsedLineWriter.h
    colorer/viewer/TextConsoleViewer.cpp
    colorer/viewer/TextConsoleViewer.h
    colorer/viewer/TextLinesStore.cpp
    colorer/viewer/TextLinesStore.h
    colorer/xml/XMLNode.cpp
    colorer/xml/XMLNode.h
    colorer/xml/XmlInputSource.cpp
    colorer/xml/XmlInputSource.h
    colorer/xml/XmlReader.cpp
    colorer/xml/XmlReader.h
)


set(SRC_COLORER_XML
  colorer/xml/libxml2/LibXmlReader.cpp
  colorer/xml/libxml2/LibXmlReader.h
  colorer/xml/libxml2/LibXmlInputSource.h
  colorer/xml/libxml2/LibXmlInputSource.cpp
)


if(COLORER_USE_ZIPINPUTSOURCE)
  set(COLORER_FEATURE_ZIPINPUTSOURCE 1)
  set(SRC_COLORER_ZIP
    colorer/xml/libxml2/SharedXmlInputSource.cpp
    colorer/xml/libxml2/SharedXmlInputSource.h
    colorer/zip/MemoryFile.cpp
    colorer/zip/MemoryFile.h
  )
endif()

if(COLORER_USE_ICU_STRINGS)
  set(SRC_COLORER_STRINGS
      colorer/strings/icu/common_icu.h
      colorer/strings/icu/strings.h
      colorer/strings/icu/Character.cpp
      colorer/strings/icu/Character.h
      colorer/strings/icu/Encodings.cpp
      colorer/strings/icu/Encodings.h
      colorer/strings/icu/UnicodeStringContainer.h
      colorer/strings/icu/UnicodeTools.cpp
      colorer/strings/icu/UnicodeTools.h
      colorer/strings/icu/UStr.cpp
      colorer/strings/icu/UStr.h
  )
  set(COLORER_FEATURE_ICU 1)

else()
  set(SRC_COLORER_STRINGS
      colorer/strings/legacy/common_legacy.h
      colorer/strings/legacy/strings.h
      colorer/strings/legacy/BitArray.cpp
      colorer/strings/legacy/BitArray.h
      colorer/strings/legacy/Character.cpp
      colorer/strings/legacy/Character.h
      colorer/strings/legacy/CharacterClass.cpp
      colorer/strings/legacy/CharacterClass.h
      colorer/strings/legacy/CommonString.h
      colorer/strings/legacy/CString.cpp
      colorer/strings/legacy/CString.h
      colorer/strings/legacy/Encodings.cpp
      colorer/strings/legacy/Encodings.h
      colorer/strings/legacy/UnicodeString.cpp
      colorer/strings/legacy/UnicodeString.h
      colorer/strings/legacy/UnicodeStringContainer.h
      colorer/strings/legacy/UnicodeTools.cpp
      colorer/strings/legacy/UnicodeTools.h
      colorer/strings/legacy/UStr.cpp
      colorer/strings/legacy/UStr.h
      colorer/strings/legacy/x_charcategory.h
      colorer/strings/legacy/x_charcategory2.h
      colorer/strings/legacy/x_charcategory_names.h
      colorer/strings/legacy/x_defines.h
      colorer/strings/legacy/x_encodings.h
      colorer/strings/legacy/x_tables.h
      colorer/strings/legacy/StringExceptions.h
  )
endif()

if(COLORER_BUILD_OLD_COMPILERS)
  set(SRC_COLORER_PLATFORM colorer/platform/filesystem.hpp)
  set(COLORER_FEATURE_OLD_COMPILERS 1)
endif()

#====================================================
# Generate headers
#====================================================
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/colorer/common/Features.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/colorer/common/Features.h
)

#====================================================
# common flags
#====================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # default for gcc
  # CMAKE_CXX_FLAGS=
  # CMAKE_CXX_FLAGS_DEBUG= -g
  # CMAKE_CXX_FLAGS_RELEASE= -O3 -DNDEBUG

  # set only new flags
  set(FLAGS_CXX_DEFAULT -fpermissive -Wall -Wextra -Wpedantic -Wsign-promo -Wnon-virtual-dtor)

  if (COLORER_BUILD_HARD_WARNINGS)
    set(FLAGS_CXX_RELEASE ${FLAGS_CXX_RELEASE} -Werror)
  endif()

  set(FLAGS_CXX_DEBUG)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")

  # default for msvc
  # CMAKE_CXX_FLAGS= /DWIN32 /D_WINDOWS /GR /EHsc
  # CMAKE_CXX_FLAGS_DEBUG= /MDd /Zi /Ob0 /Od /RTC1
  # CMAKE_CXX_FLAGS_RELEASE= /MD /O2 /Ob2 /DNDEBUG

  # remove /W3 flag from CMAKE_CXX_FLAGS
  string(REGEX REPLACE "/W[3|4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

  set(FLAGS_CXX_DEFAULT /Zi)
  set(FLAGS_CXX_RELEASE /W3)

  if (COLORER_BUILD_HARD_WARNINGS)
    set(FLAGS_CXX_RELEASE ${FLAGS_CXX_RELEASE} /WX)
  endif()

  set(FLAGS_CXX_DEBUG /W4)
endif()

set(MAIN_FLAGS_CXX_DEBUG ${FLAGS_CXX_DEBUG} ${FLAGS_CXX_DEFAULT})
set(MAIN_FLAGS_CXX_RELEASE ${FLAGS_CXX_RELEASE} ${FLAGS_CXX_DEFAULT})

#====================================================
# build
#====================================================
add_library(colorer_lib STATIC ${SRC_COLORER} ${SRC_COLORER_STRINGS} ${SRC_COLORER_XML} ${SRC_COLORER_ZIP} ${SRC_COLORER_PLATFORM})
add_library(colorer::colorer ALIAS colorer_lib)
target_include_directories(colorer_lib PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

if(COLORER_BUILD_OLD_COMPILERS)
  set_target_properties(colorer_lib PROPERTIES
          CXX_STANDARD 17
          CXX_STANDARD_REQUIRED YES
          CXX_EXTENSIONS YES
  )
else()
  set_target_properties(colorer_lib PROPERTIES
          CXX_STANDARD 17
          CXX_STANDARD_REQUIRED YES
          CXX_EXTENSIONS NO
  )
endif()

target_link_libraries(colorer_lib
        PUBLIC LibXml2::LibXml2
)

if(COLORER_USE_ICU_STRINGS)
  target_link_libraries(colorer_lib
      PUBLIC ICU::uc ICU::data
  )
  if (APPLE)
    target_include_directories(colorer_lib PUBLIC
         "$<BUILD_INTERFACE:/opt/homebrew/include>"
    )
  endif ()
endif()

if(COLORER_USE_ZIPINPUTSOURCE)
  if(COLORER_USE_VCPKG)
    target_link_libraries(colorer_lib PUBLIC unofficial::minizip::minizip)
  else()
    target_link_libraries(colorer_lib PUBLIC minizip::minizip)
  endif()
endif()

if(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
  target_link_libraries(colorer_lib PUBLIC stdc++fs)
endif()

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
  target_compile_options(colorer_lib PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${MAIN_FLAGS_CXX_DEBUG}>)
else()
  target_compile_options(colorer_lib PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${MAIN_FLAGS_CXX_RELEASE}>)
endif()

set_target_properties(colorer_lib PROPERTIES
    OUTPUT_NAME "colorer"
    VERSION ${COLORER_VERSION}
    SOVERSION ${COLORER_VERSION_MAJOR}
    DEBUG_POSTFIX "d"
)
