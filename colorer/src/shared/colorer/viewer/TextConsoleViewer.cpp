
#include<colorer/viewer/TextConsoleViewer.h>
#include<stdio.h>

TextConsoleViewer::TextConsoleViewer(BaseEditor *be, TextLinesStore *ts, int background, int encoding){
  textLinesStore = ts;
  baseEditor = be;
  if(encoding == -1) encoding = Encodings::getDefaultEncodingIndex();
  this->encoding = encoding;
  this->background = background;
};
TextConsoleViewer::~TextConsoleViewer(){};

void TextConsoleViewer::view()
{
#ifdef _WIN32
int topline, leftpos;
leftpos = topline = 0;
INPUT_RECORD ir;

  HANDLE hConI = CreateFile(TEXT("CONIN$"), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
  if (hConI == INVALID_HANDLE_VALUE) return;
  SetConsoleMode(hConI, ENABLE_MOUSE_INPUT|ENABLE_WINDOW_INPUT);

  CONSOLE_SCREEN_BUFFER_INFO csbi;

  HANDLE hCon = CreateConsoleScreenBuffer(GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, CONSOLE_TEXTMODE_BUFFER, 0);
  SetConsoleActiveScreenBuffer(hCon);
  GetConsoleScreenBufferInfo(hCon, &csbi);
  CONSOLE_CURSOR_INFO cci;
  cci.dwSize = 100;
  cci.bVisible = FALSE;
  SetConsoleCursorInfo(hCon, &cci);

  CHAR_INFO *buffer = new CHAR_INFO[csbi.dwSize.X * csbi.dwSize.Y];
  bool unc_fault = false;
  do{
    int lline = csbi.dwSize.Y;
    if (topline+lline > textLinesStore->getLineCount()) lline = textLinesStore->getLineCount()-topline;
    baseEditor->visibleTextEvent(topline, lline);

    for(int i = topline; i < topline + csbi.dwSize.Y; i++){
      int Y = i-topline;

      int li;
      for(li = 0; li < csbi.dwSize.X; li++){
        buffer[Y*csbi.dwSize.X + li].Char.UnicodeChar = ' ';
        buffer[Y*csbi.dwSize.X + li].Attributes = background;
      };

      if (i >= textLinesStore->getLineCount()) continue;
      DString iLine = textLinesStore->getLine(i);

      for(li = 0; li < csbi.dwSize.X; li++){
        if (leftpos+li >= iLine.length()) break;
        buffer[Y*csbi.dwSize.X + li].Char.UnicodeChar = iLine[leftpos+li];
        if (unc_fault)
          buffer[Y*csbi.dwSize.X + li].Char.AsciiChar = Encodings::toChar(encoding, iLine[leftpos+li]);
      };
      for(LineRegion *l1 = baseEditor->getLineRegions(i); l1 != null; l1 = l1->next){
        if (l1->special || l1->rdef == null) continue;
        int end = l1->end;
        if (end == -1) end = iLine.length();
        int X = l1->start - leftpos;
        int len = end - l1->start;
        if (X < 0){
          len += X;
          X = 0;
        };
        if (len < 0 || X >= csbi.dwSize.X) continue;
        if (len+X > csbi.dwSize.X) len = csbi.dwSize.X-X;
        WORD color = (WORD)(l1->styled()->fore + (l1->styled()->back<<4));
        if (!l1->styled()->bfore) color = (color&0xF0) + (background&0xF);
        if (!l1->styled()->bback) color = (color&0xF) + (background&0xF0);
        for(int li = 0; li < len; li++)
          buffer[Y*csbi.dwSize.X + X + li].Attributes = color;
      };
    };
    COORD coor;
    coor.X = coor.Y = 0;
    SMALL_RECT sr;
    sr.Left = 0;
    sr.Right = csbi.dwSize.X-1;
    sr.Top = 0;
    sr.Bottom = csbi.dwSize.Y-1;
    if (!unc_fault && !WriteConsoleOutputW(hCon, buffer, csbi.dwSize, coor, &sr)){
      unc_fault = true;
      continue;
    };
    if (unc_fault) WriteConsoleOutputA(hCon, buffer, csbi.dwSize, coor, &sr);

    // managing the keyboard
    do{
      DWORD tmp;
      ReadConsoleInput(hConI, &ir, 1, &tmp);
      if (ir.EventType == WINDOW_BUFFER_SIZE_EVENT){
        GetConsoleScreenBufferInfo(hCon, &csbi);
        delete[] buffer;
        buffer = new CHAR_INFO[csbi.dwSize.X * csbi.dwSize.Y];
        break;
      };
      if (ir.EventType == MOUSE_EVENT && ir.Event.MouseEvent.dwEventFlags == 0x4){
        switch(ir.Event.MouseEvent.dwButtonState){
          case 0x780000:
            topline-=csbi.dwSize.Y;
            if (topline < 0) topline = 0;
            break;
          case 0xFF880000:
            topline += csbi.dwSize.Y;
            if (topline > textLinesStore->getLineCount() - csbi.dwSize.Y) topline = textLinesStore->getLineCount() - csbi.dwSize.Y;
            if (topline < 0) topline = 0;
            break;
        };
        break;
      };
      if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown){
        // moving view position
        switch(ir.Event.KeyEvent.wVirtualKeyCode){
          case VK_UP:
            if (topline) topline--;
            break;
          case VK_DOWN:
            if (topline+csbi.dwSize.Y < textLinesStore->getLineCount()) topline++;
            break;
          case VK_LEFT:
            leftpos--;
            if (ir.Event.KeyEvent.dwControlKeyState & (RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED))
              leftpos -= 15;
            if (leftpos < 0) leftpos = 0;
            break;
          case VK_RIGHT:
            leftpos++;
            if (ir.Event.KeyEvent.dwControlKeyState & (RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED))
              leftpos += 15;
            break;
          case VK_PRIOR:
            topline-=csbi.dwSize.Y;
            if (topline < 0) topline = 0;
            break;
          case VK_NEXT:
          case VK_SPACE:
            topline += csbi.dwSize.Y;
            if (topline > textLinesStore->getLineCount() - csbi.dwSize.Y) topline = textLinesStore->getLineCount() - csbi.dwSize.Y;
            if (topline < 0) topline = 0;
            break;
          case VK_HOME:
            leftpos = topline = 0;
            break;
          case VK_END:
            topline = textLinesStore->getLineCount()-csbi.dwSize.Y;
            if (topline < 0) topline = 0;
            leftpos = 0;
            break;
        };
        break;
      };
    }while(true);
  }while(ir.Event.KeyEvent.wVirtualKeyCode != VK_ESCAPE);

  delete[] buffer;
  SetConsoleActiveScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE));
  CloseHandle(hCon);
  CloseHandle(hConI);

#else

  printf("unix edition doesn't support interactive text viewing\n\n");

  for(int i = 0; i < textLinesStore->getLineCount(); i++){
    DString line = textLinesStore->getLine(i);
    printf("%s\n", line.getChars());
  };

#endif
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
