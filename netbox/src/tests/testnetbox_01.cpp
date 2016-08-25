//------------------------------------------------------------------------------
// testnetbox_01.cpp
// Тесты для NetBox
// testnetbox_01 --run_test=netbox/test1 --log_level=all 2>&1 | tee res.txt
//------------------------------------------------------------------------------

// #include "leak_detector.h"
// extern int GC_find_leak;

#include <Classes.hpp>
#include <time.h>
#include <Winhttp.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include "boostdefines.hpp"
#define BOOST_TEST_MODULE "testnetbox_01"
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
// #include <boost/type_traits/is_base_of.hpp>

#include <FileBuffer.h>

#include "TestTexts.h"
#include "Common.h"
#include "FarPlugin.h"
#include "testutils.h"
#include "FastDelegate.h"
#include "CppProperties.h"

using namespace boost::unit_test;

//------------------------------------------------------------------------------
// stub
TCustomFarPlugin * FarPlugin = NULL;
//------------------------------------------------------------------------------

void FreeIEConfig(WINHTTP_CURRENT_USER_IE_PROXY_CONFIG* ie_config)
{
  if (ie_config->lpszAutoConfigUrl)
    GlobalFree(ie_config->lpszAutoConfigUrl);
  if (ie_config->lpszProxy)
    GlobalFree(ie_config->lpszProxy);
  if (ie_config->lpszProxyBypass)
    GlobalFree(ie_config->lpszProxyBypass);
}

/*******************************************************************************
            test suite
*******************************************************************************/

class base_fixture_t
{
public:
  base_fixture_t()
  {
    // BOOST_TEST_MESSAGE("base_fixture_t ctor");
    InitPlatformId();
  }

  virtual ~base_fixture_t()
  {
  }
public:
protected:
};

//------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(netbox)

BOOST_FIXTURE_TEST_CASE(test1, base_fixture_t)
{
  TList list;
  BOOST_CHECK_EQUAL(0, list.GetCount());
  TObject obj1;
  TObject obj2;
  if (1)
  {
    list.Add(&obj1);
    BOOST_CHECK_EQUAL(1, list.GetCount());
    list.Add(&obj2);
    BOOST_CHECK_EQUAL(2, list.GetCount());
    BOOST_CHECK_EQUAL(0, list.IndexOf(&obj1));
    BOOST_CHECK_EQUAL(1, list.IndexOf(&obj2));
  }
  list.Clear();
  if (1)
  {
    BOOST_CHECK_EQUAL(0, list.GetCount());
    list.Insert(0, &obj1);
    BOOST_CHECK_EQUAL(1, list.GetCount());
    list.Insert(0, &obj2);
    BOOST_CHECK_EQUAL(2, list.GetCount());
    BOOST_CHECK_EQUAL(1, list.IndexOf(&obj1));
    BOOST_CHECK_EQUAL(0, list.IndexOf(&obj2));
  }
  if (1)
  {
    list.Delete(1);
    BOOST_CHECK_EQUAL(1, list.GetCount());
    BOOST_CHECK_EQUAL(-1, list.IndexOf(&obj1));
    BOOST_CHECK_EQUAL(0, list.IndexOf(&obj2));
    BOOST_CHECK_EQUAL(1, list.Add(&obj1));
    list.Delete(0);
    BOOST_CHECK_EQUAL(0, list.IndexOf(&obj1));
    BOOST_CHECK_EQUAL(0, list.Remove(&obj1));
    BOOST_CHECK_EQUAL(-1, list.IndexOf(&obj1));
    BOOST_CHECK_EQUAL(0, list.GetCount());
  }
  if (1)
  {
    list.Add(&obj1);
    list.Add(&obj2);
    list.Extract(&obj1);
    BOOST_CHECK_EQUAL(1, list.GetCount());
    BOOST_CHECK_EQUAL(0, list.IndexOf(&obj2));
    list.Add(&obj1);
    BOOST_CHECK_EQUAL(2, list.GetCount());
    list.Move(0, 1);
    BOOST_CHECK_EQUAL(0, list.IndexOf(&obj1));
    BOOST_CHECK_EQUAL(1, list.IndexOf(&obj2));
  }
}

