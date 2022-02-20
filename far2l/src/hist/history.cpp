/*
history.cpp

История (Alt-F8, Alt-F11, Alt-F12)
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


#include "history.hpp"
#include "keys.hpp"
#include "vmenu.hpp"
#include "lang.hpp"
#include "message.hpp"
#include "clipboard.hpp"
#include "config.hpp"
#include "ConfigRW.hpp"
#include "strmix.hpp"
#include "dialog.hpp"
#include "interf.hpp"
#include <crc64.h>
#include "FileMasksProcessor.hpp"

static uint64_t RegKey2ID(const FARString &str)
{
	const std::string &s = str.GetMB();
	return crc64(0, (const unsigned char *)s.c_str(), s.size());
}

History::History(enumHISTORYTYPE TypeHistory, size_t HistoryCount, const std::string &RegKey, const int *EnableSave, bool SaveType):
	strRegKey(RegKey),
	EnableAdd(true),
	KeepSelectedPos(false),
	SaveType(SaveType),
	RemoveDups(1),
	TypeHistory(TypeHistory),
	HistoryCount(HistoryCount),
	EnableSave(EnableSave),
	CurrentItem(nullptr)
{
	if (*EnableSave)
		ReadHistory();
}

History::~History()
{
}

static bool IsAllowedForHistory(const wchar_t *Str)
{
	if (!Str || !*Str)
		return false;

	FileMasksProcessor fmp;
	fmp.Set(Opt.AutoComplete.Exceptions.CPtr(), FMPF_ADDASTERISK);
	if (!fmp.IsEmpty() && fmp.Compare(Str)) {
		return false;
	}
	
	return true;
}


/*
   SaveForbid - принудительно запретить запись добавляемой строки.
                Используется на панели плагина
*/
void History::AddToHistory(const wchar_t *Str, int Type, const wchar_t *Prefix, bool SaveForbid)
{
	if (!EnableAdd)
		return;
		
	if (!IsAllowedForHistory(Str)) {
		fprintf(stderr, "AddToHistory - disallowed: '%ls'\n", Str);
		return;
	}

	SyncChanges();
	AddToHistoryLocal(Str,Prefix,Type);

	if (*EnableSave && !SaveForbid)
		SaveHistory();
}


void History::AddToHistoryLocal(const wchar_t *Str, const wchar_t *Prefix, int Type)
{
	if (!Str || !*Str)
		return;

	HistoryRecord AddRecord;

	if (TypeHistory == HISTORYTYPE_FOLDER && Prefix && *Prefix)
	{
		AddRecord.strName = Prefix;
		AddRecord.strName += L":";
	}

	AddRecord.strName += Str;
	AddRecord.Type=Type;

	if (RemoveDups) // удалять дубликаты?
	{
		for (HistoryRecord *HistoryItem=HistoryList.First(); HistoryItem ; HistoryItem=HistoryList.Next(HistoryItem))
		{
			if (EqualType(AddRecord.Type,HistoryItem->Type))
			{
				if ((RemoveDups==1 && !StrCmp(AddRecord.strName,HistoryItem->strName)) ||
				        (RemoveDups==2 && !StrCmpI(AddRecord.strName,HistoryItem->strName)))
				{
					AddRecord.Lock=HistoryItem->Lock;
					HistoryItem=HistoryList.Delete(HistoryItem);
					break;
				}
			}
		}
	}

	if (HistoryList.Count()>=HistoryCount)
	{
		for (HistoryRecord *HistoryItem=HistoryList.First(); HistoryItem && HistoryList.Count()>=HistoryCount; )
		{
			if (!HistoryItem->Lock)
			{
				HistoryRecord *tmp = HistoryItem;
				HistoryItem=HistoryList.Next(HistoryItem);
				HistoryList.Delete(tmp);
			}
			else
			{
				HistoryItem=HistoryList.Next(HistoryItem);
		    }
		}
	}

	WINPORT(GetSystemTimeAsFileTime)(&AddRecord.Timestamp); // in UTC
	HistoryList.Push(&AddRecord);
	ResetPosition();
}

