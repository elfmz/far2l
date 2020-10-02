#ifndef _COLORER_TEXTVIEWER_H_
#define _COLORER_TEXTVIEWER_H_

#ifdef _WIN32
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
  int encoding;
  int background;
public:
  TextConsoleViewer(BaseEditor *be, TextLinesStore *ts, int background, int encoding);
  ~TextConsoleViewer();

  void view();
};

#endif


