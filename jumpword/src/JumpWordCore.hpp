#pragma once

// Word-matching logic shared between the plugin (src/JumpWord.cpp) and the
// standalone unit tests (tests/test_jumpword.cpp). Kept free of FAR SDK
// dependencies so it can be compiled and tested outside of the far2l tree.

#include <cstdio>
#include <string>

// isIdChar is intentionally only declared here: the plugin
// build defines it in terms of FSF.LIsAlphanum, while the test build defines
// it in terms of iswalnum.
bool isIdChar(const wchar_t c);

inline void
LogFoundWord(const wchar_t *lineBegin, const wchar_t *lineEnd, const wchar_t *foundWord) {
#ifdef _DEBUG
    std::wstring line(lineBegin, lineEnd);
    fprintf(stderr, "\033[0;31mJUMPWORD:\033[m line:  '%ls'\n", line.c_str());
    std::string markers = std::string(foundWord - lineBegin, ' ') + '^';
    fprintf(stderr, "\033[0;31mJUMPWORD:\033[m found:  %s \n", markers.c_str());
#endif
}

inline bool FindNextWord(
    const wchar_t *begin,
    const wchar_t *end,
    const wchar_t *wordBegin,
    const wchar_t *wordEnd,
    const wchar_t **result) {
    // Find the next word in the line. Two usage scenarios are possible:
    // 1. search in the lines that are below the line containing original word
    // 2. search within the line contained original word, but after the word
    // itself
    //
    // Based on the supported search scenarios, begin is guaranteed to either
    // point to the start of the line or to point to the element that is
    // immediately after the element delimiting the word being searched.
    //
    // Because of that we can safely assume that we can compare character
    // immediately.
    bool isCheckingWord          = true;
    const wchar_t *wordCurrent   = wordBegin;
    const wchar_t *foundLocation = begin;

    while (begin < end) {
        if (isCheckingWord && *begin == *wordCurrent) {
            wordCurrent++;
            if (wordCurrent == wordEnd) {
                // The whole pattern matched, but that's only a real word match if
                // it's not merely a prefix of a longer identifier (e.g. "a" inside
                // "ab").
                const wchar_t *next = begin + 1;
                if (next == end || !isIdChar(*next)) {
                    *result = foundLocation;
                    return true;
                }
                // It's the start of a longer word; skip past the rest of it.
                isCheckingWord = false;
            }
        } else {
            if (isIdChar(*begin)) {
                isCheckingWord = false;
            } else {
                isCheckingWord = true;
                wordCurrent    = wordBegin;
                // the word potentially starts after current character
                foundLocation = begin + 1;
            }
        }
        begin++;
    }
    return false;
}

inline bool FindPreviousWord(
    const wchar_t *begin,
    const wchar_t *end,
    const wchar_t *wordBegin,
    const wchar_t *wordEnd,
    const wchar_t **result) {
    // Find the previous word in the line. Two usage scenarios are possible:
    // 1. search in the lines that are above the line containing original word
    // 2. search within the line contained original word, but before the word
    // itself
    //
    // Based on the supported search scenarios, end is guaranteed to either
    // point to the end of the line or to point to the first element of the
    // original word
    //
    // Because of that we can safely assume that we can compare character
    // immediately.
    bool isCheckingWord        = true;
    const wchar_t *wordCurrent = wordEnd - 1;

    while (begin <= end) {
        if (isCheckingWord && *end == *wordCurrent) {
            if (wordCurrent == wordBegin) {
                // The whole pattern matched, but that's only a real word match if
                // it's not merely the tail of a longer identifier (e.g. "at"
                // inside "cat").
                if (end == begin || !isIdChar(*(end - 1))) {
                    *result = end;
                    return true;
                }
                // It's the tail of a longer word; skip past the rest of it.
                isCheckingWord = false;
            } else {
                wordCurrent--;
            }
        } else {
            if (isIdChar(*end)) {
                isCheckingWord = false;
            } else {
                isCheckingWord = true;
                wordCurrent    = wordEnd - 1;
            }
        }
        end--;
    }
    return false;
}

// GetLineFn: bool(int currentLine, const wchar_t **lineBegin, const wchar_t **lineEnd)
// UIUpdateFn: bool(int currentLine, int totalLines)
template <typename GetLineFn, typename UIUpdateFn>
bool FindWordBelow(
    int searchStartLine,
    int totalLines,
    const wchar_t *lineBegin,
    const wchar_t *lineEnd,
    const wchar_t *wordBegin,
    const wchar_t *wordEnd,
    int *foundPosition,
    int *foundLine,
    GetLineFn getLine,
    UIUpdateFn uiUpdate) {

    int currentLine = searchStartLine;
    // The search starts after the original word on the same line
    const wchar_t *searchStart = wordEnd + 1;

    while (currentLine < totalLines) {

        // If the word is found at the end of line, there is no need to search it
        // again in the same line
        if (currentLine != searchStartLine || wordEnd < lineEnd) {
            const wchar_t *foundWord;
            if (FindNextWord(searchStart, lineEnd, wordBegin, wordEnd, &foundWord)) {
                LogFoundWord(lineBegin, lineEnd, foundWord);
                *foundPosition = foundWord - lineBegin;
                *foundLine     = currentLine;
                return true;
            }
        }
        currentLine++;

        if (!getLine(currentLine, &lineBegin, &lineEnd)) return false;
        searchStart = lineBegin;

        if (!uiUpdate(currentLine, totalLines)) return false;
    }

    return false;
}

template <typename GetLineFn, typename UIUpdateFn>
bool FindWordAbove(
    int searchStartLine,
    int totalLines,
    const wchar_t *lineBegin,
    const wchar_t *lineEnd,
    const wchar_t *wordBegin,
    const wchar_t *wordEnd,
    int *foundPosition,
    int *foundLine,
    GetLineFn getLine,
    UIUpdateFn uiUpdate) {

    int currentLine = searchStartLine;
    // The search starts before the original word on the same line.
    // Since the search is going backward, the end pointer defines the starting
    // point for the search. wordBegin - 1 is the delimiter right before the
    // current word; starting there (rather than at wordBegin itself) keeps
    // the current word's own first character out of the search, which
    // otherwise self-matched immediately for one-letter words.
    const wchar_t *searchEnd = wordBegin - 1;

    while (currentLine >= 0) {

        // If the word is found at the start of line, there is no need to search
        // it again in the same line
        if (currentLine != searchStartLine || wordBegin > lineBegin) {
            const wchar_t *foundWord;
            if (FindPreviousWord(lineBegin, searchEnd, wordBegin, wordEnd, &foundWord)) {
                LogFoundWord(lineBegin, lineEnd, foundWord);
                *foundPosition = foundWord - lineBegin;
                *foundLine     = currentLine;
                return true;
            }
        }
        currentLine--;
        if (currentLine < 0) break;

        if (!getLine(currentLine, &lineBegin, &lineEnd)) return false;
        searchEnd = lineEnd;

        if (!uiUpdate(totalLines - 1 - currentLine, totalLines)) return false;
    }

    return false;
}
