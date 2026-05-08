# ADB Plugin for far2l

Browse and manage files on Android devices over ADB from far2l.

## Features

- Auto-detect connected devices; enumerate serial, friendly name, model, USB port
- Navigate the device filesystem with standard far2l panel operations
- F3/F5/F6/F7/F8 — view, copy, move, mkdir, delete, with progress and abort
- On-device copy/move when both panels point to the same device (no host roundtrip)
- Shell commands from the far2l command line — each command gets its stdout, stderr,
  and exit code; device `cwd` is synchronized via persistent `adb shell -T`
- Ctrl+O shows command output history (far2l's user screen)

## Build

Part of the far2l tree:

```bash
# from the far2l build directory
cmake --build . --target adb
```

Output: `install/Plugins/adb/plug/adb.far-plug-wide` + language/help files.

## Install

Copy `install/Plugins/adb/` into far2l's Plugins folder:

- macOS: `/Applications/far2l.app/Contents/MacOS/Plugins/`
- Linux (system): `/usr/lib/far2l/Plugins/`
- Linux (user): `~/.local/lib/far2l/Plugins/`

Restart far2l. The plugin appears in **F11 → ADB Plugin** and in the drives menu
**Alt+F1 / Alt+F2 → ADB**.

## Prerequisites

The `adb` binary must be installed and in one of:

1. `PATH`
2. `/opt/homebrew/bin/adb`, `/usr/local/bin/adb`, `/usr/bin/adb`
3. `$ANDROID_HOME/platform-tools/adb`, `$ANDROID_SDK_ROOT/platform-tools/adb`
4. `~/Library/Android/sdk/platform-tools/adb` (macOS)
5. `~/Android/sdk/platform-tools/adb` (Linux)

Install via: `brew install android-platform-tools` (macOS) /
`apt install adb` (Debian) / `dnf install android-tools` (Fedora) /
`pacman -S android-tools` (Arch), or download from
[SDK Platform-Tools](https://developer.android.com/tools/releases/platform-tools).

Enable USB debugging on the device (Settings → Developer options → USB debugging),
connect via USB, accept the RSA fingerprint. Verify with `adb devices`.

## Usage

1. Open from F11 menu or Alt+F1/F2 → ADB
2. If one device — auto-connects; if multiple — pick one and press Enter
3. Navigate and copy/move/delete as usual
4. Type shell commands in the command line:
   ```
   ls -la /sdcard
   ps -A | grep system
   getprop ro.build.version.release
   ```
   Output appears in far2l's user screen (Ctrl+O to revisit). Non-zero exit
   with no output is surfaced as `->[Exit code: N]`.

## Key bindings

| Key      | Action                                 |
|----------|----------------------------------------|
| Enter    | Enter directory / connect to device    |
| F3       | View file (pulled to a temp location)  |
| F5 / F6  | Copy / Move (ADB↔host or same-device)  |
| F7       | Make directory                         |
| F8       | Delete                                 |
| F10      | Close plugin                           |
| Ctrl+\\   | Go to `/`                              |

## Localization

UI strings are currently hardcoded in English. `adbEng.lng` / `adbRus.lng` are
shipped as placeholders for a future pass that wires up `GetMsg()` lookups;
help files (`.hlf`) are properly localized — `F1` in the plugin shows Russian
help when far2l runs with the Russian language.

## Notes & limitations

- **Transparency**: every command on an ADB panel is forwarded to the device shell
  (no host fallback). `git`, `vim`, etc. run on the device if installed there.
- **Interactive tools** (`vi`, `less`, `top`) don't work — the plugin uses a
  non-PTY `adb shell -T` session for deterministic marker-based IPC.
- **Command timeout** is 30 s. A long-running command (e.g., `find /`) will block
  subsequent plugin ops until it finishes or you close the plugin.
- **Command-line prompt lag**: after `cd`, the far2l prompt text shows the new
  path only on the next command (a far2l core issue — the panel title and command
  echo are updated immediately).
- **External changes** (another `adb` session modifying the device) are not
  watched; press **Ctrl+R** to refresh.
- **Sort mode** is remembered within a session; closing the plugin resets it
  (standard far2l plugin-panel behavior).

## Troubleshooting

| Symptom | Check |
|---------|-------|
| No devices found | `adb devices` in a terminal; USB debugging enabled; RSA prompt accepted |
| Permission denied | Some paths require root — try `adb root` (dev images only) |
| Slow transfers | Prefer USB 3.0 over Wi-Fi; many small files are slower than few big ones |
| `adb not found` | Install platform-tools and verify with `adb version` |
| Command hangs | 30 s timeout will clear it; if the plugin stays stuck, reopen it |

## License

Part of [far2l](https://github.com/elfmz/far2l). GPLv2.
