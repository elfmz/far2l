#include <vcl.h>
#pragma hdrstop

#include <map>

#pragma warning(push, 1)
#include <farkeys.hpp>
#include <farcolor.hpp>
#pragma warning(pop)

#include <Common.h>

#include "FarDialog.h"

inline TRect Rect(int Left, int Top, int Right, int Bottom)
{
  return TRect(Left, Top, Right, Bottom);
}

TFarDialog::TFarDialog(TCustomFarPlugin * AFarPlugin) :
  TObject(),
  FFarPlugin(AFarPlugin),
  FBounds(-1, -1, 40, 10),
  FFlags(0),
  FHelpTopic(),
  FVisible(false),
  FItems(new TObjectList()),
  FContainers(new TObjectList()),
  FHandle(0),
  FDefaultButton(nullptr),
  FBorderBox(nullptr),
  FNextItemPosition(ipNewLine),
  FDefaultGroup(0),
  FTag(0),
  FItemFocused(nullptr),
  FOnKey(nullptr),
  FDialogItems(nullptr),
  FDialogItemsCapacity(0),
  FChangesLocked(0),
  FChangesPending(false),
  FResult(-1),
  FNeedsSynchronize(false),
  FSynchronizeMethod(nullptr)
{
  DebugAssert(AFarPlugin);
  FSynchronizeObjects[0] = INVALID_HANDLE_VALUE;
  FSynchronizeObjects[1] = INVALID_HANDLE_VALUE;

  FBorderBox = new TFarBox(this);
  FBorderBox->SetBounds(TRect(3, 1, -4, -2));
  FBorderBox->SetDouble(true);
}

TFarDialog::~TFarDialog()
{
  for (intptr_t Index = 0; Index < GetItemCount(); ++Index)
  {
    GetItem(Index)->Detach();
  }
  SAFE_DESTROY(FItems);
  nb_free(FDialogItems);
  FDialogItemsCapacity = 0;
  SAFE_DESTROY(FContainers);
  if (FSynchronizeObjects[0] != INVALID_HANDLE_VALUE)
  {
    ::CloseHandle(FSynchronizeObjects[0]);
  }
  if (FSynchronizeObjects[1] != INVALID_HANDLE_VALUE)
  {
    ::CloseHandle(FSynchronizeObjects[1]);
  }
}

void TFarDialog::SetBounds(const TRect & Value)
{
  if (GetBounds() != Value)
  {
    LockChanges();
    {
      SCOPE_EXIT
      {
        UnlockChanges();
      };
      FBounds = Value;
      if (GetHandle())
      {
        COORD Coord;
        Coord.X = static_cast<short int>(GetSize().x);
        Coord.Y = static_cast<short int>(GetSize().y);
        SendDlgMessage(DM_RESIZEDIALOG, 0, reinterpret_cast<LONG_PTR>(&Coord));
        Coord.X = static_cast<short int>(FBounds.Left);
        Coord.Y = static_cast<short int>(FBounds.Top);
        SendDlgMessage(DM_MOVEDIALOG, (int)true, reinterpret_cast<LONG_PTR>(&Coord));
      }
      for (intptr_t Index = 0; Index < GetItemCount(); ++Index)
      {
        GetItem(Index)->DialogResized();
      }
    }
  }
}

TRect TFarDialog::GetClientRect() const
{
  TRect R;
  if (FBorderBox)
  {
    R = FBorderBox->GetBounds();
    R.Left += 2;
    R.Right -= 2;
    R.Top++;
    R.Bottom--;
  }
  else
  {
    R.Left = 0;
    R.Top = 0;
    R.Bottom = 0;
    R.Right = 0;
  }
  return R;
}

TPoint TFarDialog::GetClientSize() const
{
  TPoint S;
  if (FBorderBox)
  {
    TRect R = FBorderBox->GetActualBounds();
    S.x = R.Width() + 1;
    S.y = R.Height() + 1;
    S.x -= S.x > 4 ? 4 : S.x;
    S.y -= S.y > 2 ? 2 : S.y;
  }
  else
  {
    S = GetSize();
  }
  return S;
}

TPoint TFarDialog::GetMaxSize()
{
  TPoint P = GetFarPlugin()->TerminalInfo();
  P.x -= 2;
  P.y -= 3;
  return P;
}

void TFarDialog::SetHelpTopic(const UnicodeString & Value)
{
  if (FHelpTopic != Value)
  {
    DebugAssert(!GetHandle());
    FHelpTopic = Value;
  }
}

void TFarDialog::SetFlags(DWORD Value)
{
  if (GetFlags() != Value)
  {
    DebugAssert(!GetHandle());
    FFlags = Value;
  }
}

void TFarDialog::SetCentered(bool Value)
{
  if (GetCentered() != Value)
  {
    DebugAssert(!GetHandle());
    TRect B = GetBounds();
    B.Left = Value ? -1 : 0;
    B.Top = Value ? -1 : 0;
    SetBounds(B);
  }
}

bool TFarDialog::GetCentered() const
{
  return (GetBounds().Left < 0) && (GetBounds().Top < 0);
}

TPoint TFarDialog::GetSize() const
{
  if (GetCentered())
  {
    return TPoint(GetBounds().Right, GetBounds().Bottom);
  }
  else
  {
    return TPoint(GetBounds().Width() + 1, GetBounds().Height() + 1);
  }
}

void TFarDialog::SetSize(TPoint Value)
{
  TRect B = GetBounds();
  if (GetCentered())
  {
    B.Right = Value.x;
    B.Bottom = Value.y;
  }
  else
  {
    B.Right = FBounds.Left + Value.x - 1;
    B.Bottom = FBounds.Top + Value.y - 1;
  }
  SetBounds(B);
}

void TFarDialog::SetWidth(intptr_t Value)
{
  SetSize(TPoint((int)Value, (int)GetHeight()));
}

intptr_t TFarDialog::GetWidth() const
{
  return GetSize().x;
}

void TFarDialog::SetHeight(intptr_t Value)
{
  SetSize(TPoint((int)GetWidth(), (int)Value));
}

intptr_t TFarDialog::GetHeight() const
{
  return GetSize().y;
}

void TFarDialog::SetCaption(const UnicodeString & Value)
{
  if (GetCaption() != Value)
  {
    FBorderBox->SetCaption(Value);
  }
}

UnicodeString TFarDialog::GetCaption() const
{
  return FBorderBox->GetCaption();
}

intptr_t TFarDialog::GetItemCount() const
{
  return FItems->GetCount();
}

intptr_t TFarDialog::GetItem(TFarDialogItem * Item) const
{
  if (!Item)
    return -1;
  return Item->GetItem();
}

TFarDialogItem * TFarDialog::GetItem(intptr_t Index) const
{
  TFarDialogItem * DialogItem;
  if (GetItemCount())
  {
    DebugAssert(Index >= 0 && Index < FItems->GetCount());
    DialogItem = NB_STATIC_DOWNCAST(TFarDialogItem, (*GetItems())[Index]);
    DebugAssert(DialogItem);
  }
  else
  {
    DialogItem = nullptr;
  }
  return DialogItem;
}

void TFarDialog::Add(TFarDialogItem * DialogItem)
{
  TRect R = GetClientRect();
  intptr_t Left, Top;
  GetNextItemPosition(Left, Top);
  R.Left = static_cast<int>(Left);
  R.Top = static_cast<int>(Top);

  if (FDialogItemsCapacity == GetItems()->GetCount())
  {
    int DialogItemsDelta = 10;
    FarDialogItem * NewDialogItems;
    NewDialogItems = static_cast<FarDialogItem *>(
      nb_malloc(sizeof(FarDialogItem) * (GetItems()->GetCount() + DialogItemsDelta)));
    if (FDialogItems)
    {
      memmove(NewDialogItems, FDialogItems, FDialogItemsCapacity * sizeof(FarDialogItem));
      nb_free(FDialogItems);
    }
    ::memset(NewDialogItems + FDialogItemsCapacity, 0, DialogItemsDelta * sizeof(FarDialogItem));
    FDialogItems = NewDialogItems;
    FDialogItemsCapacity += DialogItemsDelta;
  }

  DebugAssert(DialogItem);
  DialogItem->SetItem(GetItems()->Add(DialogItem));

  R.Bottom = R.Top;
  DialogItem->SetBounds(R);
  DialogItem->SetGroup(GetDefaultGroup());
}

void TFarDialog::Add(TFarDialogContainer * Container)
{
  FContainers->Add(Container);
}

void TFarDialog::GetNextItemPosition(intptr_t & Left, intptr_t & Top)
{
  TRect R = GetClientRect();
  Left = R.Left;
  Top = R.Top;

  TFarDialogItem * LastItem = GetItem(GetItemCount() - 1);
  LastItem = LastItem == FBorderBox ? nullptr : LastItem;

  if (LastItem)
  {
    switch (GetNextItemPosition())
    {
      case ipNewLine:
        Top = LastItem->GetBottom() + 1;
        break;

      case ipBelow:
        Top = LastItem->GetBottom() + 1;
        Left = LastItem->GetLeft();
        break;

      case ipRight:
        Top = LastItem->GetTop();
        Left = LastItem->GetRight() + 3;
        break;
    }
  }
}

