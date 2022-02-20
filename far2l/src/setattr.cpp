/*
setattr.cpp

Установка атрибутов файлов
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


#include "lang.hpp"
#include "dialog.hpp"
#include "chgprior.hpp"
#include "scantree.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "ctrlobj.hpp"
#include "constitle.hpp"
#include "TPreRedrawFunc.hpp"
#include "keyboard.hpp"
#include "message.hpp"
#include "config.hpp"
#include "datetime.hpp"
#include "fileattr.hpp"
#include "setattr.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "fileowner.hpp"
#include "wakeful.hpp"
#include "DlgGuid.hpp"
#include "execute.hpp"


enum SETATTRDLG
{
	SA_DOUBLEBOX,
	SA_TEXT_LABEL,
	SA_TEXT_NAME,
	SA_SEPARATOR1,
	SA_TEXT_OWNER,
	SA_EDIT_OWNER,
	SA_TEXT_GROUP,
	SA_EDIT_GROUP,
	SA_SEPARATOR2,
	SA_TEXT_MODE_USER,
	SA_TEXT_MODE_GROUP,
	SA_TEXT_MODE_OTHER,
	SA_ATTR_FIRST,
	SA_CHECKBOX_USER_READ = SA_ATTR_FIRST,
	SA_CHECKBOX_USER_WRITE,
	SA_CHECKBOX_USER_EXECUTE,
	SA_CHECKBOX_GROUP_READ,
	SA_CHECKBOX_GROUP_WRITE,
	SA_CHECKBOX_GROUP_EXECUTE,
	SA_CHECKBOX_OTHER_READ,
	SA_CHECKBOX_OTHER_WRITE,
	SA_ATTR_LAST,
	SA_CHECKBOX_OTHER_EXECUTE = SA_ATTR_LAST,
	SA_SEPARATOR3,
	SA_TEXT_TITLEDATE,
	SA_TEXT_LAST_ACCESS,
	SA_FIXEDIT_LAST_ACCESS_DATE,
	SA_FIXEDIT_LAST_ACCESS_TIME,
	SA_TEXT_LAST_MODIFICATION,
	SA_FIXEDIT_LAST_MODIFICATION_DATE,
	SA_FIXEDIT_LAST_MODIFICATION_TIME,
	SA_TEXT_LAST_CHANGE,
	SA_FIXEDIT_LAST_CHANGE_DATE,
	SA_FIXEDIT_LAST_CHANGE_TIME,
	SA_BUTTON_ORIGINAL,
	SA_BUTTON_CURRENT,
	SA_BUTTON_BLANK,
	SA_SEPARATOR4,
	SA_CHECKBOX_SUBFOLDERS,
	SA_SEPARATOR5,
	SA_BUTTON_SET,
	SA_BUTTON_SYSTEMDLG,
	SA_BUTTON_CANCEL
};

enum DIALOGMODE
{
	MODE_FILE,
	MODE_FOLDER,
	MODE_MULTIPLE,
};

struct SetAttrDlgParam
{
	bool Plugin;
	DWORD FileSystemFlags;
	DIALOGMODE DialogMode;
	FARString strSelName;
	FARString strOwner;
	FARString strGroup;
	bool OwnerChanged, GroupChanged;
	// значения CheckBox`ов на момент старта диалога
	int OriginalCBAttr[SA_ATTR_LAST-SA_ATTR_FIRST+1];
	int OriginalCBAttr2[SA_ATTR_LAST-SA_ATTR_FIRST+1];
	DWORD OriginalCBFlag[SA_ATTR_LAST-SA_ATTR_FIRST+1];
	FARCHECKEDSTATE OSubfoldersState;
	bool OAccessTime, OModifyTime, OStatusChangeTime;
};

#define DM_SETATTR (DM_USER+1)


static void BlankEditIfChanged(HANDLE hDlg,int EditControl, FARString &Remembered, bool &Changed)
{
	LPCWSTR Actual = reinterpret_cast<LPCWSTR>(SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, EditControl, 0));
	if(!Changed) 
		Changed = StrCmp(Actual, Remembered)!=0;
	
	Remembered = Actual;
	
	if(!Changed)
		SendDlgMessage(hDlg, DM_SETTEXTPTR, EditControl, reinterpret_cast<LONG_PTR>(L""));
}

LONG_PTR WINAPI SetAttrDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2)
{
	SetAttrDlgParam *DlgParam=reinterpret_cast<SetAttrDlgParam*>(SendDlgMessage(hDlg,DM_GETDLGDATA,0,0));

	switch (Msg)
	{
		case DN_BTNCLICK:

			if ((Param1>=SA_ATTR_FIRST&&Param1<=SA_ATTR_LAST)||Param1==SA_CHECKBOX_SUBFOLDERS)
			{
				if(Param1!=SA_CHECKBOX_SUBFOLDERS)
				{
					DlgParam->OriginalCBAttr[Param1-SA_ATTR_FIRST]=static_cast<int>(Param2);
					DlgParam->OriginalCBAttr2[Param1-SA_ATTR_FIRST]=0;
				}
				int FocusPos=static_cast<int>(SendDlgMessage(hDlg,DM_GETFOCUS,0,0));
				FARCHECKEDSTATE SubfoldersState=static_cast<FARCHECKEDSTATE>(SendDlgMessage(hDlg,DM_GETCHECK,SA_CHECKBOX_SUBFOLDERS,0));


				{
					// если снимаем атрибуты для SubFolders
					// этот кусок всегда работает если есть хотя бы одна папка
					// иначе SA_CHECKBOX_SUBFOLDERS недоступен и всегда снят.
					if (FocusPos == SA_CHECKBOX_SUBFOLDERS)
					{
						if (DlgParam->DialogMode==MODE_FOLDER) // каталог однозначно!
						{
							if (DlgParam->OSubfoldersState != SubfoldersState) // Состояние изменилось?
							{
								// установили?
								if (SubfoldersState != BSTATE_UNCHECKED)
								{
									for (int i=SA_ATTR_FIRST; i<=SA_ATTR_LAST; i++)
									{
										SendDlgMessage(hDlg,DM_SET3STATE,i,TRUE);
										if (DlgParam->OriginalCBAttr2[i-SA_ATTR_FIRST]==-1)
										{
											SendDlgMessage(hDlg,DM_SETCHECK,i,BSTATE_3STATE);
										}
									}
									
									BlankEditIfChanged(hDlg, SA_EDIT_OWNER, DlgParam->strOwner, DlgParam->OwnerChanged);
									BlankEditIfChanged(hDlg, SA_EDIT_GROUP, DlgParam->strGroup, DlgParam->GroupChanged);
								}
								// сняли?
								else
								{
									for (int i=SA_ATTR_FIRST; i<=SA_ATTR_LAST; i++)
									{
										SendDlgMessage(hDlg,DM_SET3STATE,i,FALSE);
										SendDlgMessage(hDlg,DM_SETCHECK,i,DlgParam->OriginalCBAttr[i-SA_ATTR_FIRST]);
									}
									SendDlgMessage(hDlg, DM_SETTEXTPTR, SA_EDIT_OWNER, 
										reinterpret_cast<LONG_PTR>(DlgParam->strOwner.CPtr()));
									SendDlgMessage(hDlg, DM_SETTEXTPTR, SA_EDIT_GROUP, 
										reinterpret_cast<LONG_PTR>(DlgParam->strGroup.CPtr()));
								}


								if (Opt.SetAttrFolderRules)
								{
									FAR_FIND_DATA_EX FindData;

									if (apiGetFindDataEx(DlgParam->strSelName, FindData))
									{
										const SETATTRDLG Items[] = { SA_TEXT_LAST_ACCESS, SA_TEXT_LAST_MODIFICATION, SA_TEXT_LAST_CHANGE };
										bool* ParamTimes[]={&DlgParam->OAccessTime, &DlgParam->OModifyTime, &DlgParam->OStatusChangeTime};
										const PFILETIME FDTimes[]={&FindData.ftUnixAccessTime, 
														&FindData.ftUnixModificationTime, &FindData.ftUnixStatusChangeTime};

										for (size_t i=0; i<ARRAYSIZE(Items); i++)
										{
											if (!*ParamTimes[i])
											{
												SendDlgMessage(hDlg,DM_SETATTR,Items[i],(SubfoldersState != BSTATE_UNCHECKED)?0:(LONG_PTR)FDTimes[i]);
												*ParamTimes[i]=false;
											}
										}
									}
								}
							}
						}
						// много объектов
						else
						{
							// Состояние изменилось?
							if (DlgParam->OSubfoldersState!=SubfoldersState)
							{
								// установили?
								if (SubfoldersState != BSTATE_UNCHECKED)
								{
									for (int i=SA_ATTR_FIRST; i<= SA_ATTR_LAST; i++)
									{
										if (DlgParam->OriginalCBAttr2[i-SA_ATTR_FIRST]==-1)
										{
											SendDlgMessage(hDlg,DM_SET3STATE,i,TRUE);
											SendDlgMessage(hDlg,DM_SETCHECK,i,BSTATE_3STATE);
										}
									}
									SendDlgMessage(hDlg,DM_SETTEXTPTR,SA_EDIT_OWNER,reinterpret_cast<LONG_PTR>(L""));
									SendDlgMessage(hDlg,DM_SETTEXTPTR,SA_EDIT_GROUP,reinterpret_cast<LONG_PTR>(L""));
								}
								// сняли?
								else
								{
									for (int i=SA_ATTR_FIRST; i<= SA_ATTR_LAST; i++)
									{
										SendDlgMessage(hDlg,DM_SET3STATE,i,((DlgParam->OriginalCBFlag[i-SA_ATTR_FIRST]&DIF_3STATE)?TRUE:FALSE));
										SendDlgMessage(hDlg,DM_SETCHECK,i,DlgParam->OriginalCBAttr[i-SA_ATTR_FIRST]);
									}
									SendDlgMessage(hDlg,DM_SETTEXTPTR,SA_EDIT_OWNER,reinterpret_cast<LONG_PTR>(DlgParam->strOwner.CPtr()));
									SendDlgMessage(hDlg,DM_SETTEXTPTR,SA_EDIT_GROUP,reinterpret_cast<LONG_PTR>(DlgParam->strGroup.CPtr()));
								}
							}
						}

						DlgParam->OSubfoldersState=SubfoldersState;
					}
				}

				return TRUE;
			}
			// Set Original? / Set All? / Clear All?
			else if (Param1 == SA_BUTTON_ORIGINAL)
			{
				FAR_FIND_DATA_EX FindData;

				if (apiGetFindDataEx(DlgParam->strSelName, FindData))
				{
					SendDlgMessage(hDlg,DM_SETATTR,SA_TEXT_LAST_ACCESS,(LONG_PTR)&FindData.ftUnixAccessTime);
					SendDlgMessage(hDlg,DM_SETATTR,SA_TEXT_LAST_MODIFICATION,(LONG_PTR)&FindData.ftUnixModificationTime);
					SendDlgMessage(hDlg,DM_SETATTR,SA_TEXT_LAST_CHANGE,(LONG_PTR)&FindData.ftUnixStatusChangeTime);
					DlgParam->OAccessTime=DlgParam->OModifyTime=DlgParam->OStatusChangeTime=false;
				}

				SendDlgMessage(hDlg,DM_SETFOCUS,SA_FIXEDIT_LAST_ACCESS_DATE,0);
				return TRUE;
			}
			else if (Param1 == SA_BUTTON_CURRENT || Param1 == SA_BUTTON_BLANK)
			{
				LONG_PTR Value = 0;
				FILETIME CurrentTime;
				if(Param1 == SA_BUTTON_CURRENT)
				{
					WINPORT(GetSystemTimeAsFileTime)(&CurrentTime);
					Value = reinterpret_cast<LONG_PTR>(&CurrentTime);
				}
				SendDlgMessage(hDlg, DM_SETATTR, SA_TEXT_LAST_ACCESS, Value);
				SendDlgMessage(hDlg, DM_SETATTR, SA_TEXT_LAST_MODIFICATION, Value);
				//SendDlgMessage(hDlg, DM_SETATTR, SA_TEXT_LAST_CHANGE, Value);
				DlgParam->OAccessTime=DlgParam->OModifyTime=DlgParam->OStatusChangeTime=true;
				SendDlgMessage(hDlg,DM_SETFOCUS,SA_FIXEDIT_LAST_ACCESS_DATE,0);
				return TRUE;
			}

			break;
		case DN_MOUSECLICK:
		{
			//_SVS(SysLog(L"Msg=DN_MOUSECLICK Param1=%d Param2=%d",Param1,Param2));
			if (Param1>=SA_TEXT_LAST_ACCESS && Param1<=SA_FIXEDIT_LAST_CHANGE_TIME)
			{
				if (reinterpret_cast<MOUSE_EVENT_RECORD*>(Param2)->dwEventFlags==DOUBLE_CLICK)
				{
					// Дадим Менеджеру диалогов "попотеть"
					DefDlgProc(hDlg,Msg,Param1,Param2);
					SendDlgMessage(hDlg,DM_SETATTR,Param1,-1);
				}

				if (Param1 == SA_TEXT_LAST_ACCESS || Param1 == SA_TEXT_LAST_MODIFICATION || Param1 == SA_TEXT_LAST_CHANGE)
				{
					Param1++;
				}

				SendDlgMessage(hDlg,DM_SETFOCUS,Param1,0);
			}
		}
		break;
		case DN_EDITCHANGE:
		{
			switch (Param1)
			{				
				case SA_FIXEDIT_LAST_ACCESS_DATE:
				case SA_FIXEDIT_LAST_ACCESS_TIME:
					DlgParam->OAccessTime=true;
					break;
				case SA_FIXEDIT_LAST_MODIFICATION_DATE:
				case SA_FIXEDIT_LAST_MODIFICATION_TIME:
					DlgParam->OModifyTime=true;
					break;
				case SA_FIXEDIT_LAST_CHANGE_DATE:
				case SA_FIXEDIT_LAST_CHANGE_TIME:
					DlgParam->OStatusChangeTime=true;
					break;
			}

			break;
		}

		case DN_GOTFOCUS:
			{
				if(Param1 == SA_FIXEDIT_LAST_ACCESS_DATE || Param1 == SA_FIXEDIT_LAST_MODIFICATION_DATE || Param1 == SA_FIXEDIT_LAST_CHANGE_DATE)
				{
					if(GetDateFormat()==2)
					{
						if(reinterpret_cast<LPCWSTR>(SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, Param1, 0))[0] == L' ')
						{
							COORD Pos;
							SendDlgMessage(hDlg, DM_GETCURSORPOS, Param1, (LONG_PTR)&Pos);
							if(Pos.X ==0)
							{
								Pos.X=1;
								SendDlgMessage(hDlg, DM_SETCURSORPOS, Param1, (LONG_PTR)&Pos);
							}
						}
					}
				}
			}
			break;

		case DM_SETATTR:
		{
			FARString strDate,strTime;

			if (Param2) // Set?
			{
				FILETIME ft;

				if (Param2==-1)
				{
					WINPORT(GetSystemTimeAsFileTime)(&ft);
				}
				else
				{
					ft=*reinterpret_cast<PFILETIME>(Param2);
				}

				ConvertDate(ft,strDate,strTime,12,FALSE,FALSE,2,TRUE);
			}

			// Глянем на место, где был клик
			int Set1=-1;
			int Set2=Param1;

			switch (Param1)
			{
				case SA_TEXT_LAST_ACCESS:
					Set1 = SA_FIXEDIT_LAST_ACCESS_DATE;
					Set2 = SA_FIXEDIT_LAST_ACCESS_TIME;
					DlgParam->OAccessTime = true;
					break;
				case SA_TEXT_LAST_MODIFICATION:
					Set1 = SA_FIXEDIT_LAST_MODIFICATION_DATE;
					Set2 = SA_FIXEDIT_LAST_MODIFICATION_TIME;
					DlgParam->OModifyTime = true;
					break;
				case SA_TEXT_LAST_CHANGE:
					Set1 = SA_FIXEDIT_LAST_CHANGE_DATE;
					Set2 = SA_FIXEDIT_LAST_CHANGE_TIME;
					DlgParam->OStatusChangeTime = true;
					break;
					
				case SA_FIXEDIT_LAST_ACCESS_DATE:
				case SA_FIXEDIT_LAST_MODIFICATION_DATE:
				case SA_FIXEDIT_LAST_CHANGE_DATE:
					Set1 = Param1;
					Set2 = -1;
					break;
			}

			if (Set1!=-1)
			{
				SendDlgMessage(hDlg,DM_SETTEXTPTR,Set1,(LONG_PTR)strDate.CPtr());
			}

			if (Set2!=-1)
			{
				SendDlgMessage(hDlg,DM_SETTEXTPTR,Set2,(LONG_PTR)strTime.CPtr());
			}

			return TRUE;
		}
	}

	return DefDlgProc(hDlg,Msg,Param1,Param2);
}

void ShellSetFileAttributesMsg(const wchar_t *Name)
{
	static int Width=54;
	int WidthTemp;

	if (Name && *Name)
		WidthTemp=Max(StrLength(Name),54);
	else
		Width=WidthTemp=54;

	WidthTemp=Min(WidthTemp,WidthNameForMessage);
	Width=Max(Width,WidthTemp);
	FARString strOutFileName=Name;
	TruncPathStr(strOutFileName,Width);
	CenterStr(strOutFileName,strOutFileName,Width+4);
	Message(0,0,Msg::SetAttrTitle,Msg::SetAttrSetting,strOutFileName);
	PreRedrawItem preRedrawItem=PreRedraw.Peek();
	preRedrawItem.Param.Param1=Name;
	PreRedraw.SetParam(preRedrawItem.Param);
}

bool ReadFileTime(int Type,const wchar_t *Name,FILETIME& FileTime,const wchar_t *OSrcDate,const wchar_t *OSrcTime)
{
	bool Result=false;
	FAR_FIND_DATA_EX ffd={};

	if (apiGetFindDataEx(Name, ffd))
	{
		LPFILETIME Times[]={&ffd.ftLastWriteTime, &ffd.ftCreationTime, &ffd.ftLastAccessTime, &ffd.ftChangeTime};
		LPFILETIME OriginalFileTime=Times[Type];
		FILETIME oft={};
		if(WINPORT(FileTimeToLocalFileTime)(OriginalFileTime,&oft))
		{
			SYSTEMTIME ost={};
			if(WINPORT(FileTimeToSystemTime)(&oft,&ost))
			{
				WORD DateN[3]={};
				GetFileDateAndTime(OSrcDate,DateN,ARRAYSIZE(DateN),GetDateSeparator());
				WORD TimeN[4]={};
				GetFileDateAndTime(OSrcTime,TimeN,ARRAYSIZE(TimeN),GetTimeSeparator());
				SYSTEMTIME st={};

				switch (GetDateFormat())
				{
					case 0:
						st.wMonth=DateN[0]!=(WORD)-1?DateN[0]:ost.wMonth;
						st.wDay  =DateN[1]!=(WORD)-1?DateN[1]:ost.wDay;
						st.wYear =DateN[2]!=(WORD)-1?DateN[2]:ost.wYear;
						break;
					case 1:
						st.wDay  =DateN[0]!=(WORD)-1?DateN[0]:ost.wDay;
						st.wMonth=DateN[1]!=(WORD)-1?DateN[1]:ost.wMonth;
						st.wYear =DateN[2]!=(WORD)-1?DateN[2]:ost.wYear;
						break;
					default:
						st.wYear =DateN[0]!=(WORD)-1?DateN[0]:ost.wYear;
						st.wMonth=DateN[1]!=(WORD)-1?DateN[1]:ost.wMonth;
						st.wDay  =DateN[2]!=(WORD)-1?DateN[2]:ost.wDay;
						break;
				}

				st.wHour         = TimeN[0]!=(WORD)-1? (TimeN[0]):ost.wHour;
				st.wMinute       = TimeN[1]!=(WORD)-1? (TimeN[1]):ost.wMinute;
				st.wSecond       = TimeN[2]!=(WORD)-1? (TimeN[2]):ost.wSecond;
				st.wMilliseconds = TimeN[3]!=(WORD)-1? (TimeN[3]):ost.wMilliseconds;

				if (st.wYear<100)
				{
					st.wYear = static_cast<WORD>(ConvertYearToFull(st.wYear));
				}

				FILETIME lft={};
				if (WINPORT(SystemTimeToFileTime)(&st,&lft))
				{
					if(WINPORT(LocalFileTimeToFileTime)(&lft,&FileTime))
					{
						Result=WINPORT(CompareFileTime)(&FileTime,OriginalFileTime)!=0;
					}
				}
			}
		}
	}
	return Result;
}

void PR_ShellSetFileAttributesMsg()
{
	PreRedrawItem preRedrawItem=PreRedraw.Peek();
	ShellSetFileAttributesMsg(reinterpret_cast<const wchar_t*>(preRedrawItem.Param.Param1));
}



static void SystemProperties(const FARString &strSelName)
{
	std::vector<std::wstring> lines;

	std::string cmd = "file \"";
	cmd+= EscapeCmdStr(Wide2MB(strSelName.CPtr()));
	cmd+= '\"';

	if (!POpen(lines, cmd.c_str()))
		return;

	if (lines.empty())
		return;
		
	Messager m(Msg::SetAttrSystemDialog);
	for (const auto &l : lines)
		m.Add(l.c_str());
	m.Add(Msg::HOk);
	m.Show(0, 1);
}

static bool CheckFileOwnerGroup(DialogItemEx &EditItem, bool (WINAPI *GetFN)(const wchar_t *,const wchar_t *, FARString &),
	FARString strComputerName, FARString strSelName)
{
	FARString strCur;
	bool out = true;
	GetFN(strComputerName, strSelName, strCur);
	if (EditItem.strData.IsEmpty()) {
		EditItem.strData = strCur;		
	} else if (!EditItem.strData.Equal(0, strCur)) {
		EditItem.strData = Msg::SetAttrOwnerMultiple;
		out = false;
	}
	return out;
}

static bool ApplyFileOwnerGroupIfChanged(DialogItemEx &EditItem, int (*ESetFN)(LPCWSTR Name, LPCWSTR Owner, int SkipMode), 
	int &SkipMode, const FARString &strSelName, const FARString &strInit, bool force = false) 
{
	if (!EditItem.strData.IsEmpty() && (force || StrCmp(strInit, EditItem.strData))) {
		int Result = ESetFN(strSelName, EditItem.strData, SkipMode);
		if(Result == SETATTR_RET_SKIPALL) {
			SkipMode = SETATTR_RET_SKIP;
		} else if(Result == SETATTR_RET_ERROR) {
			return false;
		}
	}
	return true;
}

bool ShellSetFileAttributes(Panel *SrcPanel,LPCWSTR Object)
{
	SudoClientRegion scr;

	ChangePriority ChPriority(ChangePriority::NORMAL);
	short DlgX=70,DlgY=23;

	DialogDataEx AttrDlgData[]=
	{
		{DI_DOUBLEBOX,3,1,short(DlgX-4),short(DlgY-2),{},0,Msg::SetAttrTitle},
		{DI_TEXT,-1,2,0,2,{},0,Msg::SetAttrFor},
		{DI_TEXT,-1,3,0,3,{},DIF_SHOWAMPERSAND,L""},
		{DI_TEXT,3,4,0,4,{},DIF_SEPARATOR,L""},
		{DI_TEXT,5,5,17,5,{},0,Msg::SetAttrOwner},
		{DI_EDIT,18,5,short(DlgX-6),5,{},0,L""},
		{DI_TEXT,5,6,17,6,{},0,Msg::SetAttrGroup},//L"Group:",
		{DI_EDIT,18,6,short(DlgX-6),6,{},0,L""},
		{DI_TEXT,3,7,0,7,{},DIF_SEPARATOR,L""},
		{DI_TEXT,5,8,0,8,{},0,Msg::SetAttrAccessUser},//L"User",
		{DI_TEXT,short(DlgX/3),8,0,8,{},0,Msg::SetAttrAccessGroup},//L"Group",
		{DI_TEXT,short(2*DlgX/3),8,0,8,{},0,Msg::SetAttrAccessOther},//L"Other",
		{DI_CHECKBOX,5,9,0,9,{},DIF_FOCUS|DIF_3STATE, Msg::SetAttrAccessUserRead},//L"Read",
		{DI_CHECKBOX,5,10,0,10,{},DIF_3STATE, Msg::SetAttrAccessUserWrite},//L"Write",
		{DI_CHECKBOX,5,11,0,11,{},DIF_3STATE, Msg::SetAttrAccessUserExecute},//L"Execute",
		{DI_CHECKBOX,short(DlgX/3),9,0,9,{},DIF_3STATE, Msg::SetAttrAccessGroupRead},//L"Read",
		{DI_CHECKBOX,short(DlgX/3),10,0,10,{},DIF_3STATE, Msg::SetAttrAccessGroupWrite},//L"Write",
		{DI_CHECKBOX,short(DlgX/3),11,0,11,{},DIF_3STATE, Msg::SetAttrAccessGroupExecute},//L"Execute",
		{DI_CHECKBOX,short(2*DlgX/3),9,0,9,{},DIF_3STATE, Msg::SetAttrAccessOtherRead},//L"Read",
		{DI_CHECKBOX,short(2*DlgX/3),10,0,10,{},DIF_3STATE, Msg::SetAttrAccessOtherWrite},//L"Write",
		{DI_CHECKBOX,short(2*DlgX/3),11,0,11,{},DIF_3STATE, Msg::SetAttrAccessOtherExecute},//L"Execute",
		{DI_TEXT,3,12,0,12,{},DIF_SEPARATOR,L""},
		{DI_TEXT,short(DlgX-29),13,0,13,{},0,L""},
		{DI_TEXT,    5,14,0,14,{},0, Msg::SetAttrAccessTime},//L"Last access time",
		{DI_FIXEDIT,short(DlgX-29),14,short(DlgX-19),14,{},DIF_MASKEDIT,L""},
		{DI_FIXEDIT,short(DlgX-17),14,short(DlgX-6),14,{},DIF_MASKEDIT,L""},
		{DI_TEXT,    5,15,0,15,{},0, Msg::SetAttrModificationTime},//L"Last modification time",
		{DI_FIXEDIT,short(DlgX-29),15,short(DlgX-19),15,{},DIF_MASKEDIT,L""},
		{DI_FIXEDIT,short(DlgX-17),15,short(DlgX-6),15,{},DIF_MASKEDIT,L""},
		{DI_TEXT,    5,16,0,16,{},0, Msg::SetAttrStatusChangeTime},//L"Last status change time",
		{DI_FIXEDIT,short(DlgX-29),16,short(DlgX-19),16,{},DIF_MASKEDIT|DIF_READONLY,L""},
		{DI_FIXEDIT,short(DlgX-17),16,short(DlgX-6),16,{},DIF_MASKEDIT|DIF_READONLY,L""},
		{DI_BUTTON,0,18,0,18,{},DIF_CENTERGROUP|DIF_BTNNOCLOSE,Msg::SetAttrOriginal},
		{DI_BUTTON,0,18,0,18,{},DIF_CENTERGROUP|DIF_BTNNOCLOSE,Msg::SetAttrCurrent},
		{DI_BUTTON,0,18,0,18,{},DIF_CENTERGROUP|DIF_BTNNOCLOSE,Msg::SetAttrBlank},
		{DI_TEXT,3,19,0,19,{},DIF_SEPARATOR|DIF_HIDDEN,L""},
		{DI_CHECKBOX,5,20,0,20,{},DIF_DISABLE|DIF_HIDDEN,Msg::SetAttrSubfolders},
		{DI_TEXT,3,short(DlgY-4),0,short(DlgY-4),{},DIF_SEPARATOR,L""},
		{DI_BUTTON,0,short(DlgY-3),0,short(DlgY-3),{},DIF_DEFAULT|DIF_CENTERGROUP,Msg::SetAttrSet},
		{DI_BUTTON,0,short(DlgY-3),0,short(DlgY-3),{},DIF_CENTERGROUP|DIF_DISABLE,Msg::SetAttrSystemDialog},
		{DI_BUTTON,0,short(DlgY-3),0,short(DlgY-3),{},DIF_CENTERGROUP,Msg::Cancel}
	};
	MakeDialogItemsEx(AttrDlgData,AttrDlg);
	SetAttrDlgParam DlgParam{};
	int SelCount=SrcPanel?SrcPanel->GetSelCount():1;

	if (!SelCount)
	{
		return false;
	}

	if(SelCount==1)
	{
		AttrDlg[SA_BUTTON_SYSTEMDLG].Flags&=~DIF_DISABLE;
	}

	if (SrcPanel && SrcPanel->GetMode()==PLUGIN_PANEL)
	{
		OpenPluginInfo Info;
		HANDLE hPlugin=SrcPanel->GetPluginHandle();

		if (hPlugin == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		CtrlObject->Plugins.GetOpenPluginInfo(hPlugin,&Info);

		if (!(Info.Flags & OPIF_REALNAMES))
		{
			AttrDlg[SA_BUTTON_SET].Flags|=DIF_DISABLE;
			AttrDlg[SA_BUTTON_SYSTEMDLG].Flags|=DIF_DISABLE;
			DlgParam.Plugin=true;
		}
	}

	FarList NameList{};
	FARString *strLinks=nullptr;

	{
		DWORD FileAttr = INVALID_FILE_ATTRIBUTES, FileMode = 0;
		FARString strSelName;
		FAR_FIND_DATA_EX FindData;
		if(SrcPanel)
		{
			SrcPanel->GetSelName(nullptr,FileAttr, FileMode);
			SrcPanel->GetSelName(&strSelName,FileAttr, FileMode,&FindData);
/*			if (!FileMode) {
				FAR_FIND_DATA_EX FindData2;
				if (apiGetFindDataEx(strSelName, FindData2) && FindData2.dwUnixMode) {
					FindData = FindData2;
					FileAttr = FindData.dwFileAttributes;
					FileMode = FindData.dwUnixMode;
				}
			}*/
		}
		else
		{
			strSelName=Object;
			apiGetFindDataEx(Object, FindData);
			FileAttr=FindData.dwFileAttributes;
			FileMode=FindData.dwUnixMode;
		}
		fprintf(stderr, "FileMode=%u\n", FileMode);

		if (SelCount==1 && TestParentFolderName(strSelName))
			return false;

		wchar_t DateSeparator=GetDateSeparator();
		wchar_t TimeSeparator=GetTimeSeparator();
		wchar_t DecimalSeparator=GetDecimalSeparator();
		LPCWSTR FmtMask1=L"99%c99%c99%c999",FmtMask2=L"99%c99%c99999",FmtMask3=L"99999%c99%c99";
		FARString strDMask, strTMask;
		strTMask.Format(FmtMask1,TimeSeparator,TimeSeparator,DecimalSeparator);

		switch (GetDateFormat())
		{
			case 0:
				AttrDlg[SA_TEXT_TITLEDATE].strData.Format(Msg::SetAttrTimeTitle1,DateSeparator,DateSeparator,TimeSeparator,TimeSeparator,DecimalSeparator);
				strDMask.Format(FmtMask2,DateSeparator,DateSeparator);
				break;
			case 1:
				AttrDlg[SA_TEXT_TITLEDATE].strData.Format(Msg::SetAttrTimeTitle2,DateSeparator,DateSeparator,TimeSeparator,TimeSeparator,DecimalSeparator);
				strDMask.Format(FmtMask2,DateSeparator,DateSeparator);
				break;
			default:
				AttrDlg[SA_TEXT_TITLEDATE].strData.Format(Msg::SetAttrTimeTitle3,DateSeparator,DateSeparator,TimeSeparator,TimeSeparator,DecimalSeparator);
				strDMask.Format(FmtMask3,DateSeparator,DateSeparator);
				break;
		}

		AttrDlg[SA_FIXEDIT_LAST_ACCESS_DATE].strMask = 
			AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_DATE].strMask = AttrDlg[SA_FIXEDIT_LAST_CHANGE_DATE].strMask = strDMask;
			
		AttrDlg[SA_FIXEDIT_LAST_ACCESS_TIME].strMask = 
			AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_TIME].strMask = AttrDlg[SA_FIXEDIT_LAST_CHANGE_TIME].strMask = strTMask;
		bool FolderPresent=false,LinkPresent=false;
		FARString strLinkName;
		static struct MODEPAIR
		{
			SETATTRDLG Item;
			DWORD Mode;
		}
		AP[]=
		{
			{SA_CHECKBOX_USER_READ, S_IRUSR },
			{SA_CHECKBOX_USER_WRITE, S_IWUSR },
			{SA_CHECKBOX_USER_EXECUTE, S_IXUSR },
			{SA_CHECKBOX_GROUP_READ, S_IRGRP },
			{SA_CHECKBOX_GROUP_WRITE, S_IWGRP },
			{SA_CHECKBOX_GROUP_EXECUTE, S_IXGRP },
			{SA_CHECKBOX_OTHER_READ, S_IROTH },
			{SA_CHECKBOX_OTHER_WRITE, S_IWOTH },
			{SA_CHECKBOX_OTHER_EXECUTE, S_IXOTH },
		};

		if (SelCount==1)
		{
			if (FileAttr&FILE_ATTRIBUTE_DIRECTORY)
			{
				if (!DlgParam.Plugin)
				{
					FileAttr=apiGetFileAttributes(strSelName);
				}

				//_SVS(SysLog(L"SelName=%ls  FileAttr=0x%08X",SelName,FileAttr));
				AttrDlg[SA_SEPARATOR4].Flags&=~DIF_HIDDEN;
				AttrDlg[SA_CHECKBOX_SUBFOLDERS].Flags&=~(DIF_DISABLE|DIF_HIDDEN);
				AttrDlg[SA_CHECKBOX_SUBFOLDERS].Selected=Opt.SetAttrFolderRules?BSTATE_UNCHECKED:BSTATE_CHECKED;
				AttrDlg[SA_DOUBLEBOX].Y2+=2;
				for(int i=SA_SEPARATOR5;i<=SA_BUTTON_CANCEL;i++)
				{
					AttrDlg[i].Y1+=2;
					AttrDlg[i].Y2+=2;
				}
				DlgY+=2;

				if (Opt.SetAttrFolderRules)
				{
					if (DlgParam.Plugin || apiGetFindDataEx(strSelName, FindData))
					{
						ConvertDate(FindData.ftUnixAccessTime, AttrDlg[SA_FIXEDIT_LAST_ACCESS_DATE].strData, 
							AttrDlg[SA_FIXEDIT_LAST_ACCESS_TIME].strData,12,FALSE,FALSE,2,TRUE);
						ConvertDate(FindData.ftUnixModificationTime,  AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_DATE].strData,
							AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_TIME].strData,12,FALSE,FALSE,2,TRUE);
						ConvertDate(FindData.ftUnixStatusChangeTime,AttrDlg[SA_FIXEDIT_LAST_CHANGE_DATE].strData,
							AttrDlg[SA_FIXEDIT_LAST_CHANGE_TIME].strData,12,FALSE,FALSE,2,TRUE);
					}

					if (FileAttr!=INVALID_FILE_ATTRIBUTES)
					{
						for (size_t i=0; i<ARRAYSIZE(AP); i++)
						{
							AttrDlg[AP[i].Item].Selected = (FileMode&AP[i].Mode) ? BSTATE_CHECKED : BSTATE_UNCHECKED;
						}
					}

					for (size_t i=SA_ATTR_FIRST; i<= SA_ATTR_LAST; i++)
					{
						AttrDlg[i].Flags&=~DIF_3STATE;
					}
				}

				FolderPresent=TRUE;
			}
			else
			{
				for (size_t i=SA_ATTR_FIRST; i<=SA_ATTR_LAST; i++)
				{
					AttrDlg[i].Flags&=~DIF_3STATE;
				}
			}

			/* обработка случая, если ЭТО SymLink
			if (FileAttr!=INVALID_FILE_ATTRIBUTES && FileAttr&FILE_ATTRIBUTE_REPARSE_POINT)
			{
				DWORD ReparseTag=0;
				DWORD LenJunction=DlgParam.Plugin?0:GetReparsePointInfo(strSelName, strLinkName,&ReparseTag);
				AttrDlg[SA_DOUBLEBOX].Y2++;

				for (size_t i=SA_TEXT_SYMLINK; i<ARRAYSIZE(AttrDlgData); i++)
				{
					AttrDlg[i].Y1++;

					if (AttrDlg[i].Y2)
					{
						AttrDlg[i].Y2++;
					}
				}

				LinkPresent=true;
				NormalizeSymlinkName(strLinkName);
				int ID_Msg=Msg::SetAttrSymlink;

				if (ReparseTag==IO_REPARSE_TAG_MOUNT_POINT)
				{
					if (IsLocalVolumeRootPath(strLinkName))
					{
						ID_Msg=Msg::SetAttrVolMount;
					}
					else
					{
						ID_Msg=Msg::SetAttrJunction;
					}
				}

				AttrDlg[SA_TEXT_SYMLINK].Flags&=~DIF_HIDDEN;
				AttrDlg[SA_TEXT_SYMLINK].strData=(ID_Msg);
				AttrDlg[SA_EDIT_SYMLINK].Flags&=~DIF_HIDDEN;
				AttrDlg[SA_EDIT_SYMLINK].strData=LenJunction?strLinkName.CPtr():Msg::SetAttrUnknownJunction;
				DlgParam.FileSystemFlags=0;

				if (apiGetVolumeInformation(strSelName,nullptr,0,nullptr,&DlgParam.FileSystemFlags,nullptr))
				{
					if (!(DlgParam.FileSystemFlags&FILE_FILE_COMPRESSION))
					{
						AttrDlg[SA_CHECKBOX_COMPRESSED].Flags|=DIF_DISABLE;
					}

					if (!(DlgParam.FileSystemFlags&FILE_SUPPORTS_ENCRYPTION))
					{
						AttrDlg[SA_CHECKBOX_ENCRYPTED].Flags|=DIF_DISABLE;
					}

					if (!(DlgParam.FileSystemFlags&FILE_SUPPORTS_SPARSE_FILES))
					{
						AttrDlg[SA_CHECKBOX_SPARSE].Flags|=DIF_DISABLE;
					}
				}
			}*/

			NameList.ItemsNumber=1;
			// обработка случая "несколько хардлинков"
			/*NameList.ItemsNumber=(FileAttr&FILE_ATTRIBUTE_DIRECTORY)?1:GetNumberOfLinks(strSelName);

			if (NameList.ItemsNumber>1)
			{
				AttrDlg[SA_TEXT_NAME].Flags|=DIF_HIDDEN;
				AttrDlg[SA_COMBO_HARDLINK].Flags&=~DIF_HIDDEN;
				NameList.Items=new FarListItem[NameList.ItemsNumber]();
				strLinks=new FARString [NameList.ItemsNumber];
				int Current=0;
				AttrDlg[SA_COMBO_HARDLINK].Flags|=DIF_DISABLE;

				FormatString strTmp;
				strTmp<<Msg::SetAttrHardLinks<<L" ("<<NameList.ItemsNumber<<L")";
				AttrDlg[SA_COMBO_HARDLINK].strData=strTmp;
			}*/

			AttrDlg[SA_TEXT_NAME].strData = strSelName;
			TruncStr(AttrDlg[SA_TEXT_NAME].strData,DlgX-10);

			if (FileAttr!=INVALID_FILE_ATTRIBUTES)
			{
				for (size_t i=0; i<ARRAYSIZE(AP); i++)
				{
					AttrDlg[AP[i].Item].Selected = FileMode&AP[i].Mode ? BSTATE_CHECKED : BSTATE_UNCHECKED;
				}
			}

			const SETATTRDLG Dates[]={SA_FIXEDIT_LAST_ACCESS_DATE, SA_FIXEDIT_LAST_MODIFICATION_DATE, SA_FIXEDIT_LAST_CHANGE_DATE};
			const SETATTRDLG Times[]={SA_FIXEDIT_LAST_ACCESS_TIME, SA_FIXEDIT_LAST_MODIFICATION_TIME, SA_FIXEDIT_LAST_CHANGE_TIME};
			const PFILETIME TimeValues[]={&FindData.ftUnixAccessTime, &FindData.ftUnixModificationTime, &FindData.ftUnixStatusChangeTime};

			if (DlgParam.Plugin || (!DlgParam.Plugin&&apiGetFindDataEx(strSelName, FindData)))
			{
				for (size_t i=0; i<ARRAYSIZE(Dates); i++)
				{
					ConvertDate(*TimeValues[i],AttrDlg[Dates[i]].strData,AttrDlg[Times[i]].strData,12,FALSE,FALSE,2,TRUE);
				}
			}

			FARString strComputerName;
			if(SrcPanel)
			{
				FARString strCurDir;
				SrcPanel->GetCurDir(strCurDir);
			}

			GetFileOwner(strComputerName,strSelName,AttrDlg[SA_EDIT_OWNER].strData);
			GetFileGroup(strComputerName,strSelName,AttrDlg[SA_EDIT_GROUP].strData);
		}
		else
		{
			for (size_t i=0; i<ARRAYSIZE(AP); i++)
			{
				AttrDlg[AP[i].Item].Selected=BSTATE_3STATE;
			}

			AttrDlg[SA_FIXEDIT_LAST_ACCESS_DATE].strData.Clear();
			AttrDlg[SA_FIXEDIT_LAST_ACCESS_TIME].strData.Clear();
			AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_DATE].strData.Clear();
			AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_TIME].strData.Clear();
			AttrDlg[SA_FIXEDIT_LAST_CHANGE_DATE].strData.Clear();
			AttrDlg[SA_FIXEDIT_LAST_CHANGE_TIME].strData.Clear();
			AttrDlg[SA_BUTTON_ORIGINAL].Flags|=DIF_DISABLE;
			AttrDlg[SA_TEXT_NAME].strData = Msg::SetAttrSelectedObjects;

			for (size_t i=SA_ATTR_FIRST; i<=SA_ATTR_LAST; i++)
			{
				AttrDlg[i].Selected=BSTATE_UNCHECKED;
			}

			// проверка - есть ли среди выделенных - каталоги?
			// так же проверка на атрибуты
			if(SrcPanel)
			{
				SrcPanel->GetSelName(nullptr, FileAttr, FileMode);
			}
			FolderPresent=false;

			if(SrcPanel)
			{
				FARString strComputerName;
				FARString strCurDir;
				SrcPanel->GetCurDir(strCurDir);

				bool CheckOwner = true, CheckGroup = true;
				while (SrcPanel->GetSelName(&strSelName, FileAttr, FileMode, &FindData))
				{
					if (!FolderPresent&&(FileAttr&FILE_ATTRIBUTE_DIRECTORY))
					{
						FolderPresent=true;
						AttrDlg[SA_SEPARATOR4].Flags&=~DIF_HIDDEN;
						AttrDlg[SA_CHECKBOX_SUBFOLDERS].Flags&=~(DIF_DISABLE|DIF_HIDDEN);
						AttrDlg[SA_DOUBLEBOX].Y2+=2;
						for(int i=SA_SEPARATOR5;i<=SA_BUTTON_CANCEL;i++)
						{
							AttrDlg[i].Y1+=2;
							AttrDlg[i].Y2+=2;
						}
						DlgY+=2;
					}

					for (size_t i=0; i<ARRAYSIZE(AP); i++)
					{
						if (FileMode & AP[i].Mode)
						{
							AttrDlg[AP[i].Item].Selected++;
						}
					}
					if(CheckOwner) CheckFileOwnerGroup(AttrDlg[SA_EDIT_OWNER], GetFileOwner, strComputerName, strSelName);
					if(CheckGroup) CheckFileOwnerGroup(AttrDlg[SA_EDIT_GROUP], GetFileGroup, strComputerName, strSelName);
				}
			}
			else
			{
				// BUGBUG, copy-paste
				if (!FolderPresent&&(FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
				{
					FolderPresent=true;
					AttrDlg[SA_SEPARATOR4].Flags&=~DIF_HIDDEN;
					AttrDlg[SA_CHECKBOX_SUBFOLDERS].Flags&=~(DIF_DISABLE|DIF_HIDDEN);
					AttrDlg[SA_DOUBLEBOX].Y2+=2;
					for(int i=SA_SEPARATOR5;i<=SA_BUTTON_CANCEL;i++)
					{
						AttrDlg[i].Y1+=2;
						AttrDlg[i].Y2+=2;
					}
					DlgY+=2;
				}
				for (size_t i=0; i<ARRAYSIZE(AP); i++)
				{
					if (FindData.dwUnixMode & AP[i].Mode)
					{
						AttrDlg[AP[i].Item].Selected++;
					}
				}
			}
			if(SrcPanel)
			{
				SrcPanel->GetSelName(nullptr, FileAttr, FileMode);
				SrcPanel->GetSelName(&strSelName, FileAttr, FileMode,  &FindData);
			}

			// выставим "неопределенку" или то, что нужно
			for (size_t i=SA_ATTR_FIRST; i<=SA_ATTR_LAST; i++)
			{
				// снимаем 3-state, если "есть все или нет ничего"
				// за исключением случая, если есть Фолдер среди объектов
				if ((!AttrDlg[i].Selected || AttrDlg[i].Selected >= SelCount) && !FolderPresent)
				{
					AttrDlg[i].Flags&=~DIF_3STATE;
				}

				AttrDlg[i].Selected=(AttrDlg[i].Selected >= SelCount)?BST_CHECKED:(!AttrDlg[i].Selected?BSTATE_UNCHECKED:BSTATE_3STATE);
			}
		}

		// поведение для каталогов как у 1.65?
		if (FolderPresent && !Opt.SetAttrFolderRules)
		{
			AttrDlg[SA_CHECKBOX_SUBFOLDERS].Selected=BSTATE_CHECKED;
			AttrDlg[SA_FIXEDIT_LAST_ACCESS_DATE].strData.Clear();
			AttrDlg[SA_FIXEDIT_LAST_ACCESS_TIME].strData.Clear();
			AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_DATE].strData.Clear();
			AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_TIME].strData.Clear();
			AttrDlg[SA_FIXEDIT_LAST_CHANGE_DATE].strData.Clear();
			AttrDlg[SA_FIXEDIT_LAST_CHANGE_TIME].strData.Clear();

			for (size_t i=SA_ATTR_FIRST; i<= SA_ATTR_LAST; i++)
			{
				AttrDlg[i].Selected=BSTATE_3STATE;
				AttrDlg[i].Flags|=DIF_3STATE;
			}
		}

		// запомним состояние переключателей.
		for (size_t i=SA_ATTR_FIRST; i<=SA_ATTR_LAST; i++)
		{
			DlgParam.OriginalCBAttr[i-SA_ATTR_FIRST]=AttrDlg[i].Selected;
			DlgParam.OriginalCBAttr2[i-SA_ATTR_FIRST]=-1;
			DlgParam.OriginalCBFlag[i-SA_ATTR_FIRST]=AttrDlg[i].Flags;
		}

		DlgParam.strOwner = AttrDlg[SA_EDIT_OWNER].strData;
		DlgParam.strGroup = AttrDlg[SA_EDIT_GROUP].strData;
		FARString strInitOwner = DlgParam.strOwner, strInitGroup = DlgParam.strGroup;

		DlgParam.DialogMode=((SelCount==1&&!(FileAttr&FILE_ATTRIBUTE_DIRECTORY))?MODE_FILE:(SelCount==1?MODE_FOLDER:MODE_MULTIPLE));
		DlgParam.strSelName=strSelName;
		DlgParam.OSubfoldersState=static_cast<FARCHECKEDSTATE>(AttrDlg[SA_CHECKBOX_SUBFOLDERS].Selected);

		Dialog Dlg(AttrDlg,ARRAYSIZE(AttrDlgData),SetAttrDlgProc,(LONG_PTR)&DlgParam);
		Dlg.SetHelp(L"FileAttrDlg");                 //  ^ - это одиночный диалог!
		Dlg.SetId(FileAttrDlgId);

		if (LinkPresent)
		{
			DlgY++;
		}

		Dlg.SetPosition(-1,-1,DlgX,DlgY);
		Dlg.Process();

		if (NameList.Items)
			delete[] NameList.Items;

		if (strLinks)
			delete[] strLinks;

		switch(Dlg.GetExitCode())
		{
		case SA_BUTTON_SET:
			{
				const size_t Times[]={SA_FIXEDIT_LAST_ACCESS_TIME, SA_FIXEDIT_LAST_MODIFICATION_TIME, SA_FIXEDIT_LAST_CHANGE_TIME};

				for (size_t i=0; i<ARRAYSIZE(Times); i++)
				{
					LPWSTR TimePtr=AttrDlg[Times[i]].strData.GetBuffer();
					TimePtr[8]=GetTimeSeparator();
					AttrDlg[Times[i]].strData.ReleaseBuffer(AttrDlg[Times[i]].strData.GetLength());
				}

				TPreRedrawFuncGuard preRedrawFuncGuard(PR_ShellSetFileAttributesMsg);
				ShellSetFileAttributesMsg(SelCount==1?strSelName.CPtr():nullptr);
				int SkipMode=-1;

				if (SelCount==1 && !(FileAttr & FILE_ATTRIBUTE_DIRECTORY))
				{
					DWORD NewMode = 0;

					for (size_t i=0; i<ARRAYSIZE(AP); i++)
					{
						if (AttrDlg[AP[i].Item].Selected)
						{
							NewMode|= AP[i].Mode;
						}
					}

					if (!ApplyFileOwnerGroupIfChanged(AttrDlg[SA_EDIT_OWNER], ESetFileOwner, SkipMode, strSelName, strInitOwner)) break;
					if (!ApplyFileOwnerGroupIfChanged(AttrDlg[SA_EDIT_GROUP], ESetFileGroup, SkipMode, strSelName, strInitGroup)) break;
					
					FILETIME UnixAccessTime={},UnixModificationTime={};
					int SetAccessTime = DlgParam.OAccessTime  && 
						ReadFileTime(0, strSelName, UnixAccessTime,AttrDlg[SA_FIXEDIT_LAST_ACCESS_DATE].strData,AttrDlg[SA_FIXEDIT_LAST_ACCESS_TIME].strData);
					int SetModifyTime = DlgParam.OModifyTime   && 
						ReadFileTime(1,strSelName,UnixModificationTime,AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_DATE].strData,AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_TIME].strData);

					//_SVS(SysLog(L"\n\tSetWriteTime=%d\n\tSetCreationTime=%d\n\tSetLastAccessTime=%d",SetWriteTime,SetCreationTime,SetLastAccessTime));

					if (SetAccessTime || SetModifyTime)
					{
						if(ESetFileTime(strSelName, SetAccessTime? &UnixAccessTime : nullptr,
							SetModifyTime ? &UnixModificationTime : nullptr, FileAttr, SkipMode)==SETATTR_RET_SKIPALL)
						{
							SkipMode=SETATTR_RET_SKIP;
						}
					}

					if (FileMode != NewMode)
					{
						if (ESetFileMode(strSelName, NewMode, SkipMode)==SETATTR_RET_SKIPALL)
						{
							SkipMode=SETATTR_RET_SKIP;
						}
					}
				}
				/* Multi *********************************************************** */
				else
				{
					int RetCode=1;
					ConsoleTitle SetAttrTitle(Msg::SetAttrTitle);
					if(SrcPanel)
					{
						CtrlObject->Cp()->GetAnotherPanel(SrcPanel)->CloseFile();
					}
					DWORD SetMode = 0, ClearMode = 0;

					for (size_t i=0; i<ARRAYSIZE(AP); i++)
					{
						switch (AttrDlg[AP[i].Item].Selected)
						{
							case BSTATE_CHECKED:
								SetMode|= AP[i].Mode;
								break;
							case BSTATE_UNCHECKED:
								ClearMode|= AP[i].Mode;
								break;
						}
					}

					if(SrcPanel)
					{
						SrcPanel->GetSelName(nullptr, FileAttr, FileMode);
					}
					wakeful W;
					bool Cancel=false;
					DWORD LastTime=0;

					bool SingleFileDone=false;
					while ((SrcPanel?SrcPanel->GetSelName(&strSelName, FileAttr, FileMode,  &FindData):!SingleFileDone) && !Cancel)
					{
						if(!SrcPanel)
						{
							SingleFileDone=true;
						}
		//_SVS(SysLog(L"SelName='%ls'\n\tFileAttr =0x%08X\n\tSetAttr  =0x%08X\n\tClearAttr=0x%08X\n\tResult   =0x%08X",
		//    SelName,FileAttr,SetAttr,ClearAttr,((FileAttr|SetAttr)&(~ClearAttr))));
						DWORD CurTime=WINPORT(GetTickCount)();

						if (CurTime-LastTime>RedrawTimeout)
						{
							LastTime=CurTime;
							ShellSetFileAttributesMsg(strSelName);

							if (CheckForEsc())
								break;
						}

						if (!ApplyFileOwnerGroupIfChanged(AttrDlg[SA_EDIT_OWNER], ESetFileOwner, SkipMode, strSelName, strInitOwner)) break;
						if (!ApplyFileOwnerGroupIfChanged(AttrDlg[SA_EDIT_GROUP], ESetFileGroup, SkipMode, strSelName, strInitGroup)) break;
	
						FILETIME UnixAccessTime = {}, UnixModificationTime = {};
						int SetAccessTime = DlgParam.OAccessTime  && 
							ReadFileTime(0,strSelName,UnixAccessTime,AttrDlg[SA_FIXEDIT_LAST_ACCESS_DATE].strData,AttrDlg[SA_FIXEDIT_LAST_ACCESS_TIME].strData);
						int SetModifyTime = DlgParam.OModifyTime   && 
							ReadFileTime(1,strSelName,UnixModificationTime,AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_DATE].strData,AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_TIME].strData);

						RetCode = ESetFileTime(strSelName, SetAccessTime ? &UnixAccessTime : nullptr,
							SetModifyTime ? &UnixModificationTime : nullptr, FileAttr,SkipMode);

						if (RetCode == SETATTR_RET_ERROR)
							break;
						else if (RetCode == SETATTR_RET_SKIP)
							continue;
						else if (RetCode == SETATTR_RET_SKIPALL)
						{
							SkipMode=SETATTR_RET_SKIP;
							continue;
						}

						if(FileAttr!=INVALID_FILE_ATTRIBUTES)
						{
							if (((FileMode|SetMode)&~ClearMode) != FileMode)
							{

								RetCode = ESetFileMode(strSelName,((FileMode|SetMode)&(~ClearMode)),SkipMode);

								if (RetCode == SETATTR_RET_ERROR)
									break;
								else if (RetCode == SETATTR_RET_SKIP)
									continue;
								else if (RetCode == SETATTR_RET_SKIPALL)
								{
									SkipMode=SETATTR_RET_SKIP;
									continue;
								}
							}

							if ((FileAttr & FILE_ATTRIBUTE_DIRECTORY) && AttrDlg[SA_CHECKBOX_SUBFOLDERS].Selected)
							{
								ScanTree ScTree(FALSE);
								ScTree.SetFindPath(strSelName,L"*");
								DWORD LastTime=WINPORT(GetTickCount)();
								FARString strFullName;

								while (ScTree.GetNextName(&FindData,strFullName))
								{
									DWORD CurTime=WINPORT(GetTickCount)();

									if (CurTime-LastTime>RedrawTimeout)
									{
										LastTime=CurTime;
										ShellSetFileAttributesMsg(strFullName);

										if (CheckForEsc())
										{
											Cancel=true;
											break;
										}
									}

									if (!ApplyFileOwnerGroupIfChanged(AttrDlg[SA_EDIT_OWNER], ESetFileOwner, 
										SkipMode, strFullName, strInitOwner, DlgParam.OSubfoldersState)) break;
									if (!ApplyFileOwnerGroupIfChanged(AttrDlg[SA_EDIT_GROUP], ESetFileGroup, 
										SkipMode, strFullName, strInitGroup, DlgParam.OSubfoldersState)) break;									

									SetAccessTime=     DlgParam.OAccessTime  && 
										ReadFileTime(0,strFullName,UnixAccessTime,AttrDlg[SA_FIXEDIT_LAST_ACCESS_DATE].strData,AttrDlg[SA_FIXEDIT_LAST_ACCESS_TIME].strData);
									SetModifyTime=  DlgParam.OModifyTime   && 
										ReadFileTime(1,strFullName,UnixModificationTime,AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_DATE].strData,AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_TIME].strData);
									
									if (SetAccessTime || SetModifyTime)
									{
										RetCode = ESetFileTime(strFullName, SetAccessTime ? &UnixAccessTime : nullptr,
											SetModifyTime ? &UnixModificationTime : nullptr, FindData.dwFileAttributes,SkipMode);

										if (RetCode == SETATTR_RET_ERROR)
										{
											Cancel=true;
											break;
										}
										else if (RetCode == SETATTR_RET_SKIP)
											continue;
										else if (RetCode == SETATTR_RET_SKIPALL)
										{
											SkipMode=SETATTR_RET_SKIP;
											continue;
										}
									}

									if (((FindData.dwUnixMode|SetMode)&(~ClearMode)) !=
													FindData.dwUnixMode)
									{
										RetCode = ESetFileMode(strFullName,(FindData.dwUnixMode|SetMode)&(~ClearMode),SkipMode);

										if (RetCode == SETATTR_RET_ERROR)
										{
											Cancel=true;
											break;
										}
										else if (RetCode == SETATTR_RET_SKIP)
											continue;
										else if (RetCode == SETATTR_RET_SKIPALL)
										{
											SkipMode=SETATTR_RET_SKIP;
											continue;
										}
									}
								}
							}
						}
					} // END: while (SrcPanel->GetSelNameCompat(...))
				}
			}
			break;
		case SA_BUTTON_SYSTEMDLG:
			{
				SystemProperties(strSelName);
			}
			break;
		default:
			return false;
		}
	}

	if(SrcPanel)
	{
		SrcPanel->SaveSelection();
		SrcPanel->Update(UPDATE_KEEP_SELECTION);
		SrcPanel->ClearSelection();
		CtrlObject->Cp()->GetAnotherPanel(SrcPanel)->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);
	}
	CtrlObject->Cp()->Redraw();
	return true;
}
