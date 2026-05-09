# libusb v1.0.29, vendored as a static archive for the MTP plugin.
# See VENDORING.md for the source snapshot procedure and licensing notes.

set(LIBUSB_SOURCES
    libusb/core.c
    libusb/descriptor.c
    libusb/hotplug.c
    libusb/io.c
    libusb/strerror.c
    libusb/sync.c
    libusb/os/events_posix.c
    libusb/os/threads_posix.c
)

if(APPLE)
    list(APPEND LIBUSB_SOURCES libusb/os/darwin_usb.c)
elseif(UNIX)
    # Linux: usbfs core + netlink hotplug. Deliberately no libudev — far2l does
    # not use libudev anywhere else; netlink uevents cover MTP's needs.
    list(APPEND LIBUSB_SOURCES libusb/os/linux_usbfs.c libusb/os/linux_netlink.c)
endif()

add_library(libusb_static STATIC ${LIBUSB_SOURCES})

target_compile_definitions(libusb_static PRIVATE HAVE_CONFIG_H)

target_include_directories(libusb_static
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/libusb       # public libusb.h
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}              # config.h selector lives here
)

# Suppress third-party-source warnings only — kept narrow rather than -w so
# real issues in our integration glue still surface.
target_compile_options(libusb_static PRIVATE
    -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function
    -Wno-deprecated-declarations -Wno-pointer-sign -Wno-sign-compare
)

if(APPLE)
    target_link_libraries(libusb_static PUBLIC
        "-framework IOKit" "-framework CoreFoundation" "-framework Security" objc)
elseif(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(libusb_static PUBLIC Threads::Threads)
endif()
