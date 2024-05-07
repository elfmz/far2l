#ifndef _COLORER_TEXTVIEWER_H_
#define _COLORER_TEXTVIEWER_H_

#ifdef WIN32
#include<windows.h>
#endif

#include<colorer/editor/BaseEditor.h>
#include<colorer/viewer/TextLinesStore.h>

/**
    Console viewing of parsed and colored file.
    @ingroup colorer_viewer
*/
class TextConsoleViewer{

private:
  TextLinesStore *textLinesStore;
  BaseEditor *baseEditor;
  unsigned short background;
public:
 TextConsoleViewer(BaseEditor* be, TextLinesStore* ts, unsigned short background);
  ~TextConsoleViewer();

  void view();
};

#endif