LONG_PTR WINAPI TFarDialog::DialogProcGeneral(HANDLE Handle, int Msg, int Param1, LONG_PTR Param2)
{
  TFarPluginEnvGuard Guard;

  static std::map<HANDLE, LONG_PTR> Dialogs;
  TFarDialog * Dialog = nullptr;
  LONG_PTR Result = 0;
  if (Msg == DN_INITDIALOG)
  {
    DebugAssert(Dialogs.find(Handle) == Dialogs.end());
    Dialogs[Handle] = Param2;
    Dialog = reinterpret_cast<TFarDialog *>(Param2);
    Dialog->FHandle = Handle;
  }
  else
  {
    if (Dialogs.find(Handle) == Dialogs.end())
    {
      // DM_CLOSE is sent after DN_CLOSE, if the dialog was closed programmatically
      // by SendMessage(DM_CLOSE, ...)
      DebugAssert(Msg == DM_CLOSE);
      Result = static_cast<LONG_PTR>(0);
    }
    else
    {
      Dialog = reinterpret_cast<TFarDialog *>(Dialogs[Handle]);
    }
  }

  if (Dialog != nullptr)
  {
    Result = Dialog->DialogProc(Msg, static_cast<intptr_t>(Param1), Param2);
  }

  if ((Msg == DN_CLOSE) && Result)
  {
    if (Dialog != nullptr)
    {
        Dialog->FHandle = 0;
    }
    Dialogs.erase(Handle);
  }
  return Result;
}

