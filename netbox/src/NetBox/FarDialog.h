#pragma once

#include "FarPlugin.h"

#define MAX_SIZE -1

class TFarDialogContainer;
class TFarDialogItem;
class TFarButton;
class TFarSeparator;
class TFarBox;
class TFarList;
struct FarDialogItem;
enum TItemPosition
{
  ipNewLine,
  ipBelow,
  ipRight
};

DEFINE_CALLBACK_TYPE4(TFarKeyEvent, void,
  TFarDialog * /*Sender*/, TFarDialogItem * /*Item*/, long /*KeyCode*/, bool & /*Handled*/);
DEFINE_CALLBACK_TYPE2(TFarMouseClickEvent, void,
  TFarDialogItem * /*Item*/, MOUSE_EVENT_RECORD * /*Event*/);
DEFINE_CALLBACK_TYPE2(TFarProcessGroupEvent, void,
  TFarDialogItem * /*Item*/, void * /*Arg*/);

class TFarDialog : public TObject
{
friend TFarDialogItem;
friend TFarDialogContainer;
friend TFarButton;
friend TFarList;
friend class TFarListBox;
NB_DISABLE_COPY(TFarDialog)
NB_DECLARE_CLASS(TFarDialog)
public:
  explicit TFarDialog(TCustomFarPlugin * AFarPlugin);
  virtual ~TFarDialog();

  intptr_t ShowModal();
  void ShowGroup(intptr_t Group, bool Show);
  void EnableGroup(intptr_t Group, bool Enable);

  TRect GetBounds() const { return FBounds; }
  TRect GetClientRect() const;
  UnicodeString GetHelpTopic() const { return FHelpTopic; }
  void SetHelpTopic(const UnicodeString & Value);
  DWORD GetFlags() const { return FFlags; }
  void SetFlags(DWORD Value);
  bool GetCentered() const;
  void SetCentered(bool Value);
  TPoint GetSize() const;
  void SetSize(TPoint Value);
  TPoint GetClientSize() const;
  intptr_t GetWidth() const;
  void SetWidth(intptr_t Value);
  intptr_t GetHeight() const;
  void SetHeight(intptr_t Value);
  UnicodeString GetCaption() const;
  void SetCaption(const UnicodeString & Value);
  HANDLE GetHandle() const { return FHandle; }
  TFarButton * GetDefaultButton() const { return FDefaultButton; }
  TFarBox * GetBorderBox() const { return FBorderBox; }
  intptr_t GetType(TFarDialogItem * Item) const;
  intptr_t GetItem(TFarDialogItem * Item) const;
  TFarDialogItem * GetItem(intptr_t Index) const;
  TFarDialogItem * GetControl(intptr_t Index) const { return GetItem(Index); }
  intptr_t GetItemCount() const;
  intptr_t GetControlCount() const { return GetItemCount(); }
  TItemPosition GetNextItemPosition() const { return FNextItemPosition; }
  void SetNextItemPosition(const TItemPosition Value) { FNextItemPosition = Value; }
  intptr_t GetDefaultGroup() const { return FDefaultGroup; }
  void SetDefaultGroup(intptr_t Value) { FDefaultGroup = Value; }
  intptr_t GetTag() const { return FTag; }
  void SetTag(intptr_t Value) { FTag = Value; }
  TFarDialogItem * GetItemFocused() const { return FItemFocused; }
  void SetItemFocused(TFarDialogItem * Value);
  intptr_t GetResult() const { return FResult; }
  TPoint GetMaxSize();

  TFarKeyEvent & GetOnKey() { return FOnKey; }
  void SetOnKey(TFarKeyEvent Value) { FOnKey = Value; }

