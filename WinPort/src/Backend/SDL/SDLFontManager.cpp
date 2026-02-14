#include "SDLFontManager.h"

#include "SDLBackendUtils.h"
#include "SDLFontDialog.h"
#include "TestPath.h"
#include "utils.h"

#include <algorithm>
#include <atomic>
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

	bool primary_loaded = false;
	auto primary_candidates = DefaultFontCandidates();
	auto config_candidates = primary_candidates;
	AppendFontPathsFromConfig(config_candidates);
	for (const auto &path : config_candidates) {
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

	const auto fallback_candidates = DefaultFallbackFontCandidates();
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
	_font_faces.push_back(FontFace{face, hb_font, desc.path, desc.face_index});

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
	const FT_Size_Metrics &metrics = face->size->metrics;

	int max_advance = static_cast<int>(metrics.max_advance >> 6);
	int max_ascent = static_cast<int>(metrics.ascender >> 6);
	int max_descent = static_cast<int>(std::abs(metrics.descender >> 6));

	max_advance = std::max(1, max_advance);
	max_ascent = std::max(1, max_ascent);
	max_descent = std::max(1, max_descent);

	static const std::u32string samples =
		U"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
		U"!@#$%^&*()[]{}<>?/\\|`~_-+=,.;:'\""
		U"█▀▄│─╬╳╲╱"
		U"ÇçÑñÀÁÂÃÄÅàáâãäåÈÉÊËèéêëÌÍÎÏìíîïÒÓÔÕÖØòóôõöøÙÚÛÜùúûüŸÿ"
		U"ЯЖБюяжбЙйФф"
		U"あ言語漢字";

	const bool face_has_color = (face->face_flags & FT_FACE_FLAG_COLOR) != 0;
	const bool request_lcd = _subpixel_aa && !face_has_color;
	for (char32_t codepoint : samples) {
		if (FT_Load_Char(face, codepoint, BuildGlyphLoadFlags(request_lcd, face_has_color)) != 0) {
			continue;
		}

		const FT_GlyphSlot slot = face->glyph;
		int advance = static_cast<int>(slot->advance.x >> 6);
		if (advance <= 0 && slot->metrics.horiAdvance) {
			advance = static_cast<int>(slot->metrics.horiAdvance >> 6);
		}
		if (advance > 0) {
			max_advance = std::max(max_advance, advance);
		}

		const int top = slot->bitmap_top;
		const int bottom = top - slot->bitmap.rows;
		max_ascent = std::max(max_ascent, top);
		max_descent = std::max(max_descent, std::max(0, -bottom));
	}

	const int padding = std::max(1, _font_px / 16);
	_cell_w = std::max(1, max_advance);
	_ascent = std::max(1, max_ascent);
	_descent = std::max(1, max_descent);
	_cell_h = std::max(1, _ascent + _descent + padding);
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
	if (RasterizeClusterWithFace(cluster, _font_faces.front(), out)) {
		return true;
	}
	for (size_t idx = 1; idx < _font_faces.size(); ++idx) {
		if (RasterizeClusterWithFace(cluster, _font_faces[idx], out)) {
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
		int left{0};
		int top{0};
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
		pg.left = slot->bitmap_left;
		pg.top = slot->bitmap_top;

		pen_x += (pos[i].x_offset >> 6);
		pen_y += (pos[i].y_offset >> 6);
		min_left = std::min(min_left, pen_x + pg.left);
		max_right = std::max(max_right, pen_x + pg.left + bmp_w);
		max_top = std::max(max_top, pen_y + pg.top);
		min_bottom = std::min(min_bottom, pen_y + pg.top - bmp_h);
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
			pg.left - min_left,
			max_top - pg.top,
			pg.surface->w,
			pg.surface->h
		};
		SDL_BlitSurface(pg.surface.get(), nullptr, surface, &dst);
	}

	const hb_glyph_position_t *last_pos = pos + (glyph_count - 1);
	const int advance = (pen_x + (last_pos->x_advance >> 6));
	const int bearing_x = min_left;
	const int bearing_y = max_top;

	out.surface.reset(surface);
	out.width = width;
	out.height = height;
	out.bearing_x = bearing_x;
	out.bearing_y = bearing_y;
	out.advance = advance;
	out.is_color = any_color;

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
