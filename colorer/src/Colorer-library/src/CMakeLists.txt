project(colorer_lib)

#====================================================
# colorer
#====================================================
set(SRC_COLORER
    colorer/Common.h
    colorer/Exception.h
    colorer/FileType.h
    colorer/HRCParser.h
    colorer/LineSource.h
    colorer/Region.h
    colorer/RegionHandler.h
    colorer/Scheme.h
    colorer/TextParser.h
    colorer/version.h
    colorer/common/Colorer.cpp
    colorer/common/Colorer.h
    colorer/common/Exception.cpp
    colorer/common/Features.h
    colorer/cregexp/cregexp.cpp
    colorer/cregexp/cregexp.h
    colorer/editor/BaseEditor.cpp
    colorer/editor/BaseEditor.h
    colorer/editor/EditorListener.h
    colorer/editor/OutlineItem.h
    colorer/editor/Outliner.cpp
    colorer/editor/Outliner.h
    colorer/editor/PairMatch.h
    colorer/handlers/LineRegion.h
    colorer/handlers/LineRegionsCompactSupport.cpp
    colorer/handlers/LineRegionsCompactSupport.h
    colorer/handlers/LineRegionsSupport.cpp
    colorer/handlers/LineRegionsSupport.h
    colorer/handlers/RegionDefine.h
    colorer/handlers/RegionMapper.h
    colorer/handlers/RegionMapperImpl.cpp
    colorer/handlers/RegionMapperImpl.h
    colorer/handlers/StyledHRDMapper.cpp
    colorer/handlers/StyledHRDMapper.h
    colorer/handlers/StyledRegion.h
    colorer/handlers/TextHRDMapper.cpp
    colorer/handlers/TextHRDMapper.h
    colorer/handlers/TextRegion.h
    colorer/io/FileInputSource.cpp
    colorer/io/FileInputSource.h
    colorer/io/FileWriter.cpp
    colorer/io/FileWriter.h
    colorer/io/InputSource.cpp
    colorer/io/InputSource.h
    colorer/io/SharedInputSource.cpp
    colorer/io/SharedInputSource.h
    colorer/io/StreamWriter.cpp
    colorer/io/StreamWriter.h
    colorer/io/Writer.cpp
    colorer/io/Writer.h
    colorer/parsers/CatalogParser.cpp
    colorer/parsers/CatalogParser.h
    colorer/parsers/FileTypeChooser.h
    colorer/parsers/FileTypeImpl.cpp
    colorer/parsers/FileTypeImpl.h
    colorer/parsers/HRCParserImpl.cpp
    colorer/parsers/HRCParserImpl.h
    colorer/parsers/HRDNode.h
    colorer/parsers/KeywordList.cpp
    colorer/parsers/KeywordList.h
    colorer/parsers/ParserFactory.cpp
    colorer/parsers/ParserFactory.h
    colorer/parsers/ParserFactoryException.h
    colorer/parsers/SchemeImpl.h
    colorer/parsers/SchemeNode.cpp
    colorer/parsers/SchemeNode.h
    colorer/parsers/TextParserHelpers.cpp
    colorer/parsers/TextParserHelpers.h
    colorer/parsers/TextParserImpl.cpp
    colorer/parsers/TextParserImpl.h
    colorer/parsers/VirtualEntry.h
    colorer/unicode/BitArray.cpp
    colorer/unicode/BitArray.h
    colorer/unicode/CString.cpp
    colorer/unicode/CString.h
    colorer/unicode/Character.cpp
    colorer/unicode/Character.h
    colorer/unicode/CharacterClass.cpp
    colorer/unicode/CharacterClass.h
    colorer/unicode/DString.h
    colorer/unicode/Encodings.cpp
    colorer/unicode/Encodings.h
    colorer/unicode/SString.cpp
    colorer/unicode/SString.h
    colorer/unicode/String.cpp
    colorer/unicode/String.h
    colorer/unicode/UnicodeTools.cpp
    colorer/unicode/UnicodeTools.h
    colorer/unicode/x_charcategory.h
    colorer/unicode/x_charcategory2.h
    colorer/unicode/x_charcategory_names.h
    colorer/unicode/x_defines.h
    colorer/unicode/x_encodings.h
    colorer/unicode/x_tables.h
    colorer/viewer/ParsedLineWriter.h
    colorer/viewer/TextConsoleViewer.cpp
    colorer/viewer/TextConsoleViewer.h
    colorer/viewer/TextLinesStore.cpp
    colorer/viewer/TextLinesStore.h
    colorer/xml/BaseEntityResolver.cpp
    colorer/xml/BaseEntityResolver.h
    colorer/xml/LocalFileXmlInputSource.cpp
    colorer/xml/LocalFileXmlInputSource.h
    colorer/xml/SharedXmlInputSource.cpp
    colorer/xml/SharedXmlInputSource.h
    colorer/xml/XStr.cpp
    colorer/xml/XStr.h
    colorer/xml/XmlInputSource.cpp
    colorer/xml/XmlInputSource.h
    colorer/xml/XmlParserErrorHandler.cpp
    colorer/xml/XmlParserErrorHandler.h
    colorer/xml/XmlTagDefs.h
    )

