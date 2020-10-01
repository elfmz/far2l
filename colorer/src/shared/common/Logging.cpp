
#include<string.h>
#include"common/Logging.h"
#include"unicode/String.h"

static const char *levelNames[] = {"QUIET", "ERROR", "WARN", "TRACE", "INFO"};

// BaseEditor, TextParserImpl, TPCache
// ParserFactory, BaseEditorNative, NSC:JHRCParser:getRegion

static const char *toTrace[] = {"BaseEditorNative", "JavaLineSource" };

static FILE *logFile = 0;

static void file_logger(int level, const char *cname, const char *msg, va_list v){

  int idx = 0;

  while (logFile == 0 && idx < 10){
    char log_name[30];
#ifdef __unix__
    sprintf(log_name, "/tmp/clr-trace%d.log", idx);
#else
    sprintf(log_name, "c:/clr-trace%d.log", idx);
#endif
    logFile = fopen(log_name, "ab+");
    idx++;
  }

  fprintf(logFile, "[%s][%s] ", levelNames[level], cname);

  vfprintf(logFile, msg, v);

  fprintf(logFile, "\n");

  fflush(logFile);
}

static void console_logger(int level, const char *cname, const char *msg, va_list v){

  printf("[%s][%s] ", levelNames[level], cname);

  vprintf(msg, v);

  printf("\n");
}


void colorer_logger_set_target(const char *logfile){
  if (logfile == 0) return;
  if (logFile != 0){
    fclose(logFile);
  }
  logFile = fopen(logfile, "ab+");
}



void colorer_logger_error(const char *cname, const char *msg, ...){
  va_list v;
  va_start(v, msg);
  colorer_logger(COLORER_FEATURE_LOGLEVEL_ERROR, cname, msg, v);
  va_end (v);
}

void colorer_logger_warn(const char *cname, const char *msg, ...){
  va_list v;
  va_start(v, msg);
  colorer_logger(COLORER_FEATURE_LOGLEVEL_WARN, cname, msg, v);
  va_end (v);
}
void colorer_logger_trace(const char *cname, const char *msg, ...){
  va_list v;
  va_start(v, msg);
  colorer_logger(COLORER_FEATURE_LOGLEVEL_TRACE, cname, msg, v);
  va_end (v);
}
void colorer_logger_info(const char *cname, const char *msg, ...){
  va_list v;
  va_start(v, msg);
  colorer_logger(COLORER_FEATURE_LOGLEVEL_INFO, cname, msg, v);
  va_end (v);
}



void colorer_logger(int level, const char *cname, const char *msg, va_list v){

  bool found = false;

  for (size_t idx = 0; idx < sizeof(toTrace)/sizeof(toTrace[0]); idx++){
    if (stricmp(toTrace[idx], cname) == 0){
      found = true;
    }
  }
  if (!found){
  //  return;
  }
  //*/

//  console_logger(level, cname, msg, v);
  file_logger(level, cname, msg, v);
}



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
