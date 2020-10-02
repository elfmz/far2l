#ifndef __CREGEXP__
#define __CREGEXP__

#include<colorer/unicode/String.h>
#include<colorer/unicode/CharacterClass.h>

/**
    @addtogroup cregexp Regular Expressions
      Colorer Regular Expressions (cregexp) class implementation.
*/

/// with this define class uses extended command set for
/// colorer compatibility mode
/// if you undef it, it will compile stantard set for
/// regexp compatibility mode
#define COLORERMODE

/// use hashes for saving named brackets
//#define NAMED_MATCHES_IN_HASH

/// check duplicate brackets
//#define CHECKNAMES

#if defined COLORERMODE && defined NAMED_MATCHES_IN_HASH
#error COLORERMODE && NAMED_MATCHES_IN_HASH not realyzed yet
#endif

/// numeric matches num
#define MATCHES_NUM 0x10

#if !defined NAMED_MATCHES_IN_HASH
// number of named brackets (access through SMatches.ns)
#define NAMED_MATCHES_NUM 0x10
#endif

#ifdef NAMED_MATCHES_IN_HASH
struct SMatch{
  int s,e;
};
// you can redefine this class
typedef class SMatchHash{
public:
  SMatch *setItem(const String *name, SMatch &smatch){return nullptr;};
  SMatch *getItem(const String *name){return nullptr;};
}*PMatchHash;
#endif


enum EOps
{
  ReBlockOps,
  ReMul,              // *
  RePlus,             // +
  ReQuest,            // ?
  ReNGMul,            // *?
  ReNGPlus,           // +?
  ReNGQuest,          // ??
  ReRangeN,           // {n,}
  ReRangeNM,          // {n,m}
  ReNGRangeN,         // {n,}?
  ReNGRangeNM,        // {n,m}?
  ReOr,               // |
  ReBehind,           // ?#n
  ReNBehind,          // ?~n
  ReAhead,            // ?=
  ReNAhead,           // ?!

  ReSymbolOps,
  ReEmpty,
  ReMetaSymb,         // \W \s \d ...
  ReSymb,             // a b c ...
  ReWord,             // word...
  ReEnum,             // []
  ReNEnum,            // [^]
  ReBrackets,         // (...)
  ReNamedBrackets,    // (?{name} ...)
#ifdef COLORERMODE
  ReBkTrace,          // \yN
  ReBkTraceN,         // \YN
  ReBkTraceName,      // \y{name}
  ReBkTraceNName,     // \Y{name}
#endif
  ReBkBrack,          // \N
  ReBkBrackName       // \p{name}
};

enum EMetaSymbols
{
  ReBadMeta,
  ReAnyChr,           // .
  ReSoL,              // ^
#ifdef COLORERMODE
  ReSoScheme,         // ~
#endif
  ReEoL,              // $
  ReDigit,            // \d
  ReNDigit,           // \D
  ReWordSymb,         // \w
  ReNWordSymb,        // \W
  ReWSpace,           // \s isWhiteSpace()
  ReNWSpace,          // \S
  ReUCase,            // \u
  ReNUCase,           // \l
  ReWBound,           // \b
  ReNWBound,          // \B
  RePreNW,            // \c
#ifdef COLORERMODE
  ReStart,            // \m
  ReEnd,              // \M
#endif

  ReChrLast,
};

enum EError
{
  EOK = 0, EERROR, ESYNTAX, EBRACKETS, EENUM, EOP
};


/// @ingroup cregexp
struct SMatches
{
  int s[MATCHES_NUM];
  int e[MATCHES_NUM];
  int cMatch;
#if !defined NAMED_MATCHES_IN_HASH
  int ns[NAMED_MATCHES_NUM];
  int ne[NAMED_MATCHES_NUM];
  int cnMatch;
#endif
};

/** Regular expressions internal tree node.
    @ingroup cregexp
*/
class SRegInfo
{
public:
  SRegInfo();
  ~SRegInfo();

  union{
    EMetaSymbols metaSymbol;
    wchar symbol;
    String *word;
    CharacterClass *charclass;
    SRegInfo *param;
  }un;
#if defined NAMED_MATCHES_IN_HASH
  String *namedata;
#endif
  SRegInfo *parent;
  SRegInfo *next;
  SRegInfo *prev;
  int oldParse;
  int param0, param1;
  int s, e;

