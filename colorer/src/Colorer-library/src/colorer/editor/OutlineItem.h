#ifndef _COLORER_OUTLINEITEM_H_
#define _COLORER_OUTLINEITEM_H_

#include <memory>
#include <colorer/Region.h>

/**
 * Item in outliner's list.
 * Contans all the information about single
 * structured token with specified type (region reference).
 * @ingroup colorer_editor
 */
class OutlineItem
{
public:
  /** Line number */
  size_t lno;
  /** Position in line */
  int pos;
  /** Level of enclosure */
  int level;
  /** Item text */
  std::unique_ptr<SString> token;
  /** This item's region */
  const Region* region;

  /** Default constructor */
  OutlineItem() : lno(0), pos(0), level(0), token(nullptr), region(nullptr)
  {
  }

  /** Initializing constructor */
  OutlineItem(size_t lno_, int pos_, int level_, String* token_, const Region* region_):
    lno(lno_), pos(pos_), level(level_), token(nullptr), region(region_)
  {
    if (token_ != nullptr) {
      token.reset(new SString(token_));
    }
  }

  ~OutlineItem()
  {
  }
};

#endif


