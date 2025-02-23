#ifndef COLORER_TEXTPARSERPELPERS_H
#define COLORER_TEXTPARSERPELPERS_H

#include "colorer/parsers/HrcLibraryImpl.h"

#if !defined COLORERMODE || defined NAMED_MATCHES_IN_HASH
#error need (COLORERMODE & !NAMED_MATCHES_IN_HASH) in cregexp
#endif

#define MATCH_NOTHING 0
#define MATCH_RE 1
#define MATCH_SCHEME 2

#define LINE_NEXT 0
#define LINE_REPARSE 1

/** Dynamic parser's list of virtual entries.
    @ingroup colorer_parsers
*/
class VTList
{
  VirtualEntryVector* vlist = nullptr;
  VTList* prev = nullptr;
  VTList* next = nullptr;
  VTList* last = this;
  VTList* shadowlast = nullptr;
  int nodesnum = 0;

 public:
  VTList() = default;
  ~VTList();
  void deltree();
  bool push(SchemeNodeInherit* node);
  bool pop();
  SchemeImpl* pushvirt(SchemeImpl* scheme);
  void popvirt();
  void clear();
  VirtualEntryVector** store();
  bool restore(VirtualEntryVector** store);
};

/**
 * Internal parser's cache storage. Each object instance
 * stores parse information about single level of Scheme
 * objects. The object always takes more, than a single line,
 * because single-line scheme match doesn't need to be cached.
 *
 * @ingroup colorer_parsers
 */
class ParseCache
{
 public:
  /** Start and end lines of this scheme match */
  int sline = 0;
  int eline = 0;
  /** Scheme, matched for this cache entry */
  SchemeImpl* scheme = nullptr;
  /** Particular parent block object, caused this scheme to
   * be instantiated.
   */
  const SchemeNodeBlock* clender = nullptr;

  /**
   * Scheme virtualization cache entry
   */
  VirtualEntryVector** vcache = nullptr;

  /**
   * RE Match object for start RE of the enwrapped <block> object
   */
  SMatches matchstart = {};
  /**
   * Copy of the line with parent's start RE.
   */
  UnicodeString* backLine = nullptr;

  /**
   * Tree structure references in parse cache
   */
  ParseCache* children = nullptr;
  ParseCache* next = nullptr;
  ParseCache* prev = nullptr;
  ParseCache* parent = nullptr;
  ParseCache() = default;
  ~ParseCache();
  /**
   * Searched a cache position for the specified line number.
   * @param ln     Line number to search for
   * @param cache  Cache entry, filled with last child cache entry.
   * @return       Cache entry, assigned to the specified line number
   */
  ParseCache* searchLine(int ln, ParseCache** cache);
};

#endif // COLORER_TEXTPARSERPELPERS_H
