#include "Config.hpp"

#include <Escaping.h>
#include <KeyFileHelper.h>
#include <WideMB.h>
#include <farplug-wide.h>
#include <utils.h>

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace
{
	PluginStartupInfo g_info {};
	FarStandardFunctions g_fsf {};
	std::wstring g_module_name;

	enum MessageId
	{
		MTitle,
		MNoTransformations,
		MCommandFailed,
		MCommandFormat,
		MExitCodeFormat,
		MCannotReadFile,
		MCannotCreateOutput,
		MCannotCreateBackup,
		MCannotReplaceOriginal,
		MCannotModifyEditor,
		MNoRegularFiles,
		MTransformingFiles,
		MTransforming,
		MSettingsTitle,
		MCreateBackups,
		MOk,
		MCancel,
		MEditorLocked,
		MColumnSelectionUnsupported,
		MCommandSucceeded,
		MStandardOutput,
		MStandardError,
		MFileFormat,
		MSignalFormat,
		MCannotStartCommand,
		MConfigurationProblems,
		MEditorNulOutput,
		MCannotCreateViewer,
		MViewerResultFormat,
		MNoStandardOutput,
		MPluginMenu,
	};

	const wchar_t *Msg(MessageId id)
	{
		return g_info.GetMsg(g_info.ModuleNumber, static_cast<int>(id));
	}

	template <typename... Args>
	std::wstring FormatMessage(MessageId id, Args... args)
	{
		wchar_t buffer[4096] {};
		g_fsf.snprintf(buffer, std::size(buffer) - 1, Msg(id), args...);
		return buffer;
	}

	std::string ParentPath(const std::string &path)
	{
		const auto separator = path.find_last_of('/');
		if (separator == std::string::npos) {
			return ".";
		}
		return separator == 0 ? "/" : path.substr(0, separator);
	}

	std::vector<std::string> ResourceDirectories()
	{
		std::vector<std::string> result;
		std::set<std::string> seen;
		auto add = [&](const std::string &path) {
			if (!path.empty() && seen.emplace(path).second) {
				result.emplace_back(path);
			}
		};

		const std::string module_path = StrWide2MB(g_module_name);
		const std::string module_dir = ParentPath(module_path);
		add(module_dir);
		std::string shared_dir = module_dir;
		if (TranslateInstallPath_Lib2Share(shared_dir)) {
			add(shared_dir);
		}
		return result;
	}

	std::vector<std::string> FormatDirectories()
	{
		auto roots = ResourceDirectories();
		std::vector<std::string> result;
		result.reserve(roots.size() + 1);
		for (const auto &root : roots) {
			result.emplace_back(root + "/formats");
		}
		result.emplace_back(InMyConfig("plugins/transformer/formats", false));
		return result;
	}

	bool ParseBoolean(const std::string &value, bool fallback)
	{
		std::string lower;
		lower.reserve(value.size());
		for (unsigned char ch : value) {
			lower.push_back(ch >= 'A' && ch <= 'Z' ? static_cast<char>(ch + ('a' - 'A')) : ch);
		}
		if (lower == "1" || lower == "true" || lower == "yes" || lower == "on") {
			return true;
		}
		if (lower == "0" || lower == "false" || lower == "no" || lower == "off") {
			return false;
		}
		return fallback;
	}

	bool LoadBackupSetting()
	{
		bool backup = false;
		for (const auto &root : ResourceDirectories()) {
			KeyFileReadHelper settings(root + "/transformer.ini");
			if (settings.HasKey("Settings", "Backup")) {
				backup = ParseBoolean(settings.GetString("Settings", "Backup", "false"), backup);
			}
		}
		KeyFileReadHelper settings(InMyConfig("plugins/transformer/transformer.ini", false));
		if (settings.HasKey("Settings", "Backup")) {
			backup = ParseBoolean(settings.GetString("Settings", "Backup", "false"), backup);
		}
		return backup;
	}

	bool SaveBackupSetting(bool backup)
	{
		KeyFileHelper settings(InMyConfig("plugins/transformer/transformer.ini"), false);
		settings.SetInt("Settings", "Backup", backup ? 1 : 0);
		return settings.Save(false);
	}

	struct TempFile
	{
		std::string path;
		int fd = -1;

		TempFile() = default;
		TempFile(const TempFile &) = delete;
		TempFile &operator=(const TempFile &) = delete;

		TempFile(TempFile &&other) noexcept
			: path(std::move(other.path)), fd(std::exchange(other.fd, -1))
		{
		}

		TempFile &operator=(TempFile &&other) noexcept
		{
			if (this != &other) {
				Cleanup();
				path = std::move(other.path);
				fd = std::exchange(other.fd, -1);
			}
			return *this;
		}

		~TempFile()
		{
			Cleanup();
		}

		void Close()
		{
			if (fd != -1) {
				while (close(fd) == -1 && errno == EINTR) {
				}
				fd = -1;
			}
		}

		void Cleanup()
		{
			Close();
			if (!path.empty()) {
				unlink(path.c_str());
				path.clear();
			}
		}

		void Release()
		{
			Close();
			path.clear();
		}
	};

	bool CreateTempFile(const std::string &pattern, TempFile &file, int suffix_length = 0)
	{
		std::vector<char> buffer(pattern.begin(), pattern.end());
		buffer.push_back('\0');
		const int fd = suffix_length == 0
			? mkstemp(buffer.data())
			: mkstemps(buffer.data(), suffix_length);
		if (fd == -1) {
			return false;
		}
		file.path = buffer.data();
		file.fd = fd;
		return true;
	}

	bool WriteAllBytes(int fd, const char *data, std::size_t size)
	{
		while (size != 0) {
			const ssize_t written = write(fd, data, size);
			if (written > 0) {
				data += written;
				size -= static_cast<std::size_t>(written);
			} else if (written == -1 && errno == EINTR) {
				continue;
			} else {
				return false;
			}
		}
		return true;
	}

	struct ViewerRect
	{
		int x1 = -1;
		int y1 = -1;
		int x2 = -1;
		int y2 = -1;
	};

	ViewerRect CenteredViewerRect()
	{
		SMALL_RECT screen {};
		if (!g_info.AdvControl(g_info.ModuleNumber, ACTL_GETFARRECT, &screen, nullptr)) {
			return {};
		}

		const int width = static_cast<int>(screen.Right) - static_cast<int>(screen.Left) + 1;
		const int height = static_cast<int>(screen.Bottom) - static_cast<int>(screen.Top) + 1;
		if (width < 2 || height < 2) {
			return {};
		}

		const int removed_width = width / 10;
		const int removed_height = height / 10;
		const int left_margin = removed_width / 2;
		const int top_margin = removed_height / 2;
		return {
			static_cast<int>(screen.Left) + left_margin,
			static_cast<int>(screen.Top) + top_margin,
			static_cast<int>(screen.Right) - (removed_width - left_margin),
			static_cast<int>(screen.Bottom) - (removed_height - top_margin),
		};
	}

	void ShowMessageText(const std::wstring &text, bool warning = false)
	{
		g_info.Message(g_info.ModuleNumber,
			FMSG_LEFTALIGN | FMSG_ALLINONE | FMSG_MB_OK | (warning ? FMSG_WARNING : 0), L"Contents",
			reinterpret_cast<const wchar_t *const *>(text.c_str()), 0, 0);
	}

	bool ShowBytesInViewer(const char *bytes, std::size_t size, const std::wstring &title)
	{
		TempFile viewer_file;
		if (!CreateTempFile(InMyTemp("transformer/view-XXXXXX.ansi"), viewer_file, 5)
				|| !WriteAllBytes(viewer_file.fd, bytes, size)) {
			std::wstring fallback = Msg(MTitle);
			fallback += L'\n';
			fallback += Msg(MCannotCreateViewer);
			ShowMessageText(fallback, true);
			return false;
		}
		viewer_file.Close();
		const ViewerRect rect = CenteredViewerRect();
		return g_info.Viewer(StrMB2Wide(viewer_file.path).c_str(), title.c_str(),
			rect.x1, rect.y1, rect.x2, rect.y2, VF_DISABLEHISTORY, CP_UTF8) != 0;
	}

	void ShowText(const std::wstring &text)
	{
		std::string bytes;
		Wide2MB(text.data(), text.size(), bytes);
		ShowBytesInViewer(bytes.data(), bytes.size(), Msg(MTitle));
	}

	void ShowSimpleMessage(MessageId message)
	{
		std::wstring text = Msg(MTitle);
		text += L'\n';
		text += Msg(message);
		ShowMessageText(text, true);
	}

	bool ReadAllBytes(const std::string &path, std::vector<char> &data)
	{
		data.clear();
		int fd;
		do {
			fd = open(path.c_str(), O_RDONLY);
		} while (fd == -1 && errno == EINTR);
		if (fd == -1) {
			return false;
		}

		bool ok = true;
		char buffer[16384];
		for (;;) {
			const ssize_t count = read(fd, buffer, sizeof(buffer));
			if (count > 0) {
				data.insert(data.end(), buffer, buffer + count);
			} else if (count == 0) {
				break;
			} else if (errno != EINTR) {
				ok = false;
				break;
			}
		}
		while (close(fd) == -1 && errno == EINTR) {
		}
		return ok;
	}

	struct ExecuteResult
	{
		std::vector<char> output;
		std::vector<char> error;
		int exit_code = -1;
		int signal = 0;
		int launch_errno = 0;
		std::wstring internal_error;

		bool Succeeded() const
		{
			return launch_errno == 0 && internal_error.empty() && signal == 0 && exit_code == 0;
		}
	};

	ExecuteResult ExecuteFilter(const std::string &command, const std::vector<char> &input)
	{
		ExecuteResult result;
		TempFile input_file;
		TempFile output_file;
		TempFile error_file;
		const std::string temp_pattern = InMyTemp("transformer/filter-XXXXXX");

		if (!CreateTempFile(temp_pattern, input_file)
				|| !CreateTempFile(temp_pattern, output_file)
				|| !CreateTempFile(temp_pattern, error_file)) {
			result.internal_error = L"Cannot create command I/O files";
			return result;
		}
		if (!WriteAllBytes(input_file.fd, input.data(), input.size())) {
			result.internal_error = L"Cannot prepare command input";
			return result;
		}
		input_file.Close();
		output_file.Close();
		error_file.Close();

		std::string quoted_input = input_file.path;
		std::string quoted_output = output_file.path;
		std::string quoted_error = error_file.path;
		std::string quoted_command = command;
		QuoteCmdArg(quoted_input);
		QuoteCmdArg(quoted_output);
		QuoteCmdArg(quoted_error);
		QuoteCmdArg(quoted_command);
		const std::string shell_command = "/bin/sh -c " + quoted_command
			+ " transformer <" + quoted_input + " >" + quoted_output + " 2>" + quoted_error;
		const std::wstring wide_command = StrMB2Wide(shell_command);

		errno = 0;
		const int status = g_fsf.Execute(wide_command.c_str(), EF_HIDEOUT | EF_NOCMDPRINT);
		if (status == -1) {
			result.launch_errno = errno ? errno : EIO;
		} else if (WIFEXITED(status)) {
			result.exit_code = WEXITSTATUS(status);
		} else if (WIFSIGNALED(status)) {
			result.signal = WTERMSIG(status);
		} else {
			result.internal_error = L"Command returned an unknown process status";
		}

		if (!ReadAllBytes(output_file.path, result.output)
				|| !ReadAllBytes(error_file.path, result.error)) {
			result.internal_error = L"Cannot collect command output";
		}
		return result;
	}

	std::wstring BytesForDisplay(const std::vector<char> &bytes)
	{
		std::wstring result;
		MB2Wide(bytes.data(), bytes.size(), result);
		for (auto &ch : result) {
			if (ch == L'\0') {
				ch = L' ';
			}
		}
		return result;
	}

	void AppendCapturedOutput(std::wstring &report, const ExecuteResult &result)
	{
		if (!result.error.empty()) {
			report += L'\n';
			report += Msg(MStandardError);
			report += L"\n";
			report += BytesForDisplay(result.error);
		}
		if (!result.output.empty()) {
			report += L'\n';
			report += Msg(MStandardOutput);
			report += L"\n";
			report += BytesForDisplay(result.output);
		}
	}

	std::wstring CommandFailureReport(const Transformer::Transform &transform,
		const ExecuteResult &result, const std::wstring *file = nullptr)
	{
		std::wstring report = Msg(MCommandFailed);
		report += L'\n';
		report += transform.name;
		report += L'\n';
		report += FormatMessage(MCommandFormat, StrMB2Wide(transform.command).c_str());
		if (file) {
			report += L'\n';
			report += FormatMessage(MFileFormat, file->c_str());
		}
		if (result.launch_errno != 0) {
			report += L'\n';
			report += Msg(MCannotStartCommand);
			report += L": ";
			report += StrMB2Wide(strerror(result.launch_errno));
		} else if (result.signal != 0) {
			report += L'\n';
			report += FormatMessage(MSignalFormat, result.signal);
		} else if (result.exit_code >= 0) {
			report += L'\n';
			report += FormatMessage(MExitCodeFormat, result.exit_code);
		}
		if (!result.internal_error.empty()) {
			report += L'\n';
			report += result.internal_error;
		}
		AppendCapturedOutput(report, result);
		return report;
	}

	std::wstring CommandSuccessReport(const Transformer::Transform &transform,
		const ExecuteResult &result, const std::wstring *file = nullptr)
	{
		std::wstring report = Msg(MCommandSucceeded);
		report += L'\n';
		report += transform.name;
		if (file) {
			report += L'\n';
			report += FormatMessage(MFileFormat, file->c_str());
		}
		AppendCapturedOutput(report, result);
		return report;
	}

	bool ShowOutputViewer(const Transformer::Transform &transform,
		const ExecuteResult &result, const std::wstring *file = nullptr)
	{
		if (result.output.empty()) {
			std::wstring message = CommandSuccessReport(transform, result, file);
			message += L'\n';
			message += Msg(MNoStandardOutput);
			ShowMessageText(message, true);
			return false;
		}

		std::wstring title = Msg(MTitle);
		title += L": ";
		title += transform.name;
		title += L" — ";
		title += FormatMessage(MViewerResultFormat,
			static_cast<unsigned long long>(result.output.size()));
		if (file) {
			title += L" — ";
			title += *file;
		}
		return ShowBytesInViewer(result.output.data(), result.output.size(), title);
	}

	struct TextPosition
	{
		int line = 0;
		int column = 0;
	};

	bool operator<(const TextPosition &left, const TextPosition &right)
	{
		return left.line < right.line || (left.line == right.line && left.column < right.column);
	}

	bool operator==(const TextPosition &left, const TextPosition &right)
	{
		return left.line == right.line && left.column == right.column;
	}

	struct EditorSnapshot
	{
		EditorInfo info {};
		std::vector<char> input;
		bool has_selection = false;
		TextPosition start;
		TextPosition end;
		int last_line_length = 0;
	};

	void AppendWideText(std::vector<char> &target, const wchar_t *text, std::size_t length)
	{
		std::string converted;
		Wide2MB(text, length, converted);
		target.insert(target.end(), converted.begin(), converted.end());
	}

	bool ReadEditorSnapshot(EditorSnapshot &snapshot, std::wstring &error)
	{
		if (!g_info.EditorControl(ECTL_GETINFO, &snapshot.info)) {
			error = Msg(MCannotModifyEditor);
			return false;
		}
		if (snapshot.info.CurState & ECSTATE_LOCKED) {
			error = Msg(MEditorLocked);
			return false;
		}
		if (snapshot.info.BlockType == BTYPE_COLUMN) {
			error = Msg(MColumnSelectionUnsupported);
			return false;
		}

		snapshot.has_selection = snapshot.info.BlockType == BTYPE_STREAM;
		if (snapshot.has_selection) {
			bool first = true;
			for (int line = snapshot.info.BlockStartLine; line < snapshot.info.TotalLines; ++line) {
				EditorGetString string {};
				string.StringNumber = line;
				if (!g_info.EditorControl(ECTL_GETSTRING, &string) || string.SelStart < 0) {
					break;
				}
				const int begin = std::clamp(string.SelStart, 0, string.StringLength);
				const bool through_eol = string.SelEnd == -1 || string.SelEnd > string.StringLength;
				const int end = through_eol ? string.StringLength
					: std::clamp(string.SelEnd, begin, string.StringLength);
				if (first) {
					snapshot.start = {line, begin};
					first = false;
				}
				AppendWideText(snapshot.input, string.StringText + begin,
					static_cast<std::size_t>(end - begin));
				if (through_eol) {
					snapshot.input.push_back('\n');
					snapshot.end = {line + 1, 0};
				} else {
					snapshot.end = {line, end};
				}
			}
			if (first) {
				error = Msg(MCannotModifyEditor);
				return false;
			}
			return true;
		}

		snapshot.start = {0, 0};
		for (int line = 0; line < snapshot.info.TotalLines; ++line) {
			EditorGetString string {};
			string.StringNumber = line;
			if (!g_info.EditorControl(ECTL_GETSTRING, &string)) {
				error = Msg(MCannotModifyEditor);
				return false;
			}
			AppendWideText(snapshot.input, string.StringText,
				static_cast<std::size_t>(string.StringLength));
			snapshot.last_line_length = string.StringLength;
			if (string.StringEOL && *string.StringEOL) {
				snapshot.input.push_back('\n');
			}
		}
		snapshot.end = {std::max(0, snapshot.info.TotalLines - 1), snapshot.last_line_length};
		return true;
	}

	std::wstring NormalizeEditorOutput(const std::vector<char> &output)
	{
		std::wstring decoded;
		MB2Wide(output.data(), output.size(), decoded);
		std::wstring normalized;
		normalized.reserve(decoded.size());
		for (std::size_t i = 0; i < decoded.size(); ++i) {
			if (decoded[i] == L'\r') {
				if (i + 1 < decoded.size() && decoded[i + 1] == L'\n') {
					++i;
				}
				normalized.push_back(L'\r');
			} else if (decoded[i] == L'\n') {
				normalized.push_back(L'\r');
			} else {
				normalized.push_back(decoded[i]);
			}
		}
		return normalized;
	}

	TextPosition InsertedEnd(const TextPosition &start, const std::wstring &text)
	{
		TextPosition end = start;
		for (wchar_t ch : text) {
			if (ch == L'\r') {
				++end.line;
				end.column = 0;
			} else {
				++end.column;
			}
		}
		return end;
	}

	TextPosition MapSelectionCursor(const EditorSnapshot &snapshot, const TextPosition &new_end)
	{
		const TextPosition cursor {snapshot.info.CurLine, snapshot.info.CurPos};
		if (!snapshot.has_selection || cursor < snapshot.start) {
			return cursor;
		}
		if (cursor == snapshot.start) {
			return snapshot.start;
		}
		if (!(cursor < snapshot.end)) {
			if (cursor.line == snapshot.end.line) {
				return {new_end.line, new_end.column + cursor.column - snapshot.end.column};
			}
			return {cursor.line + new_end.line - snapshot.end.line, cursor.column};
		}
		return new_end;
	}

	TextPosition ClampEditorPosition(TextPosition position, const EditorInfo &info)
	{
		position.line = std::clamp(position.line, 0, std::max(0, info.TotalLines - 1));
		EditorGetString string {};
		string.StringNumber = position.line;
		if (g_info.EditorControl(ECTL_GETSTRING, &string)) {
			position.column = std::clamp(position.column, 0, string.StringLength);
		} else {
			position.column = 0;
		}
		return position;
	}

	bool ApplyEditorOutput(const EditorSnapshot &snapshot, const std::vector<char> &output)
	{
		const std::wstring text = NormalizeEditorOutput(output);
		const TextPosition new_end = InsertedEnd(snapshot.start, text);

		EditorUndoRedo undo {};
		undo.Command = EUR_BEGIN;
		if (!g_info.EditorControl(ECTL_UNDOREDO, &undo)) {
			return false;
		}

		bool changed = true;
		if (snapshot.has_selection) {
			changed = g_info.EditorControl(ECTL_DELETEBLOCK, nullptr) != 0;
		} else if (!(snapshot.info.TotalLines == 1 && snapshot.last_line_length == 0
				&& snapshot.input.empty())) {
			EditorSelect all {};
			all.BlockType = BTYPE_STREAM;
			all.BlockStartLine = 0;
			all.BlockStartPos = 0;
			all.BlockWidth = snapshot.last_line_length;
			all.BlockHeight = snapshot.info.TotalLines;
			changed = g_info.EditorControl(ECTL_SELECT, &all) != 0
				&& g_info.EditorControl(ECTL_DELETEBLOCK, nullptr) != 0;
		}

		EditorSetPosition insertion {};
		insertion.CurLine = snapshot.start.line;
		insertion.CurPos = snapshot.start.column;
		insertion.CurTabPos = -1;
		insertion.TopScreenLine = -1;
		insertion.LeftPos = -1;
		insertion.Overtype = 0;
		changed = changed && g_info.EditorControl(ECTL_SETPOSITION, &insertion) != 0;
		if (changed && !text.empty()) {
			changed = g_info.EditorControl(ECTL_INSERTTEXT,
				const_cast<wchar_t *>(text.c_str())) != 0;
		}

		undo.Command = EUR_END;
		g_info.EditorControl(ECTL_UNDOREDO, &undo);
		if (!changed) {
			undo.Command = EUR_UNDO;
			g_info.EditorControl(ECTL_UNDOREDO, &undo);
			return false;
		}

		EditorInfo after {};
		g_info.EditorControl(ECTL_GETINFO, &after);
		TextPosition cursor = snapshot.has_selection
			? MapSelectionCursor(snapshot, new_end)
			: TextPosition {snapshot.info.CurLine, snapshot.info.CurPos};
		cursor = ClampEditorPosition(cursor, after);
		EditorSetPosition restore {};
		restore.CurLine = cursor.line;
		restore.CurPos = cursor.column;
		restore.CurTabPos = -1;
		restore.TopScreenLine = std::clamp(snapshot.info.TopScreenLine, 0,
			std::max(0, after.TotalLines - 1));
		restore.LeftPos = snapshot.info.LeftPos;
		restore.Overtype = snapshot.info.Overtype;
		g_info.EditorControl(ECTL_SETPOSITION, &restore);

		if (snapshot.has_selection && !text.empty()) {
			EditorSelect selection {};
			selection.BlockType = BTYPE_STREAM;
			selection.BlockStartLine = snapshot.start.line;
			selection.BlockStartPos = snapshot.start.column;
			selection.BlockWidth = new_end.column - snapshot.start.column;
			selection.BlockHeight = new_end.line - snapshot.start.line + 1;
			g_info.EditorControl(ECTL_SELECT, &selection);
		} else {
			EditorSelect none {};
			none.BlockType = BTYPE_NONE;
			none.BlockStartPos = -1;
			g_info.EditorControl(ECTL_SELECT, &none);
		}
		g_info.EditorControl(ECTL_REDRAW, nullptr);
		return true;
	}

	std::wstring EditorFilename()
	{
		const int size = g_info.EditorControl(ECTL_GETFILENAME, nullptr);
		if (size <= 1) {
			return {};
		}
		std::vector<wchar_t> filename(static_cast<std::size_t>(size));
		if (!g_info.EditorControl(ECTL_GETFILENAME, filename.data())) {
			return {};
		}
		return filename.data();
	}

	bool CreateSiblingTemp(const std::string &original, TempFile &file)
	{
		const std::string directory = ParentPath(original);
		return CreateTempFile(directory + "/.transformer-XXXXXX", file);
	}

	bool StageFile(const std::string &original, const std::vector<char> &contents,
		mode_t mode, TempFile &staged)
	{
		if (!CreateSiblingTemp(original, staged)) {
			return false;
		}
		if (fchmod(staged.fd, mode & 07777) == -1
				|| !WriteAllBytes(staged.fd, contents.data(), contents.size())
				|| fsync(staged.fd) == -1) {
			return false;
		}
		staged.Close();
		return true;
	}

	bool ReplacePanelFile(const std::wstring &wide_path, const std::vector<char> &original,
		const std::vector<char> &output, const struct stat &metadata, bool backup,
		std::wstring &error)
	{
		const std::string path = StrWide2MB(wide_path);
		TempFile staged_output;
		if (!StageFile(path, output, metadata.st_mode, staged_output)) {
			error = FormatMessage(MCannotCreateOutput, wide_path.c_str());
			return false;
		}

		if (backup) {
			TempFile staged_backup;
			if (!StageFile(path, original, metadata.st_mode, staged_backup)) {
				error = FormatMessage(MCannotCreateBackup, wide_path.c_str());
				return false;
			}
			const std::string backup_path = path + ".bak";
			if (rename(staged_backup.path.c_str(), backup_path.c_str()) == -1) {
				error = FormatMessage(MCannotCreateBackup, wide_path.c_str());
				return false;
			}
			staged_backup.Release();
		}

		if (rename(staged_output.path.c_str(), path.c_str()) == -1) {
			error = FormatMessage(MCannotReplaceOriginal, wide_path.c_str());
			return false;
		}
		staged_output.Release();
		return true;
	}

	struct PanelSelection
	{
		std::vector<std::wstring> paths;
		std::vector<std::wstring> filenames;
	};

	bool IsAbsolutePath(const std::wstring &path)
	{
		return !path.empty() && path.front() == L'/';
	}

	PanelSelection GetPanelSelection()
	{
		PanelSelection result;
		PanelInfo panel {};
		if (!g_info.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0,
				static_cast<LONG_PTR>(reinterpret_cast<intptr_t>(&panel)))
				|| panel.PanelType != PTYPE_FILEPANEL || panel.ItemsNumber <= 0
				|| (panel.Plugin && !(panel.Flags & PFLAGS_REALNAMES))) {
			return result;
		}

		const int directory_size = g_info.Control(PANEL_ACTIVE, FCTL_GETPANELDIR, 0, 0);
		if (directory_size <= 0) {
			return result;
		}
		std::vector<wchar_t> directory(static_cast<std::size_t>(directory_size));
		if (!g_info.Control(PANEL_ACTIVE, FCTL_GETPANELDIR, directory_size,
				static_cast<LONG_PTR>(reinterpret_cast<intptr_t>(directory.data())))) {
			return result;
		}
		std::wstring prefix(directory.data());
		if (!prefix.empty() && prefix.back() != L'/') {
			prefix.push_back(L'/');
		}

		std::set<std::wstring> unique_paths;
		for (int index = 0; index < panel.SelectedItemsNumber; ++index) {
			const int size = g_info.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, index, 0);
			if (size <= 0) {
				continue;
			}
			std::vector<unsigned char> storage(static_cast<std::size_t>(size));
			auto *item = reinterpret_cast<PluginPanelItem *>(storage.data());
			if (!g_info.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, index,
					static_cast<LONG_PTR>(reinterpret_cast<intptr_t>(item)))
					|| !item->FindData.lpwszFileName
					|| (item->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				continue;
			}
			const std::wstring filename(item->FindData.lpwszFileName);
			if (filename == L"." || filename == L"..") {
				continue;
			}
			const std::wstring path = IsAbsolutePath(filename) ? filename : prefix + filename;
			if (unique_paths.emplace(path).second) {
				result.paths.emplace_back(path);
				result.filenames.emplace_back(filename);
			}
		}
		return result;
	}

	struct MenuRow
	{
		std::wstring text;
		const Transformer::Transform *transform = nullptr;
		bool heading = false;
	};

	bool CaseInsensitiveLess(const std::wstring &left, const std::wstring &right)
	{
		return g_fsf.LStricmp(left.c_str(), right.c_str()) < 0;
	}

	const Transformer::Transform *ChooseTransform(const Transformer::Config &config,
		const std::vector<std::wstring> &filenames)
	{
		if (!config.diagnostics.empty()) {
			std::wstring report = Msg(MTitle);
			report += L'\n';
			report += Msg(MConfigurationProblems);
			for (const auto &diagnostic : config.diagnostics) {
				report += L'\n';
				report += diagnostic;
			}
			ShowText(report);
		}

		std::vector<const Transformer::Group *> specific;
		std::vector<const Transformer::Group *> common;
		std::vector<const Transformer::Group *> other;
		for (const auto &group : config.groups) {
			if (group.items.empty()) {
				continue;
			}
			if (group.common) {
				common.emplace_back(&group);
			} else if (!group.masks.empty() && Transformer::MatchGroup(group, filenames)) {
				specific.emplace_back(&group);
			} else {
				other.emplace_back(&group);
			}
		}
		std::sort(other.begin(), other.end(), [](const auto *left, const auto *right) {
			return CaseInsensitiveLess(left->name, right->name);
		});

		std::vector<MenuRow> rows;
		auto append_groups = [&](const auto &groups) {
			for (const auto *group : groups) {
				rows.push_back({group->name, nullptr, true});
				std::size_t label_width = 0;
				for (const auto &transform : group->items) {
					label_width = std::max(label_width, transform.name.size());
				}
				for (const auto &transform : group->items) {
					std::wstring text = L"  " + transform.name;
					text.append(label_width - transform.name.size() + 4, L' ');
					text += StrMB2Wide(transform.command);
					rows.push_back({std::move(text), &transform, false});
				}
			}
		};
		append_groups(specific);
		append_groups(common);
		append_groups(other);

		if (rows.empty()) {
			std::wstring report = Msg(MTitle);
			report += L'\n';
			report += Msg(MNoTransformations);
			ShowText(report);
			return nullptr;
		}

		std::vector<FarMenuItemEx> items;
		items.reserve(rows.size());
		for (const auto &row : rows) {
			FarMenuItemEx item {};
			item.Flags = row.heading ? MIF_SEPARATOR : MIF_NONE;
			item.Text = row.text.c_str();
			items.emplace_back(item);
		}
		const int selected = g_info.Menu(g_info.ModuleNumber, -1, -1, 0,
			FMENU_USEEXT | FMENU_WRAPMODE | FMENU_AUTOHIGHLIGHT, Msg(MTitle), nullptr, L"Contents", nullptr,
			nullptr, reinterpret_cast<const FarMenuItem *>(items.data()),
			static_cast<int>(items.size()));
		return selected >= 0 && selected < static_cast<int>(rows.size())
			? rows[static_cast<std::size_t>(selected)].transform : nullptr;
	}

	void RunEditorTransform(const Transformer::Transform &transform)
	{
		EditorSnapshot snapshot;
		std::wstring error;
		if (!ReadEditorSnapshot(snapshot, error)) {
			std::wstring report = Msg(MTitle);
			report += L'\n';
			report += error;
			ShowText(report);
			return;
		}
		const ExecuteResult result = ExecuteFilter(transform.command, snapshot.input);
		if (!result.Succeeded()) {
			const std::wstring report = CommandFailureReport(transform, result);
			if (result.output.empty()) {
				ShowMessageText(report, true);
			} else {
				ShowText(report);
			}
			return;
		}
		if (transform.output_mode != Transformer::OutputMode::Replace) {
			if (transform.output_mode == Transformer::OutputMode::Viewer) {
				ShowOutputViewer(transform, result);
			} else {
				ShowMessageText(CommandSuccessReport(transform, result));
			}
			return;
		}
		if (std::find(result.output.begin(), result.output.end(), '\0') != result.output.end()) {
			ShowSimpleMessage(MEditorNulOutput);
			return;
		}
		if (!ApplyEditorOutput(snapshot, result.output)) {
			ShowSimpleMessage(MCannotModifyEditor);
		}
	}

	void RunPanelTransform(const Transformer::Transform &transform,
		const PanelSelection &selection)
	{
		const bool backup = LoadBackupSetting();
		std::vector<std::wstring> failures;
		std::vector<std::wstring> reports;
		bool failures_have_output = false;
		bool panel_changed = false;

		for (const auto &wide_path : selection.paths) {
			const std::string path = StrWide2MB(wide_path);
			struct stat metadata {};
			std::vector<char> input;
			if (lstat(path.c_str(), &metadata) == -1 || !S_ISREG(metadata.st_mode)
					|| !ReadAllBytes(path, input)) {
				failures.emplace_back(FormatMessage(MCannotReadFile, wide_path.c_str()));
				continue;
			}

			const ExecuteResult result = ExecuteFilter(transform.command, input);
			if (!result.Succeeded()) {
				failures.emplace_back(CommandFailureReport(transform, result, &wide_path));
				failures_have_output = failures_have_output || !result.output.empty();
				continue;
			}
			if (transform.output_mode != Transformer::OutputMode::Replace) {
				if (transform.output_mode == Transformer::OutputMode::Viewer) {
					ShowOutputViewer(transform, result, &wide_path);
				} else {
					reports.emplace_back(CommandSuccessReport(transform, result, &wide_path));
				}
				continue;
			}

			std::wstring error;
			if (!ReplacePanelFile(wide_path, input, result.output, metadata, backup, error)) {
				failures.emplace_back(std::move(error));
				continue;
			}
			panel_changed = true;
		}

		if (panel_changed) {
			g_info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
			g_info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0);
		}
		if (!failures.empty()) {
			std::wstring report = Msg(MTitle);
			for (const auto &failure : failures) {
				report += L"\n\n";
				report += failure;
			}
			if (failures_have_output) {
				ShowText(report);
			} else {
				ShowMessageText(report, true);
			}
		}
		if (!reports.empty()) {
			std::wstring report = Msg(MTitle);
			for (const auto &item : reports) {
				report += L"\n\n";
				report += item;
			}
			ShowMessageText(report);
		}
	}

	void ShowSettings()
	{
		enum DialogItem
		{
			DBox,
			DBackup,
			DSeparator,
			DOk,
			DCancel,
		};
		FarDialogItem items[] = {
			{DI_DOUBLEBOX, 3, 1, 64, 6, FALSE, {}, DIF_NONE, FALSE, Msg(MSettingsTitle), 0},
			{DI_CHECKBOX, 5, 2, 0, 2, FALSE, {}, DIF_NONE, FALSE, Msg(MCreateBackups), 0},
			{DI_TEXT, 0, 3, 0, 3, FALSE, {}, DIF_SEPARATOR, FALSE, L"", 0},
			{DI_BUTTON, 0, 4, 0, 4, FALSE, {}, DIF_CENTERGROUP, TRUE, Msg(MOk), 0},
			{DI_BUTTON, 0, 4, 0, 4, FALSE, {}, DIF_CENTERGROUP, FALSE, Msg(MCancel), 0},
		};
		items[DBackup].Selected = LoadBackupSetting();
		HANDLE dialog = g_info.DialogInit(g_info.ModuleNumber, -1, -1, 68, 8, L"Contents",
			items, std::size(items), 0, 0, nullptr, 0);
		if (dialog == INVALID_HANDLE_VALUE) {
			return;
		}
		const int result = g_info.DialogRun(dialog);
		if (result == DOk) {
			const bool backup = g_info.SendDlgMessage(dialog, DM_GETCHECK, DBackup, 0) != 0;
			SaveBackupSetting(backup);
		}
		g_info.DialogFree(dialog);
	}
}

