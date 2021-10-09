project(WinPort)

execute_process(COMMAND sh -c
	"find '${CMAKE_CURRENT_SOURCE_DIR}' -name '*.cpp' -or -name '*.c' -or -name '*.h' -or -name '*.hpp' |sort|xargs shasum|shasum"
	OUTPUT_VARIABLE SOURCES_HASH)

execute_process(COMMAND sh -c
	"${CMAKE_CXX_COMPILER} --version|shasum"
	OUTPUT_VARIABLE CXX_VER_HASH)

execute_process(COMMAND sh -c
	"echo '${SOURCES_HASH}' '${CXX_VER_HASH}' '${CMAKE_CXX_FLAGS_RELEASE}'|shasum|awk '{ printf(\"%s\", $1) }'"
	OUTPUT_VARIABLE ABI_HASH)

if (ABI_HASH STREQUAL "")
	message(FATAL_ERROR "Failed to calculate ABI_HASH. Make sure following tools are available: find, sort, xargs, shasum, awk")
endif()

message(STATUS "ABI_HASH: '${ABI_HASH}'")

if (NOT ${USEWX})
	message(STATUS "Building without GUI/wxWidgets backend due to USEWX=${USEWX}")
else()
	message(STATUS "Building with GUI/wxWidgets backend")
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
