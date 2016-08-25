
#pragma once

#include <Classes.hpp>
//#include <Buttons.hpp>
#include <Interface.h>
#include <GUIConfiguration.h>
#include <SynchronizeController.h>
#include <MsgIDs.h>

#ifdef LOCALINTERFACE
#include <LocalInterface.h>
#endif

#define SITE_ICON 1
#define SITE_FOLDER_ICON 2
#define WORKSPACE_ICON 3

class TStoredSessionList;
class TConfiguration;
class TTerminal;

const int mpNeverAskAgainCheck   = 0x01;
const int mpAllowContinueOnError = 0x02;

#define UPLOAD_IF_ANY_SWITCH L"UploadIfAny"
#define UPLOAD_SWITCH L"Upload"
#define JUMPLIST_SWITCH L"JumpList"
#define DESKTOP_SWITCH L"Desktop"
#define SEND_TO_HOOK_SWITCH L"SendToHook"
#define UNSAFE_SWITCH L"Unsafe"
#define NEWINSTANCE_SWICH L"NewInstance"
#define KEYGEN_SWITCH L"KeyGen"
#define KEYGEN_OUTPUT_SWITCH L"Output"
#define KEYGEN_COMMENT_SWITCH L"Comment"
#define KEYGEN_CHANGE_PASSPHRASE_SWITCH L"ChangePassphrase"
#define LOG_SWITCH L"Log"
#define INI_SWITCH L"Ini"

typedef int TSize;

struct TMessageParams : public TObject
{
NB_DISABLE_COPY(TMessageParams)
public:
//  TMessageParams();
  explicit TMessageParams(uintptr_t AParams = 0);
  void Assign(const TMessageParams * AParams);

  const TQueryButtonAlias * Aliases;
  uintptr_t AliasesCount;
  uintptr_t Flags;
  uintptr_t Params;
  uintptr_t Timer;
  TQueryParamsTimerEvent TimerEvent;
  UnicodeString TimerMessage;
  uintptr_t TimerAnswers;
  TQueryType TimerQueryType;
  uintptr_t Timeout;
  uintptr_t TimeoutAnswer;
  UnicodeString NeverAskAgainTitle;
  uintptr_t NeverAskAgainAnswer;
  bool NeverAskAgainCheckedInitially;
  bool AllowHelp;
  UnicodeString ImageName;
  UnicodeString MoreMessagesUrl;
  TSize MoreMessagesSize;
  UnicodeString CustomCaption;

private:
  inline void Reset();
};

class TCustomScpExplorerForm;
TCustomScpExplorerForm * CreateScpExplorer();

void ConfigureInterface();

void DoProductLicense();

extern const UnicodeString AppName;

void SetOnForeground(bool OnForeground);
void FlashOnBackground();

void ShowExtendedExceptionEx(TTerminal * Terminal, Exception * E);
//void FormHelp(TCustomForm * Form);
void SearchHelp(const UnicodeString & Message);
void MessageWithNoHelp(const UnicodeString & Message);

class TProgramParams;
bool CheckSafe(TProgramParams * Params);
bool CheckXmlLogParam(TProgramParams * Params);

#if 0
UnicodeString GetToolbarsLayoutStr(TComponent * OwnerComponent);
void LoadToolbarsLayoutStr(TComponent * OwnerComponent, UnicodeString LayoutStr);

namespace Tb2item { class TTBCustomItem; }
void AddMenuSeparator(Tb2item::TTBCustomItem * Menu);
void AddMenuLabel(Tb2item::TTBCustomItem * Menu, const UnicodeString & Label);
#endif

// windows\WinHelp.cpp
void InitializeWinHelp();
void FinalizeWinHelp();

// windows\WinInterface.cpp
uintptr_t MessageDialog(const UnicodeString & Msg, TQueryType Type,
  uintptr_t Answers, const UnicodeString & HelpKeyword = HELP_NONE, const TMessageParams * Params = nullptr);
uintptr_t MessageDialog(intptr_t Ident, TQueryType Type,
  uintptr_t Answers, const UnicodeString & HelpKeyword = HELP_NONE, const TMessageParams * Params = nullptr);
uintptr_t SimpleErrorDialog(const UnicodeString & Msg, const UnicodeString & MoreMessages = L"");

