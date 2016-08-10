#ifndef _COLORER_OUTLINER_H_
#define _COLORER_OUTLINER_H_

#include<common/Vector.h>
#include<colorer/LineSource.h>
#include<colorer/RegionHandler.h>
#include<colorer/editor/OutlineItem.h>
#include<colorer/editor/BaseEditor.h>

/**
 * Used to create, store and maintain list or tree of different special regions.
 * These can include functions, methods, fields, classes, errors and so on.
 * Works as a filter on input editor stream.
 *
 * @ingroup colorer_editor
 */
class Outliner : public RegionHandler, public EditorListener {
public:
  /**
   * Creates outliner object, that searches stream for
   * the specified type of region. Outliner is deattached
   * on its explicit destruction action.
   *
   * @param baseEditor Editor to attach this Outliner to.
   * @param searchRegion Region type to search in parser's stream
   */
  Outliner(BaseEditor *baseEditor, const Region *searchRegion);
  ~Outliner();

  /**
   * Returns reference to item with specified ordinal
   * index in list of currently generated outline items.
   * Note, that the returned pointer is vaild only between
   * subsequent parser invocations.
   */
  OutlineItem *getItem(int idx);

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
  static int manageTree(Vector<int> &treeStack, int newLevel);

  /**
   * Total number of currently available outline items
   */
  int itemCount();

  void startParsing(int lno);
  void endParsing(int lno);
  void clearLine(int lno, String *line);
  void addRegion(int lno, String *line, int sx, int ex, const Region *region);
  void enterScheme(int lno, String *line, int sx, int ex, const Region *region, const Scheme *scheme);
  void leaveScheme(int lno, String *line, int sx, int ex, const Region *region, const Scheme *scheme);
  void modifyEvent(int topLine);

protected:
  bool isOutlined(const Region*region);

  BaseEditor *baseEditor;
  const Region *searchRegion;
  Vector<OutlineItem*> outline;
  bool lineIsEmpty;
  int curLevel;
  int modifiedLine;
};

#endif
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Colorer Library.
 *
 * The Initial Developer of the Original Code is
 * Cail Lomecb <cail@nm.ru>.
 * Portions created by the Initial Developer are Copyright (C) 1999-2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
