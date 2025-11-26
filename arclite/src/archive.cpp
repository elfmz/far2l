#include "headers.hpp"

#include "msg.hpp"
#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common.hpp"
#include "ui.hpp"
#include "archive.hpp"
#include "options.hpp"

#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__APPLE__)
#include <sys/types.h>
#else
#include <sys/sysmacros.h>	  // major / minor
#endif

const wchar_t *c_method_default = L"Default";
const wchar_t *c_method_copy = L"Copy";
const wchar_t *c_method_lzma = L"LZMA";
const wchar_t *c_method_lzma2 = L"LZMA2";
const wchar_t *c_method_ppmd = L"PPMD";
const wchar_t *c_method_deflate = L"Deflate";
const wchar_t *c_method_deflate64 = L"Deflate64";
const wchar_t *c_method_bzip2 = L"BZip2";
const wchar_t *c_method_delta = L"Delta";
const wchar_t *c_method_bcj = L"BCJ";
const wchar_t *c_method_bcj2 = L"BCJ2";

const wchar_t *c_tar_method_gnu = L"gnu";
const wchar_t *c_tar_method_pax = L"pax";
const wchar_t *c_tar_method_posix = L"posix";

#if IS_BIG_ENDIAN
#define DEFINE_ARC_ID(name, v)																				\
	static constexpr unsigned char c_guid_##name[] =														\
			"\x23\x17\x0F\x69\x40\xC1\x27\x8A\x10\x00\x00\x01\x10" v "\x00\x00";							\
	const ArcType c_##name(c_guid_##name, c_guid_##name + 16);
#else
#define DEFINE_ARC_ID(name, v)																				\
	static constexpr unsigned char c_guid_##name[] =														\
			"\x69\x0F\x17\x23\xC1\x40\x8A\x27\x10\x00\x00\x01\x10" v "\x00\x00";							\
	const ArcType c_##name(c_guid_##name, c_guid_##name + 16);
#endif

DEFINE_ARC_ID(7z, "\x07")
DEFINE_ARC_ID(zip, "\x01")
DEFINE_ARC_ID(bzip2, "\x02")
DEFINE_ARC_ID(gzip, "\xEF")
DEFINE_ARC_ID(xz, "\x0C")

DEFINE_ARC_ID(iso, "\xE7")
DEFINE_ARC_ID(udf, "\xE0")
DEFINE_ARC_ID(rar, "\x03")
DEFINE_ARC_ID(rar5, "\xCC")
DEFINE_ARC_ID(arj, "\x04")
DEFINE_ARC_ID(lzh, "\x06")
DEFINE_ARC_ID(cab, "\x08")
DEFINE_ARC_ID(z, "\x05")
DEFINE_ARC_ID(lz4, "\x0f")
DEFINE_ARC_ID(lz5, "\x10")
DEFINE_ARC_ID(lizard, "\x11")

DEFINE_ARC_ID(split, "\xEA")
DEFINE_ARC_ID(wim, "\xE6")
DEFINE_ARC_ID(tar, "\xEE")
DEFINE_ARC_ID(xar, "\xE1")
DEFINE_ARC_ID(ar, "\xEC")
DEFINE_ARC_ID(rpm, "\xEB")
DEFINE_ARC_ID(cpio, "\xED")
DEFINE_ARC_ID(SWFc, "\xD8")
DEFINE_ARC_ID(dmg, "\xE4")
DEFINE_ARC_ID(hfs, "\xE3")
DEFINE_ARC_ID(zstd, "\x0E")
DEFINE_ARC_ID(lzma, "\x0A")
DEFINE_ARC_ID(lzma86, "\x0B")
DEFINE_ARC_ID(elf, "\xDE")
DEFINE_ARC_ID(pe, "\xCF")
DEFINE_ARC_ID(macho, "\xDF")
DEFINE_ARC_ID(chm, "\xCE")
DEFINE_ARC_ID(compound, "\xE5")
DEFINE_ARC_ID(vdi, "\xC9")
DEFINE_ARC_ID(vhd, "\xDC")
DEFINE_ARC_ID(vmdk, "\xC8")
DEFINE_ARC_ID(vdx, "\xC4")
DEFINE_ARC_ID(qcow, "\xCA")
DEFINE_ARC_ID(swf, "\xD7")
DEFINE_ARC_ID(flv, "\xD6")
DEFINE_ARC_ID(lp, "\xC1")
DEFINE_ARC_ID(ihex, "\xCD")
DEFINE_ARC_ID(mslz, "\xD5")
DEFINE_ARC_ID(nud, "\xE2")
DEFINE_ARC_ID(uefif, "\xD1")
DEFINE_ARC_ID(uefic, "\xD0")
DEFINE_ARC_ID(avb, "\xC0")
DEFINE_ARC_ID(Base64, "\xC5")
DEFINE_ARC_ID(CramFS, "\xD3")
DEFINE_ARC_ID(LVM, "\xBF")
DEFINE_ARC_ID(Mub, "\xE2")
DEFINE_ARC_ID(Ppmd, "\x0D")
DEFINE_ARC_ID(Sparse, "\xC2")

DEFINE_ARC_ID(fat, "\xDA")	   // FAT
DEFINE_ARC_ID(ntfs, "\xD9")	   // NTFS
DEFINE_ARC_ID(ext4, "\xC7")	   // Ext[234]
DEFINE_ARC_ID(apfs, "\xC3")	   // APFS
DEFINE_ARC_ID(uefi, "\xD1") // UEFi
DEFINE_ARC_ID(sqfs, "\xD2") // SquashFS

DEFINE_ARC_ID(mbr, "\xDB")	  // MBR
DEFINE_ARC_ID(gpt, "\xCB")	  // GPT
DEFINE_ARC_ID(apm, "\xD4")	  // APM

#undef DEFINE_ARC_ID

// https://www.tc4shell.com/en/7zip/exfat7z/
static constexpr unsigned char c_guid_xfat[] =
		"\x1e\x7e\xa5\x3e\xcb\xdd\xf0\x45\x9a\xe2\x2e\xf7\xf3\x98\x42\x53";
const ArcType c_xfat(c_guid_xfat, c_guid_xfat + 16);

const UInt64 c_min_volume_size = 16 * 1024;

const wchar_t *c_sfx_ext = L".exe";
const wchar_t *c_volume_ext = L".001";

namespace ArchiveGlobals {
	unsigned max_check_size = 0;
}

HRESULT ArcLib::get_prop(UInt32 index, PROPID prop_id, PROPVARIANT *prop) const
{
	if (GetHandlerProperty2) {
		return GetHandlerProperty2(index, prop_id, prop);
	} else {
		assert(index == 0);
		return GetHandlerProperty(prop_id, prop);
	}
}

HRESULT ArcLib::get_bool_prop(UInt32 index, PROPID prop_id, bool &value) const
{
	PropVariant prop;
	HRESULT res = get_prop(index, prop_id, prop.ref());
	if (res != S_OK)
		return res;
	if (!prop.is_bool())
		return E_FAIL;
	value = prop.get_bool();
	return S_OK;
}

HRESULT ArcLib::get_uint_prop(UInt32 index, PROPID prop_id, UInt32 &value) const
{
	PropVariant prop;
	HRESULT res = get_prop(index, prop_id, prop.ref());
	if (res != S_OK)
		return res;
	if (!prop.is_uint())
		return E_FAIL;
	value = (UInt32)prop.get_uint();
	return S_OK;
}

HRESULT ArcLib::get_string_prop(UInt32 index, PROPID prop_id, std::wstring &value) const
{
	PropVariant prop;
	HRESULT res = get_prop(index, prop_id, prop.ref());
	if (res != S_OK)
		return res;
	if (!prop.is_str())
		return E_FAIL;
	value = prop.get_str();
	return S_OK;
}

HRESULT ArcLib::get_bytes_prop(UInt32 index, PROPID prop_id, ByteVector &value) const
{
	PropVariant prop;
	HRESULT res = get_prop(index, prop_id, prop.ref());

	if (res != S_OK) {
		return res;
	}

	if (prop.vt == VT_BSTR) {
		UINT len = SysStringByteLen(prop.bstrVal);
		unsigned char *data = reinterpret_cast<unsigned char *>(prop.bstrVal);
		value.assign(data, data + len);
	} else {
		return E_FAIL;
	}

	return S_OK;
}

std::wstring ArcFormat::default_extension() const
{
	if (extension_list.empty())
		return std::wstring();
	else
		return extension_list.front();
}

ArcTypes ArcFormats::get_arc_types() const
{
	ArcTypes types;
#if 1
	for (const auto &f : *this) {
		types.push_back(f.first);
	}
#else
	for (const auto &[type, fmt] : *this) {
		types.push_back(type);
	}
#endif
	return types;
}

ArcTypes ArcFormats::find_by_name(const std::wstring &name) const
{
	ArcTypes types;
	std::wstring uc_name = upcase(name);
#if 1
	for (const auto &f : *this) {
		if (upcase(f.second.name) == uc_name)
			types.push_back(f.first);
	}
#else
	for (const auto &[type, fmt] : *this) {
		if (upcase(fmt.name) == uc_name)
			types.push_back(type);
	}
#endif
	return types;
}

ArcTypes ArcFormats::find_by_ext(const std::wstring &ext) const
{
	ArcTypes types;
	if (ext.empty())
		return types;
	std::wstring uc_ext = upcase(ext);
#if 1
	for (const auto &f : *this) {
		for (const auto &e : f.second.extension_list) {
			if (upcase(e) == uc_ext) {
				types.push_back(f.first);
				break;
			}
		}
	}
#else
	for (const auto &[type, fmt] : *this) {
		for (const auto &e : fmt.extension_list) {
			if (upcase(e) == uc_ext) {
				types.push_back(type);
				break;
			}
		}
	}
#endif
	return types;
}

uintptr_t SfxModules::find_by_name(const std::wstring &name) const
{
	for (const_iterator sfx_module = begin(); sfx_module != end(); sfx_module++) {
		if (upcase(extract_file_name(sfx_module->path)) == upcase(name))
			return distance(begin(), sfx_module);
	}
	return size();
}

std::wstring ArcChain::to_string() const
{
	std::wstring result;
	std::for_each(begin(), end(), [&](const ArcEntry &arc) {
		if (!result.empty())
			result += L"→";
		result += ArcAPI::formats().at(arc.type).name;
		if (arc.flags & 1)
			result += L"@";
	});
	return result;
}

