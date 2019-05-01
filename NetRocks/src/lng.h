#pragma once

enum LanguageID
{
  MTitle,
  MDescription,
  MOK,
  MCancel,

  MLoginAuthTitle,
  MLoginAuthRetryTitle,
  MLoginAuthTo,
  MLoginAuthRetryTo,

  MProtocol,
  MHost,
  MPort,
  MLoginMode,
  MPasswordModeNoPassword,
  MPasswordModeAskPassword,
  MPasswordModeSavedPassword,
  MUserName,
  MPassword,
  MDirectory,
  MDisplayName,
  MAdvancedOptions,
  MSave,
  MConnect,

  MEditHost,

  MXferCopyDownloadTitle,
  MXferCopyUploadTitle,
  MXferMoveDownloadTitle,
  MXferMoveUploadTitle,

  MXferCopyDownloadText,
  MXferCopyUploadText,
  MXferMoveDownloadText,
  MXferMoveUploadText,

  MXferDOAText,

  MXferDOAAsk,
  MXferDOAOverwrite,

  MXferDOASkip,
  MXferDOAOverwriteIfNewer,

  MXferDOAResume,
  MXferDOACreateDifferentName,

  MXferCurrentFile,
  MXferFileSize,
  MXferAllSize,
  MXferCount,
  MXferOf,

  MXferFileTimeSpent,
  MXferRemain,
  MXferAllTimeSpent,
  MXferSpeedCurrent,
  MXferAverage, 
  MBackground,
  MPause,
  MResume,

  MProceedCopyDownload,
  MProceedCopyUpload,
  MProceedMoveDownload,
  MProceedMoveUpload,

  MDestinationExists,
  MSourceInfo,
  MDestinationInfo,
  MOverwriteOptions,
  MRememberSelection,

  MRemoveTitle,
  MRemoveText,
  MProceedRemoval,

  MMakeDirTitle,
  MMakeDirText,
  MProceedMakeDir,

  MConnectProgressTitle,
  MGetModeProgressTitle,
  MEnumDirProgressTitle,
  MCreateDirProgressTitle,

  MAbortTitle,
  MAbortText,
  MAbortConfirm,
  MAbortNotConfirm,

  MAbortingOperationTitle,
  MBreakConnection,

  MNotificationSuccess,
  MNotificationFailed,
  MNotificationUpload,
  MNotificationDownload
};
