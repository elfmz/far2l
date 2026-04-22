#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <SDL.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H

#ifndef FT_LOAD_COLOR
#define FT_LOAD_COLOR 0
#endif

#include <hb.h>
#include <hb-ft.h>

#include "SDLShared.h"

class SDLFontManager
{
public:
	struct SDLSurfaceDeleter
	{
		void operator()(SDL_Surface *surface) const
		{
			if (surface) {
				SDL_FreeSurface(surface);
			}
		}
	};

	using SDLSurfacePtr = std::unique_ptr<SDL_Surface, SDLSurfaceDeleter>;

	struct GlyphBitmap
	{
		SDLSurfacePtr surface{nullptr};
		int width{0};
		int height{0};
		int bearing_x{0};
		int bearing_y{0};
		int advance{0};
		bool is_color{false};
		bool used_fallback{false};
		std::string source_path;
		int source_face_index{-1};
	};

	SDLFontManager() = default;
	~SDLFontManager();

	bool Initialize(float scale, bool subpixel_aa, FontHintingMode hinting_mode);
	void Shutdown();
	bool Reload(float scale);
	bool HasFaces() const { return !_font_faces.empty(); }

	bool RasterizeCluster(const std::u32string &cluster, GlyphBitmap &out) const;

	int FontPt() const { return _font_pt; }
	int FontPx() const { return _font_px; }
	int CellWidth() const { return _cell_w; }
	int CellHeight() const { return _cell_h; }
	int Ascent() const { return _ascent; }
	int Descent() const { return _descent; }

private:
	struct FontFace
	{
		FT_Face face{nullptr};
		hb_font_t *hb_font{nullptr};
		std::string path;
		int face_index{-1};
		bool primary{false};
		bool has_color{false};
		bool likely_emoji{false};
	};

	bool InitFontFaces(float scale);
	void ClearFontFaces();
	bool LoadFontFace(const std::string &path_descriptor, bool primary);
	bool SelectBestBitmapStrike(FT_Face face, int target_px);
	FT_Face OpenBestFace(const std::string &path, bool primary, int desired_index);
	int ScoreFaceStyle(FT_Face face, bool primary) const;
	void UpdatePrimaryFontMetrics(FT_Face face);
	int BuildGlyphLoadFlags(bool request_lcd, bool face_has_color) const;
	void UpdateFontPixelSize(float scale);
	bool RasterizeClusterWithFace(const std::u32string &cluster, const FontFace &face, GlyphBitmap &out) const;

	FT_Library _ft_library{nullptr};
	std::vector<FontFace> _font_faces;
	int _font_pt{18};
	int _font_px{18};
	int _cell_w{10};
	int _cell_h{18};
	int _ascent{0};
	int _descent{0};
	bool _subpixel_aa{false};
	FontHintingMode _hinting_mode{FontHintingMode::Default};
};

int ClampFontPointSize(int pt);
int NormalizeFontPointSize(float pt);
bool ExtractFontPointSizeFromLine(const std::string &line, int &out_pt);
bool SaveFontPreference(const std::string &path, int face_index, int point_size);
bool EnsureFontPreferenceSelected();
int LoadFontPointSizeFromConfig();
