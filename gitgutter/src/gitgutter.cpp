#include <utils.h>
#include <farplug-wide.h>
#include <KeyFileHelper.h>
#include <WideMB.h>
#include <Escaping.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <string>
#include <unordered_map>
#include <vector>

static PluginStartupInfo g_info{};
static FarStandardFunctions g_fsf{};

static const wchar_t *g_plugin_menu_strings[1];

static uint64_t NowMs()
{
	using namespace std::chrono;
	return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

static bool IsLikelyEditInput(const INPUT_RECORD *ir)
{
	if (!ir || ir->EventType != KEY_EVENT)
		return false;
	const auto &ke = ir->Event.KeyEvent;
	if (!ke.bKeyDown)
		return false;
	if (ke.uChar.UnicodeChar != 0)
		return true;
	switch (ke.wVirtualKeyCode) {
	case VK_BACK:
	case VK_DELETE:
	case VK_RETURN:
	case VK_TAB:
		return true;
	default:
		return false;
	}
}

static std::string TrimLineEnd(std::string s)
{
	while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) {
		s.pop_back();
	}
	return s;
}

static void SplitLines(const std::string &s, std::vector<std::string> &out)
{
	out.clear();
	std::string cur;
	for (char ch : s) {
		if (ch == '\n') {
			out.push_back(cur);
			cur.clear();
		} else if (ch != '\r') {
			cur.push_back(ch);
		}
	}
	if (!cur.empty()) {
		out.push_back(cur);
	}
}

static bool FileExists(const std::string &path)
{
	return access(path.c_str(), F_OK) == 0;
}

static bool MakeTempFile(std::string &path, FILE *&f)
{
	f = nullptr;
	std::string tmpl = "/tmp/far2l_gitgutter_XXXXXX";
	std::vector<char> buf(tmpl.begin(), tmpl.end());
	buf.push_back('\0');
	const int fd = mkstemp(buf.data());
	if (fd == -1) {
		return false;
	}
	f = fdopen(fd, "wb");
	if (!f) {
		close(fd);
		unlink(buf.data());
		return false;
	}
	path.assign(buf.data());
	return true;
}

static bool WriteTextToTempFile(const std::string &text, std::string &path)
{
	FILE *f = nullptr;
	if (!MakeTempFile(path, f)) {
		return false;
	}
	if (!text.empty()) {
		fwrite(text.data(), 1, text.size(), f);
	}
	fclose(f);
	return true;
}

static bool RunCommand(const std::string &cmd, std::string &out)
{
	out.clear();
	FILE *pipe = popen(cmd.c_str(), "r");
	if (!pipe) {
		return false;
	}
	char buffer[4096];
	while (fgets(buffer, sizeof(buffer), pipe)) {
		out.append(buffer);
	}
	const int rc = pclose(pipe);
	return rc == 0 || !out.empty();
}

struct Settings
{
	bool enabled = true;
	std::string baseline = "head";
	int interval_ms = 500;
	struct ColorPair
	{
		uint32_t fg = 0;
		uint32_t bg = 0;
	};
	ColorPair color_added{0x00c000, 0x000000};
	ColorPair color_modified{0xc0c000, 0x000000};
	ColorPair color_deleted{0xc00000, 0x000000};

	void Load()
	{
		KeyFileReadHelper kfh(InMyConfig("plugins/gitgutter/gitgutter.ini"));
		const KeyFileValues *vals = kfh.GetSectionValues("Settings");
		if (!vals) {
			return;
		}

		enabled = vals->GetInt("Enabled", enabled ? 1 : 0) != 0;
		baseline = vals->GetString("Baseline", baseline.c_str());
		interval_ms = std::max(50, vals->GetInt("IntervalMs", interval_ms));

		auto parse_color = [](const std::string &s, uint32_t def) -> uint32_t
		{
			if (s.empty())
				return def;
			char *endp = nullptr;
			unsigned long val = std::strtoul(s.c_str(), &endp, 0);
			if (endp == s.c_str())
				return def;
			return static_cast<uint32_t>(val & 0x00ffffff);
		};
		auto parse_pair = [&](const std::string &s, const ColorPair &def) -> ColorPair
		{
			if (s.empty())
				return def;
			const size_t pos = s.find_first_of(",;");
			if (pos == std::string::npos)
				return def;
			ColorPair out{};
			out.fg = parse_color(s.substr(0, pos), def.fg);
			out.bg = parse_color(s.substr(pos + 1), def.bg);
			return out;
		};

		color_added = parse_pair(vals->GetString("ColorAdded", ""), color_added);
		color_modified = parse_pair(vals->GetString("ColorModified", ""), color_modified);
		color_deleted = parse_pair(vals->GetString("ColorDeleted", ""), color_deleted);
	}

	void Save() const
	{
		KeyFileHelper kfh(InMyConfig("plugins/gitgutter/gitgutter.ini"));
		kfh.SetInt("Settings", "Enabled", enabled ? 1 : 0);
		kfh.SetString("Settings", "Baseline", baseline);
		kfh.SetInt("Settings", "IntervalMs", interval_ms);

		auto fmt_color = [](uint32_t value) -> std::string
		{
			char buf[16];
			std::snprintf(buf, sizeof(buf), "0x%06x", value & 0x00ffffff);
			return std::string(buf);
		};
		auto fmt_pair = [&](const ColorPair &c) -> std::string
		{
			return fmt_color(c.fg) + "," + fmt_color(c.bg);
		};

		kfh.SetString("Settings", "ColorAdded", fmt_pair(color_added));
		kfh.SetString("Settings", "ColorModified", fmt_pair(color_modified));
		kfh.SetString("Settings", "ColorDeleted", fmt_pair(color_deleted));
		kfh.Save();
	}
};

static Settings g_settings;
static bool g_git_available = false;

static bool IsPluginActive()
{
	return g_git_available && g_settings.enabled;
}

static bool CheckGitAvailable()
{
	std::string out;
	return RunCommand("git --version", out);
}

static uint32_t SwapRB(uint32_t rgb)
{
	return (rgb & 0x00ff00) | ((rgb & 0x0000ff) << 16) | ((rgb & 0xff0000) >> 16);
}

static uint64_t MakeTrueColor(uint32_t rgb)
{
	return (static_cast<uint64_t>(SwapRB(rgb)) << 16) | FOREGROUND_TRUECOLOR;
}

static uint64_t MakeTrueColorBack(uint32_t rgb)
{
	return (static_cast<uint64_t>(SwapRB(rgb)) << 40) | BACKGROUND_TRUECOLOR;
}

struct Hunk
{
	int start = 0;
	int end = 0;
	std::wstring text;
};

struct EditorState
{
	int editor_id = -1;
	std::wstring file_w;
	std::string file;
	std::string repo_root;
	std::string rel_path;
	std::string temp_path;
	uint64_t last_update_ms = 0;
	bool dirty = true;
	bool gutter_forced = false;
	int gutter_request = -1;
	int tab_size = 0;
	std::vector<EditorGutterMark> marks;
	std::vector<Hunk> hunks;
};

static std::unordered_map<int, EditorState> g_editors;
struct PendingPopup
{
	bool active = false;
	int editor_id = -1;
	int line = 0;
	int x = 0;
	int y = 0;
};
static PendingPopup g_pending_popup;
static bool g_popup_active = false;
static bool g_pending_tick = false;
static uint64_t g_next_tick_ms = 0;

enum PopupItemId
{
	POPUP_MEMO = 0,
	POPUP_PREV = 1,
	POPUP_NEXT = 2,
	POPUP_REVERT = 3,
};

static bool GetEditorInfo(EditorInfo &ei);
static bool GetEditorRect(SMALL_RECT &rect);