BOOST_FIXTURE_TEST_CASE(test2, base_fixture_t)
{
  UnicodeString str;
  if (1)
  {
    TStringList strings;
    BOOST_CHECK_EQUAL(0, strings.GetCount());
    BOOST_CHECK_EQUAL(0, strings.Add(L"line 1"));
    BOOST_CHECK_EQUAL(1, strings.GetCount());
    str = strings.GetString(0);
    // DEBUG_PRINTF(L"str = %s", str.c_str());
    BOOST_CHECK_EQUAL(W2MB(str.c_str()), "line 1");
    strings.SetString(0, L"line 0");
    BOOST_CHECK_EQUAL(1, strings.GetCount());
    str = strings.GetString(0);
    BOOST_CHECK_EQUAL(W2MB(str.c_str()), "line 0");
    strings.SetString(0, L"line 00");
    BOOST_CHECK_EQUAL(1, strings.GetCount());
    BOOST_CHECK_EQUAL(W2MB(strings.GetString(0).c_str()), "line 00");
    strings.Add(L"line 11");
    BOOST_CHECK_EQUAL(2, strings.GetCount());
    BOOST_CHECK_EQUAL(W2MB(strings.GetString(1).c_str()), "line 11");
    strings.Delete(1);
    BOOST_CHECK_EQUAL(1, strings.GetCount());
  }
  TStringList strings;
  if (1)
  {
    BOOST_CHECK_EQUAL(0, strings.GetCount());
    strings.Add(L"line 1");
    str = strings.GetText();
    BOOST_TEST_MESSAGE("str = " << W2MB(str.c_str()).c_str());
    // DEBUG_PRINTF(L"str = %s", BytesToHex(RawByteString(str.c_str(),  str.Length()), true, L',').c_str());
    BOOST_TEST_MESSAGE("str = " << W2MB(BytesToHex(RawByteString(str.c_str(),  str.Length()), true, L',').c_str()).c_str());
    BOOST_CHECK(_wcsicmp(str.c_str(), L"line 1\x0D\x0A") == 0);
  }
  if (1)
  {
    strings.Add(L"line 2");
    BOOST_CHECK_EQUAL(2, strings.GetCount());
    str = strings.GetText();
    BOOST_TEST_MESSAGE(L"str = " << str.c_str());
    BOOST_CHECK_EQUAL(W2MB(str.c_str()).c_str(), "line 1\r\nline 2\r\n");
    strings.Insert(0, L"line 0");
    BOOST_CHECK_EQUAL(3, strings.GetCount());
    str = strings.GetText();
    BOOST_TEST_MESSAGE(L"str = " << str.c_str());
    BOOST_CHECK_EQUAL(W2MB(str.c_str()).c_str(), "line 0\r\nline 1\r\nline 2\r\n");
    strings.SetObj(0, NULL);
    UnicodeString str = strings.GetString(0);
    BOOST_CHECK_EQUAL(W2MB(str.c_str()), "line 0");
  }
  {
    strings.SetString(0, L"line 12");
    BOOST_CHECK_EQUAL(W2MB(strings.GetString(0).c_str()), "line 12");
  }
}

BOOST_FIXTURE_TEST_CASE(test3, base_fixture_t)
{
  UnicodeString Text = L"text text text text text1\ntext text text text text2\n";
  TStringList Lines;
  Lines.SetText(Text);
  BOOST_CHECK_EQUAL(2, Lines.GetCount());
  BOOST_TEST_MESSAGE("Lines 0 = " << W2MB(Lines.GetString(0).c_str()));
  BOOST_TEST_MESSAGE("Lines 1 = " << W2MB(Lines.GetString(1).c_str()));
  BOOST_CHECK_EQUAL("text text text text text1", W2MB(Lines.GetString(0).c_str()).c_str());
  BOOST_CHECK_EQUAL("text text text text text2", W2MB(Lines.GetString(1).c_str()).c_str());
}

BOOST_FIXTURE_TEST_CASE(test4, base_fixture_t)
{
  UnicodeString Text = L"text, text text, text text1\ntext text text, text text2\n";
  TStringList Lines;
  Lines.SetCommaText(Text);
  BOOST_CHECK_EQUAL(5, Lines.GetCount());
  BOOST_CHECK_EQUAL(0, wcscmp(L"text", Lines.GetString(0).c_str()));
  BOOST_CHECK_EQUAL(0, wcscmp(L" text text", Lines.GetString(1).c_str()));
  BOOST_CHECK_EQUAL(0, wcscmp(L" text text1", Lines.GetString(2).c_str()));
  BOOST_CHECK_EQUAL(0, wcscmp(L"text text text", Lines.GetString(3).c_str()));
  BOOST_CHECK_EQUAL(0, wcscmp(L" text text2", Lines.GetString(4).c_str()));
  UnicodeString Text2 = Lines.GetCommaText();
  BOOST_TEST_MESSAGE("Text2 = '" << W2MB(Text2.c_str()) << "'");
  BOOST_CHECK_EQUAL(0, wcscmp(L"\"text\",\" text text\",\" text text1\",\"text text text\",\" text text2\"", Text2.c_str()));
}

