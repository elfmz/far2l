project(themes)

# Discover all subdirectories
file(GLOB children RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*")

set(RESOURCE_DIRS "")
set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/themes")

foreach(child ${children})
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${child}")
        list(APPEND RESOURCE_DIRS "${child}")
    endif()
endforeach()

# A single target that copies everything
add_custom_target(copy_themes ALL)

foreach(dir IN LISTS RESOURCE_DIRS)
    add_custom_command(
        TARGET copy_themes
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/${dir}"
                "$<TARGET_FILE_DIR:far2l>/themes/${dir}"
    )
endforeach()