LONG_PTR TFarDialog::DialogProc(int Msg, intptr_t Param1, LONG_PTR Param2)
{
  intptr_t Result = 0;
  bool Handled = false;

  try
  {
    if (FNeedsSynchronize)
    {
      try
      {
        FNeedsSynchronize = false;
        FSynchronizeMethod();
        ::ReleaseSemaphore(FSynchronizeObjects[0], 1, nullptr);
        BreakSynchronize();
      }
      catch (...)
      {
        DebugAssert(false);
      }
    }

    bool Changed = false;

    switch (Msg)
    {
      case DN_BTNCLICK:
      case DN_EDITCHANGE:
      case DN_GOTFOCUS:
      case DN_KILLFOCUS:
      case DN_LISTCHANGE:
        Changed = true;

      case DN_MOUSECLICK:
      case DN_CTLCOLORDLGITEM:
      case DN_CTLCOLORDLGLIST:
      case DN_DRAWDLGITEM:
      case DN_HOTKEY:
      case DN_KEY:
        if (Param1 >= 0)
        {
          TFarDialogItem * Item = GetItem(Param1);
          try
          {
            Result = Item->ItemProc(Msg, Param2);
          }
          catch (Exception & E)
          {
            Handled = true;
            DEBUG_PRINTF("before GetFarPlugin()->HandleException");
            GetFarPlugin()->HandleException(&E);
            Result = Item->FailItemProc(Msg, Param2);
          }

          if (!Result && (Msg == DN_KEY))
          {
            Result = Key(Item, Param2);
          }
          Handled = true;
        }

        // FAR WORKAROUND
        // When pressing Enter FAR forces dialog to close without calling
        // DN_BTNCLICK on default button. This fixes the scenario.
        // (first check if focused dialog item is not another button)
        if (!Result && (Msg == DN_KEY) &&
            (Param2 == KEY_ENTER) &&
            ((Param1 < 0) ||
             ((NB_STATIC_DOWNCAST(TFarButton, GetItem(Param1)) == nullptr))) &&
            GetDefaultButton()->GetEnabled() &&
            (GetDefaultButton()->GetOnClick()))
        {
          bool Close = (GetDefaultButton()->GetResult() != 0);
          GetDefaultButton()->GetOnClick()(GetDefaultButton(), Close);
          Handled = true;
          if (!Close)
          {
            Result = 1;
          }
        }
        break;

      case DN_MOUSEEVENT:
        Result = MouseEvent(reinterpret_cast<MOUSE_EVENT_RECORD *>(Param2));
        Handled = true;
        break;
    }
    if (!Handled)
    {
      switch (Msg)
      {
        case DN_INITDIALOG:
          Init();
          Result = 1;
          break;

        case DN_DRAGGED:
          if (Param1 == 1)
          {
            RefreshBounds();
          }
          break;

        case DN_DRAWDIALOG:
          // before drawing the dialog, make sure we know correct coordinates
          // (especially while the dialog is being dragged)
          RefreshBounds();
          break;

        case DN_CLOSE:
          Result = 1;
          if (Param1 >= 0)
          {
            TFarButton * Button = NB_STATIC_DOWNCAST(TFarButton, GetItem(Param1));
            // FAR WORKAROUND
            // FAR 1.70 alpha 6 calls DN_CLOSE even for non-button dialog items
            // (list boxes in particular), while FAR 1.70 beta 5 used ID of
            // default button in such case.
            // Particularly for listbox, we can prevent closing dialog using
            // flag DIF_LISTNOCLOSE.
            if (Button == nullptr)
            {
              DebugAssert(NB_STATIC_DOWNCAST(TFarListBox, GetItem(Param1)) != nullptr);
              Result = static_cast<intptr_t>(false);
            }
            else
            {
              FResult = Button->GetResult();
            }
          }
          else
          {
            FResult = -1;
          }
          if (Result)
          {
            Result = CloseQuery();
            if (!Result)
            {
              FResult = -1;
            }
          }
          Handled = true;
          break;

        case DN_ENTERIDLE:
          Idle();
          break;
      }

      if (!Handled)
      {
        Result = DefaultDialogProc(Msg, Param1, Param2);
      }
    }
    if (Changed)
    {
      Change();
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before GetFarPlugin()->HandleException");
    GetFarPlugin()->HandleException(&E);
    if (!Handled)
    {
      Result = FailDialogProc(Msg, Param1, Param2);
    }
  }
  return Result;
}

LONG_PTR TFarDialog::DefaultDialogProc(int Msg, intptr_t Param1, LONG_PTR Param2)
{
  if (GetHandle())
  {
    TFarEnvGuard Guard;
    return GetFarPlugin()->GetPluginStartupInfo()->DefDlgProc(GetHandle(), Msg, static_cast<int>(Param1), Param2);
  }
  return 0;
}

LONG_PTR TFarDialog::FailDialogProc(int Msg, intptr_t Param1, LONG_PTR Param2)
{
  intptr_t Result = 0;
  switch (Msg)
  {
    case DN_CLOSE:
      Result = 0;
      break;

    default:
      Result = DefaultDialogProc(Msg, Param1, Param2);
      break;
  }
  return Result;
}

void TFarDialog::Idle()
{
  // nothing
}

bool TFarDialog::MouseEvent(MOUSE_EVENT_RECORD * Event)
{
  bool Result = true;
  bool Handled = false;
  if (FLAGSET(Event->dwEventFlags, MOUSE_MOVED))
  {
    int X = Event->dwMousePosition.X - GetBounds().Left;
    int Y = Event->dwMousePosition.Y - GetBounds().Top;
    TFarDialogItem * Item = ItemAt(X, Y);
    if (Item != nullptr)
    {
      Result = Item->MouseMove(X, Y, Event);
      Handled = true;
    }
  }
  else
  {
    Handled = false;
  }

  if (!Handled)
  {
    Result = DefaultDialogProc(DN_MOUSEEVENT, 0, reinterpret_cast<intptr_t>(Event)) != 0;
  }

  return Result;
}

bool TFarDialog::Key(TFarDialogItem * Item, LONG_PTR KeyCode)
{
  bool Result = false;
  if (FOnKey)
  {
    FOnKey(this, Item, static_cast<long>(KeyCode), Result);
  }
  return Result;
}

bool TFarDialog::HotKey(uintptr_t Key) const
{
  bool Result = false;
  char HotKey = 0;
  if ((KEY_ALTA <= Key) && (Key <= KEY_ALTZ))
  {
    Result = true;
    HotKey = static_cast<char>('a' + static_cast<char>(Key - KEY_ALTA));
  }

  if (Result)
  {
    Result = false;
    for (intptr_t Index = 0; Index < GetItemCount(); ++Index)
    {
      if (GetItem(Index)->HotKey(HotKey))
      {
        Result = true;
      }
    }
  }

  return Result;
}

TFarDialogItem * TFarDialog::ItemAt(int X, int Y)
{
  TFarDialogItem * Result = nullptr;
  for (intptr_t Index = 0; Index < GetItemCount(); ++Index)
  {
    TRect Bounds = GetItem(Index)->GetActualBounds();
    if ((Bounds.Left <= X) && (X <= Bounds.Right) &&
        (Bounds.Top <= Y) && (Y <= Bounds.Bottom))
    {
      Result = GetItem(Index);
    }
  }
  return Result;
}

bool TFarDialog::CloseQuery()
{
  bool Result = true;
  for (intptr_t Index = 0; Index < GetItemCount() && Result; ++Index)
  {
    if (!GetItem(Index)->CloseQuery())
    {
      Result = false;
    }
  }
  return Result;
}

void TFarDialog::RefreshBounds()
{
  SMALL_RECT Rect = {0};
  SendDlgMessage(DM_GETDLGRECT, 0, reinterpret_cast<LONG_PTR>(&Rect));
  FBounds.Left = Rect.Left;
  FBounds.Top = Rect.Top;
  FBounds.Right = Rect.Right;
  FBounds.Bottom = Rect.Bottom;
}

void TFarDialog::Init()
{
  for (intptr_t Index = 0; Index < GetItemCount(); ++Index)
  {
    GetItem(Index)->Init();
  }

  RefreshBounds();

  Change();
}

intptr_t TFarDialog::ShowModal()
{
  FResult = -1;

  TFarDialog * PrevTopDialog = GetFarPlugin()->FTopDialog;
  GetFarPlugin()->FTopDialog = this;
  HANDLE Handle = INVALID_HANDLE_VALUE;
  {
    SCOPE_EXIT
    {
      GetFarPlugin()->FTopDialog = PrevTopDialog;
      if (Handle != INVALID_HANDLE_VALUE)
      {
        GetFarPlugin()->GetPluginStartupInfo()->DialogFree(Handle);
      }
    };
    DebugAssert(GetDefaultButton());
    DebugAssert(GetDefaultButton()->GetDefault());

    UnicodeString HelpTopic = GetHelpTopic();
    intptr_t BResult = 0;

    {
      TFarEnvGuard Guard;
      TRect Bounds = GetBounds();
      const PluginStartupInfo & Info = *GetFarPlugin()->GetPluginStartupInfo();
      Handle = Info.DialogInit(
        Info.ModuleNumber,
        Bounds.Left, Bounds.Top, Bounds.Right, Bounds.Bottom,
        HelpTopic.c_str(), FDialogItems,
        static_cast<uint32_t>(GetItemCount()),
        0, GetFlags(),
        DialogProcGeneral, reinterpret_cast<LONG_PTR>(this));
      BResult = Info.DialogRun(Handle);
    }

    if (BResult >= 0)
    {
      TFarButton * Button = NB_STATIC_DOWNCAST(TFarButton, GetItem(BResult));
      DebugAssert(Button);
      // correct result should be already set by TFarButton
      DebugAssert(FResult == Button->GetResult());
      FResult = Button->GetResult();
    }
    else
    {
      // allow only one negative value = -1
      FResult = -1;
    }
  }

  return FResult;
}

void TFarDialog::BreakSynchronize()
{
  ::SetEvent(FSynchronizeObjects[1]);
}

void TFarDialog::Synchronize(TThreadMethod Event)
{
  if (FSynchronizeObjects[0] == INVALID_HANDLE_VALUE)
  {
    FSynchronizeObjects[0] = ::CreateSemaphore(nullptr, 0, 2, nullptr);
    FSynchronizeObjects[1] = ::CreateEvent(nullptr, false, false, nullptr);
  }
  FSynchronizeMethod = Event;
  FNeedsSynchronize = true;
  ::WaitForMultipleObjects(_countof(FSynchronizeObjects),
    reinterpret_cast<HANDLE *>(&FSynchronizeObjects), false, INFINITE);
}

void TFarDialog::Close(TFarButton * Button)
{
  DebugAssert(Button != nullptr);
  SendDlgMessage(DM_CLOSE, Button->GetItem(), 0);
}

void TFarDialog::Change()
{
  if (FChangesLocked > 0)
  {
    FChangesPending = true;
  }
  else
  {
    std::unique_ptr<TList> NotifiedContainers(new TList());
    for (intptr_t Index = 0; Index < GetItemCount(); ++Index)
    {
      TFarDialogItem * DItem = GetItem(Index);
      DItem->Change();
      if (DItem->GetContainer() && NotifiedContainers->IndexOf(DItem->GetContainer()) == NPOS)
      {
        NotifiedContainers->Add(DItem->GetContainer());
      }
    }

    for (intptr_t Index = 0; Index < NotifiedContainers->GetCount(); ++Index)
    {
      NB_STATIC_DOWNCAST(TFarDialogContainer, (*NotifiedContainers)[Index])->Change();
    }
  }
}

LONG_PTR TFarDialog::SendDlgMessage(int Msg, intptr_t Param1, LONG_PTR Param2)
{
  if (GetHandle())
  {
    TFarEnvGuard Guard;
    return GetFarPlugin()->GetPluginStartupInfo()->SendDlgMessage(GetHandle(),
      Msg, static_cast<int>(Param1), Param2);
  }
  return 0;
}

uintptr_t TFarDialog::GetSystemColor(intptr_t Index)
{
  return static_cast<uintptr_t>(GetFarPlugin()->FarAdvControl(ACTL_GETCOLOR, ToPtr(Index)));
}

void TFarDialog::Redraw()
{
  SendDlgMessage(DM_REDRAW, 0, 0);
}

void TFarDialog::ShowGroup(intptr_t Group, bool Show)
{
  ProcessGroup(Group, MAKE_CALLBACK(TFarDialog::ShowItem, this), &Show);
}

void TFarDialog::EnableGroup(intptr_t Group, bool Enable)
{
  ProcessGroup(Group, MAKE_CALLBACK(TFarDialog::EnableItem, this), &Enable);
}

void TFarDialog::ProcessGroup(intptr_t Group, TFarProcessGroupEvent Callback,
  void * Arg)
{
  LockChanges();
  {
    SCOPE_EXIT
    {
      UnlockChanges();
    };
    for (intptr_t Index = 0; Index < GetItemCount(); ++Index)
    {
      TFarDialogItem * Item = GetItem(Index);
      if (Item->GetGroup() == Group)
      {
        Callback(Item, Arg);
      }
    }
  }
}

void TFarDialog::ShowItem(TFarDialogItem * Item, void * Arg)
{
  Item->SetVisible(*static_cast<bool *>(Arg));
}

void TFarDialog::EnableItem(TFarDialogItem * Item, void * Arg)
{
  Item->SetEnabled(*static_cast<bool *>(Arg));
}

void TFarDialog::SetItemFocused(TFarDialogItem * Value)
{
  if (Value != GetItemFocused())
  {
    DebugAssert(Value);
    Value->SetFocus();
  }
}

UnicodeString TFarDialog::GetMsg(intptr_t MsgId)
{
  return FFarPlugin->GetMsg(MsgId);
}

void TFarDialog::LockChanges()
{
  DebugAssert(FChangesLocked < 10);
  FChangesLocked++;
  if (FChangesLocked == 1)
  {
    DebugAssert(!FChangesPending);
    if (GetHandle())
    {
      SendDlgMessage(DM_ENABLEREDRAW, static_cast<intptr_t>(false), 0);
    }
  }
}

void TFarDialog::UnlockChanges()
{
  DebugAssert(FChangesLocked > 0);
  FChangesLocked--;
  if (FChangesLocked == 0)
  {
    SCOPE_EXIT
    {
      if (GetHandle())
      {
        this->SendDlgMessage(DM_ENABLEREDRAW, TRUE, 0);
      }
    };
    if (FChangesPending)
    {
      FChangesPending = false;
      Change();
    }
  }
}

bool TFarDialog::ChangesLocked()
{
  return (FChangesLocked > 0);
}

TFarDialogContainer::TFarDialogContainer(TFarDialog * ADialog) :
  TObject(),
  FLeft(0),
  FTop(0),
  FItems(new TObjectList()),
  FDialog(ADialog),
  FEnabled(true)
{
  DebugAssert(ADialog);
  FItems->SetOwnsObjects(false);
  GetDialog()->Add(this);
  GetDialog()->GetNextItemPosition(FLeft, FTop);
}

TFarDialogContainer::~TFarDialogContainer()
{
  SAFE_DESTROY(FItems);
}

UnicodeString TFarDialogContainer::GetMsg(int MsgId)
{
  return GetDialog()->GetMsg(MsgId);
}

void TFarDialogContainer::Add(TFarDialogItem * Item)
{
  DebugAssert(FItems->IndexOf(Item) == NPOS);
  Item->SetContainer(this);
  if (FItems->IndexOf(Item) == NPOS)
    FItems->Add(Item);
}

void TFarDialogContainer::Remove(TFarDialogItem * Item)
{
  DebugAssert(FItems->IndexOf(Item) != NPOS);
  Item->SetContainer(nullptr);
  FItems->Remove(Item);
  if (FItems->GetCount() == 0)
  {
    delete this;
  }
}

void TFarDialogContainer::SetPosition(intptr_t AIndex, intptr_t Value)
{
  intptr_t & Position = AIndex ? FTop : FLeft;
  if (Position != Value)
  {
    Position = Value;
    for (intptr_t Index = 0; Index < GetItemCount(); ++Index)
    {
      NB_STATIC_DOWNCAST(TFarDialogItem, (*FItems)[Index])->DialogResized();
    }
  }
}

void TFarDialogContainer::Change()
{
}

void TFarDialogContainer::SetEnabled(bool Value)
{
  if (FEnabled != Value)
  {
    FEnabled = true;
    for (intptr_t Index = 0; Index < GetItemCount(); ++Index)
    {
      NB_STATIC_DOWNCAST(TFarDialogItem, (*FItems)[Index])->UpdateEnabled();
    }
  }
}

intptr_t TFarDialogContainer::GetItemCount() const
{
  return FItems->GetCount();
}

TFarDialogItem::TFarDialogItem(TFarDialog * ADialog, uintptr_t AType) :
  TObject(),
  FDefaultType(AType),
  FGroup(0),
  FTag(0),
  FOnExit(nullptr),
  FOnMouseClick(nullptr),
  FDialog(ADialog),
  FEnabledFollow(nullptr),
  FEnabledDependency(nullptr),
  FEnabledDependencyNegative(nullptr),
  FContainer(nullptr),
  FItem(NPOS),
  FEnabled(true),
  FIsEnabled(true),
  FColors(0),
  FColorMask(0)
{
  DebugAssert(ADialog);
  GetDialog()->Add(this);

  GetDialogItem()->Type = static_cast<int>(AType);
}

TFarDialogItem::~TFarDialogItem()
{
  TFarDialog * Dlg = GetDialog();
  DebugAssert(!Dlg);
  if (Dlg)
  {
    nb_free((void*)GetDialogItem()->PtrData);
  }
}

const FarDialogItem * TFarDialogItem::GetDialogItem() const
{
  return const_cast<TFarDialogItem *>(this)->GetDialogItem();
}

FarDialogItem * TFarDialogItem::GetDialogItem()
{
  TFarDialog * Dlg = GetDialog();
  DebugAssert(Dlg);
  return &Dlg->FDialogItems[GetItem()];
}

void TFarDialogItem::SetBounds(const TRect & Value)
{
  if (FBounds != Value)
  {
    FBounds = Value;
    UpdateBounds();
  }
}

void TFarDialogItem::Detach()
{
  nb_free((void*)GetDialogItem()->PtrData);
  FDialog = nullptr;
}

void TFarDialogItem::DialogResized()
{
  UpdateBounds();
}

void TFarDialogItem::ResetBounds()
{
  TRect B = FBounds;
  FarDialogItem * DItem = GetDialogItem();
  #define BOUND(DIB, BB, DB, CB) DItem->DIB = B.BB >= 0 ? \
    (GetContainer() ? (int)GetContainer()->CB : 0) + B.BB : GetDialog()->GetSize().DB + B.BB
  BOUND(X1, Left, x, GetLeft());
  BOUND(Y1, Top, y, GetTop());
  BOUND(X2, Right, x, GetLeft());
  BOUND(Y2, Bottom, y, GetTop());
  #undef BOUND
}

void TFarDialogItem::UpdateBounds()
{
  ResetBounds();

  if (GetDialog()->GetHandle())
  {
    TRect B = GetActualBounds();
    SMALL_RECT Rect = {0};
    Rect.Left = static_cast<short int>(B.Left);
    Rect.Top = static_cast<short int>(B.Top);
    Rect.Right = static_cast<short int>(B.Right);
    Rect.Bottom = static_cast<short int>(B.Bottom);
    SendMessage(DM_SETITEMPOSITION, reinterpret_cast<LONG_PTR>(&Rect));
  }
}

char TFarDialogItem::GetColor(intptr_t Index) const
{
  return *((reinterpret_cast<const char *>(&FColors)) + Index);
}

void TFarDialogItem::SetColor(intptr_t Index, char Value)
{
  if (GetColor(Index) != Value)
  {
    *((reinterpret_cast<char *>(&FColors)) + Index) = Value;
    FColorMask |= (0xFF << (Index * 8));
  }
}

const struct PluginStartupInfo * TFarDialogItem::GetPluginStartupInfo() const
{
 return GetDialog()->GetFarPlugin()->GetPluginStartupInfo();
}

void TFarDialogItem::SetFlags(DWORD Value)
{
  if (GetFlags() != Value)
  {
    DebugAssert(!GetDialog()->GetHandle());
    UpdateFlags(Value);
  }
}

void TFarDialogItem::UpdateFlags(DWORD Value)
{
  if (GetFlags() != Value)
  {
    GetDialogItem()->Flags = Value;
    DialogChange();
  }
}

TRect TFarDialogItem::GetActualBounds() const
{
  return TRect(GetDialogItem()->X1, GetDialogItem()->Y1,
               GetDialogItem()->X2, GetDialogItem()->Y2);
}

DWORD TFarDialogItem::GetFlags() const
{
  return GetDialogItem()->Flags;
}

void TFarDialogItem::SetDataInternal(const UnicodeString & Value)
{
  UnicodeString FarData = Value.c_str();
  if (GetDialog()->GetHandle())
  {
    SendMessage(DM_SETTEXTPTR, reinterpret_cast<LONG_PTR>(FarData.c_str()));
  }
  nb_free((void*)GetDialogItem()->PtrData);
  GetDialogItem()->PtrData = TCustomFarPlugin::DuplicateStr(FarData, /*AllowEmpty=*/true);
  DialogChange();
}

void TFarDialogItem::SetData(const UnicodeString & Value)
{
  if (GetData() != Value)
  {
    SetDataInternal(Value);
  }
}

void TFarDialogItem::UpdateData(const UnicodeString & Value)
{
  UnicodeString FarData = Value.c_str();
  nb_free((void*)GetDialogItem()->PtrData);
  GetDialogItem()->PtrData = TCustomFarPlugin::DuplicateStr(FarData, /*AllowEmpty=*/true);
}

UnicodeString TFarDialogItem::GetData() const
{
  return const_cast<TFarDialogItem *>(this)->GetData();
}

UnicodeString TFarDialogItem::GetData()
{
  UnicodeString Result;
  if (GetDialogItem()->PtrData)
  {
    Result = GetDialogItem()->PtrData;
  }
  return Result;
}

void TFarDialogItem::SetType(intptr_t Value)
{
  if (GetType() != Value)
  {
    DebugAssert(!GetDialog()->GetHandle());
    GetDialogItem()->Type = static_cast<int>(Value);
  }
}

intptr_t TFarDialogItem::GetType() const
{
  return static_cast<intptr_t>(GetDialogItem()->Type);
}

void TFarDialogItem::SetAlterType(intptr_t Index, bool Value)
{
  if (GetAlterType(Index) != Value)
  {
    SetType(Value ? Index : FDefaultType);
  }
}

bool TFarDialogItem::GetAlterType(intptr_t Index) const
{
  return const_cast<TFarDialogItem *>(this)->GetAlterType(Index);
}

bool TFarDialogItem::GetAlterType(intptr_t Index)
{
  return (GetType() == Index);
}

bool TFarDialogItem::GetFlag(intptr_t Index) const
{
  bool Result = (GetFlags() & (Index & 0xFFFFFFFFFFFFFF00ULL)) != 0;
  if (Index & 0x000000FFUL)
  {
    Result = !Result;
  }
  return Result;
}

void TFarDialogItem::SetFlag(intptr_t Index, bool Value)
{
  if (GetFlag(Index) != Value)
  {
    if (Index & DIF_INVERSE)
    {
      Value = !Value;
    }

    DWORD F = GetFlags();
    FarDialogItemFlags Flag = (FarDialogItemFlags)(Index & 0xFFFFFF00ULL);
    bool ToHandle = true;

    switch (Flag)
    {
      case DIF_DISABLE:
        if (GetDialog()->GetHandle())
        {
          SendMessage(DM_ENABLE, !Value);
        }
        break;

      case DIF_HIDDEN:
        if (GetDialog()->GetHandle())
        {
          SendMessage(DM_SHOWITEM, !Value);
        }
        break;

      case DIF_3STATE:
        if (GetDialog()->GetHandle())
        {
          SendMessage(DM_SET3STATE, Value);
        }
        break;
    }

    if (ToHandle)
    {
      if (Value)
      {
        F |= Flag;
      }
      else
      {
        F &= ~Flag;
      }
      UpdateFlags(F);
    }
  }
}

void TFarDialogItem::SetEnabledFollow(TFarDialogItem * Value)
{
  if (GetEnabledFollow() != Value)
  {
    FEnabledFollow = Value;
    Change();
  }
}

void TFarDialogItem::SetEnabledDependency(TFarDialogItem * Value)
{
  if (GetEnabledDependency() != Value)
  {
    FEnabledDependency = Value;
    Change();
  }
}

void TFarDialogItem::SetEnabledDependencyNegative(TFarDialogItem * Value)
{
  if (GetEnabledDependencyNegative() != Value)
  {
    FEnabledDependencyNegative = Value;
    Change();
  }
}

bool TFarDialogItem::GetIsEmpty() const
{
  return GetData().IsEmpty();
}

LONG_PTR TFarDialogItem::FailItemProc(int Msg, LONG_PTR Param)
{
  intptr_t Result = 0;
  switch (Msg)
  {
    case DN_KILLFOCUS:
      Result = static_cast<intptr_t>(GetItem());
      break;

    default:
      Result = DefaultItemProc(Msg, Param);
      break;
  }
  return Result;
}

LONG_PTR TFarDialogItem::ItemProc(int Msg, LONG_PTR Param)
{
  LONG_PTR Result = 0;
  bool Handled = false;

  if (Msg == DN_GOTFOCUS)
  {
    DoFocus();
    UpdateFocused(true);
  }
  else if (Msg == DN_KILLFOCUS)
  {
    DoExit();
    UpdateFocused(false);
  }
  else if (Msg == DN_MOUSECLICK)
  {
    MOUSE_EVENT_RECORD * Event = reinterpret_cast<MOUSE_EVENT_RECORD *>(Param);
    if (FLAGCLEAR(Event->dwEventFlags, MOUSE_MOVED))
    {
      Result = MouseClick(Event);
      Handled = true;
    }
  }

  if (!Handled)
  {
    Result = DefaultItemProc(Msg, Param);
  }

  if (Msg == DN_CTLCOLORDLGITEM && FColorMask)
  {
    Result &= ~FColorMask;
    Result |= (FColors & FColorMask);
  }
  return Result;
}

void TFarDialogItem::DoFocus()
{
}

void TFarDialogItem::DoExit()
{
  if (FOnExit)
  {
    FOnExit(this);
  }
}

LONG_PTR TFarDialogItem::DefaultItemProc(int Msg, LONG_PTR Param)
{
  if (GetDialog() && GetDialog()->GetHandle())
  {
    TFarEnvGuard Guard;
    return GetPluginStartupInfo()->DefDlgProc(GetDialog()->GetHandle(),
      Msg, static_cast<int>(GetItem()), Param);
  }
  return 0;
}

LONG_PTR TFarDialogItem::DefaultDialogProc(int Msg, intptr_t Param1, LONG_PTR Param2)
{
  if (GetDialog() && GetDialog()->GetHandle())
  {
    TFarEnvGuard Guard;
    return GetPluginStartupInfo()->DefDlgProc(GetDialog()->GetHandle(),
      Msg, static_cast<int>(Param1), Param2);
  }
  return 0;
}

void TFarDialogItem::Change()
{
  if (GetEnabledFollow() || GetEnabledDependency() || GetEnabledDependencyNegative())
  {
    UpdateEnabled();
  }
}

void TFarDialogItem::SetEnabled(bool Value)
{
  if (GetEnabled() != Value)
  {
    FEnabled = Value;
    UpdateEnabled();
  }
}

void TFarDialogItem::UpdateEnabled()
{
  bool Value =
    GetEnabled() &&
    (!GetEnabledFollow() || GetEnabledFollow()->GetIsEnabled()) &&
    (!GetEnabledDependency() ||
     (!GetEnabledDependency()->GetIsEmpty() && GetEnabledDependency()->GetIsEnabled())) &&
    (!GetEnabledDependencyNegative() ||
     (GetEnabledDependencyNegative()->GetIsEmpty() || !GetEnabledDependencyNegative()->GetIsEnabled())) &&
    (!GetContainer() || GetContainer()->GetEnabled());

  if (Value != GetIsEnabled())
  {
    FIsEnabled = Value;
    SetFlag(DIF_DISABLE | DIF_INVERSE, GetIsEnabled());
  }
}

void TFarDialogItem::DialogChange()
{
  TFarDialog * Dlg = GetDialog();
  DebugAssert(Dlg);
  Dlg->Change();
}

LONG_PTR TFarDialogItem::SendDialogMessage(int Msg, intptr_t Param1, LONG_PTR Param2)
{
  return GetDialog()->SendDlgMessage(Msg, Param1, Param2);
}

LONG_PTR TFarDialogItem::SendMessage(int Msg, LONG_PTR Param)
{
  return GetDialog()->SendDlgMessage(Msg, GetItem(), Param);
}

void TFarDialogItem::SetSelected(intptr_t Value)
{
  if (GetSelected() != Value)
  {
    if (GetDialog()->GetHandle())
    {
      SendMessage(DM_SETCHECK, Value);
    }
    UpdateSelected(Value);
  }
}

void TFarDialogItem::UpdateSelected(intptr_t Value)
{
  if (GetSelected() != Value)
  {
    GetDialogItem()->Selected = static_cast<int>(Value);
    DialogChange();
  }
}

intptr_t TFarDialogItem::GetSelected() const
{
  return static_cast<intptr_t>(GetDialogItem()->Selected);
}

bool TFarDialogItem::GetFocused() const
{
  return GetDialogItem()->Focus != 0;
}

void TFarDialogItem::SetFocused(bool Value)
{
  GetDialogItem()->Focus = Value;
}

bool TFarDialogItem::GetChecked() const
{
  return GetSelected() == BSTATE_CHECKED;
}

void TFarDialogItem::SetChecked(bool Value)
{
  SetSelected(Value ? BSTATE_CHECKED : BSTATE_UNCHECKED);
}

void TFarDialogItem::Move(intptr_t DeltaX, intptr_t DeltaY)
{
  TRect R = GetBounds();

  R.Left += static_cast<int>(DeltaX);
  R.Right += static_cast<int>(DeltaX);
  R.Top += static_cast<int>(DeltaY);
  R.Bottom += static_cast<int>(DeltaY);

  SetBounds(R);
}

void TFarDialogItem::MoveAt(intptr_t X, intptr_t Y)
{
  Move(X - GetLeft(), Y - GetTop());
}

void TFarDialogItem::SetCoordinate(intptr_t Index, intptr_t Value)
{
  DebugAssert(sizeof(TRect) == sizeof(int) * 4);
  TRect R = GetBounds();
  int * D = reinterpret_cast<int *>(&R);
  D += Index;
  *D = static_cast<int>(Value);
  SetBounds(R);
}

intptr_t TFarDialogItem::GetCoordinate(intptr_t Index) const
{
  DebugAssert(sizeof(TRect) == sizeof(int) * 4);
  TRect R = GetBounds();
  int * D = reinterpret_cast<int *>(&R);
  D += Index;
  return static_cast<intptr_t>(*D);
}

void TFarDialogItem::SetWidth(intptr_t Value)
{
  TRect R = GetBounds();
  if (R.Left >= 0)
  {
    R.Right = R.Left + static_cast<int>(Value - 1);
  }
  else
  {
    DebugAssert(R.Right < 0);
    R.Left = R.Right - static_cast<int>(Value + 1);
  }
  SetBounds(R);
}

intptr_t TFarDialogItem::GetWidth() const
{
  return static_cast<intptr_t>(GetActualBounds().Width() + 1);
}

void TFarDialogItem::SetHeight(intptr_t Value)
{
  TRect R = GetBounds();
  if (R.Top >= 0)
  {
    R.Bottom = static_cast<int>(R.Top + Value - 1);
  }
  else
  {
    DebugAssert(R.Bottom < 0);
    R.Top = static_cast<int>(R.Bottom - Value + 1);
  }
  SetBounds(R);
}

intptr_t TFarDialogItem::GetHeight() const
{
  return static_cast<intptr_t>(GetActualBounds().Height() + 1);
}

bool TFarDialogItem::CanFocus() const
{
  intptr_t Type = GetType();
  return GetVisible() && GetEnabled() && GetTabStop() &&
    (Type == DI_EDIT || Type == DI_PSWEDIT || Type == DI_FIXEDIT ||
     Type == DI_BUTTON || Type == DI_CHECKBOX || Type == DI_RADIOBUTTON ||
     Type == DI_COMBOBOX || Type == DI_LISTBOX || Type == DI_USERCONTROL);
}

bool TFarDialogItem::Focused() const
{
  return GetFocused();
}

void TFarDialogItem::UpdateFocused(bool Value)
{
  SetFocused(Value);
  TFarDialog * Dlg = GetDialog();
  DebugAssert(Dlg);
  Dlg->SetItemFocused(Value ? this : nullptr);
}

void TFarDialogItem::SetFocus()
{
  DebugAssert(CanFocus());
  if (!Focused())
  {
    if (GetDialog()->GetHandle())
    {
      SendMessage(DM_SETFOCUS, 0);
    }
    else
    {
      if (GetDialog()->GetItemFocused())
      {
        DebugAssert(GetDialog()->GetItemFocused() != this);
        GetDialog()->GetItemFocused()->UpdateFocused(false);
      }
      UpdateFocused(true);
    }
  }
}

void TFarDialogItem::Init()
{
  if (GetFlag(DIF_CENTERGROUP))
  {
    SMALL_RECT Rect;
    ClearStruct(Rect);

    // at least for "text" item, returned item size is not correct (on 1.70 final)
    SendMessage(DM_GETITEMPOSITION, reinterpret_cast<LONG_PTR>(&Rect));

    TRect B = GetBounds();
    B.Left = Rect.Left;
    B.Right = Rect.Right;
    SetBounds(B);
  }
}

bool TFarDialogItem::CloseQuery()
{
  if (Focused() && (GetDialog()->GetResult() >= 0))
  {
    DoExit();
  }
  return true;
}

TPoint TFarDialogItem::MouseClientPosition(MOUSE_EVENT_RECORD * Event)
{
  TPoint Result;
  if (GetType() == DI_USERCONTROL)
  {
    Result = TPoint(Event->dwMousePosition.X, Event->dwMousePosition.Y);
  }
  else
  {
    Result = TPoint(
      static_cast<int>(Event->dwMousePosition.X - GetDialog()->GetBounds().Left - GetLeft()),
      static_cast<int>(Event->dwMousePosition.Y - GetDialog()->GetBounds().Top - GetTop()));
  }
  return Result;
}

bool TFarDialogItem::MouseClick(MOUSE_EVENT_RECORD * Event)
{
  if (FOnMouseClick)
  {
    FOnMouseClick(this, Event);
  }
  return DefaultItemProc(DN_MOUSECLICK, reinterpret_cast<LONG_PTR>(Event)) != 0;
}

bool TFarDialogItem::MouseMove(int /*X*/, int /*Y*/,
  MOUSE_EVENT_RECORD * Event)
{
  return DefaultDialogProc(DN_MOUSEEVENT, 0, reinterpret_cast<LONG_PTR>(Event)) != 0;
}

void TFarDialogItem::Text(int X, int Y, uintptr_t Color, const UnicodeString & Str)
{
  TFarEnvGuard Guard;
  GetPluginStartupInfo()->Text(
    static_cast<int>(GetDialog()->GetBounds().Left + GetLeft() + X),
    static_cast<int>(GetDialog()->GetBounds().Top + GetTop() + Y),
    static_cast<int>(Color), Str.c_str());
}

void TFarDialogItem::Redraw()
{
  // do not know how to force redraw of the item only
  GetDialog()->Redraw();
}

void TFarDialogItem::SetContainer(TFarDialogContainer * Value)
{
  if (GetContainer() != Value)
  {
    TFarDialogContainer * PrevContainer = GetContainer();
    FContainer = Value;
    if (PrevContainer)
    {
      PrevContainer->Remove(this);
    }
    if (GetContainer())
    {
      GetContainer()->Add(this);
    }
    UpdateBounds();
    UpdateEnabled();
  }
}

bool TFarDialogItem::HotKey(char /*HotKey*/)
{
  return false;
}

TFarBox::TFarBox(TFarDialog * ADialog) :
  TFarDialogItem(ADialog, DI_SINGLEBOX)
{
}

TFarButton::TFarButton(TFarDialog * ADialog) :
  TFarDialogItem(ADialog, DI_BUTTON),
  FResult(0),
  FOnClick(nullptr),
  FBrackets(brNormal)
{
}

void TFarButton::SetDataInternal(const UnicodeString & AValue)
{
  UnicodeString Value;
  switch (FBrackets)
  {
    case brTight:
      Value = L"[" + AValue + L"]";
      break;

    case brSpace:
      Value = L" " + AValue + L" ";
      break;

    default:
      Value = AValue;
      break;
  }

  TFarDialogItem::SetDataInternal(Value);

  if ((GetLeft() >= 0) || (GetRight() >= 0))
  {
    int Margin = 0;
    switch (FBrackets)
    {
      case brNone:
        Margin = 0;
        break;

      case brTight:
      case brSpace:
        Margin = 1;
        break;

      case brNormal:
        Margin = 2;
        break;
    }
    SetWidth(Margin + ::StripHotkey(Value).GetLength() + Margin);
  }
}

UnicodeString TFarButton::GetData()
{
  UnicodeString Result = TFarDialogItem::GetData();
  if ((FBrackets == brTight) || (FBrackets == brSpace))
  {
    bool HasBrackets = (Result.Length() >= 2) &&
      (Result[1] == ((FBrackets == brSpace) ? L' ' : L'[')) &&
      (Result[Result.Length()] == ((FBrackets == brSpace) ? L' ' : L']'));
    DebugAssert(HasBrackets);
    if (HasBrackets)
    {
      Result = Result.SubString(2, Result.Length() - 2);
    }
  }
  return Result;
}

void TFarButton::SetDefault(bool Value)
{
  if (GetDefault() != Value)
  {
    DebugAssert(!GetDialog()->GetHandle());
    GetDialogItem()->DefaultButton = Value;
    if (Value)
    {
      if (GetDialog()->GetDefaultButton() && (GetDialog()->GetDefaultButton() != this))
      {
        GetDialog()->GetDefaultButton()->SetDefault(false);
      }
      GetDialog()->FDefaultButton = this;
    }
    else if (GetDialog()->GetDefaultButton() == this)
    {
      GetDialog()->FDefaultButton = nullptr;
    }
    DialogChange();
  }
}

bool TFarButton::GetDefault() const
{
  return GetDialogItem()->DefaultButton != 0;
}

void TFarButton::SetBrackets(TFarButtonBrackets Value)
{
  if (FBrackets != Value)
  {
    UnicodeString Data = GetData();
    SetFlag(DIF_NOBRACKETS, (Value != brNormal));
    FBrackets = Value;
    SetDataInternal(Data);
  }
}

LONG_PTR TFarButton::ItemProc(int Msg, LONG_PTR Param)
{
  if (Msg == DN_BTNCLICK)
  {
    if (!GetEnabled())
    {
      return 1;
    }
    else
    {
      bool Close = (GetResult() != 0);
      if (FOnClick)
      {
        FOnClick(this, Close);
      }
      if (!Close)
      {
        return 1;
      }
    }
  }
  return TFarDialogItem::ItemProc(Msg, Param);
}

bool TFarButton::HotKey(char HotKey)
{
  UnicodeString Caption = GetCaption();
  intptr_t P = Caption.Pos(L'&');
  bool Result =
    GetVisible() && GetEnabled() &&
    (P > 0) && (P < Caption.Length()) &&
    (Caption[P + 1] == HotKey);
  if (Result)
  {
    bool Close = (GetResult() != 0);
    if (FOnClick)
    {
      FOnClick(this, Close);
    }

    if (Close)
    {
      GetDialog()->Close(this);
    }
  }
  return Result;
}

TFarCheckBox::TFarCheckBox(TFarDialog * ADialog) :
  TFarDialogItem(ADialog, DI_CHECKBOX),
  FOnAllowChange(nullptr)
{
}

LONG_PTR TFarCheckBox::ItemProc(int Msg, LONG_PTR Param)
{
  if (Msg == DN_BTNCLICK)
  {
    bool Allow = true;
    if (FOnAllowChange)
    {
      FOnAllowChange(this, Param, Allow);
    }
    if (Allow)
    {
      UpdateSelected(Param);
    }
    return static_cast<intptr_t>(Allow);
  }
  else
  {
    return TFarDialogItem::ItemProc(Msg, Param);
  }
}

bool TFarCheckBox::GetIsEmpty() const
{
  return GetChecked() != BSTATE_CHECKED;
}

void TFarCheckBox::SetData(const UnicodeString & Value)
{
  TFarDialogItem::SetData(Value);
  if (GetLeft() >= 0 || GetRight() >= 0)
  {
    SetWidth(4 + ::StripHotkey(Value).Length());
  }
}

TFarRadioButton::TFarRadioButton(TFarDialog * ADialog) :
  TFarDialogItem(ADialog, DI_RADIOBUTTON),
  FOnAllowChange(nullptr)
{
}

LONG_PTR TFarRadioButton::ItemProc(int Msg, LONG_PTR Param)
{
  if (Msg == DN_BTNCLICK)
  {
    bool Allow = true;
    if (FOnAllowChange)
    {
      FOnAllowChange(this, Param, Allow);
    }
    if (Allow)
    {
      // FAR WORKAROUND
      // This does not correspond to FAR API Manual, but it works so.
      // Manual says that Param should contain ID of previously selected dialog item
      UpdateSelected(Param);
    }
    return static_cast<intptr_t>(Allow);
  }
  else
  {
    return TFarDialogItem::ItemProc(Msg, Param);
  }
}

bool TFarRadioButton::GetIsEmpty() const
{
  return !GetChecked();
}

void TFarRadioButton::SetData(const UnicodeString & Value)
{
  TFarDialogItem::SetData(Value);
  if (GetLeft() >= 0 || GetRight() >= 0)
  {
    SetWidth(4 + ::StripHotkey(Value).Length());
  }
}

TFarEdit::TFarEdit(TFarDialog * ADialog) :
  TFarDialogItem(ADialog, DI_EDIT)
{
  SetAutoSelect(false);
}

void TFarEdit::Detach()
{
  nb_free((void*)GetDialogItem()->Mask);
  // nb_free((void*)GetDialogItem()->History);
  TFarDialogItem::Detach();
}

LONG_PTR TFarEdit::ItemProc(int Msg, LONG_PTR Param)
{
  if (Msg == DN_EDITCHANGE)
  {
    UnicodeString Data = (reinterpret_cast<FarDialogItem *>(Param))->PtrData;
    nb_free((void*)GetDialogItem()->PtrData);
    GetDialogItem()->PtrData = TCustomFarPlugin::DuplicateStr(Data, /*AllowEmpty=*/true);
  }
  return TFarDialogItem::ItemProc(Msg, Param);
}

UnicodeString TFarEdit::GetHistoryMask(size_t Index) const
{
  UnicodeString Result =
    ((Index == 0) && (GetFlags() & DIF_HISTORY)) ||
    ((Index == 1) && (GetFlags() & DIF_MASKEDIT)) ? GetDialogItem()->Mask : L"";
  return Result;
}

void TFarEdit::SetHistoryMask(size_t Index, const UnicodeString & Value)
{
  if (GetHistoryMask(Index) != Value)
  {
    DebugAssert(!GetDialog()->GetHandle());
    DebugAssert(&GetDialogItem()->Mask == &GetDialogItem()->History);

    nb_free((void*)GetDialogItem()->Mask);
    if (Value.IsEmpty())
    {
      GetDialogItem()->Mask = nullptr;
    }
    else
    {
      GetDialogItem()->Mask = TCustomFarPlugin::DuplicateStr(Value);
    }
    bool PrevHistory = !GetHistory().IsEmpty();
    SetFlag(DIF_HISTORY, (Index == 0) && !Value.IsEmpty());
    bool Masked = (Index == 1) && !Value.IsEmpty();
    SetFlag(DIF_MASKEDIT, Masked);
    if (Masked)
    {
      SetFixed(true);
    }
    bool CurrHistory = !GetHistory().IsEmpty();
    if (PrevHistory != CurrHistory)
    {
      // add/remove space for history arrow
      SetWidth(GetWidth() + (CurrHistory ? -1 : 1));
    }
    DialogChange();
  }
}

void TFarEdit::SetAsInteger(intptr_t Value)
{
  intptr_t Int = GetAsInteger();
  if (!Int || (Int != Value))
  {
    SetText(::IntToStr(Value));
    DialogChange();
  }
}

intptr_t TFarEdit::GetAsInteger()
{
  return ::StrToIntDef(::Trim(GetText()), 0);
}

TFarSeparator::TFarSeparator(TFarDialog * ADialog) :
  TFarDialogItem(ADialog, DI_TEXT)
{
  SetLeft(-1);
  SetFlag(DIF_SEPARATOR, true);
}

void TFarSeparator::ResetBounds()
{
  TFarDialogItem::ResetBounds();
  if (GetBounds().Left < 0)
  {
    GetDialogItem()->X1 = -1;
  }
}

void TFarSeparator::SetDouble(bool Value)
{
  if (GetDouble() != Value)
  {
    DebugAssert(!GetDialog()->GetHandle());
    SetFlag(DIF_SEPARATOR, !Value);
    SetFlag(DIF_SEPARATOR2, Value);
  }
}

bool TFarSeparator::GetDouble()
{
  return GetFlag(DIF_SEPARATOR2);
}

void TFarSeparator::SetPosition(intptr_t Value)
{
  TRect R = GetBounds();
  R.Top = static_cast<int>(Value);
  R.Bottom = static_cast<int>(Value);
  SetBounds(R);
}

int TFarSeparator::GetPosition()
{
  return GetBounds().Top;
}

TFarText::TFarText(TFarDialog * ADialog) :
  TFarDialogItem(ADialog, DI_TEXT)
{
}

void TFarText::SetData(const UnicodeString & Value)
{
  TFarDialogItem::SetData(Value);
  if (GetLeft() >= 0 || GetRight() >= 0)
  {
    SetWidth(::StripHotkey(Value).Length());
  }
}

TFarList::TFarList(TFarDialogItem * ADialogItem) :
  TStringList(),
  FDialogItem(ADialogItem),
  FNoDialogUpdate(false)
{
  DebugAssert((ADialogItem == nullptr) ||
    (ADialogItem->GetType() == DI_COMBOBOX) || (ADialogItem->GetType() == DI_LISTBOX));
  FListItems = static_cast<FarList *>(nb_calloc(1, sizeof(FarList)));
}

TFarList::~TFarList()
{
  for (intptr_t Index = 0; Index < FListItems->ItemsNumber; ++Index)
  {
    nb_free((void*)FListItems->Items[Index].Text);
  }
  nb_free(FListItems->Items);
  nb_free(FListItems);
}

void TFarList::Assign(const TPersistent * Source)
{
  TStringList::Assign(Source);

  const TFarList * FarList = NB_STATIC_DOWNCAST_CONST(TFarList, Source);
  if (FarList != nullptr)
  {
    for (intptr_t Index = 0; Index < FarList->GetCount(); ++Index)
    {
      SetFlags(Index, FarList->GetFlags(Index));
    }
  }
}

void TFarList::UpdateItem(intptr_t Index)
{
  FarListItem * ListItem = &FListItems->Items[Index];
  nb_free((void*)ListItem->Text);
  ListItem->Text = TCustomFarPlugin::DuplicateStr(GetString(Index), /*AllowEmpty=*/true);

  FarListUpdate ListUpdate;
  ClearStruct(ListUpdate);
  ListUpdate.Index = static_cast<int>(Index);
  ListUpdate.Item = *ListItem;
  GetDialogItem()->SendMessage(DM_LISTUPDATE, reinterpret_cast<LONG_PTR>(&ListUpdate));
}

void TFarList::Put(intptr_t Index, const UnicodeString & Str)
{
  if ((GetDialogItem() != nullptr) && GetDialogItem()->GetDialog()->GetHandle())
  {
    FNoDialogUpdate = true;
    SCOPE_EXIT
    {
      FNoDialogUpdate = false;
    };
    TStringList::SetString(Index, Str);
    if (GetUpdateCount() == 0)
    {
      UpdateItem(Index);
    }
  }
  else
  {
    TStringList::SetString(Index, Str);
  }
}

void TFarList::Changed()
{
  TStringList::Changed();

  if ((GetUpdateCount() == 0) && !FNoDialogUpdate)
  {
    intptr_t PrevSelected = 0;
    intptr_t PrevTopIndex = 0;
    if ((GetDialogItem() != nullptr) && GetDialogItem()->GetDialog()->GetHandle())
    {
      PrevSelected = GetSelected();
      PrevTopIndex = GetTopIndex();
    }
    intptr_t Count = GetCount();
    if (FListItems->ItemsNumber != Count)
    {
      FarListItem * Items = FListItems->Items;
      intptr_t ItemsNumber = FListItems->ItemsNumber;
      if (Count)
      {
        FListItems->Items = static_cast<FarListItem *>(
          nb_calloc(1, sizeof(FarListItem) * Count));
        for (intptr_t Index = 0; Index < Count; ++Index)
        {
          if (Index < FListItems->ItemsNumber)
          {
            FListItems->Items[Index].Flags = Items[Index].Flags;
          }
        }
      }
      else
      {
        FListItems->Items = nullptr;
      }
      for (intptr_t Index = 0; Index < ItemsNumber; ++Index)
      {
        nb_free((void*)Items[Index].Text);
      }
      nb_free(Items);
      FListItems->ItemsNumber = static_cast<int>(GetCount());
    }
    for (intptr_t Index = 0; Index < GetCount(); ++Index)
    {
      FListItems->Items[Index].Text = TCustomFarPlugin::DuplicateStr(GetString(Index), /*AllowEmpty=*/true);
    }
    if ((GetDialogItem() != nullptr) && GetDialogItem()->GetDialog()->GetHandle())
    {
      GetDialogItem()->GetDialog()->LockChanges();
      SCOPE_EXIT
      {
        GetDialogItem()->GetDialog()->UnlockChanges();
      };
      GetDialogItem()->SendMessage(DM_LISTSET, reinterpret_cast<LONG_PTR>(FListItems));
      if (PrevTopIndex + GetDialogItem()->GetHeight() > GetCount())
      {
        PrevTopIndex = GetCount() > GetDialogItem()->GetHeight() ? GetCount() - GetDialogItem()->GetHeight() : 0;
      }
      SetCurPos((PrevSelected >= GetCount()) ? (GetCount() - 1) : PrevSelected,
        PrevTopIndex);
    }
  }
}

void TFarList::SetSelected(intptr_t Value)
{
  TFarDialogItem * DialogItem = GetDialogItem();
  DebugAssert(DialogItem != nullptr);
  if (GetSelectedInt(false) != Value)
  {
    if (DialogItem->GetDialog()->GetHandle())
    {
      UpdatePosition(Value);
    }
    else
    {
      DialogItem->SetData(GetString(Value));
    }
  }
}

void TFarList::UpdatePosition(intptr_t Position)
{
  if (Position >= 0)
  {
    intptr_t ATopIndex = GetTopIndex();
    // even if new position is visible already, FAR will scroll the view so
    // that the new selected item is the last one, following prevents the scroll
    if ((ATopIndex <= Position) && (Position < ATopIndex + GetVisibleCount()))
    {
      SetCurPos(Position, ATopIndex);
    }
    else
    {
      SetCurPos(Position, -1);
    }
  }
}

void TFarList::SetCurPos(intptr_t Position, intptr_t TopIndex)
{
  TFarDialogItem * DialogItem = GetDialogItem();
  DebugAssert(DialogItem != nullptr);
  TFarDialog * Dlg = DialogItem->GetDialog();
  DebugAssert(Dlg);
  DebugAssert(Dlg->GetHandle());
  DebugUsedParam(Dlg);
  FarListPos ListPos;
  ListPos.SelectPos = static_cast<int>(Position);
  ListPos.TopPos = static_cast<int>(TopIndex);
  DialogItem->SendMessage(DM_LISTSETCURPOS, reinterpret_cast<LONG_PTR>(&ListPos));
}

void TFarList::SetTopIndex(intptr_t Value)
{
  if (Value != GetTopIndex())
  {
    SetCurPos(NPOS, Value);
  }
}

intptr_t TFarList::GetPosition() const
{
  TFarDialogItem * DialogItem = GetDialogItem();
  DebugAssert(DialogItem != nullptr);
  return DialogItem->SendMessage(DM_LISTGETCURPOS, 0);
}

intptr_t TFarList::GetTopIndex() const
{
  intptr_t Result = -1;
  if (GetCount() != 0)
  {
    FarListPos ListPos;
    ClearStruct(ListPos);
    TFarDialogItem * DialogItem = GetDialogItem();
    DebugAssert(DialogItem != nullptr);
    DialogItem->SendMessage(DM_LISTGETCURPOS, reinterpret_cast<LONG_PTR>(&ListPos));
    Result = static_cast<intptr_t>(ListPos.TopPos);
  }
  return Result;
}

intptr_t TFarList::GetMaxLength() const
{
  intptr_t Result = 0;
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    if (Result < GetString(Index).Length())
    {
      Result = GetString(Index).Length();
    }
  }
  return Result;
}

