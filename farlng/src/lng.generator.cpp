/*
Copyright (c) 2003 WARP ItSelf
Copyright (c) 2005 WARP ItSelf & Alex Yaroslavsky
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "lng.common.h"
#include <vector>
#include <memory>
#include <sys/stat.h>
#include <KeyFileHelper.h>
#include <utils.h>
#define VERSION "v1.5"

void UnquoteIfNeeded (char *lpStr)
{
	int nLen = strlen (lpStr);

	if ( *lpStr != '"' || lpStr[nLen-1] != '"' )
		return;

	int i = 0;

	for (; i < nLen-2; i++)
		lpStr[i] = lpStr[i+1];

	lpStr[i] = 0;
}

struct LanguageEntry {
	char *lpLNGFileName;
	char *lpLNGFileNameTemp;

	char *lpLanguageName;
	char *lpLanguageDescription;

	HANDLE hLNGFile;

	DWORD dwCRC32;
	DWORD dwOldCRC32;

	int cNeedUpdate;

	int nEncoding;
};


bool ReadFromBuffer (
		char *&lpStart,
		char *&lpEnd,
		bool bProcessEscape = true
		)
{
	bool bInQuotes = false;
	bool bEscape = false;

	while ( true )
	{
		while ( *lpStart && ( (*lpStart == ' ') || (*lpStart == '\n') || (*lpStart == '\r') || (*lpStart == '\t') ) )
			lpStart++;

		if ( *lpStart == '#' )
		{
			while ( *lpStart != '\r' && *lpStart != '\n' )
				lpStart++;
		}
		else
			break;
	}

	if ( *lpStart )
	{
		lpEnd = lpStart;

		do {
			if ( bProcessEscape )
			{
				if ( (*lpEnd == '"') && !bEscape )
					bInQuotes = !bInQuotes;

				if ( ((*lpEnd == ' ') || (*lpEnd == '\t')) && !bInQuotes )
					break;

				if ( (*lpEnd == '\\') && !bEscape && bInQuotes )
					bEscape = true;
				else
					bEscape = false;
			}

			if ( (*lpEnd == '\n') || (*lpEnd == '\r') )
				break;

			lpEnd++;

		} while ( *lpEnd );

		return true;
	}

	return false;
}

bool ReadFromBufferEx (
		char *&lpStart,
		char **lpParam,
		bool bProcessEscape = true
		)
{
	char *lpEnd = lpStart;

	if ( ReadFromBuffer (lpStart, lpEnd, bProcessEscape) )
	{
		*lpParam = (char*)malloc (lpEnd-lpStart+1);
		memset (*lpParam, 0, lpEnd-lpStart+1);
		memcpy (*lpParam, lpStart, lpEnd-lpStart);
		lpStart = lpEnd;

		return true;
	}

	return false;
}

bool ReadComments (
		char *&lpRealStart,
		char **lpParam,
		const char *lpCmtMark,
		const char *lpCmtReplace
		)
{
	char *lpStart = lpRealStart;
	char *lpEnd = lpStart;
	int dwSize = 1;
	int nCmtMarkLen = strlen (lpCmtMark);
	int nCmtReplaceLen = strlen (lpCmtReplace);
	bool bFirst = true;

	*lpParam = NULL;

	while ( true )
	{
		if ( ReadFromBuffer (lpStart, lpEnd, false) )
		{
			if ( strncmp (lpStart, lpCmtMark, nCmtMarkLen) )
				break;

			dwSize += lpEnd-lpStart+2+nCmtReplaceLen-nCmtMarkLen;

			*lpParam = (char*)realloc (*lpParam, dwSize);

			if ( bFirst )
			{
				**lpParam = 0;
				bFirst = false;
			}

			strcat (*lpParam, lpCmtReplace);
			strncat (*lpParam, lpStart+nCmtMarkLen, lpEnd-lpStart-nCmtMarkLen);
			strcat (*lpParam, "\r\n" );

			lpRealStart = lpStart = lpEnd;
		}
		else
			break;
	}

	return ( *lpParam == NULL ? false : true );
}

void SmartWrite (
		HANDLE hFile,
		const char *lpStr,
		DWORD *pCRC32,
		int nOutCP
		)
{
	DWORD dwWritten;

	if ( nOutCP == CP_UTF8 )
	{
		WINPORT(WriteFile) (hFile, lpStr, strlen(lpStr), &dwWritten, NULL);
		*pCRC32 = CRC32(*pCRC32, lpStr, dwWritten);
	}
	else
	{
		DWORD dwSize = WINPORT(MultiByteToWideChar)(CP_UTF8, 0, lpStr, -1, NULL, 0);

		wchar_t* pBuffer = new wchar_t[dwSize+1];

		WINPORT(MultiByteToWideChar)(CP_UTF8, 0, lpStr, -1, pBuffer, dwSize);

		if ( nOutCP == 1200/*CP_UNICODE*/ )
		{
			WINPORT(WriteFile) (hFile, pBuffer, (dwSize-1)*sizeof(wchar_t), &dwWritten, NULL);
			*pCRC32 = CRC32(*pCRC32, (const char*)pBuffer, dwWritten);
		}
		else
		{
			DWORD dwSizeAnsi = WINPORT(WideCharToMultiByte)(nOutCP, 0, pBuffer, -1, NULL, 0, NULL, NULL);

			char* pAnsiBuffer = new char[dwSizeAnsi+1];

			WINPORT(WideCharToMultiByte)(nOutCP, 0, pBuffer, -1, pAnsiBuffer, dwSize, NULL, NULL);

			WINPORT(WriteFile) (hFile, pAnsiBuffer, dwSizeAnsi-1, &dwWritten, NULL);
			*pCRC32 = CRC32(*pCRC32, pAnsiBuffer, dwWritten);

			delete [] pAnsiBuffer;
		}

		delete [] pBuffer;
	}
}

