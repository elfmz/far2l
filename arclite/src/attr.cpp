#include "headers.hpp"

#include "msg.hpp"
#include "error.hpp"
#include "utils.hpp"
#include "farutils.hpp"
#include "sysutils.hpp"
#include "common.hpp"
#include "comutils.hpp"
#include "ui.hpp"
#include "archive.hpp"
#include "options.hpp"

static std::wstring uint_to_hex_str(UInt64 val, unsigned num_digits = 0)
{
	wchar_t str[16];
	unsigned pos = 16;
	do {
		unsigned d = static_cast<unsigned>(val % 16);
		pos--;
		str[pos] = d < 10 ? d + L'0' : d - 10 + L'A';
		val /= 16;
	} while (val);
	if (num_digits) {
		while (pos + num_digits > 16) {
			pos--;
			str[pos] = L'0';
		}
	}
	return std::wstring(str + pos, 16 - pos);
}

static std::wstring format_str_prop(const PropVariant &prop)
{
	std::wstring str = prop.get_str();
	for (unsigned i = 0; i < str.size(); i++)
		if (str[i] == L'\r' || str[i] == L'\n')
			str[i] = L' ';
	return str;
}

static std::wstring format_int_prop(const PropVariant &prop)
{
	wchar_t buf[32];
	swprintf(buf, 32, L"%lld", prop.get_int());
	//  return std::wstring(_i64tow(prop.get_int(), buf, 10));
	return std::wstring(buf);
}

static std::wstring format_uint_prop(const PropVariant &prop)
{
	wchar_t buf[32];
	swprintf(buf, 32, L"%llu", prop.get_uint());
	//  return std::wstring(_ui64tow(prop.get_uint(), buf, 10));
	return std::wstring(buf);
}

static std::wstring format_size_prop(const PropVariant &prop)
{
	if (!prop.is_uint())
		return std::wstring();
	std::wstring short_size = format_data_size(prop.get_uint(), get_size_suffixes());
	std::wstring long_size = format_uint_prop(prop);
	if (short_size == long_size)
		return short_size;
	else
		return short_size + L" = " + long_size;
}

static std::wstring format_filetime_prop(const PropVariant &prop)
{
	if (!prop.is_filetime())
		return std::wstring();

	union {
		FILETIME ft;
		uint64_t t64;
	};

//	if (t64 > 0x23F4449128B48000) {
//	}

	ft = prop.get_filetime();
#if IS_BIG_ENDIAN
		return format_file_time(FILETIME{ft.dwLowDateTime,ft.dwHighDateTime});
#else
	return format_file_time(ft);
#endif

}

static std::wstring format_crc_prop(const PropVariant &prop)
{
	if (!prop.is_uint())
		return std::wstring();
	return uint_to_hex_str(prop.get_uint(), prop.get_int_size() * 2);
}

static const wchar_t kPosixTypes[16 + 1] = L"0pc3d5b7-9lBsDEF";
#define ATTR_CHAR(a, n, c) (((a) & (1 << (n))) ? c : L'-')

static std::wstring format_posix_attrib_prop(const PropVariant &prop)
{
	if (!prop.is_uint())
		return std::wstring();

	unsigned val = static_cast<unsigned>(prop.get_uint());
	wchar_t attr[10];

	attr[0] = kPosixTypes[(val >> 12) & 0xF];
	for (int i = 6; i >= 0; i -= 3) {
		attr[7 - i] = ATTR_CHAR(val, i + 2, L'r');
		attr[8 - i] = ATTR_CHAR(val, i + 1, L'w');
		attr[9 - i] = ATTR_CHAR(val, i + 0, L'x');
	}
	if ((val & 0x800) != 0)
		attr[3] = ((val & (1 << 6)) ? L's' : L'S');
	if ((val & 0x400) != 0)
		attr[6] = ((val & (1 << 3)) ? L's' : L'S');
	if ((val & 0x200) != 0)
		attr[9] = ((val & (1 << 0)) ? L't' : L'T');

	val &= ~(unsigned)0xFFFF;
	return val ? std::wstring(attr, 10) + L' ' + uint_to_hex_str(val, 8) : std::wstring(attr, 10);
}

static const unsigned kNumWinAtrribFlags = 21;
static const wchar_t g_WinAttribChars[kNumWinAtrribFlags + 1] = L"RHS8DAdNTsLCOIEV.X.PU";

/* FILE_ATTRIBUTE_
 0 READONLY
 1 HIDDEN
 2 SYSTEM
 3 (Volume label - obsolete)
 4 DIRECTORY
 5 ARCHIVE
 6 DEVICE
 7 NORMAL
 8 TEMPORARY
 9 SPARSE_FILE
10 REPARSE_POINT
11 COMPRESSED
12 OFFLINE
13 NOT_CONTENT_INDEXED (I - Win10 attrib/Explorer)
14 ENCRYPTED
15 INTEGRITY_STREAM (V - ReFS Win8/Win2012)
16 VIRTUAL (reserved)
17 NO_SCRUB_DATA (X - ReFS Win8/Win2012 attrib)
18 RECALL_ON_OPEN or EA
19 PINNED
20 UNPINNED
21 STRICTLY_SEQUENTIAL
22 RECALL_ON_DATA_ACCESS
*/

