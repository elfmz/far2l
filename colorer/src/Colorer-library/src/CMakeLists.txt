#====================================================
# colorer source
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
    colorer/common/platform.h
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

if(COLORER_USE_JARINPUTSOURCE)
    set(COLORER_FEATURE_JARINPUTSOURCE 1)
    set(SRC_COLORER_JAR
        colorer/xml/MemoryFile.cpp
        colorer/xml/MemoryFile.h
        colorer/xml/ZipXmlInputSource.cpp
        colorer/xml/ZipXmlInputSource.h
    )
endif()

if (COLORER_USE_DUMMY_LOGGER)
    set(COLORER_FEATURE_DUMMYLOGGER 1)
endif ()

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
set(COLORER_DEF -DNOGDI)
if (MSVC)
  set(COLORER_DEF ${COLORER_DEF} -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
endif ()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  # default for gcc
  # CMAKE_CXX_FLAGS=
  # CMAKE_CXX_FLAGS_DEBUG= -g
  # CMAKE_CXX_FLAGS_RELEASE= -O3 -DNDEBUG

  # set only new flags
  set(FLAGS_CXX_DEFAULT -fpermissive -Wall)
  set(FLAGS_CXX_RELEASE)
  set(FLAGS_CXX_DEBUG)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")

  # default for msvc
  # CMAKE_CXX_FLAGS= /DWIN32 /D_WINDOWS /GR /EHsc
  # CMAKE_CXX_FLAGS_DEBUG= /MDd /Zi /Ob0 /Od /RTC1
  # CMAKE_CXX_FLAGS_RELEASE= /MD /O2 /Ob2 /DNDEBUG

  set(FLAGS_CXX_DEFAULT)
  set(FLAGS_CXX_RELEASE /W3 /WX)
  set(FLAGS_CXX_DEBUG /W4)
endif()

set(MAIN_FLAGS_CXX_DEBUG ${FLAGS_CXX_DEBUG} ${FLAGS_CXX_DEFAULT})
set(MAIN_FLAGS_CXX_RELEASE ${FLAGS_CXX_RELEASE} ${FLAGS_CXX_DEFAULT})
set(MAIN_FLAGS_C_DEBUG ${FLAGS_C_DEBUG} ${FLAGS_C_DEFAULT})
set(MAIN_FLAGS_C_RELEASE ${FLAGS_C_RELEASE} ${FLAGS_C_DEFAULT})
#====================================================
# build
#====================================================

add_library(colorer_lib STATIC ${SRC_COLORER} ${SRC_COLORER_JAR})
target_include_directories(colorer_lib PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
if (COLORER_USE_DEPENDS_FROM_SYSTEM)
    target_link_libraries(colorer_lib
        PUBLIC XercesC::XercesC
    )
else ()
    target_include_directories(colorer_lib
        PUBLIC ../external/xercesc/src
        PUBLIC ${xerces-c_BINARY_DIR}/src
    )
    target_link_libraries(colorer_lib
        PUBLIC xerces-c
    )
endif ()

if (NOT COLORER_USE_DUMMY_LOGGER)
    if (NOT COLORER_USE_DEPENDS_FROM_SYSTEM)
        target_include_directories(colorer_lib
            PUBLIC ../external/spdlog/include
        )
        target_link_libraries(colorer_lib
            PUBLIC ${THREAD_LIBS}
            PUBLIC spdlog::spdlog
        )
    else ()
        target_link_libraries(colorer_lib
            PUBLIC spdlog::spdlog
        )
    endif ()
endif ()

if(COLORER_USE_JARINPUTSOURCE)
    if(COLORER_USE_DEPENDS_FROM_SYSTEM)
        target_link_libraries(colorer_lib PUBLIC minizip::minizip)
    else()
        target_link_libraries(colorer_lib PUBLIC ZLIB::MINIZIP)
        target_include_directories(colorer_lib
            PUBLIC ../external/zlib/contrib
        )
    endif()
endif()

target_compile_definitions(colorer_lib PUBLIC ${COLORER_DEF})
set_target_properties(colorer_lib PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    )

# set options for target
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
