#include <fcntl.h>
#include "Dumper.h"
#include "Globals.h"
#include "Storage.h"
#include "Commands.h"
#include <sudo.h>

#define ELF_ROOT "ELFRoot"
#define ELF_DASM "Disasm_%u"

namespace CommandsELF
{
	void Query(const char *section, const std::string &command, const std::string &name, const std::string &result_file)
	{
		const std::string &cmd = Commands::Get(section, command.c_str());
		if (cmd.empty()) {
			fprintf(stderr,
				"CommandsELF::Query('%s', '%s'): no command\n", section, command.c_str());
			return;
		}

		if (!Storage::Get(name, cmd, result_file)) {
			Commands::Execute(cmd, name, result_file);
		}
	}

	void Store(const char *section, const std::string &command, const std::string &name, const std::string &result_file)
	{
		const std::string &cmd = Commands::Get(section, command.c_str());
		if (cmd.empty()) {
			fprintf(stderr,
				"CommandsELF::Store('%s', '%s'): no command\n", section, command.c_str());
			return;
		}

		Storage::Put(name, cmd, result_file);
	}

	void Clear(const char *section, const std::string &command, const std::string &name)
	{
		const std::string &cmd = Commands::Get(section, command.c_str());
		if (cmd.empty()) {
			fprintf(stderr,
				"CommandsELF::Clear('%s', '%s'): no command\n", section, command.c_str());
			return;
		}
		Storage::Clear(name, cmd);
	}
}

namespace Root
{
	void Commands(std::set<std::string> &out)
	{
		Commands::Enum(ELF_ROOT, out);
	}

	void Query(const std::string &command, const std::string &name, const std::string &result_file)
	{
		CommandsELF::Query(ELF_ROOT, command, name, result_file);
	}

	void Store(const std::string &command, const std::string &name, const std::string &result_file)
	{
		CommandsELF::Store(ELF_ROOT, command, name, result_file);
	}

	void Clear(const std::string &command, const std::string &name)
	{
		CommandsELF::Clear(ELF_ROOT, command, name);
	}
}

namespace Disasm
{
	void Commands(uint16_t machine, std::set<std::string> &out)
	{
		char section[256] = {};
		snprintf(section, sizeof(section) - 1, ELF_DASM, (unsigned)machine);
		Commands::Enum(section, out);
	}

	void Query(uint16_t machine, const std::string &command, const std::string &name, const std::string &result_file)
	{
		char section[256] = {};
		snprintf(section, sizeof(section) - 1, ELF_DASM, (unsigned)machine);
		CommandsELF::Query(section, command, name, result_file);
	}

	void Store(uint16_t machine, const std::string &command, const std::string &name, const std::string &result_file)
	{
		char section[256] = {};
		snprintf(section, sizeof(section) - 1, ELF_DASM, (unsigned)machine);
		CommandsELF::Store(section, command, name, result_file);
	}

	void Clear(uint16_t machine, const std::string &command, const std::string &name)
	{
		char section[256] = {};
		snprintf(section, sizeof(section) - 1, ELF_DASM, (unsigned)machine);
		CommandsELF::Clear(section, command, name);
	}
}

namespace Binary
{
	void Query(unsigned long long ofs, unsigned long long len, const std::string &name, const std::string &result_file)
	{
		int fdo = sdc_open(result_file.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0640);
		if (fdo != -1) {
			int fdi = sdc_open(name.c_str(), O_RDONLY);
			if (fdi != -1) {
				if (sdc_lseek(fdi, (off_t)ofs, SEEK_SET) != -1) {
					char buf[0x1000];
					while (len) {
						size_t piece = (sizeof(buf) < len) ? sizeof(buf) : (size_t)len;
						ssize_t r = sdc_read(fdi, buf, piece);
						if (r <= 0) break;
						if (sdc_write(fdo, buf, r) != r) break;
						len-= r;
					}
				}
				sdc_close(fdi);
			}
			sdc_close(fdo);
		}
	}
}
