#pragma once
#include <string>
#include <vector>
namespace Environment
{
	// similar to getenv but provides extra resolution of 'special' variables that may miss in normal envs
	const char *GetVariable(const char *name);

	// performs shell-like unescaping and environment variables expansion
	bool ExpandString(std::string &s, bool empty_if_missing = false);

	// same as ExpandString but also provides vector of command-line arguments begins and ends
	struct Token
	{
		size_t begin;		// starting position of token in expanded string
		size_t len;			// length of token in expanded string
		size_t orig_begin;	// starting position of token in not expanded (original) string
		size_t orig_len;	// length of token in not expanded (original) string
	};

	typedef std::vector<Token> Tokens;
	bool ExpandAndTokenizeString(std::string &s, Tokens &tokens, bool empty_if_missing = false);

	struct ExpandAndExplodeString : std::vector<std::string>
	{
		ExpandAndExplodeString(const char *expression = nullptr);
		ExpandAndExplodeString(const std::string &expression);

		void Expand(std::string expression);
	};
}