static std::wstring format_attrib_prop(const PropVariant &prop)
{
	if (!prop.is_uint())
		return std::wstring();

	std::pair<DWORD, DWORD> pxattr = get_posix_and_nt_attributes(static_cast<DWORD>(prop.get_uint()));
	DWORD posix = pxattr.first, val = pxattr.second;

	wchar_t attr[kNumWinAtrribFlags];
	size_t na = 0;
	for (unsigned i = 0; i < kNumWinAtrribFlags; i++) {
		unsigned flag = (1U << i);
		if ((val & flag) != 0) {
			auto c = g_WinAttribChars[i];
			if (c != L'.') {
				val &= ~flag;
				// if (i != 7) // we can disable N (NORMAL) printing
				{
					attr[na++] = c;
				}
			}
		}
	}
	auto res = std::wstring(attr, na);

	if (val != 0) {
		if (na)
			res += L' ';
		res += uint_to_hex_str(val, 8);
	}

	if (posix) {
		if (!res.empty())
			res += L' ';
		PropVariant p((UInt32)posix);
		res += format_posix_attrib_prop(p);
	}

	return res;
}

typedef std::wstring (*PropToString)(const PropVariant &var);

struct PropInfo
{
	PROPID prop_id;
	unsigned name_id;
	PropToString prop_to_string;
};

static PropInfo c_prop_info[] = {
	{ kpidPath,					MSG_KPID_PATH,					nullptr },
	{ kpidName,					MSG_KPID_NAME,					nullptr },
	{ kpidExtension,			MSG_KPID_EXTENSION,				nullptr },
	{ kpidIsDir,				MSG_KPID_ISDIR,					nullptr },
	{ kpidSize,					MSG_KPID_SIZE,					format_size_prop },
	{ kpidPackSize,				MSG_KPID_PACKSIZE,				format_size_prop },
	{ kpidAttrib,				MSG_KPID_ATTRIB,				format_attrib_prop },
	{ kpidCTime,				MSG_KPID_CTIME,					format_filetime_prop },
	{ kpidATime,				MSG_KPID_ATIME,					format_filetime_prop },
	{ kpidMTime,				MSG_KPID_MTIME,					format_filetime_prop },
	{ kpidSolid,				MSG_KPID_SOLID,					nullptr },
	{ kpidCommented,			MSG_KPID_COMMENTED,				nullptr },
	{ kpidEncrypted,			MSG_KPID_ENCRYPTED,				nullptr },
	{ kpidSplitBefore,			MSG_KPID_SPLITBEFORE,			nullptr },
	{ kpidSplitAfter,			MSG_KPID_SPLITAFTER,			nullptr },
	{ kpidDictionarySize,		MSG_KPID_DICTIONARYSIZE,		format_size_prop },
	{ kpidCRC,					MSG_KPID_CRC,					format_crc_prop },
	{ kpidType,					MSG_KPID_TYPE,					nullptr },
	{ kpidIsAnti,				MSG_KPID_ISANTI,				nullptr },
	{ kpidMethod,				MSG_KPID_METHOD,				nullptr },
	{ kpidHostOS,				MSG_KPID_HOSTOS,				nullptr },
	{ kpidFileSystem,			MSG_KPID_FILESYSTEM,			nullptr },
	{ kpidUser,					MSG_KPID_USER,					nullptr },
	{ kpidGroup,				MSG_KPID_GROUP,					nullptr },
	{ kpidBlock,				MSG_KPID_BLOCK,					nullptr },
	{ kpidComment,				MSG_KPID_COMMENT,				nullptr },
	{ kpidPosition,				MSG_KPID_POSITION,				nullptr },
	{ kpidPrefix,				MSG_KPID_PREFIX,				nullptr },
	{ kpidNumSubDirs,			MSG_KPID_NUMSUBDIRS,			nullptr },
	{ kpidNumSubFiles,			MSG_KPID_NUMSUBFILES,			nullptr },
	{ kpidUnpackVer,			MSG_KPID_UNPACKVER,				nullptr },
	{ kpidVolume,				MSG_KPID_VOLUME,				nullptr },
	{ kpidIsVolume,				MSG_KPID_ISVOLUME,				nullptr },
	{ kpidOffset,				MSG_KPID_OFFSET,				nullptr },
	{ kpidLinks,				MSG_KPID_LINKS,					nullptr },
	{ kpidNumBlocks,			MSG_KPID_NUMBLOCKS,				nullptr },
	{ kpidNumVolumes,			MSG_KPID_NUMVOLUMES,			nullptr },
	{ kpidTimeType,				MSG_KPID_TIMETYPE,				nullptr },
	{ kpidBit64,				MSG_KPID_BIT64,					nullptr },
	{ kpidBigEndian,			MSG_KPID_BIGENDIAN,				nullptr },
	{ kpidCpu,					MSG_KPID_CPU,					nullptr },
	{ kpidPhySize,				MSG_KPID_PHYSIZE,				format_size_prop },
	{ kpidHeadersSize,			MSG_KPID_HEADERSSIZE,			format_size_prop },
	{ kpidChecksum,				MSG_KPID_CHECKSUM,				nullptr },
	{ kpidCharacts,				MSG_KPID_CHARACTS,				nullptr },
	{ kpidVa,					MSG_KPID_VA,					nullptr },
	{ kpidId,					MSG_KPID_ID,					nullptr },
	{ kpidShortName,			MSG_KPID_SHORTNAME,				nullptr },
	{ kpidCreatorApp,			MSG_KPID_CREATORAPP,			nullptr },
	{ kpidSectorSize,			MSG_KPID_SECTORSIZE,			format_size_prop },
	{ kpidPosixAttrib,			MSG_KPID_POSIXATTRIB,			format_posix_attrib_prop },
	{ kpidSymLink,				MSG_KPID_LINK,					nullptr },
	{ kpidError,				MSG_KPID_ERROR,					nullptr },
	{ kpidTotalSize,			MSG_KPID_TOTALSIZE,				format_size_prop },
	{ kpidFreeSpace,			MSG_KPID_FREESPACE,				format_size_prop },
	{ kpidClusterSize,			MSG_KPID_CLUSTERSIZE,			format_size_prop },
	{ kpidVolumeName,			MSG_KPID_VOLUMENAME,			nullptr },
	{ kpidLocalName,			MSG_KPID_LOCALNAME,				nullptr },
	{ kpidProvider,				MSG_KPID_PROVIDER,				nullptr },
	{ kpidNtSecure,				MSG_KPID_NTSECURE,				nullptr },
	{ kpidIsAltStream,			MSG_KPID_ISALTSTREAM,			nullptr },
	{ kpidIsAux,				MSG_KPID_ISAUX,					nullptr },
	{ kpidIsDeleted,			MSG_KPID_ISDELETED,				nullptr },
	{ kpidIsTree,				MSG_KPID_ISTREE,				nullptr },
	{ kpidSha1,					MSG_KPID_SHA1,					nullptr },
	{ kpidSha256,				MSG_KPID_SHA256,				nullptr },
	{ kpidErrorType,			MSG_KPID_ERRORTYPE,				nullptr },
	{ kpidNumErrors,			MSG_KPID_NUMERRORS,				nullptr },
	{ kpidErrorFlags,			MSG_KPID_ERRORFLAGS,			nullptr },
	{ kpidWarningFlags,			MSG_KPID_WARNINGFLAGS,			nullptr },
	{ kpidWarning,				MSG_KPID_WARNING,				nullptr },
	{ kpidNumStreams,			MSG_KPID_NUMSTREAMS,			nullptr },
	{ kpidNumAltStreams,		MSG_KPID_NUMALTSTREAMS,			nullptr },
	{ kpidAltStreamsSize,		MSG_KPID_ALTSTREAMSSIZE,		format_size_prop },
	{ kpidVirtualSize,			MSG_KPID_VIRTUALSIZE,			format_size_prop },
	{ kpidUnpackSize,			MSG_KPID_UNPACKSIZE,			format_size_prop },
	{ kpidTotalPhySize,			MSG_KPID_TOTALPHYSIZE,			format_size_prop },
	{ kpidVolumeIndex,			MSG_KPID_VOLUMEINDEX,			nullptr },
	{ kpidSubType,				MSG_KPID_SUBTYPE,				nullptr },
	{ kpidShortComment,			MSG_KPID_SHORTCOMMENT,			nullptr },
	{ kpidCodePage,				MSG_KPID_CODEPAGE,				nullptr },
	{ kpidIsNotArcType,			MSG_KPID_ISNOTARCTYPE,			nullptr },
	{ kpidPhySizeCantBeDetected,MSG_KPID_PHYSIZECANTBEDETECTED,	nullptr },
	{ kpidZerosTailIsAllowed,	MSG_KPID_ZEROSTAILISALLOWED,	nullptr },
	{ kpidTailSize,				MSG_KPID_TAILSIZE,				format_size_prop },
	{ kpidEmbeddedStubSize,		MSG_KPID_EMBEDDEDSTUBSIZE,		format_size_prop },
	{ kpidNtReparse,			MSG_KPID_NTREPARSE,				nullptr },
	{ kpidHardLink,				MSG_KPID_HARDLINK,				nullptr },
	{ kpidINode,				MSG_KPID_INODE,					nullptr },
	{ kpidStreamId,				MSG_KPID_STREAMID,				nullptr },
	{ kpidReadOnly,				MSG_KPID_READONLY,				nullptr },
	{ kpidOutName,				MSG_KPID_OUTNAME,				nullptr },
	{ kpidCopyLink,				MSG_KPID_COPYLINK,				nullptr },
	{ kpidArcFileName,			MSG_KPID_ARCFILENAME,			nullptr },
	{ kpidIsHash,				MSG_KPID_ISHASH,				nullptr },
	{ kpidChangeTime,			MSG_KPID_METADATA_CHANGED,		nullptr },
	{ kpidUserId,				MSG_KPID_USER_ID,				nullptr },
	{ kpidGroupId,				MSG_KPID_GROUP_ID,				nullptr },
	{ kpidDeviceMajor,			MSG_KPID_DEVICE_MAJOR,			nullptr },
	{ kpidDeviceMinor,			MSG_KPID_DEVICE_MINOR,			nullptr },
	{ kpidDevMajor, 			MSG_KPID_DEV_MAJOR,				nullptr },
	{ kpidDevMinor,				MSG_KPID_DEV_MINOR,				nullptr }
};

