# Memo Plugin for Far2l

A multi-page scratchpad/memo plugin for Far2l file manager.  
10 independent memo pages, always at your fingertips.

## Features

### 10 Independent Memo Pages
- 10 separate memo pages, switched with `Ctrl+1`–`Ctrl+9`, `Ctrl+0` (or `Alt+…`)
- Each page stored as a separate UTF-8 file: `memo-00.txt` … `memo-09.txt`
- Full-featured multiline text editor (`DI_MEMOEDIT`)
- Title always shows the active page: `[ Memo - 3 ]`
- Visual page indicator at bottom: `• 1 • 2 •[3]• 4 • … • 0 •`

### Keyboard

| Key | Action |
|-----|--------|
| `Ctrl+1`–`Ctrl+9`, `Ctrl+0` | Switch to page 1–9, 10 |
| `Alt+1`–`Alt+9`, `Alt+0` | Same (alternative modifier) |
| `Esc` | Close and auto-save |
| `F2` / `Shift+F2` | Export current page to an external file |

### Auto-Save
Content is saved automatically when:
- Switching between pages
- Closing the dialog (`Esc`)

### State Persistence
Last active page is remembered in `state.ini` and restored on next open.

### Export (F2)
- Saves current page to any path you specify via an input box
- Default path: `~/memo-0N.txt`

### Configuration (F11 → Plugins → Configure → Memo)
- **Enable / disable** the plugin entirely (stored in `state.ini`)
- When disabled, `Ctrl+S` / `Cmd+S` is silently ignored

## Known Limitation — Ctrl+S / Cmd+S not available in F4 Editor and Standalone Mode

> **Why**: The memo dialog uses a `DI_MEMOEDIT` item internally backed by a Far2l `Editor`
> object. When that editor gains focus, Far2l broadcasts an `EE_GOTFOCUS` event to **all**
> loaded plugins. The **colorer** plugin intercepts this event and calls
> `ECTL_GETFILENAME` to determine the file's syntax. Because the dialog editor has no
> backing file, `ECTL_GETFILENAME` returns **`nullptr`**. Colorer then passes this null
> pointer directly to `UnicodeString` constructor inside `FarEditor::chooseFileType`,
> causing an immediate `SIGABRT` / `KERN_INVALID_ADDRESS` crash.
>
> **Workaround**: When `OpenPluginW` is called, the plugin performs two checks:
> 1. If a real editor is active (`EditorControl(ECTL_GETINFO)` returns `EditorID != 0`).
> 2. If file panels exist (`Control(FCTL_CHECKPANELSEXIST)` returns true).
>
> If an editor is already active or if panels are missing (standalone mode), the plugin
> silently skips opening the dialog. This prevents the crash until colorer is fixed
> to guard against a null filename.
>
> **Affected versions**: colorer as of far2l 2.7.0-beta (Feb 2026). The crash occurs in
> `FarEditorSet::addCurrentEditor` → `FarEditorSet::getCurrentFileName` →
> `FarEditor::chooseFileType(UnicodeString const*)`.

## Hotkey Setup

The plugin does not register hotkeys itself.  
Use `key_macros.ini` with `CallPlugin` to bind `Ctrl+S` / `Cmd+S`:

```ini
[Macros.Shell.CtrlS]
Sequence=callplugin(0x53637274)

[Macros.Viewer.CtrlS]
Sequence=callplugin(0x53637274)
```

The plugin's SysID is `0x53637274`.

## Storage

Default location: `~/.config/far2l/plugins/memo/`

| File | Purpose |
|------|---------|
| `memo-00.txt` … `memo-09.txt` | Page content (UTF-8) |
| `state.ini` | `LastMemo=N`, `Enabled=0/1` |
| `debug.log` | Debug build only — removed in Release (`NDEBUG`) |

## Language Files

Installed alongside the plugin in `Plugins/memo/plug/`:

| File | Language |
|------|----------|
| `memoe.lng` | English |
| `memor.lng` | Russian |

Far2l selects the language file automatically based on the current locale.

## Plugin API

| Function | Purpose |
|----------|---------|
| `SetStartupInfoW` | Initialize with Far2l API |
| `GetPluginInfoW` | Register plugin name, menu entries, SysID |
| `OpenPluginW` | Open the memo dialog (guards: enabled check, editor check) |
| `ClosePluginW` | No-op |
| `ConfigureW` | Show enable/disable configuration dialog |
| `ProcessEditorEventW` | No-op |
| `ProcessEventW` | No-op |

## Building

```bash
cmake --build /path/to/far2l-adb --target memo
```

Output: `install/far2l.app/Contents/MacOS/Plugins/memo/plug/memo.far-plug-wide`  
Language files are copied automatically as a CMake post-build step.

### Debug Build
When built **without** `-DNDEBUG`, the plugin writes detailed logs to  
`~/.config/far2l/plugins/memo/debug.log`.  
In Release builds the logging is completely omitted at compile time.

## License

Part of the far2l-adb project. Same license as far2l.
