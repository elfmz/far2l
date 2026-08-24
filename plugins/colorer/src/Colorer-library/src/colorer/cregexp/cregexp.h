#ifndef COLORER_CREGEXP_H
#define COLORER_CREGEXP_H

#include "colorer/Common.h"
#include <array>
#include <vector>

/**
    @addtogroup cregexp Regular Expressions
      Colorer Regular Expressions (cregexp) class implementation.
*/

/// with this define class uses extended command set for
/// colorer compatibility mode
/// if you undef it, it will compile stantard set for
/// regexp compatibility mode
#define COLORERMODE

/// numeric matches num
#define MATCHES_NUM 0x10
// number of named brackets (access through SMatches.ns)
#define NAMED_MATCHES_NUM 0x10

enum class EOps {
  ReBlockOps,   // sentinel: postfix operators follow
  ReRangeN,     // {n,}  *  +
  ReRangeNM,    // {n,m} ?
  ReNGRangeN,   // {n,}? *? +?
  ReNGRangeNM,  // {n,m}? ??
  ReOr,         // |
  ReBehind,     // ?#n
  ReNBehind,    // ?~n
  ReAhead,      // ?=
  ReNAhead,     // ?!

  ReSymbolOps,  // sentinel: atoms follow
  ReEmpty,
  ReMetaSymb,       // \W \s \d ...
  ReSymb,           // a b c ...
  ReWord,           // word...
  ReEnum,           // [] [^]
  ReBrackets,       // (...)
  ReNamedBrackets,  // (?{name} ...)
#ifdef COLORERMODE
  ReBkTrace,       // \yN
  ReBkTraceN,      // \YN
  ReBkTraceName,   // \y{name}
  ReBkTraceNName,  // \Y{name}
#endif
  ReBkBrack,     // \N
  ReBkBrackName  // \p{name}
};

enum class EMetaSymbols {
  ReBadMeta,
  ReAnyChr,  // .
  ReSoL,     // ^
#ifdef COLORERMODE
  ReSoScheme,  // ~
#endif
  ReEoL,        // $
  ReDigit,      // \d
  ReNDigit,     // \D
  ReWordSymb,   // \w
  ReNWordSymb,  // \W
  ReWSpace,     // \s isWhiteSpace()
  ReNWSpace,    // \S
  ReUCase,      // \u
  ReNUCase,     // \l
  ReWBound,     // \b
  ReNWBound,    // \B
  RePreNW,      // \c
#ifdef COLORERMODE
  ReStart,  // \m
  ReEnd,    // \M
#endif

  ReChrLast,
};

enum class EError { EOK = 0, EERROR, ESYNTAX, EBRACKETS, EENUM, EOP };

/// @ingroup cregexp
struct SMatches
{
  SMatches()
  {
    reset();
  }
  void reset()
  {
    s[0] = e[0] = -1;
    cMatch = 0;
    topse = 0;
    ns[0] = ne[0] = -1;
    cnMatch = 0;
    topnse = 0;
  }

  void topseSanitize(int cur); // use before accessing s[cur]/e[cur] to ensure their lazy inited to -1
  int s[MATCHES_NUM];
  int e[MATCHES_NUM];
  int topse;
  int cMatch;

  void topnseSanitize(int cur); // use before accessing ns[cur]/ne[cur] to ensure their lazy inited to -1
  int ns[NAMED_MATCHES_NUM];
  int ne[NAMED_MATCHES_NUM];
  int topnse;
  int cnMatch;
};

/** Regular expressions internal tree node.
    @ingroup cregexp
*/
class SRegInfo
{
 public:
  SRegInfo();
  ~SRegInfo();
  SRegInfo(const SRegInfo&) = delete;
  SRegInfo& operator=(const SRegInfo&) = delete;
  SRegInfo(SRegInfo&&) = delete;
  SRegInfo& operator=(SRegInfo&&) = delete;

