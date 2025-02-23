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

VTList::~VTList()
{
  //  FAULT(next == this);
  // deletes only from root
  if (!prev && next) {
    next->deltree();
  }
}

void VTList::deltree()
{
  if (next) {
    next->deltree();
  }
  delete this;
}

bool VTList::push(SchemeNodeInherit* node)
{
  if (!node || node->virtualEntryVector.empty()) {
    return false;
  }
  auto newitem = new VTList();
  if (last->next) {
    last->next->prev = newitem;
    newitem->next = last->next;
  }
  newitem->prev = last;
  last->next = newitem;
  last = last->next;
  last->vlist = &node->virtualEntryVector;
  nodesnum++;
  return true;
}

bool VTList::pop()
{
  //  FAULT(last == this);
  VTList* ditem = last;
  if (ditem->next) {
    ditem->next->prev = ditem->prev;
  }
  ditem->prev->next = ditem->next;
  last = ditem->prev;
  delete ditem;
  nodesnum--;
  return true;
}

SchemeImpl* VTList::pushvirt(SchemeImpl* scheme)
{
  SchemeImpl* ret = scheme;
  VTList* curvl = nullptr;

  for (VTList* vl = last; vl && vl->prev; vl = vl->prev) {
    for (auto ve : *vl->vlist) {
      if (ret == ve->virtScheme && ve->substScheme) {
        ret = ve->substScheme;
        curvl = vl;
      }
    }
  }
  if (curvl) {
    curvl->shadowlast = last;
    last = curvl->prev;
    return ret;
  }
  return nullptr;
}

void VTList::popvirt()
{
  VTList* that = last->next;
  //  FAULT(!last->next || !that->shadowlast);
  last = that->shadowlast;
  that->shadowlast = nullptr;
}

void VTList::clear()
{
  nodesnum = 0;
  if (!prev && next) {
    next->deltree();
    next = nullptr;
  }
  last = this;
}

VirtualEntryVector** VTList::store()
{
  if (!nodesnum || last == this) {
    return nullptr;
  }
  int i = 0;
  auto store = new VirtualEntryVector*[nodesnum + 1];
  for (VTList* list = this->next; list; list = list->next) {
    store[i++] = list->vlist;
    if (list == this->last) {
      break;
    }
  }
  store[i] = nullptr;
  return store;
}

bool VTList::restore(VirtualEntryVector** store)
{
  if (next || prev || !store) {
    return false;
  }

  VTList* prevpos;
  VTList* pos = this;
  for (int i = 0; store[i] != nullptr; i++) {
    pos->next = new VTList;
    prevpos = pos;
    pos = pos->next;
    pos->prev = prevpos;
    pos->vlist = store[i];
    nodesnum++;
  }
  last = pos;
  return true;
}
