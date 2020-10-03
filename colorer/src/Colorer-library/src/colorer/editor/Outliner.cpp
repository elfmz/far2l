#include <colorer/editor/Outliner.h>

Outliner::Outliner(BaseEditor* baseEditor, const Region* searchRegion)
{
  this->searchRegion = searchRegion;
  modifiedLine = -1;
  this->baseEditor = baseEditor;
  baseEditor->addRegionHandler(this);
  baseEditor->addEditorListener(this);
}

Outliner::~Outliner()
{
  baseEditor->removeRegionHandler(this);
  baseEditor->removeEditorListener(this);

  for (auto it : outline) {
    delete it;
  }
  outline.clear();
}

OutlineItem* Outliner::getItem(size_t idx)
{
  return outline.at(idx);
}

size_t Outliner::itemCount()
{
  return outline.size();
}

int Outliner::manageTree(std::vector<int>& treeStack, int newLevel)
{
  while (treeStack.size() > 0 && newLevel < treeStack.back()) {
    treeStack.pop_back();
  }
  if (treeStack.size() == 0 || newLevel > treeStack.back()) {
    treeStack.push_back(newLevel);
    return treeStack.size() - 1;
  }
  if (newLevel == treeStack.back()) {
    return treeStack.size() - 1;
  }
  return 0;
}

bool Outliner::isOutlined(const Region* region)
{
  return region->hasParent(searchRegion);
}

void Outliner::modifyEvent(size_t topLine)
{
  for (size_t i = 0; i <outline.size(); ++i) {
    if (outline[i]->lno >= topLine) {
      outline.resize(i);
      break;
    }
  }

  modifiedLine = topLine;
}

void Outliner::startParsing(size_t lno)
{
  curLevel = 0;
}

void Outliner::endParsing(size_t lno)
{
  if (modifiedLine < lno) {
    modifiedLine = lno + 1;
  }
  curLevel = 0;
}

void Outliner::clearLine(size_t lno, String* line)
{
  lineIsEmpty = true;
}

void Outliner::addRegion(size_t lno, String* line, int sx, int ex, const Region* region)
{
  if (lno < modifiedLine) {
    return;
  }
  if (!isOutlined(region)) {
    return;
  }

  String* itemLabel = new CString(line, sx, ex - sx);

  if (lineIsEmpty) {
    outline.push_back(new OutlineItem(lno, sx, curLevel, itemLabel, region));
  } else {
    OutlineItem* thisItem = outline.back();
    if (thisItem->token != nullptr && thisItem->lno == lno) {
      thisItem->token->append(itemLabel);
    }
  }
  delete itemLabel;
  lineIsEmpty = false;
}

void Outliner::enterScheme(size_t lno, String* line, int sx, int ex, const Region* region, const Scheme* scheme)
{
  curLevel++;
}

void Outliner::leaveScheme(size_t lno, String* line, int sx, int ex, const Region* region, const Scheme* scheme)
{
  curLevel--;
}



