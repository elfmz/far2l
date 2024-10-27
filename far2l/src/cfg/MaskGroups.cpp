/*
MaskGroups.cpp

Groups of file masks
*/

#include "headers.hpp"

#include "config.hpp"
#include "ConfigRW.hpp"
#include "ctrlobj.hpp"
#include "DialogBuilder.hpp"
#include "dialog.hpp"
#include "DlgGuid.hpp"
#include "hilight.hpp"
#include "interf.hpp"
#include "keys.hpp"
#include "lang.hpp"
#include "message.hpp"
#include "vmenu.hpp"
#include "MaskGroups.hpp"

static const wchar_t *Help = L"MaskGroupsSettings";

struct FileMaskStrings
{
	const char *Root, *TypeFmt, *Type0, *MaskName, *MaskValue;
};

static const FileMaskStrings FMS=
{
	"MaskGroups",
	"MaskGroups/Type%d",
	"MaskGroups/Type",
	"Name",
	"Value",
};

static void ApplyDefaultMaskGroups()
{
	static const std::pair<const wchar_t*, const wchar_t*> Sets[]
	{
		{ L"arc",    L"*.rar,*.zip,*.[zj],*.[bxg7]z,*.[bg]zip,*.tar,*.t[agbx]z,*.ar[cj],*.r[0-9][0-9],*.a[0-9][0-9],*.bz2,*.cab,*.msi,*.jar,*.lha,*.lzh,"
				     L"*.ha,*.ac[bei],*.pa[ck],*.rk,*.cpio,*.rpm,*.zoo,*.hqx,*.sit,*.ice,*.uc2,*.ain,*.imp,*.777,*.ufa,*.boa,*.bs[2a],*.sea,*.hpk,*.ddi,"
				     L"*.x2,*.rkv,*.[lw]sz,*.h[ay]p,*.lim,*.sqz,*.chz" },
		{ L"temp",   L"*.cache,*.temp,*.tmp,*.bak,*.old,*.backup,*.back,temp,temp.*,temporary*" },
		{ L"tmp",    L"<temp>" },
		{ L"exec",   L"*.sh,*.pl,*.cmd,*.exe,*.bat,*.com,*.elf,*.run,*.AppImage,*.apk,*.rb" },
		{ L"sound",  L"*.aif,*.cda,*.mid,*.midi,*.mp3,*.mpa,*.ogg,*.wma,*.flac,*.wav,*.ape,*.wv,*.voc,*.669,*.digi,*.amf,*.ams,*.dbm,*.dmf,*.dsm,*.gdm,"
				     L"*.imf,*.it,*.itg,*.itp,*.j2b,*.mdl,*.med,*.mo3,*.mod,*.mt2,*.mtm,*.okt,*.plm,*.psm,*.ptm,*.s3m,*.sfx,*.stm,*.stp,*.uax,*.ult,*.xm"},
		{ L"snd",    L"<sound>" },
		{ L"pic",    L"*.avif,*.jpg,*.jpeg,*.jpeg2000,*.ico,*.gif,*.png,*.webp,*.tga,*.bmp,*.pcx,*.tiff,*.tif,*.psd,*.eps,*.indd,*.svg,*.ai,*.cpt,*.kra,"
					 L"*.pdn,*.psp,*.xcf,*.sai,*.cgm,*.mpo,*.pns,*.jps" },
		{ L"video",  L"*.mkv,*.webm,*.mpg,*.mp2,*.mpeg,*.mpe,*.mpv,*.mp4,*.m4p,*.m4v,*.avi,*.wmv,*.mov,*.qt,*.flv,*.swf,*.avchd,*.3gp,*.vob" },
		{ L"media",  L"<sound>,<video>" },
		{ L"doc",    L"*.docx,*.odt,*.pdf,*.rtf,*.tex,*.wpd,*.htm,*.html,*.key,*.odp,*.pps,*.ppt,*.pptx,*.ods,*.xls,*.xlsm,*.xlsx,*.srt,*.nfo,*.rst,*.man,"
                     L"read.me,readme*,*.txt,*.chm,*.hlp,*.doc,*.md,NEWS" },
		{ L"src",    L"*.c,*.cpp,*.c++,*.h,*.hpp,*.h++,*.asm,*.inc,*.src,*.css,*.glsl,*.lua,*.java,*.php,*.go,*.perl,*.r,*.bas,*.pas,*.jsm,*.qml,"
					 L"*.js,*.kt,*.sample,*.vs,*.fs,*.fx,*.hlsl,*.fsh,*.vsh,*.pixel,*.vertex,*.fragmentshader,*.fragment,*.vertexshader,"
					 L"*.ml,*.frag,*.geom,*.vert,*.rs,*.ts,*.jam,*.tcl, *.swift" },
		{ L"3d",     L"*.ma,*.mb,*.opengex,*.ply,*.pov-ray,*.prc,*.step,*.skp,*.stl,*.u3d,*.vrml,*.xaml,*.xgl,*.xvl,*.xvrml,*.x3d,*.3d,*.3df,*.3dm,*.3ds,"
					 L"*.3dxml,*.x3d,*.dds,*.sdkmesh,*.x,*.hdr,*.ktx,*.amf,*.asymptote,*.blend,*.collada,*.dgn,*.dwf,*.dwg,*.dxf,*.drawings,*.flt,*.fvrml,"
					 L"*.gltf,*.hsf,*.iges,*.imml,*.ipa,*.jt"},
	};

	ConfigWriter cfg_writer;
	int n = 0;

	for ( auto i : Sets ) {
		cfg_writer.SelectSectionFmt(FMS.TypeFmt, n);
		cfg_writer.SetString(FMS.MaskName, i.first);
		cfg_writer.SetString(FMS.MaskValue, i.second);
		++n;
	}
}

