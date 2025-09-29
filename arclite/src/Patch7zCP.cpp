#include "headers.hpp"

#include "msg.hpp"
#include "version.hpp"
#include "guids.hpp"
#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common.hpp"
#include "archive.hpp"

// #if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__APPLE__)
#include <dlfcn.h>	  // dlopen, dlsym и dlclose
#include <iconv.h>

#if defined(__APPLE__)
#include <mach-o/loader.h>
#include <mach-o/dyld.h>
#else
#include <link.h>
#include <elf.h>
#endif

static UINT orig_oemCP = 0, orig_ansiCP = 0;
static UINT over_oemCP = 0, over_ansiCP = 0;
static bool patched_7z_dll = false;
static char cover_oemCP[32], cover_ansiCP[32];
static const char *pcorig_oemCP = nullptr, *pcorig_ansiCP = nullptr;
static void *_target_addr = nullptr;

  // locale -> code page translation tables generated from Wine source code
static const char *lcToOemTable[] = {
	"af_ZA", "CP850", "ar_SA", "CP720", "ar_LB", "CP720", "ar_EG", "CP720",
	"ar_DZ", "CP720", "ar_BH", "CP720", "ar_IQ", "CP720", "ar_JO", "CP720",
	"ar_KW", "CP720", "ar_LY", "CP720", "ar_MA", "CP720", "ar_OM", "CP720",
	"ar_QA", "CP720", "ar_SY", "CP720", "ar_TN", "CP720", "ar_AE", "CP720",
	"ar_YE", "CP720", "ast_ES", "CP850", "az_AZ", "CP866", "az_AZ", "CP857",
	"be_BY", "CP866", "bg_BG", "CP866", "br_FR", "CP850", "ca_ES", "CP850",
	"zh_CN", "CP936", "zh_TW", "CP950", "kw_GB", "CP850", "cs_CZ", "CP852",
	"cy_GB", "CP850", "da_DK", "CP850", "de_AT", "CP850", "de_LI", "CP850",
	"de_LU", "CP850", "de_CH", "CP850", "de_DE", "CP850", "el_GR", "CP737",
	"en_AU", "CP850", "en_CA", "CP850", "en_GB", "CP850", "en_IE", "CP850",
	"en_JM", "CP850", "en_BZ", "CP850", "en_PH", "CP437", "en_ZA", "CP437",
	"en_TT", "CP850", "en_US", "CP437", "en_ZW", "CP437", "en_NZ", "CP850",
	"es_PA", "CP850", "es_BO", "CP850", "es_CR", "CP850", "es_DO", "CP850",
	"es_SV", "CP850", "es_EC", "CP850", "es_GT", "CP850", "es_HN", "CP850",
	"es_NI", "CP850", "es_CL", "CP850", "es_MX", "CP850", "es_ES", "CP850",
	"es_CO", "CP850", "es_ES", "CP850", "es_PE", "CP850", "es_AR", "CP850",
	"es_PR", "CP850", "es_VE", "CP850", "es_UY", "CP850", "es_PY", "CP850",
	"et_EE", "CP775", "eu_ES", "CP850", "fa_IR", "CP720", "fi_FI", "CP850",
	"fo_FO", "CP850", "fr_FR", "CP850", "fr_BE", "CP850", "fr_CA", "CP850",
	"fr_LU", "CP850", "fr_MC", "CP850", "fr_CH", "CP850", "ga_IE", "CP437",
	"gd_GB", "CP850", "gv_IM", "CP850", "gl_ES", "CP850", "he_IL", "CP862",
	"hr_HR", "CP852", "hu_HU", "CP852", "id_ID", "CP850", "is_IS", "CP850",
	"it_IT", "CP850", "it_CH", "CP850", "iv_IV", "CP437", "ja_JP", "CP932",
	"kk_KZ", "CP866", "ko_KR", "CP949", "ky_KG", "CP866", "lt_LT", "CP775",
	"lv_LV", "CP775", "mk_MK", "CP866", "mn_MN", "CP866", "ms_BN", "CP850",
	"ms_MY", "CP850", "nl_BE", "CP850", "nl_NL", "CP850", "nl_SR", "CP850",
	"nn_NO", "CP850", "nb_NO", "CP850", "pl_PL", "CP852", "pt_BR", "CP850",
	"pt_PT", "CP850", "rm_CH", "CP850", "ro_RO", "CP852", "ru_RU", "CP866",
	"sk_SK", "CP852", "sl_SI", "CP852", "sq_AL", "CP852", "sr_RS", "CP855",
	"sr_RS", "CP852", "sv_SE", "CP850", "sv_FI", "CP850", "sw_KE", "CP437",
	"th_TH", "CP874", "tr_TR", "CP857", "tt_RU", "CP866", "uk_UA", "CP866",
	"ur_PK", "CP720", "uz_UZ", "CP866", "uz_UZ", "CP857", "vi_VN", "CP1258",
	"wa_BE", "CP850", "zh_HK", "CP950", "zh_SG", "CP936"};