static void MoveEditorToHunk(const Hunk &h)
{
	EditorInfo ei{};
	const bool have_info = GetEditorInfo(ei);
	EditorSetPosition esp{};
	esp.CurLine = h.start;
	esp.CurPos = 0;
	esp.CurTabPos = -1;
	if (have_info) {
		const int half = std::max(1, ei.WindowSizeY / 2);
		esp.TopScreenLine = std::max(0, h.start - half);
	} else {
		esp.TopScreenLine = -1;
	}
	esp.LeftPos = -1;
	esp.Overtype = -1;
	g_info.EditorControl(ECTL_SETPOSITION, &esp);
	g_info.EditorControl(ECTL_REDRAW, nullptr);
}

static bool GetHunkAnchor(const Hunk &h, int &anchor_x, int &anchor_y)
{
	EditorInfo ei{};
	if (!GetEditorInfo(ei))
		return false;
	SMALL_RECT er{};
	if (!GetEditorRect(er))
		return false;

	int line_num_width = 1;
	const bool show_numbers = (ei.Options & EOPT_SHOWNUMBERS) != 0;
	const bool show_gutter = (ei.Options & EOPT_SHOWGUTTER) != 0;
	if (show_numbers) {
		int digits = 1;
		int temp = std::max(1, ei.TotalLines);
		while (temp >= 10) {
			digits++;
			temp /= 10;
		}
		line_num_width = std::max(4, digits) + 1;
	} else if (show_gutter) {
		line_num_width = 1;
	}

	const int gutter_x = line_num_width - 1;
	const int rel_y = h.start - ei.TopScreenLine;
	anchor_x = er.Left + gutter_x;
	anchor_y = er.Top + std::max(0, rel_y);
	return true;
}

struct PopupContext
{
	const EditorState *st = nullptr;
	size_t index = 0;
	int tab_size = 0;
};

static void UpdatePopupMemo(HANDLE hDlg, PopupContext *ctx)
{
	if (!ctx || !ctx->st || ctx->index >= ctx->st->hunks.size()) {
		return;
	}
	const auto &h = ctx->st->hunks[ctx->index];
	g_info.SendDlgMessage(hDlg, DM_SETTEXTPTR, POPUP_MEMO,
			reinterpret_cast<LONG_PTR>(h.text.c_str()));
	MoveEditorToHunk(h);
}

static LONG_PTR WINAPI PopupDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	auto *ctx = reinterpret_cast<PopupContext *>(
			g_info.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0));
	if (Msg == DN_INITDIALOG) {
		g_info.SendDlgMessage(hDlg, DM_SETFOCUS, POPUP_MEMO, 0);
		// Set memoedit tab size at init, before colorer processes focus.
		if (ctx && ctx->tab_size > 0) {
			g_info.SendDlgMessage(hDlg, DM_SETEDITTABSIZE, POPUP_MEMO, ctx->tab_size);
		}
	}
	return g_info.DefDlgProc(hDlg, Msg, Param1, Param2);
}

static void MaybeScheduleTick()
{
	if (g_pending_tick || g_settings.interval_ms <= 0) {
		return;
	}
	const uint64_t now = NowMs();
	if (now < g_next_tick_ms) {
		return;
	}
	g_next_tick_ms = now + static_cast<uint64_t>(g_settings.interval_ms);
	g_pending_tick = true;
	g_info.AdvControl(g_info.ModuleNumber, ACTL_SYNCHRO, nullptr, nullptr);
}

static void ApplyGutterRequest(EditorState &st)
{
	if (st.gutter_request == -1) {
		return;
	}
	EditorSetParameter esp{};
	esp.Type = ESPT_SHOWGUTTER;
	esp.Param.iParam = st.gutter_request;
	esp.Flags = 0;
	esp.Size = 0;
	g_info.EditorControl(ECTL_SETPARAM, &esp);
	g_info.EditorControl(ECTL_REDRAW, nullptr);
	if (st.gutter_request == 1) {
		st.gutter_forced = true;
	} else if (st.gutter_request == 0) {
		st.gutter_forced = false;
	}
	st.gutter_request = -1;
}

static bool GetEditorInfo(EditorInfo &ei)
{
	memset(&ei, 0, sizeof(ei));
	return g_info.EditorControl(ECTL_GETINFO, &ei) != 0;
}

static bool GetEditorRect(SMALL_RECT &rect)
{
	memset(&rect, 0, sizeof(rect));
	return g_info.EditorControl(ECTL_GETRECT, &rect) != 0;
}

static bool GetEditorFileName(std::wstring &out)
{
	out.clear();
	const size_t sz = g_info.EditorControl(ECTL_GETFILENAME, nullptr);
	if (sz <= 1) {
		return false;
	}
	out.resize(sz + 1);
	g_info.EditorControl(ECTL_GETFILENAME, &out[0]);
	out.resize(sz - 1);
	return !out.empty();
}

static void SetEditorLinePos(int line, int pos)
{
	EditorSetPosition esp{};
	esp.CurLine = line;
	esp.CurPos = pos;
	esp.CurTabPos = -1;
	esp.TopScreenLine = -1;
	esp.LeftPos = -1;
	esp.Overtype = -1;
	g_info.EditorControl(ECTL_SETPOSITION, &esp);
}

static bool ParseHunkHeader(const std::wstring &text, int &old_start, int &old_count, int &new_start, int &new_count, size_t &body_offset)
{
	old_start = 0;
	old_count = 0;
	new_start = 0;
	new_count = 0;
	body_offset = 0;

	const size_t line_end = text.find(L'\n');
	const size_t header_end = (line_end == std::wstring::npos) ? text.size() : line_end;
	const std::wstring header = text.substr(0, header_end);

	int os = 0, oc = 1, ns = 0, nc = 1;
	if (swscanf(header.c_str(), L"@@ -%d,%d +%d,%d @@",
			&os, &oc, &ns, &nc) != 4) {
		if (swscanf(header.c_str(), L"@@ -%d +%d,%d @@",
					&os, &ns, &nc) == 3) {
			oc = 1;
		} else if (swscanf(header.c_str(), L"@@ -%d,%d +%d @@",
					&os, &oc, &ns) == 3) {
			nc = 1;
		} else if (swscanf(header.c_str(), L"@@ -%d +%d @@",
					&os, &ns) == 2) {
			oc = 1;
			nc = 1;
		} else {
			return false;
		}
	}

	old_start = os;
	old_count = oc;
	new_start = ns;
	new_count = nc;
	body_offset = (line_end == std::wstring::npos) ? text.size() : line_end + 1;
	return true;
}

static void ExtractHunkLines(const std::wstring &text, size_t body_offset,
		std::vector<std::wstring> &old_lines,
		std::vector<std::wstring> &new_lines,
		bool &no_newline_marker)
{
	old_lines.clear();
	new_lines.clear();
	no_newline_marker = false;

	const size_t text_len = text.size();
	size_t pos = body_offset;
	while (pos < text_len) {
		size_t line_end = text.find(L'\n', pos);
		if (line_end == std::wstring::npos)
			line_end = text_len;
		std::wstring line = text.substr(pos, line_end - pos);
		if (!line.empty() && line.back() == L'\r')
			line.pop_back();

		if (line.empty() && line_end == text_len) {
			// Trailing newline yields an extra empty line; ignore it.
			break;
		}
		if (line == L"\\ No newline at end of file") {
			no_newline_marker = true;
		} else if (!line.empty()) {
			const wchar_t prefix = line[0];
			if (prefix == L' ' || prefix == L'-') {
				old_lines.emplace_back(line.substr(1));
			}
			if (prefix == L' ' || prefix == L'+') {
				new_lines.emplace_back(line.substr(1));
			}
		}

		if (line_end == text_len)
			break;
		pos = line_end + 1;
	}
}

