#include <functional>
#include <optional>
#include <wx/mstream.h>
#include <wx/image.h>
#include "wxConsoleImages.h"
#include "Backend.h"
#include "CallInMain.h"

void wxConsoleImages::Paint(wxPaintDC &dc, const wxRect &rc, unsigned int font_width, unsigned int font_height)
{
	if (_images.empty()) {
		return;
	}

	for (auto& it : _images) {
		auto sz = it.second.bitmap.GetSize();
		wxRect img_rc;
		img_rc.SetLeft(font_width * it.second.area.Left);
		img_rc.SetTop(font_height * it.second.area.Top);
		if (it.second.pixel_offset || it.second.area.Right == -1) {
			img_rc.SetWidth(sz.GetWidth());
		} else {
			img_rc.SetWidth(font_width * (it.second.area.Right + 1 - it.second.area.Left));
		}
		if (it.second.pixel_offset || it.second.area.Bottom == -1) {
			img_rc.SetHeight(sz.GetHeight());
		} else {
			img_rc.SetHeight(font_height * (it.second.area.Bottom + 1 - it.second.area.Top));
		}
		if (rc.Intersects(img_rc)) {
			if (img_rc.GetWidth() == sz.GetWidth() && img_rc.GetHeight() == sz.GetHeight()) {
				int x = img_rc.GetLeft(), y = img_rc.GetTop();
				if (it.second.pixel_offset && it.second.area.Right > 0) {
					x+= it.second.area.Right;
				}
				if (it.second.pixel_offset && it.second.area.Bottom > 0) {
					y+= it.second.area.Bottom;
				}
//						fprintf(stderr, "WX image: [%d:%d] at [%d:%d]\n", sz.GetWidth(), sz.GetHeight(), x, y);
				dc.DrawBitmap(it.second.bitmap, x, y, false);
			} else {
				auto scaled_sz = it.second.scaled_bitmap.GetSize();
				if (img_rc.GetWidth() != scaled_sz.GetWidth() || img_rc.GetHeight() != scaled_sz.GetHeight()) {
					fprintf(stderr, "WX image scaling: [%d:%d] -> [%d:%d] at [%d:%d]\n",
						sz.GetWidth(), sz.GetHeight(), img_rc.GetWidth(), img_rc.GetHeight(),
						img_rc.GetLeft(), img_rc.GetTop());
					auto scaled_image = it.second.bitmap.ConvertToImage().Scale(img_rc.GetWidth(), img_rc.GetHeight(), wxIMAGE_QUALITY_HIGH);
					it.second.scaled_bitmap = scaled_image;
				}
				dc.DrawBitmap(it.second.scaled_bitmap, img_rc.GetLeft(), img_rc.GetTop(), false);
			}
		}
	}
}

