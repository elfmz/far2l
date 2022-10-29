#include <stdio.h>
#include <string>
#include <vector>
#include <utils.h>

bool POpen(std::string &result, const char *command)
{
	FILE *f = popen(command, "r");
	if (!f) {
		perror("POpen: popen");
		return false;
	}

	char buf[0x400] = { };
	while (fgets(buf, sizeof(buf)-1, f)) {
		result+= buf;
	}
	pclose(f);
	return true;
}

bool POpen(std::vector<std::wstring> &result, const char *command)
{
	std::string tmp;
	bool out = POpen(tmp, command);

	for (size_t i = 0, ii = 0; i <= tmp.size(); ++i) {
		if (i == tmp.size() || tmp[i] == '\r' || tmp[i] == '\n') {
			if (i > ii) {
				result.emplace_back();
				StrMB2Wide(tmp.substr(ii, i - ii), result.back());
			}
			ii = i + 1;
		}
	}

	return out;
}
