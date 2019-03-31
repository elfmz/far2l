#include <plugin.hpp>
#include <utils.h>
#include "Editor.h"

Editor::Editor(int id, PluginStartupInfo &info, FarStandardFunctions &fsf, const std::wstring &autoEnableMasks)
        : id(id), info(info), fsf(fsf) {

    if (!autoEnableMasks.empty()) {
        std::wstring fileName;
        size_t fileNameSize = info.EditorControl(ECTL_GETFILENAME, NULL);

        if (fileNameSize > 1) {
            fileName.resize(fileNameSize + 1);
            info.EditorControl(ECTL_GETFILENAME, &fileName[0]);
            fileName.resize(fileNameSize - 1);

        } else
            fileName.clear();

        for (std::wstring tmp = autoEnableMasks;;) {
            size_t i = tmp.rfind(';');
            if (i == std::wstring::npos) {
                i = 0;
            } else
                ++i;

            if (i < tmp.size() && MatchWildcardICE(fileName.c_str(), tmp.c_str() + i)) {
                isEnabled = true;
                break;
            }
            if (i <= 1)
                break;

            tmp.resize(i - 1);
        }
    }
}

int Editor::getId() {
    return this->id;
}

EditorInfo Editor::getInfo() {
    EditorInfo ei = {0};
    info.EditorControl(ECTL_GETINFO, &ei);
    return ei;
}

bool Editor::getEnabled()
{
	return isEnabled;
}

void Editor::setEnabled(bool enabled)
{
    if (!enabled && isEnabled)
        declineSuggestion();

    isEnabled = enabled;
}

EditorGetString Editor::getString(int line) {
    EditorGetString es = {0};
    es.StringNumber = line;
    info.EditorControl(ECTL_GETSTRING, &es);
    return es;
}

static const std::string current_time() {
    time_t now = time(nullptr);
    char buf[80];
    struct tm tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
    return std::string(buf);
}

void Editor::debug(const std::string &msg) {
    FILE *f = fopen("/tmp/Editor.log", "a");
    if (f) {
        fprintf(f, "%s: %s\n", current_time().c_str(), msg.c_str());
        fclose(f);
    }
}

void Editor::updateWords() {
    const EditorInfo ei = getInfo();
    const EditorGetString &currentLine = getString(ei.CurLine);
    std::wstring currentEditorInfoLine = currentLine.StringText;

    // Nothing changed?
    if (ei.TotalLines == previousEditorInfo.TotalLines
        && ei.CurLine == previousEditorInfo.CurLine
        && currentLine.StringText == previousEditorInfoLine) {
        previousEditorInfo = ei;
        previousEditorInfoLine = currentEditorInfoLine;
        return;
    }

    // Nothing changed (just cursor moved)?
    if (ei.TotalLines == previousEditorInfo.TotalLines
        && std::abs(ei.CurLine - previousEditorInfo.CurLine) == 1 && !previousEditorInfoLine.empty()) {
        const EditorGetString &previousLine = getString(previousEditorInfo.CurLine);
        if (previousLine.StringText == previousEditorInfoLine) {
            previousEditorInfo = ei;
            previousEditorInfoLine = currentEditorInfoLine;
#if defined(DEBUG_EDITORCOMP)
            debug("Nothing changed (just cursor moved)");
#endif
            return;
        }
    }

    // Minor change in the current line?
    if (ei.TotalLines == previousEditorInfo.TotalLines && ei.CurLine == previousEditorInfoLineIndex
        && ei.CurLine == previousEditorInfo.CurLine && std::abs(ei.CurPos - previousEditorInfo.CurPos) <= 1) {
        int length = std::min(int(previousEditorInfoLine.length()), currentLine.StringLength);
        int commonPrefixLength = 0;
        for (int i = 0; i < length; i++)
            if (previousEditorInfoLine[i] != currentLine.StringText[i]) {
                commonPrefixLength = i;
                break;
            }
        bool minorChange = length == int(previousEditorInfoLine.length());
        if (!minorChange) {
            int commonSuffixLength = 0;
            for (int i = 0; i < length; i++)
                if (previousEditorInfoLine[previousEditorInfoLine.length() - i - 1] !=
                    currentLine.StringText[currentLine.StringLength - i - 1]) {
                    commonSuffixLength = i;
                    break;
                }
            if (std::abs(commonPrefixLength + commonSuffixLength - int(previousEditorInfoLine.length())) <= 1)
                minorChange = true;
        }
        if (minorChange) {
            std::wstring cur;
            words_current_line.clear();
            for (int j = 0; j < currentLine.StringLength; j++)
                if (isSeparator(currentLine.StringText[j])) {
                    if (!cur.empty()) {
                        if (cur.length() >= 3 && cur.length() <= MAX_WORD_LENGTH_TO_UPDATE_WORDS)
                            words_current_line.insert(cur);
                        cur = L"";
                    }
                } else
                    cur += currentLine.StringText[j];
            if (!cur.empty() && cur.length() >= 3 && cur.length() <= MAX_WORD_LENGTH_TO_UPDATE_WORDS)
                words_current_line.insert(cur);
            previousEditorInfo = ei;
            previousEditorInfoLine = currentEditorInfoLine;
#if defined(DEBUG_EDITORCOMP)
            debug("Minor change in the current line");
#endif
            return;
        }
    }

    // Rebuild words.
    words_wo_current_line.clear();
    words_current_line.clear();
    std::set<std::wstring> currentEditorInfoLineWords;
    for (int i = 0; i < ei.TotalLines; i++) {
        if (std::abs(i - ei.CurLine) <= MAX_LINE_DELTA_TO_UPDATE_WORDS) {
            const EditorGetString &string = getString(i);
            if (i == ei.CurLine) {
                currentEditorInfoLine = string.StringText;
            }
            const wchar_t *line = string.StringText;
            int length = string.StringLength;
            if (length <= MAX_LINE_LENGTH_TO_UPDATE_WORDS) {
                std::wstring cur;
                for (int j = 0; j < length; j++)
                    if (isSeparator(line[j])) {
                        if (!cur.empty()) {
                            if (cur.length() >= 3 && cur.length() <= MAX_WORD_LENGTH_TO_UPDATE_WORDS) {
                                if (i == ei.CurLine)
                                    words_current_line.insert(cur);
                                else
                                    words_wo_current_line.insert(cur);
                                if (i == ei.CurLine)
                                    currentEditorInfoLineWords.insert(cur);
                            }
                            cur = L"";
                        }
                    } else
                        cur += line[j];
                if (!cur.empty() && cur.length() >= 3 && cur.length() <= MAX_WORD_LENGTH_TO_UPDATE_WORDS) {
                    if (i == ei.CurLine)
                        words_current_line.insert(cur);
                    else
                        words_wo_current_line.insert(cur);
                    if (i == ei.CurLine)
                        currentEditorInfoLineWords.insert(cur);
                }
            }
        }
    }

    previousEditorInfo = ei;
    previousEditorInfoLine = currentEditorInfoLine;
    previousEditorInfoLineIndex = ei.CurLine;

#if defined(DEBUG_EDITORCOMP)
    debug("Rebuild words");
#endif
}

