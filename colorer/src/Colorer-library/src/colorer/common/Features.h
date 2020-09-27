#ifndef _COLORER_FEATURES_H_
#define _COLORER_FEATURES_H_

#ifdef COLORER_ENABLE_TRACE
#define CTRACE(info) info
#else
#define CTRACE(info)
#endif

/**
  If defined, HTTP InputSource is implemented.
*/
#ifndef COLORER_FEATURE_HTTPINPUTSOURCE
  #define COLORER_FEATURE_HTTPINPUTSOURCE TRUE
#endif

/**
  If defined, JAR InputSource is implemented.
*/
#ifndef COLORER_FEATURE_JARINPUTSOURCE
  #define COLORER_FEATURE_JARINPUTSOURCE TRUE
#endif

#endif