# Disabled by DCOLORER_FEATURE_JARINPUTSOURCE=FALSE
#    colorer/xml/ZipXmlInputSource.cpp
#    colorer/xml/ZipXmlInputSource.h
#    colorer/io/JARInputSource.cpp
#    colorer/io/JARInputSource.h
#    colorer/io/MemoryFile.cpp
#    colorer/io/MemoryFile.h

# Disabled by COLORER_FEATURE_HTTPINPUTSOURCE=FALSE
#    colorer/io/HTTPInputSource.cpp
#    colorer/io/HTTPInputSource.h

#====================================================
# common flags
#====================================================
set(COLORER_DEF -DNOGDI -DCOLORER_FEATURE_HTTPINPUTSOURCE=FALSE -DCOLORER_FEATURE_JARINPUTSOURCE=FALSE)
if (MSVC)
  set(COLORER_DEF ${COLORER_DEF} -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  # defaut for gcc
  # CMAKE_CXX_FLAGS=
  # CMAKE_CXX_FLAGS_DEBUG= -g
  # CMAKE_CXX_FLAGS_RELEASE= -O3 -DNDEBUG

  # set only new flags
  set(FLAGS_CXX_DEFAULT -fpermissive -Wall)
  set(FLAGS_CXX_RELEASE)
  set(FLAGS_CXX_DEBUG)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  # defaut for msvc
  # CMAKE_C_FLAGS= /DWIN32 /D_WINDOWS
  # CMAKE_C_FLAGS_DEBUG= /MDd /Zi /Ob0 /Od /RTC1
  # CMAKE_C_FLAGS_RELEASE= /MD /O2 /Ob2 /DNDEBUG

  # CMAKE_CXX_FLAGS= /DWIN32 /D_WINDOWS /GR /EHsc
  # CMAKE_CXX_FLAGS_DEBUG= /MDd /Zi /Ob0 /Od /RTC1
  # CMAKE_CXX_FLAGS_RELEASE= /MD /O2 /Ob2 /DNDEBUG

  set(FLAGS_C_DEFAULT /Zi)
  set(FLAGS_C_RELEASE /W3)
  set(FLAGS_C_DEBUG /W4)
  set(FLAGS_CXX_DEFAULT /Zi)
  set(FLAGS_CXX_RELEASE /W3)
  set(FLAGS_CXX_DEBUG /W4)

endif ()

#====================================================
# build
#====================================================

add_library(colorer_lib STATIC ${SRC_COLORER} ${SRC_MALLOC})

FIND_PACKAGE(Spdlog REQUIRED)
FIND_PACKAGE(XercesC REQUIRED)


target_include_directories(colorer_lib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	PUBLIC ${SPDLOG_INCLUDE_DIR}
	PUBLIC ${XERCESC_INCLUDE_DIR}
    )

target_link_libraries(colorer_lib
    PUBLIC ${XERCESC_LIBRARIES}
    ${THREAD_LIBS}
    )

target_compile_definitions(colorer_lib PRIVATE
    ${SPDLOG_DEFINES}
    ${COLORER_DEF}
)

#set_target_properties(colorer_lib PROPERTIES
#    CXX_STANDARD 17
#    CXX_STANDARD_REQUIRED YES
#    CXX_EXTENSIONS NO
#    )

# set options for target
set(MAIN_FLAGS_CXX_DEBUG ${FLAGS_CXX_DEBUG} ${FLAGS_CXX_DEFAULT})
set(MAIN_FLAGS_CXX_RELEASE ${FLAGS_CXX_RELEASE} ${FLAGS_CXX_DEFAULT})
set(MAIN_FLAGS_C_DEBUG ${FLAGS_C_DEBUG} ${FLAGS_C_DEFAULT})
set(MAIN_FLAGS_C_RELEASE ${FLAGS_C_RELEASE} ${FLAGS_C_DEFAULT})

if (${CMAKE_BUILD_TYPE} MATCHES Debug)
  target_compile_options(colorer_lib
      PUBLIC $<$<COMPILE_LANGUAGE:C>:${MAIN_FLAGS_C_DEBUG}>
      PUBLIC $<$<COMPILE_LANGUAGE:CXX>:${MAIN_FLAGS_CXX_DEBUG}>
      )
else ()
  target_compile_options(colorer_lib
      PUBLIC $<$<COMPILE_LANGUAGE:C>:${MAIN_FLAGS_C_RELEASE}>
      PUBLIC $<$<COMPILE_LANGUAGE:CXX>:${MAIN_FLAGS_CXX_RELEASE}>
      )
endif ()

set_target_properties(colorer_lib PROPERTIES
    OUTPUT_NAME "colorer"
    VERSION ${COLORER_VERSION}
    SOVERSION ${COLORER_VERSION_MAJOR}
    DEBUG_POSTFIX "d"
    )
#====================================================
# test
#====================================================

# static analyze
if (COLORER_PVS_STUDIO_CHECK)
  pvs_studio_add_target(TARGET ${PROJECT_NAME}.analyze ALL
      ANALYZE ${PROJECT_NAME}
      MODE GA 64 OP CS
      CFG_TEXT "exclude-path=*/external/*"
      OUTPUT FORMAT errorfile
      LOG ${PROJECT_NAME}.err)
endif ()

