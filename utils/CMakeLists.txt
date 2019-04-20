
project(utils)

find_package(PkgConfig REQUIRED)

set(SOURCES
  src/Threaded.cpp
  src/SavedScreen.cpp
  src/SharedResource.cpp
  src/KeyFileHelper.cpp
  src/utils.cpp
  src/InMy.cpp
  src/ZombieControl.cpp
  src/base64.cpp
  src/Event.cpp
  src/StackSerializer.cpp
  src/ScopeHelpers.cpp
  src/ConvertUTF.c
  src/crc64.c
  src/ini.c
)

add_library (utils ${SOURCES})

target_include_directories(utils PRIVATE ${GLIB2_INCLUDE_DIRS})

target_include_directories(utils PRIVATE include)
target_include_directories(utils PRIVATE ../WinPort)
