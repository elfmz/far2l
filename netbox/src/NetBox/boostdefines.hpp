
// boostdefines.hpp
//

#pragma once

//#define BOOST_ALL_DYN_LINK
#define BOOST_LIB_DIAGNOSTIC

#ifdef BOOST_ALL_DYN_LINK
#pragma message("Include: dynamic linking with boost libraries")
#endif

#ifdef NETBOX_DEBUG
//#define BOOST_ENABLE_ASSERT_HANDLER
#endif

#define BOOST_FILESYSTEM_VERSION 2
