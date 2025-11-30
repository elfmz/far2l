#include "headers.hpp"

#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common.hpp"
#include "archive.hpp"
#include "options.hpp"
#include "SimpleXML.hpp"

using Rt = SimpleXML::cbRet;
using Sv = SimpleXML::str_view;
using SimpleXML::operator""_v;

static const auto codecs_v = "Codecs.7z"_v;
static const auto default_v = "Options"_v;

static const auto max_check_size_v = "max_check_size"_v;
static const auto correct_name_mode_v = "correct_name_mode"_v;
static const auto qs_by_default_v = "qs_by_default"_v;
static const auto strict_case_v = "strict_case"_v;

static const auto range_v = "range"_v;
static const auto std_v = "std"_v;
static const auto bcj_v = "bcj"_v;
static const auto adv_v = "adv"_v;
static const auto true_v = "true"_v;

static std::wstring utf8_to_wstring(const Sv view)
{
	auto nw = WINPORT_MultiByteToWideChar(CP_UTF8, 0, view.data(), static_cast<int>(view.size()), nullptr, 0);
	if (nw <= 0)
		return std::wstring();
	std::wstring ret;
	ret.resize(static_cast<size_t>(nw));
	WINPORT_MultiByteToWideChar(CP_UTF8, 0, view.data(), static_cast<int>(view.size()), &ret.front(), nw);
	return ret;
}

static void parse_uints(const Sv &view, unsigned *p1, unsigned *p2 = nullptr, unsigned *p3 = nullptr,
		unsigned *p4 = nullptr, unsigned *p5 = nullptr)
{
	unsigned *pointers[] = {p1, p2, p3, p4, p5};
	const char *ps = view.begin(), *pe = view.end();
	for (auto pu : pointers) {
		if (!pu)
			break;
		while (ps < pe && *ps <= ' ')
			++ps;
		if (ps < pe && *ps >= '0' && *ps <= '9') {
			char *pn = const_cast<char *>(ps);
			*pu = strtoul(ps, &pn, 0);
			ps = pn;
		}
		while (ps < pe && *ps <= ' ')
			++ps;
		if (ps >= pe || (*ps != ',' && *ps != ';'))
			break;
		++ps;
	}
}

class xml_parser : public SimpleXML::IParseCallback
{
private:
	Options &options;
	mutable ExternalCodec codec;

public:
	xml_parser(Options &opt) : options(opt) {}

	Rt OnTag(int top, const Sv *path, const Sv & /*attrs*/) const override
	{
		if (top == 2 && path[1] == codecs_v) {
			codec.reset();
			codec.name = utf8_to_wstring(path[2]);
		}
		return Rt::Continue;
	}
	Rt OnBody(int top, const Sv *path, const Sv & /*body*/) const override
	{
		if (top == 2 && path[1] == codecs_v && !codec.name.empty()) {
			int i = static_cast<int>(options.codecs.size());
			while (--i >= 0) {
				if (codec.name == options.codecs[i].name)
					break;
			}
			if (i < 0)
				options.codecs.push_back(codec);
			else
				options.codecs[i] = codec;
		}
		return Rt::Continue;
	}
	Rt OnAttr(int top, const Sv *path, const Sv &name, const Sv &value) const override
	{
		if (top == 2 && path[1] == codecs_v) {
			if (name == range_v)
				parse_uints(value, &codec.minL, &codec.maxL, &codec.mod0L);
			else if (name == std_v)
				parse_uints(value, &codec.L1, &codec.L3, &codec.L5, &codec.L7, &codec.L9);
			else if (name == bcj_v)
				codec.bcj_only = value == true_v;
			else if (name == adv_v)
				codec.adv = utf8_to_wstring(value);
		} else if (top == 1 && path[1] == default_v) {
			if (name == max_check_size_v) {
				parse_uints(value, &options.max_check_size);
				options.loaded_from_xml.max_check_size = true;
			} else if (name == qs_by_default_v) {
				options.qs_by_default = value == true_v;
				options.loaded_from_xml.qs_by_default = true;
			} else if (name == strict_case_v) {
				options.strict_case = value == true_v;
				options.loaded_from_xml.strict_case = true;
			} else if (name == correct_name_mode_v) {
				parse_uints(value, &options.correct_name_mode);
				options.loaded_from_xml.correct_name_mode = true;
			}
		}
		return Rt::Continue;
	}