static const PropInfo *find_prop_info(PROPID prop_id)
{
	if (prop_id < kpidPath || prop_id >= kpid_NUM_DEFINED)
		return nullptr;
	else
		return c_prop_info + (prop_id - kpidPath);
}

#include "ntfs_defs.hpp"

static std::wstring utf16_to_wstring(const uint8_t* utf16_bytes, size_t byte_len) {
	std::wstring result;
	if (byte_len % 2 != 0) return result;
	result.reserve(byte_len / 2);

	const uint16_t* utf16_le = reinterpret_cast<const uint16_t*>(utf16_bytes);
	size_t char_count = byte_len / 2;

	for (size_t i = 0; i < char_count; i++) {
		uint16_t ch = le16_to_host(utf16_le[i]);

		if ((ch & 0xFC00) == 0xD800 && i + 1 < char_count) {
			uint16_t ch2 = le16_to_host(utf16_le[i + 1]);
			if ((ch2 & 0xFC00) == 0xDC00) {
				uint32_t codepoint = 0x10000 + ((ch & 0x3FF) << 10) + (ch2 & 0x3FF);
				result.push_back(static_cast<wchar_t>(codepoint));
				i++;
				continue;
			}
		}
		result.push_back(static_cast<wchar_t>(ch));
	}
	return result;
}

