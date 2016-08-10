#ifndef _FAREDITOR_H_
#define _FAREDITOR_H_

#include<colorer/editor/BaseEditor.h>
#include<colorer/handlers/StyledRegion.h>
#include<colorer/editor/Outliner.h>
#include<common/Logging.h>

#include"pcolorer.h"

struct color{
  union{
    struct{
      unsigned int cfg : 4;
      unsigned int cbk : 4;
    };
    int concolor : 32;
    struct{
      unsigned int fg :24;
      unsigned int bk :24;
      int style;
    };
  };
  //color(): concolor(0), fg(0), bk(0), style(0) {};
  color() {concolor=0; fg = 0; bk = 0; style = 0; };
};

const DString DShowCross=DString("show-cross");
const DString DNone=DString("none");
const DString DVertical=DString("vertical");
const DString DHorizontal=DString("horizontal");
const DString DBoth=DString("both");
const DString DCrossZorder=DString("cross-zorder");
const DString DBottom=DString("bottom");
const DString DTop=DString("top");
const DString DYes=DString("yes");
const DString DNo=DString("no");
const DString DTrue=DString("true");
const DString DFalse=DString("false");
const DString DBackparse=DString("backparse");
const DString DMaxLen=DString("maxlinelength");
const DString DDefFore=DString("default-fore");
const DString DDefBack=DString("default-back");
const DString DFullback=DString("fullback");
const DString DHotkey=DString("hotkey");
const DString DFavorite=DString("favorite");

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
  FarEditor(PluginStartupInfo *info, ParserFactory *pf);
  /** Drops this editor */
  ~FarEditor();

  void endJob(int lno);
  /**
  Returns line number "lno" from FAR interface. Line is only valid until next call of this function,
  it also should not be disposed, this function takes care of this.
  */
  String *getLine(int lno);

  /** Changes current assigned file type.
  */
  void setFileType(FileType *ftype);
  /** Returns currently selected file type.
  */
  FileType *getFileType();

  /** Selects file type with it's extension and first lines
  */
  void chooseFileType(String *fname);


  /** Installs specified RegionMapper implementation.
  This class serves to request mapping of regions into
  real colors.
  */
  void setRegionMapper(RegionMapper *rs);

  /**
  * Change editor properties. These overwrites default HRC settings
  */
  void setDrawCross(int _drawCross);
  void setDrawPairs(bool drawPairs);
  void setDrawSyntax(bool drawSyntax);
  void setOutlineStyle(bool oldStyle);
  void setTrueMod(bool _TrueMod);

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
  /** Editor action: Lists fuctional region.
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
  int editorEvent(int event, void *param);
  /** Dispatch editor input event */
  int editorInput(const INPUT_RECORD *ir);

  void cleanEditor();

private:
  EditorInfo ei;
  PluginStartupInfo *info;

  ParserFactory *parserFactory;
  BaseEditor *baseEditor;

  int  maxLineLength;
  bool fullBackground;

  int drawCross;//0 - off,  1 - always, 2 - if included in the scheme
  bool showVerticalCross, showHorizontalCross;
  int crossZOrder;
  color horzCrossColor, vertCrossColor;

  bool drawPairs, drawSyntax;
  bool oldOutline;
  bool TrueMod;

  int WindowSizeX;
  int WindowSizeY;
  bool inRedraw;
  int idleCount;

  int prevLinePosition, blockTopPosition;

  String *ret_str;
  int ret_strNumber;

  int newfore, newback;
  const StyledRegion *rdBackground;
  LineRegion *cursorRegion;

  int visibleLevel;
  Outliner *structOutliner;
  Outliner *errorOutliner;

  void reloadTypeSettings();
  void enterHandler();
  color convert(const StyledRegion *rd);
  bool foreDefault(color col);
  bool backDefault(color col);
  void showOutliner(Outliner *outliner);
  void addFARColor(int lno, int s, int e, color col);
  void addAnnotation(int lno, int s, int e, AnnotationInfo &ai);
  const wchar_t *GetMsg(int msg);
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