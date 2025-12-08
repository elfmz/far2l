#pragma once
#include <vector>

class Image
{
	std::vector<unsigned char> _data;
	int _width{}, _height{};
	unsigned char _bytes_per_pixel{3};

	void ScaleEnlarge(Image &dst, double scale, int y_begin, int y_end) const;
	void ScaleReduce(Image &dst, double scale, int y_begin, int y_end) const;

public:
	Image(int width = 0, int height = 0, unsigned char bytes_per_pixel = 3);

	void Swap(Image &another);

	char BytesPerPixel() const { return _bytes_per_pixel; }
	const int Width() const { return _width; }
	const int Height() const { return _height; }

	const void *Data(size_t offset = 0) const { return (char *)_data.data() + offset; }
	size_t Size() const { return _data.size(); }

	void Resize(int width = 0, int height = 0, unsigned char bytes_per_pixel = 3);
	void Assign(const void *data);

	void MirrorH();
	void MirrorV();

	void Blit(Image &dst, int dst_left, int dst_top, int width, int height, int src_left, int src_top) const;
	void Rotate(Image &dst, bool clockwise) const;
	void Scale(Image &dst, double scale) const;


	inline size_t Offset(int x, int y, unsigned char channel = 0) const
	{
		return (size_t(y) * _width + x) * _bytes_per_pixel + channel;
	}
	inline unsigned char *Ptr(int x, int y, unsigned char channel = 0)
	{
		return _data.data() + Offset(x, y, channel);
	}
	inline const unsigned char *Ptr(int x, int y, unsigned char channel = 0) const
	{
		return _data.data() + Offset(x, y, channel);
	}
};
