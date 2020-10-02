#include <colorer/handlers/LineRegionsCompactSupport.h>

LineRegionsCompactSupport::LineRegionsCompactSupport() {}
LineRegionsCompactSupport::~LineRegionsCompactSupport() {}

void LineRegionsCompactSupport::addLineRegion(size_t lno, LineRegion* ladd)
{
  LineRegion* lstart = getLineRegions(lno);
  ladd->next = nullptr;
  ladd->prev = ladd;

  if (ladd->special) {
    // adds last and returns
    if (lstart == nullptr) {
      lineRegions.at(getLineIndex(lno)) = ladd;
    } else {
      ladd->prev = lstart->prev;
      lstart->prev->next = ladd;
      lstart->prev = ladd;
    }
    return;
  }
  if (lstart == nullptr) {
    lineRegions.at(getLineIndex(lno)) = ladd;
    return;
  }

  // finds position of new 'ladd' region
  for (LineRegion* ln = lstart; ln; ln = ln->next) {
    // insert before first
    if (lstart->start >= ladd->start) {
      ladd->next = lstart;
      ladd->prev = lstart->prev;
      lstart->prev = ladd;
      lstart = ladd;
      break;
    }
    // insert before
    if (ln->start == ladd->start) {
      ln->prev->next = ladd;
      ladd->next = ln;
      ladd->prev = ln->prev;
      ln->prev = ladd;
      break;
    }
    // add last
    if (ln->start < ladd->start && !ln->next) {
      ln->next = ladd;
      ladd->next = nullptr;
      ladd->prev = ln;
      lstart->prev = ladd;
      break;
    }
    // insert between or before special
    if (ln->start < ladd->start && (ln->next->start > ladd->start || ln->next->special)) {
      ladd->next = ln->next;
      ladd->prev = ln;
      ln->next->prev = ladd;
      ln->next = ladd;
      break;
    }
  }
  // previous region intersection check
  if (ladd != lstart && ladd->prev && (ladd->prev->end > ladd->start || ladd->prev->end == -1)) {
    // our region breaks previous region into two parts
    if ((ladd->prev->end > ladd->end || ladd->prev->end == -1) && ladd->end != -1) {
      LineRegion* ln1 = new LineRegion(*ladd->prev);
      ln1->prev = ladd;
      ln1->next = ladd->next;
      if (ladd->next) {
        ladd->next->prev = ln1;
      }
      if (ln1->next == nullptr) {
        lstart->prev = ln1;
      }
      ladd->next = ln1;
      ln1->start = ladd->end;
      if (ladd->prev == flowBackground) {
        flowBackground = ln1;
      }
    }
    ladd->prev->end = ladd->start;
    // zero-width region deletion
    if (ladd->prev != lstart && ladd->prev->end == ladd->prev->start) {
      ladd->prev->prev->next = ladd;
      LineRegion* lntemp = ladd->prev;
      ladd->prev = ladd->prev->prev;
      delete lntemp;
    }
    if (ladd->prev == lstart && ladd->prev->end == ladd->prev->start) {
      LineRegion* lntemp = ladd->prev->prev;
      delete ladd->prev;
      ladd->prev = lntemp;
      lstart = ladd;
    }
  }
  // possible forward intersections
  for (LineRegion* lnext = ladd->next; lnext; lnext = lnext->next) {
    if (lnext->special) {
      continue;
    }
    if ((lnext->end == -1 || lnext->end > ladd->end) && ladd->end != -1
        && lnext->start < ladd->end) {
      lnext->start = ladd->end;
    }
    // make region zero-width, if it is hided by our new region
    if ((lnext->end <= ladd->end && lnext->end != -1) || ladd->end == -1) {
      ladd->next = lnext->next;
      if (lnext->next) {
        lnext->next->prev = ladd;
      } else {
        lstart->prev = ladd;
      }
      delete lnext;
      lnext = ladd;
      continue;
    }
  }
  lineRegions.at(getLineIndex(lno)) = lstart;
}



