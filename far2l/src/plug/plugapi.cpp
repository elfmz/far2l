/*
plugapi.cpp

API, доступное плагинам (диалоги, меню, ...)
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
#include <list>

#include "plugapi.hpp"
#include "keys.hpp"
#include "lang.hpp"
#include "help.hpp"
#include "vmenu.hpp"
#include "dialog.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "cmdline.hpp"
#include "scantree.hpp"
#include "rdrwdsk.hpp"
#include "fileview.hpp"
#include "fileedit.hpp"
#include "plugins.hpp"
#include "savescr.hpp"
#include "manager.hpp"
#include "ctrlobj.hpp"
#include "frame.hpp"
#include "scrbuf.hpp"
#include "lockscrn.hpp"
#include "constitle.hpp"
#include "TPreRedrawFunc.hpp"
#include "syslog.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "palette.hpp"
#include "message.hpp"
#include "filefilter.hpp"
#include "fileowner.hpp"
#include "stddlg.hpp"
#include "pathmix.hpp"
#include "exitcode.hpp"
#include "processname.hpp"
#include "synchro.hpp"
#include "RegExp.hpp"
#include "console.hpp"
#include "InterThreadCall.hpp"

#include "farversion.h"

wchar_t *WINAPI FarItoa(int value, wchar_t *string, int radix)
{
	if (string)
		return _itow(value, string, radix);

	return nullptr;
}

wchar_t *WINAPI FarItoa64(int64_t value, wchar_t *string, int radix)
{
	if (string)
		return _i64tow(value, string, radix);

	return nullptr;
}

int WINAPI FarAtoi(const wchar_t *s)
{
	if (s)
		return _wtoi(s);

	return 0;
}
int64_t WINAPI FarAtoi64(const wchar_t *s)
{
	return s ? _wtoi64(s) : 0;
}

void WINAPI FarQsort(void *base, size_t nelem, size_t width, int(__cdecl *fcmp)(const void *, const void *))
{
	if (base && fcmp)
		far_qsort(base, nelem, width, fcmp);
}

void WINAPI FarQsortEx(void *base, size_t nelem, size_t width,
		int(__cdecl *fcmp)(const void *, const void *, void *user), void *user)
{
	if (base && fcmp)
		far_qsortex(base, nelem, width, fcmp, user);
}

void *WINAPI FarBsearch(const void *key, const void *base, size_t nelem, size_t width,
		int(__cdecl *fcmp)(const void *, const void *))
{
	if (key && fcmp && base)
		return bsearch(key, base, nelem, width, fcmp);

	return nullptr;
}

void WINAPI DeleteBuffer(void *Buffer)
{
	if (Buffer)
		free(Buffer);
}

static void ScanPluginDir();

/*
	$ 07.12.2001 IS
	Обертка вокруг GetString для плагинов - с меньшей функциональностью.
	Сделано для того, чтобы не дублировать код GetString.
*/
static int FarInputBoxSynched(const wchar_t *Title, const wchar_t *Prompt, const wchar_t *HistoryName,
		const wchar_t *SrcText, wchar_t *DestText, int DestLength, const wchar_t *HelpTopic, DWORD Flags)
{
	if (FrameManager->ManagerIsDown())
		return FALSE;

	FARString strDest;
	int nResult = GetString(Title, Prompt, HistoryName, SrcText, strDest, HelpTopic, Flags & ~FIB_CHECKBOX,
			nullptr, nullptr);
	far_wcsncpy(DestText, strDest, DestLength + 1);
	return nResult;
}

int WINAPI FarInputBox(const wchar_t *Title, const wchar_t *Prompt, const wchar_t *HistoryName,
		const wchar_t *SrcText, wchar_t *DestText, int DestLength, const wchar_t *HelpTopic, DWORD Flags)
{
	return InterThreadCall<int, 0>(std::bind(FarInputBoxSynched, Title, Prompt, HistoryName, SrcText,
			DestText, DestLength, HelpTopic, Flags));
}

/* Функция вывода помощи */
BOOL WINAPI FarShowHelpSynched(const wchar_t *ModuleName, const wchar_t *HelpTopic, DWORD Flags)
{
	if (FrameManager->ManagerIsDown())
		return FALSE;

	if (!HelpTopic)
		HelpTopic = L"Contents";

	DWORD OFlags = Flags;
	Flags&= ~(FHELP_NOSHOWERROR | FHELP_USECONTENTS);
	FARString strPath, strTopic;
	FARString strMask;

	// двоеточие в начале топика надо бы игнорировать и в том случае,
	// если стоит FHELP_FARHELP...
	if ((Flags & FHELP_FARHELP) || *HelpTopic == L':')
		strTopic = HelpTopic + ((*HelpTopic == L':') ? 1 : 0);
	else {
		if (ModuleName) {
			// FHELP_SELFHELP=0 - трактовать первый пар-р как Info.ModuleName
			//                   и показать топик из хелпа вызвавшего плагина
			/*
				$ 17.11.2000 SVS
				А значение FHELP_SELFHELP равно чему? Правильно - 0
				И фигля здесь удивлятся тому, что функция не работает :-(
			*/
			if (Flags == FHELP_SELFHELP || (Flags & (FHELP_CUSTOMFILE | FHELP_CUSTOMPATH))) {
				strPath = ModuleName;

				if (Flags == FHELP_SELFHELP || (Flags & (FHELP_CUSTOMFILE))) {
					if (Flags & FHELP_CUSTOMFILE)
						strMask = PointToName(strPath);
					else
						strMask.Clear();

					CutToSlash(strPath);
				}
			} else
				return FALSE;

			strTopic.Format(HelpFormatLink, strPath.CPtr(), HelpTopic);
		} else
			return FALSE;
	}

	{
		Help Hlp(strTopic, strMask, OFlags);

		if (Hlp.GetError())
			return FALSE;
	}

	return TRUE;
}

BOOL WINAPI FarShowHelp(const wchar_t *ModuleName, const wchar_t *HelpTopic, DWORD Flags)
{
	return InterThreadCall<BOOL, FALSE>(std::bind(FarShowHelpSynched, ModuleName, HelpTopic, Flags));
}

