#include "colorer/handlers/LineRegion.h"

LineRegion::LineRegion(const LineRegion& lr)
{
  assigment(lr);
  next = nullptr;
  prev = nullptr;
}

LineRegion::~LineRegion()
{
  delete rdef;
}

void LineRegion::assigment(const LineRegion& lr)
{
  start = lr.start;
  end = lr.end;
  scheme = lr.scheme;
  region = lr.region;
  special = lr.special;
  rdef = nullptr;
  if (lr.rdef != nullptr) {
    rdef = lr.rdef->clone();
  }
}

LineRegion& LineRegion::operator=(const LineRegion& lr)
{
  if (this != &lr) {
    assigment(lr);
  }
  return *this;
}

void LineRegion::replaceRdef(const RegionDefine* rd)
{
  if (rd == nullptr) {
    delete rdef;
    rdef = nullptr;
    return;
  }
  if (rdef != nullptr && rdef->type == rd->type) {
    rdef->setValues(rd);
    return;
  }
  delete rdef;
  rdef = rd->clone();
}

void LineRegion::adoptFrom(const LineRegion& lr)
{
  start = lr.start;
  end = lr.end;
  scheme = lr.scheme;
  region = lr.region;
  special = lr.special;
  next = nullptr;
  prev = nullptr;
  replaceRdef(lr.rdef);
}

void LineRegion::applyRegionDefine(const RegionDefine* rd, const RegionDefine* parent)
{
  replaceRdef(rd);
  if (rdef != nullptr && parent != nullptr) {
    rdef->assignParent(parent);
  }
}

LineRegion::LineRegion()
{
  next = nullptr;
  prev = nullptr;
  start = 0;
  end = 0;
  scheme = nullptr;
  region = nullptr;
  rdef = nullptr;
  special = false;
}

const StyledRegion* LineRegion::styled() const
{
  return StyledRegion::cast(rdef);
}

const TextRegion* LineRegion::texted() const
{
  return TextRegion::cast(rdef);
}
