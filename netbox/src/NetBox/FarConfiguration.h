#pragma once

#include "GUIConfiguration.h"

class TCustomFarPlugin;
class TBookmarks;
class TBookmarkList;

class TFarConfiguration : public TGUIConfiguration
{
NB_DISABLE_COPY(TFarConfiguration)
NB_DECLARE_CLASS(TFarConfiguration)
public:
  explicit TFarConfiguration(TCustomFarPlugin * APlugin);
  virtual ~TFarConfiguration();

  virtual void Load();
  virtual void Save(bool All, bool Explicit);
  virtual void Default();
  virtual THierarchicalStorage * CreateStorage(bool & SessionList);
  void CacheFarSettings();

  const TCustomFarPlugin * GetPlugin() const { return FFarPlugin; }
  TCustomFarPlugin * GetPlugin() { return FFarPlugin; }
  void SetPlugin(TCustomFarPlugin * Value);
  bool GetConfirmOverwritingOverride() const { return FConfirmOverwritingOverride; }
  void SetConfirmOverwritingOverride(bool Value) { FConfirmOverwritingOverride = Value; }
  bool GetConfirmDeleting() const;
  bool GetConfirmSynchronizedBrowsing() const { return FConfirmSynchronizedBrowsing; }
  void SetConfirmSynchronizedBrowsing(bool Value) { FConfirmSynchronizedBrowsing = Value; }
  bool GetDisksMenu() const { return FDisksMenu; }
  void SetDisksMenu(bool Value) { FDisksMenu = Value; }
  intptr_t GetDisksMenuHotKey() const { return FDisksMenuHotKey; }
  void SetDisksMenuHotKey(intptr_t Value) { FDisksMenuHotKey = Value; }
  bool GetPluginsMenu() const { return FPluginsMenu; }
  void SetPluginsMenu(bool Value) { FPluginsMenu = Value; }
  bool GetPluginsMenuCommands() const { return FPluginsMenuCommands; }
  void SetPluginsMenuCommands(bool Value) { FPluginsMenuCommands = Value; }
  UnicodeString GetCommandPrefixes() const { return FCommandPrefixes; }
  void SetCommandPrefixes(const UnicodeString & Value) { FCommandPrefixes = Value; }
  bool GetHostNameInTitle() const { return FHostNameInTitle; }
  void SetHostNameInTitle(bool Value) { FHostNameInTitle = Value; }

  bool GetCustomPanelModeDetailed() const { return FCustomPanelModeDetailed; }
  void SetCustomPanelModeDetailed(bool Value) { FCustomPanelModeDetailed = Value; }
  bool GetFullScreenDetailed() const { return FFullScreenDetailed; }
  void SetFullScreenDetailed(bool Value) { FFullScreenDetailed = Value; }
  UnicodeString GetColumnTypesDetailed() const { return FColumnTypesDetailed; }
  void SetColumnTypesDetailed(const UnicodeString & Value) { FColumnTypesDetailed = Value; }
  UnicodeString GetColumnWidthsDetailed() const { return FColumnWidthsDetailed; }
  void SetColumnWidthsDetailed(const UnicodeString & Value) { FColumnWidthsDetailed = Value; }
  UnicodeString GetStatusColumnTypesDetailed() const { return FStatusColumnTypesDetailed; }
  void SetStatusColumnTypesDetailed(const UnicodeString & Value) { FStatusColumnTypesDetailed = Value; }
  UnicodeString GetStatusColumnWidthsDetailed() const { return FStatusColumnWidthsDetailed; }
  void SetStatusColumnWidthsDetailed(const UnicodeString & Value) { FStatusColumnWidthsDetailed = Value; }
  bool GetEditorDownloadDefaultMode() const { return FEditorDownloadDefaultMode; }
  void SetEditorDownloadDefaultMode(bool Value) { FEditorDownloadDefaultMode = Value; }
  bool GetEditorUploadSameOptions() const { return FEditorUploadSameOptions; }
  void SetEditorUploadSameOptions(bool Value) { FEditorUploadSameOptions = Value; }
  bool GetEditorUploadOnSave() const { return FEditorUploadOnSave; }
  void SetEditorUploadOnSave(bool Value) { FEditorUploadOnSave = Value; }
  bool GetEditorMultiple() const { return FEditorMultiple; }
  void SetEditorMultiple(bool Value) { FEditorMultiple = Value; }
  bool GetQueueBeep() const { return FQueueBeep; }
  void SetQueueBeep(bool Value) { FQueueBeep = Value; }

  UnicodeString GetApplyCommandCommand() const { return FApplyCommandCommand; }
  void SetApplyCommandCommand(const UnicodeString & Value) { FApplyCommandCommand = Value; }
  intptr_t GetApplyCommandParams() const { return FApplyCommandParams; }
  void SetApplyCommandParams(intptr_t Value) { FApplyCommandParams = Value; }

  UnicodeString GetPageantPath() const { return FPageantPath; }
  void SetPageantPath(const UnicodeString & Value) { FPageantPath = Value; }
  UnicodeString GetPuttygenPath() const { return FPuttygenPath; }
  void SetPuttygenPath(const UnicodeString & Value) { FPuttygenPath = Value; }
  TBookmarkList * GetBookmarks(const UnicodeString & Key);
  void SetBookmarks(const UnicodeString & Key, TBookmarkList * Value);

protected:
  virtual bool GetConfirmOverwriting() const;
  virtual void SetConfirmOverwriting(bool Value);

  virtual void SaveData(THierarchicalStorage * Storage, bool All);
  virtual void LoadData(THierarchicalStorage * Storage);

  virtual UnicodeString ModuleFileName() const;
  virtual void Saved();

private:
  TCustomFarPlugin * FFarPlugin;
  TBookmarks * FBookmarks;
  intptr_t FFarConfirmations;
  bool FConfirmOverwritingOverride;
  bool FConfirmSynchronizedBrowsing;
  bool FForceInheritance;
  bool FDisksMenu;
  intptr_t FDisksMenuHotKey;
  bool FPluginsMenu;
  bool FPluginsMenuCommands;
  UnicodeString FCommandPrefixes;
  bool FHostNameInTitle;
  bool FEditorDownloadDefaultMode;
  bool FEditorUploadSameOptions;
  bool FEditorUploadOnSave;
  bool FEditorMultiple;
  bool FQueueBeep;
  UnicodeString FPageantPath;
  UnicodeString FPuttygenPath;
  UnicodeString FApplyCommandCommand;
  intptr_t FApplyCommandParams;

  bool FCustomPanelModeDetailed;
  bool FFullScreenDetailed;
  UnicodeString FColumnTypesDetailed;
  UnicodeString FColumnWidthsDetailed;
  UnicodeString FStatusColumnTypesDetailed;
  UnicodeString FStatusColumnWidthsDetailed;

private:
  intptr_t FarConfirmations() const;
};

TFarConfiguration * GetFarConfiguration();