BOOST_FIXTURE_TEST_CASE(test5, base_fixture_t)
{
  TStringList Lines;
  TObject obj1;
  Lines.InsertObject(0, L"line 1", &obj1);
  BOOST_CHECK(&obj1 == Lines.GetObjject(0));
}

BOOST_FIXTURE_TEST_CASE(test6, base_fixture_t)
{
  TStringList Lines;
  Lines.Add(L"bbb");
  Lines.Add(L"aaa");
  // BOOST_TEST_MESSAGE("Lines = " << W2MB(Lines.Text.c_str()).c_str());
  {
    Lines.SetSorted(true);
    // BOOST_TEST_MESSAGE("Lines = " << W2MB(Lines.Text.c_str()).c_str());
    BOOST_CHECK_EQUAL("aaa", W2MB(Lines.GetString(0).c_str()).c_str());
    BOOST_CHECK_EQUAL(2, Lines.GetCount());
  }
  {
    Lines.SetSorted(false);
    Lines.Add(L"Aaa");
    Lines.SetCaseSensitive(true);
    Lines.SetSorted(true);
    BOOST_CHECK_EQUAL(3, Lines.GetCount());
    // BOOST_TEST_MESSAGE("Lines = " << W2MB(Lines.Text.c_str()).c_str());
    BOOST_CHECK_EQUAL("aaa", W2MB(Lines.GetString(0).c_str()).c_str());
    BOOST_CHECK_EQUAL("Aaa", W2MB(Lines.GetString(1).c_str()).c_str());
    BOOST_CHECK_EQUAL("bbb", W2MB(Lines.GetString(2).c_str()).c_str());
  }
}

BOOST_FIXTURE_TEST_CASE(test7, base_fixture_t)
{
  TStringList Lines;
  {
    Lines.Add(L"bbb");
    BOOST_TEST_MESSAGE("before try");
    try
    {
      BOOST_TEST_MESSAGE("before BOOST_SCOPE_EXIT");
      BOOST_SCOPE_EXIT( (&Lines) )
      {
        BOOST_TEST_MESSAGE("in BOOST_SCOPE_EXIT");
        BOOST_CHECK(1 == Lines.GetCount());
      } BOOST_SCOPE_EXIT_END
      // throw std::exception("");
      BOOST_TEST_MESSAGE("after BOOST_SCOPE_EXIT_END");
    }
    catch (...)
    {
      BOOST_TEST_MESSAGE("in catch(...) block");
    }
    BOOST_TEST_MESSAGE("after try");
    Lines.Add(L"aaa");
    BOOST_CHECK(2 == Lines.GetCount());
  }
  Lines.Clear();
  Lines.BeginUpdate();
  {
    Lines.Add(L"bbb");
    BOOST_TEST_MESSAGE("before block");
    {
      BOOST_TEST_MESSAGE("before BOOST_SCOPE_EXIT");
      BOOST_SCOPE_EXIT( (&Lines) )
      {
        BOOST_TEST_MESSAGE("in BOOST_SCOPE_EXIT");
        BOOST_CHECK(1 == Lines.GetCount());
        Lines.EndUpdate();
      } BOOST_SCOPE_EXIT_END
      // throw std::exception("");
      BOOST_TEST_MESSAGE("after BOOST_SCOPE_EXIT_END");
    }
    BOOST_TEST_MESSAGE("after block");
    Lines.Add(L"aaa");
    BOOST_CHECK(2 == Lines.GetCount());
  }
  Lines.Clear();
  int cnt = 0;
  TStringList * Lines1 = new TStringList();
  int cnt1 = 0;
  TStringList * Lines2 = new TStringList();
  {
    Lines.BeginUpdate();
    cnt++;
    Lines1->BeginUpdate();
    cnt1++;
    Lines2->Add(L"bbb");
    BOOST_TEST_MESSAGE("before block");
    try
    {
      BOOST_TEST_MESSAGE("before BOOST_SCOPE_EXIT");
      BOOST_SCOPE_EXIT( (&Lines) (&Lines1) (&Lines2) (&cnt) (&cnt1) )
      {
        BOOST_TEST_MESSAGE("in BOOST_SCOPE_EXIT");
        Lines.EndUpdate();
        cnt--;
        Lines1->EndUpdate();
        cnt1--;
        delete Lines1;
        Lines1 = NULL;
        delete Lines2;
        Lines2 = NULL;
      } BOOST_SCOPE_EXIT_END
      BOOST_CHECK(1 == cnt);
      BOOST_CHECK(1 == cnt1);
      BOOST_CHECK(1 == Lines2->GetCount());
      throw std::exception("");
      BOOST_TEST_MESSAGE("after BOOST_SCOPE_EXIT_END");
    }
    catch (const std::exception & ex)
    {
      BOOST_TEST_MESSAGE("in catch block");
      BOOST_CHECK(NULL == Lines1);
      BOOST_CHECK(NULL == Lines2);
    }
    BOOST_TEST_MESSAGE("after block");
    BOOST_CHECK(0 == cnt);
    BOOST_CHECK(0 == cnt1);
    BOOST_CHECK(NULL == Lines1);
    BOOST_CHECK(NULL == Lines2);
  }
}

