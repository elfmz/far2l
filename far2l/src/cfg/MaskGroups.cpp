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
		{ L"arc",    L"*.777,*.[bg]zip,*.[bxg7]z,*.[lw]sz,*.[zj],*.a[0-9][0-9],*.ac[bei],*.ain,*.ar[cj],*.boa,*.bs[2a],*.bz2,*.cab,*.chz,*.cpio,*.cue,*.ddi,"
				     L"*.deb,*.grp,*.h[ay]p,*.ha,*.hpk,*.hqx,*.ice,*.imp,*.iso,*.jar,*.lha,*.lim,*.lzh,*.msi,*.nrg,*.pa[ck],*.qpr,*.r[0-9][0-9],*.rar,*.rk,"
				     L"*.rkv,*.rpm,*.rpm,*.sea,*.sit,*.sqz,*.t[agbx]z,*.tar,*.uc2,*.ufa,*.x2,*.zip,*.zoo,*.zst"},

		{ L"temp",   L"*.back,*.backup,*.bak,*.cache,*.old,*.temp,*.tmp,temp,temp.*,temporary*"},

		{ L"tmp",    L"<temp>" },

		{ L"exec",   L"*.AppImage,*.apk,*.bat,*.cmd,*.com,*.elf,*.exe,*.pl,*.rb,*.run,*.sh"},

		{ L"shared", L"*.a,*.dll,*.dll.*,*.lib,*.o,*.obj,*.pyo,*.so,*.so.*,*.sys,*.vim"},

		{ L"sound",  L"*.669,*.aac,*.ac3,*.aif,*.aiff,*.amf,*.amr,*.ams,*.ape,*.as,*.asf,*.au,*.au,*.cda,*.cda,*.cmf,*.dbm,*.digi,*.dmf,*.dsm,*.fla,*.flac,"
				     L"*.gdm,*.gmd,*.gsm,*.hmi,*.hmp,*.hmz,*.iff,*.imf,*.it,*.itg,*.itp,*.itz,*.j2b,*.kar,*.la,*.lap,*.lqt,*.m3u,*.m3u8,*.m4a,*.mac,*.mdl,"
					 L"*.mdz,*.med,*.mid,*.midi,*.mids,*.miz,*.mmf,*.mo3,*.mod,*.mp1,*.mp2,*.mp3,*.mpa,*.mpc,*.mss,*.mt2,*.mtm,*.mus,*.ogg,*.okt,*.plm,*.pls,"
					 L"*.psm,*.ptm,*.ra,*.ram,*.rm,*.rmi,*.rmj,*.rmm,*.rmx,*.rns,*.rnx,*.rv,*.s3m,*.s3z,*.sfx,*.snd,*.speex,*.stm,*.stp,*.stz,*.tap,*.tta,"
					 L"*.uax,*.ult,*.voc,*.wav,*.wma,*.wv,*.xm,*.xmi,*.xmz"},

		{ L"snd",    L"<sound>"},

		{ L"pic",    L"*.ai,*.ani,*.avif,*.bmp,*.bw,*.cdr,*.cel,*.cgm,*.cmx,*.cpt,*.cur,*.dcx,*.dds,*.dib,*.emf,*.eps,*.flc,*.fli,*.fpx,*.gif,*.icl,*.ico,"
					 L"*.iff,*.indd,*.j2k,*.jp2,*.jpc,*.jpe,*.jpeg,*.jpeg2000,*.jpg,*.jps,*.kra,*.lbm,*.mng,*.mpo,*.pbm,*.pcx,*.pdn,*.pgm,*.pic,*.png,*.pns,"
					 L"*.ppm,*.psd,*.psp,*.ras,*.rgb,*.rle,*.sai,*.sgi,*.spr,*.svg,*.tga,*.tif,*.tiff,*.wbmp,*.webp,*.wmf,*.xbm,*.xcf,*.xpm"},
		{ L"video",  L"*.3g2,*.3gp,*.asf,*.avchd,*.avi,*.divx,*.enc,*.flv,*.ifo,*.m1v,*.m2ts,*.m2v,*.m4p,*.m4v,*.mkv,*.mov,*.mp2,*.mp4,*.mpe,*.mpeg,*.mpg,"
					 L"*.mpv,*.mts,*.ogm,*.qt,*.ra,*.ram,*.rmvb,*.swf,*.ts,*.vob,*.vob,*.webm,*.wm,*.wmv"},

		{ L"media",  L"<sound>,<video>"},

		{ L"doc",    L"*.chm,*.csv,*.djvu,*.doc,*.docx,*.dot,*.dot,*.epub,*.fb2,*.fb2.zip,*.hlp,*.hta,*.htm,*.html,*.htz,*.key,*.lex,*.log,*.man,*.mcw,*.md,"
                     L"*.mht,*.mhtml,*.nfo,*.odc,*.odc,*.odf,*.odf,*.odg,*.odg,*.odi,*.odi,*.odm,*.odm,*.odp,*.odp,*.ods,*.ods,*.odt,*.odt,*.otf,*.otf,*.otg,"
					 L"*.otg,*.oth,*.oth,*.oti,*.oti,*.otp,*.otp,*.ots,*.ots,*.ott,*.ott,*.pdf,*.pip,*.pps,*.ppt,*.ppt,*.pptx,*.rst,*.rtf,*.sdc,*.sdc,*.srt,"
					 L"*.stc,*.stc,*.sti,*.sti,*.stw,*.stw,*.sxc,*.sxc,*.sxi,*.sxi,*.sxw,*.sxw,*.tex,*.txt,*.wiz,*.wpd,*.wri,*.xlk,*.xls,*.xlsb,*.xlsm,*.xlsx,"
					 L"*.xlsx,*.xlt,*.xltm,*.xslm,NEWS,read.me,readme*"},

		{ L"src",    L"<build>,<cfg>,<code>"},

		{ L"build",	 L"*.gradle,makefile,*.cmake,*.make,makefile*,*.sln,*.mak,*.mk,*.in,*.mpp,*.mpt,*.mpw,*.mpx,*.mpd,*.mpp,*.vcxproj*,*.vcproj,*.project,*.prj,"
					 L"*.pro,*.dsp,*.cue,CMakeLists.txt"},

		{ L"cfg",    L"*.cfg,*.conf,*.config,*.fm[li],*.hlf,*.hrc,*.hrd,*.inf,*.ini,*.json,*.manifest,*.reg,*.sln,*.toml,*.xml,*.xsd,*.xsl,*.xslt,*.yaml,*.yml"},

		{ L"code",	 L"*.applescript,*.as,*.asa,*.asax,*.asm,*.awk,*.bas,*.bash,*.bsh,*.c,*.c++,*.cabal,*.cc,*.cgi,*.clj,*.cp,*.cpp,"
					 L"*.cr,*.cs,*.css,*.csx,*.cxx,*.d,*.dart,*.def,*.di,*.diff,*.dot,*.dpr,*.el,*.elc,*.elm,*.epp,*.erl,*.ex,*.exs,*.fish,*.frag,*.fragment,"
					 L"*.fragmentshader,*.fs,*.fsh,*.fsi,*.fsx,*.fx,*.geom,*.glsl,*.go,*.groovy,*.gv,*.gvy,*.h,*.h,*.h++,*.hh,*.hlsl,*.hpp,*.hs,*.htc,"
					 L"*.hxx,*.inc,*.inl,*.ipp,*.ipynb,*.jam,*.java,*.jl,*.js,*.jsm,*.kt,*.kts,*.less,*.lisp,*.ll,*.ltx,*.lua,*.m,*.m4"
					 L"*.matlab,*.mir,*.ml,*.mli,*.mn,*.nb,*.p,*.pas,*.patch,*.perl,*.php,*.pixel,*.pl,*.pm,*.pod,*.pp,*.prf,*.ps1,*.psd1,*.psm1,*.purs,*.py,"
					 L"*.qml,*.r,*.r,*.rb,*.reg,*.rs,*.sample,*.sass,*.sbt,*.scala,*.scss,*.sh,*.sql,*.src,*.swift,*.t,*.tcl,*.td,*.tex,*.ts,*.tsx,*.vb,"
					 L"*.vert,*.vertex,*.vertexshader,*.vs,*.vsh,*.wsdl,*.xaml,*.xhtml,*.zsh,PKGBUILD"},

		{ L"3d",     L"*.3d,*.3df,*.3dm,*.3ds,*.3dxml,*.amf,*.asymptote,*.blend,*.collada,*.dds,*.dgn,*.drawings,*.dwf,*.dwg,*.dxf,*.flt,*.fvrml,*.gltf,*.hdr,"
					 L"*.hsf,*.iges,*.imml,*.ipa,*.jt,*.ktx,*.ma,*.mb,*.opengex,*.ply,*.pov-ray,*.prc,*.sdkmesh,*.skp,*.step,*.stl,*.u3d,*.vrml,*.x,*.x3d,"
					 L"*.x3d,*.xaml,*.xgl,*.xvl,*.xvrml"},
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

				case KEY_F3:
					if (MenuPos != -1) { // view the current group with wrap long line of masks
						ConfigReader cfg_reader;
						cfg_reader.SelectSectionFmt(FMS.TypeFmt, MenuPos);

						FARString strTitle;
						strTitle = Msg::MaskGroupTitle;
						strTitle.AppendFormat(L" \"%ls\"", cfg_reader.GetString(FMS.MaskName).CPtr());

						ExMessager em(strTitle);

						FARString fs, fsmask = cfg_reader.GetString(FMS.MaskValue);
						em.AddDup(Msg::MaskGroupBeforeExpand);
						em.AddDupWrap(fsmask);

						// expand all groups
						unsigned ngroups = GetMaskGroupExpandRecursiveAll(fsmask);
						em.AddDup(L"\x1");
						fs = Msg::MaskGroupCountExpandedGroups;
						fs.AppendFormat(L" %u", ngroups);
						em.AddDup(fs);
						em.AddDup(L"");
						em.AddDup(Msg::MaskGroupAfterExpand);
						em.AddDupWrap(fsmask);

						em.AddDup(Msg::Ok);
						em.Show(MSG_LEFTALIGN, 1);
					}
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

// parameters:
//  fsMasks - initial string with file masks and groups
// return value:
//  >= 0:    number of expanded subgroups
//  fsMasks: masks after recursive expanded all subgroups
unsigned GetMaskGroupExpandRecursiveAll(FARString &fsMasks)
{
	// expand all groups
	unsigned ngroups = 0;
	size_t pos_open, pos_close = 0;
	FARString fs_group_name, fs_masks_from_group;
	for( ;; ) {
		if( !fsMasks.Pos(pos_open, '<', pos_close) )
			break;
		if( !fsMasks.Pos(pos_close, '>', pos_open+1) )
			break;
		if( pos_close-pos_open < 2 )
			continue;
		fs_group_name = fsMasks.SubStr(pos_open+1, pos_close-pos_open-1);
		if( !GetMaskGroup(fs_group_name, fs_masks_from_group) )
			continue;
		fsMasks.Replace(pos_open, pos_close-pos_open+1, fs_masks_from_group);
		pos_close = pos_open; // may be need recursive expand
		ngroups++;
	}
	return ngroups;
}
