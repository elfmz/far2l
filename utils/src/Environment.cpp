#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "Environment.h"

#if 1
# include "utils.h"
#else
static std::string GetMyHome()
{
	return "/home/user";
}
#endif

namespace Environment
{

static char *GetHostNameCached()
{
	static char s_out[0x100] = {0};
	if (!s_out[0]) {
		gethostname(&s_out[1], sizeof(s_out) - 1);
		s_out[0] = 1;
	}
	return &s_out[1];
}

const char *GetVariable(const char *name)
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

static bool ReplaceVariableAt(std::string &s,
	size_t start, size_t &edge, const std::string &env, bool empty_if_missing)
{
	bool out = true;
	const char *value = GetVariable(env.c_str());
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

static bool ExpandAndTokenizeStringInternal(std::string &s, Tokens *tokens, bool empty_if_missing)
{
	// Input example: ~/${EXPANDEDFOO}/$EXPANDEDBAR/unexpanded/part
	if (s.empty()) {
		return true;
	}

	bool out = true;
	// std::string saved_s = s;
	enum {
		ENV_NONE,
		ENV_SIMPLE,
		ENV_CURLED,
	} env_state = ENV_NONE;

	enum {
		QUOTE_NONE,
		QUOTE_SINGLE,
		QUOTE_DOUBLE,
	} quote_state = QUOTE_NONE;

	bool escaping_state = false;
	bool token_splitter = true;
	size_t i, orig_i, env_start;

	for (i = orig_i = env_start = 0; i < s.size(); ++i, ++orig_i) {
		if (token_splitter && s[i] != ' ') {
			token_splitter = false;
			if (tokens) {
				tokens->emplace_back(Token{i, 0, orig_i, 0});
			}
			if (s[i] == '~' && (i + 1 == s.size() || s[i + 1] == '/')) {
				const std::string &home = GetMyHome();
				if (!home.empty() || empty_if_missing) {
					s.replace(i, 1, home);
					i+= home.size();
					i--;
				}
			}
		}

		if (env_state == ENV_SIMPLE) {
			if (!isalnum(s[i]) && s[i] != '_') {
				if (!ReplaceVariableAt(s, env_start, i, s.substr(env_start + 1, i - env_start - 1), empty_if_missing)) {
					out = false;
				}
				env_state = ENV_NONE;
			}

		} else if (env_state == ENV_CURLED && s[i] == '}') {
			++i;
			if (!ReplaceVariableAt(s, env_start, i, s.substr(env_start + 2, i - env_start - 3), empty_if_missing)) {
				out = false;
			}
			--i;
			env_state = ENV_NONE;
		}

		if (quote_state == QUOTE_SINGLE) {
			if (s[i] == '\'') {
				quote_state = QUOTE_NONE;
				s.erase(i, 1);
				--i;
			}

		} else if (escaping_state) {
			if (quote_state == QUOTE_NONE || s[i] == '\"' || s[i] == '$') {
				s.erase(i - 1, 1);
				--i;
			}
			escaping_state = false;

		} else switch (s[i]) {
			case ' ': if (quote_state == QUOTE_NONE && !token_splitter) {
				token_splitter = true;
				if (tokens && !tokens->empty()) {
					tokens->back().len = i - tokens->back().begin;
					tokens->back().orig_len = orig_i - tokens->back().orig_begin;
				}
			} break;

			case '\'': if (quote_state != QUOTE_DOUBLE) {
				quote_state = QUOTE_SINGLE;
				s.erase(i, 1);
				--i;
			} break;

			case '\"': {
				quote_state = (quote_state == QUOTE_DOUBLE) ? QUOTE_NONE : QUOTE_DOUBLE;
				s.erase(i, 1);
				--i;
			} break;

			case '\\': {
				escaping_state = true;
			} break;

			case '$': if (env_state == ENV_NONE && i + 1 < s.size()) {
				if (i + 2 < s.size() && s[i + 1] == '{') {
					env_state = ENV_CURLED;
				} else if (isalpha(s[i + 1])) {
					env_state = ENV_SIMPLE;
				}
				env_start = i;
			} break;
		}
	}

	if (!token_splitter && tokens && !tokens->empty()) {
		tokens->back().len = i - tokens->back().begin;
		tokens->back().orig_len = orig_i - tokens->back().orig_begin;
	}

	// fprintf(stderr, "ExpandString('%s', %d): '%s' [%d]\n", saved_s.c_str(), empty_if_missing, s.c_str(), out);
	return out;
}


bool ExpandString(std::string &s, bool empty_if_missing)
{
	return ExpandAndTokenizeStringInternal(s, nullptr, empty_if_missing);
}

bool ExpandAndTokenizeString(std::string &s, Tokens &tokens, bool empty_if_missing)
{
	return ExpandAndTokenizeStringInternal(s, &tokens, empty_if_missing);
}

///

ExpandAndExplodeString::ExpandAndExplodeString(const char *expression)
{
	if (expression) {
		Expand(expression);
	}
}

ExpandAndExplodeString::ExpandAndExplodeString(const std::string &expression)
{
	Expand(expression);
}

void ExpandAndExplodeString::Expand(std::string expression)
{
	Tokens tokens;
	ExpandAndTokenizeStringInternal(expression, &tokens, false);
	for (const auto &token : tokens) {
		emplace_back(expression.substr(token.begin, token.len));
	}
}

}
