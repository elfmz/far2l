#ifndef _COLORER_HRCPARSERPELPERS_H_
#define _COLORER_HRCPARSERPELPERS_H_

#include<cregexp/cregexp.h>

#include<common/Vector.h>
#include<common/Hashtable.h>
#include<common/io/InputSource.h>

#include<colorer/Region.h>
#include<colorer/Scheme.h>

// Must be not less than MATCHES_NUM in cregexp.h
#define REGIONS_NUM MATCHES_NUM
#define NAMED_REGIONS_NUM NAMED_MATCHES_NUM

class SchemeImpl;
class FileTypeImpl;


/** Information about one parsed keyword.
    Contains keyword, symbol specifier, region reference
    and internal optimization field.
    @ingroup colorer_parsers
*/
struct KeywordInfo{
  const SString *keyword;
  const Region* region;
  bool isSymbol;
  int  ssShorter;

//#include<common/MemoryOperator.h>

};

/** List of keywords.
    @ingroup colorer_parsers
*/
class KeywordList{
public:
  int num;
  int matchCase;
  int minKeywordLength;
  CharacterClass *firstChar;
  KeywordInfo *kwList;
  KeywordList();
  ~KeywordList();
  void sortList();
  void substrIndex();

#include<common/MemoryOperator.h>

};

/** One entry of 'inherit' element virtualization content.
    @ingroup colorer_parsers
*/
class VirtualEntry{
public:
  SchemeImpl *virtScheme, *substScheme;
  String *virtSchemeName, *substSchemeName;

  VirtualEntry(const String *scheme, const String *subst){
    virtScheme = substScheme = null;
    virtSchemeName = new SString(scheme);
    substSchemeName = new SString(subst);
  };
  ~VirtualEntry(){
    delete virtSchemeName;
    delete substSchemeName;
  };

#include<common/MemoryOperator.h>

};

enum SchemeNodeType { SNT_EMPTY, SNT_RE, SNT_SCHEME, SNT_KEYWORDS, SNT_INHERIT };
extern char*schemeNodeTypeNames[];

typedef Vector<VirtualEntry*> VirtualEntryVector;

/** Scheme node.
    @ingroup colorer_parsers
*/
class SchemeNode
{
public:
  SchemeNodeType type;

  String *schemeName;
  SchemeImpl *scheme;

  VirtualEntryVector virtualEntryVector;
  KeywordList *kwList;
  CharacterClass *worddiv;

  const Region* region;
  const Region* regions[REGIONS_NUM];
  const Region* regionsn[NAMED_REGIONS_NUM];
  const Region* regione[REGIONS_NUM];
  const Region* regionen[NAMED_REGIONS_NUM];
  CRegExp *start, *end;
  bool innerRegion, lowPriority, lowContentPriority;

#include<common/MemoryOperator.h>

  SchemeNode();
  ~SchemeNode();
};


/** Scheme storage implementation.
    Manages the vector of SchemeNode's.
    @ingroup colorer_parsers
*/
class SchemeImpl : public Scheme{
  friend class HRCParserImpl;
  friend class TextParserImpl;
public:
  const String *getName() const { return schemeName; };
  FileType *getFileType() const { return (FileType*)fileType; };

#include<common/MemoryOperator.h>

protected:
  String *schemeName;
  Vector<SchemeNode*> nodes;
  FileTypeImpl *fileType;

  SchemeImpl(const String *sn){
    schemeName = new SString(sn);
    fileType = null;
  };
  ~SchemeImpl(){
    delete schemeName;
    for (int idx = 0; idx < nodes.size(); idx++)
      delete nodes.elementAt(idx);
  };
};


/** Stores regular expressions of filename and firstline
    elements and helps to detect file type.
    @ingroup colorer_parsers
*/
class FileTypeChooser{
public:
  /** Creates choose entry.
      @param type If 0 - filename RE, if 1 - firstline RE
      @param prior Priority of this rule
      @param re Associated regular expression
  */
  FileTypeChooser(int type, double prior, CRegExp *re){
    this->type = type;
    this->prior = prior;
    this->re = re;
  };
  /** Default destructor */
  ~FileTypeChooser(){
    delete re;
  };
  /** Returns type of chooser */
  bool isFileName() const { return type == 0; };
  /** Returns type of chooser */
  bool isFileContent() const { return type == 1; };
  /** Returns chooser priority */
  double getPrior() const { return prior; };
  /** Returns associated regular expression */
  CRegExp *getRE() const { return re; };
private:
  CRegExp *re;
  int type;
  double prior;
};

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
