
add_subdirectory(codepages)

file(GLOB_RECURSE SOURCES "*.c")

add_library (wineguts ${SOURCES})

