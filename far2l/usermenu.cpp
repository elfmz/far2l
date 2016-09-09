/*
usermenu.cpp

User menu и есть
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
#pragma hdrstop

#include "lang.hpp"
#include "keys.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "cmdline.hpp"
#include "vmenu.hpp"
#include "dialog.hpp"
#include "fileedit.hpp"
#include "plognmn.hpp"
#include "savefpos.hpp"
#include "ctrlobj.hpp"
#include "manager.hpp"
#include "constitle.hpp"
#include "registry.hpp"
#include "message.hpp"
#include "usermenu.hpp"
#include "filetype.hpp"
#include "fnparce.hpp"
#include "execute.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "panelmix.hpp"
#include "filestr.hpp"
#include "mix.hpp"
#include "savescr.hpp"
#include "syslog.hpp"
#include "interf.hpp"
#include "cache.hpp"

#if defined(PROJECT_DI_MEMOEDIT)
/*
  Идея в следующем.
  1. Строки в реестре храняться как и раньше, т.к. CommandXXX
  2. Для DI_MEMOEDIT мы из только преобразовываем в один массив
*/
#endif

// Коды выхода из меню (Exit codes)
enum
{
	EC_CLOSE_LEVEL      = -1, // Выйти из меню на один уровень вверх
	EC_CLOSE_MENU       = -2, // Выйти из меню по SHIFT+F10
	EC_PARENT_MENU      = -3, // Показать меню родительского каталога
	EC_MAIN_MENU        = -4, // Показать главное меню
	EC_COMMAND_SELECTED = -5, // Выбрана команда - закрыть меню и обновить папку
};

int PrepareHotKey(FARString &strHotKey)
{
	int FuncNum=0;

	if (strHotKey.GetLength() > 1)
	{
		// если хоткей больше 1 символа, считаем это случаем "F?", причем при кривизне всегда будет "F1"
		FuncNum=_wtoi(strHotKey.CPtr()+1);

		if (FuncNum < 1 || FuncNum > 24)
		{
			FuncNum=1;
			strHotKey=L"F1";
		}
	}
	else
	{
		// при наличии "&" продублируем
		if (strHotKey.At(0) == L'&')
			strHotKey += L"&";
	}

	return FuncNum;
}

void MenuRegToFile(const wchar_t *MenuKey, File& MenuFile, CachedWrite& CW, bool SingleItemMenu=false)
{
	for (int i=0;;i++)
	{
		FARString strItemKey;
		strItemKey.Format(L"%ls/Item%d",MenuKey,i);
		FARString strLabel;

		if (!GetRegKey(strItemKey,L"Label",strLabel,L""))
			break;

		FARString strHotKey;
		GetRegKey(strItemKey,L"HotKey",strHotKey,L"");
		BOOL SubMenu;
		GetRegKey(strItemKey,L"Submenu",SubMenu,0);
		CW.Write(strHotKey.CPtr(), static_cast<DWORD>(strHotKey.GetLength()*sizeof(WCHAR)));
		CW.Write(L":  ", 3*sizeof(WCHAR));
		CW.Write(strLabel.CPtr(), static_cast<DWORD>(strLabel.GetLength()*sizeof(WCHAR)));
		CW.Write(L"\r\n", 2*sizeof(WCHAR));

		if (SubMenu)
		{
			CW.Write(L"{\r\n", 3*sizeof(WCHAR));
			MenuRegToFile(strItemKey, MenuFile, CW, false);
			CW.Write(L"}\r\n", 3*sizeof(WCHAR));
		}
		else
		{
			for (int i=0;; i++)
			{
				FARString strLineName;
				strLineName.Format(L"Command%d",i);
				FARString strCommand;

				if (!GetRegKey(strItemKey,strLineName,strCommand,L""))
					break;
				CW.Write(L"    ", 4*sizeof(WCHAR));
				CW.Write(strCommand.CPtr(), static_cast<DWORD>(strCommand.GetLength()*sizeof(WCHAR)));
				CW.Write(L"\r\n", 2*sizeof(WCHAR));
			}
		}
	}
}

void MenuFileToReg(const wchar_t *MenuKey, File& MenuFile, GetFileString& GetStr, bool SingleItemMenu = false, UINT MenuCP = CP_UNICODE)
{
	INT64 Pos = 0;
	MenuFile.GetPointer(Pos);
	if(!Pos)
	{
		if (!GetFileFormat(MenuFile,MenuCP))
			MenuCP=CP_OEMCP;
	}

	LPWSTR MenuStr = nullptr;
	int MenuStrLength = 0;
	int KeyNumber=-1,CommandNumber=0;

	while(GetStr.GetString(&MenuStr, MenuCP, MenuStrLength))
	{
		FARString strItemKey;

		if (!SingleItemMenu)
			strItemKey.Format(L"%ls/Item%d",MenuKey,KeyNumber);
		else
			strItemKey=MenuKey;

		RemoveTrailingSpaces(MenuStr);

		if (!*MenuStr)
			continue;

		if (*MenuStr==L'{' && KeyNumber>=0)
		{
			MenuFileToReg(strItemKey,MenuFile, GetStr, false,MenuCP);
			continue;
		}

		if (*MenuStr==L'}')
			break;

		if (!IsSpace(*MenuStr))
		{
			wchar_t *ChPtr=nullptr;

			if (!(ChPtr=wcschr(MenuStr,L':')))
				continue;

			if (!SingleItemMenu)
			{
				strItemKey.Format(L"%ls/Item%d",MenuKey,++KeyNumber);
			}
			else
			{
				strItemKey=MenuKey;
				++KeyNumber;
			}

			*ChPtr=0;
			FARString strHotKey=MenuStr;
			FARString strLabel=ChPtr+1;
			RemoveLeadingSpaces(strLabel);
			bool SubMenu=(GetStr.PeekString(&MenuStr, MenuCP, MenuStrLength) && *MenuStr==L'{');
			UseSameRegKey();

			// Support for old 1.x separator format
			if(MenuCP==CP_OEMCP && strHotKey==L"-" && strLabel.IsEmpty())
			{
				strHotKey+=L"-";
			}

			SetRegKey(strItemKey,L"HotKey",strHotKey);
			SetRegKey(strItemKey,L"Label",strLabel);
			SetRegKey(strItemKey,L"Submenu",SubMenu);
			CloseSameRegKey();
			CommandNumber=0;
		}
		else
		{
			if (KeyNumber>=0)
			{
				FARString strLineName;
				strLineName.Format(L"Command%d",CommandNumber++);
				RemoveLeadingSpaces(MenuStr);
				SetRegKey(strItemKey,strLineName,MenuStr);
			}
		}

		SingleItemMenu=false;
	}
}

