#include <vector>
#include <thread>
#include <functional>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "Image.h"

Image::Image(int width, int height, unsigned char bytes_per_pixel)
{
	Resize(width, height, bytes_per_pixel);
}

void Image::MirrorH()
{
	for (int y = 0; y < _height; ++y) {
		for (int i = 0; i < _width - 1 - i; ++i) {
			for (unsigned char ch = 0; ch < _bytes_per_pixel; ++ch) {
				std::swap(*Ptr(i, y, ch), *Ptr(_width - 1 - i, y, ch));
			}
		}
	}
}

void Image::MirrorV()
{
	for (int x = 0; x < _width; ++x) {
		for (int i = 0; i < _height - 1 - i; ++i) {
			for (unsigned char ch = 0; ch < _bytes_per_pixel; ++ch) {
				std::swap(*Ptr(x, i, ch), *Ptr(x, _height - 1 - i, ch));
			}
		}
	}
}

void Image::Swap(Image &another)
{
	_data.swap(another._data);
	std::swap(_width, another._width);
	std::swap(_height, another._height);
	std::swap(_bytes_per_pixel, another._bytes_per_pixel);
}

void Image::Assign(const void *data)
{
	memcpy(_data.data(), data, _data.size());
}

void Image::Resize(int width, int height, unsigned char bytes_per_pixel)
{
	assert(bytes_per_pixel == 3 || bytes_per_pixel == 4); // only RGB/RGBA supported for now

	assert(width >= 0);
	assert(height >= 0);

	const size_t bytes_size = size_t(width) * size_t(height) * size_t(bytes_per_pixel);
	assert(bytes_size >= size_t(width) || !height); // overflow guard
	assert(bytes_size >= size_t(height) || !width); // overflow guard

	_data.resize(bytes_size);

	_width = width;
	_height = height;
	_bytes_per_pixel = bytes_per_pixel;
}

void Image::Rotate(Image &dst, bool clockwise) const
{
	dst.Resize(_height, _width, _bytes_per_pixel);
	for (int y = 0; y < _height; ++y) {
		for (int x = 0; x < _width; ++x) {
			auto *dpix = dst.Ptr(clockwise ? _height - 1 - y : y, x);
			const auto *spix = Ptr(clockwise ? x : _width - 1 - x, y);
			for (unsigned char ch = 0; ch < _bytes_per_pixel; ++ch) {
				dpix[ch] = spix[ch];
			}
		}
	}
}

void Image::Blit(Image &dst, int dst_left, int dst_top, int width, int height, int src_left, int src_top) const
{
	assert(_bytes_per_pixel == dst._bytes_per_pixel);
	if (dst_left < 0 || src_left < 0) {
		const int most_negative_left = std::min(src_left, dst_left);
		width+= most_negative_left;
		src_left-= most_negative_left;
		dst_left-= most_negative_left;
	}
	if (dst_top < 0 || src_top < 0) {
		const int most_negative_top = std::min(src_top, dst_top);
		height+= most_negative_top;
		src_top-= most_negative_top;
		dst_top-= most_negative_top;
	}
	if (width <= 0 || height <= 0) {
		return;
	}
	if (dst_left + width > dst._width) {
		width = dst._width - dst_left;
	}
	if (src_left + width > _width) {
		width = _width - src_left;
	}
	if (dst_top + height > dst._height) {
		height = dst._height - dst_top;
	}
	if (src_top + height > _height) {
		height = _height - src_top;
	}
	const int cpy_width = width * _bytes_per_pixel;
	if (width <= 0 || cpy_width < width) { // || overflow guard
		return;
	}

	for (int y = 0; y < height; ++y) {
		memcpy(dst.Ptr(dst_left, dst_top + y), Ptr(src_left, src_top + y), cpy_width);
	}
}

void Image::Scale(Image &dst, double scale) const
{
	if (fabs(scale - 1.0) < 0.0001) {
		dst = *this;
		return;
	}

	const int width = int(scale * Width());
	const int height = int(scale * Height());
	dst.Resize(width, height, _bytes_per_pixel);
	if (_data.empty() || dst._data.empty()) {
		std::fill(dst._data.begin(), dst._data.end(), 0);
		return;
	}

	auto scale_y_range = [&](int y_begin, int y_end) {
		try { // fprintf(stderr, "scale_y_range: y_begin=%d portion=%d\n", y_begin, y_end - y_begin);
			if (scale > 1.0) {
				ScaleEnlarge(dst, scale, y_begin, y_end);
			} else {
				ScaleReduce(dst, scale, y_begin, y_end);
			}
		} catch (...) {
			fprintf(stderr, "scale_y_range: exception at %d .. %d\n", y_begin, y_end);
		}
	};

	struct Threads : std::vector<std::thread>
	{
		~Threads()
		{
			for (auto &t : *this) {
				t.join();
			}
		}
	} threads;

	const size_t min_size_per_cpu = 32768;
	const size_t max_img_size = std::max(Size(), dst.Size());
	int y_begin = 0;
	if (max_img_size >= 2 * min_size_per_cpu && dst._height > 16) {
		const int hw_cpu_count = int(std::thread::hardware_concurrency());
		const int use_cpu_count = std::min(int(max_img_size / min_size_per_cpu), std::min(16, hw_cpu_count));
		if (use_cpu_count > 1) {
			const int base_portion = dst._height / use_cpu_count;
			const int extra_portion = dst._height - (base_portion * use_cpu_count);
			while (y_begin + base_portion < dst._height) {
				int portion = base_portion;
				if (y_begin == 0 && y_begin + portion + extra_portion < dst._height) {
					portion+= extra_portion; // 1st portion has more time than others
				}
				threads.emplace_back(std::bind(scale_y_range, y_begin, y_begin + portion));
				y_begin+= portion;
			}
		} else if (hw_cpu_count <= 0) {
			fprintf(stderr, "%s: CPU count unknown\n", __FUNCTION__);
		}
	}
	if (y_begin < dst._height) { // fprintf(stderr, "last portion at main thread\n");
		scale_y_range(y_begin, dst._height);
	}
}