void Editor::doHighlight(int row, int col, int size) {
    EditorColor ec = {0};
    ec.StringNumber = row;
    ec.StartPos = col;
    ec.EndPos = col + size - 1;
    ec.Color = 0x2F;
    info.EditorControl(ECTL_ADDCOLOR, &ec);
}

void Editor::undoHighlight(int row, int col, int size) {
    EditorColor ec = {0};
    ec.StringNumber = row;
    ec.StartPos = col;
    ec.EndPos = col + size - 1;
    ec.Color = 0;
    info.EditorControl(ECTL_ADDCOLOR, &ec);
}

// Longest common prefix
static size_t lcp(const std::wstring &a, const std::wstring &b) {
    size_t length = std::min(a.length(), b.length());
    for (size_t i = 0; i < length; i++) {
        if (a[i] != b[i]) {
            return i;
        }
    }
    return length;
}

static std::wstring
upper_bound(const std::set<std::wstring> &a, const std::set<std::wstring> &b, const std::wstring &s) {
    auto i = a.upper_bound(s);
    auto j = b.upper_bound(s);
    if (i == a.end() && j == b.end())
        return L"";
    if (i == a.end())
        return *j;
    if (j == b.end())
        return *i;
    return std::min(*i, *j);
}

void Editor::putSuggestion() {
    EditorInfo ei = getInfo();
    EditorGetString egs = getString(ei.CurLine);

    if (egs.SelStart == -1 && (ei.CurPos >= egs.StringLength || isSeparator(egs.StringText[ei.CurPos]))
        && ei.CurPos > 0 && ei.CurPos < egs.StringLength + 1 && !isSeparator(egs.StringText[ei.CurPos - 1])) {
        int from = ei.CurPos - 1;
        while (from >= 0 && !isSeparator(egs.StringText[from]))
            from--;
        from++;

        if (ei.CurPos - from >= 2) {
            std::wstring prefix(egs.StringText + from, egs.StringText + ei.CurPos);

            auto i = upper_bound(words_current_line, words_wo_current_line, prefix);
            if (!i.empty() && lcp(prefix, i) == prefix.length()) {
                auto fi = i;
                size_t cp = i.length();

                while (true) {
                    auto pi = i;
                    i = upper_bound(words_current_line, words_wo_current_line, pi);
                    if (i.empty()) {
                        break;
                    }

                    size_t ccp = lcp(pi, i);
                    if (ccp < prefix.length()) {
                        break;
                    }
                    cp = std::min(cp, ccp);
                }

                if (cp > prefix.length()) {
                    this->suggestion = fi.substr(prefix.length(), cp - prefix.length());
                    this->suggestionRow = ei.CurLine;
                    this->suggestionCol = ei.CurPos;
                    info.EditorControl(ECTL_INSERTTEXT, (void *) suggestion.c_str());

#if defined(DEBUG_EDITORCOMP)
                    debug("Added suggestion of length " + std::to_string(suggestion.length()) +
                          " at the position ("
                          + std::to_string(ei.CurLine) + ", " + std::to_string(ei.CurPos) + ")");
#endif
                    state = DO_COLOR;
                    info.EditorControl(ECTL_REDRAW, nullptr);
                }
            }
        }
    }
}