static std::wstring decode_nt_reparse_buffer(const void* data, bool bPureLink = false) {
	if (!data)
		return L"";

	const auto* rdb = static_cast<const REPARSE_DATA_BUFFER*>(data);
	std::wstring result;
	uint32_t reparse_tag = le32_to_host(rdb->ReparseTag);
//	uint16_t data_length = le16_to_host(rdb->ReparseDataLength);

	switch (reparse_tag) {

	case 0xA000000C:  { // IO_REPARSE_TAG_SYMLINK
		const auto &sym = rdb->SymbolicLinkReparseBuffer;

		uint16_t sub_name_offset = le16_to_host(sym.SubstituteNameOffset);
		uint16_t sub_name_length = le16_to_host(sym.SubstituteNameLength);
		uint16_t print_name_offset = le16_to_host(sym.PrintNameOffset);
		uint16_t print_name_length = le16_to_host(sym.PrintNameLength);
		uint32_t flags = le32_to_host(sym.Flags);
		if (print_name_length) {
			const uint8_t* print_name = reinterpret_cast<const uint8_t*>(sym.PathBuffer) + print_name_offset;
			std::wstring print_str = utf16_to_wstring(print_name, print_name_length);
			for (auto &c : print_str) if (c == L'\\') c = L'/';
			result += print_str;
		}

		if (bPureLink)
			return result;

		result += L" (Symlink ";
		if (flags & 0x00000001L)
			result += L"relative)";
		else
			result += L"absolute)";

		if (sub_name_length) {
			const uint8_t* subst_name = reinterpret_cast<const uint8_t*>(sym.PathBuffer) + sub_name_offset;
			std::wstring subst_str = utf16_to_wstring(subst_name, sub_name_length);
			result += L" [" + subst_str + L"]";
		}
		break;
	}
	case 0xA0000019: // IO_REPARSE_TAG_GLOBAL_REPARSE
	case 0xA0000003: {  // IO_REPARSE_TAG_MOUNT_POINT
		const auto &mp = rdb->MountPointReparseBuffer;
		uint16_t sub_name_offset = le16_to_host(mp.SubstituteNameOffset);
		uint16_t sub_name_length = le16_to_host(mp.SubstituteNameLength);
		uint16_t print_name_offset = le16_to_host(mp.PrintNameOffset);
		uint16_t print_name_length = le16_to_host(mp.PrintNameLength);

		if (print_name_length) {
			const uint8_t* print_name = reinterpret_cast<const uint8_t*>(mp.PathBuffer) + print_name_offset;
			std::wstring print_str = utf16_to_wstring(print_name, print_name_length);
			for (auto &c : print_str) if (c == L'\\') c = L'/';
			result += print_str;
		}

		if (bPureLink)
			return result;

		if (reparse_tag == 0xA0000019)
			result += L" (Global Reparse)";
		else
			result += L" (Junction)";
		if (sub_name_length) {
			const uint8_t* subst_name = reinterpret_cast<const uint8_t*>(mp.PathBuffer) + sub_name_offset;
			std::wstring subst_str = utf16_to_wstring(subst_name, sub_name_length);
			result += L" [" + subst_str + L" ]";
		}
		break;
	}

	case 0x8000001B:  // IO_REPARSE_TAG_APPEXECLINK
		result = L"AppExecLink: Windows Store App";

	case 0x80000018:  // IO_REPARSE_TAG_WCI
	case 0x90001018:  // IO_REPARSE_TAG_WCI_1
		result = L"WCI: Windows Container Isolation";

	case 0x9000001C:  // IO_REPARSE_TAG_PROJFS
		result = L"ProjFS: Virtualized file (Git/WSL)";

	case 0x80000017:  // IO_REPARSE_TAG_WOF
		result = L"WOF: Compressed (LZX/Xpress)";

	case 0x80000013:  // IO_REPARSE_TAG_DEDUP
		result = L"Dedup: Data deduplication";

	case 0x8000001E:  // IO_REPARSE_TAG_STORAGE_SYNC
		result = L"StorageSync: Cloud sync file";

	case 0x80000021:  // IO_REPARSE_TAG_ONEDRIVE
	case 0x9000001A:  // IO_REPARSE_TAG_CLOUD
	case 0x9000101A:  // IO_REPARSE_TAG_CLOUD_1
	case 0x9000201A:
	case 0x9000301A:
	case 0x9000401A:
	case 0x9000501A:
	case 0x9000601A:
	case 0x9000701A:
	case 0x9000801A:
	case 0x9000901A:
	case 0x9000A01A:
	case 0x9000B01A:
	case 0x9000C01A:
	case 0x9000D01A:
	case 0x9000E01A:
	case 0x9000F01A:
		result = L"Cloud: OneDrive / SharePoint / Azure";

	case 0xC0000004:  // IO_REPARSE_TAG_HSM
	case 0x80000006:  // IO_REPARSE_TAG_HSM2
		result = L"HSM: Hierarchical Storage (archived)";

	case 0x80000007:  // IO_REPARSE_TAG_SIS
		result = L"SIS: Single Instance Storage (legacy)";

	case 0x8000000A:  // IO_REPARSE_TAG_DFS
	case 0x80000012:  // IO_REPARSE_TAG_DFSR
		result = L"DFS: Distributed File System";

	default:
		if ((reparse_tag & 0x80000000) == 0) {
			result = L"Custom Reparse Tag: " + uint_to_hex_str(reparse_tag, 8);
		} else {
			result = L"Microsoft Reparse Tag: " + uint_to_hex_str(reparse_tag, 8);
		}
	}

	return result;
}