void Image::ScaleEnlarge(Image &dst, double scale, int y_begin, int y_end) const
{
	const auto src_row_stride = _width * _bytes_per_pixel;
	const auto dst_row_stride = dst._width * _bytes_per_pixel;

	// Calculate the scaling factors
	// We sample from the center of the pixel, so use (dimension - 1) for the ratio if dimension > 1
	const auto scale_x = (dst._width > 1)
		? static_cast<double>(_width - 1) / (dst._width - 1) : 0.0;
	const auto scale_y = (dst._height > 1)
		? static_cast<double>(_height - 1) / (dst._height - 1) : 0.0;

	const auto *src_data = (const unsigned char *)_data.data();
	auto *dst_data = (unsigned char *)dst._data.data();

	for (int dst_y = y_begin; dst_y < y_end; ++dst_y) {
		auto src_y = scale_y * dst_y;
		int y1 = static_cast<int>(std::floor(src_y));
		int y2 = std::min(y1 + 1, _height - 1);
		const double weight_y = (src_y - y1);
		const double one_minus_weight_y = 1.0 - weight_y;

		for (int dst_x = 0; dst_x < dst._width; ++dst_x) {
			// Map destination coordinates to source coordinates
			auto src_x = scale_x * dst_x;

			// Get the integer and fractional parts for interpolation weights
			// Ensure indices are within bounds, especially for the high end
			int x1 = static_cast<int>(std::floor(src_x));
			int x2 = std::min(x1 + 1, _width - 1);

			const double weight_x = (src_x - x1);
			const double one_minus_weight_x = 1.0 - weight_x;

			// Get the four surrounding pixels (A, B, C, D) values
			// A: Top-Left, B: Top-Right, C: Bottom-Left, D: Bottom-Right
			const auto *pA = src_data + (y1 * src_row_stride) + (x1 * _bytes_per_pixel);
			const auto *pB = src_data + (y1 * src_row_stride) + (x2 * _bytes_per_pixel);
			const auto *pC = src_data + (y2 * src_row_stride) + (x1 * _bytes_per_pixel);
			const auto *pD = src_data + (y2 * src_row_stride) + (x2 * _bytes_per_pixel);

			// Pointer to the destination pixel location
			auto *dst_pixel = dst_data + (dst_y * dst_row_stride) + (dst_x * _bytes_per_pixel);

			// Perform interpolation for each color channel (R, G, B)
			for (unsigned char k = 0; k < _bytes_per_pixel; ++k) {
				// Horizontal interpolation (R1, R2)
				double r1 = pA[k] * one_minus_weight_x + pB[k] * weight_x;
				double r2 = pC[k] * one_minus_weight_x + pD[k] * weight_x;

				// Vertical interpolation (final value)
				int p = int(r1 * one_minus_weight_y + r2 * weight_y);

				// Assign the result, clamping to the valid 8-bit range [0, 255]
				if (p >= 255) {
					dst_pixel[k] = 255;
				} else if (p <= 0) {
					dst_pixel[k] = 0;
				} else {
					dst_pixel[k] = (unsigned char)(unsigned int)(p);
				}
			}
		}
	}
}

void Image::ScaleReduce(Image &dst, double scale, int y_begin, int y_end) const
{
	const int around = (int)round(0.618 / scale);

//fprintf(stderr, "around=%d\n", around);
	for (int dst_y = y_begin; dst_y < y_end; ++dst_y) {
		const auto src_y = (int)round(double(dst_y) / scale);
		for (int dst_x = 0; dst_x < dst._width; ++dst_x) {
			const auto src_x = (int)round(double(dst_x) / scale);
			for (unsigned char ch = 0; ch < _bytes_per_pixel; ++ch) {
				unsigned int v = 0, cnt = 0;
				for (int dy = around; dy-->0 ;) {
					for (int dx = around; dx-->0 ;) {
						if (src_y + dy < _height) {
							if (src_x + dx < _width) {
								v+= *Ptr(src_x + dx, src_y + dy, ch);
								++cnt;
							}
							if (src_x - dx >= 0 && dx) {
								v+= *Ptr(src_x - dx, src_y + dy, ch);
								++cnt;
							}
						}
						if (src_y - dy >= 0 && dy) {
							if (src_x + dx < _width) {
								v+= *Ptr(src_x + dx, src_y - dy, ch);
								++cnt;
							}
							if (src_x - dx >= 0 && dx) {
								v+= *Ptr(src_x - dx, src_y - dy, ch);
								++cnt;
							}
						}
					}
					if (cnt) {
						v/= cnt;
						cnt = 1;
					}					
				}
//				if (v > 255) {fprintf(stderr, "!!!v=%d\n", v); abort();}
				*dst.Ptr(dst_x, dst_y, ch) = (unsigned char)v;
			}
		}
	}
}

