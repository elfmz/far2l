#include "FarEditor.h"
#include <vector>

const UnicodeString DShowCross("show-cross");
const UnicodeString DNone("none");
const UnicodeString DVertical("vertical");
const UnicodeString DHorizontal("horizontal");
const UnicodeString DBoth("both");
const UnicodeString DCrossZorder("cross-zorder");
const UnicodeString DBottom("bottom");
const UnicodeString DTop("top");
const UnicodeString DYes("yes");
const UnicodeString DNo("no");
const UnicodeString DTrue("true");
const UnicodeString DFalse("false");
const UnicodeString DBackparse("backparse");
const UnicodeString DMaxLen("maxlinelength");
const UnicodeString DDefFore("default-fore");
const UnicodeString DDefBack("default-back");
const UnicodeString DFullback("fullback");
const UnicodeString DHotkey("hotkey");
const UnicodeString DFavorite("favorite");
const UnicodeString DFirstLines("firstlines");
const UnicodeString DFirstLineBytes("firstlinebytes");

FarEditor::FarEditor(PluginStartupInfo* inf, ParserFactory* pf) : info(inf), parserFactory(pf)
{
  UnicodeString dso("def:Outlined");
  UnicodeString dse("def:Error");
  baseEditor = std::make_unique<BaseEditor>(parserFactory, this);
  const Region* def_Outlined = parserFactory->getHrcLibrary().getRegion(&dso);
  const Region* def_Error = parserFactory->getHrcLibrary().getRegion(&dse);
  structOutliner = std::make_unique<Outliner>(baseEditor.get(), def_Outlined);
  errorOutliner = std::make_unique<Outliner>(baseEditor.get(), def_Error);
}

void FarEditor::endJob(size_t lno)
{
  (void) lno;
  ret_str.reset();
}

UnicodeString* FarEditor::getLine(size_t lno)
{
  EditorGetString es {(int) lno};

  int len = 0;
  if (info->EditorControl(ECTL_GETSTRING, &es)) {
    len = es.StringLength;
    if (len > maxLineLength && maxLineLength > 0) {
      len = maxLineLength;
    }
    ret_str = std::make_unique<UnicodeString>((char*) es.StringText, len * sizeof(wchar_t),
                                              Encodings::ENC_UTF32);
    return ret_str.get();
  }
  return nullptr;
}

void FarEditor::chooseFileType(const UnicodeString* fname)
{
  FileType* ftype = baseEditor->chooseFileType(fname);
  setFileType(ftype);
}

void FarEditor::setFileType(FileType* ftype)
{
  baseEditor->setFileType(ftype);
  // clear Outliner
  structOutliner->modifyEvent(0);
  errorOutliner->modifyEvent(0);
  reloadTypeSettings();
}

void FarEditor::reloadTypeSettings()
{
  FileType* ftype = baseEditor->getFileType();
  auto& hrcParser = parserFactory->getHrcLibrary();
  UnicodeString ds("default");
  FileType* def = hrcParser.getFileType(&ds);

  if (def == nullptr) {
    throw Exception("No 'default' file type found");
  }

  int backparse = def->getParamValueInt(DBackparse, 2000);
  maxLineLength = def->getParamValueInt(DMaxLen, 0);
  newfore = def->getParamValueHex(DDefFore, -1);
  newback = def->getParamValueHex(DDefBack, -1);
  const UnicodeString* value = def->getParamValue(DFullback);

  if (value != nullptr && value->equals(&DNo)) {
    fullBackground = false;
  }

  value = def->getParamValue(DShowCross);
  if (drawCross == 2 && value != nullptr) {
    if (value->equals(&DNone)) {
      showHorizontalCross = false;
      showVerticalCross = false;
    }

    if (value->equals(&DVertical)) {
      showHorizontalCross = false;
      showVerticalCross = true;
    }

    if (value->equals(&DHorizontal)) {
      showHorizontalCross = true;
      showVerticalCross = false;
    }

    if (value->equals(&DBoth)) {
      showHorizontalCross = true;
      showVerticalCross = true;
    }
  }

  value = def->getParamValue(DCrossZorder);

  if (value != nullptr && value->equals(&DTop)) {
    crossZOrder = 1;
  }

  // installs custom file properties
  backparse = ftype->getParamValueInt(DBackparse, backparse);
  maxLineLength = ftype->getParamValueInt(DMaxLen, maxLineLength);
  newfore = ftype->getParamValueHex(DDefFore, newfore);
  newback = ftype->getParamValueHex(DDefBack, newback);
  value = ftype->getParamValue(DFullback);

  if (value != nullptr && value->equals(&DNo)) {
    fullBackground = false;
  }

  value = ftype->getParamValue(DShowCross);

  if (drawCross == 2 && value != nullptr) {
    if (value->equals(&DNone)) {
      showHorizontalCross = false;
      showVerticalCross = false;
    }

    if (value->equals(&DVertical)) {
      showHorizontalCross = false;
      showVerticalCross = true;
    }

    if (value->equals(&DHorizontal)) {
      showHorizontalCross = true;
      showVerticalCross = false;
    }

    if (value->equals(&DBoth)) {
      showHorizontalCross = true;
      showVerticalCross = true;
    }
  }

  value = ftype->getParamValue(DCrossZorder);

  if (value != nullptr && value->equals(&DTop)) {
    crossZOrder = 1;
  }

  baseEditor->setBackParse(backparse);
}

