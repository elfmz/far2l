
#include <Classes.hpp>
#include <Common.h>
#include "XmlStorage.h"
#include "TextsCore.h"
#include "FarUtil.h"

static const char * CONST_XML_VERSION21 = "2.1";
static const char * CONST_ROOT_NODE = "NetBox";
static const char * CONST_SESSION_NODE = "Session";
static const char * CONST_VERSION_ATTR = "version";
static const char * CONST_NAME_ATTR = "name";

TXmlStorage::TXmlStorage(const UnicodeString & AStorage,
                         const UnicodeString & StoredSessionsSubKey) :
  THierarchicalStorage(::ExcludeTrailingBackslash(AStorage)),
  FXmlDoc(nullptr),
  FCurrentElement(nullptr),
  FStoredSessionsSubKey(StoredSessionsSubKey),
  FFailed(0),
  FStoredSessionsOpened(false)
{
}

void TXmlStorage::Init()
{
  THierarchicalStorage::Init();
  FXmlDoc = new tinyxml2::XMLDocument();
}

TXmlStorage::~TXmlStorage()
{
  if (GetAccessMode() == smReadWrite)
  {
    WriteXml();
  }
  SAFE_DESTROY_EX(tinyxml2::XMLDocument, FXmlDoc);
}

bool TXmlStorage::ReadXml()
{
  CNBFile xmlFile;
  if (!xmlFile.OpenRead(GetStorage().c_str()))
  {
    return false;
  }
  size_t buffSize = static_cast<size_t>(xmlFile.GetFileSize() + 1);
  if (buffSize > 1000000)
  {
    return false;
  }
  std::string buff(buffSize, 0);
  if (!xmlFile.Read(&buff[0], buffSize))
  {
    return false;
  }

  FXmlDoc->Parse(buff.c_str());
  if (FXmlDoc->Error())
  {
    return false;
  }

  // Get and check root node
  tinyxml2::XMLElement * xmlRoot = FXmlDoc->RootElement();
  if (!xmlRoot)
    return false;
  const char * Value = xmlRoot->Value();
  if (!Value)
    return false;
  if (strcmp(Value, CONST_ROOT_NODE) != 0)
    return false;
  const char * Attr = xmlRoot->Attribute(CONST_VERSION_ATTR);
  if (!Attr)
    return false;
  uintptr_t Version = ::StrToVersionNumber(UnicodeString(Attr));
  if (Version < MAKEVERSIONNUMBER(2,0,0))
    return false;
  tinyxml2::XMLElement * Element = xmlRoot->FirstChildElement(ToStdString(FStoredSessionsSubKey).c_str());
  if (Element != nullptr)
  {
    FCurrentElement = FXmlDoc->RootElement();
    return true;
  }
  return false;
}

bool TXmlStorage::WriteXml()
{
  tinyxml2::XMLPrinter xmlPrinter;
  // xmlPrinter.SetIndent("  ");
  // xmlPrinter.SetLineBreak("\r\n");
  FXmlDoc->Accept(&xmlPrinter);
  const char * xmlContent = xmlPrinter.CStr();
  if (!xmlContent || !*xmlContent)
  {
    return false;
  }

  return (CNBFile::SaveFile(GetStorage().c_str(), xmlContent) == ERROR_SUCCESS);
}

bool TXmlStorage::Copy(TXmlStorage * /*Storage*/)
{
  ThrowNotImplemented(3020);
  bool Result = false;
  return Result;
}

void TXmlStorage::SetAccessMode(TStorageAccessMode Value)
{
  THierarchicalStorage::SetAccessMode(Value);
  switch (GetAccessMode())
  {
    case smRead:
      ReadXml();
      break;

    case smReadWrite:
    default:
      FXmlDoc->LinkEndChild(FXmlDoc->NewDeclaration());
      DebugAssert(FCurrentElement == nullptr);
      FCurrentElement = FXmlDoc->NewElement(CONST_ROOT_NODE);
      FCurrentElement->SetAttribute(CONST_VERSION_ATTR, CONST_XML_VERSION21);
      FXmlDoc->LinkEndChild(FCurrentElement);
      break;
  }
}

bool TXmlStorage::DoKeyExists(const UnicodeString & SubKey, bool /*ForceAnsi*/)
{
  UnicodeString K = PuttyMungeStr(SubKey);
  const tinyxml2::XMLElement * Element = FindChildElement(ToStdString(K));
  bool Result = Element != nullptr;
  return Result;
}

