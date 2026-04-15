/**
 * @file FARPlugin.h
 * @brief Far Manager Plugin API Documentation and Function Declarations
 * 
 * This header contains comprehensive documentation for all Far Manager API functions
 * used in the ADB plugin, based on the official Far Manager API reference.
 * 
 * @author stpork
 * @version 1.0
 * @see https://api.farmanager.com/ru/panelapi/index.html
 */

#ifndef FARPLUGIN_H
#define FARPLUGIN_H

// Include Far Manager SDK
#include "farplug-wide.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup PluginAPI Far Manager Plugin API Functions
 * @brief Complete documentation for Far Manager plugin interface functions
 * @{
 */

/**
 * @brief Get minimum Far Manager version required by the plugin
 * 
 * @return Minimum FAR version (MAKEFARVERSION(2, 0) = FAR 2.0)
 * 
 * @see https://api.farmanager.com/ru/panelapi/index.html
 */
int WINAPI GetMinFarVersionW(void);

/**
 * @brief Initialize plugin with Far Manager startup information
 * 
 * Called by Far Manager to pass plugin startup information and API functions.
 * The plugin should copy the Info structure for later use.
 * 
 * @param Info Pointer to PluginStartupInfo structure containing Far API functions
 * 
 * @see https://api.farmanager.com/ru/panelapi/index.html
 */
void WINAPI SetStartupInfoW(const PluginStartupInfo *Info);

/**
 * @brief Get plugin information and capabilities
 * 
 * Called by Far Manager to retrieve plugin information and configure capabilities.
 * This function defines what the plugin can do and how it appears in menus.
 * 
 * @param Info Pointer to PluginInfo structure to be filled with plugin details
 * 
 * Configuration options:
 * - Flags: Set plugin capabilities (PF_FULLCMDLINE, etc.)
 * - MenuStrings: Plugin menu entries
 * - ConfigStrings: Configuration menu entries
 * - CommandPrefix: Command line prefix (null for none)
 * 
 * @see https://api.farmanager.com/ru/panelapi/index.html
 */
void WINAPI GetPluginInfoW(PluginInfo *Info);

/**
 * @brief Open plugin and create a panel
 * 
 * Called by Far Manager to open the plugin and create a new panel instance.
 * This is the main entry point when user selects the plugin from menu.
 * 
 * @param OpenFrom Source of plugin opening:
 *   - OPEN_FROMMENU: Opened from plugin menu
 *   - OPEN_FROMCOMMANDLINE: Opened from command line
 *   - OPEN_FROMPLUGINS: Opened from another plugin
 * @param Item Additional parameter depending on OpenFrom
 * 
 * @return Plugin handle on success, INVALID_HANDLE_VALUE on failure
 * 
 * @see https://api.farmanager.com/ru/panelapi/index.html
 */
HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item);

/**
 * @brief Close plugin and free resources
 * 
 * Called by Far Manager to close the plugin and clean up resources.
 * This is called when user closes the plugin panel.
 * 
 * @param hPlugin Plugin handle returned by OpenPluginW
 * 
 * @see https://api.farmanager.com/ru/panelapi/index.html
 */
void WINAPI ClosePluginW(HANDLE hPlugin);

/**
 * @brief Get list of elements for the panel
 * 
 * Called by Far Manager to retrieve the list of files/directories to display
 * in the plugin panel. This is the core function that populates the panel.
 * 
 * @param hPlugin Plugin handle
 * @param pPanelItem Pointer to array of PluginPanelItem structures (output)
 * @param pItemsNumber Number of items in the array (output)
 * @param OpMode Operation mode:
 *   - OPM_FIND: Find operation
 *   - OPM_QUICKVIEW: Quick view operation
 *   - OPM_TOPLEVEL: Top level operation
 * 
 * @return TRUE on success, FALSE on failure
 * 
 * @note Plugin must allocate memory for PanelItem array. Memory will be freed by FreeFindDataW.
 * 
 * @see https://api.farmanager.com/ru/panelapi/index.html
 */
int WINAPI GetFindDataW(HANDLE hPlugin, PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode);

/**
 * @brief Free memory allocated by GetFindDataW
 * 
 * Called by Far Manager to free memory allocated for file list.
 * This is called when panel is closed or refreshed.
 * 
 * @param hPlugin Plugin handle
 * @param PanelItem Array of PluginPanelItem structures to free
 * @param ItemsNumber Number of items in the array
 * 
 * @see https://api.farmanager.com/ru/panelapi/index.html
 */
