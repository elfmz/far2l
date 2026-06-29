# libmtp 1.1.23 vendored static archive for the MTP plugin.
# Sources are upstream src/ byte-for-byte; libmtp.h, _stdint.h, gphoto2-endian.h are autoconf outputs.

if(MTP_SYSTEM_LIBMTP)
    # Wrap base-system libmtp (detected at top level via pkg-config IMPORTED_TARGET).
    # Linking the imported PkgConfig::MTP_LIBMTP target — not `${MTP_LIBMTP_LIBRARIES}` — because the
    # bare lib name "mtp" returned by pkg-config collides with our plugin target name and would be
    # silently dropped as a self-link.
    add_library(libmtp_static INTERFACE)
    target_link_libraries(libmtp_static INTERFACE PkgConfig::MTP_LIBMTP)
else()
    # libusb is vendored as the libusb_static target by ../libusb/CMakeLists.txt;
    # its PUBLIC linkage carries IOKit/CoreFoundation/Security on macOS and pthread
    # on Linux, so we don't repeat platform frameworks here.
    add_library(libmtp_static STATIC
        libmtp.c
        unicode.c
        util.c
        playlist-spl.c
        ptp.c
        libusb1-glue.c
    )

    target_include_directories(libmtp_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_options(libmtp_static PRIVATE -w)
    # Expose POSIX/BSD-derived libc decls (strdup, strcasecmp, strndup, basename…). macOS declares
    # them unconditionally; glibc/musl hide them behind feature-test macros and modern clang/gcc treat
    # implicit declarations as hard errors.
    target_compile_definitions(libmtp_static PRIVATE _GNU_SOURCE _DEFAULT_SOURCE)

    if(APPLE)
        find_library(ICONV_LIB iconv REQUIRED)
        target_link_libraries(libmtp_static PUBLIC libusb_static ${ICONV_LIB})
    else()
        target_link_libraries(libmtp_static PUBLIC libusb_static)
    endif()
endif()
