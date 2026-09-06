#ifndef FAR_EDITOR_H
#define FAR_EDITOR_H

#include <cstdlib>
#include <utility>
#include <string>
#include <set>
#include <vector>
#include "Settings.h"

enum State {
    OFF, DO_PUT, DO_COLOR, DO_ACTION
};

class Editor {
private:

    int id;
    PluginStartupInfo& info;
    FarStandardFunctions& fsf;
    Settings *settings;

    struct {
        std::wstring prefix;
        std::set<std::wstring> current_line, other_lines;
        std::vector<const wchar_t *> suggestions; // contains pointers into sets above
    } words;

    int suggestionRow = 0;
    int suggestionCol = 0;
    int suggestionLen = 0;
    State state = OFF;
    bool isEnabled = false;

    EditorInfo previousEditorInfo = {0};

    void updateWords();
    void putSuggestion();

public:
    explicit Editor(int id, PluginStartupInfo& info, FarStandardFunctions& fsf, Settings *settings);

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
