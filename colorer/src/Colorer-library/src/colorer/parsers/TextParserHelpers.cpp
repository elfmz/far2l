#include <colorer/parsers/TextParserHelpers.h>


/////////////////////////////////////////////////////////////////////////
// parser's cache structures
ParseCache::ParseCache()
{
  children = next = prev = parent = nullptr;
  backLine = nullptr;
  vcache = nullptr;
}

ParseCache::~ParseCache()
{
  CTRACE(logger->trace("[TPCache] ~ParseCache():{0},{1}-{2}", scheme->getName()->getChars(), sline, eline));
  delete backLine;
  delete children;
  prev = nullptr;

  while (next) {
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
}

ParseCache* ParseCache::searchLine(int ln, ParseCache** cache)
{
  ParseCache* r1 = nullptr, *r2 = nullptr, *tmp = this;
  *cache = nullptr;
  while (tmp) {
    CTRACE(logger->trace("[TPCache] searchLine() tmp:{0},{1}-{2}", tmp->scheme->getName()->getChars(), tmp->sline, tmp->eline));
    if (tmp->sline <= ln && tmp->eline >= ln) {
	  if (tmp->children) {
		r1 = tmp->children->searchLine(ln, &r2);
	  }
      if (r1) {
        *cache = r2;
        return r1;
      }
      *cache = r2; // last child
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
VTList::VTList()
{
  vlist = nullptr;
  prev = next = nullptr;
  last = this;
  shadowlast = nullptr;
  nodesnum = 0;
}

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

bool VTList::push(SchemeNode* node)
{
  VTList* newitem;
  if (!node || node->virtualEntryVector.size() == 0) {
    return false;
  }
  newitem = new VTList();
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
  VTList* ditem;
//  FAULT(last == this);
  ditem = last;
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
    for (size_t idx = 0; idx < vl->vlist->size(); idx++) {
      VirtualEntry* ve = vl->vlist->at(idx);
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
  VirtualEntryVector** store;
  int i = 0;
  if (!nodesnum || last == this) {
    return nullptr;
  }
  store = new VirtualEntryVector*[nodesnum + 1];
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
  VTList* prevpos, *pos = this;
  if (next || prev || !store) {
    return false;
  }
//  nodesnum = store[0].shadowlast;
  prevpos = last = nullptr;
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