static bool GetCoderInfo(Func_GetMethodProperty getMethodProperty, UInt32 index, CDllCodecInfo &info)
{
	info.DecoderIsAssigned = info.EncoderIsAssigned = false;
	std::fill((char *)&info.Decoder, sizeof(info.Decoder) + (char *)&info.Decoder, 0);
	info.Encoder = info.Decoder;
	PropVariant prop1, prop2, prop3, prop4;

	if (S_OK != getMethodProperty(index, NMethodPropID::kDecoder, prop1.ref()))
		return false;

	if (prop1.vt != VT_EMPTY) {
		if (prop1.vt != VT_BSTR || (size_t)SysStringByteLen(prop1.bstrVal) < sizeof(CLSID))
			return false;
		info.Decoder = *reinterpret_cast<const GUID *>(prop1.bstrVal);
		info.DecoderIsAssigned = true;
	}

	if (S_OK != getMethodProperty(index, NMethodPropID::kEncoder, prop2.ref()))
		return false;

	if (prop2.vt != VT_EMPTY) {
		if (prop2.vt != VT_BSTR || (size_t)SysStringByteLen(prop2.bstrVal) < sizeof(CLSID))
			return false;
		info.Encoder = *reinterpret_cast<const GUID *>(prop2.bstrVal);
		info.EncoderIsAssigned = true;
	}

	if (S_OK != getMethodProperty(index, NMethodPropID::kName, prop3.ref()) || !prop3.is_str())
		return false;

	info.Name = prop3.get_str();
	if (S_OK != getMethodProperty(index, NMethodPropID::kID, prop4.ref()) || !prop4.is_uint())
		return false;

	info.CodecId = static_cast<UInt32>(prop4.get_uint());

	return info.DecoderIsAssigned || info.EncoderIsAssigned;
}

template<bool UseVirtualDestructor>
class MyCompressCodecsInfo : public ICompressCodecsInfo<UseVirtualDestructor>,
							 public IHashers<UseVirtualDestructor>, private ComBase<UseVirtualDestructor>
{
private:
	const ArcLibs &libs_;
	ArcCodecs codecs_;
	ArcHashers hashers_;

public:
	MyCompressCodecsInfo(const ArcLibs &libs, const ArcCodecs &codecs, const ArcHashers &hashers,
			size_t skip_lib_index)
		: libs_(libs)
	{
		for (size_t ilib = 0; ilib < 1; ++ilib) {	 // 7z.dll only
			if (ilib == skip_lib_index) {
				continue;
			}
			const auto &arc_lib = libs[ilib];

			if ((arc_lib.CreateObject || arc_lib.CreateDecoder || arc_lib.CreateEncoder)
					&& arc_lib.GetMethodProperty) {
				UInt32 numMethods = 1;
				bool ok = true;
				if (arc_lib.GetNumberOfMethods)
					ok = S_OK == arc_lib.GetNumberOfMethods(&numMethods);

				for (UInt32 i = 0; ok && i < numMethods; ++i) {
					CDllCodecInfo info;
					info.LibIndex = static_cast<UInt32>(ilib);
					info.CodecIndex = i;
					if (GetCoderInfo(arc_lib.GetMethodProperty, i, info)) {
						codecs_.push_back(info);
					}
				}
			}

			if (arc_lib.ComHashers) {
				IHashers<UseVirtualDestructor> *_ComHashers = static_cast<IHashers<UseVirtualDestructor>*>(arc_lib.ComHashers);
				UInt32 numHashers = _ComHashers->GetNumHashers();
				for (UInt32 i = 0; i < numHashers; ++i) {
					CDllHasherInfo info;
					info.LibIndex = static_cast<UInt32>(ilib);
					info.HasherIndex = i;
					hashers_.push_back(info);
				}
			}
		}

		for (const auto &codec : codecs) {
			if (codec.LibIndex != skip_lib_index)
				codecs_.push_back(codec);
		}
		for (const auto &hasher : hashers) {
			if (hasher.LibIndex != skip_lib_index)
				hashers_.push_back(hasher);
		}
	}

	UNKNOWN_IMPL_BEGIN
	UNKNOWN_IMPL_ITF(ICompressCodecsInfo)
	UNKNOWN_IMPL_ITF(IHashers)
	UNKNOWN_IMPL_END

	STDMETHODIMP_(UInt32) GetNumHashers() noexcept override { return static_cast<UInt32>(hashers_.size()); }

	STDMETHODIMP GetHasherProp(UInt32 index, PROPID propID, PROPVARIANT *value) noexcept override
	{
		const CDllHasherInfo &hi = hashers_[index];
		const auto &lib = libs_[hi.LibIndex];

		if (!lib.ComHashers) {
			return E_FAIL;
		}

		auto* hashers = static_cast<IHashers<UseVirtualDestructor>*>(lib.ComHashers);
		return hashers->GetHasherProp(hi.HasherIndex, propID, value);
	}

	STDMETHODIMP CreateHasher(UInt32 index, IHasher<UseVirtualDestructor> **hasher) noexcept override
	{
		const CDllHasherInfo &hi = hashers_[index];
		const auto &lib = libs_[hi.LibIndex];

		if (!lib.ComHashers || !hasher) {
			return E_FAIL;
		}

		auto* hashers = static_cast<IHashers<UseVirtualDestructor>*>(lib.ComHashers);
		return hashers->CreateHasher(hi.HasherIndex, hasher);
	}

	STDMETHODIMP GetNumMethods(UInt32 *numMethods) noexcept override
	{
		*numMethods = static_cast<UInt32>(codecs_.size());
		return S_OK;
	}

	STDMETHODIMP GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value) noexcept override
	{
		const CDllCodecInfo &ci = codecs_[index];
		if (propID == NMethodPropID::kDecoderIsAssigned || propID == NMethodPropID::kEncoderIsAssigned) {
			PropVariant prop;
			prop = (bool)((propID == NMethodPropID::kDecoderIsAssigned)
							? ci.DecoderIsAssigned
							: ci.EncoderIsAssigned);
			prop.detach(value);
			return S_OK;
		}
		const auto &lib = libs_[ci.LibIndex];
		return lib.GetMethodProperty(ci.CodecIndex, propID, value);
	}

	STDMETHODIMP CreateDecoder(UInt32 index, const GUID *iid, void **coder) noexcept override
	{
		const CDllCodecInfo &ci = codecs_[index];
		if (ci.DecoderIsAssigned) {
			const auto &lib = libs_[ci.LibIndex];
			if (lib.CreateDecoder)
				return lib.CreateDecoder(ci.CodecIndex, iid, (void **)coder);
			else
				return lib.CreateObject(&ci.Decoder, iid, (void **)coder);
		}
		return S_OK;
	}

	STDMETHODIMP CreateEncoder(UInt32 index, const GUID *iid, void **coder) noexcept override
	{
		const CDllCodecInfo &ci = codecs_[index];
		if (ci.EncoderIsAssigned) {
			const auto &lib = libs_[ci.LibIndex];
			if (lib.CreateEncoder)
				return lib.CreateEncoder(ci.CodecIndex, iid, (void **)coder);
			else
				return lib.CreateObject(&ci.Encoder, iid, (void **)coder);
		}
		return S_OK;
	}
};

ArcAPI *ArcAPI::arc_api = nullptr;

ArcAPI::~ArcAPI()
{
	for (auto &arc_lib : arc_libs) {
		if (arc_lib.h_module && arc_lib.SetCodecs)
			arc_lib.SetCodecs(nullptr);
	}
	for (auto arc_lib = arc_libs.rbegin(); arc_lib != arc_libs.rend(); ++arc_lib) {
		if (arc_lib->h_module) {
			arc_lib->ComHashers = nullptr;
			dlclose(arc_lib->h_module);
		}
	}
}

ArcAPI *ArcAPI::get()
{
	if (arc_api == nullptr) {
		arc_api = new ArcAPI();
		arc_api->load();
		if (g_options.patchCP) {
			Patch7zCP::SetCP(static_cast<UINT>(g_options.oemCP), static_cast<UINT>(g_options.ansiCP));
		}
	}
	return arc_api;
}

void ArcAPI::reload()
{
	if (arc_api) {
		ArcAPI::free();
	}

	arc_api = new ArcAPI();
	arc_api->load();
}