UserMenu::UserMenu(bool ChoiceMenuType)
{
	ProcessUserMenu(ChoiceMenuType);
}

UserMenu::~UserMenu()
{
}

void UserMenu::ProcessUserMenu(bool ChoiceMenuType)
{
	// Путь к текущему каталогу с файлом LocalMenuFileName
	FARString strMenuFilePath;
	CtrlObject->CmdLine->GetCurDir(strMenuFilePath);
	// по умолчанию меню - это FarMenu.ini
	MenuMode=MM_LOCAL;
	FARString strLocalMenuKey;
	strLocalMenuKey.Format(L"UserMenu/LocalMenu%u",GetProcessUptimeMSec());
	DeleteKeyTree(strLocalMenuKey);
	MenuModified=MenuNeedRefresh=false;

	if (ChoiceMenuType)
	{
		int EditChoice=Message(0,3,MSG(MUserMenuTitle),MSG(MChooseMenuType),MSG(MChooseMenuMain),MSG(MChooseMenuLocal),MSG(MCancel));

		if (EditChoice<0 || EditChoice==2)
			return;

		if (!EditChoice)
		{
			MenuMode=MM_FAR;
			strMenuFilePath = g_strFarPath;
		}
	}

	// основной цикл обработки
	const wchar_t *LocalMenuFileName=L"FarMenu.ini";
	bool FirstRun=true;
	int ExitCode = 0;

	while ((ExitCode != EC_CLOSE_LEVEL) && (ExitCode != EC_CLOSE_MENU) && (ExitCode != EC_COMMAND_SELECTED))
	{
		FARString strMenuFileFullPath = strMenuFilePath;
		AddEndSlash(strMenuFileFullPath);
		strMenuFileFullPath += LocalMenuFileName;

		if (MenuMode != MM_MAIN)
		{
			// Пытаемся открыть файл на локальном диске
			File MenuFile;
			bool FileOpened = PathCanHoldRegularFile(strMenuFilePath) ? MenuFile.Open(strMenuFileFullPath,GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING) : false;
			if (FileOpened)
			{
				// сливаем содержимое в реестр "на запасной путь" и оттуда будем пользовать
				GetFileString GetStr(MenuFile);
				MenuFileToReg(strLocalMenuKey, MenuFile, GetStr);
				MenuFile.Close();
			}
			else
			{
				// Файл не открылся. Смотрим дальше.
				if (MenuMode == MM_FAR) // был в %FARHOME%?
				{
					MenuMode=MM_MAIN; // ...реестр
				}
				else
				{
					if (!ChoiceMenuType)
					{
						if (!FirstRun)
						{
							// подымаемся выше...
							size_t pos;

							if (FindLastSlash(pos,strMenuFilePath))
							{
								strMenuFilePath.SetLength(pos--);

								if (strMenuFilePath.At(pos) != L':')
									continue;
							}
						}

						FirstRun=false;
						strMenuFilePath = g_strFarPath;
						MenuMode=MM_FAR;
						continue;
					}
				}
			}
		}

		FARString strMenuRootKey = (MenuMode==MM_MAIN) ? L"UserMenu/MainMenu" : strLocalMenuKey;
		int PrevMacroMode=CtrlObject->Macro.GetMode();
		int _CurrentFrame=FrameManager->GetCurrentFrame()->GetType();
		CtrlObject->Macro.SetMode(MACRO_USERMENU);
		// вызываем меню
		ExitCode=ProcessSingleMenu(strMenuRootKey, 0,strMenuRootKey);

		if (_CurrentFrame == FrameManager->GetCurrentFrame()->GetType()) //???
			CtrlObject->Macro.SetMode(PrevMacroMode);

		// обработка локального меню...
		if (MenuMode != MM_MAIN)
		{
			// ...запишем изменения обратно в файл
			if (MenuModified)
			{
				DWORD FileAttr=apiGetFileAttributes(strMenuFileFullPath);

				if (FileAttr != INVALID_FILE_ATTRIBUTES)
				{
					if (FileAttr & FILE_ATTRIBUTE_READONLY)
					{
						int AskOverwrite;
						AskOverwrite=Message(MSG_WARNING,2,MSG(MUserMenuTitle),LocalMenuFileName,MSG(MEditRO),MSG(MEditOvr),MSG(MYes),MSG(MNo));

						if (!AskOverwrite)
							apiSetFileAttributes(strMenuFileFullPath,FileAttr & ~FILE_ATTRIBUTE_READONLY);
					}

					if (FileAttr & (FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM))
						apiSetFileAttributes(strMenuFileFullPath,FILE_ATTRIBUTE_NORMAL);
				}

				File MenuFile;
				// Don't use CreationDisposition=CREATE_ALWAYS here - it's kills alternate streams
				if (MenuFile.Open(strMenuFileFullPath,GENERIC_WRITE, FILE_SHARE_READ, nullptr, FileAttr==INVALID_FILE_ATTRIBUTES?CREATE_NEW:TRUNCATE_EXISTING))
				{
					CachedWrite CW(MenuFile);
					WCHAR Data = SIGN_UNICODE;
					CW.Write(&Data, 1*sizeof(WCHAR));
					MenuRegToFile(strLocalMenuKey,MenuFile, CW);
					CW.Flush();
					UINT64 Size = 0;
					MenuFile.GetSize(Size);
					MenuFile.Close();

					// если файл FarMenu.ini пуст, то удалим его
					if (Size<3) // 2 for BOM
					{
						apiDeleteFile(strMenuFileFullPath);
					}
					else if (FileAttr!=INVALID_FILE_ATTRIBUTES)
					{
						apiSetFileAttributes(strMenuFileFullPath,FileAttr);
					}
				}
			}

			// ...почистим реестр.
			DeleteKeyTree(strLocalMenuKey);
		}

		// что было после вызова меню?
		switch (ExitCode)
		{
				// Показать меню родительского каталога
			case EC_PARENT_MENU:
			{
				if (MenuMode == MM_LOCAL)
				{
					size_t pos;

					if (FindLastSlash(pos,strMenuFilePath))
					{
						strMenuFilePath.SetLength(pos--);

						if (strMenuFilePath.At(pos)!=L':')
							continue;
					}

					strMenuFilePath = g_strFarPath;
					MenuMode=MM_FAR;
				}
				else
				{
					MenuMode=MM_MAIN;
				}

				break;
			}
			// Показать главное меню
			case EC_MAIN_MENU:
			{
				// $ 14.07.2000 VVM: Shift+F2 переключает Главное меню/локальное в цикле
				switch (MenuMode)
				{
					case MM_LOCAL:
					{
						strMenuFilePath = g_strFarPath;
						MenuMode=MM_FAR;
						break;
					}
					case MM_FAR:
					{
						MenuMode=MM_MAIN;
						break;
					}
					default: // MM_MAIN
					{
						CtrlObject->CmdLine->GetCurDir(strMenuFilePath);
						MenuMode=MM_LOCAL;
					}
				}

				break;
			}
		}
	}

	if (FrameManager->IsPanelsActive() && (ExitCode == EC_COMMAND_SELECTED || MenuModified))
		ShellUpdatePanels(CtrlObject->Cp()->ActivePanel,FALSE);
}

