#include "headers.hpp"

#include "msg.hpp"
#include "version.hpp"
#include "guids.hpp"
#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common.hpp"
#include "ui.hpp"
#include "archive.hpp"
#include "options.hpp"
#include "cmdline.hpp"
#include "sfx.hpp"

INT_PTR g_plugin_id = 0;

static std::wstring g_plugin_prefix;
static TriState g_detect_next_time = triUndef;

static const wchar_t ext_FAT[] = L".fat";
static const wchar_t ext_ExFAT[] = L".xfat";
static const wchar_t ext_NTFS[] = L".ntfs";
static const wchar_t ext_EXT2[] = L".ext2";
static const wchar_t ext_EXT3[] = L".ext3";
static const wchar_t ext_EXT4[] = L".ext4";
static const wchar_t ext_EXTn[] = L".ext";
static const wchar_t ext_HFS[] = L".hfs";
static const wchar_t ext_APFS[] = L".apfs";

template<bool UseVirtualDestructor>
class Plugin
{
private:
	std::shared_ptr<Archive<UseVirtualDestructor>> archive;

	std::wstring extract_dir;
	std::wstring created_dir;
	std::wstring host_file;
	std::wstring panel_title;
	std::wstring arc_chain_str;

	std::vector<InfoPanelLine> info_lines;

	bool real_archive_file;
	bool need_close_panel;

public:
	std::wstring current_dir;
	char part_mode{'\0'};	 // '\0' 'm'br, 'g'pt
//	int part_idx{-1};		 // -1 or partition index
//	std::unique_ptr<Plugin<UseVirtualDestructor>> partition;
	std::unique_ptr<Plugin<UseVirtualDestructor>> child;
    Plugin<UseVirtualDestructor>* parent;

    Plugin<UseVirtualDestructor>* get_tail() {
        Plugin<UseVirtualDestructor>* current = this;
        while (current->child) {
            current = current->child.get();
        }
        return current;
    }
	Plugin<UseVirtualDestructor>* get_root() {
		Plugin<UseVirtualDestructor>* current = this;
		while (current->parent) {
			current = current->parent;
		}
		return current;
	}

	Archive<UseVirtualDestructor> *get_archive() {return archive.get();}

	bool recursive_panel{false};
	char del_on_close{'\0'};

public:
	Plugin(bool real_file = true)
		: archive(new Archive<UseVirtualDestructor>()), real_archive_file(real_file), need_close_panel(false),
      child(nullptr),
      parent(nullptr)
	{
		//fprintf(stderr, "Plugin()\n" );
	}

	~Plugin() {
		//fprintf(stderr, "~Plugin() DESTROY\n" );
	}

	static Plugin *open(const Archives<UseVirtualDestructor> &archives, bool real_file = true)
	{
		if (archives.size() == 0)
			FAIL(E_ABORT);

		intptr_t format_idx;
		if (archives.size() == 1) {
			format_idx = 0;
		} else {
			Far::MenuItems format_names;
			for (unsigned i = 0; i < archives.size(); i++) {
				format_names.add(archives[i]->arc_chain.to_string());
			}
			format_idx = Far::menu(c_format_menu_guid, Far::get_msg(MSG_PLUGIN_NAME), format_names);
			if (format_idx == -1)
				FAIL(E_ABORT);
		}

		std::unique_ptr<Plugin<UseVirtualDestructor>> plugin(new Plugin<UseVirtualDestructor>(real_file));

		plugin->archive = archives[format_idx];

		if (!plugin->archive->arc_chain.empty()) {
			const auto &type = plugin->archive->arc_chain.back().type;
			if (type == c_mbr)
				plugin->part_mode = 'm';
			if (type == c_gpt)
				plugin->part_mode = 'g';
		}

		Plugin *hp = plugin.release();

		return hp;
	}

	const wchar_t *guess_fs_ext(const uint32_t f_index)
	{
		union
		{
			unsigned char bs[1024 * 2];
			uint64_t zeroes[1024 / sizeof(uint64_t)];
		} u;
		ComObject<IInStream<UseVirtualDestructor>> sub_stream;
		if (!archive->get_stream(f_index, sub_stream.ref()))
			return nullptr;
		UInt32 nr = 0;
		if (S_OK != sub_stream->Read(u.bs, sizeof(u.bs), &nr) || nr != sizeof(u.bs))
			return nullptr;

		if (u.bs[510] == '\x55' && u.bs[511] == '\xAA') {
			if (memcmp(u.bs + 0x03, "EXFAT   ", 8) == 0)
				return ext_ExFAT;
			if (memcmp(u.bs + 0x03, "NTFS    ", 8) == 0)
				return ext_NTFS;
			if (memcmp(u.bs + 0x52, "FAT32   ", 8) == 0)
				return ext_FAT;
			if (memcmp(u.bs + 0x36, "FAT16   ", 8) == 0)
				return ext_FAT;
			if (memcmp(u.bs + 0x36, "FAT12   ", 8) == 0)
				return ext_FAT;
		}

		if (u.bs[0x438] == 0x53 && u.bs[0x439] == 0xEF) {	 // 0xEF53 -- ExtFS superblock_magic
			bool zero_1k = true;
			for (size_t i = 0; i < _countof(u.zeroes); ++i) {
				if (u.zeroes[i] != 0) {
					zero_1k = false;
					break;
				}
			}
			if (zero_1k) {
				if (u.bs[0x44c] == 0x00)	// s_rev_level
					return ext_EXT2;
				if (u.bs[0x44c] == 0x01) {										  // s_rev_level
					return (u.bs[0x45c] & 0x04) == 0x00 ? ext_EXT2 : ext_EXT3;	  // s_feature_compat
																				  // HAS_JOURNAL bit
				}
				return ext_EXTn;
			}
		}

		return nullptr;
	}
	//
	void correct_part_root_name(std::wstring &name, const uint32_t f_index, const DWORD attrs,
			const uint64_t fsize, const uint64_t psize)
	{
		if (attrs & FILE_ATTRIBUTE_DIRECTORY)
			return;
		if (fsize != psize)
			return;
		const auto offs = archive->get_offset(f_index);
		if (offs == ~0ull)
			return;

		const auto dot1 = name.find(L'.');
		if (dot1 == std::wstring::npos)	   // skip if extension missed
			return;
		const auto dot2 = name.find(L'.', dot1 + 1);
		if (dot2 == dot1 + 4 && name[dot1] == part_mode)	// N.(mpr|gpt).EXT -- already converted
			return;

		const wchar_t *ext = guess_fs_ext(f_index);
		if (!ext) {
			if (str_end_with(name, L".apfs"))
				ext = ext_APFS;
			else if (str_end_with(name, L".hfs") || str_end_with(name, L".hfsx"))
				ext = ext_HFS;
			else if (str_end_with(name, ext_EXT4))
				ext = ext_EXTn;
		}
		if (ext)
			name = std::to_wstring(f_index) + (part_mode == 'm' ? L".mbr" : L".gpt") + ext;
	}

	bool level_up(void)
	{
		if (!archive->is_open())
			FAIL(E_ABORT);

		if (archive->arc_chain.size() < 2) {
			return false;
		}

		archive->close();
		archive = archive->parent;
		set_dir(L"/");

		if (archive->m_chain_file_index != 0xFFFFFFFF) {
			const std::wstring &cfpath = archive->get_path(archive->m_chain_file_index);
			set_dir(extract_file_path(cfpath));
			Far::update_panel(PANEL_ACTIVE, false, true);
			Far::panel_set_file(PANEL_ACTIVE, extract_file_name(cfpath));
		}

		return true;
	}

	bool level_down(const Far::PanelItem &i)
	{
		if (!archive->is_open())
			FAIL(E_ABORT);

		const uint32_t index = static_cast<UInt32>(reinterpret_cast<uintptr_t>(i.user_data));

		if (i.file_name == L"..")
			return false;
		if (index >= archive->file_list.size()) { // not exist
			return false;
		}

		const ArcFileInfo& file_info = archive->file_list[index];
		if (file_info.is_dir) { // Directory
			return false;
		}

		//auto archives = std::make_unique<Archives<UseVirtualDestructor>>();
		std::unique_ptr<Archives<UseVirtualDestructor>> archives(new Archives<UseVirtualDestructor>());
		archives->push_back(this->archive);

		const auto error_log = std::make_shared<ErrorLog>();

		try {
			OpenOptions open_options;
//			open_options.arc_path = this->archive->arc_path;
			open_options.arc_path = i.file_name;
			open_options.detect = false;
			open_options.open_ex = true;
			open_options.nochain = true;
			open_options.password = this->archive->m_password;
			open_options.arc_types = ArcAPI::formats().get_arc_types();

			Archive<UseVirtualDestructor>::open(open_options, *archives, index);
//			if (archives->empty())
//				throw Error(Far::get_msg(MSG_ERROR_NOT_ARCHIVE), arc_list[i], __FILE__, __LINE__);

		} catch (const Error &error) {
//			if (error.code == E_ABORT)
//				throw;
//			error_log->push_back(error);
			return false;
		}

		if (archives->size() > 1) {
			std::shared_ptr<Archive<UseVirtualDestructor>> new_archive = archives->back();
			std::unique_ptr<Plugin<UseVirtualDestructor>> new_plugin(new Plugin<UseVirtualDestructor>(false));
			new_plugin->archive = new_archive;
			new_plugin->parent = this;
			this->child = std::move(new_plugin);
			Far::update_panel(PANEL_ACTIVE, false, true);
	        return true;
		} else {

		/// ERR
			return false;
		}

		return true;
	}

	void info(OpenPluginInfo *opi)
	{
		opi->StructSize = sizeof(OpenPluginInfo);
		opi->Flags = OPIF_ADDDOTS | OPIF_SHORTCUT | OPIF_USEHIGHLIGHTING | OPIF_USESORTGROUPS | OPIF_USEFILTER
				| (recursive_panel ? OPIF_RECURSIVEPANEL : 0)
				| (del_on_close == 'd' ? OPIF_DELETEDIRONCLOSE : 0)
				| (del_on_close == 'f' ? OPIF_DELETEFILEONCLOSE : 0);

		opi->CurDir = current_dir.c_str();

		panel_title = Far::get_msg(MSG_PLUGIN_NAME);
		if (archive->is_open()) {
			arc_chain_str = archive->arc_chain.to_string();
			panel_title += L":" + arc_chain_str + L":" + archive->arc_name();
			if (!current_dir.empty())
				panel_title += L":" + current_dir;
			host_file = archive->arc_path;
			if (archive->m_has_crc)
				opi->Flags |= OPIF_USECRC32;
		}
		else
			arc_chain_str.clear();

		opi->HostFile = host_file.c_str();
		opi->Format = g_plugin_prefix.c_str();
		opi->PanelTitle = panel_title.c_str();

		if (g_options.own_panel_view_mode) {
			opi->StartPanelMode = '0' + g_options.panel_view_mode;
			opi->StartSortMode = g_options.panel_sort_mode;
			opi->StartSortOrder = g_options.panel_reverse_sort;
		}

		info_lines.clear();
		info_lines.reserve(archive->arc_attr.size() + 2);
		InfoPanelLine ipl;

		if (archive->is_open()) {
			ipl.Text = L"Archive format & chain";
			ipl.Data = arc_chain_str.c_str();
			//			ipl.Flags = 0;
			ipl.Separator = 0;
			info_lines.push_back(ipl);
		}

		std::for_each(archive->arc_attr.begin(), archive->arc_attr.end(), [&](const Attr &attr) {
			ipl.Text = attr.name.c_str();
			ipl.Data = attr.value.c_str();
			//			ipl.Flags = 0;
			ipl.Separator = 0;
			info_lines.push_back(ipl);
		});

		opi->InfoLines = info_lines.data();
		opi->InfoLinesNumber = static_cast<int>(info_lines.size());
	}