FileType* FarEditor::getFileType() const
{
  return baseEditor->getFileType();
}

void FarEditor::setDrawCross(int _drawCross)
{
  drawCross = _drawCross;
  switch (drawCross) {
    case 0:
      showHorizontalCross = false;
      showVerticalCross = false;
      break;
    case 1:
      showHorizontalCross = true;
      showVerticalCross = true;
      break;
    case 2:
      reloadTypeSettings();
      break;
  }
}

void FarEditor::setDrawPairs(bool drawPairs)
{
  this->drawPairs = drawPairs;
}

void FarEditor::setDrawSyntax(bool drawSyntax)
{
  this->drawSyntax = drawSyntax;
}

void FarEditor::setOutlineStyle(bool oldStyle)
{
  this->oldOutline = oldStyle;
}

void FarEditor::setTrueMod(bool true_mod)
{
  this->TrueMod = true_mod;
}

void FarEditor::setRegionMapper(RegionMapper* rs)
{
  baseEditor->setRegionMapper(rs);
  rdBackground = StyledRegion::cast(baseEditor->rd_def_Text);
  horzCrossColor = convert(StyledRegion::cast(baseEditor->rd_def_HorzCross));
  vertCrossColor = convert(StyledRegion::cast(baseEditor->rd_def_VertCross));

  // TODO
  if (horzCrossColor.concolor == 0)
    horzCrossColor.concolor = 0x0E;
  if (vertCrossColor.concolor == 0)
    vertCrossColor.concolor = 0x0E;
}

void FarEditor::matchPair() const
{
  const auto ei = getEditorInfo();
  PairMatch* pm = baseEditor->searchGlobalPair(ei.CurLine, ei.CurPos);

  if ((pm == nullptr) || (pm->eline == -1)) {
    baseEditor->releasePairMatch(pm);
    return;
  }

  EditorSetPosition esp {pm->eline, 0, -1, -1, -1, -1};

  if (!pm->topPosition) {
    esp.CurPos = pm->end->start;
  }
  else {
    esp.CurPos = pm->end->end - 1;
  }

  if (esp.CurLine < ei.TopScreenLine || esp.CurLine > ei.TopScreenLine + ei.WindowSizeY) {
    esp.TopScreenLine = pm->eline - ei.WindowSizeY / 2;

    if (esp.TopScreenLine < 0) {
      esp.TopScreenLine = 0;
    }
  }

  info->EditorControl(ECTL_SETPOSITION, &esp);
  baseEditor->releasePairMatch(pm);
}

void FarEditor::selectPair() const
{
  int X1, X2, Y1, Y2;
  const auto ei = getEditorInfo();
  PairMatch* pm = baseEditor->searchGlobalPair(ei.CurLine, ei.CurPos);

  if (pm == nullptr || pm->eline == -1) {
    baseEditor->releasePairMatch(pm);
    return;
  }

  if (pm->topPosition) {
    X1 = pm->start->end;
    X2 = pm->end->start - 1;
    Y1 = pm->sline;
    Y2 = pm->eline;
  }
  else {
    X2 = pm->start->start - 1;
    X1 = pm->end->end;
    Y2 = pm->sline;
    Y1 = pm->eline;
  }

  EditorSelect es {BTYPE_STREAM, Y1, X1, X2 - X1 + 1, Y2 - Y1 + 1};
  info->EditorControl(ECTL_SELECT, &es);

  baseEditor->releasePairMatch(pm);
}

void FarEditor::selectBlock() const
{
  int X1, X2, Y1, Y2;
  const auto ei = getEditorInfo();
  PairMatch* pm = baseEditor->searchGlobalPair(ei.CurLine, ei.CurPos);

  if (pm == nullptr || pm->eline == -1) {
    baseEditor->releasePairMatch(pm);
    return;
  }

  if (pm->topPosition) {
    X1 = pm->start->start;
    X2 = pm->end->end - 1;
    Y1 = pm->sline;
    Y2 = pm->eline;
  }
  else {
    X2 = pm->start->end - 1;
    X1 = pm->end->start;
    Y2 = pm->sline;
    Y1 = pm->eline;
  }

  EditorSelect es {BTYPE_STREAM, Y1, X1, X2 - X1 + 1, Y2 - Y1 + 1};
  info->EditorControl(ECTL_SELECT, &es);

  baseEditor->releasePairMatch(pm);
}

void FarEditor::selectRegion() const
{
  const auto ei = getEditorInfo();
  EditorGetString egs {ei.CurLine};
  info->EditorControl(ECTL_GETSTRING, &egs);

  if (cursorRegion != nullptr) {
    int end = cursorRegion->end;

    if (end == -1) {
      end = egs.StringLength;
    }

    if (end - cursorRegion->start > 0) {
      EditorSelect es {BTYPE_STREAM, ei.CurLine, cursorRegion->start, end - cursorRegion->start, 1};
      info->EditorControl(ECTL_SELECT, &es);
    }
  }
}