static const char *lcToAnsiTable[] = {
	"af_ZA", "CP1252", "ar_SA", "CP1256", "ar_LB", "CP1256", "ar_EG", "CP1256",
	"ar_DZ", "CP1256", "ar_BH", "CP1256", "ar_IQ", "CP1256", "ar_JO", "CP1256",
	"ar_KW", "CP1256", "ar_LY", "CP1256", "ar_MA", "CP1256", "ar_OM", "CP1256",
	"ar_QA", "CP1256", "ar_SY", "CP1256", "ar_TN", "CP1256", "ar_AE", "CP1256",
	"ar_YE", "CP1256","ast_ES", "CP1252", "az_AZ", "CP1251", "az_AZ", "CP1254",
	"be_BY", "CP1251", "bg_BG", "CP1251", "br_FR", "CP1252", "ca_ES", "CP1252",
	"zh_CN", "CP936",  "zh_TW", "CP950",  "kw_GB", "CP1252", "cs_CZ", "CP1250",
	"cy_GB", "CP1252", "da_DK", "CP1252", "de_AT", "CP1252", "de_LI", "CP1252",
	"de_LU", "CP1252", "de_CH", "CP1252", "de_DE", "CP1252", "el_GR", "CP1253",
	"en_AU", "CP1252", "en_CA", "CP1252", "en_GB", "CP1252", "en_IE", "CP1252",
	"en_JM", "CP1252", "en_BZ", "CP1252", "en_PH", "CP1252", "en_ZA", "CP1252",
	"en_TT", "CP1252", "en_US", "CP1252", "en_ZW", "CP1252", "en_NZ", "CP1252",
	"es_PA", "CP1252", "es_BO", "CP1252", "es_CR", "CP1252", "es_DO", "CP1252",
	"es_SV", "CP1252", "es_EC", "CP1252", "es_GT", "CP1252", "es_HN", "CP1252",
	"es_NI", "CP1252", "es_CL", "CP1252", "es_MX", "CP1252", "es_ES", "CP1252",
	"es_CO", "CP1252", "es_ES", "CP1252", "es_PE", "CP1252", "es_AR", "CP1252",
	"es_PR", "CP1252", "es_VE", "CP1252", "es_UY", "CP1252", "es_PY", "CP1252",
	"et_EE", "CP1257", "eu_ES", "CP1252", "fa_IR", "CP1256", "fi_FI", "CP1252",
	"fo_FO", "CP1252", "fr_FR", "CP1252", "fr_BE", "CP1252", "fr_CA", "CP1252",
	"fr_LU", "CP1252", "fr_MC", "CP1252", "fr_CH", "CP1252", "ga_IE", "CP1252",
	"gd_GB", "CP1252", "gv_IM", "CP1252", "gl_ES", "CP1252", "he_IL", "CP1255",
	"hr_HR", "CP1250", "hu_HU", "CP1250", "id_ID", "CP1252", "is_IS", "CP1252",
	"it_IT", "CP1252", "it_CH", "CP1252", "iv_IV", "CP1252", "ja_JP", "CP932",
	"kk_KZ", "CP1251", "ko_KR", "CP949", "ky_KG", "CP1251", "lt_LT", "CP1257",
	"lv_LV", "CP1257", "mk_MK", "CP1251", "mn_MN", "CP1251", "ms_BN", "CP1252",
	"ms_MY", "CP1252", "nl_BE", "CP1252", "nl_NL", "CP1252", "nl_SR", "CP1252",
	"nn_NO", "CP1252", "nb_NO", "CP1252", "pl_PL", "CP1250", "pt_BR", "CP1252",
	"pt_PT", "CP1252", "rm_CH", "CP1252", "ro_RO", "CP1250", "ru_RU", "CP1251",
	"sk_SK", "CP1250", "sl_SI", "CP1250", "sq_AL", "CP1250", "sr_RS", "CP1251",
	"sr_RS", "CP1250", "sv_SE", "CP1252", "sv_FI", "CP1252", "sw_KE", "CP1252",
	"th_TH", "CP874", "tr_TR", "CP1254", "tt_RU", "CP1251", "uk_UA", "CP1251",
	"ur_PK", "CP1256", "uz_UZ", "CP1251", "uz_UZ", "CP1254", "vi_VN", "CP1258",
	"wa_BE", "CP1252", "zh_HK", "CP950", "zh_SG", "CP936"};

