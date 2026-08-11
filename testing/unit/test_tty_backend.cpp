#include <gtest/gtest.h>
#include <vector>

#include "WinCompat.h"
#include "TTYOutputDiff.h"

struct MockOutput {
	const CHAR_INFO *cur_buffer = nullptr;
	unsigned int width = 0;

	struct MoveCursorCall {
		unsigned int y;
		unsigned int x;
	};
	struct WriteLineCall {
		unsigned int y;
		unsigned int x;
		unsigned int count;
	};

	std::vector<MoveCursorCall> move_calls;
	std::vector<WriteLineCall> write_calls;

	int WeightOfHorizontalMoveCursor(unsigned int y, unsigned int x) const {
		return 100; // always prefer cursor moves
	}

	void MoveCursorLazy(unsigned int y, unsigned int x) {
		move_calls.push_back({y, x});
	}

	void WriteLine(const CHAR_INFO *ci, unsigned int cnt) {
		size_t offset = ci - cur_buffer;
		unsigned int y = (offset / width) + 1;
		unsigned int x = (offset % width) + 1;
		write_calls.push_back({y, x, cnt});
	}
};

static void FillBuffer(std::vector<CHAR_INFO> &buf, WCHAR ch, WORD attr = 0) {
	for (auto &ci : buf) {
		ci.Char.UnicodeChar = ch;
		ci.Attributes = attr;
	}
}

TEST(TTYOutputDiffTest, FullScreenOnlyModifiedCells) {
	const unsigned int width = 3, height = 3;
	std::vector<CHAR_INFO> prev(width * height);
	std::vector<CHAR_INFO> cur(width * height);

	FillBuffer(prev, L'A');
	FillBuffer(cur, L'A');
	cur[1 * width + 1].Char.UnicodeChar = L'B';

	MockOutput output;
	output.cur_buffer = cur.data();
	output.width = width;

	std::vector<SMALL_RECT> damage_areas;
	TTYOutputDiff::DiffLines(cur.data(), prev.data(), width, height, damage_areas, output);

	ASSERT_EQ(output.write_calls.size(), 1);
	EXPECT_EQ(output.write_calls[0].y, 2);
	EXPECT_EQ(output.write_calls[0].x, 2);
	EXPECT_EQ(output.write_calls[0].count, 1);
}

TEST(TTYOutputDiffTest, DamageAreasOnlyAffectedCells) {
	const unsigned int width = 10, height = 10;
	std::vector<CHAR_INFO> prev(width * height);
	std::vector<CHAR_INFO> cur(width * height);

	FillBuffer(prev, L'A');
	FillBuffer(cur, L'A');

	cur[0 * width + 0].Char.UnicodeChar = L'B';
	cur[5 * width + 5].Char.UnicodeChar = L'C';
	cur[9 * width + 9].Char.UnicodeChar = L'D';

	MockOutput output;
	output.cur_buffer = cur.data();
	output.width = width;

	std::vector<SMALL_RECT> damage_areas = {
		{0, 0, 2, 2} // only top-left 3x3 area
	};
	TTYOutputDiff::DiffLines(cur.data(), prev.data(), width, height, damage_areas, output);

	// Only cell (0,0) should trigger output; (5,5) and (9,9) are outside damage area
	ASSERT_EQ(output.write_calls.size(), 1);
	EXPECT_EQ(output.write_calls[0].y, 1);
	EXPECT_EQ(output.write_calls[0].x, 1);
	EXPECT_EQ(output.write_calls[0].count, 1);
}

TEST(TTYOutputDiffTest, DamageAreasNoChangesInArea) {
	const unsigned int width = 5, height = 5;
	std::vector<CHAR_INFO> prev(width * height);
	std::vector<CHAR_INFO> cur(width * height);

	FillBuffer(prev, L'A');
	FillBuffer(cur, L'A');

	MockOutput output;
	output.cur_buffer = cur.data();
	output.width = width;

	std::vector<SMALL_RECT> damage_areas = {
		{0, 0, (SHORT)(width - 1), (SHORT)(height - 1)}
	};
	TTYOutputDiff::DiffLines(cur.data(), prev.data(), width, height, damage_areas, output);

	EXPECT_EQ(output.write_calls.size(), 0);
	EXPECT_EQ(output.move_calls.size(), 0);
}
