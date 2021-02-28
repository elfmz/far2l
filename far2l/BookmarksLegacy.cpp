/*
ffolders.cpp

Folder shortcuts
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
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

#include "headers.hpp"

#include "Bookmarks.hpp"
#include <fcntl.h>

// TODO: remove this code after 2022/01/13
#ifdef WINPORT_REGISTRY

static void LegacyShortcut_GetRecord(const wchar_t *RecTypeName, int RecNumber, FARString *pValue)
{
	pValue->Clear();

	HKEY key = NULL;
	if (WINPORT(RegOpenKeyEx)(HKEY_CURRENT_USER, L"Software/Far2/FolderShortcuts", 0, GENERIC_READ, &key) == ERROR_SUCCESS) {
		FARString strValueName;
		strValueName.Format(RecTypeName, RecNumber);
		std::vector<WCHAR> data(0x10000);
		DWORD tip, data_len = (data.size() - 1) * sizeof(WCHAR);
		if (WINPORT(RegQueryValueEx)(key, strValueName, NULL, &tip, (LPBYTE)&data[0], &data_len) == ERROR_SUCCESS) {
			*pValue = &data[0];
		}
		WINPORT(RegCloseKey)(key);
	}
}

static int LegacyShortcut_Get(int Pos, FARString *pDestFolder,
		FARString *pPluginModule,
		FARString *pPluginFile,
		FARString *pPluginData)
{
	FARString strFolder;
	LegacyShortcut_GetRecord(L"Shortcut%d", Pos, &strFolder);
	apiExpandEnvironmentStrings(strFolder, *pDestFolder);

	if (pPluginModule)
		LegacyShortcut_GetRecord(L"PluginModule%d",Pos, pPluginModule);

	if (pPluginFile)
		LegacyShortcut_GetRecord(L"PluginFile%d",Pos, pPluginFile);

	if (pPluginData)
		LegacyShortcut_GetRecord(L"PluginData%d",Pos, pPluginData);

	return (!pDestFolder->IsEmpty() || (pPluginModule && !pPluginModule->IsEmpty()));
}

void CheckForImportLegacyShortcuts()
{
	const auto &new_path = InMyConfig("settings/bookmarks.ini");
	struct stat s{};
	if (stat(new_path.c_str(), &s) == 0)
		return;

	close(open(new_path.c_str(), O_RDWR, 0600));

	Bookmarks b;

	for (int i = 0; ; ++i)
	{
		FARString Folder, PluginModule, PluginFile, PluginData;
		if (LegacyShortcut_Get(i, &Folder, &PluginModule, &PluginFile, &PluginData))
		{
			b.Set(i, &Folder, &PluginModule, &PluginFile, &PluginData);
		}
		else if (i >= 10)
		{
			break;
		}
	}
}

#else // WINPORT_REGISTRY

void CheckForImportLegacyShortcuts() {}

#endif // WINPORT_REGISTRY
