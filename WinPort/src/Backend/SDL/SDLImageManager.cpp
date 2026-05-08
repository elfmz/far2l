#include "SDLImageManager.h"

#include "SDLBackendUtils.h"
#include "Backend.h"

#include <algorithm>
#include <cstring>

#if defined(__APPLE__)
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#endif

namespace
{

static void CopyPixelsRegion(const std::vector<uint8_t> &src, int src_w, int src_h,
	int src_x, int src_y, int copy_w, int copy_h,
	std::vector<uint8_t> &dst, int dst_w, int dst_h, int dst_x, int dst_y)
{
	if (copy_w <= 0 || copy_h <= 0) {
		return;
	}

	const int src_pitch = src_w * 4;
	const int dst_pitch = dst_w * 4;

	for (int row = 0; row < copy_h; ++row) {
		if (src_x + copy_w > src_w || src_y + row >= src_h || dst_x + copy_w > dst_w || dst_y + row >= dst_h) {
			break;
		}
		const uint8_t *src_row = src.data() + (src_y + row) * src_pitch + src_x * 4;
		uint8_t *dst_row = dst.data() + (dst_y + row) * dst_pitch + dst_x * 4;
		memcpy(dst_row, src_row, static_cast<size_t>(copy_w) * 4);
	}
}

#if defined(__APPLE__)
static bool DecodeImageWithImageIO(const uint8_t *data, size_t size, std::vector<uint8_t> &out_pixels, int &out_w, int &out_h)
{
	if (!data || size == 0) {
		return false;
	}

	CFDataRef cfdata = CFDataCreate(kCFAllocatorDefault, data, static_cast<CFIndex>(size));
	if (!cfdata) {
		return false;
	}
	CGImageSourceRef source = CGImageSourceCreateWithData(cfdata, nullptr);
	CFRelease(cfdata);
	if (!source) {
		return false;
	}
	CGImageRef image = CGImageSourceCreateImageAtIndex(source, 0, nullptr);
	CFRelease(source);
	if (!image) {
		return false;
	}

	const size_t w = CGImageGetWidth(image);
	const size_t h = CGImageGetHeight(image);
	if (w == 0 || h == 0) {
		CGImageRelease(image);
		return false;
	}

	out_pixels.assign(w * h * 4, 0);
	CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
	CGContextRef ctx = CGBitmapContextCreate(
		out_pixels.data(),
		w,
		h,
		8,
		w * 4,
		color_space,
		kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
	CGColorSpaceRelease(color_space);
	if (!ctx) {
		CGImageRelease(image);
		return false;
	}
	CGContextDrawImage(ctx, CGRectMake(0, 0, static_cast<CGFloat>(w), static_cast<CGFloat>(h)), image);
	CGContextRelease(ctx);
	CGImageRelease(image);

	out_w = static_cast<int>(w);
	out_h = static_cast<int>(h);
	return true;
}
#endif

} // namespace

bool SDLImageManager::DecodeImageBuffer(
	DWORD64 flags,
	DWORD width,
	DWORD height,
	const uint8_t *buffer,
	size_t buffer_size,
	int &out_w,
	int &out_h,
	std::vector<uint8_t> &out_pixels)
{
	out_w = 0;
	out_h = 0;
	out_pixels.clear();

	const auto fmt = (flags & WP_IMG_MASK_FMT);
	switch (fmt) {
	case WP_IMG_RGBA:
	{
		const size_t expected = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
		if (expected == 0 || buffer_size < expected || !buffer) {
			return false;
		}
		out_w = static_cast<int>(width);
		out_h = static_cast<int>(height);
		out_pixels.assign(buffer, buffer + expected);
		return true;
	}
	case WP_IMG_RGB:
	{
		const size_t expected = static_cast<size_t>(width) * static_cast<size_t>(height) * 3;
		if (expected == 0 || buffer_size < expected || !buffer) {
			return false;
		}
		out_w = static_cast<int>(width);
		out_h = static_cast<int>(height);
		out_pixels.resize(static_cast<size_t>(out_w) * static_cast<size_t>(out_h) * 4);
		for (size_t i = 0, pixels = static_cast<size_t>(out_w) * out_h; i < pixels; ++i) {
			out_pixels[i * 4 + 0] = buffer[i * 3 + 0];
			out_pixels[i * 4 + 1] = buffer[i * 3 + 1];
			out_pixels[i * 4 + 2] = buffer[i * 3 + 2];
			out_pixels[i * 4 + 3] = 255;
		}
		return true;
	}
	case WP_IMG_PNG:
	case WP_IMG_JPG:
	{
#if defined(__APPLE__)
		if (!buffer || buffer_size == 0) {
			return false;
		}
		return DecodeImageWithImageIO(buffer, buffer_size, out_pixels, out_w, out_h);
#else
		(void)buffer;
		(void)buffer_size;
		return false;
#endif
	}
	default:
		return false;
	}
}

bool SDLImageManager::UploadImageTexture(SDL_Renderer *renderer, Image &img)
{
	if (!renderer || img.width <= 0 || img.height <= 0 || img.pixels.empty()) {
		return false;
	}

	if (img.texture) {
		SDL_DestroyTexture(img.texture);
		img.texture = nullptr;
	}

	SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(
		img.pixels.data(),
		img.width,
		img.height,
		32,
		img.width * 4,
		SDL_PIXELFORMAT_RGBA32);
	if (!surface) {
		return false;
	}

	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	if (!texture) {
		return false;
	}
	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	img.texture = texture;
	return true;
}

void SDLImageManager::ClearImages()
{
	for (auto &kv : _images) {
		if (kv.second.texture) {
			SDL_DestroyTexture(kv.second.texture);
			kv.second.texture = nullptr;
		}
	}
	_images.clear();
	_dirty = true;
}

bool SDLImageManager::ApplyAttachment(Image &img, DWORD64 flags, const std::vector<uint8_t> &fragment, int frag_w, int frag_h)
{
	const DWORD64 attach = (flags & WP_IMG_MASK_ATTACH);
	if (attach == 0 || frag_w <= 0 || frag_h <= 0 || fragment.empty()) {
		return false;
	}

	if (img.pixels.empty() || img.width <= 0 || img.height <= 0) {
		img.width = frag_w;
		img.height = frag_h;
		img.pixels = fragment;
		return true;
	}

	const bool scroll = (flags & WP_IMG_SCROLL) != 0;
	int new_w = img.width;
	int new_h = img.height;

	switch (attach) {
	case WP_IMG_ATTACH_LEFT:
	case WP_IMG_ATTACH_RIGHT:
		if (frag_h != img.height) {
			return false;
		}
		if (!scroll) {
			new_w += frag_w;
		} else if (frag_w > img.width) {
			return false;
		}
		break;

	case WP_IMG_ATTACH_TOP:
	case WP_IMG_ATTACH_BOTTOM:
		if (frag_w != img.width) {
			return false;
		}
		if (!scroll) {
			new_h += frag_h;
		} else if (frag_h > img.height) {
			return false;
		}
		break;

	default:
		return false;
	}

	std::vector<uint8_t> new_pixels(static_cast<size_t>(new_w) * static_cast<size_t>(new_h) * 4, 0);
	switch (attach) {
	case WP_IMG_ATTACH_LEFT:
		if (scroll) {
			const int copy_w = img.width - frag_w;
			if (copy_w > 0) {
				CopyPixelsRegion(img.pixels, img.width, img.height, 0, 0, copy_w, img.height, new_pixels, new_w, new_h, frag_w, 0);
			}
		} else {
			CopyPixelsRegion(img.pixels, img.width, img.height, 0, 0, img.width, img.height, new_pixels, new_w, new_h, frag_w, 0);
		}
		CopyPixelsRegion(fragment, frag_w, frag_h, 0, 0, frag_w, frag_h, new_pixels, new_w, new_h, 0, 0);
		break;

	case WP_IMG_ATTACH_RIGHT:
		if (scroll) {
			const int copy_w = img.width - frag_w;
			if (copy_w > 0) {
				CopyPixelsRegion(img.pixels, img.width, img.height, frag_w, 0, copy_w, img.height, new_pixels, new_w, new_h, 0, 0);
			}
		} else {
			CopyPixelsRegion(img.pixels, img.width, img.height, 0, 0, img.width, img.height, new_pixels, new_w, new_h, 0, 0);
		}
		CopyPixelsRegion(fragment, frag_w, frag_h, 0, 0, frag_w, frag_h, new_pixels, new_w, new_h, new_w - frag_w, 0);
		break;

	case WP_IMG_ATTACH_TOP:
		if (scroll) {
			const int copy_h = img.height - frag_h;
			if (copy_h > 0) {
				CopyPixelsRegion(img.pixels, img.width, img.height, 0, 0, img.width, copy_h, new_pixels, new_w, new_h, 0, frag_h);
			}
		} else {
			CopyPixelsRegion(img.pixels, img.width, img.height, 0, 0, img.width, img.height, new_pixels, new_w, new_h, 0, frag_h);
		}
		CopyPixelsRegion(fragment, frag_w, frag_h, 0, 0, frag_w, frag_h, new_pixels, new_w, new_h, 0, 0);
		break;

	case WP_IMG_ATTACH_BOTTOM:
		if (scroll) {
			const int copy_h = img.height - frag_h;
			if (copy_h > 0) {
				CopyPixelsRegion(img.pixels, img.width, img.height, 0, frag_h, img.width, copy_h, new_pixels, new_w, new_h, 0, 0);
			}
		} else {
			CopyPixelsRegion(img.pixels, img.width, img.height, 0, 0, img.width, img.height, new_pixels, new_w, new_h, 0, 0);
		}
		CopyPixelsRegion(fragment, frag_w, frag_h, 0, 0, frag_w, frag_h, new_pixels, new_w, new_h, 0, new_h - frag_h);
		break;
	}

	img.width = new_w;
	img.height = new_h;
	img.pixels = std::move(new_pixels);
	return true;
}

void SDLImageManager::ApplyMirrorHorizontal(Image &img)
{
	if (img.width <= 0 || img.height <= 0) {
		return;
	}
	for (int y = 0; y < img.height; ++y) {
		for (int x = 0; x < img.width / 2; ++x) {
			const int opposite = img.width - 1 - x;
			for (int c = 0; c < 4; ++c) {
				std::swap(
					img.pixels[(y * img.width + x) * 4 + c],
					img.pixels[(y * img.width + opposite) * 4 + c]);
			}
		}
	}
}

void SDLImageManager::ApplyMirrorVertical(Image &img)
{
	if (img.width <= 0 || img.height <= 0) {
		return;
	}
	const int row_bytes = img.width * 4;
	for (int y = 0; y < img.height / 2; ++y) {
		const int opposite = img.height - 1 - y;
		uint8_t *row_a = img.pixels.data() + y * row_bytes;
		uint8_t *row_b = img.pixels.data() + opposite * row_bytes;
		for (int x = 0; x < row_bytes; ++x) {
			std::swap(row_a[x], row_b[x]);
		}
	}
}

void SDLImageManager::RotateImage(Image &img, uint16_t rotation)
{
	if (rotation == WP_IMGTF_ROTATE0 || img.width <= 0 || img.height <= 0) {
		return;
	}

	int new_w = img.width;
	int new_h = img.height;
	if (rotation == WP_IMGTF_ROTATE90 || rotation == WP_IMGTF_ROTATE270) {
		new_w = img.height;
		new_h = img.width;
	}

	std::vector<uint8_t> out(static_cast<size_t>(new_w) * static_cast<size_t>(new_h) * 4, 0);
	for (int y = 0; y < img.height; ++y) {
		for (int x = 0; x < img.width; ++x) {
			int dst_x = 0;
			int dst_y = 0;
			switch (rotation) {
			case WP_IMGTF_ROTATE90:
				dst_x = new_w - 1 - y;
				dst_y = x;
				break;
			case WP_IMGTF_ROTATE180:
				dst_x = new_w - 1 - x;
				dst_y = new_h - 1 - y;
				break;
			case WP_IMGTF_ROTATE270:
				dst_x = y;
				dst_y = new_h - 1 - x;
				break;
			default:
				dst_x = x;
				dst_y = y;
				break;
			}
			memcpy(
				out.data() + (dst_y * new_w + dst_x) * 4,
				img.pixels.data() + (y * img.width + x) * 4,
				4);
		}
	}

	img.width = new_w;
	img.height = new_h;
	img.pixels = std::move(out);
}

COORD SDLImageManager::ComputeDefaultImageAnchor(int img_height, int cell_h) const
{
	COORD pos{0, 0};
	if (!g_winport_con_out) {
		return pos;
	}
	pos = g_winport_con_out->GetCursor();
	if (img_height > 0) {
		const int ch = std::max(1, cell_h);
		const int rows = img_height / ch + ((img_height % ch) ? 1 : 0);
		pos.Y -= rows;
		if (pos.Y < 0) {
			pos.Y = 0;
		}
	}
	return pos;
}

bool SDLImageManager::SetImage(SDL_Renderer *renderer, const std::string &id, DWORD64 flags, const SMALL_RECT *area,
	DWORD width, DWORD height, const uint8_t *buffer, size_t buffer_size, int cell_h)
{
	if (id.empty()) {
		return false;
	}

	auto &img = _images[id];
	img.pixel_offset = (flags & WP_IMG_PIXEL_OFFSET) != 0;

	if (buffer_size > 0) {
		if (!buffer) {
			return false;
		}
		int decoded_w = 0;
		int decoded_h = 0;
		std::vector<uint8_t> decoded_pixels;
		if (!DecodeImageBuffer(flags, width, height, buffer, buffer_size, decoded_w, decoded_h, decoded_pixels)) {
			return false;
		}

		const DWORD64 attach = (flags & WP_IMG_MASK_ATTACH);
		if (attach) {
			if (!ApplyAttachment(img, flags, decoded_pixels, decoded_w, decoded_h)) {
				return false;
			}
		} else {
			img.width = decoded_w;
			img.height = decoded_h;
			img.pixels = std::move(decoded_pixels);
		}
	} else if ((flags & WP_IMG_MASK_ATTACH) != 0) {
		if (img.pixels.empty()) {
			return false;
		}
		// Attach with empty payload – just update placement.
	} else {
		return false;
	}

	const int anchor_height = img.height > 0 ? img.height : std::max(1, cell_h);
	COORD anchor = ComputeDefaultImageAnchor(anchor_height, cell_h);
	if (area) {
		img.area = *area;
	}
	MakeImageArea(img.area, area, anchor);

	if (!UploadImageTexture(renderer, img)) {
		return false;
	}
	_dirty = true;
	return true;
}

bool SDLImageManager::TransformImage(SDL_Renderer *renderer, const std::string &id, const SMALL_RECT *area, uint16_t tf)
{
	auto it = _images.find(id);
	if (it == _images.end()) {
		return false;
	}

	if (area) {
		if (area->Left != -1) it->second.area.Left = area->Left;
		if (area->Top != -1) it->second.area.Top = area->Top;
		if (area->Right != -1) it->second.area.Right = area->Right;
		if (area->Bottom != -1) it->second.area.Bottom = area->Bottom;
	}

	if (tf & WP_IMGTF_MIRROR_H) {
		ApplyMirrorHorizontal(it->second);
	}
	if (tf & WP_IMGTF_MIRROR_V) {
		ApplyMirrorVertical(it->second);
	}

	const uint16_t rotation = (tf & WP_IMGTF_MASK_ROTATE);
	if (rotation != WP_IMGTF_ROTATE0) {
		RotateImage(it->second, rotation);
	}

	if ((tf & (WP_IMGTF_MASK_ROTATE | WP_IMGTF_MIRROR_H | WP_IMGTF_MIRROR_V)) != 0) {
		if (!UploadImageTexture(renderer, it->second)) {
			return false;
		}
	}

	_dirty = true;
	return true;
}

bool SDLImageManager::DeleteImage(const std::string &id)
{
	auto it = _images.find(id);
	if (it == _images.end()) {
		return false;
	}
	if (it->second.texture) {
		SDL_DestroyTexture(it->second.texture);
	}
	_images.erase(it);
	_dirty = true;
	return true;
}

SDL_Rect SDLImageManager::BuildImageRect(const Image &img, const Layout &layout) const
{
	SDL_Rect rect{};
	const int base_x = img.area.Left >= 0 ? layout.origin_x + img.area.Left * layout.cell_px_w : layout.origin_x;
	const int base_y = img.area.Top >= 0 ? layout.origin_y + img.area.Top * layout.cell_px_h : layout.origin_y;
	int width = 0;
	if (img.pixel_offset || img.area.Right == -1) {
		width = img.width;
	} else if (img.area.Right >= img.area.Left) {
		width = (img.area.Right - img.area.Left + 1) * layout.cell_px_w;
	}
	if (width <= 0) {
		width = img.width;
	}

	int height = 0;
	if (img.pixel_offset || img.area.Bottom == -1) {
		height = img.height;
	} else if (img.area.Bottom >= img.area.Top) {
		height = (img.area.Bottom - img.area.Top + 1) * layout.cell_px_h;
	}
	if (height <= 0) {
		height = img.height;
	}

	rect.x = base_x;
	rect.y = base_y;
	if (img.pixel_offset) {
		if (img.area.Right > 0) {
			rect.x += img.area.Right;
		}
		if (img.area.Bottom > 0) {
			rect.y += img.area.Bottom;
		}
	}

	rect.w = width;
	rect.h = height;
	return rect;
}

void SDLImageManager::DrawImages(SDL_Renderer *renderer, const Layout &layout)
{
	if (!renderer) {
		_dirty = false;
		return;
	}
	if (_images.empty()) {
		_dirty = false;
		return;
	}
	for (auto &kv : _images) {
		const Image &img = kv.second;
		if (!img.texture) {
			continue;
		}
		SDL_Rect dst = BuildImageRect(img, layout);
		SDL_RenderCopy(renderer, img.texture, nullptr, &dst);
	}
	_dirty = false;
}
