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
	std::wstring id;        // unique identifier: path to the .desktop or .app
	bool terminal;          // whether a terminal is required or not.
};