  void Redraw();
  void LockChanges();
  void UnlockChanges();
  uintptr_t GetSystemColor(intptr_t Index);
  bool HotKey(uintptr_t Key) const;

protected:
  TCustomFarPlugin * GetFarPlugin() const { return FFarPlugin; }
  TCustomFarPlugin * GetFarPlugin() { return FFarPlugin; }
  TObjectList * GetItems() const { return FItems; }
  TObjectList * GetItems() { return FItems; }
  void Add(TFarDialogItem * Item);
  void Add(TFarDialogContainer * Container);
  LONG_PTR SendDlgMessage(int Msg, intptr_t Param1, LONG_PTR Param2);
  virtual LONG_PTR DialogProc(int Msg, intptr_t Param1, LONG_PTR Param2);
  virtual LONG_PTR FailDialogProc(int Msg, intptr_t Param1, LONG_PTR Param2);
  LONG_PTR DefaultDialogProc(int Msg, intptr_t Param1, LONG_PTR Param2);
  virtual bool MouseEvent(MOUSE_EVENT_RECORD * Event);
  virtual bool Key(TFarDialogItem * Item, LONG_PTR KeyCode);
  virtual void Change();
  virtual void Init();
  virtual bool CloseQuery();
  UnicodeString GetMsg(intptr_t MsgId);
  void GetNextItemPosition(intptr_t & Left, intptr_t & Top);
  void RefreshBounds();
  virtual void Idle();
  void BreakSynchronize();
  void Synchronize(TThreadMethod Method);
  void Close(TFarButton * Button);
  void ProcessGroup(intptr_t Group, TFarProcessGroupEvent Callback, void * Arg);
  void ShowItem(TFarDialogItem * Item, void * Arg);
  void EnableItem(TFarDialogItem * Item, void * Arg);
  bool ChangesLocked();
  TFarDialogItem * ItemAt(int X, int Y);

  static LONG_PTR WINAPI DialogProcGeneral(HANDLE Handle, int Msg, int Param1, LONG_PTR Param2);

  virtual void SetBounds(const TRect & Value);

private:
  TCustomFarPlugin * FFarPlugin;
  TRect FBounds;
  DWORD FFlags;
  UnicodeString FHelpTopic;
  bool FVisible;
  TObjectList * FItems;
  TObjectList * FContainers;
  HANDLE FHandle;
  TFarButton * FDefaultButton;
  TFarBox * FBorderBox;
  TItemPosition FNextItemPosition;
  intptr_t FDefaultGroup;
  intptr_t FTag;
  TFarDialogItem * FItemFocused;
  TFarKeyEvent FOnKey;
  FarDialogItem * FDialogItems;
  intptr_t FDialogItemsCapacity;
  intptr_t FChangesLocked;
  bool FChangesPending;
  intptr_t FResult;
  bool FNeedsSynchronize;
  HANDLE FSynchronizeObjects[2];
  TThreadMethod FSynchronizeMethod;
};

class TFarDialogContainer : public TObject
{
friend TFarDialog;
friend TFarDialogItem;
NB_DISABLE_COPY(TFarDialogContainer)
NB_DECLARE_CLASS(TFarDialogContainer)
public:
  intptr_t GetLeft() const { return FLeft; }
  void SetLeft(intptr_t Value) { SetPosition(0, Value); }
  intptr_t GetTop() const { return FTop; }
  void SetTop(intptr_t Value) { SetPosition(1, Value); }
  bool GetEnabled() const { return FEnabled; }
  void SetEnabled(bool Value);
  void SetPosition(intptr_t AIndex, intptr_t Value);
  intptr_t GetItemCount() const;

protected:
  explicit TFarDialogContainer(TFarDialog * ADialog);
  virtual ~TFarDialogContainer();

  TFarDialog * GetDialog() const { return FDialog; }
  TFarDialog * GetDialog() { return FDialog; }

  void Add(TFarDialogItem * Item);
  void Remove(TFarDialogItem * Item);
  virtual void Change();
  UnicodeString GetMsg(int MsgId);

private:
  intptr_t FLeft;
  intptr_t FTop;
  TObjectList * FItems;
  TFarDialog * FDialog;
  bool FEnabled;
};

#define DIF_INVERSE 0x00000001UL

