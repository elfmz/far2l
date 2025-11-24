#include "headers.hpp"
#include "VTShellBackend.h"
#include "lang.hpp"
#include "config.hpp"
#include <utils.h>
#include <RandomString.h>
#include <TestPath.h>

static std::string VT_ComposeInitialTitleCommand(const char *cd, const char *cmd, bool using_sudo)
{
	std::string title = cmd;
	StrTrim(title);

	if (StrStartsFrom(title, "sudo ")) {
		using_sudo = true;
		title.erase(0, 5);
		StrTrim(title);
	}

	if (title.size() > 2 && (title[0] == '\'' || title[0] == '\"')) {
		size_t p = title.find(title[0], 1);
		if (p != std::string::npos) {
			title = title.substr(1, p - 1);
		}

	} else {
		size_t p = title.find(' ');
		if (p != std::string::npos) {
			title.resize(p);
		}
	}

	size_t p = title.rfind('/');
	if (p != std::string::npos) {
		title.erase(0, p + 1);
	}

	title+= '@';
	title+= cd;

	if (using_sudo) {
		title.insert(0, "sudo ");
	}

	std::string out = "printf '\\033]2;";

	for (auto &ch : title) {
		if ((ch >= 0 && ch < 0x20)) {
			out+= '\x01';

		} else if (ch == '\'') {
			out+= "'\\''";

		} else {
			out+= ch;
		}
	}
	out+= "\\007'\n";

	return out;
}

static std::string VT_ComposeMarkerCommand(const std::string &marker)
{
	// marker contains $FARVTRESULT and thus must be in double quotes
	std::string out = "printf '\\033_far2l_%s\\007' \"";
	out+= marker;
	out+= "\"";
	return out;
}

// -----------------------------------------------------------------------
// Bash Backend Implementation
// -----------------------------------------------------------------------

class VTShellBackendBash : public IVTShellBackend
{
	static bool s_shown_tip_exit;

public:
	virtual std::string GetName() const override { return "bash"; }
	virtual std::string GetExecPath() const override { return "bash"; }

	virtual std::vector<std::string> GetStartArgs(bool interactive, bool no_profile) const override
	{
		std::vector<std::string> args;
		if (no_profile) args.emplace_back("--noprofile");
		if (interactive) args.emplace_back("-i");
		return args;
	}

	virtual void SetupEnvironment(std::map<std::string, std::string>& env) const override
	{
		// Avoid adding commands starting with space to history
		const char *hc = getenv("HISTCONTROL");
		if (!hc || (!strstr(hc, "ignorespace") && !strstr(hc, "ignoreboth"))) {
			std::string hc_override = "ignorespace";
			if (hc && *hc) {
				hc_override+= ':';
				hc_override+= hc;
			}
			env["HISTCONTROL"] = hc_override;
		}
	}

