#pragma once
#include <map>
#include <vector>
#include <string>
#include "IVTShell.h"

class VTAnsiKitty
{
	IVTShell *_vts{nullptr};
	struct Image
	{
		std::vector<unsigned char> data;
		int width{-1}, height{-1}, fmt{32};
		int cols{-1}, rows{-1};
		int ofsx{-1}, ofsy{-1};
		bool show{false}, shown{false};
	};
	struct Images : std::map<int, Image>{} _images;
	int _images_hide_cnt{0};

	const char *ShowImage(int id, Image &img);

	const char *AddImage(char action, char medium,
		int id, int fmt, int width, int height, int rows, int cols, int ofsx, int ofsy, int more, const char *b64data, size_t b64len);
	const char *DisplayImage(int id);
	const char *RemoveImage(int id);


public:
	VTAnsiKitty(IVTShell *vts);
	~VTAnsiKitty();

	void InterpretControlString(const char *s, size_t l);

	bool HasImages() const;
	void HideImages();
	void ShowImages();
};