bool TXmlStorage::DoOpenSubKey(const UnicodeString & MungedSubKey, bool CanCreate)
{
  tinyxml2::XMLElement * OldCurrentElement = FCurrentElement;
  tinyxml2::XMLElement * Element = nullptr;
  std::string subKey = ToStdString(MungedSubKey);
  if (CanCreate)
  {
    if (FStoredSessionsOpened)
    {
      Element = FXmlDoc->NewElement(CONST_SESSION_NODE);
      Element->SetAttribute(CONST_NAME_ATTR, subKey.c_str());
    }
    else
    {
      Element = FXmlDoc->NewElement(subKey.c_str());
    }
    FCurrentElement->LinkEndChild(Element);
  }
  else
  {
    Element = FindChildElement(subKey);
  }
  bool Result = Element != nullptr;
  if (Result)
  {
    FSubElements.push_back(OldCurrentElement);
    FCurrentElement = Element;
    FStoredSessionsOpened = (MungedSubKey == FStoredSessionsSubKey);
  }
  return Result;
}

void TXmlStorage::CloseSubKey()
{
  THierarchicalStorage::CloseSubKey();
  if (FKeyHistory->GetCount() && !FSubElements.empty())
  {
    FCurrentElement = FSubElements.back();
    FSubElements.pop_back();
  }
  else
  {
    FCurrentElement = nullptr;
  }
}

bool TXmlStorage::DeleteSubKey(const UnicodeString & SubKey)
{
  bool Result = false;
  tinyxml2::XMLElement * Element = FindElement(SubKey);
  if (Element != nullptr)
  {
    FCurrentElement->DeleteChild(Element);
    Result = true;
  }
  return Result;
}

void TXmlStorage::GetSubKeyNames(TStrings * Strings)
{
  for (tinyxml2::XMLElement * Element = FCurrentElement->FirstChildElement();
       Element != nullptr; Element = Element->NextSiblingElement())
  {
    UnicodeString val = GetValue(Element);
    Strings->Add(PuttyUnMungeStr(val));
  }
}

void TXmlStorage::GetValueNames(TStrings * /*Strings*/) const
{
  ThrowNotImplemented(3022);
  // FRegistry->GetValueNames(Strings);
}

bool TXmlStorage::DeleteValue(const UnicodeString & Name)
{
  bool Result = false;
  tinyxml2::XMLElement * Element = FindElement(Name);
  if (Element != nullptr)
  {
    FCurrentElement->DeleteChild(Element);
    Result = true;
  }
  return Result;
}

void TXmlStorage::RemoveIfExists(const UnicodeString & Name)
{
  tinyxml2::XMLElement * Element = FindElement(Name);
  if (Element != nullptr)
  {
    FCurrentElement->DeleteChild(Element);
  }
}

void TXmlStorage::AddNewElement(const UnicodeString & Name, const UnicodeString & Value)
{
  std::string name = ToStdString(Name);
  std::string StrValue = ToStdString(Value);
  tinyxml2::XMLElement * Element = FXmlDoc->NewElement(name.c_str());
  Element->LinkEndChild(FXmlDoc->NewText(StrValue.c_str()));
  FCurrentElement->LinkEndChild(Element);
}

UnicodeString TXmlStorage::GetSubKeyText(const UnicodeString & Name) const
{
  tinyxml2::XMLElement * Element = FindElement(Name);
  if (!Element)
  {
    return UnicodeString();
  }
  if (ToUnicodeString(CONST_SESSION_NODE) == Name)
  {
    return ToUnicodeString(Element->Attribute(CONST_NAME_ATTR));
  }
  else
  {
    return ToUnicodeString(Element->GetText());
  }
}

tinyxml2::XMLElement * TXmlStorage::FindElement(const UnicodeString & Name) const
{
  for (const tinyxml2::XMLElement * Element = FCurrentElement->FirstChildElement();
       Element != nullptr; Element = Element->NextSiblingElement())
  {
    UnicodeString ElementName = ToUnicodeString(Element->Name());
    if (ElementName == Name)
    {
      return const_cast<tinyxml2::XMLElement *>(Element);
    }
  }
  return nullptr;
}

tinyxml2::XMLElement * TXmlStorage::FindChildElement(const std::string & subKey) const
{
  tinyxml2::XMLElement * Result = nullptr;
  // DebugAssert(FCurrentElement);
  if (FStoredSessionsOpened)
  {
    tinyxml2::XMLElement * Element = FCurrentElement->FirstChildElement(CONST_SESSION_NODE);
    if (Element && !strcmp(Element->Attribute(CONST_NAME_ATTR), subKey.c_str()))
    {
      Result = Element;
    }
  }
  else if (FCurrentElement)
  {
    Result = FCurrentElement->FirstChildElement(subKey.c_str());
  }
  return Result;
}

