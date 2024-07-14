#ifndef _FAREDITOR_H_
#define _FAREDITOR_H_

#include <colorer/editor/BaseEditor.h>
#include <colorer/editor/Outliner.h>
#include <colorer/handlers/StyledRegion.h>
#include "pcolorer.h"

struct color
{
  union {
    struct
    {
      unsigned int cfg : 4;
      unsigned int cbk : 4;
    };
    int concolor : 32;
    struct
    {
      unsigned int fg : 24;
      unsigned int bk : 24;
      int style;
    };
  };
  // color(): concolor(0), fg(0), bk(0), style(0) {}
  color()
  {
    concolor = 0;
    fg = 0;
    bk = 0;
    style = 0;
  }
};

extern const UnicodeString DShowCross;
extern const UnicodeString DNone;
extern const UnicodeString DVertical;
extern const UnicodeString DHorizontal;
extern const UnicodeString DBoth;
extern const UnicodeString DCrossZorder;
extern const UnicodeString DBottom;
extern const UnicodeString DTop;
extern const UnicodeString DYes;
extern const UnicodeString DNo;
extern const UnicodeString DTrue;
extern const UnicodeString DFalse;
extern const UnicodeString DBackparse;
extern const UnicodeString DMaxLen;
extern const UnicodeString DDefFore;
extern const UnicodeString DDefBack;
extern const UnicodeString DFullback;
extern const UnicodeString DHotkey;
extern const UnicodeString DFavorite;
extern const UnicodeString DFirstLines;
extern const UnicodeString DFirstLineBytes;

/** FAR Editor internal plugin structures.
    Implements text parsing and different
    editor extended functions.
    @ingroup far_plugin
*/
class FarEditor : public LineSource
{
 public:
  /** Creates FAR editor instance.
   */
  FarEditor(PluginStartupInfo* inf, ParserFactory* pf);
  /** Drops this editor */
  ~FarEditor() override;

  void endJob(size_t lno) override;
  /**
  Returns line number "lno" from FAR interface. Line is only valid until next call of this function,
  it also should not be disposed, this function takes care of this.
  */
  UnicodeString* getLine(size_t lno) override;

  /** Changes current assigned file type.
   */
  void setFileType(FileType* ftype);
  /** Returns currently selected file type.
   */
  [[nodiscard]] FileType* getFileType() const;

  /** Selects file type with it's extension and first lines
   */
  void chooseFileType(const UnicodeString* fname);

  /** Installs specified RegionMapper implementation.
  This class serves to request mapping of regions into
  real colors.
  */
  void setRegionMapper(RegionMapper* rs);

  /**
   * Change editor properties. These overwrites default HRC settings
   */
  void setDrawCross(int _drawCross);
  void setDrawPairs(bool drawPairs);
  void setDrawSyntax(bool drawSyntax);
  void setOutlineStyle(bool oldStyle);
  void setTrueMod(bool TrueMod_);

  /** Editor action: pair matching.
   */
  void matchPair();
  /** Editor action: pair selection.
   */
  void selectPair();
  /** Editor action: pair selection with current block.
   */
  void selectBlock();
  /** Editor action: Selection of current region under cursor.
   */
  void selectRegion();
  /** Editor action: Lists functional region.
   */
  void listFunctions();
  /** Editor action: Lists syntax errors in text.
   */
  void listErrors();
  /**
   * Locates a function under cursor and tries to jump to it using outliner information
   */
  void locateFunction();

  /** Invalidates current syntax highlighting
   */
  void updateHighlighting();

  /** Handle passed FAR editor event */
  int editorEvent(int event, void* param);
  /** Dispatch editor input event */
  int editorInput(const INPUT_RECORD* ir);

  void cleanEditor();

 private:
  EditorInfo ei;
  PluginStartupInfo* info;

  ParserFactory* parserFactory;
  std::unique_ptr<BaseEditor> baseEditor;

  int maxLineLength = 0;
  bool fullBackground = true;

  int drawCross = 2;  // 0 - off,  1 - always, 2 - if included in the scheme
  bool showVerticalCross = false;
  bool showHorizontalCross = false;
  int crossZOrder = 0;
  color horzCrossColor {};
  color vertCrossColor {};

  bool drawPairs = true;
  ;
  bool drawSyntax = true;
  ;
  bool oldOutline = false;
  bool TrueMod = true;
  ;

  int WindowSizeX = 0;
  int WindowSizeY = 0;
  bool inRedraw = false;
  int idleCount = 0;

  int prevLinePosition = 0;
  int blockTopPosition = -1;

  std::unique_ptr<UnicodeString> ret_str;
  size_t ret_strNumber = -1;

  int newfore = -1;
  int newback = -1;
  const StyledRegion* rdBackground = nullptr;
  std::unique_ptr<LineRegion> cursorRegion;

  int visibleLevel = 100;
  std::unique_ptr<Outliner> structOutliner;
  std::unique_ptr<Outliner> errorOutliner;

  void reloadTypeSettings();
  void enterHandler();
  color convert(const StyledRegion* rd);
  bool foreDefault(color col);
  bool backDefault(color col);
  void showOutliner(Outliner* outliner);
  void addFARColor(int lno, int s, int e, color col, bool add_style = true);
  const wchar_t* GetMsg(int msg);
};
#endif

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
