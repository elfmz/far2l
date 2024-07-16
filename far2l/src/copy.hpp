#pragma once

/*
copy.hpp

class ShellCopy - Копирование файлов
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

#include "dizlist.hpp"
#include "udlist.hpp"
#include "flink.hpp"
class Panel;

#include <WinCompat.h>
#include "FARString.hpp"

enum COPY_CODES
{
	COPY_CANCEL,
	COPY_NEXT,
	COPY_NOFILTER,		// не считать размеры, т.к. файл не прошел по фильтру
	COPY_FAILURE,
	COPY_FAILUREREAD,
	COPY_SUCCESS,
	COPY_SUCCESS_MOVE,
	COPY_RETRY,
};

enum COPY_SYMLINK
{
	COPY_SYMLINK_ASIS   = 0,
	COPY_SYMLINK_SMART  = 1,	// Copy symbolics links content instead of making new links
	COPY_SYMLINK_ASFILE = 2		// Copy remote (to this copy operation) symbolics links content,
	//                                                 make relative links for local ones
};

struct COPY_FLAGS
{
	inline COPY_FLAGS() { memset(this, 0, sizeof(*this)); }

	bool CURRENTONLY    : 1;		// Только текущий?
	bool ONLYNEWERFILES : 1;		// Copy only newer files
	bool OVERWRITENEXT  : 1;		// Overwrite all
	bool LINK           : 1;		// создание линков
	bool MOVE           : 1;		// перенос/переименование
	bool DIZREAD        : 1;		//
	bool COPYACCESSMODE : 1;		// [x] Copy files access mode
	bool WRITETHROUGH   : 1;		// disable write cache
	bool COPYXATTR      : 1;		// copy extended attributes
	bool SPARSEFILES    : 1;		// allow producing sparse files
	bool USECOW         : 1;		// enable COW functionality if FS supports it
	bool COPYLASTTIME   : 1;		// При копировании в несколько каталогов устанавливается для последнего.
	bool UPDATEPPANEL   : 1;		// необходимо обновить пассивную панель
	COPY_SYMLINK SYMLINK : 2;
	DWORD ErrorMessageFlags;		//  MSG_WARNING | MSG_ERRORTYPE [| MSG_DISPLAYNOTIFY if Opt.NotifOpt.OnFileOperation ]
};

class ShellCopyFileExtendedAttributes
{
	FileExtendedAttributes _xattr;
	bool _apply;

public:
	ShellCopyFileExtendedAttributes(File &f);
	void ApplyToCopied(File &f);
};

struct ShellCopyBuffer
{
	ShellCopyBuffer();
	~ShellCopyBuffer();

	const DWORD Capacity;

	DWORD Size;

private:
	char *const Buffer;

public:
	char *const Ptr;
};

class ShellFileTransfer
{
	const wchar_t *_SrcName;
	const FARString &_strDestName;
	ShellCopyBuffer &_CopyBuffer;
	COPY_FLAGS &_Flags;
	const FAR_FIND_DATA_EX &_SrcData;

	clock_t _Stopwatch = 0;
	int64_t _AppendPos = -1;
	DWORD _DstFlags    = 0;
	DWORD _ModeToCreateWith = 0;

	File _SrcFile, _DestFile;
	bool _LastWriteWasHole = false;
	bool _Done             = false;
	std::unique_ptr<ShellCopyFileExtendedAttributes> _XAttrCopyPtr;

	void Undo();
	void RetryCancel(const wchar_t *Text, const wchar_t *Object);
	DWORD PieceWrite(const void *Data, DWORD Size);
	DWORD PieceWriteHole(DWORD Size);
	DWORD PieceCopy();

public:
	ShellFileTransfer(const wchar_t *SrcName, const FAR_FIND_DATA_EX &SrcData, const FARString &strDestName,
			bool Append, ShellCopyBuffer &CopyBuffer, COPY_FLAGS &Flags);
	~ShellFileTransfer();

	void Do();
};

class ShellCopy
{
	COPY_FLAGS Flags;
	Panel *SrcPanel, *DestPanel;
	int SrcPanelMode, DestPanelMode;
	DizList DestDiz;
	FARString strDestDizPath;
	FARString strCopiedName;
	FARString strRenamedName;
	FARString strRenamedFilesPath;
	int OvrMode;
	int ReadOnlyOvrMode;
	int ReadOnlyDelMode;
	int SkipMode;	// ...для пропуска при копировании залоченных файлов.
	int SkipDeleteMode;
	int SelectedFolderNameLength;
	UserDefinedList DestList;
	// тип создаваемого репарспоинта.
	// при AltF6 будет то, что выбрал юзер в диалоге,
	// в остальных случаях - RP_EXACTCOPY - как у источника
	ReparsePointTypes RPT;
	ShellCopyBuffer CopyBuffer;
	bool CaseInsensitiveFS{false};

	std::vector<FARString> SelectedPanelItems;
	struct CopiedDirectory
	{
		std::string Path;
		FILETIME ftUnixAccessTime;
		FILETIME ftUnixModificationTime;
		DWORD dwUnixMode;
	};

	std::vector<CopiedDirectory> DirectoriesAttributes;
	void EnqueueDirectoryAttributes(const FAR_FIND_DATA_EX &SrcData, FARString &strDest);
	void SetEnqueuedDirectoriesAttributes();

	bool IsSymlinkTargetAlsoCopied(const wchar_t *SymLink);

	COPY_CODES CopyFileTree(const wchar_t *Dest);
	COPY_CODES ShellCopyOneFile(const wchar_t *Src, const FAR_FIND_DATA_EX &SrcData, FARString &strDest,
			int KeepPathPos, int Rename);
	COPY_CODES ShellCopyOneFileNoRetry(const wchar_t *Src, const FAR_FIND_DATA_EX &SrcData,
			FARString &strDest, int KeepPathPos, int Rename);

	int ShellCopyFile(const wchar_t *SrcName, const FAR_FIND_DATA_EX &SrcData, FARString &strDestName,
			int Append);

	int DeleteAfterMove(const wchar_t *Name, DWORD Attr);
	void SetDestDizPath(const wchar_t *DestPath);
	int AskOverwrite(const FAR_FIND_DATA_EX &SrcData, const wchar_t *SrcName, const wchar_t *DestName,
			DWORD DestAttr, bool SameName, bool Rename, bool AskAppend, bool &Append, FARString &strNewName,
			int &RetCode);
	bool CalcTotalSize();

	bool CmpFullNames(const wchar_t *Src, const wchar_t *Dest) const;
	bool CmpNames(const wchar_t *Src, const wchar_t *Dest) const;

	COPY_CODES CreateSymLink(const char *ExistingName, const wchar_t *NewName, const FAR_FIND_DATA_EX &SrcData);
	COPY_CODES CopySymLink(const wchar_t *ExistingName, const wchar_t *NewName, const FAR_FIND_DATA_EX &SrcData);

public:
	ShellCopy(Panel *SrcPanel, int Move, int Link, int CurrentOnly, int Ask, int &ToPlugin,
			const wchar_t *PluginDestPath, bool ToSubdir = false);
	~ShellCopy();
};

LONG_PTR WINAPI CopyDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2);
