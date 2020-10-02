#ifndef _COLORER_PAIRMATCH_H_
#define _COLORER_PAIRMATCH_H_

#include<colorer/handlers/LineRegionsSupport.h>

/**
 * Representation of pair match in text.
 * Contains information about two regions on two lines.
 * @ingroup colorer_editor
 */
class PairMatch{
public:
  /**
   * Region's start position as a cloned LineRegion object.
   */
  LineRegion *start;
  /**
   * Region's end position as a cloned LineRegion object.
   */
  LineRegion *end;
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
  PairMatch(LineRegion *startRef, int lineNo, bool topPosition){
    start = end = nullptr;
    this->startRef = startRef;
    sline = lineNo;
    pairBalance = -1;
    this->topPosition = false;
    if (topPosition){
      pairBalance = 1;
      this->topPosition = true;
    }
    eline = -1;
  }

  virtual ~PairMatch(){
    delete start;
    delete end;
  }


  LineRegion *getStartRef(){
    return startRef;
  }

  /**
   * Sets a start region properties. Passed object is cloned to keep
   * pair match properties consistent between parse stages
   */
  void setStart(LineRegion *pair){
    if (start != nullptr){
      delete start;
    }
    if (pair != nullptr){
      start = new LineRegion(*pair);
    }
  }

  /**
   * Sets an end region properties. Passed object is cloned to keep
   * pair match properties consistent between parse stages
   */
  void setEnd(LineRegion *pair){
    if (end != nullptr){
      delete end;
    }
    if (pair != nullptr){
      end = new LineRegion(*pair);
    }
  }
private:
  /**
   * Region's start position as a reference to inparse sequence.
   */
  LineRegion *startRef;
};

#endif


