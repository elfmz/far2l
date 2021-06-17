#ifndef FAR_EDITOR_H
#define FAR_EDITOR_H

#include <cstdlib>
#include <utility>
#include <string>
#include <set>
#include <vector>

enum State {
    OFF, DO_PUT, DO_COLOR, DO_ACTION
};

class Editor {
private:
    // TODO: make this contants configurable
    const static int MAX_LINE_DELTA_TO_UPDATE_WORDS = 100;
    const static int MAX_LINE_LENGTH_TO_UPDATE_WORDS = 512;
    const static int MAX_WORD_LENGTH_TO_UPDATE_WORDS = 256;
    const static int MIN_WORD_LENGTH_TO_SUGGEST = 2;

    int id;
    PluginStartupInfo& info;
    FarStandardFunctions& fsf;

    struct {
        std::wstring prefix;
        std::set<std::wstring> current_line, other_lines;
        std::vector<const std::wstring *> prefixed; // contains pointers into sets above
    } words;

    std::wstring suggestion;
    size_t toggles = 0;
    int suggestionRow = 0;
    int suggestionCol = 0;
    State state = DO_PUT;
    bool isEnabled = false;

    EditorInfo previousEditorInfo = {0};
    int previousEditorInfoLineIndex = -1;

    void updateWords();
    void putSuggestion();

public:
    explicit Editor(int id, PluginStartupInfo& info, FarStandardFunctions& fsf, const std::wstring &autoEnableMasks);

    int getId();
    State getState();
    EditorInfo getInfo();

    bool getEnabled();
    void setEnabled(bool enabled);

    EditorGetString getString(int line = -1);
    int getSuggestionLength();

    void debug(const std::string& msg);

    void doHighlight(int row, int col, int size);
    void undoHighlight(int row, int col, int size);
    void processSuggestion();
    bool isSeparator(wchar_t c);

    void on();
    void toggleSuggestion();
    void confirmSuggestion();
    void declineSuggestion();

};

#endif //FAR_EDITOR_H