void ArcAPI::load_libs(const std::wstring &path)
{
	FileEnum file_enum(path);
	std::wstring dir = extract_file_path(path);

	fprintf(stderr, "ArcAPI::load_libs( %ls )\n", path.c_str());

	bool more;
	while (file_enum.next_nt(more) && more) {
		ArcLib arc_lib;
		arc_lib.module_path = add_trailing_slash(dir) + file_enum.data().cFileName;

		const std::string s2(arc_lib.module_path.begin(), arc_lib.module_path.end());

		fprintf(stderr, "ArcAPI::load_libs() Try to open %s\n", s2.c_str());

		arc_lib.h_module = dlopen(s2.c_str(), RTLD_LAZY);
		//arc_lib.h_module = dlopen(s2.c_str(), RTLD_NOW);

		if (arc_lib.h_module == nullptr) {
			fprintf(stderr, "ArcAPI::load_libs( %ls ) %s nullptr - cannot open(%d)\n", path.c_str(), s2.c_str(), errno);
			continue;
		}

		fprintf(stderr, "ArcAPI::load_libs() Opened %s, Handle = %p \n", s2.c_str(), arc_lib.h_module );

		bool is_duplicate_handle = false;
		for (const auto& existing_lib : arc_libs) {
			if (existing_lib.h_module == arc_lib.h_module) {
					fprintf(stderr, "ArcAPI::load_libs: SKIP %s - Handle %p already exists in arc_libs (duplicate dlopen)\n",
					s2.c_str(), arc_lib.h_module);
				is_duplicate_handle = true;
				break;
			}
		}

		if (is_duplicate_handle) {
			dlclose(arc_lib.h_module);
			continue;
		}

		arc_lib.CreateObject = reinterpret_cast<Func_CreateObject>(
				reinterpret_cast<void *>(dlsym(arc_lib.h_module, "CreateObject")));
		arc_lib.CreateDecoder = reinterpret_cast<Func_CreateDecoder>(
				reinterpret_cast<void *>(dlsym(arc_lib.h_module, "CreateDecoder")));
		arc_lib.CreateEncoder = reinterpret_cast<Func_CreateEncoder>(
				reinterpret_cast<void *>(dlsym(arc_lib.h_module, "CreateEncoder")));
		arc_lib.GetNumberOfMethods = reinterpret_cast<Func_GetNumberOfMethods>(
				reinterpret_cast<void *>(dlsym(arc_lib.h_module, "GetNumberOfMethods")));
		arc_lib.GetMethodProperty = reinterpret_cast<Func_GetMethodProperty>(
				reinterpret_cast<void *>(dlsym(arc_lib.h_module, "GetMethodProperty")));
		arc_lib.GetNumberOfFormats = reinterpret_cast<Func_GetNumberOfFormats>(
				reinterpret_cast<void *>(dlsym(arc_lib.h_module, "GetNumberOfFormats")));
		arc_lib.GetHandlerProperty = reinterpret_cast<Func_GetHandlerProperty>(
				reinterpret_cast<void *>(dlsym(arc_lib.h_module, "GetHandlerProperty")));
		arc_lib.GetHandlerProperty2 = reinterpret_cast<Func_GetHandlerProperty2>(
				reinterpret_cast<void *>(dlsym(arc_lib.h_module, "GetHandlerProperty2")));
		arc_lib.GetIsArc = reinterpret_cast<Func_GetIsArc>(
				reinterpret_cast<void *>(dlsym(arc_lib.h_module, "GetIsArc")));
		arc_lib.SetCodecs = reinterpret_cast<Func_SetCodecs>(
				reinterpret_cast<void *>(dlsym(arc_lib.h_module, "SetCodecs")));
		arc_lib.GetModuleProp = reinterpret_cast<Func_GetModuleProp>(
				reinterpret_cast<void *>(dlsym(arc_lib.h_module, "GetModuleProp")));

		if (arc_lib.CreateObject && ((arc_lib.GetNumberOfFormats && arc_lib.GetHandlerProperty2)
						|| arc_lib.GetHandlerProperty)) {
			bool bUseVD =  true;
			if (arc_lib.GetModuleProp) {
				PropVariant prop;
				if (arc_lib.GetModuleProp(NModulePropID::kVersion, prop.ref()) == S_OK && prop.is_uint()) {
					arc_lib.version = prop.get_uint();
				}
				if (arc_lib.GetModuleProp(NModulePropID::kInterfaceType, prop.ref()) == S_OK && prop.is_uint()) {
					uint32_t haveVirtDestructor = prop.get_uint();
					fprintf(stderr, "ArcAPI::load_libs() %s - PASSED kInterfaceType = %u\n", s2.c_str(), haveVirtDestructor);
					bUseVD = haveVirtDestructor ? true : false;
				}
			}
			else { /// p7zip ?
				arc_lib.version = get_module_version(arc_lib.module_path);
				fprintf(stderr, "ArcAPI::load_libs() %s - PASSED but no GetModuleProp - p7zip ? bUseVD =  true\n", s2.c_str());
			}

			if (arc_libs.empty()) { // set interface type
				bUseVirtualDestructor = bUseVD;
			}
			else { // check interface type
				if (bUseVirtualDestructor != bUseVD) {
					fprintf(stderr, "ArcAPI::load_libs() SKIP %s (Non-compliance with the previously established interface.)\n", s2.c_str());
					dlclose(arc_lib.h_module);
					continue;
				}
			}

			Func_GetHashers getHashers = reinterpret_cast<Func_GetHashers>(
					reinterpret_cast<void *>(dlsym(arc_lib.h_module, "GetHashers")));
			if (getHashers) {

				if (bUseVirtualDestructor) {
					IHashers<true>* hashers = nullptr;

					if (S_OK == getHashers(reinterpret_cast<void**>(&hashers)) && hashers) {
						arc_lib.ComHashers = static_cast<void*>(hashers);
					}
				} else {
					IHashers<false>* hashers = nullptr;
					if (S_OK == getHashers(reinterpret_cast<void**>(&hashers)) && hashers) {
						arc_lib.ComHashers = static_cast<void*>(hashers);
					}
				}
			}

			fprintf(stderr, "ArcAPI::load_libs( %ls ) ADDED %s - OK\n", path.c_str(), s2.c_str());
			arc_libs.push_back(arc_lib);
		} else {
			fprintf(stderr, "ArcAPI::load_libs( %ls ) SKIP %s - FAIL\n", path.c_str(), s2.c_str());
			dlclose(arc_lib.h_module);
		}
	}
}

void ArcAPI::load_codecs(const std::wstring &path)
{
	fprintf(stderr, "ArcAPI::load_codecs( %ls )\n", path.c_str());

	if (n_base_format_libs <= 0) {
		fprintf(stderr, "ArcAPI::load_codecs: No base format libs loaded, skipping.\n");
		return;
	}

	const auto &add_codecs = [this](ArcLib &arc_lib, size_t lib_index) {
		if ((arc_lib.CreateObject || arc_lib.CreateDecoder || arc_lib.CreateEncoder)
				&& arc_lib.GetMethodProperty) {
			UInt32 numMethods = 1;
			bool ok = true;
			if (arc_lib.GetNumberOfMethods)
				ok = S_OK == arc_lib.GetNumberOfMethods(&numMethods);
			for (UInt32 i = 0; ok && i < numMethods; ++i) {
				CDllCodecInfo info;
				info.LibIndex = static_cast<UInt32>(lib_index);
				info.CodecIndex = i;
				if (!GetCoderInfo(arc_lib.GetMethodProperty, i, info)) {
					fprintf(stderr, "ArcAPI::load_codecs: SKIP codec from lib '%ls' (index %zu), method %u - GetCoderInfo failed\n",
									arc_lib.module_path.c_str(), lib_index, i);
					return;
				}

				for (const auto &codec : arc_codecs)
					if (codec.Name == info.Name)
						return;

				fprintf(stderr, "Adding codec %ls \n", info.Name.c_str());
				arc_codecs.push_back(info);
			}
		}
		else {
			fprintf(stderr, "ArcAPI::load_codecs: SKIP lib '%ls' (index %zu) - Does not provide codec methods\n",
				arc_lib.module_path.c_str(), lib_index);
		}
	};

	const auto &add_hashers = [this](ArcLib &arc_lib, size_t lib_index) {
		if (arc_lib.ComHashers) {
			if (bUseVirtualDestructor) {
				auto* hashers = static_cast<IHashers<true>*>(arc_lib.ComHashers);
				UInt32 numHashers = hashers->GetNumHashers();
				for (UInt32 i = 0; i < numHashers; i++) {
					CDllHasherInfo info;
					info.LibIndex = static_cast<UInt32>(lib_index);
					info.HasherIndex = i;
					arc_hashers.push_back(info);
					fprintf(stderr, "ArcAPI::load_codecs: Adding hasher index %u from lib '%ls' (index %zu, VD=true)\n",
						i, arc_lib.module_path.c_str(), lib_index);
				}
			} else {
				auto* hashers = static_cast<IHashers<false>*>(arc_lib.ComHashers);
				UInt32 numHashers = hashers->GetNumHashers();
				for (UInt32 i = 0; i < numHashers; i++) {
					CDllHasherInfo info;
					info.LibIndex = static_cast<UInt32>(lib_index);
					info.HasherIndex = i;
					arc_hashers.push_back(info);
					fprintf(stderr, "ArcAPI::load_codecs: Adding hasher index %u from lib '%ls' (index %zu, VD=false)\n",
						i, arc_lib.module_path.c_str(), lib_index);
				}
			}
		}
	};

	for (size_t ii = 1; ii < n_format_libs; ++ii) {	   // all but 7z.dll
		auto &arc_lib = arc_libs[ii];
		add_codecs(arc_lib, ii);
		add_hashers(arc_lib, ii);
	}

	FileEnum codecs_enum(path);
	std::wstring dir = extract_file_path(path);
	bool more;
	while (codecs_enum.next_nt(more) && more && !codecs_enum.data().is_dir()) {
		ArcLib arc_lib;
		arc_lib.module_path = add_trailing_slash(dir) + codecs_enum.data().cFileName;
		const std::string s2(arc_lib.module_path.begin(), arc_lib.module_path.end());

		arc_lib.h_module = dlopen(s2.c_str(), RTLD_LAZY);
		if (arc_lib.h_module == nullptr) {
			continue;
		}

		bool is_duplicate_handle = false;
		for (const auto& existing_lib : arc_libs) {
			if (existing_lib.h_module == arc_lib.h_module) {
					fprintf(stderr, "ArcAPI::load_codecs: SKIP %s - Handle %p already exists in arc_libs (duplicate dlopen)\n",
					s2.c_str(), arc_lib.h_module);
				is_duplicate_handle = true;
				break;
			}
		}

		if (is_duplicate_handle) {
			dlclose(arc_lib.h_module);
			continue;
		}

		arc_lib.CreateObject = reinterpret_cast<Func_CreateObject>(
				reinterpret_cast<void *>(dlsym(arc_lib.h_module, "CreateObject")));
		arc_lib.CreateDecoder = reinterpret_cast<Func_CreateDecoder>(
				reinterpret_cast<void *>(dlsym(arc_lib.h_module, "CreateDecoder")));
		arc_lib.CreateEncoder = reinterpret_cast<Func_CreateEncoder>(
				reinterpret_cast<void *>(dlsym(arc_lib.h_module, "CreateEncoder")));
		arc_lib.GetNumberOfMethods = reinterpret_cast<Func_GetNumberOfMethods>(
				reinterpret_cast<void *>(dlsym(arc_lib.h_module, "GetNumberOfMethods")));
		arc_lib.GetMethodProperty = reinterpret_cast<Func_GetMethodProperty>(
				reinterpret_cast<void *>(dlsym(arc_lib.h_module, "GetMethodProperty")));
		arc_lib.GetModuleProp = reinterpret_cast<Func_GetModuleProp>(
				reinterpret_cast<void *>(dlsym(arc_lib.h_module, "GetModuleProp")));

		{
			bool bUseVD =  true;

			fprintf(stderr, "ArcAPI::load_codecs %s have GetModuleProp\n", s2.c_str() );

			if (arc_lib.GetModuleProp) {
				PropVariant prop;
				if (arc_lib.GetModuleProp(NModulePropID::kVersion, prop.ref()) == S_OK && prop.is_uint()) {
					arc_lib.version = prop.get_uint();
				}
				if (arc_lib.GetModuleProp(NModulePropID::kInterfaceType, prop.ref()) == S_OK && prop.is_uint()) {
					uint32_t haveVirtDestructor = prop.get_uint();
					fprintf(stderr, "ArcAPI::load_codecs %s kInterfaceType = %u\n", s2.c_str(), haveVirtDestructor);
					bUseVD = haveVirtDestructor ? true : false;
				}
			}
			else { /// p7zip codec ?
				arc_lib.version = get_module_version(arc_lib.module_path);
				fprintf(stderr, "ArcAPI::load_codecs %s no GetModuleProp - p7zip ? bUseVD =  true\n", s2.c_str());
			}

			if (bUseVirtualDestructor != bUseVD) {
				fprintf(stderr, "ArcAPI::load_codecs SKIP %s (Non-compliance with the previously established interface.)\n", s2.c_str());
				dlclose(arc_lib.h_module);
				continue;
			}
		}

		arc_lib.GetNumberOfFormats = nullptr;
		arc_lib.GetHandlerProperty = nullptr;
		arc_lib.GetHandlerProperty2 = nullptr;
		arc_lib.GetIsArc = nullptr;
		arc_lib.SetCodecs = nullptr;
		arc_lib.version = 0;
		auto n_start_codecs = arc_codecs.size();
		auto n_start_hashers = arc_hashers.size();
		add_codecs(arc_lib, arc_libs.size());
		Func_GetHashers getHashers = reinterpret_cast<Func_GetHashers>(
				reinterpret_cast<void *>(dlsym(arc_lib.h_module, "GetHashers")));
		if (getHashers) {
			if (bUseVirtualDestructor) {
				IHashers<true>* hashers = nullptr;
				if (S_OK == getHashers(reinterpret_cast<void**>(&hashers)) && hashers) {
					arc_lib.ComHashers = static_cast<void*>(hashers);
				}
			} else {
				IHashers<false>* hashers = nullptr;
				if (S_OK == getHashers(reinterpret_cast<void**>(&hashers)) && hashers) {
					arc_lib.ComHashers = static_cast<void*>(hashers);
				}
			}
			add_hashers(arc_lib, arc_libs.size());
		}
		if (n_start_codecs < arc_codecs.size() || n_start_hashers < arc_hashers.size())
			arc_libs.push_back(arc_lib);
		else
			dlclose(arc_lib.h_module);
	}

	n_7z_codecs = 0;
	if (arc_codecs.size() > 0) {
		std::sort(arc_codecs.begin(), arc_codecs.end(), [&](const CDllCodecInfo &a, const CDllCodecInfo &b) {
			bool a_is_zip = (a.CodecId & 0xffffff00U) == 0x040100;
			bool b_is_zip = (b.CodecId & 0xffffff00U) == 0x040100;
			if (a_is_zip != b_is_zip)
				return b_is_zip;
			else
				return StrCmpI(a.Name.c_str(), b.Name.c_str()) < 0;
		});

		for (const auto &c : arc_codecs) {
			if ((c.CodecId & 0xffffff00U) != 0x040100)
				++n_7z_codecs;
		}
	}
	for (size_t i = 0; i < n_format_libs; ++i) {
		if (arc_libs[i].SetCodecs) {
			if (bUseVirtualDestructor) {
				auto compressinfo = new MyCompressCodecsInfo<true>(arc_libs, arc_codecs, arc_hashers, i);
				UInt32 nm = 0, nh = compressinfo->GetNumHashers();
				compressinfo->GetNumMethods(&nm);
				if (nm > 0 || nh > 0) {
					arc_libs[i].SetCodecs(compressinfo);
				} else {
					delete compressinfo;
				}
			}
			else {
				auto compressinfo = new MyCompressCodecsInfo<false>(arc_libs, arc_codecs, arc_hashers, i);
				UInt32 nm = 0, nh = compressinfo->GetNumHashers();
				compressinfo->GetNumMethods(&nm);
				if (nm > 0 || nh > 0) {
					arc_libs[i].SetCodecs(compressinfo);
				} else {
					delete compressinfo;
				}
			}
		}
	}
}

