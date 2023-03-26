#pragma once

#include "KeyFileHelper.h"

#define NETROCKS_EXPORT_SITE_EXTENSION	".Site.NetRocks"
#define NETROCKS_EXPORT_DIR_EXTENSION	".Dir.NetRocks"


class SitesConfigLocation
{
	// if nonempty then its a fixed-sites config file, but not in-profile settings,
	// so can't create/traverse to subdirectories ets
	std::string _sites_config_file;

	// subdirectory path parts, used only if _sites_config_file is empty()
	std::vector<std::string> _parts;

public:
	bool operator == (const SitesConfigLocation &other) const;

	inline bool operator != (const SitesConfigLocation &other) const { return !operator ==(other); }

	SitesConfigLocation(const std::string &sites_config_file = std::string());

	bool IsStandaloneConfig() const { return !_sites_config_file.empty(); }

	void Reset();
	bool Change(const std::string &sub);
	bool Make(const std::string &sub);
	bool Remove(const std::string &sub);

	void Enum(std::vector<std::string> &children) const;

	std::string TranslateToPath(bool ending_slash) const;
	std::string TranslateToSitesConfigPath() const;

	bool Transfer(SitesConfigLocation &dst, const std::string &sub, bool mv);

	bool Import(const std::string &src_dir, const std::string &item_name, bool is_dir, bool mv);
	bool Export(const std::string &dst_dir, const std::string &item_name, bool is_dir, bool mv);
};


struct SiteSpecification
{
	SitesConfigLocation sites_cfg_location;
	std::string site;

	SiteSpecification() = default;
	SiteSpecification(const std::string &standalone_config, const std::string &s);

	bool IsValid() const {return !site.empty(); }

	std::string ToString() const;
};

class SitesConfig : protected KeyFileHelper
{
	bool _encrypt_passwords;

public:
	SitesConfig(const SitesConfigLocation &sites_cfg_location);

	bool Import(const std::string &fs_path);
	bool Export(const std::string &fs_path, const std::string &site);

	inline std::vector<std::string> EnumSites() { return EnumSections(); }
	inline void RemoveSite(const std::string &site) { RemoveSection(site.c_str()); }

	std::string GetProtocol(const std::string &site);
	void SetProtocol(const std::string &site, const std::string &value);

	std::string GetHost(const std::string &site);
	void SetHost(const std::string &site, const std::string &value);

	std::string GetDirectory(const std::string &site);
	void SetDirectory(const std::string &site, const std::string &value);

	unsigned int GetPort(const std::string &site, unsigned int def = 0);
	void SetPort(const std::string &site, unsigned int value);

	unsigned int GetLoginMode(const std::string &site, unsigned int def = 0);
	void SetLoginMode(const std::string &site, unsigned int value);

	std::string GetUsername(const std::string &site);
	void SetUsername(const std::string &site, const std::string &value);

	std::string GetPassword(const std::string &site);
	void SetPassword(const std::string &site, const std::string &value);

	std::string GetProtocolOptions(const std::string &site, const std::string &protocol);
	void SetProtocolOptions(const std::string &site, const std::string &protocol, const std::string &options);

	bool Transfer(SitesConfig &dst, const std::string &site, bool mv);
};