  union {
    EMetaSymbols metaSymbol;
    UChar symbol;
    UnicodeString* word;
    CharacterClass* charclass;
    SRegInfo* param;
  } un;
  SRegInfo* parent = nullptr;
  SRegInfo* next = nullptr;
  SRegInfo* prev = nullptr;
  int oldParse = 0;
  int param0 = 0;
  int param1 = 0;
  int s = 0;
  int e = 0;

  EOps op = EOps::ReEmpty;
};

enum ReAction {
  rea_None = -1,
  rea_False = 0,
  rea_True = 1,
  rea_Break,
  rea_RangeNM_step2,
  rea_RangeNM_step3,
  rea_RangeN_step2,
  rea_NGRangeN_step2,
  rea_NGRangeNM_step2,
  rea_NGRangeNM_step3
};

struct StackElem
{
  // local variable
  SRegInfo* re;
  SRegInfo* prev;
  int toParse;
  bool leftenter;
  ReAction ifTrueReturn;
  ReAction ifFalseReturn;
};

#define INIT_MEM_SIZE 512
#define MEM_INC 128
/** Regular Expression compiler and matcher.
    Colorer regular expressions library cregexp.

\par 1. Features.

\par 1.1. Colorer Unicode classes.
   - Unicode Consortium regexp level 1 support.
     All characters are treated as independent 16-bit units.
     The result of RE is independent of current locale.
   - Unicode syntax extensions:
     - Unicode general category char class:
         - [{L}{Nd}] - all letters and decimal digits,
         - [{ALL}]   - as '.',
         - [{ASSIGNED}] - all assigned unicode characters,
         - [{UNASSIGNED}] - all unassigned unicode characters.
     - Char classes substraction unicode extension:
         - [{ASSIGNED}-[{Lu}]-[{Ll}]] - all assigned characters except,
         - upper and lower case characters.
     - Char classes connection syntax:
         - [{Lu}[{Ll}]] - upper and lower case characters.
     - Char classes intersection syntax:
         - [{ALL}&&[{L}]] - only Letter characters.
     - Character reference syntax: \\x{2028} \\x0A as in Perl.
     - Unicode form \\u2028 is unused (\\u - upper case char).

\par 1.2. Extensions.
   - Bracket extensions:
     - (?{name} pattern ) - named bracket,
     - \\p{name} - named bracket reference.
     - (?{} pattern ) - no capturing bracket as (?: pattern ) in Perl.
   - Look Ahead/Backward:
     - pattern?=  as Perl's (?=pattern)
     - pattern?!  as Perl's (?!pattern)
     - pattern?#N - N symbols backward look for pattern
     - pattern?~N - N symbols backward look for no pattern
   - Colorer library extensions:
     - \\m \\M - sets new start and end of zero(default) bracket.
     - \\yN \\YN \\y{name} \\Y{name} - back reference into another RE's bracket.

\par 1.3. Perl compatibility.
   - Modifiers //isx
   - \\ p{name} - back reference to named bracket (but not named property as in Perl!)
   - No POSIX character classes support.



\par 2. Dislikes:

\par 2.1. According to Unicode RE level 1 support:
   - No surrogate symbols support,
   - No string length changes on case mappings (only 1 <-> 1 mappings),
\par 2.2. Algorithmic problems:
   - Explicit parse stack (grows as needed and is reused by all CRegExp
     instances; matching is single-threaded).

    @ingroup cregexp
*/
class CRegExp
{
 public:
  /**
    Empty constructor. No RE tree is builded with this constructor.
    Use #setRE method to change pattern.
  */
  CRegExp();
  /**
    Constructs regular expression and compile it with @c text pattern.
  */
  CRegExp(const UnicodeString* text);
  ~CRegExp();
  CRegExp(const CRegExp&) = delete;
  CRegExp& operator=(const CRegExp&) = delete;
  CRegExp(CRegExp&&) = delete;
  CRegExp& operator=(CRegExp&&) = delete;

  /**
    Is compilied RE well-formed.
  */
  bool isOk() const;

  /**
    Returns information about RE compilation error.
  */
  EError getError() const;

