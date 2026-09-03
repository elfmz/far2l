#ifndef _FARVIEWER_H_
#define _FARVIEWER_H_

#include <colorer/editor/BaseEditor.h>
#include <colorer/handlers/StyledRegion.h>
#include <memory>
#include "pcolorer.h"
#include <string>
#include <vector>

class FarViewer : public LineSource
{
 public:
  FarViewer(PluginStartupInfo* inf, ParserFactory* pf, RegionMapper* mapper, bool trueColor);

  UnicodeString* getLine(size_t lno) override;
  void endJob(size_t lno) override;
  void colorize(const ViewerInfo& vi);

 private:
  struct RegionColor
  {
    uint64_t attributes {};
    unsigned int fore {};
    unsigned int back {};
    unsigned int style {};
  };

  RegionColor convert(const StyledRegion* region) const;
  void addColor(size_t line, size_t start, size_t end, const RegionColor& color) const;
  void readLines(int count);

  struct VisualLine
  {
    int logicalLine;
    int offset;
    int length;
  };

  PluginStartupInfo* info;
  ParserFactory* parserFactory;
  std::unique_ptr<BaseEditor> baseEditor;
  std::unique_ptr<UnicodeString> line;
  std::vector<std::wstring> lines;
  std::vector<VisualLine> visualLines;
  const StyledRegion* background = nullptr;
  int defaultFore = -1;
  int defaultBack = -1;
  int firstVisibleLine = 0;
  int64_t filePos = -1;
  bool contextRetained = false;
  bool trueColor;
  bool initialized = false;
};

#endif