static bool EditorLineEquals(int line, const std::wstring &expected)
{
	EditorGetString egs{};
	egs.StringNumber = line;
	if (!g_info.EditorControl(ECTL_GETSTRING, &egs)) {
		return false;
	}
	if (!egs.StringText) {
		return expected.empty();
	}
	const std::wstring_view actual(egs.StringText, static_cast<size_t>(egs.StringLength));
	return actual == expected;
}

static void InsertLinesAt(int start_line, const std::vector<std::wstring> &lines, bool omit_trailing_newline)
{
	if (lines.empty()) {
		return;
	}

	if (start_line < 0) {
		start_line = 0;
	}

	SetEditorLinePos(start_line, 0);
	g_info.EditorControl(ECTL_INSERTSTRING, nullptr);
	EditorSetString first{};
	first.StringNumber = start_line;
	first.StringText = lines[0].c_str();
	first.StringEOL = (omit_trailing_newline && lines.size() == 1) ? L"" : L"\n";
	first.StringLength = static_cast<int>(lines[0].size());
	g_info.EditorControl(ECTL_SETSTRING, &first);

	for (size_t i = 1; i < lines.size(); ++i) {
		SetEditorLinePos(start_line + static_cast<int>(i), 0);
		g_info.EditorControl(ECTL_INSERTSTRING, nullptr);
		EditorSetString ins{};
		ins.StringNumber = start_line + static_cast<int>(i);
		ins.StringText = lines[i].c_str();
		const bool last = (i + 1 == lines.size());
		ins.StringEOL = (omit_trailing_newline && last) ? L"" : L"\n";
		ins.StringLength = static_cast<int>(lines[i].size());
		g_info.EditorControl(ECTL_SETSTRING, &ins);
	}
}

static bool VerifyHunkMatchesEditor(int start_line, const std::vector<std::wstring> &expected_lines)
{
	for (size_t i = 0; i < expected_lines.size(); ++i) {
		const int line = start_line + static_cast<int>(i);
		if (!EditorLineEquals(line, expected_lines[i])) {
			return false;
		}
	}
	return true;
}

static bool RevertHunkInEditor(const Hunk &h)
{
	EditorInfo ei{};
	if (!GetEditorInfo(ei)) {
		return false;
	}

	int old_start = 0, old_count = 0, new_start = 0, new_count = 0;
	size_t body_offset = 0;
	if (!ParseHunkHeader(h.text, old_start, old_count, new_start, new_count, body_offset)) {
		return false;
	}

	const int total_lines = std::max(0, ei.TotalLines);
	int start_line = std::max(0, new_start - 1);
	if (start_line > total_lines)
		start_line = total_lines;

	std::vector<std::wstring> old_lines;
	std::vector<std::wstring> new_lines;
	bool no_newline_marker = false;
	ExtractHunkLines(h.text, body_offset, old_lines, new_lines, no_newline_marker);
	if (!VerifyHunkMatchesEditor(start_line, new_lines)) {
		return false;
	}
	if (new_count == 1 && old_lines.size() == 1 && old_count == 1) {
		EditorSetString ess{};
		ess.StringNumber = start_line;
		ess.StringText = old_lines[0].c_str();
		ess.StringEOL = no_newline_marker ? L"" : nullptr;
		ess.StringLength = static_cast<int>(old_lines[0].size());
		g_info.EditorControl(ECTL_SETSTRING, &ess);
	} else {
		if (new_count > 0) {
			int to_delete = new_count;
			if (start_line + to_delete > total_lines)
				to_delete = std::max(0, total_lines - start_line);
			SetEditorLinePos(start_line, 0);
			for (int i = 0; i < to_delete; ++i) {
				g_info.EditorControl(ECTL_DELETESTRING, nullptr);
			}
		}

		if (!old_lines.empty()) {
			InsertLinesAt(start_line, old_lines, no_newline_marker);
		}
	}

	return true;
}

static bool GetRelativePath(const std::string &repo_root, const std::string &file_path, std::string &rel_path)
{
	rel_path.clear();
	if (repo_root.empty()) {
		return false;
	}
	if (file_path.rfind(repo_root, 0) == 0) {
		size_t start = repo_root.size();
		if (start < file_path.size() && file_path[start] == '/')
			start++;
		rel_path = file_path.substr(start);
		return !rel_path.empty();
	}
	rel_path = file_path;
	return !rel_path.empty();
}

static bool WriteEditorBufferToTemp(std::string &path)
{
	EditorInfo ei{};
	if (!GetEditorInfo(ei)) {
		return false;
	}
	FILE *f = nullptr;
	if (path.empty()) {
		if (!MakeTempFile(path, f)) {
			return false;
		}
	} else {
		f = fopen(path.c_str(), "wb");
		if (!f) {
			unlink(path.c_str());
			path.clear();
			if (!MakeTempFile(path, f)) {
				return false;
			}
		}
	}
	EditorGetString egs{};
	for (int i = 0; i < ei.TotalLines; ++i) {
		egs.StringNumber = i;
		if (!g_info.EditorControl(ECTL_GETSTRING, &egs)) {
			fclose(f);
			unlink(path.c_str());
			path.clear();
			return false;
		}
		if (egs.StringText && egs.StringLength > 0) {
			std::wstring wline(egs.StringText, egs.StringText + egs.StringLength);
			const std::string mb = Wide2MB(wline.c_str());
			if (!mb.empty()) {
				fwrite(mb.data(), 1, mb.size(), f);
			}
		}
		if (egs.StringEOL && *egs.StringEOL) {
			const std::string eol_mb = Wide2MB(egs.StringEOL);
			if (!eol_mb.empty()) {
				fwrite(eol_mb.data(), 1, eol_mb.size(), f);
			}
		} else if (i + 1 < ei.TotalLines) {
			fwrite("\n", 1, 1, f);
		}
	}
	fclose(f);
	return true;
}

static bool WriteBaselineToTemp(const std::string &repo_root, const std::string &rel_path,
		const std::string &baseline, std::string &path)
{
	std::string root_arg = repo_root;
	QuoteCmdArgIfNeed(root_arg);
	std::string spec;
	if (baseline == "head") {
		spec = "HEAD:" + rel_path;
	} else if (baseline == "index") {
		spec = ":" + rel_path;
	} else if (baseline == "unstaged") {
		return false;
	} else {
		spec = baseline + ":" + rel_path;
	}
	std::string spec_arg = spec;
	QuoteCmdArgIfNeed(spec_arg);
	const std::string cmd = "git -C " + root_arg + " show " + spec_arg;
	std::string out;
	if (!RunCommand(cmd, out)) {
		return false;
	}
	return WriteTextToTempFile(out, path);
}

static bool ParseHunkHeader(const std::string &line, int &old_start, int &old_count, int &new_start, int &new_count)
{
	if (line.rfind("@@ ", 0) != 0)
		return false;

	const char *p = line.c_str();
	const char *oldp = std::strchr(p, '-');
	const char *newp = std::strchr(p, '+');
	if (!oldp || !newp)
		return false;

	oldp++;
	old_start = std::strtol(oldp, const_cast<char **>(&p), 10);
	if (*p == ',') {
		p++;
		old_count = std::strtol(p, const_cast<char **>(&p), 10);
	} else {
		old_count = 1;
	}

	newp++;
	new_start = std::strtol(newp, const_cast<char **>(&p), 10);
	if (*p == ',') {
		p++;
		new_count = std::strtol(p, const_cast<char **>(&p), 10);
	} else {
		new_count = 1;
	}

	return true;
}