	virtual std::string MakeScript(const char *cd, const char *cmd, bool need_sudo,
		const std::string &start_marker, const std::string &pwd_file,
		const std::string &exit_marker, const std::string &user_profile) const override
	{
		std::string content;
		content+= "trap \"printf ''\" INT\n"; // need marker to be printed even after Ctrl+C pressed
		content+= "PS1=''; PS2=''; PS3=''; PS4=''; PROMPT_COMMAND=''\n"; // reduce risk of glitches

		// Sourcing directive (dot) with space trailing to avoid adding it to history
		content+= " . ";
		if (!user_profile.empty()) {
			std::string profile_path = user_profile;
			profile_path.insert(0, 1, '/');
			profile_path.insert(0, GetMyHome());
			if (TestPath(profile_path).Exists()) {
				content+= "\"";
				content+= profile_path;
				content+= "\";. ";
			}
		}

		if (strcmp(cmd, "exit")!=0) {
			content+= VT_ComposeInitialTitleCommand(cd, cmd, need_sudo);
		}
		content+= VT_ComposeMarkerCommand(start_marker);
		content+= "\n";

		if (strcmp(cmd, "exit")==0) {
			content+= StrPrintf(
				"echo \"%ls%ls%ls\"\n",  Msg::VTStop.CPtr(),
				s_shown_tip_exit ? L"" : L" ",
				s_shown_tip_exit ? L"" : Msg::VTStopTip.CPtr()
			);
			s_shown_tip_exit = true;
		}

		std::string pwd_suffix;
		const char *last_ch = cmd + strlen(cmd);
		while (last_ch != cmd && (*last_ch == ' ' || *last_ch == '\t' || *last_ch == 0)) {
			--last_ch;
		}

		if (*last_ch != '&') { // don't update curdir in case of background command
			pwd_suffix = StrPrintf(" && pwd >'%s'", pwd_file.c_str());
		}

		if (need_sudo) {
			content+= Opt.SudoEnabled ? "sudo -A " : "sudo ";
			content+= StrPrintf("sh -c \"cd \\\"%s\\\" && %s%s\"\n",
				EscapeEscapes(EscapeCmdStr(cd)).c_str(), EscapeCmdStr(cmd).c_str(), pwd_suffix.c_str());
		} else {
			content+= StrPrintf("cd \"%s\" && %s%s\n",
				EscapeCmdStr(cd).c_str(), cmd, pwd_suffix.c_str());
		}

		content+= "FARVTRESULT=$?\n"; // it will be echoed to caller from outside

		static std::string vthook = InMyConfig("/vtcmd.sh");
		if (TestPath(vthook).Exists()) {
			content.append(vthook)
				.append((need_sudo && !StrStartsFrom(cmd, "sudo ")) ? " sudo " : " ")
					.append(cmd).append("\n");
		}

		content+= "cd ~\n"; // avoid locking arbitrary directory

		if (Opt.CmdLine.Splitter) {
			content+= "if [ $FARVTRESULT -eq 0 ]; then\n";
			content+= "printf '\\r\\033_push-attr\\007\\033_set-blank=-\\007\\033[32m\\033[K\\033_pop-attr\\007\\012'\n";
			content+= "else\n";
			content+= "printf '\\r\\033_push-attr\\007\\033_set-blank=~\\007\\033[33m\\033[K\\033_pop-attr\\007\\012'\n";
			content+= "fi\n";
		}

		content+= VT_ComposeMarkerCommand(exit_marker + "$FARVTRESULT");
		content+= '\n';

		return content;
	}

	virtual std::string MakeRunScriptCommand(const std::string &script_path) const override
	{
		// Space prefix to avoid history
		return " . " + EscapeCmdStr(script_path);
	}
	virtual std::string GetCompletorConfigContent() const override
	{
		return "set completion-query-items 0\n"
			"set completion-display-width 0\n"
			"set colored-stats off\n"
			"set colored-completion-prefix off\n"
			"set page-completions off\n"
			"set colored-stats off\n"
			"set colored-completion-prefix off\n"
			"set show-all-if-ambiguous off\n"
			"set show-all-if-unmodified off\n";
	}

	virtual std::string GetCompletorInitCommand(const std::string &config_path) const override
	{
		std::string sendline = " PS1=''; PS2=''; PS3=''; PS4=''; PROMPT_COMMAND=''";
		if (!config_path.empty()) {
			sendline+= "; bind -f \"";
			sendline+= config_path;
			sendline+= "\"";
		}
		sendline+= '\n';
		return sendline;
	}

	virtual std::string MakeCompletionCommand(const std::string &command) const override
	{
		// For bash we simulate user input followed by tabs
		return command + "\t\t";
	}
	virtual bool ParseCompletionOutput(const std::string &output, const std::string &command, std::vector<std::string> &possibilities) const override
	{
		std::string reply = output;
		// Bash/Readline echo logic
		size_t p = reply.find(command);
		if (p == std::string::npos || p + command.size() >= reply.size()) {
			return false;
		}
		reply.erase(0, p + command.size());
		if (StrEndsBy(reply, command.c_str())) {
			reply.resize(reply.size() - command.size());
		}

		for (;;) {
			size_t p = reply.find('\n');
			if (p == std::string::npos) break;

			std::string line = reply.substr(0, p);
			StrTrim(line, "\r");
			StrTrim(line, " ");

			if (!line.empty()) {
				possibilities.emplace_back(line);
			}

			reply.erase(0, p + 1);
		}
		return true;
	}
};

