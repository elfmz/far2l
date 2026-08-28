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

	WayToShellConfig(const std::string &ways_ini, const std::string &way_name);
	~WayToShellConfig();

	std::string OptionValue(unsigned index, const StringConfig &protocol_options) const;
};
