
project(WinPort)

find_package(PkgConfig REQUIRED)

if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    set(wxWidgets_CONFIG_EXECUTABLE "/usr/local/bin/wxgtk3u-3.0-config")
    set(wxWidgets_wxrc_EXECUTABLE "/usr/local/bin/wxrc-gtk3u-3.0")
endif()

if (${USEWX})
	find_package(wxWidgets REQUIRED net core base)
	include(${wxWidgets_USE_FILE})
endif()

pkg_check_modules(GLIB2 glib-2.0)


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
src/SavedScreen.cpp
src/sudo/sudo_common.cpp
src/sudo/sudo_client.cpp
src/sudo/sudo_client_api.cpp
src/sudo/sudo_dispatcher.cpp
src/sudo/sudo_askpass.cpp
src/sudo/sudo_askpass_ipc.cpp
src/Backend/WinPortMain.cpp
src/Backend/SudoAskpassImpl.cpp
src/Backend/FSClipboardBackend.cpp
src/Backend/TTY/TTYBackend.cpp
src/Backend/TTY/TTYRevive.cpp
src/Backend/TTY/TTYInput.cpp
src/Backend/TTY/TTYInputSequenceParser.cpp
src/Backend/TTY/TTYOutput.cpp
src/Backend/TTY/TTYFar2lClipboardBackend.cpp
src/Backend/TTY/TTYNegotiateFar2l.cpp
)

if (${USEWX})
	set(SOURCES
		${SOURCES}
		src/Backend/WX/Paint.cpp
		src/Backend/WX/CustomDrawChar.cpp
		src/Backend/WX/wxMain.cpp
		src/Backend/WX/wxClipboardBackend.cpp
		src/Backend/WX/ExclusiveHotkeys.cpp
		src/Backend/WX/wxWinTranslations.cpp
		)
endif()


add_library (WinPort ${SOURCES})

if (NOT ${USEWX})
	target_compile_options(WinPort PUBLIC "-DNOWX")
endif()


target_include_directories(WinPort PRIVATE ${GLIB2_INCLUDE_DIRS})
target_include_directories(WinPort PRIVATE ${USEWX_INCS})

target_include_directories(WinPort PRIVATE .)
target_include_directories(WinPort PRIVATE src)
target_include_directories(WinPort PRIVATE src/Backend)

target_link_libraries(WinPort wineguts utils ${USEWX_LIBS})

find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
  target_compile_options(WinPort PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(WinPort "${CMAKE_THREAD_LIBS_INIT}")
endif()


if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(WinPort glib-2.0 -L/usr/local/lib)
else()
    target_link_libraries(WinPort ${GLIB2_LIBRARIES})
endif()
