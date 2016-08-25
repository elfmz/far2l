#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <Sysutils.hpp>

#include "NamedObjs.h"

static intptr_t NamedObjectSortProc(const void * Item1, const void * Item2)
{
  return static_cast<const TNamedObject *>(Item1)->Compare(static_cast<const TNamedObject *>(Item2));
}
//--- TNamedObject ----------------------------------------------------------
TNamedObject::TNamedObject(const UnicodeString & AName) :
  FHidden(false)
{
  SetName(AName);
}

void TNamedObject::SetName(const UnicodeString & Value)
{
  UnicodeString HiddenPrefix(CONST_HIDDEN_PREFIX);
  FHidden = (Value.SubString(1, HiddenPrefix.Length()) == HiddenPrefix);
  FName = Value;
}

intptr_t TNamedObject::Compare(const TNamedObject * Other) const
{
  intptr_t Result;
  if (GetHidden() && !Other->GetHidden())
  {
    Result = -1;
  }
  else if (!GetHidden() && Other->GetHidden())
  {
    Result = 1;
  }
  else
  {
    Result = CompareLogicalText(GetName(), Other->GetName());
  }
  return Result;
}

bool TNamedObject::IsSameName(const UnicodeString & AName) const
{
  return (GetName().CompareIC(AName) == 0);
}

void TNamedObject::MakeUniqueIn(TNamedObjectList * List)
{
  // This object can't be item of list, it would create infinite loop
  if (List && (List->IndexOf(this) == -1))
    while (List->FindByName(GetName()))
    {
      int64_t N = 0;
      intptr_t P = 0;
      // If name already contains number parenthesis remove it (and remember it)
      UnicodeString Name = GetName();
      if ((Name[Name.Length()] == L')') && ((P = Name.LastDelimiter(L'(')) > 0))
      {
        try
        {
          N = ::StrToInt64(Name.SubString(P + 1, Name.Length() - P - 1));
          Name.Delete(P, Name.Length() - P + 1);
          SetName(Name.TrimRight());
        }
        catch (Exception & E)
        {
          (void)E;
          N = 0;
        }
      }
      SetName(Name + L" (" + ::Int64ToStr(N+1) + L")");
    }
}
//--- TNamedObjectList ------------------------------------------------------
TNamedObjectList::TNamedObjectList() :
  TObjectList(),
  AutoSort(true),
  FHiddenCount(0),
  FControlledAdd(false)
{
}
//---------------------------------------------------------------------------
const TNamedObject * TNamedObjectList::AtObject(intptr_t Index) const
{
  return const_cast<TNamedObjectList *>(this)->AtObject(Index);
}
//---------------------------------------------------------------------------
TNamedObject * TNamedObjectList::AtObject(intptr_t Index)
{
  return NB_STATIC_DOWNCAST(TNamedObject, GetObj(Index + FHiddenCount));
}
//---------------------------------------------------------------------------
void TNamedObjectList::Recount()
{
  intptr_t Index = 0;
  while ((Index < TObjectList::GetCount()) && (NB_STATIC_DOWNCAST(TNamedObject, GetObj(Index))->GetHidden()))
  {
    ++Index;
  }
  FHiddenCount = Index;
}
//---------------------------------------------------------------------------
void TNamedObjectList::AlphaSort()
{
  Sort(NamedObjectSortProc);
  Recount();
}
//---------------------------------------------------------------------------
intptr_t TNamedObjectList::Add(TObject * AObject)
{
  intptr_t Result;
  TAutoFlag ControlledAddFlag(FControlledAdd);
  TNamedObject * NamedObject = static_cast<TNamedObject *>(AObject);
  // If temporarily not auto-sorting (when loading session list),
  // keep the hidden objects in front, so that HiddenCount is correct
  if (!AutoSort && NamedObject->GetHidden())
  {
    Result = 0;
    Insert(Result, AObject);
    FHiddenCount++;
  }
  else
  {
    Result = TObjectList::Add(AObject);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TNamedObjectList::Notify(void * Ptr, TListNotification Action)
{
  if (Action == lnDeleted)
  {
    TNamedObject * NamedObject = static_cast<TNamedObject *>(Ptr);
    if (NamedObject->GetHidden() && (FHiddenCount >= 0))
    {
      FHiddenCount--;
    }
  }
  TObjectList::Notify(Ptr, Action);
  if (Action == lnAdded)
  {
    if (!FControlledAdd)
    {
      FHiddenCount = -1;
    }
    if (AutoSort)
    {
      AlphaSort();
    }
  }
}
//---------------------------------------------------------------------------
const TNamedObject * TNamedObjectList::FindByName(const UnicodeString & Name) const
{
  return const_cast<TNamedObjectList *>(this)->FindByName(Name);
}

TNamedObject * TNamedObjectList::FindByName(const UnicodeString & Name)
{
  // This should/can be optimized when list is sorted
  for (Integer Index = 0; Index < GetCountIncludingHidden(); ++Index)
  {
    // Not using AtObject as we iterate even hidden objects here
    TNamedObject * NamedObject = static_cast<TNamedObject *>(GetObj(Index));
    if (NamedObject->IsSameName(Name))
    {
      return NamedObject;
    }
  }
  return nullptr;
}
//---------------------------------------------------------------------------
void TNamedObjectList::SetCount(intptr_t Value)
{
  TObjectList::SetCount(Value/*+HiddenCount*/);
}

intptr_t TNamedObjectList::GetCount() const
{
  DebugAssert(FHiddenCount >= 0);
  return TObjectList::GetCount() - FHiddenCount;
}

intptr_t TNamedObjectList::GetCountIncludingHidden() const
{
  DebugAssert(FHiddenCount >= 0);
  return TObjectList::GetCount();
}

NB_IMPLEMENT_CLASS(TNamedObject, NB_GET_CLASS_INFO(TPersistent), nullptr)

