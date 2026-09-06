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
    freeRegion(schemeStack[i]);
  }
  schemeStack.clear();
  for (auto* lr : regionPool) {
    delete lr;
  }
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
    freeRegionChain(ln);
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
  delete background.rdef;
  background.rdef = back != nullptr ? back->clone() : nullptr;
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
    freeRegion(schemeStack[i]);
  }
  schemeStack.clear();
  schemeStack.push_back(&background);
}

void LineRegionsSupport::clearLine(size_t lno, UnicodeString* /*line*/)
{
  if (!checkLine(lno)) {
    return;
  }

  freeRegionChain(getLineRegions(lno));
  auto* lfirst = allocRegion(*schemeStack.back());
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
  auto* lnew = allocRegion();
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
    lnew->applyRegionDefine(rd, schemeStack.back()->rdef);
  }
  else {
    lnew->applyRegionDefine(nullptr, nullptr);
  }
  addLineRegion(line_no, lnew);
}

void LineRegionsSupport::enterScheme(size_t line_no, UnicodeString* /*line*/, int start_idx, int /*end_idx*/, const Region* region, const Scheme* scheme)
{
  auto* lr = allocRegion();
  lr->region = region;
  lr->scheme = scheme;
  lr->start = start_idx;
  lr->end = -1;
  if (regionMapper != nullptr) {
    const RegionDefine* rd = regionMapper->getRegionDefine(region);
    if (rd == nullptr) {
      rd = schemeStack.back()->rdef;
    }
    lr->applyRegionDefine(rd, schemeStack.back()->rdef);
  }
  else {
    lr->applyRegionDefine(nullptr, nullptr);
  }
  schemeStack.push_back(lr);
  // ignoring out of cached interval lines
  if (!checkLine(line_no)) {
    return;
  }
  // we must skip transparent regions
  if (lr->region != nullptr) {
    auto* lr_add = allocRegion(*lr);
    flowBackground->end = lr_add->start;
    flowBackground = lr_add;
    addLineRegion(line_no, lr_add);
  }
}

void LineRegionsSupport::leaveScheme(size_t line_no, UnicodeString* /*line*/, int /*start_idx*/, int end_idx, const Region* /*region*/, const Scheme* /*scheme*/)
{
  const Region* scheme_region = schemeStack.back()->region;
  freeRegion(schemeStack.back());
  schemeStack.pop_back();
  // ignoring out of cached interval lines
  if (!checkLine(line_no)) {
    return;
  }
  // we have to skip transparent regions
  if (scheme_region != nullptr) {
    auto* lr = allocRegion(*schemeStack.back());
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

LineRegion* LineRegionsSupport::allocRegion()
{
  LineRegion* lr;
  if (regionPool.empty()) {
    lr = new LineRegion();
  }
  else {
    lr = regionPool.back();
    regionPool.pop_back();
    lr->next = nullptr;
    lr->prev = nullptr;
    lr->start = 0;
    lr->end = 0;
    lr->scheme = nullptr;
    lr->region = nullptr;
    lr->special = false;
  }
  return lr;
}

LineRegion* LineRegionsSupport::allocRegion(const LineRegion& src)
{
  LineRegion* lr = allocRegion();
  lr->adoptFrom(src);
  return lr;
}

void LineRegionsSupport::freeRegion(LineRegion* lr)
{
  if (lr == nullptr || lr == &background) {
    return;
  }
  lr->next = nullptr;
  lr->prev = nullptr;
  regionPool.push_back(lr);
}

void LineRegionsSupport::freeRegionChain(LineRegion* ln)
{
  while (ln != nullptr) {
    LineRegion* next = ln->next;
    freeRegion(ln);
    ln = next;
  }
}