class TFarDialogItem : public TObject
{
friend TFarDialog;
friend TFarMessageDialog;
friend TFarDialogContainer;
friend TFarList;
NB_DISABLE_COPY(TFarDialogItem)
NB_DECLARE_CLASS(TFarDialogItem)
public:
  TRect GetBounds() const { return FBounds; }
  TRect GetActualBounds() const;
  intptr_t GetLeft() const { return GetCoordinate(0); }
  void SetLeft(intptr_t Value) { SetCoordinate(0, Value); }
  intptr_t GetTop() const { return GetCoordinate(1); }
  void SetTop(intptr_t Value) { SetCoordinate(1, Value); }
  intptr_t GetRight() const { return GetCoordinate(2); }
  void SetRight(intptr_t Value) { SetCoordinate(2, Value); }
  intptr_t GetBottom() const { return GetCoordinate(3); }
  void SetBottom(intptr_t Value) { SetCoordinate(3, Value); }
  intptr_t GetWidth() const;
  void SetWidth(intptr_t Value);
  intptr_t GetHeight() const;
  void SetHeight(intptr_t Value);
  bool GetEnabled() const { return FEnabled; }
  void SetEnabled(bool Value);
  bool GetIsEnabled() const { return FIsEnabled; }
  TFarDialogItem * GetEnabledFollow() const { return FEnabledFollow; }
  void SetEnabledFollow(TFarDialogItem * Value);
  TFarDialogItem * GetEnabledDependency() const { return FEnabledDependency; }
  void SetEnabledDependency(TFarDialogItem * Value);
  TFarDialogItem * GetEnabledDependencyNegative() const { return FEnabledDependencyNegative; }
  void SetEnabledDependencyNegative(TFarDialogItem * Value);
  virtual bool GetIsEmpty() const;
  intptr_t GetGroup() const { return FGroup; }
  void SetGroup(intptr_t Value) { FGroup = Value; }
  bool GetVisible() const { return GetFlag(DIF_HIDDEN | DIF_INVERSE); }
  void SetVisible(bool Value) { SetFlag(DIF_HIDDEN | DIF_INVERSE, Value); }
  bool GetTabStop() const { return GetFlag(DIF_NOFOCUS | DIF_INVERSE); }
  void SetTabStop(bool Value) { SetFlag(DIF_NOFOCUS | DIF_INVERSE, Value); }
  intptr_t GetTag() const { return FTag; }
  void SetTag(intptr_t Value) { FTag = Value; }
  TFarDialog * GetDialog() const { return FDialog; }
  TFarDialog * GetDialog() { return FDialog; }
  const TFarDialog * GetOwner() const { return FDialog; }
  TFarDialog * GetOwner() { return FDialog; }

  TNotifyEvent & GetOnExit() { return FOnExit; }
  void SetOnExit(TNotifyEvent Value) { FOnExit = Value; }
  TFarMouseClickEvent & GetOnMouseClick() { return FOnMouseClick; }
  void SetOnMouseClick(TFarMouseClickEvent Value) { FOnMouseClick = Value; }
  bool GetFocused() const;
  void SetFocused(bool Value);

  void Move(intptr_t DeltaX, intptr_t DeltaY);
  void MoveAt(intptr_t X, intptr_t Y);
  virtual bool CanFocus() const;
  bool Focused() const;
  void SetFocus();
  void SetItem(intptr_t Value) { FItem = Value; }

public:
  virtual void SetDataInternal(const UnicodeString & Value);
  void UpdateData(const UnicodeString & Value);
  void UpdateSelected(intptr_t Value);

  bool GetFlag(intptr_t Index) const;
  void SetFlag(intptr_t Index, bool Value);

  virtual void DoFocus();
  virtual void DoExit();

  char GetColor(intptr_t Index) const;
  void SetColor(intptr_t Index, char Value);

protected:
  uintptr_t FDefaultType;
  intptr_t FGroup;
  intptr_t FTag;
  TNotifyEvent FOnExit;
  TFarMouseClickEvent FOnMouseClick;

  explicit TFarDialogItem(TFarDialog * ADialog, uintptr_t AType);
  virtual ~TFarDialogItem();

