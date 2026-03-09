#pragma once

#include <SDL.h>
#include <cstdint>

namespace SDLBoxChar
{
	struct Painter
	{
		int fw{0};
		int fh{0};
		int thickness{1};
		uint32_t wc{0};
		SDL_Renderer *renderer{nullptr};
		SDL_Color color{255, 255, 255, 255};
		int origin_x{0};
		int origin_y{0};

		bool MayDrawFadedEdges() const;
		void SetColorFaded();
		void SetColorExtraFaded();

		void FillRectangle(int left, int top, int right, int bottom);
		void FillPixel(int left, int top);
	};

	using DrawT = void (*)(Painter &p, unsigned int start_y, unsigned int cx);

	DrawT Get(uint32_t c);
}
