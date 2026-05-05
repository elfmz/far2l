#pragma once

#include <farplug-wide.h>

extern PluginStartupInfo g_Info;

// IDs MUST match line order in adbEng.lng / adbRus.lng (1-based after the .Language header).
enum ADBLng
{
    MPluginTitle,           // "ADB Plugin"
    MDevicesTitle,          // "ADB Devices"
    MOk,                    // "OK"
    MCancelBtn,             // "Cancel"
    MError,                 // "Error"
    MWarning,               // "Warning"

    MNoDevicesFound,        // "No devices found"
    MConnectDevice,         // "Connect device"
    MSelectDevice,          // "Select device"
    MConnecting,            // "Connecting..."

    // Copy/move/delete progress titles
    MCopyFromDevice,        // "Copy from device"
    MCopyToDevice,          // "Copy to device"
    MMoveFromDevice,        // "Move from device"
    MMoveToDevice,          // "Move to device"
    MDeleteFromDevice,      // "Delete from device"

    // Action titles
    MCopy,                  // "Copy"
    MMove,                  // "Move"
    MDelete,                // "Delete"
    MCreateDir,             // "Create directory"

    // Delete confirmation
    MDeleteFileQ,           // "Do you wish to delete the file"
    MDeleteFolderQ,         // "Do you wish to delete the folder"
    MDeleteItemsQ,          // "Do you wish to delete"
    MDeleteFiles,           // "Delete files"
    MDeleteFolders,         // "Delete folders"
    MDeleteItems,           // "Delete items"
    MFoldersAnd,            // " folders and "
    MFilesSuffix,           // " files"
    MFoldersSuffix,         // " folders"

    // Overwrite dialog
    MFileExists,            // "File already exists"
    MFolderExists,          // "Folder already exists"
    MOverwriteBtn,          // "&Overwrite"
    MSkipBtn,               // "&Skip"

    // Abort dialog
    MAbortBoxTitle,         // "Abort operation"
    MAbortConfirmText,      // "Abort current operation?"
    MAbortBtn,              // "&Abort"
    MContinueBtn,           // "&Continue"

    // Progress dialog rows
    MCopyFromColon,         // "Copy from:"
    MCopyFileFrom,          // "Copy file from:"
    MCopyFolderFrom,        // "Copy folder from:"
    MToColon,               // "to:"
    MFilesProcessed,        // "Files processed:"
    MTotal,                 // "Total:"
    MBytes,                 // "bytes"
    MTime,                  // "Time:"
    MRemaining,             // "Remaining:"

    MEnterDestPath,         // "Enter destination path:"
    MEnterDirName,          // "Enter directory name:"

    // Generic errors
    MConnectionFailed,      // "Connection failed"
    MPermissionDenied,      // "Permission denied"
    MOperationFailed,       // "Operation failed"
    MShellError,            // "Shell error"

    // Panel titles + columns
    MADBSelectDevice,       // "ADB - Select Device"
    MNoDevicesPanelTitle,   // "ADB: No devices found"
    MColName,               // "Name"
    MColSize,               // "Size"
    MColSerial,             // "Serial Number"
    MColDeviceName,         // "Device Name"
    MColModel,              // "Model"
    MColPort,               // "Port"

    // Overwrite extras
    MNewLabel,              // "New"
    MExistingLabel,         // "Existing"
    MNewerBtn,              // "Ne&wer"
    MRenameBtn,             // "&Rename"
    MCancelMnemonicBtn,     // "&Cancel"
    MRememberChoice,        // "&Remember choice"

    // Rename
    MRenameTitle,           // "Rename"
    MRenameTo,              // "Rename to:"
    MRenameFailed,          // "Rename failed"

    // Copy/transfer errors
    MCopyFailed,            // "Copy failed"
    MErrCouldNotMoveAside,  // "Could not move existing destination aside."
    MErrCpFallbackFailed,   // "Device-side cp and host-mediated pull/push both failed."

    // Same-device transfer titles
    MCopyOnDevice,          // "Copy on device"
    MMoveOnDevice,          // "Move on device"
    MCopyToHost,            // "Copy to host"
    MMoveToHost,            // "Move to host"

    MDeletingFileOrFolder,  // "Deleting the file or folder"
    MUnknownError,          // "unknown error"
    MDeleteFolderTitle,     // "Delete folder"
    MItemsSuffix,           // " items"
};

inline const wchar_t* Lng(ADBLng id)
{
    return g_Info.GetMsg(g_Info.ModuleNumber, id);
}
