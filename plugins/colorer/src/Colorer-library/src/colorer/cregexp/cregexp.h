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

/// Bit per ASCII code point (0..127). Used for the first-char and required-char prefilters.
using AsciiCharMask = std::array<uint64_t, 2>;

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
  // First ASCII characters of the left ReOr branch. Used to skip that branch
  // when the subject cannot start it; unused unless branchFirstUseful.
  AsciiCharMask branchFirst = {};
  bool branchFirstUseful = false;
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
     instances on the same thread).

\par 3. Matching pipeline (hot path).

   setRE compiles an SRegInfo tree; optimize() then fills skip facts used
   by parseRE / mayMatch before the NFA (lowParse) runs:

   - firstCharMask / firstNode — first consuming ASCII set and first
     literal/class/word (quickCheck). Unused if the prefix is nullable.
   - startAnchor — pattern begins with ^ (not /m) or ~; only pos==0 or
     pos==schemeStart can match, even with positionMoves.
   - endAnchor + maxLen — pattern ends with $ (not /m, no top-level |).
     A match cannot start more than maxLen characters before eol.
     maxLen==-1 if * / + / \\N / \\y make length unbounded.
     $ means toParse==eol (the parse() bound, not str->length()).
   - requiredChars — up to 4 ASCII sets that must appear somewhere in
     the subject (TextParser passes a per-line mask). /i letters omitted.
   - ReOr.branchFirst — skip a | branch in lowParse when the current
     ASCII char cannot start it. Left unused if the branch is nullable
     or starts with a zero-width op (\\m \\M \\b lookaround ^ $): those
     still have side effects (\\M bounds group 0 for a later alternative).

   parse() pins parseBuf to UnicodeString::getBuffer() so the NFA does
   not index the string per step. Each offset resets \\m/\\M and captures.
   The backtracking stack is thread_local and shared by every CRegExp on
   that thread; count_elem is reset per parseRE. Do not clear it from
   ParserFactory teardown. parseStepLimit (default 1e6) counts NFA steps
   in one parse(); exceeding it fails the match.

   TextParser calls mayMatch() with the same pos/eol/schemeStart/line
   mask before parse() to avoid entering the NFA.

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
  /** Runs RE parser against input string @c str.
   *  @param subjectChars optional mask of ASCII characters present anywhere in @c str
   *         (not only in [pos, eol)). Lets the matcher reject patterns whose
   *         mandatory literals are absent from the line without running the NFA.
   */
  bool parse(const UnicodeString* str, int pos, int eol, SMatches* mtch, int soscheme = 0,
             int moves = -1, const AsciiCharMask* subjectChars = nullptr);
  bool canStartWith(wchar ch) const;
  /**
   * Fills @c mask with every ASCII character of @c str; use as @c subjectChars in parse().
   */
  static void collectAsciiChars(const UnicodeString& str, AsciiCharMask& mask);
  /**
   * Cheap pre-check of what parse() would reject before running the matcher:
   * a start anchor (^ or ~) at another position, an end-anchored pattern whose
   * bounded length cannot reach @c eol, or a mandatory literal missing from
   * @c subjectChars. False means parse() cannot succeed there.
   * @c eol is the same bound parse() would receive ($ is toParse == eol).
   */
  bool mayMatch(int pos, int eol, int soscheme, const AsciiCharMask& subjectChars) const
  {
    if (startAnchor == StartAnchor::LineStart && pos != 0)
      return false;
#ifdef COLORERMODE
    if (startAnchor == StartAnchor::SchemeStart && pos != soscheme)
      return false;
#else
    (void) soscheme;
#endif
    // Moving searches skip forward to eol - maxLen in parseRE; only a
    // fixed-position attempt is impossible when too much text remains.
    if (endAnchor && maxLen >= 0 && !positionMoves && eol - pos > maxLen)
      return false;
    for (int i = 0; i < requiredCharsCount; i++) {
      if (((requiredChars[i][0] & subjectChars[0]) | (requiredChars[i][1] & subjectChars[1])) == 0)
        return false;
    }
    return true;
  }
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
  AsciiCharMask firstCharMask = {};
  bool firstCharMaskUseful = false;
  // Pattern begins with ^ (single-line) or ~: only one start position can match.
  enum class StartAnchor : uint8_t { None, LineStart, SchemeStart };
  StartAnchor startAnchor = StartAnchor::None;
  // Pattern ends with $ (single-line, no top-level alternation): match can
  // only finish at eol, so it cannot start more than maxLen before eol.
  bool endAnchor = false;
  // Maximum characters the tree can consume; -1 = unbounded (* / + / \N / \y).
  int maxLen = -1;
  // Every match must contain at least one character from each of these sets.
  static constexpr int MAX_REQUIRED_SETS = 4;
  std::array<AsciiCharMask, MAX_REQUIRED_SETS> requiredChars = {};
  int requiredCharsCount = 0;
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
  const wchar* parseBuf = nullptr;
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
    AsciiCharMask mask = {};
    bool nullable = false;
  };
  FirstChars analyzeFirstChars(const SRegInfo* re) const;
  FirstChars firstCharsForNode(const SRegInfo* re) const;
  void addFirstChar(FirstChars& result, wchar ch) const;
  std::vector<AsciiCharMask> requiredCharsForChain(const SRegInfo* re) const;
  std::vector<AsciiCharMask> requiredCharsForNode(const SRegInfo* re) const;
  void addRequiredChar(std::vector<AsciiCharMask>& out, wchar ch) const;
  void analyzeStartAnchor();
  void analyzeEndAnchor();
  int maxLenOfNode(const SRegInfo* re) const;
  int maxLenOfChain(const SRegInfo* re) const;
  void analyzeMaxLen();
  void analyzeRequiredChars();
  void analyzeBranchFirstChars(SRegInfo* re);
  void optimize();
  bool quickCheck(int toParse);
  bool isWordBoundary(int toParse);
  bool checkMetaSymbol(EMetaSymbols metaSymbol, int& toParse);
  bool matchCopiedRange(const UnicodeString& src, int from, int to, int& toParse, bool icase) const;
  bool lowParse(SRegInfo* re, SRegInfo* prev, int toParse);
  bool parseRE(int toParse, const AsciiCharMask* subjectChars);
  void bindSubject(const UnicodeString* str);

  int count_elem;
  int parseSteps = 0;
  int parseStepLimit = 1000000;
  bool stepBudgetExceeded = false;
  void growRegExpStack();
  void check_stack(bool res, SRegInfo*& re, SRegInfo*& prev, int& toParse, bool& leftenter, ReAction& action);
  void insert_stack(SRegInfo*& re, SRegInfo*& prev, int& toParse, bool& leftenter, ReAction ifTrueReturn,
                    ReAction ifFalseReturn, SRegInfo* re2, SRegInfo* prev2, int toParse2);

  static thread_local std::vector<StackElem> RegExpStack;

  static bool isLineBreak(wchar c)
  {
    return c == 0x0A || c == 0x0B || c == 0x0C || c == 0x0D || c == 0x85 || c == 0x2028 || c == 0x2029;
  }

 public:
  static void clearRegExpStack();
};

