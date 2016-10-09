
project(utils)

find_package(PkgConfig REQUIRED)

set(SOURCES
  src/KeyFileHelper.cpp
  src/utils.cpp
  src/ZombieControl.cpp
  src/ConvertUTF.c
)

add_library (utils ${SOURCES})

target_include_directories(utils PRIVATE ${GLIB2_INCLUDE_DIRS})

target_include_directories(utils PRIVATE include)
target_include_directories(utils PRIVATE ../WinPort)