static void BuildMarksFromDiff(const std::string &diff, EditorState &st)
{
	st.marks.clear();
	st.hunks.clear();

	std::unordered_map<int, uint64_t> mark_map;
	std::vector<std::string> lines;
	lines.reserve(256);

	std::string cur;
	for (char c : diff) {
		if (c == '\n') {
			lines.emplace_back(TrimLineEnd(cur));
			cur.clear();
		} else {
			cur.push_back(c);
		}
	}
	if (!cur.empty()) {
		lines.emplace_back(TrimLineEnd(cur));
	}

	int old_start = 0, old_count = 0, new_start = 0, new_count = 0;
	std::wstring hunk_text;
	bool in_hunk = false;
	int hunk_start = 0;
	int hunk_end = 0;
	uint64_t hunk_color = 0;

	auto flush_hunk = [&]()
	{
		if (!in_hunk) {
			return;
		}
		Hunk h{};
		h.start = hunk_start;
		h.end = hunk_end;
		h.text = hunk_text;
		st.hunks.push_back(std::move(h));
		in_hunk = false;
		hunk_text.clear();
	};

	for (const auto &line : lines) {
		if (ParseHunkHeader(line, old_start, old_count, new_start, new_count)) {
			flush_hunk();
			in_hunk = true;
			hunk_text = StrMB2Wide(line);

			const bool added = (old_count == 0 && new_count > 0);
			const bool deleted = (new_count == 0 && old_count > 0);

			const Settings::ColorPair *pair = &g_settings.color_modified;
			if (added) {
				pair = &g_settings.color_added;
			} else if (deleted) {
				pair = &g_settings.color_deleted;
			}
			hunk_color = MakeTrueColor(pair->fg);
			hunk_color |= MakeTrueColorBack(pair->bg);

			if (new_count > 0) {
				hunk_start = std::max(0, new_start - 1);
				hunk_end = hunk_start + new_count - 1;
				for (int i = 0; i < new_count; ++i) {
					mark_map[hunk_start + i] = hunk_color;
				}
			} else if (old_count > 0) {
				hunk_start = std::max(0, new_start - 1);
				hunk_end = hunk_start;
				mark_map[hunk_start] = hunk_color;
			}
			continue;
		}

		if (in_hunk) {
			hunk_text.append(L"\n");
			hunk_text.append(StrMB2Wide(line));
		}
	}
	flush_hunk();

	st.marks.reserve(mark_map.size());
	for (const auto &kv : mark_map) {
		EditorGutterMark m{};
		m.Line = kv.first;
		m.Color = kv.second;
		st.marks.push_back(m);
	}
}

static bool GetRepoRoot(const std::string &file_path, std::string &repo_root)
{
	repo_root.clear();
	const size_t slash = file_path.find_last_of('/');
	const std::string dir = (slash == std::string::npos) ? "." : file_path.substr(0, slash);
	std::string dir_arg = dir;
	QuoteCmdArgIfNeed(dir_arg);
	const std::string cmd = "git -C " + dir_arg + " rev-parse --show-toplevel";
	std::string out;
	if (!RunCommand(cmd, out)) {
		return false;
	}
	repo_root = TrimLineEnd(out);
	return !repo_root.empty();
}

static std::string MakeDiffCommandBaseline(const std::string &repo_root, const std::string &file_path, const std::string &baseline)
{
	std::string root_arg = repo_root;
	std::string file_arg = file_path;
	QuoteCmdArgIfNeed(root_arg);
	QuoteCmdArgIfNeed(file_arg);
	std::string base_arg = baseline;
	QuoteCmdArgIfNeed(base_arg);

	std::string cmd = "git -C " + root_arg + " diff --no-color --unified=0 ";
	if (baseline == "head") {
		cmd += "HEAD ";
	} else if (baseline == "index") {
		cmd += "--cached ";
	} else if (baseline == "unstaged") {
		// default diff shows unstaged changes
	} else {
		cmd += base_arg + " ";
	}
	cmd += "-- " + file_arg;
	return cmd;
}

static std::string MakeDiffCommand(const std::string &repo_root, const std::string &file_path)
{
	return MakeDiffCommandBaseline(repo_root, file_path, g_settings.baseline);
}

static void ApplyMarksToEditor(const EditorState &st)
{
	EditorGutterMarks gm{};
	gm.Count = st.marks.size();
	gm.Marks = st.marks.empty() ? nullptr : st.marks.data();
	g_info.EditorControl(ECTL_SETGUTTERMARKS, &gm);
}

static void UpdateEditorState(EditorState &st)
{
	if (!IsPluginActive()) {
		return;
	}
	const uint64_t now = NowMs();
	if (now - st.last_update_ms < static_cast<uint64_t>(g_settings.interval_ms)) {
		return;
	}
	st.last_update_ms = now;
	EditorInfo ei{};
	bool show_gutter = false;
	if (GetEditorInfo(ei)) {
		st.tab_size = ei.TabSize;
		show_gutter = (ei.Options & EOPT_SHOWGUTTER) != 0;
	}
	if (!show_gutter && st.gutter_forced) {
		st.gutter_forced = false;
	}

	std::wstring file_w;
	if (!GetEditorFileName(file_w)) {
		return;
	}
	if (file_w == L"gitgutter.diff") {
		return;
	}

	if (file_w != st.file_w) {
		st.file_w = file_w;
		st.file = Wide2MB(file_w.c_str());
		st.repo_root.clear();
		st.rel_path.clear();
	}

	if (st.file.empty()) {
		return;
	}

	if (st.repo_root.empty()) {
		if (!GetRepoRoot(st.file, st.repo_root)) {
			return;
		}
	}

	std::string out;
	if (WriteEditorBufferToTemp(st.temp_path)) {
		std::string base_path;
		std::string base_temp;
		if (g_settings.baseline == "unstaged") {
			base_path = st.file;
			if (!FileExists(base_path)) {
				base_path = "/dev/null";
			}
		} else {
			std::string rel_path;
			if (!GetRelativePath(st.repo_root, st.file, rel_path)
					|| !WriteBaselineToTemp(st.repo_root, rel_path, g_settings.baseline, base_temp)) {
				unlink(st.temp_path.c_str());
				st.temp_path.clear();
				goto fallback;
			}
			base_path = base_temp;
		}
		std::string root_arg = st.repo_root;
		std::string base_arg = base_path;
		std::string temp_arg = st.temp_path;
		QuoteCmdArgIfNeed(root_arg);
		QuoteCmdArgIfNeed(base_arg);
		QuoteCmdArgIfNeed(temp_arg);
		const std::string cmd = "git -C " + root_arg
				+ " diff --no-color --unified=0 --no-index -- " + base_arg + " " + temp_arg;
		RunCommand(cmd, out);
		if (!base_temp.empty()) {
			unlink(base_temp.c_str());
		}
	} else {
fallback:
		std::string cmd = MakeDiffCommand(st.repo_root, st.file);
		RunCommand(cmd, out);
	}

	if (out.empty()) {
		st.marks.clear();
		st.hunks.clear();
		if (show_gutter) {
			ApplyMarksToEditor(st);
			if (st.gutter_forced) {
				st.gutter_request = 0;
			}
		}
		return;
	}

	BuildMarksFromDiff(out, st);
	if (!st.marks.empty() && !show_gutter) {
		st.gutter_request = 1;
	}
	ApplyMarksToEditor(st);
	st.dirty = false;
}

static std::wstring FormatColorHex(uint32_t value)
{
	wchar_t buf[16];
	swprintf_ws2ls(buf, sizeof(buf) / sizeof(buf[0]), L"0x%06x", value & 0x00ffffff);
	return std::wstring(buf);
}

static std::wstring FormatColorPair(const Settings::ColorPair &value)
{
	return FormatColorHex(value.fg) + L"," + FormatColorHex(value.bg);
}

