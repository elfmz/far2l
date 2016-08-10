#ifndef _COLORER_REGIONDEFINE_H_
#define _COLORER_REGIONDEFINE_H_

/**
 * Enumeration to distinguish different types of region mapping
 * Do not use RTTI because of compatibility problems
 *
 * @ingroup colorer_handlers
 */
enum RegionDefineType{
  UNKNOWN_REGION = 0,
  STYLED_REGION  = 1,
  TEXT_REGION    = 2,
};

/**
 * Object contains information about region mapping into real colors or other properties.
 * This class represents abstract mapping information and declares required methods
 * to be implemented in it's subclasses.
 *
 * @ingroup colorer_handlers
 */
class RegionDefine{
public:
  
  /**
   * Class type identifier
   */
  RegionDefineType type;

  /**
   * Completes region define values with it's parent values.
   * If region define has some incomplete information (fe some
   * transparent fields), this methods completes them with
   * passed parent's values.
   */
  virtual void assignParent(const RegionDefine *parent) = 0;

  /**
   * Direct assign of all passed @c rd values.
   * Copies all information from passed definition into
   * this region.
   */
  virtual void setValues(const RegionDefine *rd) = 0;

  /**
   * Assign operator. Clones all values.
   * Works as setValues method.
   */
  virtual RegionDefine &operator=(const RegionDefine &rd){
    setValues(&rd);
    return *this;
  }

  /**
   * Clones current region and creates it's duplicate.
   * To be implemented in subclasses.
   */
  virtual RegionDefine *clone() const = 0;

  /** Default Destructor */
  virtual ~RegionDefine(){};
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
 * Cail Lomecb <irusskih at gmail.com>.
 * Portions created by the Initial Developer are Copyright (C) 1999-2007
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