bool History::SaveHistory()
{
	if (!*EnableSave)
		return true;

	if (!HistoryList.Count())
	{
		ConfigWriter(strRegKey).RemoveSection();
		return true;
	}

	//for dialogs, locked items should show first (be last in the list)
	if (TypeHistory == HISTORYTYPE_DIALOG)
	{
		for (const HistoryRecord *HistoryItem=HistoryList.First(), *LastItem=HistoryList.Last(); HistoryItem; )
		{
			const HistoryRecord *tmp = HistoryItem;

			HistoryItem=HistoryList.Next(HistoryItem);

			if (tmp->Lock)
				HistoryList.MoveAfter(HistoryList.Last(), tmp);

			if (tmp == LastItem)
				break;
		}
	}

	bool ret = false;
	try {
		std::wstring strTypes, strLines, strLocks;
		std::vector<FILETIME> vTimes;
		int Position = -1;
		size_t i = HistoryList.Count();
		for (const HistoryRecord *HistoryItem = HistoryList.Last(); HistoryItem ; HistoryItem = HistoryList.Prev(HistoryItem))
		{
			if (i != HistoryList.Count())
				strLines+= L'\n';

			size_t p = strLines.size();

			strLines.append(HistoryItem->strName.CPtr(), HistoryItem->strName.GetLength());

			for (; p < strLines.size(); ++p) {
				if (strLines[p] == L'\n')
					strLines[p] = L'\r';
			}

			if (SaveType)
				strTypes+= L'0' + HistoryItem->Type;

			strLocks+= L'0' + HistoryItem->Lock;
			vTimes.emplace_back(HistoryItem->Timestamp);

			--i;

			if (HistoryItem == CurrentItem)
				Position = static_cast<int>(i);
		}

		ConfigWriter cfg_writer(strRegKey);
		cfg_writer.SetString("Lines", strLines.c_str());
		if (SaveType) {
			cfg_writer.SetString("Types", strTypes.c_str());
		}
		cfg_writer.SetString("Locks", strLocks.c_str());
		cfg_writer.SetBytes("Times", (const unsigned char *)&vTimes[0], vTimes.size() * sizeof(FILETIME));
		cfg_writer.SetInt("Position", Position);

		ret = cfg_writer.Save();
		if (ret) {
			LoadedStat = ConfigReader::SavedSectionStat(strRegKey);
		}

	} catch (std::exception &e) {
	}

	return ret;
}

bool History::ReadLastItem(const char *RegKey, FARString &strStr)
{
	strStr.Clear();

	ConfigReader cfg_reader(RegKey);
	if (!cfg_reader.HasSection())
		return false;

	if (!cfg_reader.GetString(strStr, "Lines", L""))
		return false;

	// last item is first in config
	size_t p;
	if (strStr.Pos(p, L'\n'))
		strStr.Remove(p, strStr.GetLength() - p);

	return true;
}

bool History::ReadHistory(bool bOnlyLines)
{
	int Position = -1;
	FARString strLines, strLocks, strTypes;
	std::vector<unsigned char> vTimes;

	ConfigReader cfg_reader(strRegKey);

	if (!cfg_reader.GetString(strLines, "Lines", L""))
		return false;

	if (!bOnlyLines)
	{
		Position = cfg_reader.GetInt("Position", Position);
		cfg_reader.GetBytes(vTimes, "Times");
		cfg_reader.GetString(strLocks, "Locks", L"");
		cfg_reader.GetString(strTypes, "Types", L"");
	}

	size_t StrPos = 0, LinesPos = 0, TypesPos = 0, LocksPos = 0, TimePos = 0;
	while (LinesPos < strLines.GetLength() && StrPos < HistoryCount)
	{
		size_t LineEnd;
		if (!strLines.Pos(LineEnd, L'\n', LinesPos))
			LineEnd = strLines.GetLength();

		HistoryRecord *AddRecord = HistoryList.Unshift();
		AddRecord->strName = strLines.SubStr(LinesPos, LineEnd - LinesPos);
		LinesPos = LineEnd + 1;

		if (TypesPos < strTypes.GetLength())
		{
			if (iswdigit(strTypes[TypesPos]))
			{
				AddRecord->Type = strTypes[TypesPos] - L'0';
			}
			++TypesPos;
		}

		if (LocksPos < strLocks.GetLength())
		{
			if (iswdigit(strLocks[LocksPos]))
			{
				AddRecord->Lock = (strLocks[LocksPos] != L'0');
			}
			++LocksPos;
		}

		if (TimePos + sizeof(FILETIME) <= vTimes.size())
		{
			AddRecord->Timestamp = *(const FILETIME *)&vTimes[TimePos];
			++TimePos;
		}

		//HistoryList.Unshift(&AddRecord);

		if ((int)StrPos == Position)
			CurrentItem = HistoryList.First();
	}

	LoadedStat = cfg_reader.LoadedSectionStat();

	return true;
}