UnicodeString TXmlStorage::GetValue(tinyxml2::XMLElement * Element) const
{
  DebugAssert(Element);
  UnicodeString Result;
  if (FStoredSessionsOpened && Element->Attribute(CONST_NAME_ATTR))
  {
    Result = ToUnicodeString(Element->Attribute(CONST_NAME_ATTR));
  }
  else
  {
    Result = ToUnicodeString(Element->GetText());
  }
  return Result;
}

bool TXmlStorage::ValueExists(const UnicodeString & Value) const
{
  bool Result = false;
  tinyxml2::XMLElement * Element = FindElement(Value);
  if (Element != nullptr)
  {
    Result = true;
  }
  return Result;
}

size_t TXmlStorage::BinaryDataSize(const UnicodeString & /*Name*/) const
{
  ThrowNotImplemented(3026);
  size_t Result = 0; // FRegistry->GetDataSize(Name);
  return Result;
}

UnicodeString TXmlStorage::GetSource() const
{
  return GetStorage();
}

UnicodeString TXmlStorage::GetSource()
{
  return GetStorage();
}

bool TXmlStorage::ReadBool(const UnicodeString & Name, bool Default) const
{
  UnicodeString Result = ReadString(Name, L"");
  if (Result.IsEmpty())
  {
    return Default;
  }
  else
  {
    return ::AnsiCompareIC(Result, ::BooleanToEngStr(true)) == 0;
  }
}

TDateTime TXmlStorage::ReadDateTime(const UnicodeString & Name, const TDateTime & Default) const
{
  double Result = ReadFloat(Name, Default.GetValue());
  return TDateTime(Result);
}

double TXmlStorage::ReadFloat(const UnicodeString & Name, double Default) const
{
  return ::StrToFloatDef(GetSubKeyText(Name), Default);
}

intptr_t TXmlStorage::ReadInteger(const UnicodeString & Name, intptr_t Default) const
{
  return ::StrToIntDef(GetSubKeyText(Name), Default);
}

int64_t TXmlStorage::ReadInt64(const UnicodeString & Name, int64_t Default) const
{
  return ::StrToInt64Def(GetSubKeyText(Name), Default);
}

UnicodeString TXmlStorage::ReadStringRaw(const UnicodeString & Name, const UnicodeString & Default) const
{
  UnicodeString Result = GetSubKeyText(Name);
  return Result.IsEmpty() ? Default : Result;
}

size_t TXmlStorage::ReadBinaryData(const UnicodeString & /*Name*/,
  void * /*Buffer*/, size_t /*Size*/) const
{
  ThrowNotImplemented(3028);
  size_t Result = 0;
  return Result;
}

void TXmlStorage::WriteBool(const UnicodeString & Name, bool Value)
{
  WriteString(Name, ::BooleanToEngStr(Value));
}

void TXmlStorage::WriteDateTime(const UnicodeString & Name, const TDateTime & Value)
{
  WriteFloat(Name, Value);
}

void TXmlStorage::WriteFloat(const UnicodeString & Name, double Value)
{
  RemoveIfExists(Name);
  AddNewElement(Name, FORMAT(L"%.5f", Value));
}

void TXmlStorage::WriteStringRaw(const UnicodeString & Name, const UnicodeString & Value)
{
  RemoveIfExists(Name);
  AddNewElement(Name, Value);
}

void TXmlStorage::WriteInteger(const UnicodeString & Name, intptr_t Value)
{
  RemoveIfExists(Name);
  AddNewElement(Name, ::IntToStr(Value));
}

void TXmlStorage::WriteInt64(const UnicodeString & Name, int64_t Value)
{
  RemoveIfExists(Name);
  AddNewElement(Name, ::Int64ToStr(Value));
}

void TXmlStorage::WriteBinaryData(const UnicodeString & Name,
  const void * Buffer, size_t Size)
{
  RemoveIfExists(Name);
  AddNewElement(Name, ::StrToHex(UnicodeString(reinterpret_cast<const wchar_t *>(Buffer), Size), true));
}

intptr_t TXmlStorage::GetFailed()
{
  intptr_t Result = FFailed;
  FFailed = 0;
  return Result;
}
