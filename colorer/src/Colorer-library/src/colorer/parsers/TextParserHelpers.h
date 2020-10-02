#ifndef _COLORER_TEXTPARSERPELPERS_H_
#define _COLORER_TEXTPARSERPELPERS_H_

#include <colorer/parsers/HRCParserImpl.h>

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
  VirtualEntryVector* vlist;
  VTList* prev, *next, *last, *shadowlast;
  int nodesnum;
public:
  VTList();
  ~VTList();
  void deltree();
  bool push(SchemeNode* node);
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
  int sline, eline;
  /** Scheme, matched for this cache entry */
  SchemeImpl* scheme;
  /** Particular parent block object, caused this scheme to
    * be instantiated.
    */
  const SchemeNode* clender;
  /**
   * Scheme virtualization cache entry
   */
  VirtualEntryVector** vcache;
  /**
   * RE Match object for start RE of the enwrapped &lt;block> object
   */
  SMatches matchstart;
  /**
   * Copy of the line with parent's start RE.
   */
  SString* backLine;

  /**
   * Tree structure references in parse cache
   */
  ParseCache* children, *next, *prev, *parent;
  ParseCache();
  ~ParseCache();
  /**
   * Searched a cache position for the specified line number.
   * @param ln     Line number to search for
   * @param cache  Cache entry, filled with last child cache entry.
   * @return       Cache entry, assigned to the specified line number
   */
  ParseCache* searchLine(int ln, ParseCache** cache);
};

#endif


