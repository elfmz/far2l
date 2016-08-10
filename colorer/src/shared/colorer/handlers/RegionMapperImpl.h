#ifndef _COLORER_REGIONMAPPERIMPL_H_
#define _COLORER_REGIONMAPPERIMPL_H_

#include<common/Vector.h>
#include<common/Hashtable.h>
#include<common/io/Writer.h>
#include<common/io/InputSource.h>
#include<colorer/handlers/RegionMapper.h>
#include<colorer/handlers/RegionDefine.h>

/** Abstract RegionMapper.
    Stores all region mappings in hashtable and sequental vector.
    @ingroup colorer_handlers
*/
class RegionMapperImpl : public RegionMapper
{
public:
  RegionMapperImpl(){};
  ~RegionMapperImpl(){};

  /** Loads region defines from @c is InputSource
  */
  virtual void  loadRegionMappings(InputSource *is) = 0;
  /** Saves all loaded region defines into @c writer.
      Note, that result document would not be equal
      to input one, because there could be multiple input
      documents.
  */
  virtual void  saveRegionMappings(Writer *writer) const = 0;
  /** Changes specified region definition to @c rdnew
      @param region Region full qualified name.
      @param rdnew  New region definition to replace old one
  */
  virtual void  setRegionDefine(const String &region, const RegionDefine *rdnew) = 0;

  /** Enumerates all loaded region defines.
      @return RegionDefine with specified internal index, or null if @c idx is too big
  */
  const RegionDefine *enumerateRegionDefines(int idx) const;

  const RegionDefine *getRegionDefine(const Region *region) const;
  const RegionDefine *getRegionDefine(const String &name) const;

protected:
  Hashtable<RegionDefine*> regionDefines;
  mutable Vector<const RegionDefine*> regionDefinesVector;

  RegionMapperImpl(const RegionMapperImpl&);
  void operator=(const RegionMapperImpl&);
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