  const FarDialogItem * GetDialogItem() const;
  FarDialogItem * GetDialogItem();
  bool GetCenterGroup() const { return GetFlag(DIF_CENTERGROUP); }
  void SetCenterGroup(bool Value) { SetFlag(DIF_CENTERGROUP, Value); }
  virtual UnicodeString GetData() const;
  virtual UnicodeString GetData();
  virtual void SetData(const UnicodeString & Value);
  intptr_t GetType() const;
  void SetType(intptr_t Value);
  intptr_t GetItem() const { return FItem; }
  intptr_t GetSelected() const;
  void SetSelected(intptr_t Value);
  TFarDialogContainer * GetContainer() const { return FContainer; }
  void SetContainer(TFarDialogContainer * Value);
  bool GetChecked() const;
  void SetChecked(bool Value);
  void SetBounds(const TRect & Value);
  DWORD GetFlags() const;
  void SetFlags(DWORD Value);
  void UpdateFlags(DWORD Value);
  intptr_t GetCoordinate(intptr_t Index) const;
  void SetCoordinate(intptr_t Index, intptr_t Value);
  TFarDialogItem * GetPrevItem() const;
  void UpdateFocused(bool Value);
  void UpdateEnabled();

  virtual void Detach();
  void DialogResized();
  LONG_PTR SendMessage(int Msg, LONG_PTR Param);
  LONG_PTR SendDialogMessage(int Msg, intptr_t Param1, LONG_PTR Param2);
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  LONG_PTR DefaultItemProc(int Msg, LONG_PTR Param);
  LONG_PTR DefaultDialogProc(int Msg, intptr_t Param1, LONG_PTR Param2);
  virtual LONG_PTR FailItemProc(int Msg, LONG_PTR Param);
  virtual void Change();
  void DialogChange();
  bool GetAlterType(intptr_t Index) const;
  bool GetAlterType(intptr_t Index);
  void SetAlterType(intptr_t Index, bool Value);
  virtual void UpdateBounds();
  virtual void ResetBounds();
  virtual void Init();
  virtual bool CloseQuery();
  virtual bool MouseMove(int X, int Y, MOUSE_EVENT_RECORD * Event);
  virtual bool MouseClick(MOUSE_EVENT_RECORD * Event);
  TPoint MouseClientPosition(MOUSE_EVENT_RECORD * Event);
  void Text(int X, int Y, uintptr_t Color, const UnicodeString & Str);
  void Redraw();
  virtual bool HotKey(char HotKey);

private:
  const struct PluginStartupInfo * GetPluginStartupInfo() const;

private:
  TFarDialog * FDialog;
  TRect FBounds;
  TFarDialogItem * FEnabledFollow;
  TFarDialogItem * FEnabledDependency;
  TFarDialogItem * FEnabledDependencyNegative;
  TFarDialogContainer * FContainer;
  intptr_t FItem;
  bool FEnabled;
  bool FIsEnabled;
  uint32_t FColors;
  uint32_t FColorMask;
};

class TFarBox : public TFarDialogItem
{
public:
  explicit TFarBox(TFarDialog * ADialog);

  virtual UnicodeString GetCaption() const { return GetData(); }
  virtual void SetCaption(const UnicodeString & Value) { SetData(Value); }
  virtual bool GetDouble() const { return GetAlterType(DI_DOUBLEBOX); }
  virtual void SetDouble(bool Value) { SetAlterType(DI_DOUBLEBOX, Value); }
};

DEFINE_CALLBACK_TYPE2(TFarButtonClickEvent, void,
  TFarButton * /*Sender*/, bool & /*Close*/);
enum TFarButtonBrackets
{
  brNone,
  brTight,
  brSpace,
  brNormal
};

class TFarButton : public TFarDialogItem
{
NB_DECLARE_CLASS(TFarButton)
public:
  explicit TFarButton(TFarDialog * ADialog);
  virtual ~TFarButton() {}