intptr_t TFarList::GetVisibleCount() const
{
  TFarDialogItem * DialogItem = GetDialogItem();
  DebugAssert(DialogItem != nullptr);
  return DialogItem->GetHeight() - (GetDialogItem()->GetFlag(DIF_LISTNOBOX) ? 0 : 2);
}

intptr_t TFarList::GetSelectedInt(bool Init) const
{
  intptr_t Result = NPOS;
  TFarDialogItem * DialogItem = GetDialogItem();
  DebugAssert(DialogItem != nullptr);
  if (GetCount() == 0)
  {
    Result = NPOS;
  }
  else if (DialogItem->GetDialog()->GetHandle() && !Init)
  {
    Result = GetPosition();
  }
  else
  {
    const wchar_t * PtrData = DialogItem->GetDialogItem()->PtrData;
    if (PtrData)
    {
      Result = IndexOf(PtrData);
    }
  }

  return Result;
}

intptr_t TFarList::GetSelected() const
{
  intptr_t Result = GetSelectedInt(false);

  if ((Result == NPOS) && (GetCount() > 0))
  {
    Result = 0;
  }

  return Result;
}

DWORD TFarList::GetFlags(intptr_t Index) const
{
  return FListItems->Items[Index].Flags;
}

void TFarList::SetFlags(intptr_t Index, DWORD Value)
{
  if (FListItems->Items[Index].Flags != Value)
  {
    FListItems->Items[Index].Flags = Value;
    if ((GetDialogItem() != nullptr) && GetDialogItem()->GetDialog()->GetHandle() && (GetUpdateCount() == 0))
    {
      UpdateItem(Index);
    }
  }
}