// заполнение меню
int FillUserMenu(VMenu& UserMenu,const wchar_t *MenuKey,int MenuPos,int *FuncPos,const wchar_t *Name)
{
	UserMenu.DeleteItems();
	int NumLines=0;

	for (NumLines=0;; NumLines++)
	{
		FARString strItemKey;
		strItemKey.Format(L"%ls/Item%d",MenuKey,NumLines);

		if (!CheckRegKey(strItemKey))
		{
			break;
		}

		MenuItemEx UserMenuItem;
		UserMenuItem.Clear();
		FARString strHotKey;
		GetRegKey(strItemKey,L"HotKey",strHotKey,L"");
		FARString strLabel;
		GetRegKey(strItemKey,L"Label",strLabel,L"");
		int FuncNum=0;

		// сепаратором является случай, когда хоткей == "--"
		if (!StrCmp(strHotKey,L"--"))
		{
			UserMenuItem.Flags|=LIF_SEPARATOR;
			UserMenuItem.Flags&=~LIF_SELECTED;
			UserMenuItem.strName=strLabel;

			if (NumLines==MenuPos)
			{
				MenuPos++;
			}
		}
		else
		{
			SubstFileName(strLabel,Name,nullptr,nullptr,TRUE);
			apiExpandEnvironmentStrings(strLabel, strLabel);
			FuncNum=PrepareHotKey(strHotKey);
			int Offset=strHotKey.At(0)==L'&'?5:4;
			FormatString FString;
			FString<<((!strHotKey.IsEmpty() && !FuncNum)?L"&":L"")<<fmt::LeftAlign()<<fmt::Width(Offset)<<fmt::Precision(Offset)<<strHotKey;
			UserMenuItem.strName=FString.strValue();
			UserMenuItem.strName+=strLabel;

			if (GetRegKey(strItemKey,L"Submenu",0))
			{
				UserMenuItem.Flags|=MIF_SUBMENU;
			}

			UserMenuItem.SetSelect(NumLines==MenuPos);
			UserMenuItem.Flags &= ~LIF_SEPARATOR;
		}

		int ItemPos=UserMenu.AddItem(&UserMenuItem);

		if (FuncNum>0)
		{
			FuncPos[FuncNum-1]=ItemPos;
		}
	}

	MenuItemEx UserMenuItem;
	UserMenuItem.Clear();
	UserMenuItem.SetSelect(NumLines==MenuPos);
	UserMenu.AddItem(&UserMenuItem);
	return NumLines;
}