inline bool CRegExp::isWordBoundary(int toParse)
{
  const bool after = (toParse < end && Character::isLetterOrDigitOrUnderscore(parseBuf[toParse]));
  const bool before = (toParse > 0 && Character::isLetterOrDigitOrUnderscore(parseBuf[toParse - 1]));
  return before != after;
}

inline bool CRegExp::checkMetaSymbol(EMetaSymbols symb, int& toParse)
{
  switch (symb) {
    case EMetaSymbols::ReAnyChr:
      if (toParse >= end || (!singleLine && isLineBreak(parseBuf[toParse])))
        return false;
      toParse++;
      return true;

    case EMetaSymbols::ReSoL:
      return toParse == 0 || (multiLine && isLineBreak(parseBuf[toParse - 1]));

    case EMetaSymbols::ReEoL:
      return toParse == end || (multiLine && toParse && toParse < end && isLineBreak(parseBuf[toParse - 1]));

    case EMetaSymbols::ReDigit:
      if (toParse >= end || !Character::isDigit(parseBuf[toParse]))
        return false;
      toParse++;
      return true;

    case EMetaSymbols::ReNDigit:
      if (toParse >= end || Character::isDigit(parseBuf[toParse]))
        return false;
      toParse++;
      return true;

    case EMetaSymbols::ReWordSymb:
      if (toParse >= end || !Character::isLetterOrDigitOrUnderscore(parseBuf[toParse]))
        return false;
      toParse++;
      return true;

    case EMetaSymbols::ReNWordSymb:
      if (toParse >= end || Character::isLetterOrDigitOrUnderscore(parseBuf[toParse]))
        return false;
      toParse++;
      return true;

    case EMetaSymbols::ReWSpace:
      if (toParse >= end || !Character::isWhitespace(parseBuf[toParse]))
        return false;
      toParse++;
      return true;

    case EMetaSymbols::ReNWSpace:
      if (toParse >= end || Character::isWhitespace(parseBuf[toParse]))
        return false;
      toParse++;
      return true;

    case EMetaSymbols::ReUCase:
      if (toParse >= end || !Character::isUpperCase(parseBuf[toParse]))
        return false;
      toParse++;
      return true;

    case EMetaSymbols::ReNUCase:
      if (toParse >= end || !Character::isLowerCase(parseBuf[toParse]))
        return false;
      toParse++;
      return true;

    case EMetaSymbols::ReWBound:
      return isWordBoundary(toParse);

    case EMetaSymbols::ReNWBound:
      return !isWordBoundary(toParse);

    case EMetaSymbols::RePreNW:
      return toParse == 0 || toParse >= end || !Character::isLetter(parseBuf[toParse - 1]);

#ifdef COLORERMODE
    case EMetaSymbols::ReSoScheme:
      return (schemeStart == toParse);

    case EMetaSymbols::ReStart:
      matches->s[0] = toParse;
      startChange = true;
      return true;

    case EMetaSymbols::ReEnd:
      matches->e[0] = toParse;
      endChange = true;
      return true;
#endif

    default:
      return false;
  }
}

