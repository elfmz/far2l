#include "colorer/parsers/TextParserHelpers.h"

/////////////////////////////////////////////////////////////////////////
// parser's cache structures

ParseCache::~ParseCache()
{
  // COLORER_LOG_DEEPTRACE("[TPCache] ~ParseCache():%,%-%", *scheme->getName(), sline, eline);
  delete backLine;
  delete children;
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
  }

  delete[] vcache;
}

ParseCache* ParseCache::searchLine(int ln, ParseCache** cache)
{
  ParseCache* r1 = nullptr;
  ParseCache* r2 = nullptr;
  ParseCache* tmp = this;
  *cache = nullptr;
  while (tmp) {
    COLORER_LOG_DEEPTRACE("[TPCache] searchLine() tmp:%,%-%", *tmp->scheme->getName(), tmp->sline, tmp->eline);
    if (tmp->sline <= ln && tmp->eline >= ln) {
      if (tmp->children) {
        r1 = tmp->children->searchLine(ln, &r2);
      }
      if (r1) {
        *cache = r2;
        return r1;
      }
      *cache = r2;  // last child
      return tmp;
    }
    if (tmp->sline <= ln) {
      *cache = tmp;
    }
    tmp = tmp->next;
  }
  return nullptr;
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
