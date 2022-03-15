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

	int hLNGFile;

	uint32_t dwCRC32;
	uint32_t dwOldCRC32;

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
		int hFile,
		const char *lpStr,
		uint32_t *pCRC32
		)
{
	size_t dwWritten = WriteAll(hFile, lpStr, strlen(lpStr));
	*pCRC32 = CRC32(*pCRC32, lpStr, dwWritten);
}

static char *GetTempName(const char *base_name)
{
	char *out = (char *)malloc(strlen(base_name) + 8);
	sprintf(out, "%s.tmp", base_name);
	return out;
}


int ReadInteger(char*& lpStart)
{
	char* lpTemp;

	ReadFromBufferEx (lpStart, &lpTemp);
	int nResult = atol(lpTemp);

	free (lpTemp);

	return nResult;
}

void WriteSignatureIfNeeded(int hFile)
{
	uint32_t dwID = 0xBFBBEF;
	WriteAll(hFile, &dwID, 3);
}

bool CheckExists(const char *path)
{
	struct stat s = {0};
	return stat(path, &s)==0;
}

int main_generator (int argc, char** argv)
{
	printf (".LNG Generator " VERSION "\n");
	printf ("Copyright (C) 2003-2009 WARP ItSelf\n");
	printf ("Copyright (C) 2005 WARP ItSelf & Alex Yaroslavsky\n\n");

	if ( argc < 2 )
	{
		printf ("Usage: generator [options] feed_file\n");
		printf ("\nOptions:\n");
		printf ("\t-i filename - optional ini file with update status.\n");
		printf ("\t-ol output_path - language files output path.\n");
		printf ("\t-oh output_path - header file output path.\n");
		printf ("\t-nc - don't write copyright info to generated files.\n");
		return 0;
	}

	std::unique_ptr<KeyFileHelper> key_file;

	char *lpLNGOutputPath = NULL;
	char *lpHOutputPath = NULL;

	bool bWriteCopyright = true;

	if ( argc > 2 )
	{
		for (int i = 1; i < argc-1; i++)
		{
			if ( strcmp (argv[i],"-nc") == 0 )
				bWriteCopyright = false;
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

			else
			{
				fprintf(stderr, "Bad argument: %s\n", argv[i]);
				return -1;
			}
		}
	}

	fprintf(stderr, "lpLNGOutputPath=%s\n", lpLNGOutputPath);

	FDScope hFeedFile(OpenInputFile(argv[argc-1]));
	if (!hFeedFile.Valid())
	{
		printf ("ERROR: Can't open the feed file, exiting.\n");
		return 0;
	}

	size_t dwSize = QueryFileSize(hFeedFile);

	if (dwSize >= 3)
	{
		uint32_t dwID = 0;
		ReadAll(hFeedFile, &dwID, 3);
		if ( ((dwID & 0x00FFFFFF) == 0xBFBBEF) )
			dwSize-= 3;
		else
			lseek(hFeedFile, 0, SEEK_SET);
	}

	bool bUpdate;

	LanguageEntry *pLangEntries;

	uint32_t dwHeaderCRC32;
	uint32_t dwHeaderOldCRC32;

	char *lpHPPFileName = NULL;
	char *lpHPPFileNameTemp = NULL;

	char *lpFullName = (char*)malloc(0x10000);

	char *lpString = (char*)malloc (1024);

	char *pFeedBuffer = (char*)calloc(1, dwSize+1);

	ReadAll(hFeedFile, pFeedBuffer, dwSize);

	char *lpStart = pFeedBuffer;

	// read h filename

	ReadFromBufferEx (lpStart, &lpHPPFileName);
	UnquoteIfNeeded (lpHPPFileName);

	// read language count
	int dwLangs = ReadInteger(lpStart);

	if ( dwLangs > 0 )
	{
		sprintf (lpFullName, "%s/%s", lpHOutputPath?lpHOutputPath:".", lpHPPFileName);

		dwHeaderCRC32 = 0;
		dwHeaderOldCRC32 = (CheckExists(lpFullName) && key_file) ? key_file->GetInt( lpFullName, "CRC32")  : 0;

		lpHPPFileNameTemp = GetTempName(lpFullName);

		FDScope hHFile(CreateOutputFile(lpHPPFileNameTemp));
		if ( hHFile.Valid() )
		{
			WriteSignatureIfNeeded(hHFile);

			if ( bWriteCopyright )
			{
				sprintf (lpString, "// This C++ include file was generated by .LNG Generator " VERSION "\r\n// Copyright (C) 2003-2005 WARP ItSelf\r\n// Copyright (C) 2005 WARP ItSelf & Alex Yaroslavsky\r\n\n");
				SmartWrite (hHFile, lpString, &dwHeaderCRC32);
			}

			pLangEntries = (LanguageEntry*)malloc (dwLangs*sizeof (LanguageEntry));

			// read language names and create .lng files

			for (int i = 0; i < dwLangs; i++)
			{
				ReadFromBufferEx (lpStart, &pLangEntries[i].lpLNGFileName);

				ReadFromBufferEx (lpStart, &pLangEntries[i].lpLanguageName);
				ReadFromBufferEx (lpStart, &pLangEntries[i].lpLanguageDescription);

				UnquoteIfNeeded (pLangEntries[i].lpLanguageName);
				UnquoteIfNeeded (pLangEntries[i].lpLanguageDescription);
				UnquoteIfNeeded (pLangEntries[i].lpLNGFileName);

				sprintf (lpFullName, "%s/%s", lpLNGOutputPath?lpLNGOutputPath:".", pLangEntries[i].lpLNGFileName);

				pLangEntries[i].cNeedUpdate = 0;

				pLangEntries[i].dwCRC32 = 0;
				pLangEntries[i].dwOldCRC32 = (CheckExists(lpFullName) && key_file)  ? key_file->GetInt ( lpFullName, "CRC32") : 0;

				pLangEntries[i].lpLNGFileNameTemp = GetTempName(lpFullName);

				pLangEntries[i].hLNGFile = CreateOutputFile(pLangEntries[i].lpLNGFileNameTemp);
				if ( pLangEntries[i].hLNGFile == -1)
					printf ("WARNING: Can't create the language file \"%s\".\n", pLangEntries[i].lpLNGFileName);
				else
					{
						WriteSignatureIfNeeded(pLangEntries[i].hLNGFile);

						if ( bWriteCopyright )
						{
							sprintf (lpString, "// This .lng file was generated by .LNG Generator " VERSION "\r\n// Copyright (C) 2003-2005 WARP ItSelf\r\n// Copyright (C) 2005 WARP ItSelf & Alex Yaroslavsky\r\n\n");
							SmartWrite (pLangEntries[i].hLNGFile, lpString, &pLangEntries[i].dwCRC32);
						}

						sprintf (lpString, ".Language=%s,%s\r\n\n", pLangEntries[i].lpLanguageName, pLangEntries[i].lpLanguageDescription);
						SmartWrite (pLangEntries[i].hLNGFile, lpString, &pLangEntries[i].dwCRC32);
					}
				}

				char *lpHHead;
				char *lpHTail;
				char *lpEnum = NULL;

				if ( ReadComments (lpStart, &lpHHead, "hhead:", "") )
				{
					SmartWrite (hHFile, lpHHead, &dwHeaderCRC32);
					free(lpHHead);
				}

				ReadComments (lpStart, &lpHTail, "htail:", "");

				ReadComments (lpStart, &lpEnum, "enum:", "");
				//sprintf (lpString, "enum %s{\r\n", lpEnum? lpEnum : "");
				sprintf (lpString, "/* FarLang  - start */\r\n");
				free(lpEnum);
				SmartWrite (hHFile, lpString, &dwHeaderCRC32);

				// read strings

				bool bRead = true;
				int nMsgIndex = 0;

				while ( bRead )
				{
					char *lpMsgID;
					char *lpLNGString;
					char *lpHComments;

					if ( ReadComments(lpStart, &lpHComments, "h:", "") )
					{
						SmartWrite (hHFile, lpHComments, &dwHeaderCRC32);
						free (lpHComments);
					}

					ReadComments(lpStart, &lpHComments, "he:", "");

					bRead = ReadFromBufferEx (lpStart, &lpMsgID);

					if ( bRead )
					{
						char *lpLngComments = NULL;
						char *lpELngComments  = NULL;
						char *lpSpecificLngComments  = NULL;

						//sprintf (lpString, "\t%s,\r\n", lpMsgID);
						sprintf (lpString, "DECLARE_FARLANGMSG(%s, %d)\r\n", lpMsgID, nMsgIndex);
						SmartWrite (hHFile, lpString, &dwHeaderCRC32);
						++nMsgIndex;

						ReadComments(lpStart, &lpLngComments, "l:", "");
						ReadComments(lpStart, &lpELngComments, "le:", "");

						for (int i = 0; i < dwLangs; i++)
						{
							if ( lpLngComments )
								SmartWrite (pLangEntries[i].hLNGFile, lpLngComments, &pLangEntries[i].dwCRC32);

							if ( ReadComments(lpStart, &lpSpecificLngComments, "ls:", "") )
							{
								SmartWrite (pLangEntries[i].hLNGFile, lpSpecificLngComments, &pLangEntries[i].dwCRC32);
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
											"WARNING: String %s (ID = %s) of %s language needs update!\n",
											lpLNGString,
											lpMsgID,
											pLangEntries[i].lpLanguageName
											);
									*/
									SmartWrite (pLangEntries[i].hLNGFile, "// need translation:\r\n", &pLangEntries[i].dwCRC32);
									pLangEntries[i].cNeedUpdate++;
								}

								sprintf (lpString, "//[%s]\r\n%s\r\n", lpMsgID, lpLNGString);
								SmartWrite (pLangEntries[i].hLNGFile, lpString, &pLangEntries[i].dwCRC32);
								free (lpLNGString);
							}

							if ( lpSpecificLngComments )
							{
								SmartWrite (pLangEntries[i].hLNGFile, lpSpecificLngComments, &pLangEntries[i].dwCRC32);
								free (lpSpecificLngComments);
							}

							if ( lpELngComments )
								SmartWrite (pLangEntries[i].hLNGFile, lpELngComments, &pLangEntries[i].dwCRC32);
						}

						free (lpMsgID);

						if ( lpLngComments )
							free (lpLngComments);

						if ( lpELngComments )
							free (lpELngComments);

					}

					if ( lpHComments )
					{
						SmartWrite (hHFile, lpHComments, &dwHeaderCRC32);
						free (lpHComments);
					}
				}

				// output needed translations statistics
				for (int i = 0; i < dwLangs; i++)
				{
					if (pLangEntries[i].cNeedUpdate > 0)
					{
						printf ("INFO: There are %d strings that require review in %s translation!\n\n",
								pLangEntries[i].cNeedUpdate,
								pLangEntries[i].lpLanguageName);
					}
				}

				// write .h file footer

				lseek(hHFile, -2, SEEK_CUR);

//				sprintf (lpString, "\r\n};\r\n");
				sprintf (lpString, "\r\n/* FarLang  - end */\r\n");
				SmartWrite (hHFile, lpString, &dwHeaderCRC32);

				if ( lpHTail )
				{
					SmartWrite (hHFile, lpHTail, &dwHeaderCRC32);
					free (lpHTail);
				}

				// play with CRC

				for (int i = 0; i < dwLangs; i++)
				{
					close(pLangEntries[i].hLNGFile);
					pLangEntries[i].hLNGFile = -1;

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
							key_file->SetInt(lpFullName, "CRC32", pLangEntries[i].dwCRC32);
						}
					}

					if ( bUpdate )
					{
						if (rename(pLangEntries[i].lpLNGFileNameTemp, lpFullName) == -1) {
							printf ("ERROR: Failed to rename '%s' -> '%s'.\n",
								pLangEntries[i].lpLNGFileNameTemp, lpFullName);
						}
					}

					unlink(pLangEntries[i].lpLNGFileNameTemp);

					free (pLangEntries[i].lpLNGFileNameTemp);
					free (pLangEntries[i].lpLNGFileName);
					free (pLangEntries[i].lpLanguageName);
					free (pLangEntries[i].lpLanguageDescription);
				}

				free(pLangEntries);

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
						key_file->SetInt(lpFullName, "CRC32", dwHeaderCRC32);
					}
				}

				if ( bUpdate )
				{
					if (rename(lpHPPFileNameTemp, lpFullName) == -1) {
						printf ("ERROR: Failed to rename '%s' -> '%s'.\n", lpHPPFileNameTemp, lpFullName);
					}
				}

		}
		else
			printf ("ERROR: Can't create the header file, exiting.\n");
	}
	else
		printf ("ERROR: Zero languages to process, exiting.\n");

	unlink(lpHPPFileNameTemp);

	free (lpHPPFileNameTemp);
	free (lpHPPFileName);
	free (pFeedBuffer);
	free (lpString);

	free (lpFullName);
	free (lpHOutputPath);
	free (lpLNGOutputPath);

	return 0;
}
