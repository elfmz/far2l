# Memo Plugin for Far2l v0.1

A multi-page scratchpad/memo plugin for Far2l file manager.

## Features

### 10 Independent Memo Pages
- 10 separate memo pages (Memo 1-10)
- Each memo stored in a separate file: `memo-00.txt` through `memo-09.txt`
- Full-featured multiline text editor (DI_MEMOEDIT)

### Keyboard Navigation
| Key | Action |
|-----|--------|
| `Ctrl+0` - `Ctrl+9` | Switch to memo 10, 1-9 |
| `Alt+0` - `Alt+9` | Switch to memo 10, 1-9 |
| `Esc` | Close dialog (auto-saves) |
| `F2` or `Shift+F2` | Save current memo to external file |

### Auto-Save
- Content automatically saved when:
  - Switching between memos
  - Closing the dialog (Esc or Enter)
  - Switching to another memo

### State Persistence
- Last selected memo is saved in `state.ini`
- Automatically restores the last used memo on next open

### Page Indicator
- Visual indicator at bottom of dialog showing current page
- Uses bullet character (•) with current page marked (e.g., `[•1] 2 3 4 5 6 7 8 9 0`)

### Save As
- Press `F2` or `Shift+F2` to save current memo to an external file
- Default path: `$HOME/memo-XX.txt`
- Uses Far2l's InputBox for path selection

## Installation

The plugin is built as a Far2l plugin module:
```
memo.far-plug-wide
```

### Default Storage Location
- Linux/macOS: `~/.config/far2l/plugins/memo/`
- Files created:
  - `memo-00.txt` ... `memo-09.txt` - memo content
  - `state.ini` - last selected memo

## Usage

### Opening the Plugin
- Via menu: `Plugins → Memo`
- Via command: `memo`

### Switching Between Memos
1. Use `Ctrl+0` through `Ctrl+9` (or `Alt+0` through `Alt+9`)
2. Current memo auto-saves before switching

### Editor Features
The memo editor supports all standard Far2l editor features:
- Multi-line text editing
- Standard editing keys (arrow keys, Home, End, etc.)
- Text selection (Shift+arrows)
- Word wrap (if enabled in Far2l settings)
- Undo/Redo

## Technical Details

### Plugin API
The plugin implements the following Far2l plugin interface functions:

| Function | Purpose |
|----------|---------|
| `SetStartupInfoW` | Initialize plugin with Far2l API |
| `GetPluginInfoW` | Register plugin with menu and command |
| `OpenPluginW` | Open the memo dialog |
| `ClosePluginW` | Cleanup on plugin close |

### Dialog Structure
The plugin creates a dialog with 3 items:
1. **DI_TITLE** (0): Dialog title showing current memo number
2. **DI_MEMOEDIT** (1): Main multiline text editor
3. **DI_INDICATOR** (2): Page indicator at bottom

### File Format
- UTF-8 encoded text files
- Each memo stored separately
- State file uses simple `key=value` format

## Building

### Requirements
- Far2l SDK headers
- CMake
- C++ compiler with C++11 support

### Build Commands
```bash
cd /path/to/far2l-adb
mkdir -p build && cd build
cmake ..
make memo
```

The built plugin will be located at:
```
install/far2l.app/Contents/MacOS/Plugins/memo/plug/memo.far-plug-wide
```

## Configuration

No external configuration required. All settings stored in:
- `state.ini` - last memo index

### Environment Variables
- `HOME` - used for default storage path (`~/.config/far2l/plugins/memo/`)

## Troubleshooting

### Indicator Not Visible
Ensure the dialog is tall enough. The indicator appears at the bottom of the dialog.

### Keys Not Working
- Ensure the memo editor has focus (cursor should be in the editor)
- Some terminal emulators may not send Alt+key combinations properly

### Files Not Saving
Check that the `~/.config/far2l/plugins/memo/` directory exists and is writable.

## License

This plugin is part of the Far2l project and follows the same license terms.