	bool parse(const std::wstring &xml_name)
	{
		File file;
		if (!file.open_nt(xml_name, FILE_READ_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING, 0))
			return false;
		UInt64 fsize = 0;
		if (!file.size_nt(fsize) || fsize <= 0 || fsize > 100ull * 1024 * 1024)
			return false;
		size_t nr = static_cast<size_t>(fsize);
		char *buffer = new char[nr];
		bool ok = file.read_nt(buffer, nr, nr) && nr == static_cast<size_t>(fsize);
		file.close();
		if (ok)
			ok = SimpleXML::parse(buffer, nr, this) == SimpleXML::parseRet::Ok;
		delete[] buffer;
		return ok;
	}
};

static bool load_arclite_xml(Options &options)
{
	xml_parser xml(options);
//	auto plugin_path = add_trailing_slash(Far::get_plugin_module_path());
	auto plugin_path = StrMB2Wide(InMyConfig("plugins/arclite/"));
	auto r1 = xml.parse(plugin_path + L"arclite.xml");



//StrMB2Wide(symlinkdata).c_str()

//	xml.parse(plugin_path + L"arclite.xml.custom");
	//	settingsIni = InMyConfig("plugins/arclite/arclite.ini");
	//  auto profile_path = expand_env_vars(L"%FARPROFILE%");
	//  if (!profile_path.empty())
	//    xml.parse(add_trailing_slash(profile_path) + L"arclite.xml.custom");
	return r1;
}

class OptionsKey : public Far::Settings
{
public:
	template <class Integer>
	Integer get_int(const wchar_t *name, Integer def_value)
	{
		UInt64 value;
		if (get(name, value))
			return static_cast<Integer>(value);
		else
			return def_value;
	}

	bool get_bool(const wchar_t *name, bool def_value)
	{
		int value;
		if (get(name, value))
			return value != 0;
		else
			return def_value;
	}

	std::wstring get_str(const wchar_t *name, const std::wstring &def_value)
	{
		std::wstring value;
		if (get(name, value))
			return value;
		else
			return def_value;
	}

	ByteVector get_binary(const wchar_t *name, const ByteVector &def_value)
	{
		ByteVector value;
		if (get(name, value))
			return value;
		else
			return def_value;
	}

	void set_filetime(const wchar_t *name, FILETIME value, FILETIME def_value)
	{
		union {
			FILETIME fvalue;
			UInt64 value;
		} u;
		u.fvalue = value;
		set(name, (UInt64)u.value);
	}

	FILETIME get_filetime(const wchar_t *name, FILETIME def_value)
	{
		union {
			FILETIME fvalue;
			UInt64 value;
		} u;
		if (get(name, u.value))
			return u.fvalue;
		else
			return def_value;
	}

//	void set_int(const wchar_t *name, uintptr_t value, uintptr_t def_value)
//	{
//		set(name, (UInt64)value);
//	}

	void set_int64(const wchar_t *name, uint64_t value, uint64_t def_value)
	{
		set(name, (UInt64)value);
	}

	void set_int(const wchar_t *name, int value, int def_value)
	{
		set(name, value);
	}

	void set_int(const wchar_t *name, unsigned int value, unsigned int def_value)
	{
		set(name, value);
	}

	void set_bool(const wchar_t *name, bool value, bool def_value)
	{
		set(name, value ? (int)1 : (int)0);
	}

	void set_str(const wchar_t *name, const std::wstring &value, const std::wstring &def_value)
	{
		set(name, value);
	}

	void set_binary(const wchar_t *name, const ByteVector &value, const ByteVector &def_value)
	{
		set(name, value.data(), value.size());
	}
};

ExportOptions::ExportOptions() :
	export_creation_time(triUndef),
	custom_creation_time(false),
	current_creation_time(false),
    ftCreationTime( {0,0} ),
	export_last_access_time(triUndef),
	custom_last_access_time(false),
	current_last_access_time(false),
    ftLastAccessTime( {0,0} ),
	export_last_write_time(triUndef),
	custom_last_write_time(false),
	current_last_write_time(false),
    ftLastWriteTime( {0,0} ),
	export_user_id(true),
	custom_user_id(false),
    UnixOwner(0),
	export_group_id(true),
	custom_group_id(false),
    UnixGroup(0),
	export_user_name(true),
	custom_user_name(false),
	Owner(),
	export_group_name(true),
	custom_group_name(false),
	Group(),
	export_unix_device(true),
	custom_unix_device(false),
    UnixDevice(0),
	export_unix_mode(true),
	custom_unix_mode(false),
    UnixNode(0x1FF),
	export_file_attributes(true),
    dwExportAttributesMask(0xFFFFFFFF),
	custom_file_attributes(false),
    dwFileAttributes(0),
	export_file_descriptions(false)
{}

