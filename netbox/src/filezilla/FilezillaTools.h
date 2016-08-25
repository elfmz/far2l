
#ifndef FileZillaToolsH
#define FileZillaToolsH

#include <ctime>
#include <Classes.hpp>

class CFileZillaTools : public TObject
{
public:
  virtual void PreserveDownloadFileTime(HANDLE AHandle, void * UserData) = 0;
  virtual bool GetFileModificationTimeInUtc(const wchar_t * FileName, struct tm & Time) = 0;
  virtual wchar_t * LastSysErrorMessage() const = 0;
  virtual std::wstring GetClientString() const = 0;
};

#endif // FileZillaToolsH
