#pragma once

#include <vcl.h>
#include "HierarchicalStorage.h"
#include "tinyxml2.h"

class TXmlStorage : public THierarchicalStorage
{
public:
  explicit TXmlStorage(const UnicodeString & AStorage, const UnicodeString & StoredSessionsSubKey);
  virtual void Init();
  virtual ~TXmlStorage();

  bool Copy(TXmlStorage * Storage);

  virtual void CloseSubKey();
  virtual bool DeleteSubKey(const UnicodeString & SubKey);
  virtual void GetSubKeyNames(TStrings * Strings);
  virtual bool ValueExists(const UnicodeString & Value) const;
  virtual bool DeleteValue(const UnicodeString & Name);
  virtual size_t BinaryDataSize(const UnicodeString & Name) const;
  virtual UnicodeString GetSource() const;
  virtual UnicodeString GetSource();

  virtual bool ReadBool(const UnicodeString & Name, bool Default) const;
  virtual intptr_t ReadInteger(const UnicodeString & Name, intptr_t Default) const;
  virtual int64_t ReadInt64(const UnicodeString & Name, int64_t Default) const;
  virtual TDateTime ReadDateTime(const UnicodeString & Name, const TDateTime & Default) const;
  virtual double ReadFloat(const UnicodeString & Name, double Default) const;
  virtual UnicodeString ReadStringRaw(const UnicodeString & Name, const UnicodeString & Default) const;
  virtual size_t ReadBinaryData(const UnicodeString & Name, void * Buffer, size_t Size) const;

  virtual void WriteBool(const UnicodeString & Name, bool Value);
  virtual void WriteInteger(const UnicodeString & Name, intptr_t Value);
  virtual void WriteInt64(const UnicodeString & Name, int64_t Value);
  virtual void WriteDateTime(const UnicodeString & Name, const TDateTime & Value);
  virtual void WriteFloat(const UnicodeString & Name, double Value);
  virtual void WriteStringRaw(const UnicodeString & Name, const UnicodeString & Value);
  virtual void WriteBinaryData(const UnicodeString & Name, const void * Buffer, size_t Size);

  virtual void GetValueNames(TStrings * Strings) const;

  virtual void SetAccessMode(TStorageAccessMode Value);
  virtual bool DoKeyExists(const UnicodeString & SubKey, bool ForceAnsi);
  virtual bool DoOpenSubKey(const UnicodeString & MungedSubKey, bool CanCreate);

protected:
  intptr_t GetFailed();
  void SetFailed(intptr_t Value) { FFailed = Value; }

private:
  UnicodeString GetSubKeyText(const UnicodeString & Name) const;
  tinyxml2::XMLElement * FindElement(const UnicodeString & Value) const;
  std::string ToStdString(const UnicodeString & String) const { return std::string(::W2MB(String.c_str()).c_str()); }
  UnicodeString ToUnicodeString(const char * String) const { return ::MB2W(String ? String : ""); }
  void RemoveIfExists(const UnicodeString & Name);
  void AddNewElement(const UnicodeString & Name, const UnicodeString & Value);
  tinyxml2::XMLElement * FindChildElement(const std::string & subKey) const;
  UnicodeString GetValue(tinyxml2::XMLElement * Element) const;

  bool ReadXml();
  bool WriteXml();

private:
  tinyxml2::XMLDocument * FXmlDoc;
  std::vector<tinyxml2::XMLElement *> FSubElements;
  tinyxml2::XMLElement * FCurrentElement;
  UnicodeString FStoredSessionsSubKey;
  intptr_t FFailed;
  bool FStoredSessionsOpened;
};
