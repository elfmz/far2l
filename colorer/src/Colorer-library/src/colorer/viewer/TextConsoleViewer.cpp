#include <colorer/viewer/TextConsoleViewer.h>
#include <colorer/unicode/Encodings.h>
#include <stdio.h>

TextConsoleViewer::TextConsoleViewer(BaseEditor *be, TextLinesStore *ts, int background, int encoding){
  textLinesStore = ts;
  baseEditor = be;
  if(encoding == -1) encoding = Encodings::getDefaultEncodingIndex();
  this->encoding = encoding;
  this->background = background;
}
TextConsoleViewer::~TextConsoleViewer(){}

void TextConsoleViewer::view()
{
#ifdef _WIN32
int topline, leftpos;
leftpos = topline = 0;
INPUT_RECORD ir;

  HANDLE hConI = CreateFileW(L"CONIN$", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
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
      }

      if (i >= textLinesStore->getLineCount()) continue;
      CString iLine = textLinesStore->getLine(i);

      for(li = 0; li < csbi.dwSize.X; li++){
        if (leftpos+li >= iLine.length()) break;
        buffer[Y*csbi.dwSize.X + li].Char.UnicodeChar = iLine[leftpos+li];
        if (unc_fault)
          buffer[Y*csbi.dwSize.X + li].Char.AsciiChar = Encodings::toChar(encoding, iLine[leftpos+li]);
      }
      for(LineRegion *l1 = baseEditor->getLineRegions(i); l1 != nullptr; l1 = l1->next){
        if (l1->special || l1->rdef == nullptr) continue;
        int end = l1->end;
        if (end == -1) end = iLine.length();
        int X = l1->start - leftpos;
        int len = end - l1->start;
        if (X < 0){
          len += X;
          X = 0;
        }
        if (len < 0 || X >= csbi.dwSize.X) continue;
        if (len+X > csbi.dwSize.X) len = csbi.dwSize.X-X;
        WORD color = (WORD)(l1->styled()->fore + (l1->styled()->back<<4));
        if (!l1->styled()->bfore) color = (color&0xF0) + (background&0xF);
        if (!l1->styled()->bback) color = (color&0xF) + (background&0xF0);
        for(int li = 0; li < len; li++)
          buffer[Y*csbi.dwSize.X + X + li].Attributes = color;
      }
    }
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
    }
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
      }
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
        }
        break;
      }
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
        }
        break;
      }
    }while(true);
  }while(ir.Event.KeyEvent.wVirtualKeyCode != VK_ESCAPE);

  delete[] buffer;
  SetConsoleActiveScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE));
  CloseHandle(hCon);
  CloseHandle(hConI);

#else

  printf("unix edition doesn't support interactive text viewing\n\n");

  for(size_t i = 0; i < textLinesStore->getLineCount(); i++){
    CString line = textLinesStore->getLine(i);
    printf("%s\n", line.getChars());
  }

#endif
}


