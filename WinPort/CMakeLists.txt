
project(WinPort)


find_package(PkgConfig REQUIRED)
find_package(wxWidgets REQUIRED net gl core base)

pkg_check_modules(GLIB2 glib-2.0)

include(${wxWidgets_USE_FILE})
include_directories(${GLIB2_INCLUDE_DIRS})
include_directories(wxWidgets_INCLUDE_DIRS)


add_subdirectory(wineguts)
file(GLOB_RECURSE SOURCES "*.cpp")

add_library (WinPort ${SOURCES})

target_link_libraries(WinPort wineguts ${wxWidgets_LIBRARIES} ${GLIB2_LIBRARIES})

