#include "colorer/parsers/TextParserHelpers.h"

/////////////////////////////////////////////////////////////////////////
// parser's cache structures

ParseCache::~ParseCache()
{
  // COLORER_LOG_DEEPTRACE("[TPCache] ~ParseCache():%,%-%", *scheme->getName(), sline, eline);
  ParseCache* previous = prev;
  delete backLine;
  dropChildren();
  prev = nullptr;

  if (next) {
    ParseCache* tmp;
    tmp = next;
    while (tmp->next) {
      tmp = tmp->next;
    }
    while (tmp->prev) {
      tmp = tmp->prev;
      delete tmp->next;
      tmp->next = nullptr;
    }
    delete next;
    next = nullptr;
  }

  delete[] vcache;
  if (parent && parent->search_child == this) {
    parent->search_child = previous;
  }
}

void ParseCache::dropChildren()
{
  delete children;
  children = nullptr;
  search_child = nullptr;
}

void ParseCache::dropNext()
{
  delete next;
  next = nullptr;
}

namespace {

ParseCache* rightmostAtOrBefore(ParseCache* node, int ln)
{
  while (node->next && node->next->sline <= ln) {
    node = node->next;
  }
  return node;
}

}  // namespace

ParseCache* ParseCache::searchLine(int ln, ParseCache** cache)
{
  *cache = nullptr;

  ParseCache* node = this;
  if (parent && parent->search_child) {
    node = parent->search_child;
  }

  if (node->sline <= ln) {
    node = rightmostAtOrBefore(node, ln);
  }
  else if (node->prev && node->prev->sline <= ln) {
    // One sibling back: tryParseLine(line+1) then searchLine(line).
    node = node->prev;
  }
  else {
    // Long jump toward the start: walk from the head, not back from EOF.
    node = rightmostAtOrBefore(this, ln);
  }

  if (parent) {
    parent->search_child = node;
  }
  if (node->sline > ln) {
    return nullptr;
  }

  COLORER_LOG_DEEPTRACE("[TPCache] searchLine() tmp:%,%-%", *node->scheme->getName(), node->sline, node->eline);
  if (node->eline < ln) {
    *cache = node;
    return nullptr;
  }

  ParseCache* child_cache = nullptr;
  if (node->children) {
    if (auto* found = node->children->searchLine(ln, &child_cache)) {
      *cache = child_cache;
      return found;
    }
  }

  *cache = child_cache;  // last child
  return node;
}

/////////////////////////////////////////////////////////////////////////
// Virtual tables list

bool VTList::push(SchemeNodeInherit* node)
{
  if (!node || node->virtualEntryVector.empty()) {
    return false;
  }
  const int insert_at = last_index + 1;
  nodes.insert(nodes.begin() + insert_at, Node {&node->virtualEntryVector, -1});
  last_index = insert_at;
  return true;
}

bool VTList::pop()
{
  if (last_index < 0) {
    return false;
  }
  nodes.erase(nodes.begin() + last_index);
  last_index--;
  return true;
}

SchemeImpl* VTList::pushvirt(SchemeImpl* scheme)
{
  if (last_index < 0) {
    return nullptr;
  }

  SchemeImpl* ret = scheme;
  int curvl = -1;

  for (int i = last_index; i >= 0; --i) {
    for (auto* ve : *nodes[static_cast<size_t>(i)].vlist) {
      if (ret == ve->virtScheme && ve->substScheme) {
        ret = ve->substScheme;
        curvl = i;
      }
    }
  }
  if (curvl >= 0) {
    nodes[static_cast<size_t>(curvl)].shadow_last = last_index;
    last_index = curvl - 1;
    return ret;
  }
  return nullptr;
}

void VTList::popvirt()
{
  const int that = last_index + 1;
  last_index = nodes[static_cast<size_t>(that)].shadow_last;
  nodes[static_cast<size_t>(that)].shadow_last = -1;
}

void VTList::clear()
{
  nodes.clear();
  last_index = -1;
}

VirtualEntryVector** VTList::store()
{
  if (last_index < 0) {
    return nullptr;
  }
  auto store = new VirtualEntryVector*[static_cast<size_t>(last_index) + 2];
  for (int i = 0; i <= last_index; i++) {
    store[i] = nodes[static_cast<size_t>(i)].vlist;
  }
  store[last_index + 1] = nullptr;
  return store;
}

bool VTList::restore(VirtualEntryVector** store)
{
  if (last_index >= 0 || !store) {
    return false;
  }

  for (int i = 0; store[i] != nullptr; i++) {
    nodes.push_back(Node {store[i], -1});
  }
  last_index = static_cast<int>(nodes.size()) - 1;
  return true;
}