uintptr_t MoreMessageDialog(const UnicodeString & Message,
  TStrings * MoreMessages, TQueryType Type, uintptr_t Answers,
  const UnicodeString & HelpKeyword, const TMessageParams * Params = nullptr);

uintptr_t ExceptionMessageDialog(Exception * E, TQueryType Type,
  const UnicodeString & MessageFormat = L"", uintptr_t Answers = qaOK,
  const UnicodeString & HelpKeyword = HELP_NONE, const TMessageParams * Params = nullptr);
uintptr_t FatalExceptionMessageDialog(Exception * E, TQueryType Type,
  intptr_t SessionReopenTimeout, const UnicodeString & MessageFormat = L"", uintptr_t Answers = qaOK,
  const UnicodeString & HelpKeyword = HELP_NONE, const TMessageParams * Params = nullptr);

// forms\Custom.cpp
TSessionData * DoSaveSession(TSessionData * SessionData,
  TSessionData * OriginalSession, bool ForceDialog,
  TStrings * AdditionalFolders);
void SessionNameValidate(const UnicodeString & Text,
  const UnicodeString & OriginalName);
bool DoSaveWorkspaceDialog(UnicodeString & WorkspaceName,
  bool * SavePasswords, bool NotRecommendedSavingPasswords,
  bool & CreateShortcut, bool & EnableAutoSave);
class TShortCuts;
bool DoShortCutDialog(TShortCut & ShortCut,
  const TShortCuts & ShortCuts, UnicodeString HelpKeyword);

// windows\UserInterface.cpp
bool DoMasterPasswordDialog();
bool DoChangeMasterPasswordDialog(UnicodeString & NewPassword);

// windows\WinMain.cpp
int Execute();
void GetLoginData(UnicodeString SessionName, TOptions * Options,
  TObjectList * DataList, UnicodeString & DownloadFile, bool NeedSession);

bool InputDialog(const UnicodeString & ACaption,
  const UnicodeString & APrompt, UnicodeString & Value, const UnicodeString & HelpKeyword = HELP_NONE,
  TStrings * History = nullptr, bool PathInput = false,
  TInputDialogInitializeEvent OnInitialize = nullptr, bool Echo = true);

// forms\About.cpp
struct TRegistration
{
  bool Registered;
  UnicodeString Subject;
  int Licenses;
  UnicodeString ProductId;
  bool NeverExpires;
  TDateTime Expiration;
  bool EduLicense;
  TNotifyEvent OnRegistrationLink;
};
void DoAboutDialog(TConfiguration * Configuration,
  bool AllowLicense, TRegistration * Registration);
void DoAboutDialog(TConfiguration * Configuration);

// forms\Cleanup.cpp
bool DoCleanupDialog(TStoredSessionList *SessionList,
    TConfiguration *Configuration);

// forms\Console.cpp
void DoConsoleDialog(TTerminal * Terminal,
    const UnicodeString & Command = L"", const TStrings * Log = nullptr);

// forms\Copy.cpp
const int coTemp                = 0x001;
const int coDisableQueue        = 0x002;
const int coDisableDirectory    = 0x008; // not used anymore
const int coDoNotShowAgain      = 0x020;
const int coDisableSaveSettings = 0x040; // not used anymore
const int coDoNotUsePresets     = 0x080;
const int coAllowRemoteTransfer = 0x100;
const int coNoQueue             = 0x200;
const int coNoQueueIndividually = 0x400;
const int coShortCutHint        = 0x800;
const int cooDoNotShowAgain     = 0x01;
const int cooRemoteTransfer     = 0x02;
const int cooSaveSettings       = 0x04;

const int coTempTransfer        = 0x08;
const int coDisableNewerOnly    = 0x10;

bool DoCopyDialog(bool ToRemote,
  bool Move, TStrings * FileList, UnicodeString & TargetDirectory,
  TGUICopyParamType * Params, int Options, int CopyParamAttrs,
  int * OutputOptions);

// forms\CreateDirectory.cpp
bool DoCreateDirectoryDialog(UnicodeString & Directory,
  TRemoteProperties * Properties, int AllowedChanges, bool & SaveSettings);

// forms\ImportSessions.cpp
bool DoImportSessionsDialog(TList * Imported);

