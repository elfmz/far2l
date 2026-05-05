#pragma once

#include <farplug-wide.h>

extern PluginStartupInfo g_Info;

// IDs MUST match line order in mtpEng.lng / mtpRus.lng (1-based after the .Language header).
enum MTPLng
{
    MPluginTitle,           // "MTP Plugin"
    MWarning,               // "Warning"

    // AbortConfirmDialog
    MAbortBoxTitle,         // "Abort operation"
    MAbortConfirmText,      // "Confirm abort current operation"
    MAbortBtn,              // "&Abort operation"
    MContinueBtn,           // "&Continue"

    // OverwriteDialog
    MFileExists,            // "File already exists"
    MNewLabel,              // "New"
    MExistingLabel,         // "Existing"
    MRememberChoice,        // "&Remember choice"
    MOverwriteBtn,          // "&Overwrite"
    MSkipBtn,               // "&Skip"
    MNewerBtn,              // "Ne&wer"
    MRenameBtn,             // "&Rename"
    MCancelBtn,             // "&Cancel"

    // ProgressDialog
    MCopyToDevice,          // "Copy to device"
    MCopyFromDevice,        // "Copy from device"
    MMoveToDevice,          // "Move to device"
    MMoveFromDevice,        // "Move from device"
    MCopyingFile,           // "Copying the file"
    MMovingFile,            // "Moving the file"
    MTo,                    // "to"
    MTotal,                 // "Total"
    MFilesProcessed,        // "Files processed:"
    MOf,                    // "of"
    MTime,                  // "Time:"
    MRemaining,             // "Remaining:"

    // DeleteProgressDialog
    MDeleteBoxTitle,        // "Delete"
    MDeletingFileOrFolder,  // "Deleting the file or folder"

    // Panel titles + column headers
    MDevicesTitle,          // "MTP/PTP Devices"
    MColDevice,             // "Device"
    MColManufacturer,       // "Manufacturer"
    MColSerial,             // "Serial"
    MColVidPid,             // "VID:PID"
    MColName,               // "Name"
    MColSize,               // "Size"

    // Copy/Move/Rename prompts
    MPromptCopyTitle,       // "Copy"
    MPromptMoveTitle,       // "Move"
    MPromptRenameTitle,     // "Rename"
    MPromptRenameTo,        // "Rename to:"
    MPromptToColon,         // "to:"
    MPromptItems,           // "items"

    // Delete confirmation
    MDeleteFile,            // "Do you wish to delete the file"
    MDeleteFolder,          // "Do you wish to delete the folder"
    MDeleteItems,           // "Do you wish to delete"

    // Error popup titles
    MCopyFailed,            // "Copy failed"
    MMoveFailed,            // "Move failed"
    MRenameFailed,          // "Rename failed"
    MMkdirFailed,           // "Create directory failed"
    MCannotOpenDevice,      // "MTP: cannot open device"
    MUnknownError,          // "unknown error"

    // Error popup bodies
    MErrResolveDstDevice,   // "Could not resolve destination on device. Check that all path components exist or can be created."
    MErrResolveDstPath,     // "Could not resolve destination path. Use ../ for parent or subdir/ for descent."
    MErrResolveDstFolders,  // "Could not resolve destination path. Check that all folder components exist (or create them first)."
    MErrFallbackFailed,     // "CopyObject+Delete fallback also failed."
};

inline const wchar_t* Lng(MTPLng id)
{
    return g_Info.GetMsg(g_Info.ModuleNumber, id);
}
