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

std::string SitesConfigLocation::TranslateToPath() const
{
	std::string out;

	for (const auto &part : _parts) {
		if (!out.empty()) {
			out+= '/';
		}
		out+= part;
	}

	return out;
}

std::string SitesConfigLocation::TranslateToSitesConfigPath() const
{
	std::string out = SitesConfig_TranslateToDir(_parts);
	out+= "sites.cfg";
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

	std::string out = sites_cfg_location.TranslateToPath();
	out+= site;
	return out;
}

///

SitesConfig::SitesConfig(const SitesConfigLocation &sites_cfg_location)
	: KeyFileHelper(sites_cfg_location.TranslateToSitesConfigPath().c_str())
{
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