// forms\License.cpp
enum TLicense { lcNoLicense = -1, lcWinScp, lcExpat };
void DoLicenseDialog(TLicense License);

bool DoLoginDialog(TStoredSessionList * SessionList, TList * DataList);

  // forms\SiteAdvanced.cpp
bool DoSiteAdvancedDialog(TSessionData * SessionData);

// forms\OpenDirectory.cpp
enum TOpenDirectoryMode { odBrowse, odAddBookmark };
bool DoOpenDirectoryDialog(TOpenDirectoryMode Mode, TOperationSide Side,
  UnicodeString & Directory, TStrings * Directories, TTerminal * Terminal,
  bool AllowSwitch);

// forms\LocatinoProfiles.cpp
bool LocationProfilesDialog(TOpenDirectoryMode Mode,
  TOperationSide Side, UnicodeString & LocalDirectory, UnicodeString & RemoteDirectory,
  TStrings * LocalDirectories, TStrings * RemoteDirectories, TTerminal * Terminal);

// forms\Preferences.cpp
enum TPreferencesMode { pmDefault, pmEditor, pmCustomCommands,
    pmQueue, pmLogging, pmUpdates, pmPresets, pmEditors, pmCommander,
    pmEditorInternal };
struct TCopyParamRuleData;
struct TPreferencesDialogData
{
  TCopyParamRuleData * CopyParamRuleData;
};
bool DoPreferencesDialog(TPreferencesMode APreferencesMode,
  TPreferencesDialogData * DialogData = nullptr);

// forms\CustomCommand.cpp
class TCustomCommandList;
class TCustomCommandType;
class TShortCuts;
enum TCustomCommandsMode { ccmAdd, ccmEdit, ccmAdHoc };
const int ccoDisableRemote = 0x01;
//typedef void (__closure *TCustomCommandValidate)
//  (const TCustomCommandType & Command);
DEFINE_CALLBACK_TYPE1(TCustomCommandValidateEvent, void,
  const TCustomCommandType & /*Command*/);
bool DoCustomCommandDialog(TCustomCommandType & Command,
  const TCustomCommandList * CustomCommandList,
  TCustomCommandsMode Mode, int Options, TCustomCommandValidateEvent OnValidate,
  const TShortCuts * ShortCuts);

// forms\CopyParamPreset.cpp
class TCopyParamList;
enum TCopyParamPresetMode { cpmAdd, cpmAddCurrent, cpmEdit, cpmDuplicate };
bool DoCopyParamPresetDialog(TCopyParamList * CopyParamList,
  int & Index, TCopyParamPresetMode Mode, TCopyParamRuleData * CurrentRuleData,
  const TCopyParamType & DefaultCopyParams);

// forms\CopyParamCsutom.cpp
bool DoCopyParamCustomDialog(TCopyParamType & CopyParam,
  int CopyParamAttrs);

// forms\Properties.cpp
class TRemoteProperties;
class TRemoteTokenList;
struct TCalculateSizeStats;
const int cpMode =  0x01;
const int cpOwner = 0x02;
const int cpGroup = 0x04;
//typedef void (__closure *TCalculateSizeEvent)
//  (TStrings * FileList, __int64 & Size, TCalculateSizeStats & Stats,
//   bool & Close);
DEFINE_CALLBACK_TYPE4(TCalculateSizeEvent, void,
  TStrings * /*FileList*/, int64_t & /*Size*/, TCalculateSizeStats & /*Stats*/,
  bool & /*Close*/);
//typedef void (__closure *TCalculatedChecksumCallbackEvent)(
//  const UnicodeString & FileName, const UnicodeString & Alg, const UnicodeString & Hash);
DEFINE_CALLBACK_TYPE3(TCalculatedChecksumCallbackEvent, void,
  const UnicodeString & /*FileName*/, const UnicodeString & /*Alg*/, const UnicodeString & /*Hash*/);
//typedef void (__closure *TCalculateChecksumEvent)
//  (const UnicodeString & Alg, TStrings * FileList,
//   TCalculatedChecksumCallbackEvent OnCalculatedChecksum, bool & Close);
DEFINE_CALLBACK_TYPE4(TCalculateChecksumEvent, void,
  const UnicodeString & /*Alg*/, TStrings * /*FileList*/,
  TCalculatedChecksumCallbackEvent /*OnCalculatedChecksum*/, bool & /*Close*/);
