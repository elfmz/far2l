#include "SDLFontManager.h"

#include "SDLBackendUtils.h"
#include "SDLFontDialog.h"
#include "TestPath.h"
#include "utils.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <limits.h>

#if !defined(__APPLE__)
#include <fontconfig/fontconfig.h>
#endif

#if defined(__APPLE__)
#include <CoreText/CoreText.h>
#include <CoreGraphics/CoreGraphics.h>
#endif

namespace
{

struct FontPathDescriptor
{
	std::string path;
	int face_index{-1};
};

static FontPathDescriptor ParseFontPathDescriptor(const std::string &value)
{
	FontPathDescriptor desc;
	desc.face_index = -1;
	std::string trimmed = TrimString(value);
	if (trimmed.empty()) {
		return desc;
	}
	const std::string suffix = "|face=";
	const size_t pos = trimmed.rfind(suffix);
	if (pos != std::string::npos) {
		const std::string index_str = trimmed.substr(pos + suffix.size());
		trimmed = TrimString(trimmed.substr(0, pos));
		if (!index_str.empty()) {
			char *end = nullptr;
			long parsed = std::strtol(index_str.c_str(), &end, 10);
			if (end != index_str.c_str()) {
				desc.face_index = static_cast<int>(parsed);
			}
		}
	}
	desc.path = trimmed;
	return desc;
}

static std::string BuildFontPathDescriptor(const std::string &path, int face_index)
{
	if (face_index < 0) {
		return path;
	}
	return path + "|face=" + std::to_string(face_index);
}

static std::string FoldAscii(std::string value)
{
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	return value;
}

static bool ContainsCaseInsensitive(const std::string &haystack, const char *needle)
{
	if (haystack.empty() || !needle || !*needle) {
		return false;
	}
	return FoldAscii(haystack).find(FoldAscii(needle)) != std::string::npos;
}

static void AppendUniqueFontDescriptors(std::vector<std::string> &out, const std::vector<std::string> &candidates)
{
	for (const auto &candidate : candidates) {
		if (candidate.empty()) {
			continue;
		}
		if (std::find(out.begin(), out.end(), candidate) == out.end()) {
			out.emplace_back(candidate);
		}
	}
}

static bool FaceLooksEmoji(FT_Face face, const std::string &path)
{
	if (!face) {
		return false;
	}
	if ((face->face_flags & FT_FACE_FLAG_COLOR) != 0) {
		return true;
	}
	if (ContainsCaseInsensitive(path, "emoji")) {
		return true;
	}
	if (face->family_name && ContainsCaseInsensitive(face->family_name, "emoji")) {
		return true;
	}
	if (face->style_name && ContainsCaseInsensitive(face->style_name, "emoji")) {
		return true;
	}
	return false;
}

static bool ClusterPrefersEmojiPresentation(const std::u32string &cluster)
{
	unsigned int regional_indicator_count = 0;
	for (char32_t codepoint : cluster) {
		if (codepoint == 0xFE0F || codepoint == 0x200D || codepoint == 0x20E3) {
			return true;
		}
		if (codepoint >= 0x1F3FB && codepoint <= 0x1F3FF) {
			return true;
		}
		if (codepoint >= 0x1F1E6 && codepoint <= 0x1F1FF) {
			++regional_indicator_count;
		}
	}
	return regional_indicator_count >= 2;
}

#if !defined(__APPLE__)
static bool AppendFontPathFromFontconfigDescriptor(std::vector<std::string> &out, const std::string &descriptor)
{
	if (descriptor.empty()) {
		return false;
	}
	FcPattern *pattern = FcNameParse(reinterpret_cast<const FcChar8 *>(descriptor.c_str()));
	if (!pattern) {
		return false;
	}
	FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
	FcDefaultSubstitute(pattern);
	FcResult res = FcResultMatch;
	FcPattern *font = FcFontMatch(nullptr, pattern, &res);
	FcPatternDestroy(pattern);
	if (!font || res != FcResultMatch) {
		if (font) {
			FcPatternDestroy(font);
		}
		return false;
	}
	FcChar8 *file = nullptr;
	int index = 0;
	if (FcPatternGetString(font, FC_FILE, 0, &file) != FcResultMatch) {
		FcPatternDestroy(font);
		return false;
	}
	FcPatternGetInteger(font, FC_INDEX, 0, &index);
	const std::string descriptor_out = BuildFontPathDescriptor(reinterpret_cast<const char *>(file), index);
	FcPatternDestroy(font);
	out.emplace_back(descriptor_out);
	return true;
}

static void AppendFontPathFromFontconfigFamily(std::vector<std::string> &out, const std::string &family, bool require_fixed)
{
	if (family.empty()) {
		return;
	}
	FcPattern *pattern = FcPatternCreate();
	if (!pattern) {
		return;
	}
	FcPatternAddString(pattern, FC_FAMILY, reinterpret_cast<const FcChar8 *>(family.c_str()));
	if (require_fixed) {
		FcPatternAddBool(pattern, FC_SPACING, FC_MONO);
	}
	FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
	FcDefaultSubstitute(pattern);
	FcResult res = FcResultMatch;
	FcPattern *font = FcFontMatch(nullptr, pattern, &res);
	FcPatternDestroy(pattern);
	if (!font || res != FcResultMatch) {
		if (font) {
			FcPatternDestroy(font);
		}
		return;
	}
	FcChar8 *file = nullptr;
	int index = 0;
	if (FcPatternGetString(font, FC_FILE, 0, &file) != FcResultMatch) {
		FcPatternDestroy(font);
		return;
	}
	FcPatternGetInteger(font, FC_INDEX, 0, &index);
	const std::string descriptor = BuildFontPathDescriptor(reinterpret_cast<const char *>(file), index);
	FcPatternDestroy(font);
	out.emplace_back(descriptor);
}

static int AppendFontPathsFromFontconfigColorFonts(std::vector<std::string> &out)
{
	FcPattern *pattern = FcPatternCreate();
	if (!pattern) {
		return -1;
	}
	FcPatternAddBool(pattern, FC_COLOR, FcTrue);
	FcObjectSet *os = FcObjectSetBuild(FC_FILE, FC_INDEX, nullptr);
	if (!os) {
		FcPatternDestroy(pattern);
		return -1;
	}
	FcFontSet *set = FcFontList(nullptr, pattern, os);
	FcObjectSetDestroy(os);
	FcPatternDestroy(pattern);
	if (!set) {
		return -1;
	}
	for (int i = 0; i < set->nfont; ++i) {
		FcPattern *font = set->fonts[i];
		FcChar8 *file = nullptr;
		int index = 0;
		if (FcPatternGetString(font, FC_FILE, 0, &file) != FcResultMatch) {
			continue;
		}
		FcPatternGetInteger(font, FC_INDEX, 0, &index);
		const std::string descriptor = BuildFontPathDescriptor(reinterpret_cast<const char *>(file), index);
		out.emplace_back(descriptor);
	}
	FcFontSetDestroy(set);
	return static_cast<int>(out.size());
}
#endif

static void AppendFontListFromEnv(std::vector<std::string> &out, const char *env)
{
	if (!env || !*env) {
		return;
	}
	std::string line = env;
	size_t start = 0;
	while (start < line.size()) {
		size_t end = line.find_first_of(";,", start);
		if (end == std::string::npos) {
			end = line.size();
		}
		const std::string entry = TrimString(line.substr(start, end - start));
		if (!entry.empty()) {
			out.emplace_back(entry);
		}
		start = end + 1;
	}
}

static bool AppendFontPathIfReadable(std::vector<std::string> &out, const std::string &candidate)
{
	if (candidate.empty()) {
		return false;
	}
	const FontPathDescriptor desc = ParseFontPathDescriptor(candidate);
	if (desc.path.empty()) {
		return false;
	}
	const std::string expanded = ExpandUserPath(desc.path);
	if (expanded.empty()) {
		return false;
	}
	if (!TestPath(expanded).Regular()) {
		return false;
	}
	const std::string descriptor = BuildFontPathDescriptor(expanded, desc.face_index);
	out.emplace_back(descriptor);
	return true;
}

static std::vector<std::string> DefaultFontCandidates()
{
	std::vector<std::string> out;
	AppendFontListFromEnv(out, getenv("FAR2L_SDL_FONT"));
#if defined(__APPLE__)
	out.emplace_back("/System/Library/Fonts/Menlo.ttc");
	out.emplace_back("/System/Library/Fonts/SFNSMono.ttf");
	out.emplace_back("/System/Library/Fonts/SFMono-Regular.otf");
	out.emplace_back("/System/Library/Fonts/Monaco.ttf");
#else
	AppendFontPathFromFontconfigFamily(out, "DejaVu Sans Mono", true);
	AppendFontPathFromFontconfigFamily(out, "Liberation Mono", true);
	AppendFontPathFromFontconfigFamily(out, "Ubuntu Mono", true);
	AppendFontPathFromFontconfigFamily(out, "FreeMono", true);
	out.emplace_back("/usr/share/fonts/truetype/ptmono/PTM75F.ttf");
	out.emplace_back("/usr/share/fonts/truetype/ptmono/PTM55F.ttf");
	out.emplace_back("/usr/share/fonts/truetype/pt-mono/PTMono-Bold.ttf");
	out.emplace_back("/usr/share/fonts/truetype/pt-mono/PTMono-Regular.ttf");
	out.emplace_back("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf");
	out.emplace_back("/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf");
	out.emplace_back("/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf");
	out.emplace_back("/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf");
	out.emplace_back("/usr/share/fonts/truetype/freefont/FreeMono.ttf");
#endif
	return out;
}

static std::vector<std::string> DefaultFallbackFontCandidates()
{
	std::vector<std::string> out;
	AppendFontListFromEnv(out, getenv("FAR2L_SDL_FONT_FALLBACK"));
#if defined(__APPLE__)
	out.emplace_back("/System/Library/Fonts/Apple Color Emoji.ttc");
	out.emplace_back("/System/Library/Fonts/Supplemental/Arial Unicode.ttf");
	out.emplace_back("/System/Library/Fonts/Supplemental/PingFang.ttc");
	out.emplace_back("/System/Library/Fonts/Supplemental/Songti.ttc");
	out.emplace_back("/System/Library/Fonts/Supplemental/Hiragino Sans GB.ttc");
#else
	if (AppendFontPathsFromFontconfigColorFonts(out) == 0) {
		static std::atomic<bool> s_warned{false};
		bool expected = false;
		if (s_warned.compare_exchange_strong(expected, true)) {
			fprintf(stderr, "SDLConsoleRenderer: no color emoji font found via fontconfig\n");
		}
	}
	AppendFontPathFromFontconfigFamily(out, "Noto Sans CJK", false);
	AppendFontPathFromFontconfigFamily(out, "DejaVu Sans", false);
	AppendFontPathFromFontconfigFamily(out, "Unifont", false);
	out.emplace_back("/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf");
	out.emplace_back("/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc");
	out.emplace_back("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
	out.emplace_back("/usr/share/fonts/truetype/unifont/unifont.ttf");
#endif
	return out;
}

static std::vector<std::string> ExtractFontNamesFromDescriptor(const std::string &descriptor)
{
	std::vector<std::string> names;
	std::string trimmed = TrimString(descriptor);
	if (trimmed.empty()) {
		return names;
	}
	const size_t colon = trimmed.find(':');
	if (colon != std::string::npos) {
		const std::string prefix = trimmed.substr(0, colon);
		if (!prefix.empty()) {
			names.emplace_back(prefix);
		}
		const std::string tail = trimmed.substr(colon + 1);
		trimmed = TrimString(tail);
		if (trimmed.empty()) {
			return names;
		}
	}

	const char *face_key = "face=";
	const size_t face_pos = trimmed.find(face_key);
	if (face_pos != std::string::npos) {
		size_t value_start = face_pos + strlen(face_key);
		size_t value_end = trimmed.find_first_of(",;", value_start);
		const std::string face = TrimString(trimmed.substr(value_start, value_end - value_start));
		if (!face.empty()) {
			names.emplace_back(face);
			return names;
		}
	}

	const size_t last_semicolon = trimmed.find_last_of(';');
	if (last_semicolon != std::string::npos && last_semicolon + 1 < trimmed.size()) {
		const std::string tail = TrimString(trimmed.substr(last_semicolon + 1));
		if (!tail.empty()) {
			names.emplace_back(tail);
			return names;
		}
	}

	if (!trimmed.empty()) {
		names.emplace_back(trimmed);
	}
	return names;
}

#if defined(__APPLE__)
static void AppendFontPathForName(const std::string &name, std::vector<std::string> &out)
{
	if (name.empty()) {
		return;
	}
	CFStringRef cf_name = CFStringCreateWithCString(kCFAllocatorDefault, name.c_str(), kCFStringEncodingUTF8);
	if (!cf_name) {
		return;
	}
	CTFontDescriptorRef desc = CTFontDescriptorCreateWithNameAndSize(cf_name, 0.0);
	CFRelease(cf_name);
	if (!desc) {
		return;
	}
	CFURLRef url = static_cast<CFURLRef>(CTFontDescriptorCopyAttribute(desc, kCTFontURLAttribute));
	CFRelease(desc);
	if (!url) {
		return;
	}
	char buffer[PATH_MAX];
	if (CFURLGetFileSystemRepresentation(url, true, reinterpret_cast<UInt8 *>(buffer), sizeof(buffer))) {
		AppendFontPathIfReadable(out, buffer);
	}
	CFRelease(url);
}
#else
static void AppendFontPathForName(const std::string &, std::vector<std::string> &) {}
#endif

static void AppendFontPathsFromConfig(std::vector<std::string> &out)
{
	const std::string config_path = InMyConfig("sdl_font", false);
	if (config_path.empty()) {
		return;
	}

	std::ifstream file(config_path);
	if (!file.is_open()) {
		return;
	}

	std::string line;
	while (std::getline(file, line)) {
		std::string trimmed = TrimString(line);
		if (trimmed.empty()) {
			continue;
		}

		int ignored_size = 0;
		if (ExtractFontPointSizeFromLine(trimmed, ignored_size)) {
			continue;
		}

#if !defined(__APPLE__)
		if (AppendFontPathFromFontconfigDescriptor(out, trimmed)) {
			continue;
		}
#endif

		if (AppendFontPathIfReadable(out, trimmed)) {
			continue;
		}

		const auto names = ExtractFontNamesFromDescriptor(trimmed);
		for (const auto &name : names) {
			AppendFontPathForName(name, out);
		}
	}
}

} // namespace

SDLFontManager::~SDLFontManager()
{
	Shutdown();
}

bool SDLFontManager::Initialize(float scale, bool subpixel_aa, FontHintingMode hinting_mode)
{
	_subpixel_aa = subpixel_aa;
	_hinting_mode = hinting_mode;
	return InitFontFaces(scale);
}

void SDLFontManager::Shutdown()
{
	ClearFontFaces();
}

bool SDLFontManager::Reload(float scale)
{
	ClearFontFaces();
	return InitFontFaces(scale);
}

bool SDLFontManager::InitFontFaces(float scale)
{
	if (FT_Init_FreeType(&_ft_library) != 0) {
		fprintf(stderr, "SDLConsoleRenderer: FT_Init_FreeType failed\n");
		return false;
	}
	if (_subpixel_aa) {
		FT_Library_SetLcdFilter(_ft_library, FT_LCD_FILTER_DEFAULT);
	}

	_font_pt = LoadFontPointSizeFromConfig();
	UpdateFontPixelSize(scale);

	std::vector<std::string> primary_candidates;
	AppendFontListFromEnv(primary_candidates, getenv("FAR2L_SDL_FONT"));
	std::vector<std::string> configured_primary_candidates;
	AppendFontPathsFromConfig(configured_primary_candidates);
	AppendUniqueFontDescriptors(primary_candidates, configured_primary_candidates);
	AppendUniqueFontDescriptors(primary_candidates, DefaultFontCandidates());

	bool primary_loaded = false;
	for (const auto &path : primary_candidates) {
		if (LoadFontFace(path, true)) {
			primary_loaded = true;
			break;
		}
	}

	if (!primary_loaded) {
		fprintf(stderr, "SDLConsoleRenderer: failed to load any primary font\n");
		FT_Done_FreeType(_ft_library);
		_ft_library = nullptr;
		return false;
	}

	std::vector<std::string> fallback_candidates;
	AppendUniqueFontDescriptors(fallback_candidates, DefaultFallbackFontCandidates());
	for (const auto &path : fallback_candidates) {
		LoadFontFace(path, false);
	}

	return true;
}

void SDLFontManager::ClearFontFaces()
{
	for (auto &slot : _font_faces) {
		if (slot.hb_font) {
			hb_font_destroy(slot.hb_font);
			slot.hb_font = nullptr;
		}
		if (slot.face) {
			FT_Done_Face(slot.face);
			slot.face = nullptr;
		}
	}
	_font_faces.clear();
	if (_ft_library) {
		FT_Done_FreeType(_ft_library);
		_ft_library = nullptr;
	}
}

bool SDLFontManager::LoadFontFace(const std::string &path_descriptor, bool primary)
{
	if (path_descriptor.empty() || !_ft_library) {
		return false;
	}

	const FontPathDescriptor desc = ParseFontPathDescriptor(path_descriptor);
	if (desc.path.empty()) {
		return false;
	}

	FT_Face face = OpenBestFace(desc.path, primary, desc.face_index);
	if (!face) {
		return false;
	}

	if (FT_Select_Charmap(face, FT_ENCODING_UNICODE) != 0) {
		FT_Done_Face(face);
		return false;
	}

	if (FT_Set_Pixel_Sizes(face, 0, _font_px) != 0) {
		if (!SelectBestBitmapStrike(face, _font_px)) {
			FT_Done_Face(face);
			return false;
		}
	}

	hb_font_t *hb_font = hb_ft_font_create_referenced(face);
	if (!hb_font) {
		FT_Done_Face(face);
		return false;
	}
	_font_faces.push_back(FontFace{
		face,
		hb_font,
		desc.path,
		static_cast<int>(face->face_index),
		primary,
		(face->face_flags & FT_FACE_FLAG_COLOR) != 0,
		FaceLooksEmoji(face, desc.path)});

	if (primary) {
		UpdatePrimaryFontMetrics(face);
	}
	return true;
}

bool SDLFontManager::SelectBestBitmapStrike(FT_Face face, int target_px)
{
	if (!face || !face->available_sizes || face->num_fixed_sizes <= 0) {
		return false;
	}
	int best_idx = -1;
	int best_above = std::numeric_limits<int>::max();
	for (int i = 0; i < face->num_fixed_sizes; ++i) {
		const int height = face->available_sizes[i].height;
		if (height >= target_px && height < best_above) {
			best_above = height;
			best_idx = i;
		}
	}
	if (best_idx < 0) {
		best_idx = 0;
		int best_delta = std::abs(face->available_sizes[0].height - target_px);
		for (int i = 1; i < face->num_fixed_sizes; ++i) {
			const int delta = std::abs(face->available_sizes[i].height - target_px);
			if (delta < best_delta) {
				best_delta = delta;
				best_idx = i;
			}
		}
	}
	return FT_Select_Size(face, best_idx) == 0;
}

FT_Face SDLFontManager::OpenBestFace(const std::string &path, bool primary, int desired_index)
{
	if (desired_index >= 0) {
		FT_Face exact = nullptr;
		if (FT_New_Face(_ft_library, path.c_str(), desired_index, &exact) == 0) {
			return exact;
		}
	}

	FT_Face first = nullptr;
	if (FT_New_Face(_ft_library, path.c_str(), 0, &first) != 0) {
		return nullptr;
	}

	const FT_Long total_faces = std::max<FT_Long>(1, first->num_faces);
	const FT_Long max_faces = std::min<FT_Long>(total_faces, 16);

	std::vector<FT_Face> opened;
	opened.reserve(static_cast<size_t>(max_faces));
	opened.push_back(first);

	FT_Face best_face = first;
	int best_score = ScoreFaceStyle(first, primary);

	for (FT_Long idx = 1; idx < max_faces; ++idx) {
		FT_Face candidate = nullptr;
		if (FT_New_Face(_ft_library, path.c_str(), idx, &candidate) != 0) {
			continue;
		}
		opened.push_back(candidate);
		const int score = ScoreFaceStyle(candidate, primary);
		if (score > best_score) {
			best_face = candidate;
			best_score = score;
		}
	}

	for (FT_Face entry : opened) {
		if (entry != best_face) {
			FT_Done_Face(entry);
		}
	}

	return best_face;
}

int SDLFontManager::ScoreFaceStyle(FT_Face face, bool primary) const
{
	if (!face) {
		return std::numeric_limits<int>::min();
	}

	int score = 0;

	if (primary && (face->face_flags & FT_FACE_FLAG_FIXED_WIDTH)) {
		score += 20;
	}
	if (!(face->style_flags & FT_STYLE_FLAG_BOLD)) {
		score += 10;
	}
	if (!(face->style_flags & FT_STYLE_FLAG_ITALIC)) {
		score += 6;
	}

	if (face->style_name) {
		std::string style = face->style_name;
		std::transform(style.begin(), style.end(), style.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		if (style.find("regular") != std::string::npos) {
			score += 8;
		}
		if (style.find("bold") != std::string::npos) {
			score -= 5;
		}
		if (style.find("italic") != std::string::npos || style.find("oblique") != std::string::npos) {
			score -= 5;
		}
	}

	return score;
}

void SDLFontManager::UpdatePrimaryFontMetrics(FT_Face face)
{
	if (!face || !face->size) {
		_cell_w = std::max(1, _font_px / 2);
		_ascent = std::max(1, _font_px);
		_descent = 1;
		_cell_h = std::max(1, _ascent + _descent);
		return;
	}

	const FT_Size_Metrics &metrics = face->size->metrics;

	int fallback_advance = static_cast<int>(metrics.max_advance >> 6);
	int max_ascent = static_cast<int>(metrics.ascender >> 6);
	int max_descent = static_cast<int>(std::abs(metrics.descender >> 6));

	fallback_advance = std::max(1, fallback_advance);
	max_ascent = std::max(1, max_ascent);
	max_descent = std::max(1, max_descent);

	static const std::u32string width_samples =
		U" 1234567890-=!@#$%^&*()_+qwertyuiop[]asdfghjkl;'`zxcvbnm,./\\"
		U"QWERTYUIOP{}ASDFGHJKL:\"~|ZXCVBNM<>?";
	static const std::u32string vertical_metric_samples =
		U"gjpqyQW@_|"
		U"ÇçÑñÀÁÂÃÄÅàáâãäåÈÉÊËèéêëÌÍÎÏìíîïÒÓÔÕÖØòóôõöøÙÚÛÜùúûüŸÿ"
		U"ЯЖБюяжбЙйФф"
		U"█▀▄│─╬╳╲╱"
		U"あ言語漢字";

	const bool face_has_color = (face->face_flags & FT_FACE_FLAG_COLOR) != 0;
	const bool request_lcd = _subpixel_aa && !face_has_color;
	const int load_flags = BuildGlyphLoadFlags(request_lcd, face_has_color);
	std::vector<int> ascii_advances;
	ascii_advances.reserve(width_samples.size());
	int ascii_max_advance = 0;
	int ascii_min_advance = std::numeric_limits<int>::max();
	int ascii_max_extent = 0;

	auto inspect = [&](char32_t codepoint, bool collect_width) {
		if (FT_Load_Char(face, codepoint, load_flags) != 0) {
			return;
		}

		const FT_GlyphSlot slot = face->glyph;
		if (!slot) {
			return;
		}

		const int top = slot->bitmap_top;
		const int bottom = top - slot->bitmap.rows;
		max_ascent = std::max(max_ascent, top);
		max_descent = std::max(max_descent, std::max(0, -bottom));

		if (!collect_width) {
			return;
		}

		int advance = static_cast<int>(slot->advance.x >> 6);
		if (advance <= 0 && slot->metrics.horiAdvance) {
			advance = static_cast<int>(slot->metrics.horiAdvance >> 6);
		}
		if (advance > 0) {
			ascii_advances.emplace_back(advance);
			ascii_min_advance = std::min(ascii_min_advance, advance);
			ascii_max_advance = std::max(ascii_max_advance, advance);
		}

		int bmp_w = slot->bitmap.width;
		if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_LCD && bmp_w >= 3) {
			bmp_w /= 3;
		}
		const int ink_right = slot->bitmap_left + std::max(0, bmp_w);
		ascii_max_extent = std::max(ascii_max_extent, ink_right);
	};

	for (char32_t codepoint : width_samples) {
		inspect(codepoint, true);
	}
	for (char32_t codepoint : vertical_metric_samples) {
		inspect(codepoint, false);
	}

	int measured_advance = 0;
	if (!ascii_advances.empty()) {
		std::sort(ascii_advances.begin(), ascii_advances.end());
		measured_advance = ascii_advances[ascii_advances.size() / 2];

		const bool fixed_width_face = (face->face_flags & FT_FACE_FLAG_FIXED_WIDTH) != 0;
		const int advance_spread = ascii_max_advance - ascii_advances.front();
		if (!fixed_width_face && advance_spread > 1) {
			measured_advance = ascii_max_advance;
		}
	}

	const int padding = std::max(1, _font_px / 16);
	int cell_w = std::max(measured_advance, ascii_max_extent);
	if (cell_w <= 0) {
		cell_w = fallback_advance;
	}
	_cell_w = std::max(1, cell_w);
	_ascent = std::max(1, max_ascent);
	_descent = std::max(1, max_descent);
	_cell_h = std::max(1, _ascent + _descent + padding);

	if (g_sdl_debug_redraw) {
		SDLDebugLog(
			"SDLFontManager: primary metrics family=%s style=%s fixed=%d cell=%dx%d ascent=%d descent=%d "
			"asciiMedian=%d asciiRange=%d..%d asciiInk=%d maxAdvance=%d px=%d",
			face->family_name ? face->family_name : "",
			face->style_name ? face->style_name : "",
			(face->face_flags & FT_FACE_FLAG_FIXED_WIDTH) != 0 ? 1 : 0,
			_cell_w,
			_cell_h,
			_ascent,
			_descent,
			measured_advance,
			ascii_advances.empty() ? 0 : ascii_min_advance,
			ascii_advances.empty() ? 0 : ascii_max_advance,
			ascii_max_extent,
			fallback_advance,
			_font_px);
	}
}

int SDLFontManager::BuildGlyphLoadFlags(bool request_lcd, bool face_has_color) const
{
	int load_flags = FT_LOAD_RENDER;
	if (face_has_color) {
		load_flags |= FT_LOAD_COLOR;
	} else if (request_lcd) {
		load_flags |= FT_LOAD_TARGET_LCD;
	} else {
		load_flags |= FT_LOAD_TARGET_LIGHT;
	}

	switch (_hinting_mode) {
	case FontHintingMode::Auto:
		load_flags |= FT_LOAD_FORCE_AUTOHINT;
		load_flags &= ~FT_LOAD_NO_HINTING;
		break;
	case FontHintingMode::None:
		load_flags |= FT_LOAD_NO_HINTING;
		load_flags &= ~FT_LOAD_FORCE_AUTOHINT;
		break;
	default:
		break;
	}
	return load_flags;
}

void SDLFontManager::UpdateFontPixelSize(float scale)
{
	const float s = (scale > 0.f) ? scale : 1.f;
	_font_px = std::max(1, static_cast<int>(std::floor(_font_pt * s)));
}

bool SDLFontManager::RasterizeCluster(const std::u32string &cluster, GlyphBitmap &out) const
{
	if (_font_faces.empty()) {
		return false;
	}
	const bool prefer_emoji = ClusterPrefersEmojiPresentation(cluster);
	std::vector<bool> tried(_font_faces.size(), false);

	auto try_face = [&](size_t idx) -> bool {
		if (idx >= _font_faces.size() || tried[idx]) {
			return false;
		}
		tried[idx] = true;
		if (!RasterizeClusterWithFace(cluster, _font_faces[idx], out)) {
			return false;
		}
		out.used_fallback = !_font_faces[idx].primary;
		if (g_sdl_debug_redraw) {
			if (prefer_emoji && _font_faces[idx].likely_emoji) {
				SDLDebugLog(
					"SDLFontManager: emoji-preferred face path=%s face=%d color=%d cluster_len=%zu",
					out.source_path.c_str(),
					out.source_face_index,
					out.is_color ? 1 : 0,
					cluster.size());
			} else if (out.used_fallback) {
				SDLDebugLog(
					"SDLFontManager: fallback face path=%s face=%d color=%d cluster_len=%zu",
					out.source_path.c_str(),
					out.source_face_index,
					out.is_color ? 1 : 0,
					cluster.size());
			}
		}
		return true;
	};

	if (prefer_emoji) {
		for (size_t idx = 0; idx < _font_faces.size(); ++idx) {
			if (_font_faces[idx].likely_emoji && try_face(idx)) {
				return true;
			}
		}
	}

	for (size_t idx = 0; idx < _font_faces.size(); ++idx) {
		if (_font_faces[idx].primary && try_face(idx)) {
			return true;
		}
	}

	for (size_t idx = 0; idx < _font_faces.size(); ++idx) {
		if (try_face(idx)) {
			return true;
		}
	}
	return false;
}

bool SDLFontManager::RasterizeClusterWithFace(const std::u32string &cluster, const FontFace &face, GlyphBitmap &out) const
{
	out = GlyphBitmap{};
	if (!face.face || !face.hb_font || cluster.empty()) {
		return false;
	}

	hb_buffer_t *buffer = hb_buffer_create();
	hb_buffer_add_utf32(
		buffer,
		reinterpret_cast<const uint32_t *>(cluster.data()),
		static_cast<int>(cluster.size()),
		0,
		static_cast<int>(cluster.size()));
	hb_buffer_guess_segment_properties(buffer);
	hb_shape(face.hb_font, buffer, nullptr, 0);

	unsigned int glyph_count = 0;
	const hb_glyph_info_t *info = hb_buffer_get_glyph_infos(buffer, &glyph_count);
	const hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(buffer, nullptr);
	if (!glyph_count || !pos) {
		hb_buffer_destroy(buffer);
		return false;
	}

	struct PendingGlyph
	{
		SDLSurfacePtr surface{nullptr};
		int draw_left{0};
		int draw_top{0};
		bool premultiplied{false};
	};

	std::vector<PendingGlyph> pending;
	pending.reserve(glyph_count);

	int pen_x = 0;
	int pen_y = 0;
	int min_left = std::numeric_limits<int>::max();
	int max_right = std::numeric_limits<int>::min();
	int max_top = std::numeric_limits<int>::min();
	int min_bottom = std::numeric_limits<int>::max();
	bool any_color = false;

	for (unsigned int i = 0; i < glyph_count; ++i) {
		const hb_codepoint_t glyph_index = info[i].codepoint;
		if (glyph_index == 0 && cluster[0] != U' ') {
			hb_buffer_destroy(buffer);
			return false;
		}

		const bool face_has_color = (face.face->face_flags & FT_FACE_FLAG_COLOR) != 0;
		const bool request_lcd = _subpixel_aa && !face_has_color;
		const int load_flags = BuildGlyphLoadFlags(request_lcd, face_has_color);

		if (FT_Load_Glyph(face.face, glyph_index, load_flags) != 0) {
			hb_buffer_destroy(buffer);
			return false;
		}

		FT_GlyphSlot slot = face.face->glyph;
		if (!slot) {
			hb_buffer_destroy(buffer);
			return false;
		}

		const bool glyph_is_color = (slot->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA);
		const bool glyph_is_lcd = (!glyph_is_color && slot->bitmap.pixel_mode == FT_PIXEL_MODE_LCD);

		const int bmp_w_raw = slot->bitmap.width;
		const int bmp_h = slot->bitmap.rows;
		const int bmp_w = glyph_is_lcd && bmp_w_raw >= 3 ? bmp_w_raw / 3 : bmp_w_raw;
		const int surf_w = bmp_w > 0 ? bmp_w : 1;
		const int surf_h = bmp_h > 0 ? bmp_h : 1;
		const Uint32 surf_format = glyph_is_color ? SDL_PIXELFORMAT_BGRA32 : SDL_PIXELFORMAT_RGBA32;
		SDL_Surface *glyph_surface = SDL_CreateRGBSurfaceWithFormat(0, surf_w, surf_h, 32, surf_format);
		if (!glyph_surface) {
			hb_buffer_destroy(buffer);
			return false;
		}
		SDL_FillRect(glyph_surface, nullptr, 0);

		any_color = any_color || glyph_is_color;

		if (surf_w > 0 && bmp_h > 0 && slot->bitmap.buffer) {
			const Uint8 *src = slot->bitmap.buffer;
			const int src_pitch = slot->bitmap.pitch;
			Uint32 *dst = static_cast<Uint32 *>(glyph_surface->pixels);
			const int dst_pitch = glyph_surface->pitch / 4;

			if (glyph_is_color) {
				for (int y = 0; y < bmp_h; ++y) {
					const Uint8 *src_row = src + y * src_pitch;
					Uint32 *dst_row = dst + y * dst_pitch;
					for (int x = 0; x < bmp_w; ++x) {
						const Uint8 b = src_row[x * 4 + 0];
						const Uint8 g = src_row[x * 4 + 1];
						const Uint8 r = src_row[x * 4 + 2];
						const Uint8 a = src_row[x * 4 + 3];
						dst_row[x] = (static_cast<Uint32>(a) << 24) |
						             (static_cast<Uint32>(r) << 16) |
						             (static_cast<Uint32>(g) << 8) |
						             static_cast<Uint32>(b);
					}
				}
			} else if (glyph_is_lcd) {
				for (int y = 0; y < bmp_h; ++y) {
					const Uint8 *src_row = src + y * src_pitch;
					Uint32 *dst_row = dst + y * dst_pitch;
					for (int x = 0; x < bmp_w; ++x) {
						const Uint8 r = src_row[x * 3 + 0];
						const Uint8 g = src_row[x * 3 + 1];
						const Uint8 b = src_row[x * 3 + 2];
						const Uint8 a = static_cast<Uint8>((static_cast<int>(r) + g + b) / 3);
						dst_row[x] = (static_cast<Uint32>(a) << 24) |
						             (static_cast<Uint32>(r) << 16) |
						             (static_cast<Uint32>(g) << 8) |
						             static_cast<Uint32>(b);
					}
				}
			} else {
				for (int y = 0; y < bmp_h; ++y) {
					const Uint8 *src_row = src + y * src_pitch;
					Uint32 *dst_row = dst + y * dst_pitch;
					for (int x = 0; x < bmp_w; ++x) {
						const Uint8 a = src_row[x];
						dst_row[x] = (static_cast<Uint32>(a) << 24) | 0x00ffffff;
					}
				}
			}
		}

		PendingGlyph pg;
		pg.surface.reset(glyph_surface);
		const int glyph_x = pen_x + (pos[i].x_offset >> 6);
		const int glyph_y = pen_y + (pos[i].y_offset >> 6);
		pg.draw_left = glyph_x + slot->bitmap_left;
		pg.draw_top = glyph_y + slot->bitmap_top;
		pg.premultiplied = glyph_is_color;

		min_left = std::min(min_left, pg.draw_left);
		max_right = std::max(max_right, pg.draw_left + bmp_w);
		max_top = std::max(max_top, pg.draw_top);
		min_bottom = std::min(min_bottom, pg.draw_top - bmp_h);
		pen_x += (pos[i].x_advance >> 6);
		pen_y += (pos[i].y_advance >> 6);

		pending.push_back(std::move(pg));
	}

	if (pending.empty()) {
		hb_buffer_destroy(buffer);
		return false;
	}

	const int width = std::max(1, max_right - min_left);
	const int height = std::max(1, max_top - min_bottom);
	SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
	if (!surface) {
		hb_buffer_destroy(buffer);
		return false;
	}
	SDL_FillRect(surface, nullptr, 0);

	for (size_t i = 0; i < pending.size(); ++i) {
		PendingGlyph &pg = pending[i];
		if (!pg.surface) {
			continue;
		}
		SDL_Rect dst{
			pg.draw_left - min_left,
			max_top - pg.draw_top,
			pg.surface->w,
			pg.surface->h
		};
		if (pg.premultiplied) {
			SDL_SetSurfaceBlendMode(pg.surface.get(), SDL_BLENDMODE_NONE);
		}
		SDL_BlitSurface(pg.surface.get(), nullptr, surface, &dst);
	}

	const int advance = pen_x;
	const int bearing_x = min_left;
	const int bearing_y = max_top;

	out.surface.reset(surface);
	out.width = width;
	out.height = height;
	out.bearing_x = bearing_x;
	out.bearing_y = bearing_y;
	out.advance = advance;
	out.is_color = any_color;
	out.source_path = face.path;
	out.source_face_index = face.face_index;

	hb_buffer_destroy(buffer);
	return true;
}

int ClampFontPointSize(int pt)
{
	if (pt < kMinFontPointSize) {
		return kMinFontPointSize;
	}
	if (pt > kMaxFontPointSize) {
		return kMaxFontPointSize;
	}
	return pt;
}

int NormalizeFontPointSize(float pt)
{
	if (!std::isfinite(pt) || pt <= 0.f) {
		return kDefaultFontPointSize;
	}
	const int rounded = static_cast<int>(std::lround(pt));
	if (rounded <= 0) {
		return kDefaultFontPointSize;
	}
	return ClampFontPointSize(rounded);
}

bool ExtractFontPointSizeFromLine(const std::string &line, int &out_pt)
{
	static const std::string prefix = "size=";
	if (line.compare(0, prefix.size(), prefix) != 0) {
		return false;
	}
	const std::string tail = TrimString(line.substr(prefix.size()));
	if (tail.empty()) {
		return false;
	}
	char *end = nullptr;
	const float value = std::strtof(tail.c_str(), &end);
	if (end == tail.c_str()) {
		return false;
	}
	out_pt = NormalizeFontPointSize(value);
	return true;
}

bool SaveFontPreference(const std::string &path, int face_index, int point_size)
{
	if (path.empty()) {
		return false;
	}

	const int normalized_size = point_size > 0 ? ClampFontPointSize(point_size) : kDefaultFontPointSize;
	const std::string descriptor = BuildFontPathDescriptor(path, face_index);

	std::vector<std::string> existing;
	const std::string config_path = InMyConfig("sdl_font");
	{
		std::ifstream in(config_path);
		std::string line;
		while (std::getline(in, line)) {
			const std::string trimmed = TrimString(line);
			if (trimmed.empty()) {
				continue;
			}
			int ignored = 0;
			if (ExtractFontPointSizeFromLine(trimmed, ignored)) {
				continue;
			}
			if (trimmed != descriptor) {
				existing.emplace_back(trimmed);
			}
		}
	}

	std::ofstream out(config_path, std::ios::trunc);
	if (!out.is_open()) {
		fprintf(stderr, "SDLConsoleRenderer: failed to write font settings %s\n", config_path.c_str());
		return false;
	}

	out << descriptor << '\n';
	out << "size=" << normalized_size << '\n';
	for (const auto &line : existing) {
		out << line << '\n';
	}
	return true;
}

bool EnsureFontPreferenceSelected()
{
	const std::string config_path = InMyConfig("sdl_font", false);
	if (!config_path.empty() && TestPath(config_path).Regular()) {
		return true;
	}
	SDLFontSelection selection;
	const SDLFontDialogStatus status = SDLShowFontPicker(selection);
	if (status != SDLFontDialogStatus::Chosen) {
		return false;
	}
	const std::string descriptor = selection.fc_name.empty() ? selection.path : selection.fc_name;
	const int descriptor_face = selection.fc_name.empty() ? selection.face_index : -1;
	if (descriptor.empty()) {
		return false;
	}
	const int chosen_size = (selection.point_size > 0.0f)
		? NormalizeFontPointSize(selection.point_size)
		: kDefaultFontPointSize;
	return SaveFontPreference(descriptor, descriptor_face, chosen_size);
}

int LoadFontPointSizeFromConfig()
{
	const std::string config_path = InMyConfig("sdl_font", false);
	if (config_path.empty()) {
		return kDefaultFontPointSize;
	}

	std::ifstream file(config_path);
	if (!file.is_open()) {
		return kDefaultFontPointSize;
	}

	std::string line;
	while (std::getline(file, line)) {
		const std::string trimmed = TrimString(line);
		if (trimmed.empty()) {
			continue;
		}
		int parsed = 0;
		if (ExtractFontPointSizeFromLine(trimmed, parsed)) {
			return parsed;
		}
	}
	return kDefaultFontPointSize;
}
