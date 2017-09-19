#include "Dumper.h"
#include "Globals.h"
#include <KeyFileHelper.h>
#include <utils.h>
#include <sudo.h>
#include <fcntl.h>
///#include <unistd.h>

static void ExecuteCommand(const std::string &cmd, const std::string &name, const std::string &result_file)
{
	std::string expanded_cmd = cmd;
	size_t p = expanded_cmd.find("$F");
	if (p != std::string::npos) {
		std::string quoted_file("\"");
		quoted_file+= EscapeQuotas(name);
		quoted_file+= "\"";
		expanded_cmd.replace(p, 2, quoted_file);
	}

	fprintf(stderr, "EXECUTING: %s\n", expanded_cmd.c_str());
	int fdo = sdc_open(result_file.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0644);
	if (fdo != -1) {
		FILE *fi = popen(expanded_cmd.c_str(), "r");
		if (fi) {
			char buf[0x400];
			for (;;) {
				size_t n = fread(buf, 1, sizeof(buf), fi);
				if (!n) break;
				if (sdc_write(fdo, buf, n) != (ssize_t)n) break;
			}
			if (pclose(fi) != 0) {
				std::string error_message = "Error executing: ";
				error_message+= expanded_cmd;
				sdc_write(fdo, error_message.c_str(), error_message.size());
			}
		} else {
			std::string error_message = "Failed to execute: ";
			error_message+= expanded_cmd;
			sdc_write(fdo, error_message.c_str(), error_message.size());
		}
		close(fdo);
	}
}

namespace Common
{
	static void Commands(const char *section, std::set<std::string> &out)
	{
		for (const auto &config : G.configs) {
			KeyFileHelper kfh(config.c_str());
			const std::vector<std::string> &sections = kfh.EnumKeys(section);
			out.insert(sections.begin(), sections.end());
		}
	}

	void Query(const char *section, const std::string &command, const std::string &name, const std::string &result_file)
	{
		for (const auto &config : G.configs) {
			KeyFileHelper kfh(config.c_str());
			const std::string &cmd = kfh.GetString(section, command.c_str());
			if (!cmd.empty()) {
				ExecuteCommand(cmd, name, result_file);
				return;
			}
		}
		fprintf(stderr, "Common::Query('%s', '%s'): no command\n", section, command.c_str());
	}
}

namespace Root
{
	void Commands(std::set<std::string> &out)
	{
		Common::Commands("Root", out);
	}

	void Query(const std::string &command, const std::string &name, const std::string &result_file)
	{
		Common::Query("Root", command, name, result_file);
	}
}

namespace Disasm
{
	void Commands(uint16_t machine, std::set<std::string> &out)
	{
		char section[256] = {};
		snprintf(section, sizeof(section) - 1, "Disasm_%u", (unsigned)machine);
		Common::Commands(section, out);
	}

	void Query(uint16_t machine, const std::string &command, const std::string &name, const std::string &result_file)
	{
		char section[256] = {};
		snprintf(section, sizeof(section) - 1, "Disasm_%u", (unsigned)machine);
		Common::Query(section, command, name, result_file);
	}

	void Store(uint16_t machine, const std::string &command, const std::string &name, const std::string &result_file)
	{
	}
}