bool DoPropertiesDialog(TStrings * FileList,
    const UnicodeString & Directory, const TRemoteTokenList * GroupList,
    const TRemoteTokenList * UserList, TStrings * ChecksumAlgs,
    TRemoteProperties * Properties,
    int AllowedChanges, bool UserGroupByID, TCalculateSizeEvent OnCalculateSize,
    TCalculateChecksumEvent OnCalculateChecksum);

bool DoRemoteMoveDialog(bool Multi, UnicodeString & Target, UnicodeString & FileMask);
enum TDirectRemoteCopy { drcDisallow, drcAllow, drcConfirmCommandSession };
bool DoRemoteCopyDialog(TStrings * Sessions, TStrings * Directories,
  TDirectRemoteCopy AllowDirectCopy, bool Multi, void *& Session,
  UnicodeString & Target, UnicodeString & FileMask, bool & DirectCopy);

// forms\SelectMask.cpp
#ifdef CustomdirviewHPP
bool DoSelectMaskDialog(TCustomDirView * Parent, bool Select,
    TFileFilter * Filter, TConfiguration * Configuration);
bool DoFilterMaskDialog(TCustomDirView * Parent,
  TFileFilter * Filter);
#endif

// forms\EditMask.cpp
bool DoEditMaskDialog(TFileMasks & Mask);

const int spDelete = 0x01;
const int spNoConfirmation = 0x02;
const int spExistingOnly = 0x04;
const int spPreviewChanges = 0x40; // not used by core
const int spTimestamp = 0x100;
const int spNotByTime = 0x200;
const int spBySize = 0x400;
const int spSelectedOnly = 0x800;
const int spMirror = 0x1000;

// forms\Synchronize.cpp
const int soDoNotUsePresets =  0x01;
const int soNoMinimize =       0x02;
const int soAllowSelectedOnly = 0x04;
//typedef void (__closure *TGetSynchronizeOptionsEvent)
//  (int Params, TSynchronizeOptions & Options);
DEFINE_CALLBACK_TYPE2(TGetSynchronizeOptionsEvent, void,
  intptr_t /*Params*/, TSynchronizeOptions & /*Options*/);
//typedef void (__closure *TFeedSynchronizeError)
//  (const UnicodeString & Message, TStrings * MoreMessages, TQueryType Type,
//   const UnicodeString & HelpKeyword);
DEFINE_CALLBACK_TYPE4(TFeedSynchronizeErrorEvent, void,
  const UnicodeString & /*Message*/, TStrings * /*MoreMessages*/, TQueryType /*Type*/,
  const UnicodeString & /*HelpKeyword*/);
bool DoSynchronizeDialog(TSynchronizeParamType & Params,
  const TCopyParamType * CopyParams, TSynchronizeStartStopEvent OnStartStop,
  bool & SaveSettings, int Options, int CopyParamAttrs,
  TGetSynchronizeOptionsEvent OnGetOptions,
  TFeedSynchronizeErrorEvent & OnFeedSynchronizeError,
  bool Start);

// forms\FullSynchronize.cpp
struct TUsableCopyParamAttrs;
enum TSynchronizeMode { smRemote, smLocal, smBoth };
const int fsoDisableTimestamp = 0x01;
const int fsoDoNotUsePresets =  0x02;
const int fsoAllowSelectedOnly = 0x04;
bool DoFullSynchronizeDialog(TSynchronizeMode & Mode, intptr_t & Params,
  UnicodeString & LocalDirectory, UnicodeString & RemoteDirectory,
  TCopyParamType * CopyParams, bool & SaveSettings, bool & SaveMode,
  intptr_t Options, const TUsableCopyParamAttrs & CopyParamAttrs);

// forms\SynchronizeChecklist.cpp
class TSynchronizeChecklist;
//typedef void (__closure *TCustomCommandMenuEvent)
//  (TAction * Action, TStrings * LocalFileList, TStrings * RemoteFileList);
DEFINE_CALLBACK_TYPE3(TCustomCommandMenuEvent, void,
  void * /*Action*/, TStrings * /*LocalFileList*/, TStrings * /*RemoteFileList*/);