bool TFarList::GetFlag(intptr_t Index, DWORD Flag) const
{
  return FLAGSET(GetFlags(Index), Flag);
}

void TFarList::SetFlag(intptr_t Index, DWORD Flag, bool Value)
{
  SetFlags(Index, (GetFlags(Index) & ~Flag) | FLAGMASK(Value, Flag));
}

void TFarList::Init()
{
  UpdatePosition(GetSelectedInt(true));
}

LONG_PTR TFarList::ItemProc(int Msg, LONG_PTR Param)
{
  TFarDialogItem * DialogItem = GetDialogItem();
  DebugAssert(DialogItem != nullptr);
  if (Msg == DN_LISTCHANGE)
  {
    if ((Param < 0) || ((Param == 0) && (GetCount() == 0)))
    {
      DialogItem->UpdateData(L"");
    }
    else
    {
      DebugAssert(Param >= 0 && Param < GetCount());
      DialogItem->UpdateData(GetString(Param));
    }
  }
  return 0;
}

TFarListBox::TFarListBox(TFarDialog * ADialog) :
  TFarDialogItem(ADialog, DI_LISTBOX),
  FAutoSelect(asOnlyFocus),
  FDenyClose(false)
{
  FList = new TFarList(this);
  GetDialogItem()->ListItems = FList->GetListItems();
}

