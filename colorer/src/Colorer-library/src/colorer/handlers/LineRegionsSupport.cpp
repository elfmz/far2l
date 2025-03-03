#include "colorer/handlers/LineRegionsSupport.h"

LineRegionsSupport::LineRegionsSupport()
{
  lineCount = 0;
  firstLineNo = 0;
  regionMapper = nullptr;
  special = nullptr;
  flowBackground = nullptr;
}

LineRegionsSupport::~LineRegionsSupport()
{
  clear();
  for (size_t i = 1; i < schemeStack.size(); i++) {
    delete schemeStack[i];
  }
  schemeStack.clear();
}

void LineRegionsSupport::resize(size_t lineCount_)
{
  lineRegions.resize(lineCount_);
  this->lineCount = lineCount_;
}

size_t LineRegionsSupport::size() const
{
  return lineCount;
}

void LineRegionsSupport::clear()
{
  for (auto& lineRegion : lineRegions) {
    LineRegion* ln = lineRegion;
    lineRegion = nullptr;
    while (ln != nullptr) {
      LineRegion* lnn = ln->next;
      delete ln;
      ln = lnn;
    }
  }
}

size_t LineRegionsSupport::getLineIndex(size_t lno) const
{
  return ((firstLineNo % lineCount) + lno - firstLineNo) % lineCount;
}

LineRegion* LineRegionsSupport::getLineRegions(size_t lno) const
{
  if (!checkLine(lno)) {
    return nullptr;
  }
  return lineRegions.at(getLineIndex(lno));
}

void LineRegionsSupport::setFirstLine(size_t first)
{
  firstLineNo = first;
}

size_t LineRegionsSupport::getFirstLine() const
{
  return firstLineNo;
}

void LineRegionsSupport::setBackground(const RegionDefine* back)
{
  background.rdef = const_cast<RegionDefine*>(back);
}

void LineRegionsSupport::setSpecialRegion(const Region* _special)
{
  special = _special;
}

void LineRegionsSupport::setRegionMapper(const RegionMapper* rs)
{
  regionMapper = rs;
}

bool LineRegionsSupport::checkLine(size_t lno) const
{
  if (lno < firstLineNo || lno >= firstLineNo + lineCount) {
    COLORER_LOG_TRACE("[LineRegionsSupport] checkLine: line % out of range", lno);
    return false;
  }
  return true;
}

void LineRegionsSupport::startParsing(size_t /*lno*/)
{
  for (size_t i = 1; i < schemeStack.size(); i++) {
    delete schemeStack[i];
  }
  schemeStack.clear();
  schemeStack.push_back(&background);
}

void LineRegionsSupport::clearLine(size_t lno, UnicodeString* /*line*/)
{
  if (!checkLine(lno)) {
    return;
  }

  LineRegion* ln = getLineRegions(lno);
  while (ln != nullptr) {
    LineRegion* lnn = ln->next;
    delete ln;
    ln = lnn;
  }
  auto* lfirst = new LineRegion(*schemeStack.back());
  lfirst->start = 0;
  lfirst->end = -1;
  lfirst->next = nullptr;
  lfirst->prev = lfirst;
  lineRegions.at(getLineIndex(lno)) = lfirst;
  flowBackground = lfirst;
}

void LineRegionsSupport::addRegion(size_t line_no, UnicodeString* /*line*/, int start_idx, int end_idx, const Region* region)
{
  // ignoring out of cached interval lines
  if (!checkLine(line_no)) {
    return;
  }
  auto* lnew = new LineRegion();
  lnew->start = start_idx;
  lnew->end = end_idx;
  lnew->region = region;
  lnew->scheme = schemeStack.back()->scheme;
  if (region->hasParent(special)) {
    lnew->special = true;
  }
  if (regionMapper != nullptr) {
    const RegionDefine* rd = regionMapper->getRegionDefine(region);
    if (rd == nullptr) {
      rd = schemeStack.back()->rdef;
    }
    if (rd != nullptr) {
      lnew->rdef = rd->clone();
      lnew->rdef->assignParent(schemeStack.back()->rdef);
    }
  }
  addLineRegion(line_no, lnew);
}

void LineRegionsSupport::enterScheme(size_t line_no, UnicodeString* /*line*/, int start_idx, int /*end_idx*/, const Region* region, const Scheme* scheme)
{
  auto* lr = new LineRegion();
  lr->region = region;
  lr->scheme = scheme;
  lr->start = start_idx;
  lr->end = -1;
  if (regionMapper != nullptr) {
    const RegionDefine* rd = regionMapper->getRegionDefine(region);
    if (rd == nullptr) {
      rd = schemeStack.back()->rdef;
    }
    if (rd != nullptr) {
      lr->rdef = rd->clone();
      lr->rdef->assignParent(schemeStack.back()->rdef);
    }
  }
  schemeStack.push_back(lr);
  // ignoring out of cached interval lines
  if (!checkLine(line_no)) {
    return;
  }
  // we must skip transparent regions
  if (lr->region != nullptr) {
    auto* lr_add = new LineRegion(*lr);
    flowBackground->end = lr_add->start;
    flowBackground = lr_add;
    addLineRegion(line_no, lr_add);
  }
}

void LineRegionsSupport::leaveScheme(size_t line_no, UnicodeString* /*line*/, int /*start_idx*/, int end_idx, const Region* /*region*/, const Scheme* /*scheme*/)
{
  const Region* scheme_region = schemeStack.back()->region;
  delete schemeStack.back();
  schemeStack.pop_back();
  // ignoring out of cached interval lines
  if (!checkLine(line_no)) {
    return;
  }
  // we have to skip transparent regions
  if (scheme_region != nullptr) {
    auto* lr = new LineRegion(*schemeStack.back());
    lr->start = end_idx;
    lr->end = -1;
    flowBackground->end = lr->start;
    flowBackground = lr;
    addLineRegion(line_no, lr);
  }
}

void LineRegionsSupport::addLineRegion(size_t line_no, LineRegion* lr)
{
  LineRegion* lstart = getLineRegions(line_no);
  lr->next = nullptr;
  lr->prev = lr;
  if (lstart == nullptr) {
    lineRegions.at(getLineIndex(line_no)) = lr;
  } else {
    lr->prev = lstart->prev;
    lr->prev->next = lr;
    lstart->prev = lr;
  }
}