bool DoSynchronizeChecklistDialog(TSynchronizeChecklist * Checklist,
  TSynchronizeMode Mode, intptr_t Params,
  const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory,
  TCustomCommandMenuEvent OnCustomCommandMenu);

// forms\Editor.cpp
//typedef void (__closure *TFileClosedEvent)
//  (TObject * Sender, bool Forced);
#if 0
DEFINE_CALLBACK_TYPE2(TFileClosedEvent, void,
  TObject * /*Sender* /, bool /*Forced*/);
DEFINE_CALLBACK_TYPE2(TAnyModifiedEvent, void,
  TObject * /*Sender* /, bool & /*Modified*/);
TForm * ShowEditorForm(const UnicodeString FileName, TCustomForm * ParentForm,
  TNotifyEvent OnFileChanged, TNotifyEvent OnFileReload, TFileClosedEvent OnClose,
  TNotifyEvent OnSaveAll, TAnyModifiedEvent OnAnyModified,
  const UnicodeString Caption, bool StandaloneEditor, TColor Color);
void ReconfigureEditorForm(TForm * Form);
void EditorFormFileUploadComplete(TForm * Form);
void EditorFormFileSave(TForm * Form);
bool IsEditorFormModified(TForm * Form);
#endif

bool DoSymlinkDialog(UnicodeString & FileName, UnicodeString & PointTo,
  TOperationSide Side, bool & SymbolicLink, bool Edit, bool AllowSymbolic);

// forms\FileSystemInfo.cpp
struct TSpaceAvailable;
struct TFileSystemInfo;
struct TSessionInfo;
//typedef void (__closure *TGetSpaceAvailable)
//  (const UnicodeString Path, TSpaceAvailable & ASpaceAvailable, bool & Close);
DEFINE_CALLBACK_TYPE3(TGetSpaceAvailableEvent, void,
  const UnicodeString & /*Path*/, TSpaceAvailable & /*ASpaceAvailable*/, bool & /*Close*/);
void DoFileSystemInfoDialog(
  const TSessionInfo & SessionInfo, const TFileSystemInfo & FileSystemInfo,
  const UnicodeString & SpaceAvailablePath, TGetSpaceAvailableEvent OnGetSpaceAvailable);

//moved to FarInterface.h
#if 0
// forms\MessageDlg.cpp
void AnswerNameAndCaption(
  uintptr_t Answer, UnicodeString & Name, UnicodeString & Caption);
TFarDialog * CreateMoreMessageDialog(const UnicodeString & Msg,
  TStrings * MoreMessages, TMsgDlgType DlgType, uintptr_t Answers,
  const TQueryButtonAlias * Aliases, uintptr_t AliasesCount,
  uintptr_t TimeoutAnswer, TFarButton ** TimeoutButton,
  const UnicodeString & ImageName, const UnicodeString & NeverAskAgainCaption,
  const UnicodeString & MoreMessagesUrl, TSize MoreMessagesSize,
  const UnicodeString & CustomCaption);
TFarDialog * CreateMoreMessageDialogEx(const UnicodeString & Message, TStrings * MoreMessages,
  TQueryType Type, uintptr_t Answers, UnicodeString HelpKeyword, const TMessageParams * Params);
uintptr_t ExecuteMessageDialog(TFarDialog * Dialog, uintptr_t Answers, const TMessageParams * Params);
void InsertPanelToMessageDialog(TFarDialog * Form, TPanel * Panel);
void NavigateMessageDialogToUrl(TFarDialog * Form, const UnicodeString & Url);
#endif

// windows\Console.cpp
enum TConsoleMode { cmNone, cmScripting, cmHelp, cmBatchSettings, cmKeyGen };
int Console(TConsoleMode Mode);

// forms\EditorPreferences.cpp
enum TEditorPreferencesMode { epmAdd, epmEdit, epmAdHoc };
class TEditorData;
bool DoEditorPreferencesDialog(TEditorData * Editor,
  bool & Remember, TEditorPreferencesMode Mode, bool MayRemote);

// forms\Find.cpp
//typedef void (__closure *TFindEvent)
//  (UnicodeString Directory, const TFileMasks & FileMask,
//   TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile);
DEFINE_CALLBACK_TYPE4(TFindEvent, void,
  const UnicodeString & /*Directory*/, const TFileMasks & /*FileMask*/,
  TFileFoundEvent /*OnFileFound*/, TFindingFileEvent /*OnFindingFile*/);
