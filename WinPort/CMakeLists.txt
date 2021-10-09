project(WinPort)

execute_process(COMMAND sh -c
	"find '${CMAKE_CURRENT_SOURCE_DIR}' -name '*.cpp' -or -name '*.c' -or -name '*.h' -or -name '*.hpp' |sort|xargs md5sum|md5sum"
	OUTPUT_VARIABLE SOURCES_HASH)

execute_process(COMMAND sh -c
	"${CMAKE_CXX_COMPILER} --version|md5sum"
	OUTPUT_VARIABLE CXX_VER_HASH)

execute_process(COMMAND sh -c
	"echo '${SOURCES_HASH}' '${CXX_VER_HASH}' '${CMAKE_CXX_FLAGS_RELEASE}'|md5sum|awk '{ printf(\"%s\", $1) }'"
	OUTPUT_VARIABLE ABI_HASH)

if (ABI_HASH STREQUAL "")
	message(FATAL_ERROR "Failed to calculate ABI_HASH. Make sure following tools are available: find, sort, xargs, md5sum, awk")
endif()

message(STATUS "ABI_HASH: '${ABI_HASH}'")

if (NOT ${USEWX})
	message(STATUS "Building without GUI/wxWidgets backend due to USEWX=${USEWX}")
	set(USEWX_INCS "")
	set(USEWX_LIBS "")
	set(USEWX_CXX_FLAGS "")
	set(USEWX_USEFILE "")
else()
	message(STATUS "Building with GUI/wxWidgets backend")
	if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		if (DEFINED wxWidgets_CONFIG_EXECUTABLE)
			message(STATUS "Using pre-defined wx config: ${wxWidgets_CONFIG_EXECUTABLE}")

		elseif(EXISTS "/usr/local/bin/wxgtk3u-3.1-config")
			set(wxWidgets_CONFIG_EXECUTABLE "/usr/local/bin/wxgtk3u-3.1-config")
			set(wxWidgets_wxrc_EXECUTABLE "/usr/local/bin/wxrc-gtk3u-3.1")
			message(STATUS "Using known location of wx-config 3.1")

		elseif (EXISTS "/usr/local/bin/wxgtk3u-3.0-config")
			set(wxWidgets_CONFIG_EXECUTABLE "/usr/local/bin/wxgtk3u-3.0-config")
			set(wxWidgets_wxrc_EXECUTABLE "/usr/local/bin/wxrc-gtk3u-3.0")
			message(STATUS "Using known location of wx-config 3.0")

		else()
			message(STATUS "No wxgtk3u-3.*-config in expected locations")
		endif()
	elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		if (EXISTS "/opt/local/Library/Frameworks/wxWidgets.framework/Versions/wxWidgets/3.1/bin/wx-config")
			# MacPorts
			set(wxWidgets_CONFIG_EXECUTABLE "/opt/local/Library/Frameworks/wxWidgets.framework/Versions/wxWidgets/3.1/bin/wx-config")
			set(wxWidgets_wxrc_EXECUTABLE "/opt/local/Library/Frameworks/wxWidgets.framework/Versions/wxWidgets/3.1/bin/wxrc")
			message(STATUS "Using known location of wx-config 3.1")
		else()
			message(STATUS "No wx-config in expected locations")
		endif()
	endif()

	if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		find_package(wxWidgets REQUIRED core base adv)
	else()
		find_package(wxWidgets REQUIRED core base)
	endif()

	include(${wxWidgets_USE_FILE})

	set(USEWX_USEFILE ${wxWidgets_USE_FILE})
	set(USEWX_INCS ${wxWidgets_INCLUDE_DIRS})
	set(USEWX_LIBS ${wxWidgets_LIBRARIES})
	set(USEWX_CXX_FLAGS ${wxWidgets_CXX_FLAGS})
	add_subdirectory(src/Backend/WX)
endif()

add_subdirectory(wineguts)

set(SOURCES
src/APIClipboard.cpp
src/APIConsole.cpp
src/APIFiles.cpp
src/APIKeyboard.cpp
src/APIMemory.cpp
src/APIOther.cpp
src/APIPrintFormat.cpp
src/APIRegistry.cpp
src/APIStringCodepages.cpp
src/APIStringMap.cpp
src/APITime.cpp
src/ConsoleBuffer.cpp
src/ConsoleInput.cpp
src/ConsoleOutput.cpp
src/WinPortHandle.cpp
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

add_library (WinPort ${SOURCES})

target_include_directories(WinPort PRIVATE .)
target_include_directories(WinPort PRIVATE src)
target_include_directories(WinPort PRIVATE src/Backend)

target_link_libraries(WinPort wineguts utils)

find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
  target_compile_options(WinPort PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(WinPort "${CMAKE_THREAD_LIBS_INIT}")
endif()

target_compile_definitions(WinPort PUBLIC -DABI_HASH=${ABI_HASH})