/*
	$ 05.07.2000 IS
	Функция, которая будет действовать и в редакторе, и в панелях, и...
*/
static INT_PTR WINAPI FarAdvControlSynched(INT_PTR ModuleNumber, int Command, void *Param1, void *Param2)
{
	if (ACTL_SYNCHRO == Command)	// must be first
	{
		PluginSynchroManager.Synchro(true, ModuleNumber, Param1);
		return 0;
	}

	struct Opt2Flags
	{
		int *Opt;
		DWORD Flags;
	};

	switch (Command) {
		case ACTL_GETFARVERSION:
		case ACTL_GETSYSWORDDIV:
		case ACTL_GETCOLOR:
		case ACTL_GETARRAYCOLOR:
		case ACTL_GETFARHWND:
		case ACTL_GETSYSTEMSETTINGS:
		case ACTL_GETPANELSETTINGS:
		case ACTL_GETINTERFACESETTINGS:
		case ACTL_GETCONFIRMATIONS:
		case ACTL_GETDESCSETTINGS:
		case ACTL_GETPOLICIES:
		case ACTL_GETPLUGINMAXREADDATA:
		case ACTL_GETMEDIATYPE:
		case ACTL_SETPROGRESSSTATE:
		case ACTL_SETPROGRESSVALUE:
		case ACTL_GETFARRECT:
		case ACTL_GETCURSORPOS:
		case ACTL_SETCURSORPOS:
		case ACTL_PROGRESSNOTIFY:
			break;
		default:

			if (FrameManager && FrameManager->ManagerIsDown())
				return 0;
	}

	switch (Command) {
		case ACTL_GETFARVERSION: {
			if (Param1)
				*(DWORD *)Param1 = FAR_VERSION;

			return FAR_VERSION;
		}
		case ACTL_GETPLUGINMAXREADDATA: {
			return Opt.PluginMaxReadData;
		}
		case ACTL_GETSYSWORDDIV: {
			if (Param1)
				wcscpy((wchar_t *)Param1, Opt.strWordDiv);

			return (int)Opt.strWordDiv.GetLength() + 1;
		}
		/*
			$ 24.08.2000 SVS
			ожидать определенную (или любую) клавишу
			(int)Param1 - внутренний код клавиши, которую ожидаем, или -1
			если все равно какую клавишу ждать.
			возвращает 0;
		*/
		case ACTL_WAITKEY: {
			return WaitKey(Param1 ? (DWORD)(DWORD_PTR)Param1 : (DWORD)-1, 0, false);
		}
		/*
			$ 04.12.2000 SVS
			ACTL_GETCOLOR - получить определенный цвет по индекс, определенному
			в farcolor.hpp
			(int)Param1 - индекс.
			(uint64_t *)Param2 - адрес для получения цвета.
			Return - значение цвета или -1 если индекс неверен.

		*/
		case ACTL_GETCOLOR: {
			if ((int)(INT_PTR)Param1 < SIZE_ARRAY_PALETTE && (int)(INT_PTR)Param1 >= 0) {

				*(uint64_t *)Param2 = (uint64_t)Palette[(int)(INT_PTR)Param1];
				return TRUE;
			}

			return FALSE;
		}
		/*
			ACTL_GETARRAYCOLOR - получить весь массив цветов
			Param1 - размер буфера (в элементах FarColor)
			Param2 - указатель на буфер или nullptr, чтобы получить необходимый размер
			Return - размер массива.
		*/
		case ACTL_GETARRAYCOLOR: {
			if ((int)(intptr_t)Param1 > SIZE_ARRAY_PALETTE)
				return SIZE_ARRAY_PALETTE;

			if (Param2)
				memcpy(Param2, Palette, (int)(intptr_t)Param1 * sizeof(Palette[0]));

			return SIZE_ARRAY_PALETTE;
		}
		/*
			Param1=FARColor{
				DWORD Flags;
				int StartIndex;
				int ColorItem;
				//LPBYTE Colors;
				uint64_t *Colors;
			};
		*/
		case ACTL_SETARRAYCOLOR: {
			if (Param1) {
				FarSetColors *Pal = (FarSetColors *)Param1;

				if (Pal->Colors && Pal->StartIndex >= 0
						&& Pal->StartIndex + Pal->ColorCount <= SIZE_ARRAY_PALETTE) {
					memmove(Palette + Pal->StartIndex, Pal->Colors, Pal->ColorCount * sizeof(Palette[0]));

					if (Pal->Flags & FCLR_REDRAW) {
						ScrBuf.Lock();					// отменяем всякую прорисовку
						FrameManager->ResizeAllFrame();
						FrameManager->PluginCommit();	// коммитим.
						ScrBuf.Unlock();				// разрешаем прорисовку
					}

					return TRUE;
				}
			}

			return FALSE;
		}
		/*
			$ 14.12.2000 SVS
			ACTL_EJECTMEDIA - извлечь диск из съемного накопителя
			Param1 - указатель на структуру ActlEjectMedia
			Return - TRUE - успешное извлечение, FALSE - ошибка.
		*/
		case ACTL_EJECTMEDIA: {
			return FALSE;	/*Param1?EjectVolume((wchar_t)((ActlEjectMedia*)Param1)->Letter,
							   ((ActlEjectMedia*)Param1)->Flags):FALSE;*/
							/*
								if(Param1)
								{
									ActlEjectMedia *aem=(ActlEjectMedia *)Param1;
									char DiskLetter[4]=" :/";
									DiskLetter[0]=(char)aem->Letter;
									int DriveType = FAR_GetDriveType(DiskLetter,nullptr,FALSE); // здесь не определяем тип CD
				
									if(DriveType == DRIVE_USBDRIVE && RemoveUSBDrive((char)aem->Letter,aem->Flags))
										return TRUE;
									if(DriveType == DRIVE_SUBSTITUTE && DelSubstDrive(DiskLetter))
										return TRUE;
									if(IsDriveTypeCDROM(DriveType) && EjectVolume((char)aem->Letter,aem->Flags))
										return TRUE;
				
								}
								return FALSE;
							*/
		}
		/*
			case ACTL_GETMEDIATYPE:
			{
				ActlMediaType *amt=(ActlMediaType *)Param1;
				char DiskLetter[4]=" :/";
				DiskLetter[0]=(amt)?(char)amt->Letter:0;
				return FAR_GetDriveType(DiskLetter,nullptr,(amt && !(amt->Flags&MEDIATYPE_NODETECTCDROM)?TRUE:FALSE));
			}
		*/
		/*
			$ 21.12.2000 SVS
			Macro API
		*/
		case ACTL_KEYMACRO: {
			if (CtrlObject && Param1)					// все зависит от этой бадяги.
			{
				KeyMacro &Macro = CtrlObject->Macro;	//??
				ActlKeyMacro *KeyMacro = (ActlKeyMacro *)Param1;

				switch (KeyMacro->Command) {
					case MCMD_LOADALL:		// из реестра в память ФАР с затиранием предыдущего
					{
						if (Macro.IsRecording())
							return FALSE;

						return Macro.LoadMacros(!Macro.IsExecuting());
					}
					case MCMD_SAVEALL:				// из памяти ФАРа в реестра
					{
						if (Macro.IsRecording())	// || Macro.IsExecuting())
							return FALSE;

						Macro.SaveMacros();
						return TRUE;
					}
					case MCMD_POSTMACROSTRING: {
						if (KeyMacro->Param.PlainText.SequenceText && *KeyMacro->Param.PlainText.SequenceText)
							return Macro.PostNewMacro(KeyMacro->Param.PlainText.SequenceText,
									KeyMacro->Param.PlainText.Flags << 8, KeyMacro->Param.PlainText.AKey);

						return FALSE;
					}
					case MCMD_RUNMACROSTRING: {
						return FALSE;
					}
					case MCMD_CHECKMACRO:		// проверка макроса
					{
						MacroRecord CurMacro{};
						int Ret = Macro.ParseMacroString(&CurMacro, KeyMacro->Param.PlainText.SequenceText,
								(KeyMacro->Param.PlainText.Flags & KSFLAGS_SILENTCHECK) ? TRUE : FALSE);

						if (Ret) {
							if (CurMacro.BufferSize > 1)
								free(CurMacro.Buffer);
							memset(&KeyMacro->Param.MacroResult, 0, sizeof(struct MacroParseResult));
						} else {
							static FARString ErrSrc;
							Macro.GetMacroParseError(&KeyMacro->Param.MacroResult.ErrCode,
									&KeyMacro->Param.MacroResult.ErrPos, &ErrSrc);
							KeyMacro->Param.MacroResult.ErrSrc = ErrSrc;
						}

						return Ret;
					}
					case MCMD_GETSTATE: {
						return Macro.GetCurRecord(nullptr, nullptr);
					}

					case MCMD_GETAREA: {
						return Macro.GetMode();
					}
#if 0
					case MCMD_COMPILEMACRO:
					{
						MacroRecord CurMacro={0};
						int Ret=Macro.ParseMacroString(&CurMacro,KeyMacro->Param1.PlainText.SequenceText);

						if (Ret)
						{
							//KeyMacro->Param1s.Compile.Flags=CurMacro.Flags;
							KeyMacro->Param1.Compile.Sequence=CurMacro.Buffer;
							KeyMacro->Param1.Compile.Count=CurMacro.BufferSize;
						}

						return Ret;
					}
#endif
				}
			}

			return FALSE;
		}
		case ACTL_POSTKEYSEQUENCE: {
			if (CtrlObject && Param1 && ((KeySequence *)Param1)->Count > 0) {
				MacroRecord MRec{};
				MRec.Flags = (((KeySequence *)Param1)->Flags) << 8;
				MRec.BufferSize = ((KeySequence *)Param1)->Count;

				if (MRec.BufferSize == 1)
					MRec.Buffer = (DWORD *)(DWORD_PTR)((KeySequence *)Param1)->Sequence[0];
				else
					MRec.Buffer = const_cast<DWORD *>(((KeySequence *)Param1)->Sequence);

				return CtrlObject->Macro.PostNewMacro(&MRec, TRUE, TRUE);
#if 0
				// Этот кусок - для дальнейших экспериментов
				{
					//CtrlObject->Macro.PostNewMacro(&MRec);
					for (int I=0; I < MRec.BufferSize; ++I)
					{
						int Key=MRec.Buffer[I];

						if (CtrlObject->Macro.ProcessKey(Key))
						{
							while ((Key=CtrlObject->Macro.GetKey()) )
							{
								FrameManager->ProcessKey(Key);
							}
						}
						else
							FrameManager->ProcessKey(Key);

						FrameManager->PluginCommit();
					}

					return TRUE;
				}
#endif
			}

			return FALSE;
		}
		/*
			$ 05.06.2001 tran
			новые ACTL_ для работы с фреймами
		*/
		case ACTL_GETWINDOWINFO:
			/*
				$ 12.04.2005 AY
				thread safe window info
			*/
		case ACTL_GETSHORTWINDOWINFO: {
			if (FrameManager && Param1) {
				FARString strType, strName;
				WindowInfo *wi = (WindowInfo *)Param1;
				Frame *f;

				/*
					$ 22.12.2001 VVM
					+ Если Pos == -1 то берем текущий фрейм
				*/
				if (wi->Pos == -1)
					f = FrameManager->GetCurrentFrame();
				else
					f = FrameManager->operator[](wi->Pos);

				if (!f)
					return FALSE;

				if (Command == ACTL_GETWINDOWINFO) {
					f->GetTypeAndName(strType, strName);

					if (wi->TypeNameSize && wi->TypeName) {
						far_wcsncpy(wi->TypeName, strType, wi->TypeNameSize);
					} else {
						wi->TypeNameSize = static_cast<int>(strType.GetLength() + 1);
					}

					if (wi->NameSize && wi->Name) {
						far_wcsncpy(wi->Name, strName, wi->NameSize);
					} else {
						wi->NameSize = static_cast<int>(strName.GetLength() + 1);
					}
				} else {
					wi->TypeName = nullptr;
					wi->Name = nullptr;
					wi->NameSize = 0;
					wi->TypeName = 0;
				}

				wi->Pos = FrameManager->IndexOf(f);
				wi->Type = f->GetType();
				wi->Modified = f->IsFileModified();
				wi->Current = f == FrameManager->GetCurrentFrame();
				return TRUE;
			}

			return FALSE;
		}
		case ACTL_GETWINDOWCOUNT: {
			return FrameManager ? FrameManager->GetFrameCount() : 0;
		}
		case ACTL_SETCURRENTWINDOW: {
			// Запретим переключение фрэймов, если находимся в модальном редакторе/вьюере.
			if (FrameManager && !FrameManager->InModalEV() && FrameManager->operator[]((int)(INT_PTR)Param1)) {
				int TypeFrame = FrameManager->GetCurrentFrame()->GetType();

				// Запретим переключение фрэймов, если находимся в хелпе или диалоге (тоже модальных)
				if (TypeFrame != MODALTYPE_HELP && TypeFrame != MODALTYPE_DIALOG) {
					Frame *PrevFrame = FrameManager->GetCurrentFrame();
					FrameManager->ActivateFrame((int)(INT_PTR)Param1);
					FrameManager->DeactivateFrame(PrevFrame, 0);
					return TRUE;
				}
			}

			return FALSE;
		}
		/*
			$ 26.06.2001 SKV
			Для полноценной работы с ACTL_SETCURRENTWINDOW
			(и может еще для чего в будущем)
		*/
		case ACTL_COMMIT: {
			return FrameManager ? FrameManager->PluginCommit() : FALSE;
		}
		/*
			$ 15.09.2001 tran
			пригодится плагинам
		*/
		case ACTL_GETFARHWND: {
			return 0;
		}
		case ACTL_GETDIALOGSETTINGS: {
			DWORD Options = 0;
			static Opt2Flags ODlg[] = {
					{&Opt.Dialogs.EditHistory,      FDIS_HISTORYINDIALOGEDITCONTROLS   },
					{&Opt.Dialogs.EditBlock,        FDIS_PERSISTENTBLOCKSINEDITCONTROLS},
					{&Opt.Dialogs.AutoComplete,     FDIS_AUTOCOMPLETEININPUTLINES      },
					{&Opt.Dialogs.EULBsClear,       FDIS_BSDELETEUNCHANGEDTEXT         },
					{&Opt.Dialogs.DelRemovesBlocks, FDIS_DELREMOVESBLOCKS              },
					{&Opt.Dialogs.MouseButton,      FDIS_MOUSECLICKOUTSIDECLOSESDIALOG },
			};

			for (size_t I = 0; I < ARRAYSIZE(ODlg); ++I)
				if (*ODlg[I].Opt)
					Options|= ODlg[I].Flags;

			return Options;
		}
		/*
			$ 24.11.2001 IS
			Ознакомим с настройками системными, панели, интерфейса, подтверждений
		*/
		case ACTL_GETSYSTEMSETTINGS: {
			DWORD Options = 0;
			static Opt2Flags OSys[] = {
					{&Opt.DeleteToRecycleBin, FSS_DELETETORECYCLEBIN    },
					{&Opt.CMOpt.WriteThrough, FSS_WRITETHROUGH          },
					{&Opt.ScanJunction,       FSS_SCANSYMLINK           },
					{&Opt.SaveHistory,        FSS_SAVECOMMANDSHISTORY   },
					{&Opt.SaveFoldersHistory, FSS_SAVEFOLDERSHISTORY    },
					{&Opt.SaveViewHistory,    FSS_SAVEVIEWANDEDITHISTORY},
					{&Opt.AutoSaveSetup,      FSS_AUTOSAVESETUP         },
			};

			for (size_t I = 0; I < ARRAYSIZE(OSys); ++I)
				if (*OSys[I].Opt)
					Options|= OSys[I].Flags;

			return Options;
		}
		case ACTL_GETPANELSETTINGS: {
			DWORD Options = 0;
			static Opt2Flags OSys[] = {
					{&Opt.ShowHidden,            FPS_SHOWHIDDENANDSYSTEMFILES   },
					{&Opt.Highlight,             FPS_HIGHLIGHTFILES             },
					{&Opt.Tree.AutoChangeFolder, FPS_AUTOCHANGEFOLDER           },
					{&Opt.SelectFolders,         FPS_SELECTFOLDERS              },
					{&Opt.ReverseSort,           FPS_ALLOWREVERSESORTMODES      },
					{&Opt.ShowColumnTitles,      FPS_SHOWCOLUMNTITLES           },
					{&Opt.ShowPanelStatus,       FPS_SHOWSTATUSLINE             },
					{&Opt.ShowPanelTotals,       FPS_SHOWFILESTOTALINFORMATION  },
					{&Opt.ShowPanelFree,         FPS_SHOWFREESIZE               },
					{&Opt.ShowPanelScrollbar,    FPS_SHOWSCROLLBAR              },
					{&Opt.ShowScreensNumber,     FPS_SHOWBACKGROUNDSCREENSNUMBER},
					{&Opt.ShowSortMode,          FPS_SHOWSORTMODELETTER         },
			};

			for (size_t I = 0; I < ARRAYSIZE(OSys); ++I)
				if (*OSys[I].Opt)
					Options|= OSys[I].Flags;

			return Options;
		}
		case ACTL_GETINTERFACESETTINGS: {
			DWORD Options = 0;
			static Opt2Flags OSys[] = {
					{&Opt.Clock,               FIS_CLOCKINPANELS                 },
					{&Opt.ViewerEditorClock,   FIS_CLOCKINVIEWERANDEDITOR        },
					{&Opt.Mouse,               FIS_MOUSE                         },
					{&Opt.ShowKeyBar,          FIS_SHOWKEYBAR                    },
					{&Opt.ShowMenuBar,         FIS_ALWAYSSHOWMENUBAR             },
					{&Opt.CMOpt.CopyShowTotal, FIS_SHOWTOTALCOPYPROGRESSINDICATOR},
					{&Opt.CMOpt.CopyTimeRule,  FIS_SHOWCOPYINGTIMEINFO           },
					{&Opt.PgUpChangeDisk,      FIS_USECTRLPGUPTOCHANGEDRIVE      },
					{&Opt.DelOpt.DelShowTotal, FIS_SHOWTOTALDELPROGRESSINDICATOR },
			};

			for (size_t I = 0; I < ARRAYSIZE(OSys); ++I)
				if (*OSys[I].Opt)
					Options|= OSys[I].Flags;

			return Options;
		}
		case ACTL_GETCONFIRMATIONS: {
			DWORD Options = 0;
			static Opt2Flags OSys[] = {
					{&Opt.Confirm.Copy,             FCS_COPYOVERWRITE         },
					{&Opt.Confirm.Move,             FCS_MOVEOVERWRITE         },
					{&Opt.Confirm.RO,               FCS_OVERWRITEDELETEROFILES},
					{&Opt.Confirm.Drag,             FCS_DRAGANDDROP           },
					{&Opt.Confirm.Delete,           FCS_DELETE                },
					{&Opt.Confirm.DeleteFolder,     FCS_DELETENONEMPTYFOLDERS },
					{&Opt.Confirm.Esc,              FCS_INTERRUPTOPERATION    },
					{&Opt.Confirm.RemoveConnection, FCS_DISCONNECTNETWORKDRIVE},
					{&Opt.Confirm.AllowReedit,      FCS_RELOADEDITEDFILE      },
					{&Opt.Confirm.HistoryClear,     FCS_CLEARHISTORYLIST      },
					{&Opt.Confirm.ExitEffective(),  FCS_EXIT                  },
			};

			for (size_t I = 0; I < ARRAYSIZE(OSys); ++I)
				if (*OSys[I].Opt)
					Options|= OSys[I].Flags;

			return Options;
		}
		case ACTL_GETDESCSETTINGS: {
			// опций мало - с массивом не заморачиваемся
			DWORD Options = 0;

			if (Opt.Diz.UpdateMode == DIZ_UPDATE_IF_DISPLAYED)
				Options|= FDS_UPDATEIFDISPLAYED;
			else if (Opt.Diz.UpdateMode == DIZ_UPDATE_ALWAYS)
				Options|= FDS_UPDATEALWAYS;

			if (Opt.Diz.SetHidden)
				Options|= FDS_SETHIDDEN;

			if (Opt.Diz.ROUpdate)
				Options|= FDS_UPDATEREADONLY;

			return Options;
		}
		case ACTL_GETPOLICIES: {
			return Opt.Policies.DisabledOptions
					| (Opt.Policies.ShowHiddenDrives ? FFPOL_SHOWHIDDENDRIVES : 0);
		}
		case ACTL_REDRAWALL: {
			if (!FrameManager)
				return TRUE;

			int Ret = FrameManager->ProcessKey(KEY_CONSOLE_BUFFER_RESIZE);
			FrameManager->PluginCommit();
			return Ret;
		}

		case ACTL_SETPROGRESSSTATE: {
			return TRUE;
		}

		case ACTL_SETPROGRESSVALUE: {
			BOOL Result = FALSE;
			if (Param1) {
				// PROGRESSVALUE* PV=reinterpret_cast<PROGRESSVALUE*>(Param1);
				Result = TRUE;
			}
			return Result;
		}

		case ACTL_QUIT: {
			CloseFARMenu = TRUE;
			if (FrameManager)
				FrameManager->ExitMainLoop(FALSE);
			return TRUE;
		}

		case ACTL_GETFARRECT: {
			BOOL Result = FALSE;
			if (Param1) {
				SMALL_RECT &Rect = *reinterpret_cast<PSMALL_RECT>(Param1);
				COORD Size;
				if (Console.GetSize(Size)) {
					Rect.Left = 0;
					Rect.Top = 0;
					Rect.Right = Size.X - 1;
					Rect.Bottom = Size.Y - 1;
					Result = TRUE;
				}
			}
			return Result;
		} break;

		case ACTL_GETCURSORPOS: {
			BOOL Result = FALSE;
			if (Param1) {
				COORD &Pos = *reinterpret_cast<PCOORD>(Param1);
				Result = Console.GetCursorPosition(Pos);
			}
			return Result;
		} break;

		case ACTL_SETCURSORPOS: {
			BOOL Result = FALSE;
			if (Param1) {
				COORD &Pos = *reinterpret_cast<PCOORD>(Param1);
				Result = Console.SetCursorPosition(Pos);
			}
			return Result;
		} break;

		case ACTL_PROGRESSNOTIFY: {
			return TRUE;
		}
	}

	return FALSE;
}

