#pragma once
#include <string>

namespace openwith
{
	inline constexpr const char* INI_FILEPATH = "plugins/openwith/config.ini";

	struct Field
	{
		std::wstring label;
		std::wstring content;
	};


	struct CandidateInfo
	{
		std::wstring name;             // display name for the application selection menu
		std::wstring id;               // unique identifier: Desktop file ID or path to the .app
		bool terminal = false;         // whether a terminal is required or not
		bool multi_file_aware = false; // whether the application can open multiple files in a single invocation
	};


	struct CandidateContextLocation
	{
		std::wstring title;
		std::wstring target_filepath;
	};

} // namespace openwith
