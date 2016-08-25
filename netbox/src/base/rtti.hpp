#pragma once

#include <string>
#include <map>
#include "nbglobals.h"

class TObject;
class THashTable;

enum TObjectClassId
{
  OBJECT_CLASS_TObject = 1,
  OBJECT_CLASS_Exception,
  OBJECT_CLASS_ExtException,
  OBJECT_CLASS_EAbort,
  OBJECT_CLASS_EAccessViolation,
  OBJECT_CLASS_ESsh,
  OBJECT_CLASS_ETerminal,
  OBJECT_CLASS_ECommand,
  OBJECT_CLASS_EScp,
  OBJECT_CLASS_ESkipFile,
  OBJECT_CLASS_EFileSkipped,
  OBJECT_CLASS_EOSExtException,
  OBJECT_CLASS_EFatal,
  OBJECT_CLASS_ESshFatal,
  OBJECT_CLASS_ESshTerminate,
  OBJECT_CLASS_ECallbackGuardAbort,
  OBJECT_CLASS_EFileNotFoundError,
  OBJECT_CLASS_EOSError,
  OBJECT_CLASS_TPersistent,
  OBJECT_CLASS_TStrings,
  OBJECT_CLASS_TNamedObject,
  OBJECT_CLASS_TSessionData,
  OBJECT_CLASS_TDateTime,
  OBJECT_CLASS_TSHFileInfo,
  OBJECT_CLASS_TStream,
  OBJECT_CLASS_TRegistry,
  OBJECT_CLASS_TShortCut,
  OBJECT_CLASS_TDelphiSet,
  OBJECT_CLASS_TFormatSettings,
  OBJECT_CLASS_TCriticalSection,
  OBJECT_CLASS_TBookmarks,
  OBJECT_CLASS_TDateTimeParams,
  OBJECT_CLASS_TGuard,
  OBJECT_CLASS_TUnguard,
  OBJECT_CLASS_TValueRestorer,
  OBJECT_CLASS_TConfiguration,
  OBJECT_CLASS_TShortCuts,
  OBJECT_CLASS_TCopyParamType,
  OBJECT_CLASS_TFileBuffer,
  OBJECT_CLASS_TFileMasks,
  OBJECT_CLASS_TParams,
  OBJECT_CLASS_TFile,
  OBJECT_CLASS_TRemoteFile,
  OBJECT_CLASS_TList,
  OBJECT_CLASS_TObjectList,
  OBJECT_CLASS_TStringList,
  OBJECT_CLASS_TBookmarkList,
  OBJECT_CLASS_TBookmark,
  OBJECT_CLASS_TCustomCommand,
  OBJECT_CLASS_TCustomCommandData,
  OBJECT_CLASS_TFileOperationProgressType,
  OBJECT_CLASS_TSuspendFileOperationProgress,
  OBJECT_CLASS_TSinkFileParams,
  OBJECT_CLASS_TFileTransferData,
  OBJECT_CLASS_TClipboardHandler,
  OBJECT_CLASS_TOverwriteFileParams,
  OBJECT_CLASS_TOpenRemoteFileParams,
  OBJECT_CLASS_TCustomFileSystem,
  OBJECT_CLASS_TWebDAVFileSystem,
  OBJECT_CLASS_TMessageQueue,
  OBJECT_CLASS_TFTPFileListHelper,
  OBJECT_CLASS_THierarchicalStorage,
  OBJECT_CLASS_TQueryButtonAlias,
  OBJECT_CLASS_TQueryParams,
  OBJECT_CLASS_TNamedObjectList,
  OBJECT_CLASS_TOptions,
  OBJECT_CLASS_TUserAction,
  OBJECT_CLASS_TSimpleThread,
  OBJECT_CLASS_TSignalThread,
  OBJECT_CLASS_TQueueItem,
  OBJECT_CLASS_TInfo,
  OBJECT_CLASS_TQueueItemProxy,
  OBJECT_CLASS_TTerminalQueueStatus,
  OBJECT_CLASS_TRemoteToken,
  OBJECT_CLASS_TRemoteTokenList,
  OBJECT_CLASS_TRemoteFileList,
  OBJECT_CLASS_TRights,
  OBJECT_CLASS_TValidProperties,
  OBJECT_CLASS_TRemoteProperties,
  OBJECT_CLASS_TCommandSet,
  OBJECT_CLASS_TPoolForDataEvent,
  OBJECT_CLASS_TSecureShell,
  OBJECT_CLASS_TIEProxyConfig,
  OBJECT_CLASS_TSessionActionRecord,
  OBJECT_CLASS_TSessionInfo,
  OBJECT_CLASS_TFileSystemInfo,
  OBJECT_CLASS_TSessionAction,
  OBJECT_CLASS_TActionLog,
  OBJECT_CLASS_TSFTPSupport,
  OBJECT_CLASS_TSFTPPacket,
  OBJECT_CLASS_TSFTPQueuePacket,
  OBJECT_CLASS_TSFTPQueue,
  OBJECT_CLASS_TSFTPBusy,
  OBJECT_CLASS_TLoopDetector,
  OBJECT_CLASS_TMoveFileParams,
  OBJECT_CLASS_TFilesFindParams,
  OBJECT_CLASS_TCallbackGuard,
  OBJECT_CLASS_TOutputProxy,
  OBJECT_CLASS_TSynchronizeFileData,
  OBJECT_CLASS_TSynchronizeData,
  OBJECT_CLASS_TTerminal,
  OBJECT_CLASS_TTerminalList,
  OBJECT_CLASS_TTerminalItem,
  OBJECT_CLASS_TCustomCommandParams,
  OBJECT_CLASS_TCalculateSizeStats,
  OBJECT_CLASS_TCalculateSizeParams,
  OBJECT_CLASS_TMakeLocalFileListParams,
  OBJECT_CLASS_TSynchronizeOptions,
  OBJECT_CLASS_TSynchronizeChecklist,
  OBJECT_CLASS_TItem,
  OBJECT_CLASS_TChecklistItem,
  OBJECT_CLASS_TFileInfo,
  OBJECT_CLASS_TSpaceAvailable,
  OBJECT_CLASS_TWebDAVFileListHelper,
  OBJECT_CLASS_TCFileZillaApi,
  OBJECT_CLASS_TFTPServerCapabilities,
  OBJECT_CLASS_TCFileZillaTools,
  OBJECT_CLASS_T_directory,
  OBJECT_CLASS_T_direntry,
  OBJECT_CLASS_TCopyParamRuleData,
  OBJECT_CLASS_TCopyParamRule,
  OBJECT_CLASS_TCopyParamList,
  OBJECT_CLASS_TProgramParamsOwner,
  OBJECT_CLASS_TSynchronizeParamType,
  OBJECT_CLASS_TSynchronizeController,
  OBJECT_CLASS_TFarDialog,
  OBJECT_CLASS_TFarDialogContainer,
  OBJECT_CLASS_TFarDialogItem,
  OBJECT_CLASS_TConsoleTitleParam,
  OBJECT_CLASS_TFarMessageParams,
  OBJECT_CLASS_TCustomFarPlugin,
  OBJECT_CLASS_TCustomFarFileSystem,
  OBJECT_CLASS_TFarButton,
  OBJECT_CLASS_TTabButton,
  OBJECT_CLASS_TFarList,
  OBJECT_CLASS_TWinSCPPlugin,
  OBJECT_CLASS_TWinSCPFileSystem,
  OBJECT_CLASS_TFarPanelModes,
  OBJECT_CLASS_TFarKeyBarTitles,
  OBJECT_CLASS_TCustomFarPanelItem,
  OBJECT_CLASS_TFarPanelInfo,
  OBJECT_CLASS_TFarEditorInfo,
  OBJECT_CLASS_TFarEnvGuard,
  OBJECT_CLASS_TFarListBox,
  OBJECT_CLASS_TFarEdit,
  OBJECT_CLASS_TLabelList,
  OBJECT_CLASS_TGUIConfiguration,
  OBJECT_CLASS_TFarConfiguration,
  OBJECT_CLASS_TGUICopyParamType,
  OBJECT_CLASS_TCNBFile,
  OBJECT_CLASS_TMultipleEdit,
  OBJECT_CLASS_TEditHistory,
  OBJECT_CLASS_TFarMessageData,
  OBJECT_CLASS_TMessageParams,
  OBJECT_CLASS_TFileZillaIntern,
  OBJECT_CLASS_CApiLog,
  OBJECT_CLASS_TFarPanelItem,
  OBJECT_CLASS_TFarText,
  OBJECT_CLASS_TFarCheckBox,

};