// обработка единичного меню
int UserMenu::ProcessSingleMenu(const wchar_t *MenuKey,int MenuPos,const wchar_t *MenuRootKey,const wchar_t *Title)
{
	MenuItemEx UserMenuItem;

	for (;;)
	{
		UserMenuItem.Clear();
		int NumLine=0,ExitCode,FuncPos[24];

		// очистка F-хоткеев
		for (size_t I=0 ; I < ARRAYSIZE(FuncPos) ; I++)
			FuncPos[I]=-1;

		FARString strName;
		CtrlObject->Cp()->ActivePanel->GetCurName(strName);
		/* $ 24.07.2000 VVM + При показе главного меню в заголовок добавляет тип - FAR/Registry */
		FARString strMenuTitle;

		if (Title && *Title)
			strMenuTitle = Title;
		else
		{
			switch (MenuMode)
			{
				case MM_LOCAL:
					strMenuTitle = MSG(MLocalMenuTitle);
					break;
				case MM_FAR:
				{
					strMenuTitle=MSG(MMainMenuTitle);
					strMenuTitle+=L" (";
					strMenuTitle+=MSG(MMainMenuFAR);
					strMenuTitle+=L")";
				}
				break;
				default:
				{
					strMenuTitle=MSG(MMainMenuTitle);
					const wchar_t *Ptr=MSG(MMainMenuREG);

					if (*Ptr)
					{
						strMenuTitle+=L" (";
						strMenuTitle+=Ptr;
						strMenuTitle+=L")";
					}
				}
			}
		}

		{
			VMenu UserMenu(strMenuTitle,nullptr,0,ScrY-4);
			UserMenu.SetFlags(VMENU_WRAPMODE);
			UserMenu.SetHelp(L"UserMenu");
			UserMenu.SetPosition(-1,-1,0,0);
			UserMenu.SetBottomTitle(MSG(MMainMenuBottomTitle));
			//NumLine=FillUserMenu(UserMenu,MenuKey,MenuPos,FuncPos,Name);
			MenuNeedRefresh=true;

			while (!UserMenu.Done())
			{
				if (MenuNeedRefresh)
				{
					UserMenu.Hide(); // спрячем
					// "изнасилуем" (перезаполним :-)
					NumLine=FillUserMenu(UserMenu,MenuKey,MenuPos,FuncPos,strName);
					// заставим манагер менюхи корректно отрисовать ширину и
					// высоту, а заодно и скорректировать вертикальные позиции
					UserMenu.SetPosition(-1,-1,-1,-1);
					UserMenu.Show();
					MenuNeedRefresh=false;
				}

				int Key=UserMenu.ReadInput();
				MenuPos=UserMenu.GetSelectPos();

				if ((unsigned int)Key>=KEY_F1 && (unsigned int)Key<=KEY_F24)
				{
					int FuncItemPos;

					if ((FuncItemPos=FuncPos[Key-KEY_F1])!=-1)
					{
						UserMenu.Modal::SetExitCode(FuncItemPos);
						continue;
					}
				}
				else if (Key == L' ') // исключаем пробел из "хоткеев"!
					continue;

				switch (Key)
				{
						/* $ 24.08.2001 VVM + Стрелки вправо/влево открывают/закрывают подменю соответственно */
					case KEY_RIGHT:
					case KEY_NUMPAD6:
					case KEY_MSWHEEL_RIGHT:
					{
						FARString strCurrentKey;
						int SubMenu;
						strCurrentKey.Format(L"%ls/Item%d",MenuKey,MenuPos);
						GetRegKey(strCurrentKey,L"Submenu",SubMenu,0);

						if (SubMenu)
							UserMenu.SetExitCode(MenuPos);

						break;
					}
					case KEY_LEFT:
					case KEY_NUMPAD4:
					case KEY_MSWHEEL_LEFT:

						if (Title && *Title)
							UserMenu.SetExitCode(-1);

						break;
					case KEY_NUMDEL:
					case KEY_DEL:

						if (MenuPos<NumLine)
							DeleteMenuRecord(MenuKey,MenuPos);

						break;
					case KEY_INS:
					case KEY_F4:
					case KEY_SHIFTF4:
					case KEY_NUMPAD0:

						if (Key != KEY_INS && Key != KEY_NUMPAD0 && MenuPos>=NumLine)
							break;

						EditMenu(MenuKey,MenuPos,NumLine,Key == KEY_INS || Key == KEY_NUMPAD0);
						break;
					case KEY_CTRLUP:
					case KEY_CTRLDOWN:
					{
						int Pos=UserMenu.GetSelectPos();

						if (Pos!=UserMenu.GetItemCount()-1)
						{
							if (!(Key==KEY_CTRLUP && !Pos) && !(Key==KEY_CTRLDOWN && Pos==UserMenu.GetItemCount()-2))
							{
								MenuPos=Pos+(Key==KEY_CTRLUP?-1:+1);
								MoveMenuItem(MenuKey,Pos,MenuPos);
							}
						}
					}
					break;
					//case KEY_ALTSHIFTF4:  // редактировать только текущий пункт (если субменю - то все субменю)
					case KEY_ALTF4:       // редактировать все меню
					{
						(*FrameManager)[0]->Unlock();
						FARString strMenuFileName;
						File MenuFile;
						if (!FarMkTempEx(strMenuFileName) || (!MenuFile.Open(strMenuFileName, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS)))
						{
							break;
						}

						FARString strCurrentKey;

						if (Key==KEY_ALTSHIFTF4)
							strCurrentKey.Format(L"%ls/Item%d",MenuKey,MenuPos);
						else
							strCurrentKey=MenuRootKey;
						CachedWrite CW(MenuFile);
						WCHAR Data = SIGN_UNICODE;
						CW.Write(&Data, 1*sizeof(WCHAR));
						MenuRegToFile(strCurrentKey, MenuFile, CW, Key==KEY_ALTSHIFTF4);
						CW.Flush();
						MenuNeedRefresh=true;
						MenuFile.Close();
						{
							ConsoleTitle *OldTitle=new ConsoleTitle;
							FARString strFileName = strMenuFileName;
							FileEditor ShellEditor(strFileName,CP_UNICODE,FFILEEDIT_DISABLEHISTORY,-1,-1,nullptr);
							delete OldTitle;
							ShellEditor.SetDynamicallyBorn(false);
							FrameManager->EnterModalEV();
							FrameManager->ExecuteModal();
							FrameManager->ExitModalEV();

							if (!ShellEditor.IsFileChanged() || (!MenuFile.Open(strMenuFileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING)))
							{
								apiDeleteFile(strMenuFileName);

								if (Key == KEY_ALTSHIFTF4) // для тукущего пункта меню закрывать ненадо
									break;

								return 0;
							}
						}
						DeleteKeyTree(strCurrentKey);
						GetFileString GetStr(MenuFile);
						MenuFileToReg(strCurrentKey, MenuFile, GetStr, Key==KEY_ALTSHIFTF4);
						MenuFile.Close();
						apiDeleteFile(strMenuFileName);
						MenuModified=true;
						UserMenu.Hide();

						if (Key == KEY_ALTSHIFTF4) // для тукущего пункта меню закрывать ненадо
							break;

						return 0; // Закрыть меню
					}
					/* $ 28.06.2000 tran
					выход из пользовательского меню по ShiftF10 из любого уровня
					вложенности просто задаем ExitCode -1, и возвращаем FALSE -
					по FALSE оно и выйдет откуда угодно */
					case KEY_SHIFTF10:
						//UserMenu.SetExitCode(-1);
						return(EC_CLOSE_MENU);
					case KEY_SHIFTF2: // Показать главное меню
						return(EC_MAIN_MENU);
					case KEY_BS: // Показать меню из родительского каталога только в MM_LOCAL режиме

						if (MenuMode != MM_MAIN)
							return(EC_PARENT_MENU);

					default:
						UserMenu.ProcessInput();

						if (Key == KEY_F1)
							MenuNeedRefresh=true;

						break;
				} // switch(Key)
			} // while (!UserMenu.Done())

			ExitCode=UserMenu.Modal::GetExitCode();
		}

		if (ExitCode<0 || ExitCode>=NumLine)
			return(EC_CLOSE_LEVEL); //  вверх на один уровень

		FARString strCurrentKey;
		int SubMenu;
		strCurrentKey.Format(L"%ls/Item%d",MenuKey,ExitCode);
		GetRegKey(strCurrentKey,L"Submenu",SubMenu,0);

		if (SubMenu)
		{
			/* $ 20.08.2001 VVM + При вложенных меню показывает заголовки предыдущих */
			FARString strSubMenuKey, strSubMenuLabel, strSubMenuTitle;
			strSubMenuKey.Format(L"%ls/Item%d",MenuKey,ExitCode);

			if (GetRegKey(strSubMenuKey,L"Label",strSubMenuLabel,L""))
			{
				SubstFileName(strSubMenuLabel,strName,nullptr,nullptr,TRUE);
				apiExpandEnvironmentStrings(strSubMenuLabel, strSubMenuLabel);
				size_t pos;

				if (strSubMenuLabel.Pos(pos,L'&'))
					strSubMenuLabel.LShift(1,pos);

				if (Title && *Title)
				{
					strSubMenuTitle=Title;
					strSubMenuTitle+=L" -> ";
					strSubMenuTitle+=strSubMenuLabel;
				}
				else
					strSubMenuTitle = strSubMenuLabel;
			}

			/* $ 14.07.2000 VVM ! Если закрыли подменю, то остаться. Инече передать управление выше */
			MenuPos=ProcessSingleMenu(strSubMenuKey,0,MenuRootKey,strSubMenuTitle);

			if (MenuPos!=EC_CLOSE_LEVEL)
				return(MenuPos);

			MenuPos=ExitCode;
			continue;
		}

		/* $ 01.05.2001 IS Отключим до лучших времен */
		//int LeftVisible,RightVisible,PanelsHidden=0;
		int CurLine=0;
		FARString strCmdLineDir;
		CtrlObject->CmdLine->GetCurDir(strCmdLineDir);
		FARString strOldCmdLine;
		CtrlObject->CmdLine->GetString(strOldCmdLine);
		int OldCmdLineCurPos = CtrlObject->CmdLine->GetCurPos();
		int OldCmdLineLeftPos = CtrlObject->CmdLine->GetLeftPos();
		int OldCmdLineSelStart, OldCmdLineSelEnd;
		CtrlObject->CmdLine->GetSelection(OldCmdLineSelStart,OldCmdLineSelEnd);
		CtrlObject->CmdLine->LockUpdatePanel(TRUE);

		// Цикл исполнения команд меню (CommandX)
		for (;;)
		{
			FormatString strLineName;
			FARString strCommand;
			strLineName<<L"Command"<<CurLine;

			if (!GetRegKey(strCurrentKey,strLineName,strCommand,L""))
				break;

			FARString strListName, strAnotherListName;

			if (!((!StrCmpNI(strCommand,L"REM",3) && IsSpaceOrEos(strCommand.At(3))) || !StrCmpNI(strCommand,L"::",2)))
			{
				/*
				  Осталось корректно обработать ситуацию, например:
				  if exist !#!\!^!.! far:edit < diff -c -p !#!\!^!.! !\!.!
				  Т.е. сначала "вычислить" кусок "if exist !#!\!^!.!", ну а если
				  выполнится, то делать дальше.
				  Или еще пример,
				  if exist ..\a.bat D:\FAR\170\DIFF.MY\mkdiff.bat !?&Номер патча?!
				  ЭТО выполняется всегда, т.к. парсинг всей строки идет, а надо
				  проверить фазу "if exist ..\a.bat", а уж потом делать выводы...
				*/
				{
					/* $ 01.05.2001 IS Отключим до лучших времен */
					/*
					if (!PanelsHidden)
					{
						LeftVisible=CtrlObject->Cp()->LeftPanel->IsVisible();
						RightVisible=CtrlObject->Cp()->RightPanel->IsVisible();
						CtrlObject->Cp()->LeftPanel->Hide();
						CtrlObject->Cp()->RightPanel->Hide();
						CtrlObject->Cp()->LeftPanel->SetUpdateMode(FALSE);
						CtrlObject->Cp()->RightPanel->SetUpdateMode(FALSE);
						PanelsHidden=TRUE;
					}
					*/
					//;
					int PreserveLFN=SubstFileName(strCommand,strName,&strListName,&strAnotherListName, FALSE, strCmdLineDir);
					bool ListFileUsed=!strListName.IsEmpty()||!strAnotherListName.IsEmpty();

					{
						//PreserveLongName PreserveName(PreserveLFN);
						RemoveExternalSpaces(strCommand);

						if (!strCommand.IsEmpty())
						{
							bool isSilent=false;

							if (strCommand.At(0) == L'@')
							{
								strCommand.LShift(1);
								isSilent=true;
							}

							ProcessOSAliases(strCommand);
							// TODO: Ахтунг. В режиме isSilent имеем проблемы с командами, которые выводят что-то на экран
							//       Здесь необходимо переделка, например, перед исполнением подсунуть временный экранный буфер, а потом его содержимое подсунуть в ScreenBuf...

							if (!isSilent)
							{
								CtrlObject->CmdLine->ExecString(strCommand,FALSE, 0, 0, ListFileUsed);
							}
							else
							{
								SaveScreen SaveScr;
								CtrlObject->Cp()->LeftPanel->CloseFile();
								CtrlObject->Cp()->RightPanel->CloseFile();
								Execute(strCommand,TRUE, 0, 0, 0, ListFileUsed, true);
							}
						}
					}
				}
			} // strCommand != "REM"

			if (!strListName.IsEmpty())
				apiDeleteFile(strListName);

			if (!strAnotherListName.IsEmpty())
				apiDeleteFile(strAnotherListName);

			CurLine++;
		} // while (1)

		CtrlObject->CmdLine->LockUpdatePanel(FALSE);

		if (!strOldCmdLine.IsEmpty())  // восстановим сохраненную командную строку
		{
			CtrlObject->CmdLine->SetString(strOldCmdLine, FrameManager->IsPanelsActive());
			CtrlObject->CmdLine->SetCurPos(OldCmdLineCurPos, OldCmdLineLeftPos);
			CtrlObject->CmdLine->Select(OldCmdLineSelStart, OldCmdLineSelEnd);
		}

		/* $ 01.05.2001 IS Отключим до лучших времен */
		/*
		if (PanelsHidden)
		{
			CtrlObject->Cp()->LeftPanel->SetUpdateMode(TRUE);
			CtrlObject->Cp()->RightPanel->SetUpdateMode(TRUE);
			CtrlObject->Cp()->LeftPanel->Update(UPDATE_KEEP_SELECTION);
			CtrlObject->Cp()->RightPanel->Update(UPDATE_KEEP_SELECTION);
			if (RightVisible)
				CtrlObject->Cp()->RightPanel->Show();
			if (LeftVisible)
				CtrlObject->Cp()->LeftPanel->Show();
		}
		*/
		/* $ 14.07.2000 VVM ! Закрыть меню */
		/* $ 25.04.2001 DJ - сообщаем, что была выполнена команда (нужно перерисовать панели) */
		return(EC_COMMAND_SELECTED);
	}
}