static void Get_AOEMCP(void)
{
	char *lc = setlocale(LC_CTYPE, "");
	if (!lc || !lc[0])
		return;

	size_t lcLen;
	// Compare up to the dot, if it exists, e.g. en_US.UTF-8
	for (lcLen = 0; lc[lcLen] != '.' && lc[lcLen] != '\0'; ++lcLen)
		;

	for (size_t i = 0; i < Z7_ARRAY_SIZE(lcToOemTable); i += 2) {
		if (strncmp(lc, lcToOemTable[i], lcLen) == 0) {
			pcorig_oemCP = lcToOemTable[i + 1];
			sscanf(pcorig_oemCP, "CP%u", &orig_oemCP);
			break;	  // Stop searching once a match is found
		}
	}
	for (size_t i = 0; i < Z7_ARRAY_SIZE(lcToAnsiTable); i += 2) {
		if (strncmp(lc, lcToAnsiTable[i], lcLen) == 0) {
			pcorig_ansiCP = lcToAnsiTable[i + 1];
			sscanf(pcorig_ansiCP, "CP%u", &orig_ansiCP);
			break;	  // Stop searching once a match is found
		}
	}

	if (!orig_oemCP) {
		orig_oemCP = 866;
	}

	if (!orig_ansiCP) {
		orig_ansiCP = 1251;
	}
}

int Patch7zCP::GetDefCP_OEM()
{
	if (orig_oemCP == 0) {
		Get_AOEMCP( );
	}
	return orig_oemCP;
}

int Patch7zCP::GetDefCP_ANSI()
{
	if (orig_ansiCP == 0) {
		Get_AOEMCP( );
	}
	return orig_ansiCP;
}

static bool is_valid_cp(UINT cp)
{
	//  CPINFO cpinfo;
	//  return cp > CP_THREAD_ACP && ::GetCPInfo(cp, &cpinfo) != FALSE;
	return TRUE;
}

#include "MyStringLite.h"

static void (*_MultiByteToUnicodeString2)(UString &dest, const AString &src, UINT codePage) = NULL;
static bool (*_ConvertUTF8ToUnicode)(const AString &src, UString &dest) = NULL;
static bool (*_Check_UTF8_Buf)(const char *src, size_t size, bool allowReduced) throw() = NULL;
static UInt32 (*_CrcCalc)(const void *data, size_t size) = NULL;
static bool (*_Convert_UTF8_Buf_To_Unicode)(const char *src, size_t srcSize, UString &dest, unsigned flags) = NULL;

#include "7z/h/C/CpuArch.h"
#include "7z/h/CPP/Common/Defs.h"
#include "7z/h/CPP/Common/MyBuffer.h"
#include "7z/h/CPP/Common/MyVector.h"
#include "ZipHeader.h"

namespace NArchive
{
namespace NZip
{

struct CVersion
{
	Byte Version;
	Byte HostOS;
};

struct CExtraSubBlock
{
	UInt32 ID;
	CByteBuffer Data;

	bool CheckIzUnicode(const AString &s) const;
};

const unsigned k_WzAesExtra_Size = 7;

struct CWzAesExtra
{
	UInt16 VendorVersion;	 // 1: AE-1, 2: AE-2,
	// UInt16 VendorId; // 'A' 'E'
	Byte Strength;	  // 1: 128-bit, 2: 192-bit, 3: 256-bit
	UInt16 Method;

	CWzAesExtra() : VendorVersion(2), Strength(3), Method(0) {}

	bool NeedCrc() const { return (VendorVersion == 1); }

	bool ParseFromSubBlock(const CExtraSubBlock &sb)
	{
		if (sb.ID != NFileHeader::NExtraID::kWzAES)
			return false;
		if (sb.Data.Size() < k_WzAesExtra_Size)
			return false;
		const Byte *p = (const Byte *)sb.Data;
		VendorVersion = GetUi16(p);
		if (p[2] != 'A' || p[3] != 'E')
			return false;
		Strength = p[4];
		// 9.31: The BUG was fixed:
		Method = GetUi16(p + 5);
		return true;
	}

	void SetSubBlock(CExtraSubBlock &sb) const
	{
		sb.Data.Alloc(k_WzAesExtra_Size);
		sb.ID = NFileHeader::NExtraID::kWzAES;
		Byte *p = (Byte *)sb.Data;
		p[0] = (Byte)VendorVersion;
		p[1] = (Byte)(VendorVersion >> 8);
		p[2] = 'A';
		p[3] = 'E';
		p[4] = Strength;
		p[5] = (Byte)Method;
		p[6] = (Byte)(Method >> 8);
	}
};

/**
namespace NStrongCrypto_AlgId
{
const UInt16 kDES = 0x6601;
const UInt16 kRC2old = 0x6602;
const UInt16 k3DES168 = 0x6603;
const UInt16 k3DES112 = 0x6609;
const UInt16 kAES128 = 0x660E;
const UInt16 kAES192 = 0x660F;
const UInt16 kAES256 = 0x6610;
const UInt16 kRC2 = 0x6702;
const UInt16 kBlowfish = 0x6720;
const UInt16 kTwofish = 0x6721;
const UInt16 kRC4 = 0x6801;
}	 // namespace NStrongCrypto_AlgId
**/

struct CStrongCryptoExtra
{
	UInt16 Format;
	UInt16 AlgId;
	UInt16 BitLen;
	UInt16 Flags;