SfxOptions::SfxOptions() : replace_icon(false), replace_version(false), append_install_config(false) {}

Options g_options;

Options::Options()
	: plugin_enabled(false),
	handle_create(true),
	handle_commands(true),
	preferred_7zip_path(L"/usr/lib/7zip/"),
	plugin_prefix(L"arc"),
	max_check_size(1 << 20),
	relay_buffer_size(64),
	max_arc_cache_size(128),
	extract_ignore_errors(false),
	extract_access_rights(true),
	extract_owners_groups(0),
	extract_attributes(false),
	extract_duplicate_hardlinks(false),
	extract_restore_special_files(false),
	extract_overwrite(oaAsk),
	extract_separate_dir(triUndef),
	extract_open_dir(false),
	update_arc_format_name(L"7z"),
	update_arc_repack_format_name(L"xz"),
	update_level(5),
	update_levels(L"7z=7;zip=5;bzip2=9;xz=5;wim=0;tar=0;gzip=5"),
	update_method(L"LZMA2"),
	update_multithreading(true),
	update_process_priority(2),
	update_threads_num(0),
	update_repack(false),
	update_solid(true),
	update_encrypt(false),
	update_show_password(false),
	update_encrypt_header(triUndef),
	update_create_sfx(false),
	update_sfx_options(),
	update_export_options(),
	update_use_export_settings(false),
	update_enable_volumes(false),
	update_volume_size(),
	update_skip_symlinks(false),
	update_symlink_fix_path_mode(0),
	update_dereference_symlinks(false),
	update_skip_hardlinks(false),
	update_duplicate_hardlinks(false),
	update_move_files(false),
	update_ignore_errors(false),
	update_overwrite(oaAsk),
	update_append_ext(false),
	confirm_esc_interrupt_operation(true),
	own_panel_view_mode(true),
	panel_view_mode(2),
	panel_sort_mode(SM_NAME),
	panel_reverse_sort(false),
	use_include_masks(false),

