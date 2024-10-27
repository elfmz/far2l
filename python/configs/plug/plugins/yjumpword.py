
"""
This plugins allows to quickly jump back and forth between
alpha-numeric words (identifiers). Useful when bound to
hotkeys like Ctrl+Alt+Up / Ctrl+Alt+Down.

Also allows to show all lines with word under cursor.

This is a simple editor plugin which servers and example of
how to make editor plugins using yfar library.
"""

__author__ = 'Yaroslav Yanovsky'

from yfar import FarPlugin


class Plugin(FarPlugin):
    label = 'Jump Between Words in Editor'
    openFrom = ['PLUGINSMENU', 'EDITOR']

    def OpenPlugin(self, _):
        editor = self.get_editor()
        x, y = editor.cursor
        line = editor[y]

        # check if there is a word under cursor
        if x > len(line):
            return

        # extract word
        is_word_char = lambda ch: ch == '_' or ch.isalnum()
        x2 = x
        while x > 0 and is_word_char(line[x - 1]):
            x -= 1
        while x2 < len(line) and is_word_char(line[x2]):
            x2 += 1
        word = line[x:x2]
        if not word:
            return

        def look_back(s, i):
            while i >= 0:
                i = s.rfind(word, 0, i)
                if i != -1:
                    if i > 0 and is_word_char(s[i - 1]):
                        i -= 1
                    else:
                        j = i + len(word)
                        if j < len(s) and is_word_char(s[j]):
                            i -= 1
                        else:
                            return i
            return -1

        def look_forward(s, i):
            while i >= 0:
                i = s.find(word, i)
                if i != -1:
                    if i > 0 and is_word_char(s[i - 1]):
                        i += 1
                    else:
                        j = i + len(word)
                        if j < len(s) and is_word_char(s[j]):
                            i += 1
                        else:
                            return i
            return -1

        option = self.menu(('Jump to &Previous Word', 'Jump to &Next Word',
                            'Display &All lines with Word'),
                           'Jump between words "{}"'.format(word))

        if option == 0:  # move up
            while y >= 0:
                x = look_back(line, x)
                if x == -1:  # not found - search previous line
                    y -= 1
                    if y < 0:
                        break
                    line = editor[y]
                    x = len(line)
                else:
                    editor.focus_cursor(x, y)
                    break

        elif option == 1:  # move down
            total = len(editor)
            x = x2
            while y < total:
                x = look_forward(line, x)
                if x == -1:  # not found - search next line
                    y += 1
                    if y >= total:
                        break
                    line = editor[y]
                    x = 0
                else:
                    editor.focus_cursor(x, y)
                    break

        elif option == 2:  # show all lines with such word
            results = []
            current = 0
            for yi, line in enumerate(editor):
                xi = look_forward(line, 0)
                if xi != -1:
                    if yi == y:
                        current = len(results)
                    results.append((xi, yi, line))
            lno_width = len(str(results[-1][0]))
            fmt = ('{0[1]: >' + str(lno_width) + 'd}:& {0[2]}').format
            i = self.menu([fmt(r) for r in results], word, selected=current)
            if i != -1:
                editor.focus_cursor(results[i][0], results[i][1])
                    
                                