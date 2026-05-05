# LIBUSB_* vars come from the parent CMakeLists.txt (which already gated the whole plugin
# on libusb-1.0 being available); no need to re-probe here.

add_library(libmtp_static STATIC
    libmtp.c
    unicode.c
    util.c
    playlist-spl.c
    ptp.c
    libusb1-glue.c
)

target_include_directories(libmtp_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${LIBUSB_INCLUDE_DIRS})
target_link_directories(libmtp_static PUBLIC ${LIBUSB_LIBRARY_DIRS})
target_compile_options(libmtp_static PRIVATE -w)
# Expose POSIX/BSD-derived libc decls (strdup, strcasecmp, strndup, basename…). macOS declares
# them unconditionally; glibc/musl hide them behind feature-test macros and modern clang/gcc treat
# implicit declarations as hard errors.
target_compile_definitions(libmtp_static PRIVATE _GNU_SOURCE _DEFAULT_SOURCE)

if(APPLE)
    find_library(ICONV_LIB iconv REQUIRED)
    target_link_libraries(libmtp_static PUBLIC ${LIBUSB_LIBRARIES} ${ICONV_LIB} "-framework IOKit" "-framework CoreFoundation")
else()
    target_link_libraries(libmtp_static PUBLIC ${LIBUSB_LIBRARIES})
endif()
