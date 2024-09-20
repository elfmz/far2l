#ifndef COLORER_COMMON_H
#define COLORER_COMMON_H

#include "colorer/common/Features.h"

#ifdef COLORER_FEATURE_ICU
#include "colorer/strings/icu/strings.h"
#else
#include "colorer/strings/legacy/strings.h"
#endif

/*
 Logging
*/
#include "colorer/common/Logger.h"

#endif  // COLORER_COMMON_H