bool VTShellBackendBash::s_shown_tip_exit = false;


// -----------------------------------------------------------------------
// Fish Backend Implementation
// -----------------------------------------------------------------------

class VTShellBackendFish : public IVTShellBackend
{
	static bool s_shown_tip_exit;

public:
	virtual std::string GetName() const override { return "fish"; }
	virtual std::string GetExecPath() const override { return "fish"; }

	virtual std::vector<std::string> GetStartArgs(bool interactive, bool no_profile) const override
	{
		std::vector<std::string> args;
		if (no_profile) args.emplace_back("--no-config");
		if (interactive) args.emplace_back("-i");
		return args;
	}

	virtual void SetupEnvironment(std::map<std::string, std::string>& env) const override
	{
		// Fish usually manages history differently, but we can try to disable history file
		// for the session if needed, or rely on fish ignoring space-prefixed commands naturally
		// if configured. For now, we leave it as is.
	}

	virtual std::string MakeScript(const char *cd, const char *cmd, bool need_sudo,
		const std::string &start_marker, const std::string &pwd_file,
		const std::string &exit_marker, const std::string &user_profile) const override
	{
		std::string content;

		// Function to silence output/prompt and handle interrupts
		content+= "function fish_prompt; end\n";
		content+= "function fish_mode_prompt; end\n";
		content+= "function fish_right_prompt; end\n";

		// Trap INT (Ctrl+C) - in fish we use 'function fish_user_key_bindings; bind \cc ...; end'
		// or handle status after execution. Fish handling of traps is different.
		// For simplicity in this wrapper, we rely on markers.

		// Sourcing logic (dot)
		// Fish supports '.' since version 2.something, but 'source' is more canonical.
		// However, '.' works fine in modern fish.

		if (!user_profile.empty()) {
			std::string profile_path = user_profile;
			profile_path.insert(0, 1, '/');
			profile_path.insert(0, GetMyHome());
			if (TestPath(profile_path).Exists()) {
				content+= "source \"";
				content+= profile_path;
				content+= "\"; ";
			}
		}

		if (strcmp(cmd, "exit")!=0) {
			content+= VT_ComposeInitialTitleCommand(cd, cmd, need_sudo);
		}

		// Print start marker
		content+= VT_ComposeMarkerCommand(start_marker);
		content+= "; printf '\\r'\n";

		if (strcmp(cmd, "exit")==0) {
			content+= StrPrintf(
				"echo \"%ls%ls%ls\"\n",  Msg::VTStop.CPtr(),
				s_shown_tip_exit ? L"" : L" ",
				s_shown_tip_exit ? L"" : Msg::VTStopTip.CPtr()
			);
			s_shown_tip_exit = true;
		}

		std::string pwd_suffix;
		const char *last_ch = cmd + strlen(cmd);
		while (last_ch != cmd && (*last_ch == ' ' || *last_ch == '\t' || *last_ch == 0)) {
			--last_ch;
		}

		if (*last_ch != '&') {
			// In fish redirection is same >
			// Use 'and' to execute pwd only if command succeeded, preserving error status otherwise
			pwd_suffix = StrPrintf("; and pwd >'%s'", pwd_file.c_str());
		}

		// Execution block
		// We use 'begin; ...; end' to group commands
		if (need_sudo) {
			// sudo in fish works same way
			content+= Opt.SudoEnabled ? "sudo -A " : "sudo ";
			content+= StrPrintf("fish -c \"cd \\\"%s\\\"; and %s%s\"\n",
				EscapeEscapes(EscapeCmdStr(cd)).c_str(), cmd, pwd_suffix.c_str()); // Assuming cmd is fish-compatible syntax
		} else {
			content+= StrPrintf("cd \"%s\"; and %s%s\n",
				EscapeCmdStr(cd).c_str(), cmd, pwd_suffix.c_str());
		}

		content+= "set FARVTRESULT $status\n";

		static std::string vthook = InMyConfig("/vtcmd.sh"); // .sh extension might be misleading for fish, but keeping legacy name
		if (TestPath(vthook).Exists()) {
			// Sourcing bash script in fish might fail.
			// We assume if user uses fish, they might provide compatible hook or we skip it.
			// For now, let's try to source it if it exists.
			content+= "source \"";
			content+= vthook;
			content+= "\"\n";
		}

		content+= "cd ~\n";

		if (Opt.CmdLine.Splitter) {
			content+= "if test $FARVTRESULT -eq 0\n";
			content+= "printf '\\r\\033_push-attr\\007\\033_set-blank=-\\007\\033[32m\\033[K\\033_pop-attr\\007\\012'\n";
			content+= "else\n";
			content+= "printf '\\r\\033_push-attr\\007\\033_set-blank=~\\007\\033[33m\\033[K\\033_pop-attr\\007\\012'\n";
			content+= "end\n";
		}

		content+= VT_ComposeMarkerCommand(exit_marker + "$FARVTRESULT");
		content+= '\n';

		return content;
	}

