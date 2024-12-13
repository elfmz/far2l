#ifndef COLORER_FILESYSTEMS_HPP
#define COLORER_FILESYSTEMS_HPP

#include <colorer/Common.h>

#ifdef COLORER_FEATURE_OLD_COMPILERS
#include "colorer/platform/filesystem.hpp"
namespace fs = ghc::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

#endif