TFarListBox::~TFarListBox()
{
  SAFE_DESTROY(FList);
}

LONG_PTR TFarListBox::ItemProc(int Msg, LONG_PTR Param)
{
  intptr_t Result = 0;
  // FAR WORKAROUND
  // Since 1.70 final, hotkeys do not work when list box has focus.
  if ((Msg == DN_KEY) && GetDialog()->HotKey(Param))
  {
    Result = 1;
  }
  else if (FList->ItemProc(Msg, Param))
  {
    Result = 1;
  }
  else
  {
    Result = TFarDialogItem::ItemProc(Msg, Param);
  }
  return Result;
}

void TFarListBox::Init()
{
  TFarDialogItem::Init();
  GetItems()->Init();
  UpdateMouseReaction();
}

void TFarListBox::SetAutoSelect(TFarListBoxAutoSelect Value)
{
  if (GetAutoSelect() != Value)
  {
    FAutoSelect = Value;
    if (GetDialog()->GetHandle())
    {
      UpdateMouseReaction();
    }
  }
}

void TFarListBox::UpdateMouseReaction()
{
  SendMessage(DM_LISTSETMOUSEREACTION, static_cast<int>(GetAutoSelect()));
}

void TFarListBox::SetItems(TStrings * Value)
{
  FList->Assign(Value);
}

