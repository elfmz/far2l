#include <assert.h>
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

static inline bool IsPrefixed(const std::wstring &str, const std::wstring &prefix)
{
	return str.size() > prefix.size() && wmemcmp(str.c_str(), prefix.c_str(), prefix.size()) == 0;
}

void Editor::updateWords() {
    const EditorInfo ei = getInfo();

    // Change only in the current line?
    const bool only_current_line = (ei.CurLine == previousEditorInfo.CurLine
        && ei.TotalLines == previousEditorInfo.TotalLines);

    // Rebuild words.
    words.prefixed.clear();
    words.current_line.clear();
    if (!only_current_line) {
        words.other_lines.clear();
    }
    std::wstring cur;
    for (int i = 0; i < ei.TotalLines; i++) {
        if (i == ei.CurLine || (!only_current_line && std::abs(i - ei.CurLine) <= MAX_LINE_DELTA_TO_UPDATE_WORDS)) {
            const EditorGetString &string = getString(i);
            const int length = string.StringLength;
            if (length <= MAX_LINE_LENGTH_TO_UPDATE_WORDS) {
                const wchar_t *line = string.StringText;
                for (int j = 0, k = 0; j <= length; j++) {
                    if (j == length || isSeparator(line[j])) {
                        if ((size_t)(j - k) > MIN_WORD_LENGTH_TO_SUGGEST && j - k <= MAX_WORD_LENGTH_TO_UPDATE_WORDS) {
                            cur.assign(&line[k], j - k);
                            if (i == ei.CurLine)
                                words.current_line.insert(cur);
                            else
                                words.other_lines.insert(cur);
                        }
                        k = j + 1;
                    }
                }
            }
        }
    }


    words.prefixed.reserve(words.current_line.size() + words.other_lines.size());

    for (const auto &w : words.current_line) if (IsPrefixed(w, words.prefix)) {
        words.prefixed.emplace_back(&w);
    }
    for (const auto &w : words.other_lines) if (IsPrefixed(w, words.prefix)) {
        if (words.current_line.find(w) == words.current_line.end()) {
            words.prefixed.emplace_back(&w);
        }
    }

    previousEditorInfo = ei;

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

void Editor::putSuggestion() {
    EditorInfo ei = getInfo();
    EditorGetString egs = getString(ei.CurLine);

    if (egs.SelStart == -1 && (ei.CurPos >= egs.StringLength || isSeparator(egs.StringText[ei.CurPos]))
        && ei.CurPos > 0 && ei.CurPos < egs.StringLength + 1 && !isSeparator(egs.StringText[ei.CurPos - 1])) {
        int from = ei.CurPos - 1;
        while (from >= 0 && !isSeparator(egs.StringText[from]))
            from--;
        from++;

        if (ei.CurPos - from >= MIN_WORD_LENGTH_TO_SUGGEST) {
            std::wstring prefix(egs.StringText + from, egs.StringText + ei.CurPos);
            if (toggles == 0 || toggles >= words.prefixed.size() || prefix != words.prefix) {
                toggles = 0;
                words.prefix = prefix;
                updateWords();
            }
            if (toggles < words.prefixed.size()) {
                const auto &fi = *words.prefixed[toggles];
                assert(fi.length() > prefix.length());
                this->suggestion = fi.substr(prefix.length(), fi.size() - prefix.length());
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

void Editor::toggleSuggestion()
{
    auto saved_toggles = toggles;
    declineSuggestion();
    state = DO_PUT;
    toggles = saved_toggles + 1;
    info.EditorControl(ECTL_REDRAW, nullptr);
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
        toggles = 0;
        suggestion.clear();
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
        toggles = 0;
        suggestion.clear();
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
        toggles = 0;
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
