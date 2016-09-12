
project(farlng)

file(GLOB_RECURSE SOURCES "*.cpp")


find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB2 glib-2.0)

add_executable (farlng ${SOURCES})

target_include_directories(farlng SYSTEM PRIVATE ${GLIB2_INCLUDE_DIRS})
target_include_directories(farlng PRIVATE ../WinPort)

target_link_libraries (farlng WinPort utils ${wxWidgets_LIBRARIES} ${GLIB2_LIBRARIES} dl)