static std::wstring sid_to_string(const SID* sid) {
	if (!sid || sid->Revision != 1) return L"INVALID_SID";

	std::wstring result = L"S-1-";

	// Identifier Authority
	uint64_t authority = 0;
	for (int i = 0; i < 6; i++) {
		authority = (authority << 8) | sid->IdentifierAuthority.Value[i];
	}
	result += std::to_wstring(authority);

	const uint8_t* sid_bytes = reinterpret_cast<const uint8_t*>(sid);
	const uint32_t* sub_auth_ptr = reinterpret_cast<const uint32_t*>(
		sid_bytes + offsetof(SID, IdentifierAuthority) + sizeof(SID_IDENTIFIER_AUTHORITY));

	std::vector<uint32_t> sub_auths;
	for (uint8_t i = 0; i < sid->SubAuthorityCount; i++) {
		uint32_t raw_sub = sub_auth_ptr[i];
		uint32_t sub_auth = le32_to_host(raw_sub);
		sub_auths.push_back(sub_auth);
		result += L"-" + std::to_wstring(sub_auth);
	}

	if (!sub_auths.empty()) {
		if (authority == 5) { // SECURITY_NT_AUTHORITY
			uint32_t sub0 = sub_auths[0];
			uint32_t sub1 = (sub_auths.size() >= 2) ? sub_auths[1] : 0;
			uint32_t sub2 = (sub_auths.size() >= 3) ? sub_auths[2] : 0;

			if (sub_auths.size() == 1 && (sub0 == 1 || sub0 == 18)) {
				return L"LOCAL_SYSTEM";
			}

			if (sub_auths.size() == 2 && sub0 == 32 && sub1 == 544) {
				return L"BUILTIN_ADMINISTRATORS";
			}

			if (sub_auths.size() == 3 && sub0 == 5 && sub1 == 32) {
				switch (sub2) {
					case 544: return L"BUILTIN_ADMINISTRATORS";
					case 545: return L"BUILTIN_USERS";
					case 546: return L"BUILTIN_GUESTS";
					case 547: return L"BUILTIN_POWER_USERS";
					case 548: return L"BUILTIN_ACCOUNT_OPERATORS";
					case 549: return L"BUILTIN_SYSTEM_OPERATORS";
					case 550: return L"BUILTIN_PRINT_OPERATORS";
					case 551: return L"BUILTIN_BACKUP_OPERATORS";
					case 552: return L"BUILTIN_REPLICATOR";
					case 582: return L"BUILTIN_REMOTE_DESKTOP_USERS";
				}
			}

			if (sub_auths.size() == 2 && sub0 == 5) {
				switch (sub1) {
					case 1:  return L"EVERYONE";
					case 9:  return L"ENTERPRISE_DOMAIN_CONTROLLERS";
					case 10: return L"SELF";
					case 11: return L"AUTHENTICATED_USERS";
					case 18: return L"LOCAL_SYSTEM";
					case 19: return L"LOCAL_SERVICE";
					case 20: return L"NETWORK_SERVICE";
					case 21: return L"ANONYMOUS";
				}
			}
		}
		else if (authority == 1) { // SECURITY_WORLD_SID_AUTHORITY
			uint32_t sub0 = sub_auths[0];
			if (sub_auths.size() == 1 && sub0 == 0) {
				return L"EVERYONE";
			}
		}
		else if (authority == 2) { // SECURITY_LOCAL_SID_AUTHORITY
			uint32_t sub0 = sub_auths[0];
			if (sub_auths.size() == 1 && sub0 == 0) {
				return L"LOCAL";
			}
		}
		else if (authority == 3) { // SECURITY_CREATOR_SID_AUTHORITY
			uint32_t sub0 = sub_auths[0];
			if (sub_auths.size() == 1 && sub0 == 1) {
				return L"CREATOR_OWNER";
			}
			if (sub_auths.size() == 1 && sub0 == 2) {
				return L"CREATOR_GROUP";
			}
		}
	}
	return result;
}

static std::wstring access_mask_to_string(uint32_t mask) {
	std::wstring result;

	if (mask & GENERIC_ALL) { result += L"GA|"; mask &= ~GENERIC_ALL; }
	if (mask & GENERIC_READ) { result += L"GR|"; mask &= ~GENERIC_READ; }
	if (mask & GENERIC_WRITE) { result += L"GW|"; mask &= ~GENERIC_WRITE; }
	if (mask & GENERIC_EXECUTE) { result += L"GX|"; mask &= ~GENERIC_EXECUTE; }

	if (mask & FILE_READ_DATA) { result += L"RD|"; mask &= ~FILE_READ_DATA; }
	if (mask & FILE_WRITE_DATA) { result += L"WD|"; mask &= ~FILE_WRITE_DATA; }
	if (mask & FILE_APPEND_DATA) { result += L"AD|"; mask &= ~FILE_APPEND_DATA; }
	if (mask & FILE_EXECUTE) { result += L"EX|"; mask &= ~FILE_EXECUTE; }
	if (mask & FILE_DELETE_CHILD) { result += L"DC|"; mask &= ~FILE_DELETE_CHILD; }

	if (mask & DELETE) { result += L"DE|"; mask &= ~DELETE; }
	if (mask & READ_CONTROL) { result += L"RC|"; mask &= ~READ_CONTROL; }
	if (mask & WRITE_DAC) { result += L"WDAC|"; mask &= ~WRITE_DAC; }
	if (mask & WRITE_OWNER) { result += L"WO|"; mask &= ~WRITE_OWNER; }
	if (mask & SYNCHRONIZE) { result += L"SY|"; mask &= ~SYNCHRONIZE; }

	if (mask) {
		result += L"0x" + uint_to_hex_str(mask, 8) + L"|";
	}

	if (!result.empty() && result.back() == L'|') {
		result.pop_back();
	}

	return result;
}