class TClassInfo
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  TClassInfo(int classId,
             const TClassInfo * baseInfo1,
             const TClassInfo * baseInfo2);
  ~TClassInfo();

  int GetClassId() const { return m_classId; }
  int GetBaseClassId1() const
  { return m_baseInfo1 ? m_baseInfo1->GetClassId() : 0; }
  int GetBaseClassId2() const
  { return m_baseInfo2 ? m_baseInfo2->GetClassId() : 0; }
  const TClassInfo * GetBaseClassInfo1() const { return m_baseInfo1; }
  const TClassInfo * GetBaseClassInfo2() const { return m_baseInfo2; }

  static const TClassInfo * GetFirst() { return sm_first; }
  const TClassInfo * GetNext() const { return m_next; }
  static const TClassInfo * FindClass(int classId);

  // Climb upwards through inheritance hierarchy.
  bool IsKindOf(const TClassInfo * info) const;

private:
  int m_classId;

  const TClassInfo * m_baseInfo1;
  const TClassInfo * m_baseInfo2;

  // class info object live in a linked list:
  // pointers to its head and the next element in it
  static TClassInfo * sm_first;
  TClassInfo * m_next;

  static THashTable * sm_classTable;

protected:
  // registers the class
  void Register();
  void Unregister();

private:
  TClassInfo(const TClassInfo &);
  TClassInfo & operator=(const TClassInfo &);
};

