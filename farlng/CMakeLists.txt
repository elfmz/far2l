
file(GLOB_RECURSE SOURCES "*.cpp")


find_package(PkgConfig REQUIRED)
find_package(wxWidgets REQUIRED net gl core base)

pkg_check_modules(GLIB2 glib-2.0)

include(${wxWidgets_USE_FILE})
include_directories(${GLIB2_INCLUDE_DIRS})
include_directories(wxWidgets_INCLUDE_DIRS)

include_directories(../WinPort)

add_executable (farlng ${SOURCES})

target_link_libraries (farlng WinPort ${wxWidgets_LIBRARIES} ${GLIB2_LIBRARIES} dl)