enum EditMenuItems
{
	EM_DOUBLEBOX,
	EM_HOTKEY_TEXT,
	EM_HOTKEY_EDIT,
	EM_LABEL_TEXT,
	EM_LABEL_EDIT,
	EM_SEPARATOR1,
	EM_COMMANDS_TEXT,
#ifdef PROJECT_DI_MEMOEDIT
	EM_MEMOEDIT,
#else
	EM_EDITLINE_0,
	EM_EDITLINE_1,
	EM_EDITLINE_2,
	EM_EDITLINE_3,
	EM_EDITLINE_4,
	EM_EDITLINE_5,
	EM_EDITLINE_6,
	EM_EDITLINE_7,
	EM_EDITLINE_8,
	EM_EDITLINE_9,
#endif
	EM_SEPARATOR2,
	EM_BUTTON_OK,
	EM_BUTTON_CANCEL,
};

LONG_PTR WINAPI EditMenuDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2)
{
#if defined(PROJECT_DI_MEMOEDIT)
	Dialog* Dlg=(Dialog*)hDlg;

	switch (Msg)
	{
		case DN_INITDIALOG:
		{
			break;
		}
	}

#endif

	switch (Msg)
	{
		case DN_CLOSE:

			if (Param1==EM_BUTTON_OK)
			{
				BOOL Result=TRUE;
				LPCWSTR HotKey=reinterpret_cast<LPCWSTR>(SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,EM_HOTKEY_EDIT,0));
				LPCWSTR Label=reinterpret_cast<LPCWSTR>(SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,EM_LABEL_EDIT,0));
				int FocusPos=-1;

				if(StrCmp(HotKey,L"--"))
				{
					if (!*Label)
					{
						FocusPos=EM_LABEL_EDIT;
					}
					else if (StrLength(HotKey)>1)
					{
						FocusPos=EM_HOTKEY_EDIT;

						if (Upper(*HotKey)==L'F')
						{
							int FuncNum=_wtoi(HotKey+1);

							if (FuncNum > 0 && FuncNum < 25)
								FocusPos=-1;
						}
					}
				}

				if (FocusPos!=-1)
				{
					Message(MSG_WARNING,1,MSG(MUserMenuTitle),MSG((*Label?MUserMenuInvalidInputHotKey:MUserMenuInvalidInputLabel)),MSG(MOk));
					SendDlgMessage(hDlg,DM_SETFOCUS,FocusPos,0);
					Result=FALSE;
				}

				return Result;
			}

			break;
	}

	return DefDlgProc(hDlg,Msg,Param1,Param2);
}


