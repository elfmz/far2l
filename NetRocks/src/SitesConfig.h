#pragma once

#include "KeyFileHelper.h"

class SitesConfigLocation
{
	std::vector<std::string> _parts;

public:
	void Reset();
	bool Change(const std::string &sub);
	bool Make(const std::string &sub);
	bool Remove(const std::string &sub);

	void Enum(std::vector<std::string> &children) const;

	std::string TranslateToPath() const;
	std::string TranslateToSitesConfigPath() const;
};


struct SiteSpecification
{
	SitesConfigLocation sites_cfg_location;
	std::string site;

	SiteSpecification() = default;
	SiteSpecification(const std::string &s);

	bool IsValid() const {return  !site.empty(); }

	std::string ToString() const;
};

class SitesConfig : protected KeyFileHelper
{
public:
	SitesConfig(const SitesConfigLocation &sites_cfg_location);

	inline std::vector<std::string> EnumSites() { return EnumSections(); } 
	inline void RemoveSite(const std::string &site) { return RemoveSection(site.c_str()); } 

	std::string GetProtocol(const std::string &site);
	void PutProtocol(const std::string &site, const std::string &value);

	std::string GetHost(const std::string &site);
	void PutHost(const std::string &site, const std::string &value);

	std::string GetDirectory(const std::string &site);
	void PutDirectory(const std::string &site, const std::string &value);

	unsigned int GetPort(const std::string &site, unsigned int def = 0);
	void PutPort(const std::string &site, unsigned int value);

	unsigned int GetLoginMode(const std::string &site, unsigned int def = 0);
	void PutLoginMode(const std::string &site, unsigned int value);

	std::string GetUsername(const std::string &site);
	void PutUsername(const std::string &site, const std::string &value);

	std::string GetPassword(const std::string &site);
	void PutPassword(const std::string &site, const std::string &value);

	std::string GetProtocolOptions(const std::string &site, const std::string &protocol);
	void PutProtocolOptions(const std::string &site, const std::string &protocol, const std::string &options);
};
