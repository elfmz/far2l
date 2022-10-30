#pragma once
#include <string>
#include <vector>
namespace Environment
{
	void UnescapeCLikeSequences(std::string &s);

	// similar to getenv but provides extra resolution of 'special' variables that may miss in normal envs
	const char *GetVariable(const char *name);

	// performs only environment variables expansion and unescaping of only '$' character
	bool ExpandString(std::string &s, bool empty_if_missing, bool allow_exec_cmd = false);

	enum Quoting {
		QUOT_NONE,
		QUOT_SINGLE,
		QUOT_DOUBLE,
		QUOT_DOLLAR_SINGLE
	};

	struct Argument
	{
		size_t begin;		// starting position of argument in expanded string
		size_t len;			// length of argument in expanded string
		size_t orig_begin;	// starting position of argument in not expanded (original) string
		size_t orig_len;	// length of argument in not expanded (original) string
		Quoting quot;		// kind of quoting still pending within this argument
	};

	typedef std::vector<Argument> Arguments;

	// performs environment variables expansion, full-featured unescaping and command line
	// tokenization returning vector of command-line arguments begins and ends
	bool ParseCommandLine(std::string &s, Arguments &args, bool empty_if_missing, bool allow_exec_cmd = false);

	struct ExplodeCommandLine : std::vector<std::string>
	{
		ExplodeCommandLine(const char *expression = nullptr);
		ExplodeCommandLine(const std::string &expression);

		void Parse(std::string expression);
	};
}
