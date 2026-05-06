# far2l MTP plugin — version 1.0

Browse and manage files on MTP-class devices (Android, cameras, media players) over USB. Backed by a vendored static [libmtp](https://libmtp.sourceforge.io/) 1.1.23 in `mtp/libmtp/` — only system dependency is libusb-1.0.

## Features

- Auto-detect connected MTP devices; enumerate manufacturer, product, serial
- Two-level navigation: device → storage → folders/files
- F3/F5/F6/F7/F8 — view, copy, move, mkdir, delete (progress + Esc-abort)
- Shared progress dialog with byte and total bars; debounced repaints
- Overwrite dialog — `Overwrite` / `Skip` / `Newer` (mtime gate) / `Rename` (auto `name(2)`) / `Cancel`; `Remember choice` checkbox makes the action sticky for the batch; `New` / `Existing` rows show size + mtime and open the corresponding file in the viewer when clicked
- Cross-panel F5/F6 between two MTP panels on the same device uses libmtp's `Copy_Object` / `Move_Object` directly — no host roundtrip; transparent fallback to host-mediated `download → upload` if the responder rejects
- Unified destination prompt for Shift+F5 / Shift+F6 / cross-panel F5 / F6 — accepts a basename, relative path (`./sub/`, `../`), or storage-relative path (`Internal/Temp/foo`)
- Capability-driven (no vendor/product tables): plugin queries `Check_Capability` + `Get_Supported_Filetypes` per session and routes uploads to a filetype the device accepts (Sony Xperia rejects `Undefined`; the plugin picks a neutral advertised type instead)
- Galaxy "blank-filename for new objects" quirk patched via a session-wide `{handle: name}` cache

## Build

From the far2l build directory:

```bash
cmake --build . --target mtp
```

Output: `install/Plugins/mtp/plug/mtp.far-plug-wide` plus language and help files.

Disable with `-DMTP=NO` at CMake-configure time.

## Install

Copy `install/Plugins/mtp/` into far2l's Plugins folder:

- macOS: `/Applications/far2l.app/Contents/MacOS/Plugins/`
- Linux (system): `/usr/lib/far2l/Plugins/`
- Linux (user): `~/.local/lib/far2l/Plugins/`

Restart far2l. The plugin appears in **Alt+F1 / Alt+F2 → MTP**.

## Prerequisites

### Linux

```bash
sudo apt install libusb-1.0-0-dev                 # Debian/Ubuntu
sudo dnf install libusb1-devel                    # Fedora
sudo pacman -S libusb                             # Arch
```

User must be in the `plugdev` group (or equivalent) for udev to grant non-root USB access. If `libmtp` is also installed system-wide, its udev rules cover MTP/PTP device permissions; if not, copy a ruleset from upstream libmtp's `udev-rules/` (vendored under `mtp/libmtp/` in this repo).

### macOS

```bash
brew install libusb
```

The system `icdd` (Image Capture daemon) and `ptpcamerad` claim MTP-class devices on plug-in with `kIOUSBExclusiveAccess`, blocking libmtp's open. The plugin works around this automatically: it writes the per-device "Connecting this device opens: No application" preference, then SIGKILLs `icdd` / `mscamerad-xpc` / `ptpcamerad` so launchd's respawn reads the new preference and skips the claim. No GUI step or `sudo` required.

Set `MTP_NO_ICDD_BYPASS=1` to disable the bypass (useful for diagnosing whether icdd is the cause of an open failure).

### Device

Connect via USB, unlock the device, switch USB mode to **File transfer (MTP)**. Some devices show an "Allow file transfer" prompt the first time — accept it. Until accepted, listings may be empty and uploads may fail with `0x2002`; the plugin's `Get_Storage` retry loop covers the few seconds before the user taps.

## Usage

1. Open from **Alt+F1 / Alt+F2 → MTP**.
2. Pick a device, press Enter — opens the storage list (single-storage devices skip this and jump straight to the file view).
3. Navigate folders, copy/move/delete with standard keys.
4. **Shift+F5** in-place copy: prompts for a new name with the full storage path prefilled; multi-select auto-suffixes `.copy`.
5. **Shift+F6** rename: same prompt; type `../` or `./sub/` to also move.
6. **Cross-panel F5/F6** between two MTP panels on the same device uses `Copy_Object` / `Move_Object` directly — no host roundtrip.
7. **Drives menu** (Alt+F1 / Alt+F2 → "Other panel" entry) — when one panel is already on MTP, picking the suggested `{/Storage/Path}` on the other panel adopts the same device automatically and navigates to that path; no need to re-pick the device.

## Key bindings

| Key | Action |
|---|---|
| Enter | Enter directory / connect to device |
| `..` | Up; on storage root → device selector; on selector → close plugin |
| F3 | View file (downloaded to temp, chmod 0644) |
| F5 / F6 | Copy / Move (host↔device or same-device) |
| Shift+F5 | In-place copy with prompt (rename or relocate) |
| Shift+F6 | Rename / move with prompt |
| F7 | Make directory |
| F8 | Delete |
| Esc | Abort current transfer |
| F10 | Close plugin |

## Localization

All user-visible strings (panel titles, column headers, dialog titles, buttons, progress labels, error pop-ups, confirmations) go through `GetMsg()` and ship in English (`mtpEng.lng`) and Russian (`mtpRus.lng`); the active language follows far2l's UI language. Help files (`.hlf`) are also localized — pressing **F1** in the plugin shows Russian help under Russian far2l. To add a language: copy `mtpEng.lng` to `mtp<XX>.lng`, translate the strings, keep the line order; same for the `.hlf`.

## Diagnostics

Plugin-side logs (`mtp.log`) and libmtp wire traces (`mtp_libmtp.log`) land next to the plugin binary. In Debug builds (`-DCMAKE_BUILD_TYPE=Debug`) both files are written by default with USB+PTP traces. In Release builds nothing is logged — set `MTP_LIBMTP_DEBUG=usb,ptp` (or `all`, `data`, etc.) before launching far2l to enable.