struct SfxModuleInfo
{
	const wchar_t *module_name;
	unsigned descr_id;
	bool all_codecs;
	bool install_config;
};

const SfxModuleInfo c_known_sfx_modules[] = {
		{L"7z.sfx",		MSG_SFX_DESCR_7Z,	  true,	false},
		{L"7zCon.sfx",   MSG_SFX_DESCR_7ZCON,	true,  false},
		{L"7zS.sfx",	 MSG_SFX_DESCR_7ZS,		false, true },
		{L"7zSD.sfx",	  MSG_SFX_DESCR_7ZSD,	  false, true },
		{L"7zS2.sfx",	  MSG_SFX_DESCR_7ZS2,	  false, false},
		{L"7zS2con.sfx", MSG_SFX_DESCR_7ZS2CON, false, false},
};

static const SfxModuleInfo *find(const std::wstring &path)
{
	unsigned i = 0;
	for (; i < ARRAYSIZE(c_known_sfx_modules)
			&& upcase(extract_file_name(path)) != upcase(c_known_sfx_modules[i].module_name);
			i++)
		;
	if (i < ARRAYSIZE(c_known_sfx_modules))
		return c_known_sfx_modules + i;
	else
		return nullptr;
}

std::wstring SfxModule::description() const
{
	const SfxModuleInfo *info = find(path);
	return info ? Far::get_msg(info->descr_id)
				: Far::get_msg(MSG_SFX_DESCR_UNKNOWN) + L" [" + extract_file_name(path) + L"]";
}

bool SfxModule::all_codecs() const
{
	const SfxModuleInfo *info = find(path);
	return info ? info->all_codecs : true;
}

bool SfxModule::install_config() const
{
	const SfxModuleInfo *info = find(path);
	return info ? info->install_config : true;
}

void ArcAPI::find_sfx_modules(const std::wstring &path)
{
	FileEnum file_enum(path);
	fprintf(stderr, " ArcAPI::find_sfx_modules: %ls ... \n", path.c_str());

	std::wstring dir = extract_file_path(path);
	bool more;
	while (file_enum.next_nt(more) && more) {
		SfxModule sfx_module;
		sfx_module.path = add_trailing_slash(dir) + file_enum.data().cFileName;
		File file;
		if (!file.open_nt(sfx_module.path, FILE_READ_DATA, FILE_SHARE_READ, OPEN_EXISTING, 0))
			continue;
		Buffer<char> buffer(2);
		size_t sz;
		if (!file.read_nt(buffer.data(), buffer.size(), sz))
			continue;
		std::string sig(buffer.data(), sz);

//		if (sig == "MZ") {
			//  fprintf(stderr, " MZ ???? \n");
//			  continue;
//		}
//		fprintf(stderr, "ArcAPI::find_sfx_modules: Added sfx!\n");

		sfx_modules.push_back(sfx_module);
	}
}

static bool ParseSignatures(const Byte *data, size_t size, std::vector<ByteVector> &signatures)
{
	signatures.clear();
	while (size > 0) {
		size_t len = *data++;
		size--;
		if (len > size)
			return false;

		ByteVector v(data, data + len);
		signatures.push_back(v);

		data += len;
		size -= len;
	}
	return true;
}


