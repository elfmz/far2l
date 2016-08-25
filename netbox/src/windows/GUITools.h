#pragma once

#include <BaseDefs.hpp>

// from shlobj.h
#define CSIDL_DESKTOP                   0x0000        // <desktop>
#define CSIDL_SENDTO                    0x0009        // <user name>\SendTo
#define CSIDL_DESKTOPDIRECTORY          0x0010        // <user name>\Desktop
#define CSIDL_COMMON_DESKTOPDIRECTORY   0x0019        // All Users\Desktop
#define CSIDL_APPDATA                   0x001a        // <user name>\Application Data
#define CSIDL_PROGRAM_FILES             0x0026        // C:\Program Files
#define CSIDL_PERSONAL                  0x0005        // My Documents

#include <FileMasks.h>

class TSessionData;

DEFINE_CALLBACK_TYPE0(TProcessMessagesEvent, void);

bool FindFile(UnicodeString & APath);
bool FindTool(const UnicodeString & Name, UnicodeString & APath);
bool FileExistsEx(const UnicodeString & APath);
bool ExecuteShell(const UnicodeString & APath, const UnicodeString & Params);
bool ExecuteShell(const UnicodeString & APath, const UnicodeString & Params,
  HANDLE & Handle);
bool ExecuteShellAndWait(HINSTANCE Handle, const UnicodeString & APath,
  const UnicodeString & Params, TProcessMessagesEvent ProcessMessages);
bool ExecuteShellAndWait(HINSTANCE Handle, const UnicodeString & Command,
  TProcessMessagesEvent ProcessMessages);
void OpenSessionInPutty(const UnicodeString & PuttyPath,
  TSessionData * SessionData);
bool SpecialFolderLocation(int PathID, UnicodeString & APath);
UnicodeString GetPersonalFolder();
UnicodeString ItemsFormatString(const UnicodeString & SingleItemFormat,
  const UnicodeString & MultiItemsFormat, intptr_t Count, const UnicodeString & FirstItem);
UnicodeString ItemsFormatString(const UnicodeString & SingleItemFormat,
  const UnicodeString & MultiItemsFormat, const TStrings * Items);
UnicodeString FileNameFormatString(const UnicodeString & SingleFileFormat,
  const UnicodeString & MultiFileFormat, const TStrings * AFiles, bool Remote);
UnicodeString UniqTempDir(const UnicodeString & BaseDir,
  const UnicodeString & Identity, bool Mask = false);
bool DeleteDirectory(const UnicodeString & ADirName);
UnicodeString FormatDateTimeSpan(const UnicodeString & TimeFormat, const TDateTime & DateTime);

class TLocalCustomCommand : public TFileCustomCommand
{
public:
  TLocalCustomCommand();
  explicit TLocalCustomCommand(
    const TCustomCommandData & Data, const UnicodeString & RemotePath, const UnicodeString & LocalPath);
  explicit TLocalCustomCommand(
    const TCustomCommandData & Data, const UnicodeString & RemotePath, const UnicodeString & LocalPath,
    const UnicodeString & AFileName, const UnicodeString & LocalFileName,
    const UnicodeString & FileList);
  virtual ~TLocalCustomCommand() {}

  virtual bool IsFileCommand(const UnicodeString & Command) const;
  bool HasLocalFileName(const UnicodeString & Command) const;

protected:
  virtual intptr_t PatternLen(const UnicodeString & Command, intptr_t Index) const;
  virtual bool PatternReplacement(intptr_t Index, const UnicodeString & Pattern,
    UnicodeString & Replacement, bool & Delimit) const;
  virtual void DelimitReplacement(UnicodeString & Replacement, wchar_t Quote);

private:
  UnicodeString FLocalPath;
  UnicodeString FLocalFileName;
};

#if 0
void ValidateMaskEdit(TFarComboBox * Edit);
void ValidateMaskEdit(TFarEdit * Edit);
#endif

#define PageantTool L"pageant.exe"
#define PuttygenTool L"puttygen.exe"

