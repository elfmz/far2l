
#include<stdio.h>
#include<string.h>
#include<unicode/CharacterClass.h>
#include<unicode/UnicodeTools.h>
#include<unicode/x_charcategory_names.h>
#include<unicode/x_charcategory2.h>

CharacterClass::CharacterClass(){
  infoIndex = new BitArray*[256];
  memset(infoIndex, 0, 256*sizeof(*infoIndex));
}
CharacterClass::~CharacterClass(){
  clear();
  delete[] infoIndex;
}
/**
  Creates CharacterClass object from regexp character class syntax.
  Extensions (comparing to Perl):
  inner class substraction [{L}-[{Lu}]], addition [{L}[1234]], intersection [{L}&[{Lu}]]
*/
CharacterClass *CharacterClass::createCharClass(const String &ccs, int pos, int *retPos)
{
if (ccs[pos] != '[') return null;

CharacterClass *cc = new CharacterClass();
CharacterClass cc_temp;
bool inverse = false;
wchar prev_char = BAD_WCHAR;

  pos++;
  if (ccs[pos] == '^'){
    inverse = true;
    pos++;
  };
  for(; pos < ccs.length(); pos++){
    if(ccs[pos] == ']'){
      if (retPos != null) *retPos = pos;
      if (inverse){
        CharacterClass *newcc = new CharacterClass();
        newcc->fill();
        newcc->clearClass(*cc);
        delete cc;
        cc = newcc;
      };
      return cc;
    };
    if(ccs[pos] == '{'){
      String *categ = UnicodeTools::getCurlyContent(ccs, pos);
      if (categ == null){
        delete cc;
        return 0;
      }
      if (*categ == "ALL") cc->fill();
      else if (*categ == "ASSIGNED") cc->addCategory("");
      else if (*categ == "UNASSIGNED"){
        cc_temp.clear();
        cc_temp.addCategory("");
        cc->fill();
        cc->clearClass(cc_temp);
      }else if(categ->length()) cc->addCategory(*categ);
      pos += categ->length()+1;
      delete categ;
      prev_char = BAD_WCHAR;
      continue;
    };
    if (ccs[pos] == '\\' && pos+1 < ccs.length()){
      int retEnd;
      prev_char = BAD_WCHAR;
      switch(ccs[pos+1]){
        case 'd':
          cc->addCategory(CHAR_CATEGORY_Nd);
          break;
        case 'D':
          cc_temp.fill();
          cc_temp.clearCategory(CHAR_CATEGORY_Nd);
          cc->addClass(cc_temp);
          break;
        case 'w':
          cc->addCategory("L");
          cc->addCategory(CHAR_CATEGORY_Nd);
          cc->addChar('_');
          break;
        case 'W':
          cc_temp.fill();
          cc_temp.clearCategory("L");
          cc_temp.clearCategory(CHAR_CATEGORY_Nd);
          cc_temp.clearChar('_');
          cc->addClass(cc_temp);
          break;
        case 's':
          cc->addCategory("Z");
          cc->addChar(0x09);
          cc->addChar(0x0A);
          cc->addChar(0x0C);
          cc->addChar(0x0D);
          break;
        case 'S':
          cc_temp.fill();
          cc_temp.clearCategory("Z");
          cc_temp.clearChar(0x09);
          cc_temp.clearChar(0x0A);
          cc_temp.clearChar(0x0C);
          cc_temp.clearChar(0x0D);
          cc->addClass(cc_temp);
          break;
        default:
          prev_char = UnicodeTools::getEscapedChar(ccs, pos, retEnd);
          if (prev_char == BAD_WCHAR) break;
          cc->addChar(prev_char);
          pos = retEnd-1;
          break;
      };
      pos++;
      continue;
    };
    // substract -[class]
    if (pos+1 < ccs.length() && ccs[pos] == '-' && ccs[pos+1] == '['){
      int retEnd;
      CharacterClass *scc = createCharClass(ccs, pos+1, &retEnd);
      if (retEnd == ccs.length()){
        delete cc;
        return null;
      };
      if (scc == null){
        delete cc;
        return null;
      };
      cc->clearClass(*scc);
      delete scc;
      pos = retEnd;
      prev_char = BAD_WCHAR;
      continue;
    };
    // intersect &&[class]
    if (pos+2 < ccs.length() && ccs[pos] == '&' && ccs[pos+1] == '&' && ccs[pos+2] == '['){
      int retEnd;
      CharacterClass *scc = createCharClass(ccs, pos+2, &retEnd);
      if (retEnd == ccs.length()){
        delete cc;
        return null;
      };
      if (scc == null){
        delete cc;
        return null;
      };
      cc->intersectClass(*scc);
      delete scc;
      pos = retEnd;
      prev_char = BAD_WCHAR;
      continue;
    };
    // add [class]
    if (ccs[pos] == '['){
      int retEnd;
      CharacterClass *scc = createCharClass(ccs, pos, &retEnd);
      if (scc == null){
        delete cc;
        return null;
      };
      cc->addClass(*scc);
      delete scc;
      pos = retEnd;
      prev_char = BAD_WCHAR;
      continue;
    };
    if (ccs[pos] == '-' && prev_char != BAD_WCHAR && pos+1 < ccs.length() && ccs[pos+1] != ']' ){
      int retEnd;
      wchar nextc = UnicodeTools::getEscapedChar(ccs, pos+1, retEnd);
      if (nextc == BAD_WCHAR) break;
      cc->addRange(prev_char, nextc);
      pos = retEnd;
      continue;
    };
    cc->addChar(ccs[pos]);
    prev_char = ccs[pos];
  };
  delete cc;
  return null;
}

