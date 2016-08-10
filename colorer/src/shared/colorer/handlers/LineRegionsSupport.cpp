
#include<colorer/handlers/LineRegionsSupport.h>
#include<common/Logging.h>

LineRegionsSupport::LineRegionsSupport(){
  lineCount = 0;
  firstLineNo = 0;
  regionMapper = null;
  special = null;
};

LineRegionsSupport::~LineRegionsSupport(){
  clear();
  while(schemeStack.size() > 1){
    delete schemeStack.lastElement();
    schemeStack.removeElementAt(schemeStack.size()-1);
  };
};

void LineRegionsSupport::resize(int lineCount)
{
  lineRegions.setSize(lineCount);
  this->lineCount = lineCount;
};

int LineRegionsSupport::size()
{
  return lineCount;
};

void LineRegionsSupport::clear(){
  for(int idx = 0; idx < lineRegions.size(); idx++){
    LineRegion *ln = lineRegions.elementAt(idx);
    lineRegions.setElementAt(null, idx);
    while(ln != null){
      LineRegion *lnn = ln->next;
      delete ln;
      ln = lnn;
    }
  }
}

int LineRegionsSupport::getLineIndex(int lno) const {
  return ((firstLineNo % lineCount) + lno - firstLineNo) % lineCount;
}

LineRegion *LineRegionsSupport::getLineRegions(int lno) const {
  if (!checkLine(lno)){
    return null;
  }
  return lineRegions.elementAt(getLineIndex(lno));
}

void LineRegionsSupport::setFirstLine(int first){
  firstLineNo = first;
}

int LineRegionsSupport::getFirstLine(){
  return firstLineNo;
}

void LineRegionsSupport::setBackground(const RegionDefine *back){
  background.rdef = (RegionDefine*)back;
}

void LineRegionsSupport::setSpecialRegion(const Region *special){
  this->special = special;
}

void LineRegionsSupport::setRegionMapper(const RegionMapper*rs){
  regionMapper = rs;
}

bool LineRegionsSupport::checkLine(int lno) const{
  if (lno < firstLineNo || lno >= firstLineNo+lineCount){
    CLR_WARN("LineRegionsSupport", "checkLine: line %d out of range", lno);
    return false;
  }
  return true;
}

void LineRegionsSupport::startParsing(int lno){
  while(schemeStack.size() > 1){
    delete schemeStack.lastElement();
    schemeStack.removeElementAt(schemeStack.size()-1);
  };
  schemeStack.clear();
  schemeStack.addElement(&background);
}

void LineRegionsSupport::clearLine(int lno, String *line){
  if (!checkLine(lno)) return;

  LineRegion *ln = getLineRegions(lno);
  while(ln != null){
    LineRegion *lnn = ln->next;
    delete ln;
    ln = lnn;
  };
  LineRegion *lfirst = new LineRegion(*schemeStack.lastElement());
  lfirst->start = 0;
  lfirst->end = -1;
  lfirst->next = null;
  lfirst->prev = lfirst;
  lineRegions.setElementAt(lfirst, getLineIndex(lno));
  flowBackground = lfirst;
}

void LineRegionsSupport::addRegion(int lno, String *line, int sx, int ex, const Region* region){
  // ignoring out of cached interval lines
  if (!checkLine(lno)) return;
  LineRegion *lnew = new LineRegion();
  lnew->start = sx;
  lnew->end = ex;
  lnew->region = region;
  lnew->scheme = schemeStack.lastElement()->scheme;
  if (region->hasParent(special))
    lnew->special = true;
  if (regionMapper != null){
    const RegionDefine *rd = regionMapper->getRegionDefine(region);
    if (rd == null) rd = schemeStack.lastElement()->rdef;
    if (rd != null){
      lnew->rdef = rd->clone();
      lnew->rdef->assignParent(schemeStack.lastElement()->rdef);
    };
  };
  addLineRegion(lno, lnew);
}

void LineRegionsSupport::enterScheme(int lno, String *line, int sx, int ex, const Region* region, const Scheme *scheme){
  LineRegion *lr = new LineRegion();
  lr->region = region;
  lr->scheme = scheme;
  lr->start = sx;
  lr->end = -1;
  if (regionMapper != null){
    const RegionDefine *rd = regionMapper->getRegionDefine(region);
    if (rd == null) rd = schemeStack.lastElement()->rdef;
    if (rd != null){
      lr->rdef = rd->clone();
      lr->rdef->assignParent(schemeStack.lastElement()->rdef);
    };
  };
  schemeStack.addElement(lr);
  // ignoring out of cached interval lines
  if (!checkLine(lno)) return;
  // we must skip transparent regions
  if (lr->region != null){
    LineRegion *lr_add = new LineRegion(*lr);
    flowBackground->end = lr_add->start;
    flowBackground = lr_add;
    addLineRegion(lno, lr_add);
  };
}

void LineRegionsSupport::leaveScheme(int lno, String *line, int sx, int ex, const Region* region, const Scheme *scheme){
  const Region* scheme_region = schemeStack.lastElement()->region;
  delete schemeStack.lastElement();
  schemeStack.setSize(schemeStack.size()-1);
  // ignoring out of cached interval lines
  if (!checkLine(lno)) return;
  // we have to skip transparent regions
  if (scheme_region != null){
    LineRegion *lr = new LineRegion(*schemeStack.lastElement());
    lr->start = ex;
    lr->end = -1;
    flowBackground->end = lr->start;
    flowBackground = lr;
    addLineRegion(lno, lr);
  };
}

void LineRegionsSupport::addLineRegion(int lno, LineRegion *lr){
  LineRegion *lstart = getLineRegions(lno);
  lr->next = null;
  lr->prev = lr;
  if (lstart == null)
    lineRegions.setElementAt(lr, getLineIndex(lno));
  else{
    lr->prev = lstart->prev;
    lr->prev->next = lr;
    lstart->prev = lr;
  };
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