  virtual UnicodeString GetCaption() const { return GetData(); }
  virtual void SetCaption(const UnicodeString & Value) { SetData(Value); }
  virtual intptr_t GetModalResult() const { return FResult; }
  virtual intptr_t GetResult() const { return FResult; }
  virtual void SetResult(intptr_t Value) { FResult = Value; }
  virtual UnicodeString GetData() const { return const_cast<TFarButton *>(this)->GetData(); }
  virtual UnicodeString GetData();
  bool GetDefault() const;
  void SetDefault(bool Value);
  TFarButtonBrackets GetBrackets() const { return FBrackets; }
  void SetBrackets(TFarButtonBrackets Value);
  bool GetCenterGroup() const { return TFarDialogItem::GetCenterGroup(); }
  void SetCenterGroup(bool Value) { TFarDialogItem::SetCenterGroup(Value); }
  virtual TFarButtonClickEvent & GetOnClick() { return FOnClick; }
  virtual void SetOnClick(TFarButtonClickEvent Value) { FOnClick = Value; }

protected:
  virtual void SetDataInternal(const UnicodeString & AValue);
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  virtual bool HotKey(char HotKey);

private:
  intptr_t FResult;
  TFarButtonClickEvent FOnClick;
  TFarButtonBrackets FBrackets;
};

DEFINE_CALLBACK_TYPE3(TFarAllowChangeEvent, void,
  TFarDialogItem * /*Sender*/, intptr_t /*NewState*/, bool & /*AllowChange*/);

class TFarCheckBox : public TFarDialogItem
{
NB_DISABLE_COPY(TFarCheckBox)
NB_DECLARE_CLASS(TFarCheckBox)
public:
  explicit TFarCheckBox(TFarDialog * ADialog);

  virtual UnicodeString GetCaption() const { return GetData(); }
  virtual void SetCaption(const UnicodeString & Value) { SetData(Value); }
  bool GetAllowGrayed() const { return GetFlag(DIF_3STATE); }
  void SetAllowGrayed(bool Value) { SetFlag(DIF_3STATE, Value); }
  virtual TFarAllowChangeEvent & GetOnAllowChange() { return FOnAllowChange; }
  virtual void SetOnAllowChange(TFarAllowChangeEvent Value) { FOnAllowChange = Value; }
  bool GetChecked() const { return TFarDialogItem::GetChecked(); }
  void SetChecked(bool Value) { TFarDialogItem::SetChecked(Value); }
  intptr_t GetSelected() const { return TFarDialogItem::GetSelected(); }
  void SetSelected(intptr_t Value) { TFarDialogItem::SetSelected(Value); }

protected:
  TFarAllowChangeEvent FOnAllowChange;
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  virtual bool GetIsEmpty() const;
  virtual void SetData(const UnicodeString & Value);
};

class TFarRadioButton : public TFarDialogItem
{
public:
  explicit TFarRadioButton(TFarDialog * ADialog);

  bool GetChecked() const { return TFarDialogItem::GetChecked(); }
  void SetChecked(bool Value) { TFarDialogItem::SetChecked(Value); }
  virtual UnicodeString GetCaption() const { return GetData(); }
  virtual void SetCaption(const UnicodeString & Value) { SetData(Value); }
  virtual TFarAllowChangeEvent & GetOnAllowChange() { return FOnAllowChange; }
  virtual void SetOnAllowChange(TFarAllowChangeEvent Value) { FOnAllowChange = Value; }

protected:
  TFarAllowChangeEvent FOnAllowChange;
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  virtual bool GetIsEmpty() const;
  virtual void SetData(const UnicodeString & Value);
};

class TFarEdit : public TFarDialogItem
{
NB_DECLARE_CLASS(TFarEdit)
public:
  explicit TFarEdit(TFarDialog * ADialog);

  virtual UnicodeString GetText() const { return GetData(); }
  virtual void SetText(const UnicodeString & Value) { SetData(Value); }
  intptr_t GetAsInteger();
  void SetAsInteger(intptr_t Value);
  virtual bool GetPassword() const { return GetAlterType(DI_PSWEDIT); }
  virtual void SetPassword(bool Value) { SetAlterType(DI_PSWEDIT, Value); }
  virtual bool GetFixed() const { return GetAlterType(DI_FIXEDIT); }
  virtual void SetFixed(bool Value) { SetAlterType(DI_FIXEDIT, Value); }
  virtual UnicodeString GetMask() const { return GetHistoryMask(1); }
  virtual void SetMask(const UnicodeString & Value) { SetHistoryMask(1, Value); }
  virtual UnicodeString GetHistory() const { return GetHistoryMask(0); }
  virtual void SetHistory(const UnicodeString & Value) { SetHistoryMask(0, Value); }
  bool GetExpandEnvVars() const { return GetFlag(DIF_EDITEXPAND); }
  void SetExpandEnvVars(bool Value) { SetFlag(DIF_EDITEXPAND, Value); }
  bool GetAutoSelect() const { return GetFlag(DIF_SELECTONENTRY); }
  void SetAutoSelect(bool Value) { SetFlag(DIF_SELECTONENTRY, Value); }
  bool GetReadOnly() const { return GetFlag(DIF_READONLY); }
  void SetReadOnly(bool Value) { SetFlag(DIF_READONLY, Value); }

protected:
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  virtual void Detach();

private:
  UnicodeString GetHistoryMask(size_t Index) const;
  void SetHistoryMask(size_t Index, const UnicodeString & Value);
};

