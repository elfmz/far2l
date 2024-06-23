#include <fcntl.h>
#include "MultiArc.hpp"
#include "marclng.hpp"

PluginClass::PluginClass(int ArcPluginNumber)
{
	*CurDir = 0;
	PluginClass::ArcPluginNumber = ArcPluginNumber;
	DizPresent = FALSE;
	bGOPIFirstCall = true;
	ZeroFill(CurArcInfo);
}

PluginClass::~PluginClass()
{
	FreeArcData();
}

void PluginClass::FreeArcData()
{
	ArcData.Clear();
	ArcDataCount = 0;
}

int PluginClass::PreReadArchive(const char *Name)
{
	if (sdc_stat(Name, &ArcStat) == -1) {
		return FALSE;
	}

	ArcName = Name;

	if (FindExt(ArcName) == std::string::npos) {
		ArcName+= '.';
	}

	return TRUE;
}

static void SanitizeString(std::string &s)
{
	while (!s.empty() && !s.back()) {
		s.pop_back();
	}
}

int PluginClass::ReadArchive(const char *Name, int OpMode)
{
	bGOPIFirstCall = true;
	FreeArcData();
	DizPresent = FALSE;

	if (sdc_stat(Name, &ArcStat) == -1)
		return FALSE;

	if (!ArcPlugin->OpenArchive(ArcPluginNumber, Name, &ArcPluginType, (OpMode & OPM_SILENT) != 0))
		return FALSE;

	ItemsInfo = ArcItemInfo{};
	ZeroFill(CurArcInfo);
	TotalSize = PackedSize = 0;
	ArcDataCount = 0;

	HANDLE hScreen = Info.SaveScreen(0, 0, -1, -1);

	DWORD UpdateTime = GetProcessUptimeMSec() + 1000;
	bool MessageShown = false;
	int GetItemCode;

	ArcItemInfo CurItemInfo;
	PathParts CurPP;
	while (1) {
		CurItemInfo = ArcItemInfo();
		GetItemCode = ArcPlugin->GetArcItem(ArcPluginNumber, &CurItemInfo);
		if (GetItemCode != GETARC_SUCCESS)
			break;

		SanitizeString(CurItemInfo.PathName);
		if (CurItemInfo.Description)
			SanitizeString(*CurItemInfo.Description);
		if (CurItemInfo.LinkName)
			SanitizeString(*CurItemInfo.LinkName);

		if ((ArcDataCount & 0x1f) == 0) {
			if (CheckForEsc()) {
				FreeArcData();
				ArcPlugin->CloseArchive(ArcPluginNumber, &CurArcInfo);
				Info.RestoreScreen(NULL);
				Info.RestoreScreen(hScreen);
				return FALSE;
			}

			const DWORD Now = GetProcessUptimeMSec();
			if (Now >= UpdateTime) {
				UpdateTime = Now + 100;
				const auto &NameMsg = FormatMessagePath(Name, false);
				const auto &FilesMsg = StrPrintf(GetMsg(MArcReadFiles), (unsigned int)ArcDataCount);
				const char *MsgItems[] = {GetMsg(MArcReadTitle), GetMsg(MArcReading), NameMsg.c_str(), FilesMsg.c_str()};
				Info.Message(Info.ModuleNumber, MessageShown ? FMSG_KEEPBACKGROUND : 0, NULL, MsgItems,
						ARRAYSIZE(MsgItems), 0);
				MessageShown = true;
			}
		}

		if (CurItemInfo.Description)
			DizPresent = TRUE;

		if (CurItemInfo.HostOS && (!ItemsInfo.HostOS || strcmp(ItemsInfo.HostOS, CurItemInfo.HostOS) != 0))
			ItemsInfo.HostOS = (ItemsInfo.HostOS ? CurItemInfo.HostOS : GetMsg(MSeveralOS));

		if (ItemsInfo.Codepage <= 0)
			ItemsInfo.Codepage = CurItemInfo.Codepage;

		ItemsInfo.Solid|= CurItemInfo.Solid;
		ItemsInfo.Comment|= CurItemInfo.Comment;
		ItemsInfo.Encrypted|= CurItemInfo.Encrypted;

		if (CurItemInfo.Encrypted)
			CurItemInfo.Flags|= F_ENCRYPTED;

		if (CurItemInfo.DictSize > ItemsInfo.DictSize)
			ItemsInfo.DictSize = CurItemInfo.DictSize;

		if (CurItemInfo.UnpVer > ItemsInfo.UnpVer)
			ItemsInfo.UnpVer = CurItemInfo.UnpVer;

		CurItemInfo.NumberOfLinks = 1;

		size_t PrefixSize = 0;
		if (StrStartsFrom(CurItemInfo.PathName, "./"))
			PrefixSize = 2;
		else if (StrStartsFrom(CurItemInfo.PathName, "../"))
			PrefixSize = 3;
		while (PrefixSize < CurItemInfo.PathName.size() && CurItemInfo.PathName[PrefixSize] == '/')
			PrefixSize++;

		if (PrefixSize) {
			CurItemInfo.Prefix.reset(new std::string(CurItemInfo.PathName.substr(0, PrefixSize)));
			CurItemInfo.PathName.erase(0, PrefixSize);
		}

		if (!CurItemInfo.PathName.empty() && CurItemInfo.PathName.back() == '/')
			CurItemInfo.dwFileAttributes|= FILE_ATTRIBUTE_DIRECTORY;

		TotalSize+= CurItemInfo.nFileSize;
		PackedSize+= CurItemInfo.nPhysicalSize;

		CurPP.clear();
		CurPP.Traverse(CurItemInfo.PathName);
		ArcItemAttributes *CurAttrs = ArcData.Ensure(CurPP.begin(), CurPP.end());
		*CurAttrs = std::move(CurItemInfo);
		++ArcDataCount;
	}

	Info.RestoreScreen(NULL);
	Info.RestoreScreen(hScreen);

	ArcPlugin->CloseArchive(ArcPluginNumber, &CurArcInfo);

	if (GetItemCode != GETARC_EOF && GetItemCode != GETARC_SUCCESS) {
		switch (GetItemCode) {
			case GETARC_BROKEN:
				GetItemCode = MBadArchive;
				break;

			case GETARC_UNEXPEOF:
				GetItemCode = MUnexpEOF;
				break;

			case GETARC_READERROR:
				GetItemCode = MReadError;
				break;
		}

		const auto &NameMsg = FormatMessagePath(Name, true);
		const char *MsgItems[] = {GetMsg(MError), NameMsg.c_str(), GetMsg(GetItemCode), GetMsg(MOk)};
		Info.Message(Info.ModuleNumber, FMSG_WARNING, NULL, MsgItems, ARRAYSIZE(MsgItems), 1);
		return FALSE;	// Mantis#0001241
	}

	// Info.RestoreScreen(NULL);
	// Info.RestoreScreen(hScreen);
	return TRUE;
}

