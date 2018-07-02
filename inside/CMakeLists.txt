project(inside)

set(SOURCES
src/inside.cpp
src/Globals.cpp
src/PluginImpl.cpp

src/pdf/PluginImplPDF.cpp

src/pdf/pdf2text/AcroForm.cc
src/pdf/pdf2text/Annot.cc
src/pdf/pdf2text/Array.cc
src/pdf/pdf2text/BuiltinFont.cc
src/pdf/pdf2text/BuiltinFontTables.cc
src/pdf/pdf2text/Catalog.cc
src/pdf/pdf2text/CharCodeToUnicode.cc
src/pdf/pdf2text/CMap.cc
src/pdf/pdf2text/Decrypt.cc
src/pdf/pdf2text/Dict.cc
src/pdf/pdf2text/DisplayState.cc
src/pdf/pdf2text/Error.cc
src/pdf/pdf2text/FontEncodingTables.cc
src/pdf/pdf2text/Form.cc
src/pdf/pdf2text/Function.cc
src/pdf/pdf2text/Gfx.cc
src/pdf/pdf2text/GfxFont.cc
src/pdf/pdf2text/GfxState.cc
src/pdf/pdf2text/GlobalParams.cc
src/pdf/pdf2text/Lexer.cc
src/pdf/pdf2text/Link.cc
src/pdf/pdf2text/NameToCharCode.cc
src/pdf/pdf2text/Object.cc
src/pdf/pdf2text/OptionalContent.cc
src/pdf/pdf2text/Outline.cc
src/pdf/pdf2text/OutputDev.cc
src/pdf/pdf2text/Page.cc
src/pdf/pdf2text/Parser.cc
src/pdf/pdf2text/PDFCore.cc
src/pdf/pdf2text/PDFDoc.cc
src/pdf/pdf2text/PDFDocEncoding.cc
src/pdf/pdf2text/pdftotext.cc
src/pdf/pdf2text/PSTokenizer.cc
src/pdf/pdf2text/SecurityHandler.cc
src/pdf/pdf2text/Stream.cc
src/pdf/pdf2text/TextOutputDev.cc
src/pdf/pdf2text/TextString.cc
src/pdf/pdf2text/TileCache.cc
src/pdf/pdf2text/TileCompositor.cc
src/pdf/pdf2text/TileMap.cc
src/pdf/pdf2text/UnicodeMap.cc
src/pdf/pdf2text/UnicodeTypeTable.cc
src/pdf/pdf2text/UTF8.cc
src/pdf/pdf2text/XFAForm.cc
src/pdf/pdf2text/XRef.cc
src/pdf/pdf2text/Zoox.cc

src/pdf/pdf2text/fofi/FoFiBase.cc
src/pdf/pdf2text/fofi/FoFiEncodings.cc
src/pdf/pdf2text/fofi/FoFiIdentifier.cc
src/pdf/pdf2text/fofi/FoFiTrueType.cc
src/pdf/pdf2text/fofi/FoFiType1.cc
src/pdf/pdf2text/fofi/FoFiType1C.cc

src/pdf/pdf2text/goo/FixedPoint.cc
src/pdf/pdf2text/goo/GHash.cc
src/pdf/pdf2text/goo/GList.cc
src/pdf/pdf2text/goo/GString.cc
src/pdf/pdf2text/goo/gfile.cc
src/pdf/pdf2text/goo/gmem.cc
src/pdf/pdf2text/goo/gmempp.cc
src/pdf/pdf2text/goo/parseargs.c

src/pdf/pdf2text/splash/Splash.cc
src/pdf/pdf2text/splash/SplashBitmap.cc
src/pdf/pdf2text/splash/SplashClip.cc
src/pdf/pdf2text/splash/SplashFTFont.cc
src/pdf/pdf2text/splash/SplashFTFontEngine.cc
src/pdf/pdf2text/splash/SplashFTFontFile.cc
src/pdf/pdf2text/splash/SplashFont.cc
src/pdf/pdf2text/splash/SplashFontEngine.cc
src/pdf/pdf2text/splash/SplashFontFile.cc
src/pdf/pdf2text/splash/SplashFontFileID.cc
src/pdf/pdf2text/splash/SplashPath.cc
src/pdf/pdf2text/splash/SplashPattern.cc
src/pdf/pdf2text/splash/SplashScreen.cc
src/pdf/pdf2text/splash/SplashState.cc
src/pdf/pdf2text/splash/SplashXPath.cc
src/pdf/pdf2text/splash/SplashXPathScanner.cc

)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(SOURCES
		${SOURCES}
		src/elf/Dumper.cpp
		src/elf/Storage.cpp
		src/elf/PluginImplELF.cpp
	)
endif()




add_library (inside MODULE ${SOURCES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS "${CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS} -flat_namespace -undefined suppress")
endif()

target_link_libraries(inside utils FARStdlib)

target_include_directories(inside PRIVATE src)
target_include_directories(inside PRIVATE src/elf)
target_include_directories(inside PRIVATE src/pdf)
target_include_directories(inside PRIVATE src/pdf/pdf2text)
target_include_directories(inside PRIVATE src/pdf/pdf2text/fofi)
target_include_directories(inside PRIVATE src/pdf/pdf2text/goo)
target_include_directories(inside PRIVATE src/pdf/pdf2text/splash)
target_include_directories(inside PRIVATE ../FARStdlib/include)
target_include_directories(inside PRIVATE ../WinPort)
target_include_directories(inside PRIVATE ../far2l)
target_include_directories(inside PRIVATE ../far2l/Include)

target_compile_definitions(inside PRIVATE)

set_target_properties(inside
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/inside/plug"
    PREFIX ""
    SUFFIX ".far-plug-mb")

add_custom_command(TARGET inside POST_BUILD
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/inside")
