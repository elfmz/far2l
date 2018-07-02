project(inside)

set(SOURCES
src/inside.cpp
src/Globals.cpp
src/PluginImpl.cpp

src/pdf/PluginImplPDF.cpp

src/pdf/xpdf/AcroForm.cc
src/pdf/xpdf/Annot.cc
src/pdf/xpdf/Array.cc
src/pdf/xpdf/BuiltinFont.cc
src/pdf/xpdf/BuiltinFontTables.cc
src/pdf/xpdf/Catalog.cc
src/pdf/xpdf/CharCodeToUnicode.cc
src/pdf/xpdf/CMap.cc
src/pdf/xpdf/Decrypt.cc
src/pdf/xpdf/Dict.cc
src/pdf/xpdf/DisplayState.cc
src/pdf/xpdf/Error.cc
src/pdf/xpdf/FontEncodingTables.cc
src/pdf/xpdf/Form.cc
src/pdf/xpdf/Function.cc
src/pdf/xpdf/Gfx.cc
src/pdf/xpdf/GfxFont.cc
src/pdf/xpdf/GfxState.cc
src/pdf/xpdf/GlobalParams.cc
src/pdf/xpdf/Lexer.cc
src/pdf/xpdf/Link.cc
src/pdf/xpdf/NameToCharCode.cc
src/pdf/xpdf/Object.cc
src/pdf/xpdf/OptionalContent.cc
src/pdf/xpdf/Outline.cc
src/pdf/xpdf/OutputDev.cc
src/pdf/xpdf/Page.cc
src/pdf/xpdf/Parser.cc
src/pdf/xpdf/PDFCore.cc
src/pdf/xpdf/PDFDoc.cc
src/pdf/xpdf/PDFDocEncoding.cc
src/pdf/xpdf/pdftotext.cc
src/pdf/xpdf/PSTokenizer.cc
src/pdf/xpdf/SecurityHandler.cc
src/pdf/xpdf/Stream.cc
src/pdf/xpdf/TextOutputDev.cc
src/pdf/xpdf/TextString.cc
src/pdf/xpdf/TileCache.cc
src/pdf/xpdf/TileCompositor.cc
src/pdf/xpdf/TileMap.cc
src/pdf/xpdf/UnicodeMap.cc
src/pdf/xpdf/UnicodeTypeTable.cc
src/pdf/xpdf/UTF8.cc
src/pdf/xpdf/XFAForm.cc
src/pdf/xpdf/XRef.cc
src/pdf/xpdf/Zoox.cc

src/pdf/xpdf/fofi/FoFiBase.cc
src/pdf/xpdf/fofi/FoFiEncodings.cc
src/pdf/xpdf/fofi/FoFiIdentifier.cc
src/pdf/xpdf/fofi/FoFiTrueType.cc
src/pdf/xpdf/fofi/FoFiType1.cc
src/pdf/xpdf/fofi/FoFiType1C.cc

src/pdf/xpdf/goo/FixedPoint.cc
src/pdf/xpdf/goo/GHash.cc
src/pdf/xpdf/goo/GList.cc
src/pdf/xpdf/goo/GString.cc
src/pdf/xpdf/goo/gfile.cc
src/pdf/xpdf/goo/gmem.cc
src/pdf/xpdf/goo/gmempp.cc
src/pdf/xpdf/goo/parseargs.c

src/pdf/xpdf/splash/Splash.cc
src/pdf/xpdf/splash/SplashBitmap.cc
src/pdf/xpdf/splash/SplashClip.cc
src/pdf/xpdf/splash/SplashFTFont.cc
src/pdf/xpdf/splash/SplashFTFontEngine.cc
src/pdf/xpdf/splash/SplashFTFontFile.cc
src/pdf/xpdf/splash/SplashFont.cc
src/pdf/xpdf/splash/SplashFontEngine.cc
src/pdf/xpdf/splash/SplashFontFile.cc
src/pdf/xpdf/splash/SplashFontFileID.cc
src/pdf/xpdf/splash/SplashPath.cc
src/pdf/xpdf/splash/SplashPattern.cc
src/pdf/xpdf/splash/SplashScreen.cc
src/pdf/xpdf/splash/SplashState.cc
src/pdf/xpdf/splash/SplashXPath.cc
src/pdf/xpdf/splash/SplashXPathScanner.cc

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
target_include_directories(inside PRIVATE src/pdf/xpdf)
target_include_directories(inside PRIVATE src/pdf/xpdf/fofi)
target_include_directories(inside PRIVATE src/pdf/xpdf/goo)
target_include_directories(inside PRIVATE src/pdf/xpdf/splash)
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
