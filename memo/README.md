# Memo Plugin for Far2l

A multi-page scratchpad/memo plugin for Far2l.  
10 independent memo pages, always at your fingertips.

## Features

### 10 Independent Memo Pages
- 10 separate memo pages, switched via keyboard shortcuts `Ctrl+1`–`0` / `Alt+1`–`0` or by **clicking the `[1]`–`[0]` UI buttons with the mouse**.
- **Dynamic UI:** Dialog size adapts to your terminal, with a safe 75x20 minimum.
- **Robust Paths:** Full UTF-8 support for home directory and file exports.
- **Tabbed Interface:** Visually switch between your memos directly using the dialog buttons.

### Keyboard & UI Buttons

| Key / Button | Action |
|--------------|--------|
| `Ctrl+1`–`9`, `0` / `[1]`–`[0]` | Switch to page 1–9, 10 (can be clicked with mouse) |
| `Alt+1`–`9`, `0` | Same as Ctrl (alternative modifier) |
| `Esc` / `Ctrl+S` | Close and auto-save |
| `F2` / `[F2 Save]` | Export current page to an external file (invokable directly from Memo) |
| `F9` / `[F9 Config]` | Open plugin configuration dialog (invokable directly from Memo) |

### Auto-Save & State
- Content is saved automatically when switching pages or closing the dialog, **but overwrites the file only if there are real changes**.
- Last active page is remembered across sessions.

### Configuration
Available via `F11` → `Plugins` → `Configure` → `Memo` or by clicking the `[F9 Config]` button inside the memo editor.
- **Enable Memo Plugin:** Enable/disable the plugin functionality.
- **Use Ctrl+S to open:** Automatically registers `Ctrl+S` as a global hotkey in `settings/key_macros.ini`.

## Global Hotkey (Ctrl+S)

The plugin can automatically manage its own `Ctrl+S` binding.
- **Auto-Registration:** Enabling "Use Ctrl+S" in settings adds `callplugin(0x4D454D4F)` to your global macros.
- **Editor Safety:** The plugin may interact cleanly depending on the active Far dialog state, avoiding conflicts with normal editor contexts.

The plugin's SysID is `0x4D454D4F`.

## Storage

Default location: `~/.config/far2l/plugins/memo/`

| File | Purpose |
|------|---------|
| `memo-01.txt` … `memo-09.txt`, `memo-00.txt` | Page content (UTF-8) |
| `state.ini` | `LastMemo`, `Enabled`, `HotkeyEnabled` |
| `debug.log` | Debug build only (`DEBUG` builds only) |

## Plugin API

| Function | Purpose |
|----------|---------|
| `OpenPluginW` | Entry point to open the Memo editor dialog. |
| `ConfigureW` | Configuration dialog with hotkey and state management. |
| `SetStartupInfoW` | Initialization and automatic macro registration. |

## Building

```bash
cmake --build /path/to/far2l-adb --target memo
```

Output: `Plugins/memo/plug/memo.far-plug-wide`  
Language files and `key_macros.ini` template are copied automatically.

## License

Part of the far2l-adb project. Same license as far2l.
