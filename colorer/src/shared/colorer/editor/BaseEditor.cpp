
#include<common/Logging.h>
#include<colorer/editor/BaseEditor.h>

#define IDLE_PARSE(time) (100+time*4)

const int CHOOSE_STR = 4;
const int CHOOSE_LEN = 200 * CHOOSE_STR;

ErrorHandler *eh;

BaseEditor::BaseEditor(ParserFactory *parserFactory, LineSource *lineSource)
{
  if (parserFactory == null || lineSource == null){
    throw Exception(DString("Bad BaseEditor constructor parameters"));
  }
  this->parserFactory = parserFactory;
  this->lineSource = lineSource;

  hrcParser = parserFactory->getHRCParser();
  textParser = parserFactory->createTextParser();

  textParser->setRegionHandler(this);
  textParser->setLineSource(lineSource);

  lrSupport = null;

  invalidLine = 0;
  changedLine = 0;
  backParse = -1;
  lineCount = 0;
  wStart = 0;
  wSize = 20;
  lrSize = wSize*3;
  internalRM = false;
  regionMapper = null;
  regionCompact = false;
  currentFileType = null;

  breakParse = false;
  validationProcess = false;

DString ds_text("def:Text"), ds_syntax("def:Syntax"), ds_special("def:Special");
DString ds_pair_start("def:PairStart"), ds_pair_end("def:PairEnd");
  def_Text = hrcParser->getRegion(&ds_text);
  def_Syntax = hrcParser->getRegion(&ds_syntax);
	def_Special = hrcParser->getRegion(&ds_special);
	def_PairStart = hrcParser->getRegion(&ds_pair_start);
	def_PairEnd = hrcParser->getRegion(&ds_pair_end);

	setRegionCompact(regionCompact);

  rd_def_Text = rd_def_HorzCross = rd_def_VertCross = null;
  eh = parserFactory->getErrorHandler();
}

BaseEditor::~BaseEditor(){
  textParser->breakParse();
  breakParse = true;
  while(validationProcess); /// @todo wait until validation is finished
  if (internalRM) delete regionMapper;
  delete lrSupport;
  delete textParser;
}

void BaseEditor::setRegionCompact(bool compact){
  if (!lrSupport || regionCompact != compact){
    regionCompact = compact;
    remapLRS(true);
  }
}

void BaseEditor::setRegionMapper(RegionMapper *rs){
  if (internalRM) delete regionMapper;
  regionMapper = rs;
  internalRM = false;
  remapLRS(false);
}

void BaseEditor::setRegionMapper(const String *hrdClass, const String *hrdName){
  if (internalRM) delete regionMapper;
  regionMapper = parserFactory->createStyledMapper(hrdClass, hrdName);
  internalRM = true;
  remapLRS(false);
}

void BaseEditor::remapLRS(bool recreate){
  if (recreate || lrSupport == null){
    delete lrSupport;
    if (regionCompact){
      lrSupport = new LineRegionsCompactSupport();
    }else{
      lrSupport = new LineRegionsSupport();
    }
    lrSupport->resize(lrSize);
    lrSupport->clear();
  };
  lrSupport->setRegionMapper(regionMapper);
  lrSupport->setSpecialRegion(def_Special);
  invalidLine = 0;
  rd_def_Text = rd_def_HorzCross = rd_def_VertCross = null;
  if (regionMapper != null){
    rd_def_Text = regionMapper->getRegionDefine(DString("def:Text"));
    rd_def_HorzCross = regionMapper->getRegionDefine(DString("def:HorzCross"));
    rd_def_VertCross = regionMapper->getRegionDefine(DString("def:VertCross"));
  };
}

void BaseEditor::setFileType(FileType *ftype){
  CLR_INFO("BaseEditor", "setFileType:%s", ftype->getName()->getChars());
  currentFileType = ftype;
  textParser->setFileType(currentFileType);
  invalidLine = 0;
}

FileType *BaseEditor::setFileType(const String &fileType){
  currentFileType = hrcParser->getFileType(&fileType);
  setFileType(currentFileType);
  return currentFileType;
}


