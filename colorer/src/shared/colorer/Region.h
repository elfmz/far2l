#ifndef _COLORER_REGION_H_
#define _COLORER_REGION_H_

#include<common/Common.h>

/**
  HRC Region implementation.
  Contains information about HRC Region and it attributes:
  <ul>
    <li>name
    <li>description
    <li>parent
  </ul>
  @ingroup colorer
*/
class Region{
public:
  /** Full Qualified region name (<code>def:Text</code> for example) */
  virtual const String *getName() const{ return name; };
  /** Region description */
  virtual const String *getDescription() const{ return description; };
  /** Direct region ancestor (parent) */
  virtual const Region *getParent() const{ return parent; };
  /** Quick access region id (incrementable) */
  virtual int getID() const{ return id; };
  /** Checks if region has the specified parent in all of it's ancestors.
      This method is useful to check if region has specified parent,
      and use this information, as region type specification.
      For example, <code>def:Comment</code> has <code>def:Syntax</code> parent,
      so, some syntax checking can be made with it's content.
  */
  bool hasParent(const Region *region) const{
    const Region *elem = this;
    while(elem != null){
      if (region == elem) return true;
      elem = elem->getParent();
    };
    return false;
  };
  /**
    Basic constructor.
    Used only by HRCParser.
  */
  Region(const String *_name, const String* _description, const Region *_parent, int _id){
    name = new SString(_name);
    description = null;
    if (_description != null) description = new SString(_description);
    parent = _parent;
    id = _id;
  };
  virtual ~Region(){
    delete name;
    delete description;
  };
protected:
  /** Internal members */
  String *name, *description;
  const Region *parent;
  int id;
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
