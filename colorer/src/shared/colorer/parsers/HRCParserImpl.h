#ifndef _COLORER_HRCPARSERIMPL_H_
#define _COLORER_HRCPARSERIMPL_H_

#include<xml/xmldom.h>
#include<cregexp/cregexp.h>
#include<common/Vector.h>
#include<common/Hashtable.h>
#include<colorer/HRCParser.h>
#include<unicode/UnicodeTools.h>
#include<colorer/parsers/helpers/HRCParserHelpers.h>

class FileTypeImpl;

/** Implementation of HRCParser.
    Reads and mantains HRC database of syntax rules,
    used by TextParser implementations to make
    realtime text syntax parsing.
    @ingroup colorer_parsers
*/
class HRCParserImpl : public HRCParser
{
public:
  HRCParserImpl();
  ~HRCParserImpl();

  void setErrorHandler(ErrorHandler *eh);

  void loadSource(InputSource *is);
  FileType *getFileType(const String *name);
  FileType *enumerateFileTypes(int index);
  FileType *chooseFileType(const String *fileName, const String *firstLine, int typeNo = 0);
  int getFileTypesCount();

  int getRegionCount();
  const Region *getRegion(int id);
  const Region* getRegion(const String *name);

  const String *getVersion();

protected:
friend class FileTypeImpl;

  enum QualifyNameType { QNT_DEFINE, QNT_SCHEME, QNT_ENTITY };

  // types and packages
  Hashtable<FileTypeImpl*> fileTypeHash;
  // types, not packages
  Vector<FileTypeImpl*>    fileTypeVector;

  Hashtable<SchemeImpl*>   schemeHash;
  Hashtable<int>           disabledSchemes;

  Vector<const Region*>    regionNamesVector;
  Hashtable<const Region*> regionNamesHash;
  Hashtable<String*>       schemeEntitiesHash;

  String *versionName;

  FileTypeImpl *parseType;
  InputSource *curInputSource;
  bool structureChanged;
  bool updateStarted;

  DocumentBuilder docbuilder;
  ErrorHandler *errorHandler;

  void loadFileType(FileType *filetype);
  void unloadFileType(FileTypeImpl *filetype);

  void parseHRC(InputSource *is);
  void addPrototype(Element *elem);
  void addType(Element *elem);

  void addScheme(Element *elem);
  void addSchemeNodes(SchemeImpl *scheme, Node *elem);
  
  void loadBlockRegions(SchemeNode *node, Element *el);
  void loadRegions(SchemeNode *node, Element *el, bool st);

  String *qualifyOwnName(const String *name);
  bool checkNameExist(const String *name, FileTypeImpl *parseType, QualifyNameType qntype, bool logErrors);
  String *qualifyForeignName(const String *name, QualifyNameType qntype, bool logErrors);

  void updateLinks();
  String *useEntities(const String *name);
  const Region *getNCRegion(Element *el, const String &tag);
  const Region *getNCRegion(const String *name, bool logErrors);
};

#include<colorer/parsers/helpers/FileTypeImpl.h>

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
 *  Eugene Efremov <4mirror@mail.ru>
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
