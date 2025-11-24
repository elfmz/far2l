#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>

// Abstract interface for shell backends
class IVTShellBackend
{
public:
	virtual ~IVTShellBackend() = default;

	// Process execution info
	virtual std::string GetName() const = 0; // e.g. "bash"
	virtual std::string GetExecPath() const = 0; // e.g. "/bin/bash" or just "bash"

	// Arguments to start the shell (e.g. {"-i"} or {"--noprofile", "-i"})
	virtual std::vector<std::string> GetStartArgs(bool interactive, bool no_profile) const = 0;

	// Environment variables setup (e.g. HISTCONTROL for bash)
	virtual void SetupEnvironment(std::map<std::string, std::string>& env) const = 0;

	// Script Generation
	virtual std::string MakeScript(const char *cd, const char *cmd, bool need_sudo,
		const std::string &start_marker, const std::string &pwd_file,
		const std::string &exit_marker, const std::string &user_profile) const = 0;

	// Completion support
	// Returns the content of config file (like inputrc) if needed, or empty string
	virtual std::string GetCompletorConfigContent() const = 0;
	// Returns commands to silence prompt and prepare shell for completion interaction
	virtual std::string GetCompletorInitCommand(const std::string &config_path) const = 0;

	// Generates the command string to request completions
	// For bash: just the command itself (tabs will be appended by caller/completor logic)
	// For fish: complete -C "command"
	virtual std::string MakeCompletionCommand(const std::string &command) const = 0;
	// Parse the raw output from the shell after sending the completion command
	virtual bool ParseCompletionOutput(const std::string &output, const std::string &command, std::vector<std::string> &possibilities) const = 0;
	// Generates the command to run/source the temporary script file
	// e.g. " . /tmp/script" for bash
	virtual std::string MakeRunScriptCommand(const std::string &script_path) const = 0;
};

// Factory
std::unique_ptr<IVTShellBackend> CreateVTShellBackend(const std::string &forced_shell = "");