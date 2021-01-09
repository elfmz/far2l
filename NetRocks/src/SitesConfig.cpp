#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>

#include <string>
#include <fstream>
#include <streambuf>
#include <utils.h>
#include <base64.h>

#include "SitesConfig.h"

#define NETROCKS_EXPORT_SITE_EXTENSION	".Site.NetRocks"
#define NETROCKS_EXPORT_DIR_EXTENSION	".Dir.NetRocks"


static std::string ObtainObfuscationKey()
{
	std::string out;
	try {
		std::ifstream f("/etc/machine-id");
		out = std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
	} catch(std::exception &) {
	}

	if (out.empty()) {
		out = "DummyObfuscationKey";
	}

	return out;
}

static const std::string &ObfuscationKey()
{
	static std::string s_out = ObtainObfuscationKey();
	return s_out;
}

static void StringObfuscate(std::string &s)
{
	const std::string &key = ObfuscationKey();
	std::vector<unsigned char> data;
	unsigned char salt_len = (unsigned char)(rand() & 0x7);
	data.emplace_back(salt_len ^ ((unsigned char)key[key.size() - 1]));
	for (unsigned char i = 0; i < salt_len; ++i) {
		data.emplace_back((unsigned char)(rand()&0xff));
	}
	for (size_t i = 0; i < s.size(); ++i) {
		data.emplace_back(((unsigned char)s[i]) ^ ((unsigned char)key[i % key.size()]));
	}
	s.clear();
	if (!data.empty()) {
		base64_encode(s, &data[0], data.size());
	}
}