void CheckMaskGroups( void )
{
	ConfigReader cfg_reader(FMS.Root);
	const auto &maskgroups = cfg_reader.EnumSectionsAt();

	if (!maskgroups.size()) {

		ApplyDefaultMaskGroups();
	}
}

static int FillMasksMenu(VMenu *TypesMenu, int MenuPos)
{
	ConfigReader cfg_reader;
	int DizWidth = 10;
	MenuItemEx TypesMenuItem;
	TypesMenu->DeleteItems();
	int NumLine = 0;

	for (;; NumLine++)
	{
		cfg_reader.SelectSectionFmt(FMS.TypeFmt, NumLine);
		FARString strMask;
		if (!cfg_reader.GetString(strMask, FMS.MaskValue))
			break;

		FARString strMenuText;

		if (DizWidth)
		{
			FARString strName = cfg_reader.GetString(FMS.MaskName);
			if (static_cast<int>(strName.GetLength()) > DizWidth)
			{
				strName.Truncate(DizWidth - (Opt.NoGraphics ? 3 : 1));
				strName += (Opt.NoGraphics ? L"..." : L"…");
			}
			strMenuText.Format(L"%-*.*ls %lc ", DizWidth, DizWidth, strName.CPtr(), BoxSymbols[BS_V1]);
		}

		strMenuText += strMask;
		TypesMenuItem.Clear();
		TypesMenuItem.strName = strMenuText;
		TypesMenuItem.SetSelect(NumLine == MenuPos);
		TypesMenu->AddItem(&TypesMenuItem);
	}

	return NumLine;
}