BOOST_FIXTURE_TEST_CASE(test8, base_fixture_t)
{
  UnicodeString ProgramsFolder;
  UnicodeString DefaultPuttyPathOnly = ::IncludeTrailingBackslash(ProgramsFolder) + L"PuTTY\\putty.exe";
  BOOST_CHECK(DefaultPuttyPathOnly == L"\\PuTTY\\putty.exe");
  BOOST_CHECK(L"" == ::ExcludeTrailingBackslash(::IncludeTrailingBackslash(ProgramsFolder)));
}

BOOST_FIXTURE_TEST_CASE(test9, base_fixture_t)
{
  UnicodeString Folder = L"C:\\Program Files\\Putty";
  BOOST_TEST_MESSAGE("ExtractFileDir = " << W2MB(::ExtractFileDir(Folder).c_str()).c_str());
  BOOST_CHECK(L"C:\\Program Files\\" == ::ExtractFileDir(Folder));
  BOOST_CHECK(L"C:\\Program Files\\" == ::ExtractFilePath(Folder));
  BOOST_TEST_MESSAGE("GetCurrentDir = " << W2MB(::GetCurrentDir().c_str()).c_str());
  BOOST_CHECK(::GetCurrentDir().Length() > 0);
  BOOST_CHECK(::DirectoryExists(::GetCurrentDir()));
}

BOOST_FIXTURE_TEST_CASE(test10, base_fixture_t)
{
  TDateTime dt1(23, 58, 59, 102);
  BOOST_TEST_MESSAGE("dt1 = " << dt1);
  BOOST_CHECK(dt1 > 0.0);
  unsigned short H, M, S, MS;
  dt1.DecodeTime(H, M, S, MS);
  BOOST_CHECK_EQUAL(H, 23);
  BOOST_CHECK_EQUAL(M, 58);
  BOOST_CHECK_EQUAL(S, 59);
  BOOST_CHECK_EQUAL(MS, 102);
}

BOOST_FIXTURE_TEST_CASE(test11, base_fixture_t)
{
  TDateTime dt1 = EncodeDateVerbose(2009, 12, 29);
  BOOST_TEST_MESSAGE("dt1 = " << dt1);
  // bg::date::ymd_type ymd(2009, 12, 29);
  // BOOST_TEST_MESSAGE("ymd.year = " << ymd.year << ", ymd.month = " << ymd.month << ", ymd.day = " << ymd.day);
  unsigned short Y, M, D;
  dt1.DecodeDate(Y, M, D);
  BOOST_TEST_MESSAGE("Y = " << Y << ", M = " << M << ", D = " << D);
  BOOST_CHECK(Y == 2009);
  BOOST_CHECK(M == 12);
  BOOST_CHECK(D == 29);
  int DOW = ::DayOfWeek(dt1);
  BOOST_CHECK_EQUAL(3, DOW);
}

BOOST_FIXTURE_TEST_CASE(test12, base_fixture_t)
{
  BOOST_TEST_MESSAGE("Is2000 = " << Is2000());
  BOOST_TEST_MESSAGE("IsWin7 = " << IsWin7());
  BOOST_TEST_MESSAGE("IsExactly2008R2 = " << IsExactly2008R2());
  TDateTime dt = ::EncodeDateVerbose(2009, 12, 29);
  FILETIME ft = ::DateTimeToFileTime(dt, dstmWin);
  BOOST_TEST_MESSAGE("ft.dwLowDateTime = " << ft.dwLowDateTime);
  BOOST_TEST_MESSAGE("ft.dwHighDateTime = " << ft.dwHighDateTime);
}

