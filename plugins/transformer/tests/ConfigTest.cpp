#include "Config.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

int main(int argc, char **argv)
{
	assert(argc == 2);
	const auto presets = Transformer::LoadFormatDirectories({argv[1]});
	assert(presets.diagnostics.empty());
	bool has_preset = false;
	for (const auto &group : presets.groups) {
		has_preset = has_preset || !group.items.empty();
	}
	assert(has_preset);

	auto temporary = fs::temp_directory_path() / "far2l-transformer-config-test";
	fs::remove_all(temporary);
	fs::create_directories(temporary);
	{
		std::ofstream file(temporary / "custom.ini", std::ios::binary);
		file << "[FileType \"Test\"]\n"
			 << "Masks=*.test\n"
			 << "Item=First\n"
			 << "Command=tr a b\n"
			 << "Item=Second\n"
			 << "Command=\"sed 's/a/b/'\"\n"
			 << "Output=Viewer\n";
	}
	const auto custom = Transformer::LoadFormatDirectories({temporary.string()});
	assert(custom.diagnostics.empty());
	assert(custom.groups.size() == 1);
	assert(custom.groups[0].items.size() == 2);
	assert(custom.groups[0].items[0].name == L"First");
	assert(custom.groups[0].items[0].output_mode == Transformer::OutputMode::Replace);
	assert(custom.groups[0].items[1].command == "sed 's/a/b/'");
	assert(custom.groups[0].items[1].output_mode == Transformer::OutputMode::Viewer);
	assert(Transformer::MatchGroup(custom.groups[0], {L"/tmp/example.test"}));
	assert(!Transformer::MatchGroup(custom.groups[0], {L"/tmp/example.txt"}));
	fs::remove_all(temporary);
	return 0;
}
