# libusb v1.0.30 vendored static archive for the MTP plugin; see VENDORING.md.

if(MTP_SYSTEM_LIBUSB)
    # Wrap base-system libusb-1.0 (detected at top level). Auto on FreeBSD/Nix, opt-in elsewhere.
    add_library(libusb_static INTERFACE)
    target_include_directories(libusb_static INTERFACE ${MTP_LIBUSB_INCLUDE_DIRS})
    target_link_directories(libusb_static INTERFACE ${MTP_LIBUSB_LIBRARY_DIRS})
    target_link_libraries(libusb_static INTERFACE ${MTP_LIBUSB_LIBRARIES})
else()
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
        # Linux: usbfs + netlink hotplug; no libudev (far2l doesn't use it elsewhere).
        list(APPEND LIBUSB_SOURCES libusb/os/linux_usbfs.c libusb/os/linux_netlink.c)
    endif()

    add_library(libusb_static STATIC ${LIBUSB_SOURCES})

    target_compile_definitions(libusb_static PRIVATE HAVE_CONFIG_H)

    target_include_directories(libusb_static
        PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/libusb
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    )

    # Narrow third-party warning suppression — real issues in our glue still surface.
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
endif()
