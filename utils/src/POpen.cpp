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

	char buf[0x1000];
	const size_t prev_result_len = result.size();
	while (fgets(buf, sizeof(buf), f)) {
		result+= buf;
	}
	const int rc = pclose(f);
	return rc == 0 || result.size() > prev_result_len;
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
