#ifndef COLORER_LINEREGIONSSUPPORT_H
#define COLORER_LINEREGIONSSUPPORT_H

#include "colorer/RegionHandler.h"
#include "colorer/handlers/LineRegion.h"
#include "colorer/handlers/RegionDefine.h"
#include "colorer/handlers/RegionMapper.h"
#include <vector>

/** Region store implementation of RegionHandler.
    @ingroup colorer_handlers
*/
class LineRegionsSupport : public RegionHandler
{
 public:
  LineRegionsSupport();
  ~LineRegionsSupport() override;

  /**
   * Resize structures to maintain regions for @c lineCount lines.
   */
  void resize(size_t lineCount);

  /**
   * Return current size of this line regions structure
   */
  size_t size() const;

  /**
   * Drops all internal structures
   */
  void clear();

  /**
   * Sets start line position of line structures.
   * This position tells, that first line structure refers
   * not to first line of text, but to @c first parameter value.
   */
  void setFirstLine(size_t first);

  /**
   * Returns first line position, installed in this line structures.
   */
  size_t getFirstLine() const;

  /**
   * Background region define, which is used to
   * fill transparent regions. If background is @c null,
   * then regions with transparent fields would leave these fields unfilled
   */
  void setBackground(const RegionDefine* back);

  /**
   * Tells handler to mark with special field
   * all Regions with specified ancestor.
   */
  void setSpecialRegion(const Region* special);

  /**
   * Choose the source of RegionDefine definitions.
   * This source returns information about mapping
   * Region objects into RegionDefine objects.
   */
  void setRegionMapper(const RegionMapper* rds);

  /**
   * Returns LineRegion object for @c lno line number.
   * This object is linked with all other stored @c LineRegion objects
   */
  [[nodiscard]] LineRegion* getLineRegions(size_t lno) const;

  /**
   * RegionHandler implementation
   */
  void startParsing(size_t lno) override;
  void clearLine(size_t lno, UnicodeString* line) override;
  void addRegion(size_t line_no, UnicodeString* line, int start_idx, int end_idx, const Region* region) override;
  void enterScheme(size_t line_no, UnicodeString* line, int start_idx, int end_idx, const Region* region, const Scheme* scheme) override;
  void leaveScheme(size_t line_no, UnicodeString* line, int start_idx, int end_idx, const Region* region, const Scheme* scheme) override;

 protected:
  /**
   * Behaviour is redefined in derived classes
   */
  virtual void addLineRegion(size_t line_no, LineRegion* lr);
  [[nodiscard]] size_t getLineIndex(size_t lno) const;
  [[nodiscard]] bool checkLine(size_t lno) const;

  std::vector<LineRegion*> lineRegions;
  std::vector<LineRegion*> schemeStack;

  const RegionMapper* regionMapper;
  LineRegion* flowBackground;
  const Region* special;

  LineRegion background;
  size_t firstLineNo;
  size_t lineCount;
};

#endif // COLORER_LINEREGIONSSUPPORT_H
