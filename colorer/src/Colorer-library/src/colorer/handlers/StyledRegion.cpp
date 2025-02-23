#include "colorer/handlers/StyledRegion.h"
#include "colorer/Exception.h"

StyledRegion::StyledRegion(const bool _isForeSet, const bool _isBackSet, const unsigned int _fore,
                           const unsigned int _back, const unsigned int _style)
{
  type = RegionDefineType::STYLED_REGION;
  isForeSet = _isForeSet;
  isBackSet = _isBackSet;
  fore = _fore;
  back = _back;
  style = _style;
}

StyledRegion::StyledRegion()
{
  type = RegionDefineType::STYLED_REGION;
}

StyledRegion::StyledRegion(const StyledRegion& rd) : RegionDefine()
{
  operator=(rd);
}

StyledRegion& StyledRegion::operator=(const StyledRegion& rd)
{
  if (this != &rd) {
    setValues(&rd);
  }
  return *this;
}

const StyledRegion* StyledRegion::cast(const RegionDefine* rd)
{
  if (rd == nullptr) {
    return nullptr;
  }
  if (rd->type != RegionDefineType::STYLED_REGION) {
    throw Exception("Bad type cast exception into StyledRegion");
  }
  const auto* sr = dynamic_cast<const StyledRegion*>(rd);
  return sr;
}

void StyledRegion::assignParent(const RegionDefine* _parent)
{
  const StyledRegion* parent = cast(_parent);
  if (parent == nullptr) {
    return;
  }
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
  const StyledRegion* rd = cast(_rd);
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