bool wxConsoleImages::Set(const char *id, DWORD64 flags, const SMALL_RECT *area, DWORD width, DWORD height, const void *buffer, unsigned int font_height)
{
	std::string str_id(id);
	try {
		std::optional<wxImage> wx_img;
		unsigned char *pixel_data = (unsigned char *)buffer;
		const auto fmt = (flags & WP_IMG_MASK_FMT);
		if (fmt == WP_IMG_PNG || fmt == WP_IMG_JPG) {
			if (height != 1) {
				fprintf(stderr, "%s('%s'): unexpected height=%u\n", __FUNCTION__, id, height);
				return false;
			}
			static bool s_ih = false;
			if (!s_ih) {
				wxInitAllImageHandlers();
				s_ih = true;
			}
			wxMemoryInputStream stream(pixel_data, width);
		    wx_img.emplace();
		    if (!wx_img->LoadFile(stream, (fmt == WP_IMG_JPG) ? wxBITMAP_TYPE_JPEG : wxBITMAP_TYPE_PNG)) {
				fprintf(stderr, "%s('%s'): PNG load failed\n", __FUNCTION__, id);
				return false;
		    }
			width = wx_img->GetWidth();
			height = wx_img->GetHeight();
			fprintf(stderr, "%s('%s'): PNG loaded %u x %u pixels\n", __FUNCTION__, id, width, height);

		} else if (fmt == WP_IMG_RGBA) {
			const size_t num_pixels = size_t(width) * height;
			unsigned char *rgb = (unsigned char *)malloc(num_pixels * 3);
			unsigned char *alpha = (unsigned char *)malloc(num_pixels);
			for (size_t i = 0; i < num_pixels; ++i) {
				rgb[i * 3 + 0] = pixel_data[i * 4 + 0];
				rgb[i * 3 + 1] = pixel_data[i * 4 + 1];
				rgb[i * 3 + 2] = pixel_data[i * 4 + 2];
				alpha[i]       = pixel_data[i * 4 + 3];
			}
			wx_img.emplace((int)width, (int)height, rgb, alpha, false);
			if (!wx_img->IsOk()) {
				free(rgb);
				free(alpha);
			}

		} else if (fmt == WP_IMG_RGB) {
			wx_img.emplace((int)width, (int)height, pixel_data, true);

		} else {
			fprintf(stderr, "%s('%s'): bad fmt flags\n", __FUNCTION__, id);
			return false;
		}
		if (!wx_img->IsOk()) {
			fprintf(stderr, "%s('%s'): failed to create wxImage\n", __FUNCTION__, id);
			return false;
		}

		auto default_pos = g_winport_con_out->GetCursor();
		default_pos.Y-= (height / font_height) + ((height % font_height) ? 1 : 0);
		if (default_pos.Y < 0) {
			default_pos.Y = 0;
		}

		const auto attach = (flags & WP_IMG_MASK_ATTACH);
		auto &img = _images[str_id];
		img.pixel_offset = (flags & WP_IMG_PIXEL_OFFSET) != 0;
		MakeImageArea(img.area, area, default_pos);
		fprintf(stderr, "%s('%s'): area %d x %d - %d x %d\n", __FUNCTION__, id, img.area.Left, img.area.Top, img.area.Right, img.area.Bottom);

		if (attach) { // attach/move existing image
			if (width && height) { // attaching, but if empty image specified - its just a move operation
				auto sz = img.bitmap.GetSize();
				if (attach == WP_IMG_ATTACH_LEFT || attach == WP_IMG_ATTACH_RIGHT) {
					if (height != (DWORD)sz.GetHeight()) {
						fprintf(stderr, "%s: WP_IMG_ATTACH - height mismatch, %u != %d\n", __FUNCTION__, height, sz.GetHeight());
						return false;
					}
					if ((flags & WP_IMG_SCROLL) == 0) {
						sz.IncBy(width, 0);
					}
				} else if (attach == WP_IMG_ATTACH_TOP || attach == WP_IMG_ATTACH_BOTTOM) {
					if (width != (DWORD)sz.GetWidth()) {
						fprintf(stderr, "%s: WP_IMG_ATTACH - width mismatch, %u != %d\n", __FUNCTION__, width, sz.GetWidth());
						return false;
					}
					if ((flags & WP_IMG_SCROLL) == 0) {
						sz.IncBy(0, height);
					}
				} else {
					fprintf(stderr, "%s: bad attach=%llu\n", __FUNCTION__, (unsigned long long)attach);
					return false;
				}
				wxBitmap new_bmp(sz);
				wxBitmap edge_bmp = *wx_img;
				wxMemoryDC img_dc(img.bitmap), edge_dc(edge_bmp), new_dc(new_bmp);
				switch (attach) {
					case WP_IMG_ATTACH_LEFT:
						new_dc.Blit(width, 0, sz.GetWidth() - width, sz.GetHeight(), &img_dc, 0, 0, wxCOPY, false);
						new_dc.Blit(0, 0, width, height, &edge_dc, 0, 0, wxCOPY, false);
						break;
					case WP_IMG_ATTACH_RIGHT:
						new_dc.Blit(0, 0, sz.GetWidth() - width, sz.GetHeight(), &img_dc, (flags & WP_IMG_SCROLL) ? width : 0, 0, wxCOPY, false);
						new_dc.Blit(sz.GetWidth() - width, 0, width, height, &edge_dc, 0, 0, wxCOPY, false);
						break;
					case WP_IMG_ATTACH_TOP:
						new_dc.Blit(0, height, sz.GetWidth(), sz.GetHeight() - height, &img_dc, 0, 0, wxCOPY, false);
						new_dc.Blit(0, 0, width, height, &edge_dc, 0, 0, wxCOPY, false);
						break;
					case WP_IMG_ATTACH_BOTTOM:
						new_dc.Blit(0, 0, sz.GetWidth(), sz.GetHeight() - height, &img_dc, 0, (flags & WP_IMG_SCROLL) ? height : 0, wxCOPY, false);
						new_dc.Blit(0, sz.GetHeight() - height, width, height, &edge_dc, 0, 0, wxCOPY, false);
						break;
				}
				img.bitmap = new_bmp;
			}
		} else {
			img.bitmap = *wx_img;
		}

	} catch (...) {
		fprintf(stderr, "%s('%s'): exception\n", __FUNCTION__, id);
		_images.erase(str_id);
		return false;
	}
	return true;
}

bool wxConsoleImages::Transform(const char *id, const SMALL_RECT *area, uint16_t tf)
{
	try {
		std::string str_id(id);
		auto it = _images.find(id);
		if (it == _images.end()) {
			fprintf(stderr, "%s('%s'): no such image\n", __FUNCTION__, id);
			return false;
		}
		if (area && area->Left != -1) {
			it->second.area.Left = area->Left;
		}
		if (area && area->Right != -1) {
			it->second.area.Right = area->Right;
		}
		if (area && area->Top != -1) {
			it->second.area.Top = area->Top;
		}
		if (area && area->Bottom != -1) {
			it->second.area.Bottom = area->Bottom;
		}
		if ( tf != WP_IMGTF_ROTATE0) { // otherwise its a trivial move
			wxImage img = it->second.bitmap.ConvertToImage();
			if (tf & WP_IMGTF_MIRROR_H) {
				img = img.Mirror(true);
			}
			if (tf & WP_IMGTF_MIRROR_V) {
				img = img.Mirror(false);
			}
			switch (tf & WP_IMGTF_MASK_ROTATE) {
				case WP_IMGTF_ROTATE90:  img = img.Rotate90(true); break;
				case WP_IMGTF_ROTATE180: img = img.Rotate180(); break;
				case WP_IMGTF_ROTATE270: img = img.Rotate90(false); break;
				default: ;
			}
			it->second.bitmap = img;
		}
	} catch (...) {
		fprintf(stderr, "%s('%s', 0x%x): exception\n", __FUNCTION__, id, tf);
		return false;
	}

	return true;
}

bool wxConsoleImages::Delete(const char *id)
{
	std::string str_id(id);
	return _images.erase(str_id) != 0;
}