static uint32_t ParseColorHex(const wchar_t *s, uint32_t def)
{
	if (!s || !*s) {
		return def;
	}
	wchar_t *endp = nullptr;
	unsigned long val = std::wcstoul(s, &endp, 0);
	if (endp == s) {
		return def;
	}
	return static_cast<uint32_t>(val & 0x00ffffff);
}

static Settings::ColorPair ParseColorPair(const wchar_t *s, const Settings::ColorPair &def)
{
	if (!s || !*s) {
		return def;
	}
	const wchar_t *sep = std::wcschr(s, L',');
	if (!sep) {
		sep = std::wcschr(s, L';');
	}
	if (!sep) {
		return def;
	}
	Settings::ColorPair out{};
	out.fg = ParseColorHex(s, def.fg);
	out.bg = ParseColorHex(sep + 1, def.bg);
	return out;
}

static void ApplyDialogEditColor(HANDLE hdlg, int edit_id, const Settings::ColorPair &color)
{
	uint64_t colors[4]{};
	g_info.SendDlgMessage(hdlg, DM_GETDEFAULTCOLOR, edit_id, reinterpret_cast<LONG_PTR>(colors));
	const uint64_t bg_mask = 0xFFFFFF00000000F0ull | BACKGROUND_TRUECOLOR;
	for (size_t i = 0; i < 4; ++i) {
		colors[i] &= ~bg_mask;
		colors[i] |= MakeTrueColorBack(color.bg);
	}
	const uint64_t fg_mask = 0x000000FFFFFF000Full | FOREGROUND_TRUECOLOR;
	for (size_t i = 0; i < 4; ++i) {
		colors[i] &= ~fg_mask;
		colors[i] |= MakeTrueColor(color.fg);
	}
	g_info.SendDlgMessage(hdlg, DM_SETCOLOR, edit_id, reinterpret_cast<LONG_PTR>(colors));
}

static void ShowHunkPopup(const EditorState &st, int line, int anchor_x, int anchor_y)
{
	if (g_popup_active) {
		return;
	}
	struct PopupGuard
	{
		bool &flag;
		explicit PopupGuard(bool &f) : flag(f) { flag = true; }
		~PopupGuard() { flag = false; }
	} guard(g_popup_active);

	size_t index = st.hunks.size();
	for (size_t i = 0; i < st.hunks.size(); ++i) {
		if (line >= st.hunks[i].start && line <= st.hunks[i].end) {
			index = i;
			break;
		}
	}
	if (index == st.hunks.size()) {
		return;
	}

	for (;;) {
		const auto &h = st.hunks[index];
		int max_line = 0;
		int cur_line = 0;
		int lines = 1;
		const int tab_size = std::max(1, st.tab_size);
		for (wchar_t ch : h.text) {
			if (ch == L'\n') {
				max_line = std::max(max_line, cur_line);
				cur_line = 0;
				lines++;
				continue;
			}
			if (ch == L'\t') {
				const int advance = tab_size - (cur_line % tab_size);
				cur_line += advance;
				continue;
			}
			if (ch != L'\r') {
				cur_line++;
			}
		}
		max_line = std::max(max_line, cur_line);

		const int min_w = 40;
		const int min_h = 6;
		int max_w = 120;
		SMALL_RECT fr{};
		if (g_info.AdvControl(g_info.ModuleNumber, ACTL_GETFARRECT, &fr, nullptr)) {
			const int screen_w = static_cast<int>(fr.Right) - static_cast<int>(fr.Left) + 1;
			max_w = std::max(min_w, (screen_w * 2) / 3);
		}
		const int dlg_w = std::min(max_w, std::max(min_w, max_line + 4));
		const int dlg_h = std::min(30, std::max(min_h, lines + 2));

		FarDialogItem items[4]{};
		items[POPUP_MEMO].Type = DI_MEMOEDIT;
		items[POPUP_MEMO].X1 = 0;
		items[POPUP_MEMO].Y1 = 1;
		items[POPUP_MEMO].X2 = dlg_w - 1;
		items[POPUP_MEMO].Y2 = dlg_h - 1;
		items[POPUP_MEMO].Focus = 1;
		items[POPUP_MEMO].Flags = DIF_READONLY | DIF_FOCUS;
		items[POPUP_MEMO].PtrData = h.text.c_str();
		items[POPUP_MEMO].MaxLen = h.text.size();

		items[POPUP_PREV].Type = DI_BUTTON;
		items[POPUP_PREV].X1 = 0;
		items[POPUP_PREV].Y1 = 0;
		items[POPUP_PREV].X2 = 2;
		items[POPUP_PREV].Y2 = 0;
		items[POPUP_PREV].Flags = 0;
		items[POPUP_PREV].PtrData = L"\x25C1";

		items[POPUP_NEXT].Type = DI_BUTTON;
		items[POPUP_NEXT].X1 = 5;
		items[POPUP_NEXT].Y1 = 0;
		items[POPUP_NEXT].X2 = 7;
		items[POPUP_NEXT].Y2 = 0;
		items[POPUP_NEXT].Flags = 0;
		items[POPUP_NEXT].PtrData = L"\x25B7";

		items[POPUP_REVERT].Type = DI_BUTTON;
		items[POPUP_REVERT].X1 = 12;
		items[POPUP_REVERT].Y1 = 0;
		items[POPUP_REVERT].X2 = 14;
		items[POPUP_REVERT].Y2 = 0;
		items[POPUP_REVERT].Flags = 0;
		items[POPUP_REVERT].PtrData = L"\x21A9";

		int local_anchor_x = anchor_x;
		int local_anchor_y = anchor_y;
		GetHunkAnchor(h, local_anchor_x, local_anchor_y);

		static const wchar_t k_virtual_filename[] = L"gitgutter.diff";
		items[POPUP_MEMO].Param.Reserved = reinterpret_cast<DWORD_PTR>(k_virtual_filename);
		int dlg_x1 = -1;
		int dlg_y1 = -1;
		int dlg_x2 = -1;
		int dlg_y2 = -1;
		if (g_info.AdvControl(g_info.ModuleNumber, ACTL_GETFARRECT, &fr, nullptr)) {
			const int fr_left = static_cast<int>(fr.Left);
			const int fr_top = static_cast<int>(fr.Top);
			const int fr_right = static_cast<int>(fr.Right);
			const int fr_bottom = static_cast<int>(fr.Bottom);
			const int max_x = std::max(fr_left, fr_right - dlg_w + 1);
			const int max_y = std::max(fr_top, fr_bottom - dlg_h + 1);
			dlg_x1 = std::min(std::max(fr_left, local_anchor_x + 1), max_x);
			dlg_y1 = std::min(std::max(fr_top, local_anchor_y - 1), max_y);
			dlg_x2 = dlg_x1 + dlg_w - 1;
			dlg_y2 = dlg_y1 + dlg_h - 1;
		}
		PopupContext ctx{};
		ctx.st = &st;
		ctx.index = index;
		ctx.tab_size = st.tab_size;
		HANDLE hdlg = g_info.DialogInit(
				g_info.ModuleNumber, dlg_x1, dlg_y1, dlg_x2, dlg_y2, L"GitGutter", items, 4, 0, 0,
				PopupDlgProc, reinterpret_cast<LONG_PTR>(&ctx));
		if (hdlg == INVALID_HANDLE_VALUE)
			return;
		LONG_PTR rc = g_info.DialogRun(hdlg);
		g_info.DialogFree(hdlg);
		if (rc == POPUP_PREV) {
			if (!st.hunks.empty()) {
				index = (index == 0) ? (st.hunks.size() - 1) : (index - 1);
				MoveEditorToHunk(st.hunks[index]);
				continue;
			}
		} else if (rc == POPUP_NEXT) {
			if (!st.hunks.empty()) {
				index = (index + 1 >= st.hunks.size()) ? 0 : (index + 1);
				MoveEditorToHunk(st.hunks[index]);
				continue;
			}
		} else if (rc == POPUP_REVERT) {
			const Hunk old_hunk = st.hunks[index];
			RevertHunkInEditor(st.hunks[index]);
			MaybeScheduleTick();
			UpdateEditorState(const_cast<EditorState &>(st));
			if (st.hunks.empty())
				return;
			size_t found = st.hunks.size();
			for (size_t i = 0; i < st.hunks.size(); ++i) {
				const Hunk &h = st.hunks[i];
				if (h.start == old_hunk.start && h.end == old_hunk.end && h.text == old_hunk.text) {
					found = i;
					break;
				}
			}
			if (found != st.hunks.size()) {
				if (found + 1 >= st.hunks.size())
					return;
				index = found + 1;
			} else {
				if (index >= st.hunks.size())
					return;
			}
			MoveEditorToHunk(st.hunks[index]);
			continue;
		}
		return;
	}
}

