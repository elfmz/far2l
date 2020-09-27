#ifndef _COLORER_LINESOURCE_H_
#define _COLORER_LINESOURCE_H_

#include <colorer/Common.h>

/**
 * Interface for editor line information requests.
 * Basic data source interface, used in TextParser processing.
 * @note Methods startJob and endJob are optional, and
 * could be implemented or not depending on system archtecture.
 * @ingroup colorer
 */
class LineSource
{
public:

  /**
   * Called by parser, when it starts text parsing.
   * @param lno Line number, which will be used as
   * initial position of all subsequend parsing.
   */
  virtual void startJob(size_t lno) {};

  /**
   * Called by parser, when it has finished text parsing.
   * Could be used to cleanup objects, allocated by last
   * #getLine() call.
   */
  virtual void endJob(size_t lno) {};

  /**
   * Returns line of text with specified number.
   * Returns String class pointer, which incapsulates information
   * about line with number <code>lno</code>.
   * @note Returned pointer must be valid until next getLine method call.
   *       If requested line can't be returned, fe there is no line with the passed
   *       index, method must return null.
   * @param lno Requested line number
   * @return Unicode string, enwrapped into String class.
   */
  virtual SString* getLine(size_t lno) = 0;
protected:
  LineSource() {};
  virtual ~LineSource() {};
};

#endif