static bool EditMaskRecord (int EditPos, bool NewRec)
{
	bool Result = false;
	FARString strName, strMasks;

	if (!NewRec)
	{
		ConfigReader cfg_reader;
		cfg_reader.SelectSectionFmt(FMS.TypeFmt, EditPos);
		strMasks = cfg_reader.GetString(FMS.MaskValue);
		strName = cfg_reader.GetString(FMS.MaskName);
	}

	DialogBuilder Builder(Msg::MaskGroupTitle, Help);
//	Builder.SetId(EditMaskGroupId);
	Builder.AddText(Msg::MaskGroupName);
	Builder.AddEditField(&strName, 60);
	Builder.AddText(Msg::MaskGroupMasks);
	Builder.AddEditField(&strMasks, 60);
	Builder.AddOKCancel();

	if (Builder.ShowDialog() && !strName.IsEmpty() && !strMasks.IsEmpty())
	{
		ConfigWriter cfg_writer;
		cfg_writer.SelectSectionFmt(FMS.TypeFmt, EditPos);

		if (NewRec)
		{
			cfg_writer.ReserveIndexedSection(FMS.Type0, (unsigned int)EditPos);
		}

		cfg_writer.SetString(FMS.MaskValue, strMasks);
		cfg_writer.SetString(FMS.MaskName, strName);
		Result = true;
	}

	return Result;
}

static bool DeleteMaskRecord(int DeletePos)
{
	bool Result = false;
	FARString strItemName;

	{
		ConfigReader cfg_reader;
		cfg_reader.SelectSectionFmt(FMS.TypeFmt, DeletePos);
		strItemName = cfg_reader.GetString(FMS.MaskName);
	}

	if (!Message(MSG_WARNING, 2, Msg::MaskGroupTitle, Msg::MaskGroupAskDelete, strItemName, Msg::Delete, Msg::Cancel))
	{
		ConfigWriter cfg_writer;
		cfg_writer.SelectSectionFmt(FMS.TypeFmt, DeletePos);
		cfg_writer.RemoveSection();
		cfg_writer.DefragIndexedSections(FMS.Type0);
		Result = true;
	}

	return Result;
}