	bool ParseFromSubBlock(const CExtraSubBlock &sb)
	{
		if (sb.ID != NFileHeader::NExtraID::kStrongEncrypt)
			return false;
		const Byte *p = (const Byte *)sb.Data;
		if (sb.Data.Size() < 8)
			return false;
		Format = GetUi16(p + 0);
		AlgId = GetUi16(p + 2);
		BitLen = GetUi16(p + 4);
		Flags = GetUi16(p + 6);
		return (Format == 2);
	}

	bool CertificateIsUsed() const { return (Flags > 0x0001); }
};

struct CExtraBlock
{
	CObjectVector<CExtraSubBlock> SubBlocks;
	bool Error;
	bool MinorError;
	bool IsZip64;
	bool IsZip64_Error;

	CExtraBlock() : Error(false), MinorError(false), IsZip64(false), IsZip64_Error(false) {}

	void Clear()
	{
		SubBlocks.Clear();
		IsZip64 = false;
	}

	size_t GetSize() const
	{
		size_t res = 0;
		FOR_VECTOR(i, SubBlocks)
		res += SubBlocks[i].Data.Size() + 2 + 2;
		return res;
	}

	bool GetWzAes(CWzAesExtra &e) const
	{
		FOR_VECTOR(i, SubBlocks)
		if (e.ParseFromSubBlock(SubBlocks[i]))
			return true;
		return false;
	}

	bool HasWzAes() const
	{
		CWzAesExtra e;
		return GetWzAes(e);
	}

	bool GetStrongCrypto(CStrongCryptoExtra &e) const
	{
		FOR_VECTOR(i, SubBlocks)
		if (e.ParseFromSubBlock(SubBlocks[i]))
			return true;
		return false;
	}

	//  bool GetNtfsTime(unsigned index, FILETIME &ft) const;
	//  bool GetUnixTime(bool isCentral, unsigned index, UInt32 &res) const;

	//  void PrintInfo(AString &s) const;

	void RemoveUnknownSubBlocks()
	{
		for (unsigned i = SubBlocks.Size(); i != 0;) {
			i--;
			switch (SubBlocks[i].ID) {
				case NFileHeader::NExtraID::kStrongEncrypt:
				case NFileHeader::NExtraID::kWzAES:
					break;
				default:
					SubBlocks.Delete(i);
			}
		}
	}
};

bool CExtraSubBlock::CheckIzUnicode(const AString &s) const
{
	size_t size = Data.Size();
	if (size < 1 + 4)
		return false;
	const Byte *p = (const Byte *)Data;
	if (p[0] > 1)
		return false;
	if (_CrcCalc(s, s.Len()) != GetUi32(p + 1))
		return false;
	size -= 5;
	p += 5;
	for (size_t i = 0; i < size; i++)
		if (p[i] == 0)
			return false;
	return _Check_UTF8_Buf((const char *)(const void *)p, size, false);
}

class CLocalItem
{
public:
	UInt16 Flags;
	UInt16 Method;

	/*
	  Zip specification doesn't mention that ExtractVersion field uses HostOS subfield.
	  18.06: 7-Zip now doesn't use ExtractVersion::HostOS to detect codePage
	*/

	CVersion ExtractVersion;

	UInt64 Size;
	UInt64 PackSize;
	UInt32 Time;
	UInt32 Crc;

	UInt32 Disk;

	AString Name;

	CExtraBlock LocalExtra;

	unsigned GetDescriptorSize() const
	{
		return LocalExtra.IsZip64 ? kDataDescriptorSize64 : kDataDescriptorSize32;
	}

	UInt64 GetPackSizeWithDescriptor() const
	{
		return PackSize + (HasDescriptor() ? GetDescriptorSize() : 0);
	}

	bool IsUtf8() const { return (Flags & NFileHeader::NFlags::kUtf8) != 0; }
	bool IsEncrypted() const { return (Flags & NFileHeader::NFlags::kEncrypted) != 0; }
	bool IsStrongEncrypted() const
	{
		return IsEncrypted() && (Flags & NFileHeader::NFlags::kStrongEncrypted) != 0;
	}
	bool IsAesEncrypted() const
	{
		return IsEncrypted() && (IsStrongEncrypted() || Method == NFileHeader::NCompressionMethod::kWzAES);
	}
	bool IsLzmaEOS() const { return (Flags & NFileHeader::NFlags::kLzmaEOS) != 0; }
	bool HasDescriptor() const { return (Flags & NFileHeader::NFlags::kDescriptorUsedMask) != 0; }
	// bool IsAltStream() const { return (Flags & NFileHeader::NFlags::kAltStream) != 0; }