FileType *BaseEditor::chooseFileTypeCh(const String *fileName, int chooseStr, int chooseLen)
{
  StringBuffer textStart;
  int totalLength = 0;
  for(int i = 0; i < chooseStr; i++)
  {
    String *iLine = lineSource->getLine(i);
    if (iLine == null) break;
    textStart.append(iLine);
    textStart.append(DString("\n"));
    totalLength += iLine->length();
    if (totalLength > chooseLen) break;
  }
  currentFileType = hrcParser->chooseFileType(fileName, &textStart);
  
  int chooseStrNext=currentFileType->getParamValueInt(DString("firstlines"), chooseStr);
  int chooseLenNext=currentFileType->getParamValueInt(DString("firstlinebytes"), chooseLen);
  
  if(chooseStrNext != chooseStr || chooseLenNext != chooseLen)
  {
    currentFileType = chooseFileTypeCh(fileName, chooseStrNext, chooseLenNext);
  }
  return currentFileType;
}

FileType *BaseEditor::chooseFileType(const String *fileName)
{
  if (lineSource == null)
  {
    currentFileType = hrcParser->chooseFileType(fileName, null);
  }
  else
  {
    int chooseStr=CHOOSE_STR, chooseLen=CHOOSE_LEN;
    DString dsd("default");
    FileType *def = hrcParser->getFileType(&dsd);
    if(def)
    {
      chooseStr = def->getParamValueInt(DString("firstlines"), chooseStr);
      chooseLen = def->getParamValueInt(DString("firstlinebytes"), chooseLen);
    }
    
    currentFileType = chooseFileTypeCh(fileName, chooseStr, chooseLen);
  }
  setFileType(currentFileType);
  return currentFileType;
}


FileType *BaseEditor::getFileType(){
  return currentFileType;
}

void BaseEditor::setBackParse(int backParse){
  this->backParse = backParse;
}

void BaseEditor::addRegionHandler(RegionHandler *rh){
  regionHandlers.addElement(rh);
}

void BaseEditor::removeRegionHandler(RegionHandler *rh){
  regionHandlers.removeElement(rh);
}

void BaseEditor::addEditorListener(EditorListener *el){
  editorListeners.addElement(el);
}

void BaseEditor::removeEditorListener(EditorListener *el){
  editorListeners.removeElement(el);
}


PairMatch *BaseEditor::getPairMatch(int lineNo, int linePos)
{
  LineRegion *lrStart = getLineRegions(lineNo);
  if (lrStart == null){
    return null;
  }
  LineRegion *pair = null;
  for(LineRegion *l1 = lrStart; l1; l1 = l1->next){
    if ((l1->region->hasParent(def_PairStart) ||
         l1->region->hasParent(def_PairEnd)) &&
         linePos >= l1->start && linePos <= l1->end)
      pair = l1;
  }
  if (pair != null){
    PairMatch *pm = new PairMatch(pair, lineNo, pair->region->hasParent(def_PairStart));
    pm->setStart(pair);
    return pm;
  }
  return null;
}

PairMatch *BaseEditor::getEnwrappedPairMatch(int lineNo, int pos){
  return null;
}

void BaseEditor::releasePairMatch(PairMatch *pm){
  delete pm;
}

PairMatch *BaseEditor::searchLocalPair(int lineNo, int pos)
{
  int lno;
  int end_line = getLastVisibleLine();
  PairMatch *pm = getPairMatch(lineNo, pos);
  if (pm == null){
    return null;
  }

  lno = pm->sline;

  LineRegion *pair = pm->getStartRef();
  LineRegion *slr = getLineRegions(lno);
  while(true){
    if (pm->pairBalance > 0){
      pair = pair->next;
      while(pair == null){
        lno++;
        if (lno > end_line) break;
        pair = getLineRegions(lno);
      }
      if (lno > end_line) break;
    }else{
      if(pair->prev == slr->prev){ // first region
        lno--;
        if (lno < wStart) break;
        slr = getLineRegions(lno);
        pair = slr;
      }
      if (lno < wStart) break;
      pair = pair->prev;
    }
    if (pair->region->hasParent(def_PairStart)){
      pm->pairBalance++;
    }
    if (pair->region->hasParent(def_PairEnd)){
      pm->pairBalance--;
    }
    if (pm->pairBalance == 0){
      break;
    }
  }
  if (pm->pairBalance == 0){
    pm->eline = lno;
    pm->setEnd(pair);
  }
  return pm;
}

