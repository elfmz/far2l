# ADB Plugin for far2l

Browse and manage files on Android devices via ADB directly from far2l.

## Features

- Auto-detect and list connected ADB devices (serial, model, friendly name)
- Navigate the Android file system with standard far2l panel operations
- Copy/move files between device and host (F5/F6) with progress, speed, ETA
- On-device copy/move when both panels point to the same device (no host roundtrip)
- Delete files and directories (F8) with confirmation dialogs
- Create directories on device (F7)
- View files (F3) — pulls to temp and opens
- Execute shell commands from the far2l command line (with `adb:` prefix or directly)
- Working directory syncs between shell commands and the panel

## Prerequisites

The plugin requires the `adb` (Android Debug Bridge) command-line tool.

### macOS

```bash
# Homebrew (recommended)
brew install android-platform-tools

# Or install Android Studio — adb is in ~/Library/Android/sdk/platform-tools/
```

### Ubuntu / Debian

```bash
sudo apt install adb
```

### Fedora / RHEL

```bash
sudo dnf install android-tools
```

### Arch Linux

```bash
sudo pacman -S android-tools
```

### Manual install (any platform)

Download [SDK Platform-Tools](https://developer.android.com/tools/releases/platform-tools) from Google, extract, and add to `PATH`:

```bash
export PATH="$PATH:/path/to/platform-tools"
```

### Verify ADB is accessible

```bash
adb version
# Android Debug Bridge version 1.0.41
```

The plugin searches for `adb` in this order:
1. `adb` in `PATH`
2. `/opt/homebrew/bin/adb` (macOS Homebrew)
3. `/usr/local/bin/adb`, `/usr/bin/adb`
4. `$ANDROID_HOME/platform-tools/adb`
5. `$ANDROID_SDK_ROOT/platform-tools/adb`
6. `~/Library/Android/sdk/platform-tools/adb` (macOS)
7. `~/Android/sdk/platform-tools/adb` (Linux)

### Enable USB debugging on the device

1. Go to **Settings > About phone** and tap **Build number** 7 times
2. Go to **Settings > Developer options** and enable **USB debugging**
3. Connect the device via USB and accept the RSA key prompt
4. Verify: `adb devices` should list your device

## Building

Build as part of the far2l tree:

```bash
# From the far2l build directory
cmake --build . --target adb
```

Output: `Plugins/adb/plug/adb.far-plug-wide`
Language and help files are copied automatically.

The plugin requires the far2l source tree (headers and libraries) to compile.

## Installation

After building, the plugin is at:
```
<build-dir>/install/Plugins/adb/plug/
    adb.far-plug-wide    # plugin binary
    adbEng.hlf           # English help
    adbEng.lng           # English language
    adbRus.hlf           # Russian help
    adbRus.lng           # Russian language
```

Copy the entire `adb/` directory to your far2l plugins folder:

```bash
# macOS (app bundle)
cp -r install/Plugins/adb /Applications/far2l.app/Contents/MacOS/Plugins/

# Linux (system install)
cp -r install/Plugins/adb /usr/lib/far2l/Plugins/

# Linux (user install)
cp -r install/Plugins/adb ~/.local/lib/far2l/Plugins/
```

Restart far2l. The plugin appears in **F11 > Plugins > ADB Plugin**.

## Usage

1. Open the plugin from F11 menu or Alt+F1/Alt+F2 location menu
2. If one device is connected — connects automatically
3. If multiple devices — select from the list and press Enter
4. Navigate, copy, delete as with any far2l panel
5. Type shell commands in the command line (e.g. `ls -la /sdcard`)

## Panel columns

### Device list mode

| Column | Description |
|--------|-------------|
| Serial Number | Device serial or network address |
| Device Name | Friendly name from device settings |
| Model | Device model identifier |
| Port | USB port or connection type |

### File browser mode

| Column | Description |
|--------|-------------|
| Name | File or directory name |
| Size | File size in bytes |

## Keyboard shortcuts

| Key | Action |
|-----|--------|
| Enter | Enter directory / connect to device |
| F3 | View file |
| F5 | Copy |
| F6 | Move |
| F7 | Create directory |
| F8 | Delete |
| F10 | Close plugin |
| Ctrl+\\ | Go to root `/` |

## Troubleshooting

| Problem | Solution |
|---------|----------|
| No devices found | Enable USB debugging, accept RSA prompt, check `adb devices` |
| Permission denied | Some paths require root — try `adb root` |
| Slow transfers | Use USB 3.0; Wi-Fi ADB is slower; many small files are slower than few large ones |
| Connection lost | Reconnect USB cable, reopen plugin |
| ADB not found | Install platform-tools and ensure `adb` is in PATH (see above) |

## License

Part of the [far2l](https://github.com/elfmz/far2l) project. Same license as far2l (GPLv2).