void FarEditor::listFunctions()
{
  baseEditor->validate(-1, false);
  showOutliner(structOutliner.get());
}

void FarEditor::listErrors()
{
  baseEditor->validate(-1, false);
  showOutliner(errorOutliner.get());
}

void FarEditor::locateFunction()
{
  // extract word
  auto ei = getEditorInfo();
  UnicodeString& curLine = *getLine(ei.CurLine);
  int cpos = ei.CurPos;
  int sword = cpos;
  int eword = cpos;

  while (cpos < curLine.length() &&
         (Character::isLetterOrDigit(curLine[cpos]) || curLine[cpos] != '_'))
  {
    while (Character::isLetterOrDigit(curLine[eword]) || curLine[eword] == '_') {
      if (eword == curLine.length() - 1) {
        break;
      }

      eword++;
    }

    while (Character::isLetterOrDigit(curLine[sword]) || curLine[sword] == '_') {
      if (sword == 0) {
        break;
      }

      sword--;
    }

    UnicodeString funcname(curLine, sword + 1, eword - sword - 1);
    COLORER_LOG_DEBUG("FC] Letter %", funcname);
    baseEditor->validate(-1, false);
    OutlineItem* item_found = nullptr;
    OutlineItem* item_last = nullptr;
    int items_num = structOutliner->itemCount();

    if (items_num == 0) {
      break;
    }

    // search through the outliner
    for (int idx = 0; idx < items_num; idx++) {
      OutlineItem* item = structOutliner->getItem(idx);

      if (item->token->indexOfIgnoreCase(UnicodeString(funcname)) != -1) {
        if (item->lno == (size_t) ei.CurLine) {
          item_last = item;
        }
        else {
          item_found = item;
        }
      }
    }

    if (!item_found) {
      item_found = item_last;
    }

    if (!item_found) {
      break;
    }

    EditorSetPosition esp {(int) item_found->lno,
                           item_found->pos,
                           -1,
                           (int) item_found->lno - ei.WindowSizeY / 2,
                           -1,
                           -1};

    if (esp.TopScreenLine < 0) {
      esp.TopScreenLine = 0;
    }

    info->EditorControl(ECTL_SETPOSITION, &esp);
    info->EditorControl(ECTL_REDRAW, nullptr);
    info->EditorControl(ECTL_GETINFO, &ei);
    return;
  }

  const wchar_t* msg[2] = {GetMsg(mNothingFound), GetMsg(mGotcha)};
  info->Message(info->ModuleNumber, 0, nullptr, msg, 2, 1);
}

void FarEditor::updateHighlighting()
{
  const auto ei = getEditorInfo();
  baseEditor->validate(ei.TopScreenLine, true);
}

int FarEditor::editorInput(const INPUT_RECORD* ir)
{
  if (ir->EventType == KEY_EVENT && ir->Event.KeyEvent.wVirtualKeyCode == 0) {
    if (baseEditor->haveInvalidLine()) {
      auto invalid_line1 = baseEditor->getInvalidLine();
      idleCount++;
      if (idleCount > 10) {
        idleCount = 10;
      }
      baseEditor->idleJob(idleCount * 10);
      auto invalid_line2 = baseEditor->getInvalidLine();

      EditorInfo ei = getEditorInfo();
      if ((invalid_line1 < ei.TopScreenLine && invalid_line2 >= ei.TopScreenLine) ||
          (invalid_line1 < ei.TopScreenLine + ei.WindowSizeY &&
           invalid_line2 >= ei.TopScreenLine + ei.WindowSizeY))
      {
        info->EditorControl(ECTL_REDRAW, nullptr);
      }
    }
  }
  else if (ir->EventType == KEY_EVENT) {
    idleCount = 0;
  }

  return 0;
}

