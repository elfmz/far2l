#include <fcntl.h>
#include <utils.h>
#include <KeyFileHelper.h>
#include "Globals.h"
#include "Commands.h"

namespace Commands
{
	void Enum(const char *section, std::set<std::string> &out)
	{
		for (const auto &config : G.configs) {
			KeyFileHelper kfh(config);
			const std::vector<std::string> &commands = kfh.EnumKeys(section);
			out.insert(commands.begin(), commands.end());
		}
	}


	std::string Get(const char *section, const std::string &name)
	{
		for (const auto &config : G.configs) {
			KeyFileHelper kfh(config);
			const std::string &cmd = kfh.GetString(section, name);
			if (!cmd.empty()) {
				return cmd;
			}
		}
		return std::string();
	}

	void Execute(const std::string &cmd, const std::string &name, const std::string &result_file)
	{
		std::string expanded_cmd = cmd;
		size_t p = expanded_cmd.find("$F");
		if (p != std::string::npos) {
			std::string quoted("\"");
			quoted+= EscapeCmdStr(name);
			quoted+= "\"";
			expanded_cmd.replace(p, 2, quoted);
		}

		p = expanded_cmd.find("$T");
		if (p != std::string::npos) {
			std::string quoted("\"");
			quoted+= EscapeCmdStr(result_file);
			quoted+= "\"";
			expanded_cmd.replace(p, 2, quoted);

			fprintf(stderr, "EXEC: %s\n", expanded_cmd.c_str());
			int r = system(expanded_cmd.c_str());
			if (r != 0) {
				int fdo = sdc_open(result_file.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0640);
				if (fdo != -1) {
					std::string error_message = "Failed to execute: ";
					error_message+= expanded_cmd;
					sdc_write(fdo, error_message.c_str(), error_message.size());
					sdc_close(fdo);
				}
			}

		} else {
			fprintf(stderr, "EXEC+STDOUT: %s\n", expanded_cmd.c_str());
			int fdo = sdc_open(result_file.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0640);
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
	}
}
