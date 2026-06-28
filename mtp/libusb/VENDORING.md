# Vendoring libusb

Static, in-tree build of libusb-1.0 used by the MTP plugin. Replaces the prior
runtime dependency on a system `libusb-1.0` shared library, so the produced
`mtp.far-plug-wide` carries no external libusb dep on macOS or Linux.

## Pinned version

- Upstream: <https://github.com/libusb/libusb>
- Tag: **`v1.0.30`** (current upstream stable as of 2026-05-17). Brings macOS
  26 Tahoe compatibility fix, descriptor-parsing memory-safety hardening,
  Darwin concurrency fixes, and new `libusb_get_device_string()` /
  `libusb_get_session_data()` APIs. No ABI breaks; vendored file list and
  config-* headers unchanged from 1.0.29.

## Source files copied

From the upstream tag's tree, into `mtp/libusb/`:

```
libusb/libusb.h
libusb/libusbi.h
libusb/version.h
libusb/version_nano.h
libusb/core.c
libusb/descriptor.c
libusb/hotplug.c
libusb/io.c
libusb/strerror.c
libusb/sync.c
libusb/os/events_posix.{c,h}
libusb/os/threads_posix.{c,h}
libusb/os/darwin_usb.{c,h}        # macOS backend
libusb/os/linux_usbfs.{c,h}       # Linux backend (core)
libusb/os/linux_netlink.c         # Linux hotplug via kernel netlink (no libudev)
COPYING                           # LGPL-2.1+
```

Backends NOT copied: Windows, Haiku, NetBSD/OpenBSD, Solaris, Emscripten,
Android-specific bits, the libudev variant of the Linux backend.

## FreeBSD

Upstream libusb has no `freebsd_usb.c`. On FreeBSD `mtp/libusb/CMakeLists.txt` wraps the base-system `libusb-1.0` (in FreeBSD base since 8.0) under the `libusb_static` target name; top-level CMake auto-detects via pkg-config and silently skips MTP if absent.

## Refresh procedure

1. `git clone --depth 1 -b vX.Y.Z https://github.com/libusb/libusb /tmp/libusb`
2. Re-copy the file list above from `/tmp/libusb` over `mtp/libusb/`.
3. Refresh `config-darwin.h` from `/tmp/libusb/Xcode/config.h` — Apple
   maintainers keep this header in sync with autotools output, so it's the
   canonical macOS config. Adopt verbatim.
4. Refresh `config-linux.h` by either:
   - Running `./bootstrap.sh && ./configure --disable-udev` on a current
     Debian/Ubuntu host and copying the resulting `config.h`, then stripping
     `PACKAGE_*` autotools noise; or
   - Manually reconciling against `/tmp/libusb/configure.ac` if no Linux
     host is handy. The HAVE_* macros referenced by our included sources are:
     `HAVE_ASM_TYPES_H`, `HAVE_CLOCK_GETTIME`, `HAVE_EVENTFD`, `HAVE_NFDS_T`,
     `HAVE_PIPE2`, `HAVE_PTHREAD_CONDATTR_SETCLOCK`, `HAVE_PTHREAD_SETNAME_NP`,
     `HAVE_SYS_TIME_H`, `HAVE_TIMERFD`, plus `PLATFORM_POSIX`, `_GNU_SOURCE`,
     `DEFAULT_VISIBILITY`, `PRINTF_FORMAT`, `ENABLE_LOGGING`. We keep
     `HAVE_LIBUDEV` undefined (we use `linux_netlink.c`).
5. Re-run the verification steps in this directory's CMake build (no `libusb`
   in `otool -L` / `ldd`, libusb_ symbols present in the built plugin, MTP
   smoke-test against an attached device).

## Why static?

Removes the runtime dependency on `libusb-1.0` shared libraries (Homebrew on
macOS, distro packages on Linux) so the plugin is self-contained and packageable.

## Why no libudev on Linux?

far2l does not use libudev anywhere else; introducing it solely for libusb's
hotplug path would be a new project-wide dependency. libusb's `linux_netlink.c`
provides hotplug via kernel uevents — sufficient for MTP's "pick a device, copy
files" usage, and works in any environment where the kernel exposes uevent
netlink (the default on every desktop Linux).

## License

libusb is **LGPL-2.1-or-later**; far2l is **GPL-2.0-or-later**. Static linking
is permitted under **LGPL-2.1 §3**, which explicitly allows the LGPL'd library
to be combined into a work distributed under GPL-2 (or later). The combined
binary inherits GPL-2.0+. Ship the `COPYING` file alongside the vendored
sources to satisfy the LGPL's "include a copy of the license" requirement.