PairMatch *BaseEditor::searchGlobalPair(int lineNo, int pos)
{
  int lno;
  int end_line = lineCount;
  PairMatch *pm = getPairMatch(lineNo, pos);
  if (pm == null){
    return null;
  }

  lno = pm->sline;

  LineRegion *pair = pm->getStartRef();
  LineRegion *slr = getLineRegions(lno);
  while(true){
    if (pm->pairBalance > 0){
      pair = pair->next;
      while(pair == null){
        lno++;
        if (lno > end_line) break;
        pair = getLineRegions(lno);
      }
      if (lno > end_line) break;
    }else{
      if(pair->prev == slr->prev){ // first region
        lno--;
        if (lno < 0) break;
        slr = getLineRegions(lno);
        pair = slr;
      }
      if (lno < 0) break;
      pair = pair->prev;
    }
    if (pair->region->hasParent(def_PairStart)){
      pm->pairBalance++;
    }
    if (pair->region->hasParent(def_PairEnd)){
      pm->pairBalance--;
    }
    if (pm->pairBalance == 0){
      break;
    }
  }
  if (pm->pairBalance == 0){
    pm->eline = lno;
    pm->setEnd(pair);
  }
  return pm;
}


LineRegion *BaseEditor::getLineRegions(int lno){
  /*
   * Backparse value check
   */
  if (backParse > 0 && lno - invalidLine > backParse){
    return null;
  }
  validate(lno, true);
  return lrSupport->getLineRegions(lno);
}

void BaseEditor::modifyEvent(int topLine){
  CLR_TRACE("BaseEditor", "modifyEvent:%d", topLine);
  if (invalidLine > topLine){
    invalidLine = topLine;
    for(int idx = editorListeners.size()-1; idx >= 0; idx--){
      editorListeners.elementAt(idx)->modifyEvent(topLine);
    }
  }
}

void BaseEditor::modifyLineEvent(int line){
  if (invalidLine > line){
    invalidLine = line;
  }
  // changedLine = topLine;!!!
}

void BaseEditor::visibleTextEvent(int wStart, int wSize){
  CLR_TRACE("BaseEditor", "visibleTextEvent:%d-%d", wStart, wSize);
  this->wStart = wStart;
  this->wSize = wSize;
}

void BaseEditor::lineCountEvent(int newLineCount){
  CLR_TRACE("BaseEditor", "lineCountEvent:%d", newLineCount);
  lineCount = newLineCount;
}


inline int BaseEditor::getLastVisibleLine(){
  int r1 = (wStart+wSize);
  int r2 = lineCount;
  return ((r1 > r2)?r2:r1)-1;
}

