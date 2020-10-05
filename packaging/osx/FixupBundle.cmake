include(BundleUtilities)

# Patching CMake's set_bundle_key_values function to correctly fix up nested executables (e.g. brokers)
# By default CMake assumes that all executables are contained in Contents/MacOS folder, so fixing up them doesn't work
# If you're able to understand the code below, you're genius
function(set_bundle_key_values keys_var context item exepath dirs copyflag)
    # context is the full path to binary for which dependencies are calculated
    # item is particular dependency

    get_item_key(${item} key)
    get_filename_component(item_name "${item}" NAME)

    # process only executables in MacOS folder
    is_file_executable("${context}" is_executable)
    if (${is_executable} AND (context MATCHES "Contents/MacOS/") AND NOT ("${item}" STREQUAL "${context}"))
        set(is_processing 1)
    else ()
        set(is_processing 0)
    endif ()

    if (is_processing)
        # exepath also should be rewritten to point to nested folder
        get_filename_component(exepath "${context}" DIRECTORY)
    else ()
        get_filename_component(exepath "${executable}" PATH)
    endif ()

    _set_bundle_key_values(${keys_var} "${context}" "${item}" "${exepath}" "${dirs}" ${copyflag})

    set(default_embedded_path ${${key}_DEFAULT_EMBEDDED_PATH})
    set(resolved_embedded_item ${${key}_RESOLVED_EMBEDDED_ITEM})
    set(embedded_item ${${key}_EMBEDDED_ITEM})

    if (is_processing)
        # calculate relative path for dependency (@executable_path/..(/..)/Frameworks)
        string(REPLACE "${APP_INSTALL_DIR}/Contents/MacOS" "" relative_path "${exepath}")
        string(REGEX REPLACE "[^/]+" ".." relative_path "${relative_path}")
        set(default_embedded_path "@executable_path${relative_path}/../Frameworks")
        set(embedded_item "${default_embedded_path}/${item_name}")
        string(REPLACE "@executable_path" "${exepath}" resolved_embedded_item "${embedded_item}")
        get_filename_component(resolved_embedded_item "${resolved_embedded_item}" ABSOLUTE)
        if (NOT copyflag)
            set(resolved_embedded_item "${resolved_item}")
        endif ()
    endif ()

    # setting up back all variables to parent scope
    set(exepath ${exepath} PARENT_SCOPE)
    set(${keys_var} ${${keys_var}} PARENT_SCOPE)
    set(${key}_ITEM "${${key}_ITEM}" PARENT_SCOPE)
    set(${key}_RESOLVED_ITEM "${${key}_RESOLVED_ITEM}" PARENT_SCOPE)
    set(${key}_DEFAULT_EMBEDDED_PATH "${default_embedded_path}" PARENT_SCOPE)
    set(${key}_EMBEDDED_ITEM "${embedded_item}" PARENT_SCOPE)
    set(${key}_RESOLVED_EMBEDDED_ITEM "${resolved_embedded_item}" PARENT_SCOPE)
    set(${key}_COPYFLAG "${${key}_COPYFLAG}" PARENT_SCOPE)
    set(${key}_RPATHS "${${key}_RPATHS}" PARENT_SCOPE)
    set(${key}_RDEP_RPATHS "${${key}_RDEP_RPATHS}" PARENT_SCOPE)
endfunction()

set(BU_CHMOD_BUNDLE_ITEMS TRUE)
set(APP_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/@APP_NAME@.app")
file(GLOB_RECURSE PLUGINS "${APP_INSTALL_DIR}/**/*.far-plug*")
fixup_bundle("${APP_INSTALL_DIR}" "${PLUGINS}" "" IGNORE_ITEM "python;python3;python3.8;Python;.Python")
