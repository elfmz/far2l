
file(GLOB_RECURSE SOURCES "*.c")

set(CMAKE_CXX_VISIBILITY_PRESET default)
set(CMAKE_C_VISIBILITY_PRESET default)

add_library (codepages ${SOURCES})

