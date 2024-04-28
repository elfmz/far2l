#include "colorer/Exception.h"
#include "colorer/handlers/StyledRegion.h"

StyledRegion::StyledRegion(bool _isForeSet, bool _isBackSet, unsigned int _fore, unsigned int _back, unsigned int _style)
{
  type = RegionDefine::RegionDefineType::STYLED_REGION;
  isForeSet = _isForeSet;
  isBackSet = _isBackSet;
  fore = _fore;
  back = _back;
  style = _style;
}

StyledRegion::StyledRegion()
{
  type = RegionDefine::RegionDefineType::STYLED_REGION;
  isForeSet = false;
  isBackSet = false;
  fore = 0;
  back = 0;
  style = RD_NONE;
}

StyledRegion::StyledRegion(const StyledRegion& rd) : RegionDefine()
{
  operator=(rd);
}

StyledRegion& StyledRegion::operator=(const StyledRegion& rd)
{
  if (this == &rd)
    return *this;
  setValues(&rd);
  return *this;
}

const StyledRegion* StyledRegion::cast(const RegionDefine* rd)
{
  if (rd == nullptr)
    return nullptr;
  if (rd->type != RegionDefine::RegionDefineType::STYLED_REGION)
    throw Exception("Bad type cast exception into StyledRegion");
  const auto* sr = (const StyledRegion*) (rd);
  return sr;
}

void StyledRegion::assignParent(const RegionDefine* _parent)
{
  const StyledRegion* parent = StyledRegion::cast(_parent);
  if (parent == nullptr)
    return;
  if (!isForeSet) {
    fore = parent->fore;
    isForeSet = parent->isForeSet;
  }
  if (!isBackSet) {
    back = parent->back;
    isBackSet = parent->isBackSet;
  }
  style = style | parent->style;
}

void StyledRegion::setValues(const RegionDefine* _rd)
{
  const StyledRegion* rd = StyledRegion::cast(_rd);
  if (rd) {
    fore = rd->fore;
    isForeSet = rd->isForeSet;
    back = rd->back;
    isBackSet = rd->isBackSet;
    style = rd->style;
    type = rd->type;
  }
}

RegionDefine* StyledRegion::clone() const
{
  RegionDefine* rd = new StyledRegion(*this);
  return rd;
}