void ArcAPI::load()
{
	auto dll_path = add_trailing_slash(Far::get_plugin_module_path());

	fprintf(stderr, "==== ArcAPI::load() {\n");
	fprintf(stderr, "get_plugin_module_path() = %ls\n", (Far::get_plugin_module_path()).c_str());
	fprintf(stderr, "dll_path = %ls\n", dll_path.c_str());
	fprintf(stderr, "dll_path = %ls + *.so\n", dll_path.c_str());

	std::wstring _7zip_path = g_options.preferred_7zip_path;

	load_libs(_7zip_path + L"*7z*.so");
#if defined(__APPLE__)
	load_libs(_7zip_path + L"*7z*.dylib");
#endif
	find_sfx_modules(_7zip_path + L"*.sfx");

	if (arc_libs.empty()) {
		_7zip_path = dll_path;
		load_libs(_7zip_path + L"*7z*.so");
#if defined(__APPLE__)
		load_libs(_7zip_path + L"*7z*.dylib");
#endif
		if (sfx_modules.empty())
			find_sfx_modules(_7zip_path + L"*.sfx");
	}

	static const std::vector<std::wstring> search_paths = {
		L"/usr/lib/7zip/",
		L"/usr/local/lib/7zip/",
		L"/usr/lib/p7zip/",
		L"/usr/local/lib/p7zip/",
#if defined(__APPLE__)
		L"/opt/homebrew/lib/7zip/",	   // macOS Homebrew
		L"/opt/homebrew/lib/p7zip/",	   // macOS Homebrew
		L"/opt/homebrew/lib/",	   // macOS Homebrew
#endif
		L"/usr/libexec/7zip/",
		L"/usr/libexec/p7zip/",
		L"/usr/libexec/",
#if 1
		L"/usr/lib64/7zip/",
		L"/usr/local/lib64/7zip/",
		L"/usr/lib64/p7zip/",
		L"/usr/local/lib64/p7zip/",
		L"/usr/lib32/7zip/",
		L"/usr/local/lib32/7zip/",
		L"/usr/lib32/p7zip/",
		L"/usr/local/lib32/p7zip/",
		L"/usr/lib64/",
		L"/usr/lib/",
		L"/usr/lib32/",
#endif
	};

	if (arc_libs.empty()) {

		for (const auto& wstr_path : search_paths) {
			_7zip_path = wstr_path;

			load_libs(_7zip_path + L"*7z*.so");
#if defined(__APPLE__)
			load_libs(_7zip_path + L"*7z*.dylib");
#endif
			if (sfx_modules.empty()) {
				find_sfx_modules(_7zip_path + L"*.sfx");
			}

			if (!arc_libs.empty())
				break;
		}
	}

	n_base_format_libs = n_format_libs = arc_libs.size();
	load_libs(_7zip_path + L"Formats/*.format");
	n_format_libs = arc_libs.size();
	load_codecs(_7zip_path + L"Codecs/*.codec");
	load_codecs(_7zip_path + L"Codecs/*.so");

	fprintf(stderr, "n_format_libs = %u\n", (unsigned int)n_format_libs);

	for (unsigned i = 0; i < n_format_libs; i++) {
		const ArcLib &arc_lib = arc_libs[i];

		UInt32 num_formats;
		if (arc_lib.GetNumberOfFormats) {
			if (arc_lib.GetNumberOfFormats(&num_formats) != S_OK)
				num_formats = 0;
		} else
			num_formats = 1;

		fprintf(stderr, "list num_formats = %u\n", num_formats);

		for (UInt32 idx = 0; idx < num_formats; idx++) {
			ArcFormat format;

			if (arc_lib.get_bytes_prop(idx, NArchive::NHandlerPropID::kClassID, format.ClassID) != S_OK) {
				fprintf(stderr, "	[Format %u] FAILED: cannot read ClassID\n", idx);
				continue;
			}

			arc_lib.get_string_prop(idx, NArchive::NHandlerPropID::kName, format.name);
			if (format.name.empty()) {
				fprintf(stderr, "	[Format %u] WARNING: empty name\n", idx);
			}
			if (arc_lib.get_bool_prop(idx, NArchive::NHandlerPropID::kUpdate, format.updatable) != S_OK)
				format.updatable = false;

			fprintf(stderr, "Format %ls updatable = %u\n", format.name.c_str(), format.updatable);
			fprintf(stderr, "	  ClassID: ");
			for (size_t j = 0; j < 16; ++j) {
				fprintf(stderr, "%02X", static_cast<unsigned char>(format.ClassID[j]));
			}

			std::wstring extension_list_str;
			arc_lib.get_string_prop(idx, NArchive::NHandlerPropID::kExtension, extension_list_str);
			format.extension_list = split(extension_list_str, L' ');
			std::wstring add_extension_list_str;
			arc_lib.get_string_prop(idx, NArchive::NHandlerPropID::kAddExtension, add_extension_list_str);
			std::list<std::wstring> add_extension_list = split(add_extension_list_str, L' ');
			auto add_ext_iter = add_extension_list.cbegin();

			fprintf(stderr, "\n");
			fprintf(stderr, "	  Extensions: ");
			for (const auto& ext : format.extension_list) {
				fprintf(stderr, " %ls ", ext.c_str());
			}
			fprintf(stderr, "\n");

			for (auto ext_iter = format.extension_list.begin(); ext_iter != format.extension_list.end();
					++ext_iter) {
				ext_iter->insert(0, 1, L'.');
				if (add_ext_iter != add_extension_list.cend()) {
					if (*add_ext_iter != L"*") {
						format.nested_ext_mapping[upcase(*ext_iter)] = *add_ext_iter;
						fprintf(stderr, "	  NestedExt[%ls] -> %ls\n", ext_iter->c_str(), add_ext_iter->c_str());
					}
					++add_ext_iter;
				}
			}

			format.lib_index = (int)i;
			format.FormatIndex = idx;

			format.NewInterface = arc_lib.get_uint_prop(idx, NArchive::NHandlerPropID::kFlags, format.Flags) == S_OK;
			if (!format.NewInterface) {	   // support for DLL version < 9.31:
				bool v = false;
				if (arc_lib.get_bool_prop(idx, NArchive::NHandlerPropID::kKeepName, v) != S_OK && v)
					format.Flags |= NArcInfoFlags::kKeepName;
				if (arc_lib.get_bool_prop(idx, NArchive::NHandlerPropID::kAltStreams, v) != S_OK && v)
					format.Flags |= NArcInfoFlags::kAltStreams;
				if (arc_lib.get_bool_prop(idx, NArchive::NHandlerPropID::kNtSecure, v) != S_OK && v)
					format.Flags |= NArcInfoFlags::kNtSecure;
			}

			UInt32 timeFlags = 0;
			if (arc_lib.get_uint_prop(idx, NArchive::NHandlerPropID::kTimeFlags, timeFlags) == S_OK) {
				format.TimeFlags = timeFlags;
			}

			ByteVector sig;
			arc_lib.get_bytes_prop(idx, NArchive::NHandlerPropID::kSignature, sig);
			if (!sig.empty())
				format.Signatures.push_back(sig);
			else {
				arc_lib.get_bytes_prop(idx, NArchive::NHandlerPropID::kMultiSignature, sig);
				ParseSignatures(sig.data(), sig.size(), format.Signatures);
			}

			if (arc_lib.get_uint_prop(idx, NArchive::NHandlerPropID::kSignatureOffset, format.SignatureOffset)
					!= S_OK)
				format.SignatureOffset = 0;

			if (arc_lib.GetIsArc) {
				arc_lib.GetIsArc(idx, &format.IsArc);
			}

			if (format.Signatures.empty()) {
				fprintf(stderr, "	  Signatures: (none)\n");
			} else {
				fprintf(stderr, "	  Signatures (%zu):\n", format.Signatures.size());
				for (const auto& s : format.Signatures) {
					fprintf(stderr, "		");
					for (size_t j = 0; j < s.size(); ++j) {
						fprintf(stderr, "%02X ", static_cast<unsigned char>(s[j]));
					}
					bool is_text = true;
					for (size_t j = 0; j < s.size(); ++j) {
						unsigned char c = s[j];
						if (c < 0x20 || c > 0x7E) { is_text = false; break; }
					}
					if (is_text) {
						fprintf(stderr, " ('");
						for (size_t j = 0; j < s.size(); ++j) {
							fprintf(stderr, "%c", s[j]);
						}
						fprintf(stderr, "')");
					}
					fprintf(stderr, "\n");
				}
			}

			if (format.SignatureOffset != 0) {
				fprintf(stderr, "	SignatureOffset: %u\n", (unsigned int)format.SignatureOffset);
			}

			fprintf(stderr, "	Flags: 0x%02X (", format.Flags);
			if (format.Flags & NArcInfoFlags::kKeepName)		fprintf(stderr, "KeepName ");
			if (format.Flags & NArcInfoFlags::kFindSignature) 	fprintf(stderr, "FindSignature ");

			if (format.Flags & NArcInfoFlags::kAltStreams)		fprintf(stderr, "AltStreams ");
			if (format.Flags & NArcInfoFlags::kNtSecure)		fprintf(stderr, "NtSecure ");
			if (format.Flags & NArcInfoFlags::kSymLinks)		fprintf(stderr, "SymLinks ");
			if (format.Flags & NArcInfoFlags::kHardLinks)		fprintf(stderr, "HardLinks ");

			if (format.Flags & NArcInfoFlags::kUseGlobalOffset)	fprintf(stderr, "UseGlobalOffset ");
			if (format.Flags & NArcInfoFlags::kStartOpen)		fprintf(stderr, "StartOpen ");
			if (format.Flags & NArcInfoFlags::kBackwardOpen)	fprintf(stderr, "BackwardOpen ");
			if (format.Flags & NArcInfoFlags::kPreArc)			fprintf(stderr, "PreArc ");
			if (format.Flags & NArcInfoFlags::kPureStartOpen)	fprintf(stderr, "PureStartOpen ");
			if (format.Flags & NArcInfoFlags::kByExtOnlyOpen)	fprintf(stderr, "ByExtOnlyOpen ");

			if (format.Flags & 0x01) fprintf(stderr, "Container ");

			fprintf(stderr, ")\n");
			fprintf(stderr, "		Updatable: %s\n", format.updatable ? "yes" : "no");
			fprintf(stderr, "		IsArc(): %s\n", format.IsArc ? "true" : "false");
			fprintf(stderr, "		LibIndex: %d, FormatIndex: %u\n", format.lib_index, format.FormatIndex);
			fprintf(stderr, "		------------------------------\n");

			ArcFormats::const_iterator existing_format = arc_formats.find(format.ClassID);
			if (existing_format == arc_formats.end()
					|| arc_libs[existing_format->second.lib_index].version < arc_lib.version)
				arc_formats[format.ClassID] = format;
		}
	}
}

//void ArcAPI::create_in_archive(const ArcType &arc_type, IInArchive **in_arc)
void ArcAPI::create_in_archive(const ArcType &arc_type, void **in_arc)
{
	CHECK_COM(libs()[formats().at(arc_type).lib_index]
					.CreateObject(reinterpret_cast<const GUID *>(arc_type.data()), &IID_IInArchive,
							reinterpret_cast<void **>(in_arc)));
}

//void ArcAPI::create_out_archive(const ArcType &arc_type, IOutArchive **out_arc)
void ArcAPI::create_out_archive(const ArcType &arc_type, void **out_arc)
{
	CHECK_COM(libs()[formats().at(arc_type).lib_index]
					.CreateObject(reinterpret_cast<const GUID *>(arc_type.data()), &IID_IOutArchive,
							reinterpret_cast<void **>(out_arc)));
}

void ArcAPI::free()
{
	if (arc_api) {
		delete arc_api;
		arc_api = nullptr;
	}
}

template<bool UseVirtualDestructor>
std::wstring Archive<UseVirtualDestructor>::get_default_name() const
{
	std::wstring name = arc_name();
	std::wstring ext = extract_file_ext(name);
	name.erase(name.size() - ext.size(), ext.size());
	if (arc_chain.empty())
		return name;
	const ArcType &arc_type = arc_chain.back().type;
	auto &nested_ext_mapping = ArcAPI::formats().at(arc_type).nested_ext_mapping;
	auto nested_ext_iter = nested_ext_mapping.find(upcase(ext));
	if (nested_ext_iter == nested_ext_mapping.end())
		return name;
	const std::wstring &nested_ext = nested_ext_iter->second;
	ext = extract_file_ext(name);
	if (upcase(nested_ext) == upcase(ext))
		return name;
	name.replace(name.size() - ext.size(), ext.size(), nested_ext);
	return name;
}

template<bool UseVirtualDestructor>
std::wstring Archive<UseVirtualDestructor>::get_temp_file_name() const
{
	GUID guid;
	CoCreateGuid(&guid);
	wchar_t guid_str[50];
	StringFromGUID2(&guid, guid_str, ARRAYSIZE(guid_str));
	return add_trailing_slash(arc_dir()) + guid_str + L".tmp";
}

bool ArcFileInfo::operator<(const ArcFileInfo &file_info) const
{
	if (parent == file_info.parent)
		if (is_dir == file_info.is_dir) {
			return StrCmp(name.c_str(), file_info.name.c_str()) < 0;
		} else
			return is_dir;
	else
		return parent < file_info.parent;
}

#define add_file_to_hard_link_group(_index, _group) \
	hard_link_groups[_group].push_back(_index); \
	file_list[_index].hl_group = _group;