bool UserMenu::EditMenu(const wchar_t *MenuKey,int EditPos,int TotalRecords,bool Create)
{
	bool Result=false;
	FormatString strItemKey;
	strItemKey<<MenuKey<<L"/Item"<<EditPos;
	MenuNeedRefresh=true;
	bool SubMenu=false,Continue=true;

	if (Create)
	{
		switch (Message(0,2,MSG(MUserMenuTitle),MSG(MAskInsertMenuOrCommand),MSG(MMenuInsertCommand),MSG(MMenuInsertMenu)))
		{
			case -1:
			case -2:
				Continue=false;
			case 1:
				SubMenu=true;
		}
	}
	else
	{
		int _SubMenu;
		GetRegKey(strItemKey,L"Submenu",_SubMenu,0);
		SubMenu=_SubMenu?true:false;
	}

	if (Continue)
	{
		const int DLG_X=76, DLG_Y=SubMenu?10:22;
		DWORD State=SubMenu?DIF_HIDDEN|DIF_DISABLE:0;
		DialogDataEx EditDlgData[]=
		{
			DI_DOUBLEBOX,3,1,DLG_X-4,(short)(DLG_Y-2),0,0,MSG(SubMenu?MEditSubmenuTitle:MEditMenuTitle),
			DI_TEXT,5,2,0,2,0,0,MSG(MEditMenuHotKey),
			DI_FIXEDIT,5,3,7,3,0,DIF_FOCUS,L"",
			DI_TEXT,5,4,0,4,0,0,MSG(MEditMenuLabel),
			DI_EDIT,5,5,DLG_X-6,5,0,0,L"",

			DI_TEXT,3,6,0,6,0,DIF_SEPARATOR|State,L"",
			DI_TEXT,5,7,0,7,0,State,MSG(MEditMenuCommands),
#ifdef PROJECT_DI_MEMOEDIT
			DI_MEMOEDIT,5, 8,DLG_X-6,17,0,DIF_EDITPATH,L"",
#else
			DI_EDIT,5, 8,DLG_X-6,8,0,DIF_EDITPATH|DIF_EDITOR|State,L"",
			DI_EDIT,5, 9,DLG_X-6,9,0,DIF_EDITPATH|DIF_EDITOR|State,L"",
			DI_EDIT,5,10,DLG_X-6,10,0,DIF_EDITPATH|DIF_EDITOR|State,L"",
			DI_EDIT,5,11,DLG_X-6,11,0,DIF_EDITPATH|DIF_EDITOR|State,L"",
			DI_EDIT,5,12,DLG_X-6,12,0,DIF_EDITPATH|DIF_EDITOR|State,L"",
			DI_EDIT,5,13,DLG_X-6,13,0,DIF_EDITPATH|DIF_EDITOR|State,L"",
			DI_EDIT,5,14,DLG_X-6,14,0,DIF_EDITPATH|DIF_EDITOR|State,L"",
			DI_EDIT,5,15,DLG_X-6,15,0,DIF_EDITPATH|DIF_EDITOR|State,L"",
			DI_EDIT,5,16,DLG_X-6,16,0,DIF_EDITPATH|DIF_EDITOR|State,L"",
			DI_EDIT,5,17,DLG_X-6,17,0,DIF_EDITPATH|DIF_EDITOR|State,L"",
#endif

			DI_TEXT,3,(short)(DLG_Y-4),0,(short)(DLG_Y-4),0,DIF_SEPARATOR,L"",
			DI_BUTTON,0,(short)(DLG_Y-3),0,(short)(DLG_Y-3),0,DIF_DEFAULT|DIF_CENTERGROUP,MSG(MOk),
			DI_BUTTON,0,(short)(DLG_Y-3),0,(short)(DLG_Y-3),0,DIF_CENTERGROUP,MSG(MCancel),
		};
		MakeDialogItemsEx(EditDlgData,EditDlg);
#ifndef PROJECT_DI_MEMOEDIT
		enum {DI_EDIT_COUNT=EM_SEPARATOR2-EM_COMMANDS_TEXT-1};
#endif

		if (!Create)
		{
			GetRegKey(strItemKey,L"HotKey",EditDlg[EM_HOTKEY_EDIT].strData,L"");
			GetRegKey(strItemKey,L"Label",EditDlg[EM_LABEL_EDIT].strData,L"");
#if defined(PROJECT_DI_MEMOEDIT)
			/*
				...
				здесь добавка строк из "Command%d" в EMR_MEMOEDIT
				...
			*/
			FARString strBuffer;
			int CommandNumber=0;

			while (true)
			{
				FARString strCommandName, strCommand;
				strCommandName.Format(L"Command%d",CommandNumber);

				if (!GetRegKey(strItemKey,strCommandName,strCommand,L""))
					break;

				strBuffer+=strCommand;
				strBuffer+=L"\n";    //??? "\n\r"
				CommandNumber++;
			}

			EditDlg[EM_MEMOEDIT].strData = strBuffer; //???
#else
			int CommandNumber=0;

			while (CommandNumber < DI_EDIT_COUNT)
			{
				FARString strCommandName, strCommand;
				strCommandName.Format(L"Command%d",CommandNumber);

				if (!GetRegKey(strItemKey,strCommandName,strCommand,L""))
					break;

				EditDlg[EM_EDITLINE_0+CommandNumber].strData = strCommand;
				CommandNumber++;
			}

#endif
		}

		Dialog Dlg(EditDlg,ARRAYSIZE(EditDlg),EditMenuDlgProc);
		Dlg.SetHelp(L"UserMenu");
		Dlg.SetPosition(-1,-1,DLG_X,DLG_Y);
		Dlg.Process();

		if (Dlg.GetExitCode()==EM_BUTTON_OK)
		{
			MenuModified=true;

			if (Create)
			{
				FARString strKeyMask;
				strKeyMask.Format(L"%ls/Item%%d",MenuKey);
				InsertKeyRecord(strKeyMask,EditPos,TotalRecords);
			}

			SetRegKey(strItemKey,L"HotKey",EditDlg[EM_HOTKEY_EDIT].strData);
			SetRegKey(strItemKey,L"Label",EditDlg[EM_LABEL_EDIT].strData);
			SetRegKey(strItemKey,L"Submenu",(DWORD)0);

			if (SubMenu)
			{
				SetRegKey(strItemKey,L"Submenu",(DWORD)1);
			}
			else
			{
#if defined(PROJECT_DI_MEMOEDIT)
				/*
				...
				здесь преобразование содержимого итема EMR_MEMOEDIT в "Command%d"
				...
				*/
#else
				int CommandNumber=0;

				for (int i=0 ; i < DI_EDIT_COUNT ; i++)
					if (!EditDlg[i+EM_EDITLINE_0].strData.IsEmpty())
						CommandNumber=i+1;

				for (int i=0 ; i < DI_EDIT_COUNT ; i++)
				{
					FARString strCommandName;
					strCommandName.Format(L"Command%d",i);

					if (i>=CommandNumber)
						DeleteRegValue(strItemKey,strCommandName);
					else
						SetRegKey(strItemKey,strCommandName,EditDlg[i+EM_EDITLINE_0].strData);
				}

#endif
			}

			Result=true;
		}
	}

	return Result;
}

