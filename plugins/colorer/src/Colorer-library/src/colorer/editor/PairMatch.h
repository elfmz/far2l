#ifndef COLORER_PAIRMATCH_H
#define COLORER_PAIRMATCH_H

#include "colorer/handlers/LineRegionsSupport.h"

/**
 * Representation of pair match in text.
 * Contains information about two regions on two lines.
 * @ingroup colorer_editor
 */
class PairMatch
{
 public:
  /**
   * Region's start position as a cloned LineRegion object.
   */
  LineRegion* start;
  /**
   * Region's end position as a cloned LineRegion object.
   */
  LineRegion* end;
  /**
   * Starting Line of pair
   */
  int sline;
  /**
   * Ending Line of pair
   */
  int eline;
  /**
   * Identifies initial position of cursor in pair
   */
  bool topPosition;
  /**
   * Internal pair search counter
   */
  int pairBalance;

  /**
   * Default constructor.
   * Clears all fields
   */
  PairMatch(LineRegion* startRef, int lineNo, bool _topPosition)
  {
    start = end = nullptr;
    this->startRef = startRef;
    sline = lineNo;
    pairBalance = -1;
    this->topPosition = false;
    if (_topPosition) {
      pairBalance = 1;
      this->topPosition = true;
    }
    eline = -1;
  }

  virtual ~PairMatch()
  {
    delete start;
    delete end;
  }

  LineRegion* getStartRef()
  {
    return startRef;
  }

  /**
   * Sets a start region properties. Passed object is cloned to keep
   * pair match properties consistent between parse stages
   */
  void setStart(const LineRegion* pair)
  {
    delete start;
    if (pair != nullptr) {
      start = new LineRegion(*pair);
    }
  }

  /**
   * Sets an end region properties. Passed object is cloned to keep
   * pair match properties consistent between parse stages
   */
  void setEnd(const LineRegion* pair)
  {
    delete end;
    if (pair != nullptr) {
      end = new LineRegion(*pair);
    }
  }

  PairMatch(PairMatch&&) = delete;
  PairMatch(const PairMatch&) = delete;
  PairMatch& operator=(const PairMatch&) = delete;
  PairMatch& operator=(PairMatch&&) = delete;

 private:
  /**
   * Region's start position as a reference to inparse sequence.
   */
  LineRegion* startRef;
};

#endif // COLORER_PAIRMATCH_H
