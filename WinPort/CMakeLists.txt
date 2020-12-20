
project(WinPort)

if (${USEWX})
	include(${wxWidgets_USE_FILE})
endif()

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

	if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		set(SOURCES
			${SOURCES}
			src/Backend/WX/Mac/touchbar.mm
			src/Backend/WX/Mac/dockicon.cpp
		)
	endif()
endif()


add_library (WinPort ${SOURCES})

if (${USEWX} AND ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set_property (TARGET WinPort APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc")
endif()


if (NOT ${USEWX})
	target_compile_options(WinPort PUBLIC "-DNOWX")
endif()


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
