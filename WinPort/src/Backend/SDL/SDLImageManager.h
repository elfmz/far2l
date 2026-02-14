#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <SDL.h>

#include "WinPort.h"

class SDLImageManager
{
public:
	struct Layout
	{
		int origin_x{0};
		int origin_y{0};
		int cell_px_w{0};
		int cell_px_h{0};
	};

	SDLImageManager() = default;

	bool SetImage(SDL_Renderer *renderer, const std::string &id, DWORD64 flags, const SMALL_RECT *area,
		DWORD width, DWORD height, const uint8_t *buffer, size_t buffer_size, int cell_h);
	bool TransformImage(SDL_Renderer *renderer, const std::string &id, const SMALL_RECT *area, uint16_t tf);
	bool DeleteImage(const std::string &id);

	void DrawImages(SDL_Renderer *renderer, const Layout &layout);
	void ClearImages();

	bool HasDirty() const { return _dirty; }

private:
	struct Image
	{
		SMALL_RECT area{-1, -1, -1, -1};
		bool pixel_offset{false};
		int width{0};
		int height{0};
		std::vector<uint8_t> pixels;
		SDL_Texture *texture{nullptr};
	};

	bool DecodeImageBuffer(DWORD64 flags, DWORD width, DWORD height, const uint8_t *buffer, size_t buffer_size,
		int &out_w, int &out_h, std::vector<uint8_t> &out_pixels);
	bool ApplyAttachment(Image &img, DWORD64 flags, const std::vector<uint8_t> &fragment, int frag_w, int frag_h);
	void ApplyMirrorHorizontal(Image &img);
	void ApplyMirrorVertical(Image &img);
	void RotateImage(Image &img, uint16_t rotation);
	bool UploadImageTexture(SDL_Renderer *renderer, Image &img);
	SDL_Rect BuildImageRect(const Image &img, const Layout &layout) const;
	COORD ComputeDefaultImageAnchor(int img_height, int cell_h) const;

	std::unordered_map<std::string, Image> _images;
	bool _dirty{false};
};
