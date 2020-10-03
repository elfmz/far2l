#ifndef _COLORER_OUTLINER_H_
#define _COLORER_OUTLINER_H_

#include <colorer/LineSource.h>
#include <colorer/RegionHandler.h>
#include <colorer/editor/OutlineItem.h>
#include <colorer/editor/BaseEditor.h>

/**
 * Used to create, store and maintain list or tree of different special regions.
 * These can include functions, methods, fields, classes, errors and so on.
 * Works as a filter on input editor stream.
 *
 * @ingroup colorer_editor
 */
class Outliner : public RegionHandler, public EditorListener
{
public:
  /**
   * Creates outliner object, that searches stream for
   * the specified type of region. Outliner is deattached
   * on its explicit destruction action.
   *
   * @param baseEditor Editor to attach this Outliner to.
   * @param searchRegion Region type to search in parser's stream
   */
  Outliner(BaseEditor* baseEditor, const Region* searchRegion);
  ~Outliner();

  /**
   * Returns reference to item with specified ordinal
   * index in list of currently generated outline items.
   * Note, that the returned pointer is vaild only between
   * subsequent parser invocations.
   */
  OutlineItem* getItem(size_t idx);

  /**
   * Static service method to make easy tree reconstruction
   * from created list of outline items. This list contains
   * unpacked level indexed of item's enclosure in scheme.
   * @param treeStack external Vector of integer, storing
   *        temporary tree structure. Must not be changed
   *        externally.
   * @param newLevel Unpacked level of item to be added into
   *        the tree. This index is converted into packed one
   *        and returned.
   * @return Packed index of item, which could be used to
   *         reconstruct tree of outlined items.
   */
  static int manageTree(std::vector<int>& treeStack, int newLevel);

  /**
   * Total number of currently available outline items
   */
  size_t itemCount();

  void startParsing(size_t lno);
  void endParsing(size_t lno);
  void clearLine(size_t lno, String* line);
  void addRegion(size_t lno, String* line, int sx, int ex, const Region* region);
  void enterScheme(size_t lno, String* line, int sx, int ex, const Region* region, const Scheme* scheme);
  void leaveScheme(size_t lno, String* line, int sx, int ex, const Region* region, const Scheme* scheme);
  void modifyEvent(size_t topLine);

protected:
  bool isOutlined(const Region* region);

  BaseEditor* baseEditor;
  const Region* searchRegion;
  std::vector<OutlineItem*> outline;
  bool lineIsEmpty;
  int curLevel;
  size_t modifiedLine;
};

#endif