static void StringDeobfuscate(std::string &s)
{
	const std::string &key = ObfuscationKey();
	std::vector<unsigned char> data;
	base64_decode(data, s);
	s.clear();
	if (!data.empty()) {
		unsigned char salt_len = ((unsigned char)data[0]) ^ ((unsigned char)key[key.size() - 1]);
		if (salt_len <= 7) {
			for (size_t i = 0; i + 1 + salt_len < data.size(); ++i) {
				 s+= (char)(((unsigned char)data[i + 1 + salt_len]) ^ ((unsigned char)key[i % key.size()]));
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool SitesConfig_AppendSubParts(std::vector<std::string> &parts, const std::string &sub)
{
	StrExplode(parts, sub, "/");

	for (auto it = parts.begin(); it != parts.end(); ) {
		if (*it == ".") {
			it = parts.erase(it);

		} else if (*it == "..") {
			if (it == parts.begin()) {
				return false;
			}
			--it;
			it = parts.erase(parts.erase(it));

		} else {
			++it;
		}
	}

	return true;	
}

static std::string SitesConfig_TranslateToDir(const std::vector<std::string> &parts)
{
	std::string out = InMyConfig("NetRocks/");
	for (const auto &part : parts) {
		out+= part;
		out+= ".sites/";
	}
	return out;
}


void SitesConfigLocation::Reset()
{
	_parts.clear();
}

bool SitesConfigLocation::Change(const std::string &sub)
{
	std::vector<std::string> parts = _parts;
	if (!SitesConfig_AppendSubParts(parts, sub)) {
		return false;
	}

	struct stat s{};
	if (stat(SitesConfig_TranslateToDir(parts).c_str(), &s) != 0) {
		return false;
	}

	_parts.swap(parts);
	return true;
}

bool SitesConfigLocation::Make(const std::string &sub)
{
	std::vector<std::string> parts = _parts;
	if (!SitesConfig_AppendSubParts(parts, sub)) {
		return false;
	}

	const std::string &dir = SitesConfig_TranslateToDir(parts);

	struct stat s{};
	if (stat(dir.c_str(), &s) != 0) {
		if (mkdir(dir.c_str(), 0700) != 0) {
			return false;
		}
	}

	return true;
}

bool SitesConfigLocation::Remove(const std::string &sub)
{
	SitesConfigLocation tmp = *this;
	if (!SitesConfig_AppendSubParts(tmp._parts, sub) || tmp._parts.empty()) {
		return false;
	}


	if (!SitesConfig(tmp).EnumSites().empty()) {
		return false;
	}

	unlink(tmp.TranslateToSitesConfigPath().c_str());

	return rmdir(SitesConfig_TranslateToDir(tmp._parts).c_str()) == 0;
}


void SitesConfigLocation::Enum(std::vector<std::string> &children) const
{
	DIR *d = opendir(SitesConfig_TranslateToDir(_parts).c_str());
	if (d) {
		for (;;) {
			struct dirent *de = readdir(d);
			if (!de) break;
			const size_t l = strlen(de->d_name);
			if (l > 6 && memcmp(de->d_name + l - 6, ".sites", 6) == 0) {
				children.emplace_back(de->d_name, l - 6);
			}
		}
		closedir(d);
	}
}

std::string SitesConfigLocation::TranslateToPath(bool ending_slash) const
{
	std::string out;

	for (const auto &part : _parts) {
		out+= part;
		out+= '/';
	}

	if (!ending_slash && !out.empty()) {
		out.resize(out.size() - 1);
	}

	return out;
}

std::string SitesConfigLocation::TranslateToSitesConfigPath() const
{
	std::string out = SitesConfig_TranslateToDir(_parts);
	out+= "sites.cfg";
	return out;
}

bool SitesConfigLocation::Transfer(SitesConfigLocation &dst, const std::string &sub, bool mv)
{
	std::vector<std::string> parts = _parts;
	if (!SitesConfig_AppendSubParts(parts, sub) || parts.empty()) {
		return false;
	}
	std::string src_arg = SitesConfig_TranslateToDir(parts);
	src_arg.resize(src_arg.size() - 1); // remove slash
	src_arg = EscapeCmdStr(src_arg);
	const std::string &dst_arg = EscapeCmdStr(SitesConfig_TranslateToDir(dst._parts));

	std::string cmd;
	if (mv) {
		cmd = StrPrintf("mv -f \"%s\" \"%s\" >/dev/null 2>&1", src_arg.c_str(), dst_arg.c_str());
	} else {
		cmd = StrPrintf("cp -R -f \"%s\" \"%s\" >/dev/null 2>&1", src_arg.c_str(), dst_arg.c_str());
	}

	fprintf(stderr, "SitesConfigLocation::Transfer: %s\n", cmd.c_str());


	return system(cmd.c_str()) == 0;
}

bool SitesConfigLocation::Import(const std::string &src_dir, const std::string &item_name, bool is_dir, bool mv)
{
	fprintf(stderr,
		"SitesConfigLocation::Import('%s', '%s', %d\n",
			src_dir.c_str(), item_name.c_str(), is_dir);

	std::string item_fs_path = src_dir;
	if (!item_fs_path.empty() && item_fs_path.back() != '/') {
		item_fs_path+= '/';
	}
	item_fs_path+= item_name;

	if (!is_dir) {
		SitesConfig sc(*this);
		if (!sc.Import(item_fs_path)) {
			return false;
		}
		if (mv) {
			unlink(item_fs_path.c_str());
		}
		return true;
	}


	DIR *d = opendir(item_fs_path.c_str());
	if (!d) {
		return false;
	}

	std::string actual_name = item_name;
	if (StrEndsBy(actual_name, NETROCKS_EXPORT_DIR_EXTENSION)) {
		actual_name.resize(actual_name.size() - (sizeof(NETROCKS_EXPORT_DIR_EXTENSION) - 1));
	}

	std::vector<std::string> saved_parts = _parts;	

	if (!Change(actual_name)) {
		Make(actual_name);
		if (!Change(actual_name)) {
			closedir(d);
			_parts = saved_parts;
			return false;
		}
	}

	std::string de_name;
	bool out = true;
	for (;;) {
		struct dirent *de = readdir(d);
		if (!de) break;
		de_name = de->d_name;
		if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
			if (StrEndsBy(de_name, NETROCKS_EXPORT_DIR_EXTENSION)) {
				if (!Import(item_fs_path, de_name, true, mv)) {
					out = false;
				}

			} else if (StrEndsBy(de_name, NETROCKS_EXPORT_SITE_EXTENSION)) {
				if (!Import(item_fs_path, de_name, false, mv)) {
					out = false;
				}
			}
		}
	}
	closedir(d);
	_parts = saved_parts;
	if (out && mv) {
		rmdir(item_fs_path.c_str());
	}

	return out;
}

bool SitesConfigLocation::Export(const std::string &dst_dir, const std::string &item_name, bool is_dir, bool mv)
{
	std::string item_fs_path = dst_dir;
	if (!item_fs_path.empty() && item_fs_path.back() != '/') {
		item_fs_path+= '/';
	}
	item_fs_path+= item_name;

	if (!is_dir) {
		item_fs_path+= NETROCKS_EXPORT_SITE_EXTENSION;
		SitesConfig sc(*this);
		if (!sc.Export(item_fs_path, item_name)) {
			return false;
		}
		if (mv) {
			sc.RemoveSite(item_name);
		}
		return true;
	}

	item_fs_path+= NETROCKS_EXPORT_DIR_EXTENSION;

	mkdir(item_fs_path.c_str(), 0700);

	std::vector<std::string> saved_parts = _parts;
	bool out = Change(item_name);
	if (out) {
		{
			std::vector<std::string> sites = SitesConfig(*this).EnumSites();
			for (const auto &site : sites) {
				if (!Export(item_fs_path, site, false, mv)) {
					out = false;
				}
			}
		}

		std::vector<std::string> children;
		for (const auto &child : children) {
			if (!Export(item_fs_path, child, true, mv)) {
				out = false;

			}
		}
	}
	_parts = saved_parts;
	if (out && mv) {
		Remove(item_name);
	}
	return out;
}

///

SiteSpecification::SiteSpecification(const std::string &s)
{
	size_t p = s.rfind('/');
	if (p == std::string::npos) {
		site = s;

	} else if (!sites_cfg_location.Change(s.substr(0, p))) {
		fprintf(stderr, "SiteSpecification('%s') - bad config location\n", s.c_str());
		site.clear();

	} else {
		site = s.substr(p + 1, s.size() - p - 1);
	}
	
}

std::string SiteSpecification::ToString() const
{
	if (!IsValid()) {
		return std::string();
	}

	std::string out = sites_cfg_location.TranslateToPath(true);
	out+= site;
	return out;
}

///

SitesConfig::SitesConfig(const SitesConfigLocation &sites_cfg_location)
	: KeyFileHelper(sites_cfg_location.TranslateToSitesConfigPath().c_str())
{
}

bool SitesConfig::Export(const std::string &fs_path, const std::string &site)
{
	KeyFileHelper kfh(fs_path.c_str(), false);
	const std::vector<std::string> &keys = EnumKeys(site.c_str());
	for (const auto &key : keys) {
		std::string s = GetString(site.c_str(), key.c_str());
		if (key == "Password") {
			StringDeobfuscate(s);
			kfh.PutString(site.c_str(), "PasswordPlain", s.c_str());
		} else {
			kfh.PutString(site.c_str(), key.c_str(), s.c_str());
		}
	}

	return kfh.Save();
}

bool SitesConfig::Import(const std::string &fs_path)
{
	KeyFileHelper kfh(fs_path.c_str(), true);
	if (!kfh.IsLoaded()) {
		return false;
	}

	const std::vector<std::string> &sites = kfh.EnumSections();
	if (sites.empty()) {
		return false;
	}

	for (const auto &site : sites) {
		const std::vector<std::string> &keys = kfh.EnumKeys(site.c_str());
		for (const auto &key : keys) {
			std::string s = kfh.GetString(site.c_str(), key.c_str());
			if (key == "PasswordPlain") {
				StringObfuscate(s);
				PutString(site.c_str(), "Password", s.c_str());
			} else {
				PutString(site.c_str(), key.c_str(), s.c_str());
			}
		}
	}

	return true;
}


std::string SitesConfig::GetProtocol(const std::string &site)
{
	return GetString(site.c_str(), "Protocol");
}

void SitesConfig::PutProtocol(const std::string &site, const std::string &value)
{
	PutString(site.c_str(), "Protocol", value.c_str());
}


std::string SitesConfig::GetHost(const std::string &site)
{
	return GetString(site.c_str(), "Host");
}

void SitesConfig::PutHost(const std::string &site, const std::string &value)
{
	PutString(site.c_str(), "Host", value.c_str());
}


std::string SitesConfig::GetDirectory(const std::string &site)
{
	return GetString(site.c_str(), "Directory");
}

void SitesConfig::PutDirectory(const std::string &site, const std::string &value)
{
	PutString(site.c_str(), "Directory", value.c_str());
}


unsigned int SitesConfig::GetPort(const std::string &site, unsigned int def)
{
	return (unsigned int)GetInt(site.c_str(), "Port", def);
}

void SitesConfig::PutPort(const std::string &site, unsigned int value)
{
	PutInt(site.c_str(), "Port", value);
}

unsigned int SitesConfig::GetLoginMode(const std::string &site, unsigned int def)
{
	return (unsigned int)GetInt(site.c_str(), "LoginMode", def);
}

void SitesConfig::PutLoginMode(const std::string &site, unsigned int value)
{
	PutInt(site.c_str(), "LoginMode", value);
}


std::string SitesConfig::GetUsername(const std::string &site)
{
	return GetString(site.c_str(), "Username");
}

void SitesConfig::PutUsername(const std::string &site, const std::string &value)
{
	PutString(site.c_str(), "Username", value.c_str());
}


std::string SitesConfig::GetPassword(const std::string &site)
{
	std::string s = GetString(site.c_str(), "Password");
	if (s.empty()) {
		s = GetString(site.c_str(), "PasswordPlain");
		if (!s.empty()) {
			StringObfuscate(s);
			PutString(site.c_str(), "Password", s.c_str());
			RemoveKey(site.c_str(), "PasswordPlain");
		}
	}
	StringDeobfuscate(s);
	return s;
}

void SitesConfig::PutPassword(const std::string &site, const std::string &value)
{
	std::string s(value);
	StringObfuscate(s);
	PutString(site.c_str(), "Password", s.c_str());
}

std::string SitesConfig::GetProtocolOptions(const std::string &site, const std::string &protocol)
{
	return GetString(site.c_str(), std::string("Options_").append(protocol).c_str());
}

void SitesConfig::PutProtocolOptions(const std::string &site, const std::string &protocol, const std::string &options)
{
	PutString(site.c_str(), std::string("Options_").append(protocol).c_str(), options.c_str());
}

bool SitesConfig::Transfer(SitesConfig &dst, const std::string &site, bool mv)
{
	const auto keys = KeyFileHelper::EnumKeys(site.c_str());
	dst.RemoveSection(site.c_str());
	for (const auto &key : keys) {
		const std::string &value = GetString(site.c_str(), key.c_str());
		dst.PutString(site.c_str(), key.c_str(), value.c_str());
	}

	if (mv) {
		RemoveSection(site.c_str());
	}
	return true;
}