	unsigned GetDeflateLevel() const { return (Flags >> 1) & 3; }

	//  bool IsDir() const;

private:
	void SetFlag(unsigned bitMask, bool enable)
	{
		if (enable)
			Flags = (UInt16)(Flags | bitMask);
		else
			Flags = (UInt16)(Flags & ~bitMask);
	}

public:
	void ClearFlags() { Flags = 0; }
	void SetEncrypted(bool encrypted) { SetFlag(NFileHeader::NFlags::kEncrypted, encrypted); }
	void SetUtf8(bool isUtf8) { SetFlag(NFileHeader::NFlags::kUtf8, isUtf8); }
	// void SetFlag_AltStream(bool isAltStream) { SetFlag(NFileHeader::NFlags::kAltStream, isAltStream); }
	void SetDescriptorMode(bool useDescriptor)
	{
		SetFlag(NFileHeader::NFlags::kDescriptorUsedMask, useDescriptor);
	}

	UINT GetCodePage() const
	{
		if (IsUtf8())
			return CP_UTF8;
		return CP_OEMCP;
	}
};

class CItem : public CLocalItem
{
public:
	CVersion MadeByVersion;
	UInt16 InternalAttrib;
	UInt32 ExternalAttrib;

	UInt64 LocalHeaderPos;

	CExtraBlock CentralExtra;
	CByteBuffer Comment;

	bool FromLocal;
	bool FromCentral;

	// CItem can be used as CLocalItem. So we must clear unused fields
	CItem() : InternalAttrib(0), ExternalAttrib(0), FromLocal(false), FromCentral(false)
	{
		MadeByVersion.Version = 0;
		MadeByVersion.HostOS = 0;
	}

	const CExtraBlock &GetMainExtra() const { return *(FromCentral ? &CentralExtra : &LocalExtra); }

	//  bool IsDir() const;
	UInt32 GetWinAttrib() const;
	bool GetPosixAttrib(UInt32 &attrib) const;

	// 18.06: 0 instead of ExtractVersion.HostOS for local item
	Byte GetHostOS() const { return FromCentral ? MadeByVersion.HostOS : (Byte)0; }

	void GetUnicodeString(UString &res, const AString &s, bool isComment, bool useSpecifiedCodePage,
			UINT codePage) const;

	bool Is_MadeBy_Unix() const
	{
		if (!FromCentral)
			return false;
		return (MadeByVersion.HostOS == NFileHeader::NHostOS::kUnix);
	}