#if 0
	include_masks(L"*.zip,*.z01,*.zipx,*.j[ar],*.xp[i],*.apk,*.appx,*.[7btr]z*,*.t[btx][2z],"
					L"*.r[0-9][0-9],*.a[rjz],*.lzh*,*.lh[a],*.cab,*.nsis,*.lzma*,*.xz,*.tzst,*.zst,*.lpi[mg],*.simg,*.ap[fhs],*.img,"
					L"*.vh[dxi],*.avhdx,*.b64,*.obj,*.ext[234],*.vmdk,*.qcow[2c]?,*.gpt,*.mbr,*.ihex,*.hx[swrq],*.lit,*.te,*.scap,*.uefif,"
					L"*.squashfs,*.cramfs,*.apm,*.mslz,*.flv,*.swf,*.ntfs,*.fat,*.exe,*.dll,*.sys,*.elf,*.macho,*.udf,*.iso,*.xar,*.pkg,*.xip,"
					L"*.mub,*.hf[sx],*.dmg,*.msi,*.msp,*.001,*.rp[m],"
					L"*.de[b],*.udeb,*.lib,*.cpio,*.tar,*.gzi[p],*.tgz,*.tpz";

#endif

	include_masks(L"*.zip,*.rar,*.[7bgx]z,*.[bg]zip,*.tar,*.t[agbx]z,*.z,*.ar[cj],*.r[0-9][0-9],*.a[0-9][0-"
					L"9],*.bz2,*.cab,*.jar,*.lha,*.lzh,*.ha,*.ac[bei],*.pa[ck],*.rk,*.cpio,*.rpm,*.zoo,*.hqx,"
					L"*.sit,*.ice,*.uc2,*.ain,*.imp,*.777,*.ufa,*.boa,*.bs[2a],*.sea,*.[ah]pk,*.ddi,*.x2,*."
					L"rkv,*.[lw]sz,*.h[ay]p,*.lim,*.sqz,*.chz,*.aa[br]"),

//include_masks(L"*.7z,*.a00,*.a09,*.a[arjz],*.ac[bei],*.apk,*.appx,*.ap[fhs],*.avhdx,*.b64,*.bzip2?,*.bz2,"
//                    L"*.cab,*.cramfs,*.cpio,*.dmg,*.dll,*.ddi,*.de[b],*.elf,*.exe,*.fat,*.flv,*.gpt,*.gzi[p],*.ha,"
//                    L"*.hf[sx],*.hx[swrq],*.ihex,*.img,*.iso,*.jar,*.lha,*.lh[a],*.lib,*.lpi[mg],*.lzma*,*.mub,"
//                    L"*.macho,*.msi,*.msp,*.mslz,*.nsis,*.obj,*.pkg,*.qcow[2c]?,*.r[0-9][0-9],*.rpm,*.rz,*.simg,"
//                    L"*.squashfs,*.scap,*.sys,*.tar,*.te,*.tgz,*.tpz,*.t[btx][2z],*.tzst,*.udf,*.uefif,*.udef,"
//                    L"*.vh[dxi],*.vmdk,*.wim,*.xar,*.xip,*.xz,*.z,*.z01,*.zoo,*.zst,*.zip,*.zipx");

	use_exclude_masks(false),
	exclude_masks(),
	pgdn_masks(false),
	use_enabled_formats(false),
	enabled_formats(),
	use_disabled_formats(false),
	disabled_formats(),
	pgdn_formats(false),
	patchCP(false),
	oemCP(Patch7zCP::GetDefCP_OEM()),
	ansiCP(Patch7zCP::GetDefCP_ANSI()),
	correct_name_mode(0x12),
	qs_by_default(false),
	strict_case(true)
{}

static void load_export_options(OptionsKey &key, ExportOptions &export_options)
{
//	fprintf(stderr, "load_export_options()\n");
	ExportOptions def_export_options;
#define GET_VALUE(name, type) export_options.name = key.get_##type(L"export." L## #name, def_export_options.name)
	GET_VALUE(export_creation_time, int);
	GET_VALUE(custom_creation_time, bool);
	GET_VALUE(current_creation_time, bool);
	GET_VALUE(ftCreationTime, filetime);
	GET_VALUE(export_last_access_time, int);
	GET_VALUE(custom_last_access_time, bool);
	GET_VALUE(current_last_access_time, bool);
	GET_VALUE(ftLastAccessTime, filetime);
	GET_VALUE(export_last_write_time, int);
	GET_VALUE(custom_last_write_time, bool);
	GET_VALUE(current_last_write_time, bool);
	GET_VALUE(ftLastWriteTime, filetime);
	GET_VALUE(export_user_id, bool);
	GET_VALUE(custom_user_id, bool);
	GET_VALUE(UnixOwner, int);
	GET_VALUE(export_group_id, bool);
	GET_VALUE(custom_group_id, bool);
	GET_VALUE(UnixGroup, int);
	GET_VALUE(export_user_name, bool);
	GET_VALUE(custom_user_name, bool);
	GET_VALUE(Owner, str);
	GET_VALUE(export_group_name, bool);
	GET_VALUE(custom_group_name, bool);
	GET_VALUE(Group, str);
	GET_VALUE(export_unix_device, bool);
	GET_VALUE(custom_unix_device, bool);
	GET_VALUE(UnixDevice, int);
	GET_VALUE(export_unix_mode, bool);
	GET_VALUE(custom_unix_mode, bool);
	GET_VALUE(UnixNode, int);
	GET_VALUE(export_file_attributes, bool);
	GET_VALUE(dwExportAttributesMask, int);
	GET_VALUE(custom_file_attributes, bool);
	GET_VALUE(dwFileAttributes, int);
	GET_VALUE(export_file_descriptions, bool);
#undef GET_VALUE
}

static void save_export_options(OptionsKey &key, const ExportOptions &export_options)
{
//	fprintf(stderr, "save_export_options()\n");

	ExportOptions def_export_options;
#define SET_VALUE(name, type) key.set_##type(L"export." L## #name, export_options.name, def_export_options.name)
	SET_VALUE(export_creation_time, int);
	SET_VALUE(custom_creation_time, bool);
	SET_VALUE(current_creation_time, bool);
	SET_VALUE(ftCreationTime, filetime);
	SET_VALUE(export_last_access_time, int);
	SET_VALUE(custom_last_access_time, bool);
	SET_VALUE(current_last_access_time, bool);
	SET_VALUE(ftLastAccessTime, filetime);
	SET_VALUE(export_last_write_time, int);
	SET_VALUE(custom_last_write_time, bool);
	SET_VALUE(current_last_write_time, bool);
	SET_VALUE(ftLastWriteTime, filetime);
	SET_VALUE(export_user_id, bool);
	SET_VALUE(custom_user_id, bool);
	SET_VALUE(UnixOwner, int);
	SET_VALUE(export_group_id, bool);
	SET_VALUE(custom_group_id, bool);
	SET_VALUE(UnixGroup, int);
	SET_VALUE(export_user_name, bool);
	SET_VALUE(custom_user_name, bool);
	SET_VALUE(Owner, str);
	SET_VALUE(export_group_name, bool);
	SET_VALUE(custom_group_name, bool);
	SET_VALUE(Group, str);
	SET_VALUE(export_unix_device, bool);
	SET_VALUE(custom_unix_device, bool);
	SET_VALUE(UnixDevice, int64);
	SET_VALUE(export_unix_mode, bool);
	SET_VALUE(custom_unix_mode, bool);
	SET_VALUE(UnixNode, int64);
	SET_VALUE(export_file_attributes, bool);
	SET_VALUE(dwExportAttributesMask, int);
	SET_VALUE(custom_file_attributes, bool);
	SET_VALUE(dwFileAttributes, int);
	SET_VALUE(export_file_descriptions, bool);
#undef SET_VALUE
}


static void load_sfx_options(OptionsKey &key, SfxOptions &sfx_options)
{
//	fprintf(stderr, "load_sfx_options()\n");
	SfxOptions def_sfx_options;
#define GET_VALUE(name, type) sfx_options.name = key.get_##type(L"sfx." L## #name, def_sfx_options.name)
	GET_VALUE(name, str);
	GET_VALUE(replace_icon, bool);
	GET_VALUE(icon_path, str);
	GET_VALUE(replace_version, bool);
	GET_VALUE(ver_info.version, str);
	GET_VALUE(ver_info.comments, str);
	GET_VALUE(ver_info.company_name, str);
	GET_VALUE(ver_info.file_description, str);
	GET_VALUE(ver_info.legal_copyright, str);
	GET_VALUE(ver_info.product_name, str);
	GET_VALUE(append_install_config, bool);
	GET_VALUE(install_config.title, str);
	GET_VALUE(install_config.begin_prompt, str);
	GET_VALUE(install_config.progress, str);
	GET_VALUE(install_config.run_program, str);
	GET_VALUE(install_config.directory, str);
	GET_VALUE(install_config.execute_file, str);
	GET_VALUE(install_config.execute_parameters, str);
#undef GET_VALUE
}

static void save_sfx_options(OptionsKey &key, const SfxOptions &sfx_options)
{
//	fprintf(stderr, "save_sfx_options()\n");

	SfxOptions def_sfx_options;
#define SET_VALUE(name, type) key.set_##type(L"sfx." L## #name, sfx_options.name, def_sfx_options.name)
	SET_VALUE(name, str);
	SET_VALUE(replace_icon, bool);
	SET_VALUE(icon_path, str);
	SET_VALUE(replace_version, bool);
	SET_VALUE(ver_info.version, str);
	SET_VALUE(ver_info.comments, str);
	SET_VALUE(ver_info.company_name, str);
	SET_VALUE(ver_info.file_description, str);
	SET_VALUE(ver_info.legal_copyright, str);
	SET_VALUE(ver_info.product_name, str);
	SET_VALUE(append_install_config, bool);
	SET_VALUE(install_config.title, str);
	SET_VALUE(install_config.begin_prompt, str);
	SET_VALUE(install_config.progress, str);
	SET_VALUE(install_config.run_program, str);
	SET_VALUE(install_config.directory, str);
	SET_VALUE(install_config.execute_file, str);
	SET_VALUE(install_config.execute_parameters, str);
#undef SET_VALUE
}

bool Options::load()
{
//	fprintf(stderr, "Options::load()\n");
	OptionsKey key;
	key.create();
	Options def_options;

//	fprintf(stderr, "load_arclite_xml\n");
	load_arclite_xml(*this);

#define GET_VALUE(name, type) name = key.get_##type(L## #name, def_options.name)
#define GET_VALUE_XML(name, type)                                                                            \
	if (!loaded_from_xml.name)                                                                               \
	GET_VALUE(name, type)
	GET_VALUE(plugin_enabled, bool);
	GET_VALUE(handle_create, bool);
	GET_VALUE(handle_commands, bool);
	GET_VALUE(preferred_7zip_path, str);
	GET_VALUE(plugin_prefix, str);
	GET_VALUE_XML(max_check_size, int);
	GET_VALUE(relay_buffer_size, int);
	GET_VALUE(max_arc_cache_size, int);
	GET_VALUE(extract_ignore_errors, bool);
	GET_VALUE(extract_ignore_errors, bool);
	GET_VALUE(extract_access_rights, bool);
	GET_VALUE(extract_owners_groups, int);
	GET_VALUE(extract_attributes, bool);
	GET_VALUE(extract_duplicate_hardlinks, bool);
	GET_VALUE(extract_restore_special_files, bool);
	GET_VALUE(extract_overwrite, int);
	GET_VALUE(extract_separate_dir, int);
	GET_VALUE(extract_open_dir, bool);
	GET_VALUE(update_arc_format_name, str);
	GET_VALUE(update_arc_repack_format_name, str);
	GET_VALUE(update_level, int);
	GET_VALUE(update_levels, str);
	GET_VALUE(update_method, str);
	GET_VALUE(update_multithreading, bool);
	GET_VALUE(update_process_priority, int);
	GET_VALUE(update_threads_num, int);
	GET_VALUE(update_repack, bool);
	GET_VALUE(update_solid, bool);
	GET_VALUE(update_advanced, str);
	GET_VALUE(update_encrypt, bool);
	GET_VALUE(update_show_password, bool);
	GET_VALUE(update_encrypt_header, int);
	GET_VALUE(update_password, str);
	GET_VALUE(update_create_sfx, bool);
	load_sfx_options(key, update_sfx_options);
	load_export_options(key, update_export_options);
	GET_VALUE(update_use_export_settings, bool);
	GET_VALUE(update_enable_volumes, bool);
	GET_VALUE(update_volume_size, str);
	GET_VALUE(update_skip_symlinks, bool);
	GET_VALUE(update_symlink_fix_path_mode, int);
	GET_VALUE(update_dereference_symlinks, bool);
	GET_VALUE(update_skip_hardlinks, bool);
	GET_VALUE(update_duplicate_hardlinks, bool);
	GET_VALUE(update_move_files, bool);
	GET_VALUE(update_ignore_errors, bool);
	GET_VALUE(update_overwrite, int);
	GET_VALUE(update_append_ext, bool);
	GET_VALUE(confirm_esc_interrupt_operation, bool);
	GET_VALUE(own_panel_view_mode, bool);
	GET_VALUE(panel_view_mode, int);
	GET_VALUE(panel_sort_mode, int);
	GET_VALUE(panel_reverse_sort, bool);
	GET_VALUE(use_include_masks, bool);
	GET_VALUE(include_masks, str);
	GET_VALUE(use_exclude_masks, bool);
	GET_VALUE(exclude_masks, str);
	GET_VALUE(pgdn_masks, bool);
	GET_VALUE(use_enabled_formats, bool);
	GET_VALUE(enabled_formats, str);
	GET_VALUE(use_disabled_formats, bool);
	GET_VALUE(disabled_formats, str);
	GET_VALUE(pgdn_formats, bool);
	GET_VALUE(patchCP, bool);
	GET_VALUE(oemCP, int);
	GET_VALUE(ansiCP, int);
	GET_VALUE_XML(correct_name_mode, int);
	GET_VALUE_XML(qs_by_default, bool);
	GET_VALUE_XML(strict_case, bool);
#undef GET_VALUE
#undef GET_VALUE_XML
	return key.have_dir(L"Settings");
};

void Options::save() const
{
//	fprintf(stderr, "Options::save()\n");
	OptionsKey key;
	key.create();
	Options def_options;

#define SET_VALUE(name, type) key.set_##type(L## #name, name, def_options.name)
#define SET_VALUE_XML(name, type)                                                                            \
	if (!loaded_from_xml.name)                                                                               \
	SET_VALUE(name, type)
	SET_VALUE(plugin_enabled, bool);
	SET_VALUE(handle_create, bool);
	SET_VALUE(handle_commands, bool);
	SET_VALUE(preferred_7zip_path, str);
	SET_VALUE(plugin_prefix, str);
	SET_VALUE_XML(max_check_size, int);
	SET_VALUE(relay_buffer_size, int);
	SET_VALUE(max_arc_cache_size, int);
	SET_VALUE(extract_ignore_errors, bool);
	SET_VALUE(extract_access_rights, bool);
	SET_VALUE(extract_owners_groups, int);
	SET_VALUE(extract_attributes, bool);
	SET_VALUE(extract_duplicate_hardlinks, bool);
	SET_VALUE(extract_restore_special_files, bool);
	SET_VALUE(extract_overwrite, int);
	SET_VALUE(extract_separate_dir, int);
	SET_VALUE(extract_open_dir, bool);
	SET_VALUE(update_arc_format_name, str);
	SET_VALUE(update_arc_repack_format_name, str);
	SET_VALUE(update_level, int);
	SET_VALUE(update_levels, str);
	SET_VALUE(update_method, str);
	SET_VALUE(update_multithreading, bool);
	SET_VALUE(update_process_priority, int);
	SET_VALUE(update_threads_num, int);
	SET_VALUE(update_repack, bool);
	SET_VALUE(update_solid, bool);
	SET_VALUE(update_advanced, str);
	SET_VALUE(update_encrypt, bool);
	SET_VALUE(update_show_password, bool);
	SET_VALUE(update_encrypt_header, int);
	SET_VALUE(update_password, str);
	SET_VALUE(update_create_sfx, bool);
	save_sfx_options(key, update_sfx_options);
	save_export_options(key, update_export_options);
	SET_VALUE(update_use_export_settings, bool);
	SET_VALUE(update_enable_volumes, bool);
	SET_VALUE(update_volume_size, str);
	SET_VALUE(update_skip_symlinks, bool);
	SET_VALUE(update_symlink_fix_path_mode, int);
	SET_VALUE(update_dereference_symlinks, bool);
	SET_VALUE(update_skip_hardlinks, bool);
	SET_VALUE(update_duplicate_hardlinks, bool);
	SET_VALUE(update_move_files, bool);
	SET_VALUE(update_ignore_errors, bool);
	SET_VALUE(update_overwrite, int);
	SET_VALUE(update_append_ext, bool);
	SET_VALUE(confirm_esc_interrupt_operation, bool);
	SET_VALUE(own_panel_view_mode, bool);
	SET_VALUE(panel_view_mode, int);
	SET_VALUE(panel_sort_mode, int);
	SET_VALUE(panel_reverse_sort, bool);
	SET_VALUE(use_include_masks, bool);
	SET_VALUE(include_masks, str);
	SET_VALUE(use_exclude_masks, bool);
	SET_VALUE(exclude_masks, str);
	SET_VALUE(pgdn_masks, bool);
	SET_VALUE(use_enabled_formats, bool);
	SET_VALUE(enabled_formats, str);
	SET_VALUE(use_disabled_formats, bool);
	SET_VALUE(disabled_formats, str);
	SET_VALUE(pgdn_formats, bool);
	SET_VALUE(patchCP, bool);
	SET_VALUE(oemCP, int);
	SET_VALUE(ansiCP, int);
	SET_VALUE_XML(correct_name_mode, int);
	SET_VALUE_XML(qs_by_default, bool);
	SET_VALUE_XML(strict_case, bool);
#undef SET_VALUE
#undef SET_VALUE_XML
}
UpdateProfiles g_profiles;

const wchar_t c_profiles_key_name[] = L"profiles";

static std::wstring get_profile_key_name(const std::wstring &name)
{
	return add_trailing_slash(c_profiles_key_name) + name;
}

ProfileOptions::ProfileOptions()
	: arc_type(c_7z),
	repack_arc_type(c_xz),
	level(5),
	method(),
	multithreading(true),
	process_priority(2),
	threads_num(0),
	repack(),
	solid(false),
	password(),
	encrypt(false),
	encrypt_header(triUndef),
	create_sfx(false),
	use_export_settings(false),
	enable_volumes(false),
	volume_size(),
	skip_symlinks(false),
	symlink_fix_path_mode(0),
	dereference_symlinks(false),
	skip_hardlinks(false),
	duplicate_hardlinks(false),
	move_files(false),
	ignore_errors(false),
	advanced()
{}

UpdateOptions::UpdateOptions()
	: arc_path(), show_password(false), open_shared(false), overwrite(oaAsk), append_ext(false)
{}

void UpdateProfiles::load()
{
//	fprintf(stderr, "UpdateProfiles::load()\n");
	clear();
	OptionsKey key;
	key.create();
	key.set_dir(c_profiles_key_name);
	std::vector<std::wstring> profile_names;
	key.list_dir(profile_names);
	ProfileOptions def_profile_options;
	for (unsigned i = 0; i < profile_names.size(); i++) {
		UpdateProfile profile;
		profile.name = extract_file_name(profile_names[i]);
		key.set_dir(get_profile_key_name(profile.name));
#define GET_VALUE(name, type) profile.options.name = key.get_##type(L## #name, def_profile_options.name)
		GET_VALUE(arc_type, binary);
		GET_VALUE(repack_arc_type, binary);
		GET_VALUE(level, int);
		GET_VALUE(method, str);
		GET_VALUE(multithreading, bool);
		GET_VALUE(process_priority, int);
		GET_VALUE(threads_num, int);
		GET_VALUE(repack, bool);
		GET_VALUE(solid, bool);
		GET_VALUE(password, str);
		GET_VALUE(encrypt, bool);
		GET_VALUE(encrypt_header, int);
		GET_VALUE(create_sfx, bool);
		load_sfx_options(key, profile.options.sfx_options);
		load_export_options(key, profile.options.export_options);
		GET_VALUE(use_export_settings, bool);
		GET_VALUE(enable_volumes, bool);
		GET_VALUE(volume_size, str);
		GET_VALUE(skip_symlinks, bool);
		GET_VALUE(symlink_fix_path_mode, int);
		GET_VALUE(dereference_symlinks, bool);
		GET_VALUE(skip_hardlinks, bool);
		GET_VALUE(duplicate_hardlinks, bool);
		GET_VALUE(move_files, bool);
		GET_VALUE(ignore_errors, bool);
		GET_VALUE(advanced, str);
#undef GET_VALUE
		push_back(profile);
	}
	sort_by_name();
}

void UpdateProfiles::save() const
{
//	fprintf(stderr, "UpdateProfiles::save()\n");
	OptionsKey key;
	key.create();
	key.set_dir(c_profiles_key_name);
	std::vector<std::wstring> profile_names;
	key.list_dir(profile_names);
	for (unsigned i = 0; i < profile_names.size(); i++) {
		key.del_dir(profile_names[i].c_str());
	}
	ProfileOptions def_profile_options;
	std::for_each(cbegin(), cend(), [&](const UpdateProfile &profile) {
		key.set_dir(get_profile_key_name(profile.name));
#define SET_VALUE(name, type) key.set_##type(L## #name, profile.options.name, def_profile_options.name)
		SET_VALUE(arc_type, binary);
		SET_VALUE(repack_arc_type, binary);
		SET_VALUE(level, int);
		SET_VALUE(method, str);
		SET_VALUE(multithreading, bool);
		SET_VALUE(process_priority, int);
		SET_VALUE(threads_num, int);
		SET_VALUE(repack, bool);
		SET_VALUE(solid, bool);
		SET_VALUE(password, str);
		SET_VALUE(encrypt, bool);
		SET_VALUE(encrypt_header, int);
		SET_VALUE(create_sfx, bool);
		save_sfx_options(key, profile.options.sfx_options);
		save_export_options(key, profile.options.export_options);
		SET_VALUE(use_export_settings, bool);
		SET_VALUE(enable_volumes, bool);
		SET_VALUE(volume_size, str);
		SET_VALUE(skip_symlinks, bool);
		SET_VALUE(symlink_fix_path_mode, int);
		SET_VALUE(dereference_symlinks, bool);
		SET_VALUE(skip_hardlinks, bool);
		SET_VALUE(duplicate_hardlinks, bool);
		SET_VALUE(move_files, bool);
		SET_VALUE(ignore_errors, bool);
		SET_VALUE(advanced, str);
#undef SET_VALUE
	});
}

unsigned UpdateProfiles::find_by_name(const std::wstring &name)
{
	unsigned idx = 0;
	for (idx = 0; idx < size() && upcase(at(idx).name) != upcase(name); idx++)
		;
	return idx;
}

unsigned UpdateProfiles::find_by_options(const UpdateOptions &options)
{
	unsigned idx;
	for (idx = 0; idx < size() && !(at(idx).options == options); idx++)
		;
	return idx;
}

void UpdateProfiles::sort_by_name()
{
	sort(begin(), end(), [](const UpdateProfile &p1, const UpdateProfile &p2) -> bool {
		return upcase(p1.name) < upcase(p2.name);
	});
}

void UpdateProfiles::update(const std::wstring &name, const UpdateOptions &options)
{
	unsigned name_idx = find_by_name(name);
	unsigned options_idx = find_by_options(options);
	if (name_idx < size() && options_idx < size()) {
		at(name_idx).name = name;
		at(name_idx).options = options;
		if (options_idx != name_idx)
			erase(begin() + options_idx);
	} else if (name_idx < size()) {
		at(name_idx).name = name;
		at(name_idx).options = options;
	} else if (options_idx < size()) {
		at(options_idx).name = name;
		at(options_idx).options = options;
	} else {
		UpdateProfile new_profile;
		new_profile.name = name;
		new_profile.options = options;
		push_back(new_profile);
	}
	sort_by_name();
}

const wchar_t *c_copy_opened_files_option = L"CopyOpened";
const wchar_t *c_esc_confirmation_option = L"Esc";

bool get_app_option(size_t category, const wchar_t *name, bool &value)
{
	value = true;
	return true;

/**
	CHECK(get_app_option(FSSF_CONFIRMATIONS, c_esc_confirmation_option, confirm_esc));

	Far::Settings settings;

	if (!settings.create(true)) {
		return false;
	}

	UInt64 setting_value;
	if (!settings.get(category, name, setting_value)) {

		return false;
	}
	value = setting_value != 0;

**/

	return true;
}