static bool HandleGutterClick(const INPUT_RECORD *ir)
{
	if (ir->EventType != MOUSE_EVENT)
		return false;

	const auto &me = ir->Event.MouseEvent;
	EditorInfo ei{};
	if (!GetEditorInfo(ei))
		return false;
	SMALL_RECT er{};
	if (!GetEditorRect(er))
		return false;

	// Only react to button press; defer popup to EE_REDRAW (Message is unsafe from input handler).
	if (me.dwEventFlags != 0 || (me.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) == 0)
		return false;
	if (g_popup_active || g_pending_popup.active)
		return true;

	const int rel_x = me.dwMousePosition.X - er.Left;
	const int rel_y = me.dwMousePosition.Y - er.Top;
	if (rel_x < 0 || rel_y < 0)
		return false;

	const bool show_numbers = (ei.Options & EOPT_SHOWNUMBERS) != 0;
	const bool show_gutter = (ei.Options & EOPT_SHOWGUTTER) != 0;
	if (!show_numbers && !show_gutter)
		return false;

	int line_num_width = 0;
	if (show_numbers) {
		int digits = 1;
		int temp = std::max(1, ei.TotalLines);
		while (temp >= 10) {
			digits++;
			temp /= 10;
		}
		line_num_width = std::max(4, digits) + 1;
	} else {
		line_num_width = 1;
	}
	const int gutter_x = line_num_width - 1;
	if (rel_x > gutter_x)
		return false;

	const int line = ei.TopScreenLine + rel_y;
	g_pending_popup.active = true;
	g_pending_popup.editor_id = ei.EditorID;
	g_pending_popup.line = line;
	g_pending_popup.x = me.dwMousePosition.X;
	g_pending_popup.y = me.dwMousePosition.Y;
	g_info.AdvControl(g_info.ModuleNumber, ACTL_SYNCHRO, nullptr, nullptr);
	return true;
}

SHAREDSYMBOL void PluginModuleOpen(const char *path)
{
	(void)path;
}

SHAREDSYMBOL void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info)
{
	g_info = *Info;
	g_fsf = *Info->FSF;
	g_info.FSF = &g_fsf;
	g_settings.Load();
	g_git_available = CheckGitAvailable();
	g_plugin_menu_strings[0] = L"GitGutter";
}

SHAREDSYMBOL void WINAPI GetPluginInfoW(struct PluginInfo *Info)
{
	memset(Info, 0, sizeof(*Info));
	Info->StructSize = sizeof(*Info);
	if (!g_git_available) {
		return;
	}
	Info->Flags = PF_EDITOR | PF_DISABLEPANELS | PF_PRELOAD;
	Info->PluginConfigStringsNumber = 1;
	Info->PluginConfigStrings = g_plugin_menu_strings;
	Info->PluginMenuStringsNumber = 1;
	Info->PluginMenuStrings = g_plugin_menu_strings;
}

