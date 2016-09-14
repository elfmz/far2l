
project(WinPort)


find_package(PkgConfig REQUIRED)
find_package(wxWidgets REQUIRED net core base)

pkg_check_modules(GLIB2 glib-2.0)


include(${wxWidgets_USE_FILE})

add_subdirectory(wineguts)

set(SOURCES
src/APIClipboard.cpp
src/APIConsole.cpp
src/APIFiles.cpp
src/APIFSNotify.cpp
src/APIKeyboard.cpp
src/APIMemory.cpp
src/APIOther.cpp
src/APIPrintFormat.cpp
src/APIRegistry.cpp
src/APIStringCodepages.cpp
src/APIStringMap.cpp
src/APISynch.cpp
src/APITime.cpp
src/ConsoleBuffer.cpp
src/ConsoleInput.cpp
src/ConsoleOutput.cpp
src/WinPortHandle.cpp
src/WinPortSynch.cpp
src/ConvertUTF.c
src/PathHelpers.cpp
src/UI/Paint.cpp
src/UI/Main.cpp
src/UI/wxWinTranslations.cpp
 src/ConvertUTF.c
)

add_library (WinPort ${SOURCES})

target_include_directories(WinPort PRIVATE ${GLIB2_INCLUDE_DIRS})
target_include_directories(WinPort PRIVATE ${wxWidgets_INCLUDE_DIRS})

target_include_directories(WinPort PRIVATE .)
target_include_directories(WinPort PRIVATE src/UI)
target_include_directories(WinPort PRIVATE src)

target_link_libraries(WinPort wineguts utils ${wxWidgets_LIBRARIES} ${GLIB2_LIBRARIES})

