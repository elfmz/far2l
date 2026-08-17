# MTP/PTP plugin module backed by libmtp (vendored static build in libmtp/).
# libusb is also vendored statically — see libusb/VENDORING.md.

add_subdirectory(libusb)
add_subdirectory(libmtp)

set(SOURCES
    src/FARPlugin.cpp
    src/MTPPlugin.cpp
    src/MTPDialogs.cpp
    src/MTPLog.cpp
    src/LibMtpBackend.cpp
    src/ProgressBatch.cpp
)

add_library(mtp MODULE ${SOURCES})

# libusb_static is also pulled transitively via vendored libmtp_static, but link it explicitly so
# the plugin (which uses libusb directly — see ReadDeviceSerialViaLibusb, SharedLibusbContext) still
# resolves when libmtp_static is an INTERFACE wrapper around system libmtp.
target_link_libraries(mtp utils far2l libmtp_static libusb_static)

if(APPLE)
    # CoreFoundation: CFPreferences{SetValue,Synchronize} for icdd bypass.
    target_link_libraries(mtp "-framework CoreFoundation")
endif()

target_compile_definitions(mtp PRIVATE
    -DWINPORT_DIRECT
    -DUNICODE
    -D_UNICODE
    -DFAR_DONT_USE_INTERNALS
    # Activate MTPLog DBG() in Debug builds — macro gated on DEBUG/_DEBUG (matches ADBLog), CMake Debug config doesn't set them on Unix.
    "$<$<CONFIG:Debug>:DEBUG>"
)

if(MTP_SYSTEM_LIBMTP)
    # System libmtp hides its internal PTP symbols (libmtp.sym version-script); ListChildrenFast falls back to the slow path.
    target_compile_definitions(mtp PRIVATE MTP_SYSTEM_LIBMTP)
endif()

target_include_directories(mtp PRIVATE .)
target_include_directories(mtp PRIVATE ../far2l/far2sdk)
target_include_directories(mtp PRIVATE ../WinPort)

set_target_properties(mtp PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/mtp/plug"
    PREFIX ""
    SUFFIX ".far-plug-wide"
)

set(CURRENT_TARGET "mtp")
file(GLOB_RECURSE RESOURCE_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/configs
    "${CMAKE_CURRENT_SOURCE_DIR}/configs/*"
)
setup_target_auxiliary_files(${CURRENT_TARGET}
    RESOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/configs
    "${INSTALL_DIR}/Plugins/${CURRENT_TARGET}/"
)
add_dependencies(${CURRENT_TARGET} copy_aux_files_for_${CURRENT_TARGET})