void TFarListBox::SetList(TFarList * Value)
{
  SetItems(Value);
}

bool TFarListBox::CloseQuery()
{
  return true;
}

TFarComboBox::TFarComboBox(TFarDialog * ADialog) :
  TFarDialogItem(ADialog, DI_COMBOBOX),
  FList(nullptr)
{
  FList = new TFarList(this);
  GetDialogItem()->ListItems = FList->GetListItems();
  SetAutoSelect(false);
}

TFarComboBox::~TFarComboBox()
{
  SAFE_DESTROY(FList);
}

void TFarComboBox::ResizeToFitContent()
{
  SetWidth(FList->GetMaxLength());
}

LONG_PTR TFarComboBox::ItemProc(int Msg, LONG_PTR Param)
{
  if (Msg == DN_EDITCHANGE)
  {
    UnicodeString Data = (reinterpret_cast<FarDialogItem *>(Param))->PtrData;
    nb_free((void*)GetDialogItem()->PtrData);
    GetDialogItem()->PtrData = TCustomFarPlugin::DuplicateStr(Data, /*AllowEmpty=*/true);
  }

  if (FList->ItemProc(Msg, Param))
  {
    return 1;
  }
  else
  {
    return TFarDialogItem::ItemProc(Msg, Param);
  }
}

