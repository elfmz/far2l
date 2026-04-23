project(far2l_gui)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(FREETYPE REQUIRED freetype2)
pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)
if(NOT APPLE)
    pkg_check_modules(FONTCONFIG REQUIRED fontconfig)
endif()

set(SOURCES
    SDLMain.cpp
    SDLBoxChars.cpp
    SDLConsoleRenderer.cpp
    SDLShared.cpp
    SDLBackendUtils.cpp
    SDLImageManager.cpp
    SDLFontManager.cpp
    SDLPrinterSupport.cpp
)

if(APPLE)
    list(APPEND SOURCES SDLFontDialog.mm Mac/printing.mm)
else()
    list(APPEND SOURCES SDLFontDialog.cpp)
endif()

add_library(far2l_gui MODULE ${SOURCES})

target_include_directories(far2l_gui PRIVATE ..)
target_include_directories(far2l_gui PRIVATE ../..)
target_include_directories(far2l_gui PRIVATE ../../..)
target_include_directories(far2l_gui PRIVATE ../../../../utils/include)
target_include_directories(far2l_gui PRIVATE ${SDL2_INCLUDE_DIRS})
target_include_directories(far2l_gui PRIVATE ${FREETYPE_INCLUDE_DIRS})
target_include_directories(far2l_gui PRIVATE ${HARFBUZZ_INCLUDE_DIRS})
if(NOT APPLE)
    target_include_directories(far2l_gui PRIVATE ${FONTCONFIG_INCLUDE_DIRS})
endif()

target_link_directories(far2l_gui PRIVATE ${SDL2_LIBRARY_DIRS} ${FREETYPE_LIBRARY_DIRS} ${HARFBUZZ_LIBRARY_DIRS})
if(NOT APPLE)
    target_link_directories(far2l_gui PRIVATE ${FONTCONFIG_LIBRARY_DIRS})
endif()

target_compile_options(far2l_gui PRIVATE ${SDL2_CFLAGS_OTHER})
target_compile_options(far2l_gui PRIVATE ${FREETYPE_CFLAGS_OTHER})
target_compile_options(far2l_gui PRIVATE ${HARFBUZZ_CFLAGS_OTHER})
if(NOT APPLE)
    target_compile_options(far2l_gui PRIVATE ${FONTCONFIG_CFLAGS_OTHER})
endif()

target_link_libraries(far2l_gui utils far2l ${SDL2_LIBRARIES} ${FREETYPE_LIBRARIES} ${HARFBUZZ_LIBRARIES})
if(NOT APPLE)
    target_link_libraries(far2l_gui ${FONTCONFIG_LIBRARIES})
endif()
if(APPLE)
    target_link_libraries(far2l_gui "-framework CoreText" "-framework CoreFoundation" "-framework CoreGraphics" "-framework ImageIO" "-framework AppKit" "-framework UniformTypeIdentifiers" "-framework WebKit")
endif()

find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
    target_compile_options(far2l_gui PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(far2l_gui "${CMAKE_THREAD_LIBS_INIT}")
endif()

set_target_properties(far2l_gui
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}"
        PREFIX ""
        SUFFIX ".so")