void WINAPI FreeFindDataW(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber);

/**
 * @brief Get information about the open panel
 * 
 * Called by Far Manager to get panel information and configuration.
 * This function provides details about the current panel state.
 * 
 * @param hPlugin Plugin handle
 * @param Info Pointer to OpenPluginInfo structure to be filled
 * 
 * Configuration options:
 * - Flags: Panel flags (OPIF_USEFILTER, OPIF_USESORTGROUPS, etc.)
 * - HostFile: Host file name
 * - CurDir: Current directory
 * - Format: Panel format string
 * - PanelTitle: Panel title
 * - InfoLines: Information panel lines
 * - DescrFiles: Description files
 * 
 * @see https://api.farmanager.com/ru/panelapi/index.html
 */
void WINAPI GetOpenPluginInfoW(HANDLE hPlugin, OpenPluginInfo *Info);

/**
 * @brief Handle keyboard and mouse input in the panel
 * 
 * Called by Far Manager to process keyboard input and hotkeys in the plugin panel.
 * Plugin can define custom hotkeys and handle special key combinations.
 * 
 * @param hPlugin Plugin handle
 * @param Key Virtual key code
 * @param ControlState Control key state (SHIFT, CTRL, ALT combinations)
 * 
 * @return TRUE if key was processed, FALSE if key should be handled by Far Manager
 * 
 * @see https://api.farmanager.com/ru/panelapi/index.html
 */
int WINAPI ProcessKeyW(HANDLE hPlugin, int Key, unsigned int ControlState);

/**
 * @brief Handle panel events
 * 
 * Called by Far Manager to process various panel events.
 * Currently disabled in ADB plugin but can be used for custom event handling.
 * 
 * @param hPlugin Plugin handle
 * @param Event Event type
 * @param Param Event-specific parameter
 * 
 * @return TRUE if event was processed, FALSE if event should be handled by Far Manager
 * 
 * @see https://api.farmanager.com/ru/panelapi/index.html
 */
int WINAPI ProcessEventW(HANDLE hPlugin, int Event, void *Param);

/**
 * @brief Set current directory on the emulated file system
 * 
 * Called by Far Manager to navigate to a different directory in the plugin panel.
 * Plugin should update its current directory and refresh the panel content.
 * 
 * @param hPlugin Plugin handle
 * @param Dir Directory path to change to
 * @param OpMode Operation mode:
 *   - OPM_SILENT: Silent operation
 *   - OPM_FIND: Find operation
 *   - OPM_TOPLEVEL: Top level operation
 * 
 * @return TRUE on success, FALSE on failure
 * 
 * @see https://api.farmanager.com/ru/panelapi/index.html
 */
int WINAPI SetDirectoryW(HANDLE hPlugin, const wchar_t *Dir, int OpMode);

/**
 * @brief Create directory
 * 
 * Called by Far Manager to create new directories in the emulated file system.
 * Currently not implemented in ADB plugin.
 * 
 * @param hPlugin Plugin handle
 * @param Name Array of directory names to create
 * @param OpMode Operation mode
 * 
 * @return TRUE on success, FALSE on failure
 * 
 * @see https://api.farmanager.com/ru/panelapi/index.html
 */
int WINAPI MakeDirectoryW(HANDLE hPlugin, const wchar_t **Name, int OpMode);

/**
 * @brief Delete files
 * 
 * Called by Far Manager to delete selected files from the emulated file system.
 * Plugin should handle file deletion and report status to user.
 * 
 * @param hPlugin Plugin handle
 * @param PanelItem Array of files to delete
 * @param ItemsNumber Number of files in array
 * @param OpMode Operation mode
 * 
 * @return TRUE on success, FALSE on failure
 * 
 * @see https://api.farmanager.com/ru/panelapi/index.html
 */
int WINAPI DeleteFilesW(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int OpMode);

/**
 * @brief Get files for processing (copying/moving/viewing...)
 * 
 * Called by Far Manager to download files from plugin to local system.
 * This handles file operations like copy, move, and view.
 * 
 * @param hPlugin Plugin handle
 * @param PanelItem Array of files to copy
 * @param ItemsNumber Number of files in array
 * @param Move TRUE to move files, FALSE to copy
 * @param DestPath Destination path array
 * @param OpMode Operation mode
 * 
 * @return TRUE on success, FALSE on failure
 * 
 * @see https://api.farmanager.com/ru/panelapi/index.html
 */