  EOps op;
};

struct StackElem{
  //local variable
  SRegInfo *re;
  SRegInfo *prev;
  int toParse;
  bool leftenter;
  // step if function return true
  int ifTrueReturn;
  // step if function return false
  int ifFalseReturn;
};

extern StackElem *RegExpStack;
extern int RegExpStack_Size;

#define INIT_MEM_SIZE 512
#define MEM_INC 128

enum ReAction {
  rea_False=0,
  rea_True=1,
  rea_Break,
  rea_RangeNM_step2,
  rea_RangeNM_step3,
  rea_RangeN_step2,
  rea_NGRangeN_step2,
  rea_NGRangeNM_step2,
  rea_NGRangeNM_step3
};
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
   - Modifiers //ismx
   - \\ p{name} - back reference to named bracket (but not named property as in Perl!)
   - No POSIX character classes support.



\par 2. Dislikes:

\par 2.1. According to Unicode RE level 1 support:
   - No surrogate symbols support,
   - No string length changes on case mappings (only 1 <-> 1 mappings),
\par 2.2. Algorithmic problems:
   - Stack recursion implementation.

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
  CRegExp(const String *text);
  ~CRegExp();

  /**
    Is compilied RE well-formed.
  */
  bool isOk();

  /**
    Returns information about RE compilation error.
  */
  EError getError();

  /**
    Tells RE parser, that it must make moves on tested string while RE matching.
  */
  bool setPositionMoves(bool moves);
  /**
    Returns count of named brackets.
  */
  int getBracketNo(const String *brname);
  /**
    Returns named bracked name by it's index.
  */
  String *getBracketName(int no);
#ifdef COLORERMODE
  bool setBackRE(CRegExp *bkre);
  /**
    Changes RE object, used for backreferences with named \y{} \Y{} operators.
  */
  bool setBackTrace(const String *str, SMatches *trace);
  /**
    Returns current RE object, used for backreferences with \y \Y operators.
  */
  bool getBackTrace(const String **str, SMatches **trace);
#endif
  /**
    Compiles specified regular expression and drops all
    previous structures.
  */
  bool setRE(const String *re);
#ifdef NAMED_MATCHES_IN_HASH
  /** Runs RE parser against input string @c str
  */
  bool parse(const String *str, SMatches *mtch, SMatchHash *nmtch = nullptr);
  /** Runs RE parser against input string @c str
  */
  bool parse(const String *str, int pos, int eol, SMatches *mtch, SMatchHash *nmtch = nullptr, int soscheme = 0, int moves = -1);
#else
  /** Runs RE parser against input string @c str
  */
  bool parse(const String *str, SMatches *mtch);
  /** Runs RE parser against input string @c str
  */
  bool parse(const String *str, int pos, int eol, SMatches *mtch, int soscheme = 0, int moves = -1);
#endif

private:
  bool ignoreCase, extend, positionMoves, singleLine, multiLine;
  SRegInfo *tree_root;
  EError error;
  wchar firstChar;
  EMetaSymbols firstMetaChar;
#ifdef COLORERMODE
  CRegExp *backRE;
  const String *backStr;
  SMatches *backTrace;
  int schemeStart;
#endif
  bool startChange, endChange;
  const String *global_pattern;
  int end;

  SMatches *matches;
  int cMatch;
#if !defined NAMED_MATCHES_IN_HASH
  String* brnames[NAMED_MATCHES_NUM];
  int cnMatch;
#else
  SMatchHash *namedMatches;
#endif

  void init();
  EError setRELow(const String &re);
  EError setStructs(SRegInfo *&, const String &expr, int &endPos);

  void optimize();
  bool quickCheck(int toParse);
  bool isWordBoundary(int &toParse);
  bool isNWordBoundary(int &toParse);
  bool checkMetaSymbol(EMetaSymbols metaSymbol, int &toParse);
  bool lowParse(SRegInfo *re, SRegInfo *prev, int toParse);
  bool parseRE(int toParse);

  int count_elem;
  void check_stack(bool res,SRegInfo **re, SRegInfo **prev, int *toParse,bool *leftenter, int *action);
  void insert_stack(SRegInfo **re, SRegInfo **prev, int *toParse, bool *leftenter, int ifTrueReturn, int ifFalseReturn, SRegInfo **re2, SRegInfo **prev2, int toParse2);

};



#endif


