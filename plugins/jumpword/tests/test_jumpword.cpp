#include <cstdio>
#include <cwctype>
#include <string>
#include <vector>

bool isIdChar(const wchar_t c) {
    return c == L'_' || iswalnum(c);
}

#include "../src/JumpWordCore.hpp"

// ---- mocked multi-line editor content ----
std::vector<std::wstring> g_lines;

bool GetLine(int currentLine, const wchar_t **lineBegin, const wchar_t **lineEnd) {
    if (currentLine < 0 || currentLine >= (int)g_lines.size()) return false;
    *lineBegin = g_lines[currentLine].c_str();
    *lineEnd   = *lineBegin + g_lines[currentLine].size();
    return true;
}

bool NoOpUIUpdate(int /*currentLine*/, int /*totalLines*/) {
    return true;
}

// ---- test harness ----
static int g_failures = 0;

void Check(bool condition, const char *name, const std::string &detail) {
    if (condition) {
        printf("PASS: %s\n", name);
    } else {
        printf("FAIL: %s -- %s\n", name, detail.c_str());
        g_failures++;
    }
}

int main() {
    // Down-search must not match inside a longer word, e.g. "a" inside "ab".
    {
        std::wstring line          = L"a ab";
        const wchar_t *wordBegin   = &line[0];
        const wchar_t *wordEnd     = &line[1];
        const wchar_t *searchStart = wordEnd + 1;
        const wchar_t *result      = nullptr;
        bool found = FindNextWord(searchStart, &line[0] + line.size(), wordBegin, wordEnd, &result);
        Check(
            !found,
            "Partial match down",
            found ? ("matched at offset " + std::to_string(result - &line[0])) : "");
    }

    // Up-search must not match inside a longer word either, e.g. "at" must not match the tail of
    // "cat".
    {
        std::wstring line        = L"cat at";
        const wchar_t *wordBegin = &line[4];
        const wchar_t *wordEnd   = &line[6]; // word "at" (the standalone occurrence)
        const wchar_t *result    = nullptr;
        bool found = FindPreviousWord(&line[0], &line[3], wordBegin, wordEnd, &result);
        Check(
            !found,
            "Partial match up",
            found ? ("matched at offset " + std::to_string(result - &line[0])) : "");
    }

    // Up-search must not self-match the current one-letter word.
    {
        g_lines = {L"a x a"};
        const wchar_t *lineBegin, *lineEnd;
        GetLine(0, &lineBegin, &lineEnd);
        const wchar_t *wordBegin = lineBegin + 4, *wordEnd = lineBegin + 5; // "a" at the end
        int pos = -1, line = -1;
        bool found = FindWordAbove(
            0,
            1,
            lineBegin,
            lineEnd,
            wordBegin,
            wordEnd,
            &pos,
            &line,
            GetLine,
            NoOpUIUpdate);
        Check(
            found && line == 0 && pos == 0,
            "One-letter word up on the same line",
            "found=" + std::to_string(found) + " line=" + std::to_string(line) +
                " pos=" + std::to_string(pos) + " (expected line=0 pos=0)");
    }

    // Up-search self-matches when a preceding char exists on the same line as a one-letter word,
    // instead of continuing onto earlier lines.
    {
        g_lines = {L"a", L"x", L"z a"};
        const wchar_t *lineBegin, *lineEnd;
        GetLine(2, &lineBegin, &lineEnd);
        const wchar_t *wordBegin = lineBegin + 2, *wordEnd = lineBegin + 3; // "a" in "z a"
        int pos = -1, line = -1;
        bool found = FindWordAbove(
            2,
            3,
            lineBegin,
            lineEnd,
            wordBegin,
            wordEnd,
            &pos,
            &line,
            GetLine,
            NoOpUIUpdate);
        Check(
            found && line == 0 && pos == 0,
            "One-letter word up another line",
            "found=" + std::to_string(found) + " line=" + std::to_string(line) +
                " pos=" + std::to_string(pos) + " (expected line=0 pos=0)");
    }

    // Down-search crossing lines works when there's no bug1 interference.
    {
        g_lines = {L"foo", L"", L"bar", L"foo"};
        const wchar_t *lineBegin, *lineEnd;
        GetLine(0, &lineBegin, &lineEnd);
        const wchar_t *wordBegin = lineBegin, *wordEnd = lineBegin + 3;
        int pos = -1, line = -1;
        bool found = FindWordBelow(
            0,
            4,
            lineBegin,
            lineEnd,
            wordBegin,
            wordEnd,
            &pos,
            &line,
            GetLine,
            NoOpUIUpdate);
        Check(
            found && line == 3 && pos == 0,
            "Multi-letter word down another line",
            "found=" + std::to_string(found) + " line=" + std::to_string(line) +
                " pos=" + std::to_string(pos));
    }

    // Up-search must stop cleanly at the top of the buffer instead of calling GetLine(-1, ...)
    {
        g_lines = {L"a b"};
        const wchar_t *lineBegin, *lineEnd;
        GetLine(0, &lineBegin, &lineEnd);
        const wchar_t *wordBegin = lineBegin + 2, *wordEnd = lineBegin + 3; // "b"
        int pos = -1, line = -1;
        bool found = FindWordAbove(
            0,
            1,
            lineBegin,
            lineEnd,
            wordBegin,
            wordEnd,
            &pos,
            &line,
            GetLine,
            NoOpUIUpdate);
        Check(!found, "Up-search stops at top of buffer without out-of-bounds GetLine", "found=" + std::to_string(found));
    }

    printf("\n%d test(s) failed.\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