int FarEditor::editorEvent(int event, void* param)
{
  // ignore event
  if (event != EE_REDRAW || (event == EE_REDRAW && param == EEREDRAW_ALL && inRedraw)) {
    return 0;
  }

  const auto ei = getEditorInfo();
  WindowSizeX = ei.WindowSizeX;
  WindowSizeY = ei.WindowSizeY;

  baseEditor->visibleTextEvent(ei.TopScreenLine, WindowSizeY);

  baseEditor->lineCountEvent(ei.TotalLines);

  if (param == EEREDRAW_CHANGE) {
    int ml = (prevLinePosition < ei.CurLine ? prevLinePosition : ei.CurLine) - 1;

    if (ml < 0) {
      ml = 0;
    }

    if (blockTopPosition != -1 && ml > blockTopPosition) {
      ml = blockTopPosition;
    }

    baseEditor->modifyEvent(ml);
  }

  prevLinePosition = ei.CurLine;
  blockTopPosition = -1;

  if (ei.BlockType != BTYPE_NONE) {
    blockTopPosition = ei.BlockStartLine;
  }

  // hack against tabs in FAR's editor
  EditorConvertPos ecp {-1, ei.CurPos};
  EditorConvertPos ecp_cl {};
  info->EditorControl(ECTL_REALTOTAB, &ecp);
  cursorRegion.reset();

  if (rdBackground == nullptr) {
    throw Exception("HRD Background region 'def:Text' not found");
  }

  for (int lno = ei.TopScreenLine; lno < ei.TopScreenLine + WindowSizeY; lno++) {
    if (lno >= ei.TotalLines) {
      break;
    }

    LineRegion* l1 = nullptr;

    if (drawSyntax || drawPairs) {
      l1 = baseEditor->getLineRegions(lno);
    }

    // clean line in far editor
    addFARColor(lno, -1, 0, color());
    EditorGetString egs {lno};
    info->EditorControl(ECTL_GETSTRING, &egs);
    int llen = egs.StringLength;

    // fills back
    if (lno == ei.CurLine && showHorizontalCross) {
      addFARColor(lno, 0, ei.LeftPos + ei.WindowSizeX, horzCrossColor);
    }
    else {
      addFARColor(lno, 0, ei.LeftPos + ei.WindowSizeX, convert(nullptr));
    }

    if (showVerticalCross) {
      auto col = vertCrossColor;
      ecp_cl.StringNumber = lno;
      ecp_cl.SrcPos = ecp.DestPos;
      info->EditorControl(ECTL_TABTOREAL, &ecp_cl);
      if (!TrueMod) {
        col.concolor |= 0x10000;
      }
      addFARColor(lno, ecp_cl.DestPos, ecp_cl.DestPos + 1, col);
    }

    bool vertCrossDone = false;

    if (drawSyntax) {
      for (; l1; l1 = l1->next) {
        if (l1->special) {
          continue;
        }
        if (l1->start == l1->end) {
          continue;
        }
        if (l1->start > ei.LeftPos + ei.WindowSizeX) {
          continue;
        }
        if (l1->end != -1 && l1->end < ei.LeftPos - ei.WindowSizeX) {
          continue;
        }

        if ((lno != ei.CurLine || !showHorizontalCross || crossZOrder == 0)) {
          color col = convert(l1->styled());

          if (lno == ei.CurLine && showHorizontalCross) {
            if (foreDefault(col)) {
              if (!TrueMod) {
                col.concolor = (col.concolor & 0xF0) + (horzCrossColor.concolor & 0xF);
              }
              else {
                col.fg = horzCrossColor.fg;
              }
            }

            if (backDefault(col)) {
              if (!TrueMod) {
                col.concolor = (col.concolor & 0xF) + (horzCrossColor.concolor & 0xF0);
              }
              else {
                col.bk = horzCrossColor.bk;
              }
            }
          }
          if (!col.concolor) {
            continue;
          }
          //
          int lend = l1->end;

          if (lend == -1) {
            if (fullBackground) {
              addFARColor(lno, llen, ei.LeftPos + ei.WindowSizeX * 2, col, false);
            }
            lend = llen;
          }

          addFARColor(lno, l1->start, lend, col);

          if (lno == ei.CurLine && (l1->start <= ei.CurPos) && (ei.CurPos <= lend)) {
            cursorRegion = std::make_unique<LineRegion>(*l1);
          }

          // column
          if (showVerticalCross && crossZOrder == 0 && l1->start <= ecp_cl.DestPos &&
              ecp_cl.DestPos < lend)
          {
            col = convert(l1->styled());

            if (foreDefault(col)) {
              if (!TrueMod) {
                col.concolor = (col.concolor & 0xF0) + (vertCrossColor.concolor & 0xF);
              }
              else {
                col.fg = horzCrossColor.fg;
              }
            }

            if (backDefault(col)) {
              if (!TrueMod) {
                col.concolor = (col.concolor & 0xF) + (vertCrossColor.concolor & 0xF0);
              }
              else {
                col.bk = horzCrossColor.bk;
              }
            }

            ecp_cl.StringNumber = lno;
            ecp_cl.SrcPos = ecp.DestPos;
            info->EditorControl(ECTL_TABTOREAL, &ecp_cl);
            if (!TrueMod) {
              col.concolor |= 0x10000;
            }
            addFARColor(lno, ecp_cl.DestPos, ecp_cl.DestPos + 1, col);
            vertCrossDone = true;
          }
        }
      }
    }
    if (showVerticalCross && !vertCrossDone) {
      ecp_cl.StringNumber = lno;
      ecp_cl.SrcPos = ecp.DestPos;
      info->EditorControl(ECTL_TABTOREAL, &ecp_cl);
      if (!TrueMod) {
        vertCrossColor.concolor |= 0x10000;
      }
      addFARColor(lno, ecp_cl.DestPos, ecp_cl.DestPos + 1, vertCrossColor);
    }
  }

  /// pair brackets
  PairMatch* pm = nullptr;

  if (drawPairs) {
    pm = baseEditor->searchLocalPair(ei.CurLine, ei.CurPos);
  }

  if (pm != nullptr) {
    color col = convert(pm->start->styled());

    if (showHorizontalCross) {
      if (foreDefault(col)) {
        if (!TrueMod) {
          col.concolor = (col.concolor & 0xF0) + (horzCrossColor.concolor & 0xF);
        }
        else {
          col.fg = horzCrossColor.fg;
        }
      }

      if (backDefault(col)) {
        if (!TrueMod) {
          col.concolor = (col.concolor & 0xF) + (horzCrossColor.concolor & 0xF0);
        }
        else {
          col.bk = horzCrossColor.bk;
        }
      }
    }
    //
    addFARColor(ei.CurLine, pm->start->start, pm->start->end, col);

    // TODO
    if (showVerticalCross && !showHorizontalCross && pm->start->start <= ei.CurPos &&
        ei.CurPos < pm->start->end)
    {
      col = convert(pm->start->styled());

      if (foreDefault(col)) {
        if (!TrueMod) {
          col.concolor = (col.concolor & 0xF0) + (vertCrossColor.concolor & 0xF);
        }
        else {
          col.fg = vertCrossColor.fg;
        }
      }

      if (backDefault(col)) {
        if (!TrueMod) {
          col.concolor = (col.concolor & 0xF) + (vertCrossColor.concolor & 0xF0);
        }
        else {
          col.bk = vertCrossColor.bk;
        }
      }

      col.concolor |= 0x10000;
      addFARColor(pm->sline, ei.CurPos, ei.CurPos + 1, col);
    }
    //
    if (pm->eline != -1) {
      col = convert(pm->end->styled());

      //
      if (showHorizontalCross && pm->eline == ei.CurLine) {
        if (foreDefault(col)) {
          if (!TrueMod) {
            col.concolor = (col.concolor & 0xF0) + (horzCrossColor.concolor & 0xF);
          }
          else {
            col.fg = horzCrossColor.fg;
          }
        }

        if (backDefault(col)) {
          if (!TrueMod) {
            col.concolor = (col.concolor & 0xF) + (horzCrossColor.concolor & 0xF0);
          }
          else {
            col.bk = horzCrossColor.bk;
          }
        }
      }
      //
      addFARColor(pm->eline, pm->end->start, pm->end->end, col);
      ecp.StringNumber = pm->eline;
      ecp.SrcPos = ecp.DestPos;
      info->EditorControl(ECTL_TABTOREAL, &ecp);

      //
      if (showVerticalCross && pm->end->start <= ecp.DestPos && ecp.DestPos < pm->end->end) {
        col = convert(pm->end->styled());

        if (foreDefault(col)) {
          if (!TrueMod) {
            col.concolor = (col.concolor & 0xF0) + (vertCrossColor.concolor & 0xF);
          }
          else {
            col.fg = vertCrossColor.fg;
          }
        }

        if (backDefault(col)) {
          if (!TrueMod) {
            col.concolor = (col.concolor & 0xF0) + (vertCrossColor.concolor & 0xF);
          }
          else {
            col.bk = vertCrossColor.bk;
          }
        }

        col.concolor |= 0x10000;
        addFARColor(pm->eline, ecp.DestPos, ecp.DestPos + 1, col);
      }
    }

    baseEditor->releasePairMatch(pm);
  }

  if (param != EEREDRAW_ALL) {
    inRedraw = true;
    info->EditorControl(ECTL_REDRAW, nullptr);
    inRedraw = false;
  }

  return true;
}