class TFarSeparator : public TFarDialogItem
{
public:
  explicit TFarSeparator(TFarDialog * ADialog);

  bool GetDouble();
  void SetDouble(bool Value);
  virtual UnicodeString GetCaption() { return GetData(); }
  virtual void SetCaption(const UnicodeString & Value) { SetData(Value); }
  void SetPosition(intptr_t Value);
  int GetPosition();

protected:
  virtual void ResetBounds();
};

class TFarText : public TFarDialogItem
{
NB_DECLARE_CLASS(TFarText)
public:
  explicit TFarText(TFarDialog * ADialog);

  virtual UnicodeString GetCaption() const { return GetData(); }
  virtual void SetCaption(const UnicodeString & Value) { SetData(Value); }
  bool GetCenterGroup() const { return TFarDialogItem::GetCenterGroup(); }
  void SetCenterGroup(bool Value) { TFarDialogItem::SetCenterGroup(Value); }
  char GetColor() const { return TFarDialogItem::GetColor(0); }
  void SetColor(char Value) { TFarDialogItem::SetColor(0, Value); }

protected:
  virtual void SetData(const UnicodeString & Value);
};

class TFarListBox;
class TFarComboBox;
class TFarLister;

class TFarList : public TStringList
{
friend TFarListBox;
friend TFarLister;
friend TFarComboBox;
NB_DISABLE_COPY(TFarList)
NB_DECLARE_CLASS(TFarList)
public:
  explicit TFarList(TFarDialogItem * ADialogItem = nullptr);
  virtual ~TFarList();

  virtual void Assign(const TPersistent * Source);

  intptr_t GetSelected() const;
  void SetSelected(intptr_t Value);
  intptr_t GetTopIndex() const;
  void SetTopIndex(intptr_t Value);
  inline intptr_t GetSelectedInt(bool Init) const;
  bool GetFlag(intptr_t Index, DWORD Flag) const;
  void SetFlag(intptr_t Index, DWORD Flag, bool Value);
  DWORD GetFlags(intptr_t Index) const;
  void SetFlags(intptr_t Index, DWORD Value);
  intptr_t GetMaxLength() const;
  intptr_t GetVisibleCount() const;
  bool GetDisabled(intptr_t Index) const { return GetFlag(Index, LIF_DISABLE); }
  void SetDisabled(intptr_t Index, bool Value) { SetFlag(Index, LIF_DISABLE, Value); }
  bool GetChecked(intptr_t Index) const { return GetFlag(Index, LIF_CHECKED); }
  void SetChecked(intptr_t Index, bool Value) { SetFlag(Index, LIF_CHECKED, Value); }

protected:
  virtual void Changed();
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  virtual void Init();
  void UpdatePosition(intptr_t Position);
  intptr_t GetPosition() const;
  virtual void Put(intptr_t Index, const UnicodeString & Str);
  void SetCurPos(intptr_t Position, intptr_t TopIndex);
  void UpdateItem(intptr_t Index);

  FarList * GetListItems() const { return FListItems; }
  FarList * GetListItems() { return FListItems; }
  TFarDialogItem * GetDialogItem() const { return FDialogItem; }
  TFarDialogItem * GetDialogItem() { return FDialogItem; }

private:
  FarList * FListItems;
  TFarDialogItem * FDialogItem;
  bool FNoDialogUpdate;
};

enum TFarListBoxAutoSelect
{
  asOnlyFocus,
  asAlways,
  asNever
};

