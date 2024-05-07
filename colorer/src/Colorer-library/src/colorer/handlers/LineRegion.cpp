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
  if (this == &lr)
    return *this;
  assigment(lr);
  return *this;
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

const StyledRegion* LineRegion::styled()
{
  return StyledRegion::cast(rdef);
}

const TextRegion* LineRegion::texted()
{
  return TextRegion::cast(rdef);
}
