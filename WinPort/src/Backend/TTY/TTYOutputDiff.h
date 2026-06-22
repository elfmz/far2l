#pragma once
#include <vector>
#include <algorithm>
#include <cctweaks.h>
#include <WinCompat.h>

namespace TTYOutputDiff {

template <typename Output>
void DiffLines(
	const CHAR_INFO *cur_output, const CHAR_INFO *prev_output,
	unsigned int width, unsigned int height,
	const std::vector<SMALL_RECT> &damage_areas,
	Output &output)
{
	if (damage_areas.empty()) {
		// Full screen diff - original behavior when no damage areas provided
		for (unsigned int y = 0; y < height; ++y) {
			const CHAR_INFO *cur_line = &cur_output[size_t(y) * width];
			const CHAR_INFO *prev_line = &prev_output[size_t(y) * width];

			const auto ApproxWeight = [&](unsigned int x_)
			{
				if (CI_USING_COMPOSITE_CHAR(cur_line[x_])) {
					return 4;
				}
				return ((cur_line[x_].Char.UnicodeChar > 0x7f) ? 2 : 1);
			};

			const auto Modified = [&](unsigned int x_)
			{
				return (cur_line[x_].Char.UnicodeChar != prev_line[x_].Char.UnicodeChar
					|| cur_line[x_].Attributes != prev_line[x_].Attributes);
			};

			for (unsigned int x = 0, skipped_start = 0, skipped_weight = 0; x < width; ++x) {
				if (!Modified(x)) {
					skipped_weight += ApproxWeight(x);
					continue;
				}

				bool print_skipped = false;
				if (x != skipped_start && output.WeightOfHorizontalMoveCursor(y + 1, skipped_start + 1) == 0) {
					const int move_cursor_weight = output.WeightOfHorizontalMoveCursor(y + 1, x + 1);
					print_skipped = (move_cursor_weight >= 0 && skipped_weight <= (unsigned int)move_cursor_weight);
				}
				if (print_skipped) {
					output.WriteLine(&cur_line[skipped_start], x + 1 - skipped_start);
				} else {
					output.MoveCursorLazy(y + 1, x + 1);
					output.WriteLine(&cur_line[x], 1);
				}
				skipped_start = x + 1;
				skipped_weight = 0;
			}
		}
	} else {
		// Partial diff using damage areas - only process cells within specified rectangles
		for (const auto &area : damage_areas) {
			unsigned int top = (unsigned int)std::max(0, (int)area.Top);
			unsigned int bottom = std::min((unsigned int)area.Bottom, height - 1);
			unsigned int left = (unsigned int)std::max(0, (int)area.Left);
			unsigned int right = std::min((unsigned int)area.Right, width - 1);

			for (unsigned int y = top; y <= bottom; ++y) {
				const CHAR_INFO *cur_line = &cur_output[size_t(y) * width];
				const CHAR_INFO *prev_line = &prev_output[size_t(y) * width];

				const auto ApproxWeight = [&](unsigned int x_)
				{
					if (CI_USING_COMPOSITE_CHAR(cur_line[x_])) {
						return 4;
					}
					return ((cur_line[x_].Char.UnicodeChar > 0x7f) ? 2 : 1);
				};

				const auto Modified = [&](unsigned int x_)
				{
					return (cur_line[x_].Char.UnicodeChar != prev_line[x_].Char.UnicodeChar
						|| cur_line[x_].Attributes != prev_line[x_].Attributes);
				};

				for (unsigned int x = left, skipped_start = left, skipped_weight = 0; x <= right; ++x) {
					if (!Modified(x)) {
						skipped_weight += ApproxWeight(x);
						continue;
					}

					bool print_skipped = false;
					if (x != skipped_start && output.WeightOfHorizontalMoveCursor(y + 1, skipped_start + 1) == 0) {
						const int move_cursor_weight = output.WeightOfHorizontalMoveCursor(y + 1, x + 1);
						print_skipped = (move_cursor_weight >= 0 && skipped_weight <= (unsigned int)move_cursor_weight);
					}
					if (print_skipped) {
						output.WriteLine(&cur_line[skipped_start], x + 1 - skipped_start);
					} else {
						output.MoveCursorLazy(y + 1, x + 1);
						output.WriteLine(&cur_line[x], 1);
					}
					skipped_start = x + 1;
					skipped_weight = 0;
				}
			}
		}
	}
}

} // namespace TTYOutputDiff