inline void CRegExp::check_stack(bool res, SRegInfo*& re, SRegInfo*& prev, int& toParse, bool& leftenter,
                                 ReAction& action)
{
  if (count_elem == 0) {
    action = res ? rea_True : rea_False;
    return;
  }

  const StackElem& ne = RegExpStack[--count_elem];
  action = res ? ne.ifTrueReturn : ne.ifFalseReturn;
  re = ne.re;
  prev = ne.prev;
  toParse = ne.toParse;
  leftenter = ne.leftenter;
}

inline void CRegExp::insert_stack(SRegInfo*& re, SRegInfo*& prev, int& toParse, bool& leftenter,
                                  ReAction ifTrueReturn, ReAction ifFalseReturn, SRegInfo* re2, SRegInfo* prev2,
                                  int toParse2)
{
  if (RegExpStack.size() == static_cast<size_t>(count_elem)) {
    growRegExpStack();
  }
  RegExpStack[static_cast<size_t>(count_elem++)] =
      StackElem{re, prev, toParse, leftenter, ifTrueReturn, ifFalseReturn};

  prev = prev2;
  re = re2;
  toParse = toParse2;
  leftenter = true;
  if (!re && prev != nullptr) {
    re = prev->parent;
    leftenter = false;
  }
}

#endif  // COLORER_CREGEXP_H