int UserMenu::DeleteMenuRecord(const wchar_t *MenuKey,int DeletePos)
{
	FARString strRecText;
	FormatString strRegKey;
	strRegKey<<MenuKey<<L"/Item"<<DeletePos;
	GetRegKey(strRegKey,L"Label",strRecText,L"");
	int SubMenu;
	GetRegKey(strRegKey,L"Submenu",SubMenu,0);
	FARString strItemName=strRecText;
	InsertQuote(strItemName);

	if (Message(MSG_WARNING,2,MSG(MUserMenuTitle),MSG(!SubMenu?MAskDeleteMenuItem:MAskDeleteSubMenuItem),strItemName,MSG(MDelete),MSG(MCancel)))
		return FALSE;

	MenuModified=MenuNeedRefresh=true;
	strRegKey.Clear();
	strRegKey<<MenuKey<<L"/Item%d";
	DeleteKeyRecord(strRegKey,DeletePos);
	return TRUE;
}

bool UserMenu::MoveMenuItem(const wchar_t *MenuKey,int Pos,int NewPos)
{
	FormatString strSrc,strDst,strTmp;
	strSrc<<MenuKey<<L"/Item"<<Pos;
	strDst<<MenuKey<<L"/Item"<<NewPos;
	strTmp<<MenuKey<<L"/Item"<<WINPORT(GetTickCount)();
	CopyLocalKeyTree(strDst,strTmp);
	DeleteKeyTree(strDst);
	CopyLocalKeyTree(strSrc,strDst);
	DeleteKeyTree(strSrc);
	CopyLocalKeyTree(strTmp,strSrc);
	DeleteKeyTree(strTmp);
	MenuModified=MenuNeedRefresh=true;
	return true;
}