void Editor::processSuggestion() {
    if (state == DO_PUT) {
        putSuggestion();
    } else if (state == DO_COLOR) {
        doHighlight(suggestionRow, suggestionCol, static_cast<int>(suggestion.length()));

#if defined(DEBUG_EDITORCOMP)
        debug("Highlighted suggestion of length " + std::to_string(suggestion.length()) +
              " at the position ("
              + std::to_string(suggestionRow) + ", " + std::to_string(suggestionCol) + ")");
#endif
        state = DO_ACTION;
        info.EditorControl(ECTL_REDRAW, nullptr);
    } else if (state == DO_ACTION) {
        doHighlight(suggestionRow, suggestionCol, static_cast<int>(suggestion.length()));
    }
}

State Editor::getState() {
    return state;
}

void Editor::confirmSuggestion() {
    if (state == DO_ACTION) {
        undoHighlight(suggestionRow, suggestionCol, static_cast<int>(suggestion.length()));

        EditorSetPosition esp = {0};
        esp.CurTabPos = -1;
        esp.TopScreenLine = -1;
        esp.LeftPos = -1;
        esp.Overtype = -1;
        esp.CurLine = suggestionRow;
        esp.CurPos = suggestionCol + int(suggestion.length());
        info.EditorControl(ECTL_SETPOSITION, &esp);

#if defined(DEBUG_EDITORCOMP)
        debug("Confirmed suggestion of length " + std::to_string(suggestion.length()) + "");
#endif
        suggestion = L"";
        suggestionRow = 0;
        suggestionCol = 0;

        state = DO_PUT;
        info.EditorControl(ECTL_REDRAW, nullptr);
    }
}

void Editor::declineSuggestion() {
    if (state == DO_ACTION) {
        undoHighlight(suggestionRow, suggestionCol, static_cast<int>(suggestion.length()));

        const EditorGetString &string = getString(suggestionRow);
        if (string.SelStart >= 0) {
            EditorSelect es = {0};
            es.BlockType = BTYPE_NONE;
            info.EditorControl(ECTL_SELECT, &es);
        }

        const EditorInfo &editorInfo = getInfo();
        if (editorInfo.CurLine == suggestionRow && editorInfo.CurPos == suggestionCol) {
            for (int i = 0; i < int(suggestion.length()); i++)
                info.EditorControl(ECTL_DELETECHAR, nullptr);
        } else {
            int beforeRow = editorInfo.CurLine;
            int beforeCol = editorInfo.CurPos;

            int delta = 0;
            if (beforeRow == suggestionRow && beforeCol >= suggestionCol) {
                delta = std::min(beforeCol - suggestionCol, int(suggestion.length()));
            }

            EditorSetPosition esp = {0};
            esp.CurTabPos = -1;
            esp.TopScreenLine = -1;
            esp.LeftPos = -1;
            esp.Overtype = -1;
            esp.CurLine = suggestionRow;
            esp.CurPos = suggestionCol;
            info.EditorControl(ECTL_SETPOSITION, &esp);

            for (int i = 0; i < int(suggestion.length()); i++)
                info.EditorControl(ECTL_DELETECHAR, nullptr);

            esp.CurLine = beforeRow;
            esp.CurPos = beforeCol - delta;
            info.EditorControl(ECTL_SETPOSITION, &esp);
        }

#if defined(DEBUG_EDITORCOMP)
        debug("Declined suggestion of length " + std::to_string(suggestion.length()) + "");
#endif
        suggestion = L"";
        suggestionRow = 0;
        suggestionCol = 0;

        state = OFF;
        info.EditorControl(ECTL_REDRAW, nullptr);
    }
}

void Editor::on() {
    if (state == OFF) {
#if defined(DEBUG_EDITORCOMP)
        debug("Editor::on");
#endif
        state = DO_PUT;
        info.EditorControl(ECTL_REDRAW, nullptr);
    }
}

bool Editor::isSeparator(wchar_t c) {
    return !(fsf.LIsAlphanum(c) || c == '_' || c == '-');
}

int Editor::getSuggestionLength() {
    return int(suggestion.length());
}