class TFarListBox : public TFarDialogItem
{
NB_DISABLE_COPY(TFarListBox)
NB_DECLARE_CLASS(TFarListBox)
public:
  explicit TFarListBox(TFarDialog * ADialog);
  virtual ~TFarListBox();

  void SetItems(TStrings * Value);

  bool GetNoAmpersand() const { return GetFlag(DIF_LISTNOAMPERSAND); }
  void SetNoAmpersand(bool Value) { SetFlag(DIF_LISTNOAMPERSAND, Value); }
  bool GetAutoHighlight() const { return GetFlag(DIF_LISTAUTOHIGHLIGHT); }
  void SetAutoHighlight(bool Value) { SetFlag(DIF_LISTAUTOHIGHLIGHT, Value); }
  bool GetNoBox() const { return GetFlag(DIF_LISTNOBOX); }
  void SetNoBox(bool Value) { SetFlag(DIF_LISTNOBOX, Value); }
  bool GetWrapMode() const { return GetFlag(DIF_LISTWRAPMODE); }
  void SetWrapMode(bool Value) { SetFlag(DIF_LISTWRAPMODE, Value); }
  TFarList * GetItems() const { return FList; }
  TFarList * GetItems() { return FList; }
  void SetList(TFarList * Value);
  TFarListBoxAutoSelect GetAutoSelect() { return FAutoSelect; }
  void SetAutoSelect(TFarListBoxAutoSelect Value);

protected:
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  virtual void Init();
  virtual bool CloseQuery();

private:
  void UpdateMouseReaction();

private:
  TFarList * FList;
  TFarListBoxAutoSelect FAutoSelect;
  bool FDenyClose;
};

class TFarComboBox : public TFarDialogItem
{
NB_DISABLE_COPY(TFarComboBox)
public:
  explicit TFarComboBox(TFarDialog * ADialog);
  virtual ~TFarComboBox();

  void ResizeToFitContent();

  bool GetNoAmpersand() const { return GetFlag(DIF_LISTNOAMPERSAND); }
  void SetNoAmpersand(bool Value) { SetFlag(DIF_LISTNOAMPERSAND, Value); }
  bool GetAutoHighlight() const { return GetFlag(DIF_LISTAUTOHIGHLIGHT); }
  void SetAutoHighlight(bool Value) { SetFlag(DIF_LISTAUTOHIGHLIGHT, Value); }
  bool GetWrapMode() const { return GetFlag(DIF_LISTWRAPMODE); }
  void SetWrapMode(bool Value) { SetFlag(DIF_LISTWRAPMODE, Value); }
  TFarList * GetItems() const { return FList; }
  virtual UnicodeString GetText() const { return GetData(); }
  virtual void SetText(const UnicodeString & Value) { SetData(Value); }
  bool GetAutoSelect() const { return GetFlag(DIF_SELECTONENTRY); }
  void SetAutoSelect(bool Value) { SetFlag(DIF_SELECTONENTRY, Value); }
  bool GetDropDownList() const { return GetFlag(DIF_DROPDOWNLIST); }
  void SetDropDownList(bool Value) { SetFlag(DIF_DROPDOWNLIST, Value); }
  intptr_t GetItemIndex() const { return FList->GetSelected(); }
  void SetItemIndex(intptr_t Index) { FList->SetSelected(Index); }

protected:
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  virtual void Init();

private:
  TFarList * FList;
};

class TFarLister : public TFarDialogItem
{
NB_DISABLE_COPY(TFarLister)
public:
  explicit TFarLister(TFarDialog * ADialog);
  virtual ~TFarLister();

  TStrings * GetItems() const;
  void SetItems(const TStrings * Value);
  intptr_t GetTopIndex() const { return FTopIndex; }
  void SetTopIndex(intptr_t Value);
  bool GetScrollBar() const;

protected:
  virtual LONG_PTR ItemProc(int Msg, LONG_PTR Param);
  virtual void DoFocus();

private:
  void ItemsChange(TObject * Sender);

private:
  TStringList * FItems;
  intptr_t FTopIndex;
};

inline TRect Rect(int Left, int Top, int Right, int Bottom);

