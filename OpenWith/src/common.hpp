#pragma once

#include <string>

struct Field
{
	std::wstring label;
	std::wstring content;
};


struct CandidateInfo
{
	std::wstring name;      // display name for the application selection menu
	std::wstring id;        // unique identifier: Desktop file ID or path to the .app
	bool terminal;          // whether a terminal is required or not
	bool multi_file_aware = false;
};