BOOST_FIXTURE_TEST_CASE(test13, base_fixture_t)
{
  UnicodeString str_value = ::IntToStr(1234);
  BOOST_TEST_MESSAGE("str_value = " << W2MB(str_value.c_str()));
  BOOST_CHECK(W2MB(str_value.c_str()) == "1234");
  int int_value = ::StrToInt(L"1234");
  BOOST_TEST_MESSAGE("int_value = " << int_value);
  BOOST_CHECK(int_value == 1234);
}

BOOST_FIXTURE_TEST_CASE(test14, base_fixture_t)
{
  TStringList Strings1;
  TStringList Strings2;
  Strings1.AddStrings(&Strings2);
  BOOST_CHECK(0 == Strings1.GetCount());
  Strings2.Add(L"lalalla");
  Strings1.AddStrings(&Strings2);
  BOOST_CHECK(1 == Strings1.GetCount());
  BOOST_CHECK(L"lalalla" == Strings1.GetString(0));
}

BOOST_FIXTURE_TEST_CASE(test15, base_fixture_t)
{
  UnicodeString res = ::IntToHex(10, 2);
  BOOST_TEST_MESSAGE("res = " << W2MB(res.c_str()));
  BOOST_CHECK(res == L"0A");
}

BOOST_FIXTURE_TEST_CASE(test16, base_fixture_t)
{
  {
    UnicodeString Name1 = L"1";
    UnicodeString Name2 = L"2";
    int res = ::AnsiCompareIC(Name1, Name2);
    BOOST_TEST_MESSAGE("res = " << res);
    BOOST_CHECK(res != 0);
    res = ::AnsiCompare(Name1, Name2);
    BOOST_TEST_MESSAGE("res = " << res);
    BOOST_CHECK(res != 0);
  }
  {
    UnicodeString Name1 = L"abc";
    UnicodeString Name2 = L"ABC";
    int res = ::AnsiCompareIC(Name1, Name2);
    BOOST_TEST_MESSAGE("res = " << res);
    BOOST_CHECK(res == 0);
    res = ::AnsiCompare(Name1, Name2);
    BOOST_TEST_MESSAGE("res = " << res);
    BOOST_CHECK(res != 0);
  }
  {
    UnicodeString Name1 = L"Unlimited";
    UnicodeString Name2 = L"Unlimited";
    BOOST_CHECK(::AnsiSameText(Name1, Name2));
  }
}

BOOST_FIXTURE_TEST_CASE(test17, base_fixture_t)
{
  TStringList List1;
  List1.SetText(L"123\n456");
  BOOST_CHECK(2 == List1.GetCount());
  BOOST_TEST_MESSAGE("List1.GetString(0] = " << W2MB(List1.GetString(0).c_str()));
  BOOST_CHECK("123" == W2MB(List1.GetString(0).c_str()));
  BOOST_TEST_MESSAGE("List1.GetString(1] = " << W2MB(List1.GetString(1).c_str()));
  BOOST_CHECK("456" == W2MB(List1.GetString(1).c_str()));
  List1.Move(0, 1);
  BOOST_TEST_MESSAGE("List1.GetString(0] = " << W2MB(List1.GetString(0).c_str()));
  BOOST_CHECK("456" == W2MB(List1.GetString(0).c_str()));
  BOOST_TEST_MESSAGE("List1.GetString(1] = " << W2MB(List1.GetString(1).c_str()));
  BOOST_CHECK("123" == W2MB(List1.GetString(1).c_str()));
}

BOOST_FIXTURE_TEST_CASE(test18, base_fixture_t)
{
  {
    UnicodeString Key = L"Interface";
    UnicodeString Res = ::CutToChar(Key, L'\\', false);
    BOOST_CHECK(Key.IsEmpty());
    BOOST_CHECK("Interface" == W2MB(Res.c_str()));
  }
  {
    UnicodeString Key = L"Interface\\SubKey";
    UnicodeString Res = ::CutToChar(Key, L'\\', false);
    BOOST_CHECK("SubKey" == W2MB(Key.c_str()));
    BOOST_CHECK("Interface" == W2MB(Res.c_str()));
  }
}

BOOST_FIXTURE_TEST_CASE(test19, base_fixture_t)
{
  TStringList Strings1;
  Strings1.Add(L"Name1=Value1");
  BOOST_CHECK(0 == Strings1.IndexOfName(L"Name1"));
}