static std::wstring ace_flags_to_string(uint8_t flags) {
	std::wstring result;

	if (flags & OBJECT_INHERIT_ACE) result += L"OI|";
	if (flags & CONTAINER_INHERIT_ACE) result += L"CI|";
	if (flags & NO_PROPAGATE_INHERIT_ACE) result += L"NP|";
	if (flags & INHERIT_ONLY_ACE) result += L"IO|";
	if (flags & INHERITED_ACE) result += L"IN|";

	if (!result.empty() && result.back() == L'|') {
		result.pop_back();
	}

	return result;
}

static std::wstring decode_nt_security_descriptor(const void* data, UInt32 data_size) {
	if (!data || data_size < sizeof(SECURITY_DESCRIPTOR_RELATIVE)) {
		return L"Invalid Security Descriptor";
	}

	const uint8_t* base = static_cast<const uint8_t*>(data);
	const SECURITY_DESCRIPTOR_RELATIVE* sd = reinterpret_cast<const SECURITY_DESCRIPTOR_RELATIVE*>(data);

	if (sd->Revision != SECURITY_DESCRIPTOR_REVISION) {
		return L"Unsupported SD revision: " + std::to_wstring(sd->Revision);
	}

	uint16_t control = le16_to_host(sd->Control);
	uint32_t owner_offset = le32_to_host(sd->Owner);
	uint32_t group_offset = le32_to_host(sd->Group);
	uint32_t dacl_offset = le32_to_host(sd->Dacl);
	uint32_t sacl_offset = le32_to_host(sd->Sacl);

	std::wstring result;

	result += L"Control: ";
	if (control & SE_OWNER_DEFAULTED) result += L"OD|";
	if (control & SE_GROUP_DEFAULTED) result += L"GD|";
	if (control & SE_DACL_PRESENT) result += L"DP|";
	if (control & SE_DACL_DEFAULTED) result += L"DD|";
	if (control & SE_SACL_PRESENT) result += L"SP|";
	if (control & SE_SACL_DEFAULTED) result += L"SD|";
	if (control & SE_SELF_RELATIVE) result += L"SR|";

	if (!result.empty() && result.back() == L'|') {
		result.pop_back();
	}
	result += L",";

	if (owner_offset) {
		const SID* owner_sid = reinterpret_cast<const SID*>(base + owner_offset);
		result += L"Owner: " + sid_to_string(owner_sid) + L",";
	}

	if (group_offset) {
		const SID* group_sid = reinterpret_cast<const SID*>(base + group_offset);
		result += L"Group: " + sid_to_string(group_sid) + L",";
	}

	if ((control & SE_DACL_PRESENT) && dacl_offset) {
		const ACL* dacl = reinterpret_cast<const ACL*>(base + dacl_offset);

		uint16_t acl_size = le16_to_host(dacl->AclSize);
		uint16_t ace_count = le16_to_host(dacl->AceCount);

		if (acl_size <= data_size && dacl->AclRevision == 2) {
			result += L"DACL: " + std::to_wstring(ace_count) + L"ACEs: ";

			const uint8_t* ace_ptr = base + dacl_offset + sizeof(ACL);

			for (uint16_t i = 0; i < ace_count; i++) {
				const ACE_HEADER* ace_header = reinterpret_cast<const ACE_HEADER*>(ace_ptr);

				uint16_t ace_size = le16_to_host(ace_header->AceSize);

				if (ace_header->AceType == ACCESS_ALLOWED_ACE_TYPE) {
					const ACCESS_ALLOWED_ACE* ace = reinterpret_cast<const ACCESS_ALLOWED_ACE*>(ace_ptr);
					const SID* sid = reinterpret_cast<const SID*>(&ace->SidStart);

					uint32_t mask = le32_to_host(ace->Mask);

					result += L"Allow:" + sid_to_string(sid) + L":";
					result += L"(" + access_mask_to_string(mask) + L")";

					std::wstring flags = ace_flags_to_string(ace->Header.AceFlags);
					if (!flags.empty()) {
						result += L"[" + flags + L"]";
					}
					result += L".";
				}
				else if (ace_header->AceType == ACCESS_DENIED_ACE_TYPE) {
					const ACCESS_ALLOWED_ACE* ace = reinterpret_cast<const ACCESS_ALLOWED_ACE*>(ace_ptr);
					const SID* sid = reinterpret_cast<const SID*>(&ace->SidStart);

					uint32_t mask = le32_to_host(ace->Mask);
					result += L"Deny:" + sid_to_string(sid) + L":";
					result += L"(" + access_mask_to_string(mask) + L")";

					std::wstring flags = ace_flags_to_string(ace->Header.AceFlags);
					if (!flags.empty()) {
						result += L"[" + flags + L"]";
					}
					result += L".";
				}
				else {
					result += L"ACE type: 0x" + uint_to_hex_str(ace_header->AceType, 2) +
							 L" size:" + std::to_wstring(ace_size) + L",";
				}

				ace_ptr += ace_size;

				if (ace_ptr > base + data_size) {
					result += L"[DACL truncated] ";
					break;
				}
			}
		} else {
			result += L"DACL: invalid";
		}
	} else if (control & SE_DACL_PRESENT) {
		result += L"DACL: NULL (no access)";
	} else {
		result += L"DACL: not present (full access)";
	}

	if ((control & SE_SACL_PRESENT) && sacl_offset) {
		result += L"SACL: present ";
	}

	return result;
}

