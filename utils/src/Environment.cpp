#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "utils.h"


static char *GetHostNameCached()
{
	static char s_out[0x100] = {0};
	if (!s_out[0]) {
		gethostname(&s_out[1], sizeof(s_out) - 1);
		s_out[0] = 1;
	}
	return &s_out[1];
}

const char *GetEnvironmentString(const char *name)
{
	const char *out = getenv(name);
	if (out) {
		return out;
	}

	if (strcmp(name, "HOSTNAME") == 0) {
		return GetHostNameCached();
	}

	return nullptr;
}

static bool ReplaceEnvironmentSubString(std::string &s,
	size_t start, size_t &edge, const std::string &env, bool empty_if_missing)
{
	bool out = true;
	const char *value = GetEnvironmentString(env.c_str());
	if (!value) {
		if (!empty_if_missing) {
			return false;
		}
		out = false;
		value = "";
	}
	size_t value_len = strlen(value);

	s.replace(start, edge - start, value, value_len);
	if (value_len < (edge - start)) {
		edge-= ((edge - start) - value_len);

	} else {
		edge+= (value_len - (edge - start));
	}

	return out;
}

bool ExpandEnvironmentStrings(std::string &s, bool empty_if_missing)
{
	// Input example: ~/${EXPANDEDFOO}/$EXPANDEDBAR/unexpanded/part
	if (s.empty()) {
		return true;
	}

	bool out = true;
	std::string saved_s = s;
	enum State {
		S_NONE,
		S_PLAIN,
		S_CURLED
	} state = S_NONE;

	for (size_t i = 0, start = 0; i < s.size(); ++i) {
		if (state == S_PLAIN) {
			if (!isalnum(s[i]) && s[i] != '_') {
				if (!ReplaceEnvironmentSubString(s, start, i, s.substr(start + 1, i - start - 1), empty_if_missing)) {
					out = false;
				}
				state = S_NONE;
			}

		} else if (state == S_CURLED) {
			if (s[i] == '}') {
				++i;
				if (!ReplaceEnvironmentSubString(s, start, i, s.substr(start + 2, i - start - 3), empty_if_missing)) {
					out = false;
				}
				--i;
				state = S_NONE;
			}
		}

		if (state == S_NONE && s[i] == '$') {
			state = (i + 2 < s.size() && s[i + 1] == '{') ? S_CURLED : S_PLAIN;
			start = i;
		}
	}

	if (s[0] == '~' && (s.size() == 1 || s[1] == '/')) {
		s.replace(0, 1, GetMyHome());
	}

	// fprintf(stderr, "ExpandEnvironmentStrings('%s', %d): '%s' [%d]\n", saved_s.c_str(), empty_if_missing, s.c_str(), out);
	return out;
}