template<bool UseVirtualDestructor>
void Archive<UseVirtualDestructor>::groupHardLinks(HardLinkPrepData &prep_data)
{
	std::map<std::wstring, UInt32> target_to_group;

	for (const auto &trg_data : prep_data.target_hardlinks) {
		uint32_t source_index = trg_data.source_index;
		const std::wstring &target_path = trg_data.target_path;

		//fprintf(stderr, ">> source=%u -> target='%ls'\n", source_index, target_path.c_str());
		auto target_it = prep_data.name_to_index.find(target_path);
		if (target_it != prep_data.name_to_index.end()) {
			uint32_t target_index = target_it->second;

			if (file_list[target_index].hl_group != (UInt32)-1) {
				add_file_to_hard_link_group(source_index, file_list[target_index].hl_group);
			} else {
				auto group_it = target_to_group.find(target_path);
				if (group_it != target_to_group.end()) {
					uint32_t group_index = group_it->second;
					add_file_to_hard_link_group(source_index, group_index);
				} else { /// first add with main file target index
					uint32_t group_index = addHardLinkGroup();
					target_to_group[target_path] = group_index;
					add_file_to_hard_link_group(target_index, group_index);
					add_file_to_hard_link_group(source_index, group_index);
				}
			}
		} else {
			file_list[source_index].num_links = 0;
		}
	}
}

template<bool UseVirtualDestructor>
void Archive<UseVirtualDestructor>::update_hard_link_counts()
{
	for (const auto &group : hard_link_groups) {
		uint32_t link_count = group.size();
		for (auto file_index : group) {
			file_list[file_index].num_links = link_count;
//			file_list[file_index].attributes |= FILE_ATTRIBUTE_HARDLINKS;
		}
	}
}

template<bool UseVirtualDestructor>
void Archive<UseVirtualDestructor>::make_index()
{
	m_num_indices = 0;
	in_arc->GetNumberOfItems(&m_num_indices);
	file_list.clear();
	file_list.reserve(m_num_indices);

	struct DirInfo
	{
		UInt32 index;
		UInt32 parent;
		std::wstring name;
		bool operator<(const DirInfo &dir_info) const
		{
			if (parent == dir_info.parent) {
				return StrCmp(name.c_str(), dir_info.name.c_str()) < 0;
			} else
				return parent < dir_info.parent;
		}
	};
	typedef std::set<DirInfo> DirList;
	std::map<UInt32, unsigned> dir_index_map;
	DirList dir_list;

	// https://forum.farmanager.com/viewtopic.php?p=169196#p169196
	const auto is_dir_split = [](const wchar_t ch) {
		return ch == wchar_t(0xf05c) || is_slash(ch);
	};

	DirInfo dir_info;
	UInt32 dir_index = 0;
	ArcFileInfo file_info;
	std::wstring path;
	PropVariant prop;
	HardLinkPrepData prep_data;

	for (UInt32 i = 0; i < m_num_indices; i++) {
		// is directory?
		file_info.is_dir = in_arc->GetProperty(i, kpidIsDir, prop.ref()) == S_OK && prop.is_bool() && prop.get_bool();
		file_info.is_altstream = get_isaltstream(i);

		// file name
		if (in_arc->GetProperty(i, kpidPath, prop.ref()) == S_OK && prop.is_str())
			path.assign(prop.get_str());
		else
			path.assign(get_default_name());

		size_t name_end_pos = path.size();
		while (name_end_pos && is_dir_split(path[name_end_pos - 1]))
			name_end_pos--;
		size_t name_pos = name_end_pos;
		while (name_pos && !is_dir_split(path[name_pos - 1]))
			name_pos--;
		file_info.name.assign(path.data() + name_pos, name_end_pos - name_pos);

		prep_data.name_to_index[path] = i;
		file_info.hl_group = (uint32_t)-1;

		if (file_info.is_dir) {
			file_info.num_links = 1;
		}
		else if (in_arc->GetProperty(i, kpidHardLink, prop.ref()) == S_OK && prop.is_str()) {
			TrHardLinkData trg_data;
			trg_data.source_index = i;
			trg_data.target_path = prop.get_str();
			prep_data.target_hardlinks.push_back(trg_data);
		}
		else if (in_arc->GetProperty(i, kpidLinks, prop.ref()) == S_OK && prop.is_uint()) {
			file_info.num_links = static_cast<UInt32>(prop.get_uint());
			if (file_info.num_links > 1) {
				UInt64 inode = 0;
				UInt64 device = 0;

				if (in_arc->GetProperty(i, kpidINode, prop.ref()) == S_OK && prop.is_uint()) {
					inode = prop.get_uint();
					if (in_arc->GetProperty(i, kpidDevMajor, prop.ref()) == S_OK && prop.is_uint()) {
						UInt32 dev_major = prop.get_uint();
						if (in_arc->GetProperty(i, kpidDevMinor, prop.ref()) == S_OK && prop.is_uint()) {
							UInt32 dev_minor = prop.get_uint();
							device = (static_cast<UInt64>(dev_major) << 32) | dev_minor;
						}
					}
					if (inode != 0) {
						HardLinkPrepData::InodeKey key;
						key.inode = inode;
						key.device = device;

						UInt32 group_index = 0;
						auto group_it = prep_data.inode_to_group.find(key);

						if (group_it != prep_data.inode_to_group.end()) {
							group_index = group_it->second;
						} else {
							group_index = addHardLinkGroup();
							prep_data.inode_to_group[key] = group_index;
						}

						hard_link_groups[group_index].push_back(i);
						file_info.hl_group = group_index;
					 }
				}
			}
		}
		else
			file_info.num_links = 1;

//		fprintf(stderr, " path = %ls name = %ls\n", path.c_str(), file_info.name.c_str() );
		// =======================================================================================================================================

		if (in_arc->GetProperty(i, kpidUser, prop.ref()) == S_OK && prop.is_str())
			file_info.owner.assign(prop.get_str());

		if (in_arc->GetProperty(i, kpidGroup, prop.ref()) == S_OK && prop.is_str())
			file_info.group.assign(prop.get_str());

		if (in_arc->GetProperty(i, kpidUserId, prop.ref()) == S_OK && prop.is_uint())
			file_info.fuid = prop.get_uint();
		else
			file_info.fuid = static_cast<uid_t>(-1);

		if (in_arc->GetProperty(i, kpidGroupId, prop.ref()) == S_OK && prop.is_uint())
			file_info.fgid = prop.get_uint();
		else
			file_info.fgid = static_cast<uid_t>(-1);

		if (in_arc->GetProperty(i, kpidComment, prop.ref()) == S_OK && prop.is_str())
			file_info.desc.assign(prop.get_str());

		// split path into individual directories and put them into DirList
		dir_info.parent = c_root_index;
		std::stack<UInt32> dir_parents;
		size_t begin_pos = 0;
		while (begin_pos < name_pos) {
			dir_info.index = dir_index;
			size_t end_pos = begin_pos;
			while (end_pos < name_pos && !is_dir_split(path[end_pos]))
				end_pos++;
			if (end_pos != begin_pos) {
				dir_info.name = path.substr(begin_pos, end_pos - begin_pos);

				if (dir_info.name == L"..") {
					if (!dir_parents.empty()) {
						dir_info.parent = dir_parents.top();
						dir_parents.pop();
					}
				} else if (dir_info.name != L".") {
					std::pair<typename DirList::iterator, bool> ins_pos = dir_list.insert(dir_info);
					if (ins_pos.second)
						dir_index++;
					dir_parents.push(dir_info.parent);
					dir_info.parent = ins_pos.first->index;
				}
			}
			begin_pos = end_pos + 1;
		}
		file_info.parent = dir_info.parent;

		if (file_info.is_dir) {
			dir_info.index = dir_index;
			dir_info.name = file_info.name;
			std::pair<typename DirList::iterator, bool> ins_pos = dir_list.insert(dir_info);
			if (ins_pos.second) {
				dir_index++;
				dir_index_map[dir_info.index] = i;
			} else {
				if (dir_index_map.count(ins_pos.first->index))
					file_info.parent = c_dup_index;
				else
					dir_index_map[ins_pos.first->index] = i;
			}
		}

		file_list.push_back(file_info);
	}

	// add directories that not present in archive index
	file_list.reserve(file_list.size() + dir_list.size() - dir_index_map.size());
	dir_index = m_num_indices;
	std::for_each(dir_list.begin(), dir_list.end(), [&](const DirInfo &item) {
		if (dir_index_map.count(item.index) == 0) {
			dir_index_map[item.index] = dir_index;
			file_info.parent = item.parent;
			file_info.name = item.name;
			file_info.is_dir = true;
			file_info.is_altstream = false;
			dir_index++;
			file_list.push_back(file_info);
		}
	});

	// fix parent references
	std::for_each(file_list.begin(), file_list.end(), [&](ArcFileInfo &item) {
		if (item.parent != c_root_index && item.parent != c_dup_index)
			item.parent = dir_index_map[item.parent];
	});

	// create search index
	file_list_index.clear();
	file_list_index.reserve(file_list.size());
	for (UInt32 i = 0; i < file_list.size(); i++) {
		file_list_index.push_back(i);
	}
	std::sort(file_list_index.begin(), file_list_index.end(), [&](UInt32 left, UInt32 right) -> bool {
		return file_list[left] < file_list[right];
	});

	load_arc_attr();
	groupHardLinks(prep_data);
	update_hard_link_counts();
//	debug_print_hard_link_groups();
}

template<bool UseVirtualDestructor>
UInt32 Archive<UseVirtualDestructor>::find_dir(const std::wstring &path)
{
	if (file_list.empty())
		make_index();

	ArcFileInfo dir_info;
	dir_info.is_dir = true;
	dir_info.parent = c_root_index;
	size_t begin_pos = 0;
	while (begin_pos < path.size()) {
		size_t end_pos = begin_pos;
		while (end_pos < path.size() && !is_slash(path[end_pos]))
			end_pos++;
		if (end_pos != begin_pos) {
			dir_info.name.assign(path.data() + begin_pos, end_pos - begin_pos);
			FileIndexRange fi_range = std::equal_range(file_list_index.begin(), file_list_index.end(), -1,
					[&](UInt32 left, UInt32 right) -> bool {
						const ArcFileInfo &fi_left = left == (UInt32)-1 ? dir_info : file_list[left];
						const ArcFileInfo &fi_right = right == (UInt32)-1 ? dir_info : file_list[right];
						return fi_left < fi_right;
					});
			if (fi_range.first == fi_range.second)
				FAIL(HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND));
			dir_info.parent = *fi_range.first;
		}
		begin_pos = end_pos + 1;
	}
	return dir_info.parent;
}