BOOST_FIXTURE_TEST_CASE(test20, base_fixture_t)
{
  TDateTime DateTime = Now();
  unsigned short H, M, S, MS;
  DateTime.DecodeTime(H, M, S, MS);
  // UnicodeString str = ::FormatDateTime(L"HH:MM:SS", DateTime);
  UnicodeString str = FORMAT("%02d:%02d:%02d", H, M, S);
  BOOST_TEST_MESSAGE("str = " << W2MB(str.c_str()));
  // BOOST_CHECK(str == L"20:20:20");
}

BOOST_FIXTURE_TEST_CASE(test21, base_fixture_t)
{
  UnicodeString str = ::FormatFloat(L"#,##0", 23.456);
  BOOST_TEST_MESSAGE("str = " << W2MB(str.c_str()));
  // BOOST_CHECK(str.c_str() == L"23.46");
  BOOST_CHECK("23.46" == W2MB(str.c_str()));
}

BOOST_FIXTURE_TEST_CASE(test22, base_fixture_t)
{
  UnicodeString FileName = L"testfile";
  ::DeleteFile(FileName);
  std::string str = "test string";
  {
    unsigned int CreateAttrs = FILE_ATTRIBUTE_NORMAL;
    HANDLE FileHandle = ::CreateFile(FileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                               NULL, CREATE_ALWAYS, CreateAttrs, 0);
    BOOST_CHECK(FileHandle != 0);
    TStream * FileStream = new TSafeHandleStream(FileHandle);
    TFileBuffer * BlockBuf = new TFileBuffer();
    // BlockBuf->SetSize(1024);
    BlockBuf->SetPosition(0);
    BlockBuf->Insert(0, str.c_str(), str.size());
    BOOST_TEST_MESSAGE("BlockBuf->GetSize = " << BlockBuf->GetSize());
    BOOST_CHECK(BlockBuf->GetSize() == str.size());
    BlockBuf->WriteToStream(FileStream, BlockBuf->GetSize());
    delete FileStream; FileStream = NULL;
    delete BlockBuf; BlockBuf = NULL;
    ::CloseHandle(FileHandle);
    BOOST_TEST_MESSAGE("FileName1 = " << W2MB(FileName.c_str()));
    BOOST_REQUIRE(::FileExists(FileName));
  }
  {
    BOOST_TEST_MESSAGE("FileName2 = " << W2MB(FileName.c_str()));
    TSearchRec Rec;
    BOOST_CHECK(FileSearchRec(FileName, Rec));
  }
  {
    HANDLE File = ::CreateFile(
                    FileName.c_str(),
                    GENERIC_READ,
                    0, // FILE_SHARE_READ,
                    NULL,
                    OPEN_ALWAYS, // OPEN_EXISTING,
                    0, // FILE_ATTRIBUTE_NORMAL, // 0,
                    NULL);
    DEBUG_PRINTF(L"File = %d", File);
    TStream * FileStream = new TSafeHandleStream(File);
    TFileBuffer * BlockBuf = new TFileBuffer();
    BlockBuf->ReadStream(FileStream, str.size(), true);
    BOOST_TEST_MESSAGE("BlockBuf->GetSize = " << BlockBuf->GetSize());
    BOOST_CHECK(BlockBuf->GetSize() == str.size());
    delete FileStream; FileStream = NULL;
    delete BlockBuf; BlockBuf = NULL;
    ::CloseHandle(File);
  }
}

BOOST_FIXTURE_TEST_CASE(test23, base_fixture_t)
{
  UnicodeString Dir1 = L"subdir1";
  UnicodeString Dir2 = L"subdir1/subdir2";
  ::RemoveDir(Dir2);
  ::RemoveDir(Dir1);
  BOOST_TEST_MESSAGE("DirectoryExists(Dir2) = " << DirectoryExists(Dir2));
  BOOST_CHECK(!::DirectoryExists(Dir2));
  ::ForceDirectories(Dir2);
  BOOST_CHECK(::DirectoryExists(Dir2));
  ::RemoveDir(Dir2);
  ::ForceDirectories(Dir2);
  BOOST_CHECK(::DirectoryExists(Dir2));
  BOOST_CHECK(::RecursiveDeleteFile(Dir1, false));
  BOOST_CHECK(!::DirectoryExists(Dir1));
}