char *GetTempName ()
{
	std::vector<WCHAR> tmp(MAX_PATH);
	WINPORT(GetTempFileName) (L".", L"lngg", 0, &tmp[0]);
	return strdup(Wide2MB(&tmp[0]).c_str());
	//char *lpTempName = (char*)malloc (MAX_PATH);
	//return lpTempName;
}


int ReadInteger(char*& lpStart)
{
	char* lpTemp;

	ReadFromBufferEx (lpStart, &lpTemp);
	int nResult = atol(lpTemp);

	free (lpTemp);

	return nResult;
}

void WriteSignatureIfNeeded(HANDLE hFile, int nEncoding)
{
	DWORD dwID;
	DWORD dwWritten;

	if ( nEncoding == CP_UTF8 )
	{
		dwID = 0xBFBBEF;
		WINPORT(WriteFile)(hFile, &dwID, 3, &dwWritten, NULL);
	}
	else

	if ( nEncoding == 1200/*CP_UNICODE*/ )
	{
		dwID = 0xFEFF;
		WINPORT(WriteFile)(hFile, &dwID, 2, &dwWritten, NULL);
	}
}

bool CheckExists(const char *path)
{
	struct stat s = {0};
	return stat(path, &s)==0;
}

int main_generator (int argc, char** argv)
{
	printf (".LNG Generator " VERSION "\n\r");
	printf ("Copyright (C) 2003-2009 WARP ItSelf\n\r");
	printf ("Copyright (C) 2005 WARP ItSelf & Alex Yaroslavsky\n\n\r");

	if ( argc < 2 )
	{
		printf ("Usage: generator [options] feed_file\n\r");
		printf ("\nOptions:\n");
		printf ("\t-i filename - optional ini file with update status.\n\r");
		printf ("\t-ol output_path - language files output path.\n\r");
		printf ("\t-oh output_path - header file output path.\n\r");
		printf ("\t-nc - don't write copyright info to generated files.\n\r");
		printf ("\t-e - output encoding set in feed file for each output file (UTF8 otherwise).\n\r");
		return 0;
	}

	std::unique_ptr<KeyFileHelper> key_file;

	char *lpLNGOutputPath = NULL;
	char *lpHOutputPath = NULL;

	bool bWriteCopyright = true;

	bool bOutputInUTF8 = true;

	if ( argc > 2 )
	{
		for (int i = 1; i < argc-1; i++)
		{
			if ( strcmp (argv[i],"-nc") == 0 )
				bWriteCopyright = false;
			else

			if ( strcmp (argv[i],"-e") == 0 )
				bOutputInUTF8 = false;
			else

			if ( strcmp (argv[i],"-i") == 0 && ++i < argc-1 )
			{
				key_file.reset(new KeyFileHelper(argv[i]));
//				lpIniFileName = (char*)malloc (MAX_PATH);
//				GetFullPathName (argv[i], MAX_PATH, lpIniFileName, NULL);
			}
			else

			if ( strcmp (argv[i],"-ol") == 0 && ++i < argc-1 )
			{
				lpLNGOutputPath = strdup(argv[i]);
				UnquoteIfNeeded (lpLNGOutputPath);
			}
			else

			if ( strcmp (argv[i],"-oh") == 0 && ++i < argc-1 )
			{
				lpHOutputPath = strdup (argv[i]);

				UnquoteIfNeeded (lpHOutputPath);
			}
		}
	}

				fprintf(stderr, "lpLNGOutputPath=%s\n", lpLNGOutputPath);

	HANDLE hFeedFile = WINPORT(CreateFile) (
			MB2Wide(argv[argc-1]).c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			0,
			NULL
			);

	if ( hFeedFile == INVALID_HANDLE_VALUE )
	{
		printf ("ERROR: Can't open the feed file, exiting.\n\r");
		return 0;
	}

	DWORD dwRead;
//	DWORD dwWritten;
	DWORD dwID;

	bool bUTF8 = false;

	WINPORT(ReadFile) (hFeedFile, &dwID, 3, &dwRead, NULL);

	bUTF8 = ((dwID & 0x00FFFFFF) == 0xBFBBEF);

	if ( !bUTF8 )
		WINPORT(SetFilePointer) (hFeedFile, 0, NULL, FILE_BEGIN);

	bool bUpdate;

	LanguageEntry *pLangEntries;

	DWORD dwHeaderCRC32;
	DWORD dwHeaderOldCRC32;

	char *lpHPPFileName = NULL;
	char *lpHPPFileNameTemp = GetTempName ();

	char *lpFullName = (char*)malloc (MAX_PATH);

	char *lpString = (char*)malloc (1024);

	DWORD dwSize = WINPORT(GetFileSize) (hFeedFile, NULL);

	char *pFeedBuffer = (char*)malloc (dwSize+1);
	memset (pFeedBuffer, 0, dwSize+1);

	WINPORT(ReadFile) (hFeedFile, pFeedBuffer, dwSize, &dwRead, NULL);

	char *lpStart = pFeedBuffer;

	// read h filename

	ReadFromBufferEx (lpStart, &lpHPPFileName);
	UnquoteIfNeeded (lpHPPFileName);

	// read h encoding
	int nHPPEncoding = bOutputInUTF8 ? CP_UTF8 : ReadInteger(lpStart);

	// read language count
	int dwLangs = ReadInteger(lpStart);

	if ( dwLangs > 0 )
	{
		sprintf (lpFullName, "%s/%s", lpHOutputPath?lpHOutputPath:".", lpHPPFileName);

		dwHeaderCRC32 = 0;
		dwHeaderOldCRC32 = (CheckExists(lpFullName) && key_file) ? key_file->GetInt( lpFullName, "CRC32")  : 0;

		HANDLE hHFile = WINPORT(CreateFile) (
				MB2Wide(lpHPPFileNameTemp).c_str(),
				GENERIC_WRITE,
				FILE_SHARE_READ,
				NULL,
				CREATE_ALWAYS,
				0,
				NULL
				);

		if ( hHFile != INVALID_HANDLE_VALUE )
		{
			WriteSignatureIfNeeded(hHFile, nHPPEncoding);

				if ( bWriteCopyright )
				{
					sprintf (lpString, "// This C++ include file was generated by .LNG Generator " VERSION "\r\n// Copyright (C) 2003-2005 WARP ItSelf\r\n// Copyright (C) 2005 WARP ItSelf & Alex Yaroslavsky\r\n\r\n");
					SmartWrite (hHFile, lpString, &dwHeaderCRC32, nHPPEncoding);
				}

				pLangEntries = (LanguageEntry*)malloc (dwLangs*sizeof (LanguageEntry));

				// read language names and create .lng files

				for (int i = 0; i < dwLangs; i++)
				{
					ReadFromBufferEx (lpStart, &pLangEntries[i].lpLNGFileName);

					pLangEntries[i].nEncoding = bOutputInUTF8 ? CP_UTF8 : ReadInteger(lpStart);

					ReadFromBufferEx (lpStart, &pLangEntries[i].lpLanguageName);
					ReadFromBufferEx (lpStart, &pLangEntries[i].lpLanguageDescription);

					UnquoteIfNeeded (pLangEntries[i].lpLanguageName);
					UnquoteIfNeeded (pLangEntries[i].lpLanguageDescription);
					UnquoteIfNeeded (pLangEntries[i].lpLNGFileName);

					sprintf (lpFullName, "%s/%s", lpLNGOutputPath?lpLNGOutputPath:".", pLangEntries[i].lpLNGFileName);

					pLangEntries[i].cNeedUpdate = 0;

					pLangEntries[i].dwCRC32 = 0;
					pLangEntries[i].dwOldCRC32 = (CheckExists(lpFullName) && key_file)  ? key_file->GetInt ( lpFullName, "CRC32") : 0;

					pLangEntries[i].lpLNGFileNameTemp = GetTempName ();

					pLangEntries[i].hLNGFile = WINPORT(CreateFile) (
							MB2Wide(pLangEntries[i].lpLNGFileNameTemp).c_str(),
							GENERIC_WRITE,
							FILE_SHARE_READ,
							NULL,
							CREATE_ALWAYS,
							0,
							NULL
							);

					if ( pLangEntries[i].hLNGFile == INVALID_HANDLE_VALUE )
						printf ("WARNING: Can't create the language file \"%s\".\n\r", pLangEntries[i].lpLNGFileName);
					else
					{
						WriteSignatureIfNeeded(pLangEntries[i].hLNGFile, pLangEntries[i].nEncoding);

						if ( bWriteCopyright )
						{
							sprintf (lpString, "// This .lng file was generated by .LNG Generator " VERSION "\r\n// Copyright (C) 2003-2005 WARP ItSelf\r\n// Copyright (C) 2005 WARP ItSelf & Alex Yaroslavsky\r\n\r\n");
							SmartWrite (pLangEntries[i].hLNGFile, lpString, &pLangEntries[i].dwCRC32, pLangEntries[i].nEncoding);
						}

						sprintf (lpString, ".Language=%s,%s\r\n\r\n", pLangEntries[i].lpLanguageName, pLangEntries[i].lpLanguageDescription);
						SmartWrite (pLangEntries[i].hLNGFile, lpString, &pLangEntries[i].dwCRC32, pLangEntries[i].nEncoding);
					}
				}

				char *lpHHead;
				char *lpHTail;
				char *lpEnum = NULL;

				if ( ReadComments (lpStart, &lpHHead, "hhead:", "") )
				{
					SmartWrite (hHFile, lpHHead, &dwHeaderCRC32, nHPPEncoding);
					free(lpHHead);
				}

				ReadComments (lpStart, &lpHTail, "htail:", "");

				ReadComments (lpStart, &lpEnum, "enum:", "");
				sprintf (lpString, "enum %s{\r\n", lpEnum? lpEnum : "");
				free(lpEnum);
				SmartWrite (hHFile, lpString, &dwHeaderCRC32, nHPPEncoding);

				// read strings

				bool bRead = true;

				while ( bRead )
				{
					char *lpMsgID;
					char *lpLNGString;
					char *lpHComments;

					if ( ReadComments(lpStart, &lpHComments, "h:", "") )
					{
						SmartWrite (hHFile, lpHComments, &dwHeaderCRC32, nHPPEncoding);
						free (lpHComments);
					}

					ReadComments(lpStart, &lpHComments, "he:", "");

					bRead = ReadFromBufferEx (lpStart, &lpMsgID);

					if ( bRead )
					{
						char *lpLngComments = NULL;
						char *lpELngComments  = NULL;
						char *lpSpecificLngComments  = NULL;

						sprintf (lpString, "\t%s,\r\n", lpMsgID);
						SmartWrite (hHFile, lpString, &dwHeaderCRC32, nHPPEncoding);

						ReadComments(lpStart, &lpLngComments, "l:", "");
						ReadComments(lpStart, &lpELngComments, "le:", "");

						for (int i = 0; i < dwLangs; i++)
						{
							if ( lpLngComments )
								SmartWrite (pLangEntries[i].hLNGFile, lpLngComments, &pLangEntries[i].dwCRC32, pLangEntries[i].nEncoding);

							if ( ReadComments(lpStart, &lpSpecificLngComments, "ls:", "") )
							{
								SmartWrite (pLangEntries[i].hLNGFile, lpSpecificLngComments, &pLangEntries[i].dwCRC32, pLangEntries[i].nEncoding);
								free (lpSpecificLngComments);
							}

							ReadComments(lpStart, &lpSpecificLngComments, "lse:", "");

							bRead = ReadFromBufferEx (lpStart, &lpLNGString);

							if ( bRead )
							{
								if ( !strncmp (lpLNGString, "upd:", 4) )
								{
									size_t length = strlen(lpLNGString);
									memmove(lpLNGString, lpLNGString+4, length-3);

									/*
									printf (
											"WARNING: String %s (ID = %s) of %s language needs update!\n\r",
											lpLNGString,
											lpMsgID,
											pLangEntries[i].lpLanguageName
											);
									*/
									SmartWrite (pLangEntries[i].hLNGFile, "// need translation:\r\n", &pLangEntries[i].dwCRC32, pLangEntries[i].nEncoding);
									pLangEntries[i].cNeedUpdate++;
								}

								sprintf (lpString, "//[%s]\r\n%s\r\n", lpMsgID, lpLNGString);
								SmartWrite (pLangEntries[i].hLNGFile, lpString, &pLangEntries[i].dwCRC32, pLangEntries[i].nEncoding);
								free (lpLNGString);
							}

							if ( lpSpecificLngComments )
							{
								SmartWrite (pLangEntries[i].hLNGFile, lpSpecificLngComments, &pLangEntries[i].dwCRC32, pLangEntries[i].nEncoding);
								free (lpSpecificLngComments);
							}

							if ( lpELngComments )
								SmartWrite (pLangEntries[i].hLNGFile, lpELngComments, &pLangEntries[i].dwCRC32, pLangEntries[i].nEncoding);
						}

						free (lpMsgID);

						if ( lpLngComments )
							free (lpLngComments);

						if ( lpELngComments )
							free (lpELngComments);

					}

					if ( lpHComments )
					{
						SmartWrite (hHFile, lpHComments, &dwHeaderCRC32, nHPPEncoding);
						free (lpHComments);
					}
				}

				// output needed translations statistics
				for (int i = 0; i < dwLangs; i++)
				{
					if (pLangEntries[i].cNeedUpdate > 0)
					{
						printf ("INFO: There are %d strings that require review in %s translation!\n\n\r",
								pLangEntries[i].cNeedUpdate,
								pLangEntries[i].lpLanguageName);
					}
				}

				// write .h file footer

				WINPORT(SetFilePointer) (hHFile, -2, NULL, FILE_CURRENT);

				sprintf (lpString, "\r\n};\r\n");
				SmartWrite (hHFile, lpString, &dwHeaderCRC32, nHPPEncoding);

				if ( lpHTail )
				{
					SmartWrite (hHFile, lpHTail, &dwHeaderCRC32, nHPPEncoding);
					free (lpHTail);
				}

				// play with CRC

				for (int i = 0; i < dwLangs; i++)
				{
					WINPORT(CloseHandle) (pLangEntries[i].hLNGFile);

					sprintf (lpFullName, "%s/%s", lpLNGOutputPath?lpLNGOutputPath:".", pLangEntries[i].lpLNGFileName);

					bUpdate = true;

					if ( key_file )
					{
						if ( pLangEntries[i].dwCRC32 == pLangEntries[i].dwOldCRC32 )
						{
							// printf ("INFO: Language file \"%s\" doesn't need to be updated.\r\n", pLangEntries[i].lpLNGFileName);
							bUpdate = false;
						}
						else
						{
							key_file->PutInt(lpFullName, "CRC32", pLangEntries[i].dwCRC32);
						}
					}

					if ( bUpdate )
					{
						WINPORT(MoveFileEx) (
								MB2Wide(pLangEntries[i].lpLNGFileNameTemp).c_str(),
								MB2Wide(lpFullName).c_str(),
								MOVEFILE_REPLACE_EXISTING|MOVEFILE_COPY_ALLOWED
								);
					}

					WINPORT(DeleteFile) (MB2Wide(pLangEntries[i].lpLNGFileNameTemp).c_str());

					free (pLangEntries[i].lpLNGFileNameTemp);
					free (pLangEntries[i].lpLNGFileName);
					free (pLangEntries[i].lpLanguageName);
					free (pLangEntries[i].lpLanguageDescription);
				}

                free(pLangEntries);

				WINPORT(CloseHandle) (hHFile);

				sprintf (lpFullName, "%s/%s", lpHOutputPath?lpHOutputPath:".", lpHPPFileName);

				bUpdate = true;

				if ( key_file )
				{
					if ( dwHeaderCRC32 == dwHeaderOldCRC32 )
					{
						// printf ("INFO: Header file \"%s\" doesn't need to be updated.\r\n", lpHPPFileName);
						bUpdate = false;
					}
					else
					{
						key_file->PutInt(lpFullName, "CRC32", dwHeaderCRC32);
					}
				}

				if ( bUpdate )
				{
					WINPORT(MoveFileEx) (
							MB2Wide(lpHPPFileNameTemp).c_str(),
							MB2Wide(lpFullName).c_str(),
							MOVEFILE_REPLACE_EXISTING|MOVEFILE_COPY_ALLOWED
							);
				}

		}
		else
			printf ("ERROR: Can't create the header file, exiting.\n\r");
	}
	else
		printf ("ERROR: Zero languages to process, exiting.\n\r");

	WINPORT(DeleteFile) (MB2Wide(lpHPPFileNameTemp).c_str());

	free (lpHPPFileNameTemp);
	free (lpHPPFileName);
	free (pFeedBuffer);
	free (lpString);

	WINPORT(CloseHandle) (hFeedFile);

	free (lpFullName);
	free (lpHOutputPath);
	free (lpLNGOutputPath);

	return 0;
}
