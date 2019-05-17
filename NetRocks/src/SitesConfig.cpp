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

SitesConfig::SitesConfig()
	: KeyFileHelper(InMyConfig("NetRocks/sites.cfg").c_str())
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

