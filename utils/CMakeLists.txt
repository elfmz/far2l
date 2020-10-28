
project(utils)

find_package(PkgConfig REQUIRED)

set(SOURCES
  src/Threaded.cpp
  src/SharedResource.cpp
  src/KeyFileHelper.cpp
  src/utils.cpp
  src/InstallPath.cpp
  src/StrPrintf.cpp
  src/TimeUtils.cpp
  src/StringConfig.cpp
  src/WordExpansion.cpp
  src/InMy.cpp
  src/ZombieControl.cpp
  src/base64.cpp
  src/Event.cpp
  src/StackSerializer.cpp
  src/ScopeHelpers.cpp
  src/ConvertUTF.c
  src/crc64.c
  src/ini.c
  src/TTYRawMode.cpp
  src/LocalSocket.cpp
)

add_library (utils ${SOURCES})


target_include_directories(utils PRIVATE include)
target_include_directories(utils PRIVATE ../WinPort)

find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
  target_compile_options(utils PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(utils "${CMAKE_THREAD_LIBS_INIT}")
endif()