	virtual std::string MakeRunScriptCommand(const std::string &script_path) const override
	{
		// Space prefix to avoid history
		return " source " + EscapeCmdStr(script_path);
	}
	virtual std::string GetCompletorConfigContent() const override
	{
		// Fish doesn't use inputrc. We can return empty or specific fish config.
		// For now disabling completion for fish until separate logic is implemented.
		return "";
	}

	virtual std::string GetCompletorInitCommand(const std::string &config_path) const override
	{
		// Silence prompt
		return "function fish_prompt; end; function fish_mode_prompt; end; function fish_right_prompt; end\n";
	}

	virtual std::string MakeCompletionCommand(const std::string &command) const override
	{
		// For fish we use introspection command
		// Need to escape the command properly for the double-quoted string
		std::string escaped = command;
		// Simple escaping for double quotes and backslashes
		size_t pos = 0;
		while ((pos = escaped.find_first_of("\\\"", pos)) != std::string::npos) {
			escaped.insert(pos, 1, '\\');
			pos += 2;
		}
		return "complete -C \"" + escaped + "\"\n";
	}

	virtual bool ParseCompletionOutput(const std::string &output, const std::string &command, std::vector<std::string> &possibilities) const override
	{
		std::string reply = output;
		// Fish output format from 'complete -C' is lines of "suggestion\tdescription"

		// If reply contains the completion command itself, strip it
		std::string comp_cmd = MakeCompletionCommand(command);
		if (!comp_cmd.empty() && comp_cmd.back() == '\n') comp_cmd.pop_back();

		size_t p = reply.find(comp_cmd);
		if (p != std::string::npos) {
			reply.erase(0, p + comp_cmd.size());
		}

		for (;;) {
			size_t p = reply.find('\n');
			if (p == std::string::npos) break;

			std::string line = reply.substr(0, p);
			StrTrim(line, "\r");

			// Parse "candidate<TAB>description"
			size_t tab_pos = line.find('\t');
			if (tab_pos != std::string::npos) {
				line.resize(tab_pos);
			}

			if (!line.empty()) {
				possibilities.emplace_back(line);
			}

			reply.erase(0, p + 1);
		}
		return true;
	}
};

bool VTShellBackendFish::s_shown_tip_exit = false;
std::unique_ptr<IVTShellBackend> CreateVTShellBackend(const std::string &forced_shell)
{
	const char *env_shell = !forced_shell.empty() ? forced_shell.c_str() : getenv("SHELL");
	if (!env_shell || !*env_shell) {
		env_shell = "/bin/sh";
	}

	const char *slash = strrchr(env_shell, '/');
	const char *shell_name = slash ? slash + 1 : env_shell;

	// currently we only support bash logic, forcing bash for others
	// logic for fish/csh/etc will be added later

	if (strcmp(shell_name, "fish") == 0) return std::make_unique<VTShellBackendFish>();

	return std::make_unique<VTShellBackendBash>();
}