void TFarComboBox::Init()
{
  TFarDialogItem::Init();
  GetItems()->Init();
}

TFarLister::TFarLister(TFarDialog * ADialog) :
  TFarDialogItem(ADialog, DI_USERCONTROL),
  FItems(new TStringList()),
  FTopIndex(0)
{
  FItems->SetOnChange(MAKE_CALLBACK(TFarLister::ItemsChange, this));
}

TFarLister::~TFarLister()
{
  SAFE_DESTROY(FItems);
}

void TFarLister::ItemsChange(TObject * /*Sender*/)
{
  FTopIndex = 0;
  if (GetDialog()->GetHandle())
  {
    Redraw();
  }
}

bool TFarLister::GetScrollBar() const
{
  return (GetItems()->GetCount() > GetHeight());
}

void TFarLister::SetTopIndex(intptr_t Value)
{
  if (GetTopIndex() != Value)
  {
    FTopIndex = Value;
    Redraw();
  }
}

TStrings * TFarLister::GetItems() const
{
  return FItems;
}

void TFarLister::SetItems(const TStrings * Value)
{
  if (!FItems->Equals(Value))
  {
    FItems->Assign(Value);
  }
}

void TFarLister::DoFocus()
{
  TFarDialogItem::DoFocus();
  TODO("hide cursor");
}

LONG_PTR TFarLister::ItemProc(int Msg, LONG_PTR Param)
{
  intptr_t Result = 0;

  if (Msg == DN_DRAWDLGITEM)
  {
    bool AScrollBar = GetScrollBar();
    intptr_t ScrollBarPos = 0;
    if (GetItems()->GetCount() > GetHeight())
    {
      ScrollBarPos = static_cast<intptr_t>((static_cast<float>(GetHeight() - 3) * (static_cast<float>(FTopIndex) / (GetItems()->GetCount() - GetHeight())))) + 1;
    }
    intptr_t DisplayWidth = GetWidth() - (AScrollBar ? 1 : 0);
    uintptr_t Color = GetDialog()->GetSystemColor(
      FLAGSET(GetDialog()->GetFlags(), FDLG_WARNING) ? COL_WARNDIALOGLISTTEXT : COL_DIALOGLISTTEXT);
    UnicodeString Buf;
    for (intptr_t Row = 0; Row < GetHeight(); Row++)
    {
      intptr_t Index = GetTopIndex() + Row;
      Buf = L" ";
      if (Index < GetItems()->GetCount())
      {
        UnicodeString Value = GetItems()->GetString(Index).SubString(1, DisplayWidth - 1);
        Buf += Value;
      }
      UnicodeString Value = ::StringOfChar(' ', DisplayWidth - Buf.Length());
      Value.SetLength(DisplayWidth - Buf.Length());
      Buf += Value;
      if (AScrollBar)
      {
        if (Row == 0)
        {
          Buf += static_cast<wchar_t>(0x25B2); // ucUpScroll
        }
        else if (Row == ScrollBarPos)
        {
          Buf += static_cast<wchar_t>(0x2592); // ucBox50
        }
        else if (Row == GetHeight() - 1)
        {
          Buf += static_cast<wchar_t>(0x25BC); // ucDnScroll
        }
        else
        {
          Buf += static_cast<wchar_t>(0x2591); // ucBox25
        }
      }
      Text(0, (int)Row, Color, Buf);
    }
  }
  else if (Msg == DN_KEY)
  {
    Result = 1;

    intptr_t NewTopIndex = GetTopIndex();
    if ((Param == KEY_UP) || (Param == KEY_LEFT))
    {
      if (NewTopIndex > 0)
      {
        --NewTopIndex;
      }
      else
      {
        long ShiftTab = KEY_SHIFTTAB;
        SendDialogMessage(DM_KEY, 1, reinterpret_cast<intptr_t>(&ShiftTab));
      }
    }
    else if ((Param == KEY_DOWN) || (Param == KEY_RIGHT))
    {
      if (NewTopIndex < GetItems()->GetCount() - GetHeight())
      {
        ++NewTopIndex;
      }
      else
      {
        intptr_t Tab = KEY_TAB;
        SendDialogMessage(DM_KEY, 1, reinterpret_cast<LONG_PTR>(&Tab));
      }
    }
    else if (Param == KEY_PGUP)
    {
      if (NewTopIndex > GetHeight() - 1)
      {
        NewTopIndex -= GetHeight() - 1;
      }
      else
      {
        NewTopIndex = 0;
      }
    }
    else if (Param == KEY_PGDN)
    {
      if (NewTopIndex < GetItems()->GetCount() - GetHeight() - GetHeight() + 1)
      {
        NewTopIndex += GetHeight() - 1;
      }
      else
      {
        NewTopIndex = GetItems()->GetCount() - GetHeight();
      }
    }
    else if (Param == KEY_HOME)
    {
      NewTopIndex = 0;
    }
    else if (Param == KEY_END)
    {
      NewTopIndex = GetItems()->GetCount() - GetHeight();
    }
    else
    {
      Result = TFarDialogItem::ItemProc(Msg, Param);
    }

    SetTopIndex(NewTopIndex);
  }
  else if (Msg == DN_MOUSECLICK)
  {
    if (!Focused() && CanFocus())
    {
      SetFocus();
    }

    MOUSE_EVENT_RECORD * Event = reinterpret_cast<MOUSE_EVENT_RECORD *>(Param);
    TPoint P = MouseClientPosition(Event);

    if (FLAGSET(Event->dwEventFlags, DOUBLE_CLICK) &&
        (P.x < GetWidth() - 1))
    {
      Result = TFarDialogItem::ItemProc(Msg, Param);
    }
    else
    {
      intptr_t NewTopIndex = GetTopIndex();

      if (((P.x == static_cast<int>(GetWidth()) - 1) && (P.y == 0)) ||
          ((P.x < static_cast<int>(GetWidth() - 1)) && (P.y < static_cast<int>(GetHeight() / 2))))
      {
        if (NewTopIndex > 0)
        {
          --NewTopIndex;
        }
      }
      else if (((P.x == GetWidth() - 1) && (P.y == static_cast<int>(GetHeight() - 1))) ||
          ((P.x < GetWidth() - 1) && (P.y >= static_cast<int>(GetHeight() / 2))))
      {
        if (NewTopIndex < GetItems()->GetCount() - GetHeight())
        {
          ++NewTopIndex;
        }
      }
      else
      {
        DebugAssert(P.x == GetWidth() - 1);
        DebugAssert((P.y > 0) && (P.y < static_cast<int>(GetHeight() - 1)));
        NewTopIndex = static_cast<intptr_t>(ceil(static_cast<float>(P.y - 1) / (GetHeight() - 2) * (GetItems()->GetCount() - GetHeight() + 1)));
      }

      Result = 1;

      SetTopIndex(NewTopIndex);
    }
  }
  else
  {
    Result = TFarDialogItem::ItemProc(Msg, Param);
  }

  return Result;
}

NB_IMPLEMENT_CLASS(TFarDialog, NB_GET_CLASS_INFO(TObject), nullptr)
NB_IMPLEMENT_CLASS(TFarDialogItem, NB_GET_CLASS_INFO(TObject), nullptr)
NB_IMPLEMENT_CLASS(TFarCheckBox, NB_GET_CLASS_INFO(TFarDialogItem), nullptr)
NB_IMPLEMENT_CLASS(TFarButton, NB_GET_CLASS_INFO(TFarDialogItem), nullptr)
NB_IMPLEMENT_CLASS(TFarListBox, NB_GET_CLASS_INFO(TFarDialogItem), nullptr)
NB_IMPLEMENT_CLASS(TFarEdit, NB_GET_CLASS_INFO(TFarDialogItem), nullptr)
NB_IMPLEMENT_CLASS(TFarText, NB_GET_CLASS_INFO(TFarDialogItem), nullptr)
NB_IMPLEMENT_CLASS(TFarList, NB_GET_CLASS_INFO(TStringList), nullptr)
NB_IMPLEMENT_CLASS(TFarDialogContainer, NB_GET_CLASS_INFO(TObject), nullptr)