static int ShowConfigDialog()
{
	enum
	{
		CD_BOX,
		CD_ENABLED,
		CD_BASELINE_TEXT,
		CD_BASELINE,
		CD_INTERVAL_TEXT,
		CD_INTERVAL,
		CD_COLOR_ADDED_TEXT,
		CD_COLOR_ADDED,
		CD_COLOR_ADDED_PICK,
		CD_COLOR_MODIFIED_TEXT,
		CD_COLOR_MODIFIED,
		CD_COLOR_MODIFIED_PICK,
		CD_COLOR_DELETED_TEXT,
		CD_COLOR_DELETED,
		CD_COLOR_DELETED_PICK,
		CD_SEPARATOR,
		CD_OK,
		CD_CANCEL,
		CD_COUNT
	};

	std::wstring interval_w = std::to_wstring(g_settings.interval_ms);
	std::wstring color_added_w = FormatColorPair(g_settings.color_added);
	std::wstring color_modified_w = FormatColorPair(g_settings.color_modified);
	std::wstring color_deleted_w = FormatColorPair(g_settings.color_deleted);

	std::vector<std::string> baseline_values;
	std::vector<std::wstring> baseline_labels;
	auto add_baseline = [&](const std::string &v)
	{
		baseline_values.push_back(v);
		baseline_labels.push_back(StrMB2Wide(v));
	};
	add_baseline("head");
	add_baseline("index");
	add_baseline("unstaged");

	std::string branches_out;
	if (RunCommand("git branch --format='%(refname:short)'", branches_out)) {
		std::vector<std::string> lines;
		SplitLines(branches_out, lines);
		for (const auto &line : lines) {
			const std::string name = TrimLineEnd(line);
			if (name.empty())
				continue;
			if (std::find(baseline_values.begin(), baseline_values.end(), name) == baseline_values.end()) {
				add_baseline(name);
			}
		}
	}

	std::vector<FarListItem> baseline_items(baseline_values.size());
	int baseline_index = 0;
	for (size_t i = 0; i < baseline_values.size(); ++i) {
		baseline_items[i].Text = baseline_labels[i].c_str();
		if (baseline_values[i] == g_settings.baseline) {
			baseline_index = static_cast<int>(i);
		}
	}
	if (!baseline_items.empty()) {
		baseline_items[baseline_index].Flags |= LIF_SELECTED;
	}
	FarList baseline_list{static_cast<int>(baseline_items.size()), baseline_items.data()};

	FarDialogItem items[CD_COUNT]{};
	items[CD_BOX].Type = DI_DOUBLEBOX;
	items[CD_BOX].X1 = 3;
	items[CD_BOX].Y1 = 1;
	items[CD_BOX].X2 = 58;
	items[CD_BOX].Y2 = 14;
	items[CD_BOX].PtrData = L"GitGutter settings";

	items[CD_ENABLED].Type = DI_CHECKBOX;
	items[CD_ENABLED].X1 = 5;
	items[CD_ENABLED].Y1 = 2;
	items[CD_ENABLED].Param.Selected = g_settings.enabled ? 1 : 0;
	items[CD_ENABLED].PtrData = L"Enabled";

	items[CD_BASELINE_TEXT].Type = DI_TEXT;
	items[CD_BASELINE_TEXT].X1 = 5;
	items[CD_BASELINE_TEXT].Y1 = 4;
	items[CD_BASELINE_TEXT].PtrData = L"Baseline";

	items[CD_BASELINE].Type = DI_COMBOBOX;
	items[CD_BASELINE].X1 = 20;
	items[CD_BASELINE].Y1 = 4;
	items[CD_BASELINE].X2 = 50;
	items[CD_BASELINE].Y2 = 4;
	items[CD_BASELINE].Flags = DIF_DROPDOWNLIST;
	items[CD_BASELINE].Param.ListItems = &baseline_list;

	items[CD_INTERVAL_TEXT].Type = DI_TEXT;
	items[CD_INTERVAL_TEXT].X1 = 5;
	items[CD_INTERVAL_TEXT].Y1 = 6;
	items[CD_INTERVAL_TEXT].PtrData = L"Update interval (ms)";

	items[CD_INTERVAL].Type = DI_EDIT;
	items[CD_INTERVAL].X1 = 28;
	items[CD_INTERVAL].Y1 = 6;
	items[CD_INTERVAL].X2 = 50;
	items[CD_INTERVAL].Y2 = 6;
	items[CD_INTERVAL].PtrData = interval_w.c_str();
	items[CD_INTERVAL].MaxLen = 10;

	items[CD_COLOR_ADDED_TEXT].Type = DI_TEXT;
	items[CD_COLOR_ADDED_TEXT].X1 = 5;
	items[CD_COLOR_ADDED_TEXT].Y1 = 8;
	items[CD_COLOR_ADDED_TEXT].PtrData = L"Color added";

	items[CD_COLOR_ADDED].Type = DI_EDIT;
	items[CD_COLOR_ADDED].X1 = 20;
	items[CD_COLOR_ADDED].Y1 = 8;
	items[CD_COLOR_ADDED].X2 = 49;
	items[CD_COLOR_ADDED].Y2 = 8;
	items[CD_COLOR_ADDED].PtrData = color_added_w.c_str();
	items[CD_COLOR_ADDED].MaxLen = 20;

	items[CD_COLOR_ADDED_PICK].Type = DI_BUTTON;
	items[CD_COLOR_ADDED_PICK].X1 = 51;
	items[CD_COLOR_ADDED_PICK].Y1 = 8;
	items[CD_COLOR_ADDED_PICK].X2 = 54;
	items[CD_COLOR_ADDED_PICK].Y2 = 8;
	items[CD_COLOR_ADDED_PICK].Flags = DIF_BTNNOCLOSE;
	items[CD_COLOR_ADDED_PICK].PtrData = L"...";

	items[CD_COLOR_MODIFIED_TEXT].Type = DI_TEXT;
	items[CD_COLOR_MODIFIED_TEXT].X1 = 5;
	items[CD_COLOR_MODIFIED_TEXT].Y1 = 9;
	items[CD_COLOR_MODIFIED_TEXT].PtrData = L"Color modified";

	items[CD_COLOR_MODIFIED].Type = DI_EDIT;
	items[CD_COLOR_MODIFIED].X1 = 20;
	items[CD_COLOR_MODIFIED].Y1 = 9;
	items[CD_COLOR_MODIFIED].X2 = 49;
	items[CD_COLOR_MODIFIED].Y2 = 9;
	items[CD_COLOR_MODIFIED].PtrData = color_modified_w.c_str();
	items[CD_COLOR_MODIFIED].MaxLen = 20;

	items[CD_COLOR_MODIFIED_PICK].Type = DI_BUTTON;
	items[CD_COLOR_MODIFIED_PICK].X1 = 51;
	items[CD_COLOR_MODIFIED_PICK].Y1 = 9;
	items[CD_COLOR_MODIFIED_PICK].X2 = 54;
	items[CD_COLOR_MODIFIED_PICK].Y2 = 9;
	items[CD_COLOR_MODIFIED_PICK].Flags = DIF_BTNNOCLOSE;
	items[CD_COLOR_MODIFIED_PICK].PtrData = L"...";

	items[CD_COLOR_DELETED_TEXT].Type = DI_TEXT;
	items[CD_COLOR_DELETED_TEXT].X1 = 5;
	items[CD_COLOR_DELETED_TEXT].Y1 = 10;
	items[CD_COLOR_DELETED_TEXT].PtrData = L"Color deleted";

	items[CD_COLOR_DELETED].Type = DI_EDIT;
	items[CD_COLOR_DELETED].X1 = 20;
	items[CD_COLOR_DELETED].Y1 = 10;
	items[CD_COLOR_DELETED].X2 = 49;
	items[CD_COLOR_DELETED].Y2 = 10;
	items[CD_COLOR_DELETED].PtrData = color_deleted_w.c_str();
	items[CD_COLOR_DELETED].MaxLen = 20;

	items[CD_COLOR_DELETED_PICK].Type = DI_BUTTON;
	items[CD_COLOR_DELETED_PICK].X1 = 51;
	items[CD_COLOR_DELETED_PICK].Y1 = 10;
	items[CD_COLOR_DELETED_PICK].X2 = 54;
	items[CD_COLOR_DELETED_PICK].Y2 = 10;
	items[CD_COLOR_DELETED_PICK].Flags = DIF_BTNNOCLOSE;
	items[CD_COLOR_DELETED_PICK].PtrData = L"...";

	items[CD_SEPARATOR].Type = DI_TEXT;
	items[CD_SEPARATOR].X1 = 3;
	items[CD_SEPARATOR].Y1 = 12;
	items[CD_SEPARATOR].Flags = DIF_SEPARATOR;

	items[CD_OK].Type = DI_BUTTON;
	items[CD_OK].X1 = 0;
	items[CD_OK].Y1 = 13;
	items[CD_OK].Flags = DIF_DEFAULT | DIF_CENTERGROUP;
	items[CD_OK].PtrData = L"OK";

	items[CD_CANCEL].Type = DI_BUTTON;
	items[CD_CANCEL].X1 = 0;
	items[CD_CANCEL].Y1 = 13;
	items[CD_CANCEL].Flags = DIF_CENTERGROUP;
	items[CD_CANCEL].PtrData = L"Cancel";

	auto dlg_proc = [](HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2) -> LONG_PTR
	{
		if (Msg == DN_INITDIALOG) {
			ApplyDialogEditColor(hDlg, CD_COLOR_ADDED, g_settings.color_added);
			ApplyDialogEditColor(hDlg, CD_COLOR_MODIFIED, g_settings.color_modified);
			ApplyDialogEditColor(hDlg, CD_COLOR_DELETED, g_settings.color_deleted);
		}
		if (Msg == DN_KILLFOCUS) {
			int edit_id = -1;
			switch (Param1) {
			case CD_COLOR_ADDED:
			case CD_COLOR_MODIFIED:
			case CD_COLOR_DELETED:
				edit_id = Param1;
				break;
			default:
				break;
			}
			if (edit_id != -1) {
				const wchar_t *ptr = reinterpret_cast<const wchar_t *>(
						g_info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, edit_id, 0));
				Settings::ColorPair color = ParseColorPair(ptr, Settings::ColorPair{0, 0});
				ApplyDialogEditColor(hDlg, edit_id, color);
			}
		}
		if (Msg == DN_BTNCLICK) {
			int edit_id = -1;
			switch (Param1) {
			case CD_COLOR_ADDED_PICK:
				edit_id = CD_COLOR_ADDED;
				break;
			case CD_COLOR_MODIFIED_PICK:
				edit_id = CD_COLOR_MODIFIED;
				break;
			case CD_COLOR_DELETED_PICK:
				edit_id = CD_COLOR_DELETED;
				break;
			default:
				break;
			}
			if (edit_id != -1) {
				const wchar_t *ptr = reinterpret_cast<const wchar_t *>(
						g_info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, edit_id, 0));
				Settings::ColorPair color = ParseColorPair(ptr, Settings::ColorPair{0, 0});
				uint64_t dlg_color = MakeTrueColor(color.fg) | MakeTrueColorBack(color.bg);
				if (g_info.ColorDialog(0, &dlg_color)) {
					if (dlg_color & FOREGROUND_TRUECOLOR) {
						const uint32_t fg = static_cast<uint32_t>((dlg_color >> 16) & 0x00FFFFFF);
						color.fg = SwapRB(fg);
					}
					if (dlg_color & BACKGROUND_TRUECOLOR) {
						const uint32_t bg = static_cast<uint32_t>((dlg_color >> 40) & 0x00FFFFFF);
						color.bg = SwapRB(bg);
					}
					const std::wstring text = FormatColorPair(color);
					g_info.SendDlgMessage(hDlg, DM_SETTEXTPTR, edit_id, reinterpret_cast<LONG_PTR>(text.c_str()));
					ApplyDialogEditColor(hDlg, edit_id, color);
				}
			}
		}
		return g_info.DefDlgProc(hDlg, Msg, Param1, Param2);
	};

	HANDLE hdlg = g_info.DialogInit(
			g_info.ModuleNumber, -1, -1, 62, 16, L"GitGutter settings", items, CD_COUNT, 0, 0,
			dlg_proc, 0);
	if (hdlg == INVALID_HANDLE_VALUE) {
		return 0;
	}
	LONG_PTR rc = g_info.DialogRun(hdlg);
	if (rc == CD_OK) {
		g_settings.enabled = g_info.SendDlgMessage(hdlg, DM_GETCHECK, CD_ENABLED, 0) == BSTATE_CHECKED;
		const int baseline_pos = g_info.SendDlgMessage(hdlg, DM_LISTGETCURPOS, CD_BASELINE, 0);
		if (baseline_pos >= 0 && baseline_pos < static_cast<int>(baseline_values.size())) {
			g_settings.baseline = baseline_values[baseline_pos];
		} else {
			g_settings.baseline = "head";
		}
		const wchar_t *interval_ptr = reinterpret_cast<const wchar_t *>(
				g_info.SendDlgMessage(hdlg, DM_GETCONSTTEXTPTR, CD_INTERVAL, 0));
		if (interval_ptr && *interval_ptr) {
			const long interval_val = std::wcstol(interval_ptr, nullptr, 10);
			g_settings.interval_ms = std::max(50, static_cast<int>(interval_val));
		}
		g_settings.color_added = ParseColorPair(reinterpret_cast<const wchar_t *>(
				g_info.SendDlgMessage(hdlg, DM_GETCONSTTEXTPTR, CD_COLOR_ADDED, 0)), g_settings.color_added);
		g_settings.color_modified = ParseColorPair(reinterpret_cast<const wchar_t *>(
				g_info.SendDlgMessage(hdlg, DM_GETCONSTTEXTPTR, CD_COLOR_MODIFIED, 0)), g_settings.color_modified);
		g_settings.color_deleted = ParseColorPair(reinterpret_cast<const wchar_t *>(
				g_info.SendDlgMessage(hdlg, DM_GETCONSTTEXTPTR, CD_COLOR_DELETED, 0)), g_settings.color_deleted);
		if (!g_settings.enabled) {
			EditorGutterMarks gm{};
			g_info.EditorControl(ECTL_SETGUTTERMARKS, &gm);
		}
		g_settings.Save();
		if (g_settings.enabled) {
			EditorInfo ei{};
			if (GetEditorInfo(ei)) {
				EditorState &st = g_editors[ei.EditorID];
				st.editor_id = ei.EditorID;
				st.dirty = true;
				UpdateEditorState(st);
				ApplyGutterRequest(st);
			}
		}
	}
	g_info.DialogFree(hdlg);
	return 1;
}