void FarEditor::showOutliner(Outliner* outliner)
{
  FarMenuItem* menu;
  EditorSetPosition esp {};
  bool moved = false;
  int code = 0;
  const int FILTER_SIZE = 40;
  int breakKeys[] = {VK_BACK,
                     VK_RETURN,
                     VK_OEM_1,
                     VK_OEM_MINUS,
                     VK_TAB,
                     (PKF_CONTROL << 16) + VK_UP,
                     (PKF_CONTROL << 16) + VK_DOWN,
                     (PKF_CONTROL << 16) + VK_LEFT,
                     (PKF_CONTROL << 16) + VK_RIGHT,
                     (PKF_CONTROL << 16) + VK_RETURN,
                     (PKF_SHIFT << 16) + VK_OEM_1,
                     (PKF_SHIFT << 16) + VK_OEM_MINUS,
                     (PKF_SHIFT << 16) + VK_OEM_3,
                     VK_NUMPAD0,
                     VK_NUMPAD1,
                     VK_NUMPAD2,
                     VK_NUMPAD3,
                     VK_NUMPAD4,
                     VK_NUMPAD5,
                     VK_NUMPAD6,
                     VK_NUMPAD7,
                     VK_NUMPAD8,
                     VK_NUMPAD9,
                     '0',
                     '1',
                     '2',
                     '3',
                     '4',
                     '5',
                     '6',
                     '7',
                     '8',
                     '9',
                     'A',
                     'B',
                     'C',
                     'D',
                     'E',
                     'F',
                     'G',
                     'H',
                     'I',
                     'J',
                     'K',
                     'L',
                     'M',
                     'N',
                     'O',
                     'P',
                     'Q',
                     'R',
                     'S',
                     'T',
                     'U',
                     'V',
                     'W',
                     'X',
                     'Y',
                     'Z',
                     ' ',
                     0};
  int keys_size = sizeof(breakKeys) / sizeof(int) - 1;

  wchar_t prefix[FILTER_SIZE + 1];
  wchar_t autofilter[FILTER_SIZE + 1];
  wchar_t filter[FILTER_SIZE + 1];
  int flen = 0;
  *filter = 0;
  int maxLevel = -1;
  bool stopMenu = false;
  int items_num = outliner->itemCount();

  if (items_num == 0) {
    stopMenu = true;
  }

  menu = new FarMenuItem[items_num];
  const auto ei_curr = getEditorInfo();

  while (!stopMenu) {
    int i;
    memset(menu, 0, sizeof(FarMenuItem) * items_num);
    // items in FAR's menu;
    int menu_size = 0;
    int selectedItem = 0;
    std::vector<int> treeStack;

    auto ei = getEditorInfo();
    for (i = 0; i < items_num; i++) {
      OutlineItem* item = outliner->getItem(i);

      if (item->token->indexOfIgnoreCase(UnicodeString(filter)) != -1) {
        int treeLevel = Outliner::manageTree(treeStack, item->level);

        if (maxLevel < treeLevel) {
          maxLevel = treeLevel;
        }

        if (treeLevel > visibleLevel) {
          continue;
        }

        auto menuItem = new wchar_t[255];

        if (!oldOutline) {
          int si = swprintf(menuItem, 255, L"%4ld ", item->lno + 1);

          for (int lIdx = 0; lIdx < treeLevel; lIdx++) {
            menuItem[si++] = ' ';
            menuItem[si++] = ' ';
          }

          auto region = item->region->getName();

          wchar cls = Character::toLowerCase(region[region.indexOf(':') + 1]);

          si += swprintf(menuItem + si, 255 - si, L"%lc ", cls);

          int labelLength = item->token->length();

          if (labelLength + si > 110) {
            labelLength = 110;
          }

          wcsncpy(menuItem + si, item->token->getWChars(), labelLength);
          menuItem[si + labelLength] = 0;
        }
        else {
          UnicodeString* line = getLine(item->lno);
          int labelLength = line->length();

          if (labelLength > 110) {
            labelLength = 110;
          }

          wcsncpy(menuItem, line->getWChars(), labelLength);
          menuItem[labelLength] = 0;
        }

        *(OutlineItem**) (&menuItem[124]) = item;
        // set position on nearest top function
        menu[menu_size].Text = menuItem;

        if ((size_t) ei.CurLine >= item->lno) {
          selectedItem = menu_size;
        }

        menu_size++;
      }
    }

    if (selectedItem > 0) {
      menu[selectedItem].Selected = 1;
    }

    if (menu_size == 0 && flen > 0) {
      flen--;
      filter[flen] = 0;
      continue;
    }

    int aflen = flen;
    // Find same function prefix
    bool same = true;
    int plen = 0;
    wcscpy(autofilter, filter);

    while (code != 0 && menu_size > 1 && same && plen < FILTER_SIZE) {
      plen = aflen + 1;
      int auto_ptr = UnicodeString(menu[0].Text).indexOfIgnoreCase(UnicodeString(autofilter));

      if (int(wcslen(menu[0].Text) - auto_ptr) < plen) {
        break;
      }

      wcsncpy(prefix, menu[0].Text + auto_ptr, plen);
      prefix[plen] = 0;

      for (int j = 1; j < menu_size; j++) {
        if (UnicodeString(menu[j].Text).indexOfIgnoreCase(UnicodeString(prefix)) == -1) {
          same = false;
          break;
        }
      }

      if (same) {
        aflen++;
        wcscpy(autofilter, prefix);
      }
    }

    wchar_t top[128];
    const wchar_t* topline = GetMsg(mOutliner);
    wchar_t captionfilter[FILTER_SIZE + 1];
    wcsncpy(captionfilter, filter, flen);
    captionfilter[flen] = 0;

    if (aflen > flen) {
      wcscat(captionfilter, L"?");
      wcsncat(captionfilter, autofilter + flen, aflen - flen);
      captionfilter[aflen + 1] = 0;
    }

    swprintf(top, 128, topline, captionfilter);
    int sel = 0;
    sel = info->Menu(info->ModuleNumber, -1, -1, 0, FMENU_SHOWAMPERSAND | FMENU_WRAPMODE, top,
                     GetMsg(mChoose), L"add", breakKeys, &code, menu, menu_size);

    // handle mouse selection
    if (sel != -1 && code == -1) {
      code = 1;
    }

    switch (code) {
      case -1:
        stopMenu = true;
        break;
      case 0:  // VK_BACK

        if (flen > 0) {
          flen--;
        }

        filter[flen] = 0;
        break;
      case 1:  // VK_RETURN
      {
        if (menu_size == 0) {
          break;
        }

        esp.CurTabPos = esp.LeftPos = esp.Overtype = esp.TopScreenLine = -1;
        OutlineItem* item = *(OutlineItem**) (&menu[sel].Text[124]);
        esp.CurLine = item->lno;
        esp.CurPos = item->pos;
        esp.TopScreenLine = esp.CurLine - ei.WindowSizeY / 2;

        if (esp.TopScreenLine < 0) {
          esp.TopScreenLine = 0;
        }

        info->EditorControl(ECTL_SETPOSITION, &esp);
        stopMenu = true;
        moved = true;
        break;
      }
      case 2:  // ;

        if (flen == FILTER_SIZE) {
          break;
        }

        filter[flen] = ';';
        filter[++flen] = 0;
        break;
      case 3:  // -

        if (flen == FILTER_SIZE) {
          break;
        }

        filter[flen] = '-';
        filter[++flen] = 0;
        break;
      case 4:  // VK_TAB
        wcscpy(filter, autofilter);
        flen = aflen;
        break;
      case 5:  // ctrl-up
      {
        if (menu_size == 0) {
          break;
        }

        if (sel == 0) {
          sel = menu_size - 1;
        }
        else {
          sel--;
        }

        esp.CurTabPos = esp.LeftPos = esp.Overtype = esp.TopScreenLine = -1;
        OutlineItem* item = *(OutlineItem**) (&menu[sel].Text[124]);
        esp.CurLine = item->lno;
        esp.CurPos = item->pos;
        esp.TopScreenLine = esp.CurLine - ei.WindowSizeY / 2;

        if (esp.TopScreenLine < 0) {
          esp.TopScreenLine = 0;
        }

        info->EditorControl(ECTL_SETPOSITION, &esp);
        info->EditorControl(ECTL_REDRAW, nullptr);
        info->EditorControl(ECTL_GETINFO, &ei);
        break;
      }
      case 6:  // ctrl-down
      {
        if (menu_size == 0) {
          break;
        }

        if (sel == menu_size - 1) {
          sel = 0;
        }
        else {
          sel++;
        }

        esp.CurTabPos = esp.LeftPos = esp.Overtype = esp.TopScreenLine = -1;
        OutlineItem* item = *(OutlineItem**) (&menu[sel].Text[124]);
        esp.CurLine = item->lno;
        esp.CurPos = item->pos;
        esp.TopScreenLine = esp.CurLine - ei.WindowSizeY / 2;

        if (esp.TopScreenLine < 0) {
          esp.TopScreenLine = 0;
        }

        info->EditorControl(ECTL_SETPOSITION, &esp);
        info->EditorControl(ECTL_REDRAW, nullptr);
        info->EditorControl(ECTL_GETINFO, &ei);
        break;
      }
      case 7:  // ctrl-left
      {
        if (visibleLevel > maxLevel) {
          visibleLevel = maxLevel - 1;
        }
        else {
          if (visibleLevel > 0) {
            visibleLevel--;
          }
        }

        if (visibleLevel < 0) {
          visibleLevel = 0;
        }

        break;
      }
      case 8:  // ctrl-right
      {
        visibleLevel++;
        break;
      }
      case 9:  // ctrl-return
      {
        // read current position
        info->EditorControl(ECTL_GETINFO, &ei);
        // insert text
        OutlineItem* item = *(OutlineItem**) (&menu[sel].Text[124]);
        info->EditorControl(ECTL_INSERTTEXT, (void*) item->token->getWChars());

        // move the cursor to the end of the inserted string
        esp.CurTabPos = esp.LeftPos = esp.Overtype = esp.TopScreenLine = -1;
        esp.CurLine = -1;
        esp.CurPos = ei.CurPos + item->token->length();
        info->EditorControl(ECTL_SETPOSITION, &esp);

        stopMenu = true;
        moved = true;
        break;
      }
      case 10:  // :

        if (flen == FILTER_SIZE) {
          break;
        }

        filter[flen] = ':';
        filter[++flen] = 0;
        break;
      case 11:  // _

        if (flen == FILTER_SIZE) {
          break;
        }

        filter[flen] = '_';
        filter[++flen] = 0;
        break;
      case 12:  // _

        if (flen == FILTER_SIZE) {
          break;
        }

        filter[flen] = '~';
        filter[++flen] = 0;
        break;
      default:

        if (flen == FILTER_SIZE || code > keys_size) {
          break;
        }
        if (code < 23) {
          filter[flen] = Character::toLowerCase('0' + code - 13);
        }
        else {
          filter[flen] = Character::toLowerCase(breakKeys[code]);
        }
        filter[++flen] = 0;
        break;
    }
  }

  for (int i = 0; i < items_num; i++) {
    delete[] menu[i].Text;
  }

  delete[] menu;

  if (!moved) {
    // restoring position
    esp.CurLine = ei_curr.CurLine;
    esp.CurPos = ei_curr.CurPos;
    esp.CurTabPos = ei_curr.CurTabPos;
    esp.TopScreenLine = ei_curr.TopScreenLine;
    esp.LeftPos = ei_curr.LeftPos;
    esp.Overtype = ei_curr.Overtype;
    info->EditorControl(ECTL_SETPOSITION, &esp);
  }

  if (items_num == 0) {
    const wchar_t* msg[2] = {GetMsg(mNothingFound), GetMsg(mGotcha)};
    info->Message(info->ModuleNumber, 0, nullptr, msg, 2, 1);
  }
}