	UINT GetCodePage() const
	{
		// 18.06: now we use HostOS only from Central::MadeByVersion
		if (IsUtf8())
			return CP_UTF8;
		if (!FromCentral)
			return CP_OEMCP;
		Byte hostOS = MadeByVersion.HostOS;
		return (UINT)((hostOS == NFileHeader::NHostOS::kFAT || hostOS == NFileHeader::NHostOS::kNTFS
							  || hostOS == NFileHeader::NHostOS::kUnix	  // do we need it?
							  )
						? CP_OEMCP
						: CP_ACP);
	}
};

FAR_ALIGNED(16) void CItem::GetUnicodeString(UString &res, const AString &s, bool isComment, bool useSpecifiedCodePage,
		UINT codePage) const
{
	bool isUtf8 = IsUtf8();

	if (!isUtf8) {
		{
			const unsigned id = isComment
					? NFileHeader::NExtraID::kIzUnicodeComment
					: NFileHeader::NExtraID::kIzUnicodeName;
			const CObjectVector<CExtraSubBlock> &subBlocks = GetMainExtra().SubBlocks;

			FOR_VECTOR(i, subBlocks)
			{
				const CExtraSubBlock &sb = subBlocks[i];
				if (sb.ID == id) {
					if (sb.CheckIzUnicode(s)) {
						// const unsigned kIzUnicodeHeaderSize = 5;
						if (_Convert_UTF8_Buf_To_Unicode((const char *)(const void *)(const Byte *)sb.Data
											+ 5,
									sb.Data.Size() - 5, res, 0))
							return;
					}
					break;
				}
			}
		}
		if (useSpecifiedCodePage) {
			isUtf8 = (codePage == CP_UTF8);
		}
#ifdef _WIN32
		else if (GetHostOS() == NFileHeader::NHostOS::kUnix) {
			/* Some ZIP archives in Unix use UTF-8 encoding without Utf8 flag in header.
			   We try to get name as UTF-8.
			   Do we need to do it in POSIX version also? */
			isUtf8 = true;

			/* 21.02: we want to ignore UTF-8 errors to support file paths that are mixed
			  of UTF-8 and non-UTF-8 characters. */
			// ignore_Utf8_Errors = false;
			// ignore_Utf8_Errors = true;
		}
#endif
	}

#if 1

	bool isAnsi = false;
	bool isOem = false;

	if (!isUtf8 && MadeByVersion.HostOS == NFileHeader::NHostOS::kNTFS && MadeByVersion.Version >= 20) {
		isAnsi = true;
	} else if (!isUtf8
			&& (MadeByVersion.HostOS == NFileHeader::NHostOS::kNTFS
					|| MadeByVersion.HostOS == NFileHeader::NHostOS::kFAT)) {
		isOem = true;
	}

	if (isOem || isAnsi) {

		#if 0 // Use iconv
		const char *legacyCp = nullptr;
		if (isOem) {
			if (over_oemCP)
				legacyCp = cover_oemCP;
			else
				legacyCp = pcorig_oemCP;
		} else if (isAnsi) {
			if (over_ansiCP)
				legacyCp = cover_ansiCP;
			else
				legacyCp = pcorig_ansiCP;
		}
		// fprintf(stderr, "legacyCP = %s\n", legacyCp ? legacyCp : "none");
		if (legacyCp) {
			iconv_t cd;
			if ((cd = iconv_open("UTF-8", legacyCp)) != (iconv_t)-1) {

				size_t slen = s.Len();
				// size_t dlen = 256;
				size_t dlen = slen * 4;
				AString s_utf8(dlen);

				char *srcPtr = (char *)s.Ptr();	   // iconv requires non-const input pointer
				char *destPtr = (char *)s_utf8.Ptr();

				size_t done = iconv(cd, &srcPtr, &slen, &destPtr, &dlen);
				if (done != (size_t)-1) {
					size_t rezlen = destPtr - s_utf8.Ptr();
					s_utf8.SetLen(rezlen);

					iconv_close(cd);
					if (_ConvertUTF8ToUnicode(s_utf8, res)) {
						return;
					}
				}
			}
		}
		#else // Use far2l WINPORT
		UINT uilegacyCp = 0;
		if (isOem) {
			if (over_oemCP)
				uilegacyCp = over_oemCP;
			else
				uilegacyCp = orig_oemCP;
		} else if (isAnsi) {
			if (over_ansiCP)
				uilegacyCp = over_ansiCP;
			else
				uilegacyCp = orig_ansiCP;
		}

		if (uilegacyCp) {
			size_t slen = s.Len();
//			// size_t dlen = 256;
			size_t dlen = slen * 4 + 1;
			res.SetNewSize( dlen );
			char *srcPtr = (char *)s.Ptr();	   // iconv requires non-const input pointer
			wchar_t *destPtr = (wchar_t *)res.Ptr();

			int r = WINPORT(MultiByteToWideChar)(uilegacyCp, 0, srcPtr, slen, destPtr, dlen - 1);
			if (r != 0) {
				res.SetLen(r);
				return;
			}
		}
		#endif
	}	 // if (isOem || isAnsi) {

#endif
	if (isUtf8) {
		_ConvertUTF8ToUnicode(s, res);
		return;
	}
	_MultiByteToUnicodeString2(res, s, useSpecifiedCodePage ? codePage : GetCodePage());
}

}	 // namespace NZip
}	 // namespace NArchive