void History::SyncChanges()
{
	const struct stat &CurrentStat = ConfigReader::SavedSectionStat(strRegKey);
	if (LoadedStat.st_ino != CurrentStat.st_ino
			|| LoadedStat.st_size != CurrentStat.st_size
			|| LoadedStat.st_mtime != CurrentStat.st_mtime) {
		fprintf(stderr, "History::SyncChanges: %s\n", strRegKey.c_str());
		CurrentItem = nullptr;
		HistoryList.Clear();
		ReadHistory();
	}
}

const wchar_t *History::GetTitle(int Type)
{
	switch (Type)
	{
		case 0: // вьювер
			return Msg::HistoryView;
		case 1: // обычное открытие в редакторе
		case 4: // открытие с локом
			return Msg::HistoryEdit;
		case 2: // external - без ожидания
		case 3: // external - AlwaysWaitFinish
			return Msg::HistoryExt;
	}

	return L"";
}

int History::Select(const wchar_t *Title, const wchar_t *HelpTopic, FARString &strStr, int &Type)
{
	int Height=ScrY-8;
	VMenu HistoryMenu(Title,nullptr,0,Height);
	HistoryMenu.SetFlags(VMENU_SHOWAMPERSAND|VMENU_WRAPMODE);

	if (HelpTopic)
		HistoryMenu.SetHelp(HelpTopic);

	HistoryMenu.SetPosition(-1,-1,0,0);
	HistoryMenu.AssignHighlights(TRUE);
	return ProcessMenu(strStr, Title, HistoryMenu, Height, Type, nullptr);
}

int History::Select(VMenu &HistoryMenu, int Height, Dialog *Dlg, FARString &strStr)
{
	int Type=0;
	return ProcessMenu(strStr, nullptr, HistoryMenu, Height, Type, Dlg);
}

