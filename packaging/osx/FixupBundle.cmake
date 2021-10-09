include(BundleUtilities)

# By default CMake assumes that all executables are contained in Contents/MacOS folder,
# so fixing up executable files belonging to plugins doesn't work. So some han..
# manual job must be done to fixup plugins.

# STEP 1: manually find and fixup plugins files
set(BU_CHMOD_BUNDLE_ITEMS TRUE)
set(APP_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/@APP_NAME@.app")
file(GLOB_RECURSE PLUGINS "${APP_INSTALL_DIR}/**/*.far-plug*" "${APP_INSTALL_DIR}/**/*.so")
fixup_bundle("${APP_INSTALL_DIR}" "${PLUGINS}" "" IGNORE_ITEM "python;python3;python3.8;Python;.Python")

# STEP 2:
# Unfortunately cmake's fixup_bundle doesnt know about @loader_path and previous solution
# for plugins created incorrect dependencies in copied into Frameworks libraries.
# So here is stupidly simple workaround: create Frameworks symlink in eaach plugin's dir
# that relatively point to main bundle's Frameworks.
file(GLOB PLUGINDIRS "${APP_INSTALL_DIR}/Contents/MacOS/Plugins/*")
foreach(PLUGINDIR ${PLUGINDIRS})
	execute_process(COMMAND ln -s ../../../Frameworks "${PLUGINDIR}/Frameworks")
endforeach()
