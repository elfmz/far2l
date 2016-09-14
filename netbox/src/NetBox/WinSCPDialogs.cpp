#include <vcl.h>


#include "WinSCPPlugin.h"
#include "WinSCPFileSystem.h"
#include "FarDialog.h"
#include "FarConfiguration.h"
#include "FarInterface.h"

#ifndef __linux__
#include <shellapi.h>
#endif
#include <PuttyTools.h>
#include <GUITools.h>
#include <Tools.h>
#include <CoreMain.h>
#include <Common.h>
#include <CopyParam.h>
#include <TextsCore.h>
#include <Terminal.h>
#include <Bookmarks.h>
#include <Queue.h>
#include <MsgIDs.h>
#include <farkeys.hpp>
#include <farcolor.hpp>
#include "plugin_version.hpp"

enum TButtonResult
{
  brCancel = -1,
  brOK = 1,
  brConnect
};

class TWinSCPDialog : public TFarDialog
{
public:
  explicit TWinSCPDialog(TCustomFarPlugin * AFarPlugin);

  void AddStandardButtons(int Shift = 0, bool ButtonsOnly = false);

  TFarSeparator * ButtonSeparator;
  TFarButton * OkButton;
  TFarButton * CancelButton;
};

TWinSCPDialog::TWinSCPDialog(TCustomFarPlugin * AFarPlugin) :
  TFarDialog(AFarPlugin),
  ButtonSeparator(nullptr),
  OkButton(nullptr),
  CancelButton(nullptr)
{
}

void TWinSCPDialog::AddStandardButtons(int Shift, bool ButtonsOnly)
{
  if (!ButtonsOnly)
  {
    SetNextItemPosition(ipNewLine);

    ButtonSeparator = new TFarSeparator(this);
    if (Shift >= 0)
    {
      ButtonSeparator->Move(0, Shift);
    }
    else
    {
      ButtonSeparator->SetTop(Shift);
      ButtonSeparator->SetBottom(Shift);
    }
  }

  DebugAssert(OkButton == nullptr);
  OkButton = new TFarButton(this);
  if (ButtonsOnly)
  {
    if (Shift >= 0)
    {
      OkButton->Move(0, Shift);
    }
    else
    {
      OkButton->SetTop(Shift);
      OkButton->SetBottom(Shift);
    }
  }
  OkButton->SetCaption(GetMsg(MSG_BUTTON_OK));
  OkButton->SetDefault(true);
  OkButton->SetResult(brOK);
  OkButton->SetCenterGroup(true);

  SetNextItemPosition(ipRight);

  DebugAssert(CancelButton == nullptr);
  CancelButton = new TFarButton(this);
  CancelButton->SetCaption(GetMsg(MSG_BUTTON_Cancel));
  CancelButton->SetResult(brCancel);
  CancelButton->SetCenterGroup(true);
}

class TTabButton;
class TTabbedDialog : public TWinSCPDialog
{
  friend class TTabButton;

public:
  explicit TTabbedDialog(TCustomFarPlugin * AFarPlugin, int TabCount);
  virtual ~TTabbedDialog() {}

  intptr_t GetTab() const { return FTab; }

protected:
  void HideTabs();
  virtual void SelectTab(intptr_t Tab);
  void TabButtonClick(TFarButton * Sender, bool & Close);
  virtual bool Key(TFarDialogItem * Item, LONG_PTR KeyCode);
  virtual UnicodeString GetTabName(intptr_t Tab) const;
  TTabButton * GetTabButton(intptr_t Tab) const;
  intptr_t GetTabCount() const { return FTabCount; }

private:
  UnicodeString FOrigCaption;
  intptr_t FTab;
  intptr_t FTabCount;
};

class TTabButton : public TFarButton
{
NB_DECLARE_CLASS(TTabButton)
public:
  explicit TTabButton(TTabbedDialog * Dialog);

  intptr_t GetTab() const { return FTab; }
  void SetTab(intptr_t Value) { FTab = Value; }
  UnicodeString GetTabName() const { return FTabName; }
  void SetTabName(const UnicodeString & Value);

private:
  UnicodeString FTabName;
  intptr_t FTab;
};

TTabbedDialog::TTabbedDialog(TCustomFarPlugin * AFarPlugin, int TabCount) :
  TWinSCPDialog(AFarPlugin),
  FTab(0),
  FTabCount(TabCount)
{
  // FAR WORKAROUND
  // (to avoid first control on dialog be a button, that would be "pressed"
  // when listbox loses focus)
  TFarText * Text = new TFarText(this);
  // make next item be inserted to default position
  Text->Move(0, -1);
  // on FAR 1.70 alpha 6 and later, empty text control would overwrite the
  // dialog box caption
  Text->SetVisible(false);
}

void TTabbedDialog::HideTabs()
{
  for (intptr_t Index = 0; Index < GetItemCount(); ++Index)
  {
    TFarDialogItem * Item = GetItem(Index);
    if (Item->GetGroup())
    {
      Item->SetVisible(false);
    }
  }
}

void TTabbedDialog::SelectTab(intptr_t Tab)
{
  /*for (intptr_t I = FTabCount - 1; I >= 1; I--)
  {
    TTabButton * Button = TabButton(I);
    Button->SetBrackets(Button->GetTab() == Tab ? brTight : brNone);
  }*/
  if (FTab != Tab)
  {
    if (FTab)
    {
      ShowGroup(FTab, false);
    }
    ShowGroup(Tab, true);
    FTab = Tab;
  }

  for (intptr_t Index = 0; Index < GetItemCount(); ++Index)
  {
    TFarDialogItem * Item = GetItem(Index);
    if ((Item->GetGroup() == Tab) && Item->CanFocus())
    {
      Item->SetFocus();
      break;
    }
  }

  if (FOrigCaption.IsEmpty())
  {
    FOrigCaption = GetCaption();
  }
  SetCaption(FORMAT(L"%s - %s", GetTabName(Tab).c_str(), FOrigCaption.c_str()));
}

TTabButton * TTabbedDialog::GetTabButton(intptr_t Tab) const
{
  TTabButton * Result = nullptr;
  for (intptr_t Index = 0; Index < GetItemCount(); ++Index)
  {
    TTabButton * T = NB_STATIC_DOWNCAST(TTabButton, GetItem(Index));
    if ((T != nullptr) && (T->GetTab() == Tab))
    {
      Result = T;
      break;
    }
  }

  if (!Result)
  {
    DEBUG_PRINTF("Tab = %d", Tab);
  }
  DebugAssert(Result != nullptr);

  return Result;
}

UnicodeString TTabbedDialog::GetTabName(intptr_t Tab) const
{
  return GetTabButton(Tab)->GetTabName();
}

void TTabbedDialog::TabButtonClick(TFarButton * Sender, bool & Close)
{
  TTabButton * Tab = NB_STATIC_DOWNCAST(TTabButton, Sender);
  DebugAssert(Tab != nullptr);

  // HideTabs();
  SelectTab(Tab->GetTab());

  Close = false;
}

bool TTabbedDialog::Key(TFarDialogItem * /*Item*/, LONG_PTR KeyCode)
{
  bool Result = false;
  if ((KeyCode == KEY_CTRLPGDN) || (KeyCode == KEY_CTRLPGUP) ||
      (KeyCode == KEY_CTRLNUMPAD3) || (KeyCode == KEY_CTRLNUMPAD9))
  {
    intptr_t NewTab = FTab;
    do
    {
      if ((KeyCode == KEY_CTRLPGDN) || (KeyCode == KEY_CTRLNUMPAD3))
      {
        NewTab = NewTab == FTabCount - 1 ? 1 : NewTab + 1;
      }
      else
      {
        NewTab = NewTab == 1 ? FTabCount - 1 : NewTab - 1;
      }
    }
    while (!GetTabButton(NewTab)->GetEnabled());
    SelectTab(NewTab);
    Result = true;
  }
  return Result;
}

TTabButton::TTabButton(TTabbedDialog * Dialog) :
  TFarButton(Dialog),
  FTab(0)
{
  SetCenterGroup(true);
  SetOnClick(MAKE_CALLBACK(TTabbedDialog::TabButtonClick, Dialog));
}

void TTabButton::SetTabName(const UnicodeString & Value)
{
  UnicodeString Val = Value;
  if (FTabName != Val)
  {
    UnicodeString C;
    intptr_t P = ::Pos(Val, L"|");
    if (P > 0)
    {
      C = Val.SubString(1, P - 1);
      Val.Delete(1, P);
    }
    else
    {
      C = Val;
    }
    SetCaption(C);
    FTabName = ::StripHotkey(Val);
  }
}

bool TWinSCPPlugin::ConfigurationDialog()
{
  std::unique_ptr<TWinSCPDialog> DialogPtr(new TWinSCPDialog(this));
  TWinSCPDialog * Dialog = DialogPtr.get();

  TFarText * Text;

  Dialog->SetSize(TPoint(67, 22));
  Dialog->SetCaption(FORMAT(L"%s - %s",
    GetMsg(PLUGIN_TITLE).c_str(), ::StripHotkey(GetMsg(CONFIG_INTERFACE)).c_str()));

  TFarCheckBox * DisksMenuCheck = new TFarCheckBox(Dialog);
  DisksMenuCheck->SetCaption(GetMsg(CONFIG_DISKS_MENU));

  Dialog->SetNextItemPosition(ipNewLine);

  TFarCheckBox * PluginsMenuCheck = new TFarCheckBox(Dialog);
  PluginsMenuCheck->SetCaption(GetMsg(CONFIG_PLUGINS_MENU));

  TFarCheckBox * PluginsMenuCommandsCheck = new TFarCheckBox(Dialog);
  PluginsMenuCommandsCheck->SetCaption(GetMsg(CONFIG_PLUGINS_MENU_COMMANDS));

  TFarCheckBox * HostNameInTitleCheck = new TFarCheckBox(Dialog);
  HostNameInTitleCheck->SetCaption(GetMsg(CONFIG_HOST_NAME_IN_TITLE));

  new TFarSeparator(Dialog);

  Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(CONFIG_COMAND_PREFIXES));

  TFarEdit * CommandPrefixesEdit = new TFarEdit(Dialog);

  new TFarSeparator(Dialog);

  TFarCheckBox * CustomPanelCheck = new TFarCheckBox(Dialog);
  CustomPanelCheck->SetCaption(GetMsg(CONFIG_PANEL_MODE_CHECK));
  CustomPanelCheck->SetEnabled(true);

  Text = new TFarText(Dialog);
  Text->SetLeft(Text->GetLeft() + 4);
  Text->SetEnabledDependency(CustomPanelCheck);
  Text->SetCaption(GetMsg(CONFIG_PANEL_MODE_TYPES));

  Dialog->SetNextItemPosition(ipBelow);

  TFarEdit * CustomPanelTypesEdit = new TFarEdit(Dialog);
  CustomPanelTypesEdit->SetEnabledDependency(CustomPanelCheck);
  CustomPanelTypesEdit->SetWidth(CustomPanelTypesEdit->GetWidth() / 2 - 1);

  Dialog->SetNextItemPosition(ipRight);

  Text = new TFarText(Dialog);
  Text->SetEnabledDependency(CustomPanelCheck);
  Text->Move(0, -1);
  Text->SetCaption(GetMsg(CONFIG_PANEL_MODE_STATUS_TYPES));

  Dialog->SetNextItemPosition(ipBelow);

  TFarEdit * CustomPanelStatusTypesEdit = new TFarEdit(Dialog);
  CustomPanelStatusTypesEdit->SetEnabledDependency(CustomPanelCheck);

  Dialog->SetNextItemPosition(ipNewLine);

  Text = new TFarText(Dialog);
  Text->SetLeft(Text->GetLeft() + 4);
  Text->SetEnabledDependency(CustomPanelCheck);
  Text->SetCaption(GetMsg(CONFIG_PANEL_MODE_WIDTHS));

  Dialog->SetNextItemPosition(ipBelow);

  TFarEdit * CustomPanelWidthsEdit = new TFarEdit(Dialog);
  CustomPanelWidthsEdit->SetEnabledDependency(CustomPanelCheck);
  CustomPanelWidthsEdit->SetWidth(CustomPanelTypesEdit->GetWidth());

  Dialog->SetNextItemPosition(ipRight);

  Text = new TFarText(Dialog);
  Text->SetEnabledDependency(CustomPanelCheck);
  Text->Move(0, -1);
  Text->SetCaption(GetMsg(CONFIG_PANEL_MODE_STATUS_WIDTHS));

  Dialog->SetNextItemPosition(ipBelow);

  TFarEdit * CustomPanelStatusWidthsEdit = new TFarEdit(Dialog);
  CustomPanelStatusWidthsEdit->SetEnabledDependency(CustomPanelCheck);

  Dialog->SetNextItemPosition(ipNewLine);

  TFarCheckBox * CustomPanelFullScreenCheck = new TFarCheckBox(Dialog);
  CustomPanelFullScreenCheck->SetLeft(CustomPanelFullScreenCheck->GetLeft() + 4);
  CustomPanelFullScreenCheck->SetEnabledDependency(CustomPanelCheck);
  CustomPanelFullScreenCheck->SetCaption(GetMsg(CONFIG_PANEL_MODE_FULL_SCREEN));

  Text = new TFarText(Dialog);
  Text->SetLeft(Text->GetLeft());
  Text->SetEnabledDependency(CustomPanelCheck);
  Text->SetCaption(GetMsg(CONFIG_PANEL_MODE_HINT));
  Text = new TFarText(Dialog);
  Text->SetLeft(Text->GetLeft());
  Text->SetEnabledDependency(CustomPanelCheck);
  Text->SetCaption(GetMsg(CONFIG_PANEL_MODE_HINT2));

  Dialog->AddStandardButtons();

  TFarConfiguration * FarConfiguration = GetFarConfiguration();
  DisksMenuCheck->SetChecked(FarConfiguration->GetDisksMenu());
  PluginsMenuCheck->SetChecked(FarConfiguration->GetPluginsMenu());
  PluginsMenuCommandsCheck->SetChecked(FarConfiguration->GetPluginsMenuCommands());
  HostNameInTitleCheck->SetChecked(FarConfiguration->GetHostNameInTitle());
  CommandPrefixesEdit->SetText(FarConfiguration->GetCommandPrefixes());

  CustomPanelCheck->SetChecked(FarConfiguration->GetCustomPanelModeDetailed());
  CustomPanelTypesEdit->SetText(FarConfiguration->GetColumnTypesDetailed());
  CustomPanelWidthsEdit->SetText(FarConfiguration->GetColumnWidthsDetailed());
  CustomPanelStatusTypesEdit->SetText(FarConfiguration->GetStatusColumnTypesDetailed());
  CustomPanelStatusWidthsEdit->SetText(FarConfiguration->GetStatusColumnWidthsDetailed());
  CustomPanelFullScreenCheck->SetChecked(FarConfiguration->GetFullScreenDetailed());

  bool Result = (Dialog->ShowModal() == brOK);
  if (Result)
  {
    FarConfiguration->SetDisksMenu(DisksMenuCheck->GetChecked());
    FarConfiguration->SetPluginsMenu(PluginsMenuCheck->GetChecked());
    FarConfiguration->SetPluginsMenuCommands(PluginsMenuCommandsCheck->GetChecked());
    FarConfiguration->SetHostNameInTitle(HostNameInTitleCheck->GetChecked());

    FarConfiguration->SetCommandPrefixes(CommandPrefixesEdit->GetText());

    FarConfiguration->SetCustomPanelModeDetailed(CustomPanelCheck->GetChecked());
    FarConfiguration->SetColumnTypesDetailed(CustomPanelTypesEdit->GetText());
    FarConfiguration->SetColumnWidthsDetailed(CustomPanelWidthsEdit->GetText());
    FarConfiguration->SetStatusColumnTypesDetailed(CustomPanelStatusTypesEdit->GetText());
    FarConfiguration->SetStatusColumnWidthsDetailed(CustomPanelStatusWidthsEdit->GetText());
    FarConfiguration->SetFullScreenDetailed(CustomPanelFullScreenCheck->GetChecked());
  }
  return Result;
}

bool TWinSCPPlugin::PanelConfigurationDialog()
{
  std::unique_ptr<TWinSCPDialog> Dialog(new TWinSCPDialog(this));
  Dialog->SetSize(TPoint(65, 7));
  Dialog->SetCaption(FORMAT(L"%s - %s",
    GetMsg(PLUGIN_TITLE).c_str(), ::StripHotkey(GetMsg(CONFIG_PANEL)).c_str()));

  TFarCheckBox * AutoReadDirectoryAfterOpCheck = new TFarCheckBox(Dialog.get());
  AutoReadDirectoryAfterOpCheck->SetCaption(GetMsg(CONFIG_AUTO_READ_DIRECTORY_AFTER_OP));

  Dialog->AddStandardButtons();

  AutoReadDirectoryAfterOpCheck->SetChecked(GetConfiguration()->GetAutoReadDirectoryAfterOp());

  bool Result = (Dialog->ShowModal() == brOK);

  if (Result)
  {
    GetConfiguration()->BeginUpdate();
    SCOPE_EXIT
    {
      GetConfiguration()->EndUpdate();
    };
    GetConfiguration()->SetAutoReadDirectoryAfterOp(AutoReadDirectoryAfterOpCheck->GetChecked());
  }
  return Result;
}

bool TWinSCPPlugin::LoggingConfigurationDialog()
{
  std::unique_ptr<TWinSCPDialog> DialogPtr(new TWinSCPDialog(this));
  TWinSCPDialog * Dialog = DialogPtr.get();

  TFarSeparator * Separator;
  TFarText * Text;

  Dialog->SetSize(TPoint(65, 15));
  Dialog->SetCaption(FORMAT(L"%s - %s",
    GetMsg(PLUGIN_TITLE).c_str(), ::StripHotkey(GetMsg(CONFIG_LOGGING)).c_str()));

  TFarCheckBox * LoggingCheck = new TFarCheckBox(Dialog);
  LoggingCheck->SetCaption(GetMsg(LOGGING_ENABLE));

  Separator = new TFarSeparator(Dialog);
  Separator->SetCaption(GetMsg(LOGGING_OPTIONS_GROUP));

  Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(LOGGING_LOG_PROTOCOL));
  Text->SetEnabledDependency(LoggingCheck);

  Dialog->SetNextItemPosition(ipRight);

  TFarComboBox * LogProtocolCombo = new TFarComboBox(Dialog);
  LogProtocolCombo->SetDropDownList(true);
  LogProtocolCombo->SetWidth(10);
  for (intptr_t Index = 0; Index <= 2; ++Index)
  {
    LogProtocolCombo->GetItems()->Add(GetMsg(LOGGING_LOG_PROTOCOL_0 + Index));
  }
  LogProtocolCombo->SetEnabledDependency(LoggingCheck);

  Dialog->SetNextItemPosition(ipNewLine);

  new TFarSeparator(Dialog);

  TFarCheckBox * LogToFileCheck = new TFarCheckBox(Dialog);
  LogToFileCheck->SetCaption(GetMsg(LOGGING_LOG_TO_FILE));
  LogToFileCheck->SetEnabledDependency(LoggingCheck);

  TFarEdit * LogFileNameEdit = new TFarEdit(Dialog);
  LogFileNameEdit->SetLeft(LogFileNameEdit->GetLeft() + 4);
  LogFileNameEdit->SetHistory(LOG_FILE_HISTORY);
  LogFileNameEdit->SetEnabledDependency(LogToFileCheck);

  Dialog->SetNextItemPosition(ipBelow);

  Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(LOGGING_LOG_FILE_HINT1));
  Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(LOGGING_LOG_FILE_HINT2));

  TFarRadioButton * LogFileAppendButton = new TFarRadioButton(Dialog);
  LogFileAppendButton->SetCaption(GetMsg(LOGGING_LOG_FILE_APPEND));
  LogFileAppendButton->SetEnabledDependency(LogToFileCheck);

  Dialog->SetNextItemPosition(ipRight);

  TFarRadioButton * LogFileOverwriteButton = new TFarRadioButton(Dialog);
  LogFileOverwriteButton->SetCaption(GetMsg(LOGGING_LOG_FILE_OVERWRITE));
  LogFileOverwriteButton->SetEnabledDependency(LogToFileCheck);

  Dialog->AddStandardButtons();

  LoggingCheck->SetChecked(GetConfiguration()->GetLogging());
  LogProtocolCombo->SetItemIndex(GetConfiguration()->GetLogProtocol());
  LogToFileCheck->SetChecked(GetConfiguration()->GetLogToFile());
  LogFileNameEdit->SetText(
    (!GetConfiguration()->GetLogToFile() && GetConfiguration()->GetLogFileName().IsEmpty()) ?
    ::IncludeTrailingBackslash(GetSystemTemporaryDirectory()) + L"&s.log" :
    GetConfiguration()->GetLogFileName());
  LogFileAppendButton->SetChecked(GetConfiguration()->GetLogFileAppend());
  LogFileOverwriteButton->SetChecked(!GetConfiguration()->GetLogFileAppend());

  bool Result = (Dialog->ShowModal() == brOK);

  if (Result)
  {
    GetConfiguration()->BeginUpdate();
    SCOPE_EXIT
    {
      GetConfiguration()->EndUpdate();
    };
    GetConfiguration()->SetLogging(LoggingCheck->GetChecked());
    GetConfiguration()->SetLogProtocol(LogProtocolCombo->GetItemIndex());
    GetConfiguration()->SetLogToFile(LogToFileCheck->GetChecked());
    if (LogToFileCheck->GetChecked())
    {
      GetConfiguration()->SetLogFileName(LogFileNameEdit->GetText());
    }
    GetConfiguration()->SetLogFileAppend(LogFileAppendButton->GetChecked());
  }
  return Result;
}

bool TWinSCPPlugin::TransferConfigurationDialog()
{
  UnicodeString Caption = FORMAT(L"%s - %s",
    GetMsg(PLUGIN_TITLE).c_str(), ::StripHotkey(GetMsg(CONFIG_TRANSFER)).c_str());

  TGUICopyParamType & CopyParam = GetGUIConfiguration()->GetDefaultCopyParam();
  bool Result = CopyParamDialog(Caption, CopyParam, 0);
  if (Result)
  {
    GetGUIConfiguration()->SetDefaultCopyParam(CopyParam);
  }

  return Result;
}

bool TWinSCPPlugin::EnduranceConfigurationDialog()
{
  std::unique_ptr<TWinSCPDialog> DialogPtr(new TWinSCPDialog(this));
  TWinSCPDialog * Dialog = DialogPtr.get();

  TFarSeparator * Separator;
  TFarText * Text;

  Dialog->SetSize(TPoint(76, 13));
  Dialog->SetCaption(FORMAT(L"%s - %s",
    GetMsg(PLUGIN_TITLE).c_str(), ::StripHotkey(GetMsg(CONFIG_ENDURANCE)).c_str()));

  Separator = new TFarSeparator(Dialog);
  Separator->SetCaption(GetMsg(TRANSFER_RESUME));

  TFarRadioButton * ResumeOnButton = new TFarRadioButton(Dialog);
  ResumeOnButton->SetCaption(GetMsg(TRANSFER_RESUME_ON));

  TFarRadioButton * ResumeSmartButton = new TFarRadioButton(Dialog);
  ResumeSmartButton->SetCaption(GetMsg(TRANSFER_RESUME_SMART));
  intptr_t ResumeThresholdLeft = ResumeSmartButton->GetRight();

  TFarRadioButton * ResumeOffButton = new TFarRadioButton(Dialog);
  ResumeOffButton->SetCaption(GetMsg(TRANSFER_RESUME_OFF));

  TFarEdit * ResumeThresholdEdit = new TFarEdit(Dialog);
  ResumeThresholdEdit->Move(0, -2);
  ResumeThresholdEdit->SetLeft(ResumeThresholdLeft + 3);
  ResumeThresholdEdit->SetFixed(true);
  ResumeThresholdEdit->SetMask(L"9999999");
  ResumeThresholdEdit->SetWidth(9);
  ResumeThresholdEdit->SetEnabledDependency(ResumeSmartButton);

  Dialog->SetNextItemPosition(ipRight);

  Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(TRANSFER_RESUME_THRESHOLD_UNIT));
  Text->SetEnabledDependency(ResumeSmartButton);

  Dialog->SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(Dialog);
  Separator->SetCaption(GetMsg(TRANSFER_SESSION_REOPEN_GROUP));
  Separator->Move(0, 1);

  TFarCheckBox * SessionReopenAutoCheck = new TFarCheckBox(Dialog);
  SessionReopenAutoCheck->SetCaption(GetMsg(TRANSFER_SESSION_REOPEN_AUTO_LABEL));

  Dialog->SetNextItemPosition(ipRight);

  TFarEdit * SessionReopenAutoEdit = new TFarEdit(Dialog);
  SessionReopenAutoEdit->SetEnabledDependency(SessionReopenAutoCheck);
  SessionReopenAutoEdit->SetFixed(true);
  SessionReopenAutoEdit->SetMask(L"999");
  SessionReopenAutoEdit->SetWidth(5);
  SessionReopenAutoEdit->Move(12, 0);

  Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(TRANSFER_SESSION_REOPEN_AUTO_LABEL2));
  Text->SetEnabledDependency(SessionReopenAutoCheck);

  Dialog->SetNextItemPosition(ipNewLine);

  Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(TRANSFER_SESSION_REOPEN_NUMBER_OF_RETRIES_LABEL));
  Text->SetEnabledDependency(SessionReopenAutoCheck);
  Text->Move(4, 0);

  Dialog->SetNextItemPosition(ipRight);

  TFarEdit * SessionReopenNumberOfRetriesEdit = new TFarEdit(Dialog);
  SessionReopenNumberOfRetriesEdit->SetEnabledDependency(SessionReopenAutoCheck);
  SessionReopenNumberOfRetriesEdit->SetFixed(true);
  SessionReopenNumberOfRetriesEdit->SetMask(L"999");
  SessionReopenNumberOfRetriesEdit->SetWidth(5);

  Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(TRANSFER_SESSION_REOPEN_NUMBER_OF_RETRIES_LABEL2));
  Text->SetEnabledDependency(SessionReopenAutoCheck);

  Dialog->AddStandardButtons();

  TGUICopyParamType & CopyParam = GetGUIConfiguration()->GetDefaultCopyParam();
  ResumeOnButton->SetChecked(CopyParam.GetResumeSupport() == rsOn);
  ResumeSmartButton->SetChecked(CopyParam.GetResumeSupport() == rsSmart);
  ResumeOffButton->SetChecked(CopyParam.GetResumeSupport() == rsOff);
  ResumeThresholdEdit->SetAsInteger(
    static_cast<int>(CopyParam.GetResumeThreshold() / 1024));

  SessionReopenAutoCheck->SetChecked((GetConfiguration()->GetSessionReopenAuto() > 0));
  SessionReopenAutoEdit->SetAsInteger((GetConfiguration()->GetSessionReopenAuto() > 0 ?
    (GetConfiguration()->GetSessionReopenAuto() / 1000) : 5));
  intptr_t Value = GetConfiguration()->GetSessionReopenAutoMaximumNumberOfRetries();
  SessionReopenNumberOfRetriesEdit->SetAsInteger(Value < 0 || Value > 99 ?
    CONST_DEFAULT_NUMBER_OF_RETRIES : Value);

  bool Result = (Dialog->ShowModal() == brOK);

  if (Result)
  {
    GetConfiguration()->BeginUpdate();
    {
      SCOPE_EXIT
      {
        GetConfiguration()->EndUpdate();
      };
      if (ResumeOnButton->GetChecked())
      {
        CopyParam.SetResumeSupport(rsOn);
      }
      if (ResumeSmartButton->GetChecked())
      {
        CopyParam.SetResumeSupport(rsSmart);
      }
      if (ResumeOffButton->GetChecked())
      {
        CopyParam.SetResumeSupport(rsOff);
      }
      CopyParam.SetResumeThreshold(ResumeThresholdEdit->GetAsInteger() * 1024);

      GetGUIConfiguration()->SetDefaultCopyParam(CopyParam);

      GetConfiguration()->SetSessionReopenAuto(
        (SessionReopenAutoCheck->GetChecked() ? (SessionReopenAutoEdit->GetAsInteger() * 1000) : 0));
      GetConfiguration()->SetSessionReopenAutoMaximumNumberOfRetries(
        (SessionReopenAutoCheck->GetChecked() ? SessionReopenNumberOfRetriesEdit->GetAsInteger() : CONST_DEFAULT_NUMBER_OF_RETRIES));
    }
  }
  return Result;
}

bool TWinSCPPlugin::QueueConfigurationDialog()
{
  std::unique_ptr<TWinSCPDialog> DialogPtr(new TWinSCPDialog(this));
  TWinSCPDialog * Dialog = DialogPtr.get();

  TFarText * Text;

  Dialog->SetSize(TPoint(76, 11));
  Dialog->SetCaption(FORMAT(L"%s - %s",
    GetMsg(PLUGIN_TITLE).c_str(), ::StripHotkey(GetMsg(CONFIG_BACKGROUND)).c_str()));

  Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(TRANSFER_QUEUE_LIMIT));

  Dialog->SetNextItemPosition(ipRight);

  TFarEdit * QueueTransferLimitEdit = new TFarEdit(Dialog);
  QueueTransferLimitEdit->SetFixed(true);
  QueueTransferLimitEdit->SetMask(L"9");
  QueueTransferLimitEdit->SetWidth(3);

  Dialog->SetNextItemPosition(ipNewLine);

  TFarCheckBox * QueueCheck = new TFarCheckBox(Dialog);
  QueueCheck->SetCaption(GetMsg(TRANSFER_QUEUE_DEFAULT));

  TFarCheckBox * QueueAutoPopupCheck = new TFarCheckBox(Dialog);
  QueueAutoPopupCheck->SetCaption(GetMsg(TRANSFER_AUTO_POPUP));

  TFarCheckBox * RememberPasswordCheck = new TFarCheckBox(Dialog);
  RememberPasswordCheck->SetCaption(GetMsg(TRANSFER_REMEMBER_PASSWORD));

  TFarCheckBox * QueueBeepCheck = new TFarCheckBox(Dialog);
  QueueBeepCheck->SetCaption(GetMsg(TRANSFER_QUEUE_BEEP));

  Dialog->AddStandardButtons();

  TFarConfiguration * FarConfiguration = GetFarConfiguration();
  QueueTransferLimitEdit->SetAsInteger(FarConfiguration->GetQueueTransfersLimit());
  QueueCheck->SetChecked(FarConfiguration->GetDefaultCopyParam().GetQueue());
  QueueAutoPopupCheck->SetChecked(FarConfiguration->GetQueueAutoPopup());
  RememberPasswordCheck->SetChecked(GetGUIConfiguration()->GetSessionRememberPassword());
  QueueBeepCheck->SetChecked(FarConfiguration->GetQueueBeep());

  bool Result = (Dialog->ShowModal() == brOK);

  if (Result)
  {
    GetConfiguration()->BeginUpdate();
    SCOPE_EXIT
    {
      GetConfiguration()->EndUpdate();
    };
    TGUICopyParamType & CopyParam = GetGUIConfiguration()->GetDefaultCopyParam();

    FarConfiguration->SetQueueTransfersLimit(QueueTransferLimitEdit->GetAsInteger());
    CopyParam.SetQueue(QueueCheck->GetChecked());
    FarConfiguration->SetQueueAutoPopup(QueueAutoPopupCheck->GetChecked());
    GetGUIConfiguration()->SetSessionRememberPassword(RememberPasswordCheck->GetChecked());
    FarConfiguration->SetQueueBeep(QueueBeepCheck->GetChecked());

    GetGUIConfiguration()->SetDefaultCopyParam(CopyParam);
  }
  return Result;
}

class TTransferEditorConfigurationDialog : public TWinSCPDialog
{
public:
  explicit TTransferEditorConfigurationDialog(TCustomFarPlugin * AFarPlugin);

  bool Execute();

protected:
  virtual void Change();

private:
  virtual void UpdateControls();

private:
  TFarCheckBox * EditorMultipleCheck;
  TFarCheckBox * EditorUploadOnSaveCheck;
  TFarRadioButton * EditorDownloadDefaultButton;
  TFarRadioButton * EditorDownloadOptionsButton;
  TFarRadioButton * EditorUploadSameButton;
  TFarRadioButton * EditorUploadOptionsButton;
};

TTransferEditorConfigurationDialog::TTransferEditorConfigurationDialog(
  TCustomFarPlugin * AFarPlugin) :
  TWinSCPDialog(AFarPlugin)
{
  TFarSeparator * Separator;

  SetSize(TPoint(65, 14));
  SetCaption(FORMAT(L"%s - %s",
    GetMsg(PLUGIN_TITLE).c_str(), ::StripHotkey(GetMsg(CONFIG_TRANSFER_EDITOR)).c_str()));

  EditorMultipleCheck = new TFarCheckBox(this);
  EditorMultipleCheck->SetCaption(GetMsg(TRANSFER_EDITOR_MULTIPLE));

  EditorUploadOnSaveCheck = new TFarCheckBox(this);
  EditorUploadOnSaveCheck->SetCaption(GetMsg(TRANSFER_EDITOR_UPLOAD_ON_SAVE));

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(TRANSFER_EDITOR_DOWNLOAD));

  EditorDownloadDefaultButton = new TFarRadioButton(this);
  EditorDownloadDefaultButton->SetCaption(GetMsg(TRANSFER_EDITOR_DOWNLOAD_DEFAULT));

  EditorDownloadOptionsButton = new TFarRadioButton(this);
  EditorDownloadOptionsButton->SetCaption(GetMsg(TRANSFER_EDITOR_DOWNLOAD_OPTIONS));

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(TRANSFER_EDITOR_UPLOAD));

  EditorUploadSameButton = new TFarRadioButton(this);
  EditorUploadSameButton->SetCaption(GetMsg(TRANSFER_EDITOR_UPLOAD_SAME));

  EditorUploadOptionsButton = new TFarRadioButton(this);
  EditorUploadOptionsButton->SetCaption(GetMsg(TRANSFER_EDITOR_UPLOAD_OPTIONS));

  AddStandardButtons();
}

bool TTransferEditorConfigurationDialog::Execute()
{
  TFarConfiguration * FarConfiguration = GetFarConfiguration();
  EditorDownloadDefaultButton->SetChecked(FarConfiguration->GetEditorDownloadDefaultMode());
  EditorDownloadOptionsButton->SetChecked(!FarConfiguration->GetEditorDownloadDefaultMode());
  EditorUploadSameButton->SetChecked(FarConfiguration->GetEditorUploadSameOptions());
  EditorUploadOptionsButton->SetChecked(!FarConfiguration->GetEditorUploadSameOptions());
  EditorUploadOnSaveCheck->SetChecked(FarConfiguration->GetEditorUploadOnSave());
  EditorMultipleCheck->SetChecked(FarConfiguration->GetEditorMultiple());

  bool Result = (ShowModal() == brOK);

  if (Result)
  {
    GetConfiguration()->BeginUpdate();
    SCOPE_EXIT
    {
      GetConfiguration()->EndUpdate();
    };
    FarConfiguration->SetEditorDownloadDefaultMode(EditorDownloadDefaultButton->GetChecked());
    FarConfiguration->SetEditorUploadSameOptions(EditorUploadSameButton->GetChecked());
    FarConfiguration->SetEditorUploadOnSave(EditorUploadOnSaveCheck->GetChecked());
    FarConfiguration->SetEditorMultiple(EditorMultipleCheck->GetChecked());
  }

  return Result;
}

void TTransferEditorConfigurationDialog::Change()
{
  TWinSCPDialog::Change();

  if (GetHandle())
  {
    LockChanges();
    SCOPE_EXIT
    {
      UnlockChanges();
    };
    UpdateControls();
  }
}

void TTransferEditorConfigurationDialog::UpdateControls()
{
  EditorDownloadDefaultButton->SetEnabled(!EditorMultipleCheck->GetChecked());
  EditorDownloadOptionsButton->SetEnabled(EditorDownloadDefaultButton->GetEnabled());

  EditorUploadSameButton->SetEnabled(
    !EditorMultipleCheck->GetChecked() && !EditorUploadOnSaveCheck->GetChecked());
  EditorUploadOptionsButton->SetEnabled(EditorUploadSameButton->GetEnabled());
}

bool TWinSCPPlugin::TransferEditorConfigurationDialog()
{
  std::unique_ptr<TTransferEditorConfigurationDialog> Dialog(new TTransferEditorConfigurationDialog(this));
  bool Result = Dialog->Execute();
  return Result;
}

bool TWinSCPPlugin::ConfirmationsConfigurationDialog()
{
  std::unique_ptr<TWinSCPDialog> DialogPtr(new TWinSCPDialog(this));
  TWinSCPDialog * Dialog = DialogPtr.get();

  Dialog->SetSize(TPoint(67, 10));
  Dialog->SetCaption(FORMAT(L"%s - %s",
    GetMsg(PLUGIN_TITLE).c_str(), ::StripHotkey(GetMsg(CONFIG_CONFIRMATIONS)).c_str()));

  TFarCheckBox * ConfirmOverwritingCheck = new TFarCheckBox(Dialog);
  ConfirmOverwritingCheck->SetAllowGrayed(true);
  ConfirmOverwritingCheck->SetCaption(GetMsg(CONFIRMATIONS_CONFIRM_OVERWRITING));

  TFarCheckBox * ConfirmCommandSessionCheck = new TFarCheckBox(Dialog);
  ConfirmCommandSessionCheck->SetCaption(GetMsg(CONFIRMATIONS_OPEN_COMMAND_SESSION));

  TFarCheckBox * ConfirmResumeCheck = new TFarCheckBox(Dialog);
  ConfirmResumeCheck->SetCaption(GetMsg(CONFIRMATIONS_CONFIRM_RESUME));

  TFarCheckBox * ConfirmSynchronizedBrowsingCheck = new TFarCheckBox(Dialog);
  ConfirmSynchronizedBrowsingCheck->SetCaption(GetMsg(CONFIRMATIONS_SYNCHRONIZED_BROWSING));

  Dialog->AddStandardButtons();

  TFarConfiguration * FarConfiguration = GetFarConfiguration();
  ConfirmOverwritingCheck->SetSelected(!FarConfiguration->GetConfirmOverwritingOverride() ?
    BSTATE_3STATE : (GetConfiguration()->GetConfirmOverwriting() ? BSTATE_CHECKED :
                       BSTATE_UNCHECKED));
  ConfirmCommandSessionCheck->SetChecked(GetGUIConfiguration()->GetConfirmCommandSession());
  ConfirmResumeCheck->SetChecked(GetGUIConfiguration()->GetConfirmResume());
  ConfirmSynchronizedBrowsingCheck->SetChecked(FarConfiguration->GetConfirmSynchronizedBrowsing());

  bool Result = (Dialog->ShowModal() == brOK);

  if (Result)
  {
    GetConfiguration()->BeginUpdate();
    SCOPE_EXIT
    {
      GetConfiguration()->EndUpdate();
    };
    FarConfiguration->SetConfirmOverwritingOverride(
      ConfirmOverwritingCheck->GetSelected() != BSTATE_3STATE);
    GetGUIConfiguration()->SetConfirmCommandSession(ConfirmCommandSessionCheck->GetChecked());
    GetGUIConfiguration()->SetConfirmResume(ConfirmResumeCheck->GetChecked());
    if (FarConfiguration->GetConfirmOverwritingOverride())
    {
      GetConfiguration()->SetConfirmOverwriting(ConfirmOverwritingCheck->GetChecked());
    }
    FarConfiguration->SetConfirmSynchronizedBrowsing(ConfirmSynchronizedBrowsingCheck->GetChecked());
  }
  return Result;
}

bool TWinSCPPlugin::IntegrationConfigurationDialog()
{
  std::unique_ptr<TWinSCPDialog> DialogPtr(new TWinSCPDialog(this));
  TWinSCPDialog * Dialog = DialogPtr.get();

  TFarText * Text;

  Dialog->SetSize(TPoint(65, 14));
  Dialog->SetCaption(FORMAT(L"%s - %s",
    GetMsg(PLUGIN_TITLE).c_str(), ::StripHotkey(GetMsg(CONFIG_INTEGRATION)).c_str()));

  Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(INTEGRATION_PUTTY));

  TFarEdit * PuttyPathEdit = new TFarEdit(Dialog);

  TFarCheckBox * PuttyPasswordCheck = new TFarCheckBox(Dialog);
  PuttyPasswordCheck->SetCaption(GetMsg(INTEGRATION_PUTTY_PASSWORD));
  PuttyPasswordCheck->SetEnabledDependency(PuttyPathEdit);

  TFarCheckBox * TelnetForFtpInPuttyCheck = new TFarCheckBox(Dialog);
  TelnetForFtpInPuttyCheck->SetCaption(GetMsg(INTEGRATION_TELNET_FOR_FTP_IN_PUTTY));
  TelnetForFtpInPuttyCheck->SetEnabledDependency(PuttyPathEdit);

  Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(INTEGRATION_PAGEANT));

  TFarEdit * PageantPathEdit = new TFarEdit(Dialog);

  Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(INTEGRATION_PUTTYGEN));

  TFarEdit * PuttygenPathEdit = new TFarEdit(Dialog);

  Dialog->AddStandardButtons();

  PuttyPathEdit->SetText(GetGUIConfiguration()->GetPuttyPath());
  PuttyPasswordCheck->SetChecked(GetGUIConfiguration()->GetPuttyPassword());
  TelnetForFtpInPuttyCheck->SetChecked(GetGUIConfiguration()->GetTelnetForFtpInPutty());
  PageantPathEdit->SetText(GetFarConfiguration()->GetPageantPath());
  PuttygenPathEdit->SetText(GetFarConfiguration()->GetPuttygenPath());

  bool Result = (Dialog->ShowModal() == brOK);

  if (Result)
  {
    GetConfiguration()->BeginUpdate();
    SCOPE_EXIT
    {
      GetConfiguration()->EndUpdate();
    };
    GetGUIConfiguration()->SetPuttyPath(PuttyPathEdit->GetText());
    GetGUIConfiguration()->SetPuttyPassword(PuttyPasswordCheck->GetChecked());
    GetGUIConfiguration()->SetTelnetForFtpInPutty(TelnetForFtpInPuttyCheck->GetChecked());
    GetFarConfiguration()->SetPageantPath(PageantPathEdit->GetText());
    GetFarConfiguration()->SetPuttygenPath(PuttygenPathEdit->GetText());
  }
  return Result;
}

class TAboutDialog : public TFarDialog
{
public:
  explicit TAboutDialog(TCustomFarPlugin * AFarPlugin);

private:
  void UrlButtonClick(TFarButton * Sender, bool & Close);
  void UrlTextClick(TFarDialogItem * Item, MOUSE_EVENT_RECORD * Event);
};

UnicodeString ReplaceCopyright(const UnicodeString & S)
{
  return ::StringReplaceAll(S, L"©", L"(c)");
}

TAboutDialog::TAboutDialog(TCustomFarPlugin * AFarPlugin) :
  TFarDialog(AFarPlugin)
{
  TFarText * Text;
  TFarButton * Button;

  // UnicodeString ProductName = GetConfiguration()->GetFileInfoString(L"ProductName");
  UnicodeString ProductName = LoadStr(WINSCPFAR_NAME);
  UnicodeString Comments;
  try
  {
    Comments = GetConfiguration()->GetFileInfoString(L"Comments");
  }
  catch (...)
  {
    Comments.Clear();
  }
  UnicodeString LegalCopyright; // = GetConfiguration()->GetFileInfoString(L"LegalCopyright");

  int Height = 15;
#ifndef NO_FILEZILLA
  Height += 2;
#endif
  if (!ProductName.IsEmpty())
  {
    Height++;
  }
  if (!Comments.IsEmpty())
  {
    Height++;
  }
  if (!LegalCopyright.IsEmpty())
  {
    Height++;
  }
  SetSize(TPoint(55, Height));

  SetCaption(FORMAT(L"%s - %s",
    GetMsg(PLUGIN_TITLE).c_str(), ::StripHotkey(GetMsg(CONFIG_ABOUT)).c_str()));
  Text = new TFarText(this);
  Text->SetCaption(GetConfiguration()->GetFileInfoString(L"FileDescription"));
  Text->SetCenterGroup(true);

  Text = new TFarText(this);
  Text->SetCaption(FORMAT(GetMsg(ABOUT_VERSION).c_str(), GetConfiguration()->GetProductVersion().c_str(), NETBOX_VERSION_BUILD));
  Text->SetCenterGroup(true);

  Text = new TFarText(this);
  Text->Move(0, 1);
  Text->SetCaption(LoadStr(WINSCPFAR_BASED_ON));
  Text->SetCenterGroup(true);

  Text = new TFarText(this);
  Text->SetCaption(FMTLOAD(WINSCPFAR_BASED_VERSION, LoadStr(WINSCPFAR_VERSION).c_str()));
  Text->SetCenterGroup(true);

  if (!ProductName.IsEmpty())
  {
    Text = new TFarText(this);
    Text->SetCaption(FORMAT(GetMsg(ABOUT_PRODUCT_VERSION).c_str(),
      ProductName.c_str(),
      LoadStr(WINSCP_VERSION).c_str()));
    Text->SetCenterGroup(true);
  }

  Text = new TFarText(this);
  Text->SetCaption(LoadStr(WINSCPFAR_BASED_COPYRIGHT));
  Text->SetCenterGroup(true);

  if (!Comments.IsEmpty())
  {
    Text = new TFarText(this);
    if (ProductName.IsEmpty())
    {
      Text->Move(0, 1);
    }
    Text->SetCaption(Comments);
    Text->SetCenterGroup(true);
  }
#if 0
  if (!LegalCopyright.IsEmpty())
  {
    Text = new TFarText(this);
    Text->Move(0, 1);
    Text->SetCaption(GetConfiguration()->GetFileInfoString(L"LegalCopyright"));
    Text->SetCenterGroup(true);
  }

  Text = new TFarText(this);
  if (LegalCopyright.IsEmpty())
  {
    Text->Move(0, 1);
  }
  Text->SetCaption(GetMsg(ABOUT_URL));
  // FIXME Text->SetColor(static_cast<int>((GetSystemColor(COL_DIALOGTEXT) & 0xF0) | 0x09));
  Text->SetCenterGroup(true);
  Text->SetOnMouseClick(MAKE_CALLBACK(TAboutDialog::UrlTextClick, this));

  Button = new TFarButton(this);
  Button->Move(0, 1);
  Button->SetCaption(GetMsg(ABOUT_HOMEPAGE));
  Button->SetOnClick(MAKE_CALLBACK(TAboutDialog::UrlButtonClick, this));
  Button->SetTag(1);
  Button->SetCenterGroup(true);

  SetNextItemPosition(ipRight);

  Button = new TFarButton(this);
  Button->SetCaption(GetMsg(ABOUT_FORUM));
  Button->SetOnClick(MAKE_CALLBACK(TAboutDialog::UrlButtonClick, this));
  Button->SetTag(2);
  Button->SetCenterGroup(true);
  SetNextItemPosition(ipNewLine);
#endif

  new TFarSeparator(this);

  Text = new TFarText(this);
  Text->SetCaption(FMTLOAD(PUTTY_BASED_ON, LoadStr(PUTTY_VERSION).c_str()));
  Text->SetCenterGroup(true);

  Text = new TFarText(this);
  Text->SetCaption(LoadStr(PUTTY_COPYRIGHT));
  Text->SetCenterGroup(true);

#ifndef NO_FILEZILLA
  Text = new TFarText(this);
  Text->SetCaption(LoadStr(FILEZILLA_BASED_ON2));
  Text->SetCenterGroup(true);

  Text = new TFarText(this);
  Text->SetCaption(LoadStr(FILEZILLA_COPYRIGHT2));
  Text->SetCenterGroup(true);
#endif

  new TFarSeparator(this);

  Button = new TFarButton(this);
  Button->SetCaption(GetMsg(MSG_BUTTON_CLOSE));
  Button->SetDefault(true);
  Button->SetResult(brOK);
  Button->SetCenterGroup(true);
  Button->SetFocus();
}

void TAboutDialog::UrlTextClick(TFarDialogItem * /*Item*/,
  MOUSE_EVENT_RECORD * /*Event*/)
{
#ifndef __linux__
  UnicodeString Address = GetMsg(ABOUT_URL);
  ::ShellExecute(nullptr, L"open", const_cast<wchar_t *>(Address.c_str()), nullptr, nullptr, SW_SHOWNORMAL);
#endif
}

void TAboutDialog::UrlButtonClick(TFarButton * Sender, bool & /*Close*/)
{
#ifndef __linux__
  UnicodeString Address;
  switch (Sender->GetTag())
  {
    case 1:
      Address = GetMsg(ABOUT_URL) + L"eng/docs/far";
      break;
    case 2:
      Address = GetMsg(ABOUT_URL) + L"forum/";
      break;
  }
  ::ShellExecute(nullptr, L"open", const_cast<wchar_t *>(Address.c_str()), nullptr, nullptr, SW_SHOWNORMAL);
#endif
}

void TWinSCPPlugin::AboutDialog()
{
  std::unique_ptr<TFarDialog> Dialog(new TAboutDialog(this));
  Dialog->ShowModal();
}

class TPasswordDialog : public TFarDialog
{
public:
  explicit TPasswordDialog(TCustomFarPlugin * AFarPlugin,
    const UnicodeString & SessionName, TPromptKind Kind, const UnicodeString & Name,
    const UnicodeString & Instructions, const TStrings * Prompts,
    bool StoredCredentialsTried);
  virtual ~TPasswordDialog();
  bool Execute(TStrings * Results);

private:
  void ShowPromptClick(TFarButton * Sender, bool & Close);
  void GenerateLabel(const UnicodeString & ACaption, bool & Truncated);
  TFarEdit * GenerateEdit(bool Echo);
  void GeneratePrompt(bool ShowSavePassword,
    const UnicodeString & Instructions, const TStrings * Prompts, bool & Truncated);

private:
  TSessionData * FSessionData;
  UnicodeString FPrompt;
  TList * FEdits;
  TFarCheckBox * SavePasswordCheck;
};

TPasswordDialog::TPasswordDialog(TCustomFarPlugin * AFarPlugin,
  const UnicodeString & SessionName, TPromptKind Kind, const UnicodeString & Name,
  const UnicodeString & Instructions, const TStrings * Prompts,
  bool /*StoredCredentialsTried*/) :
  TFarDialog(AFarPlugin),
  FSessionData(nullptr),
  FEdits(new TList()),
  SavePasswordCheck(nullptr)
{
  TFarButton * Button;

  bool ShowSavePassword = false;
  if (((Kind == pkPassword) || (Kind == pkTIS) || (Kind == pkCryptoCard) ||
       (Kind == pkKeybInteractive)) &&
       (Prompts->GetCount() == 1) && // FLAGSET((intptr_t)Prompts->GetObject(0), pupRemember) &&
      !SessionName.IsEmpty())
  //    // StoredCredentialsTried)
  {
    FSessionData = NB_STATIC_DOWNCAST(TSessionData, StoredSessions->FindByName(SessionName));
    ShowSavePassword = (FSessionData != nullptr);
  }

  bool Truncated = false;
  GeneratePrompt(ShowSavePassword, Instructions, Prompts, Truncated);

  SetCaption(Name);

  if (ShowSavePassword)
  {
    SavePasswordCheck = new TFarCheckBox(this);
    SavePasswordCheck->SetCaption(GetMsg(PASSWORD_SAVE));
  }
  else
  {
    SavePasswordCheck = nullptr;
  }

  new TFarSeparator(this);

  Button = new TFarButton(this);
  Button->SetCaption(GetMsg(MSG_BUTTON_OK));
  Button->SetDefault(true);
  Button->SetResult(brOK);
  Button->SetCenterGroup(true);

  SetNextItemPosition(ipRight);

  if (Truncated)
  {
    Button = new TFarButton(this);
    Button->SetCaption(GetMsg(PASSWORD_SHOW_PROMPT));
    Button->SetOnClick(MAKE_CALLBACK(TPasswordDialog::ShowPromptClick, this));
    Button->SetCenterGroup(true);
  }

  Button = new TFarButton(this);
  Button->SetCaption(GetMsg(MSG_BUTTON_Cancel));
  Button->SetResult(brCancel);
  Button->SetCenterGroup(true);
}

TPasswordDialog::~TPasswordDialog()
{
  SAFE_DESTROY(FEdits);
}

void TPasswordDialog::GenerateLabel(const UnicodeString & ACaption,
  bool & Truncated)
{
  UnicodeString Caption = ACaption;
  TFarText * Result = new TFarText(this);

  if (!FPrompt.IsEmpty())
  {
    FPrompt += L"\n\n";
  }
  FPrompt += Caption;

  if (GetSize().x - 10 < static_cast<int>(Caption.Length()))
  {
    Caption.SetLength(GetSize().x - 10 - 4);
    Caption += L" ...";
    Truncated = true;
  }

  Result->SetCaption(Caption);
}

TFarEdit * TPasswordDialog::GenerateEdit(bool Echo)
{
  TFarEdit * Result = new TFarEdit(this);
  Result->SetPassword(!Echo);
  return Result;
}

void TPasswordDialog::GeneratePrompt(bool ShowSavePassword,
  const UnicodeString & Instructions, const TStrings * Prompts, bool & Truncated)
{
  FEdits->Clear();
  TPoint S = TPoint(40, ShowSavePassword ? 1 : 0);

  int x = static_cast<int>(Instructions.Length());
  if (S.x < x)
  {
    S.x = x;
  }
  if (!Instructions.IsEmpty())
  {
    S.y += 2;
  }

  for (intptr_t Index = 0; Index < Prompts->GetCount(); ++Index)
  {
    int l = static_cast<int>(Prompts->GetString(Index).Length());
    if (S.x < l)
    {
      S.x = l;
    }
    S.y += 2;
  }

  if (S.x > 80 - 10)
  {
    S.x = 80 - 10;
  }

  SetSize(TPoint(S.x + 10, S.y + 6));

  if (!Instructions.IsEmpty())
  {
    GenerateLabel(Instructions, Truncated);
    // dumb way to add empty line
    GenerateLabel(L"", Truncated);
  }

  for (intptr_t Index = 0; Index < Prompts->GetCount(); ++Index)
  {
    GenerateLabel(Prompts->GetString(Index), Truncated);

    FEdits->Add(GenerateEdit(FLAGSET(reinterpret_cast<intptr_t>(Prompts->GetObj(Index)), pupEcho)));
  }
}

void TPasswordDialog::ShowPromptClick(TFarButton * /*Sender*/,
  bool & /*Close*/)
{
  TWinSCPPlugin * WinSCPPlugin = NB_STATIC_DOWNCAST(TWinSCPPlugin, FarPlugin);

  WinSCPPlugin->MoreMessageDialog(FPrompt, nullptr, qtInformation, qaOK);
}

bool TPasswordDialog::Execute(TStrings * Results)
{
  for (intptr_t Index = 0; Index < FEdits->GetCount(); ++Index)
  {
    NB_STATIC_DOWNCAST(TFarEdit, FEdits->GetItem(Index))->SetText(Results->GetString(Index));
  }

  bool Result = (ShowModal() != brCancel);
  if (Result)
  {
    for (intptr_t Index = 0; Index < FEdits->GetCount(); ++Index)
    {
      UnicodeString Text = NB_STATIC_DOWNCAST(TFarEdit, FEdits->GetItem(Index))->GetText();
      Results->SetString(Index, Text);
    }

    if ((SavePasswordCheck != nullptr) && SavePasswordCheck->GetChecked())
    {
      DebugAssert(FSessionData != nullptr);
      FSessionData->SetPassword(Results->GetString(0));
      // modified only, explicit
      StoredSessions->Save(false, true);
    }
  }
  return Result;
}

bool TWinSCPFileSystem::PasswordDialog(TSessionData * SessionData,
  TPromptKind Kind, const UnicodeString & Name, const UnicodeString & Instructions,
  TStrings * Prompts,
  TStrings * Results, bool StoredCredentialsTried)
{
  std::unique_ptr<TPasswordDialog> Dialog(new TPasswordDialog(FPlugin, SessionData->GetName(),
    Kind, Name, Instructions, Prompts, StoredCredentialsTried));
  bool Result = Dialog->Execute(Results);
  return Result;
}

bool TWinSCPFileSystem::BannerDialog(const UnicodeString & SessionName,
  const UnicodeString & Banner, bool & NeverShowAgain, intptr_t Options)
{
  std::unique_ptr<TWinSCPDialog> DialogPtr(new TWinSCPDialog(FPlugin));
  TWinSCPDialog * Dialog = DialogPtr.get();

  Dialog->SetSize(TPoint(70, 21));
  Dialog->SetCaption(FORMAT(GetMsg(BANNER_TITLE).c_str(), SessionName.c_str()));

  TFarLister * Lister = new TFarLister(Dialog);
  FarWrapText(Banner, Lister->GetItems(), Dialog->GetBorderBox()->GetWidth() - 4);
  Lister->SetHeight(15);
  Lister->SetLeft(Dialog->GetBorderBox()->GetLeft() + 1);
  Lister->SetRight(Dialog->GetBorderBox()->GetRight() - (Lister->GetScrollBar() ? 0 : 1));

  new TFarSeparator(Dialog);

  TFarCheckBox * NeverShowAgainCheck = nullptr;
  if (FLAGCLEAR(Options, boDisableNeverShowAgain))
  {
    NeverShowAgainCheck = new TFarCheckBox(Dialog);
    NeverShowAgainCheck->SetCaption(GetMsg(BANNER_NEVER_SHOW_AGAIN));
    NeverShowAgainCheck->SetVisible(FLAGCLEAR(Options, boDisableNeverShowAgain));
    NeverShowAgainCheck->SetChecked(NeverShowAgain);

    Dialog->SetNextItemPosition(ipRight);
  }

  TFarButton * Button = new TFarButton(Dialog);
  Button->SetCaption(GetMsg(BANNER_CONTINUE));
  Button->SetDefault(true);
  Button->SetResult(brOK);
  if (NeverShowAgainCheck != nullptr)
  {
    Button->SetLeft(Dialog->GetBorderBox()->GetRight() - Button->GetWidth() - 1);
  }
  else
  {
    Button->SetCenterGroup(true);
  }

  bool Result = (Dialog->ShowModal() == brOK);

  if (Result)
  {
    if (NeverShowAgainCheck != nullptr)
    {
      NeverShowAgain = NeverShowAgainCheck->GetChecked();
    }
  }
  return Result;
}

class TSessionDialog : public TTabbedDialog
{
public:
  enum TSessionTab
  {
    tabSession = 1, tabEnvironment, tabDirectories, tabSFTP, tabSCP, tabFTP,
    tabConnection, tabTunnel, tabProxy, tabSsh, tabKex, tabAuthentication, tabBugs, tabWebDAV, tabCount
  };

  explicit TSessionDialog(TCustomFarPlugin * AFarPlugin, TSessionActionEnum Action);
  virtual ~TSessionDialog();

  bool Execute(TSessionData * Data, TSessionActionEnum & Action);

protected:
  virtual void Change();
  virtual void Init();
  virtual bool CloseQuery();
  virtual void SelectTab(intptr_t Tab);

private:
  void LoadPing(TSessionData * SessionData);
  void SavePing(TSessionData * SessionData);
  intptr_t ProxyMethodToIndex(TProxyMethod ProxyMethod, TFarList * Items) const;
  TProxyMethod IndexToProxyMethod(intptr_t Index, TFarList * Items) const;
  TFarComboBox * GetProxyMethodCombo() const;
  TFarComboBox * GetOtherProxyMethodCombo() const;
  intptr_t FSProtocolToIndex(TFSProtocol FSProtocol, bool & AllowScpFallback) const;
  TFSProtocol IndexToFSProtocol(intptr_t Index, bool AllowScpFallback) const;
  TFSProtocol GetFSProtocol() const;
  inline intptr_t GetLastSupportedFtpProxyMethod() const;
  bool GetSupportedFtpProxyMethod(intptr_t Method) const;
  TProxyMethod GetProxyMethod() const;
  intptr_t GetFtpProxyLogonType() const;
  TFtps IndexToFtps(intptr_t Index) const;
  TFtps GetFtps() const;
  TLoginType IndexToLoginType(intptr_t Index) const;
  bool VerifyKey(const UnicodeString & AFileName, bool TypeOnly);
  void PrevTabClick(TFarButton * /*Sender*/, bool & Close);
  void NextTabClick(TFarButton * /*Sender*/, bool & Close);
  void CipherButtonClick(TFarButton * Sender, bool & Close);
  void KexButtonClick(TFarButton * Sender, bool & Close);
  void AuthGSSAPICheckAllowChange(TFarDialogItem * Sender, intptr_t NewState, bool & Allow);
  void UnixEnvironmentButtonClick(TFarButton * Sender, bool & Close);
  void WindowsEnvironmentButtonClick(TFarButton * Sender, bool & Close);
  void UpdateControls();
  void TransferProtocolComboChange();
  void LoginTypeComboChange();
  void FillCodePageEdit();
  void CodePageEditAdd(uint32_t Cp);
  void FtpProxyMethodComboAddNewItem(int ProxyTypeId, TProxyMethod ProxyType);
  void SshProxyMethodComboAddNewItem(int ProxyTypeId, TProxyMethod ProxyType);
  bool IsSshProtocol(TFSProtocol FSProtocol) const;
  bool IsWebDAVProtocol(TFSProtocol FSProtocol) const;
  bool IsSshOrWebDAVProtocol(TFSProtocol FSProtocol) const;

  void ChangeTabs(intptr_t FirstVisibleTabIndex);
  intptr_t GetVisibleTabsCount(intptr_t TabIndex, bool Forward) const;
  intptr_t AddTab(intptr_t TabID, const wchar_t * TabCaption);

private:
  TSessionActionEnum FAction;
  TSessionData * FSessionData;
  intptr_t FTransferProtocolIndex;
  intptr_t FFtpEncryptionComboIndex;

  TTabButton * SshTab;
  TTabButton * AuthenticationTab;
  TTabButton * KexTab;
  TTabButton * BugsTab;
  TTabButton * WebDAVTab;
  TTabButton * ScpTab;
  TTabButton * SftpTab;
  TTabButton * FtpTab;
  TTabButton * TunnelTab;
  TTabButton * PrevTab;
  TTabButton * NextTab;
  TFarButton * ConnectButton;
  TFarEdit * HostNameEdit;
  TFarEdit * PortNumberEdit;
  TFarEdit * UserNameEdit;
  TFarEdit * PasswordEdit;
  TFarEdit * PrivateKeyEdit;
  TFarComboBox * TransferProtocolCombo;
  TFarCheckBox * AllowScpFallbackCheck;
  TFarText * HostNameLabel;
  TFarText * InsecureLabel;
  TFarText * FtpEncryptionLabel;
  TFarComboBox * FtpEncryptionCombo;
  TFarCheckBox * UpdateDirectoriesCheck;
  TFarCheckBox * CacheDirectoriesCheck;
  TFarCheckBox * CacheDirectoryChangesCheck;
  TFarCheckBox * PreserveDirectoryChangesCheck;
  TFarCheckBox * ResolveSymlinksCheck;
  TFarEdit * RemoteDirectoryEdit;
  TFarComboBox * EOLTypeCombo;
  TFarRadioButton * DSTModeWinCheck;
  TFarRadioButton * DSTModeKeepCheck;
  TFarRadioButton * DSTModeUnixCheck;
  TFarCheckBox * CompressionCheck;
  TFarRadioButton * SshProt1onlyButton;
  TFarRadioButton * SshProt1Button;
  TFarRadioButton * SshProt2Button;
  TFarRadioButton * SshProt2onlyButton;
  TFarListBox * CipherListBox;
  TFarButton * CipherUpButton;
  TFarButton * CipherDownButton;
  TFarCheckBox * Ssh2DESCheck;
  TFarComboBox * ShellEdit;
  TFarComboBox * ReturnVarEdit;
  TFarCheckBox * LookupUserGroupsCheck;
  TFarCheckBox * ClearAliasesCheck;
  TFarCheckBox * UnsetNationalVarsCheck;
  TFarComboBox * ListingCommandEdit;
  TFarCheckBox * IgnoreLsWarningsCheck;
  TFarCheckBox * SCPLsFullTimeAutoCheck;
  TFarCheckBox * Scp1CompatibilityCheck;
  TFarEdit * PostLoginCommandsEdits[3];
  TFarEdit * TimeDifferenceEdit;
  TFarEdit * TimeDifferenceMinutesEdit;
  TFarEdit * TimeoutEdit;
  TFarRadioButton * PingOffButton;
  TFarRadioButton * PingNullPacketButton;
  TFarRadioButton * PingDummyCommandButton;
  TFarEdit * PingIntervalSecEdit;
  TFarComboBox * CodePageEdit;
  TFarComboBox * SshProxyMethodCombo;
  TFarComboBox * FtpProxyMethodCombo;
  TFarEdit * ProxyHostEdit;
  TFarEdit * ProxyPortEdit;
  TFarEdit * ProxyUsernameEdit;
  TFarEdit * ProxyPasswordEdit;
  TFarText * ProxyLocalCommandLabel;
  TFarEdit * ProxyLocalCommandEdit;
  TFarText * ProxyTelnetCommandLabel;
  TFarEdit * ProxyTelnetCommandEdit;
  TFarCheckBox * ProxyLocalhostCheck;
  TFarRadioButton * ProxyDNSOffButton;
  TFarRadioButton * ProxyDNSAutoButton;
  TFarRadioButton * ProxyDNSOnButton;
  TFarCheckBox * TunnelCheck;
  TFarEdit * TunnelHostNameEdit;
  TFarEdit * TunnelPortNumberEdit;
  TFarEdit * TunnelUserNameEdit;
  TFarEdit * TunnelPasswordEdit;
  TFarEdit * TunnelPrivateKeyEdit;
  TFarComboBox * TunnelLocalPortNumberEdit;
  TFarComboBox * BugIgnore1Combo;
  TFarComboBox * BugPlainPW1Combo;
  TFarComboBox * BugRSA1Combo;
  TFarComboBox * BugHMAC2Combo;
  TFarComboBox * BugDeriveKey2Combo;
  TFarComboBox * BugRSAPad2Combo;
  TFarComboBox * BugPKSessID2Combo;
  TFarComboBox * BugRekey2Combo;
  TFarCheckBox * SshNoUserAuthCheck;
  TFarCheckBox * AuthTISCheck;
  TFarCheckBox * TryAgentCheck;
  TFarCheckBox * AuthKICheck;
  TFarCheckBox * AuthKIPasswordCheck;
  TFarCheckBox * AgentFwdCheck;
  TFarCheckBox * AuthGSSAPICheck3;
  TFarCheckBox * GSSAPIFwdTGTCheck;
  TFarCheckBox * DeleteToRecycleBinCheck;
  TFarCheckBox * OverwrittenToRecycleBinCheck;
  TFarEdit * RecycleBinPathEdit;
  TFarComboBox * SFTPMaxVersionCombo;
  TFarComboBox * SftpServerEdit;
  TFarComboBox * SFTPBugSymlinkCombo;
  TFarComboBox * SFTPBugSignedTSCombo;
  TFarListBox * KexListBox;
  TFarButton * KexUpButton;
  TFarButton * KexDownButton;
  TFarEdit * SFTPMinPacketSizeEdit;
  TFarEdit * SFTPMaxPacketSizeEdit;
  TFarEdit * RekeyTimeEdit;
  TFarEdit * RekeyDataEdit;
  TFarRadioButton * IPAutoButton;
  TFarRadioButton * IPv4Button;
  TFarRadioButton * IPv6Button;
  TFarCheckBox * SshBufferSizeCheck;
  TFarComboBox * FtpUseMlsdCombo;
  TFarCheckBox * FtpPasvModeCheck;
  TFarCheckBox * FtpAllowEmptyPasswordCheck;
  TFarCheckBox * FtpDupFFCheck;
  TFarCheckBox * FtpUndupFFCheck;
  TFarCheckBox * SslSessionReuseCheck;
  TFarCheckBox * WebDAVCompressionCheck;
  TObjectList * FTabs;
  intptr_t FFirstVisibleTabIndex;
};

#define BUG(BUGID, MSG, PREFIX) \
  TRISTATE(PREFIX ## Bug ## BUGID ## Combo, PREFIX ## Bug(sb ## BUGID), MSG)
#define BUGS() \
  BUG(Ignore1, LOGIN_BUGS_IGNORE1, ); \
  BUG(PlainPW1, LOGIN_BUGS_PLAIN_PW1, ); \
  BUG(RSA1, LOGIN_BUGS_RSA1, ); \
  BUG(HMAC2, LOGIN_BUGS_HMAC2, ); \
  BUG(DeriveKey2, LOGIN_BUGS_DERIVE_KEY2, ); \
  BUG(RSAPad2, LOGIN_BUGS_RSA_PAD2, ); \
  BUG(PKSessID2, LOGIN_BUGS_PKSESSID2, ); \
  BUG(Rekey2, LOGIN_BUGS_REKEY2, );
#define SFTP_BUGS() \
  BUG(Symlink, LOGIN_SFTP_BUGS_SYMLINK, SFTP); \
  BUG(SignedTS, LOGIN_SFTP_BUGS_SIGNED_TS, SFTP);

static const TFSProtocol FSOrder[] = { fsSFTPonly, fsSCPonly, fsFTP, fsWebDAV };

TSessionDialog::TSessionDialog(TCustomFarPlugin * AFarPlugin, TSessionActionEnum Action) :
  TTabbedDialog(AFarPlugin, tabCount),
  FAction(Action),
  FSessionData(nullptr),
  FTransferProtocolIndex(0),
  FFtpEncryptionComboIndex(0),
  FTabs(new TObjectList()),
  FFirstVisibleTabIndex(0)
{
  TPoint S = TPoint(67, 23);
  bool Limited = (S.y > GetMaxSize().y);
  if (Limited)
  {
    S.y = GetMaxSize().y;
  }
  SetSize(S);

  FTabs->SetOwnsObjects(false);
  FFirstVisibleTabIndex = 0;

#define TRISTATE(COMBO, PROP, MSG) \
    Text = new TFarText(this); \
    Text->SetCaption(GetMsg(MSG)); \
    SetNextItemPosition(ipRight); \
    COMBO = new TFarComboBox(this); \
    COMBO->SetDropDownList(true); \
    COMBO->SetWidth(7); \
    COMBO->GetItems()->BeginUpdate(); \
    { \
      SCOPE_EXIT \
      { \
        COMBO->GetItems()->EndUpdate(); \
      }; \
      COMBO->GetItems()->Add(GetMsg(LOGIN_BUGS_AUTO)); \
      COMBO->GetItems()->Add(GetMsg(LOGIN_BUGS_OFF)); \
      COMBO->GetItems()->Add(GetMsg(LOGIN_BUGS_ON)); \
    } \
    Text->SetEnabledFollow(COMBO); \
    SetNextItemPosition(ipNewLine);

  TRect CRect = GetClientRect();

  TFarButton * Button;
  // TTabButton * Tab;
  TFarSeparator * Separator;
  TFarText * Text;
  intptr_t GroupTop;
  intptr_t Pos;
  intptr_t Index1;

  Index1 = AddTab(tabSession, GetMsg(LOGIN_TAB_SESSION).c_str());
  // Tab = NB_STATIC_DOWNCAST(TTabButton, GetItem(Index));

  SetNextItemPosition(ipRight);

  Index1 = AddTab(tabEnvironment, GetMsg(LOGIN_TAB_ENVIRONMENT).c_str());

  Index1 = AddTab(tabDirectories, GetMsg(LOGIN_TAB_DIRECTORIES).c_str());

  Index1 = AddTab(tabSFTP, GetMsg(LOGIN_TAB_SFTP).c_str());
  SftpTab = NB_STATIC_DOWNCAST(TTabButton, GetItem(Index1));

  Index1 = AddTab(tabSCP, GetMsg(LOGIN_TAB_SCP).c_str());
  ScpTab = NB_STATIC_DOWNCAST(TTabButton, GetItem(Index1));

  PrevTab = new TTabButton(this);
  PrevTab->SetTabName(UnicodeString('\x11'));
  PrevTab->SetBrackets(brNone);
  PrevTab->SetCenterGroup(false);
  PrevTab->SetOnClick(MAKE_CALLBACK(TSessionDialog::PrevTabClick, this));

  NextTab = new TTabButton(this);
  NextTab->SetTabName(UnicodeString('\x10'));
  NextTab->SetBrackets(brNone);
  NextTab->SetCenterGroup(false);
  NextTab->SetOnClick(MAKE_CALLBACK(TSessionDialog::NextTabClick, this));

  intptr_t PWidth = PrevTab->GetWidth();
  intptr_t NWidth = NextTab->GetWidth();
  int R = S.x - 4;
  PrevTab->SetLeft(R - PWidth - NWidth - 2);
  PrevTab->SetWidth(PWidth);
  NextTab->SetLeft(R - NWidth - 1);
  NextTab->SetWidth(PWidth);

  Index1 = AddTab(tabFTP, GetMsg(LOGIN_TAB_FTP).c_str());
  FtpTab = NB_STATIC_DOWNCAST(TTabButton, GetItem(Index1));

  Index1 = AddTab(tabConnection, GetMsg(LOGIN_TAB_CONNECTION).c_str());

  Index1 = AddTab(tabProxy, GetMsg(LOGIN_TAB_PROXY).c_str());

  Index1 = AddTab(tabTunnel, GetMsg(LOGIN_TAB_TUNNEL).c_str());
  TunnelTab = NB_STATIC_DOWNCAST(TTabButton, GetItem(Index1));

  Index1 = AddTab(tabSsh, GetMsg(LOGIN_TAB_SSH).c_str());
  SshTab = NB_STATIC_DOWNCAST(TTabButton, GetItem(Index1));

  Index1 = AddTab(tabKex, GetMsg(LOGIN_TAB_KEX).c_str());
  KexTab = NB_STATIC_DOWNCAST(TTabButton, GetItem(Index1));

  Index1 = AddTab(tabAuthentication, GetMsg(LOGIN_TAB_AUTH).c_str());
  AuthenticationTab = NB_STATIC_DOWNCAST(TTabButton, GetItem(Index1));

  Index1 = AddTab(tabBugs, GetMsg(LOGIN_TAB_BUGS).c_str());
  BugsTab = NB_STATIC_DOWNCAST(TTabButton, GetItem(Index1));

  Index1 = AddTab(tabWebDAV, GetMsg(LOGIN_TAB_WEBDAV).c_str());
  WebDAVTab = NB_STATIC_DOWNCAST(TTabButton, GetItem(Index1));

  // Session tab

  SetNextItemPosition(ipNewLine);
  SetDefaultGroup(tabSession);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(LOGIN_GROUP_SESSION));
  GroupTop = Separator->GetTop();

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_TRANSFER_PROTOCOL));

  SetNextItemPosition(ipRight);

  TransferProtocolCombo = new TFarComboBox(this);
  TransferProtocolCombo->SetDropDownList(true);
  TransferProtocolCombo->SetWidth(10);
  TransferProtocolCombo->GetItems()->Add(GetMsg(LOGIN_SFTP));
  TransferProtocolCombo->GetItems()->Add(GetMsg(LOGIN_SCP));
#ifndef NO_FILEZILLA
  TransferProtocolCombo->GetItems()->Add(GetMsg(LOGIN_FTP));
#endif
  TransferProtocolCombo->GetItems()->Add(GetMsg(LOGIN_WEBDAV));

  AllowScpFallbackCheck = new TFarCheckBox(this);
  AllowScpFallbackCheck->SetCaption(GetMsg(LOGIN_ALLOW_SCP_FALLBACK));

  InsecureLabel = new TFarText(this);
  InsecureLabel->SetCaption(GetMsg(LOGIN_INSECURE));
  InsecureLabel->MoveAt(AllowScpFallbackCheck->GetLeft(), AllowScpFallbackCheck->GetTop());

  SetNextItemPosition(ipNewLine);

  FtpEncryptionLabel = new TFarText(this);
  FtpEncryptionLabel->SetCaption(GetMsg(LOGIN_FTP_ENCRYPTION));
  FtpEncryptionLabel->SetWidth(15);

  SetNextItemPosition(ipRight);

  FtpEncryptionCombo = new TFarComboBox(this);
  FtpEncryptionCombo->SetDropDownList(true);
  FtpEncryptionCombo->GetItems()->Add(GetMsg(LOGIN_FTP_USE_PLAIN_FTP));
  FtpEncryptionCombo->GetItems()->Add(GetMsg(LOGIN_FTP_REQUIRE_IMPLICIT_FTP));
  FtpEncryptionCombo->GetItems()->Add(GetMsg(LOGIN_FTP_REQUIRE_EXPLICIT_FTP));
  FtpEncryptionCombo->SetRight(CRect.Right);
  FtpEncryptionCombo->SetWidth(30);

  SetNextItemPosition(ipNewLine);

  new TFarSeparator(this);

  HostNameLabel = new TFarText(this);
  HostNameLabel->SetCaption(GetMsg(LOGIN_HOST_NAME));

  HostNameEdit = new TFarEdit(this);
  HostNameEdit->SetRight(CRect.Right - 12 - 2);

  SetNextItemPosition(ipRight);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_PORT_NUMBER));
  Text->Move(0, -1);

  SetNextItemPosition(ipBelow);

  PortNumberEdit = new TFarEdit(this);
  PortNumberEdit->SetFixed(true);
  PortNumberEdit->SetMask(L"999999");

  SetNextItemPosition(ipNewLine);
  Text = new TFarText(this);
  SetNextItemPosition(ipNewLine);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_USER_NAME));
  Text->SetWidth(20);

  SetNextItemPosition(ipRight);

  UserNameEdit = new TFarEdit(this);
  UserNameEdit->SetWidth(20);
  UserNameEdit->SetRight(CRect.Right - 12 - 2);

  SetNextItemPosition(ipNewLine);
  Text = new TFarText(this);
  SetNextItemPosition(ipNewLine);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_PASSWORD));
  Text->SetWidth(20);

  SetNextItemPosition(ipRight);

  PasswordEdit = new TFarEdit(this);
  PasswordEdit->SetPassword(true);
  PasswordEdit->SetWidth(20);
  PasswordEdit->SetRight(CRect.Right - 12 - 2);

  SetNextItemPosition(ipNewLine);
  Text = new TFarText(this);
  SetNextItemPosition(ipNewLine);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_PRIVATE_KEY));

  PrivateKeyEdit = new TFarEdit(this);
  Text->SetEnabledFollow(PrivateKeyEdit);

  Separator = new TFarSeparator(this);

  Text = new TFarText(this);
  Text->SetTop(CRect.Bottom - 3);
  Text->SetBottom(Text->GetTop());
  Text->SetCaption(GetMsg(LOGIN_TAB_HINT1));
  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_TAB_HINT2));

  // Environment tab

  SetDefaultGroup(tabEnvironment);
  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(LOGIN_ENVIRONMENT_GROUP));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_EOL_TYPE));

  SetNextItemPosition(ipRight);

  EOLTypeCombo = new TFarComboBox(this);
  EOLTypeCombo->SetDropDownList(true);
  EOLTypeCombo->SetWidth(7);
  EOLTypeCombo->SetRight(CRect.Right);
  EOLTypeCombo->GetItems()->Add(L"LF");
  EOLTypeCombo->GetItems()->Add(L"CR/LF");

  SetNextItemPosition(ipNewLine);

  // UTF_TRISTATE();
  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_CODE_PAGE));

  SetNextItemPosition(ipRight);

  CodePageEdit = new TFarComboBox(this);
  CodePageEdit->SetWidth(30);
  CodePageEdit->SetRight(CRect.Right);
  FillCodePageEdit();

  SetNextItemPosition(ipNewLine);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_TIME_DIFFERENCE));

  SetNextItemPosition(ipRight);

  TimeDifferenceEdit = new TFarEdit(this);
  TimeDifferenceEdit->SetFixed(true);
  TimeDifferenceEdit->SetMask(L"###");
  TimeDifferenceEdit->SetWidth(4);
  Text->SetEnabledFollow(TimeDifferenceEdit);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_TIME_DIFFERENCE_HOURS));
  Text->SetEnabledFollow(TimeDifferenceEdit);

  TimeDifferenceMinutesEdit = new TFarEdit(this);
  TimeDifferenceMinutesEdit->SetFixed(true);
  TimeDifferenceMinutesEdit->SetMask(L"###");
  TimeDifferenceMinutesEdit->SetWidth(4);
  TimeDifferenceMinutesEdit->SetEnabledFollow(TimeDifferenceEdit);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_TIME_DIFFERENCE_MINUTES));
  Text->SetEnabledFollow(TimeDifferenceEdit);

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(LOGIN_DST_MODE_GROUP));

  DSTModeUnixCheck = new TFarRadioButton(this);
  DSTModeUnixCheck->SetCaption(GetMsg(LOGIN_DST_MODE_UNIX));

  DSTModeWinCheck = new TFarRadioButton(this);
  DSTModeWinCheck->SetCaption(GetMsg(LOGIN_DST_MODE_WIN));
  DSTModeWinCheck->SetEnabledFollow(DSTModeUnixCheck);

  DSTModeKeepCheck = new TFarRadioButton(this);
  DSTModeKeepCheck->SetCaption(GetMsg(LOGIN_DST_MODE_KEEP));
  DSTModeKeepCheck->SetEnabledFollow(DSTModeUnixCheck);

  Button = new TFarButton(this);
  Button->SetCaption(GetMsg(LOGIN_ENVIRONMENT_UNIX));
  Button->SetOnClick(MAKE_CALLBACK(TSessionDialog::UnixEnvironmentButtonClick, this));
  Button->SetCenterGroup(true);

  SetNextItemPosition(ipRight);

  Button = new TFarButton(this);
  Button->SetCaption(GetMsg(LOGIN_ENVIRONMENT_WINDOWS));
  Button->SetOnClick(MAKE_CALLBACK(TSessionDialog::WindowsEnvironmentButtonClick, this));
  Button->SetCenterGroup(true);

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(LOGIN_RECYCLE_BIN_GROUP));

  DeleteToRecycleBinCheck = new TFarCheckBox(this);
  DeleteToRecycleBinCheck->SetCaption(GetMsg(LOGIN_RECYCLE_BIN_DELETE));

  OverwrittenToRecycleBinCheck = new TFarCheckBox(this);
  OverwrittenToRecycleBinCheck->SetCaption(GetMsg(LOGIN_RECYCLE_BIN_OVERWRITE));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_RECYCLE_BIN_LABEL));

  RecycleBinPathEdit = new TFarEdit(this);
  Text->SetEnabledFollow(RecycleBinPathEdit);

  SetNextItemPosition(ipNewLine);

  // Directories tab

  SetDefaultGroup(tabDirectories);
  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(LOGIN_DIRECTORIES_GROUP));

  UpdateDirectoriesCheck = new TFarCheckBox(this);
  UpdateDirectoriesCheck->SetCaption(GetMsg(LOGIN_UPDATE_DIRECTORIES));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_REMOTE_DIRECTORY));

  RemoteDirectoryEdit = new TFarEdit(this);
  RemoteDirectoryEdit->SetHistory(REMOTE_DIR_HISTORY);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(LOGIN_DIRECTORY_OPTIONS_GROUP));

  CacheDirectoriesCheck = new TFarCheckBox(this);
  CacheDirectoriesCheck->SetCaption(GetMsg(LOGIN_CACHE_DIRECTORIES));

  CacheDirectoryChangesCheck = new TFarCheckBox(this);
  CacheDirectoryChangesCheck->SetCaption(GetMsg(LOGIN_CACHE_DIRECTORY_CHANGES));

  PreserveDirectoryChangesCheck = new TFarCheckBox(this);
  PreserveDirectoryChangesCheck->SetCaption(GetMsg(LOGIN_PRESERVE_DIRECTORY_CHANGES));
  PreserveDirectoryChangesCheck->SetLeft(PreserveDirectoryChangesCheck->GetLeft() + 4);

  ResolveSymlinksCheck = new TFarCheckBox(this);
  ResolveSymlinksCheck->SetCaption(GetMsg(LOGIN_RESOLVE_SYMLINKS));

  new TFarSeparator(this);

  // SCP Tab

  SetNextItemPosition(ipNewLine);

  SetDefaultGroup(tabSCP);

  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(LOGIN_SHELL_GROUP));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_SHELL_SHELL));

  SetNextItemPosition(ipRight);

  ShellEdit = new TFarComboBox(this);
  ShellEdit->GetItems()->Add(GetMsg(LOGIN_SHELL_SHELL_DEFAULT));
  ShellEdit->GetItems()->Add(L"/bin/bash");
  ShellEdit->GetItems()->Add(L"/bin/ksh");

  SetNextItemPosition(ipNewLine);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_SHELL_RETURN_VAR));

  SetNextItemPosition(ipRight);

  ReturnVarEdit = new TFarComboBox(this);
  ReturnVarEdit->GetItems()->Add(GetMsg(LOGIN_SHELL_RETURN_VAR_AUTODETECT));
  ReturnVarEdit->GetItems()->Add(L"?");
  ReturnVarEdit->GetItems()->Add(L"status");

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(LOGIN_SCP_LS_OPTIONS_GROUP));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_LISTING_COMMAND));

  SetNextItemPosition(ipRight);

  ListingCommandEdit = new TFarComboBox(this);
  ListingCommandEdit->GetItems()->Add(L"ls -la");
  ListingCommandEdit->GetItems()->Add(L"ls -gla");
  Text->SetEnabledFollow(ListingCommandEdit);

  SetNextItemPosition(ipNewLine);

  IgnoreLsWarningsCheck = new TFarCheckBox(this);
  IgnoreLsWarningsCheck->SetCaption(GetMsg(LOGIN_IGNORE_LS_WARNINGS));

  SetNextItemPosition(ipRight);

  SCPLsFullTimeAutoCheck = new TFarCheckBox(this);
  SCPLsFullTimeAutoCheck->SetCaption(GetMsg(LOGIN_SCP_LS_FULL_TIME_AUTO));

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(LOGIN_SCP_OPTIONS));

  LookupUserGroupsCheck = new TFarCheckBox(this);
  LookupUserGroupsCheck->SetCaption(GetMsg(LOGIN_LOOKUP_USER_GROUPS));

  SetNextItemPosition(ipRight);

  UnsetNationalVarsCheck = new TFarCheckBox(this);
  UnsetNationalVarsCheck->SetCaption(GetMsg(LOGIN_CLEAR_NATIONAL_VARS));

  SetNextItemPosition(ipNewLine);

  ClearAliasesCheck = new TFarCheckBox(this);
  ClearAliasesCheck->SetCaption(GetMsg(LOGIN_CLEAR_ALIASES));

  SetNextItemPosition(ipRight);

  Scp1CompatibilityCheck = new TFarCheckBox(this);
  Scp1CompatibilityCheck->SetCaption(GetMsg(LOGIN_SCP1_COMPATIBILITY));

  SetNextItemPosition(ipNewLine);

  new TFarSeparator(this);

  // SFTP Tab

  SetNextItemPosition(ipNewLine);

  SetDefaultGroup(tabSFTP);

  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(LOGIN_SFTP_PROTOCOL_GROUP));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_SFTP_SERVER));
  SetNextItemPosition(ipRight);
  SftpServerEdit = new TFarComboBox(this);
  SftpServerEdit->GetItems()->Add(GetMsg(LOGIN_SFTP_SERVER_DEFAULT));
  SftpServerEdit->GetItems()->Add(L"/bin/sftp-server");
  SftpServerEdit->GetItems()->Add(L"sudo su -c /bin/sftp-server");
  Text->SetEnabledFollow(SftpServerEdit);

  SetNextItemPosition(ipNewLine);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_SFTP_MAX_VERSION));
  SetNextItemPosition(ipRight);
  SFTPMaxVersionCombo = new TFarComboBox(this);
  SFTPMaxVersionCombo->SetDropDownList(true);
  SFTPMaxVersionCombo->SetWidth(7);
  for (intptr_t Index2 = 0; Index2 <= 5; ++Index2)
  {
    SFTPMaxVersionCombo->GetItems()->Add(::IntToStr(Index2));
  }
  Text->SetEnabledFollow(SFTPMaxVersionCombo);

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(LOGIN_SFTP_BUGS_GROUP));

  SFTP_BUGS();

  new TFarSeparator(this);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_SFTP_MIN_PACKET_SIZE));
  SetNextItemPosition(ipRight);

  SFTPMinPacketSizeEdit = new TFarEdit(this);
  SFTPMinPacketSizeEdit->SetFixed(true);
  SFTPMinPacketSizeEdit->SetMask(L"99999999");
  SFTPMinPacketSizeEdit->SetWidth(8);
  // SFTPMinPacketSizeEdit->SetEnabledDependencyNegative(SshProt1onlyButton);

  SetNextItemPosition(ipNewLine);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_SFTP_MAX_PACKET_SIZE));
  SetNextItemPosition(ipRight);

  SFTPMaxPacketSizeEdit = new TFarEdit(this);
  SFTPMaxPacketSizeEdit->SetFixed(true);
  SFTPMaxPacketSizeEdit->SetMask(L"99999999");
  SFTPMaxPacketSizeEdit->SetWidth(8);
  // SFTPMaxPacketSizeEdit->SetEnabledDependencyNegative(SshProt1onlyButton);

  // FTP tab

  SetNextItemPosition(ipNewLine);

  SetDefaultGroup(tabFTP);

  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(LOGIN_FTP_GROUP));

  TRISTATE(FtpUseMlsdCombo, FtpUseMlsd, LOGIN_FTP_USE_MLSD);

  FtpPasvModeCheck = new TFarCheckBox(this);
  FtpPasvModeCheck->SetCaption(GetMsg(LOGIN_FTP_PASV_MODE));

  FtpAllowEmptyPasswordCheck = new TFarCheckBox(this);
  FtpAllowEmptyPasswordCheck->SetCaption(GetMsg(LOGIN_FTP_ALLOW_EMPTY_PASSWORD));

  SetNextItemPosition(ipNewLine);

  FtpDupFFCheck = new TFarCheckBox(this);
  FtpDupFFCheck->SetCaption(GetMsg(LOGIN_FTP_DUPFF));

  SetNextItemPosition(ipNewLine);

  FtpUndupFFCheck = new TFarCheckBox(this);
  FtpUndupFFCheck->SetCaption(GetMsg(LOGIN_FTP_UNDUPFF));

  SetNextItemPosition(ipNewLine);

  SslSessionReuseCheck = new TFarCheckBox(this);
  SslSessionReuseCheck->SetCaption(GetMsg(LOGIN_FTP_SSLSESSIONREUSE));

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_FTP_POST_LOGIN_COMMANDS));

  for (intptr_t Index3 = 0; Index3 < static_cast<intptr_t>(_countof(PostLoginCommandsEdits)); ++Index3)
  {
    TFarEdit * Edit = new TFarEdit(this);
    PostLoginCommandsEdits[Index3] = Edit;
  }

  new TFarSeparator(this);

  // Connection tab

  SetNextItemPosition(ipNewLine);

  SetDefaultGroup(tabConnection);

  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(LOGIN_CONNECTION_GROUP));

  Text = new TFarText(this);
  SetNextItemPosition(ipNewLine);

  SshBufferSizeCheck = new TFarCheckBox(this);
  SshBufferSizeCheck->SetCaption(GetMsg(LOGIN_SSH_OPTIMIZE_BUFFER_SIZE));

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(LOGIN_TIMEOUTS_GROUP));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_TIMEOUT));

  SetNextItemPosition(ipRight);

  TimeoutEdit = new TFarEdit(this);
  TimeoutEdit->SetFixed(true);
  TimeoutEdit->SetMask(L"####");
  TimeoutEdit->SetWidth(5);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_TIMEOUT_SECONDS));

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(LOGIN_PING_GROUP));

  PingOffButton = new TFarRadioButton(this);
  PingOffButton->SetCaption(GetMsg(LOGIN_PING_OFF));

  PingNullPacketButton = new TFarRadioButton(this);
  PingNullPacketButton->SetCaption(GetMsg(LOGIN_PING_NULL_PACKET));

  PingDummyCommandButton = new TFarRadioButton(this);
  PingDummyCommandButton->SetCaption(GetMsg(LOGIN_PING_DUMMY_COMMAND));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_PING_INTERVAL));
  Text->SetEnabledDependencyNegative(PingOffButton);

  SetNextItemPosition(ipRight);

  PingIntervalSecEdit = new TFarEdit(this);
  PingIntervalSecEdit->SetFixed(true);
  PingIntervalSecEdit->SetMask(L"####");
  PingIntervalSecEdit->SetWidth(6);
  PingIntervalSecEdit->SetEnabledDependencyNegative(PingOffButton);

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(LOGIN_IP_GROUP));

  IPAutoButton = new TFarRadioButton(this);
  IPAutoButton->SetCaption(GetMsg(LOGIN_IP_AUTO));

  SetNextItemPosition(ipRight);

  IPv4Button = new TFarRadioButton(this);
  IPv4Button->SetCaption(GetMsg(LOGIN_IP_V4));

  IPv6Button = new TFarRadioButton(this);
  IPv6Button->SetCaption(GetMsg(LOGIN_IP_V6));

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);

  SetNextItemPosition(ipNewLine);

  // Proxy tab

  SetDefaultGroup(tabProxy);
  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(LOGIN_PROXY_GROUP));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_PROXY_METHOD));

  SetNextItemPosition(ipRight);

  FtpProxyMethodCombo = new TFarComboBox(this);
  FtpProxyMethodCombo->SetDropDownList(true);
  FtpProxyMethodComboAddNewItem(LOGIN_PROXY_NONE, pmNone);
  FtpProxyMethodComboAddNewItem(LOGIN_PROXY_SOCKS4, pmSocks4);
  FtpProxyMethodComboAddNewItem(LOGIN_PROXY_SOCKS5, pmSocks5);
  FtpProxyMethodComboAddNewItem(LOGIN_PROXY_HTTP, pmHTTP);
  FtpProxyMethodComboAddNewItem(LOGIN_PROXY_SYSTEM, pmSystem);
  TProxyMethod FtpProxyMethod = static_cast<TProxyMethod>(GetLastSupportedFtpProxyMethod());
  FtpProxyMethodComboAddNewItem(LOGIN_PROXY_FTP_SITE, static_cast<TProxyMethod>(FtpProxyMethod + 1));
  FtpProxyMethodComboAddNewItem(LOGIN_PROXY_FTP_PROXYUSER_USERHOST, static_cast<TProxyMethod>(FtpProxyMethod + 2));
  FtpProxyMethodComboAddNewItem(LOGIN_PROXY_FTP_OPEN_HOST, static_cast<TProxyMethod>(FtpProxyMethod + 3));
  FtpProxyMethodComboAddNewItem(LOGIN_PROXY_FTP_PROXYUSER_USERUSER, static_cast<TProxyMethod>(FtpProxyMethod + 4));
  FtpProxyMethodComboAddNewItem(LOGIN_PROXY_FTP_USER_USERHOST, static_cast<TProxyMethod>(FtpProxyMethod + 5));
  FtpProxyMethodComboAddNewItem(LOGIN_PROXY_FTP_PROXYUSER_HOST, static_cast<TProxyMethod>(FtpProxyMethod + 6));
  FtpProxyMethodComboAddNewItem(LOGIN_PROXY_FTP_USERHOST_PROXYUSER, static_cast<TProxyMethod>(FtpProxyMethod + 7));
  FtpProxyMethodComboAddNewItem(LOGIN_PROXY_FTP_USER_USERPROXYUSERHOST, static_cast<TProxyMethod>(FtpProxyMethod + 8));
  FtpProxyMethodCombo->SetWidth(40);

  SshProxyMethodCombo = new TFarComboBox(this);
  SshProxyMethodCombo->SetLeft(FtpProxyMethodCombo->GetLeft());
  SshProxyMethodCombo->SetWidth(FtpProxyMethodCombo->GetWidth());
  SshProxyMethodCombo->SetRight(FtpProxyMethodCombo->GetRight());
  SshProxyMethodCombo->SetDropDownList(true);

  SshProxyMethodComboAddNewItem(LOGIN_PROXY_NONE, pmNone);
  SshProxyMethodComboAddNewItem(LOGIN_PROXY_SOCKS4, pmSocks4);
  SshProxyMethodComboAddNewItem(LOGIN_PROXY_SOCKS5, pmSocks5);
  SshProxyMethodComboAddNewItem(LOGIN_PROXY_HTTP, pmHTTP);
  SshProxyMethodComboAddNewItem(LOGIN_PROXY_TELNET, pmTelnet);
  SshProxyMethodComboAddNewItem(LOGIN_PROXY_LOCAL, pmCmd);
  SshProxyMethodComboAddNewItem(LOGIN_PROXY_SYSTEM, pmSystem);

  SetNextItemPosition(ipNewLine);

  new TFarSeparator(this);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_PROXY_HOST));

  SetNextItemPosition(ipNewLine);

  ProxyHostEdit = new TFarEdit(this);
  ProxyHostEdit->SetWidth(42);
  Text->SetEnabledFollow(ProxyHostEdit);

  SetNextItemPosition(ipRight);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_PROXY_PORT));
  Text->Move(0, -1);

  SetNextItemPosition(ipBelow);

  ProxyPortEdit = new TFarEdit(this);
  ProxyPortEdit->SetFixed(true);
  ProxyPortEdit->SetMask(L"999999");
  // ProxyPortEdit->SetWidth(12);
  Text->SetEnabledFollow(ProxyPortEdit);

  SetNextItemPosition(ipNewLine);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_PROXY_USERNAME));

  ProxyUsernameEdit = new TFarEdit(this);
  ProxyUsernameEdit->SetWidth(ProxyUsernameEdit->GetWidth() / 2 - 1);
  Text->SetEnabledFollow(ProxyUsernameEdit);

  SetNextItemPosition(ipRight);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_PROXY_PASSWORD));
  Text->Move(0, -1);

  SetNextItemPosition(ipBelow);

  ProxyPasswordEdit = new TFarEdit(this);
  ProxyPasswordEdit->SetPassword(true);
  Text->SetEnabledFollow(ProxyPasswordEdit);

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(LOGIN_PROXY_SETTINGS_GROUP));

  ProxyTelnetCommandLabel = new TFarText(this);
  ProxyTelnetCommandLabel->SetCaption(GetMsg(LOGIN_PROXY_TELNET_COMMAND));

  SetNextItemPosition(ipRight);

  ProxyTelnetCommandEdit = new TFarEdit(this);
  ProxyTelnetCommandLabel->SetEnabledFollow(ProxyTelnetCommandEdit);

  SetNextItemPosition(ipNewLine);

  ProxyLocalCommandLabel = new TFarText(this);
  ProxyLocalCommandLabel->SetCaption(GetMsg(LOGIN_PROXY_LOCAL_COMMAND));
  ProxyLocalCommandLabel->Move(0, -1);

  SetNextItemPosition(ipRight);

  ProxyLocalCommandEdit = new TFarEdit(this);
  ProxyLocalCommandLabel->SetEnabledFollow(ProxyLocalCommandEdit);

  SetNextItemPosition(ipNewLine);

  ProxyLocalhostCheck = new TFarCheckBox(this);
  ProxyLocalhostCheck->SetCaption(GetMsg(LOGIN_PROXY_LOCALHOST));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_PROXY_DNS));

  ProxyDNSOffButton = new TFarRadioButton(this);
  ProxyDNSOffButton->SetCaption(GetMsg(LOGIN_PROXY_DNS_NO));
  Text->SetEnabledFollow(ProxyDNSOffButton);

  SetNextItemPosition(ipRight);

  ProxyDNSAutoButton = new TFarRadioButton(this);
  ProxyDNSAutoButton->SetCaption(GetMsg(LOGIN_PROXY_DNS_AUTO));
  ProxyDNSAutoButton->SetEnabledFollow(ProxyDNSOffButton);

  ProxyDNSOnButton = new TFarRadioButton(this);
  ProxyDNSOnButton->SetCaption(GetMsg(LOGIN_PROXY_DNS_YES));
  ProxyDNSOnButton->SetEnabledFollow(ProxyDNSOffButton);

  SetNextItemPosition(ipNewLine);

  new TFarSeparator(this);

  // Tunnel tab

  SetNextItemPosition(ipNewLine);

  SetDefaultGroup(tabTunnel);
  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(LOGIN_TUNNEL_GROUP));

  TunnelCheck = new TFarCheckBox(this);
  TunnelCheck->SetCaption(GetMsg(LOGIN_TUNNEL_TUNNEL));

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(LOGIN_TUNNEL_SESSION_GROUP));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_HOST_NAME));
  Text->SetEnabledDependency(TunnelCheck);

  TunnelHostNameEdit = new TFarEdit(this);
  TunnelHostNameEdit->SetRight(CRect.Right - 12 - 2);
  TunnelHostNameEdit->SetEnabledDependency(TunnelCheck);

  SetNextItemPosition(ipRight);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_PORT_NUMBER));
  Text->Move(0, -1);
  Text->SetEnabledDependency(TunnelCheck);

  SetNextItemPosition(ipBelow);

  TunnelPortNumberEdit = new TFarEdit(this);
  TunnelPortNumberEdit->SetFixed(true);
  TunnelPortNumberEdit->SetMask(L"999999");
  TunnelPortNumberEdit->SetEnabledDependency(TunnelCheck);

  SetNextItemPosition(ipNewLine);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_USER_NAME));
  Text->SetEnabledDependency(TunnelCheck);

  TunnelUserNameEdit = new TFarEdit(this);
  TunnelUserNameEdit->SetWidth(TunnelUserNameEdit->GetWidth() / 2 - 1);
  TunnelUserNameEdit->SetEnabledDependency(TunnelCheck);

  SetNextItemPosition(ipRight);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_PASSWORD));
  Text->Move(0, -1);
  Text->SetEnabledDependency(TunnelCheck);

  SetNextItemPosition(ipBelow);

  TunnelPasswordEdit = new TFarEdit(this);
  TunnelPasswordEdit->SetPassword(true);
  TunnelPasswordEdit->SetEnabledDependency(TunnelCheck);

  SetNextItemPosition(ipNewLine);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_PRIVATE_KEY));
  Text->SetEnabledDependency(TunnelCheck);

  TunnelPrivateKeyEdit = new TFarEdit(this);
  TunnelPrivateKeyEdit->SetEnabledDependency(TunnelCheck);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(LOGIN_TUNNEL_OPTIONS_GROUP));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_TUNNEL_LOCAL_PORT_NUMBER));
  Text->SetEnabledDependency(TunnelCheck);

  SetNextItemPosition(ipRight);

  TunnelLocalPortNumberEdit = new TFarComboBox(this);
  TunnelLocalPortNumberEdit->SetLeft(TunnelPortNumberEdit->GetLeft());
  TunnelLocalPortNumberEdit->SetEnabledDependency(TunnelCheck);
  TunnelLocalPortNumberEdit->GetItems()->BeginUpdate();
  {
    SCOPE_EXIT
    {
      TunnelLocalPortNumberEdit->GetItems()->EndUpdate();
    };
    TunnelLocalPortNumberEdit->GetItems()->Add(GetMsg(LOGIN_TUNNEL_LOCAL_PORT_NUMBER_AUTOASSIGN));
    for (intptr_t Index4 = GetConfiguration()->GetTunnelLocalPortNumberLow();
         Index4 <= GetConfiguration()->GetTunnelLocalPortNumberHigh(); ++Index4)
    {
      TunnelLocalPortNumberEdit->GetItems()->Add(::IntToStr(Index1));
    }
  }

  SetNextItemPosition(ipNewLine);

  new TFarSeparator(this);

  // SSH tab

  SetNextItemPosition(ipNewLine);

  SetDefaultGroup(tabSsh);
  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(LOGIN_SSH_GROUP));

  CompressionCheck = new TFarCheckBox(this);
  CompressionCheck->SetCaption(GetMsg(LOGIN_COMPRESSION));

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(LOGIN_SSH_PROTOCOL_GROUP));

  SshProt1onlyButton = new TFarRadioButton(this);
  SshProt1onlyButton->SetCaption(GetMsg(LOGIN_SSH1_ONLY));

  SetNextItemPosition(ipRight);

  SshProt1Button = new TFarRadioButton(this);
  SshProt1Button->SetCaption(GetMsg(LOGIN_SSH1));

  SshProt2Button = new TFarRadioButton(this);
  SshProt2Button->SetCaption(GetMsg(LOGIN_SSH2));

  SshProt2onlyButton = new TFarRadioButton(this);
  SshProt2onlyButton->SetCaption(GetMsg(LOGIN_SSH2_ONLY));

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(LOGIN_ENCRYPTION_GROUP));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_CIPHER));

  CipherListBox = new TFarListBox(this);
  CipherListBox->SetRight(CipherListBox->GetRight() - 15);
  CipherListBox->SetHeight(1 + CIPHER_COUNT + 1);
  Pos = CipherListBox->GetBottom();

  SetNextItemPosition(ipRight);

  CipherUpButton = new TFarButton(this);
  CipherUpButton->SetCaption(GetMsg(LOGIN_UP));
  CipherUpButton->Move(0, 1);
  CipherUpButton->SetResult(-1);
  CipherUpButton->SetOnClick(MAKE_CALLBACK(TSessionDialog::CipherButtonClick, this));

  SetNextItemPosition(ipBelow);

  CipherDownButton = new TFarButton(this);
  CipherDownButton->SetCaption(GetMsg(LOGIN_DOWN));
  CipherDownButton->SetResult(1);
  CipherDownButton->SetOnClick(MAKE_CALLBACK(TSessionDialog::CipherButtonClick, this));

  SetNextItemPosition(ipNewLine);

  if (!Limited)
  {
    Ssh2DESCheck = new TFarCheckBox(this);
    Ssh2DESCheck->Move(0, Pos - Ssh2DESCheck->GetTop() + 1);
    Ssh2DESCheck->SetCaption(GetMsg(LOGIN_SSH2DES));
    Ssh2DESCheck->SetEnabledDependencyNegative(SshProt1onlyButton);
  }
  else
  {
    Ssh2DESCheck = nullptr;
  }

  // KEX tab

  SetDefaultGroup(tabKex);
  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(LOGIN_KEX_REEXCHANGE_GROUP));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_KEX_REKEY_TIME));
  Text->SetEnabledDependencyNegative(SshProt1onlyButton);

  SetNextItemPosition(ipRight);

  RekeyTimeEdit = new TFarEdit(this);
  RekeyTimeEdit->SetFixed(true);
  RekeyTimeEdit->SetMask(L"####");
  RekeyTimeEdit->SetWidth(6);
  RekeyTimeEdit->SetEnabledDependencyNegative(SshProt1onlyButton);

  SetNextItemPosition(ipNewLine);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_KEX_REKEY_DATA));
  Text->SetEnabledDependencyNegative(SshProt1onlyButton);

  SetNextItemPosition(ipRight);

  RekeyDataEdit = new TFarEdit(this);
  RekeyDataEdit->SetWidth(6);
  RekeyDataEdit->SetEnabledDependencyNegative(SshProt1onlyButton);

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(LOGIN_KEX_OPTIONS_GROUP));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(LOGIN_KEX_LIST));
  Text->SetEnabledDependencyNegative(SshProt1onlyButton);

  KexListBox = new TFarListBox(this);
  KexListBox->SetRight(KexListBox->GetRight() - 15);
  KexListBox->SetHeight(1 + KEX_COUNT + 1);
  KexListBox->SetEnabledDependencyNegative(SshProt1onlyButton);

  SetNextItemPosition(ipRight);

  KexUpButton = new TFarButton(this);
  KexUpButton->SetCaption(GetMsg(LOGIN_UP));
  KexUpButton->Move(0, 1);
  KexUpButton->SetResult(-1);
  KexUpButton->SetOnClick(MAKE_CALLBACK(TSessionDialog::KexButtonClick, this));

  SetNextItemPosition(ipBelow);

  KexDownButton = new TFarButton(this);
  KexDownButton->SetCaption(GetMsg(LOGIN_DOWN));
  KexDownButton->SetResult(1);
  KexDownButton->SetOnClick(MAKE_CALLBACK(TSessionDialog::KexButtonClick, this));

  SetNextItemPosition(ipNewLine);

  // Authentication tab

  SetDefaultGroup(tabAuthentication);

  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);

  SshNoUserAuthCheck = new TFarCheckBox(this);
  SshNoUserAuthCheck->SetCaption(GetMsg(LOGIN_AUTH_SSH_NO_USER_AUTH));

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(LOGIN_AUTH_GROUP));

  TryAgentCheck = new TFarCheckBox(this);
  TryAgentCheck->SetCaption(GetMsg(LOGIN_AUTH_TRY_AGENT));

  AuthTISCheck = new TFarCheckBox(this);
  AuthTISCheck->SetCaption(GetMsg(LOGIN_AUTH_TIS));

  AuthKICheck = new TFarCheckBox(this);
  AuthKICheck->SetCaption(GetMsg(LOGIN_AUTH_KI));

  AuthKIPasswordCheck = new TFarCheckBox(this);
  AuthKIPasswordCheck->SetCaption(GetMsg(LOGIN_AUTH_KI_PASSWORD));
  AuthKIPasswordCheck->Move(4, 0);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(LOGIN_AUTH_PARAMS_GROUP));

  AgentFwdCheck = new TFarCheckBox(this);
  AgentFwdCheck->SetCaption(GetMsg(LOGIN_AUTH_AGENT_FWD));

  // GSSAPI
  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(LOGIN_AUTH_GSSAPI_PARAMS_GROUP));

  AuthGSSAPICheck3 = new TFarCheckBox(this);
  AuthGSSAPICheck3->SetCaption(GetMsg(LOGIN_AUTH_ATTEMPT_GSSAPI_AUTHENTICATION));
  AuthGSSAPICheck3->SetOnAllowChange(MAKE_CALLBACK(TSessionDialog::AuthGSSAPICheckAllowChange, this));

  GSSAPIFwdTGTCheck = new TFarCheckBox(this);
  GSSAPIFwdTGTCheck->SetCaption(GetMsg(LOGIN_AUTH_ALLOW_GSSAPI_CREDENTIAL_DELEGATION));

  new TFarSeparator(this);

  // Bugs tab

  SetDefaultGroup(tabBugs);

  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(LOGIN_BUGS_GROUP));

  BUGS();

  BugIgnore1Combo->SetEnabledDependencyNegative(SshProt2onlyButton);
  BugPlainPW1Combo->SetEnabledDependencyNegative(SshProt2onlyButton);
  BugRSA1Combo->SetEnabledDependencyNegative(SshProt2onlyButton);
  BugHMAC2Combo->SetEnabledDependencyNegative(SshProt1onlyButton);
  BugDeriveKey2Combo->SetEnabledDependencyNegative(SshProt1onlyButton);
  BugRSAPad2Combo->SetEnabledDependencyNegative(SshProt1onlyButton);
  BugPKSessID2Combo->SetEnabledDependencyNegative(SshProt1onlyButton);
  BugRekey2Combo->SetEnabledDependencyNegative(SshProt1onlyButton);

  // WebDAV tab

  SetNextItemPosition(ipNewLine);

  SetDefaultGroup(tabWebDAV);
  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(LOGIN_WEBDAV_GROUP));

  WebDAVCompressionCheck = new TFarCheckBox(this);
  WebDAVCompressionCheck->SetCaption(GetMsg(LOGIN_COMPRESSION));

#undef TRISTATE

  new TFarSeparator(this);

  // Buttons

  SetNextItemPosition(ipNewLine);
  SetDefaultGroup(0);

  Separator = new TFarSeparator(this);
  Separator->SetPosition(CRect.Bottom - 1);

  Button = new TFarButton(this);
  Button->SetCaption(GetMsg(MSG_BUTTON_OK));
  Button->SetDefault(Action != saConnect);
  Button->SetResult(brOK);
  Button->SetCenterGroup(true);

  SetNextItemPosition(ipRight);

  ConnectButton = new TFarButton(this);
  ConnectButton->SetCaption(GetMsg(LOGIN_CONNECT_BUTTON));
  ConnectButton->SetDefault(Action == saConnect);
  ConnectButton->SetResult(brConnect);
  ConnectButton->SetCenterGroup(true);

  Button = new TFarButton(this);
  Button->SetCaption(GetMsg(MSG_BUTTON_Cancel));
  Button->SetResult(brCancel);
  Button->SetCenterGroup(true);
}

void TSessionDialog::FtpProxyMethodComboAddNewItem(int ProxyTypeId, TProxyMethod ProxyType)
{
  FtpProxyMethodCombo->GetItems()->AddObject(GetMsg(ProxyTypeId),
    static_cast<TObject *>(ToPtr(ProxyType)));
}

void TSessionDialog::SshProxyMethodComboAddNewItem(int ProxyTypeId, TProxyMethod ProxyType)
{
  SshProxyMethodCombo->GetItems()->AddObject(GetMsg(ProxyTypeId),
    static_cast<TObject *>(ToPtr(ProxyType)));
}

TSessionDialog::~TSessionDialog()
{
  SAFE_DESTROY(FTabs);
}

void TSessionDialog::Change()
{
  TTabbedDialog::Change();

  if (GetHandle() && !ChangesLocked())
  {
    if ((FTransferProtocolIndex != TransferProtocolCombo->GetItemIndex()) ||
        (FFtpEncryptionComboIndex != FtpEncryptionCombo->GetItemIndex()))
    {
      TransferProtocolComboChange();
    }

    LockChanges();
    SCOPE_EXIT
    {
      UnlockChanges();
    };
    UpdateControls();
  }
}

void TSessionDialog::Init()
{
  TTabbedDialog::Init();

}

static void AdjustRemoteDir(
  UnicodeString & HostName,
  TFarEdit * PortNumberEdit,
  TFarEdit * RemoteDirectoryEdit)
{
  UnicodeString Dir;
  intptr_t P = HostName.Pos(LOTHER_SLASH);
  if (P > 0)
  {
    Dir = HostName.SubString(P, HostName.Length() - P + 1);
    intptr_t P2 = Dir.Pos(L':');
    if (P2 > 0)
    {
      UnicodeString Port = Dir.SubString(P2 + 1, Dir.Length() - P2);
      Dir.SetLength(P2 - 1);
      if (Port.ToInt())
        PortNumberEdit->SetAsInteger(Port.ToInt());
    }
    HostName.SetLength(P - 1);
  }
  UnicodeString RemoteDir = RemoteDirectoryEdit->GetText();
  if (RemoteDir.IsEmpty() && !Dir.IsEmpty())
  {
    RemoteDirectoryEdit->SetText(Dir);
  }
}

void TSessionDialog::TransferProtocolComboChange()
{
  TFtps Ftps = GetFtps();
  // note that this modifies the session for good,
  // even if user cancels the dialog
  SavePing(FSessionData);

  FTransferProtocolIndex = TransferProtocolCombo->GetItemIndex();
  FFtpEncryptionComboIndex = FtpEncryptionCombo->GetItemIndex();
  intptr_t Port = PortNumberEdit->GetAsInteger();

  LoadPing(FSessionData);
  TFSProtocol FSProtocol = GetFSProtocol();
  if (FSProtocol == fsSFTPonly || FSProtocol == fsSCPonly)
  {
    if (Port == FtpPortNumber)
    {
      PortNumberEdit->SetAsInteger(SshPortNumber);
    }
  }
  else if ((FSProtocol == fsFTP) && ((Ftps == ftpsNone) || (Ftps == ftpsExplicitSsl) || (Ftps == ftpsExplicitTls)))
  {
    if ((Port == SshPortNumber) || (Port == FtpsImplicitPortNumber) || (Port == HTTPPortNumber) || (Port == HTTPSPortNumber))
    {
      PortNumberEdit->SetAsInteger(FtpPortNumber);
    }
  }
  else if ((FSProtocol == fsFTP) && (Ftps == ftpsImplicit))
  {
    if ((Port == SshPortNumber) || (Port == FtpPortNumber) || (Port == HTTPPortNumber) || (Port == HTTPSPortNumber))
    {
      PortNumberEdit->SetAsInteger(FtpsImplicitPortNumber);
    }
  }
  else if ((FSProtocol == fsWebDAV) && (Ftps == ftpsNone))
  {
    if ((Port == FtpPortNumber) || (Port == FtpsImplicitPortNumber) || (Port == HTTPSPortNumber))
    {
      PortNumberEdit->SetAsInteger(HTTPPortNumber);
      UnicodeString HostName = HostNameEdit->GetText();
      ::AdjustRemoteDir(HostName, PortNumberEdit, RemoteDirectoryEdit);
      HostNameEdit->SetText(HostName);
    }
  }
  else if ((FSProtocol == fsWebDAV) && (Ftps != ftpsNone))
  {
    if ((Port == FtpPortNumber) || (Port == FtpsImplicitPortNumber) || (Port == HTTPPortNumber))
    {
      PortNumberEdit->SetAsInteger(HTTPSPortNumber);
      UnicodeString HostName = HostNameEdit->GetText();
      ::AdjustRemoteDir(HostName, PortNumberEdit, RemoteDirectoryEdit);
      HostNameEdit->SetText(HostName);
    }
  }
}

bool TSessionDialog::IsSshProtocol(TFSProtocol FSProtocol) const
{
  bool Result =
    (FSProtocol == fsSFTPonly) || (FSProtocol == fsSFTP) || (FSProtocol == fsSCPonly);

  return Result;
}

bool TSessionDialog::IsWebDAVProtocol(TFSProtocol FSProtocol) const
{
  return FSProtocol == fsWebDAV;
}

bool TSessionDialog::IsSshOrWebDAVProtocol(TFSProtocol FSProtocol) const
{
  return IsSshProtocol(FSProtocol) || IsWebDAVProtocol(FSProtocol);
}

void TSessionDialog::UpdateControls()
{
  TFSProtocol FSProtocol = GetFSProtocol();
  TFtps Ftps = GetFtps();
  bool InternalSshProtocol = IsSshProtocol(FSProtocol);
  bool InternalWebDAVProtocol = IsWebDAVProtocol(FSProtocol);
  bool HTTPSProtocol = (FSProtocol == fsWebDAV) && (Ftps != ftpsNone);
  bool SshProtocol = InternalSshProtocol;
  bool SftpProtocol = (FSProtocol == fsSFTPonly) || (FSProtocol == fsSFTP);
  bool ScpOnlyProtocol = (FSProtocol == fsSCPonly);
  bool FtpProtocol = (FSProtocol == fsFTP) && (Ftps == ftpsNone);
  bool FtpsProtocol = (FSProtocol == fsFTP) && (Ftps != ftpsNone);
  bool LoginAnonymous = false;

  ConnectButton->SetEnabled(!HostNameEdit->GetIsEmpty());

  // Basic tab
  AllowScpFallbackCheck->SetVisible(
    TransferProtocolCombo->GetVisible() &&
    (IndexToFSProtocol(TransferProtocolCombo->GetItemIndex(), false) == fsSFTPonly));
  InsecureLabel->SetVisible(TransferProtocolCombo->GetVisible() && !SshProtocol && !FtpsProtocol && !HTTPSProtocol);
  bool FtpEncryptionVisible = (GetTab() == FtpEncryptionCombo->GetGroup()) &&
    (FtpProtocol || FtpsProtocol || InternalWebDAVProtocol || HTTPSProtocol);
  FtpEncryptionLabel->SetVisible(FtpEncryptionVisible);
  FtpEncryptionCombo->SetVisible(FtpEncryptionVisible);
  PrivateKeyEdit->SetEnabled(SshProtocol || FtpsProtocol || HTTPSProtocol);
  HostNameLabel->SetCaption(GetMsg(LOGIN_HOST_NAME));

  UserNameEdit->SetEnabled(!LoginAnonymous);
  PasswordEdit->SetEnabled(!LoginAnonymous);

  // Connection sheet
  FtpPasvModeCheck->SetEnabled(FtpProtocol);
  if (FtpProtocol && (FtpProxyMethodCombo->GetItemIndex() != pmNone) && !FtpPasvModeCheck->GetChecked())
  {
    FtpPasvModeCheck->SetChecked(true);
    TWinSCPPlugin * WinSCPPlugin = NB_STATIC_DOWNCAST(TWinSCPPlugin, FarPlugin);
    WinSCPPlugin->MoreMessageDialog(GetMsg(FTP_PASV_MODE_REQUIRED),
      nullptr, qtInformation, qaOK);
  }
  SshBufferSizeCheck->SetEnabled(SshProtocol);
  PingNullPacketButton->SetEnabled(SshProtocol);
  IPAutoButton->SetEnabled(SshProtocol);

  // SFTP tab
  SftpTab->SetEnabled(SftpProtocol);

  // FTP tab
  FtpTab->SetEnabled(FtpProtocol || FtpsProtocol);
  FtpAllowEmptyPasswordCheck->SetEnabled(FtpProtocol || FtpsProtocol);
  SslSessionReuseCheck->SetEnabled(FtpsProtocol);

  // SSH tab
  SshTab->SetEnabled(SshProtocol);
  CipherUpButton->SetEnabled(CipherListBox->GetItems()->GetSelected() != 0);
  CipherDownButton->SetEnabled(
    CipherListBox->GetItems()->GetSelected() < CipherListBox->GetItems()->GetCount() - 1);

  // Authentication tab
  AuthenticationTab->SetEnabled(SshProtocol);
  SshNoUserAuthCheck->SetEnabled(!SshProt1onlyButton->GetChecked());
  bool Authentication = !SshNoUserAuthCheck->GetEnabled() || !SshNoUserAuthCheck->GetChecked();
  TryAgentCheck->SetEnabled(Authentication);
  AuthTISCheck->SetEnabled(Authentication && !SshProt2onlyButton->GetChecked());
  AuthKICheck->SetEnabled(Authentication && !SshProt1onlyButton->GetChecked());
  AuthKIPasswordCheck->SetEnabled(
    Authentication &&
    ((AuthTISCheck->GetEnabled() && AuthTISCheck->GetChecked()) ||
     (AuthKICheck->GetEnabled() && AuthKICheck->GetChecked())));
  AuthGSSAPICheck3->SetEnabled(
    Authentication && !SshProt1onlyButton->GetChecked());
  GSSAPIFwdTGTCheck->SetEnabled(
    Authentication && !SshProt1onlyButton->GetChecked());

  // Directories tab
  CacheDirectoryChangesCheck->SetEnabled(
    (FSProtocol != fsSCPonly) || CacheDirectoriesCheck->GetChecked());
  PreserveDirectoryChangesCheck->SetEnabled(
    CacheDirectoryChangesCheck->GetIsEnabled() && CacheDirectoryChangesCheck->GetChecked());
  ResolveSymlinksCheck->SetEnabled(!InternalWebDAVProtocol);

  // Environment tab
  DSTModeUnixCheck->SetEnabled(!FtpProtocol);
  TimeDifferenceEdit->SetEnabled((FtpProtocol || (FSProtocol == fsSCPonly)));

  // Recycle bin tab
  OverwrittenToRecycleBinCheck->SetEnabled((FSProtocol != fsSCPonly) &&
      !FtpProtocol);
  RecycleBinPathEdit->SetEnabled(
    (DeleteToRecycleBinCheck->GetIsEnabled() && DeleteToRecycleBinCheck->GetChecked()) ||
    (OverwrittenToRecycleBinCheck->GetIsEnabled() && OverwrittenToRecycleBinCheck->GetChecked()));

  // Kex tab
  KexTab->SetEnabled(SshProtocol && !SshProt1onlyButton->GetChecked() &&
    (BugRekey2Combo->GetItemIndex() != 2));
  KexUpButton->SetEnabled((KexListBox->GetItems()->GetSelected() > 0));
  KexDownButton->SetEnabled(
    (KexListBox->GetItems()->GetSelected() < KexListBox->GetItems()->GetCount() - 1));

  // Bugs tab
  BugsTab->SetEnabled(SshProtocol);

  // WebDAV tab
  WebDAVTab->SetEnabled(InternalWebDAVProtocol);

  // Scp/Shell tab
  ScpTab->SetEnabled(InternalSshProtocol);
  // disable also for SFTP with SCP fallback, as if someone wants to configure
  // these he/she probably intends to use SCP and should explicitly select it.
  // (note that these are not used for secondary shell session)
  ListingCommandEdit->SetEnabled(ScpOnlyProtocol);
  IgnoreLsWarningsCheck->SetEnabled(ScpOnlyProtocol);
  SCPLsFullTimeAutoCheck->SetEnabled(ScpOnlyProtocol);
  LookupUserGroupsCheck->SetEnabled(ScpOnlyProtocol);
  UnsetNationalVarsCheck->SetEnabled(ScpOnlyProtocol);
  ClearAliasesCheck->SetEnabled(ScpOnlyProtocol);
  Scp1CompatibilityCheck->SetEnabled(ScpOnlyProtocol);

  // Connection/Proxy tab
  TFarComboBox * ProxyMethodCombo = GetProxyMethodCombo();
  TProxyMethod ProxyMethod = IndexToProxyMethod(ProxyMethodCombo->GetItemIndex(), ProxyMethodCombo->GetItems());
  ProxyMethodCombo->SetVisible(GetTab() == ProxyMethodCombo->GetGroup());
  TFarComboBox * OtherProxyMethodCombo = GetOtherProxyMethodCombo();
  OtherProxyMethodCombo->SetVisible(false);

  bool Proxy = (ProxyMethod != pmNone);
  UnicodeString ProxyCommand =
    (ProxyMethod == pmCmd) ?
     ProxyLocalCommandEdit->GetText() : ProxyTelnetCommandEdit->GetText();
  ProxyHostEdit->SetEnabled(Proxy && (ProxyMethod != pmSystem) &&
    ((ProxyMethod != pmCmd) ||
     ::AnsiContainsText(ProxyCommand, L"%proxyhost")));
  ProxyPortEdit->SetEnabled(Proxy && (ProxyMethod != pmSystem) &&
    ((ProxyMethod != pmCmd) ||
     ::AnsiContainsText(ProxyCommand, L"%proxyport")));
  ProxyUsernameEdit->SetEnabled(Proxy &&
    // FZAPI does not support username for SOCKS4
    (((ProxyMethod == pmSocks4) && SshProtocol) ||
     (ProxyMethod == pmSocks5) ||
     (ProxyMethod == pmHTTP) ||
     (((ProxyMethod == pmTelnet) ||
       (ProxyMethod == pmCmd)) &&
      ::AnsiContainsText(ProxyCommand, L"%user")) ||
     (ProxyMethod == pmSystem)));
  ProxyPasswordEdit->SetEnabled(Proxy &&
    ((ProxyMethod == pmSocks5) ||
     (ProxyMethod == pmHTTP) ||
     (((ProxyMethod == pmTelnet) ||
       (ProxyMethod == pmCmd)) &&
      ::AnsiContainsText(ProxyCommand, L"%pass")) ||
     (ProxyMethod == pmSystem)));
  bool ProxySettings = Proxy && SshProtocol;
  ProxyTelnetCommandEdit->SetEnabled(ProxySettings && (ProxyMethod == pmTelnet));
  ProxyLocalCommandEdit->SetVisible((GetTab() == ProxyMethodCombo->GetGroup()) && (ProxyMethod == pmCmd));
  ProxyLocalCommandLabel->SetVisible(ProxyLocalCommandEdit->GetVisible());
  ProxyTelnetCommandEdit->SetVisible((GetTab() == ProxyMethodCombo->GetGroup()) && (ProxyMethod != pmCmd));
  ProxyTelnetCommandLabel->SetVisible(ProxyTelnetCommandEdit->GetVisible());
  ProxyLocalhostCheck->SetEnabled(ProxySettings);
  ProxyDNSOffButton->SetEnabled(ProxySettings);

  // Tunnel tab
  TunnelTab->SetEnabled(InternalSshProtocol);
}

bool TSessionDialog::Execute(TSessionData * SessionData, TSessionActionEnum & Action)
{
  int Captions[] =
  {
    LOGIN_ADD,
    LOGIN_EDIT,
    LOGIN_CONNECT
  };
  SetCaption(GetMsg(Captions[Action]));

  FSessionData = SessionData;
  FTransferProtocolIndex = TransferProtocolCombo->GetItemIndex();

  FFtpEncryptionComboIndex = FtpEncryptionCombo->GetItemIndex();

  HideTabs();
  SelectTab(tabSession);

  // load session data

  // Basic tab
  HostNameEdit->SetText(SessionData->GetHostName());
  PortNumberEdit->SetAsInteger(SessionData->GetPortNumber());

  UserNameEdit->SetText(SessionData->SessionGetUserName());
  PasswordEdit->SetText(SessionData->GetPassword());
  PrivateKeyEdit->SetText(SessionData->GetPublicKeyFile());

  bool AllowScpFallback;
  TransferProtocolCombo->SetItemIndex(
    static_cast<intptr_t>(FSProtocolToIndex(SessionData->GetFSProtocol(), AllowScpFallback)));
  AllowScpFallbackCheck->SetChecked(AllowScpFallback);

  // Directories tab
  RemoteDirectoryEdit->SetText(SessionData->GetRemoteDirectory());
  UpdateDirectoriesCheck->SetChecked(SessionData->GetUpdateDirectories());
  CacheDirectoriesCheck->SetChecked(SessionData->GetCacheDirectories());
  CacheDirectoryChangesCheck->SetChecked(SessionData->GetCacheDirectoryChanges());
  PreserveDirectoryChangesCheck->SetChecked(SessionData->GetPreserveDirectoryChanges());
  ResolveSymlinksCheck->SetChecked(SessionData->GetResolveSymlinks());

  // Environment tab
  if (SessionData->GetEOLType() == eolLF)
  {
    EOLTypeCombo->SetItemIndex(0);
  }
  else
  {
    EOLTypeCombo->SetItemIndex(1);
  }

  switch (SessionData->GetDSTMode())
  {
    case dstmWin:
      DSTModeWinCheck->SetChecked(true);
      break;

    case dstmKeep:
      DSTModeKeepCheck->SetChecked(true);
      break;

    default:
    case dstmUnix:
      DSTModeUnixCheck->SetChecked(true);
      break;
  }

  DeleteToRecycleBinCheck->SetChecked(SessionData->GetDeleteToRecycleBin());
  OverwrittenToRecycleBinCheck->SetChecked(SessionData->GetOverwrittenToRecycleBin());
  RecycleBinPathEdit->SetText(SessionData->GetRecycleBinPath());

  // Shell tab
  if (SessionData->GetDefaultShell())
  {
    ShellEdit->SetText(ShellEdit->GetItems()->GetString(0));
  }
  else
  {
    ShellEdit->SetText(SessionData->GetShell());
  }
  if (SessionData->GetDetectReturnVar())
  {
    ReturnVarEdit->SetText(ReturnVarEdit->GetItems()->GetString(0));
  }
  else
  {
    ReturnVarEdit->SetText(SessionData->GetReturnVar());
  }
  LookupUserGroupsCheck->SetChecked(SessionData->GetLookupUserGroups() != asOff);
  ClearAliasesCheck->SetChecked(SessionData->GetClearAliases());
  IgnoreLsWarningsCheck->SetChecked(SessionData->GetIgnoreLsWarnings());
  Scp1CompatibilityCheck->SetChecked(SessionData->GetScp1Compatibility());
  UnsetNationalVarsCheck->SetChecked(SessionData->GetUnsetNationalVars());
  ListingCommandEdit->SetText(SessionData->GetListingCommand());
  SCPLsFullTimeAutoCheck->SetChecked((SessionData->GetSCPLsFullTime() != asOff));
  intptr_t TimeDifferenceMin = ::DateTimeToTimeStamp(SessionData->GetTimeDifference()).Time / 60000;
  if (SessionData->GetTimeDifference().GetValue() < 0)
  {
    TimeDifferenceMin = -TimeDifferenceMin;
  }
  TimeDifferenceEdit->SetAsInteger(TimeDifferenceMin / 60);
  TimeDifferenceMinutesEdit->SetAsInteger(TimeDifferenceMin % 60);

  // SFTP tab

#define TRISTATE(COMBO, PROP, MSG) \
    COMBO->SetItemIndex(static_cast<intptr_t>(2 - SessionData->Get ## PROP))
  SFTP_BUGS();

  if (SessionData->GetSftpServer().IsEmpty())
  {
    SftpServerEdit->SetText(SftpServerEdit->GetItems()->GetString(0));
  }
  else
  {
    SftpServerEdit->SetText(SessionData->GetSftpServer());
  }
  SFTPMaxVersionCombo->SetItemIndex(static_cast<intptr_t>(SessionData->GetSFTPMaxVersion()));
  SFTPMinPacketSizeEdit->SetAsInteger(SessionData->GetSFTPMinPacketSize());
  SFTPMaxPacketSizeEdit->SetAsInteger(SessionData->GetSFTPMaxPacketSize());

  // FTP tab
  FtpUseMlsdCombo->SetItemIndex(static_cast<intptr_t>(2 - SessionData->GetFtpUseMlsd()));
  FtpAllowEmptyPasswordCheck->SetChecked(SessionData->GetFtpAllowEmptyPassword());
  std::unique_ptr<TStrings> PostLoginCommands(new TStringList());
  PostLoginCommands->SetText(SessionData->GetPostLoginCommands());
  for (intptr_t Index = 0; (Index < PostLoginCommands->GetCount()) &&
       (Index < static_cast<intptr_t>(_countof(PostLoginCommandsEdits))); ++Index)
  {
    PostLoginCommandsEdits[Index]->SetText(PostLoginCommands->GetString(Index));
  }

  FtpDupFFCheck->SetChecked(SessionData->GetFtpDupFF());
  FtpUndupFFCheck->SetChecked(SessionData->GetFtpUndupFF());
  SslSessionReuseCheck->SetChecked(SessionData->GetSslSessionReuse());

  TFtps Ftps = SessionData->GetFtps();
  switch (Ftps)
  {
    case ftpsNone:
      FtpEncryptionCombo->SetItemIndex(0);
      break;

    case ftpsImplicit:
      FtpEncryptionCombo->SetItemIndex(1);
      break;

    case ftpsExplicitSsl:
      FtpEncryptionCombo->SetItemIndex(2);
      break;

    case ftpsExplicitTls:
      FtpEncryptionCombo->SetItemIndex(3);
      break;

    default:
      FtpEncryptionCombo->SetItemIndex(0);
      break;
  }

  // Connection tab
  FtpPasvModeCheck->SetChecked(SessionData->GetFtpPasvMode());
  SshBufferSizeCheck->SetChecked((FSessionData->GetSendBuf() > 0) && FSessionData->GetSshSimple());
  LoadPing(SessionData);
  TimeoutEdit->SetAsInteger(SessionData->GetTimeout());

  switch (SessionData->GetAddressFamily())
  {
    case afIPv4:
      IPv4Button->SetChecked(true);
      break;

    case afIPv6:
      IPv6Button->SetChecked(true);
      break;

    case afAuto:
    default:
      IPAutoButton->SetChecked(true);
      break;
  }

  if (SessionData->GetCodePage().IsEmpty())
  {
    CodePageEdit->SetText(CodePageEdit->GetItems()->GetString(0));
  }
  else
  {
    CodePageEdit->SetText(SessionData->GetCodePage());
  }

  // Proxy tab
  TFarComboBox * ProxyMethodCombo = GetProxyMethodCombo();
  intptr_t Index = ProxyMethodToIndex(SessionData->GetProxyMethod(), ProxyMethodCombo->GetItems());
  ProxyMethodCombo->SetItemIndex(Index);
  if (SessionData->GetProxyMethod() != pmSystem)
  {
    ProxyHostEdit->SetText(SessionData->GetProxyHost());
    ProxyPortEdit->SetAsInteger(SessionData->GetProxyPort());
  }
  ProxyUsernameEdit->SetText(SessionData->GetProxyUsername());
  ProxyPasswordEdit->SetText(SessionData->GetProxyPassword());
  ProxyTelnetCommandEdit->SetText(SessionData->GetProxyTelnetCommand());
  ProxyLocalCommandEdit->SetText(SessionData->GetProxyLocalCommand());
  ProxyLocalhostCheck->SetChecked(SessionData->GetProxyLocalhost());
  switch (SessionData->GetProxyDNS())
  {
    case asOn:
      ProxyDNSOnButton->SetChecked(true);
      break;
    case asOff:
      ProxyDNSOffButton->SetChecked(true);
      break;
    default:
      ProxyDNSAutoButton->SetChecked(true);
      break;
  }

  // Tunnel tab
  TunnelCheck->SetChecked(SessionData->GetTunnel());
  TunnelUserNameEdit->SetText(SessionData->GetTunnelUserName());
  TunnelPortNumberEdit->SetAsInteger(SessionData->GetTunnelPortNumber());
  TunnelHostNameEdit->SetText(SessionData->GetTunnelHostName());
  TunnelPasswordEdit->SetText(SessionData->GetTunnelPassword());
  TunnelPrivateKeyEdit->SetText(SessionData->GetTunnelPublicKeyFile());
  if (SessionData->GetTunnelAutoassignLocalPortNumber())
  {
    TunnelLocalPortNumberEdit->SetText(TunnelLocalPortNumberEdit->GetItems()->GetString(0));
  }
  else
  {
    TunnelLocalPortNumberEdit->SetText(::IntToStr(SessionData->GetTunnelLocalPortNumber()));
  }

  // SSH tab
  CompressionCheck->SetChecked(SessionData->GetCompression());
  if (Ssh2DESCheck != nullptr)
  {
    Ssh2DESCheck->SetChecked(SessionData->GetSsh2DES());
  }

  switch (SessionData->GetSshProt())
  {
    case ssh1only:
      SshProt1onlyButton->SetChecked(true);
      break;
    case ssh1deprecated:
      SshProt1Button->SetChecked(true);
      break;
    case ssh2deprecated:
      SshProt2Button->SetChecked(true);
      break;
    case ssh2only:
      SshProt2onlyButton->SetChecked(true);
      break;
  }

  CipherListBox->GetItems()->BeginUpdate();
  {
    SCOPE_EXIT
    {
      CipherListBox->GetItems()->EndUpdate();
    };
    CipherListBox->GetItems()->Clear();
    DebugAssert(CIPHER_NAME_WARN + CIPHER_COUNT - 1 == CIPHER_NAME_CHACHA20);
    for (intptr_t Index2 = 0; Index2 < CIPHER_COUNT; ++Index2)
    {
      TObject * Obj = static_cast<TObject *>(ToPtr(SessionData->GetCipher(Index2)));
      CipherListBox->GetItems()->AddObject(
        GetMsg(CIPHER_NAME_WARN + static_cast<intptr_t>(SessionData->GetCipher(Index2))),
        Obj);
    }
  }

  // KEX tab

  RekeyTimeEdit->SetAsInteger(SessionData->GetRekeyTime());
  RekeyDataEdit->SetText(SessionData->GetRekeyData());

  KexListBox->GetItems()->BeginUpdate();
  {
    SCOPE_EXIT
    {
      KexListBox->GetItems()->EndUpdate();
    };
    KexListBox->GetItems()->Clear();
    DebugAssert(KEX_NAME_WARN + KEX_COUNT - 1 == KEX_NAME_ECDH);
    for (intptr_t Index3 = 0; Index3 < KEX_COUNT; ++Index3)
    {
      KexListBox->GetItems()->AddObject(
        GetMsg(KEX_NAME_WARN + static_cast<intptr_t>(SessionData->GetKex(Index3))),
        static_cast<TObject *>(ToPtr(SessionData->GetKex(Index3))));
    }
  }

  // Authentication tab
  SshNoUserAuthCheck->SetChecked(SessionData->GetSshNoUserAuth());
  TryAgentCheck->SetChecked(SessionData->GetTryAgent());
  AuthTISCheck->SetChecked(SessionData->GetAuthTIS());
  AuthKICheck->SetChecked(SessionData->GetAuthKI());
  AuthKIPasswordCheck->SetChecked(SessionData->GetAuthKIPassword());
  AgentFwdCheck->SetChecked(SessionData->GetAgentFwd());
  AuthGSSAPICheck3->SetChecked(SessionData->GetAuthGSSAPI());
  GSSAPIFwdTGTCheck->SetChecked(SessionData->GetGSSAPIFwdTGT());

  // Bugs tab

  BUGS();

  // WebDAV tab
  WebDAVCompressionCheck->SetChecked(SessionData->GetCompression());

#undef TRISTATE

  intptr_t Button = ShowModal();
  bool Result = (Button == brOK || Button == brConnect);
  if (Result)
  {
    if (Button == brConnect)
    {
      Action = saConnect;
    }
    else if (Action == saConnect)
    {
      Action = saEdit;
    }

    UnicodeString HostName = HostNameEdit->GetText();
    UnicodeString UserName = UserNameEdit->GetText();
    UnicodeString Password = PasswordEdit->GetText();
    SessionData->RemoveProtocolPrefix(HostName);
    // parse username, password and directory, if any
    {
      intptr_t Pos = HostName.RPos(L'@');
      if (Pos > 0)
      {
        UnicodeString UserNameAndPassword = HostName.SubString(1, Pos - 1);
        Pos = UserNameAndPassword.RPos(L':');
        if (Pos > 0)
        {
          UserName = UserNameAndPassword.SubString(1, Pos - 1);
          Password = UserNameAndPassword.SubString(Pos + 1, - 1);
        }
        else
        {
          UserName = UserNameAndPassword;
        }
      }
    }
    // if (GetFSProtocol() == fsWebDAV)
    {
      ::AdjustRemoteDir(HostName, PortNumberEdit, RemoteDirectoryEdit);
    }

    // save session data

    // Basic tab
    SessionData->SetFSProtocol(GetFSProtocol());

    SessionData->SetHostName(HostName);
    SessionData->SetPortNumber(PortNumberEdit->GetAsInteger());
    SessionData->SetUserName(UserName);
    SessionData->SetPassword(Password);
    SessionData->SetLoginType(ltNormal);
    SessionData->SetPublicKeyFile(PrivateKeyEdit->GetText());

    // Directories tab
    SessionData->SetRemoteDirectory(RemoteDirectoryEdit->GetText());
    SessionData->SetUpdateDirectories(UpdateDirectoriesCheck->GetChecked());
    SessionData->SetCacheDirectories(CacheDirectoriesCheck->GetChecked());
    SessionData->SetCacheDirectoryChanges(CacheDirectoryChangesCheck->GetChecked());
    SessionData->SetPreserveDirectoryChanges(PreserveDirectoryChangesCheck->GetChecked());
    SessionData->SetResolveSymlinks(ResolveSymlinksCheck->GetChecked());

    // Environment tab
    if (DSTModeUnixCheck->GetChecked())
    {
      SessionData->SetDSTMode(dstmUnix);
    }
    else if (DSTModeKeepCheck->GetChecked())
    {
      SessionData->SetDSTMode(dstmKeep);
    }
    else
    {
      SessionData->SetDSTMode(dstmWin);
    }
    if (EOLTypeCombo->GetItemIndex() == 0)
    {
      SessionData->SetEOLType(eolLF);
    }
    else
    {
      SessionData->SetEOLType(eolCRLF);
    }

    SessionData->SetDeleteToRecycleBin(DeleteToRecycleBinCheck->GetChecked());
    SessionData->SetOverwrittenToRecycleBin(OverwrittenToRecycleBinCheck->GetChecked());
    SessionData->SetRecycleBinPath(RecycleBinPathEdit->GetText());

    // SCP tab
    SessionData->SetDefaultShell(ShellEdit->GetText() == ShellEdit->GetItems()->GetString(0));
    SessionData->SetShell((SessionData->GetDefaultShell() ? UnicodeString() : ShellEdit->GetText()));
    SessionData->SetDetectReturnVar(ReturnVarEdit->GetText() == ReturnVarEdit->GetItems()->GetString(0));
    SessionData->SetReturnVar((SessionData->GetDetectReturnVar() ? UnicodeString() : ReturnVarEdit->GetText()));
    SessionData->SetLookupUserGroups(LookupUserGroupsCheck->GetChecked() ? asOn : asOff);
    SessionData->SetClearAliases(ClearAliasesCheck->GetChecked());
    SessionData->SetIgnoreLsWarnings(IgnoreLsWarningsCheck->GetChecked());
    SessionData->SetScp1Compatibility(Scp1CompatibilityCheck->GetChecked());
    SessionData->SetUnsetNationalVars(UnsetNationalVarsCheck->GetChecked());
    SessionData->SetListingCommand(ListingCommandEdit->GetText());
    SessionData->SetSCPLsFullTime(SCPLsFullTimeAutoCheck->GetChecked() ? asAuto : asOff);
    SessionData->SetTimeDifference(TDateTime(
      (ToDouble(TimeDifferenceEdit->GetAsInteger()) / 24) +
      (ToDouble(TimeDifferenceMinutesEdit->GetAsInteger()) / 24 / 60)));

    // SFTP tab

#define TRISTATE(COMBO, PROP, MSG) \
      SessionData->Set##PROP(sb##PROP, static_cast<TAutoSwitch>(2 - COMBO->GetItemIndex()));
    // SFTP_BUGS();
    SessionData->SetSFTPBug(sbSymlink, static_cast<TAutoSwitch>(2 - SFTPBugSymlinkCombo->GetItemIndex()));
    SessionData->SetSFTPBug(sbSignedTS, static_cast<TAutoSwitch>(2 - SFTPBugSignedTSCombo->GetItemIndex()));

    SessionData->SetSftpServer(
      (SftpServerEdit->GetText() == SftpServerEdit->GetItems()->GetString(0)) ?
      UnicodeString() : SftpServerEdit->GetText());
    SessionData->SetSFTPMaxVersion(SFTPMaxVersionCombo->GetItemIndex());
    SessionData->SetSFTPMinPacketSize(SFTPMinPacketSizeEdit->GetAsInteger());
    SessionData->SetSFTPMaxPacketSize(SFTPMaxPacketSizeEdit->GetAsInteger());

    // FTP tab
    SessionData->SetFtpUseMlsd(static_cast<TAutoSwitch>(2 - FtpUseMlsdCombo->GetItemIndex()));
    SessionData->SetFtpAllowEmptyPassword(FtpAllowEmptyPasswordCheck->GetChecked());
    SessionData->SetFtpDupFF(FtpDupFFCheck->GetChecked());
    SessionData->SetFtpUndupFF(FtpUndupFFCheck->GetChecked());
    SessionData->SetSslSessionReuse(SslSessionReuseCheck->GetChecked());
    TODO("TlsCertificateFileEdit->GetText()");
    SessionData->SetTlsCertificateFile(PrivateKeyEdit->GetText());
    std::unique_ptr<TStrings> PostLoginCommands2(new TStringList());
    for (intptr_t Index4 = 0; Index4 < static_cast<intptr_t>(_countof(PostLoginCommandsEdits)); ++Index4)
    {
      UnicodeString Text = PostLoginCommandsEdits[Index4]->GetText();
      if (!Text.IsEmpty())
      {
        PostLoginCommands2->Add(PostLoginCommandsEdits[Index4]->GetText());
      }
    }

    SessionData->SetPostLoginCommands(PostLoginCommands2->GetText());
    if ((GetFSProtocol() == fsFTP) && (GetFtps() != ftpsNone))
    {
      SessionData->SetFtps(GetFtps());
    }
    else
    {
      SessionData->SetFtps(ftpsNone);
    }

    switch (FtpEncryptionCombo->GetItemIndex())
    {
      case 0:
        SessionData->SetFtps(ftpsNone);
        break;
      case 1:
        SessionData->SetFtps(ftpsImplicit);
        break;
      case 2:
        SessionData->SetFtps(ftpsExplicitSsl);
        break;
      case 3:
        SessionData->SetFtps(ftpsExplicitTls);
        break;
      default:
        SessionData->SetFtps(ftpsNone);
        break;
    }

    // Connection tab
    SessionData->SetFtpPasvMode(FtpPasvModeCheck->GetChecked());
    SessionData->SetSendBuf(SshBufferSizeCheck->GetChecked() ? DefaultSendBuf : 0);
    SessionData->SetSshSimple(SshBufferSizeCheck->GetChecked());
    if (PingOffButton->GetChecked())
    {
      SessionData->SetPingType(ptOff);
    }
    else if (PingNullPacketButton->GetChecked())
    {
      SessionData->SetPingType(ptNullPacket);
    }
    else if (PingDummyCommandButton->GetChecked())
    {
      SessionData->SetPingType(ptDummyCommand);
    }
    else
    {
      SessionData->SetPingType(ptOff);
    }
    if (GetFSProtocol() == fsFTP)
    {
      if (PingOffButton->GetChecked())
      {
        SessionData->SetFtpPingType(ptOff);
      }
      else if (PingNullPacketButton->GetChecked())
      {
        SessionData->SetFtpPingType(ptNullPacket);
      }
      else if (PingDummyCommandButton->GetChecked())
      {
        SessionData->SetFtpPingType(ptDummyCommand);
      }
      else
      {
        SessionData->SetFtpPingType(ptOff);
      }
      SessionData->SetFtpPingInterval(PingIntervalSecEdit->GetAsInteger());
    }
    else
    {
      SessionData->SetPingInterval(PingIntervalSecEdit->GetAsInteger());
    }
    SessionData->SetTimeout(TimeoutEdit->GetAsInteger());

    if (IPv4Button->GetChecked())
    {
      SessionData->SetAddressFamily(afIPv4);
    }
    else if (IPv6Button->GetChecked())
    {
      SessionData->SetAddressFamily(afIPv6);
    }
    else
    {
      SessionData->SetAddressFamily(afAuto);
    }
    SessionData->SetCodePage(
      (CodePageEdit->GetText() == CodePageEdit->GetItems()->GetString(0)) ?
      UnicodeString() : CodePageEdit->GetText());

    // Proxy tab
    SessionData->SetProxyMethod(GetProxyMethod());
    SessionData->SetFtpProxyLogonType(GetFtpProxyLogonType());
    SessionData->SetProxyHost(ProxyHostEdit->GetText());
    SessionData->SetProxyPort(ProxyPortEdit->GetAsInteger());
    SessionData->SetProxyUsername(ProxyUsernameEdit->GetText());
    SessionData->SetProxyPassword(ProxyPasswordEdit->GetText());
    SessionData->SetProxyTelnetCommand(ProxyTelnetCommandEdit->GetText());
    SessionData->SetProxyLocalCommand(ProxyLocalCommandEdit->GetText());
    SessionData->SetProxyLocalhost(ProxyLocalhostCheck->GetChecked());

    if (ProxyDNSOnButton->GetChecked())
    {
      SessionData->SetProxyDNS(asOn);
    }
    else if (ProxyDNSOffButton->GetChecked())
    {
      SessionData->SetProxyDNS(asOff);
    }
    else
    {
      SessionData->SetProxyDNS(asAuto);
    }

    // Tunnel tab
    SessionData->SetTunnel(TunnelCheck->GetChecked());
    SessionData->SetTunnelUserName(TunnelUserNameEdit->GetText());
    SessionData->SetTunnelPortNumber(TunnelPortNumberEdit->GetAsInteger());
    SessionData->SetTunnelHostName(TunnelHostNameEdit->GetText());
    SessionData->SetTunnelPassword(TunnelPasswordEdit->GetText());
    SessionData->SetTunnelPublicKeyFile(TunnelPrivateKeyEdit->GetText());
    if (TunnelLocalPortNumberEdit->GetText() == TunnelLocalPortNumberEdit->GetItems()->GetString(0))
    {
      SessionData->SetTunnelLocalPortNumber(0);
    }
    else
    {
      SessionData->SetTunnelLocalPortNumber(::StrToIntDef(TunnelLocalPortNumberEdit->GetText(), 0));
    }

    // SSH tab
    SessionData->SetCompression(CompressionCheck->GetChecked());
    if (Ssh2DESCheck != nullptr)
    {
      SessionData->SetSsh2DES(Ssh2DESCheck->GetChecked());
    }

    if (SshProt1onlyButton->GetChecked())
    {
      SessionData->SetSshProt(ssh1only);
    }
    else if (SshProt1Button->GetChecked())
    {
      SessionData->SetSshProt(ssh1deprecated);
    }
    else if (SshProt2Button->GetChecked())
    {
      SessionData->SetSshProt(ssh2deprecated);
    }
    else
    {
      SessionData->SetSshProt(ssh2only);
    }

    for (intptr_t Index5 = 0; Index5 < CIPHER_COUNT; ++Index5)
    {
      TObject * Obj = static_cast<TObject *>(CipherListBox->GetItems()->GetObj(Index5));
      SessionData->SetCipher(Index5, static_cast<TCipher>(reinterpret_cast<intptr_t>(Obj)));
    }

    // KEX tab

    SessionData->SetRekeyTime(RekeyTimeEdit->GetAsInteger());
    SessionData->SetRekeyData(RekeyDataEdit->GetText());

    for (intptr_t Index6 = 0; Index6 < KEX_COUNT; ++Index6)
    {
      SessionData->SetKex(Index6, static_cast<TKex>(reinterpret_cast<intptr_t>(KexListBox->GetItems()->GetObj(Index))));
    }

    // Authentication tab
    SessionData->SetSshNoUserAuth(SshNoUserAuthCheck->GetChecked());
    SessionData->SetTryAgent(TryAgentCheck->GetChecked());
    SessionData->SetAuthTIS(AuthTISCheck->GetChecked());
    SessionData->SetAuthKI(AuthKICheck->GetChecked());
    SessionData->SetAuthKIPassword(AuthKIPasswordCheck->GetChecked());
    SessionData->SetAgentFwd(AgentFwdCheck->GetChecked());
    SessionData->SetAuthGSSAPI(AuthGSSAPICheck3->GetChecked());
    SessionData->SetGSSAPIFwdTGT(GSSAPIFwdTGTCheck->GetChecked());

    // Bugs tab
    // BUGS();

    // WebDAV tab
    if (GetFSProtocol() == fsWebDAV)
      SessionData->SetCompression(WebDAVCompressionCheck->GetChecked());

#undef TRISTATE
    SessionData->SetBug(sbIgnore1, static_cast<TAutoSwitch>(2 - BugIgnore1Combo->GetItemIndex()));
    SessionData->SetBug(sbPlainPW1, static_cast<TAutoSwitch>(2 - BugPlainPW1Combo->GetItemIndex()));
    SessionData->SetBug(sbRSA1, static_cast<TAutoSwitch>(2 - BugRSA1Combo->GetItemIndex()));
    SessionData->SetBug(sbHMAC2, static_cast<TAutoSwitch>(2 - BugHMAC2Combo->GetItemIndex()));
    SessionData->SetBug(sbDeriveKey2, static_cast<TAutoSwitch>(2 - BugDeriveKey2Combo->GetItemIndex()));
    SessionData->SetBug(sbRSAPad2, static_cast<TAutoSwitch>(2 - BugRSAPad2Combo->GetItemIndex()));
    SessionData->SetBug(sbPKSessID2, static_cast<TAutoSwitch>(2 - BugPKSessID2Combo->GetItemIndex()));
    SessionData->SetBug(sbRekey2, static_cast<TAutoSwitch>(2 - BugRekey2Combo->GetItemIndex()));
  }

  return Result;
}

void TSessionDialog::LoadPing(TSessionData * SessionData)
{
  TFSProtocol FSProtocol = IndexToFSProtocol(FTransferProtocolIndex,
    AllowScpFallbackCheck->GetChecked());

  switch ((FSProtocol == fsFTP) ? SessionData->GetFtpPingType() : SessionData->GetPingType())
  {
    case ptOff:
      PingOffButton->SetChecked(true);
      break;
    case ptNullPacket:
      PingNullPacketButton->SetChecked(true);
      break;

    case ptDummyCommand:
      PingDummyCommandButton->SetChecked(true);
      break;

    default:
      PingOffButton->SetChecked(true);
      break;
  }
  PingIntervalSecEdit->SetAsInteger(
    (GetFSProtocol() == fsFTP) ?
     SessionData->GetFtpPingInterval() : SessionData->GetPingInterval());
}

void TSessionDialog::SavePing(TSessionData * SessionData)
{
  TPingType PingType;
  if (PingOffButton->GetChecked())
  {
    PingType = ptOff;
  }
  else if (PingNullPacketButton->GetChecked())
  {
    PingType = ptNullPacket;
  }
  else if (PingDummyCommandButton->GetChecked())
  {
    PingType = ptDummyCommand;
  }
  else
  {
    PingType = ptOff;
  }
  TFSProtocol FSProtocol = IndexToFSProtocol(FTransferProtocolIndex,
    AllowScpFallbackCheck->GetChecked());
  if (FSProtocol == fsFTP)
  {
    SessionData->SetFtpPingType(PingType);
    SessionData->SetFtpPingInterval(PingIntervalSecEdit->GetAsInteger());
  }
  else
  {
    SessionData->SetPingType(PingType);
    SessionData->SetPingInterval(PingIntervalSecEdit->GetAsInteger());
  }
}

intptr_t TSessionDialog::FSProtocolToIndex(TFSProtocol FSProtocol,
  bool & AllowScpFallback) const
{
  if (FSProtocol == fsSFTP)
  {
    AllowScpFallback = true;
    bool Dummy;
    return FSProtocolToIndex(fsSFTPonly, Dummy);
  }
  else
  {
    AllowScpFallback = false;
    for (intptr_t Index = 0; Index < TransferProtocolCombo->GetItems()->GetCount(); ++Index)
    {
      if (FSOrder[Index] == FSProtocol)
      {
        return Index;
      }
    }
    // SFTP is always present
    return FSProtocolToIndex(fsSFTP, AllowScpFallback);
  }
}

intptr_t TSessionDialog::ProxyMethodToIndex(TProxyMethod ProxyMethod, TFarList * Items) const
{
  for (intptr_t Index = 0; Index < Items->GetCount(); ++Index)
  {
    TObject * Obj = static_cast<TObject *>(Items->GetObj(Index));
    TProxyMethod Method = static_cast<TProxyMethod>(reinterpret_cast<intptr_t>(Obj));
    if (Method == ProxyMethod)
      return Index;
  }
  return -1;
}

TProxyMethod TSessionDialog::IndexToProxyMethod(intptr_t Index, TFarList * Items) const
{
  TProxyMethod Result = pmNone;
  if (Index >= 0 && Index < Items->GetCount())
  {
    TObject * Obj = static_cast<TObject *>(Items->GetObj(Index));
    Result = static_cast<TProxyMethod>(reinterpret_cast<intptr_t>(Obj));
  }
  return Result;
}

TFarComboBox * TSessionDialog::GetProxyMethodCombo() const
{
  return IsSshOrWebDAVProtocol(GetFSProtocol()) ? SshProxyMethodCombo : FtpProxyMethodCombo;
}

TFarComboBox * TSessionDialog::GetOtherProxyMethodCombo() const
{
  return IsSshOrWebDAVProtocol(GetFSProtocol()) ? FtpProxyMethodCombo : SshProxyMethodCombo;
}

TFSProtocol TSessionDialog::GetFSProtocol() const
{
  return IndexToFSProtocol(TransferProtocolCombo->GetItemIndex(),
    AllowScpFallbackCheck->GetChecked());
}

inline intptr_t TSessionDialog::GetLastSupportedFtpProxyMethod() const
{
  return pmSystem;
}

bool TSessionDialog::GetSupportedFtpProxyMethod(intptr_t Method) const
{
  return (Method >= 0) && (Method <= GetLastSupportedFtpProxyMethod());
}

TProxyMethod TSessionDialog::GetProxyMethod() const
{
  TFarComboBox * ProxyMethodCombo = GetProxyMethodCombo();
  TProxyMethod Result = IndexToProxyMethod(ProxyMethodCombo->GetItemIndex(), ProxyMethodCombo->GetItems());
  return Result;
}

intptr_t TSessionDialog::GetFtpProxyLogonType() const
{
  intptr_t Result = GetProxyMethod();
  if (Result > GetLastSupportedFtpProxyMethod())
    Result -= GetLastSupportedFtpProxyMethod();
  else
    Result = 0;
  return Result;
}

TFtps TSessionDialog::IndexToFtps(intptr_t Index) const
{
  bool InBounds = (Index != NPOS) && (Index < FtpEncryptionCombo->GetItems()->GetCount());
  DebugAssert(InBounds);
  TFtps Result = ftpsNone;
  if (InBounds)
  {
    switch (Index)
    {
      case 0:
        Result = ftpsNone;
        break;

      case 1:
        Result = ftpsImplicit;
        break;

      case 2:
        Result = ftpsExplicitSsl;
        break;

      case 3:
        Result = ftpsExplicitTls;
        break;

      default:
        break;
    }
  }
  return Result;
}

TFtps TSessionDialog::GetFtps() const
{
  return static_cast<TFtps>(IndexToFtps(FtpEncryptionCombo->GetItemIndex()));
}

TFSProtocol TSessionDialog::IndexToFSProtocol(intptr_t Index, bool AllowScpFallback) const
{
  bool InBounds = (Index >= 0) && (Index < static_cast<intptr_t>(_countof(FSOrder)));
  DebugAssert(InBounds || (Index == -1));
  TFSProtocol Result = fsSFTP;
  if (InBounds)
  {
    Result = FSOrder[Index];
    if ((Result == fsSFTPonly) && AllowScpFallback)
    {
      Result = fsSFTP;
    }
  }
  return Result;
}

TLoginType TSessionDialog::IndexToLoginType(intptr_t Index) const
{
  bool InBounds = (Index != NPOS) && (Index <= ltNormal);
  DebugAssert(InBounds);
  TLoginType Result = ltAnonymous;
  if (InBounds)
  {
    Result = static_cast<TLoginType>(Index);
  }
  return Result;
}

bool TSessionDialog::VerifyKey(const UnicodeString & AFileName, bool TypeOnly)
{
  bool Result = true;

//  Result = ::VerifyKey(AFileName, TypeOnly);
  Result = ::VerifyAndConvertKey(AFileName, TypeOnly);
#if 0
  if (!::Trim(AFileName).IsEmpty())
  {
    TKeyType KeyType = GetKeyType(AFileName);
    UnicodeString Message;
    switch (KeyType)
    {
      case ktOpenSSHAuto:
        Message = FMTLOAD(KEY_TYPE_UNSUPPORTED2, AFileName.c_str(), L"OpenSSH SSH-2");
        break;

      case ktOpenSSHPEM:
      case ktOpenSSHNew:
      case ktSSHCom:
        Message = FMTLOAD(KEY_TYPE_UNSUPPORTED2, AFileName.c_str(), L"ssh.com SSH-2");
        break;

      case ktSSH1Public:
      case ktSSH2PublicRFC4716:
      case ktSSH2PublicOpenSSH:
        // noop
        // Do not even bother checking SSH protocol version
        break;

      case ktSSH1:
      case ktSSH2:
        if (!TypeOnly)
        {
          if ((KeyType == ktSSH1) !=
              (SshProt1onlyButton->GetChecked() || SshProt1Button->GetChecked()))
          {
            Message = FMTLOAD(KEY_TYPE_DIFFERENT_SSH,
              AFileName.c_str(), (KeyType == ktSSH1 ? L"SSH-1" : L"PuTTY SSH-2"));
          }
        }
        break;

      default:
        DebugAssert(false);
        // fallthru
      case ktUnopenable:
      case ktUnknown:
        Message = FMTLOAD(KEY_TYPE_UNKNOWN2, AFileName.c_str());
        break;
    }

    if (!Message.IsEmpty())
    {
      TWinSCPPlugin * WinSCPPlugin = NB_STATIC_DOWNCAST(TWinSCPPlugin, FarPlugin);
      Result = (WinSCPPlugin->MoreMessageDialog(Message, nullptr, qtWarning,
                qaIgnore | qaAbort) != qaAbort);
    }
  }
#endif
  return Result;
}

bool TSessionDialog::CloseQuery()
{
  bool CanClose = TTabbedDialog::CloseQuery();

  if (CanClose && (GetResult() != brCancel))
  {
    CanClose =
      VerifyKey(PrivateKeyEdit->GetText(), false) &&
      // for tunnel key do not check SSH version as it is not configurable
      VerifyKey(TunnelPrivateKeyEdit->GetText(), true);
  }

  if (CanClose && !PasswordEdit->GetText().IsEmpty() &&
      !GetConfiguration()->GetDisablePasswordStoring() &&
      (PasswordEdit->GetText() != FSessionData->GetPassword()) &&
      (((GetResult() == brOK)) ||
       ((GetResult() == brConnect) && (FAction == saEdit))))
  {
    TWinSCPPlugin * WinSCPPlugin = NB_STATIC_DOWNCAST(TWinSCPPlugin, FarPlugin);
    CanClose = (WinSCPPlugin->MoreMessageDialog(GetMsg(SAVE_PASSWORD), nullptr,
                qtWarning, qaOK | qaCancel) == qaOK);
  }

  return CanClose;
}

void TSessionDialog::SelectTab(intptr_t Tab)
{
  TTabbedDialog::SelectTab(Tab);
  TTabButton * SelectedTabBtn = GetTabButton(Tab);
  intptr_t Index;
  /*for (Index = 0; Index < FTabs->Count; ++Index)
  {
    TTabButton * TabBtn = NB_STATIC_DOWNCAST(TTabButton, FTabs->GetItem(Index));
    // Button->SetBrackets(Button->GetTab() == Tab ? brTight : brNone);
    if (TabBtn == SelectedTabBtn)
      TabBtn->SetColor(0, static_cast<char>((GetSystemColor(COL_DIALOGTEXT) & 0xF0) | 0x09));
    else
      TabBtn->SetColor(0, static_cast<char>((GetSystemColor(COL_DIALOGTEXT) & 0xF0)));
  }*/
  for (Index = 0; Index < FTabs->GetCount(); ++Index)
  {
    TTabButton * TabBtn = NB_STATIC_DOWNCAST(TTabButton, FTabs->GetObj(Index));
    if (TabBtn == SelectedTabBtn)
    {
      break;
    }
  }
  intptr_t SelectedTabIndex = Index;
  intptr_t VisibleTabsCount = GetVisibleTabsCount(SelectedTabIndex, false);
  if ((FFirstVisibleTabIndex < SelectedTabIndex - VisibleTabsCount) ||
      (SelectedTabIndex - VisibleTabsCount == 0))
  {
    FFirstVisibleTabIndex = SelectedTabIndex - VisibleTabsCount;
    ChangeTabs(FFirstVisibleTabIndex);
  }
}

void TSessionDialog::PrevTabClick(TFarButton * /*Sender*/, bool & Close)
{
  Key(nullptr, KEY_CTRLPGUP);
  Close = false;
}

void TSessionDialog::NextTabClick(TFarButton * /*Sender*/, bool & Close)
{
  Key(nullptr, KEY_CTRLPGDN);
  Close = false;
}

void TSessionDialog::ChangeTabs(intptr_t FirstVisibleTabIndex)
{
  // Calculate which tabs are visible
  intptr_t VisibleTabsCount = GetVisibleTabsCount(FirstVisibleTabIndex, true);
  intptr_t LastVisibleTabIndex = FirstVisibleTabIndex + VisibleTabsCount;
  // Change visibility
  for (intptr_t Index = 0; Index < FirstVisibleTabIndex; ++Index)
  {
    TTabButton * TabBtn = NB_STATIC_DOWNCAST(TTabButton, FTabs->GetObj(Index));
    TabBtn->SetVisible(false);
  }
  intptr_t LeftPos = GetBorderBox()->GetLeft() + 2;
  for (intptr_t Index = FirstVisibleTabIndex; Index <= LastVisibleTabIndex; ++Index)
  {
    TTabButton * TabBtn = NB_STATIC_DOWNCAST(TTabButton, FTabs->GetObj(Index));
    intptr_t Width = TabBtn->GetWidth();
    TabBtn->SetLeft(LeftPos);
    TabBtn->SetWidth(Width);
    LeftPos += Width + 1;
    TabBtn->SetVisible(true);
  }
  for (intptr_t Index = LastVisibleTabIndex + 1; Index < FTabs->GetCount(); ++Index)
  {
    TTabButton * TabBtn = NB_STATIC_DOWNCAST(TTabButton, FTabs->GetObj(Index));
    TabBtn->SetVisible(false);
  }
}

intptr_t TSessionDialog::GetVisibleTabsCount(intptr_t TabIndex, bool Forward) const
{
  intptr_t Result = 0;
  intptr_t PWidth = PrevTab->GetWidth();
  intptr_t NWidth = NextTab->GetWidth();
  intptr_t DialogWidth = GetBorderBox()->GetWidth() - 2 - PWidth - NWidth - 2;
  intptr_t TabsWidth = 0;
  if (Forward)
  {
    for (intptr_t Index = TabIndex; Index < FTabs->GetCount() - 1; ++Index)
    {
      TTabButton * TabBtn = NB_STATIC_DOWNCAST(TTabButton, FTabs->GetObj(Index));
      TabsWidth += TabBtn->GetWidth() + 1;
      TTabButton * NextTabBtn = NB_STATIC_DOWNCAST(TTabButton, FTabs->GetObj(Index + 1));
      intptr_t NextTabWidth = NextTabBtn->GetWidth() + 1;
      if (TabsWidth + NextTabWidth >= DialogWidth)
        break;
      Result++;
    }
  }
  else
  {
    for (intptr_t Index = TabIndex; Index >= 1; Index--)
    {
      TTabButton * TabBtn = NB_STATIC_DOWNCAST(TTabButton, FTabs->GetObj(Index));
      TabsWidth += TabBtn->GetWidth() + 1;
      TTabButton * PrevTabBtn = NB_STATIC_DOWNCAST(TTabButton, FTabs->GetObj(Index - 1));
      intptr_t PrevTabWidth = PrevTabBtn->GetWidth() + 1;
      if (TabsWidth + PrevTabWidth >= DialogWidth)
        break;
      Result++;
    }
  }
  return Result;
}

void TSessionDialog::CipherButtonClick(TFarButton * Sender, bool & Close)
{
  if (Sender->GetEnabled())
  {
    intptr_t Source = CipherListBox->GetItems()->GetSelected();
    intptr_t Dest = Source + Sender->GetResult();

    CipherListBox->GetItems()->Move(Source, Dest);
    CipherListBox->GetItems()->SetSelected(Dest);
  }

  Close = false;
}

void TSessionDialog::KexButtonClick(TFarButton * Sender, bool & Close)
{
  if (Sender->GetEnabled())
  {
    intptr_t Source = KexListBox->GetItems()->GetSelected();
    intptr_t Dest = Source + Sender->GetResult();

    KexListBox->GetItems()->Move(Source, Dest);
    KexListBox->GetItems()->SetSelected(Dest);
  }

  Close = false;
}

void TSessionDialog::AuthGSSAPICheckAllowChange(TFarDialogItem * /*Sender*/,
  intptr_t NewState, bool & Allow)
{
  if ((NewState == BSTATE_CHECKED) && !HasGSSAPI(L""))
  {
    Allow = false;
    TWinSCPPlugin * WinSCPPlugin = NB_STATIC_DOWNCAST(TWinSCPPlugin, FarPlugin);

    WinSCPPlugin->MoreMessageDialog(GetMsg(GSSAPI_NOT_INSTALLED),
      nullptr, qtError, qaOK);
  }
}

void TSessionDialog::UnixEnvironmentButtonClick(
  TFarButton * /*Sender*/, bool & /*Close*/)
{
  EOLTypeCombo->SetItemIndex(0);
  DSTModeUnixCheck->SetChecked(true);
}

void TSessionDialog::WindowsEnvironmentButtonClick(
  TFarButton * /*Sender*/, bool & /*Close*/)
{
  EOLTypeCombo->SetItemIndex(1);
  DSTModeWinCheck->SetChecked(true);
}

void TSessionDialog::FillCodePageEdit()
{
  // CodePageEditAdd(CP_UTF8);
  CodePageEdit->GetItems()->AddObject(L"65001 (UTF-8)",
    static_cast<TObject *>(ToPtr(65001)));
  CodePageEditAdd(CP_ACP);
  CodePageEditAdd(CP_OEMCP);
  CodePageEditAdd(20866); // KOI8-r
}

void TSessionDialog::CodePageEditAdd(uint32_t Cp)
{
  CPINFOEX cpInfoEx;
  ::ClearStruct(cpInfoEx);
  if (::GetCodePageInfo(Cp, cpInfoEx))
  {
    CodePageEdit->GetItems()->AddObject(cpInfoEx.CodePageName,
      static_cast<TObject *>(ToPtr(cpInfoEx.CodePage)));
  }
}

intptr_t TSessionDialog::AddTab(intptr_t TabID, const wchar_t * TabCaption)
{
  TFarButtonBrackets TabBrackets = brNone; // brSpace; //
  TTabButton * Tab = new TTabButton(this);
  Tab->SetTabName(UnicodeString(TabCaption));
  Tab->SetTab(TabID);
  Tab->SetBrackets(TabBrackets);
  // SetTabCount(GetTabCount() + 1);
  Tab->SetCenterGroup(false);
  FTabs->Add(Tab);
  return GetItem(Tab);
}

bool TWinSCPFileSystem::SessionDialog(TSessionData * SessionData,
  TSessionActionEnum & Action)
{
  std::unique_ptr<TSessionDialog> Dialog(new TSessionDialog(FPlugin, Action));
  bool Result = Dialog->Execute(SessionData, Action);
  return Result;
}

class TRightsContainer : public TFarDialogContainer
{
NB_DISABLE_COPY(TRightsContainer)
public:
  explicit TRightsContainer(TFarDialog * ADialog, bool AAnyDirectories,
    bool ShowButtons, bool ShowSpecials,
    TFarDialogItem * EnabledDependency);
protected:
  bool FAnyDirectories;
  TFarCheckBox * FCheckBoxes[12];
  TRights::TState FFixedStates[12];
  TFarEdit * FOctalEdit;
  TFarCheckBox * FDirectoriesXCheck;

  virtual void Change();
  void UpdateControls();

public:
  TRights GetRights();
  void SetRights(const TRights & Value);
  void SetAddXToDirectories(bool Value);
  bool GetAddXToDirectories();
  TFarCheckBox * GetChecks(TRights::TRight Right);
  TRights::TState GetStates(TRights::TRight Right);
  bool GetAllowUndef();
  void SetAllowUndef(bool Value);
  void SetStates(TRights::TRight Flag, TRights::TState Value);
  void OctalEditExit(TObject * Sender);
  void RightsButtonClick(TFarButton * Sender, bool & Close);
};

TRightsContainer::TRightsContainer(TFarDialog * ADialog,
  bool AAnyDirectories, bool ShowButtons,
  bool ShowSpecials, TFarDialogItem * EnabledDependency) :
  TFarDialogContainer(ADialog),
  FAnyDirectories(AAnyDirectories),
  FOctalEdit(nullptr),
  FDirectoriesXCheck(nullptr)
{
  GetDialog()->SetNextItemPosition(ipNewLine);

  static int RowLabels[] =
  {
    PROPERTIES_OWNER_RIGHTS,
    PROPERTIES_GROUP_RIGHTS,
    PROPERTIES_OTHERS_RIGHTS
  };
  static int ColLabels[] =
  {
    PROPERTIES_READ_RIGHTS,
    PROPERTIES_WRITE_RIGHTS,
    PROPERTIES_EXECUTE_RIGHTS
  };
  static int SpecialLabels[] =
  {
    PROPERTIES_SETUID_RIGHTS,
    PROPERTIES_SETGID_RIGHTS,
    PROPERTIES_STICKY_BIT_RIGHTS
  };

  for (intptr_t RowIndex = 0; RowIndex < 3; ++RowIndex)
  {
    GetDialog()->SetNextItemPosition(ipNewLine);
    TFarText * Text = new TFarText(GetDialog());
    if (RowIndex == 0)
    {
      Text->SetTop(0);
    }
    Text->SetLeft(0);
    Add(Text);
    Text->SetEnabledDependency(EnabledDependency);
    Text->SetCaption(GetMsg(RowLabels[RowIndex]));

    GetDialog()->SetNextItemPosition(ipRight);

    for (intptr_t ColIndex = 0; ColIndex < 3; ++ColIndex)
    {
      TFarCheckBox * CheckBox = new TFarCheckBox(GetDialog());
      FCheckBoxes[(RowIndex + 1) * 3 + ColIndex] = CheckBox;
      Add(CheckBox);
      CheckBox->SetEnabledDependency(EnabledDependency);
      CheckBox->SetCaption(GetMsg(ColLabels[ColIndex]));
    }

    if (ShowSpecials)
    {
      TFarCheckBox * CheckBox = new TFarCheckBox(GetDialog());
      Add(CheckBox);
      CheckBox->SetVisible(ShowSpecials);
      CheckBox->SetEnabledDependency(EnabledDependency);
      CheckBox->SetCaption(GetMsg(SpecialLabels[RowIndex]));
      FCheckBoxes[RowIndex] = CheckBox;
    }
    else
    {
      FCheckBoxes[RowIndex] = nullptr;
      FFixedStates[RowIndex] = TRights::rsNo;
    }
  }

  GetDialog()->SetNextItemPosition(ipNewLine);

  TFarText * Text = new TFarText(GetDialog());
  Add(Text);
  Text->SetEnabledDependency(EnabledDependency);
  Text->SetLeft(0);
  Text->SetCaption(GetMsg(PROPERTIES_OCTAL));

  GetDialog()->SetNextItemPosition(ipRight);

  FOctalEdit = new TFarEdit(GetDialog());
  Add(FOctalEdit);
  FOctalEdit->SetEnabledDependency(EnabledDependency);
  FOctalEdit->SetWidth(5);
  FOctalEdit->SetMask(L"9999");
  FOctalEdit->SetOnExit(MAKE_CALLBACK(TRightsContainer::OctalEditExit, this));

  if (ShowButtons)
  {
    GetDialog()->SetNextItemPosition(ipRight);

    TFarButton * Button = new TFarButton(GetDialog());
    Add(Button);
    Button->SetEnabledDependency(EnabledDependency);
    Button->SetCaption(GetMsg(PROPERTIES_NONE_RIGHTS));
    Button->SetTag(TRights::rfNo);
    Button->SetOnClick(MAKE_CALLBACK(TRightsContainer::RightsButtonClick, this));

    Button = new TFarButton(GetDialog());
    Add(Button);
    Button->SetEnabledDependency(EnabledDependency);
    Button->SetCaption(GetMsg(PROPERTIES_DEFAULT_RIGHTS));
    Button->SetTag(TRights::rfDefault);
    Button->SetOnClick(MAKE_CALLBACK(TRightsContainer::RightsButtonClick, this));

    Button = new TFarButton(GetDialog());
    Add(Button);
    Button->SetEnabledDependency(EnabledDependency);
    Button->SetCaption(GetMsg(PROPERTIES_ALL_RIGHTS));
    Button->SetTag(TRights::rfAll);
    Button->SetOnClick(MAKE_CALLBACK(TRightsContainer::RightsButtonClick, this));
  }

  GetDialog()->SetNextItemPosition(ipNewLine);

  if (FAnyDirectories)
  {
    FDirectoriesXCheck = new TFarCheckBox(GetDialog());
    Add(FDirectoriesXCheck);
    FDirectoriesXCheck->SetEnabledDependency(EnabledDependency);
    FDirectoriesXCheck->SetLeft(0);
    FDirectoriesXCheck->SetCaption(GetMsg(PROPERTIES_DIRECTORIES_X));
  }
  else
  {
    FDirectoriesXCheck = nullptr;
  }
  ClearArray(FFixedStates);
}

void TRightsContainer::RightsButtonClick(TFarButton * Sender,
  bool & /*Close*/)
{
  TRights R = GetRights();
  R.SetNumber(static_cast<uint16_t>(Sender->GetTag()));
  SetRights(R);
}

void TRightsContainer::OctalEditExit(TObject * /*Sender*/)
{
  if (!::Trim(FOctalEdit->GetText()).IsEmpty())
  {
    TRights R = GetRights();
    R.SetOctal(::Trim(FOctalEdit->GetText()));
    SetRights(R);
  }
}

void TRightsContainer::UpdateControls()
{
  if (GetDialog()->GetHandle())
  {
    TRights R = GetRights();

    if (FDirectoriesXCheck)
    {
      FDirectoriesXCheck->SetEnabled(
        !((R.GetNumberSet() & TRights::rfExec) == TRights::rfExec));
    }

    if (!FOctalEdit->Focused())
    {
      FOctalEdit->SetText(R.GetIsUndef() ? UnicodeString() : R.GetOctal());
    }
    else if (::Trim(FOctalEdit->GetText()).Length() >= 3)
    {
      try
      {
        OctalEditExit(nullptr);
      }
      catch (...)
      {
      }
    }
  }
}

void TRightsContainer::Change()
{
  TFarDialogContainer::Change();

  if (GetDialog()->GetHandle())
  {
    UpdateControls();
  }
}

TFarCheckBox * TRightsContainer::GetChecks(TRights::TRight Right)
{
  DebugAssert((Right >= 0) && (static_cast<size_t>(Right) < _countof(FCheckBoxes)));
  return FCheckBoxes[Right];
}

TRights::TState TRightsContainer::GetStates(TRights::TRight Right)
{
  TFarCheckBox * CheckBox = GetChecks(Right);
  if (CheckBox != nullptr)
  {
    switch (CheckBox->GetSelected())
    {
      case BSTATE_UNCHECKED: return TRights::rsNo;
      case BSTATE_CHECKED: return TRights::rsYes;
      case BSTATE_3STATE:
      default: return TRights::rsUndef;
    }
  }
  else
  {
    return FFixedStates[Right];
  }
}

void TRightsContainer::SetStates(TRights::TRight Right,
  TRights::TState Value)
{
  TFarCheckBox * CheckBox = GetChecks(Right);
  if (CheckBox != nullptr)
  {
    switch (Value)
    {
      case TRights::rsNo:
        CheckBox->SetSelected(BSTATE_UNCHECKED);
        break;
      case TRights::rsYes:
        CheckBox->SetSelected(BSTATE_CHECKED);
        break;
      case TRights::rsUndef:
        CheckBox->SetSelected(BSTATE_3STATE);
        break;
    }
  }
  else
  {
    FFixedStates[Right] = Value;
  }
}

TRights TRightsContainer::GetRights()
{
  TRights Result;
  Result.SetAllowUndef(GetAllowUndef());
  for (size_t Right = 0; Right < _countof(FCheckBoxes); Right++)
  {
    Result.SetRightUndef(static_cast<TRights::TRight>(Right),
      GetStates(static_cast<TRights::TRight>(Right)));
  }
  return Result;
}

void TRightsContainer::SetRights(const TRights & Value)
{
  if (GetRights() != Value)
  {
    GetDialog()->LockChanges();
    SCOPE_EXIT
    {
      GetDialog()->UnlockChanges();
    };
    SetAllowUndef(true); // temporarily
    for (size_t Right = 0; Right < _countof(FCheckBoxes); Right++)
    {
      SetStates(static_cast<TRights::TRight>(Right),
        Value.GetRightUndef(static_cast<TRights::TRight>(Right)));
    }
    SetAllowUndef(Value.GetAllowUndef());
  }
}

bool TRightsContainer::GetAddXToDirectories()
{
  return FDirectoriesXCheck ? FDirectoriesXCheck->GetChecked() : false;
}

void TRightsContainer::SetAddXToDirectories(bool Value)
{
  if (FDirectoriesXCheck)
  {
    FDirectoriesXCheck->SetChecked(Value);
  }
}

bool TRightsContainer::GetAllowUndef()
{
  DebugAssert(FCheckBoxes[_countof(FCheckBoxes) - 1] != nullptr);
  return FCheckBoxes[_countof(FCheckBoxes) - 1]->GetAllowGrayed();
}

void TRightsContainer::SetAllowUndef(bool Value)
{
  for (size_t Right = 0; Right < _countof(FCheckBoxes); Right++)
  {
    if (FCheckBoxes[Right] != nullptr)
    {
      FCheckBoxes[Right]->SetAllowGrayed(Value);
    }
  }
}

class TPropertiesDialog : public TFarDialog
{
NB_DISABLE_COPY(TPropertiesDialog)
public:
  explicit TPropertiesDialog(TCustomFarPlugin * AFarPlugin, TStrings * AFileList,
    const UnicodeString & Directory,
    // TStrings * GroupList, TStrings * UserList,
    const TRemoteTokenList * GroupList, const TRemoteTokenList * UserList,
    intptr_t AllowedChanges);

  bool Execute(TRemoteProperties * Properties);

protected:
  virtual void Change();
  void UpdateProperties(TRemoteProperties & Properties);

private:
  bool FAnyDirectories;
  intptr_t FAllowedChanges;
  TRemoteProperties FOrigProperties;
  bool FMultiple;

  TRightsContainer * RightsContainer;
  TFarComboBox * OwnerComboBox;
  TFarComboBox * GroupComboBox;
  TFarCheckBox * RecursiveCheck;
  TFarButton * OkButton;
};

TPropertiesDialog::TPropertiesDialog(TCustomFarPlugin * AFarPlugin,
  TStrings * AFileList, const UnicodeString & /*Directory*/,
  const TRemoteTokenList * GroupList, const TRemoteTokenList * UserList,
  intptr_t AAllowedChanges) :
  TFarDialog(AFarPlugin),
  FAnyDirectories(false),
  FAllowedChanges(AAllowedChanges),
  RightsContainer(nullptr),
  OwnerComboBox(nullptr),
  GroupComboBox(nullptr),
  RecursiveCheck(nullptr),
  OkButton(nullptr)
{
  DebugAssert(AFileList->GetCount() > 0);
  TRemoteFile * OnlyFile = NB_STATIC_DOWNCAST(TRemoteFile, AFileList->GetObj(0));
  DebugUsedParam(OnlyFile);
  DebugAssert(OnlyFile);
  FMultiple = (AFileList->GetCount() > 1);

  {
    std::unique_ptr<TStrings> UsedGroupList;
    std::unique_ptr<TStrings> UsedUserList;
    if ((GroupList == nullptr) || (GroupList->GetCount() == 0))
    {
      UsedGroupList.reset(new TStringList());
      UsedGroupList->SetDuplicates(dupIgnore);
      UsedGroupList->SetSorted(true);
    }
    if ((UserList == nullptr) || (UserList->GetCount() == 0))
    {
      UsedUserList.reset(new TStringList());
      UsedUserList->SetDuplicates(dupIgnore);
      UsedUserList->SetSorted(true);
    }

    intptr_t Directories = 0;
    for (intptr_t Index = 0; Index < AFileList->GetCount(); ++Index)
    {
      TRemoteFile * File = NB_STATIC_DOWNCAST(TRemoteFile, AFileList->GetObj(Index));
      DebugAssert(File);
      if (UsedGroupList.get() && !File->GetFileGroup().GetName().IsEmpty())
      {
        UsedGroupList->Add(File->GetFileGroup().GetName());
      }
      if (UsedUserList.get() && !File->GetFileOwner().GetName().IsEmpty())
      {
        UsedUserList->Add(File->GetFileOwner().GetName());
      }
      if (File->GetIsDirectory())
      {
        Directories++;
      }
    }
    FAnyDirectories = (Directories > 0);

    SetCaption(GetMsg(PROPERTIES_CAPTION));

    SetSize(TPoint(56, 19));

    TFarButton * Button;
    TFarSeparator * Separator;
    TFarText * Text;
    TRect CRect = GetClientRect();

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(PROPERTIES_PROMPT));
    Text->SetCenterGroup(true);

    SetNextItemPosition(ipNewLine);

    Text = new TFarText(this);
    Text->SetCenterGroup(true);
    if (AFileList->GetCount() > 1)
    {
      Text->SetCaption(FORMAT(GetMsg(PROPERTIES_PROMPT_FILES).c_str(), AFileList->GetCount()));
    }
    else
    {
      Text->SetCaption(core::MinimizeName(AFileList->GetString(0), static_cast<intptr_t>(GetClientSize().x), true));
    }
    TRemoteFile * File = NB_STATIC_DOWNCAST(TRemoteFile, AFileList->GetObj(0));
    if (!File->GetLinkTo().IsEmpty())
    {
      Text = new TFarText(this);
      Text->SetCaption(GetMsg(PROPERTIES_LINKTO));

      SetNextItemPosition(ipRight);

      TFarEdit* Edit = new TFarEdit(this);
      Edit->SetText(File->GetLinkTo());
      Edit->SetReadOnly(true);

      SetNextItemPosition(ipNewLine);
    }

    new TFarSeparator(this);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(PROPERTIES_OWNER));
    Text->SetEnabled((FAllowedChanges & cpOwner) != 0);

    SetNextItemPosition(ipRight);

    OwnerComboBox = new TFarComboBox(this);
    OwnerComboBox->SetWidth(20);
    OwnerComboBox->SetEnabled((FAllowedChanges & cpOwner) != 0);
    if (UsedUserList.get())
    {
      OwnerComboBox->GetItems()->Assign(UsedUserList.get());
    }
    else if (UserList)
    {
      for (intptr_t Index = 0; Index < UserList->GetCount(); ++Index)
      {
        OwnerComboBox->GetItems()->Add(UserList->Token(Index)->GetName());
      }
    }

    SetNextItemPosition(ipNewLine);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(PROPERTIES_GROUP));
    Text->SetEnabled((FAllowedChanges & cpGroup) != 0);

    SetNextItemPosition(ipRight);

    GroupComboBox = new TFarComboBox(this);
    GroupComboBox->SetWidth(OwnerComboBox->GetWidth());
    GroupComboBox->SetEnabled((FAllowedChanges & cpGroup) != 0);
    if (UsedGroupList.get())
    {
      GroupComboBox->GetItems()->Assign(UsedGroupList.get());
    }
    else if (GroupList)
    {
      for (intptr_t Index = 0; Index < GroupList->GetCount(); ++Index)
      {
        GroupComboBox->GetItems()->Add(GroupList->Token(Index)->GetName());
      }
    }

    SetNextItemPosition(ipNewLine);

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(PROPERTIES_RIGHTS));

    RightsContainer = new TRightsContainer(this, FAnyDirectories,
      true, true, nullptr);
    RightsContainer->SetEnabled(FAllowedChanges & cpMode);

    if (FAnyDirectories)
    {
      Separator = new TFarSeparator(this);
      Separator->SetPosition(Separator->GetPosition() + RightsContainer->GetTop());

      RecursiveCheck = new TFarCheckBox(this);
      RecursiveCheck->SetCaption(GetMsg(PROPERTIES_RECURSIVE));
    }
    else
    {
      RecursiveCheck = nullptr;
    }

    SetNextItemPosition(ipNewLine);

    Separator = new TFarSeparator(this);
    Separator->SetPosition(CRect.Bottom - 1);

    OkButton = new TFarButton(this);
    OkButton->SetCaption(GetMsg(MSG_BUTTON_OK));
    OkButton->SetDefault(true);
    OkButton->SetResult(brOK);
    OkButton->SetCenterGroup(true);

    SetNextItemPosition(ipRight);

    Button = new TFarButton(this);
    Button->SetCaption(GetMsg(MSG_BUTTON_Cancel));
    Button->SetResult(brCancel);
    Button->SetCenterGroup(true);
  }
}

void TPropertiesDialog::Change()
{
  TFarDialog::Change();

  if (GetHandle())
  {
    TRemoteProperties FileProperties;
    UpdateProperties(FileProperties);

    if (!FMultiple)
    {
      // when setting properties for one file only, allow undef state
      // only when the input right explicitly requires it or
      // when "recursive" is on (possible for directory only).
      bool AllowUndef =
        (FOrigProperties.Valid.Contains(vpRights) &&
         FOrigProperties.Rights.GetAllowUndef()) ||
        ((RecursiveCheck != nullptr) && (RecursiveCheck->GetChecked()));
      if (!AllowUndef)
      {
        // when disallowing undef state, make sure, all undef are turned into unset
        RightsContainer->SetRights(TRights(RightsContainer->GetRights().GetNumberSet()));
      }
      RightsContainer->SetAllowUndef(AllowUndef);
    }

    OkButton->SetEnabled(
      // group name is specified or we set multiple-file properties and
      // no valid group was specified (there are at least two different groups)
      (!GroupComboBox->GetText().IsEmpty() ||
       (FMultiple && !FOrigProperties.Valid.Contains(vpGroup)) ||
       (FOrigProperties.Group.GetName() == GroupComboBox->GetText())) &&
      // same but with owner
      (!OwnerComboBox->GetText().IsEmpty() ||
       (FMultiple && !FOrigProperties.Valid.Contains(vpOwner)) ||
       (FOrigProperties.Owner.GetName() == OwnerComboBox->GetText())) &&
      ((FileProperties != FOrigProperties) || (RecursiveCheck && RecursiveCheck->GetChecked())));
  }
}

void TPropertiesDialog::UpdateProperties(TRemoteProperties & Properties)
{
  if (FAllowedChanges & cpMode)
  {
    Properties.Valid << vpRights;
    Properties.Rights = RightsContainer->GetRights();
    Properties.AddXToDirectories = RightsContainer->GetAddXToDirectories();
  }

#define STORE_NAME(PROPERTY) \
    if (!PROPERTY ## ComboBox->GetText().IsEmpty() && \
        FAllowedChanges & cp ## PROPERTY) \
    { \
      Properties.Valid << vp ## PROPERTY; \
      Properties.PROPERTY.SetName(::Trim(PROPERTY ## ComboBox->GetText())); \
    }
  STORE_NAME(Group);
  STORE_NAME(Owner);
#undef STORE_NAME

  Properties.Recursive = RecursiveCheck != nullptr && RecursiveCheck->GetChecked();
}

bool TPropertiesDialog::Execute(TRemoteProperties * Properties)
{
  TValidProperties Valid;
  if (Properties->Valid.Contains(vpRights) && FAllowedChanges & cpMode)
  {
    Valid << vpRights;
  }
  if (Properties->Valid.Contains(vpOwner) && FAllowedChanges & cpOwner)
  {
    Valid << vpOwner;
  }
  if (Properties->Valid.Contains(vpGroup) && FAllowedChanges & cpGroup)
  {
    Valid << vpGroup;
  }
  FOrigProperties = *Properties;
  FOrigProperties.Valid = Valid;
  FOrigProperties.Recursive = false;

  if (Properties->Valid.Contains(vpRights))
  {
    RightsContainer->SetRights(Properties->Rights);
    RightsContainer->SetAddXToDirectories(Properties->AddXToDirectories);
  }
  else
  {
    RightsContainer->SetRights(TRights());
    RightsContainer->SetAddXToDirectories(false);
  }
  OwnerComboBox->SetText(Properties->Valid.Contains(vpOwner) ?
    Properties->Owner.GetName() : UnicodeString());
  GroupComboBox->SetText(Properties->Valid.Contains(vpGroup) ?
    Properties->Group.GetName() : UnicodeString());
  if (RecursiveCheck)
  {
    RecursiveCheck->SetChecked(Properties->Recursive);
  }

  bool Result = ShowModal() != brCancel;
  if (Result)
  {
    *Properties = TRemoteProperties();
    UpdateProperties(*Properties);
  }
  return Result;
}

bool TWinSCPFileSystem::PropertiesDialog(TStrings * AFileList,
  const UnicodeString & Directory,
  const TRemoteTokenList * GroupList, const TRemoteTokenList * UserList,
  TRemoteProperties * Properties, intptr_t AllowedChanges)
{
  std::unique_ptr<TPropertiesDialog> Dialog(new TPropertiesDialog(FPlugin, AFileList,
    Directory, GroupList, UserList, AllowedChanges));
  bool Result = Dialog->Execute(Properties);
  return Result;
}

class TCopyParamsContainer : public TFarDialogContainer
{
public:
  explicit TCopyParamsContainer(TFarDialog * ADialog,
    intptr_t Options, intptr_t CopyParamAttrs);

  void SetParams(const TCopyParamType & Value);
  TCopyParamType GetParams();
  int GetHeight();

protected:
  TFarRadioButton * TMTextButton;
  TFarRadioButton * TMBinaryButton;
  TFarRadioButton * TMAutomaticButton;
  TFarEdit * AsciiFileMaskEdit;
  TRightsContainer * RightsContainer;
  TFarRadioButton * CCNoChangeButton;
  TFarRadioButton * CCUpperCaseButton;
  TFarRadioButton * CCLowerCaseButton;
  TFarRadioButton * CCFirstUpperCaseButton;
  TFarRadioButton * CCLowerCaseShortButton;
  TFarCheckBox * ReplaceInvalidCharsCheck;
  TFarCheckBox * PreserveRightsCheck;
  TFarCheckBox * PreserveTimeCheck;
  TFarCheckBox * PreserveReadOnlyCheck;
  TFarCheckBox * IgnorePermErrorsCheck;
  TFarCheckBox * ClearArchiveCheck;
  TFarCheckBox * CalculateSizeCheck;
  TFarText * FileMaskText;
  TFarEdit * FileMaskEdit;
  TFarComboBox * SpeedCombo;

  void ValidateMaskComboExit(TObject * Sender);
  void ValidateSpeedComboExit(TObject * Sender);
  virtual void Change();
  void UpdateControls();

private:
  intptr_t FOptions;
  intptr_t FCopyParamAttrs;
  TCopyParamType FParams;
};

TCopyParamsContainer::TCopyParamsContainer(TFarDialog * ADialog,
  intptr_t Options, intptr_t CopyParamAttrs) :
  TFarDialogContainer(ADialog),
  TMTextButton(nullptr),
  TMBinaryButton(nullptr),
  TMAutomaticButton(nullptr),
  AsciiFileMaskEdit(nullptr),
  RightsContainer(nullptr),
  CCNoChangeButton(nullptr),
  CCUpperCaseButton(nullptr),
  CCLowerCaseButton(nullptr),
  CCFirstUpperCaseButton(nullptr),
  CCLowerCaseShortButton(nullptr),
  ReplaceInvalidCharsCheck(nullptr),
  PreserveRightsCheck(nullptr),
  PreserveTimeCheck(nullptr),
  PreserveReadOnlyCheck(nullptr),
  IgnorePermErrorsCheck(nullptr),
  ClearArchiveCheck(nullptr),
  CalculateSizeCheck(nullptr),
  FileMaskText(nullptr),
  FileMaskEdit(nullptr),
  SpeedCombo(nullptr),
  FOptions(Options), FCopyParamAttrs(CopyParamAttrs)
{
  TFarBox * Box;
  TFarSeparator * Separator;
  TFarText * Text;

  intptr_t TMWidth = 37;
  intptr_t TMTop;
  intptr_t TMBottom;

  SetLeft(GetLeft() - 1);

  Box = new TFarBox(GetDialog());
  Box->SetLeft(0);
  Box->SetTop(0);
  Box->SetHeight(1);
  Add(Box);
  Box->SetWidth(TMWidth + 2);
  Box->SetCaption(GetMsg(TRANSFER_MODE));

  GetDialog()->SetNextItemPosition(ipRight);

  Box = new TFarBox(GetDialog());
  Add(Box);
  Box->SetLeft(Box->GetLeft() - 2);
  Box->SetRight(Box->GetRight() + 1);
  Box->SetCaption(GetMsg(TRANSFER_UPLOAD_OPTIONS));

  GetDialog()->SetNextItemPosition(ipNewLine);

  TMTextButton = new TFarRadioButton(GetDialog());
  TMTextButton->SetLeft(1);
  Add(TMTextButton);
  TMTop = TMTextButton->GetTop();
  TMTextButton->SetCaption(GetMsg(TRANSFER_MODE_TEXT));
  TMTextButton->SetEnabled(
    FLAGCLEAR(CopyParamAttrs, cpaNoTransferMode));

  TMBinaryButton = new TFarRadioButton(GetDialog());
  TMBinaryButton->SetLeft(1);
  Add(TMBinaryButton);
  TMBinaryButton->SetCaption(GetMsg(TRANSFER_MODE_BINARY));
  TMBinaryButton->SetEnabled(TMTextButton->GetEnabled());

  TMAutomaticButton = new TFarRadioButton(GetDialog());
  TMAutomaticButton->SetLeft(1);
  Add(TMAutomaticButton);
  TMAutomaticButton->SetCaption(GetMsg(TRANSFER_MODE_AUTOMATIC));
  TMAutomaticButton->SetEnabled(TMTextButton->GetEnabled());

  Text = new TFarText(GetDialog());
  Text->SetLeft(1);
  Add(Text);
  Text->SetCaption(GetMsg(TRANSFER_MODE_MASK));
  Text->SetEnabledDependency(TMAutomaticButton);

  AsciiFileMaskEdit = new TFarEdit(GetDialog());
  AsciiFileMaskEdit->SetLeft(1);
  Add(AsciiFileMaskEdit);
  AsciiFileMaskEdit->SetEnabledDependency(TMAutomaticButton);
  AsciiFileMaskEdit->SetWidth(TMWidth);
  AsciiFileMaskEdit->SetHistory(ASCII_MASK_HISTORY);
  AsciiFileMaskEdit->SetOnExit(MAKE_CALLBACK(TCopyParamsContainer::ValidateMaskComboExit, this));

  Box = new TFarBox(GetDialog());
  Box->SetLeft(0);
  Add(Box);
  Box->SetWidth(TMWidth + 2);
  Box->SetCaption(GetMsg(TRANSFER_FILENAME_MODIFICATION));

  CCNoChangeButton = new TFarRadioButton(GetDialog());
  CCNoChangeButton->SetLeft(1);
  Add(CCNoChangeButton);
  CCNoChangeButton->SetCaption(GetMsg(TRANSFER_FILENAME_NOCHANGE));
  CCNoChangeButton->SetEnabled(true);

  GetDialog()->SetNextItemPosition(ipRight);

  CCUpperCaseButton = new TFarRadioButton(GetDialog());
  Add(CCUpperCaseButton);
  CCUpperCaseButton->SetCaption(GetMsg(TRANSFER_FILENAME_UPPERCASE));
  CCUpperCaseButton->SetEnabled(CCNoChangeButton->GetEnabled());

  GetDialog()->SetNextItemPosition(ipNewLine);

  CCFirstUpperCaseButton = new TFarRadioButton(GetDialog());
  CCFirstUpperCaseButton->SetLeft(1);
  Add(CCFirstUpperCaseButton);
  CCFirstUpperCaseButton->SetCaption(GetMsg(TRANSFER_FILENAME_FIRSTUPPERCASE));
  CCFirstUpperCaseButton->SetEnabled(CCNoChangeButton->GetEnabled());

  GetDialog()->SetNextItemPosition(ipRight);

  CCLowerCaseButton = new TFarRadioButton(GetDialog());
  Add(CCLowerCaseButton);
  CCLowerCaseButton->SetCaption(GetMsg(TRANSFER_FILENAME_LOWERCASE));
  CCLowerCaseButton->SetEnabled(CCNoChangeButton->GetEnabled());

  GetDialog()->SetNextItemPosition(ipNewLine);

  CCLowerCaseShortButton = new TFarRadioButton(GetDialog());
  CCLowerCaseShortButton->SetLeft(1);
  Add(CCLowerCaseShortButton);
  CCLowerCaseShortButton->SetCaption(GetMsg(TRANSFER_FILENAME_LOWERCASESHORT));
  CCLowerCaseShortButton->SetEnabled(CCNoChangeButton->GetEnabled());

  GetDialog()->SetNextItemPosition(ipRight);

  ReplaceInvalidCharsCheck = new TFarCheckBox(GetDialog());
  Add(ReplaceInvalidCharsCheck);
  ReplaceInvalidCharsCheck->SetCaption(GetMsg(TRANSFER_FILENAME_REPLACE_INVALID));
  ReplaceInvalidCharsCheck->SetEnabled(CCNoChangeButton->GetEnabled());

  GetDialog()->SetNextItemPosition(ipNewLine);

  Box = new TFarBox(GetDialog());
  Box->SetLeft(0);
  Add(Box);
  Box->SetWidth(TMWidth + 2);
  Box->SetCaption(GetMsg(TRANSFER_DOWNLOAD_OPTIONS));

  PreserveReadOnlyCheck = new TFarCheckBox(GetDialog());
  Add(PreserveReadOnlyCheck);
  PreserveReadOnlyCheck->SetLeft(1);
  PreserveReadOnlyCheck->SetCaption(GetMsg(TRANSFER_PRESERVE_READONLY));
  PreserveReadOnlyCheck->SetEnabled(
    FLAGCLEAR(CopyParamAttrs, cpaNoPreserveReadOnly));
  TMBottom = PreserveReadOnlyCheck->GetTop();

  PreserveRightsCheck = new TFarCheckBox(GetDialog());
  Add(PreserveRightsCheck);
  PreserveRightsCheck->SetLeft(TMWidth + 3);
  PreserveRightsCheck->SetTop(TMTop);
  PreserveRightsCheck->SetBottom(TMTop);
  PreserveRightsCheck->SetCaption(GetMsg(TRANSFER_PRESERVE_RIGHTS));
  PreserveRightsCheck->SetEnabled(
    FLAGCLEAR(CopyParamAttrs, cpaNoRights));

  GetDialog()->SetNextItemPosition(ipBelow);

  RightsContainer = new TRightsContainer(GetDialog(), true, false,
    false, PreserveRightsCheck);
  RightsContainer->SetLeft(PreserveRightsCheck->GetActualBounds().Left);
  RightsContainer->SetTop(PreserveRightsCheck->GetActualBounds().Top + 1);

  IgnorePermErrorsCheck = new TFarCheckBox(GetDialog());
  Add(IgnorePermErrorsCheck);
  IgnorePermErrorsCheck->SetLeft(PreserveRightsCheck->GetLeft());
  IgnorePermErrorsCheck->SetTop(TMTop + 6);
  IgnorePermErrorsCheck->SetCaption(GetMsg(TRANSFER_PRESERVE_PERM_ERRORS));

  ClearArchiveCheck = new TFarCheckBox(GetDialog());
  ClearArchiveCheck->SetLeft(IgnorePermErrorsCheck->GetLeft());
  Add(ClearArchiveCheck);
  ClearArchiveCheck->SetTop(TMTop + 7);
  ClearArchiveCheck->SetCaption(GetMsg(TRANSFER_CLEAR_ARCHIVE));
  ClearArchiveCheck->SetEnabled(
    FLAGCLEAR(FOptions, coTempTransfer) &&
    FLAGCLEAR(CopyParamAttrs, cpaNoClearArchive));

  Box = new TFarBox(GetDialog());
  Box->SetTop(TMTop + 8);
  Add(Box);
  Box->SetBottom(Box->GetTop());
  Box->SetLeft(TMWidth + 3 - 1);
  Box->SetCaption(GetMsg(TRANSFER_COMMON_OPTIONS));

  PreserveTimeCheck = new TFarCheckBox(GetDialog());
  Add(PreserveTimeCheck);
  PreserveTimeCheck->SetLeft(TMWidth + 3);
  PreserveTimeCheck->SetCaption(GetMsg(TRANSFER_PRESERVE_TIMESTAMP));
  PreserveTimeCheck->SetEnabled(
    FLAGCLEAR(CopyParamAttrs, cpaNoPreserveTime));

  CalculateSizeCheck = new TFarCheckBox(GetDialog());
  CalculateSizeCheck->SetCaption(GetMsg(TRANSFER_CALCULATE_SIZE));
  Add(CalculateSizeCheck);
  CalculateSizeCheck->SetLeft(TMWidth + 3);

  GetDialog()->SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(GetDialog());
  Add(Separator);
  Separator->SetPosition(TMBottom + 1);
  Separator->SetCaption(GetMsg(TRANSFER_OTHER));

  FileMaskText = new TFarText(GetDialog());
  FileMaskText->SetLeft(1);
  Add(FileMaskText);
  FileMaskText->SetCaption(GetMsg(TRANSFER_FILE_MASK));

  GetDialog()->SetNextItemPosition(ipNewLine);

  FileMaskEdit = new TFarEdit(GetDialog());
  FileMaskEdit->SetLeft(1);
  Add(FileMaskEdit);
  FileMaskEdit->SetWidth(TMWidth);
  FileMaskEdit->SetHistory(WINSCP_FILE_MASK_HISTORY);
  FileMaskEdit->SetOnExit(MAKE_CALLBACK(TCopyParamsContainer::ValidateMaskComboExit, this));
  FileMaskEdit->SetEnabled(true);

  GetDialog()->SetNextItemPosition(ipNewLine);

  Text = new TFarText(GetDialog());
  Add(Text);
  Text->SetCaption(GetMsg(TRANSFER_SPEED));
  Text->MoveAt(TMWidth + 3, FileMaskText->GetTop());

  GetDialog()->SetNextItemPosition(ipRight);

  SpeedCombo = new TFarComboBox(GetDialog());
  Add(SpeedCombo);
  SpeedCombo->GetItems()->Add(LoadStr(SPEED_UNLIMITED));
  intptr_t Speed = 1024;
  while (Speed >= 8)
  {
    SpeedCombo->GetItems()->Add(::IntToStr(Speed));
    Speed = Speed / 2;
  }
  SpeedCombo->SetOnExit(MAKE_CALLBACK(TCopyParamsContainer::ValidateSpeedComboExit, this));

  GetDialog()->SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(GetDialog());
  Separator->SetPosition(FileMaskEdit->GetBottom() + 1);
  Separator->SetLeft(0);
  Add(Separator);
}

void TCopyParamsContainer::UpdateControls()
{
  if (IgnorePermErrorsCheck != nullptr)
  {
    IgnorePermErrorsCheck->SetEnabled(
      ((PreserveRightsCheck->GetEnabled() && PreserveRightsCheck->GetChecked()) ||
       (PreserveTimeCheck->GetEnabled() && PreserveTimeCheck->GetChecked())) &&
      FLAGCLEAR(FCopyParamAttrs, cpaNoIgnorePermErrors));
  }
}

void TCopyParamsContainer::Change()
{
  TFarDialogContainer::Change();

  if (GetDialog()->GetHandle())
  {
    UpdateControls();
  }
}

void TCopyParamsContainer::SetParams(const TCopyParamType & Value)
{
  if (TMBinaryButton->GetEnabled())
  {
    switch (Value.GetTransferMode())
    {
      case tmAscii:
        TMTextButton->SetChecked(true);
        break;

      case tmBinary:
        TMBinaryButton->SetChecked(true);
        break;

      default:
        TMAutomaticButton->SetChecked(true);
        break;
    }
  }
  else
  {
    TMBinaryButton->SetChecked(true);
  }

  AsciiFileMaskEdit->SetText(Value.GetAsciiFileMask().GetMasks());

  switch (Value.GetFileNameCase())
  {
    case ncLowerCase:
      CCLowerCaseButton->SetChecked(true);
      break;

    case ncUpperCase:
      CCUpperCaseButton->SetChecked(true);
      break;

    case ncFirstUpperCase:
      CCFirstUpperCaseButton->SetChecked(true);
      break;

    case ncLowerCaseShort:
      CCLowerCaseShortButton->SetChecked(true);
      break;

    default:
    case ncNoChange:
      CCNoChangeButton->SetChecked(true);
      break;
  }

  RightsContainer->SetAddXToDirectories(Value.GetAddXToDirectories());
  RightsContainer->SetRights(Value.GetRights());
  PreserveRightsCheck->SetChecked(Value.GetPreserveRights());
  IgnorePermErrorsCheck->SetChecked(Value.GetIgnorePermErrors());

  PreserveReadOnlyCheck->SetChecked(Value.GetPreserveReadOnly());
  ReplaceInvalidCharsCheck->SetChecked(
    Value.GetInvalidCharsReplacement() != TCopyParamType::NoReplacement);

  ClearArchiveCheck->SetChecked(Value.GetClearArchive());

  FileMaskEdit->SetText(Value.GetIncludeFileMask().GetMasks());
  PreserveTimeCheck->SetChecked(Value.GetPreserveTime());
  CalculateSizeCheck->SetChecked(Value.GetCalculateSize());

  SpeedCombo->SetText(SetSpeedLimit(Value.GetCPSLimit()));

  FParams = Value;
}

TCopyParamType TCopyParamsContainer::GetParams()
{
  TCopyParamType Result = FParams;

  DebugAssert(TMTextButton->GetChecked() || TMBinaryButton->GetChecked() || TMAutomaticButton->GetChecked());
  if (TMTextButton->GetChecked())
  {
    Result.SetTransferMode(tmAscii);
  }
  else if (TMAutomaticButton->GetChecked())
  {
    Result.SetTransferMode(tmAutomatic);
  }
  else
  {
    Result.SetTransferMode(tmBinary);
  }

  if (Result.GetTransferMode() == tmAutomatic)
  {
    Result.GetAsciiFileMask().SetMasks(AsciiFileMaskEdit->GetText());
  }

  if (CCLowerCaseButton->GetChecked())
  {
    Result.SetFileNameCase(ncLowerCase);
  }
  else if (CCUpperCaseButton->GetChecked())
  {
    Result.SetFileNameCase(ncUpperCase);
  }
  else if (CCFirstUpperCaseButton->GetChecked())
  {
    Result.SetFileNameCase(ncFirstUpperCase);
  }
  else if (CCLowerCaseShortButton->GetChecked())
  {
    Result.SetFileNameCase(ncLowerCaseShort);
  }
  else
  {
    Result.SetFileNameCase(ncNoChange);
  }

  Result.SetAddXToDirectories(RightsContainer->GetAddXToDirectories());
  Result.SetRights(RightsContainer->GetRights());
  Result.SetPreserveRights(PreserveRightsCheck->GetChecked());
  Result.SetIgnorePermErrors(IgnorePermErrorsCheck->GetChecked());

  Result.SetReplaceInvalidChars(ReplaceInvalidCharsCheck->GetChecked());
  Result.SetPreserveReadOnly(PreserveReadOnlyCheck->GetChecked());

  Result.SetClearArchive(ClearArchiveCheck->GetChecked());

  Result.GetIncludeFileMask().SetMasks(FileMaskEdit->GetText());
  Result.SetPreserveTime(PreserveTimeCheck->GetChecked());
  Result.SetCalculateSize(CalculateSizeCheck->GetChecked());

  Result.SetCPSLimit(GetSpeedLimit(SpeedCombo->GetText()));

  return Result;
}

void TCopyParamsContainer::ValidateMaskComboExit(TObject * Sender)
{
  ValidateMaskEdit(NB_STATIC_DOWNCAST(TFarEdit, Sender));
}

void TCopyParamsContainer::ValidateSpeedComboExit(TObject * /*Sender*/)
{
  try
  {
    GetSpeedLimit(SpeedCombo->GetText());
  }
  catch (...)
  {
    SpeedCombo->SetFocus();
    throw;
  }
}

int TCopyParamsContainer::GetHeight()
{
  return 16;
}

class TCopyDialog : TFarDialog
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  explicit TCopyDialog(TCustomFarPlugin * AFarPlugin,
    bool ToRemote, bool Move, const TStrings * AFileList, intptr_t Options, intptr_t CopyParamAttrs);

  bool Execute(OUT UnicodeString & TargetDirectory, OUT TGUICopyParamType * Params);

protected:
  virtual bool CloseQuery();
  virtual void Change();
  void CustomCopyParam();

  void CopyParamListerClick(TFarDialogItem * Item, MOUSE_EVENT_RECORD * Event);
  void TransferSettingsButtonClick(TFarButton * Sender, bool & Close);

private:
  TFarEdit * DirectoryEdit;
  TFarLister * CopyParamLister;
  TFarCheckBox * NewerOnlyCheck;
  TFarCheckBox * SaveSettingsCheck;
  TFarCheckBox * QueueCheck;
  TFarCheckBox * QueueNoConfirmationCheck;

  const TStrings * FFileList;
  intptr_t FOptions;
  intptr_t FCopyParamAttrs;
  TGUICopyParamType FCopyParams;
  bool FToRemote;
};

TCopyDialog::TCopyDialog(TCustomFarPlugin * AFarPlugin,
  bool ToRemote, bool Move, const TStrings * AFileList,
  intptr_t Options, intptr_t CopyParamAttrs) :
  TFarDialog(AFarPlugin),
  DirectoryEdit(nullptr),
  NewerOnlyCheck(nullptr),
  QueueCheck(nullptr),
  QueueNoConfirmationCheck(nullptr),
  FFileList(AFileList),
  FOptions(Options),
  FCopyParamAttrs(CopyParamAttrs),
  FToRemote(ToRemote)
{
  DebugAssert(FFileList);
  const intptr_t DlgLength = 78;
  SetSize(TPoint(DlgLength, 12 + (FLAGCLEAR(FOptions, coTempTransfer) ? 4 : 0)));

  SetCaption(GetMsg(Move ? MOVE_TITLE : COPY_TITLE));

  if (FLAGCLEAR(FOptions, coTempTransfer))
  {
    UnicodeString Prompt;
    if (FFileList->GetCount() > 1)
    {
      Prompt = FORMAT(GetMsg(Move ? MOVE_FILES_PROMPT : COPY_FILES_PROMPT).c_str(), FFileList->GetCount());
    }
    else
    {
      UnicodeString PromptMsg = GetMsg(Move ? MOVE_FILE_PROMPT : COPY_FILE_PROMPT);
      UnicodeString FileName = FFileList->GetString(0);
      UnicodeString OnlyFileName = ToRemote ?
        base::ExtractFileName(FileName, false) :
        base::UnixExtractFileName(FileName);
      UnicodeString MinimizedName = core::MinimizeName(OnlyFileName, DlgLength - PromptMsg.Length() - 6, false);
      Prompt = FORMAT(PromptMsg.c_str(), MinimizedName.c_str());
    }

    TFarText * Text = new TFarText(this);
    Text->SetCaption(Prompt);

    DirectoryEdit = new TFarEdit(this);
    DirectoryEdit->SetHistory(ToRemote ? REMOTE_DIR_HISTORY : L"Copy");
  }

  TFarSeparator * Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(COPY_PARAM_GROUP));

  CopyParamLister = new TFarLister(this);
  CopyParamLister->SetHeight(3);
  CopyParamLister->SetLeft(GetBorderBox()->GetLeft() + 1);
  CopyParamLister->SetTabStop(false);
  CopyParamLister->SetOnMouseClick(MAKE_CALLBACK(TCopyDialog::CopyParamListerClick, this));

  new TFarSeparator(this);

  if (FLAGCLEAR(FOptions, coTempTransfer))
  {
    NewerOnlyCheck = new TFarCheckBox(this);
    NewerOnlyCheck->SetCaption(GetMsg(TRANSFER_NEWER_ONLY));
    NewerOnlyCheck->SetEnabled(FLAGCLEAR(FOptions, coDisableNewerOnly));

    QueueCheck = new TFarCheckBox(this);
    QueueCheck->SetCaption(GetMsg(TRANSFER_QUEUE));

    SetNextItemPosition(ipRight);

    QueueNoConfirmationCheck = new TFarCheckBox(this);
    QueueNoConfirmationCheck->SetCaption(GetMsg(TRANSFER_QUEUE_NO_CONFIRMATION));
    QueueNoConfirmationCheck->SetEnabledDependency(QueueCheck);

    SetNextItemPosition(ipNewLine);
  }
  else
  {
    DebugAssert(FLAGSET(FOptions, coDisableNewerOnly));
  }

  SaveSettingsCheck = new TFarCheckBox(this);
  SaveSettingsCheck->SetCaption(GetMsg(TRANSFER_REUSE_SETTINGS));

  new TFarSeparator(this);

  TFarButton * Button = new TFarButton(this);
  Button->SetCaption(GetMsg(TRANSFER_SETTINGS_BUTTON));
  Button->SetResult(-1);
  Button->SetCenterGroup(true);
  Button->SetOnClick(MAKE_CALLBACK(TCopyDialog::TransferSettingsButtonClick, this));

  SetNextItemPosition(ipRight);

  Button = new TFarButton(this);
  Button->SetCaption(GetMsg(MSG_BUTTON_OK));
  Button->SetDefault(true);
  Button->SetResult(brOK);
  Button->SetCenterGroup(true);
  Button->SetEnabledDependency(
    ((Options & coTempTransfer) == 0) ? DirectoryEdit : nullptr);

  Button = new TFarButton(this);
  Button->SetCaption(GetMsg(MSG_BUTTON_Cancel));
  Button->SetResult(brCancel);
  Button->SetCenterGroup(true);
}

bool TCopyDialog::Execute(OUT UnicodeString & TargetDirectory,
  OUT TGUICopyParamType * Params)
{
  FCopyParams.Assign(Params);

  if (FLAGCLEAR(FOptions, coTempTransfer))
  {
    NewerOnlyCheck->SetChecked(FLAGCLEAR(FOptions, coDisableNewerOnly) && Params->GetNewerOnly());

    UnicodeString FileMask = Params->GetFileMask();
    UnicodeString Directory = FToRemote ?
      core::UnixIncludeTrailingBackslash(TargetDirectory) :
      ::IncludeTrailingBackslash(TargetDirectory);
    if (FFileList->GetCount() == 1)
    {
      UnicodeString DestFileName = FFileList->GetString(0);
      DestFileName = FToRemote ? DestFileName : FCopyParams.ChangeFileName(DestFileName, osRemote, true);
      FileMask = base::ExtractFileName(DestFileName, false);
    }
    DirectoryEdit->SetText(Directory + FileMask);
    QueueCheck->SetChecked(Params->GetQueue());
    QueueNoConfirmationCheck->SetChecked(Params->GetQueueNoConfirmation());
  }

  bool Result = ShowModal() != brCancel;

  if (Result)
  {
    Params->Assign(&FCopyParams);

    if (FLAGCLEAR(FOptions, coTempTransfer))
    {
      UnicodeString NewTargetDirectory;
      if (FToRemote)
      {
        Params->SetFileMask(base::UnixExtractFileName(DirectoryEdit->GetText()));
        NewTargetDirectory = core::UnixExtractFilePath(DirectoryEdit->GetText());
        if (!NewTargetDirectory.IsEmpty())
          TargetDirectory = NewTargetDirectory;
      }
      else
      {
        Params->SetFileMask(base::ExtractFileName(DirectoryEdit->GetText(), false));
        NewTargetDirectory = ::ExtractFilePath(DirectoryEdit->GetText());
        if (!NewTargetDirectory.IsEmpty())
          TargetDirectory = NewTargetDirectory;
      }

      Params->SetNewerOnly(FLAGCLEAR(FOptions, coDisableNewerOnly) && NewerOnlyCheck->GetChecked());

      Params->SetQueue(QueueCheck->GetChecked());
      Params->SetQueueNoConfirmation(QueueNoConfirmationCheck->GetChecked());
    }

    GetConfiguration()->BeginUpdate();
    SCOPE_EXIT
    {
      GetConfiguration()->EndUpdate();
    };
    if (SaveSettingsCheck->GetChecked())
    {
      GetGUIConfiguration()->SetDefaultCopyParam(*Params);
    }
  }
  return Result;
}

bool TCopyDialog::CloseQuery()
{
  bool CanClose = TFarDialog::CloseQuery();

  if (CanClose && GetResult() >= 0)
  {
    if (!FToRemote && ((FOptions & coTempTransfer) == 0))
    {
      UnicodeString Directory = ::ExtractFilePath(DirectoryEdit->GetText());
      if (!Directory.IsEmpty() && !::DirectoryExists(Directory))
      {
        TWinSCPPlugin * WinSCPPlugin = NB_STATIC_DOWNCAST(TWinSCPPlugin, FarPlugin);

        if (WinSCPPlugin->MoreMessageDialog(FORMAT(GetMsg(CREATE_LOCAL_DIRECTORY).c_str(), Directory.c_str()),
          nullptr, qtConfirmation, qaOK | qaCancel) != qaCancel)
        {
          if (!::ForceDirectories(ApiPath(Directory)))
          {
            DirectoryEdit->SetFocus();
            throw ExtException(FORMAT(GetMsg(CREATE_LOCAL_DIR_ERROR).c_str(), Directory.c_str()));
          }
        }
        else
        {
          DirectoryEdit->SetFocus();
          Abort();
        }
      }
    }
  }
  return CanClose;
}

void TCopyDialog::Change()
{
  TFarDialog::Change();

  if (GetHandle())
  {
    UnicodeString InfoStr = FCopyParams.GetInfoStr(L"; ", FCopyParamAttrs);
    std::unique_ptr<TStrings> InfoStrLines(new TStringList());
    FarWrapText(InfoStr, InfoStrLines.get(), GetBorderBox()->GetWidth() - 4);
    CopyParamLister->SetItems(InfoStrLines.get());
    CopyParamLister->SetRight(GetBorderBox()->GetRight() - (CopyParamLister->GetScrollBar() ? 0 : 1));
  }
}

void TCopyDialog::TransferSettingsButtonClick(
  TFarButton * /*Sender*/, bool & Close)
{
  CustomCopyParam();
  Close = false;
}

void TCopyDialog::CopyParamListerClick(
  TFarDialogItem * /*Item*/, MOUSE_EVENT_RECORD * Event)
{
  if (FLAGSET(Event->dwEventFlags, DOUBLE_CLICK))
  {
    CustomCopyParam();
  }
}

void TCopyDialog::CustomCopyParam()
{
  TWinSCPPlugin * WinSCPPlugin = NB_STATIC_DOWNCAST(TWinSCPPlugin, FarPlugin);
  if (WinSCPPlugin->CopyParamCustomDialog(FCopyParams, FCopyParamAttrs))
  {
    Change();
  }
}

bool TWinSCPFileSystem::CopyDialog(bool ToRemote,
  bool Move, const TStrings * AFileList,
  intptr_t Options,
  intptr_t CopyParamAttrs,
  OUT UnicodeString & TargetDirectory,
  OUT TGUICopyParamType * Params)
{
  std::unique_ptr<TCopyDialog> Dialog(new TCopyDialog(FPlugin, ToRemote,
    Move, AFileList, Options, CopyParamAttrs));
  bool Result = Dialog->Execute(TargetDirectory, Params);
  return Result;
}

bool TWinSCPPlugin::CopyParamDialog(const UnicodeString & Caption,
  TCopyParamType & CopyParam, intptr_t CopyParamAttrs)
{
  std::unique_ptr<TWinSCPDialog> DialogPtr(new TWinSCPDialog(this));
  TWinSCPDialog * Dialog = DialogPtr.get();

  Dialog->SetCaption(Caption);

  // temporary
  Dialog->SetSize(TPoint(78, 10));

  TCopyParamsContainer * CopyParamsContainer = new TCopyParamsContainer(
    Dialog, 0, CopyParamAttrs);

  Dialog->SetSize(TPoint(78, 2 + CopyParamsContainer->GetHeight() + 3));

  Dialog->SetNextItemPosition(ipNewLine);

  Dialog->AddStandardButtons(2, true);

  CopyParamsContainer->SetParams(CopyParam);

  bool Result = (Dialog->ShowModal() == brOK);

  if (Result)
  {
    CopyParam = CopyParamsContainer->GetParams();
  }
  return Result;
}

bool TWinSCPPlugin::CopyParamCustomDialog(TCopyParamType & CopyParam,
  intptr_t CopyParamAttrs)
{
  return CopyParamDialog(GetMsg(COPY_PARAM_CUSTOM_TITLE), CopyParam, CopyParamAttrs);
}

class TLinkDialog : TFarDialog
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  explicit TLinkDialog(TCustomFarPlugin * AFarPlugin,
    bool Edit, bool AllowSymbolic);

  bool Execute(UnicodeString & AFileName, UnicodeString & PointTo,
    bool & Symbolic);

protected:
  virtual void Change();

private:
  TFarEdit * FileNameEdit;
  TFarEdit * PointToEdit;
  TFarCheckBox * SymbolicCheck;
  TFarButton * OkButton;
};

TLinkDialog::TLinkDialog(TCustomFarPlugin * AFarPlugin,
  bool Edit, bool AllowSymbolic) : TFarDialog(AFarPlugin)
{
  TFarButton * Button;
  TFarSeparator * Separator;
  TFarText * Text;

  SetSize(TPoint(76, 12));
  TRect CRect = GetClientRect();

  SetCaption(GetMsg(Edit ? STRING_LINK_EDIT_CAPTION : STRING_LINK_ADD_CAPTION));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(STRING_LINK_FILE));
  Text->SetEnabled(!Edit);

  FileNameEdit = new TFarEdit(this);
  FileNameEdit->SetEnabled(!Edit);
  FileNameEdit->SetHistory(LINK_FILENAME_HISTORY);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(STRING_LINK_POINT_TO));

  PointToEdit = new TFarEdit(this);
  PointToEdit->SetHistory(LINK_POINT_TO_HISTORY);

  new TFarSeparator(this);

  SymbolicCheck = new TFarCheckBox(this);
  SymbolicCheck->SetCaption(GetMsg(STRING_LINK_SYMLINK));
  SymbolicCheck->SetEnabled(AllowSymbolic && !Edit);

  Separator = new TFarSeparator(this);
  Separator->SetPosition(CRect.Bottom - 1);

  OkButton = new TFarButton(this);
  OkButton->SetCaption(GetMsg(MSG_BUTTON_OK));
  OkButton->SetDefault(true);
  OkButton->SetResult(brOK);
  OkButton->SetCenterGroup(true);

  SetNextItemPosition(ipRight);

  Button = new TFarButton(this);
  Button->SetCaption(GetMsg(MSG_BUTTON_Cancel));
  Button->SetResult(brCancel);
  Button->SetCenterGroup(true);
}

void TLinkDialog::Change()
{
  TFarDialog::Change();

  if (GetHandle())
  {
    OkButton->SetEnabled(!FileNameEdit->GetText().IsEmpty() &&
      !PointToEdit->GetText().IsEmpty());
  }
}

bool TLinkDialog::Execute(UnicodeString & AFileName, UnicodeString & PointTo,
  bool & Symbolic)
{
  FileNameEdit->SetText(AFileName);
  PointToEdit->SetText(PointTo);
  SymbolicCheck->SetChecked(Symbolic);

  bool Result = ShowModal() != brCancel;
  if (Result)
  {
    AFileName = FileNameEdit->GetText();
    PointTo = PointToEdit->GetText();
    Symbolic = SymbolicCheck->GetChecked();
  }
  return Result;
}

bool TWinSCPFileSystem::LinkDialog(UnicodeString & AFileName,
  UnicodeString & PointTo, bool & Symbolic, bool Edit, bool AllowSymbolic)
{
  std::unique_ptr<TLinkDialog> Dialog(new TLinkDialog(FPlugin, Edit, AllowSymbolic));
  bool Result = Dialog->Execute(AFileName, PointTo, Symbolic);
  return Result;
}

DEFINE_CALLBACK_TYPE3(TFeedFileSystemDataEvent, void,
  TObject * /*Control*/, int /*Label*/, const UnicodeString & /*Value*/);

class TLabelList;
class TFileSystemInfoDialog : TTabbedDialog
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  enum
  {
    tabProtocol = 1,
    tabCapabilities,
    tabSpaceAvailable,
    tabCount
  };

  explicit TFileSystemInfoDialog(TCustomFarPlugin * AFarPlugin,
    TGetSpaceAvailableEvent OnGetSpaceAvailable);
  virtual ~TFileSystemInfoDialog();
  void Execute(const TSessionInfo & SessionInfo,
    const TFileSystemInfo & FileSystemInfo, const UnicodeString & SpaceAvailablePath);

protected:
  void Feed(TFeedFileSystemDataEvent AddItem);
  UnicodeString CapabilityStr(TFSCapability Capability);
  UnicodeString CapabilityStr(TFSCapability Capability1,
    TFSCapability Capability2);
  UnicodeString SpaceStr(int64_t Bytes);
  void ControlsAddItem(TObject * Control, int Label, const UnicodeString & Value);
  void CalculateMaxLenAddItem(TObject * Control, int Label, const UnicodeString & Value);
  void ClipboardAddItem(TObject * Control, int Label, const UnicodeString & Value);
  void FeedControls();
  void UpdateControls();
  TLabelList * CreateLabelArray(intptr_t Count);
  virtual void SelectTab(intptr_t Tab);
  virtual void Change();
  void SpaceAvailableButtonClick(TFarButton * Sender, bool & Close);
  void ClipboardButtonClick(TFarButton * Sender, bool & Close);
  void CheckSpaceAvailable();
  void NeedSpaceAvailable();
  bool SpaceAvailableSupported();
  virtual bool Key(TFarDialogItem * Item, LONG_PTR KeyCode);

private:
  TGetSpaceAvailableEvent FOnGetSpaceAvailable;
  TFileSystemInfo FFileSystemInfo;
  TSessionInfo FSessionInfo;
  bool FSpaceAvailableLoaded;
  TSpaceAvailable FSpaceAvailable;
  TObject * FLastFeededControl;
  intptr_t FLastListItem;
  UnicodeString FClipboard;

  TLabelList * ServerLabels;
  TLabelList * ProtocolLabels;
  TLabelList * SpaceAvailableLabels;
  TTabButton * SpaceAvailableTab;
  TFarText * HostKeyFingerprintLabel;
  TFarEdit * HostKeyFingerprintEdit;
  TFarText * InfoLabel;
  TFarSeparator * InfoSeparator;
  TFarLister * InfoLister;
  TFarEdit * SpaceAvailablePathEdit;
  TFarButton * OkButton;
};

class TLabelList : public TList
{
NB_DECLARE_CLASS(TLabelList)
public:
  explicit TLabelList() :
    TList(), MaxLen(0)
  {
  }

  intptr_t MaxLen;
};

TFileSystemInfoDialog::TFileSystemInfoDialog(TCustomFarPlugin * AFarPlugin,
    TGetSpaceAvailableEvent OnGetSpaceAvailable) : TTabbedDialog(AFarPlugin, tabCount),
  FSpaceAvailableLoaded(false),
  FLastFeededControl(nullptr),
  FLastListItem(0),
  InfoLabel(nullptr)
{
  FOnGetSpaceAvailable = OnGetSpaceAvailable;
  TFarText * Text;
  TFarSeparator * Separator;
  TFarButton * Button;
  TTabButton * Tab;
  intptr_t GroupTop;

  SetSize(TPoint(73, 22));
  SetCaption(GetMsg(SERVER_PROTOCOL_INFORMATION));

  Tab = new TTabButton(this);
  Tab->SetTabName(GetMsg(SERVER_PROTOCOL_TAB_PROTOCOL));
  Tab->SetTab(tabProtocol);

  SetNextItemPosition(ipRight);

  Tab = new TTabButton(this);
  Tab->SetTabName(GetMsg(SERVER_PROTOCOL_TAB_CAPABILITIES));
  Tab->SetTab(tabCapabilities);

  SpaceAvailableTab = new TTabButton(this);
  SpaceAvailableTab->SetTabName(GetMsg(SERVER_PROTOCOL_TAB_SPACE_AVAILABLE));
  SpaceAvailableTab->SetTab(tabSpaceAvailable);

  // Server tab

  SetNextItemPosition(ipNewLine);
  SetDefaultGroup(tabProtocol);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(SERVER_INFORMATION_GROUP));
  GroupTop = Separator->GetTop();

  ServerLabels = CreateLabelArray(5);

  new TFarSeparator(this);

  HostKeyFingerprintLabel = new TFarText(this);
  HostKeyFingerprintLabel->SetCaption(GetMsg(SERVER_HOST_KEY));
  HostKeyFingerprintEdit = new TFarEdit(this);
  HostKeyFingerprintEdit->SetReadOnly(true);

  // Protocol tab

  SetDefaultGroup(tabCapabilities);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(PROTOCOL_INFORMATION_GROUP));
  Separator->SetPosition(GroupTop);

  ProtocolLabels = CreateLabelArray(9);

  InfoSeparator = new TFarSeparator(this);
  InfoSeparator->SetCaption(GetMsg(PROTOCOL_INFO_GROUP));

  InfoLister = new TFarLister(this);
  InfoLister->SetHeight(4);
  InfoLister->SetLeft(GetBorderBox()->GetLeft() + 1);
  // Right edge is adjusted in FeedControls

  // Space available tab

  SetDefaultGroup(tabSpaceAvailable);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(SPACE_AVAILABLE_GROUP));
  Separator->SetPosition(GroupTop);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(SPACE_AVAILABLE_PATH));

  SetNextItemPosition(ipRight);

  SpaceAvailablePathEdit = new TFarEdit(this);
  SpaceAvailablePathEdit->SetRight(
    - (static_cast<int>(GetMsg(SPACE_AVAILABLE_CHECK_SPACE).Length() + 11)));

  Button = new TFarButton(this);
  Button->SetCaption(GetMsg(SPACE_AVAILABLE_CHECK_SPACE));
  Button->SetEnabledDependency(SpaceAvailablePathEdit);
  Button->SetOnClick(MAKE_CALLBACK(TFileSystemInfoDialog::SpaceAvailableButtonClick, this));

  SetNextItemPosition(ipNewLine);

  new TFarSeparator(this);

  SpaceAvailableLabels = CreateLabelArray(5);

  // Buttons

  SetDefaultGroup(0);

  Separator = new TFarSeparator(this);
  Separator->SetPosition(GetClientRect().Bottom - 1);

  Button = new TFarButton(this);
  Button->SetCaption(GetMsg(SERVER_PROTOCOL_COPY_CLIPBOARD));
  Button->SetOnClick(MAKE_CALLBACK(TFileSystemInfoDialog::ClipboardButtonClick, this));
  Button->SetCenterGroup(true);

  SetNextItemPosition(ipRight);

  OkButton = new TFarButton(this);
  OkButton->SetCaption(GetMsg(MSG_BUTTON_OK));
  OkButton->SetDefault(true);
  OkButton->SetResult(brOK);
  OkButton->SetCenterGroup(true);
}

TFileSystemInfoDialog::~TFileSystemInfoDialog()
{
  SAFE_DESTROY(ServerLabels);
  SAFE_DESTROY(ProtocolLabels);
  SAFE_DESTROY(SpaceAvailableLabels);
}

TLabelList * TFileSystemInfoDialog::CreateLabelArray(intptr_t Count)
{
  std::unique_ptr<TLabelList> List(new TLabelList());
  for (intptr_t Index = 0; Index < Count; ++Index)
  {
    List->Add(new TFarText(this));
  }
  return List.release();
}

UnicodeString TFileSystemInfoDialog::CapabilityStr(TFSCapability Capability)
{
  return BooleanToStr(FFileSystemInfo.IsCapable[Capability]);
}

UnicodeString TFileSystemInfoDialog::CapabilityStr(TFSCapability Capability1,
    TFSCapability Capability2)
{
  return FORMAT(L"%s/%s", CapabilityStr(Capability1).c_str(), CapabilityStr(Capability2).c_str());
}

UnicodeString TFileSystemInfoDialog::SpaceStr(int64_t Bytes)
{
  UnicodeString Result;
  if (Bytes == 0)
  {
    Result = GetMsg(SPACE_AVAILABLE_BYTES_UNKNOWN);
  }
  else
  {
    Result = FormatBytes(Bytes);
    UnicodeString SizeUnorderedStr = FormatBytes(Bytes, false);
    if (Result != SizeUnorderedStr)
    {
      Result = FORMAT(L"%s (%s)", Result.c_str(), SizeUnorderedStr.c_str());
    }
  }
  return Result;
}

void TFileSystemInfoDialog::Feed(TFeedFileSystemDataEvent AddItem)
{
  AddItem(ServerLabels, SERVER_REMOTE_SYSTEM, FFileSystemInfo.RemoteSystem);
  AddItem(ServerLabels, SERVER_SESSION_PROTOCOL, FSessionInfo.ProtocolName);
  AddItem(ServerLabels, SERVER_SSH_IMPLEMENTATION, FSessionInfo.SshImplementation);

  UnicodeString Str = FSessionInfo.CSCipher;
  if (FSessionInfo.CSCipher != FSessionInfo.SCCipher)
  {
    Str += FORMAT(L"/%s", FSessionInfo.SCCipher.c_str());
  }
  AddItem(ServerLabels, SERVER_CIPHER, Str);

  Str = DefaultStr(FSessionInfo.CSCompression, LoadStr(NO_STR));
  if (FSessionInfo.CSCompression != FSessionInfo.SCCompression)
  {
    Str += FORMAT(L"/%s", DefaultStr(FSessionInfo.SCCompression, LoadStr(NO_STR)).c_str());
  }
  AddItem(ServerLabels, SERVER_COMPRESSION, Str);
  if (FSessionInfo.ProtocolName != FFileSystemInfo.ProtocolName)
  {
    AddItem(ServerLabels, SERVER_FS_PROTOCOL, FFileSystemInfo.ProtocolName);
  }

  AddItem(HostKeyFingerprintEdit, 0, FSessionInfo.HostKeyFingerprint);

  AddItem(ProtocolLabels, PROTOCOL_MODE_CHANGING, CapabilityStr(fcModeChanging));
  AddItem(ProtocolLabels, PROTOCOL_OWNER_GROUP_CHANGING, CapabilityStr(fcGroupChanging));
  UnicodeString AnyCommand;
  if (!FFileSystemInfo.IsCapable[fcShellAnyCommand] &&
      FFileSystemInfo.IsCapable[fcAnyCommand])
  {
    AnyCommand = GetMsg(PROTOCOL_PROTOCOL_ANY_COMMAND);
  }
  else
  {
    AnyCommand = CapabilityStr(fcAnyCommand);
  }
  AddItem(ProtocolLabels, PROTOCOL_ANY_COMMAND, AnyCommand);
  AddItem(ProtocolLabels, PROTOCOL_SYMBOLIC_HARD_LINK, CapabilityStr(fcSymbolicLink, fcHardLink));
  AddItem(ProtocolLabels, PROTOCOL_USER_GROUP_LISTING, CapabilityStr(fcUserGroupListing));
  AddItem(ProtocolLabels, PROTOCOL_REMOTE_COPY, CapabilityStr(fcRemoteCopy));
  AddItem(ProtocolLabels, PROTOCOL_CHECKING_SPACE_AVAILABLE, CapabilityStr(fcCheckingSpaceAvailable));
  AddItem(ProtocolLabels, PROTOCOL_CALCULATING_CHECKSUM, CapabilityStr(fcCalculatingChecksum));
  AddItem(ProtocolLabels, PROTOCOL_NATIVE_TEXT_MODE, CapabilityStr(fcNativeTextMode));

  AddItem(InfoLister, 0, FFileSystemInfo.AdditionalInfo);

  AddItem(SpaceAvailableLabels, SPACE_AVAILABLE_BYTES_ON_DEVICE, SpaceStr(FSpaceAvailable.BytesOnDevice));
  AddItem(SpaceAvailableLabels, SPACE_AVAILABLE_UNUSED_BYTES_ON_DEVICE, SpaceStr(FSpaceAvailable.UnusedBytesOnDevice));
  AddItem(SpaceAvailableLabels, SPACE_AVAILABLE_BYTES_AVAILABLE_TO_USER, SpaceStr(FSpaceAvailable.BytesAvailableToUser));
  AddItem(SpaceAvailableLabels, SPACE_AVAILABLE_UNUSED_BYTES_AVAILABLE_TO_USER, SpaceStr(FSpaceAvailable.UnusedBytesAvailableToUser));
  AddItem(SpaceAvailableLabels, SPACE_AVAILABLE_BYTES_PER_ALLOCATION_UNIT, SpaceStr(FSpaceAvailable.BytesPerAllocationUnit));
}

void TFileSystemInfoDialog::ControlsAddItem(TObject * Control,
  int Label, const UnicodeString & Value)
{
  if (FLastFeededControl != Control)
  {
    FLastFeededControl = Control;
    FLastListItem = 0;
  }

  if (Control == HostKeyFingerprintEdit)
  {
    HostKeyFingerprintEdit->SetText(Value);
    HostKeyFingerprintEdit->SetEnabled(!Value.IsEmpty());
    if (!HostKeyFingerprintEdit->GetEnabled())
    {
      HostKeyFingerprintEdit->SetVisible(false);
      HostKeyFingerprintEdit->SetGroup(0);
      HostKeyFingerprintLabel->SetVisible(false);
      HostKeyFingerprintLabel->SetGroup(0);
    }
  }
  else if (Control == InfoLister)
  {
    InfoLister->GetItems()->SetText(Value);
    InfoLister->SetEnabled(!Value.IsEmpty());
    if (!InfoLister->GetEnabled())
    {
      InfoLister->SetVisible(false);
      InfoLister->SetGroup(0);
      InfoSeparator->SetVisible(false);
      InfoSeparator->SetGroup(0);
    }
  }
  else
  {
    TLabelList * List = NB_STATIC_DOWNCAST(TLabelList, Control);
    DebugAssert(List != nullptr);
    if (!Value.IsEmpty())
    {
      TFarText * Text = NB_STATIC_DOWNCAST(TFarText, List->GetItem(FLastListItem));
      FLastListItem++;

      Text->SetCaption(FORMAT(L"%d-%s  %s", List->MaxLen, GetMsg(Label).c_str(), Value.c_str()));
    }
  }
}

void TFileSystemInfoDialog::CalculateMaxLenAddItem(TObject * Control,
  int Label, const UnicodeString & )
{
  TLabelList * List = NB_STATIC_DOWNCAST(TLabelList, Control);
  if (List != nullptr)
  {
    UnicodeString S = GetMsg(Label);
    if (List->MaxLen < S.Length())
    {
      List->MaxLen = S.Length();
    }
  }
}

void TFileSystemInfoDialog::ClipboardAddItem(TObject * AControl,
  int Label, const UnicodeString & Value)
{
  TFarDialogItem * Control = NB_STATIC_DOWNCAST(TFarDialogItem, AControl);
  // check for Enabled instead of Visible, as Visible is false
  // when control is on non-active tab
  if ((!Value.IsEmpty() &&
      ((Control == nullptr) || Control->GetEnabled()) &&
      (AControl != SpaceAvailableLabels)) ||
       SpaceAvailableSupported())
  {
    if (FLastFeededControl != AControl)
    {
      if (FLastFeededControl != nullptr)
      {
        FClipboard += ::StringOfChar('-', 60) + L"\r\n";
      }
      FLastFeededControl = AControl;
    }

    if (NB_STATIC_DOWNCAST(TLabelList, AControl) == nullptr)
    {
      UnicodeString LabelStr;
      if (Control == HostKeyFingerprintEdit)
      {
        LabelStr = GetMsg(SERVER_HOST_KEY);
      }
      else if (Control == InfoLister)
      {
        LabelStr = ::Trim(GetMsg(PROTOCOL_INFO_GROUP));
      }
      else
      {
        DebugAssert(false);
      }

      if (!LabelStr.IsEmpty() && (LabelStr[LabelStr.Length()] == L':'))
      {
        LabelStr.SetLength(LabelStr.Length() - 1);
      }

      UnicodeString Value2 = Value;
      if ((Value2.Length() >= 2) && (Value2.SubString(Value2.Length() - 1, 2) == L"\r\n"))
      {
        Value2.SetLength(Value2.Length() - 2);
      }

      FClipboard += FORMAT(L"%s\r\n%s\r\n", LabelStr.c_str(), Value2.c_str());
    }
    else
    {
      DebugAssert(NB_STATIC_DOWNCAST(TLabelList, AControl) != nullptr);
      UnicodeString LabelStr = GetMsg(Label);
      if (!LabelStr.IsEmpty() && (LabelStr[LabelStr.Length()] == L':'))
      {
        LabelStr.SetLength(LabelStr.Length() - 1);
      }
      FClipboard += FORMAT(L"%s = %s\r\n", LabelStr.c_str(), Value.c_str());
    }
  }
}

void TFileSystemInfoDialog::FeedControls()
{
  FLastFeededControl = nullptr;
  Feed(MAKE_CALLBACK(TFileSystemInfoDialog::ControlsAddItem, this));
  InfoLister->SetRight(GetBorderBox()->GetRight() - (InfoLister->GetScrollBar() ? 0 : 1));
}

void TFileSystemInfoDialog::SelectTab(intptr_t Tab)
{
  TTabbedDialog::SelectTab(Tab);
  if (InfoLister->GetVisible())
  {
    // At first the dialog border box hides the eventual scrollbar of infolister,
    // so redraw to reshow it.
    Redraw();
  }

  if (Tab == tabSpaceAvailable)
  {
    NeedSpaceAvailable();
  }
}

void TFileSystemInfoDialog::Execute(
  const TSessionInfo & SessionInfo, const TFileSystemInfo & FileSystemInfo,
  const UnicodeString & SpaceAvailablePath)
{
  FFileSystemInfo = FileSystemInfo;
  FSessionInfo = SessionInfo;
  SpaceAvailablePathEdit->SetText(SpaceAvailablePath);
  UpdateControls();

  Feed(MAKE_CALLBACK(TFileSystemInfoDialog::CalculateMaxLenAddItem, this));
  FeedControls();
  HideTabs();
  SelectTab(tabProtocol);

  ShowModal();
}

bool TFileSystemInfoDialog::Key(TFarDialogItem * Item, LONG_PTR KeyCode)
{
  bool Result = false;
  if ((Item == SpaceAvailablePathEdit) && (KeyCode == KEY_ENTER))
  {
    CheckSpaceAvailable();
    Result = true;
  }
  else
  {
    Result = TTabbedDialog::Key(Item, KeyCode);
  }
  return Result;
}

void TFileSystemInfoDialog::Change()
{
  TTabbedDialog::Change();

  if (GetHandle())
  {
    UpdateControls();
  }
}

void TFileSystemInfoDialog::UpdateControls()
{
  SpaceAvailableTab->SetEnabled(SpaceAvailableSupported());
}

void TFileSystemInfoDialog::ClipboardButtonClick(TFarButton * /*Sender*/,
  bool & Close)
{
  NeedSpaceAvailable();
  FLastFeededControl = nullptr;
  FClipboard.Clear();
  Feed(MAKE_CALLBACK(TFileSystemInfoDialog::ClipboardAddItem, this));
  FarPlugin->FarCopyToClipboard(FClipboard);
  Close = false;
}

void TFileSystemInfoDialog::SpaceAvailableButtonClick(
  TFarButton * /*Sender*/, bool & Close)
{
  CheckSpaceAvailable();
  Close = false;
}

void TFileSystemInfoDialog::CheckSpaceAvailable()
{
  DebugAssert(FOnGetSpaceAvailable);
  DebugAssert(!SpaceAvailablePathEdit->GetText().IsEmpty());

  FSpaceAvailableLoaded = true;

  bool DoClose = false;

  FOnGetSpaceAvailable(SpaceAvailablePathEdit->GetText(), FSpaceAvailable, DoClose);

  FeedControls();
  if (DoClose)
  {
    Close(OkButton);
  }
}

void TFileSystemInfoDialog::NeedSpaceAvailable()
{
  if (!FSpaceAvailableLoaded && SpaceAvailableSupported())
  {
    CheckSpaceAvailable();
  }
}

bool TFileSystemInfoDialog::SpaceAvailableSupported()
{
  return (FOnGetSpaceAvailable);
}

void TWinSCPFileSystem::FileSystemInfoDialog(
  const TSessionInfo & SessionInfo, const TFileSystemInfo & FileSystemInfo,
  const UnicodeString & SpaceAvailablePath, TGetSpaceAvailableEvent OnGetSpaceAvailable)
{
  std::unique_ptr<TFileSystemInfoDialog> Dialog(new TFileSystemInfoDialog(FPlugin, OnGetSpaceAvailable));
  Dialog->Execute(SessionInfo, FileSystemInfo, SpaceAvailablePath);
}

bool TWinSCPFileSystem::OpenDirectoryDialog(
  bool Add, UnicodeString & Directory, TBookmarkList * BookmarkList)
{
  bool Result = false;
  bool Repeat = false;

  intptr_t ItemFocused = -1;

  do
  {
    std::unique_ptr<TStrings> BookmarkPaths(new TStringList());
    std::unique_ptr<TFarMenuItems> BookmarkItems(new TFarMenuItems());
    std::unique_ptr<TList> Bookmarks(new TList());
    intptr_t BookmarksOffset = -1;

    intptr_t MaxLength = FPlugin->MaxMenuItemLength();
    intptr_t MaxHistory = 40;
    intptr_t FirstHistory = 0;

    if (FPathHistory->GetCount() > MaxHistory)
    {
      FirstHistory = FPathHistory->GetCount() - MaxHistory + 1;
    }

    for (intptr_t Index = FirstHistory; Index < FPathHistory->GetCount(); ++Index)
    {
      UnicodeString Path = FPathHistory->GetString(Index);
      BookmarkPaths->Add(Path);
      BookmarkItems->Add(core::MinimizeName(Path, MaxLength, true));
    }

    intptr_t FirstItemFocused = -1;
    std::unique_ptr<TStringList> BookmarkDirectories(new TStringList());
    BookmarkDirectories->SetSorted(true);
    for (intptr_t Index = 0; Index < BookmarkList->GetCount(); ++Index)
    {
      TBookmark * Bookmark = BookmarkList->GetBookmarks(Index);
      UnicodeString RemoteDirectory = Bookmark->GetRemote();
      if (!RemoteDirectory.IsEmpty() && (BookmarkDirectories->IndexOf(RemoteDirectory.c_str()) == NPOS))
      {
        intptr_t Pos = BookmarkDirectories->Add(RemoteDirectory);
        if (RemoteDirectory == Directory)
        {
          FirstItemFocused = Pos;
        }
        else if ((FirstItemFocused >= 0) && (FirstItemFocused >= Pos))
        {
          FirstItemFocused++;
        }
        Bookmarks->Insert(Pos, Bookmark);
      }
    }

    if (BookmarkDirectories->GetCount() == 0)
    {
      FirstItemFocused = BookmarkItems->Add(L"");
      BookmarkPaths->Add(L"");
      BookmarksOffset = BookmarkItems->GetCount();
    }
    else
    {
      if (BookmarkItems->GetCount() > 0)
      {
        BookmarkItems->AddSeparator();
        BookmarkPaths->Add(L"");
      }

      BookmarksOffset = BookmarkItems->GetCount();

      if (FirstItemFocused >= 0)
      {
        FirstItemFocused += BookmarkItems->GetCount();
      }
      else
      {
        FirstItemFocused = BookmarkItems->GetCount();
      }

      for (intptr_t II = 0; II < BookmarkDirectories->GetCount(); II++)
      {
        UnicodeString Path = BookmarkDirectories->GetString(II);
        BookmarkItems->Add(Path);
        BookmarkPaths->Add(core::MinimizeName(Path, MaxLength, true));
      }
    }

    if (ItemFocused < 0)
    {
      BookmarkItems->SetItemFocused(FirstItemFocused);
    }
    else if (ItemFocused < BookmarkItems->GetCount())
    {
      BookmarkItems->SetItemFocused(ItemFocused);
    }
    else
    {
      BookmarkItems->SetItemFocused(BookmarkItems->GetCount() - 1);
    }

    int BreakCode;

    Repeat = false;
    UnicodeString Caption = GetMsg(Add ? OPEN_DIRECTORY_ADD_BOOMARK_ACTION :
      OPEN_DIRECTORY_BROWSE_CAPTION);
    const int BreakKeys[] =
    {
      VK_DELETE,
      VK_F8,
      VK_RETURN + (PKF_CONTROL << 16),
      'C' + (PKF_CONTROL << 16),
      VK_INSERT + (PKF_CONTROL << 16),
      0
    };

    ItemFocused = FPlugin->Menu(FMENU_REVERSEAUTOHIGHLIGHT | FMENU_SHOWAMPERSAND | FMENU_WRAPMODE,
      Caption, GetMsg(OPEN_DIRECTORY_HELP), BookmarkItems.get(), BreakKeys, BreakCode);
    if (BreakCode >= 0)
    {
      DebugAssert(BreakCode >= 0 && BreakCode <= 4);
      if ((BreakCode == 0) || (BreakCode == 1))
      {
        DebugAssert(ItemFocused >= 0);
        if (ItemFocused >= BookmarksOffset)
        {
          TBookmark * Bookmark = NB_STATIC_DOWNCAST(TBookmark, Bookmarks->GetItem(ItemFocused - BookmarksOffset));
          BookmarkList->Delete(Bookmark);
        }
        else
        {
          FPathHistory->Clear();
          ItemFocused = -1;
        }
        Repeat = true;
      }
      else if (BreakCode == 2)
      {
        FarControl(FCTL_INSERTCMDLINE, 0, reinterpret_cast<intptr_t>(BookmarkPaths->GetString(ItemFocused).c_str()));
      }
      else if (BreakCode == 3 || BreakCode == 4)
      {
        FPlugin->FarCopyToClipboard(BookmarkPaths->GetString(ItemFocused));
        Repeat = true;
      }
    }
    else if (ItemFocused >= 0)
    {
      Directory = BookmarkPaths->GetString(ItemFocused);
      if (Directory.IsEmpty())
      {
        // empty trailing line in no-bookmark mode selected
        ItemFocused = -1;
      }
    }

    Result = (BreakCode < 0) && (ItemFocused >= 0);
  }
  while (Repeat);

  return Result;
}

class TApplyCommandDialog : public TWinSCPDialog
{
public:
  explicit TApplyCommandDialog(TCustomFarPlugin * AFarPlugin);

  bool Execute(UnicodeString & Command, intptr_t & Params);

protected:
  virtual void Change();

private:
  intptr_t FParams;

  TFarEdit * CommandEdit;
  TFarText * LocalHintText;
  TFarRadioButton * RemoteCommandButton;
  TFarRadioButton * LocalCommandButton;
  TFarCheckBox * ApplyToDirectoriesCheck;
  TFarCheckBox * RecursiveCheck;
  TFarCheckBox * ShowResultsCheck;
  TFarCheckBox * CopyResultsCheck;

  UnicodeString FPrompt;
  TFarEdit * PasswordEdit;
  TFarEdit * NormalEdit;
  TFarCheckBox * HideTypingCheck;
};

TApplyCommandDialog::TApplyCommandDialog(TCustomFarPlugin * AFarPlugin) :
  TWinSCPDialog(AFarPlugin),
  FParams(0),
  PasswordEdit(nullptr),
  NormalEdit(nullptr),
  HideTypingCheck(nullptr)
{
  TFarText * Text;

  SetSize(TPoint(76, 18));
  SetCaption(GetMsg(APPLY_COMMAND_TITLE));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(APPLY_COMMAND_PROMPT));

  CommandEdit = new TFarEdit(this);
  CommandEdit->SetHistory(APPLY_COMMAND_HISTORY);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(APPLY_COMMAND_HINT1));
  Text = new TFarText(this);
  Text->SetCaption(GetMsg(APPLY_COMMAND_HINT2));
  Text = new TFarText(this);
  Text->SetCaption(GetMsg(APPLY_COMMAND_HINT3));
  Text = new TFarText(this);
  Text->SetCaption(GetMsg(APPLY_COMMAND_HINT4));
  Text = new TFarText(this);
  Text->SetCaption(GetMsg(APPLY_COMMAND_HINT5));
  LocalHintText = new TFarText(this);
  LocalHintText->SetCaption(GetMsg(APPLY_COMMAND_HINT_LOCAL));

  new TFarSeparator(this);

  RemoteCommandButton = new TFarRadioButton(this);
  RemoteCommandButton->SetCaption(GetMsg(APPLY_COMMAND_REMOTE_COMMAND));

  SetNextItemPosition(ipRight);

  LocalCommandButton = new TFarRadioButton(this);
  LocalCommandButton->SetCaption(GetMsg(APPLY_COMMAND_LOCAL_COMMAND));

  LocalHintText->SetEnabledDependency(LocalCommandButton);

  SetNextItemPosition(ipNewLine);

  ApplyToDirectoriesCheck = new TFarCheckBox(this);
  ApplyToDirectoriesCheck->SetCaption(
    GetMsg(APPLY_COMMAND_APPLY_TO_DIRECTORIES));

  SetNextItemPosition(ipRight);

  RecursiveCheck = new TFarCheckBox(this);
  RecursiveCheck->SetCaption(GetMsg(APPLY_COMMAND_RECURSIVE));

  SetNextItemPosition(ipNewLine);

  ShowResultsCheck = new TFarCheckBox(this);
  ShowResultsCheck->SetCaption(GetMsg(APPLY_COMMAND_SHOW_RESULTS));
  ShowResultsCheck->SetEnabledDependency(RemoteCommandButton);

  SetNextItemPosition(ipRight);

  CopyResultsCheck = new TFarCheckBox(this);
  CopyResultsCheck->SetCaption(GetMsg(APPLY_COMMAND_COPY_RESULTS));
  CopyResultsCheck->SetEnabledDependency(RemoteCommandButton);

  AddStandardButtons();

  OkButton->SetEnabledDependency(CommandEdit);
}

void TApplyCommandDialog::Change()
{
  TWinSCPDialog::Change();

  if (GetHandle())
  {
    bool RemoteCommand = RemoteCommandButton->GetChecked();
    bool AllowRecursive = true;
    bool AllowApplyToDirectories = true;
    try
    {
      TRemoteCustomCommand RemoteCustomCommand;
      TLocalCustomCommand LocalCustomCommand;
      TFileCustomCommand * FileCustomCommand =
        (RemoteCommand ? &RemoteCustomCommand : &LocalCustomCommand);

      TInteractiveCustomCommand InteractiveCustomCommand(FileCustomCommand);
      UnicodeString Cmd = InteractiveCustomCommand.Complete(CommandEdit->GetText(), false);
      bool FileCommand = FileCustomCommand->IsFileCommand(Cmd);
      AllowRecursive = FileCommand && !FileCustomCommand->IsFileListCommand(Cmd);
      if (AllowRecursive && !RemoteCommand)
      {
        AllowRecursive = !LocalCustomCommand.HasLocalFileName(Cmd);
      }
      AllowApplyToDirectories = FileCommand;
    }
    catch (...)
    {
    }

    RecursiveCheck->SetEnabled(AllowRecursive);
    ApplyToDirectoriesCheck->SetEnabled(AllowApplyToDirectories);
  }
}

bool TApplyCommandDialog::Execute(UnicodeString & Command, intptr_t & Params)
{
  CommandEdit->SetText(Command);
  FParams = Params;
  RemoteCommandButton->SetChecked(FLAGCLEAR(Params, ccLocal));
  LocalCommandButton->SetChecked(FLAGSET(Params, ccLocal));
  ApplyToDirectoriesCheck->SetChecked(FLAGSET(Params, ccApplyToDirectories));
  RecursiveCheck->SetChecked(FLAGSET(Params, ccRecursive));
  ShowResultsCheck->SetChecked(FLAGSET(Params, ccShowResults));
  CopyResultsCheck->SetChecked(FLAGSET(Params, ccCopyResults));

  bool Result = (ShowModal() != brCancel);
  if (Result)
  {
    Command = CommandEdit->GetText();
    Params &= ~(ccLocal | ccApplyToDirectories | ccRecursive | ccShowResults | ccCopyResults);
    Params |=
      FLAGMASK(!RemoteCommandButton->GetChecked(), ccLocal) |
      FLAGMASK(ApplyToDirectoriesCheck->GetChecked(), ccApplyToDirectories) |
      FLAGMASK(RecursiveCheck->GetChecked() && RecursiveCheck->GetEnabled(), ccRecursive) |
      FLAGMASK(ShowResultsCheck->GetChecked() && ShowResultsCheck->GetEnabled(), ccShowResults) |
      FLAGMASK(CopyResultsCheck->GetChecked() && CopyResultsCheck->GetEnabled(), ccCopyResults);
  }
  return Result;
}

bool TWinSCPFileSystem::ApplyCommandDialog(UnicodeString & Command,
  intptr_t & Params)
{
  std::unique_ptr<TApplyCommandDialog> Dialog(new TApplyCommandDialog(FPlugin));
  bool Result = Dialog->Execute(Command, Params);
  return Result;
}

class TFullSynchronizeDialog : public TWinSCPDialog
{
public:
  explicit TFullSynchronizeDialog(TCustomFarPlugin * AFarPlugin, intptr_t Options,
    const TUsableCopyParamAttrs & CopyParamAttrs);

  bool Execute(TTerminal::TSynchronizeMode & Mode,
    intptr_t & Params, UnicodeString & LocalDirectory, UnicodeString & RemoteDirectory,
    TCopyParamType * CopyParams, bool & SaveSettings, bool & SaveMode);

protected:
  virtual bool CloseQuery();
  virtual void Change();
  virtual LONG_PTR DialogProc(int Msg, intptr_t Param1, LONG_PTR Param2);

  void TransferSettingsButtonClick(TFarButton * Sender, bool & Close);
  void CopyParamListerClick(TFarDialogItem * Item, MOUSE_EVENT_RECORD * Event);

  intptr_t ActualCopyParamAttrs();
  void CustomCopyParam();
  void AdaptSize();

private:
  TFarEdit * LocalDirectoryEdit;
  TFarEdit * RemoteDirectoryEdit;
  TFarRadioButton * SynchronizeBothButton;
  TFarRadioButton * SynchronizeRemoteButton;
  TFarRadioButton * SynchronizeLocalButton;
  TFarRadioButton * SynchronizeFilesButton;
  TFarRadioButton * MirrorFilesButton;
  TFarRadioButton * SynchronizeTimestampsButton;
  TFarCheckBox * SynchronizeDeleteCheck;
  TFarCheckBox * SynchronizeExistingOnlyCheck;
  TFarCheckBox * SynchronizeSelectedOnlyCheck;
  TFarCheckBox * SynchronizePreviewChangesCheck;
  TFarCheckBox * SynchronizeByTimeCheck;
  TFarCheckBox * SynchronizeBySizeCheck;
  TFarCheckBox * SaveSettingsCheck;
  TFarLister * CopyParamLister;

  bool FSaveMode;
  intptr_t FOptions;
  intptr_t FFullHeight;
  TTerminal::TSynchronizeMode FOrigMode;
  TUsableCopyParamAttrs FCopyParamAttrs;
  TCopyParamType FCopyParams;

  TTerminal::TSynchronizeMode GetMode() const;
};

TFullSynchronizeDialog::TFullSynchronizeDialog(
  TCustomFarPlugin * AFarPlugin, intptr_t Options,
  const TUsableCopyParamAttrs & CopyParamAttrs) :
  TWinSCPDialog(AFarPlugin),
  FOptions(Options),
  FCopyParamAttrs(CopyParamAttrs)
{
  TFarText * Text;
  TFarSeparator * Separator;

  SetSize(TPoint(78, 25));
  SetCaption(GetMsg(FULL_SYNCHRONIZE_TITLE));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(FULL_SYNCHRONIZE_LOCAL_LABEL));

  LocalDirectoryEdit = new TFarEdit(this);
  LocalDirectoryEdit->SetHistory(LOCAL_SYNC_HISTORY);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(FULL_SYNCHRONIZE_REMOTE_LABEL));

  RemoteDirectoryEdit = new TFarEdit(this);
  RemoteDirectoryEdit->SetHistory(REMOTE_SYNC_HISTORY);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(FULL_SYNCHRONIZE_DIRECTION_GROUP));

  SynchronizeBothButton = new TFarRadioButton(this);
  SynchronizeBothButton->SetCaption(GetMsg(FULL_SYNCHRONIZE_BOTH));

  SetNextItemPosition(ipRight);

  SynchronizeRemoteButton = new TFarRadioButton(this);
  SynchronizeRemoteButton->SetCaption(GetMsg(FULL_SYNCHRONIZE_REMOTE));

  SynchronizeLocalButton = new TFarRadioButton(this);
  SynchronizeLocalButton->SetCaption(GetMsg(FULL_SYNCHRONIZE_LOCAL));

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(FULL_SYNCHRONIZE_MODE_GROUP));

  SynchronizeFilesButton = new TFarRadioButton(this);
  SynchronizeFilesButton->SetCaption(GetMsg(SYNCHRONIZE_SYNCHRONIZE_FILES));

  SetNextItemPosition(ipRight);

  MirrorFilesButton = new TFarRadioButton(this);
  MirrorFilesButton->SetCaption(GetMsg(SYNCHRONIZE_MIRROR_FILES));

  SynchronizeTimestampsButton = new TFarRadioButton(this);
  SynchronizeTimestampsButton->SetCaption(GetMsg(SYNCHRONIZE_SYNCHRONIZE_TIMESTAMPS));
  SynchronizeTimestampsButton->SetEnabled(FLAGCLEAR(Options, fsoDisableTimestamp));

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(FULL_SYNCHRONIZE_GROUP));

  SynchronizeDeleteCheck = new TFarCheckBox(this);
  SynchronizeDeleteCheck->SetCaption(GetMsg(SYNCHRONIZE_DELETE));

  SetNextItemPosition(ipRight);

  SynchronizeExistingOnlyCheck = new TFarCheckBox(this);
  SynchronizeExistingOnlyCheck->SetCaption(GetMsg(SYNCHRONIZE_EXISTING_ONLY));
  SynchronizeExistingOnlyCheck->SetEnabledDependencyNegative(SynchronizeTimestampsButton);

  SetNextItemPosition(ipNewLine);

  SynchronizePreviewChangesCheck = new TFarCheckBox(this);
  SynchronizePreviewChangesCheck->SetCaption(GetMsg(SYNCHRONIZE_PREVIEW_CHANGES));

  SetNextItemPosition(ipRight);

  SynchronizeSelectedOnlyCheck = new TFarCheckBox(this);
  SynchronizeSelectedOnlyCheck->SetCaption(GetMsg(SYNCHRONIZE_SELECTED_ONLY));
  SynchronizeSelectedOnlyCheck->SetEnabled(FLAGSET(FOptions, fsoAllowSelectedOnly));

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(FULL_SYNCHRONIZE_CRITERIONS_GROUP));

  SynchronizeByTimeCheck = new TFarCheckBox(this);
  SynchronizeByTimeCheck->SetCaption(GetMsg(SYNCHRONIZE_BY_TIME));

  SetNextItemPosition(ipRight);

  SynchronizeBySizeCheck = new TFarCheckBox(this);
  SynchronizeBySizeCheck->SetCaption(GetMsg(SYNCHRONIZE_BY_SIZE));
  SynchronizeBySizeCheck->SetEnabledDependencyNegative(SynchronizeBothButton);

  SetNextItemPosition(ipNewLine);

  new TFarSeparator(this);

  SaveSettingsCheck = new TFarCheckBox(this);
  SaveSettingsCheck->SetCaption(GetMsg(SYNCHRONIZE_REUSE_SETTINGS));

  Separator = new TFarSeparator(this);
  Separator->SetGroup(1);
  Separator->SetCaption(GetMsg(COPY_PARAM_GROUP));

  CopyParamLister = new TFarLister(this);
  CopyParamLister->SetHeight(3);
  CopyParamLister->SetLeft(GetBorderBox()->GetLeft() + 1);
  CopyParamLister->SetTabStop(false);
  CopyParamLister->SetOnMouseClick(MAKE_CALLBACK(TFullSynchronizeDialog::CopyParamListerClick, this));
  CopyParamLister->SetGroup(1);
  // Right edge is adjusted in Change

  // Align buttons with bottom of the window
  Separator = new TFarSeparator(this);
  Separator->SetPosition(-4);

  TFarButton * Button = new TFarButton(this);
  Button->SetCaption(GetMsg(TRANSFER_SETTINGS_BUTTON));
  Button->SetResult(-1);
  Button->SetCenterGroup(true);
  Button->SetOnClick(MAKE_CALLBACK(TFullSynchronizeDialog::TransferSettingsButtonClick, this));

  SetNextItemPosition(ipRight);

  AddStandardButtons(0, true);

  FFullHeight = GetSize().y;
  AdaptSize();
}

void TFullSynchronizeDialog::AdaptSize()
{
  bool ShowCopyParam = (FFullHeight <= GetMaxSize().y);
  if (ShowCopyParam != CopyParamLister->GetVisible())
  {
    ShowGroup(1, ShowCopyParam);
    SetHeight(FFullHeight - (ShowCopyParam ? 0 : CopyParamLister->GetHeight() + 1));
  }
}

TTerminal::TSynchronizeMode TFullSynchronizeDialog::GetMode() const
{
  TTerminal::TSynchronizeMode Mode;

  if (SynchronizeRemoteButton->GetChecked())
  {
    Mode = TTerminal::smRemote;
  }
  else if (SynchronizeLocalButton->GetChecked())
  {
    Mode = TTerminal::smLocal;
  }
  else
  {
    Mode = TTerminal::smBoth;
  }

  return Mode;
}

void TFullSynchronizeDialog::TransferSettingsButtonClick(
  TFarButton * /*Sender*/, bool & Close)
{
  CustomCopyParam();
  Close = false;
}

void TFullSynchronizeDialog::CopyParamListerClick(
  TFarDialogItem * /*Item*/, MOUSE_EVENT_RECORD * Event)
{
  if (FLAGSET(Event->dwEventFlags, DOUBLE_CLICK))
  {
    CustomCopyParam();
  }
}

void TFullSynchronizeDialog::CustomCopyParam()
{
  TWinSCPPlugin * WinSCPPlugin = NB_STATIC_DOWNCAST(TWinSCPPlugin, FarPlugin);
  if (WinSCPPlugin->CopyParamCustomDialog(FCopyParams, ActualCopyParamAttrs()))
  {
    Change();
  }
}

void TFullSynchronizeDialog::Change()
{
  TWinSCPDialog::Change();

  if (GetHandle())
  {
    if (SynchronizeTimestampsButton->GetChecked())
    {
      SynchronizeExistingOnlyCheck->SetChecked(true);
      SynchronizeDeleteCheck->SetChecked(false);
      SynchronizeByTimeCheck->SetChecked(true);
    }
    if (SynchronizeBothButton->GetChecked())
    {
      SynchronizeBySizeCheck->SetChecked(false);
      if (MirrorFilesButton->GetChecked())
      {
        SynchronizeFilesButton->SetChecked(true);
      }
    }
    if (MirrorFilesButton->GetChecked())
    {
      SynchronizeByTimeCheck->SetChecked(true);
    }
    MirrorFilesButton->SetEnabled(!SynchronizeBothButton->GetChecked());
    SynchronizeDeleteCheck->SetEnabled(!SynchronizeBothButton->GetChecked() &&
      !SynchronizeTimestampsButton->GetChecked());
    SynchronizeByTimeCheck->SetEnabled(!SynchronizeBothButton->GetChecked() &&
      !SynchronizeTimestampsButton->GetChecked() && !MirrorFilesButton->GetChecked());
    SynchronizeBySizeCheck->SetCaption(SynchronizeTimestampsButton->GetChecked() ?
      GetMsg(SYNCHRONIZE_SAME_SIZE) : GetMsg(SYNCHRONIZE_BY_SIZE));

    if (!SynchronizeBySizeCheck->GetChecked() && !SynchronizeByTimeCheck->GetChecked())
    {
      // suppose that in FAR the checkbox cannot be unchecked unless focused
      if (SynchronizeByTimeCheck->Focused())
      {
        SynchronizeBySizeCheck->SetChecked(true);
      }
      else
      {
        SynchronizeByTimeCheck->SetChecked(true);
      }
    }

    UnicodeString InfoStr = FCopyParams.GetInfoStr(L"; ", ActualCopyParamAttrs());
    std::unique_ptr<TStrings> InfoStrLines(new TStringList());
    FarWrapText(InfoStr, InfoStrLines.get(), GetBorderBox()->GetWidth() - 4);
    CopyParamLister->SetItems(InfoStrLines.get());
    CopyParamLister->SetRight(GetBorderBox()->GetRight() - (CopyParamLister->GetScrollBar() ? 0 : 1));
  }
}

intptr_t TFullSynchronizeDialog::ActualCopyParamAttrs()
{
  intptr_t Result = -1;
  if (SynchronizeTimestampsButton->GetChecked())
  {
    Result = cpaIncludeMaskOnly;
  }
  else
  {
    switch (GetMode())
    {
      case TTerminal::smRemote:
        Result = FCopyParamAttrs.Upload;
        break;

      case TTerminal::smLocal:
        Result = FCopyParamAttrs.Download;
        break;

      default:
        DebugAssert(false);
        //fallthru
      case TTerminal::smBoth:
        Result = FCopyParamAttrs.General;
        break;
    }
  }
  return Result | cpaNoPreserveTime;
}

bool TFullSynchronizeDialog::CloseQuery()
{
  bool CanClose = TWinSCPDialog::CloseQuery();

  if (CanClose && (GetResult() == brOK) &&
      SaveSettingsCheck->GetChecked() && (FOrigMode != GetMode()) && !FSaveMode)
  {
    TWinSCPPlugin * WinSCPPlugin = NB_STATIC_DOWNCAST(TWinSCPPlugin, FarPlugin);

    switch (WinSCPPlugin->MoreMessageDialog(GetMsg(SAVE_SYNCHRONIZE_MODE), nullptr,
              qtConfirmation, qaYes | qaNo | qaCancel, 0))
    {
      case qaYes:
        FSaveMode = true;
        break;

      case qaCancel:
        CanClose = false;
        break;
    }
  }

  return CanClose;
}

LONG_PTR TFullSynchronizeDialog::DialogProc(int Msg, intptr_t Param1, LONG_PTR Param2)
{
  if (Msg == DN_RESIZECONSOLE)
  {
    AdaptSize();
  }

  return TFarDialog::DialogProc(Msg, Param1, Param2);
}

bool TFullSynchronizeDialog::Execute(TTerminal::TSynchronizeMode & Mode,
  intptr_t & Params, UnicodeString & LocalDirectory, UnicodeString & RemoteDirectory,
  TCopyParamType * CopyParams, bool & SaveSettings, bool & SaveMode)
{
  LocalDirectoryEdit->SetText(LocalDirectory);
  RemoteDirectoryEdit->SetText(RemoteDirectory);
  SynchronizeRemoteButton->SetChecked((Mode == TTerminal::smRemote));
  SynchronizeLocalButton->SetChecked((Mode == TTerminal::smLocal));
  SynchronizeBothButton->SetChecked((Mode == TTerminal::smBoth));
  SynchronizeDeleteCheck->SetChecked(FLAGSET(Params, TTerminal::spDelete));
  SynchronizeExistingOnlyCheck->SetChecked(FLAGSET(Params, TTerminal::spExistingOnly));
  SynchronizePreviewChangesCheck->SetChecked(FLAGSET(Params, TTerminal::spPreviewChanges));
  SynchronizeSelectedOnlyCheck->SetChecked(FLAGSET(Params, spSelectedOnly));
  if (FLAGSET(Params, TTerminal::spTimestamp) && FLAGCLEAR(FOptions, fsoDisableTimestamp))
  {
    SynchronizeTimestampsButton->SetChecked(true);
  }
  else if (FLAGSET(Params, TTerminal::spMirror))
  {
    MirrorFilesButton->SetChecked(true);
  }
  else
  {
    SynchronizeFilesButton->SetChecked(true);
  }
  SynchronizeByTimeCheck->SetChecked(FLAGCLEAR(Params, TTerminal::spNotByTime));
  SynchronizeBySizeCheck->SetChecked(FLAGSET(Params, TTerminal::spBySize));
  SaveSettingsCheck->SetChecked(SaveSettings);
  FSaveMode = SaveMode;
  FOrigMode = Mode;
  FCopyParams = *CopyParams;

  bool Result = (ShowModal() == brOK);

  if (Result)
  {
    RemoteDirectory = RemoteDirectoryEdit->GetText();
    LocalDirectory = LocalDirectoryEdit->GetText();

    Mode = GetMode();

    Params &= ~(TTerminal::spDelete | TTerminal::spNoConfirmation |
      TTerminal::spExistingOnly | TTerminal::spPreviewChanges |
      TTerminal::spTimestamp | TTerminal::spNotByTime | TTerminal::spBySize |
      spSelectedOnly | TTerminal::spMirror);
    Params |=
      FLAGMASK(SynchronizeDeleteCheck->GetChecked(), TTerminal::spDelete) |
      FLAGMASK(SynchronizeExistingOnlyCheck->GetChecked(), TTerminal::spExistingOnly) |
      FLAGMASK(SynchronizePreviewChangesCheck->GetChecked(), TTerminal::spPreviewChanges) |
      FLAGMASK(SynchronizeSelectedOnlyCheck->GetChecked(), spSelectedOnly) |
      FLAGMASK(SynchronizeTimestampsButton->GetChecked() && FLAGCLEAR(FOptions, fsoDisableTimestamp),
        TTerminal::spTimestamp) |
      FLAGMASK(MirrorFilesButton->GetChecked(), TTerminal::spMirror) |
      FLAGMASK(!SynchronizeByTimeCheck->GetChecked(), TTerminal::spNotByTime) |
      FLAGMASK(SynchronizeBySizeCheck->GetChecked(), TTerminal::spBySize);

    SaveSettings = SaveSettingsCheck->GetChecked();
    SaveMode = FSaveMode;
    *CopyParams = FCopyParams;
  }

  return Result;
}

bool TWinSCPFileSystem::FullSynchronizeDialog(TTerminal::TSynchronizeMode & Mode,
  intptr_t & Params, UnicodeString & LocalDirectory, UnicodeString & RemoteDirectory,
  TCopyParamType * CopyParams, bool & SaveSettings, bool & SaveMode, intptr_t Options,
  const TUsableCopyParamAttrs & CopyParamAttrs)
{
  std::unique_ptr<TFullSynchronizeDialog> Dialog(new TFullSynchronizeDialog(
    FPlugin, Options, CopyParamAttrs));
  bool Result = Dialog->Execute(Mode, Params, LocalDirectory, RemoteDirectory,
    CopyParams, SaveSettings, SaveMode);
  return Result;
}

class TSynchronizeChecklistDialog : public TWinSCPDialog
{
public:
  explicit TSynchronizeChecklistDialog(
    TCustomFarPlugin * AFarPlugin, TTerminal::TSynchronizeMode Mode, intptr_t Params,
    const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory);

  virtual bool Execute(TSynchronizeChecklist * Checklist);

protected:
  virtual LONG_PTR DialogProc(int Msg, intptr_t Param1, LONG_PTR Param2);
  virtual bool Key(TFarDialogItem * Item, LONG_PTR KeyCode);
  void CheckAllButtonClick(TFarButton * Sender, bool & Close);
  void VideoModeButtonClick(TFarButton * Sender, bool & Close);
  void ListBoxClick(TFarDialogItem * Item, MOUSE_EVENT_RECORD * Event);

private:
  TFarText * Header;
  TFarListBox * ListBox;
  TFarButton * CheckAllButton;
  TFarButton * UncheckAllButton;
  TFarButton * VideoModeButton;

  TSynchronizeChecklist * FChecklist;
  UnicodeString FLocalDirectory;
  UnicodeString FRemoteDirectory;
  static const int FColumns = 8;
  int FWidths[FColumns];
  UnicodeString FActions[TSynchronizeChecklist::ActionCount];
  intptr_t FScroll;
  bool FCanScrollRight;
  intptr_t FChecked;

  void AdaptSize();
  int ColumnWidth(intptr_t Index);
  void LoadChecklist();
  void RefreshChecklist(bool Scroll);
  void UpdateControls();
  void CheckAll(bool Check);
  UnicodeString ItemLine(const TChecklistItem * ChecklistItem);
  void AddColumn(UnicodeString & List, const UnicodeString & Value, size_t Column,
    bool AHeader = false);
  UnicodeString FormatSize(int64_t Size, int Column);
};

TSynchronizeChecklistDialog::TSynchronizeChecklistDialog(
  TCustomFarPlugin * AFarPlugin, TTerminal::TSynchronizeMode /*Mode*/, intptr_t /*Params*/,
  const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory) :
  TWinSCPDialog(AFarPlugin),
  FChecklist(nullptr),
  FLocalDirectory(LocalDirectory),
  FRemoteDirectory(RemoteDirectory),
  FScroll(0),
  FCanScrollRight(false)
{
  SetCaption(GetMsg(CHECKLIST_TITLE));

  Header = new TFarText(this);

  ListBox = new TFarListBox(this);
  ListBox->SetNoBox(true);
  // align list with bottom of the window
  ListBox->SetBottom(-5);
  ListBox->SetOnMouseClick(MAKE_CALLBACK(TSynchronizeChecklistDialog::ListBoxClick, this));

  UnicodeString Actions = GetMsg(CHECKLIST_ACTIONS);
  size_t Action = 0;
  while (!Actions.IsEmpty() && (Action < _countof(FActions)))
  {
    FActions[Action] = CutToChar(Actions, '|', false);
    Action++;
  }

  // align buttons with bottom of the window
  ButtonSeparator = new TFarSeparator(this);
  ButtonSeparator->SetTop(-4);
  ButtonSeparator->SetBottom(ButtonSeparator->GetTop());

  CheckAllButton = new TFarButton(this);
  CheckAllButton->SetCaption(GetMsg(CHECKLIST_CHECK_ALL));
  CheckAllButton->SetCenterGroup(true);
  CheckAllButton->SetOnClick(MAKE_CALLBACK(TSynchronizeChecklistDialog::CheckAllButtonClick, this));

  SetNextItemPosition(ipRight);

  UncheckAllButton = new TFarButton(this);
  UncheckAllButton->SetCaption(GetMsg(CHECKLIST_UNCHECK_ALL));
  UncheckAllButton->SetCenterGroup(true);
  UncheckAllButton->SetOnClick(MAKE_CALLBACK(TSynchronizeChecklistDialog::CheckAllButtonClick, this));

  VideoModeButton = new TFarButton(this);
  VideoModeButton->SetCenterGroup(true);
  VideoModeButton->SetOnClick(MAKE_CALLBACK(TSynchronizeChecklistDialog::VideoModeButtonClick, this));

  AddStandardButtons(0, true);

  AdaptSize();
  UpdateControls();
  ListBox->SetFocus();
}

void TSynchronizeChecklistDialog::AddColumn(UnicodeString & List,
  const UnicodeString & Value, size_t Column, bool AHeader)
{
  wchar_t Separator = L'|'; // '\xB3';
  intptr_t Len = Value.Length();
  intptr_t Width = static_cast<size_t>(FWidths[Column]);
  bool Right = (Column == 2) || (Column == 3) || (Column == 6) || (Column == 7);
  bool LastCol = (Column == FColumns - 1);
  if (Len <= Width)
  {
    intptr_t Added = 0;
    if (AHeader && (Len < Width))
    {
      Added += (Width - Len) / 2;
    }
    else if (Right && (Len < Width))
    {
      Added += Width - Len;
    }
    List += ::StringOfChar(L' ', Added) + Value;
    Added += Value.Length();
    if (Width > Added)
    {
      List += ::StringOfChar(' ', Width - Added);
    }
    if (!LastCol)
    {
      List += Separator;
    }
  }
  else
  {
    intptr_t Scroll = FScroll;
    if ((Scroll > 0) && !AHeader)
    {
      if (List.IsEmpty())
      {
        List += L'{';
        Width--;
        Scroll++;
      }
      else
      {
        List[List.Length()] = L'{';
      }
    }
    if (Scroll > Len - Width)
    {
      Scroll = Len - Width;
    }
    else if (!AHeader && LastCol && (Scroll < Len - Width))
    {
      Width--;
    }
    List += Value.SubString(Scroll + 1, Width);
    if (!Header && (Len - Scroll > Width))
    {
      List += L'}';
      FCanScrollRight = true;
    }
    else if (!LastCol)
    {
      List += Separator;
    }
  }
}

void TSynchronizeChecklistDialog::AdaptSize()
{
  FScroll = 0;
  SetSize(GetMaxSize());
#ifndef __linux__
  VideoModeButton->SetCaption(GetMsg(
    FarPlugin->ConsoleWindowState() == SW_SHOWMAXIMIZED ?
      CHECKLIST_RESTORE : CHECKLIST_MAXIMIZE));
#endif
  static const int Ratio[FColumns] = { 140, 100, 80, 150, -2, 100, 80, 150 };

  intptr_t Width = ListBox->GetWidth() - 2 /*checkbox*/ - 1 /*scrollbar*/ - FColumns;
  double Temp[FColumns];

  int TotalRatio = 0;
  int FixedRatio = 0;
  for (intptr_t Index = 0; Index < FColumns; ++Index)
  {
    if (Ratio[Index] >= 0)
    {
      TotalRatio += Ratio[Index];
    }
    else
    {
      FixedRatio += -Ratio[Index];
    }
  }

  intptr_t TotalAssigned = 0;
  for (intptr_t Index = 0; Index < FColumns; ++Index)
  {
    if (Ratio[Index] >= 0)
    {
      double W = ToDouble(Ratio[Index]) * (Width - FixedRatio) / TotalRatio;
      FWidths[Index] = static_cast<int>(floor(W));
      Temp[Index] = W - FWidths[Index];
    }
    else
    {
      FWidths[Index] = -Ratio[Index];
      Temp[Index] = 0;
    }
    TotalAssigned += FWidths[Index];
  }

  while (TotalAssigned < Width)
  {
    size_t GrowIndex = 0;
    double MaxMissing = 0.0;
    for (intptr_t Index = 0; Index < FColumns; ++Index)
    {
      if (MaxMissing < Temp[Index])
      {
        MaxMissing = Temp[Index];
        GrowIndex = Index;
      }
    }

    DebugAssert(MaxMissing > 0.0);

    FWidths[GrowIndex]++;
    Temp[GrowIndex] = 0.0;
    TotalAssigned++;
  }

  RefreshChecklist(false);
}

UnicodeString TSynchronizeChecklistDialog::FormatSize(
  int64_t Size, int Column)
{
  intptr_t Width = static_cast<intptr_t>(FWidths[Column]);
  UnicodeString Result = FORMAT(L"%lu", Size);

  if (Result.Length() > Width)
  {
    Result = FORMAT(L"%.2f 'K'", Size / 1024.0);
    if (Result.Length() > Width)
    {
      Result = FORMAT(L"%.2f 'M'", Size / (1024.0 * 1024));
      if (Result.Length() > Width)
      {
        Result = FORMAT(L"%.2f 'G'", Size / (1024.0 * 1024 * 1024));
        if (Result.Length() > Width)
        {
          // back to default
          Result = FORMAT(L"%lu", Size);
        }
      }
    }
  }

  return Result;
}

UnicodeString TSynchronizeChecklistDialog::ItemLine(const TChecklistItem * ChecklistItem)
{
  UnicodeString Line;
  UnicodeString S;

  S = ChecklistItem->GetFileName();
  if (ChecklistItem->IsDirectory)
  {
    S = ::IncludeTrailingBackslash(S);
  }
  AddColumn(Line, S, 0);

  if (ChecklistItem->Action == saDeleteRemote)
  {
    AddColumn(Line, L"", 1);
    AddColumn(Line, L"", 2);
    AddColumn(Line, L"", 3);
  }
  else
  {
    S = ChecklistItem->Local.Directory;
    if (::AnsiSameText(FLocalDirectory, S.SubString(1, FLocalDirectory.Length())))
    {
      S[1] = L'.';
      S.Delete(2, FLocalDirectory.Length() - 1);
    }
    else
    {
      DebugAssert(false);
    }
    AddColumn(Line, S, 1);
    if (ChecklistItem->Action == saDownloadNew)
    {
      AddColumn(Line, L"", 2);
      AddColumn(Line, L"", 3);
    }
    else
    {
      if (ChecklistItem->IsDirectory)
      {
        AddColumn(Line, L"", 2);
      }
      else
      {
        AddColumn(Line, FormatSize(ChecklistItem->Local.Size, 2), 2);
      }
      AddColumn(Line, core::UserModificationStr(ChecklistItem->Local.Modification,
        ChecklistItem->Local.ModificationFmt), 3);
    }
  }

  intptr_t Action = static_cast<intptr_t>(ChecklistItem->Action - 1);
  DebugAssert((Action != NPOS) && (Action < static_cast<intptr_t>(_countof(FActions))));
  AddColumn(Line, FActions[Action], 4);

  if (ChecklistItem->Action == saDeleteLocal)
  {
    AddColumn(Line, L"", 5);
    AddColumn(Line, L"", 6);
    AddColumn(Line, L"", 7);
  }
  else
  {
    S = ChecklistItem->Remote.Directory;
    if (::AnsiSameText(FRemoteDirectory, S.SubString(1, FRemoteDirectory.Length())))
    {
      S[1] = L'.';
      S.Delete(2, FRemoteDirectory.Length() - 1);
    }
    else
    {
      DebugAssert(false);
    }
    AddColumn(Line, S, 5);
    if (ChecklistItem->Action == saUploadNew)
    {
      AddColumn(Line, L"", 6);
      AddColumn(Line, L"", 7);
    }
    else
    {
      if (ChecklistItem->IsDirectory)
      {
        AddColumn(Line, L"", 6);
      }
      else
      {
        AddColumn(Line, FormatSize(ChecklistItem->Remote.Size, 6), 6);
      }
      AddColumn(Line, core::UserModificationStr(ChecklistItem->Remote.Modification,
        ChecklistItem->Remote.ModificationFmt), 7);
    }
  }

  return Line;
}

void TSynchronizeChecklistDialog::LoadChecklist()
{
  FChecked = 0;
  std::unique_ptr<TFarList> List(new TFarList());
  List->BeginUpdate();
  for (intptr_t Index = 0; Index < FChecklist->GetCount(); ++Index)
  {
    const TChecklistItem * ChecklistItem = FChecklist->GetItem(Index);

    List->AddObject(ItemLine(ChecklistItem),
      const_cast<TObject *>(reinterpret_cast<const TObject *>(ChecklistItem)));
  }
  List->EndUpdate();

  // items must be checked in second pass once the internal array is allocated
  for (intptr_t Index = 0; Index < FChecklist->GetCount(); ++Index)
  {
    const TChecklistItem * ChecklistItem = FChecklist->GetItem(Index);

    List->SetChecked(Index, ChecklistItem->Checked);
    if (ChecklistItem->Checked)
    {
      FChecked++;
    }
  }

  ListBox->SetItems(List.get());

  UpdateControls();
}

void TSynchronizeChecklistDialog::RefreshChecklist(bool Scroll)
{
  UnicodeString HeaderStr = GetMsg(CHECKLIST_HEADER);
  UnicodeString HeaderCaption(::StringOfChar(' ', 2));

  for (intptr_t Index = 0; Index < FColumns; ++Index)
  {
    AddColumn(HeaderCaption, CutToChar(HeaderStr, '|', false), Index, true);
  }
  Header->SetCaption(HeaderCaption);

  FCanScrollRight = false;
  TFarList * List = ListBox->GetItems();
  List->BeginUpdate();
  {
    SCOPE_EXIT
    {
      List->EndUpdate();
    };
    for (intptr_t Index = 0; Index < List->GetCount(); ++Index)
    {
      if (!Scroll || (List->GetString(Index).LastDelimiter(L"{}") > 0))
      {
        const TChecklistItem * ChecklistItem =
          NB_STATIC_DOWNCAST(TChecklistItem, List->GetObj(Index));

        List->SetString(Index, ItemLine(ChecklistItem));
      }
    }
  }
}

void TSynchronizeChecklistDialog::UpdateControls()
{
  ButtonSeparator->SetCaption(
    FORMAT(GetMsg(CHECKLIST_CHECKED).c_str(), FChecked, ListBox->GetItems()->GetCount()));
  CheckAllButton->SetEnabled(FChecked < ListBox->GetItems()->GetCount());
  UncheckAllButton->SetEnabled(FChecked > 0);
}

LONG_PTR TSynchronizeChecklistDialog::DialogProc(int Msg, intptr_t Param1, LONG_PTR Param2)
{
  if (Msg == DN_RESIZECONSOLE)
  {
    AdaptSize();
  }

  return TFarDialog::DialogProc(Msg, Param1, Param2);
}

void TSynchronizeChecklistDialog::CheckAll(bool Check)
{
  TFarList * List = ListBox->GetItems();
  List->BeginUpdate();
  {
    SCOPE_EXIT
    {
      List->EndUpdate();
    };
    intptr_t Count = List->GetCount();
    for (intptr_t Index = 0; Index < Count; ++Index)
    {
      List->SetChecked(Index, Check);
    }

    FChecked = Check ? Count : 0;
  }

  UpdateControls();
}

void TSynchronizeChecklistDialog::CheckAllButtonClick(
  TFarButton * Sender, bool & Close)
{
  CheckAll(Sender == CheckAllButton);
  ListBox->SetFocus();

  Close = false;
}

void TSynchronizeChecklistDialog::VideoModeButtonClick(
  TFarButton * /*Sender*/, bool & Close)
{
  FarPlugin->ToggleVideoMode();

  Close = false;
}

void TSynchronizeChecklistDialog::ListBoxClick(
  TFarDialogItem * /*Item*/, MOUSE_EVENT_RECORD * /*Event*/)
{
  intptr_t Index = ListBox->GetItems()->GetSelected();
  if (Index >= 0)
  {
    if (ListBox->GetItems()->GetChecked(Index))
    {
      ListBox->GetItems()->SetChecked(Index, false);
      FChecked--;
    }
    else if (!ListBox->GetItems()->GetChecked(Index))
    {
      ListBox->GetItems()->SetChecked(Index, true);
      FChecked++;
    }

    UpdateControls();
  }
}

bool TSynchronizeChecklistDialog::Key(TFarDialogItem * Item, LONG_PTR KeyCode)
{
  bool Result = false;
  if (ListBox->Focused())
  {
    if ((KeyCode == KEY_SHIFTADD) || (KeyCode == KEY_SHIFTSUBTRACT))
    {
      CheckAll(KeyCode == KEY_SHIFTADD);
      Result = true;
    }
    else if ((KeyCode == KEY_SPACE) || (KeyCode == KEY_INS) ||
             (KeyCode == KEY_ADD) || (KeyCode == KEY_SUBTRACT))
    {
      intptr_t Index = ListBox->GetItems()->GetSelected();
      if (Index >= 0)
      {
        if (ListBox->GetItems()->GetChecked(Index) && (KeyCode != KEY_ADD))
        {
          ListBox->GetItems()->SetChecked(Index, false);
          FChecked--;
        }
        else if (!ListBox->GetItems()->GetChecked(Index) && (KeyCode != KEY_SUBTRACT))
        {
          ListBox->GetItems()->SetChecked(Index, true);
          FChecked++;
        }

        // FAR WORKAROUND
        // Changing "checked" state is not always drawn.
        Redraw();
        UpdateControls();
        if ((KeyCode == KEY_INS) &&
            (Index < ListBox->GetItems()->GetCount() - 1))
        {
          ListBox->GetItems()->SetSelected(Index + 1);
        }
        else
        {
          ListBox->GetItems()->SetSelected(Index);
        }
      }
      Result = true;
    }
    else if (KeyCode == KEY_ALTLEFT)
    {
      if (FScroll > 0)
      {
        FScroll--;
        RefreshChecklist(true);
      }
      Result = true;
    }
    else if (KeyCode == KEY_ALTRIGHT)
    {
      if (FCanScrollRight)
      {
        FScroll++;
        RefreshChecklist(true);
      }
      Result = true;
    }
  }

  if (!Result)
  {
    Result = TWinSCPDialog::Key(Item, KeyCode);
  }

  return Result;
}

bool TSynchronizeChecklistDialog::Execute(TSynchronizeChecklist * Checklist)
{
  FChecklist = Checklist;
  LoadChecklist();
  bool Result = (ShowModal() == brOK);

  if (Result)
  {
    TFarList * List = ListBox->GetItems();
    intptr_t Count = List->GetCount();
    for (intptr_t Index = 0; Index < Count; ++Index)
    {
      TChecklistItem * ChecklistItem =
        NB_STATIC_DOWNCAST(TChecklistItem, List->GetObj(Index));
      ChecklistItem->Checked = List->GetChecked(Index);
    }
  }

  return Result;
}

bool TWinSCPFileSystem::SynchronizeChecklistDialog(
  TSynchronizeChecklist * Checklist, TTerminal::TSynchronizeMode Mode, intptr_t Params,
  const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory)
{
  std::unique_ptr<TSynchronizeChecklistDialog> Dialog(new TSynchronizeChecklistDialog(
    FPlugin, Mode, Params, LocalDirectory, RemoteDirectory));
  bool Result = Dialog->Execute(Checklist);
  return Result;
}

class TSynchronizeDialog : TFarDialog
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  explicit TSynchronizeDialog(TCustomFarPlugin * AFarPlugin,
    TSynchronizeStartStopEvent OnStartStop,
    intptr_t Options, intptr_t CopyParamAttrs, TGetSynchronizeOptionsEvent OnGetOptions);
  virtual ~TSynchronizeDialog();

  bool Execute(TSynchronizeParamType & Params,
    const TCopyParamType * CopyParams, bool & SaveSettings);

protected:
  virtual void Change();
  void UpdateControls();
  void StartButtonClick(TFarButton * Sender, bool & Close);
  void StopButtonClick(TFarButton * Sender, bool & Close);
  void TransferSettingsButtonClick(TFarButton * Sender, bool & Close);
  void CopyParamListerClick(TFarDialogItem * Item, MOUSE_EVENT_RECORD * Event);
  void Stop();
  void DoStartStop(bool Start, bool Synchronize);
  TSynchronizeParamType GetParams();
  void DoAbort(TObject * Sender, bool Close);
  void DoLog(TSynchronizeController * Controller,
    TSynchronizeLogEntry Entry, const UnicodeString & Message);
  void DoSynchronizeThreads(TObject * Sender, TThreadMethod Slot);
  virtual LONG_PTR DialogProc(int Msg, intptr_t Param1, LONG_PTR Param2);
  virtual bool CloseQuery();
  virtual bool Key(TFarDialogItem * Item, LONG_PTR KeyCode);
  TCopyParamType GetCopyParams();
  intptr_t ActualCopyParamAttrs();
  void CustomCopyParam();

private:
  bool FSynchronizing;
  bool FStarted;
  bool FAbort;
  bool FClose;
  TSynchronizeParamType FParams;
  TSynchronizeStartStopEvent FOnStartStop;
  intptr_t FOptions;
  TSynchronizeOptions * FSynchronizeOptions;
  TCopyParamType FCopyParams;
  TGetSynchronizeOptionsEvent FOnGetOptions;
  intptr_t FCopyParamAttrs;

  TFarEdit * LocalDirectoryEdit;
  TFarEdit * RemoteDirectoryEdit;
  TFarCheckBox * SynchronizeDeleteCheck;
  TFarCheckBox * SynchronizeExistingOnlyCheck;
  TFarCheckBox * SynchronizeSelectedOnlyCheck;
  TFarCheckBox * SynchronizeRecursiveCheck;
  TFarCheckBox * SynchronizeSynchronizeCheck;
  TFarCheckBox * SaveSettingsCheck;
  TFarButton * StartButton;
  TFarButton * StopButton;
  TFarButton * CloseButton;
  TFarLister * CopyParamLister;
};

TSynchronizeDialog::TSynchronizeDialog(TCustomFarPlugin * AFarPlugin,
  TSynchronizeStartStopEvent OnStartStop,
  intptr_t Options, intptr_t CopyParamAttrs, TGetSynchronizeOptionsEvent OnGetOptions) :
  TFarDialog(AFarPlugin)
{
  TFarText * Text;
  TFarSeparator * Separator;

  FSynchronizing = false;
  FStarted = false;
  FOnStartStop = OnStartStop;
  FAbort = false;
  FClose = false;
  FOptions = Options;
  FOnGetOptions = OnGetOptions;
  FSynchronizeOptions = nullptr;
  FCopyParamAttrs = CopyParamAttrs;

  SetSize(TPoint(76, 20));

  SetDefaultGroup(1);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(SYNCHRONIZE_LOCAL_LABEL));

  LocalDirectoryEdit = new TFarEdit(this);
  LocalDirectoryEdit->SetHistory(LOCAL_SYNC_HISTORY);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(SYNCHRONIZE_REMOTE_LABEL));

  RemoteDirectoryEdit = new TFarEdit(this);
  RemoteDirectoryEdit->SetHistory(REMOTE_SYNC_HISTORY);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(SYNCHRONIZE_GROUP));
  Separator->SetGroup(0);

  SynchronizeDeleteCheck = new TFarCheckBox(this);
  SynchronizeDeleteCheck->SetCaption(GetMsg(SYNCHRONIZE_DELETE));

  SetNextItemPosition(ipRight);

  SynchronizeExistingOnlyCheck = new TFarCheckBox(this);
  SynchronizeExistingOnlyCheck->SetCaption(GetMsg(SYNCHRONIZE_EXISTING_ONLY));

  SetNextItemPosition(ipNewLine);

  SynchronizeRecursiveCheck = new TFarCheckBox(this);
  SynchronizeRecursiveCheck->SetCaption(GetMsg(SYNCHRONIZE_RECURSIVE));

  SetNextItemPosition(ipRight);

  SynchronizeSelectedOnlyCheck = new TFarCheckBox(this);
  SynchronizeSelectedOnlyCheck->SetCaption(GetMsg(SYNCHRONIZE_SELECTED_ONLY));
  // have more complex enable rules
  SynchronizeSelectedOnlyCheck->SetGroup(0);

  SetNextItemPosition(ipNewLine);

  SynchronizeSynchronizeCheck = new TFarCheckBox(this);
  SynchronizeSynchronizeCheck->SetCaption(GetMsg(SYNCHRONIZE_SYNCHRONIZE));
  SynchronizeSynchronizeCheck->SetAllowGrayed(true);

  Separator = new TFarSeparator(this);
  Separator->SetGroup(0);

  SaveSettingsCheck = new TFarCheckBox(this);
  SaveSettingsCheck->SetCaption(GetMsg(SYNCHRONIZE_REUSE_SETTINGS));

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(COPY_PARAM_GROUP));

  CopyParamLister = new TFarLister(this);
  CopyParamLister->SetHeight(3);
  CopyParamLister->SetLeft(GetBorderBox()->GetLeft() + 1);
  CopyParamLister->SetTabStop(false);
  CopyParamLister->SetOnMouseClick(MAKE_CALLBACK(TSynchronizeDialog::CopyParamListerClick, this));
  // Right edge is adjusted in Change

  SetDefaultGroup(0);

  // align buttons with bottom of the window
  Separator = new TFarSeparator(this);
  Separator->SetPosition(-4);

  TFarButton * Button = new TFarButton(this);
  Button->SetCaption(GetMsg(TRANSFER_SETTINGS_BUTTON));
  Button->SetResult(-1);
  Button->SetCenterGroup(true);
  Button->SetOnClick(MAKE_CALLBACK(TSynchronizeDialog::TransferSettingsButtonClick, this));

  SetNextItemPosition(ipRight);

  StartButton = new TFarButton(this);
  StartButton->SetCaption(GetMsg(SYNCHRONIZE_START_BUTTON));
  StartButton->SetDefault(true);
  StartButton->SetCenterGroup(true);
  StartButton->SetOnClick(MAKE_CALLBACK(TSynchronizeDialog::StartButtonClick, this));

  StopButton = new TFarButton(this);
  StopButton->SetCaption(GetMsg(SYNCHRONIZE_STOP_BUTTON));
  StopButton->SetCenterGroup(true);
  StopButton->SetOnClick(MAKE_CALLBACK(TSynchronizeDialog::StopButtonClick, this));

  SetNextItemPosition(ipRight);

  CloseButton = new TFarButton(this);
  CloseButton->SetCaption(GetMsg(MSG_BUTTON_CLOSE));
  CloseButton->SetResult(brCancel);
  CloseButton->SetCenterGroup(true);
}

TSynchronizeDialog::~TSynchronizeDialog()
{
  SAFE_DESTROY(FSynchronizeOptions);
}

void TSynchronizeDialog::TransferSettingsButtonClick(
  TFarButton * /*Sender*/, bool & Close)
{
  CustomCopyParam();
  Close = false;
}

void TSynchronizeDialog::CopyParamListerClick(
  TFarDialogItem * /*Item*/, MOUSE_EVENT_RECORD * Event)
{
  if (FLAGSET(Event->dwEventFlags, DOUBLE_CLICK))
  {
    CustomCopyParam();
  }
}

void TSynchronizeDialog::CustomCopyParam()
{
  TWinSCPPlugin * WinSCPPlugin = NB_STATIC_DOWNCAST(TWinSCPPlugin, FarPlugin);
  // PreserveTime is forced for some settings, but avoid hard-setting it until
  // user really confirms it on custom dialog
  TCopyParamType ACopyParams = GetCopyParams();
  if (WinSCPPlugin->CopyParamCustomDialog(ACopyParams, ActualCopyParamAttrs()))
  {
    FCopyParams = ACopyParams;
    Change();
  }
}

bool TSynchronizeDialog::Execute(TSynchronizeParamType & Params,
  const TCopyParamType * CopyParams, bool & SaveSettings)
{
  RemoteDirectoryEdit->SetText(Params.RemoteDirectory);
  LocalDirectoryEdit->SetText(Params.LocalDirectory);
  SynchronizeDeleteCheck->SetChecked(FLAGSET(Params.Params, TTerminal::spDelete));
  SynchronizeExistingOnlyCheck->SetChecked(FLAGSET(Params.Params, TTerminal::spExistingOnly));
  SynchronizeSelectedOnlyCheck->SetChecked(FLAGSET(Params.Params, spSelectedOnly));
  SynchronizeRecursiveCheck->SetChecked(FLAGSET(Params.Options, soRecurse));
  SynchronizeSynchronizeCheck->SetSelected(
    FLAGSET(Params.Options, soSynchronizeAsk) ? BSTATE_3STATE :
      (FLAGSET(Params.Options, soSynchronize) ? BSTATE_CHECKED : BSTATE_UNCHECKED));
  SaveSettingsCheck->SetChecked(SaveSettings);

  FParams = Params;
  FCopyParams = *CopyParams;

  ShowModal();

  Params = GetParams();
  SaveSettings = SaveSettingsCheck->GetChecked();

  return true;
}

TSynchronizeParamType TSynchronizeDialog::GetParams()
{
  TSynchronizeParamType Result = FParams;
  Result.RemoteDirectory = RemoteDirectoryEdit->GetText();
  Result.LocalDirectory = LocalDirectoryEdit->GetText();
  Result.Params =
    (Result.Params & ~(TTerminal::spDelete | TTerminal::spExistingOnly |
     spSelectedOnly | TTerminal::spTimestamp)) |
    FLAGMASK(SynchronizeDeleteCheck->GetChecked(), TTerminal::spDelete) |
    FLAGMASK(SynchronizeExistingOnlyCheck->GetChecked(), TTerminal::spExistingOnly) |
    FLAGMASK(SynchronizeSelectedOnlyCheck->GetChecked(), spSelectedOnly);
  Result.Options =
    (Result.Options & ~(soRecurse | soSynchronize | soSynchronizeAsk)) |
    FLAGMASK(SynchronizeRecursiveCheck->GetChecked(), soRecurse) |
    FLAGMASK(SynchronizeSynchronizeCheck->GetSelected() == BSTATE_CHECKED, soSynchronize) |
    FLAGMASK(SynchronizeSynchronizeCheck->GetSelected() == BSTATE_3STATE, soSynchronizeAsk);
  return Result;
}

void TSynchronizeDialog::DoStartStop(bool Start, bool Synchronize)
{
  if (FOnStartStop)
  {
    TSynchronizeParamType SParams = GetParams();
    SParams.Options =
      (SParams.Options & ~(soSynchronize | soSynchronizeAsk)) |
      FLAGMASK(Synchronize, soSynchronize);
    if (Start)
    {
      SAFE_DESTROY(FSynchronizeOptions);
      FSynchronizeOptions = new TSynchronizeOptions;
      FOnGetOptions(SParams.Params, *FSynchronizeOptions);
    }
    FOnStartStop(this, Start, SParams, GetCopyParams(), FSynchronizeOptions,
      MAKE_CALLBACK(TSynchronizeDialog::DoAbort, this),
      MAKE_CALLBACK(TSynchronizeDialog::DoSynchronizeThreads, this),
      MAKE_CALLBACK(TSynchronizeDialog::DoLog, this));
  }
}

void TSynchronizeDialog::DoSynchronizeThreads(TObject * /*Sender*/,
  TThreadMethod Slot)
{
  if (FStarted)
  {
    Synchronize(Slot);
  }
}

LONG_PTR TSynchronizeDialog::DialogProc(int Msg, intptr_t Param1, LONG_PTR Param2)
{
  if (FAbort)
  {
    FAbort = false;

    if (FSynchronizing)
    {
      Stop();
    }

    if (FClose)
    {
      DebugAssert(CloseButton->GetEnabled());
      Close(CloseButton);
    }
  }

  return TFarDialog::DialogProc(Msg, Param1, Param2);
}

bool TSynchronizeDialog::CloseQuery()
{
  return TFarDialog::CloseQuery() && !FSynchronizing;
}

void TSynchronizeDialog::DoAbort(TObject * /*Sender*/, bool Close)
{
  FAbort = true;
  FClose = Close;
}

void TSynchronizeDialog::DoLog(TSynchronizeController * /*Controller*/,
  TSynchronizeLogEntry /*Entry*/, const UnicodeString & /*Message*/)
{
  // void
}

void TSynchronizeDialog::StartButtonClick(TFarButton * /*Sender*/,
  bool & /*Close*/)
{
  bool Synchronize = false;
  bool Continue = true;
  if (SynchronizeSynchronizeCheck->GetSelected() == BSTATE_3STATE)
  {
    TMessageParams Params;
    Params.Params = qpNeverAskAgainCheck;
    TWinSCPPlugin * WinSCPPlugin = NB_STATIC_DOWNCAST(TWinSCPPlugin, FarPlugin);
    switch (WinSCPPlugin->MoreMessageDialog(GetMsg(SYNCHRONISE_BEFORE_KEEPUPTODATE),
        nullptr, qtConfirmation, qaYes | qaNo | qaCancel, &Params))
    {
      case qaNeverAskAgain:
        SynchronizeSynchronizeCheck->SetSelected(BSTATE_CHECKED);
        // fall thru

      case qaYes:
        Synchronize = true;
        break;

      case qaNo:
        Synchronize = false;
        break;

      default:
      case qaCancel:
        Continue = false;
        break;
    }
  }
  else
  {
    Synchronize = SynchronizeSynchronizeCheck->GetChecked();
  }

  if (Continue)
  {
    DebugAssert(!FSynchronizing);

    FSynchronizing = true;
    try
    {
      UpdateControls();

      DoStartStop(true, Synchronize);

      StopButton->SetFocus();
      FStarted = true;
    }
    catch (Exception & E)
    {
      FSynchronizing = false;
      UpdateControls();

      FarPlugin->HandleException(&E);
    }
  }
}

void TSynchronizeDialog::StopButtonClick(TFarButton * /*Sender*/,
  bool & /*Close*/)
{
  Stop();
}

void TSynchronizeDialog::Stop()
{
  FSynchronizing = false;
  FStarted = false;
  BreakSynchronize();
  DoStartStop(false, false);
  UpdateControls();
  StartButton->SetFocus();
}

void TSynchronizeDialog::Change()
{
  TFarDialog::Change();

  if (GetHandle() && !ChangesLocked())
  {
    UpdateControls();

    UnicodeString InfoStr = FCopyParams.GetInfoStr(L"; ", ActualCopyParamAttrs());
    std::unique_ptr<TStrings> InfoStrLines(new TStringList());
    FarWrapText(InfoStr, InfoStrLines.get(), GetBorderBox()->GetWidth() - 4);
    CopyParamLister->SetItems(InfoStrLines.get());
    CopyParamLister->SetRight(GetBorderBox()->GetRight() - (CopyParamLister->GetScrollBar() ? 0 : 1));
  }
}

bool TSynchronizeDialog::Key(TFarDialogItem * /*Item*/, LONG_PTR KeyCode)
{
  bool Result = false;
  if ((KeyCode == KEY_ESC) && FSynchronizing)
  {
    Stop();
    Result = true;
  }
  return Result;
}

void TSynchronizeDialog::UpdateControls()
{
  SetCaption(GetMsg(FSynchronizing ? SYNCHRONIZE_SYCHRONIZING : SYNCHRONIZE_TITLE));
  StartButton->SetEnabled(!FSynchronizing);
  StopButton->SetEnabled(FSynchronizing);
  CloseButton->SetEnabled(!FSynchronizing);
  EnableGroup(1, !FSynchronizing);
  SynchronizeSelectedOnlyCheck->SetEnabled(
    !FSynchronizing && FLAGSET(FOptions, soAllowSelectedOnly));
}

TCopyParamType TSynchronizeDialog::GetCopyParams()
{
  TCopyParamType Result = FCopyParams;
  Result.SetPreserveTime(true);
  return Result;
}

intptr_t TSynchronizeDialog::ActualCopyParamAttrs()
{
  return FCopyParamAttrs | cpaNoPreserveTime;
}

bool TWinSCPFileSystem::SynchronizeDialog(TSynchronizeParamType & Params,
  const TCopyParamType * CopyParams, TSynchronizeStartStopEvent OnStartStop,
  bool & SaveSettings, intptr_t Options, intptr_t CopyParamAttrs, TGetSynchronizeOptionsEvent OnGetOptions)
{
  std::unique_ptr<TSynchronizeDialog> Dialog(new TSynchronizeDialog(FPlugin, OnStartStop,
    Options, CopyParamAttrs, OnGetOptions));
  bool Result = Dialog->Execute(Params, CopyParams, SaveSettings);
  return Result;
}

bool TWinSCPFileSystem::RemoteTransferDialog(TStrings * AFileList,
  UnicodeString & Target, UnicodeString & FileMask, bool Move)
{
  UnicodeString Prompt = FileNameFormatString(
    GetMsg(Move ? REMOTE_MOVE_FILE : REMOTE_COPY_FILE),
    GetMsg(Move ? REMOTE_MOVE_FILES : REMOTE_COPY_FILES), AFileList, true);

  UnicodeString Value = core::UnixIncludeTrailingBackslash(Target) + FileMask;
  bool Result = FPlugin->InputBox(
    GetMsg(Move ? REMOTE_MOVE_TITLE : REMOTE_COPY_TITLE), Prompt,
    Value, 0, MOVE_TO_HISTORY) && !Value.IsEmpty();
  if (Result)
  {
    Target = core::UnixExtractFilePath(Value);
    FileMask = base::UnixExtractFileName(Value);
  }
  return Result;
}

bool TWinSCPFileSystem::RenameFileDialog(TRemoteFile * AFile,
  UnicodeString & NewName)
{
  return FPlugin->InputBox(GetMsg(RENAME_FILE_TITLE).c_str(),
    FORMAT(GetMsg(RENAME_FILE).c_str(), AFile->GetFileName().c_str()), NewName, 0) &&
    !NewName.IsEmpty();
}

class TQueueDialog : TFarDialog
{
CUSTOM_MEM_ALLOCATION_IMPL
NB_DISABLE_COPY(TQueueDialog)
public:
  explicit TQueueDialog(TCustomFarPlugin * AFarPlugin,
    TWinSCPFileSystem * AFileSystem, bool ClosingPlugin);
  virtual ~TQueueDialog() {}
  bool Execute(TTerminalQueueStatus * Status);

protected:
  virtual void Change();
  virtual void Idle();
  bool UpdateQueue();
  void LoadQueue();
  void RefreshQueue();
  bool FillQueueItemLine(UnicodeString & Line,
    TQueueItemProxy * QueueItem, size_t Index);
  bool QueueItemNeedsFrequentRefresh(TQueueItemProxy * QueueItem);
  void UpdateControls();
  virtual bool Key(TFarDialogItem * Item, LONG_PTR KeyCode);
  virtual bool CloseQuery();

private:
  void OperationButtonClick(TFarButton * Sender, bool & Close);

private:
  TTerminalQueueStatus * FStatus;
  TWinSCPFileSystem * FFileSystem;
  bool FClosingPlugin;

  TFarListBox * QueueListBox;
  TFarButton * ShowButton;
  TFarButton * ExecuteButton;
  TFarButton * DeleteButton;
  TFarButton * MoveUpButton;
  TFarButton * MoveDownButton;
  TFarButton * CloseButton;
};

TQueueDialog::TQueueDialog(TCustomFarPlugin * AFarPlugin,
  TWinSCPFileSystem * AFileSystem, bool ClosingPlugin) :
  TFarDialog(AFarPlugin),
  FStatus(nullptr),
  FFileSystem(AFileSystem),
  FClosingPlugin(ClosingPlugin),
  QueueListBox(nullptr),
  ShowButton(nullptr),
  ExecuteButton(nullptr),
  DeleteButton(nullptr),
  MoveUpButton(nullptr),
  MoveDownButton(nullptr),
  CloseButton(nullptr)
{
  TFarSeparator * Separator = nullptr;
  TFarText * Text = nullptr;

  SetSize(TPoint(80, 23));
  // TRect CRect = GetClientRect();
  intptr_t ListTop;
  intptr_t ListHeight = GetClientSize().y - 4;

  SetCaption(GetMsg(QUEUE_TITLE));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(QUEUE_HEADER));

  Separator = new TFarSeparator(this);
  ListTop = Separator->GetBottom();

  Separator = new TFarSeparator(this);
  Separator->Move(0, ListHeight);

  ExecuteButton = new TFarButton(this);
  ExecuteButton->SetCaption(GetMsg(QUEUE_EXECUTE));
  ExecuteButton->SetOnClick(MAKE_CALLBACK(TQueueDialog::OperationButtonClick, this));
  ExecuteButton->SetCenterGroup(true);

  SetNextItemPosition(ipRight);

  DeleteButton = new TFarButton(this);
  DeleteButton->SetCaption(GetMsg(QUEUE_DELETE));
  DeleteButton->SetOnClick(MAKE_CALLBACK(TQueueDialog::OperationButtonClick, this));
  DeleteButton->SetCenterGroup(true);

  MoveUpButton = new TFarButton(this);
  MoveUpButton->SetCaption(GetMsg(QUEUE_MOVE_UP));
  MoveUpButton->SetOnClick(MAKE_CALLBACK(TQueueDialog::OperationButtonClick, this));
  MoveUpButton->SetCenterGroup(true);

  MoveDownButton = new TFarButton(this);
  MoveDownButton->SetCaption(GetMsg(QUEUE_MOVE_DOWN));
  MoveDownButton->SetOnClick(MAKE_CALLBACK(TQueueDialog::OperationButtonClick, this));
  MoveDownButton->SetCenterGroup(true);

  CloseButton = new TFarButton(this);
  CloseButton->SetCaption(GetMsg(QUEUE_CLOSE));
  CloseButton->SetResult(brCancel);
  CloseButton->SetCenterGroup(true);
  CloseButton->SetDefault(true);

  SetNextItemPosition(ipNewLine);

  QueueListBox = new TFarListBox(this);
  QueueListBox->SetTop(ListTop + 1);
  QueueListBox->SetHeight(ListHeight);
  QueueListBox->SetNoBox(true);
  QueueListBox->SetFocus();
}

void TQueueDialog::OperationButtonClick(TFarButton * Sender,
  bool & /*Close*/)
{
  if (QueueListBox->GetItems()->GetSelected() != NPOS)
  {
    TQueueItemProxy * QueueItem = NB_STATIC_DOWNCAST(TQueueItemProxy,
      QueueListBox->GetItems()->GetObj(QueueListBox->GetItems()->GetSelected()));

    if (Sender == ExecuteButton)
    {
      if (QueueItem->GetStatus() == TQueueItem::qsProcessing)
      {
        QueueItem->Pause();
      }
      else if (QueueItem->GetStatus() == TQueueItem::qsPaused)
      {
        QueueItem->Resume();
      }
      else if (QueueItem->GetStatus() == TQueueItem::qsPending)
      {
        QueueItem->ExecuteNow();
      }
      else if (TQueueItem::IsUserActionStatus(QueueItem->GetStatus()))
      {
        QueueItem->ProcessUserAction();
      }
      else
      {
        DebugAssert(false);
      }
    }
    else if ((Sender == MoveUpButton) || (Sender == MoveDownButton))
    {
      QueueItem->Move(Sender == MoveUpButton);
    }
    else if (Sender == DeleteButton)
    {
      QueueItem->Delete();
    }
  }
}

bool TQueueDialog::Key(TFarDialogItem * /*Item*/, LONG_PTR KeyCode)
{
  bool Result = false;
  if (QueueListBox->Focused())
  {
    TFarButton * DoButton = nullptr;
    if (KeyCode == KEY_ENTER)
    {
      if (ExecuteButton->GetEnabled())
      {
        DoButton = ExecuteButton;
      }
      Result = true;
    }
    else if (KeyCode == KEY_DEL)
    {
      if (DeleteButton->GetEnabled())
      {
        DoButton = DeleteButton;
      }
      Result = true;
    }
    else if (KeyCode == KEY_CTRLUP)
    {
      if (MoveUpButton->GetEnabled())
      {
        DoButton = MoveUpButton;
      }
      Result = true;
    }
    else if (KeyCode == KEY_CTRLDOWN)
    {
      if (MoveDownButton->GetEnabled())
      {
        DoButton = MoveDownButton;
      }
      Result = true;
    }

    if (DoButton != nullptr)
    {
      bool Close;
      OperationButtonClick(DoButton, Close);
    }
  }
  return Result;
}

void TQueueDialog::UpdateControls()
{
  TQueueItemProxy * QueueItem = nullptr;
  if (QueueListBox->GetItems()->GetSelected() >= 0)
  {
    QueueItem = NB_STATIC_DOWNCAST(TQueueItemProxy,
      QueueListBox->GetItems()->GetObj(QueueListBox->GetItems()->GetSelected()));
  }

  if ((QueueItem != nullptr) && (QueueItem->GetStatus() == TQueueItem::qsProcessing))
  {
    ExecuteButton->SetCaption(GetMsg(QUEUE_PAUSE));
    ExecuteButton->SetEnabled(true);
  }
  else if ((QueueItem != nullptr) && (QueueItem->GetStatus() == TQueueItem::qsPaused))
  {
    ExecuteButton->SetCaption(GetMsg(QUEUE_RESUME));
    ExecuteButton->SetEnabled(true);
  }
  else if ((QueueItem != nullptr) && TQueueItem::IsUserActionStatus(QueueItem->GetStatus()))
  {
    ExecuteButton->SetCaption(GetMsg(QUEUE_SHOW));
    ExecuteButton->SetEnabled(true);
  }
  else
  {
    ExecuteButton->SetCaption(GetMsg(QUEUE_EXECUTE));
    ExecuteButton->SetEnabled(
      (QueueItem != nullptr) && (QueueItem->GetStatus() == TQueueItem::qsPending));
  }
  DeleteButton->SetEnabled((QueueItem != nullptr) &&
    (QueueItem->GetStatus() != TQueueItem::qsDone));
  MoveUpButton->SetEnabled((QueueItem != nullptr) &&
    (QueueItem->GetStatus() == TQueueItem::qsPending) &&
    (QueueItem->GetIndex() > FStatus->GetActiveCount()));
  MoveDownButton->SetEnabled((QueueItem != nullptr) &&
    (QueueItem->GetStatus() == TQueueItem::qsPending) &&
    (QueueItem->GetIndex() < FStatus->GetCount() - 1));
}

void TQueueDialog::Idle()
{
  TFarDialog::Idle();

  if (UpdateQueue())
  {
    LoadQueue();
    UpdateControls();
  }
  else
  {
    RefreshQueue();
  }
}

bool TQueueDialog::CloseQuery()
{
  bool Result = TFarDialog::CloseQuery();
  if (Result)
  {
    TWinSCPPlugin * WinSCPPlugin = NB_STATIC_DOWNCAST(TWinSCPPlugin, FarPlugin);
    Result = !FClosingPlugin || (FStatus->GetCount() == 0) ||
      (WinSCPPlugin->MoreMessageDialog(GetMsg(QUEUE_PENDING_ITEMS), nullptr,
        qtWarning, qaOK | qaCancel) == qaCancel);
  }
  return Result;
}

bool TQueueDialog::UpdateQueue()
{
  DebugAssert(FFileSystem != nullptr);
  TTerminalQueueStatus * Status = FFileSystem->ProcessQueue(false);
  bool Result = (Status != nullptr);
  if (Result)
  {
    FStatus = Status;
  }
  return Result;
}

void TQueueDialog::Change()
{
  TFarDialog::Change();

  if (GetHandle())
  {
    UpdateControls();
  }
}

void TQueueDialog::RefreshQueue()
{
  if (QueueListBox->GetItems()->GetCount() > 0)
  {
    bool Change = false;
    intptr_t TopIndex = QueueListBox->GetItems()->GetTopIndex();
    intptr_t Index = TopIndex;

    intptr_t ILine = 0;
    while ((Index > ILine) &&
           (QueueListBox->GetItems()->GetObj(Index) ==
            QueueListBox->GetItems()->GetObj(Index - ILine - 1)))
    {
      ILine++;
    }

    TQueueItemProxy * PrevQueueItem = nullptr;
    TQueueItemProxy * QueueItem = nullptr;
    UnicodeString Line;
    while ((Index < QueueListBox->GetItems()->GetCount()) &&
           (Index < TopIndex + QueueListBox->GetHeight()))
    {
      QueueItem = NB_STATIC_DOWNCAST(TQueueItemProxy,
        QueueListBox->GetItems()->GetObj(Index));
      DebugAssert(QueueItem != nullptr);
      if ((PrevQueueItem != nullptr) && (QueueItem != PrevQueueItem))
      {
        ILine = 0;
      }

      if (QueueItemNeedsFrequentRefresh(QueueItem) &&
          !QueueItem->GetProcessingUserAction())
      {
        FillQueueItemLine(Line, QueueItem, ILine);
        if (QueueListBox->GetItems()->GetString(Index) != Line)
        {
          Change = true;
          QueueListBox->GetItems()->SetString(Index, Line);
        }
      }

      PrevQueueItem = QueueItem;
      ++Index;
      ++ILine;
    }

    if (Change)
    {
      Redraw();
    }
  }
}

void TQueueDialog::LoadQueue()
{
  std::unique_ptr<TFarList> List(new TFarList());
  UnicodeString Line;
  for (intptr_t Index = 0; Index < FStatus->GetCount(); ++Index)
  {
    TQueueItemProxy * QueueItem = FStatus->GetItem(Index);
    size_t ILine = 0;
    while (FillQueueItemLine(Line, QueueItem, ILine))
    {
      List->AddObject(Line, QueueItem);
      List->SetDisabled(List->GetCount() - 1, (ILine > 0));
      ILine++;
    }
  }
  QueueListBox->SetItems(List.get());
}

bool TQueueDialog::FillQueueItemLine(UnicodeString & Line,
  TQueueItemProxy * QueueItem, size_t Index)
{
  int PathMaxLen = 49;

  if ((Index > 2) ||
      ((Index == 2) && (QueueItem->GetStatus() == TQueueItem::qsPending)))
  {
    return false;
  }

  UnicodeString ProgressStr;

  switch (QueueItem->GetStatus())
  {
    case TQueueItem::qsPending:
      ProgressStr = GetMsg(QUEUE_PENDING);
      break;

    case TQueueItem::qsConnecting:
      ProgressStr = GetMsg(QUEUE_CONNECTING);
      break;

    case TQueueItem::qsQuery:
      ProgressStr = GetMsg(QUEUE_QUERY);
      break;

    case TQueueItem::qsError:
      ProgressStr = GetMsg(QUEUE_ERROR);
      break;

    case TQueueItem::qsPrompt:
      ProgressStr = GetMsg(QUEUE_PROMPT);
      break;

    case TQueueItem::qsPaused:
      ProgressStr = GetMsg(QUEUE_PAUSED);
      break;
  }

  bool BlinkHide = QueueItemNeedsFrequentRefresh(QueueItem) &&
    !QueueItem->GetProcessingUserAction() &&
    ((::GetTickCount() % 2000) >= 1000);

  UnicodeString Operation;
  UnicodeString Direction;
  UnicodeString Values[2];
  TFileOperationProgressType * ProgressData = QueueItem->GetProgressData();
  TQueueItem::TInfo * Info = QueueItem->GetInfo();

  if (Index == 0)
  {
    if (!BlinkHide)
    {
      switch (Info->Operation)
      {
        case foCopy:
          Operation = GetMsg(QUEUE_COPY);
          break;

        case foMove:
          Operation = GetMsg(QUEUE_MOVE);
          break;
      }
      Direction = GetMsg((Info->Side == osLocal) ? QUEUE_UPLOAD : QUEUE_DOWNLOAD);
    }

    Values[0] = core::MinimizeName(Info->Source, PathMaxLen, (Info->Side == osRemote));

    if ((ProgressData != nullptr) &&
        (ProgressData->Operation == Info->Operation))
    {
      Values[1] = FormatBytes(ProgressData->TotalTransfered);
    }
  }
  else if (Index == 1)
  {
    Values[0] = core::MinimizeName(Info->Destination, PathMaxLen, (Info->Side == osLocal));

    if (ProgressStr.IsEmpty())
    {
      if (ProgressData != nullptr)
      {
        if (ProgressData->Operation == Info->Operation)
        {
          Values[1] = FORMAT(L"%d%%", ProgressData->OverallProgress());
        }
        else if (ProgressData->Operation == foCalculateSize)
        {
          Values[1] = GetMsg(QUEUE_CALCULATING_SIZE);
        }
      }
    }
    else if (!BlinkHide)
    {
      Values[1] = ProgressStr;
    }
  }
  else
  {
    if (ProgressData != nullptr)
    {
      Values[0] = core::MinimizeName(ProgressData->FileName, PathMaxLen,
        (Info->Side == osRemote));
      if (ProgressData->Operation == Info->Operation)
      {
        Values[1] = FORMAT(L"%d%%", ProgressData->TransferProgress());
      }
    }
    else
    {
      Values[0] = ProgressStr;
    }
  }

  Line = FORMAT(L"%1s %1s  %-49s %s",
    Operation.c_str(), Direction.c_str(), Values[0].c_str(), Values[1].c_str());

  return true;
}

bool TQueueDialog::QueueItemNeedsFrequentRefresh(
  TQueueItemProxy * QueueItem)
{
  return (QueueItem &&
    (TQueueItem::IsUserActionStatus(QueueItem->GetStatus()) ||
     (QueueItem->GetStatus() == TQueueItem::qsPaused)));
}

bool TQueueDialog::Execute(TTerminalQueueStatus * Status)
{
  FStatus = Status;

  UpdateQueue();
  LoadQueue();

  bool Result = (ShowModal() != brCancel);

  FStatus = nullptr;

  return Result;
}

bool TWinSCPFileSystem::QueueDialog(
  TTerminalQueueStatus * Status, bool ClosingPlugin)
{
  std::unique_ptr<TQueueDialog> Dialog(new TQueueDialog(FPlugin, this, ClosingPlugin));
  bool Result = Dialog->Execute(Status);
  return Result;
}

bool TWinSCPFileSystem::CreateDirectoryDialog(UnicodeString & Directory,
  TRemoteProperties * Properties, bool & SaveSettings)
{
  std::unique_ptr<TWinSCPDialog> Dialog(new TWinSCPDialog(FPlugin));
  TFarText * Text;
  TFarSeparator * Separator;

  Dialog->SetCaption(GetMsg(CREATE_FOLDER_TITLE));
  Dialog->SetSize(TPoint(66, 15));

  Text = new TFarText(Dialog.get());
  Text->SetCaption(GetMsg(CREATE_FOLDER_PROMPT));

  TFarEdit * DirectoryEdit = new TFarEdit(Dialog.get());
  DirectoryEdit->SetHistory(L"NewFolder");

  Separator = new TFarSeparator(Dialog.get());
  Separator->SetCaption(GetMsg(CREATE_FOLDER_ATTRIBUTES));

  TFarCheckBox * SetRightsCheck = new TFarCheckBox(Dialog.get());
  SetRightsCheck->SetCaption(GetMsg(CREATE_FOLDER_SET_RIGHTS));

  TRightsContainer * RightsContainer = new TRightsContainer(Dialog.get(), false, true,
      true, SetRightsCheck);

  TFarCheckBox * SaveSettingsCheck = new TFarCheckBox(Dialog.get());
  SaveSettingsCheck->SetCaption(GetMsg(CREATE_FOLDER_REUSE_SETTINGS));
  SaveSettingsCheck->Move(0, 6);

  Dialog->AddStandardButtons();

  DirectoryEdit->SetText(Directory);
  SaveSettingsCheck->SetChecked(SaveSettings);
  DebugAssert(Properties != nullptr);
  SetRightsCheck->SetChecked(Properties->Valid.Contains(vpRights));
  // expect sensible value even if rights are not set valid
  RightsContainer->SetRights(Properties->Rights);

  bool Result = (Dialog->ShowModal() == brOK);

  if (Result)
  {
    Directory = DirectoryEdit->GetText();
    SaveSettings = SaveSettingsCheck->GetChecked();
    if (SetRightsCheck->GetChecked())
    {
      Properties->Valid = Properties->Valid << vpRights;
      Properties->Rights = RightsContainer->GetRights();
    }
    else
    {
      Properties->Valid = Properties->Valid >> vpRights;
    }
  }
  return Result;
}

NB_IMPLEMENT_CLASS(TTabButton, NB_GET_CLASS_INFO(TFarButton), nullptr)
NB_IMPLEMENT_CLASS(TLabelList, NB_GET_CLASS_INFO(TList), nullptr)