/*
 Return:
  -1 - Error???
   0 - Esc
   1 - Enter
   2 - Shift-Enter
   3 - Ctrl-Enter
   4 - F3
   5 - F4
   6 - Ctrl-Shift-Enter
   7 - Ctrl-Alt-Enter
*/
int History::ProcessMenu(FARString &strStr, const wchar_t *Title, VMenu &HistoryMenu, int Height, int &Type, Dialog *Dlg)
{
	MenuItemEx MenuItem;
	HistoryRecord *SelectedRecord=nullptr;
	FarListPos Pos={0,0};
	int Code=-1;
	int RetCode=1;
	bool Done=false;
	bool SetUpMenuPos=false;

	SyncChanges();
	if (TypeHistory == HISTORYTYPE_DIALOG && HistoryList.Empty())
		return 0;

	while (!Done)
	{
		bool IsUpdate=false;
		HistoryMenu.DeleteItems();
		HistoryMenu.Modal::ClearDone();
		HistoryMenu.SetBottomTitle(Msg::HistoryFooter);

		// заполнение пунктов меню
		for (const HistoryRecord *HistoryItem=TypeHistory==HISTORYTYPE_DIALOG?HistoryList.Last():HistoryList.First(); HistoryItem ; HistoryItem=TypeHistory==HISTORYTYPE_DIALOG?HistoryList.Prev(HistoryItem):HistoryList.Next(HistoryItem))
		{
			FARString strRecord = HistoryItem->strName;
			strRecord.Clear();

			if (TypeHistory == HISTORYTYPE_VIEW)
			{
				strRecord += GetTitle(HistoryItem->Type);
				strRecord += L":";
				strRecord += (HistoryItem->Type==4?L"-":L" ");
			}

			/*
				TODO: возможно здесь! или выше....
				char Date[16],Time[16], OutStr[32];
				ConvertDate(HistoryItem->Timestamp,Date,Time,5,TRUE,FALSE,TRUE,TRUE);
				а дальше
				strRecord += дату и время
			*/
			strRecord += HistoryItem->strName;;

			if (TypeHistory != HISTORYTYPE_DIALOG)
				ReplaceStrings(strRecord, L"&",L"&&", -1);

			MenuItem.Clear();
			MenuItem.strName = strRecord;
			MenuItem.SetCheck(HistoryItem->Lock?1:0);

			if (!SetUpMenuPos)
				MenuItem.SetSelect(CurrentItem==HistoryItem || (!CurrentItem && HistoryItem==HistoryList.Last()));

			//NB: here is really should be used sizeof(HistoryItem), not sizeof(*HistoryItem)
			//cuz sizeof(void *) has special meaning in SetUserData!
			HistoryMenu.SetUserData(HistoryItem,sizeof(HistoryItem),HistoryMenu.AddItem(&MenuItem));
		}

		//MenuItem.Clear ();
		//MenuItem.strName = L"                    ";
		//if (!SetUpMenuPos)
		//MenuItem.SetSelect(CurLastPtr==-1 || CurLastPtr>=HistoryList.Length);
		//HistoryMenu.SetUserData(nullptr,sizeof(OneItem *),HistoryMenu.AddItem(&MenuItem));

		if (TypeHistory == HISTORYTYPE_DIALOG)
			Dlg->SetComboBoxPos();
		else
			HistoryMenu.SetPosition(-1,-1,0,0);

		if (SetUpMenuPos)
		{
			Pos.SelectPos=Pos.SelectPos < (int)HistoryList.Count() ? Pos.SelectPos : (int)HistoryList.Count()-1;
			Pos.TopPos=Min(Pos.TopPos,HistoryMenu.GetItemCount()-Height);
			HistoryMenu.SetSelectPos(&Pos);
			SetUpMenuPos=false;
		}

		/*BUGBUG???
			if (TypeHistory == HISTORYTYPE_DIALOG)
			{
					//  Перед отрисовкой спросим об изменении цветовых атрибутов
					BYTE RealColors[VMENU_COLOR_COUNT];
					FarListColors ListColors={0};
					ListColors.ColorCount=VMENU_COLOR_COUNT;
					ListColors.Colors=RealColors;
					HistoryMenu.GetColors(&ListColors);
					if(DlgProc((HANDLE)this,DN_CTLCOLORDLGLIST,CurItem->ID,(LONG_PTR)&ListColors))
						HistoryMenu.SetColors(&ListColors);
				}
		*/
		HistoryMenu.Show();

		while (!HistoryMenu.Done())
		{
			if (TypeHistory == HISTORYTYPE_DIALOG && (!Dlg->GetDropDownOpened() || HistoryList.Empty()))
			{
				HistoryMenu.ProcessKey(KEY_ESC);
				continue;
			}

			int Key=HistoryMenu.ReadInput();

			if (TypeHistory == HISTORYTYPE_DIALOG && Key==KEY_TAB) // Tab в списке хистори диалогов - аналог Enter
			{
				HistoryMenu.ProcessKey(KEY_ENTER);
				continue;
			}

			HistoryMenu.GetSelectPos(&Pos);
			HistoryRecord *CurrentRecord=(HistoryRecord *)HistoryMenu.GetUserData(nullptr,sizeof(HistoryRecord *),Pos.SelectPos);

			switch (Key)
			{
				case KEY_CTRLR: // обновить с удалением недоступных
				{
					if (TypeHistory == HISTORYTYPE_FOLDER || TypeHistory == HISTORYTYPE_VIEW)
					{
						bool ModifiedHistory=false;

						for (HistoryRecord *HistoryItem=HistoryList.First(); HistoryItem ; HistoryItem=HistoryList.Next(HistoryItem))
						{
							if (HistoryItem->Lock) // залоченные не трогаем
								continue;

							// убить запись из истории
							if (apiGetFileAttributes(HistoryItem->strName) == INVALID_FILE_ATTRIBUTES)
							{
								HistoryItem=HistoryList.Delete(HistoryItem);
								ModifiedHistory=true;
							}
						}

						if (ModifiedHistory) // избавляемся от лишних телодвижений
						{
							SaveHistory(); // сохранить
							HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
							HistoryMenu.SetUpdateRequired(TRUE);
							IsUpdate=true;
						}

						ResetPosition();
					}

					break;
				}
				case KEY_CTRLSHIFTNUMENTER:
				case KEY_CTRLNUMENTER:
				case KEY_SHIFTNUMENTER:
				case KEY_CTRLSHIFTENTER:
				case KEY_CTRLENTER:
				case KEY_SHIFTENTER:
				case KEY_CTRLALTENTER:
				case KEY_CTRLALTNUMENTER:
				{
					if (TypeHistory == HISTORYTYPE_DIALOG)
						break;

					HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
					Done=true;
					RetCode = Key==KEY_CTRLALTENTER||Key==KEY_CTRLALTNUMENTER?7:(Key==KEY_CTRLSHIFTENTER||Key==KEY_CTRLSHIFTNUMENTER?6:(Key==KEY_SHIFTENTER||Key==KEY_SHIFTNUMENTER?2:3));
					break;
				}
				case KEY_F3:
				case KEY_F4:
				case KEY_NUMPAD5:  case KEY_SHIFTNUMPAD5:
				{
					if (TypeHistory == HISTORYTYPE_DIALOG)
						break;

					HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
					Done=true;
					RetCode=(Key==KEY_F4? 5 : 4);
					break;
				}
				// $ 09.04.2001 SVS - Фича - копирование из истории строки в Clipboard
				case KEY_CTRLC:
				case KEY_CTRLINS:  case KEY_CTRLNUMPAD0:
				{
					if (CurrentRecord)
						CopyToClipboard(CurrentRecord->strName);

					break;
				}
				// Lock/Unlock
				case KEY_INS:
				case KEY_NUMPAD0:
				{
					if (HistoryMenu.GetItemCount()/* > 1*/)
					{
						CurrentItem=CurrentRecord;
						CurrentItem->Lock=CurrentItem->Lock?false:true;
						HistoryMenu.Hide();
						ResetPosition();
						SaveHistory();
						HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
						HistoryMenu.SetUpdateRequired(TRUE);
						IsUpdate=true;
						SetUpMenuPos=true;
					}

					break;
				}
				case KEY_SHIFTNUMDEL:
				case KEY_SHIFTDEL:
				{
					if (HistoryMenu.GetItemCount()/* > 1*/)
					{
						if (!CurrentRecord->Lock)
						{
							HistoryMenu.Hide();
							HistoryList.Delete(CurrentRecord);
							ResetPosition();
							SaveHistory();
							HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
							HistoryMenu.SetUpdateRequired(TRUE);
							IsUpdate=true;
							SetUpMenuPos=true;
						}
					}

					break;
				}
				case KEY_NUMDEL:
				case KEY_DEL:
				{
					if (HistoryMenu.GetItemCount()/* > 1*/ &&
					        (!Opt.Confirm.HistoryClear ||
					         (Opt.Confirm.HistoryClear &&
					          !Message(MSG_WARNING,2,
					                      ((TypeHistory==HISTORYTYPE_CMD || TypeHistory==HISTORYTYPE_DIALOG ? Msg::HistoryTitle
					                       : (TypeHistory==HISTORYTYPE_FOLDER ? Msg::FolderHistoryTitle : Msg::ViewHistoryTitle))),
					                  Msg::HistoryClear,
					                  Msg::Clear,Msg::Cancel))))
					{
						for (HistoryRecord *HistoryItem=HistoryList.First(); HistoryItem ; HistoryItem=HistoryList.Next(HistoryItem))
						{
							if (HistoryItem->Lock) // залоченные не трогаем
								continue;

							HistoryItem=HistoryList.Delete(HistoryItem);
						}

						ResetPosition();
						HistoryMenu.Hide();
						SaveHistory();
						HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
						HistoryMenu.SetUpdateRequired(TRUE);
						IsUpdate=true;
					}

					break;
				}
				default:
					HistoryMenu.ProcessInput();
					break;
			}
		}

		if (IsUpdate)
			continue;

		Done=true;
		Code=HistoryMenu.Modal::GetExitCode();

		if (Code >= 0)
		{
			SelectedRecord=(HistoryRecord *)HistoryMenu.GetUserData(nullptr,sizeof(HistoryRecord *),Code);

			if (!SelectedRecord)
				return -1;

			//BUGUBUG: eliminate those magic numbers!
			if (SelectedRecord->Type != 2 && SelectedRecord->Type != 3 // ignore external
				&& RetCode != 3 && ((TypeHistory == HISTORYTYPE_FOLDER && !SelectedRecord->Type) || TypeHistory == HISTORYTYPE_VIEW) && apiGetFileAttributes(SelectedRecord->strName) == INVALID_FILE_ATTRIBUTES)
			{
				WINPORT(SetLastError)(ERROR_FILE_NOT_FOUND);

				if (SelectedRecord->Type == 1 && TypeHistory == HISTORYTYPE_VIEW) // Edit? тогда спросим и если надо создадим
				{
					if (!Message(MSG_WARNING|MSG_ERRORTYPE,2,Title,SelectedRecord->strName,Msg::ViewHistoryIsCreate,Msg::HYes,Msg::HNo))
						break;
				}
				else
				{
					Message(MSG_WARNING|MSG_ERRORTYPE,1,Title,SelectedRecord->strName,Msg::Ok);
				}

				Done=false;
				SetUpMenuPos=true;
				HistoryMenu.Modal::SetExitCode(Pos.SelectPos=Code);
				continue;
			}
		}
	}

	if (Code < 0 || !SelectedRecord)
		return 0;

	if (KeepSelectedPos)
	{
		CurrentItem = SelectedRecord;
	}

	strStr = SelectedRecord->strName;

	if (RetCode < 4 || RetCode == 6 || RetCode == 7)
	{
		Type=SelectedRecord->Type;
	}
	else
	{
		Type=RetCode-4;

		if (Type == 1 && SelectedRecord->Type == 4)
			Type=4;

		RetCode=1;
	}

	return RetCode;
}

