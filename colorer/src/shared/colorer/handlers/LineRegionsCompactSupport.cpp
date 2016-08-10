
#include<colorer/handlers/LineRegionsCompactSupport.h>

LineRegionsCompactSupport::LineRegionsCompactSupport(){};
LineRegionsCompactSupport::~LineRegionsCompactSupport(){};

void LineRegionsCompactSupport::addLineRegion(int lno, LineRegion *ladd)
{
  LineRegion *lstart = getLineRegions(lno);
  ladd->next = null;
  ladd->prev = ladd;

  if (ladd->special){
    // adds last and returns
    if (lstart == null){
      lineRegions.setElementAt(ladd, getLineIndex(lno));
    }else{
      ladd->prev = lstart->prev;
      lstart->prev->next = ladd;
      lstart->prev = ladd;
    };
    return;
  };
  if (lstart == null){
    lineRegions.setElementAt(ladd, getLineIndex(lno));
    return;
  };

  // finds position of new 'ladd' region
  for(LineRegion *ln = lstart; ln; ln = ln->next){
    // insert before first
    if (lstart->start >= ladd->start){
      ladd->next = lstart;
      ladd->prev = lstart->prev;
      lstart->prev = ladd;
      lstart = ladd;
      break;
    };
    // insert before
    if (ln->start == ladd->start){
      ln->prev->next = ladd;
      ladd->next = ln;
      ladd->prev = ln->prev;
      ln->prev = ladd;
      break;
    };
    // add last
    if (ln->start < ladd->start && !ln->next){
      ln->next = ladd;
      ladd->next = null;
      ladd->prev = ln;
      lstart->prev = ladd;
      break;
    };
    // insert between or before special
    if (ln->start < ladd->start && (ln->next->start > ladd->start || ln->next->special)){
      ladd->next = ln->next;
      ladd->prev = ln;
      ln->next->prev = ladd;
      ln->next = ladd;
      break;
    };
  };
  // previous region intersection check
  if (ladd != lstart && ladd->prev && (ladd->prev->end > ladd->start || ladd->prev->end == -1)){
    // our region breaks previous region into two parts
    if ((ladd->prev->end > ladd->end || ladd->prev->end == -1) && ladd->end != -1){
      LineRegion *ln1 = new LineRegion(*ladd->prev);
      ln1->prev = ladd;
      ln1->next = ladd->next;
      if (ladd->next) ladd->next->prev = ln1;
      if (ln1->next == null) lstart->prev = ln1;
      ladd->next = ln1;
      ln1->start = ladd->end;
      if (ladd->prev == flowBackground) flowBackground = ln1;
    };
    ladd->prev->end = ladd->start;
    // zero-width region deletion
    if (ladd->prev != lstart && ladd->prev->end == ladd->prev->start){
      ladd->prev->prev->next = ladd;
      LineRegion *lntemp = ladd->prev;
      ladd->prev = ladd->prev->prev;
      delete lntemp;
    };
    if (ladd->prev == lstart && ladd->prev->end == ladd->prev->start){
      LineRegion *lntemp = ladd->prev->prev;
      delete ladd->prev;
      ladd->prev = lntemp;
      lstart = ladd;
    };
  };
  // possible forward intersections
  for(LineRegion *lnext = ladd->next; lnext; lnext = lnext->next){
    if (lnext->special) continue;
    if ((lnext->end == -1 || lnext->end > ladd->end) && ladd->end != -1
      && lnext->start < ladd->end ){
      lnext->start = ladd->end;
    };
    // make region zero-width, if it is hided by our new region
    if ((lnext->end <= ladd->end && lnext->end != -1) || ladd->end == -1){
      ladd->next = lnext->next;
      if (lnext->next) lnext->next->prev = ladd;
      else lstart->prev = ladd;
      delete lnext;
      lnext = ladd;
      continue;
    };
  };
  lineRegions.setElementAt(lstart, getLineIndex(lno));
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