bool PluginClass::EnsureFindDataUpToDate(int OpMode)
{
	if (!ArcData.empty()) {
		struct stat NewArcStat{};
		if (sdc_stat(ArcName.c_str(), &NewArcStat) == -1)
			return false;

		if (ArcStat.st_mtime == NewArcStat.st_mtime && ArcStat.st_size == NewArcStat.st_size)
			return true;
	}

	DWORD size = (DWORD)Info.AdvControl(Info.ModuleNumber, ACTL_GETPLUGINMAXREADDATA, (void *)0);
	int fd = sdc_open(ArcName.c_str(), O_RDONLY);
	if (fd == -1)
		return false;

	unsigned char *Data = (unsigned char *)malloc(size);
	ssize_t read_size = Data ? sdc_read(fd, Data, size) : -1;
	sdc_close(fd);

	bool ReadArcOK = false;
	if (read_size > 0) {
		DWORD SFXSize = 0;

		ReadArcOK = (ArcPlugin->IsArchive(ArcPluginNumber, ArcName.c_str(), Data, read_size, &SFXSize)
				&& ReadArchive(ArcName.c_str(), OpMode));
	}
	free(Data);

	return ReadArcOK;
}

int PluginClass::GetFindData(PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode)
{
	*pPanelItem = NULL;
	*pItemsNumber = 0;

	PathParts CurDirPP;
	CurDirPP.Traverse(CurDir);

	if (!EnsureFindDataUpToDate(OpMode)) {
		fprintf(stderr, "MA::GetFindData: can't update at '%s'\n", CurDirPP.Join().c_str());
		return FALSE;
	}

	const auto *DirNode = ArcData.Find(CurDirPP.begin(), CurDirPP.end());
	if (!DirNode) {
		fprintf(stderr, "MA::GetFindData: no node at '%s'\n", CurDirPP.Join().c_str());
		return FALSE;
	}

	if (DirNode->empty())
		return TRUE;

	PluginPanelItem *CurrentItem = *pPanelItem =
			(PluginPanelItem *)calloc(DirNode->size(), sizeof(PluginPanelItem));
	if (!CurrentItem) {
		fprintf(stderr, "MA::GetFindData: can't alloc %lu items at '%s'\n", (unsigned long)DirNode->size(),
				CurDirPP.Join().c_str());
		return FALSE;
	}
	*pItemsNumber = (int)DirNode->size();

	for (const auto &it : *DirNode) {
		CurrentItem->FindData.ftCreationTime = it.second.ftCreationTime;
		CurrentItem->FindData.ftLastAccessTime = it.second.ftLastAccessTime;
		CurrentItem->FindData.ftLastWriteTime = it.second.ftLastWriteTime;
		CurrentItem->FindData.nPhysicalSize = it.second.nPhysicalSize;
		CurrentItem->FindData.nFileSize = it.second.nFileSize;
		CurrentItem->FindData.dwFileAttributes = it.second.dwFileAttributes;
		CurrentItem->FindData.dwUnixMode = it.second.dwUnixMode;
		strncpy(CurrentItem->FindData.cFileName, it.first.c_str(),
				ARRAYSIZE(CurrentItem->FindData.cFileName));
		CurrentItem->Flags = it.second.Flags;
		CurrentItem->NumberOfLinks = it.second.NumberOfLinks;
		CurrentItem->CRC32 = it.second.CRC32;
		CurrentItem->Description = it.second.Description ? (char *)it.second.Description->c_str() : nullptr;
		CurrentItem->UserData = (DWORD_PTR)&it.second;

		if (!it.second.empty())
			CurrentItem->FindData.dwFileAttributes|= FILE_ATTRIBUTE_DIRECTORY;
		if (CurrentItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			CurrentItem->FindData.nPhysicalSize = CurrentItem->FindData.nFileSize = 0;

		++CurrentItem;
	}

	return TRUE;
}

void PluginClass::FreeFindData(PluginPanelItem *PanelItem, int ItemsNumber)
{
	if (PanelItem)
		free(PanelItem);
}

int PluginClass::SetDirectory(const char *Dir, int OpMode)
{
	if (*Dir == '/' && *(++Dir) == 0) {
		*CurDir = 0;
		return TRUE;
	}

	PathParts NewDirPP;
	NewDirPP.Traverse(CurDir);
	NewDirPP.Traverse(Dir);
	const auto &NewDir = NewDirPP.Join();

	auto *DirNode = ArcData.Find(NewDirPP.begin(), NewDirPP.end());
	if (!DirNode) {
		fprintf(stderr, "MA::SetDirectory('%s', %d): no node for '%s'\n", Dir, OpMode, NewDir.c_str());
		return FALSE;
	}

	if (NewDir.size() >= ARRAYSIZE(CurDir)) {
		fprintf(stderr, "MA::SetDirectory('%s', %d): too long path '%s'\n", Dir, OpMode, NewDir.c_str());
		return FALSE;
	}

	CharArrayCpyZ(CurDir, NewDir.c_str());
	return TRUE;
}

bool PluginClass::FarLangChanged()
{
	const char *tmplang = getenv("FARLANG");

	if (!tmplang)
		tmplang = "English";

	if (farlang == tmplang)
		return false;

	farlang = tmplang;
	return true;
}

static void AppendInfoData(std::string &Str, const char *Data)
{
	if (!Str.empty())
		Str+= ' ';
	Str+= Data;
}

void PluginClass::SetInfoLineSZ(size_t Index, int TextID, const char *Data)
{
	CharArrayCpyZ(InfoLines[Index].Text, GetMsg(TextID));
	CharArrayCpyZ(InfoLines[Index].Data, Data);
}

void PluginClass::SetInfoLine(size_t Index, int TextID, const std::string &Data)
{
	SetInfoLineSZ(Index, TextID, Data.c_str());
}

void PluginClass::SetInfoLine(size_t Index, int TextID, int DataID)
{
	SetInfoLineSZ(Index, TextID, GetMsg(DataID));
}

void PluginClass::GetOpenPluginInfo(struct OpenPluginInfo *Info)
{
	Info->StructSize = sizeof(*Info);
	Info->Flags = OPIF_USEFILTER | OPIF_USESORTGROUPS | OPIF_USEHIGHLIGHTING | OPIF_ADDDOTS | OPIF_COMPAREFATTIME;
	Info->HostFile = ArcName.c_str();
	Info->CurDir = CurDir;

	if (bGOPIFirstCall)
		ArcPlugin->GetFormatName(ArcPluginNumber, ArcPluginType, FormatName, DefExt);

	std::string NameTitle;
	struct PanelInfo PInfo;
	if (::Info.Control((HANDLE)this, FCTL_GETPANELSHORTINFO, &PInfo)) {		// TruncStr
		NameTitle = FormatMessagePath(ArcName.c_str(), true,
			(PInfo.PanelRect.right - PInfo.PanelRect.left + 1 - (FormatName.size() + 3 + 4)));
	} else {
		NameTitle = FormatMessagePath(ArcName.c_str(), true, -1);
	}

	PanelTitle = StrPrintf(" %s:%s%s%s ", FormatName.c_str(), NameTitle.c_str(), *CurDir ? "/" : "", *CurDir ? CurDir : "");

	Info->PanelTitle = PanelTitle.c_str();

	if (bGOPIFirstCall || FarLangChanged()) {
		Format = StrPrintf(GetMsg(MArcFormat), FormatName.c_str());

		ZeroFill(InfoLines);
		FSF.snprintf(InfoLines[0].Text, ARRAYSIZE(InfoLines[0].Text), GetMsg(MInfoTitle), FSF.PointToName((char *)ArcName.c_str()));
		InfoLines[0].Separator = TRUE;

		std::string TmpInfoData = FormatName;
		if (ItemsInfo.UnpVer != 0)
			TmpInfoData+= StrPrintf(" %d.%d", ItemsInfo.UnpVer / 256, ItemsInfo.UnpVer % 256);
		if (ItemsInfo.HostOS)
			TmpInfoData+= StrPrintf("/%s", ItemsInfo.HostOS);
		SetInfoLine(1, MInfoArchive, TmpInfoData);

		TmpInfoData = ItemsInfo.Solid ? GetMsg(MInfoSolid) : "";
		if (CurArcInfo.SFXSize)
			AppendInfoData(TmpInfoData, GetMsg(MInfoSFX));
		if (CurArcInfo.Flags & AF_HDRENCRYPTED)
			AppendInfoData(TmpInfoData, GetMsg(MInfoHdrEncrypted));
		if (CurArcInfo.Volume)
			AppendInfoData(TmpInfoData, GetMsg(MInfoVolume));
		if (TmpInfoData.empty())
			TmpInfoData = GetMsg(MInfoNormal);
		SetInfoLine(2, MInfoArcType, TmpInfoData);

		SetInfoLine(3, MInfoArcComment, CurArcInfo.Comment ? MInfoPresent : MInfoAbsent);
		SetInfoLine(4, MInfoFileComments, ItemsInfo.Comment ? MInfoPresent : MInfoAbsent);
		SetInfoLine(5, MInfoPasswords, ItemsInfo.Encrypted ? MInfoPresent : MInfoAbsent);
		SetInfoLine(6, MInfoRecovery, CurArcInfo.Recovery ? MInfoPresent : MInfoAbsent);
		SetInfoLine(7, MInfoLock, CurArcInfo.Lock ? MInfoPresent : MInfoAbsent);
		SetInfoLine(8, MInfoAuthVer, (CurArcInfo.Flags & AF_AVPRESENT) ? MInfoPresent : MInfoAbsent);

		if (ItemsInfo.DictSize)
			SetInfoLine(9, MInfoDict, StrPrintf("%d %s", ItemsInfo.DictSize, GetMsg(MInfoDictKb)));
		else
			SetInfoLine(9, MInfoDict, MInfoAbsent);

		if (CurArcInfo.Chapters)
			SetInfoLine(10, MInfoChapters, std::to_string(CurArcInfo.Chapters));
		else
			SetInfoLine(10, MInfoChapters, MInfoAbsent);

		SetInfoLine(11, MInfoTotalFiles, std::to_string(ArcDataCount));
		SetInfoLine(12, MInfoTotalSize, NumberWithCommas(TotalSize));
		SetInfoLine(13, MInfoPackedSize, NumberWithCommas(PackedSize));
		SetInfoLine(14, MInfoRatio, StrPrintf("%d%%", MA_ToPercent(PackedSize, TotalSize)));

		ZeroFill(KeyBar);
		KeyBar.ShiftTitles[1 - 1] = (char *)"";
		KeyBar.AltTitles[6 - 1] = (char *)GetMsg(MAltF6);
		KeyBar.AltShiftTitles[9 - 1] = (char *)GetMsg(MAltShiftF9);
	}

	Info->Format = Format.c_str();
	Info->KeyBar = &KeyBar;
	Info->InfoLines = InfoLines;
	Info->InfoLinesNumber = ARRAYSIZE(InfoLines);

	CharArrayCpyZ(DescrFilesString, Opt.DescriptionNames.c_str());

	size_t DescrFilesNumber = 0;
	char *NamePtr = DescrFilesString;

	while (DescrFilesNumber < ARRAYSIZE(DescrFiles)) {
		while (__isspace(*NamePtr))
			NamePtr++;
		if (*NamePtr == 0)
			break;
		DescrFiles[DescrFilesNumber++] = NamePtr;
		if ((NamePtr = strchr(NamePtr, ',')) == NULL)
			break;
		*(NamePtr++) = 0;
	}

	Info->DescrFiles = DescrFiles;

	if (!Opt.ReadDescriptions || DizPresent)
		Info->DescrFilesNumber = 0;
	else
		Info->DescrFilesNumber = (int)DescrFilesNumber;

	bGOPIFirstCall = false;
}