	void set_dir(const std::wstring &dir)
	{
		if (!archive->is_open())
			FAIL(E_ABORT);

		std::wstring new_dir;
		if (dir.empty() || dir == L"/")
			new_dir.assign(dir);
		else if (dir == L"..")
			new_dir = extract_file_path(current_dir);
		else if (dir[0] == L'/')	// absolute path
			new_dir.assign(dir);
		else
			new_dir.assign(L"/").append(add_trailing_slash(remove_path_root(current_dir))).append(dir);

		if (new_dir == L"/")
			new_dir.clear();

		archive->find_dir(new_dir);
		current_dir = std::move(new_dir);
		return;
	}

	void list(PluginPanelItem **panel_items, int *items_number)
	{
		if (!archive->is_open()) {
			FAIL(E_ABORT);
		}

		UInt32 dir_index = archive->find_dir(current_dir);
		FileIndexRange dir_list = archive->get_dir_list(dir_index);
		size_t size = dir_list.second - dir_list.first;
		//    auto items = std::make_unique<PluginPanelItem[]>(size);
		std::unique_ptr<PluginPanelItem[]> items(new PluginPanelItem[size]);

		memset(items.get(), 0, size * sizeof(PluginPanelItem));
		unsigned idx = 0;

		std::for_each(dir_list.first, dir_list.second, [&](UInt32 file_index) {
			ArcFileInfo &file_info = archive->file_list[file_index];
			DWORD attr = 0, posixattr = 0, farattr = 0;

			attr = archive->get_attr(file_index, &posixattr);

			if (posixattr) {
				switch (posixattr & S_IFMT) {
					case 0: case S_IFREG: farattr = FILE_ATTRIBUTE_ARCHIVE; break;
					case S_IFDIR: farattr = FILE_ATTRIBUTE_DIRECTORY; break;
					#ifndef _WIN32
					case S_IFLNK: farattr = FILE_ATTRIBUTE_REPARSE_POINT; break;
					case S_IFSOCK: farattr = FILE_ATTRIBUTE_DEVICE_SOCK; break;
					#endif
					case S_IFCHR: farattr = FILE_ATTRIBUTE_DEVICE_CHAR; break;
					case S_IFBLK: farattr = FILE_ATTRIBUTE_DEVICE_BLOCK; break;
					case S_IFIFO: farattr = FILE_ATTRIBUTE_DEVICE_FIFO; break;
					default: farattr = FILE_ATTRIBUTE_DEVICE_CHAR | FILE_ATTRIBUTE_BROKEN;
				}

				if ((posixattr & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0)
					farattr |= FILE_ATTRIBUTE_EXECUTABLE;

				if ((posixattr & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0)
					farattr |= FILE_ATTRIBUTE_READONLY;
			}

#if 1
			if (file_info.name.length()) {
				if (file_info.name[0] == L'.') {
					if (file_info.name.length() == 1) {
						--size;
						return;
					}
					farattr |= FILE_ATTRIBUTE_HIDDEN;
				}
			}
			else { // no name ?
				file_info.name = L".[NO NAME]";
				farattr |= FILE_ATTRIBUTE_HIDDEN;
			}
#endif
			if (archive->get_encrypted(file_index))
				farattr |= FILE_ATTRIBUTE_ENCRYPTED;

			items[idx].NumberOfLinks = file_info.num_links;
			if (!file_info.num_links)
				farattr |= FILE_ATTRIBUTE_BROKEN;
			else if (file_info.num_links > 1) {
				farattr |= FILE_ATTRIBUTE_HARDLINKS;
			}

			items[idx].FindData.dwFileAttributes = attr | farattr;
			items[idx].FindData.dwUnixMode = posixattr;

			items[idx].FindData.ftCreationTime = archive->get_ctime(file_index);
			items[idx].FindData.ftLastAccessTime = archive->get_atime(file_index);
			items[idx].FindData.ftLastWriteTime = archive->get_mtime(file_index);
//			items[idx].FindData.ftChangeTime = archive->get_chtime(file_index);

			items[idx].FindData.nFileSize = archive->get_size(file_index);
			items[idx].FindData.nPhysicalSize = archive->get_psize(file_index);

			if (part_mode && current_dir.empty())	 // MBR or GPT root
				correct_part_root_name(file_info.name, file_index, items[idx].FindData.dwFileAttributes,
						items[idx].FindData.nFileSize, items[idx].FindData.nPhysicalSize);

			items[idx].FindData.lpwszFileName = const_cast<wchar_t *>(file_info.name.c_str());
			items[idx].Owner = const_cast<wchar_t *>(file_info.owner.c_str());
			items[idx].Group = const_cast<wchar_t *>(file_info.group.c_str());
			items[idx].Description = const_cast<wchar_t *>(file_info.desc.c_str());

			items[idx].UserData = (DWORD_PTR)file_index;

			items[idx].CRC32 = archive->get_crc(file_index);
			idx++;
		});

		*panel_items = items.release();
		*items_number = size;
	}

	static std::wstring get_separate_dir_path(const std::wstring &dst_dir, const std::wstring &arc_name)
	{
		std::wstring final_dir = add_trailing_slash(dst_dir) + arc_name;
		std::wstring ext = extract_file_ext(final_dir);
		final_dir.erase(final_dir.size() - ext.size(), ext.size());
		final_dir = auto_rename(final_dir);
		return final_dir;
	}

	class PluginPanelItemAccessor
	{
	public:
		virtual ~PluginPanelItemAccessor() = default;
		virtual const PluginPanelItem *get(size_t idx) const = 0;
		virtual size_t size() const = 0;
	};

	void extract(const PluginPanelItemAccessor &panel_items, std::wstring &dst_dir, bool move,
			OPERATION_MODES op_mode)
	{
		if (panel_items.size() == 0)
			return;
		bool single_item = panel_items.size() == 1;
		if (single_item && std::wcscmp(panel_items.get(0)->FindData.lpwszFileName, L"..") == 0)
			return;
		ExtractOptions options;
		static ExtractOptions batch_options;
		options.dst_dir = dst_dir;
		options.move_files = archive->updatable() ? (move ? triTrue : triFalse) : triUndef;
		options.delete_archive = false;
		options.disable_delete_archive = !real_archive_file;

		bool show_dialog = (op_mode & (OPM_FIND | OPM_VIEW | OPM_EDIT | OPM_QUICKVIEW)) == 0;

		if (show_dialog && (op_mode & OPM_SILENT))
			show_dialog = false;

		if (op_mode & (OPM_FIND | OPM_QUICKVIEW))
			options.ignore_errors = true;
		else
			options.ignore_errors = g_options.extract_ignore_errors;

		if (show_dialog || (op_mode & OPM_COMMANDS)) {
			options.extract_access_rights = g_options.extract_access_rights;
			options.extract_owners_groups = g_options.extract_owners_groups;
			options.extract_attributes = g_options.extract_attributes;
			options.duplicate_hardlinks = g_options.extract_duplicate_hardlinks;
			options.restore_special_files = g_options.extract_restore_special_files;
		}
		else {
			options.extract_access_rights = false;
			options.extract_owners_groups = 0;
			options.extract_attributes = false;
			options.duplicate_hardlinks = false;
			options.restore_special_files = false;
		}

		if (show_dialog) {
			options.overwrite = g_options.extract_overwrite;
			options.separate_dir = g_options.extract_separate_dir;
			options.open_dir = (op_mode & OPM_TOPLEVEL) == 0
					? triUndef
					: (g_options.extract_open_dir ? triTrue : triFalse);
		} else {
			options.overwrite = oaOverwrite;
			options.separate_dir = triFalse;
		}

		const auto update_dst_dir = [&] {
			if (options.separate_dir == triTrue
					|| (options.separate_dir == triUndef && !single_item && (op_mode & OPM_TOPLEVEL))) {
				options.dst_dir = get_separate_dir_path(dst_dir, archive->arc_name());
			}
		};

		if (show_dialog) {
			if (!extract_dialog(options))
				FAIL(E_ABORT);
			if (options.dst_dir.empty())
				options.dst_dir = L".";
			if (!is_absolute_path(options.dst_dir))
				options.dst_dir = Far::get_absolute_path(options.dst_dir);
			dst_dir = options.dst_dir;
			update_dst_dir();
			if (!options.password.empty())
				archive->m_password = options.password;
		}
		if (op_mode & OPM_TOPLEVEL) {
			if (op_mode & OPM_SILENT) {
				options = batch_options;
				update_dst_dir();
			} else
				batch_options = options;
		}

		UInt32 src_dir_index = archive->find_dir(current_dir);

		std::vector<UInt32> indices;
		indices.reserve(panel_items.size());
		for (size_t i = 0; i < panel_items.size(); i++) {
			indices.push_back((UInt32)panel_items.get(i)->UserData);
		}

		const auto error_log = std::make_shared<ErrorLog>();
		std::vector<UInt32> extracted_indices;
		archive->extract(src_dir_index, indices, options, error_log,
				options.move_files == triTrue ? &extracted_indices : nullptr);

		if (!error_log->empty() && show_dialog) {
			show_error_log(*error_log);
		}

		if (error_log->empty()) {
			if (options.delete_archive) {
				archive->close();
				archive->delete_archive();
				need_close_panel = true;
			} else if (options.move_files == triTrue) {
				archive->delete_files(extracted_indices);
			}
			Far::progress_notify();
		}

		if (options.open_dir == triTrue) {
			if (single_item)
				Far::panel_go_to_file(PANEL_ACTIVE,
						add_trailing_slash(options.dst_dir) + panel_items.get(0)->FindData.lpwszFileName);
			else
				Far::panel_go_to_dir(PANEL_ACTIVE, options.dst_dir);
		}
	}

	void get_files(const PluginPanelItem *panel_items, intptr_t items_number, int move,
			const wchar_t **dest_path, OPERATION_MODES op_mode)
	{
		class PluginPanelItems : public PluginPanelItemAccessor
		{
		private:
			const PluginPanelItem *panel_items;
			size_t items_number;

		public:
			PluginPanelItems(const PluginPanelItem *panel_items, size_t items_number)
				: panel_items(panel_items), items_number(items_number)
			{}
			const PluginPanelItem *get(size_t idx) const override { return panel_items + idx; }
			size_t size() const override { return items_number; }
		};
		PluginPanelItems pp_items(panel_items, items_number);
		extract_dir = *dest_path;
		extract(pp_items, extract_dir, move != 0, op_mode);
		if (need_close_panel) {
			Far::close_panel(this, archive->arc_dir());
		} else if (extract_dir != *dest_path) {
			*dest_path = extract_dir.c_str();
		}
	}

	void extract()
	{
		class PluginPanelItems : public PluginPanelItemAccessor
		{
		private:
			HANDLE h_plugin;
			PanelInfo panel_info;

			mutable Buffer<unsigned char> buf;

		public:
			PluginPanelItems(HANDLE h_plugin) : h_plugin(h_plugin)
			{
				CHECK(Far::get_panel_info(h_plugin, panel_info));
			}
			const PluginPanelItem *get(size_t idx) const override
			{
				Far::get_panel_item(h_plugin, FCTL_GETSELECTEDPANELITEM, idx, buf);
				return reinterpret_cast<const PluginPanelItem *>(buf.data());
			}
			size_t size() const override { return panel_info.SelectedItemsNumber; }
		};
		PluginPanelItems pp_items(this->get_root());
		auto dst = extract_file_path(archive->get_root()->arc_path);
		extract(pp_items, dst, false, OPM_NONE);
	}

	static void extract(const std::vector<std::wstring> &arc_list, ExtractOptions options)
	{
		std::wstring dst_dir = options.dst_dir;
		std::wstring dst_file_name;
		const auto error_log = std::make_shared<ErrorLog>();
		for (unsigned i = 0; i < arc_list.size(); i++) {
			std::unique_ptr<Archives<UseVirtualDestructor>> archives;
			try {
				OpenOptions open_options;
				open_options.arc_path = arc_list[i];
				open_options.detect = false;
				open_options.open_ex = true;
				open_options.nochain = false;
				open_options.password = options.password;
				open_options.arc_types = ArcAPI::formats().get_arc_types();
				archives = Archive<UseVirtualDestructor>::open(open_options);
				if (archives->empty())
					throw Error(Far::get_msg(MSG_ERROR_NOT_ARCHIVE), arc_list[i], __FILE__, __LINE__);
			} catch (const Error &error) {
				if (error.code == E_ABORT)
					throw;
				error_log->push_back(error);
				continue;
			}

			std::shared_ptr<Archive<UseVirtualDestructor>> archive = (*archives)[0];
			if (archive->m_password.empty())
				archive->m_password = options.password;
			archive->make_index();

			FileIndexRange dir_list = archive->get_dir_list(c_root_index);

			uintptr_t num_items = dir_list.second - dir_list.first;
			if (arc_list.size() == 1 && num_items == 1) {
				dst_file_name = archive->file_list[*dir_list.first].name;
			}

			std::vector<UInt32> indices;
			indices.reserve(num_items);
			std::for_each(dir_list.first, dir_list.second, [&](UInt32 file_index) {
				indices.push_back(file_index);
			});

			if (options.separate_dir == triTrue || (options.separate_dir == triUndef && indices.size() > 1))
				options.dst_dir = get_separate_dir_path(dst_dir, archive->arc_name());
			else
				options.dst_dir = dst_dir;

			size_t error_count = error_log->size();
			archive->extract(c_root_index, indices, options, error_log);

			if (options.delete_archive && error_count == error_log->size()) {
				archive->close();
				archive->delete_archive();
			}
		}

		if (!error_log->empty()) {
			show_error_log(*error_log);
		} else {
			Far::update_panel(PANEL_ACTIVE, false);
			Far::progress_notify();
		}

		if (options.open_dir == triTrue) {
			if (arc_list.size() > 1)
				Far::panel_go_to_dir(PANEL_ACTIVE, dst_dir);
			else if (dst_file_name.empty())
				Far::panel_go_to_dir(PANEL_ACTIVE, options.dst_dir);
			else
				Far::panel_go_to_file(PANEL_ACTIVE, add_trailing_slash(options.dst_dir) + dst_file_name);
		}
	}

	static void bulk_extract(const std::vector<std::wstring> &arc_list)
	{
		ExtractOptions options;
		PanelInfo panel_info;
		if (Far::get_panel_info(PANEL_PASSIVE, panel_info) && Far::is_real_file_panel(panel_info))
			options.dst_dir = Far::get_panel_dir(PANEL_PASSIVE);
		options.move_files = triUndef;
		options.delete_archive = false;
		options.ignore_errors = g_options.extract_ignore_errors;
		options.overwrite = g_options.extract_overwrite;
		options.separate_dir = g_options.extract_separate_dir;
		options.open_dir = g_options.extract_open_dir ? triTrue : triFalse;

		if (!extract_dialog(options))
			FAIL(E_ABORT);
		if (options.dst_dir.empty())
			options.dst_dir = L".";
		if (!is_absolute_path(options.dst_dir))
			options.dst_dir = Far::get_absolute_path(options.dst_dir);

		extract(arc_list, options);
	}

	static void cmdline_extract(const ExtractCommand &cmd)
	{
		std::vector<std::wstring> arc_list;
		arc_list.reserve(cmd.arc_list.size());
		std::for_each(cmd.arc_list.begin(), cmd.arc_list.end(), [&](const std::wstring &arc_name) {
			arc_list.emplace_back(Far::get_absolute_path(arc_name));
		});
		ExtractOptions options = cmd.options;
		options.dst_dir = Far::get_absolute_path(cmd.options.dst_dir);
		extract(arc_list, options);
	}

	static void delete_items(const std::wstring &arch_name, const ExtractOptions &options,
			const std::vector<std::wstring> &items)
	{
		std::unique_ptr<Archives<UseVirtualDestructor>> archives;
		OpenOptions open_options;
		open_options.arc_path = arch_name;
		open_options.detect = false;
		open_options.open_ex = false;
		open_options.nochain = true;
		open_options.password = options.password;
		open_options.arc_types = ArcAPI::formats().get_arc_types();
		archives = Archive<UseVirtualDestructor>::open(open_options);
		if (archives->empty())
			throw Error(Far::get_msg(MSG_ERROR_NOT_ARCHIVE), arch_name, __FILE__, __LINE__);

		std::shared_ptr<Archive<UseVirtualDestructor>> archive = (*archives)[0];
		if (archive->m_password.empty())
			archive->m_password = options.password;
		archive->make_index();
		auto nf = static_cast<UInt32>(archive->file_list.size());
		std::vector<UInt32> matched_indices;
		for (UInt32 j = 0; j < nf; ++j) {
			auto file_path = archive->get_path(j);
			for (const auto &n : items) {
				if (wcscmp(file_path.c_str(), n.c_str()) == 0 || Far::match_masks(file_path, n)) {
					matched_indices.push_back(j);
					break;
				}
			}
		}
		if (!matched_indices.empty()) {
			archive->delete_files(matched_indices);
			Far::progress_notify();
		}
		return;
	}

	static void extract_items(const std::wstring &arch_name, const ExtractOptions &options,
			const std::vector<std::wstring> &items)
	{
		std::unique_ptr<Archives<UseVirtualDestructor>> archives;
		OpenOptions open_options;
		open_options.arc_path = arch_name;
		open_options.detect = false;
		open_options.open_ex = true;
		open_options.nochain = false;
		open_options.password = options.password;
		open_options.arc_types = ArcAPI::formats().get_arc_types();
		archives = Archive<UseVirtualDestructor>::open(open_options);
		if (archives->empty())
			throw Error(Far::get_msg(MSG_ERROR_NOT_ARCHIVE), arch_name, __FILE__, __LINE__);

		std::shared_ptr<Archive<UseVirtualDestructor>> archive = (*archives)[0];
		if (archive->m_password.empty())
			archive->m_password = options.password;
		archive->make_index();
		auto nf = static_cast<UInt32>(archive->file_list.size());
		std::vector<UInt32> matched_indices;
		for (UInt32 j = 0; j < nf; ++j) {
			auto file_path = archive->get_path(j);
			for (const auto &n : items) {
				if (wcscmp(file_path.c_str(), n.c_str()) == 0 || Far::match_masks(file_path, n)) {
					matched_indices.push_back(j);
					break;
				}
			}
		}
		if (!matched_indices.empty()) {
			std::sort(matched_indices.begin(), matched_indices.end(),
					[&](const UInt32 &a, const UInt32 &b) {	   // group by parent
						return static_cast<Int32>(archive->file_list[a].parent)
								< static_cast<Int32>(archive->file_list[b].parent);
					});
			const auto error_log = std::make_shared<ErrorLog>();
			size_t im = 0, nm = matched_indices.size();
			std::vector<UInt32> indices;
			indices.reserve(nm);
			while (im < nm) {
				indices.clear();
				auto parent = archive->file_list[matched_indices[im]].parent;
				while (im < nm && archive->file_list[matched_indices[im]].parent == parent)
					indices.push_back(matched_indices[im++]);
				archive->extract(parent, indices, options, error_log);
			}
			if (!error_log->empty()) {
				show_error_log(*error_log);
			} else {
				Far::update_panel(PANEL_ACTIVE, false);
				Far::progress_notify();
			}
		}
		return;
	}

	static void cmdline_extract(const ExtractItemsCommand &cmd)
	{
		auto full_arch_name = Far::get_absolute_path(cmd.archname);
		auto options = cmd.options;
		if (cmd.options.dst_dir.empty())
			options.dst_dir = Far::get_panel_dir(PANEL_ACTIVE);
		if (!is_absolute_path(options.dst_dir))
			options.dst_dir = Far::get_absolute_path(options.dst_dir);
		extract_items(full_arch_name, options, cmd.items);
	}

	static void cmdline_delete(const ExtractItemsCommand &cmd)
	{
		auto full_arch_name = Far::get_absolute_path(cmd.archname);
		auto options = cmd.options;
		delete_items(full_arch_name, options, cmd.items);
	}

	void test_files(struct PluginPanelItem *panel_items, intptr_t items_number, OPERATION_MODES op_mode)
	{
		Error err;
		err.objects = {archive->arc_path};
		archive->read_open_results();
		err.SetResults(archive->get_open_errors(), archive->get_open_warnings());

		UInt32 src_dir_index = archive->find_dir(current_dir);
		std::vector<UInt32> indices;
		indices.reserve(items_number);
		for (int i = 0; i < items_number; i++) {
			indices.push_back((UInt32)panel_items[i].UserData);
		}

		try {
			archive->test(src_dir_index, indices);
		} catch (const Error &error) {
			err.Append(error);
		}
		if (err)
			throw err;

		if (op_mode == OPM_NONE) {
			for (int i = 0; i < items_number; i++) {
				panel_items[i].Flags &= ~PPIF_SELECTED;
			}
		}
		Far::info_dlg(c_test_ok_dialog_guid, Far::get_msg(MSG_PLUGIN_NAME), Far::get_msg(MSG_TEST_OK));
	}

	static void bulk_test(const std::vector<std::wstring> &arc_list)
	{
		const auto error_log = std::make_shared<ErrorLog>();
		for (unsigned i = 0; i < arc_list.size(); i++) {
			std::unique_ptr<Archives<UseVirtualDestructor>> archives;
			Error err;
			err.objects = {arc_list[i]};
			try {
				OpenOptions open_options;
				open_options.arc_path = arc_list[i];
				open_options.detect = false;
				open_options.open_ex = false; // !
				open_options.nochain = true;
				open_options.arc_types = ArcAPI::formats().get_arc_types();
				archives = Archive<UseVirtualDestructor>::open(open_options);
				if (archives->empty())
					throw Error(Far::get_msg(MSG_ERROR_NOT_ARCHIVE), __FILE__, __LINE__);
			} catch (const Error &error) {
				if (error.code == E_ABORT)
					throw;
				err.Append(error);
				error_log->push_back(err);
				continue;
			}

			std::shared_ptr<Archive<UseVirtualDestructor>> archive = (*archives)[0];
			archive->read_open_results();
			err.SetResults(archive->get_open_errors(), archive->get_open_warnings());
			archive->make_index();

			FileIndexRange dir_list = archive->get_dir_list(c_root_index);
			std::vector<UInt32> indices;
			indices.reserve(dir_list.second - dir_list.first);
			std::for_each(dir_list.first, dir_list.second, [&](UInt32 file_index) {
				indices.push_back(file_index);
			});

			try {
				archive->test(c_root_index, indices);
			} catch (const Error &error) {
				if (error.code == E_ABORT)
					throw;
				err.Append(error);
			}
			if (err)
				error_log->push_back(err);
		}

		if (!error_log->empty()) {
			show_error_log(*error_log);
		} else {
			Far::update_panel(PANEL_ACTIVE, false);
			Far::info_dlg(c_test_ok_dialog_guid, Far::get_msg(MSG_PLUGIN_NAME), Far::get_msg(MSG_TEST_OK));
		}
	}

	static void cmdline_test(const TestCommand &cmd)
	{
		std::vector<std::wstring> arc_list;
		arc_list.reserve(cmd.arc_list.size());
		std::for_each(cmd.arc_list.begin(), cmd.arc_list.end(), [&](const std::wstring &arc_name) {
			arc_list.emplace_back(Far::get_absolute_path(arc_name));
		});
		Plugin::bulk_test(arc_list);
	}

	void put_files(const PluginPanelItem *panel_items, intptr_t items_number, int move,
			const wchar_t *src_path, OPERATION_MODES op_mode)
	{
		if (items_number == 1 && std::wcscmp(panel_items[0].FindData.lpwszFileName, L"..") == 0)
			return;
		UpdateOptions options;

//		fprintf(stderr, " +++ put_files( )\n");
//		fprintf(stderr, " +++ ====================================================\n");
//		for (int i = 0; i < items_number; ++i) {
//			fprintf(stderr, " item %i %ls \n", i, panel_items[i].FindData.lpwszFileName);
//		}

		bool new_arc = !archive->is_open();
		bool multifile = items_number > 1
				|| (items_number == 1
						&& (panel_items[0].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
		if (!new_arc && !archive->updatable()) {
			FAIL_MSG(Far::get_msg(MSG_ERROR_NOT_UPDATABLE));
		}

		if (new_arc) {
			if (!is_absolute_path(panel_items[0].FindData.lpwszFileName)) {
				if (items_number == 1 || is_root_path(src_path)) {
					options.arc_path = panel_items[0].FindData.lpwszFileName;
				} else {
					options.arc_path = extract_file_name(src_path);
				}
			} else {
				options.arc_path = add_trailing_slash(src_path)
						+ (is_root_path(src_path) ? std::wstring(L"_") : extract_file_name(src_path));
			}

			ArcTypes arc_types = ArcAPI::formats().find_by_name(g_options.update_arc_format_name);
			options.arc_type = arc_types.empty() ? c_7z : arc_types.front();
			arc_types = ArcAPI::formats().find_by_name(g_options.update_arc_repack_format_name);
			options.repack_arc_type = arc_types.empty() ? c_7z : arc_types.front();
			options.repack = g_options.update_repack;

			options.level = g_options.update_level;
			options.levels = g_options.update_levels;
			options.method = g_options.update_method;
			options.solid = g_options.update_solid;
			options.append_ext = g_options.update_append_ext;

			options.advanced = g_options.update_advanced;

			options.encrypt = g_options.update_encrypt;
			options.encrypt_header = g_options.update_encrypt_header;
			if (options.encrypt)
				options.password = g_options.update_password;

			options.create_sfx = g_options.update_create_sfx;
			options.sfx_options = g_options.update_sfx_options;

			options.enable_volumes = g_options.update_enable_volumes;
			options.volume_size = g_options.update_volume_size;

			options.move_files = g_options.update_move_files;
		} else {
			options.arc_type = archive->arc_chain.back().type;	  // required to set update properties
			if (ArcAPI::is_single_file_format(options.arc_type)) {
				if (items_number != 1
						|| panel_items[0].FindData.lpwszFileName != archive->file_list[0].name) {
					FAIL_MSG(Far::get_msg(MSG_ERROR_UPDATE_UNSUPPORTED_FOR_SINGLEFILEARCHIVE));
				}
			}
			archive->load_update_props(options.arc_type);
			options.method = archive->m_method;
			options.solid = archive->m_solid;
			options.encrypt = archive->m_encrypted;
			options.encrypt_header = triUndef;
			options.password = archive->m_password;

			// options.level = archive->level;
			options.level = g_options.update_level;
			options.levels = g_options.update_levels;

			options.overwrite = g_options.update_overwrite;
			if (op_mode & OPM_EDIT)
				options.overwrite = oaOverwrite;
			options.move_files = move != 0;
			options.create_sfx = false;
			options.enable_volumes = false;
		}

		options.multithreading = g_options.update_multithreading;
		options.process_priority = g_options.update_process_priority;
		options.threads_num = g_options.update_threads_num;
	    options.skip_symlinks = g_options.update_skip_symlinks;
	    options.symlink_fix_path_mode = g_options.update_symlink_fix_path_mode;
	    options.dereference_symlinks = g_options.update_dereference_symlinks;
	    options.skip_hardlinks = g_options.update_skip_hardlinks;
	    options.duplicate_hardlinks = g_options.update_duplicate_hardlinks;
		options.export_options = g_options.update_export_options;
		options.use_export_settings = g_options.update_use_export_settings;
		options.show_password = g_options.update_show_password;
		options.ignore_errors = g_options.update_ignore_errors;

		bool res = update_dialog(new_arc, multifile, options, g_profiles);

		if (!res)
			FAIL(E_ABORT);
		if (ArcAPI::formats().count(options.arc_type) == 0)
			FAIL_MSG(Far::get_msg(MSG_ERROR_NO_FORMAT));

		if (new_arc) {
			if (!options.encrypt)
				options.password.clear();
			if (File::exists(options.arc_path)) {
				if (Far::message(c_overwrite_archive_dialog_guid,
							Far::get_msg(MSG_PLUGIN_NAME) + L"\n"
									+ Far::get_msg(MSG_UPDATE_DLG_CONFIRM_OVERWRITE),
							0, FMSG_MB_YESNO)
						!= 0)
					FAIL(E_ABORT);
			}
		} else {
			archive->m_level = options.level;
			archive->m_method = options.method;
			archive->m_solid = options.solid;
			archive->m_encrypted = options.encrypt;
		}

		bool all_path_abs = true;
		std::wstring common_src_path(src_path);
		std::vector<std::wstring> file_names;
		file_names.reserve(items_number);

		for (int i = 0; i < items_number; i++) {
			file_names.push_back(panel_items[i].FindData.lpwszFileName);
			all_path_abs = all_path_abs && is_absolute_path(file_names.back());
		}

		if (all_path_abs) {
			std::wstring common_start = add_trailing_slash(upcase(src_path));
			for (int i = 0; i < items_number; i++) {
				std::wstring upcase_name = upcase(file_names[i]);
				while (common_start.size() > 1 && !substr_match(upcase_name, 0, common_start.c_str())) {
					size_t next_slash = common_start.size() - 1;
					while (next_slash > 0 && !is_slash(common_start[next_slash - 1])) {
						--next_slash;
					}
					common_start.resize(next_slash);
				}
				if (common_start.size() <= 1)
					break;
			}
			if (common_start.size() <= 1) {
				common_start.clear();
			} else {
				size_t common_len = common_start.size() - 1;
				common_src_path.assign(src_path, common_len);
				if (common_src_path.back() == L':')
					common_src_path += L"/";
				for (int i = 0; i < items_number; i++) {
					file_names[i] = file_names[i].substr(common_len + 1);
				}
			}
		}

		const auto error_log = std::make_shared<ErrorLog>();

		if (new_arc) {
//			fprintf(stderr, " +++ put_files( ) archive create( )\n");
			archive->create(common_src_path, file_names, options, error_log);
		} else {
//			fprintf(stderr, " +++ put_files( ) archive update( )\n");
			archive->update(common_src_path, file_names, remove_path_root(current_dir), options, error_log);
		}

		if (!error_log->empty()) {
			show_error_log(*error_log);
		} else {
			Far::progress_notify();
		}

		if (new_arc) {
			if (upcase(Far::get_panel_dir(PANEL_ACTIVE)) == upcase(extract_file_path(options.arc_path))) {
				Far::panel_go_to_file(PANEL_ACTIVE, options.arc_path);
			}
		}
	}

	static void create_archive()
	{
		PanelInfo panel_info;

		if (!Far::get_panel_info(PANEL_ACTIVE, panel_info))
			FAIL(E_ABORT);
		if (!Far::is_real_file_panel(panel_info))
			FAIL(E_ABORT);

		std::vector<std::wstring> file_list;
		file_list.reserve(panel_info.SelectedItemsNumber);
		std::wstring src_path = Far::get_panel_dir(PANEL_ACTIVE);
		bool multifile = false;

		for (size_t i = 0; i < (size_t)panel_info.SelectedItemsNumber; i++) {
			Far::PanelItem panel_item = Far::get_selected_panel_item(PANEL_ACTIVE, i);
			file_list.push_back(panel_item.file_name);
			if (file_list.size() > 1 || (panel_item.file_attributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
				multifile = true;
		}

		if (file_list.empty())
			FAIL(E_ABORT);
		if (file_list.size() == 1 && file_list[0] == L"..")
			FAIL(E_ABORT);

		UpdateOptions options;

		if (file_list.size() == 1 || is_root_path(src_path))
			options.arc_path = file_list[0];
		else
			options.arc_path = extract_file_name(src_path);

		ArcTypes arc_types = ArcAPI::formats().find_by_name(g_options.update_arc_format_name);
		if (arc_types.empty())
			options.arc_type = c_7z;
		else
			options.arc_type = arc_types.front();

		arc_types = ArcAPI::formats().find_by_name(g_options.update_arc_repack_format_name);
		options.repack_arc_type = arc_types.empty() ? c_7z : arc_types.front();
		options.repack = g_options.update_repack;

		options.sfx_options = g_options.update_sfx_options;
		options.level = g_options.update_level;
		options.levels = g_options.update_levels;
		options.method = g_options.update_method;
		options.solid = g_options.update_solid;
		options.show_password = g_options.update_show_password;
		options.encrypt = false;
		options.encrypt_header = g_options.update_encrypt_header;
		options.create_sfx = false;
		options.enable_volumes = false;
		options.volume_size = g_options.update_volume_size;
		options.move_files = false;

//		CHECK(get_app_option(FSSF_SYSTEM, c_copy_opened_files_option, options.open_shared));

		options.ignore_errors = g_options.update_ignore_errors;
		options.append_ext = g_options.update_append_ext;

		options.multithreading = g_options.update_multithreading;
		options.process_priority = g_options.update_process_priority;
		options.threads_num = g_options.update_threads_num;
	    options.skip_symlinks = g_options.update_skip_symlinks;
	    options.symlink_fix_path_mode = g_options.update_symlink_fix_path_mode;
	    options.dereference_symlinks = g_options.update_dereference_symlinks;
		options.skip_hardlinks = g_options.update_skip_hardlinks;
		options.duplicate_hardlinks = g_options.update_duplicate_hardlinks;
		options.export_options = g_options.update_export_options;
		options.use_export_settings = g_options.update_use_export_settings;

		bool res = update_dialog(true, multifile, options, g_profiles);
		if (!res)
			FAIL(E_ABORT);
		if (ArcAPI::formats().count(options.arc_type) == 0)
			FAIL_MSG(Far::get_msg(MSG_ERROR_NO_FORMAT));

		if (File::exists(options.arc_path)) {
			if (Far::message(c_overwrite_archive_dialog_guid,
						Far::get_msg(MSG_PLUGIN_NAME) + L"\n"
								+ Far::get_msg(MSG_UPDATE_DLG_CONFIRM_OVERWRITE),
						0, FMSG_MB_YESNO)
					!= 0)
				FAIL(E_ABORT);
		}

		const auto error_log = std::make_shared<ErrorLog>();
		std::make_shared<Archive<UseVirtualDestructor>>()->create(src_path, file_list, options, error_log);

		if (!error_log->empty()) {
			show_error_log(*error_log);
		} else {
			Far::progress_notify();
		}

		if (upcase(Far::get_panel_dir(PANEL_ACTIVE)) == upcase(extract_file_path(options.arc_path)))
			Far::panel_go_to_file(PANEL_ACTIVE, options.arc_path);
		if (upcase(Far::get_panel_dir(PANEL_PASSIVE)) == upcase(extract_file_path(options.arc_path)))
			Far::panel_go_to_file(PANEL_PASSIVE, options.arc_path);
	}

	static void cmdline_update(const UpdateCommand &cmd)
	{
		UpdateOptions options = cmd.options;
		options.arc_path = Far::get_absolute_path(options.arc_path);

		std::vector<std::wstring> files;
		files.reserve(cmd.files.size());
		std::for_each(cmd.files.begin(), cmd.files.end(), [&](const std::wstring &file) {
			files.emplace_back(Far::get_absolute_path(file));
		});

		// load listfiles
		std::for_each(cmd.listfiles.begin(), cmd.listfiles.end(), [&](const std::wstring &listfile) {
			std::wstring str = load_file(Far::get_absolute_path(listfile));
			std::list<std::wstring> fl = parse_listfile(str);
			files.reserve(files.size() + fl.size());
			std::for_each(fl.begin(), fl.end(), [&](const std::wstring &file) {
				files.emplace_back(Far::get_absolute_path(file));
			});
		});

		if (files.empty())
			FAIL(E_ABORT);

		// common source directory
		std::wstring src_path = extract_file_path(Far::get_absolute_path(files.front()));
		std::wstring src_path_upcase = upcase(src_path);
		std::wstring full_path;
		std::for_each(files.begin(), files.end(), [&](const std::wstring &file) {
			while (!substr_match(upcase(file), 0, src_path_upcase.c_str())) {
				if (is_root_path(src_path))
					src_path.clear();
				else
					src_path = extract_file_path(src_path);
				src_path_upcase = upcase(src_path);
			}
		});

		// relative paths
		if (!src_path.empty()) {
			size_t size = src_path.size() + (src_path.back() == L'/' ? 0 : 1);
			std::for_each(files.begin(), files.end(), [&](std::wstring &file) {
				file.erase(0, size);
			});
		}

//		CHECK(get_app_option(FSSF_SYSTEM, c_copy_opened_files_option, options.open_shared));

		const auto error_log = std::make_shared<ErrorLog>();
		if (cmd.new_arc) {
			std::make_shared<Archive<UseVirtualDestructor>>()->create(src_path, files, options, error_log);

			if (upcase(Far::get_panel_dir(PANEL_ACTIVE)) == upcase(extract_file_path(options.arc_path)))
				Far::panel_go_to_file(PANEL_ACTIVE, options.arc_path);
			if (upcase(Far::get_panel_dir(PANEL_PASSIVE)) == upcase(extract_file_path(options.arc_path)))
				Far::panel_go_to_file(PANEL_PASSIVE, options.arc_path);
		} else {
			OpenOptions open_options;
			open_options.arc_path = options.arc_path;
			open_options.detect = false;
			open_options.open_ex = false;
			open_options.nochain = true;
			open_options.password = options.password;
			open_options.arc_types = ArcAPI::formats().get_arc_types();
			const auto archives = Archive<UseVirtualDestructor>::open(open_options);
			if (archives->empty())
				throw Error(Far::get_msg(MSG_ERROR_NOT_ARCHIVE), options.arc_path, __FILE__, __LINE__);

			const auto archive = (*archives)[0];
			if (!archive->updatable())
				throw Error(Far::get_msg(MSG_ERROR_NOT_UPDATABLE), options.arc_path, __FILE__, __LINE__);

			archive->make_index();

			options.arc_type = archive->arc_chain.back().type;
			archive->load_update_props(options.arc_type);
			if (!cmd.level_defined)
				options.level = archive->m_level;
			if (!cmd.method_defined)
				options.method = archive->m_method;
			if (!cmd.solid_defined)
				options.solid = archive->m_solid;
			if (!cmd.encrypt_defined) {
				options.encrypt = archive->m_encrypted;
				options.password = archive->m_password;
			}

			archive->update(src_path, files, std::wstring(), options, error_log);
		}
	}

	void delete_files(const PluginPanelItem *panel_items, intptr_t items_number, OPERATION_MODES op_mode)
	{
		if (items_number == 1 && std::wcscmp(panel_items[0].FindData.lpwszFileName, L"..") == 0)
			return;

		if (!archive->updatable()) {
			FAIL_MSG(Far::get_msg(MSG_ERROR_NOT_UPDATABLE));
		}
		if (ArcAPI::is_single_file_format(archive->arc_chain.back().type)) {
			FAIL_MSG(Far::get_msg(MSG_ERROR_UPDATE_UNSUPPORTED_FOR_SINGLEFILEARCHIVE));
		}

		bool show_dialog = (op_mode & (OPM_SILENT | OPM_FIND | OPM_VIEW | OPM_EDIT | OPM_QUICKVIEW)) == 0;
		if (show_dialog) {
			if (Far::message(c_delete_files_dialog_guid,
						Far::get_msg(MSG_PLUGIN_NAME) + L"\n" + Far::get_msg(MSG_DELETE_DLG_CONFIRM), 0,
						FMSG_MB_OKCANCEL)
					!= 0)
				FAIL(E_ABORT);
		}

		std::vector<UInt32> indices;
		indices.reserve(items_number);
		for (int i = 0; i < items_number; i++) {
			indices.push_back((UInt32)panel_items[i].UserData);
		}
		archive->delete_files(indices);

		Far::progress_notify();
	}

	void describe_files()
	{
		PanelInfo panel_info;
		if (!Far::get_panel_info(PANEL_ACTIVE, panel_info))
			FAIL(E_ABORT);
		// ----------------------
	}

	void create_dir(const wchar_t **name, OPERATION_MODES op_mode)
	{
		if (!archive->updatable()) {
			FAIL_MSG(Far::get_msg(MSG_ERROR_NOT_UPDATABLE));
		}
		bool show_dialog = (op_mode & (OPM_SILENT | OPM_FIND | OPM_VIEW | OPM_EDIT | OPM_QUICKVIEW)) == 0;
		created_dir = *name;
		if (show_dialog) {
			if (!Far::input_dlg(c_create_dir_dialog_guid, Far::get_msg(MSG_PLUGIN_NAME),
						Far::get_msg(MSG_CREATE_DIR_DLG_NAME), created_dir))
				FAIL(E_ABORT);
			*name = created_dir.c_str();
		}
		archive->create_dir(created_dir, remove_path_root(current_dir));
	}

	void show_attr()
	{
		AttrList attr_list;
		Far::PanelItem panel_item = Far::get_current_panel_item(PANEL_ACTIVE);
		if (panel_item.file_name == L"..") {
			if (is_root_path(current_dir)) {
				attr_list = archive->arc_attr;
			} else {
				attr_list = archive->get_attr_list(archive->find_dir(current_dir));
			}
		} else {
			attr_list = archive->get_attr_list(
					static_cast<UInt32>(reinterpret_cast<size_t>(panel_item.user_data)));
		}
		if (!attr_list.empty())
			attr_dialog(attr_list);
	}


	void show_hardlinks()
	{
		std::vector<std::wstring> hl_names;
		std::vector<FarMenuItem> menu_items;
		static const int BreakKeys[] = {VK_ESCAPE, 0};
		int BreakCode = -1;
		FarMenuItem mi;
	    Far::PanelItem panel_item = Far::get_current_panel_item(PANEL_ACTIVE);
	    std::wstring empty_str = L"";

		if (panel_item.file_name == L"..") {

		//		hl_names.reserve(group.size());
		//		menu_items.reserve(group.size());

	        for (size_t group_idx = 0; group_idx < archive->hard_link_groups.size(); ++group_idx) {
	            const HardLinkGroup &group = archive->hard_link_groups[group_idx];

				for (size_t i = 0; i < group.size(); ++i) {
				    UInt32 file_index = group[i];
				    if (file_index < archive->file_list.size()) {
				        std::wstring full_path = archive->get_path(file_index);
			            hl_names.emplace_back(std::to_wstring(i + 1) + L": " + std::to_wstring(file_index) + L": " + full_path);

						mi.Text = hl_names.back().c_str();
						mi.Selected = 0;
						mi.Checked = 0;
						mi.Separator = 0;
						menu_items.push_back(mi);
				    }
				}
				if (group_idx < archive->hard_link_groups.size() - 1) {
					mi.Text = empty_str.c_str();
					mi.Selected = 0;
					mi.Checked = 0;
					mi.Separator = 1;
					menu_items.push_back(mi);
				}
			}

			std::wstring bottom_title = L"Total: " + std::to_wstring(999);

			uint32_t ret = (uint32_t)Far::g_far.Menu(Far::g_far.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE, L"HardLinks", bottom_title.c_str(), nullptr,
					BreakKeys, &BreakCode, menu_items.data(), static_cast<int>(menu_items.size()));

			(void)ret;
			return;
		}

	    uint32_t index = static_cast<UInt32>(reinterpret_cast<size_t>(panel_item.user_data));
	    const ArcFileInfo &current_file_info = archive->file_list[index];

	    uint32_t group_index = current_file_info.hl_group;
	    if (group_index == (uint32_t)-1)
			return;

	    const HardLinkGroup &group = archive->hard_link_groups[group_index];
		if (!group.size())
			return;

		hl_names.reserve(group.size());
		menu_items.reserve(group.size());

		uint32_t isel = 0;
		for (size_t i = 0; i < group.size(); ++i) {
		    UInt32 file_index = group[i];
		    if (file_index < archive->file_list.size()) {
		        std::wstring full_path = archive->get_path(file_index);
	            hl_names.emplace_back(std::to_wstring(i) + L": " + std::to_wstring(file_index) + L": " + full_path);

				mi.Text = hl_names.back().c_str();
				if (index == file_index) {
					mi.Selected = (index == file_index);
					isel = i;
				}
				else
					mi.Selected = 0;

				mi.Checked = 0;
				mi.Separator = 0;
				menu_items.push_back(mi);
		    }
		}

		std::wstring bottom_title = L"Total: " + std::to_wstring(group.size());

		while( 1 ) {

			uint32_t ret = (uint32_t)Far::g_far.Menu(Far::g_far.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE, L"HardLinks", bottom_title.c_str(), nullptr,
					BreakKeys, &BreakCode, menu_items.data(), static_cast<int>(menu_items.size()));

			if (BreakCode >= 0) {
				//ret = (uint32_t)-1;
				break;
			}

			if (ret != (uint32_t)-1) {
				menu_items[isel].Selected = 0;
				menu_items[ret].Selected = 1;
				isel = ret;

			    UInt32 file_index = group[ret];

		        const std::wstring &cfpath = archive->get_path(file_index);

				set_dir(add_leading_slash(extract_file_path(cfpath)));

				Far::update_panel(PANEL_ACTIVE, false, true);
				Far::panel_set_file(PANEL_ACTIVE, extract_file_name(cfpath));
			}
		}
	}

	void close()
	{
		PanelInfo panel_info;
		if (Far::get_panel_info(this, panel_info)) {
			uintptr_t panel_view_mode = panel_info.ViewMode;
			//      OPENPANELINFO_SORTMODES panel_sort_mode = panel_info.SortMode;
			OPENPLUGININFO_SORTMODES panel_sort_mode = (OPENPLUGININFO_SORTMODES)panel_info.SortMode;

			bool panel_reverse_sort = (panel_info.Flags & PFLAGS_REVERSESORTORDER) != 0;
			if (g_options.panel_view_mode != panel_view_mode || g_options.panel_sort_mode != panel_sort_mode
					|| g_options.panel_reverse_sort != panel_reverse_sort) {
				g_options.panel_view_mode = panel_view_mode;
				g_options.panel_sort_mode = panel_sort_mode;
				g_options.panel_reverse_sort = panel_reverse_sort;
				g_options.save();
			}
		}
	}

	static void convert_to_sfx(const std::vector<std::wstring> &file_list)
	{
		SfxOptions sfx_options = g_options.update_sfx_options;
		if (!sfx_options_dialog(sfx_options, g_profiles))
			FAIL(E_ABORT);
		for (unsigned i = 0; i < file_list.size(); i++) {
			attach_sfx_module(file_list[i], sfx_options);
		}
	}
};

std::wstring g_plugin_path;

SHAREDSYMBOL void WINAPI PluginModuleOpen(const char *path)
{
	MB2Wide(path, g_plugin_path);
	// G.plugin_path = path;
	//	fprintf(stderr, "NetRocks::PluginModuleOpen\n");
}

SHAREDSYMBOL void WINAPI SetStartupInfoW(const PluginStartupInfo *info)
{
	// CriticalSectionLock lock(GetExportSync());

	enable_lfh();
	Far::init(info);

	if (!g_options.load())
		g_options.save();

	if (g_options.max_arc_cache_size < 4)
		g_options.max_arc_cache_size = 4;

//	if (g_options.max_arc_cache_size > 1024)
//		g_options.max_arc_cache_size = 1024;

	g_profiles.load();

	g_plugin_prefix = g_options.plugin_prefix;
	ArchiveGlobals::max_check_size = g_options.max_check_size;
}

SHAREDSYMBOL void WINAPI GetPluginInfoW(PluginInfo *info)
{
	// CriticalSectionLock lock(GetExportSync());
	FAR_ERROR_HANDLER_BEGIN
	static const wchar_t *plugin_menu[1];
	static const wchar_t *config_menu[1];
	plugin_menu[0] = Far::msg_ptr(MSG_PLUGIN_NAME);
	config_menu[0] = Far::msg_ptr(MSG_PLUGIN_NAME);

	info->StructSize = sizeof(PluginInfo);

	info->PluginMenuStrings = plugin_menu;
	info->PluginMenuStringsNumber = ARRAYSIZE(plugin_menu);
	info->PluginConfigStrings = config_menu;
	info->PluginConfigStringsNumber = ARRAYSIZE(config_menu);
	info->CommandPrefix = g_plugin_prefix.c_str();

	FAR_ERROR_HANDLER_END(return, return, false)
}

static HANDLE analyse_open(const AnalyseInfo *info, bool from_analyse)
{
	GUARD(g_detect_next_time = triUndef);

	fprintf(stderr, "**********Analyse open ********************\n");

	OpenOptions options;
	options.arc_path = info->FileName;
	bool pgdn = (info->OpMode & OPM_PGDN) != 0;

	options.arc_types = ArcAPI::formats().get_arc_types();
	options.open_ex = !pgdn;
	options.nochain = pgdn;

	if (g_detect_next_time == triUndef) {

		options.detect = false;
		if (!g_options.handle_commands)
			FAIL(E_INVALIDARG);

		if (!pgdn || g_options.pgdn_masks) {
			if (g_options.use_include_masks
					&& !Far::match_masks(extract_file_name(info->FileName), g_options.include_masks))
				FAIL(E_INVALIDARG);
			if (g_options.use_exclude_masks
					&& Far::match_masks(extract_file_name(info->FileName), g_options.exclude_masks))
				FAIL(E_INVALIDARG);
		}

		if ((g_options.use_enabled_formats || g_options.use_disabled_formats)
				&& (pgdn ? g_options.pgdn_formats : true)) {

			std::set<std::wstring> enabled_formats;
			if (g_options.use_enabled_formats) {
				std::list<std::wstring> name_list = split(upcase(g_options.enabled_formats), L',');
				copy(name_list.cbegin(), name_list.cend(),
						inserter(enabled_formats, enabled_formats.begin()));
			}
			std::set<std::wstring> disabled_formats;
			if (g_options.use_disabled_formats) {
				std::list<std::wstring> name_list = split(upcase(g_options.disabled_formats), L',');
				copy(name_list.cbegin(), name_list.cend(),
						std::inserter(disabled_formats, disabled_formats.begin()));
			}

			const ArcFormats &arc_formats = ArcAPI::formats();
			ArcTypes::iterator arc_type = options.arc_types.begin();
			while (arc_type != options.arc_types.end()) {
				std::wstring arc_name = upcase(arc_formats.at(*arc_type).name);
				if (g_options.use_enabled_formats && enabled_formats.count(arc_name) == 0)
					arc_type = options.arc_types.erase(arc_type);
				else if (g_options.use_disabled_formats && disabled_formats.count(arc_name) != 0)
					arc_type = options.arc_types.erase(arc_type);
				else
					++arc_type;
			}

			if (options.arc_types.empty())
				FAIL(E_INVALIDARG);
		}

	} else {
		options.detect = g_detect_next_time == triTrue;
	}

	int password_len;
	options.open_password_len = &password_len;

	for (;;) {
//		password_len = from_analyse ? -'A' : 0;
		password_len = 0;

		if (ArcAPI::have_virt_destructor()) {
			std::unique_ptr<Archives<true>> archives(Archive<true>::open(options));
			if (!archives->empty())
				return archives.release();
		}
		else {
			std::unique_ptr<Archives<false>> archives(Archive<false>::open(options));
			if (!archives->empty())
				return archives.release();
		}

		if (from_analyse || password_len <= 0) {
			break;
		}
	}

	FAIL(password_len ? E_ABORT : E_FAIL);
}

SHAREDSYMBOL HANDLE WINAPI AnalyseW(const AnalyseInfo *info)
{
	// HANDLE WINAPI AnalyseW(const AnalyseData* info) {
	// CriticalSectionLock lock(GetExportSync());

	fprintf(stderr, " +++ AnalyseW( ) +++\n");
//	fprintf(stderr, "==== TREAD [%ld] \n", pthread_self());

//	fprintf(stderr, " +++ info->StructSize = %lu\n", info->StructSize);
//	fprintf(stderr, " +++ info->FileName   = %ls\n", info->FileName);
//	fprintf(stderr, " +++ info->Buffer     = %lx\n", (long)info->Buffer);
//	fprintf(stderr, " +++ info->BufferSize = %lu\n", info->BufferSize);
//	fprintf(stderr, " +++ info->Instance   = %lx\n", (long)info->Instance);
//	fprintf(stderr, " +++ info->OpMode     = %u\n", info->OpMode);

//	fprintf(stderr, "**********AnalyseW ********************\n");

	if (!g_options.plugin_enabled)
		return INVALID_HANDLE_VALUE;

	try {
		if (!info->FileName) {

			if (!g_options.handle_create) {
				FAIL(E_INVALIDARG);
			}

			if (!ArcAPI::libs().size()) {
				FAIL(E_INVALIDARG);
			}

			if (ArcAPI::have_virt_destructor())
				return new Archives<true>();
			else
				return new Archives<false>();

		} else {
			return analyse_open(info, true);
		}
	} catch (const Error &e) {

		return INVALID_HANDLE_VALUE;
	} catch (const std::exception & /*e*/) {

		return INVALID_HANDLE_VALUE;
	} catch (...) {

		return INVALID_HANDLE_VALUE;
	}
}

SHAREDSYMBOL void WINAPI CloseAnalyseW(const CloseAnalyseInfo *info)
{
	fprintf(stderr, " +++ CloseAnalyseW +++\n");
	// CriticalSectionLock lock(GetExportSync());
	if (info->Handle != INVALID_HANDLE_VALUE) {
		if (ArcAPI::have_virt_destructor())
			delete static_cast<Archives<true>*>(info->Handle);
		else
			delete static_cast<Archives<false>*>(info->Handle);
	}
}

SHAREDSYMBOL HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Data)
{
	// CriticalSectionLock lock(GetExportSync());

	bool delayed_analyse_open = false;
	FAR_ERROR_HANDLER_BEGIN

	fprintf(stderr, "**********OpenPluginW ********************\n");

	if (!g_options.plugin_enabled)
		return INVALID_HANDLE_VALUE;

	if (OpenFrom == OPEN_PLUGINSMENU) {
		Far::MenuItems menu_items;
		unsigned open_menu_id = menu_items.add(Far::get_msg(MSG_MENU_OPEN));
		unsigned detect_menu_id = menu_items.add(Far::get_msg(MSG_MENU_DETECT));
		unsigned create_menu_id = menu_items.add(Far::get_msg(MSG_MENU_CREATE));
		unsigned extract_menu_id = menu_items.add(Far::get_msg(MSG_MENU_EXTRACT));
		unsigned test_menu_id = menu_items.add(Far::get_msg(MSG_MENU_TEST));
		unsigned sfx_convert_menu_id = menu_items.add(Far::get_msg(MSG_MENU_SFX_CONVERT));

		auto item =
				(unsigned)Far::menu(c_main_menu_guid, Far::get_msg(MSG_PLUGIN_NAME), menu_items, L"Contents");

		if (item == open_menu_id || item == detect_menu_id) {
			OpenOptions options;
			options.detect = item == detect_menu_id;

//			fprintf(stderr, "OpenFrom == OPEN_PLUGINSMENU !!!\n");

//			fprintf(stderr, "detect_menu_id = %u item = %u\n", detect_menu_id, item );
//			fprintf(stderr, "Option detect = %u\n", options.detect );

			PanelInfo panel_info;
			if (!Far::get_panel_info(PANEL_ACTIVE, panel_info)) {
				return INVALID_HANDLE_VALUE;
			}

			Far::PanelItem panel_item = Far::get_current_panel_item(PANEL_ACTIVE);

			if (panel_item.file_attributes & FILE_ATTRIBUTE_DIRECTORY) {
				return INVALID_HANDLE_VALUE;
			}
			bool real_panel = Far::is_real_file_panel(panel_info);
			if (!real_panel) {
				if ((panel_item.file_attributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
					Far::post_macro(L"Keys('CtrlPgDn')");
					g_detect_next_time = options.detect ? triTrue : triFalse;
				}
				return INVALID_HANDLE_VALUE;
			}

			options.arc_path = add_trailing_slash(Far::get_panel_dir(PANEL_ACTIVE)) + panel_item.file_name;
			options.arc_types = ArcAPI::formats().get_arc_types();

			if (ArcAPI::have_virt_destructor())
				return Plugin<true>::open(*Archive<true>::open(options), real_panel);
			else
				return Plugin<false>::open(*Archive<false>::open(options), real_panel);

		} else if (item == create_menu_id) {

			if (ArcAPI::have_virt_destructor())
				Plugin<true>::create_archive();
			else
				Plugin<false>::create_archive();

		} else if (item == extract_menu_id || item == test_menu_id || item == sfx_convert_menu_id) {

			PanelInfo panel_info;

			if (!Far::get_panel_info(PANEL_ACTIVE, panel_info))
				return INVALID_HANDLE_VALUE;
			if (!Far::is_real_file_panel(panel_info))
				return INVALID_HANDLE_VALUE;

			std::vector<std::wstring> file_list;
			file_list.reserve(panel_info.SelectedItemsNumber);
			std::wstring dir = Far::get_panel_dir(PANEL_ACTIVE);

			for (size_t i = 0; i < (size_t)panel_info.SelectedItemsNumber; i++) {
				Far::PanelItem panel_item = Far::get_selected_panel_item(PANEL_ACTIVE, i);
				if ((panel_item.file_attributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
					std::wstring file_path = add_trailing_slash(dir) + panel_item.file_name;
					file_list.push_back(file_path);
				}
			}

			if (file_list.empty())
				return INVALID_HANDLE_VALUE;

			if (item == extract_menu_id) {
				if (ArcAPI::have_virt_destructor())
					Plugin<true>::bulk_extract(file_list);
				else
					Plugin<false>::bulk_extract(file_list);
			}
			else if (item == test_menu_id) {
				if (ArcAPI::have_virt_destructor())
					Plugin<true>::bulk_test(file_list);
				else
					Plugin<false>::bulk_test(file_list);
			}
			else if (item == sfx_convert_menu_id) {
				if (ArcAPI::have_virt_destructor())
					Plugin<true>::convert_to_sfx(file_list);
				else
					Plugin<false>::convert_to_sfx(file_list);
			}
		}
	}

#if 1
  else if (OpenFrom == OPEN_COMMANDLINE) {

    try {
//      CommandArgs cmd_args = parse_command(reinterpret_cast<OpenCommandLineInfo*>(Data)->CommandLine);
      CommandArgs cmd_args = parse_command((const wchar_t *)Data);
      switch (cmd_args.cmd) {
      case cmdOpen: {
        OpenOptions options = parse_open_command(cmd_args).options;
        options.arc_path = Far::get_absolute_path(options.arc_path);

		if (ArcAPI::have_virt_destructor()) {
	        auto plugin = Plugin<true>::open(*Archive<true>::open(options));
			  if (plugin) {
				  plugin->recursive_panel = options.recursive_panel;
				  plugin->del_on_close = options.delete_on_close;
			  }
			  return plugin;
		}
		else {
	        auto plugin = Plugin<false>::open(*Archive<false>::open(options));
			  if (plugin) {
				  plugin->recursive_panel = options.recursive_panel;
				  plugin->del_on_close = options.delete_on_close;
			  }
			  return plugin;
		}
      }
      case cmdCreate:
      case cmdUpdate:
		if (ArcAPI::have_virt_destructor())
	        Plugin<true>::cmdline_update(parse_update_command(cmd_args));
		else
	        Plugin<false>::cmdline_update(parse_update_command(cmd_args));
        break;
      case cmdExtract:
		if (ArcAPI::have_virt_destructor())
	        Plugin<true>::cmdline_extract(parse_extract_command(cmd_args));
		else
	        Plugin<false>::cmdline_extract(parse_extract_command(cmd_args));
        break;
      case cmdExtractItems:
		if (ArcAPI::have_virt_destructor())
	        Plugin<true>::cmdline_extract(parse_extractitems_command(cmd_args));
		else
	        Plugin<false>::cmdline_extract(parse_extractitems_command(cmd_args));
        break;
      case cmdDeleteItems:
		if (ArcAPI::have_virt_destructor())
	        Plugin<true>::cmdline_delete(parse_extractitems_command(cmd_args));
		else
	        Plugin<false>::cmdline_delete(parse_extractitems_command(cmd_args));
        break;
      case cmdTest:
		if (ArcAPI::have_virt_destructor())
	        Plugin<true>::cmdline_test(parse_test_command(cmd_args));
		else
	        Plugin<false>::cmdline_test(parse_test_command(cmd_args));
        break;
      }
    }
    catch (const Error& e) {
      if (e.code == E_BAD_FORMAT)
        Far::open_help(L"Prefix");
      else
        throw;
    }
  }
#endif
#if 1
  else if (OpenFrom == OPEN_FROMMACRO) {
    try {
//      CommandArgs cmd_args = parse_plugin_call(reinterpret_cast<const OpenMacroInfo*>(Data));
      CommandArgs cmd_args = parse_plugin_call(reinterpret_cast<const OpenMacroInfo*>(Data));

      switch (cmd_args.cmd) {
      case cmdCreate:
      case cmdUpdate:
		if (ArcAPI::have_virt_destructor())
	        Plugin<true>::cmdline_update(parse_update_command(cmd_args));
		else
	        Plugin<false>::cmdline_update(parse_update_command(cmd_args));
        break;
      case cmdExtract:
		if (ArcAPI::have_virt_destructor())
	        Plugin<true>::cmdline_extract(parse_extract_command(cmd_args));
		else
	        Plugin<false>::cmdline_extract(parse_extract_command(cmd_args));
        break;
      case cmdExtractItems:
		if (ArcAPI::have_virt_destructor())
	        Plugin<true>::cmdline_extract(parse_extractitems_command(cmd_args));
		else
	        Plugin<false>::cmdline_extract(parse_extractitems_command(cmd_args));
        break;
      case cmdDeleteItems:
		if (ArcAPI::have_virt_destructor())
	        Plugin<true>::cmdline_delete(parse_extractitems_command(cmd_args));
		else
	        Plugin<false>::cmdline_delete(parse_extractitems_command(cmd_args));
        break;
      case cmdTest:
		if (ArcAPI::have_virt_destructor())
	        Plugin<true>::cmdline_test(parse_test_command(cmd_args));
		else
	        Plugin<false>::cmdline_test(parse_test_command(cmd_args));
        break;
      default:
        break;
      }
    }
    catch (const Error& e) {
      (void)e; //if (e.code != E_BAD_FORMAT) throw;
      return INVALID_HANDLE_VALUE;
    }
    return INVALID_HANDLE_VALUE;
  }
#endif

	else if (OpenFrom == OPEN_ANALYSE) {
		const OpenAnalyseInfo *oai = reinterpret_cast<const OpenAnalyseInfo *>(Data);
		delayed_analyse_open = (oai->Handle == INVALID_HANDLE_VALUE);

		HANDLE handle = delayed_analyse_open ? analyse_open(oai->Info, false) : oai->Handle;
		delayed_analyse_open = false;

		if (ArcAPI::have_virt_destructor()) {
			std::unique_ptr<Archives<true>> archives(static_cast<Archives<true> *>(handle));
			bool real_panel = true;
			PanelInfo panel_info;

			if (Far::get_panel_info(PANEL_ACTIVE, panel_info) && !Far::is_real_file_panel(panel_info))
				real_panel = false;

			if (archives->empty()) {
				return new Plugin<true>(real_panel);
			} else {
				return Plugin<true>::open(*archives, real_panel);
			}
		}
		else {
			std::unique_ptr<Archives<false>> archives(static_cast<Archives<false> *>(handle));
			bool real_panel = true;
			PanelInfo panel_info;

			if (Far::get_panel_info(PANEL_ACTIVE, panel_info) && !Far::is_real_file_panel(panel_info))
				real_panel = false;

			if (archives->empty()) {
				return new Plugin<false>(real_panel);
			} else {
				return Plugin<false>::open(*archives, real_panel);
			}
		}
	}
#if 0
  else if (OpenFrom == OPEN_SHORTCUT) {
    const OpenShortcutInfo* osi = reinterpret_cast<const OpenShortcutInfo*>(Data);
    OpenOptions options;
    options.arc_path = null_to_empty(osi->HostFile);
    options.arc_types = ArcAPI::formats().get_arc_types();
    options.detect = true;
    return Plugin::open(*Archive::open(options));
  }
#endif
//	fprintf(stderr, " +++ OpenPluginW - return INVALID_HANDLE_VALUE!\n");

	return INVALID_HANDLE_VALUE;
	FAR_ERROR_HANDLER_END(return INVALID_HANDLE_VALUE,
			return delayed_analyse_open ? PANEL_STOP : INVALID_HANDLE_VALUE, delayed_analyse_open)
}

// void WINAPI ClosePanelW(const struct ClosePanelInfo* info) {
SHAREDSYMBOL void WINAPI ClosePluginW(HANDLE hPlugin)
{
	// CriticalSectionLock lock(GetExportSync());
	fprintf(stderr, " +++ ClosePluginW +++\n");
	FAR_ERROR_HANDLER_BEGIN
	if (ArcAPI::have_virt_destructor()) {
		Plugin<true> *plugin = reinterpret_cast<Plugin<true> *>(hPlugin);
		IGNORE_ERRORS(plugin->get_tail()->close());
		delete plugin;
	}
	else {
		Plugin<false> *plugin = reinterpret_cast<Plugin<false> *>(hPlugin);
		IGNORE_ERRORS(plugin->get_tail()->close());
		delete plugin;
	}
	FAR_ERROR_HANDLER_END(return, return, true)
//	fprintf(stderr, " OK +++ ClosePluginW +++ OK\n");
}

SHAREDSYMBOL void WINAPI GetOpenPluginInfoW(HANDLE hPlugin, struct OpenPluginInfo *Info)
{
	CriticalSectionLock lock(GetExportSync());
	FAR_ERROR_HANDLER_BEGIN

	if (ArcAPI::have_virt_destructor())
		reinterpret_cast<Plugin<true> *>(hPlugin)->get_tail()->info(Info);
	else
		reinterpret_cast<Plugin<false> *>(hPlugin)->get_tail()->info(Info);

	FAR_ERROR_HANDLER_END(return, return, false)
}

// intptr_t WINAPI SetDirectoryW(const SetDirectoryInfo* info) {
SHAREDSYMBOL int WINAPI SetDirectoryW(HANDLE hPlugin, const wchar_t *Dir, int OpMode)
{
	// CriticalSectionLock lock(GetExportSync());
	FAR_ERROR_HANDLER_BEGIN

	//fprintf(stderr, " +++ <<<<<<<<<<< SetDirectoryW( %ls ) OpMode = %i        >>>>>>>>>>>\n", Dir,  OpMode);

	if (ArcAPI::have_virt_destructor())
		reinterpret_cast<Plugin<true> *>(hPlugin)->get_tail()->set_dir(Dir);
	else
		reinterpret_cast<Plugin<false> *>(hPlugin)->get_tail()->set_dir(Dir);

	return TRUE;
	FAR_ERROR_HANDLER_END(return FALSE, return FALSE, (OpMode & (OPM_SILENT | OPM_FIND)) != 0)
}

SHAREDSYMBOL int WINAPI GetFindDataW(HANDLE hPlugin, struct PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode)
{
	// CriticalSectionLock lock(GetExportSync());

//	fprintf(stderr, "**********GetFindDataW  ********************\n");

	FAR_ERROR_HANDLER_BEGIN
	if (ArcAPI::have_virt_destructor())
		reinterpret_cast<Plugin<true> *>(hPlugin)->get_tail()->list(pPanelItem, pItemsNumber);
	else
		reinterpret_cast<Plugin<false> *>(hPlugin)->get_tail()->list(pPanelItem, pItemsNumber);
	return TRUE;
	FAR_ERROR_HANDLER_END(return FALSE, return FALSE, (OpMode & (OPM_SILENT | OPM_FIND)) != 0)
}

// void WINAPI FreeFindDataW(const FreeFindDataInfo* info) {
SHAREDSYMBOL void WINAPI FreeFindDataW(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber)
{
	// CriticalSectionLock lock(GetExportSync());
	FAR_ERROR_HANDLER_BEGIN
	delete[] PanelItem;
	FAR_ERROR_HANDLER_END(return, return, false)
}

// intptr_t WINAPI GetFilesW(GetFilesInfo *info) {
SHAREDSYMBOL int WINAPI GetFilesW(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber,
		int Move, const wchar_t **DestPath, int OpMode)
{
	// CriticalSectionLock lock(GetExportSync());
	FAR_ERROR_HANDLER_BEGIN
	if (ArcAPI::have_virt_destructor())
		reinterpret_cast<Plugin<true> *>(hPlugin)->get_tail()->get_files(PanelItem, ItemsNumber, Move, DestPath, OpMode);
	else
		reinterpret_cast<Plugin<false> *>(hPlugin)->get_tail()->get_files(PanelItem, ItemsNumber, Move, DestPath, OpMode);

	return 1;
	FAR_ERROR_HANDLER_END(return 0, return -1, (OpMode & (OPM_FIND | OPM_QUICKVIEW)) != 0)
}

// intptr_t WINAPI PutFilesW(const PutFilesInfo* info) {
SHAREDSYMBOL int WINAPI PutFilesW(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber,
		int Move, const wchar_t *SrcPath, int OpMode)
{
	FAR_ERROR_HANDLER_BEGIN
	if (ArcAPI::have_virt_destructor())
		reinterpret_cast<Plugin<true> *>(hPlugin)->get_tail()->put_files(PanelItem, ItemsNumber, Move, SrcPath, OpMode);
	else
		reinterpret_cast<Plugin<false> *>(hPlugin)->get_tail()->put_files(PanelItem, ItemsNumber, Move, SrcPath, OpMode);

	return 2;
	FAR_ERROR_HANDLER_END(return 0, return -1, (OpMode & OPM_FIND) != 0)
}

// intptr_t WINAPI DeleteFilesW(const DeleteFilesInfo* info) {
SHAREDSYMBOL int WINAPI
DeleteFilesW(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	// CriticalSectionLock lock(GetExportSync());
	FAR_ERROR_HANDLER_BEGIN
	if (ArcAPI::have_virt_destructor())
		reinterpret_cast<Plugin<true> *>(hPlugin)->get_tail()->delete_files(PanelItem, ItemsNumber, OpMode);
	else
		reinterpret_cast<Plugin<false> *>(hPlugin)->get_tail()->delete_files(PanelItem, ItemsNumber, OpMode);

	return TRUE;
	FAR_ERROR_HANDLER_END(return FALSE, return FALSE, (OpMode & OPM_SILENT) != 0)
}

// intptr_t WINAPI MakeDirectoryW(MakeDirectoryInfo* info) {
SHAREDSYMBOL int WINAPI MakeDirectoryW(HANDLE hPlugin, const wchar_t **Name, int OpMode)
{
	// CriticalSectionLock lock(GetExportSync());
	FAR_ERROR_HANDLER_BEGIN
	if (ArcAPI::have_virt_destructor())
		reinterpret_cast<Plugin<true> *>(hPlugin)->get_tail()->create_dir(Name, OpMode);
	else
		reinterpret_cast<Plugin<false> *>(hPlugin)->get_tail()->create_dir(Name, OpMode);

	return 1;
	FAR_ERROR_HANDLER_END(return -1, return -1, (OpMode & OPM_SILENT) != 0)
}

SHAREDSYMBOL int WINAPI
ProcessHostFileW(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{

	// intptr_t WINAPI ProcessHostFileW(const ProcessHostFileInfo* info) {
	// CriticalSectionLock lock(GetExportSync());

	FAR_ERROR_HANDLER_BEGIN
	Far::MenuItems menu_items;
	menu_items.add(Far::get_msg(MSG_TEST_MENU));

	intptr_t item = Far::menu(c_arccmd_menu_guid, Far::get_msg(MSG_PLUGIN_NAME), menu_items);

	if (item == 0) {
		if (ArcAPI::have_virt_destructor())
			reinterpret_cast<Plugin<true> *>(hPlugin)->get_tail()->test_files(PanelItem, ItemsNumber, OpMode);
		else
			reinterpret_cast<Plugin<false> *>(hPlugin)->get_tail()->test_files(PanelItem, ItemsNumber, OpMode);

		//    if (info->OpMode == OPM_NONE)
		if (OpMode == OPM_NONE)
			return FALSE;	 // to avoid setting modification flag (there is readonly test operation)
		else
			return TRUE;
	}

	return FALSE;

	FAR_ERROR_HANDLER_END(return FALSE, return FALSE, (OpMode & OPM_SILENT) != 0)
}

template<bool UseVirtualDestructor>
static bool set_partition(Plugin<UseVirtualDestructor> *hPlugin, const Far::PanelItem &i)
{
	if (hPlugin->parent && i.file_name == L".." && hPlugin->current_dir.empty()) {
		if (!hPlugin->get_archive()->is_open())
			FAIL(E_ABORT);

		//if (hPlugin->archive->arc_chain.size() < 2) {
		//	return false;
		//}
		hPlugin = hPlugin->parent;
		hPlugin->child.reset();
		hPlugin->set_dir(L"/");

		if (hPlugin->get_archive()->m_chain_file_index != 0xFFFFFFFF) {
			const std::wstring &cfpath = hPlugin->get_archive()->get_path(hPlugin->get_archive()->m_chain_file_index);

			hPlugin->set_dir(extract_file_path(cfpath));
			Far::update_panel(PANEL_ACTIVE, false, true);
			Far::panel_set_file(PANEL_ACTIVE, extract_file_name(cfpath));
			Far::update_panel(PANEL_ACTIVE, false, false);
		}
		return true;
	}

	return false;
}

template<bool UseVirtualDestructor>
static bool level_up(Plugin<UseVirtualDestructor> *hPlugin)
{
	if (!hPlugin->part_mode && hPlugin->parent) {
		if (!hPlugin->get_archive()->is_open())
			FAIL(E_ABORT);

		hPlugin = hPlugin->parent;
		hPlugin->child.reset();
		hPlugin->set_dir(L"/");

		if (hPlugin->get_archive()->m_chain_file_index != 0xFFFFFFFF) {
			const std::wstring &cfpath = hPlugin->get_archive()->get_path(hPlugin->get_archive()->m_chain_file_index);
			hPlugin->set_dir(extract_file_path(cfpath));
			Far::update_panel(PANEL_ACTIVE, false, true);
			Far::panel_set_file(PANEL_ACTIVE, extract_file_name(cfpath));
			Far::update_panel(PANEL_ACTIVE, false, false);
		}
		return true;
	}

	return hPlugin->level_up();
}

SHAREDSYMBOL int WINAPI _export ProcessKeyW(HANDLE hPlugin, int Key, unsigned int ControlState)
{
	// CriticalSectionLock lock(GetExportSync());

	FAR_ERROR_HANDLER_BEGIN

    // Ctrl+A
	if (Key == 'A' && ControlState == PKF_CONTROL) {
		if (ArcAPI::have_virt_destructor())
			reinterpret_cast<Plugin<true> *>(hPlugin)->get_tail()->show_attr();
		else
			reinterpret_cast<Plugin<false> *>(hPlugin)->get_tail()->show_attr();
		return TRUE;
	}

	if (Key == 'H' && ControlState == PKF_CONTROL) {
		if (ArcAPI::have_virt_destructor())
			reinterpret_cast<Plugin<true> *>(hPlugin)->get_tail()->show_hardlinks();
		else
			reinterpret_cast<Plugin<false> *>(hPlugin)->get_tail()->show_hardlinks();
		return TRUE;
	}

    // Alt+F6
	if (Key == VK_F6 && ControlState == PKF_ALT) {
		if (ArcAPI::have_virt_destructor())
			reinterpret_cast<Plugin<true> *>(hPlugin)->get_tail()->extract();
		else
			reinterpret_cast<Plugin<false> *>(hPlugin)->get_tail()->extract();
		return TRUE;
	}


	if ( Key == VK_LEFT &&
			(ControlState == (PKF_CONTROL | PKF_ALT) || ControlState == (PKF_CONTROL | PKF_SHIFT)) ) {
//		fprintf(stderr, "*********ProcessKeyW  handle ctrl + alt + left  ********************\n");

		bool bRez;
		if (ArcAPI::have_virt_destructor())
			bRez = level_up(reinterpret_cast<Plugin<true> *>(hPlugin)->get_tail());
		else
			bRez = level_up(reinterpret_cast<Plugin<false> *>(hPlugin)->get_tail());

		if (bRez) {
			PanelInfo panel_info2;
			Far::get_panel_info(PANEL_PASSIVE, panel_info2);
			if (panel_info2.PanelType == PTYPE_INFOPANEL) {
				Far::update_panel(PANEL_PASSIVE, true, false); // for update Info panel
			}
			return TRUE;
		}
		else {
			return FALSE;
		}
	}

	if ( Key == VK_RIGHT &&
			(ControlState == (PKF_CONTROL | PKF_ALT) || ControlState == (PKF_CONTROL | PKF_SHIFT)) ) {

		Far::PanelItem panel_item = Far::get_current_panel_item(PANEL_ACTIVE);
		bool bRez;

		if (ArcAPI::have_virt_destructor())
			bRez = reinterpret_cast<Plugin<true> *>(hPlugin)->get_tail()->level_down(panel_item);
		else
			bRez = reinterpret_cast<Plugin<false> *>(hPlugin)->get_tail()->level_down(panel_item);

		if (bRez) {
			PanelInfo panel_info2;
			Far::get_panel_info(PANEL_PASSIVE, panel_info2);
			if (panel_info2.PanelType == PTYPE_INFOPANEL) {
				Far::update_panel(PANEL_PASSIVE, true, false); // for update Info panel
			}
			return TRUE;
		}
		else {
			return FALSE;
		}
	}

	if ( Key == VK_RETURN && (!ControlState || (ControlState & PKF_CONTROL)) ) {

		Far::PanelItem panel_item = Far::get_current_panel_item(PANEL_ACTIVE);
		bool bRez = false;

		if (ArcAPI::have_virt_destructor())
			bRez = reinterpret_cast<Plugin<true> *>(hPlugin)->get_tail()->level_down(panel_item);
		else
			bRez = reinterpret_cast<Plugin<false> *>(hPlugin)->get_tail()->level_down(panel_item);

		if (!bRez) {
			if (ArcAPI::have_virt_destructor())
				bRez = set_partition(reinterpret_cast<Plugin<true> *>(hPlugin)->get_tail(), panel_item);
			else
				bRez = set_partition(reinterpret_cast<Plugin<false> *>(hPlugin)->get_tail(), panel_item);
		}

		if (bRez) {
			PanelInfo panel_info2;
			Far::get_panel_info(PANEL_PASSIVE, panel_info2);
			if (panel_info2.PanelType == PTYPE_INFOPANEL) {
				Far::update_panel(PANEL_PASSIVE, true, false); // for update Info panel
			}
			return TRUE;
		}

		return FALSE;
	}


#if 0
	if (Key == 'Z' && ControlState == PKF_CONTROL) {
		plugin->describe_files();
//		void describe_files(const PluginPanelItem *panel_items, intptr_t items_number, OPERATION_MODES op_mode)
		return TRUE;
	}
#endif

	return FALSE;
	FAR_ERROR_HANDLER_END(return FALSE, return FALSE, false)
}

SHAREDSYMBOL int WINAPI ConfigureW(int ItemNumber)
{
	FAR_ERROR_HANDLER_BEGIN
	PluginSettings settings;

	settings.plugin_enabled = g_options.plugin_enabled;
	settings.handle_create = g_options.handle_create;
	settings.handle_commands = g_options.handle_commands;
	settings.own_panel_view_mode = g_options.own_panel_view_mode;
	settings.use_include_masks = g_options.use_include_masks;
	settings.include_masks = g_options.include_masks;
	settings.use_exclude_masks = g_options.use_exclude_masks;
	settings.exclude_masks = g_options.exclude_masks;
	settings.pgdn_masks = g_options.pgdn_masks;
	settings.use_enabled_formats = g_options.use_enabled_formats;
	settings.enabled_formats = g_options.enabled_formats;
	settings.use_disabled_formats = g_options.use_disabled_formats;
	settings.disabled_formats = g_options.disabled_formats;
	settings.pgdn_formats = g_options.pgdn_formats;
	settings.patchCP = g_options.patchCP;
	settings.oemCP = (UINT)g_options.oemCP;
	settings.ansiCP = (UINT)g_options.ansiCP;
	settings.preferred_7zip_path = g_options.preferred_7zip_path;

	if (settings_dialog(settings)) {
		g_options.plugin_enabled = settings.plugin_enabled;
		g_options.handle_create = settings.handle_create;
		g_options.handle_commands = settings.handle_commands;
		g_options.own_panel_view_mode = settings.own_panel_view_mode;
		g_options.use_include_masks = settings.use_include_masks;
		g_options.include_masks = settings.include_masks;
		g_options.use_exclude_masks = settings.use_exclude_masks;
		g_options.exclude_masks = settings.exclude_masks;
		g_options.pgdn_masks = settings.pgdn_masks;
		g_options.use_enabled_formats = settings.use_enabled_formats;
		g_options.enabled_formats = settings.enabled_formats;
		g_options.use_disabled_formats = settings.use_disabled_formats;
		g_options.disabled_formats = settings.disabled_formats;
		g_options.pgdn_formats = settings.pgdn_formats;
		g_options.patchCP = settings.patchCP;
		g_options.oemCP = settings.oemCP;
		g_options.ansiCP = settings.ansiCP;
		g_options.save();
		if (g_options.patchCP) {
			Patch7zCP::SetCP(settings.oemCP, settings.ansiCP);
		}
		g_options.preferred_7zip_path = settings.preferred_7zip_path;
		return TRUE;
	} else
		return FALSE;
	FAR_ERROR_HANDLER_END(return FALSE, return FALSE, false)
}

SHAREDSYMBOL void WINAPI ExitFARW(void)
{
	// CriticalSectionLock lock(GetExportSync());
	ArcAPI::free();
}

// #endif
