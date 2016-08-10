
#include<colorer/handlers/RegionMapperImpl.h>

const RegionDefine *RegionMapperImpl::enumerateRegionDefines(int idx) const{
  return regionDefines.get(regionDefines.key(idx));
};

const RegionDefine *RegionMapperImpl::getRegionDefine(const Region *region) const{
  if (region == null) return null;
  const RegionDefine *rd = null;
  if (region->getID() < regionDefinesVector.size())
    rd = regionDefinesVector.elementAt(region->getID());
  if (rd != null) return rd;

  if (regionDefinesVector.size() < region->getID()+1)
    regionDefinesVector.setSize(region->getID()*2);

  RegionDefine *rd_new = regionDefines.get(region->getName());
  if (rd_new != null){
    regionDefinesVector.setElementAt(rd_new, region->getID());
    return rd_new;
  };

  if (region->getParent()){
    rd = getRegionDefine(region->getParent());
    regionDefinesVector.setElementAt(rd, region->getID());
  };
  return rd;
};

/** Returns region mapping by it's full qualified name.
*/
const RegionDefine *RegionMapperImpl::getRegionDefine(const String &name) const{
  return regionDefines.get(&name);
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
