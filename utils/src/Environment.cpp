#include "utils.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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

static void ReplaceSubstringAt(std::string &s, size_t start, size_t &edge, const char *value)
{
	size_t value_len = strlen(value);

	s.replace(start, edge - start, value, value_len);
	if (value_len < (edge - start)) {
		edge-= ((edge - start) - value_len);

	} else {
		edge+= (value_len - (edge - start));
	}
}

static void ReplaceSubstringAt(std::string &s, size_t start, size_t &edge, const std::string &value)
{
	ReplaceSubstringAt(s, start, edge, value.c_str());
}

static void ReplaceSubstringAt(std::string &s, size_t start, size_t &edge, char value)
{
	s.replace(start, edge - start, 1, value);
	if (1 < (edge - start)) {
		edge-= ((edge - start) - 1);

	} else {
		edge+= (1 - (edge - start));
	}
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
	ReplaceSubstringAt(s, start, edge, value);
	return out;
}

static void UnescapeCLikeSequence(std::string &s, size_t &i)
{
	// check {s[i - 1]=\, s[i]=..} for sequence encoded by EscapeLikeInC and reverse that encoding

	++i; // adjust i cuz ReplaceSubstringAt needs past-substring index

	if (i < s.size()) switch (s[i - 1]) {
		/// first check for trivial single-character sequences
		case 'a': ReplaceSubstringAt(s, i - 2, i, '\a'); break;
		case 'b': ReplaceSubstringAt(s, i - 2, i, '\b'); break;
		case 'e': ReplaceSubstringAt(s, i - 2, i, '\e'); break;
		case 'f': ReplaceSubstringAt(s, i - 2, i, '\f'); break;
		case 'n': ReplaceSubstringAt(s, i - 2, i, '\n'); break;
		case 'r': ReplaceSubstringAt(s, i - 2, i, '\r'); break;
		case 't': ReplaceSubstringAt(s, i - 2, i, '\t'); break;
		case 'v': ReplaceSubstringAt(s, i - 2, i, '\v'); break;
		case '\\': ReplaceSubstringAt(s, i - 2, i, '\\'); break;
		case '\'': ReplaceSubstringAt(s, i - 2, i, '\''); break;
		case '\"': ReplaceSubstringAt(s, i - 2, i, '\"'); break;
		case '?': ReplaceSubstringAt(s, i - 2, i, '?'); break;

		/// now check for multi-character codes
		// \x## where ## is a hexadecimal char code
		case 'x': if (i + 1 < s.size()) {
				unsigned long code = strtol(s.substr(i, 2).c_str(), nullptr, 16);
				i++;
				ReplaceSubstringAt(s, i - 4, i, StrPrintf("%c", (char)(unsigned char)code));
			} break;

		// \u#### where #### is a hexadecimal UTF16 code
		case 'u': if (i + 3 < s.size()) {
				unsigned long code = strtol(s.substr(i, 4).c_str(), nullptr, 16);
				i+= 3;
				ReplaceSubstringAt(s, i - 6, i, StrPrintf("%lc", (wchar_t)code));
			} break;

		// \u######## where ######## is a hexadecimal UTF32 code
		case 'U': if (i + 7 < s.size()) {
				unsigned long code = strtol(s.substr(i, 8).c_str(), nullptr, 16);
				i+= 7;
				ReplaceSubstringAt(s, i - 10, i, StrPrintf("%lc", (wchar_t)code));
			} break;

		// \### where ### is a octal char code
		case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
			if (i + 1 < s.size()) {
				unsigned long code = strtol(s.substr(i - 1, 3).c_str(), nullptr, 8);
				i++;
				ReplaceSubstringAt(s, i - 4, i, StrPrintf("%c", (char)(unsigned char)code));
			} break;
	}
	--i; // adjust i back
}