int WINAPI GetFilesW(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t **DestPath, int OpMode);

/**
 * @brief Open a file as a plugin
 * 
 * Called by Far Manager to open a file using this plugin.
 * Used for file type associations. Currently not supported in ADB plugin.
 * 
 * @param Name Name of the file to open
 * @param Data File data buffer
 * @param DataSize Size of the data buffer
 * @param OpMode Operation mode
 * 
 * @return Plugin handle on success, INVALID_HANDLE_VALUE if not supported
 * 
 * @see https://api.farmanager.com/ru/panelapi/index.html
 */
HANDLE WINAPI OpenFilePluginW(const wchar_t *Name, const unsigned char *Data, int DataSize, int OpMode);

/**
 * @brief Put files on the emulated file system
 * 
 * Called by Far Manager to upload files from local system to plugin.
 * This handles file operations like copy and move to the plugin's file system.
 * 
 * @param hPlugin Plugin handle
 * @param PanelItem Array of destination files
 * @param ItemsNumber Number of files in array
 * @param Move TRUE to move files, FALSE to copy
 * @param SrcPath Source path
 * @param OpMode Operation mode
 * 
 * @return TRUE on success, FALSE on failure
 * 
 * @see https://api.farmanager.com/ru/panelapi/index.html
 */
int WINAPI PutFilesW(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t *SrcPath, int OpMode);

/**
 * @brief Execute Far Manager archive command
 * 
 * Called by Far Manager to perform Far Manager archive commands and additional file operations.
 * This is recommended for additional operations on files handled by file processing plugins.
 * 
 * @param hPlugin Plugin handle
 * @param PanelItem Array of files to process
 * @param ItemsNumber Number of files in array
 * @param OpMode Operation mode (0 or OPM_TOPLEVEL)
 * 
 * @return TRUE on success, FALSE on failure
 * 
 * @note If operation fails but some files were processed, plugin can clear
 *       PPIF_SELECTED flag in processed items to remove selection.
 * 
 * @see https://api.farmanager.com/ru/panelapi/index.html
 */
int WINAPI ProcessHostFileW(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int OpMode);

/**
 * @brief Get symbolic link target
 * 
 * Called by Far Manager to get the target of a symbolic link.
 * Currently not implemented in ADB plugin.
 * 
 * @param hPlugin Plugin handle
 * @param PanelItem File item to get link target for
 * @param Target Buffer to store target path
 * @param TargetSize Size of target buffer
 * @param OpMode Operation mode
 * 
 * @return TRUE on success, FALSE on failure
 * 
 * @see https://api.farmanager.com/ru/panelapi/index.html
 */
int WINAPI GetLinkTargetW(HANDLE hPlugin, PluginPanelItem *PanelItem, wchar_t *Target, size_t TargetSize, int OpMode);

/**
 * @brief Execute files or commands
 * 
 * Called by Far Manager to execute selected files or run commands.
 * Currently not implemented in ADB plugin.
 * 
 * @param hPlugin Plugin handle
 * @param PanelItem Array of files to execute
 * @param ItemsNumber Number of files in array
 * @param OpMode Operation mode
 * 
 * @return TRUE on success, FALSE on failure
 * 
 * @see https://api.farmanager.com/ru/panelapi/index.html
 */
int WINAPI ExecuteW(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int OpMode);

/**
 * @brief Open plugin configuration dialog
 * 
 * Called by Far Manager to open plugin settings and configuration.
 * Currently not implemented in ADB plugin.
 * 
 * @param ItemNumber Configuration item number (0 for main settings)
 * 
 * @return TRUE if configuration was successful, FALSE otherwise
 * 
 * @see https://api.farmanager.com/ru/panelapi/index.html
 */
int WINAPI ConfigureW(int ItemNumber);

/**
 * @brief Plugin cleanup before Far Manager exit
 * 
 * Called by Far Manager when it's about to exit.
 * Allows plugin to perform final cleanup operations.
 * 
 * @see https://api.farmanager.com/ru/panelapi/index.html
 */
void WINAPI ExitFARW(void);

/**
 * @brief Check if Far Manager can safely exit
 * 
 * Called by Far Manager to check if it's safe to exit.
 * Plugin should return FALSE if it has active operations that prevent exit.
 * 
 * @return TRUE if Far can exit safely, FALSE if plugin has active operations
 * 
 * @see https://api.farmanager.com/ru/panelapi/index.html
 */
int WINAPI MayExitFARW(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif // FARPLUGIN_H