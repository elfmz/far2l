#pragma once
#include <string>
#include <vector>
#include <StringConfig.h>

struct WaysToShell : std::vector<std::string>
{
	WaysToShell(const std::string &ways_ini);
	~WaysToShell();
};

struct WayToShellConfig
{
	struct Option
	{
		struct Item
		{
			std::string info;
			std::string value;
		};

		std::string name;
		std::vector<Item> items;
		size_t def{0};
	};
	std::vector<Option> options;
	std::string command;
	std::string serial;

	// Which helper flavor this way lands on, when the way knows: "posix" for a
	// way that ends at a POSIX shell, "pwsh" for one that ends at a PowerShell
	// host. Empty means the way does not say - a hand-written way in a user's
	// ways.ini - and then the choice is left to the site's own Flavor setting.
	// Only FISH+ reads it; SHELL's ways.ini does not set it.
	std::string flavor;

	WayToShellConfig(const std::string &ways_ini, const std::string &way_name);
	~WayToShellConfig();

	std::string OptionValue(unsigned index, const StringConfig &protocol_options) const;
};