bool DoFileFindDialog(UnicodeString Directory,
  TFindEvent OnFind, UnicodeString & Path);

// forms\GenerateUrl.cpp
void DoGenerateUrlDialog(TSessionData * Data, TStrings * Paths);

#if 0
void CopyParamListButton(TButton * Button);
const int cplNone =             0x00;
const int cplCustomize =        0x01;
const int cplCustomizeDefault = 0x02;
const int cplSaveSettings =     0x04;
void CopyParamListPopup(TRect R, TPopupMenu * Menu,
  const TCopyParamType & Param, UnicodeString Preset, TNotifyEvent OnClick,
  int Options, int CopyParamAttrs, bool SaveSettings = false);
bool CopyParamListPopupClick(TObject * Sender,
  TCopyParamType & Param, UnicodeString & Preset, int CopyParamAttrs,
  bool * SaveSettings = nullptr);

void MenuPopup(TPopupMenu * Menu, TRect Rect, TComponent * PopupComponent);
void MenuPopup(TPopupMenu * Menu, TButton * Button);
void MenuPopup(TObject * Sender, const TPoint & MousePos, bool & Handled);
void MenuButton(TButton * Button);
TComponent * GetPopupComponent(TObject * Sender);
TRect CalculatePopupRect(TButton * Button);
TRect CalculatePopupRect(TControl * Control, TPoint MousePos);

typedef void (__closure *TColorChangeEvent)
  (TColor Color);
TPopupMenu * CreateSessionColorPopupMenu(TColor Color,
  TColorChangeEvent OnColorChange);
void CreateSessionColorMenu(TComponent * AOwner, TColor Color,
  TColorChangeEvent OnColorChange);
void CreateEditorBackgroundColorMenu(TComponent * AOwner, TColor Color,
  TColorChangeEvent OnColorChange);
TPopupMenu * CreateColorPopupMenu(TColor Color,
  TColorChangeEvent OnColorChange);

void FixButtonImage(TButton * Button);
void CenterButtonImage(TButton * Button);

void UpgradeSpeedButton(TSpeedButton * Button);

int AdjustLocaleFlag(const UnicodeString & S, TLocaleFlagOverride LocaleFlagOverride, bool Recommended, int On, int Off);

void SetGlobalMinimizeHandler(TCustomForm * Form, TNotifyEvent OnMinimize);
void ClearGlobalMinimizeHandler(TNotifyEvent OnMinimize);
void CallGlobalMinimizeHandler(TObject * Sender);
bool IsApplicationMinimized();
void ApplicationMinimize();
void ApplicationRestore();
bool HandleMinimizeSysCommand(TMessage & Message);
void WinInitialize();
void WinFinalize();
#endif

void ShowNotification(TTerminal * Terminal, const UnicodeString & Str,
  TQueryType Type);
#if 0
void InitializeShortCutCombo(TComboBox * ComboBox,
  const TShortCuts & ShortCuts);
void SetShortCutCombo(TComboBox * ComboBox, TShortCut Value);
TShortCut GetShortCutCombo(TComboBox * ComboBox);
bool IsCustomShortCut(TShortCut ShortCut);

class TAnimationsModule;
TAnimationsModule * GetAnimationsModule();
void ReleaseAnimationsModule();
#endif
#ifdef _DEBUG
void ForceTracing();
#endif

#define HIDDEN_WINDOW_NAME L"WinSCPHiddenWindow2"

struct TCopyDataMessage
{
  enum { CommandCanCommandLine, CommandCommandLine };
  static const uintptr_t Version1 = 1;

  uintptr_t Version;
  uintptr_t Command;

  union
  {
    wchar_t CommandLine[10240];
  };
};

class TWinInteractiveCustomCommand : public TInteractiveCustomCommand
{
public:
  TWinInteractiveCustomCommand(TCustomCommand * ChildCustomCommand,
    const UnicodeString & CustomCommandName);

protected:
  virtual void Prompt(const UnicodeString & Prompt,
    UnicodeString & Value);
  virtual void Execute(const UnicodeString & Command,
    UnicodeString & Value);

private:
  UnicodeString FCustomCommandName;
};

