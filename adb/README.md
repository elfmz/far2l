# ADB Plugin for far2l

Browse and manage files on Android devices over ADB.

## Features

- Auto-detect devices; pick from the list when more than one is connected
- F3/F5/F6/F7/F8 — view, copy, move, mkdir, delete (progress + Esc-abort)
- Unified destination parser for F5/F6 and Shift+F5/F6:
  - `adb:/abs/path` — on-device absolute
  - `/abs/path` — local FS (host pull/push)
  - `rel/path` — on-device relative
  - single leaf — rename intent
- Overwrite dialog — `Overwrite` / `Skip` / `Newer` (mtime gate) / `Rename` (auto `name(2)`) / `Cancel`; `Remember choice` checkbox makes the action sticky for the batch; `New` / `Existing` rows show size + mtime and open the corresponding file in the viewer when clicked
- Same-device cross-panel: in-device `cp -a` / `mv` (no host roundtrip); host-mediated fallback if the device refuses
- Atomic-aside-rename overwrite — push failures leave the original intact
- Auto-mkdir of intermediate destination dirs
- Shell commands from the far2l command line; output to user screen (Ctrl+O)

## Build

From the far2l build directory:

```bash
cmake --build . --target adb
```

Output: `install/Plugins/adb/plug/adb.far-plug-wide` plus language and help files.

## Install

Copy `install/Plugins/adb/` into far2l's Plugins folder:

- macOS: `/Applications/far2l.app/Contents/MacOS/Plugins/`
- Linux (system): `/usr/lib/far2l/Plugins/`
- Linux (user): `~/.local/lib/far2l/Plugins/`

Restart far2l. The plugin appears in **F11 → ADB Plugin** and in **Alt+F1 / Alt+F2 → ADB**.

## Prerequisites

`adb` in `PATH`, or one of: `/opt/homebrew/bin`, `/usr/local/bin`, `/usr/bin`, `$ANDROID_HOME/platform-tools`, `$ANDROID_SDK_ROOT/platform-tools`, `~/Library/Android/sdk/platform-tools` (macOS), `~/Android/sdk/platform-tools` (Linux).

Install: `brew install android-platform-tools` / `apt install adb` / `dnf install android-tools` / `pacman -S android-tools`, or [SDK Platform-Tools](https://developer.android.com/tools/releases/platform-tools).

Enable USB debugging on the device, accept the RSA fingerprint, verify with `adb devices`.

## Usage

1. Open from the F11 menu or **Alt+F1 / Alt+F2 → ADB**.
2. One device — auto-connects; multiple — pick one and press Enter.
3. Navigate and copy/move/delete as usual.
4. Type shell commands in the command line:
   ```
   ls -la /sdcard
   ps -A | grep system
   getprop ro.build.version.release
   ```
   Output appears in far2l's user screen (Ctrl+O to revisit). A non-zero exit with no stdout is surfaced as `->[Exit code: N]`.

## Key bindings

| Key | Action |
|---|---|
| Enter | Enter directory / connect to device |
| `..` | Up; on device root → device selector; on selector → close plugin |
| F3 | View file (pulled to temp, chmod 0644) |
| F5 / F6 | Copy / Move (single dialog, parser-driven) |
| Shift+F5 | Duplicate (default `<name>.copy`) |
| Shift+F6 | Rename file under cursor |
| F7 | Make directory |
| F8 | Delete |
| Esc | Abort current transfer |
| F10 | Close plugin |
| Ctrl+\ | Go to root |
| Ctrl+R | Refresh panel |
| Ctrl+O | Show command output history |

## Localization

Dialog button labels are currently hardcoded in English; `adbEng.lng` / `adbRus.lng` are placeholders for a future `GetMsg()` pass. Help files (`.hlf`) are localized — pressing **F1** in the plugin shows Russian help when far2l runs with the Russian language.

## Notes

- All commands typed on an ADB panel are forwarded to the device shell — no host fallback.
- Interactive tools (`vi`, `less`, `top`) are not supported (non-PTY session).
- Per-command timeout: 30 s. Long commands block subsequent plugin ops.
- External device-side changes are not watched — Ctrl+R to refresh.
- Sort mode resets on plugin close (standard far2l behavior).

## Troubleshooting

| Symptom | Check |
|---|---|
| No devices found | `adb devices`; USB debugging enabled; RSA fingerprint accepted |
| Permission denied | Some paths require root — try `adb root` (dev images only) |
| Slow transfers | Prefer USB 3.0 over Wi-Fi; many small files transfer slower than few large |
| `adb not found` | Install platform-tools and verify with `adb version` |
| Plugin hangs | 30 s timeout will release; otherwise reopen the plugin |

## Debug logging

Debug builds (`-DCMAKE_BUILD_TYPE=Debug`) write `adb_plugin.log` next to the plugin. Release builds compile out `DBG()` entirely.

## License

Part of [far2l](https://github.com/elfmz/far2l). GPLv2.
