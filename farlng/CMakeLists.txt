
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

pkg_check_modules(GLIB2 glib-2.0)

add_executable (farlng ${SOURCES})

target_include_directories(farlng SYSTEM PRIVATE ${GLIB2_INCLUDE_DIRS})
target_include_directories(farlng PRIVATE ../WinPort)

if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    set(wxWidgets_CONFIG_EXECUTABLE "/usr/local/bin/wxgtk3u-3.0-config")
    target_link_libraries (farlng WinPort utils ${wxWidgets_LIBRARIES} c)
else()
    target_link_libraries (farlng WinPort utils ${wxWidgets_LIBRARIES} dl)
endif()
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(farlng glib-2.0 -L/usr/local/lib)
else()
    target_link_libraries(farlng ${GLIB2_LIBRARIES})
endif()