void CharacterClass::addChar(wchar c){
  BitArray *tablePos = infoIndex[(c>>8)&0xFF];
  if (!tablePos){
    tablePos = new BitArray();
    infoIndex[(c>>8)&0xFF] = tablePos;
  };
  tablePos->setBit(c&0xFF);
}
void CharacterClass::clearChar(wchar c){
  BitArray *tablePos = infoIndex[(c>>8)&0xFF];
  if (!tablePos) return;
  tablePos->clearBit(c&0xFF);
}
void CharacterClass::addRange(wchar s, wchar e){
  for(int ti = (s>>8)&0xFF; ti <= ((e>>8)&0xFF); ti++){
    if (!infoIndex[ti]) infoIndex[ti] = new BitArray();
    infoIndex[ti]->addRange((ti == s>>8)?s&0xFF:0, (ti == e>>8)?e&0xFF:0xFF);
  };
}
void CharacterClass::clearRange(wchar s, wchar e){
  for(int ti = (s>>8)&0xFF; ti <= ((e>>8)&0xFF); ti++){
    if (!infoIndex[ti]) infoIndex[ti] = new BitArray();
    infoIndex[ti]->clearRange(ti == s>>8?s&0xFF:0, ti == e>>8?e&0xFF:0xFF);
  };
}

void CharacterClass::addCategory(ECharCategory cat){
  if (!cat || cat >= CHAR_CATEGORY_LAST) return;
  for (int i = 0; i < 0x100; i++){
    unsigned short pos = arr_idxCharCategoryIdx[(int(cat)-1)*0x100 + i];
    if (!pos) continue;
    BitArray *tablePos = infoIndex[i];
    if (!tablePos){
      tablePos = new BitArray();
      infoIndex[i] = tablePos;
    };
    tablePos->addBitArray((char*)(arr_idxCharCategory+pos), 8*4);
  };
}
void CharacterClass::addCategory(const String &cat){
  for(size_t pos = 0; pos < ARRAY_SIZE(char_category_names); pos++){
    int ci;
    for(ci = 0; ci < cat.length() && cat[ci] == char_category_names[pos][ci]; ci++);
    if (ci == cat.length()) addCategory(ECharCategory(pos));
  };
}
void CharacterClass::addCategory(const char *cat){
  addCategory(DString(cat));
}

void CharacterClass::clearCategory(ECharCategory cat){
  if (!cat || cat >= CHAR_CATEGORY_LAST) return;
  for (int i = 0; i < 0x100; i++){
    unsigned short pos = arr_idxCharCategoryIdx[(int(cat)-1)*0x100 + i];
    if (!pos) continue;
    BitArray *tablePos = infoIndex[i];
    if (!tablePos){
      tablePos = new BitArray();
      infoIndex[i] = tablePos;
    };
    tablePos->clearBitArray((char*)(arr_idxCharCategory+pos), 8*4);
  };
}
void CharacterClass::clearCategory(const String &cat){
  for(size_t pos = 0; pos < ARRAY_SIZE(char_category_names); pos++){
    int ci;
    for(ci = 0; ci < cat.length() && cat[ci] == char_category_names[pos][ci]; ci++);
    if (ci == cat.length()) clearCategory(ECharCategory(pos));
  };
}
void CharacterClass::clearCategory(const char *cat){
  clearCategory(DString(cat));
}

void CharacterClass::addClass(const CharacterClass &cclass){
  for(int p = 0; p < 256; p++){
    if (!infoIndex[p]) infoIndex[p] = new BitArray();
    infoIndex[p]->addBitArray(cclass.infoIndex[p]);
  };
}
void CharacterClass::intersectClass(const CharacterClass &cclass){
  for(int p = 0; p < 256; p++){
    if (infoIndex[p])
      infoIndex[p]->intersectBitArray(cclass.infoIndex[p]);
  };
}

void CharacterClass::clearClass(const CharacterClass &cclass){
  for(int p = 0; p < 256; p++)
    if (infoIndex[p])
      infoIndex[p]->clearBitArray(cclass.infoIndex[p]);
}

void CharacterClass::clear(){
  for(int i = 0; i < 256; i++)
    if (infoIndex[i]){
      delete infoIndex[i];
      infoIndex[i] = 0;
    };
}
void CharacterClass::fill(){
  for(int i = 0; i < 256; i++){
    if (!infoIndex[i]) infoIndex[i] = new BitArray();
    infoIndex[i]->addRange(0,0xFF);
  };
}

bool CharacterClass::inClass(wchar c) const{
  BitArray *tablePos = infoIndex[(c>>8)&0xFF];
  if (!tablePos) return false;
  return tablePos->getBit(c&0xFF);
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
