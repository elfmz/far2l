
#include<colorer/viewer/TextLinesStore.h>
#include<stdio.h>

void TextLinesStore::replaceTabs(int lno){
  String *od = lines.elementAt(lno)->replace(DString("\t"), DString("    "));
  delete lines.elementAt(lno);
  lines.setElementAt(od, lno);
};

TextLinesStore::TextLinesStore(){
  fileName = null;
};
TextLinesStore::~TextLinesStore(){
  freeFile();
};
void TextLinesStore::freeFile(){
  delete fileName;
  fileName = null;
  for(int i = 0; i < lines.size(); i++)
    delete lines.elementAt(i);
  lines.setSize(0);
};

void TextLinesStore::loadFile(const String *fileName, const String *inputEncoding, bool tab2spaces)
{
  if (this->fileName != null){
    freeFile();
  }

  if (fileName == null){
    char line[256];
    while(gets(line) != null){
      lines.addElement(new SString(line));
      if (tab2spaces) replaceTabs(lines.size()-1);
    }
  }else{
    this->fileName = new SString(fileName);
    InputSource *is = InputSource::newInstance(fileName);

    const byte *data = null;
    try{
      data = is->openStream();
    }catch (InputSourceException &e){
      delete is;
      throw e;
    }
    int len = is->length();

    int ei = inputEncoding == null ? -1 : Encodings::getEncodingIndex(inputEncoding->getChars());
    DString file(data, len, ei);
    int length = file.length();
    lines.ensureCapacity(length/30); // estimate number of lines

    int i = 0;
    int filepos = 0;
    int prevpos = 0;
    if (length && file[0] == 0xFEFF) filepos = prevpos = 1;
    while(filepos < length+1){
      if (filepos == length || file[filepos] == '\r' || file[filepos] == '\n'){
        lines.addElement(new SString(&file, prevpos, filepos-prevpos));
        if (tab2spaces) replaceTabs(lines.size()-1);
        if (filepos+1 < length && file[filepos] == '\r' && file[filepos+1] == '\n')
          filepos++;
        else if (filepos+1 < length && file[filepos] == '\n' && file[filepos+1] == '\r')
          filepos++;
        prevpos = filepos+1;
        i++;
      };
      filepos++;
    };
    delete is;
  }
};
const String *TextLinesStore::getFileName(){
  return fileName;
};

String *TextLinesStore::getLine(int lno){
  if (lines.size() <= lno) return null;
  return lines.elementAt(lno);
};
int TextLinesStore::getLineCount(){
  return lines.size();
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
