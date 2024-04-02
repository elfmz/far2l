project(WinPort)

add_subdirectory(wineguts)

set(SOURCES
src/APIClipboard.cpp
src/APIConsole.cpp
src/APIFiles.cpp
src/APIKeyboard.cpp
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
src/CustomPanic.cpp
src/PathHelpers.cpp
src/SavedScreen.cpp
src/sudo/sudo_common.cpp
src/sudo/sudo_client.cpp
src/sudo/sudo_client_api.cpp
src/sudo/sudo_dispatcher.cpp
src/sudo/sudo_askpass.cpp
src/sudo/sudo_askpass_ipc.cpp
src/Backend/WinPortMain.cpp
src/Backend/WinPortRGB.cpp
src/Backend/SudoAskpassImpl.cpp
src/Backend/FSClipboardBackend.cpp
src/Backend/ExtClipboardBackend.cpp
src/Backend/TTY/TTYBackend.cpp
src/Backend/TTY/TTYRevive.cpp
src/Backend/TTY/TTYInput.cpp
src/Backend/TTY/TTYInputSequenceParser.cpp
src/Backend/TTY/TTYInputSequenceParserExts.cpp
src/Backend/TTY/TTYOutput.cpp
src/Backend/TTY/TTYFar2lClipboardBackend.cpp
src/Backend/TTY/OSC52ClipboardBackend.cpp
src/Backend/TTY/TTYNegotiateFar2l.cpp
src/Backend/TTY/TTYXGlue.cpp
)

if(TESTING)
    message(STATUS "Building with smoke tests support due to TESTING=${TESTING}")
    set(SOURCES
        ${SOURCES}
        src/Backend/TestController.cpp
    )
else()
    message(STATUS "Building without smoke tests support due to TESTING=${TESTING}")
endif()

add_library (WinPort STATIC ${SOURCES})

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
if(TESTING)
    target_compile_options(WinPort PUBLIC -DTESTING)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Haiku")
    target_link_libraries(WinPort root)
endif()