#if defined(__APPLE__)
//void **find_plt_entry_for_symbol(struct link_map *map, void *target_addr)
//{
//	return NULL;
//}
#else
void **find_plt_entry_for_symbol(struct link_map *map, void *target_addr)
{
//	ElfW(Dyn) *dynamic = map->l_ld;
	ElfW(Dyn) *dynamic = (ElfW(Dyn)*)map->l_ld;

	ElfW(Addr) symtab_addr = 0;
	ElfW(Addr) strtab_addr = 0;
	ElfW(Addr) pltgot_addr = 0;

	ElfW(Xword) strsz = 0;
	ElfW(Xword) syment = 0;

	if (!dynamic)
		return NULL;

	for (; dynamic->d_tag != DT_NULL; dynamic++) {
		switch (dynamic->d_tag) {
			case DT_SYMTAB:
				symtab_addr = dynamic->d_un.d_ptr;
				break;
			case DT_STRTAB:
				strtab_addr = dynamic->d_un.d_ptr;
				break;
			case DT_STRSZ:
				strsz = dynamic->d_un.d_val;
				break;
			case DT_SYMENT:
				syment = dynamic->d_un.d_val;
				break;
			case DT_PLTGOT:
				pltgot_addr = dynamic->d_un.d_val;
				break;
		}
	}

	if (!symtab_addr || !strtab_addr || !pltgot_addr) {
		fprintf(stderr, "Error: Cannot find required dynamic tags in library %s\n", map->l_name);
		return NULL;
	}

//	fprintf(stderr, "strsz = %lu:\n", strsz);
//	fprintf(stderr, "syment = %lu:\n", syment);
//	fprintf(stderr, "sizeof(ElfW(Sym)) = %lu:\n", sizeof(ElfW(Sym)));

	//	ElfW(Sym) *symtab = (ElfW(Sym) *) (map->l_addr + symtab_addr);
	//	char *strtab = (char *) (map->l_addr + strtab_addr);
	//	void **got_plt = (void **) (map->l_addr + pltgot_addr);
	ElfW(Sym) *symtab = (ElfW(Sym) *)(symtab_addr);
	char *strtab = (char *)(strtab_addr);
	void **got_plt = (void **)(pltgot_addr);

	(void)symtab;
	(void)strtab;
	(void)strsz;
	(void)syment;

	FILE *maps = fopen("/proc/self/maps", "r");
	if (!maps) {
		perror("fopen /proc/self/maps");
		return NULL;
	}

	char line[512];
	ElfW(Addr) got_start = pltgot_addr;
	ElfW(Addr) got_end = 0;

	while (fgets(line, sizeof(line), maps)) {
		unsigned long start, end;
		char perm[5], path[256];

		if (sscanf(line, "%lx-%lx %4s %*x %*x:%*x %*d %255s", &start, &end, perm, path) == 4) {
			if (start <= got_start && got_start < end) {
				got_end = end;
				break;
			}
		}
	}
    fclose(maps);

    size_t got_size = got_end - got_start;
//    fprintf(stderr, "gplt size: %lu bytes\n", got_size);
    size_t num_got_entries = got_size / sizeof(void *);
//    fprintf(stderr, "num_got_entries = %lu\n", num_got_entries);

	if (num_got_entries > 0) {
		void **found_entry = NULL;
		for (size_t i = 0; i < num_got_entries; i++) {
			void **got_entry = got_plt + i;
			if (got_entry && *got_entry == target_addr) {
				found_entry = got_entry;
				return found_entry;
			}
		}
//		return found_entry;
	}

	return NULL;
}
#endif

bool get_faddrs(void *handle)
{
	_MultiByteToUnicodeString2 = (void (*)(UString &dest, const AString &src, UINT codePage))dlsym(handle,
			"_Z25MultiByteToUnicodeString2R7UStringRK7AStringj");
	_ConvertUTF8ToUnicode = (bool (*)(const AString &src, UString &dest))dlsym(handle,
			"_Z20ConvertUTF8ToUnicodeRK7AStringR7UString");
	_Check_UTF8_Buf = (bool (*)(const char *src, size_t size, bool allowReduced) throw())dlsym(handle,
			"_Z14Check_UTF8_BufPKcmb");
	_CrcCalc = (UInt32(*)(const void *data, size_t size))dlsym(handle, "CrcCalc");
	_Convert_UTF8_Buf_To_Unicode = (bool (*)(const char *src, size_t srcSize, UString &dest,
			unsigned flags))dlsym(handle, "_Z27Convert_UTF8_Buf_To_UnicodePKcmR7UStringj");

	_target_addr = dlsym(handle, "_ZNK8NArchive4NZip5CItem16GetUnicodeStringER7UStringRK7AStringbbj");

	fprintf(stderr, "_MultiByteToUnicodeString2 = %p\n", _MultiByteToUnicodeString2);
	fprintf(stderr, "_ConvertUTF8ToUnicode = %p\n", _ConvertUTF8ToUnicode);
	fprintf(stderr, "_Check_UTF8_Buf = %p\n", _Check_UTF8_Buf);
	fprintf(stderr, "_CrcCalc = %p\n", _CrcCalc);
	fprintf(stderr, "_Convert_UTF8_Buf_To_Unicode = %p\n", _Convert_UTF8_Buf_To_Unicode);

	fprintf(stderr, "&NArchive::NZip::CItem::GetUnicodeString = %p\n", _target_addr);

	if (!_MultiByteToUnicodeString2 || !_ConvertUTF8ToUnicode || !_Check_UTF8_Buf || !_CrcCalc
			|| !_Convert_UTF8_Buf_To_Unicode || !_target_addr) {

		fprintf(stderr, "get_faddrs() failed!!!\n");
		return false;
	}

	fprintf(stderr, "get_faddrs() ok\n");

	return true;
}

static bool patch_plt(void *handle)
{
#if defined(__APPLE__) || defined(__UCLIBC__)
	return false;
#else
	struct link_map *map;
	if (dlinfo(handle, RTLD_DI_LINKMAP, &map) != 0) {
		perror("dlinfo failed");
		return false;
	}

	void **got_entry = find_plt_entry_for_symbol(map, _target_addr);
	if (!got_entry) {
		fprintf(stderr, "Error: Cannot find plt entry for %p\n", _target_addr);
		return false;
	}

	void *page = (void *)((uintptr_t)got_entry & ~(getpagesize() - 1));
	if (mprotect(page, getpagesize(), PROT_READ | PROT_WRITE) == -1) {
		fprintf(stderr, "Error: mprotect failed\n");
		return false;
	}

	union {
		void (NArchive::NZip::CItem::*methodPtr)(UString&, const AString&, bool, bool, UINT) const = &NArchive::NZip::CItem::GetUnicodeString;
		void *fptr;
	} u;

	fprintf(stderr, "got_entry = %p\n", got_entry);

	*got_entry = (void *)u.fptr;
	mprotect(page, getpagesize(), PROT_READ);

	return true;
#endif
}