// nasty and allmighty function that actually implements ExpandString and ParseCommandLine
static bool ExpandStringOrParseCommandLine(std::string &s, Arguments *args, bool empty_if_missing)
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

	Quoting quot = QUOT_NONE;

	bool escaping_state = false;
	bool token_splitter = true;
	size_t i, orig_i, env_start;

	for (i = orig_i = env_start = 0; i < s.size(); ++i, ++orig_i) {
		if (args && env_state == ENV_NONE && !escaping_state) {
			// Check for operation and if so - grab it into dedicated token. Examples:
			//  foo&bar -> 'foo' '&' 'bar'
			//  foo & bar -> 'foo' '&' 'bar'
			//  foo& -> 'foo' '&'
			//  foo & -> 'foo' '&'
			size_t binops_count = 0;
			while (i + binops_count < s.size() && strchr("<>&|()", s[i + binops_count]) != nullptr) {
				++binops_count;
			}
			if (binops_count != 0) {
				if (!token_splitter) {
					args->back().len = i - args->back().begin;
					args->back().orig_len = orig_i - args->back().orig_begin;
				}
				args->emplace_back(Argument{i, binops_count, orig_i, binops_count, QUOT_NONE});
				token_splitter = true;
				i+= (binops_count - 1);
				orig_i+= (binops_count - 1);
				continue;
			}
		}

		if (token_splitter && s[i] != ' ') {
			token_splitter = false;
			if (args) {
				args->emplace_back(Argument{i, 0, orig_i, 0, QUOT_NONE});
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

		if (quot == QUOT_SINGLE) {
			if (s[i] == '\'') {
				quot = QUOT_NONE;
				s.erase(i, 1);
				--i;
			}

		} else if (escaping_state) {
			if (quot == QUOT_DOLLAR_SINGLE) {
				UnescapeCLikeSequence(s, i);

			} else if (quot == QUOT_NONE || s[i] == '"' || s[i] == '$') {
				s.erase(i - 1, 1);
				--i;
			}
			escaping_state = false;

		} else switch (s[i]) {
			case ' ': if (quot == QUOT_NONE && !token_splitter) {
				token_splitter = true;
				if (args && !args->empty()) {
					args->back().len = i - args->back().begin;
					args->back().orig_len = orig_i - args->back().orig_begin;
				}
			} break;

			case '\'': if (quot == QUOT_DOLLAR_SINGLE) {
				quot = QUOT_NONE;
				s.erase(i, 1);
				--i;

			} else if (quot != QUOT_DOUBLE && args) {
				quot = QUOT_SINGLE;
				s.erase(i, 1);
				--i;
			} break;

			case '\"': if (args) {
				quot = (quot == QUOT_DOUBLE) ? QUOT_NONE : QUOT_DOUBLE;
				s.erase(i, 1);
				--i;
			} break;

			case '\\': if (args || (i + 1 < s.size() && s[i + 1] == '$')) {
				escaping_state = true;
			} break;

			case '$': if (env_state == ENV_NONE && i + 1 < s.size()) {
				if (i + 2 < s.size() && s[i + 1] == '{') {
					env_state = ENV_CURLED;
					env_start = i;

				} else if (isalpha(s[i + 1])) {
					env_state = ENV_SIMPLE;
					env_start = i;

				} else if (s[i + 1] == '\'' && args) {
					s.erase(i, 2);
					i-= 2;
					quot = QUOT_DOLLAR_SINGLE;
				}

			} break;
		}

		if (args && !args->empty()) { // quot != QUOT_NONE &&
			args->back().quot = quot;
		}
	}

	if (!token_splitter && args && !args->empty()) {
		args->back().len = i - args->back().begin;
		args->back().orig_len = orig_i - args->back().orig_begin;
	}

	// fprintf(stderr, "ExpandString('%s', %d): '%s' [%d]\n", saved_s.c_str(), empty_if_missing, s.c_str(), out);
	return out;
}


bool ExpandString(std::string &s, bool empty_if_missing)
{
	return ExpandStringOrParseCommandLine(s, nullptr, empty_if_missing);
}

bool ParseCommandLine(std::string &s, Arguments &args, bool empty_if_missing)
{
	return ExpandStringOrParseCommandLine(s, &args, empty_if_missing);
}

///

ExplodeCommandLine::ExplodeCommandLine(const char *expression)
{
	if (expression) {
		Parse(expression);
	}
}

ExplodeCommandLine::ExplodeCommandLine(const std::string &expression)
{
	Parse(expression);
}

void ExplodeCommandLine::Parse(std::string expression)
{
	Arguments args;
	ExpandStringOrParseCommandLine(expression, &args, false);
	for (const auto &token : args) {
		emplace_back(expression.substr(token.begin, token.len));
	}
}

}