SHAREDSYMBOL int WINAPI EXP_NAME(GetMinFarVersion)()
{
	return FARMANAGERVERSION;
}

SHAREDSYMBOL void WINAPI EXP_NAME(SetStartupInfo)(const PluginStartupInfo *info)
{
	g_info = *info;
	g_fsf = *info->FSF;
	g_info.FSF = &g_fsf;
	g_module_name = info->ModuleName ? info->ModuleName : L"";
}

SHAREDSYMBOL void WINAPI EXP_NAME(GetPluginInfo)(PluginInfo *info)
{
	info->StructSize = sizeof(*info);
	info->SysID = 0x5452464d; // TRFM
	info->Flags = PF_EDITOR;
	info->DiskMenuStringsNumber = 0;
	static const wchar_t *menu_strings[1];
	menu_strings[0] = Msg(MPluginMenu);
	info->PluginMenuStrings = menu_strings;
	info->PluginMenuStringsNumber = std::size(menu_strings);
	static const wchar_t *config_strings[1];
	config_strings[0] = Msg(MTitle);
	info->PluginConfigStrings = config_strings;
	info->PluginConfigStringsNumber = std::size(config_strings);
	info->CommandPrefix = nullptr;
}

SHAREDSYMBOL HANDLE WINAPI EXP_NAME(OpenPlugin)(int open_from, INT_PTR)
{
	const bool editor = open_from == OPEN_EDITOR
		|| open_from == (OPEN_FROMMACRO | MACROAREA_EDITOR);
	const bool panels = open_from == OPEN_PLUGINSMENU || open_from == OPEN_FILEPANEL
		|| open_from == (OPEN_FROMMACRO | MACROAREA_SHELL);
	if (!editor && !panels) {
		return INVALID_HANDLE_VALUE;
	}

	std::vector<std::wstring> filenames;
	PanelSelection panel_selection;
	if (editor) {
		const std::wstring filename = EditorFilename();
		if (!filename.empty()) {
			filenames.emplace_back(filename);
		}
	} else {
		panel_selection = GetPanelSelection();
		if (panel_selection.paths.empty()) {
			ShowSimpleMessage(MNoRegularFiles);
			return INVALID_HANDLE_VALUE;
		}
		filenames = panel_selection.filenames;
	}

	const Transformer::Config config = Transformer::LoadFormatDirectories(FormatDirectories());
	const Transformer::Transform *transform = ChooseTransform(config, filenames);
	if (!transform) {
		return INVALID_HANDLE_VALUE;
	}
	if (editor) {
		RunEditorTransform(*transform);
	} else {
		RunPanelTransform(*transform, panel_selection);
	}
	return INVALID_HANDLE_VALUE;
}

SHAREDSYMBOL int WINAPI EXP_NAME(Configure)(int)
{
	ShowSettings();
	return FALSE;
}