static bool patch_addr(void *handle)
{
	void *func_addr = _target_addr;
	fprintf(stderr, "patch_addr() at %p\n", func_addr);

	union {
		void (NArchive::NZip::CItem::*methodPtr)(UString&, const AString&, bool, bool, UINT) const = &NArchive::NZip::CItem::GetUnicodeString;
		void *fptr;
	} u;

	void *newf_addr = u.fptr;

	fprintf(stderr, "func_addr = %p\n", func_addr );
	fprintf(stderr, "newf_addr = %p\n", newf_addr );

	uintptr_t pagesize = sysconf(_SC_PAGESIZE);

	void *page = (void *)((uintptr_t)func_addr & ~(pagesize - 1));
	if (mprotect(page, pagesize, PROT_READ | PROT_WRITE | PROT_EXEC) == -1) {
		fprintf(stderr, "Error: mprotect failed\n");
		return false;
	}

#ifdef __i386__
	intptr_t offset = (intptr_t)newf_addr - ((intptr_t)func_addr + 5);

	unsigned char jmp_instruction[5] = {0xE9, 0x00, 0x00, 0x00, 0x00}; // jmp rel addr
	memcpy(jmp_instruction + 1, &offset, sizeof(intptr_t));
	memcpy((void *)func_addr, jmp_instruction, sizeof(jmp_instruction));

#elif defined(__arm__)
	intptr_t offset = ((intptr_t)newf_addr - ((intptr_t)func_addr + 8)) / 4;
	if (offset > 0x7FFFFF || offset < -0x800000) {
		return false;
	}
	uint32_t instruction = 0xEA000000 | (offset & 0x00FFFFFF);
	memcpy(func_addr, &instruction, sizeof(instruction));

#elif defined(__x86_64__)
	unsigned char patch_code[12] = {
		0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rax, imm64
		0xFF, 0xE0                                                  // jmp rax
	};

	memcpy(patch_code + 2, &newf_addr, sizeof(void *));
	memcpy((void *)func_addr, patch_code, sizeof(patch_code));

#elif defined(__aarch64__)
    uintptr_t addr = (uintptr_t)newf_addr;

    unsigned char patch_code[20] = {
		0x58, 0x00, 0x02, 0xD5, // movz x16, #imm16 << 0
		0x92, 0x00, 0x42, 0xF2, // movk x16, #imm16 << 16
		0xd2, 0x00, 0x82, 0xf2, // movk x16, #imm16 << 32
		0x12, 0x00, 0xc2, 0xf2, // movk x16, #imm16 << 48
		0x00, 0x00, 0x00, 0xD6  // br x16
    };

	patch_code[2] = (addr >> 0) & 0xFF;
	patch_code[3] = ((addr >> 8) & 0xFF) | 0x02;

	patch_code[6] = (addr >> 16) & 0xFF;
	patch_code[7] = ((addr >> 24) & 0xFF) | 0x42;

	patch_code[10] = (addr >> 32) & 0xFF;
	patch_code[11] = ((addr >> 40) & 0xFF) | 0x82;

	patch_code[14] = (addr >> 48) & 0xFF;
	patch_code[15] = ((addr >> 56) & 0xFF) | 0xC2;

	memcpy(func_addr, patch_code, sizeof(patch_code));
#else

#endif
	return true;
}

static bool patch_7z_dll()
{
	const ArcLibs &libs = ArcAPI::libs();
	if (libs.empty())
		return false;

	for (size_t i = 0; i < libs.size(); ++i) {
		if (!get_faddrs(libs[i].h_module))
			continue;
//		if (patch_plt(libs[i].h_module)) // You might crash.
//			return true;
		if (patch_addr(libs[i].h_module))
			return true;
	}

	return false;
}

void Patch7zCP::SetCP(UINT oemCP, UINT ansiCP, bool bRePatch)
{
	Get_AOEMCP();

	over_oemCP = oemCP;
	over_ansiCP = ansiCP;
	snprintf(cover_oemCP, 32, "CP%u", over_oemCP);
	snprintf(cover_ansiCP, 32, "CP%u", over_ansiCP);

	if (!patched_7z_dll || bRePatch) {
		if (patch_7z_dll()) {
//			patched_7z_dll = true;
		}
		patched_7z_dll = true;
	}
}

// #############################################################################
// #############################################################################
