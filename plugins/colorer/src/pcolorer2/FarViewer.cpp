#include "FarViewer.h"
#include "FarEditor.h"
#include <limits>

FarViewer::FarViewer(PluginStartupInfo* inf, ParserFactory* pf, RegionMapper* mapper, bool trueColor_)
    : info(inf), parserFactory(pf), baseEditor(std::make_unique<BaseEditor>(pf, this)),
      trueColor(trueColor_)
{
  baseEditor->setRegionMapper(mapper);
  background = StyledRegion::cast(baseEditor->rd_def_Text);
}

UnicodeString* FarViewer::getLine(size_t lno)
{
  if (lno >= lines.size()) {
    return nullptr;
  }

  line = std::make_unique<UnicodeString>(lines[lno].c_str());
  return line.get();
}

void FarViewer::endJob(size_t lno)
{
  (void) lno;
  line.reset();
}

void FarViewer::colorize(const ViewerInfo& vi)
{
  const bool positionChanged = filePos != vi.FilePos;
  filePos = vi.FilePos;
  auto previousLines = std::move(lines);
  readLines(vi.WindowSizeY);

  size_t firstChanged = 0;
  const size_t commonSize = std::min(previousLines.size(), lines.size());
  while (firstChanged < commonSize && previousLines[firstChanged] == lines[firstChanged]) {
    ++firstChanged;
  }

  baseEditor->lineCountEvent(static_cast<int>(lines.size()));
  baseEditor->visibleTextEvent(firstVisibleLine,
                               static_cast<int>(lines.size()) - firstVisibleLine);

  if (!initialized) {
    UnicodeString fileName(vi.FileName);
    FileType* fileType = baseEditor->chooseFileType(&fileName);
    if (!fileType) {
      return;
    }
    baseEditor->setFileType(fileType);

    FileType* defaultType = parserFactory->getHrcLibrary().getFileType("default");
    if (defaultType) {
      defaultFore = defaultType->getParamValueHex(DDefFore, -1);
      defaultBack = defaultType->getParamValueHex(DDefBack, -1);
    }
    defaultFore = fileType->getParamValueHex(DDefFore, defaultFore);
    defaultBack = fileType->getParamValueHex(DDefBack, defaultBack);
    initialized = true;
  } else if (positionChanged && !contextRetained) {
    baseEditor->modifyEvent(0);
  } else if (firstChanged < previousLines.size() || firstChanged < lines.size()) {
    baseEditor->modifyEvent(static_cast<int>(firstChanged));
  }

  for (int visualLine = 0; visualLine < static_cast<int>(visualLines.size()); ++visualLine) {
    const auto& string = visualLines[visualLine];
    const auto backgroundEnd = std::min<int64_t>(
        std::numeric_limits<int>::max(), string.length + vi.LeftPos + vi.WindowSizeX);
    addColor(visualLine, 0, static_cast<int>(backgroundEnd), convert(nullptr));
  }

  for (int lno = firstVisibleLine; lno < static_cast<int>(lines.size()); ++lno) {
    for (LineRegion* region = baseEditor->getLineRegions(lno); region; region = region->next) {
      if (region->special || region->start == region->end) {
        continue;
      }

      const int end = region->end == -1 ? static_cast<int>(lines[lno].size()) : region->end;
      const auto color = convert(region->styled());
      for (int visualLine = 0; visualLine < static_cast<int>(visualLines.size()); ++visualLine) {
        const auto& string = visualLines[visualLine];
        if (string.logicalLine != lno) {
          continue;
        }

        const int start = std::max(region->start, string.offset);
        const int fragmentEnd = std::min(end, string.offset + string.length);
        if (start < fragmentEnd) {
          addColor(visualLine, start - string.offset, fragmentEnd - string.offset, color);
        }
      }
    }
  }
}

void FarViewer::readLines(int count)
{
  lines.clear();
  visualLines.clear();
  contextRetained = false;
  bool continuation = false;

  for (size_t lno = 0;; ++lno) {
    ViewerGetString string {lno};
    if (!info->ViewerControl(VCTL_GETCONTEXT, &string)) {
      break;
    }
    if (!continuation) {
      lines.emplace_back();
    }
    contextRetained = (string.Flags & VGS_CONTEXT_RETAINED) != 0;
    lines.back().append(string.StringText, string.StringLength);
    continuation = (string.Flags & VGS_WRAPS_TO_NEXT) != 0;
  }
  firstVisibleLine = static_cast<int>(lines.size()) - (continuation ? 1 : 0);

  for (size_t lno = 0; lno < static_cast<size_t>(count); ++lno) {
    ViewerGetString string {lno};
    if (!info->ViewerControl(VCTL_GETSTRING, &string)) {
      break;
    }

    if (!continuation) {
      lines.emplace_back();
    }

    auto& logicalLine = lines.back();
    visualLines.push_back({static_cast<int>(lines.size()) - 1,
                           static_cast<int>(logicalLine.size()),
                           static_cast<int>(string.StringLength)});
    logicalLine.append(string.StringText, string.StringLength);
    continuation = (string.Flags & VGS_WRAPS_TO_NEXT) != 0;
  }
}

FarViewer::RegionColor FarViewer::convert(const StyledRegion* region) const
{
  RegionColor color;
  if (!background) {
    return color;
  }

  color.fore = defaultFore == -1 ? background->fore : defaultFore;
  color.back = defaultBack == -1 ? background->back : defaultBack;
  if (region) {
    if (region->isForeSet) {
      color.fore = region->fore;
    }
    if (region->isBackSet) {
      color.back = region->back;
    }
    color.style = region->style;
  }

  if (!trueColor) {
    color.attributes = color.fore | (color.back << 4);
  }
  return color;
}

void FarViewer::addColor(size_t lineNumber, size_t start, size_t end, const RegionColor& color) const
{
  if (!trueColor) {
    ViewerColor viewerColor {lineNumber, start, end - 1, color.attributes};
    info->ViewerControl(VCTL_ADDCOLOR, &viewerColor);
    return;
  }

  ViewerTrueColor viewerColor {};
  viewerColor.Base.StringNumber = lineNumber;
  viewerColor.Base.StartPos = start;
  viewerColor.Base.EndPos = end - 1;
  viewerColor.TrueColor.Fore = {
      static_cast<unsigned char>((color.fore >> 16) & 0xff),
      static_cast<unsigned char>((color.fore >> 8) & 0xff),
      static_cast<unsigned char>(color.fore & 0xff), 1};
  viewerColor.TrueColor.Back = {
      static_cast<unsigned char>((color.back >> 16) & 0xff),
      static_cast<unsigned char>((color.back >> 8) & 0xff),
      static_cast<unsigned char>(color.back & 0xff), 1};

  if (color.style & StyledRegion::RD_UNDERLINE) {
    viewerColor.Base.Color |= COMMON_LVB_UNDERSCORE;
  }
  if (color.style & StyledRegion::RD_STRIKEOUT) {
    viewerColor.Base.Color |= COMMON_LVB_STRIKEOUT;
  }
  info->ViewerControl(VCTL_ADDTRUECOLOR, &viewerColor);
}