void History::GetPrev(FARString &strStr)
{
	CurrentItem=HistoryList.Prev(CurrentItem);

	if (!CurrentItem) {
		SyncChanges();
		CurrentItem=HistoryList.First();
	}

	if (CurrentItem)
		strStr = CurrentItem->strName;
	else
		strStr.Clear();
}


void History::GetNext(FARString &strStr)
{
	if (CurrentItem)
		CurrentItem=HistoryList.Next(CurrentItem);
	else
		SyncChanges();

	if (CurrentItem)
		strStr = CurrentItem->strName;
	else
		strStr.Clear();
}

bool History::DeleteMatching(FARString &strStr)
{
	SyncChanges();

	for (HistoryRecord *HistoryItem = HistoryList.Prev(CurrentItem);
		HistoryItem != CurrentItem; HistoryItem = HistoryList.Prev(HistoryItem))
	{
		if (!HistoryItem || HistoryItem->Lock)
			continue;

		if (HistoryItem->strName == strStr)
		{
			HistoryList.Delete(HistoryItem);
			SaveHistory();
			return true;
		}
	}

	return false;
}

bool History::GetSimilar(FARString &strStr, int LastCmdPartLength, bool bAppend)
{
	SyncChanges();
	int Length=(int)strStr.GetLength();

	if (LastCmdPartLength!=-1 && LastCmdPartLength<Length)
		Length=LastCmdPartLength;

	if (LastCmdPartLength==-1)
	{
		ResetPosition();
	}

	for (HistoryRecord *HistoryItem=HistoryList.Prev(CurrentItem); HistoryItem != CurrentItem; HistoryItem=HistoryList.Prev(HistoryItem))
	{
		if (!HistoryItem)
			continue;

		if (!StrCmpNI(strStr,HistoryItem->strName,Length) && StrCmp(strStr,HistoryItem->strName))
		{
			if (bAppend)
				strStr += &HistoryItem->strName[Length];
			else
				strStr = HistoryItem->strName;

			CurrentItem = HistoryItem;
			return true;
		}
	}

	return false;
}

bool History::GetAllSimilar(VMenu &HistoryMenu,const wchar_t *Str)
{
	SyncChanges();
	int Length=StrLength(Str);
	for (HistoryRecord *HistoryItem=HistoryList.Last();HistoryItem;HistoryItem=HistoryList.Prev(HistoryItem))
	{
		if (!StrCmpNI(Str,HistoryItem->strName,Length) && StrCmp(Str,HistoryItem->strName) && IsAllowedForHistory(HistoryItem->strName.CPtr()))
		{
			HistoryMenu.AddItem(HistoryItem->strName);
		}
	}
	return false;
}

void History::SetAddMode(bool EnableAdd, int RemoveDups, bool KeepSelectedPos)
{
	History::EnableAdd=EnableAdd;
	History::RemoveDups=RemoveDups;
	History::KeepSelectedPos=KeepSelectedPos;
}

bool History::EqualType(int Type1, int Type2)
{
	return Type1 == Type2 || (TypeHistory == HISTORYTYPE_VIEW && ((Type1 == 4 && Type2 == 1) || (Type1 == 1 && Type2 == 4)))?true:false;
}