template<bool UseVirtualDestructor>
AttrList Archive<UseVirtualDestructor>::get_attr_list(UInt32 item_index)
{
	AttrList attr_list;
	if (item_index >= m_num_indices)	// fake index
		return attr_list;
	UInt32 num_props;
	CHECK_COM(in_arc->GetNumberOfProperties(&num_props));

	for (unsigned i = 0; i < num_props; i++) {
		Attr attr;
		BStr name;
		PROPID prop_id;
		VARTYPE vt;

		CHECK_COM(in_arc->GetPropertyInfo(i, name.ref(), &prop_id, &vt));
		const PropInfo *prop_info = find_prop_info(prop_id);
		if (prop_info)
			attr.name = Far::get_msg(prop_info->name_id);
		else if (name)
			attr.name.assign(name, name.size());
		else
			attr.name = int_to_str(prop_id);

		PropVariant prop;
		CHECK_COM(in_arc->GetProperty(item_index, prop_id, prop.ref()));

		if (prop_info != nullptr && prop_info->prop_to_string) {
			attr.value = prop_info->prop_to_string(prop);
		} else {
			if (prop.is_str())
				attr.value = format_str_prop(prop);
			else if (prop.is_bool())
				attr.value = Far::get_msg(prop.get_bool() ? MSG_PROPERTY_TRUE : MSG_PROPERTY_FALSE);
			else if (prop.is_uint())
				attr.value = format_uint_prop(prop);
			else if (prop.is_int())
				attr.value = format_int_prop(prop);
			else if (prop.is_filetime())
				attr.value = format_filetime_prop(prop);
		}

		if (!attr.value.empty())
			attr_list.push_back(attr);
	}

	ComObject<IArchiveGetRawProps<UseVirtualDestructor>> raw_props;
	if (in_arc->QueryInterface(IID_IArchiveGetRawProps, reinterpret_cast<void**>(&raw_props)) != S_OK || !raw_props) {
		return attr_list;
	}

	UInt32 num_raw_props;
	CHECK_COM(raw_props->GetNumRawProps(&num_raw_props));

	const auto find_and_handle_prop_info = [](PROPID const prop_id, BStr const &name, Attr &attr) {
		const auto prop_info = find_prop_info(prop_id);
		if (prop_info)
			attr.name = Far::get_msg(prop_info->name_id);
		else if (name)
			attr.name.assign(name, name.size());
		else
			attr.name = int_to_str(prop_id);

		return prop_info;
	};

	for (unsigned i = 0; i != num_raw_props; ++i) {
		Attr attr;
		BStr name;
		PROPID prop_id;
		CHECK_COM(raw_props->GetRawPropInfo(i, name.ref(), &prop_id));

		const void *data;
		UInt32 data_size;
		UInt32 prop_type;
		CHECK_COM(raw_props->GetRawProp(item_index, prop_id, &data, &data_size, &prop_type));

		if (prop_type != NPropDataType::kRaw) {
			continue;
		}

		switch (const auto prop_info = find_and_handle_prop_info(prop_id, name, attr); prop_info->prop_id) {
			case kpidNtSecure:
				attr.value = decode_nt_security_descriptor(data, data_size);
			break;
			case kpidNtReparse:
				attr.value = decode_nt_reparse_buffer(data);
			break;
			default:
				attr.value.append(std::to_wstring(data_size).append(L" ").append(Far::get_msg(MSG_SUFFIX_SIZE_B)));
			break;
		}

		attr_list.push_back(attr);
	}

	return attr_list;
}

template<bool UseVirtualDestructor>
bool Archive<UseVirtualDestructor>::get_symlink(UInt32 index, std::wstring &str) const
{
	PropVariant prop;
	if (index >= m_num_indices)
		return false;

	if (in_arc->GetProperty(index, kpidSymLink, prop.ref()) == S_OK && prop.is_str()) {
		str = prop.get_str();
		return true;
	}

	DWORD attr = 0, _posixattr = 0;
	if (in_arc->GetProperty(index, kpidAttrib, prop.ref()) == S_OK && prop.is_uint()) {
		attr = static_cast<DWORD>(prop.get_uint());
		if (attr & 0xF0000000) {
			_posixattr = (attr >> 16);
			attr &= 0x0000FFFF;
		}
		attr &= c_valid_import_attributes;
	}

	if (in_arc->GetProperty(index, kpidPosixAttrib, prop.ref()) == S_OK && prop.is_uint()) {
		_posixattr = prop.get_uint();
	}

	if (!(attr & FILE_ATTRIBUTE_REPARSE_POINT) && ((_posixattr & S_IFMT) != S_IFLNK)) {
		return false;
	}

	ComObject<IArchiveGetRawProps<UseVirtualDestructor>> raw_props;
	if (in_arc->QueryInterface(IID_IArchiveGetRawProps, reinterpret_cast<void**>(&raw_props)) != S_OK || !raw_props) {
		return false;
	}

	const void *data;
	UInt32 data_size;
	UInt32 prop_type;
	if (raw_props->GetRawProp(index, kpidNtReparse, &data, &data_size, &prop_type) != S_OK || prop_type != NPropDataType::kRaw) {
		return false;
	}

	str = decode_nt_reparse_buffer(data, true);

	return true;
}

