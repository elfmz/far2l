//========================================================================
//
// Array.h
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef ARRAY_H
#define ARRAY_H

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#if MULTITHREADED
#include "GMutex.h"
#endif
#include "Object.h"

class XRef;

//------------------------------------------------------------------------
// Array
//------------------------------------------------------------------------

class Array {
public:

  // Constructor.
  Array(XRef *xrefA);

  // Destructor.
  ~Array();

  // Reference counting.
#if MULTITHREADED
  int incRef() { return gAtomicIncrement(&ref); }
  int decRef() { return gAtomicDecrement(&ref); }
#else
  int incRef() { return ++ref; }
  int decRef() { return --ref; }
#endif

  // Get number of elements.
  int getLength() { return length; }

  // Add an element.
  void add(Object *elem);

  // Accessors.
  Object *get(int i, Object *obj);
  Object *getNF(int i, Object *obj);

private:

  XRef *xref;			// the xref table for this PDF file
  Object *elems;		// array of elements
  int size;			// size of <elems> array
  int length;			// number of elements in array
#if MULTITHREADED
  GAtomicCounter ref;		// reference count
#else
  int ref;			// reference count
#endif
};

#endif