BOOST_FIXTURE_TEST_CASE(test24, base_fixture_t)
{
  TDateTime now = Now();
  BOOST_TEST_MESSAGE("now = " << (double)now);
  BOOST_CHECK(now > 0.0);
}

BOOST_FIXTURE_TEST_CASE(test25, base_fixture_t)
{
#if 0
  GC_find_leak = 1;
  int * i = (int *)malloc(sizeof(int));
  CHECK_LEAKS();
#endif
}

//------------------------------------------------------------------------------

class CBaseClass
{
protected:
  char * m_name;
public:
  CBaseClass(char * name) : m_name(name) {};
  void SimpleMemberFunction(int num, char * str)
  {
    printf("In SimpleMemberFunction in %s. Num=%d, str = %s\n", m_name, num, str);
  }
  int SimpleMemberFunctionReturnsInt(int num, char * str)
  {
    printf("In SimpleMemberFunction in %s. Num=%d, str = %s\n", m_name, num, str); return -1;
  }
  void ConstMemberFunction(int num, char * str) const
  {
    printf("In ConstMemberFunction in %s. Num=%d, str = %s\n", m_name, num, str);
  }
  virtual void SimpleVirtualFunction(int num, char * str)
  {
    printf("In SimpleVirtualFunction in %s. Num=%d, str = %s\n", m_name, num, str);
  }
  static void StaticMemberFunction(int num, char * str)
  {
    printf("In StaticMemberFunction. Num=%d, str =%s\n", num, str);
  }
};

BOOST_FIXTURE_TEST_CASE(test26, base_fixture_t)
{
  DEFINE_CALLBACK_TYPE2(TEvent, int, int, char *);
  TEvent sig;

  CBaseClass a("Base A");
  sig = fastdelegate::MakeDelegate(&a, &CBaseClass::SimpleMemberFunctionReturnsInt);
  int Result = sig(10, "abc");
  BOOST_TEST_MESSAGE("Result = " << Result);
  BOOST_CHECK(Result == -1);
}
//------------------------------------------------------------------------------
BOOST_FIXTURE_TEST_CASE(test27, base_fixture_t)
{
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ProxyConfig = {0};
    if (!WinHttpGetIEProxyConfigForCurrentUser(&ProxyConfig))
    {
        DWORD Err = GetLastError();
        switch (Err)
        {
        case ERROR_FILE_NOT_FOUND:
            BOOST_TEST_MESSAGE("The error is ERROR_FILE_NOT_FOUND");
            break;
        case ERROR_WINHTTP_INTERNAL_ERROR:
            BOOST_TEST_MESSAGE("ERROR_WINHTTP_INTERNAL_ERROR");
            break;
        case ERROR_NOT_ENOUGH_MEMORY:
            BOOST_TEST_MESSAGE("ERROR_NOT_ENOUGH_MEMORY");
            break;
        default:
            BOOST_TEST_MESSAGE("Look up error in header file.");
        }
    }
    else
    {
        //no error so check the proxy settings and free any strings
        BOOST_TEST_MESSAGE("AutoConfigDetect is: " << ProxyConfig.fAutoDetect);
        if (NULL != ProxyConfig.lpszAutoConfigUrl)
        {
            BOOST_TEST_MESSAGE("AutoConfigURL is: " << W2MB(ProxyConfig.lpszAutoConfigUrl));
        }
        if (NULL != ProxyConfig.lpszProxy)
        {
            BOOST_TEST_MESSAGE("AutoConfigProxy is: " << W2MB(ProxyConfig.lpszProxy));
        }
        if (NULL != ProxyConfig.lpszProxyBypass)
        {
            BOOST_TEST_MESSAGE("AutoConfigBypass is: " << W2MB(ProxyConfig.lpszProxyBypass));
        }
        FreeIEConfig(&ProxyConfig);
    }
}
//------------------------------------------------------------------------------
BOOST_FIXTURE_TEST_CASE(test28, base_fixture_t)
{
  DEBUG_PRINTF(L"1");
  if (1)
  {
    UnicodeString Buf;
    UnicodeString DestFileName(L"FileName");
    TFileBuffer AsciiBuf;
    AsciiBuf.SetSize(0x1000);
    DEBUG_PRINTF(L"2");
    Buf.Clear();
    Buf.SetLength(MAX_PATH * 2);
    DEBUG_PRINTF(L"AsciiBuf.GetSize = %lld", AsciiBuf.GetSize());
    swprintf_s(const_cast<wchar_t *>(Buf.c_str()), Buf.Length(), L"C%s %lld %s",
      L"",
      AsciiBuf.GetSize(),
      DestFileName.c_str());
    DEBUG_PRINTF(L"3");
    BOOST_TEST_MESSAGE("Buf1 = " << AnsiString(Buf).c_str());
    DEBUG_PRINTF(L"Buf = %s", Buf.c_str());
    BOOST_CHECK(Buf.GetLength() > 0);
  }
  if (1)
  {
    UnicodeString Buf;
    Buf.SetLength(20);
    UnicodeString DestFileName(L"FileName");
    TFileBuffer AsciiBuf;
    // swprintf_s causes error
    swprintf(const_cast<wchar_t *>(Buf.c_str()), L"C%s %lld %s",
      L"",
      AsciiBuf.GetSize(),
      DestFileName.c_str());
    BOOST_TEST_MESSAGE("Buf2 = " << AnsiString(Buf).c_str());
    BOOST_CHECK(AnsiString(Buf) == "C 0 FileName");
    BOOST_CHECK("C 0 FileName" == AnsiString(Buf));
    BOOST_CHECK(AnsiString(Buf) == AnsiString("C 0 FileName"));
    BOOST_CHECK(Buf == L"C 0 FileName");
    BOOST_CHECK(L"C 0 FileName" == Buf);
  }
  DEBUG_PRINTF(L"4");
  if (1)
  {
    // Ctrl = T, Line = 1338899268 0 1338899268 0
    UnicodeString Line = L"1338899268 0 1338899268 0";
    unsigned long MTime, ATime;
    if (swscanf(Line.c_str(), L"%ld %*d %ld %*d",  &MTime, &ATime) != 2)
      BOOST_FAIL("swscanf");
    BOOST_CHECK(MTime == 1338899268LU);
    BOOST_CHECK(ATime == 1338899268LU);
  }
  DEBUG_PRINTF(L"5");
  if (1)
  {
    intptr_t errCode = 0xFF;
    wchar_t codeNum[16];
    DEBUG_PRINTF(L"6");
    // swprintf_s(codeNum, sizeof(codeNum), L"[0x%08X]", errCode);  // Causes AV x64
    swprintf(codeNum, L"[0x%08X]", errCode);
    DEBUG_PRINTF(L"7");
    BOOST_TEST_MESSAGE("codeNum = " << AnsiString(codeNum).c_str());
    DEBUG_PRINTF(L"8");
    BOOST_CHECK(AnsiString(codeNum) == AnsiString("[0x000000FF]")); // Causes AV x64
    // BOOST_CHECK(AnsiString(codeNum) == AnsiString(""));
    BOOST_CHECK(wcscmp(codeNum, L"[0x000000FF]") == 0);
  }
  DEBUG_PRINTF(L"9");
}
//------------------------------------------------------------------------------
#if 0
class Foo
{
public:
    Foo(int j) { i=new int[j]; }
    ~Foo() { delete i; }
private:
    int* i;
};

