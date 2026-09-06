/* Platform selector for libusb's HAVE_CONFIG_H build. */

#ifdef __APPLE__
#  include "config-darwin.h"
#elif defined(__linux__)
#  include "config-linux.h"
#else
#  error "libusb static build: unsupported platform"
#endif