template<bool UseVirtualDestructor>
FileIndexRange Archive<UseVirtualDestructor>::get_dir_list(UInt32 dir_index)
{
	if (file_list.empty())
		make_index();

	ArcFileInfo file_info;
	file_info.parent = dir_index;
	FileIndexRange index_range = std::equal_range(file_list_index.begin(), file_list_index.end(), -1,
			[&](UInt32 left, UInt32 right) -> bool {
				const ArcFileInfo &fi_left = left == (UInt32)-1 ? file_info : file_list[left];
				const ArcFileInfo &fi_right = right == (UInt32)-1 ? file_info : file_list[right];
				return fi_left.parent < fi_right.parent;
			});

	return index_range;
}

template<bool UseVirtualDestructor>
std::wstring Archive<UseVirtualDestructor>::get_path(UInt32 index)
{
	if (file_list.empty())
		make_index();

	std::wstring file_path = file_list[index].name;
	UInt32 file_parent = file_list[index].parent;
	while (file_parent != c_root_index) {
		file_path.insert(0, 1, L'/').insert(0, file_list[file_parent].name);
		file_parent = file_list[file_parent].parent;
	}
	return file_path;
}

template<bool UseVirtualDestructor>
FindData Archive<UseVirtualDestructor>::get_file_info(UInt32 index)
{
	if (file_list.empty())
		make_index();

	FindData file_info{};
	std::wcscpy(file_info.cFileName, file_list[index].name.c_str());
	file_info.dwFileAttributes = get_attr(index, &file_info.dwUnixMode);
	file_info.set_size(get_size(index));
	file_info.ftCreationTime = get_ctime(index);
	file_info.ftLastWriteTime = get_mtime(index);
	file_info.ftLastAccessTime = get_atime(index);
	//file_info.ftChangeTime = get_chtime(index);

	return file_info;
}

template<bool UseVirtualDestructor>
bool Archive<UseVirtualDestructor>::get_main_file(UInt32 &index)
{
	PropVariant prop;

	if (in_arc->GetArchiveProperty(kpidMainSubfile, prop.ref()) == S_OK && prop.is_uint()) {
		index = prop.get_uint();
		return true;
	}

	UInt32 num_indices = 0;
	in_arc->GetNumberOfItems(&num_indices);
	if (!num_indices) {
		return false;
	}

//	const ArcType &rArcType = arc_chain.back().type;
	const ArcType &lArcType = arc_chain.front().type;
	std::wstring ext = extract_file_ext(arc_path);

	if (file_list.empty())
		make_index();

	if (lArcType == c_ar && !StrCmpI(ext.c_str(), L".deb" ) && num_indices < 32) {
//		UInt32 iindex = 0xFFFFFFFF;
		for (UInt32 ii = 0; ii < num_indices; ++ii) {
			if (file_list[ii].is_dir) continue;
			std::wstring _name = file_list[ii].name;
			removeExtension(_name);
			if (!StrCmp(_name.c_str(), L"data.tar")) {
				index = ii;
				return true;
				break;
			}
		}
	}

	return false;
}


template<bool UseVirtualDestructor>
DWORD Archive<UseVirtualDestructor>::get_links(UInt32 index) const
{
	PropVariant prop;
	DWORD n = 1;

	if (index >= m_num_indices)
		return 0;

	if (in_arc->GetProperty(index, kpidLinks, prop.ref()) == S_OK && prop.is_uint()) {
		n = static_cast<DWORD>(prop.get_uint());
	}

	return n;
}

DWORD	SetFARAttributes(DWORD attr, DWORD posixattr)
{
	DWORD farattr = 0;

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

	return farattr;

#if 0

#define S_IFMT   0170000  /* type of file mask */
#define S_IFIFO  0010000  /* named pipe (fifo) */
#define S_IFCHR  0020000  /* character special */
#define S_IFDIR  0040000  /* directory */
#define S_IFBLK  0060000  /* block special */

#define S_IFREG  0100000  /* regular */
#define S_IFLNK  0120000  /* symbolic link */
#define S_IFSOCK 0140000  /* socket */
#define S_ISUID  0004000  /* set-user-ID on execution */
#define S_ISGID  0002000  /* set-group-ID on execution */
#define S_ISVTX  0001000  /* save swapped text even after use */
#define S_IRWXU  0000700  /* RWX mask for owner */
#define S_IRUSR  0000400  /* R for owner */
#define S_IWUSR  0000200  /* W for owner */
#define S_IXUSR  0000100  /* X for owner */
#define S_IRWXG  0000070  /* RWX mask for group */
#define S_IRGRP  0000040  /* R for group */
#define S_IWGRP  0000020  /* W for group */
#define S_IXGRP  0000010  /* X for group */
#define S_IRWXO  0000007  /* RWX mask for other */
#define S_IROTH  0000004  /* R for other */
#define S_IWOTH  0000002  /* W for other */
#define S_IXOTH  0000001  /* X for other */

S_ISBLK(st_mode m)  /* block special */
S_ISCHR(st_mode m)  /* char special */
S_ISDIR(st_mode m)  /* directory */
S_ISFIFO(st_mode m) /* fifo */
S_ISLNK(st_mode m)  /* symbolic link */
S_ISREG(st_mode m)  /* regular file */
S_ISSOCK(st_mode m) /* socket */

#endif
}


template<bool UseVirtualDestructor>
DWORD Archive<UseVirtualDestructor>::get_attr(UInt32 index, DWORD *posixattr) const
{
	PropVariant prop;
	DWORD attr = 0, _posixattr = 0;

	if (index >= m_num_indices) {
		attr = FILE_ATTRIBUTE_DIRECTORY;
		if (posixattr)
			*posixattr = _posixattr;
		return attr;
	}

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

	if (in_arc->GetProperty(index, kpidSymLink, prop.ref()) == S_OK && prop.is_str()) {
		if (prop.vt == VT_BSTR && SysStringLen(prop.bstrVal) ) {
			_posixattr |= S_IFLNK;
		}
	}

	if (posixattr) *posixattr = _posixattr;

	if (file_list[index].is_dir)
		attr |= FILE_ATTRIBUTE_DIRECTORY;

	return attr;
}

template<bool UseVirtualDestructor>
bool Archive<UseVirtualDestructor>::get_user(UInt32 index, std::wstring &str) const
{
	PropVariant prop;
	if (index >= m_num_indices)
		return false;

	if (in_arc->GetProperty(index, kpidUser, prop.ref()) == S_OK && prop.is_str()) {
		str = prop.get_str();
		return true;
	}

	return false;
}

template<bool UseVirtualDestructor>
bool Archive<UseVirtualDestructor>::get_user_id(UInt32 index, UInt32 &id) const
{
	PropVariant prop;
	if (index >= m_num_indices)
		return false;

	if (in_arc->GetProperty(index, kpidUserId, prop.ref()) == S_OK && prop.is_uint()) {
		id = prop.get_uint();
		return true;
	}

	return false;
}

template<bool UseVirtualDestructor>
bool Archive<UseVirtualDestructor>::get_group_id(UInt32 index, UInt32 &id) const
{
	PropVariant prop;
	if (index >= m_num_indices)
		return false;

	if (in_arc->GetProperty(index, kpidGroupId, prop.ref()) == S_OK && prop.is_uint()) {
		id = prop.get_uint();
		return true;
	}

	return false;
}


template<bool UseVirtualDestructor>
bool Archive<UseVirtualDestructor>::get_group(UInt32 index, std::wstring &str) const
{
	PropVariant prop;
	if (index >= m_num_indices)
		return false;

	if (in_arc->GetProperty(index, kpidGroup, prop.ref()) == S_OK && prop.is_str()) {
		str = prop.get_str();
		return true;
	}

	return false;
}

template<bool UseVirtualDestructor>
bool Archive<UseVirtualDestructor>::get_encrypted(UInt32 index) const
{
	PropVariant prop;
	if (index >= m_num_indices)
		return false;
	else if (!file_list[index].is_dir && in_arc->GetProperty(index, kpidEncrypted, prop.ref()) == S_OK
			&& prop.is_bool())
		return prop.get_bool();
	else
		return false;
}

template<bool UseVirtualDestructor>
bool Archive<UseVirtualDestructor>::get_hardlink(UInt32 index, std::wstring &str) const
{
	PropVariant prop;
	if (index >= m_num_indices)
		return false;

	if (in_arc->GetProperty(index, kpidHardLink, prop.ref()) == S_OK && prop.is_str()) {
		str = prop.get_str();
		return true;
	}

	return false;
}

template<bool UseVirtualDestructor>
UInt64 Archive<UseVirtualDestructor>::get_size(UInt32 index) const
{
	PropVariant prop;
	if (index >= m_num_indices)
		return 0;

	if (!file_list[index].is_dir && in_arc->GetProperty(index, kpidSize, prop.ref()) == S_OK && prop.is_uint()) {
		uint64_t _size = prop.get_uint();
		const ArcFileInfo &file_info = file_list[index];
		if (_size || file_info.num_links < 2)
			return _size;

		uint32_t tf_index = hard_link_groups[file_info.hl_group][0];
		if (tf_index >= m_num_indices)
			return _size;

		if (in_arc->GetProperty(tf_index, kpidSize, prop.ref()) != S_OK && !prop.is_uint())
			return _size;

		return prop.get_uint();
	}

	return 0;
}

template<bool UseVirtualDestructor>
UInt64 Archive<UseVirtualDestructor>::get_psize(UInt32 index) const
{
	PropVariant prop;
	if (index >= m_num_indices)
		return 0;

	if (!file_list[index].is_dir && in_arc->GetProperty(index, kpidPackSize, prop.ref()) == S_OK && prop.is_uint()) {
		uint64_t _size = prop.get_uint();
		const ArcFileInfo &file_info = file_list[index];
		if (_size || file_info.num_links < 2)
			return _size;

		uint32_t tf_index = hard_link_groups[file_info.hl_group][0];
		if (tf_index >= m_num_indices)
			return _size;

		if (in_arc->GetProperty(tf_index, kpidPackSize, prop.ref()) != S_OK && !prop.is_uint())
			return _size;

		return prop.get_uint();
	}

	return 0;
}

template<bool UseVirtualDestructor>
UInt64 Archive<UseVirtualDestructor>::get_offset(UInt32 index) const
{
	PropVariant prop;
	if (index < m_num_indices)
		if (in_arc->GetProperty(index, kpidOffset, prop.ref()) == S_OK && prop.is_uint())
			return prop.get_uint();
	return ~0ULL;
}