class Bar: public Foo
{
public:
    Bar(int j): Foo(j) { i=new char[j]; }
    ~Bar() { delete i; }
private:
    char* i;
};


BOOST_FIXTURE_TEST_CASE(test29, base_fixture_t)
{
  Foo* f=new Foo(100);
  Foo* b=new Bar(200);
  *f=*b;
  delete f;
  delete b;
}
#endif
//------------------------------------------------------------------------------
BOOST_FIXTURE_TEST_CASE(test30, base_fixture_t)
{
  UnicodeString Instructions = L"Using keyboard authentication.\x0A\x0A\x0APlease enter your password.";
  BOOST_TEST_MESSAGE("Instructions = " << AnsiString(Instructions).c_str());
  UnicodeString Instructions2 = ReplaceStrAll(Instructions, L"\x0D\x0A", L"\x01");
  Instructions2 = ReplaceStrAll(Instructions, L"\x0A\x0D", L"\x01");
  Instructions2 = ReplaceStrAll(Instructions, L"\x0A", L"\x01");
  Instructions2 = ReplaceStrAll(Instructions, L"\x0D", L"\x01");
  Instructions2 = ReplaceStrAll(Instructions2, L"\x01", L"\x0D\x0A");
  BOOST_TEST_MESSAGE("Instructions2 = " << AnsiString(Instructions2).c_str());
  BOOST_CHECK(wcscmp(Instructions2.c_str(), UnicodeString(L"Using keyboard authentication.\x0D\x0A\x0D\x0A\x0D\x0APlease enter your password.").c_str()) == 0);
}
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE_END()
