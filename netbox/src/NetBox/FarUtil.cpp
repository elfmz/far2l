#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <MsgIDs.h>

#include "FarUtil.h"

bool CNBFile::OpenWrite(const wchar_t *fileName)
{
  DebugAssert(m_File == INVALID_HANDLE_VALUE);
  DebugAssert(fileName);
  m_LastError = ERROR_SUCCESS;

  m_File = ::CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (m_File == INVALID_HANDLE_VALUE)
  {
    m_LastError = ::GetLastError();
  }
  return (m_LastError == ERROR_SUCCESS);
}

bool CNBFile::OpenRead(const wchar_t *fileName)
{
  DebugAssert(m_File == INVALID_HANDLE_VALUE);
  DebugAssert(fileName);
  m_LastError = ERROR_SUCCESS;

  m_File = ::CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (m_File == INVALID_HANDLE_VALUE)
  {
    m_LastError = ::GetLastError();
  }
  return (m_LastError == ERROR_SUCCESS);
}

bool CNBFile::Read(void *buff, size_t &buffSize)
{
  DebugAssert(m_File != INVALID_HANDLE_VALUE);
  m_LastError = ERROR_SUCCESS;

  DWORD bytesRead = static_cast<DWORD>(buffSize);
  if (!ReadFile(m_File, buff, bytesRead, &bytesRead, nullptr))
  {
    m_LastError = ::GetLastError();
    buffSize = 0;
  }
  else
  {
    buffSize = static_cast<size_t>(bytesRead);
  }
  return (m_LastError == ERROR_SUCCESS);
}

bool CNBFile::Write(const void *buff, const size_t buffSize)
{
  DebugAssert(m_File != INVALID_HANDLE_VALUE);
  m_LastError = ERROR_SUCCESS;

  DWORD bytesWritten;
  if (!WriteFile(m_File, buff, static_cast<DWORD>(buffSize), &bytesWritten, nullptr))
  {
    m_LastError = ::GetLastError();
  }
  return (m_LastError == ERROR_SUCCESS);
}

int64_t CNBFile::GetFileSize()
{
  DebugAssert(m_File != INVALID_HANDLE_VALUE);
  m_LastError = ERROR_SUCCESS;

  LARGE_INTEGER fileSize;
  if (!GetFileSizeEx(m_File, &fileSize))
  {
    m_LastError = ::GetLastError();
    return -1;
  }
  return fileSize.QuadPart;
}

void CNBFile::Close()
{
  if (m_File != INVALID_HANDLE_VALUE)
  {
    ::CloseHandle(m_File);
    m_File = INVALID_HANDLE_VALUE;
  }
}

DWORD CNBFile::LastError() const
{
  return m_LastError;
}

DWORD CNBFile::SaveFile(const wchar_t *fileName, const std::vector<char>& fileContent)
{
  CNBFile f;
  if (f.OpenWrite(fileName) && !fileContent.empty())
  {
    f.Write(&fileContent[0], fileContent.size());
  }
  return f.LastError();
}

DWORD CNBFile::SaveFile(const wchar_t *fileName, const char *fileContent)
{
  DebugAssert(fileContent);
  CNBFile f;
  if (f.OpenWrite(fileName) && *fileContent)
  {
    f.Write(fileContent, strlen(fileContent));
  }
  return f.LastError();
}

DWORD CNBFile::LoadFile(const wchar_t *fileName, std::vector<char>& fileContent)
{
  fileContent.clear();

  CNBFile f;
  if (f.OpenRead(fileName))
  {
    const int64_t fs = f.GetFileSize();
    if (fs < 0)
    {
      return f.LastError();
    }
    if (fs == 0)
    {
      return ERROR_SUCCESS;
    }
    size_t s = static_cast<size_t>(fs);
    fileContent.resize(s);
    f.Read(&fileContent[0], s);
  }
  return f.LastError();
}