#define NB_DECLARE_RUNTIME_CLASS(name)        \
public:                                       \
  static TClassInfo FClassInfo;               \
  virtual TClassInfo * GetClassInfo() const;

#define NB_DECLARE_CLASS(name)          \
  NB_DECLARE_RUNTIME_CLASS(name)        \

#define NB_GET_CLASS_INFO(name)         \
  &name::FClassInfo

#define NB_IMPLEMENT_CLASS(name, baseclassinfo1, baseclassinfo2) \
  TClassInfo name::FClassInfo(OBJECT_CLASS_##name,    \
    baseclassinfo1,                                   \
    baseclassinfo2);                                  \
  TClassInfo * name::GetClassInfo() const             \
  { return &name::FClassInfo; }

const TObject * NbStaticDownCastConst(TObjectClassId ClassId, const TObject * Object);
TObject * NbStaticDownCast(TObjectClassId ClassId, TObject * Object);

#define NB_STATIC_DOWNCAST_CONST(class_name, object) (static_cast<const class_name *>(NbStaticDownCastConst(OBJECT_CLASS_##class_name, static_cast<const TObject *>(object))))
#define NB_STATIC_DOWNCAST(class_name, object) (static_cast<class_name *>(NbStaticDownCast(OBJECT_CLASS_##class_name, static_cast<TObject *>(object))))

class THashTable : public std::map<int, const TClassInfo *>
{
CUSTOM_MEM_ALLOCATION_IMPL
typedef std::map<int, const TClassInfo *> ancestor;
public:
  ancestor::mapped_type Get(const ancestor::key_type & key) const
  {
    ancestor::const_iterator it = ancestor::find(key);
    if (it != ancestor::end())
      return it->second;
    else
      return nullptr;
  }
  void Put(const ancestor::key_type & key, ancestor::mapped_type value)
  {
    ancestor::insert(ancestor::value_type(key, value));
  }
  void Delete(const ancestor::key_type & key)
  {
    ancestor::iterator it = ancestor::find(key);
    if (it != ancestor::end())
      ancestor::erase(it);
  }
  size_t GetCount() const { return ancestor::size(); }
};

