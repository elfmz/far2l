#include "headers.hpp"
#include <algorithm>
#include <utils.h>
#include <KeyFileHelper.h>
#include "AllXLats.hpp"
#include "dirmix.hpp"

AllXlats::AllXlats()
		: std::vector<std::string>(KeyFileReadHelper(InMyConfig("xlats.ini")).EnumSections())
{
	const auto &xlats_global = KeyFileReadHelper(GetHelperPathName("xlats.ini")).EnumSections();
	for (const auto &xlat : xlats_global) { // local overrides global
		if (std::find(begin(), end(), xlat) == end()) {
			emplace_back(xlat);
		}
	}
}