template<bool UseVirtualDestructor>
void Archive<UseVirtualDestructor>::load_arc_attr()
{
	arc_attr.clear();

	Attr attr;
	attr.name = Far::get_msg(MSG_KPID_PATH);
	attr.value = arc_path;
	arc_attr.push_back(attr);

	UInt32 num_props;
	CHECK_COM(in_arc->GetNumberOfArchiveProperties(&num_props));
	for (unsigned i = 0; i < num_props; i++) {
		BStr name;
		PROPID prop_id;
		VARTYPE vt;
		CHECK_COM(in_arc->GetArchivePropertyInfo(i, name.ref(), &prop_id, &vt));
		const PropInfo *prop_info = find_prop_info(prop_id);
		if (prop_info)
			attr.name = Far::get_msg(prop_info->name_id);
		else if (name)
			attr.name.assign(name, name.size());
		else
			attr.name = int_to_str(prop_id);

		PropVariant prop;
		CHECK_COM(in_arc->GetArchiveProperty(prop_id, prop.ref()));

		attr.value.clear();
		if (prop_info != nullptr && prop_info->prop_to_string) {
			attr.value = prop_info->prop_to_string(prop);
		} else {
			if (prop.is_str())
				attr.value = format_str_prop(prop);
			else if (prop.is_bool())
				attr.value = Far::get_msg(prop.get_bool() ? MSG_PROPERTY_TRUE : MSG_PROPERTY_FALSE);
			else if (prop.is_uint())
				attr.value = format_uint_prop(prop);
			else if (prop.is_int())
				attr.value = format_int_prop(prop);
			else if (prop.is_filetime())
				attr.value = format_filetime_prop(prop);
		}

		if (!attr.value.empty())
			arc_attr.push_back(attr);
	}

	// compression ratio
	bool total_size_defined = true;
	UInt64 total_size = 0;
	bool total_packed_size_defined = true;
	UInt64 total_packed_size = 0;
	unsigned file_count = 0;
	PropVariant prop;
	for (UInt32 file_id = 0; file_id < m_num_indices && total_size_defined; file_id++) {
		if (!file_list[file_id].is_dir) {
			if (in_arc->GetProperty(file_id, kpidSize, prop.ref()) == S_OK && prop.is_uint())
				total_size += prop.get_uint();
			else
				total_size_defined = false;
			if (in_arc->GetProperty(file_id, kpidPackSize, prop.ref()) == S_OK && prop.is_uint())
				total_packed_size += prop.get_uint();
			else
				total_packed_size_defined = false;
			bool is_dir = in_arc->GetProperty(file_id, kpidIsDir, prop.ref()) == S_OK && prop.is_bool()
					&& prop.get_bool();
			if (!is_dir)
				++file_count;
		}
	}
	if (total_size_defined) {
		attr.name = Far::get_msg(MSG_PROPERTY_COMPRESSION_RATIO);
		auto arc_size = archive_filesize();
		unsigned ratio = total_size
				? al_round(static_cast<double>(arc_size) / static_cast<double>(total_size) * 100.0)
				: 100;
		if (ratio > 100)
			ratio = 100;
		attr.value = int_to_str(ratio) + L'%';
		arc_attr.push_back(attr);
		attr.name = Far::get_msg(MSG_PROPERTY_TOTAL_SIZE);
		attr.value = format_size_prop(total_size);
		arc_attr.push_back(attr);
	}
	if (total_packed_size_defined) {
		attr.name = Far::get_msg(MSG_PROPERTY_TOTAL_PACKED_SIZE);
		attr.value = format_size_prop(total_packed_size);
		arc_attr.push_back(attr);
	}
	attr.name = Far::get_msg(MSG_PROPERTY_FILE_COUNT);
	attr.value = int_to_str(file_count);
	arc_attr.push_back(attr);
	attr.name = Far::get_msg(MSG_PROPERTY_DIR_COUNT);
	attr.value = int_to_str(m_num_indices - file_count);
	arc_attr.push_back(attr);

	// archive files have CRC?
	m_has_crc = true;
	for (UInt32 file_id = 0; file_id < m_num_indices && m_has_crc; file_id++) {
		if (!file_list[file_id].is_dir) {
			if (in_arc->GetProperty(file_id, kpidCRC, prop.ref()) != S_OK || !prop.is_uint())
				m_has_crc = false;
		}
	}
}

template<bool UseVirtualDestructor>
void Archive<UseVirtualDestructor>::load_update_props(const ArcType &arc_type)
{
	if (m_update_props_defined)
		return;

	m_encrypted = false;
	PropVariant prop;
	for (UInt32 i = 0; i < m_num_indices; i++) {
		if (in_arc->GetProperty(i, kpidEncrypted, prop.ref()) == S_OK && prop.is_bool() && prop.get_bool()) {
			m_encrypted = true;
			break;
		}
	}

	m_solid = in_arc->GetArchiveProperty(kpidSolid, prop.ref()) == S_OK && prop.is_bool() && prop.get_bool();

	m_level = (unsigned)-1;
	m_method.clear();

	if (UInt32 NumberOfItems;
		(in_arc->GetArchiveProperty(kpidMethod, prop.ref()) == S_OK && prop.is_str()) ||
		(in_arc->GetNumberOfItems(&NumberOfItems) && NumberOfItems && in_arc->GetProperty(0, kpidMethod, prop.ref()) == S_OK && prop.is_str())
		) {

		std::list<std::wstring> m_list = split(prop.get_str(), L' ');

		static const wchar_t *known_methods[] = {c_method_lzma, c_method_lzma2, c_method_ppmd,
				c_method_deflate, c_method_deflate64};

		for (const auto &m_full_str : m_list) {
			const auto m_str = m_full_str.substr(0, m_full_str.find(L':'));

			if (StrCmpI(m_str.c_str(), c_method_copy) == 0) {
				m_level = 0;
				m_method = c_method_lzma;
				break;
			}
			for (const auto known : known_methods) {
				if (StrCmpI(m_str.c_str(), known) == 0) {
					m_method = known;
					break;
				}
			}
			for (const auto &known : g_options.codecs) {
				if (StrCmpI(m_str.c_str(), known.name.c_str()) == 0) {
					m_method = known.name;
					break;
				}
			}
			if (!m_method.empty())
				break;
		}
	}
	else if (arc_type == c_zip) {
		m_method = c_method_deflate;
	}

	if (m_level == (unsigned)-1)
		m_level = 7;	// maximum
	if (m_method.empty())
		m_method = c_method_lzma;

	m_update_props_defined = true;
}

template class Archive<true>;
template class Archive<false>;
