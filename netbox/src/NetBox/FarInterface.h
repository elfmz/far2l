//from windows/WinInterface.h
#pragma once
#include <Classes.hpp>
#include <Interface.h>
#include <GUIConfiguration.h>
#include <FarDialog.h>

enum TMsgDlgType
{
  mtConfirmation,
  mtInformation,
  mtError,
  mtWarning,
};

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
/*void InsertPanelToMessageDialog(TFarDialog * Form, TPanel * Panel);
void NavigateMessageDialogToUrl(TFarDialog * Form, const UnicodeString & Url);*/

//from windows/GUITools.h
void ValidateMaskEdit(TFarComboBox * Edit);
void ValidateMaskEdit(TFarEdit * Edit);