void MaskGroupsSettings()
{
	int NumLine = 0;
	int MenuPos = 0;
	VMenu MasksMenu(Msg::MaskGroupTitle, nullptr, 0, ScrY-4);
	MasksMenu.SetHelp(Help);
	MasksMenu.SetFlags(VMENU_WRAPMODE);
	MasksMenu.SetPosition(-1, -1, 0, 0);
//	MasksMenu.SetId(MaskGroupsMenuId);
	MasksMenu.SetBottomTitle(Msg::MaskGroupBottomTitle);

	while (1)
	{
		bool OuterLoop = true;
		bool MenuModified = false;
		bool bFilter = false;

		while (!MasksMenu.Done())
		{
			if (OuterLoop || MenuModified)
			{
				if (MenuModified)
					CtrlObject->HiFiles->UpdateHighlighting(true);

				if (bFilter) {
					bFilter = false;
					MasksMenu.SetTitle(Msg::MaskGroupTitle);
					MasksMenu.SetBottomTitle(Msg::MaskGroupBottomTitle);
				}

				MasksMenu.Hide();
				NumLine = FillMasksMenu(&MasksMenu, MenuPos);
				MasksMenu.SetPosition(-1, -1, -1, -1);
				MasksMenu.Show();
				MenuModified = false;
				OuterLoop = false;
			}

			FarKey Key = MasksMenu.ReadInput();
			MenuPos = MasksMenu.GetSelectPos();

			if (bFilter && (Key == KEY_ESC || Key == KEY_F10 || Key == KEY_ENTER || Key == KEY_NUMENTER) ) {

				bFilter = false;
				int n = MasksMenu.GetItemCount( );

				for (int i = 0; i < n; i++) {
					MenuItemEx *item = MasksMenu.GetItemPtr(i);
					if (!item) continue;
					MasksMenu.UpdateItemFlags(i, item->Flags & ~MIF_HIDDEN);
				}

				MasksMenu.SetPosition(-1, -1, 0, 0);
				MasksMenu.SetSelectPos(0, 1);
				MasksMenu.SetTitle(Msg::MaskGroupTitle);
				MasksMenu.SetBottomTitle(Msg::MaskGroupBottomTitle);
				MasksMenu.Show();

				continue;
			}

			switch (Key)
			{
				case KEY_NUMDEL:
				case KEY_DEL:
					if (MenuPos < 0 || MenuPos >= NumLine) break;
					MenuModified = DeleteMaskRecord(MenuPos);
					break;

				case KEY_NUMPAD0:
				case KEY_INS:
					if (bFilter) break;
					MenuModified = EditMaskRecord(MenuPos < 0 ? 0 : MenuPos, true);
					break;

				case KEY_NUMENTER:
				case KEY_ENTER:
				case KEY_F4:
					if (MenuPos != -1)
						MenuModified = (MenuPos < NumLine) && EditMaskRecord(MenuPos, false);
					break;

				case KEY_CTRLR:
					if (bFilter) break;
					if (Message(MSG_WARNING, 2, Msg::MaskGroupTitle, Msg::MaskGroupWarning,
								Msg::MaskGroupRestore, Msg::Yes, Msg::Cancel))
						break;
					{
						ConfigWriter(FMS.Root).RemoveSection();
					}

					ApplyDefaultMaskGroups();
					MenuModified = true;
					break;

				case KEY_F7: {
					FARString Value;
					BOOL bCaseSensitive = FALSE;
					DialogBuilder Builder(Msg::FileFilterTitle, Help);

					Builder.AddText(Msg::MaskGroupTargetFilter);
					Builder.AddEditField(&Value, 60);
					Builder.AddCheckbox(Msg::EditSearchCase, &bCaseSensitive);
					Builder.AddOKCancel();

					if (!Builder.ShowDialog())
						break;

					ConfigReader cfg_reader;
					CFileMask chkFileMask;
					FARString strMasks;
					wchar_t tmp[64];
					int n = MasksMenu.GetItemCount();

					for (int i = 0; i < n; i++) {
						cfg_reader.SelectSectionFmt(FMS.TypeFmt, i);
						strMasks = cfg_reader.GetString(FMS.MaskValue);

						if (!chkFileMask.Set(strMasks.CPtr(), 0))
							continue;

						if ( !chkFileMask.Compare(Value.CPtr(), !(bool)(bCaseSensitive)) ) {
							MenuItemEx *item = MasksMenu.GetItemPtr(i);
							if (!item) continue;
							MasksMenu.UpdateItemFlags(i, item->Flags | MIF_HIDDEN);
						}
					}

					MasksMenu.SetTitle( Value.CPtr());
					swprintf(tmp, 64, Msg::MaskGroupTotal, MasksMenu.GetShowItemCount());
					MasksMenu.SetBottomTitle(tmp);
					MasksMenu.SetPosition(-1, -1, 0, 0);
					MasksMenu.SetSelectPos(0, 1);
					MasksMenu.Show();
					bFilter = true;
				}
				break;

				case KEY_CTRLUP:
				case KEY_CTRLDOWN: {
					if (bFilter) 
						break;
					if (MenuPos < MasksMenu.GetItemCount() && !(Key == KEY_CTRLUP && !MenuPos)
							&& !(Key == KEY_CTRLDOWN && MenuPos == (MasksMenu.GetItemCount() - 1))) {
						int NewPos = MenuPos + (Key == KEY_CTRLDOWN ? 1 : -1);

						if (MenuPos != NewPos) {
							ConfigWriter().MoveIndexedSection(FMS.Type0, MenuPos, NewPos);
							MenuPos = NewPos;
							MenuModified = true;
						}
					}
				}
				break;

				default:
					MasksMenu.ProcessInput();
					break;
			}
		}

		int ExitCode = MasksMenu.Modal::GetExitCode();

		if (ExitCode != -1)
		{
			MenuPos = ExitCode;
			MasksMenu.ClearDone();
			MasksMenu.WriteInput(KEY_F4);
			continue;
		}

		break;
	}
}

bool GetMaskGroup(const FARString &MaskName, FARString &MaskValue)
{
	ConfigReader cfg_reader;
	FARString strMaskName;

	for (int Num = 0;
		cfg_reader.SelectSectionFmt(FMS.TypeFmt, Num),
		cfg_reader.GetString(strMaskName, FMS.MaskName);
			Num++)
	{
		if (!StrCmpI(strMaskName, MaskName))
			return cfg_reader.GetString(MaskValue, FMS.MaskValue);
	}
	return false;
}
