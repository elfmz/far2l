
#include<stdio.h>
#include<colorer/handlers/StyledHRDMapper.h>
#include<unicode/UnicodeTools.h>
#include<xml/xmldom.h>

const int StyledRegion::RD_BOLD = 1;
const int StyledRegion::RD_ITALIC = 2;
const int StyledRegion::RD_UNDERLINE = 4;
const int StyledRegion::RD_STRIKEOUT = 8;

StyledHRDMapper::StyledHRDMapper(){};
StyledHRDMapper::~StyledHRDMapper(){
  for(RegionDefine *rd = regionDefines.enumerate(); rd;rd = regionDefines.next())
    delete rd;
};

void StyledHRDMapper::loadRegionMappings(InputSource *is)
{
  DocumentBuilder docbuilder;
  Document *hrdbase = docbuilder.parse(is);
  Element *hbase = hrdbase->getDocumentElement();
  if (*hbase->getNodeName() != "hrd"){
    docbuilder.free(hrdbase);
    throw Exception(DString("Error loading HRD file"));
  };

  for(Node *curel = hbase->getFirstChild(); curel; curel = curel->getNextSibling()){
    if (curel->getNodeType() == Node::ELEMENT_NODE && *curel->getNodeName() == "assign"){
      const String *name = ((Element*)curel)->getAttribute(DString("name"));
      if (name == null) continue;

      if (regionDefines.get(name) != null){
        delete regionDefines.get(name);
      }
      int val = 0;
      bool bfore = UnicodeTools::getNumber(((Element*)curel)->getAttribute(DString("fore")), &val);
      int fore = val;
      bool bback = UnicodeTools::getNumber(((Element*)curel)->getAttribute(DString("back")), &val);
      int back = val;
      int style = 0;
      if (UnicodeTools::getNumber(((Element*)curel)->getAttribute(DString("style")), &val)){
        style = val;
      }
      RegionDefine *rdef = new StyledRegion(bfore, bback, fore, back, style);
      regionDefines.put(name, rdef);
    };
  };
  docbuilder.free(hrdbase);
};

/** Writes all currently loaded region definitions into
    XML file. Note, that this method writes all loaded
    defines from all loaded HRD files.
*/
void StyledHRDMapper::saveRegionMappings(Writer *writer) const
{
  writer->write(DString("<?xml version=\"1.0\"?>\n\
<!DOCTYPE hrd SYSTEM \"../hrd.dtd\">\n\n\
<hrd>\n"));
  for(String *key = regionDefines.enumerateKey(); key; key=regionDefines.nextkey()){
    const StyledRegion *rdef = StyledRegion::cast(regionDefines.get(key));
    char temporary[256];
    writer->write(StringBuffer("  <define name='")+key+"'");
    if (rdef->bfore){
      sprintf(temporary, " fore=\"#%06x\"", rdef->fore);
      writer->write(DString(temporary));
    };
    if (rdef->bback){
      sprintf(temporary, " back=\"#%06x\"", rdef->back);
      writer->write(DString(temporary));
    };
    if (rdef->style){
      sprintf(temporary, " style=\"#%06x\"", rdef->style);
      writer->write(DString(temporary));
    };
    writer->write(DString("/>\n"));
  };
  writer->write(DString("\n</hrd>\n"));
};

/** Adds or replaces region definition */
void StyledHRDMapper::setRegionDefine(const String &name, const RegionDefine *rd)
{
  RegionDefine *rd_old = regionDefines.get(&name);

  const StyledRegion *new_region = StyledRegion::cast(rd);
  RegionDefine *rd_new = new StyledRegion(*new_region);
  regionDefines.put(&name, rd_new);

  // Searches and replaces old region references
  for(int idx = 0; idx < regionDefinesVector.size(); idx++){
    if (regionDefinesVector.elementAt(idx) == rd_old){
      regionDefinesVector.setElementAt(rd_new, idx);
      break;
    };
  };
  delete rd_old;
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
