
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
src/PathHelpers.cpp
src/UI/Paint.cpp
src/UI/Main.cpp
src/UI/wxWinTranslations.cpp
src/sudo/sudo_common.cpp
src/sudo/sudo_client.cpp
src/sudo/sudo_client_api.cpp
src/sudo/sudo_dispatcher.cpp
)

add_library (WinPort ${SOURCES})

target_include_directories(WinPort PRIVATE ${GLIB2_INCLUDE_DIRS})
target_include_directories(WinPort PRIVATE ${wxWidgets_INCLUDE_DIRS})

target_include_directories(WinPort PRIVATE .)
target_include_directories(WinPort PRIVATE src/UI)
target_include_directories(WinPort PRIVATE src)

target_link_libraries(WinPort wineguts utils ${wxWidgets_LIBRARIES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(WinPort glib-2.0 -L/usr/local/lib)
else()
    target_link_libraries(WinPort ${GLIB2_LIBRARIES})
endif()
