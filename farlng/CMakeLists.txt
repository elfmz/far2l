
project(farlng)

set(SOURCES
  src/farlng.cpp
  src/lng.common.cpp
  src/lng.convertor.cpp
  src/lng.generator.cpp
  src/lng.inserter.cpp
)

set(CMAKE_CXX_VISIBILITY_PRESET default)
set(CMAKE_C_VISIBILITY_PRESET default)

find_package(PkgConfig REQUIRED)

add_executable (farlng ${SOURCES})

target_include_directories(farlng PRIVATE ../WinPort)

if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    set(wxWidgets_CONFIG_EXECUTABLE "/usr/local/bin/wxgtk3u-3.0-config")
    target_link_libraries (farlng WinPort utils ${wxWidgets_LIBRARIES} c)
else()
    target_link_libraries (farlng WinPort utils ${wxWidgets_LIBRARIES} dl)
endif()
