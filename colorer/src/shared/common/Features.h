#ifndef _COLORER_FEATURES_H_
#define _COLORER_FEATURES_H_

/**
  Default log-level
*/
#ifndef COLORER_FEATURE_LOGLEVEL
  #define COLORER_FEATURE_LOGLEVEL COLORER_FEATURE_LOGLEVEL_ERROR
#endif

/**
  If defined, use dlmalloc memory management implementation
*/
#ifndef COLORER_FEATURE_USE_DL_MALLOC
//  #define COLORER_FEATURE_USE_DL_MALLOC TRUE
#endif

/**
  If defined, long-living classes use chunk allocation procedure
*/
#ifndef COLORER_FEATURE_USE_CHUNK_ALLOC
//  #define COLORER_FEATURE_USE_CHUNK_ALLOC TRUE
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
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Colorer Library.
 *
 * The Initial Developer of the Original Code is
 * Cail Lomecb <cail@nm.ru>.
 * Portions created by the Initial Developer are Copyright (C) 1999-2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