EditorInfo FarEditor::getEditorInfo() const
{
  EditorInfo ei {};
  info->EditorControl(ECTL_GETINFO, &ei);
  return ei;
}

color FarEditor::convert(const StyledRegion* rd) const
{
  color col {};

  if (rdBackground == nullptr)
    return col;

  int fore = (newfore != -1) ? newfore : rdBackground->fore;
  int back = (newback != -1) ? newback : rdBackground->back;

  if (TrueMod) {
    if (rd != nullptr) {
      col.fg = rd->fore;
      col.bk = rd->back;
    }

    if (rd == nullptr || !rd->isForeSet)
      col.fg = fore;

    if (rd == nullptr || !rd->isBackSet)
      col.bk = back;

    if (rd != nullptr)
      col.style = rd->style;

    return col;
  }
  else {
    if (rd != nullptr) {
      col.cfg = rd->fore;
      col.cbk = rd->back;
    }

    if (rd == nullptr || !rd->isForeSet)
      col.cfg = fore;

    if (rd == nullptr || !rd->isBackSet)
      col.cbk = back;

    return col;
  }
}

bool FarEditor::foreDefault(const color& col) const
{
  if (TrueMod)
    return col.fg == rdBackground->fore;
  else
    return col.cfg == rdBackground->fore;
}

