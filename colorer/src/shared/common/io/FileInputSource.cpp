#include<stdio.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/timeb.h>
#include<fcntl.h>
#include<time.h>
#include <windows.h>
#include <utils.h>

#if defined _WIN32
#include<io.h>
#include<windows.h>
#endif
#if defined __unix__ || defined __GNUC__
#include<unistd.h>
#endif
#ifndef O_BINARY
#define O_BINARY 0x0
#endif

#include<common/io/FileInputSource.h>

FileInputSource::FileInputSource(const String *basePath, FileInputSource *base){
  bool prefix = true;
  if (basePath->startsWith(DString("file://"))){
    baseLocation = new SString(basePath, 7, -1);
  }else if (basePath->startsWith(DString("file:/"))){
    baseLocation = new SString(basePath, 6, -1);
  }else if (basePath->startsWith(DString("file:"))){
    baseLocation = new SString(basePath, 5, -1);
  }else{
    if (isRelative(basePath) && base != null)
      baseLocation = getAbsolutePath(base->getLocation(), basePath);
    else
      baseLocation = new SString(basePath);
    prefix = false;
  };
#if defined _WIN32
   // replace the environment variables to their values
  size_t i=ExpandEnvironmentStrings(baseLocation->getTChars(),NULL,0);
  TCHAR *temp = new TCHAR[i];
  ExpandEnvironmentStrings(baseLocation->getTChars(),temp,static_cast<DWORD>(i));
  delete baseLocation;
  baseLocation = new SString(temp);
  delete[] temp;
#endif
  if(prefix && (baseLocation->indexOf(':') == -1 || baseLocation->indexOf(':') > 10) && !baseLocation->startsWith(DString("/"))){
    StringBuffer *n_baseLocation = new StringBuffer();
    n_baseLocation->append(DString("/")).append(baseLocation);
    delete baseLocation;
    baseLocation = n_baseLocation;
  }
  stream = null;
};

FileInputSource::~FileInputSource(){
  delete baseLocation;
  delete[] stream;
};
InputSource *FileInputSource::createRelative(const String *relPath){
  return new FileInputSource(relPath, this);
};

const String *FileInputSource::getLocation() const{
  return baseLocation;
};

#include <string>

const byte *FileInputSource::openStream()
{
  if (stream != null) throw InputSourceException(StringBuffer("openStream(): source stream already opened: '")+baseLocation+"'");
#ifdef _UNICODE
  int source = open(Wide2MB(baseLocation->getWChars()).c_str(), O_BINARY);
#else
  int source = open(baseLocation->getChars(), O_BINARY);
#endif
  if (source == -1) {
		fprintf(stderr, "failed to open %ls\n",  baseLocation->getWChars());
    throw InputSourceException(StringBuffer("Can't open file '")+baseLocation+"'");
	}
  struct stat st;
  fstat(source, &st);
  len = st.st_size;

  stream = new byte[len];
  memset(stream,0, sizeof(byte)*len);
  read(source, stream, len);
  close(source);
  return stream;
};

void FileInputSource::closeStream(){
  if (stream == null) throw InputSourceException(StringBuffer("closeStream(): source stream is not yet opened"));
  delete[] stream;
  stream = null;
};

int FileInputSource::length() const{
  if (stream == null)
    throw InputSourceException(DString("length(): stream is not yet opened"));
  return len;
};

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