SHAREDSYMBOL int WINAPI ConfigureW(int ItemNumber)
{
	(void)ItemNumber;
	g_settings.Load();
	g_git_available = CheckGitAvailable();
	if (!g_git_available) {
		return 0;
	}
	return ShowConfigDialog();
}

SHAREDSYMBOL HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item)
{
	(void)OpenFrom;
	(void)Item;
	g_settings.Load();
	g_git_available = CheckGitAvailable();
	if (!g_git_available) {
		return INVALID_HANDLE_VALUE;
	}
	ShowConfigDialog();
	return INVALID_HANDLE_VALUE;
}

SHAREDSYMBOL int WINAPI ProcessEditorEventW(int Event, void *Param)
{
	if (!IsPluginActive()) {
		return 0;
	}
	EditorInfo ei{};
	if (!GetEditorInfo(ei)) {
		return 0;
	}

	if (Event == EE_CLOSE) {
		int closed_id = -1;
		if (Param) {
			closed_id = *reinterpret_cast<int *>(Param);
		}
		if (closed_id != -1) {
			auto it = g_editors.find(closed_id);
			if (it != g_editors.end() && !it->second.temp_path.empty()) {
				unlink(it->second.temp_path.c_str());
			}
			g_editors.erase(closed_id);
		} else {
			auto it = g_editors.find(ei.EditorID);
			if (it != g_editors.end() && !it->second.temp_path.empty()) {
				unlink(it->second.temp_path.c_str());
			}
			g_editors.erase(ei.EditorID);
		}
		return 0;
	}

	EditorState &st = g_editors[ei.EditorID];
	st.editor_id = ei.EditorID;

	if (Event == EE_READ || Event == EE_SAVE) {
		st.dirty = true;
		UpdateEditorState(st);
		if (st.gutter_request != -1) {
			g_info.AdvControl(g_info.ModuleNumber, ACTL_SYNCHRO, nullptr, nullptr);
		}
	} else if (Event == EE_REDRAW) {
		if (st.dirty) {
			UpdateEditorState(st);
			if (st.gutter_request != -1) {
				g_info.AdvControl(g_info.ModuleNumber, ACTL_SYNCHRO, nullptr, nullptr);
			}
		}
	}

	return 0;
}

SHAREDSYMBOL int WINAPI ProcessSynchroEventW(int Event, void *Param)
{
	(void)Param;
	if (!IsPluginActive()) {
		return 0;
	}
	if (Event != SE_COMMONSYNCHRO)
		return 0;

	bool did_work = false;

	if (g_pending_popup.active) {
		const int editor_id = g_pending_popup.editor_id;
		const int line = g_pending_popup.line;
		const int anchor_x = g_pending_popup.x;
		const int anchor_y = g_pending_popup.y;
		g_pending_popup.active = false;

		auto it = g_editors.find(editor_id);
		if (it != g_editors.end()) {
			ShowHunkPopup(it->second, line, anchor_x, anchor_y);
			did_work = true;
		}
	}

	if (g_pending_tick) {
		g_pending_tick = false;
		EditorInfo ei{};
		if (GetEditorInfo(ei)) {
			EditorState &st = g_editors[ei.EditorID];
			st.editor_id = ei.EditorID;
			if (st.dirty) {
				UpdateEditorState(st);
				did_work = true;
			}
			ApplyGutterRequest(st);
		}
	}
	EditorInfo ei{};
	if (GetEditorInfo(ei)) {
		EditorState &st = g_editors[ei.EditorID];
		st.editor_id = ei.EditorID;
		if (st.gutter_request != -1) {
			ApplyGutterRequest(st);
			did_work = true;
		}
	}

	return did_work ? 1 : 0;
}

SHAREDSYMBOL int WINAPI ProcessEditorInputW(const INPUT_RECORD *ir)
{
	if (!IsPluginActive()) {
		return 0;
	}
	if (!ir) {
		return 0;
	}
	if (IsLikelyEditInput(ir)) {
		EditorInfo ei{};
		if (GetEditorInfo(ei)) {
			EditorState &st = g_editors[ei.EditorID];
			st.editor_id = ei.EditorID;
			st.dirty = true;
		}
		MaybeScheduleTick();
	}
	if (HandleGutterClick(ir)) {
		return 1;
	}
	return 0;
}

SHAREDSYMBOL void WINAPI ExitFARW()
{
	g_editors.clear();
}