INT_PTR WINAPI FarAdvControl(INT_PTR ModuleNumber, int Command, void *Param1, void *Param2)
{
	return InterThreadCall<LONG_PTR, 0>(std::bind(FarAdvControlSynched, ModuleNumber, Command, Param1, Param2));
}

static int FarMenuFnSynched(INT_PTR PluginNumber, int X, int Y, int MaxHeight, DWORD Flags,
		const wchar_t *Title, const wchar_t *Bottom, const wchar_t *HelpTopic, const int *BreakKeys,
		int *BreakCode, const FarMenuItem *Item, int ItemsNumber)
{
	if (FrameManager->ManagerIsDown())
		return -1;

	if (DisablePluginsOutput)
		return -1;

	int ExitCode;
	{
		VMenu FarMenu(Title, nullptr, 0, MaxHeight);
		CtrlObject->Macro.SetMode(MACRO_MENU);
		FarMenu.SetPosition(X, Y, 0, 0);

		if (BreakCode)
			*BreakCode = -1;

		{
			FARString strTopic;

			if (Help::MkTopic(PluginNumber, HelpTopic, strTopic))
				FarMenu.SetHelp(strTopic);
		}

		if (Bottom)
			FarMenu.SetBottomTitle(Bottom);

		// общие флаги меню
		DWORD MenuFlags = 0;

		if (Flags & FMENU_SHOWAMPERSAND)
			MenuFlags|= VMENU_SHOWAMPERSAND;

		if (Flags & FMENU_WRAPMODE)
			MenuFlags|= VMENU_WRAPMODE;

		if (Flags & FMENU_CHANGECONSOLETITLE)
			MenuFlags|= VMENU_CHANGECONSOLETITLE;

		FarMenu.SetFlags(MenuFlags);
		MenuItemEx CurItem;
		CurItem.Clear();
		int Selected = 0;

		if (Flags & FMENU_USEEXT) {
			FarMenuItemEx *ItemEx = (FarMenuItemEx *)Item;

			for (int i = 0; i < ItemsNumber; i++) {
				CurItem.Flags = ItemEx[i].Flags;
				CurItem.strName.Clear();
				// исключаем MultiSelected, т.к. у нас сейчас движок к этому не приспособлен, оставляем только первый
				DWORD SelCurItem = CurItem.Flags & LIF_SELECTED;
				CurItem.Flags&= ~LIF_SELECTED;

				if (!Selected && !(CurItem.Flags & LIF_SEPARATOR) && SelCurItem) {
					CurItem.Flags|= SelCurItem;
					Selected++;
				}

				CurItem.strName = ItemEx[i].Text;
				CurItem.AccelKey = (CurItem.Flags & LIF_SEPARATOR) ? 0 : ItemEx[i].AccelKey;
				FarMenu.AddItem(&CurItem);
			}
		} else {
			for (int i = 0; i < ItemsNumber; i++) {
				CurItem.Flags = Item[i].Checked ? (LIF_CHECKED | (Item[i].Checked & 0xFFFF)) : 0;
				CurItem.Flags|= Item[i].Selected ? LIF_SELECTED : 0;
				CurItem.Flags|= Item[i].Separator ? LIF_SEPARATOR : 0;

				if (Item[i].Separator)
					CurItem.strName.Clear();
				else
					CurItem.strName = Item[i].Text;

				DWORD SelCurItem = CurItem.Flags & LIF_SELECTED;
				CurItem.Flags&= ~LIF_SELECTED;

				if (!Selected && !(CurItem.Flags & LIF_SEPARATOR) && SelCurItem) {
					CurItem.Flags|= SelCurItem;
					Selected++;
				}

				FarMenu.AddItem(&CurItem);
			}
		}

		if (!Selected)
			FarMenu.SetSelectPos(0, 1);

		// флаги меню, с забитым контентом
		if (Flags & FMENU_AUTOHIGHLIGHT)
			FarMenu.AssignHighlights(FALSE);

		if (Flags & FMENU_REVERSEAUTOHIGHLIGHT)
			FarMenu.AssignHighlights(TRUE);

		FarMenu.SetTitle(Title);
		FarMenu.Show();

		while (!FarMenu.Done() && !CloseFARMenu) {
			INPUT_RECORD ReadRec;
			const auto ReadKey = GetInputRecord(&ReadRec);

			if (ReadKey == KEY_CONSOLE_BUFFER_RESIZE) {
				LockScreen LckScr;
				FarMenu.Hide();
				FarMenu.Show();
			} else if (ReadRec.EventType == MOUSE_EVENT) {
				FarMenu.ProcessMouse(&ReadRec.Event.MouseEvent);
			} else if (ReadKey != KEY_NONE) {
				if (BreakKeys) {
					for (int I = 0; BreakKeys[I]; I++) {
						if (CtrlObject->Macro.IsExecuting()) {
							int VirtKey, ControlState;
							TranslateKeyToVK(ReadKey, VirtKey, ControlState, &ReadRec);
						}

						if (ReadRec.Event.KeyEvent.wVirtualKeyCode == (BreakKeys[I] & 0xffff)) {
							DWORD Flags = BreakKeys[I] >> 16;
							DWORD RealFlags = ReadRec.Event.KeyEvent.dwControlKeyState
									& (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED | LEFT_ALT_PRESSED
											| RIGHT_ALT_PRESSED | SHIFT_PRESSED);
							DWORD f = 0;

							if (RealFlags & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
								f|= PKF_CONTROL;

							if (RealFlags & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
								f|= PKF_ALT;

							if (RealFlags & SHIFT_PRESSED)
								f|= PKF_SHIFT;

							if (f == Flags) {
								if (BreakCode)
									*BreakCode = I;

								FarMenu.Hide();
								//								CheckScreenLock();
								return FarMenu.GetSelectPos();
							}
						}
					}
				}

				FarMenu.ProcessKey(ReadKey);
			}
		}

		ExitCode = FarMenu.Modal::GetExitCode();
	}
	//	CheckScreenLock();
	return (ExitCode);
}

int WINAPI FarMenuFn(INT_PTR PluginNumber, int X, int Y, int MaxHeight, DWORD Flags, const wchar_t *Title,
		const wchar_t *Bottom, const wchar_t *HelpTopic, const int *BreakKeys, int *BreakCode,
		const FarMenuItem *Item, int ItemsNumber)
{
	return InterThreadCall<int, -1>(std::bind(FarMenuFnSynched, PluginNumber, X, Y, MaxHeight, Flags, Title,
			Bottom, HelpTopic, BreakKeys, BreakCode, Item, ItemsNumber));
}

// Функция FarDefDlgProc обработки диалога по умолчанию
LONG_PTR WINAPI FarDefDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	if (hDlg)	// исключаем лишний вызов для hDlg=0
		return DefDlgProc(hDlg, Msg, Param1, Param2);

	return 0;
}

// Посылка сообщения диалогу
LONG_PTR WINAPI FarSendDlgMessage(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	if (hDlg)	// исключаем лишний вызов для hDlg=0
		return SendDlgMessage(hDlg, Msg, Param1, Param2);

	return 0;
}

static HANDLE FarDialogInitSynched(INT_PTR PluginNumber, int X1, int Y1, int X2, int Y2,
		const wchar_t *HelpTopic, FarDialogItem *Item, unsigned int ItemsNumber, DWORD Reserved, DWORD Flags,
		FARWINDOWPROC DlgProc, LONG_PTR Param)
{
	HANDLE hDlg = INVALID_HANDLE_VALUE;

	if (FrameManager->ManagerIsDown())
		return hDlg;

	if (DisablePluginsOutput || ItemsNumber <= 0 || !Item)
		return hDlg;

	// ФИЧА! нельзя указывать отрицательные X2 и Y2
	if (X2 < 0 || Y2 < 0)
		return hDlg;

	{
		Dialog *FarDialog = new (std::nothrow) Dialog(Item, ItemsNumber, DlgProc, Param);

		if (!FarDialog)
			return hDlg;

		hDlg = (HANDLE)FarDialog;
		FarDialog->SetPosition(X1, Y1, X2, Y2);

		if (Flags & FDLG_WARNING)
			FarDialog->SetDialogMode(DMODE_WARNINGSTYLE);

		if (Flags & FDLG_SMALLDIALOG)
			FarDialog->SetDialogMode(DMODE_SMALLDIALOG);

		if (Flags & FDLG_NODRAWSHADOW)
			FarDialog->SetDialogMode(DMODE_NODRAWSHADOW);

		if (Flags & FDLG_NODRAWPANEL)
			FarDialog->SetDialogMode(DMODE_NODRAWPANEL);

		if (Flags & FDLG_KEEPCONSOLETITLE)
			FarDialog->SetDialogMode(DMODE_KEEPCONSOLETITLE);

		if (Flags & FDLG_NONMODAL)
			FarDialog->SetCanLoseFocus(TRUE);

		if (Flags & FDLG_REGULARIDLE)
			FarDialog->SetRegularIdle(TRUE);

		FarDialog->SetHelp(HelpTopic);
		/*
			$ 29.08.2000 SVS
			Запомним номер плагина - сейчас в основном для формирования HelpTopic
		*/
		FarDialog->SetPluginNumber(PluginNumber);
	}
	return hDlg;
}

static int FarDialogRunSynched(HANDLE hDlg)
{
	if (hDlg == INVALID_HANDLE_VALUE)
		return -1;

	if (FrameManager->ManagerIsDown())
		return -1;

	int ExitCode;

	{
		Dialog *FarDialog = (Dialog *)hDlg;
		LockBottomFrame lbf;	// временно отменим прорисовку фрейма
		// CtrlObject->Plugins.Flags.Clear(PSIF_DIALOG);
		FarDialog->Process();
		ExitCode = FarDialog->GetExitCode();
	}

	// CheckScreenLock();
	FrameManager->RefreshFrame();	//?? - //AY - это нужно чтоб обновлять панели после выхода из диалога
	return (ExitCode);
}

static bool FarDialogFreeSynched(HANDLE hDlg)
{
	if (hDlg == INVALID_HANDLE_VALUE)
		return false;

	Dialog *FarDialog = (Dialog *)hDlg;
	delete FarDialog;
	return true;
}

/////////

HANDLE WINAPI FarDialogInit(INT_PTR PluginNumber, int X1, int Y1, int X2, int Y2, const wchar_t *HelpTopic,
		FarDialogItem *Item, unsigned int ItemsNumber, DWORD Reserved, DWORD Flags, FARWINDOWPROC DlgProc,
		LONG_PTR Param)
{
	HANDLE out = InterThreadCall<HANDLE, nullptr>(std::bind(FarDialogInitSynched, PluginNumber, X1, Y1, X2,
			Y2, HelpTopic, Item, ItemsNumber, Reserved, Flags, DlgProc, Param));

	return (out != nullptr) ? out : INVALID_HANDLE_VALUE;
}

int WINAPI FarDialogRun(HANDLE hDlg)
{
	return InterThreadCall<int, -1>(std::bind(FarDialogRunSynched, hDlg));
}

void WINAPI FarDialogFree(HANDLE hDlg)
{
	InterThreadCall<bool, false>(std::bind(FarDialogFreeSynched, hDlg));
}

/////////

static CriticalSection s_get_msg_cs;
const wchar_t *FarGetMsgFn(INT_PTR PluginHandle, FarLangMsgID MsgId)
{
	// BUGBUG, надо проверять, что PluginHandle - плагин
	PluginW *pPlugin = (PluginW *)PluginHandle;
	std::wstring strPath = pPlugin->GetModuleName().CPtr();
	CutToSlash(strPath);

	CriticalSectionLock lock(s_get_msg_cs);
	if (!pPlugin->InitLang(strPath.c_str())) {
		return nullptr;
	}

	return pPlugin->GetMsg(MsgId);
}

static int FarMessageFnSynched(INT_PTR PluginNumber, DWORD Flags, const wchar_t *HelpTopic,
		const wchar_t *const *Items, int ItemsNumber, int ButtonsNumber)
{
	if (FrameManager->ManagerIsDown())
		return -1;

	if (DisablePluginsOutput)
		return -1;

	if ((!(Flags & (FMSG_ALLINONE | FMSG_ERRORTYPE)) && ItemsNumber < 2) || !Items)
		return -1;

	std::list<std::wstring> AllInOneParts;
	Messager m;
	if (Flags & FMSG_ALLINONE) {
		const wchar_t *Msg = (const wchar_t *)Items, *Edge;

		while ((Edge = wcschr(Msg, L'\n')) != nullptr) {
			AllInOneParts.emplace_back(Msg, Edge - Msg);
			m.Add(AllInOneParts.back().c_str());
			Msg = Edge + 1;
		}
		if (*Msg)
			m.Add(Msg);
	} else {
		for (int i = 0; i < ItemsNumber; i++)
			m.Add(Items[i]);
	}

	/*
		$ 22.03.2001 tran
		ItemsNumber++ -> ++ItemsNumber
		тереялся последний элемент
	*/
	switch (Flags & 0x000F0000) {
		case FMSG_MB_OK:
			ButtonsNumber = 1;
			m.Add(Msg::Ok);
			break;
		case FMSG_MB_OKCANCEL:
			ButtonsNumber = 2;
			m.Add(Msg::Ok);
			m.Add(Msg::Cancel);
			break;
		case FMSG_MB_ABORTRETRYIGNORE:
			ButtonsNumber = 3;
			m.Add(Msg::Abort);
			m.Add(Msg::Retry);
			m.Add(Msg::Ignore);
			break;
		case FMSG_MB_YESNO:
			ButtonsNumber = 2;
			m.Add(Msg::Yes);
			m.Add(Msg::No);
			break;
		case FMSG_MB_YESNOCANCEL:
			ButtonsNumber = 3;
			m.Add(Msg::Yes);
			m.Add(Msg::No);
			m.Add(Msg::Cancel);
			break;
		case FMSG_MB_RETRYCANCEL:
			ButtonsNumber = 2;
			m.Add(Msg::Retry);
			m.Add(Msg::Cancel);
			break;
	}

	// запоминаем топик
	if (PluginNumber != -1) {
		FARString strTopic;

		if (Help::MkTopic(PluginNumber, HelpTopic, strTopic))
			SetMessageHelp(strTopic);
	}

	// непосредственно... вывод
	LockBottomFrame lbf;	// временно отменим прорисовку фрейма
	return m.Show(Flags, ButtonsNumber, PluginNumber);
}

int WINAPI FarMessageFn(INT_PTR PluginNumber, DWORD Flags, const wchar_t *HelpTopic,
		const wchar_t *const *Items, int ItemsNumber, int ButtonsNumber)
{
	return InterThreadCall<int, -1>(std::bind(FarMessageFnSynched, PluginNumber, Flags, HelpTopic, Items,
			ItemsNumber, ButtonsNumber));
}

static int FarControlSynched(HANDLE hPlugin, int Command, int Param1, LONG_PTR Param2)
{
	_FCTLLOG(CleverSysLog CSL(L"Control"));
	_FCTLLOG(SysLog(L"(hPlugin=0x%08X, Command=%ls, Param1=[%d/0x%08X], Param2=[%d/0x%08X])", hPlugin,
			_FCTL_ToName(Command), (int)Param1, Param1, (int)Param2, Param2));
	_ALGO(CleverSysLog clv(L"FarControl"));
	_ALGO(SysLog(L"(hPlugin=0x%08X, Command=%ls, Param1=[%d/0x%08X], Param2=[%d/0x%08X])", hPlugin,
			_FCTL_ToName(Command), (int)Param1, Param1, (int)Param2, Param2));

	if (Command == FCTL_CHECKPANELSEXIST)
		return Opt.OnlyEditorViewerUsed ? FALSE : TRUE;

	if (Opt.OnlyEditorViewerUsed || !CtrlObject || !FrameManager || FrameManager->ManagerIsDown())
		return 0;

	FilePanels *FPanels = CtrlObject->Cp();
	CommandLine *CmdLine = CtrlObject->CmdLine;

	switch (Command) {
		case FCTL_CLOSEPLUGIN:
			g_strDirToSet = (wchar_t *)Param2;
		case FCTL_GETPANELINFO:
		case FCTL_GETPANELITEM:
		case FCTL_GETSELECTEDPANELITEM:
		case FCTL_GETCURRENTPANELITEM:
		case FCTL_GETPANELDIR:
		case FCTL_GETCOLUMNTYPES:
		case FCTL_GETCOLUMNWIDTHS:
		case FCTL_UPDATEPANEL:
		case FCTL_REDRAWPANEL:
		case FCTL_SETPANELDIR:
		case FCTL_BEGINSELECTION:
		case FCTL_SETSELECTION:
		case FCTL_CLEARSELECTION:
		case FCTL_ENDSELECTION:
		case FCTL_SETVIEWMODE:
		case FCTL_SETSORTMODE:
		case FCTL_SETSORTORDER:
		case FCTL_SETNUMERICSORT:
		case FCTL_SETCASESENSITIVESORT:
		case FCTL_SETDIRECTORIESFIRST:
		case FCTL_GETPANELFORMAT:
		case FCTL_GETPANELHOSTFILE:
		case FCTL_GETPANELPLUGINHANDLE:
		case FCTL_SETPANELLOCATION: {
			if (!FPanels)
				return FALSE;

			if ((hPlugin == PANEL_ACTIVE) || (hPlugin == PANEL_PASSIVE)) {
				Panel *pPanel = (hPlugin == PANEL_ACTIVE)
						? FPanels->ActivePanel
						: FPanels->GetAnotherPanel(FPanels->ActivePanel);

				if (pPanel) {
					return pPanel->SetPluginCommand(Command, Param1, Param2);
				}

				return FALSE;	//???
			}

			HANDLE hInternal;
			Panel *LeftPanel = FPanels->LeftPanel;
			Panel *RightPanel = FPanels->RightPanel;
			int Processed = FALSE;
			PluginHandle *PlHandle;

			if (LeftPanel && LeftPanel->GetMode() == PLUGIN_PANEL) {
				PlHandle = (PluginHandle *)LeftPanel->GetPluginHandle();

				if (PlHandle) {
					hInternal = PlHandle->hPlugin;

					if (hPlugin == hInternal) {
						Processed = LeftPanel->SetPluginCommand(Command, Param1, Param2);
					}
				}
			}

			if (RightPanel && RightPanel->GetMode() == PLUGIN_PANEL) {
				PlHandle = (PluginHandle *)RightPanel->GetPluginHandle();

				if (PlHandle) {
					hInternal = PlHandle->hPlugin;

					if (hPlugin == hInternal) {
						Processed = RightPanel->SetPluginCommand(Command, Param1, Param2);
					}
				}
			}

			return (Processed);
		}
		case FCTL_SETUSERSCREEN: {
			if (!FPanels || !FPanels->LeftPanel || !FPanels->RightPanel)
				return FALSE;

			KeepUserScreen++;
			FPanels->LeftPanel->ProcessingPluginCommand++;
			FPanels->RightPanel->ProcessingPluginCommand++;
			ScrBuf.FillBuf();
			ScrollScreen(1);
			SaveScreen SaveScr;
			{
				RedrawDesktop Redraw;
				CmdLine->Hide();
				SaveScr.RestoreArea(FALSE);
			}
			KeepUserScreen--;
			FPanels->LeftPanel->ProcessingPluginCommand--;
			FPanels->RightPanel->ProcessingPluginCommand--;
			return TRUE;
		}
		case FCTL_GETUSERSCREEN: {
			FrameManager->ShowBackground();
			int Lock = ScrBuf.GetLockCount();
			ScrBuf.SetLockCount(0);
			MoveCursor(0, ScrY - 1);
			SetInitialCursorType();
			ScrBuf.Flush();
			ScrBuf.SetLockCount(Lock);
			return TRUE;
		}
		case FCTL_GETCMDLINE:
		case FCTL_GETCMDLINESELECTEDTEXT: {
			FARString strParam;

			if (Command == FCTL_GETCMDLINE)
				CmdLine->GetString(strParam);
			else
				CmdLine->GetSelString(strParam);

			if (Param1 && Param2)
				far_wcsncpy((wchar_t *)Param2, strParam, Param1);

			return (int)strParam.GetLength() + 1;
		}
		case FCTL_SETCMDLINE:
		case FCTL_INSERTCMDLINE: {
			CmdLine->DisableAC();
			if (Command == FCTL_SETCMDLINE)
				CmdLine->SetString((const wchar_t *)Param2);
			else
				CmdLine->InsertString((const wchar_t *)Param2);
			CmdLine->RevertAC();
			CmdLine->Redraw();
			return TRUE;
		}
		case FCTL_SETCMDLINEPOS: {
			CmdLine->SetCurPos(Param1);
			CmdLine->Redraw();
			return TRUE;
		}
		case FCTL_GETCMDLINEPOS: {
			if (Param2) {
				*(int *)Param2 = CmdLine->GetCurPos();
				return TRUE;
			}

			return FALSE;
		}
		case FCTL_GETCMDLINESELECTION: {
			if (Param2) {
				CmdLineSelect *sel = (CmdLineSelect *)Param2;
				CmdLine->GetSelection(sel->SelStart, sel->SelEnd);
				return TRUE;
			}

			return FALSE;
		}
		case FCTL_SETCMDLINESELECTION: {
			if (Param2) {
				CmdLineSelect *sel = (CmdLineSelect *)Param2;
				CmdLine->Select(sel->SelStart, sel->SelEnd);
				CmdLine->Redraw();
				return TRUE;
			}

			return FALSE;
		}
		case FCTL_ISACTIVEPANEL: {
			if (hPlugin == PANEL_ACTIVE)
				return TRUE;

			Panel *pPanel = FPanels->ActivePanel;
			PluginHandle *PlHandle;

			if (pPanel && (pPanel->GetMode() == PLUGIN_PANEL)) {
				PlHandle = (PluginHandle *)pPanel->GetPluginHandle();

				if (PlHandle) {
					if (PlHandle->hPlugin == hPlugin)
						return TRUE;
				}
			}

			return FALSE;
		}
	}

	return FALSE;
}

int WINAPI FarControl(HANDLE hPlugin, int Command, int Param1, LONG_PTR Param2)
{
	return InterThreadCall<int, 0>(std::bind(FarControlSynched, hPlugin, Command, Param1, Param2));
}

//

HANDLE WINAPI FarSaveScreen(int X1, int Y1, int X2, int Y2)
{
	if (DisablePluginsOutput || FrameManager->ManagerIsDown())
		return nullptr;

	if (X2 == -1)
		X2 = ScrX;

	if (Y2 == -1)
		Y2 = ScrY;

	return ((HANDLE)(new SaveScreen(X1, Y1, X2, Y2)));
}

void WINAPI FarRestoreScreen(HANDLE hScreen)
{
	if (DisablePluginsOutput || FrameManager->ManagerIsDown())
		return;

	if (!hScreen)
		ScrBuf.FillBuf();

	if (hScreen)
		delete (SaveScreen *)hScreen;
}

static void PR_FarGetDirListMsg()
{
	Message(0, 0, L"", Msg::PreparingList);
}

static int FarGetDirListSynched(const wchar_t *Dir, FAR_FIND_DATA **pPanelItem, int *pItemsNumber)
{
	if (FrameManager->ManagerIsDown() || !Dir || !*Dir || !pItemsNumber || !pPanelItem)
		return FALSE;

	FARString strDirName;
	ConvertNameToFull(Dir, strDirName);
	{
		TPreRedrawFuncGuard preRedrawFuncGuard(PR_FarGetDirListMsg);
		SaveScreen SaveScr;
		clock_t StartTime = GetProcessUptimeMSec();
		int MsgOut = 0;
		FAR_FIND_DATA_EX FindData;
		FARString strFullName;
		ScanTree ScTree(FALSE);
		ScTree.SetFindPath(strDirName, L"*");
		*pItemsNumber = 0;
		*pPanelItem = nullptr;
		FAR_FIND_DATA *ItemsList = nullptr, *TmpList;
		int ItemsNumber = 0;

		while (ScTree.GetNextName(&FindData, strFullName)) {
			if (!(ItemsNumber & 31)) {
				if (CheckForEsc()) {
					FarFreeDirList(ItemsList, ItemsNumber);
					return FALSE;
				}

				if (!MsgOut && GetProcessUptimeMSec() - StartTime > 500) {
					SetCursorType(FALSE, 0);
					PR_FarGetDirListMsg();
					MsgOut = 1;
				}

				TmpList = (FAR_FIND_DATA *)realloc(ItemsList, sizeof(*ItemsList) * (ItemsNumber + 32 + 1));

				if (TmpList)
					ItemsList = TmpList;
				else {
					FarFreeDirList(ItemsList,ItemsNumber);
					return FALSE;
				}
			}

			ItemsList[ItemsNumber].dwFileAttributes = FindData.dwFileAttributes;
			ItemsList[ItemsNumber].nFileSize = FindData.nFileSize;
			ItemsList[ItemsNumber].nPhysicalSize = FindData.nPhysicalSize;
			ItemsList[ItemsNumber].ftCreationTime = FindData.ftCreationTime;
			ItemsList[ItemsNumber].ftLastAccessTime = FindData.ftLastAccessTime;
			ItemsList[ItemsNumber].ftLastWriteTime = FindData.ftLastWriteTime;
			ItemsList[ItemsNumber].dwUnixMode = FindData.dwUnixMode;
			ItemsList[ItemsNumber].lpwszFileName = wcsdup(strFullName.CPtr());
			ItemsNumber++;
		}

		*pPanelItem = ItemsList;
		*pItemsNumber = ItemsNumber;
	}
	return TRUE;
}

int WINAPI FarGetDirList(const wchar_t *Dir, FAR_FIND_DATA **pPanelItem, int *pItemsNumber)
{
	return InterThreadCall<int, 0>(std::bind(FarGetDirListSynched, Dir, pPanelItem, pItemsNumber));
}

static PluginPanelItem *PluginDirList;
static int DirListItemsNumber;
static FARString strPluginSearchPath;
static int StopSearch;
static HANDLE hDirListPlugin;
static int PluginSearchMsgOut;

static void FarGetPluginDirListMsg(const wchar_t *Name, DWORD Flags)
{
	Message(Flags, 0, L"", Msg::PreparingList, Name);
	PreRedrawItem preRedrawItem = PreRedraw.Peek();
	preRedrawItem.Param.Flags = Flags;
	preRedrawItem.Param.Param1 = (void *)Name;
	PreRedraw.SetParam(preRedrawItem.Param);
}

static void PR_FarGetPluginDirListMsg()
{
	PreRedrawItem preRedrawItem = PreRedraw.Peek();
	FarGetPluginDirListMsg((const wchar_t *)preRedrawItem.Param.Param1,
			preRedrawItem.Param.Flags & (~MSG_KEEPBACKGROUND));
}

int FarGetPluginDirListSynched(INT_PTR PluginNumber, HANDLE hPlugin, const wchar_t *Dir,
		PluginPanelItem **pPanelItem, int *pItemsNumber)
{
	if (FrameManager->ManagerIsDown() || !Dir || !*Dir || !pItemsNumber || !pPanelItem)
		return FALSE;

	{
		if (!StrCmp(Dir, L".") || TestParentFolderName(Dir))
			return FALSE;

		static PluginHandle DirListPlugin;

		// А не хочет ли плагин посмотреть на текущую панель?
		if (hPlugin == PANEL_ACTIVE || hPlugin == PANEL_PASSIVE) {
			/*
				$ 30.11.2001 DJ
				А плагиновая ли это панель?
			*/
			HANDLE Handle = ((hPlugin == PANEL_ACTIVE)
							? CtrlObject->Cp()->ActivePanel
							: CtrlObject->Cp()->GetAnotherPanel(CtrlObject->Cp()->ActivePanel))
									->GetPluginHandle();

			if (!Handle || Handle == INVALID_HANDLE_VALUE)
				return FALSE;

			DirListPlugin = *(PluginHandle *)Handle;
		} else {
			DirListPlugin.pPlugin = (Plugin *)PluginNumber;
			DirListPlugin.hPlugin = hPlugin;
		}

		{
			SaveScreen SaveScr;
			TPreRedrawFuncGuard preRedrawFuncGuard(PR_FarGetPluginDirListMsg);
			{
				FARString strDirName;
				strDirName = Dir;
				TruncStr(strDirName, 30);
				CenterStr(strDirName, strDirName, 30);
				SetCursorType(FALSE, 0);
				FarGetPluginDirListMsg(strDirName, 0);
				PluginSearchMsgOut = FALSE;
				hDirListPlugin = (HANDLE)&DirListPlugin;
				StopSearch = FALSE;
				*pItemsNumber = DirListItemsNumber = 0;
				*pPanelItem = PluginDirList = nullptr;
				OpenPluginInfo Info;
				CtrlObject->Plugins.GetOpenPluginInfo(hDirListPlugin, &Info);
				FARString strPrevDir = Info.CurDir;

				if (CtrlObject->Plugins.SetDirectory(hDirListPlugin, Dir, OPM_SILENT)) {
					strPluginSearchPath = Dir;
					strPluginSearchPath+= L"\x1";
					ScanPluginDir();
					*pPanelItem = PluginDirList;
					*pItemsNumber = DirListItemsNumber;
					CtrlObject->Plugins.SetDirectory(hDirListPlugin, L"..", OPM_SILENT);
					OpenPluginInfo NewInfo;
					CtrlObject->Plugins.GetOpenPluginInfo(hDirListPlugin, &NewInfo);

					if (StrCmp(strPrevDir, NewInfo.CurDir)) {
						PluginPanelItem *PanelData = nullptr;
						int ItemCount = 0;

						if (CtrlObject->Plugins.GetFindData(hDirListPlugin, &PanelData, &ItemCount,
									OPM_SILENT)) {
							CtrlObject->Plugins.FreeFindData(hDirListPlugin, PanelData, ItemCount);
						}

						CtrlObject->Plugins.SetDirectory(hDirListPlugin, strPrevDir, OPM_SILENT);
					}
				}
			}
		}
	}
	return (!StopSearch);
}

int WINAPI FarGetPluginDirList(INT_PTR PluginNumber, HANDLE hPlugin, const wchar_t *Dir,
		PluginPanelItem **pPanelItem, int *pItemsNumber)
{
	return InterThreadCall<int, 0>(
			std::bind(FarGetPluginDirListSynched, PluginNumber, hPlugin, Dir, pPanelItem, pItemsNumber));
}

/*
	$ 30.11.2001 DJ
	вытащим в функцию общий код для копирования айтема в ScanPluginDir()
*/

static void CopyPluginDirItem(PluginPanelItem *CurPanelItem)
{
	FARString strFullName;
	strFullName = strPluginSearchPath;
	strFullName+= CurPanelItem->FindData.lpwszFileName;
	ReplaceChars(strFullName, L'\x1', GOOD_SLASH);
	PluginPanelItem *DestItem = PluginDirList + DirListItemsNumber;
	*DestItem = *CurPanelItem;

	if (CurPanelItem->UserData && (CurPanelItem->Flags & PPIF_USERDATA)) {
		DWORD Size = *(DWORD *)CurPanelItem->UserData;
		DestItem->UserData = (DWORD_PTR)malloc(Size);
		memcpy((void *)DestItem->UserData, (void *)CurPanelItem->UserData, Size);
	}

	DestItem->FindData.lpwszFileName = wcsdup(strFullName);
	DirListItemsNumber++;
}

static void ScanPluginDir()
{
	PluginPanelItem *PanelData = nullptr;
	int ItemCount = 0;
	int AbortOp = FALSE;
	FARString strDirName = strPluginSearchPath;
	ReplaceChars(strDirName, '\x1', GOOD_SLASH);
	while (strDirName.GetLength() > 1 && strDirName.Ends(GOOD_SLASH))
		strDirName.Truncate(strDirName.GetLength() - 1);
	TruncStr(strDirName, 30);
	CenterStr(strDirName, strDirName, 30);

	FarGetPluginDirListMsg(strDirName, AbortOp ? 0 : MSG_KEEPBACKGROUND);

	if (CheckForEscSilent()) {
		if (Opt.Confirm.Esc)	// Будет выдаваться диалог?
			AbortOp = TRUE;

		if (ConfirmAbortOp())
			StopSearch = TRUE;
	}

	if (StopSearch || !CtrlObject->Plugins.GetFindData(hDirListPlugin, &PanelData, &ItemCount, OPM_FIND))
		return;

	PluginPanelItem *NewList = (PluginPanelItem *)realloc(PluginDirList,
			1 + sizeof(*PluginDirList) * (DirListItemsNumber + ItemCount));

	if (!NewList) {
		StopSearch = TRUE;
		return;
	}

	PluginDirList = NewList;

	for (int i = 0; i < ItemCount && !StopSearch; i++) {
		PluginPanelItem *CurPanelItem = PanelData + i;

		if (!(CurPanelItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			CopyPluginDirItem(CurPanelItem);
	}

	for (int i = 0; i < ItemCount && !StopSearch; i++) {
		PluginPanelItem *CurPanelItem = PanelData + i;

		if ((CurPanelItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				&& StrCmp(CurPanelItem->FindData.lpwszFileName, L".")
				&& !TestParentFolderName(CurPanelItem->FindData.lpwszFileName)) {
			PluginPanelItem *NewList = (PluginPanelItem *)realloc(PluginDirList,
					sizeof(*PluginDirList) * (DirListItemsNumber + 1));

			if (!NewList) {
				StopSearch = TRUE;
				return;
			}

			PluginDirList = NewList;
			/*
				$ 30.11.2001 DJ
				используем общую функцию для копирования FindData (не забываем
				обработать PPIF_USERDATA)
			*/
			CopyPluginDirItem(CurPanelItem);
			FARString strFileName = CurPanelItem->FindData.lpwszFileName;

			if (CtrlObject->Plugins.SetDirectory(hDirListPlugin, strFileName, OPM_FIND)) {
				strPluginSearchPath+= CurPanelItem->FindData.lpwszFileName;
				strPluginSearchPath+= L"\x1";
				ScanPluginDir();
				size_t pos = (size_t)-1;
				strPluginSearchPath.RPos(pos, L'\x1');
				strPluginSearchPath.Truncate(pos);

				if (strPluginSearchPath.RPos(pos, L'\x1'))
					strPluginSearchPath.Truncate(pos + 1);
				else
					strPluginSearchPath.Clear();

				if (!CtrlObject->Plugins.SetDirectory(hDirListPlugin, L"..", OPM_FIND)) {
					StopSearch = TRUE;
					break;
				}
			}
		}
	}

	CtrlObject->Plugins.FreeFindData(hDirListPlugin, PanelData, ItemCount);
}

void WINAPI FarFreeDirList(FAR_FIND_DATA *PanelItem, int nItemsNumber)
{
	if (!PanelItem)
		return;

	for (int I = 0; I < nItemsNumber; I++) {
		FAR_FIND_DATA *CurPanelItem = PanelItem + I;
		apiFreeFindData(CurPanelItem);
	}

	free(PanelItem);
}

void WINAPI FarFreePluginDirList(PluginPanelItem *PanelItem, int ItemsNumber)
{
	if (!PanelItem)
		return;

	for (int I = 0; I < ItemsNumber; I++) {
		PluginPanelItem *CurPanelItem = PanelItem + I;

		if (CurPanelItem->UserData && (CurPanelItem->Flags & PPIF_USERDATA)) {
			free((void *)CurPanelItem->UserData);
		}

		apiFreeFindData(&CurPanelItem->FindData);
	}

	free(PanelItem);
}

static FileHolderPtr ViewerFileHolderPtr(const wchar_t *FileName, DWORD Flags)
{
	/*
		$ 14.06.2002 IS
		Обработка VF_DELETEONLYFILEONCLOSE - этот флаг имеет более низкий
		приоритет по сравнению с VF_DELETEONCLOSE
	*/
	FARString strFullFileName;
	if (FileName && *FileName) {
		ConvertNameToFull(FileName, strFullFileName);
		if ((Flags & (VF_DELETEONCLOSE | VF_DELETEONLYFILEONCLOSE)) != 0) {
			return std::make_shared<TempFileHolder>(strFullFileName, (Flags & VF_DELETEONCLOSE) != 0);
		}
	}

	return std::make_shared<FileHolder>(strFullFileName);
}

static int FarViewerSynched(const wchar_t *FileName, const wchar_t *Title, int X1, int Y1, int X2, int Y2,
		DWORD Flags, UINT CodePage)
{
	if (FrameManager->ManagerIsDown())
		return FALSE;

	class ConsoleTitle ct;
	int DisableHistory = (Flags & VF_DISABLEHISTORY) ? TRUE : FALSE;

	// $ 15.05.2002 SKV - Запретим вызов немодального редактора вьюера из модального.
	if (FrameManager->InModalEV()) {
		Flags&= ~VF_NONMODAL;
	}

	if (Flags & VF_NONMODAL) {
		/* 09.09.2001 IS ! Добавим имя файла в историю, если потребуется */
		FileViewer *Viewer = new (std::nothrow)
				FileViewer(ViewerFileHolderPtr(FileName, Flags), TRUE, DisableHistory, Title, X1, Y1, X2, Y2, CodePage);

		if (!Viewer)
			return FALSE;

		Viewer->SetEnableF6((Flags & VF_ENABLE_F6));

		/*
			$ 21.05.2002 SKV
			Запускаем свой цикл только если не был указан флаг.
		*/
		if (!(Flags & VF_IMMEDIATERETURN)) {
			FrameManager->ExecuteNonModal();
		} else {
			if (GlobalSaveScrPtr)
				GlobalSaveScrPtr->Discard();

			FrameManager->PluginCommit();
		}
	} else {
		/* 09.09.2001 IS ! Добавим имя файла в историю, если потребуется */
		FileViewer Viewer(ViewerFileHolderPtr(FileName, Flags), FALSE, DisableHistory, Title, X1, Y1, X2, Y2, CodePage);

		Viewer.SetEnableF6((Flags & VF_ENABLE_F6));

		/* $ 28.05.2001 По умолчанию Вьюер, поэтому нужно здесь признак выставиль явно */
		Viewer.SetDynamicallyBorn(false);
		FrameManager->ExecuteModalEV();

		if (!Viewer.GetExitCode()) {
			return FALSE;
		}
	}

	return TRUE;
}

int WINAPI FarViewer(const wchar_t *FileName, const wchar_t *Title, int X1, int Y1, int X2, int Y2,
		DWORD Flags, UINT CodePage)
{
	return InterThreadCall<int, 0>(
			std::bind(FarViewerSynched, FileName, Title, X1, Y1, X2, Y2, Flags, CodePage));
}

int FarEditorSynched(const wchar_t *FileName, const wchar_t *Title, int X1, int Y1, int X2, int Y2,
		DWORD Flags, int StartLine, int StartChar, UINT CodePage)
{
	if (FrameManager->ManagerIsDown())
		return EEC_OPEN_ERROR;

	ConsoleTitle ct;
	/*
		$ 12.07.2000 IS
		Проверка флагов редактора (раньше они игнорировались) и открытие
		немодального редактора, если есть соответствующий флаг
	*/
	int CreateNew = (Flags & EF_CREATENEW) ? TRUE : FALSE;
	int Locked = (Flags & EF_LOCKED) ? TRUE : FALSE;
	int DisableHistory = (Flags & EF_DISABLEHISTORY) ? TRUE : FALSE;
	/*
		$ 14.06.2002 IS
		Обработка EF_DELETEONLYFILEONCLOSE - этот флаг имеет более низкий
		приоритет по сравнению с EF_DELETEONCLOSE
	*/
	std::shared_ptr<FileHolder> TFH;
	if (Flags & (EF_DELETEONCLOSE | EF_DELETEONLYFILEONCLOSE))
		TFH = std::make_shared<TempFileHolder>(FileName, (Flags & EF_DELETEONCLOSE) != 0);
	else
		TFH = std::make_shared<FileHolder>(FileName);

	int OpMode = FEOPMODE_QUERY;

	if ((Flags & EF_OPENMODE_MASK))
		OpMode = Flags & EF_OPENMODE_MASK;

	/*
		$ 15.05.2002 SKV
		Запретим вызов немодального редактора, если находимся в модальном
		редакторе или вьюере.
	*/
	if (FrameManager->InModalEV()) {
		Flags&= ~EF_NONMODAL;
	}

	int editorExitCode;
	int ExitCode = EEC_OPEN_ERROR;

	if (Flags & EF_NONMODAL) {
		/* 09.09.2001 IS ! Добавим имя файла в историю, если потребуется */
		FileEditor *Editor = new (std::nothrow) FileEditor(TFH, CodePage,
				(CreateNew ? FFILEEDIT_CANNEWFILE : 0) | FFILEEDIT_ENABLEF6
						| (DisableHistory ? FFILEEDIT_DISABLEHISTORY : 0) | (Locked ? FFILEEDIT_LOCKED : 0),
				StartLine, StartChar, Title, X1, Y1, X2, Y2, OpMode);

		if (Editor) {
			editorExitCode = Editor->GetExitCode();

			// добавочка - проверка кода возврата (почему возникает XC_OPEN_ERROR - см. код FileEditor::Init())
			if (editorExitCode == XC_OPEN_ERROR || editorExitCode == XC_LOADING_INTERRUPTED) {
				delete Editor;
				Editor = nullptr;
				return editorExitCode;
			}

			Editor->SetEnableF6((Flags & EF_ENABLE_F6));
			Editor->SetPluginTitle(Title);

			/* $ 21.05.2002 SKV - Запускаем свой цикл, только если не был указан флаг. */
			if (!(Flags & EF_IMMEDIATERETURN)) {
				FrameManager->ExecuteNonModal();
			} else {
				if (GlobalSaveScrPtr)
					GlobalSaveScrPtr->Discard();

				FrameManager->PluginCommit();
			}

			ExitCode = XC_MODIFIED;
		}
	} else {
		/* 09.09.2001 IS ! Добавим имя файла в историю, если потребуется */
		FileEditor Editor(TFH, CodePage,
				(CreateNew ? FFILEEDIT_CANNEWFILE : 0) | (DisableHistory ? FFILEEDIT_DISABLEHISTORY : 0)
						| (Locked ? FFILEEDIT_LOCKED : 0),
				StartLine, StartChar, Title, X1, Y1, X2, Y2, OpMode);
		editorExitCode = Editor.GetExitCode();

		// выполним предпроверку (ошибки разные могут быть)
		if (editorExitCode == XC_OPEN_ERROR || editorExitCode == XC_LOADING_INTERRUPTED)
			ExitCode = editorExitCode;
		else {
			Editor.SetDynamicallyBorn(false);
			Editor.SetEnableF6((Flags & EF_ENABLE_F6));
			Editor.SetPluginTitle(Title);
			/*
				$ 15.05.2002 SKV
				Зафиксируем вход и выход в/из модального редактора.
			*/
			FrameManager->ExecuteModalEV();
			ExitCode = Editor.GetExitCode();

			if (ExitCode) {
#if 0

				if (OpMode==FEOPMODE_BREAKIFOPEN && ExitCode==XC_QUIT)
					ExitCode = XC_OPEN_ERROR;
				else
#endif
				ExitCode = Editor.IsFileChanged() ? XC_MODIFIED : XC_NOT_MODIFIED;
			}

			// workaround for non-repained background of (if) pending modal dialogs
			GenerateWINDOW_BUFFER_SIZE_EVENT(-1, -1, true);
		}
	}

	return ExitCode;
}

int WINAPI FarEditor(const wchar_t *FileName, const wchar_t *Title, int X1, int Y1, int X2, int Y2,
		DWORD Flags, int StartLine, int StartChar, UINT CodePage)
{
	return InterThreadCall<int, EEC_OPEN_ERROR>(std::bind(FarEditorSynched, FileName, Title, X1, Y1, X2, Y2,
			Flags, StartLine, StartChar, CodePage));
}

int WINAPI FarCmpName(const wchar_t *pattern, const wchar_t *string, int skippath)
{
	return (CmpName(pattern, string, skippath != 0) ? TRUE : FALSE);
}

static bool FarTextSynched(int X, int Y, uint64_t Color, const wchar_t *Str)
{
	if (DisablePluginsOutput || FrameManager->ManagerIsDown())
		return false;

	if (!Str) {
		int PrevLockCount = ScrBuf.GetLockCount();
		ScrBuf.SetLockCount(0);
		ScrBuf.Flush();
		ScrBuf.SetLockCount(PrevLockCount);
	} else {
		Text(X, Y, FarColorToReal(Color), Str);
	}
	return true;
}

void WINAPI FarText(int X, int Y, uint64_t Color, const wchar_t *Str)
{
	InterThreadCall<bool>(std::bind(FarTextSynched, X, Y, Color, Str));
}

static int FarEditorControlSynched(int Command, void *Param)
{
	if (FrameManager->ManagerIsDown() || !CtrlObject->Plugins.CurEditor)
		return 0;

	return (CtrlObject->Plugins.CurEditor->EditorControl(Command, Param));
}

int WINAPI FarEditorControl(int Command, void *Param)
{
	return InterThreadCall<int, 0>(std::bind(FarEditorControlSynched, Command, Param));
}

static int FarViewerControlSynched(int Command, void *Param)
{
	if (FrameManager->ManagerIsDown() || !CtrlObject->Plugins.CurViewer)
		return 0;

	return (CtrlObject->Plugins.CurViewer->ViewerControl(Command, Param));
}

int WINAPI FarViewerControl(int Command, void *Param)
{
	return InterThreadCall<int, 0>(std::bind(FarViewerControlSynched, Command, Param));
}

void WINAPI farUpperBuf(wchar_t *Buf, int Length)
{
	return UpperBuf(Buf, Length);
}

void WINAPI farLowerBuf(wchar_t *Buf, int Length)
{
	return LowerBuf(Buf, Length);
}

void WINAPI farStrUpper(wchar_t *s1)
{
	return StrUpper(s1);
}

void WINAPI farStrLower(wchar_t *s1)
{
	return StrLower(s1);
}

wchar_t WINAPI farUpper(wchar_t Ch)
{
	return Upper(Ch);
}

wchar_t WINAPI farLower(wchar_t Ch)
{
	return Lower(Ch);
}

int WINAPI farStrCmpNI(const wchar_t *s1, const wchar_t *s2, int n)
{
	return StrCmpNI(s1, s2, n);
}

int WINAPI farStrCmpI(const wchar_t *s1, const wchar_t *s2)
{
	return StrCmpI(s1, s2);
}

int WINAPI farStrCmpN(const wchar_t *s1, const wchar_t *s2, int n)
{
	return StrCmpN(s1, s2, n);
}

int WINAPI farStrCmp(const wchar_t *s1, const wchar_t *s2)
{
	return StrCmp(s1, s2);
}

int WINAPI farIsLower(wchar_t Ch)
{
	return IsLower(Ch);
}

int WINAPI farIsUpper(wchar_t Ch)
{
	return IsUpper(Ch);
}

int WINAPI farIsAlpha(wchar_t Ch)
{
	return IsAlpha(Ch);
}

int WINAPI farIsAlphaNum(wchar_t Ch)
{
	return IsAlphaNum(Ch);
}

int WINAPI farGetFileOwner(const wchar_t *Computer, const wchar_t *Name, wchar_t *Owner, int Size)
{
	FARString strOwner;
	/*int Ret=*/GetFileOwner(Computer, Name, strOwner);

	if (Owner && Size)
		far_wcsncpy(Owner, strOwner, Size);

	return static_cast<int>(strOwner.GetLength() + 1);
}

int WINAPI farConvertPath(CONVERTPATHMODES Mode, const wchar_t *Src, wchar_t *Dest, int DestSize)
{
	if (Src && *Src) {
		FARString strDest;

		switch (Mode) {
			case CPM_NATIVE:
				strDest = NTPath(Src).Get();
				break;
			case CPM_REAL:
				ConvertNameToReal(Src, strDest);
				break;
			case CPM_FULL:
			default:
				ConvertNameToFull(Src, strDest);
				break;
		}

		if (Dest && DestSize)
			far_wcsncpy(Dest, strDest.CPtr(), DestSize);

		return static_cast<int>(strDest.GetLength()) + 1;
	} else {
		if (Dest && DestSize)
			*Dest = 0;

		return 1;
	}
}

int WINAPI farGetReparsePointInfo(const wchar_t *Src, wchar_t *Dest, int DestSize)
{
	_LOGCOPYR(CleverSysLog Clev(L"farGetReparsePointInfo()"));
	_LOGCOPYR(SysLog(L"Params: Src='%ls'", Src));
	/*
	if (Src && *Src)
	{
		FARString strSrc(Src);
		FARString strDest;
		AddEndSlash(strDest);
		DWORD Size=GetReparsePointInfo(strSrc,strDest,nullptr);
		_LOGCOPYR(SysLog(L"return -> %d strSrc='%ls', strDest='%ls'",__LINE__,strSrc.CPtr(),strDest.CPtr()));

		if (DestSize && Dest)
			far_wcsncpy(Dest,strDest,DestSize);

		return Size;
	}*/

	return 0;
}

int WINAPI farGetPathRoot(const wchar_t *Path, wchar_t *Root, int DestSize)
{
	fprintf(stderr, "DEPRECATED: %s('%ls')\n", __FUNCTION__, Path);

	if (Path && *Path) {
		if (DestSize >= 2 && Root) {
			Root[0] = GOOD_SLASH;
			Root[1] = 0;
		}

		return 2;
	} else {
		if (DestSize >= 1 && Root) {
			Root[0] = 0;
		}

		return 1;
	}
}

static int farPluginsControlSynched(HANDLE hHandle, int Command, int Param1, LONG_PTR Param2)
{
	switch (Command) {
		case PCTL_CACHEFORGET:
		case PCTL_LOADPLUGIN:
		case PCTL_UNLOADPLUGIN:
		case PCTL_FORCEDLOADPLUGIN: {
			if (Param1 == PLT_PATH) {
				if (Param2) {
					FARString strPath;
					ConvertNameToFull((const wchar_t *)Param2, strPath);

					if (Command == PCTL_CACHEFORGET)
						return CtrlObject->Plugins.CacheForget(strPath);
					if (Command == PCTL_LOADPLUGIN)
						return CtrlObject->Plugins.LoadPluginExternal(strPath, false);
					if (Command == PCTL_FORCEDLOADPLUGIN)
						return CtrlObject->Plugins.LoadPluginExternal(strPath, true);
					else
						return CtrlObject->Plugins.UnloadPluginExternal(strPath);
				}
			}

			break;
		}
	}

	return 0;
}

int WINAPI farPluginsControl(HANDLE hHandle, int Command, int Param1, LONG_PTR Param2)
{
	return InterThreadCall<int, 0>(std::bind(farPluginsControlSynched, hHandle, Command, Param1, Param2));
}

int WINAPI farFileFilterControl(HANDLE hHandle, int Command, int Param1, LONG_PTR Param2)
{
	FileFilter *Filter = nullptr;

	if (Command != FFCTL_CREATEFILEFILTER) {
		if (hHandle == INVALID_HANDLE_VALUE)
			return FALSE;

		Filter = (FileFilter *)hHandle;
	}

	switch (Command) {
		case FFCTL_CREATEFILEFILTER: {
			if (!Param2)
				break;

			*((HANDLE *)Param2) = INVALID_HANDLE_VALUE;

			if (hHandle != PANEL_ACTIVE && hHandle != PANEL_PASSIVE && hHandle != PANEL_NONE)
				break;

			switch (Param1) {
				case FFT_PANEL:
				case FFT_FINDFILE:
				case FFT_COPY:
				case FFT_SELECT:
				case FFT_CUSTOM:
					break;
				default:
					return FALSE;
			}

			Filter = new (std::nothrow) FileFilter((Panel *)hHandle, (FAR_FILE_FILTER_TYPE)Param1);

			if (Filter) {
				*((HANDLE *)Param2) = (HANDLE)Filter;
				return TRUE;
			}

			break;
		}
		case FFCTL_FREEFILEFILTER: {
			delete Filter;
			return TRUE;
		}
		case FFCTL_OPENFILTERSMENU: {
			return Filter->FilterEdit() ? TRUE : FALSE;
		}
		case FFCTL_STARTINGTOFILTER: {
			Filter->UpdateCurrentTime();
			return TRUE;
		}
		case FFCTL_ISFILEINFILTER: {
			if (!Param2)
				break;

			return Filter->FileInFilter(*(const FAR_FIND_DATA *)Param2) ? TRUE : FALSE;
		}
	}

	return FALSE;
}

int WINAPI farRegExpControl(HANDLE hHandle, int Command, LONG_PTR Param)
{
	RegExp *re = nullptr;

	if (Command != RECTL_CREATE) {
		if (hHandle == INVALID_HANDLE_VALUE)
			return FALSE;

		re = (RegExp *)hHandle;
	}

	switch (Command) {
		case RECTL_CREATE:

			if (!Param)
				break;

			*((HANDLE *)Param) = INVALID_HANDLE_VALUE;
			re = new (std::nothrow) RegExp;

			if (re) {
				*((HANDLE *)Param) = (HANDLE)re;
				return TRUE;
			}

			break;
		case RECTL_FREE:
			delete re;
			return TRUE;
		case RECTL_COMPILE:
			return re->Compile((const wchar_t *)Param, OP_PERLSTYLE);
		case RECTL_OPTIMIZE:
			return re->Optimize();
		case RECTL_MATCHEX: {
			RegExpSearch *data = (RegExpSearch *)Param;
			return re->MatchEx(ReStringView(data->Text, data->Length), data->Position, data->Match,
					data->Count
#ifdef NAMEDBRACKETS
					,
					data->Reserved
#endif
			);
		}
		case RECTL_SEARCHEX: {
			RegExpSearch *data = (RegExpSearch *)Param;
			return re->SearchEx(ReStringView(data->Text, data->Length), data->Position, data->Match,
					data->Count
#ifdef NAMEDBRACKETS
					,
					data->Reserved
#endif
			);
		}
		case RECTL_BRACKETSCOUNT:
			return re->GetBracketsCount();
	}

	return FALSE;
}

DWORD WINAPI farGetCurrentDirectory(DWORD Size, wchar_t *Buffer)
{
	FARString strCurDir;
	apiGetCurrentDirectory(strCurDir);

	if (Buffer && Size) {
		far_wcsncpy(Buffer, strCurDir, Size);
	}

	return static_cast<DWORD>(strCurDir.GetLength() + 1);
}