bool FarEditor::backDefault(const color& col) const
{
  if (TrueMod)
    return col.bk == rdBackground->back;
  else
    return col.cbk == rdBackground->back;
}

void FarEditor::addFARColor(int lno, int s, int e, const color& col, bool add_style) const
{
  if (TrueMod) {
    EditorTrueColor ec {};
    ec.Base.StringNumber = lno;
    ec.Base.StartPos = s;
    ec.Base.EndPos = e - 1;
    if (col.fg || col.bk) {
      ec.TrueColor.Fore.R = ((col.fg >> 16) & 0xFF);
      ec.TrueColor.Fore.G = ((col.fg >> 8) & 0xFF);
      ec.TrueColor.Fore.B = ((col.fg) & 0xFF);
      ec.TrueColor.Fore.Flags = 1;
      ec.TrueColor.Back.R = ((col.bk >> 16) & 0xFF);
      ec.TrueColor.Back.G = ((col.bk >> 8) & 0xFF);
      ec.TrueColor.Back.B = ((col.bk) & 0xFF);
      ec.TrueColor.Back.Flags = 1;

      if (ec.TrueColor.Fore.R > 0x10)
        ec.Base.Color |= FOREGROUND_RED;
      if (ec.TrueColor.Fore.G > 0x10)
        ec.Base.Color |= FOREGROUND_GREEN;
      if (ec.TrueColor.Fore.B > 0x10)
        ec.Base.Color |= FOREGROUND_BLUE;

      if (ec.TrueColor.Back.R > 0x10)
        ec.Base.Color |= BACKGROUND_RED;
      if (ec.TrueColor.Back.G > 0x10)
        ec.Base.Color |= BACKGROUND_GREEN;
      if (ec.TrueColor.Back.B > 0x10)
        ec.Base.Color |= BACKGROUND_BLUE;

      if (ec.TrueColor.Fore.R > 0x80 || ec.TrueColor.Fore.G > 0x80 || ec.TrueColor.Fore.B > 0x80) {
        ec.Base.Color = FOREGROUND_INTENSITY;
      }
      if (ec.Base.Color == 0 || ec.TrueColor.Back.R > 0x80 || ec.TrueColor.Back.G > 0x80 ||
          ec.TrueColor.Back.B > 0x80)
      {
        ec.Base.Color = BACKGROUND_INTENSITY;
      }
      if (add_style) {
        if (col.style & StyledRegion::RD_UNDERLINE) {
          ec.Base.Color |= COMMON_LVB_UNDERSCORE;
        }
        if (col.style & StyledRegion::RD_STRIKEOUT) {
          ec.Base.Color |= COMMON_LVB_STRIKEOUT;
        }
      }
    }

    info->EditorControl(ECTL_ADDTRUECOLOR, &ec);
  }
  else {
    EditorColor ec {};
    ec.StringNumber = lno;
    ec.StartPos = s;
    ec.EndPos = e - 1;
    ec.Color = col.concolor;
    info->EditorControl(ECTL_ADDCOLOR, &ec);
  }
}

const wchar_t* FarEditor::GetMsg(int msg) const
{
  return (info->GetMsg(info->ModuleNumber, msg));
}

void FarEditor::cleanEditor()
{
  color col;
  const auto ei = getEditorInfo();
  for (int i = 0; i < ei.TotalLines; i++) {
    addFARColor(i, -1, 0, col);
  }
}

/* ***** BEGIN LICENSE BLOCK *****
 * Copyright (C) 1999-2009 Cail Lomecb <irusskih at gmail dot com>.
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; If not, see <http://www.gnu.org/licenses/>.
 * ***** END LICENSE BLOCK ***** */