  /**
    Tells RE parser, that it must make moves on tested string while RE matching.
  */
  bool setPositionMoves(bool moves);
  /**
    Returns named bracket index, or -1 if the name is unknown.
  */
  int getBracketNo(const UnicodeString* brname) const;
  /**
    Returns named bracket name by its index. Owned by this CRegExp.
  */
  const UnicodeString* getBracketName(int no) const;
#ifdef COLORERMODE
  bool setBackRE(CRegExp* bkre);
  /**
    Changes RE object, used for backreferences with named \y{} \Y{} operators.
  */
  bool setBackTrace(const UnicodeString* str, SMatches* trace);
  /**
    Returns current RE object, used for backreferences with \y \Y operators.
  */
  bool getBackTrace(const UnicodeString** str, SMatches** trace) const;
  /**
    True if this RE contains \y / \Y backtrace operators and needs the start-line copy.
  */
  bool hasBackTrace() const;
#endif
  /**
    Compiles specified regular expression and drops all
    previous structures.
  */
  bool setRE(const UnicodeString* re);
  /** Runs RE parser against input string @c str
   */
  bool parse(const UnicodeString* str, SMatches* mtch);
  /** Runs RE parser against input string @c str
   */
  bool parse(const UnicodeString* str, int pos, int eol, SMatches* mtch, int soscheme = 0,
             int moves = -1);
  bool canStartWith(wchar ch) const;
  /**
   * Caps backtracking steps in one parse() call. When exceeded, the match
   * fails (it is not a wall-clock quantum). Default 1 000 000.
   */
  void setParseStepLimit(int limit);
  int getParseStepLimit() const;
  bool exceededParseStepLimit() const;

 private:
  bool ignoreCase = false;
  bool extend = false;
  bool positionMoves = false;
  bool singleLine = false;
  bool multiLine = false;
  SRegInfo* tree_root = nullptr;
  EError error = EError::EOK;
  SRegInfo* firstNode = nullptr;
  std::array<uint64_t, 2> firstCharMask = {};
  bool firstCharMaskUseful = false;
#ifdef COLORERMODE
  CRegExp* backRE = nullptr;
  const UnicodeString* backStr = nullptr;
  SMatches* backTrace = nullptr;
  bool usesBackTrace = false;
  int schemeStart = 0;
#endif
  bool startChange = false;
  bool endChange = false;
  const UnicodeString* global_pattern = nullptr;
  int end = 0;

  SMatches* matches = nullptr;
  int cMatch = 0;

  UnicodeString* brnames[NAMED_MATCHES_NUM] = {};
  int cnMatch = 0;

  void init();
  EError setRELow(const UnicodeString& re);
  EError setStructs(SRegInfo*&, const UnicodeString& expr, int from, int to, int& endPos);

  bool matchChars(wchar one, wchar another) const;
  struct FirstChars
  {
    std::array<uint64_t, 2> mask = {};
    bool nullable = false;
  };
  FirstChars analyzeFirstChars(const SRegInfo* re) const;
  FirstChars firstCharsForNode(const SRegInfo* re) const;
  void addFirstChar(FirstChars& result, wchar ch) const;
  void optimize();
  bool quickCheck(int toParse);
  bool isWordBoundary(int toParse);
  bool checkMetaSymbol(EMetaSymbols metaSymbol, int& toParse);
  bool matchCopiedRange(const UnicodeString& src, int from, int to, int& toParse, bool icase) const;
  bool lowParse(SRegInfo* re, SRegInfo* prev, int toParse);
  bool parseRE(int toParse);

  int count_elem;
  int parseSteps = 0;
  int parseStepLimit = 1000000;
  bool stepBudgetExceeded = false;
  void check_stack(bool res, SRegInfo** re, SRegInfo** prev, int* toParse, bool* leftenter,
                   ReAction* action);
  void insert_stack(SRegInfo** re, SRegInfo** prev, int* toParse, bool* leftenter, ReAction ifTrueReturn,
                    ReAction ifFalseReturn, SRegInfo** re2, SRegInfo** prev2, int toParse2);

  static std::vector<StackElem> RegExpStack;

 public:
  static void clearRegExpStack();
};

#endif  // COLORER_CREGEXP_H