template<bool UseVirtualDestructor>
FILETIME Archive<UseVirtualDestructor>::get_ctime(UInt32 index) const
{
	PropVariant prop;
	FILETIME ft;

	if (index >= m_num_indices) {
		return nullftime;
	}

	if (in_arc->GetProperty(index, kpidCTime, prop.ref()) == S_OK && prop.is_filetime()) {
		ft = prop.get_filetime();
#if IS_BIG_ENDIAN
		return FILETIME{ft.dwLowDateTime,ft.dwHighDateTime};
#else
		return ft;
#endif
	}

	return nullftime;
	//return arc_info.ftCreationTime;
}

template<bool UseVirtualDestructor>
FILETIME Archive<UseVirtualDestructor>::get_mtime(UInt32 index) const
{
	PropVariant prop;
	FILETIME ft;

	if (index >= m_num_indices) {
		return nullftime;
	}

	if (in_arc->GetProperty(index, kpidMTime, prop.ref()) == S_OK && prop.is_filetime()) {
		ft = prop.get_filetime();
#if IS_BIG_ENDIAN
		return FILETIME{ft.dwLowDateTime,ft.dwHighDateTime};
#else
		return ft;
#endif
	}

	return nullftime;
	//return arc_info.ftLastWriteTime;
}

template<bool UseVirtualDestructor>
FILETIME Archive<UseVirtualDestructor>::get_atime(UInt32 index) const
{
	PropVariant prop;
	FILETIME ft;

	if (index >= m_num_indices) {
		return nullftime;
	}

	if (in_arc->GetProperty(index, kpidATime, prop.ref()) == S_OK && prop.is_filetime()) {
		ft = prop.get_filetime();
#if IS_BIG_ENDIAN
		return FILETIME{ft.dwLowDateTime,ft.dwHighDateTime};
#else
		return ft;
#endif
	}

	return nullftime;
	//return arc_info.ftLastAccessTime;
}

template<bool UseVirtualDestructor>
FILETIME Archive<UseVirtualDestructor>::get_chtime(UInt32 index) const
{
	PropVariant prop;
	FILETIME ft;

	if (index >= m_num_indices) {
		return nullftime;
	}

	if (in_arc->GetProperty(index, kpidChangeTime, prop.ref()) == S_OK && prop.is_filetime()) {
		ft = prop.get_filetime();
#if IS_BIG_ENDIAN
		return FILETIME{ft.dwLowDateTime,ft.dwHighDateTime};
#else
		return ft;
#endif
	}

	return nullftime;
	//return arc_info.ftLastAccessTime;
}

template<bool UseVirtualDestructor>
unsigned Archive<UseVirtualDestructor>::get_crc(UInt32 index) const
{
	PropVariant prop;
	if (index >= m_num_indices)
		return 0;
	else if (in_arc->GetProperty(index, kpidCRC, prop.ref()) == S_OK && prop.is_uint())
		return static_cast<DWORD>(prop.get_uint());
	else
		return 0;
}

template<bool UseVirtualDestructor>
bool Archive<UseVirtualDestructor>::get_anti(UInt32 index) const
{
	PropVariant prop;
	if (index >= m_num_indices)
		return false;
	else if (in_arc->GetProperty(index, kpidIsAnti, prop.ref()) == S_OK && prop.is_bool())
		return prop.get_bool();
	else
		return false;
}

template<bool UseVirtualDestructor>
bool Archive<UseVirtualDestructor>::get_isaltstream(UInt32 index) const
{
	PropVariant prop;
	if (index >= m_num_indices)
		return false;
	else if (in_arc->GetProperty(index, kpidIsAltStream, prop.ref()) == S_OK && prop.is_bool())
		return prop.get_bool();
	else
		return false;
}

template<bool UseVirtualDestructor>
bool Archive<UseVirtualDestructor>::get_device(UInt32 index, dev_t &_device) const
{
	UInt32 dev_major, dev_minor;
	PropVariant prop;
	if (index >= m_num_indices)
		return false;

	if (in_arc->GetProperty(index, kpidDeviceMajor, prop.ref()) != S_OK || !prop.is_uint())
		return false;

	dev_major = prop.get_uint();

	if (in_arc->GetProperty(index, kpidDeviceMinor, prop.ref()) != S_OK || !prop.is_uint())
		return false;

	dev_minor = prop.get_uint();
	_device = makedev(dev_major, dev_minor);
	return true;
}

template<bool UseVirtualDestructor>
bool Archive<UseVirtualDestructor>::get_dev(UInt32 index, dev_t &_device) const
{
	UInt32 dev_major, dev_minor;
	PropVariant prop;
	if (index >= m_num_indices)
		return false;

	if (in_arc->GetProperty(index, kpidDevMajor, prop.ref()) != S_OK || !prop.is_uint())
		return false;

	dev_major = prop.get_uint();

	if (in_arc->GetProperty(index, kpidDevMinor, prop.ref()) != S_OK || !prop.is_uint())
		return false;

	dev_minor = prop.get_uint();
	_device = makedev(dev_major, dev_minor);
	return true;
}

template<bool UseVirtualDestructor>
void Archive<UseVirtualDestructor>::read_open_results()
{
	PropVariant prop;

	error_flags = 0;
	if (in_arc->GetArchiveProperty(kpidErrorFlags, prop.ref()) == S_OK
			&& (prop.vt == VT_UI4 || prop.vt == VT_UI8))
		error_flags = static_cast<UInt32>(prop.get_uint());

	error_text.clear();
	if (in_arc->GetArchiveProperty(kpidError, prop.ref()) == S_OK && prop.is_str())
		error_text = prop.get_str();

	warning_flags = 0;
	if (in_arc->GetArchiveProperty(kpidWarningFlags, prop.ref()) == S_OK
			&& (prop.vt == VT_UI4 || prop.vt == VT_UI8))
		warning_flags = static_cast<UInt32>(prop.get_uint());

	warning_text.clear();
	if (in_arc->GetArchiveProperty(kpidWarning, prop.ref()) == S_OK && prop.is_str())
		warning_text = prop.get_str();

	UInt64 phy_size = 0;
	if (in_arc->GetArchiveProperty(kpidPhySize, prop.ref()) == S_OK && prop.is_size())
		phy_size = prop.get_size();

	if (phy_size) {
		UInt64 offset = 0;
		if (in_arc->GetArchiveProperty(kpidOffset, prop.ref()) == S_OK && prop.is_size())
			offset = prop.get_size();
		auto file_size = archive_filesize();
		auto end_pos = offset + phy_size;
		if (end_pos < file_size)
			warning_flags |= kpv_ErrorFlags_DataAfterEnd;
		else if (end_pos > file_size)
			error_flags |= kpv_ErrorFlags_UnexpectedEnd;
	}
}

static std::list<std::wstring> flags2texts(UInt32 flags)
{
	std::list<std::wstring> texts;
	if (flags != 0) {
		if ((flags & kpv_ErrorFlags_IsNotArc) == kpv_ErrorFlags_IsNotArc) {	   // 1
			texts.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_IS_NOT_ARCHIVE));
			flags &= ~kpv_ErrorFlags_IsNotArc;
		}
		if ((flags & kpv_ErrorFlags_HeadersError) == kpv_ErrorFlags_HeadersError) {	   // 2
			texts.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_HEADERS_ERROR));
			flags &= ~kpv_ErrorFlags_HeadersError;
		}
		if ((flags & kpv_ErrorFlags_EncryptedHeadersError) == kpv_ErrorFlags_EncryptedHeadersError) {	 // 4
			texts.emplace_back(L"EncryptedHeadersError");	 // TODO: localize
			flags &= ~kpv_ErrorFlags_EncryptedHeadersError;
		}
		if ((flags & kpv_ErrorFlags_UnavailableStart) == kpv_ErrorFlags_UnavailableStart) {	   // 8
			// errors.emplace_back(L"UnavailableStart"); // TODO: localize
			texts.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_UNAVAILABLE_DATA));
			flags &= ~kpv_ErrorFlags_UnavailableStart;
		}
		if ((flags & kpv_ErrorFlags_UnconfirmedStart) == kpv_ErrorFlags_UnconfirmedStart) {	   // 16
			texts.emplace_back(L"UnconfirmedStart");	// TODO: localize
			flags &= ~kpv_ErrorFlags_UnconfirmedStart;
		}
		if ((flags & kpv_ErrorFlags_UnexpectedEnd) == kpv_ErrorFlags_UnexpectedEnd) {	 // 32
			texts.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_UNEXPECTED_END_DATA));
			flags &= ~kpv_ErrorFlags_UnexpectedEnd;
		}
		if ((flags & kpv_ErrorFlags_DataAfterEnd) == kpv_ErrorFlags_DataAfterEnd) {	   // 64
			texts.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_DATA_AFTER_END));
			flags &= ~kpv_ErrorFlags_DataAfterEnd;
		}
		if ((flags & kpv_ErrorFlags_UnsupportedMethod) == kpv_ErrorFlags_UnsupportedMethod) {	 // 128
			texts.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_UNSUPPORTED_METHOD));
			flags &= ~kpv_ErrorFlags_UnsupportedMethod;
		}
		if ((flags & kpv_ErrorFlags_UnsupportedFeature) == kpv_ErrorFlags_UnsupportedFeature) {	   // 256
			texts.emplace_back(L"UnsupportedFeature");	  // TODO: localize
			flags &= ~kpv_ErrorFlags_UnsupportedFeature;
		}
		if ((flags & kpv_ErrorFlags_DataError) == kpv_ErrorFlags_DataError) {	 // 512
			texts.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_DATA_ERROR));
			flags &= ~kpv_ErrorFlags_DataError;
		}
		if ((flags & kpv_ErrorFlags_CrcError) == kpv_ErrorFlags_CrcError) {	   // 1024
			texts.emplace_back(Far::get_msg(MSG_ERROR_EXTRACT_CRC_ERROR));
			flags &= ~kpv_ErrorFlags_CrcError;
		}
		if (flags != 0) {
			wchar_t buf[32];
			swprintf(buf, 16, L"%u", flags);
			texts.emplace_back(L"Unknown error: ");
			texts.back().append(buf);
		}
	}
	return texts;
}

template<bool UseVirtualDestructor>
std::list<std::wstring> Archive<UseVirtualDestructor>::get_open_errors() const
{
	auto errors = flags2texts(error_flags);
	if (!error_text.empty())
		errors.emplace_back(error_text);
	return errors;
}

template<bool UseVirtualDestructor>
std::list<std::wstring> Archive<UseVirtualDestructor>::get_open_warnings() const
{
	auto warnings = flags2texts(warning_flags);
	if (!warning_text.empty())
		warnings.emplace_back(warning_text);
	return warnings;
}

template class Archive<true>;
template class Archive<false>;
