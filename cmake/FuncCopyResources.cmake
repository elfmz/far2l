# Function to copy auxiliary files next to a target and track their changes.
#
# ARGN:
#   TARGET_NAME                 - The name of the CMake target (e.g., my_app).
#   AUX_FILES_VAR               - The NAME of the CMake list variable containing relative paths to auxiliary files.
#   AUX_FILES_BASE_SOURCE_DIR   - The base directory in your source tree from which the paths in AUX_FILES_VAR are relative.
#   AUX_FILES_DESTINATION_SUBDIR- A subdirectory within the target's output directory where files will be copied.
#
function(setup_target_auxiliary_files TARGET_NAME AUX_FILES_VAR AUX_FILES_BASE_SOURCE_DIR AUX_FILES_DESTINATION_SUBDIR)

    if(NOT TARGET_NAME)
        message(FATAL_ERROR "setup_target_auxiliary_files: TARGET_NAME is required.")
    endif()
    if(NOT AUX_FILES_VAR)
        message(FATAL_ERROR "setup_target_auxiliary_files: AUX_FILES_VAR (name of the list variable) is required.")
    endif()
    if(NOT AUX_FILES_BASE_SOURCE_DIR)
        message(FATAL_ERROR "setup_target_auxiliary_files: AUX_FILES_BASE_SOURCE_DIR is required.")
    endif()
    if(NOT EXISTS "${AUX_FILES_BASE_SOURCE_DIR}")
        message(FATAL_ERROR "setup_target_auxiliary_files: AUX_FILES_BASE_SOURCE_DIR '${AUX_FILES_BASE_SOURCE_DIR}' does not exist.")
    endif()
    if(NOT AUX_FILES_DESTINATION_SUBDIR)
        message(FATAL_ERROR "setup_target_auxiliary_files: AUX_FILES_DESTINATION_SUBDIR is required.")
    endif()

    if(NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "setup_target_auxiliary_files: Target '${TARGET_NAME}' does not exist. Please define it before calling this function.")
    endif()

    set(AUX_SOURCE_FILES_RELATIVE_PATHS ${${AUX_FILES_VAR}})

    if(NOT AUX_SOURCE_FILES_RELATIVE_PATHS)
        message(STATUS "setup_target_auxiliary_files: No auxiliary files provided for target '${TARGET_NAME}' via variable '${AUX_FILES_VAR}'.")
        return()
    endif()

    set(COPIED_AUX_FILES_OUTPUTS "")

    foreach(AUX_FILE_RELATIVE_PATH ${AUX_SOURCE_FILES_RELATIVE_PATHS})
        set(SOURCE_FULL_PATH "${AUX_FILES_BASE_SOURCE_DIR}/${AUX_FILE_RELATIVE_PATH}")

        if(NOT EXISTS "${SOURCE_FULL_PATH}")
            message(WARNING "setup_target_auxiliary_files: Source auxiliary file '${SOURCE_FULL_PATH}' does not exist for target '${TARGET_NAME}'. Skipping.")
            continue()
        endif()


        set(FINAL_DESTINATION_PATH "${AUX_FILES_DESTINATION_SUBDIR}/${AUX_FILE_RELATIVE_PATH}")
        set(MKDIR_BASE_PATH "${AUX_FILES_DESTINATION_SUBDIR}")

        get_filename_component(AUX_FILE_SUBDIR ${AUX_FILE_RELATIVE_PATH} DIRECTORY)
        if(AUX_FILE_SUBDIR AND NOT AUX_FILE_SUBDIR STREQUAL ".")
            set(FULL_DIR_TO_CREATE "${MKDIR_BASE_PATH}/${AUX_FILE_SUBDIR}")
        else()
            set(FULL_DIR_TO_CREATE "${MKDIR_BASE_PATH}")
        endif()

        add_custom_command(
                OUTPUT "${FINAL_DESTINATION_PATH}"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${FULL_DIR_TO_CREATE}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${SOURCE_FULL_PATH}"
                "${FINAL_DESTINATION_PATH}"
                DEPENDS "${SOURCE_FULL_PATH}"
                COMMENT "Copying aux file for ${TARGET_NAME}: ${AUX_FILE_RELATIVE_PATH}"
                VERBATIM
        )
        list(APPEND COPIED_AUX_FILES_OUTPUTS "${FINAL_DESTINATION_PATH}")
    endforeach()

    if(COPIED_AUX_FILES_OUTPUTS)
        set(AUX_FILES_CUSTOM_TARGET_NAME "copy_aux_files_for_${TARGET_NAME}")
        add_custom_target(${AUX_FILES_CUSTOM_TARGET_NAME} ALL
                DEPENDS ${COPIED_AUX_FILES_OUTPUTS}
                COMMENT "Ensuring auxiliary files for ${TARGET_NAME} are copied"
        )
        # cycle dependecy
        # add_dependencies(${TARGET_NAME} ${AUX_FILES_CUSTOM_TARGET_NAME})
    endif()

    message(STATUS "Setup auxiliary files copying for target '${TARGET_NAME}'.")
endfunction()