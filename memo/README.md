# Memo Plugin for Far2l

A multi-page scratchpad/memo plugin for Far2l.  
10 independent memo pages, always at your fingertips.

## Features

### 10 Independent Memo Pages
- 10 separate memo pages, switched with `Ctrl+1`–`Ctrl+9`, `Ctrl+0` (or `Alt+…`)
- **Cursor Persistence:** Caret position is saved and restored per page.
- **Dynamic UI:** Dialog size adapts to your terminal, with a safe 60x15 minimum.
- **Robust Paths:** Full UTF-8 support for home directory and file exports.
- Visual page indicator at bottom: `• 1 • 2 •[3]• 4 • … • 0 •`

### Keyboard

| Key | Action |
|-----|--------|
| `Ctrl+1`–`Ctrl+9`, `Ctrl+0` | Switch to page 1–9, 10 |
| `Alt+1`–`Alt+9`, `Alt+0` | Same (alternative modifier) |
| `Esc` | Close and auto-save |
| `F2` / `Shift+F2` | Export current page to an external file |

### Auto-Save & State
- Content is saved automatically when switching pages or closing.
- Last active page and cursor positions are remembered across sessions.

### Configuration (F11 → Plugins → Configure → Memo)
- **Enable Memo Plugin:** Enable/disable the plugin functionality.
- **Use Ctrl+S to open:** Automatically registers `Ctrl+S` as a global hotkey in `settings/key_macros.ini`.

## Global Hotkey (Ctrl+S)

The plugin can automatically manage its own `Ctrl+S` binding.
- **Auto-Registration:** Enabling "Use Ctrl+S" in settings adds `callplugin(0x4D454D4F)` to your global macros.
- **Editor Safety:** The plugin silently ignores `Ctrl+S` when called from an active Editor to prevent a known conflict with the **Colorer** plugin (avoids Segmentation Faults).

The plugin's SysID is `0x4D454D4F`. Command line prefix: `memo:`.

## Storage

Default location: `~/.config/far2l/plugins/memo/`

| File | Purpose |
|------|---------|
| `memo-00.txt` … `memo-09.txt` | Page content (UTF-8) |
| `state.ini` | `LastMemo`, `Enabled`, `HotkeyEnabled` |
| `debug.log` | Debug build only (`NDEBUG` builds omit this) |

## Plugin API

| Function | Purpose |
|----------|---------|
| `OpenPluginW` | Entry point. Guards: enabled check, **Editor area check** (prevents crash). |
| `ConfigureW` | Configuration dialog with hotkey management. |
| `SetStartupInfoW` | Initialization and automatic macro registration. |

## Building

```bash
cmake --build /path/to/far2l-adb --target memo
```

Output: `Plugins/memo/plug/memo.far-plug-wide`  
Language files and `key_macros.ini` template are copied automatically.

## License

Part of the far2l-adb project. Same license as far2l.