void BaseEditor::validate(int lno, bool rebuildRegions)
{
  int parseFrom, parseTo;
  bool layoutChanged = false;
  TextParseMode tpmode = TPM_CACHE_READ;

  if (lno == -1 || lno > lineCount){
    lno = lineCount-1;
  }

  int firstLine = lrSupport->getFirstLine();
  parseFrom = parseTo = (wStart+wSize);

  /*
   * Calculate changes, required by new screen position, if any
   */
  if (lrSize != wSize*2){
    lrSize = wSize*2;
    lrSupport->resize(lrSize);
    lrSupport->clear();
    // Regions were dropped
    layoutChanged = true;
    CLR_TRACE("BaseEditor", "lrSize != wSize*2");
  }

  /* Fixes window position according to line number */
  if (lno < wStart || lno > wStart+wSize){
    wStart = lno;
    //if enable, introduces heavy delays on pair searching
    //layoutChanged = true;
  }

  if (layoutChanged || wStart < firstLine || wStart+wSize > firstLine+lrSize){
    /*
     * visible area is shifted and line regions
     * should be rearranged according to
     */
    int newFirstLine = (wStart/wSize)*wSize;
    parseFrom = newFirstLine;
    parseTo   = newFirstLine+lrSize;
    /*
     * Change LineRegions parameters only in case
     * of validate-for-usage request.
     */
    if (rebuildRegions){
      lrSupport->setFirstLine(newFirstLine);
    }
    /* Save time - already has the info in line cache */
    if (!layoutChanged && firstLine - newFirstLine == wSize){
      parseTo -= wSize-1;
    }
    firstLine = newFirstLine;
    layoutChanged = true;
    CLR_TRACE("BaseEditor", "newFirstLine=%d, parseFrom=%d, parseTo=%d", firstLine, parseFrom, parseTo);
  }

  if (!layoutChanged){
    /* Text modification only event */
    if (invalidLine <= parseTo){
      parseFrom = invalidLine;
      tpmode = TPM_CACHE_UPDATE;
    }
  }

  /* Text modification general ajustment */
  if (invalidLine <= parseFrom){
    parseFrom = invalidLine;
    tpmode = TPM_CACHE_UPDATE;
  }

  if (parseTo > lineCount){
    parseTo = lineCount;
  }

  /* Runs parser */
  if (parseTo-parseFrom > 0){

    CLR_TRACE("BaseEditor", "validate:parse:%d-%d, %s", parseFrom, parseTo, tpmode == TPM_CACHE_READ?"READ":"UPDATE");

    int stopLine = textParser->parse(parseFrom, parseTo-parseFrom, tpmode);

    if (tpmode == TPM_CACHE_UPDATE){
      invalidLine = stopLine+1;
    }
    CLR_TRACE("BaseEditor", "validate:parsed: invalidLine=%d", invalidLine);
  }
}

void BaseEditor::idleJob(int time)
{
  if (invalidLine < lineCount) {
    if (time < 0) time = 0;
    if (time > 1000) time = 1000;
    validate(invalidLine+IDLE_PARSE(time), false);
  }
}

void BaseEditor::startParsing(int lno){
  lrSupport->startParsing(lno);
  for(int idx = 0; idx < regionHandlers.size(); idx++)
    regionHandlers.elementAt(idx)->startParsing(lno);
}
void BaseEditor::endParsing(int lno){
  lrSupport->endParsing(lno);
  for(int idx = 0; idx < regionHandlers.size(); idx++)
    regionHandlers.elementAt(idx)->endParsing(lno);
}
void BaseEditor::clearLine(int lno, String *line){
  lrSupport->clearLine(lno, line);
  for(int idx = 0; idx < regionHandlers.size(); idx++)
    regionHandlers.elementAt(idx)->clearLine(lno, line);
}
void BaseEditor::addRegion(int lno, String *line, int sx, int ex, const Region *region){
  lrSupport->addRegion(lno, line, sx, ex, region);
  for(int idx = 0; idx < regionHandlers.size(); idx++)
    regionHandlers.elementAt(idx)->addRegion(lno, line, sx, ex, region);
}
void BaseEditor::enterScheme(int lno, String *line, int sx, int ex, const Region *region, const Scheme *scheme){
  lrSupport->enterScheme(lno, line, sx, ex, region, scheme);
  for(int idx = 0; idx < regionHandlers.size(); idx++)
    regionHandlers.elementAt(idx)->enterScheme(lno, line, sx, ex, region, scheme);
}
void BaseEditor::leaveScheme(int lno, String *line, int sx, int ex, const Region *region, const Scheme *scheme){
  lrSupport->leaveScheme(lno, line, sx, ex, region, scheme);
  for(int idx = 0; idx < regionHandlers.size(); idx++)
    regionHandlers.elementAt(idx)->leaveScheme(lno, line, sx, ex, region, scheme);
}

bool BaseEditor::haveInvalidLine()
{ 
  return invalidLine < lineCount;
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
 * decision by deleting thd provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
