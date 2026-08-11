#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>

#include "WinPort.h"
#include "Backend.h"
#include "ConsoleInput.h"
#include "TTYInputSequenceParser.h"
#include "TTYInput.h"
#include "StackSerializer.h"

// Mock handler that captures key events for inspection
class MockHandler : public ITTYInputSpecialSequenceHandler {
public:
	std::vector<KEY_EVENT_RECORD> inspected;

	void OnUsingExtension(char) override {}
	void OnFocusChange(bool) override {}
	void OnFar2lEvent(StackSerializer &) override {}
	void OnFar2lReply(StackSerializer &) override {}
	void OnKittyGraphicsResponse(const std::string &) override {}
	void OnStatusResponse(char) override {}
	void OnCursorShape(int) override {}
	void OnInputBroken() override {}
	void OnGetCellSize(unsigned int, unsigned int) override {}

	void OnInspectKeyEvent(KEY_EVENT_RECORD &event) override {
		inspected.push_back(event);
	}

	void Clear() {
		inspected.clear();
	}
};

// Test fixture
class TTYInputParserTest : public ::testing::Test {
protected:
	MockHandler handler;
	ConsoleInput con_in;

	void SetUp() override {
		g_winport_con_in = &con_in;
	}

	void TearDown() override {
		g_winport_con_in = nullptr;
	}

	// Feed raw bytes to the parser and collect enqueued INPUT_RECORDs
	std::vector<INPUT_RECORD> FeedBytes(const std::vector<char> &bytes) {
		TTYInputSequenceParser parser(&handler);
		size_t offset = 0;
		while (offset < bytes.size()) {
			size_t r = parser.Parse(bytes.data() + offset, bytes.size() - offset, false);
			if (r == TTY_PARSED_WANTMORE) {
				break;
			}
			if (r == TTY_PARSED_BADSEQUENCE) {
				offset += 1;
			} else if (r == TTY_PARSED_PLAINCHARS) {
				offset += 1;
			} else {
				offset += r;
			}
		}

		// Drain the console input queue
		std::vector<INPUT_RECORD> result;
		INPUT_RECORD ir;
		while (con_in.Dequeue(&ir, 1, 0)) {
			result.push_back(ir);
		}
		return result;
	}
};

TEST_F(TTYInputParserTest, CtrlD_MapsTo_D_WithLeftCtrl) {
	auto records = FeedBytes({'\x04'});

	ASSERT_GE(records.size(), 2u);
	EXPECT_EQ(records[0].EventType, KEY_EVENT);
	EXPECT_TRUE(records[0].Event.KeyEvent.bKeyDown);
	EXPECT_EQ(records[0].Event.KeyEvent.wVirtualKeyCode, WORD('D'));
	EXPECT_NE(records[0].Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED, 0u);
	EXPECT_EQ(records[0].Event.KeyEvent.wRepeatCount, 1u);
}

TEST_F(TTYInputParserTest, CtrlK_MapsTo_K_WithLeftCtrl) {
	auto records = FeedBytes({'\x0b'});

	ASSERT_GE(records.size(), 2u);
	EXPECT_EQ(records[0].EventType, KEY_EVENT);
	EXPECT_TRUE(records[0].Event.KeyEvent.bKeyDown);
	EXPECT_EQ(records[0].Event.KeyEvent.wVirtualKeyCode, WORD('K'));
	EXPECT_NE(records[0].Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED, 0u);
}

TEST_F(TTYInputParserTest, CtrlU_MapsTo_U_WithLeftCtrl) {
	auto records = FeedBytes({'\x15'});

	ASSERT_GE(records.size(), 2u);
	EXPECT_EQ(records[0].EventType, KEY_EVENT);
	EXPECT_TRUE(records[0].Event.KeyEvent.bKeyDown);
	EXPECT_EQ(records[0].Event.KeyEvent.wVirtualKeyCode, WORD('U'));
	EXPECT_NE(records[0].Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED, 0u);
}

TEST_F(TTYInputParserTest, AllCtrlLetters_A_through_Z) {
	for (int i = 0; i < 26; ++i) {
		char ctrl_byte = (char)(0x01 + i);
		char expected_vk = 'A' + i;
		// Ctrl+I (0x09) = Tab, Ctrl+M (0x0D) = Enter
		// These are special-cased in ParseIntoPending
		bool is_special = (ctrl_byte == 0x09 || ctrl_byte == 0x0d);
		WORD expected_vk_code = is_special
			? (ctrl_byte == 0x09 ? WORD(VK_TAB) : WORD(VK_RETURN))
			: WORD(expected_vk);
		bool should_have_ctrl = !is_special;

		handler.Clear();
		auto records = FeedBytes({ctrl_byte});

		ASSERT_GE(records.size(), 2u)
			<< "Ctrl+" << expected_vk << " (0x" << std::hex << (int)(unsigned char)ctrl_byte << ")";
		EXPECT_EQ(records[0].Event.KeyEvent.wVirtualKeyCode, expected_vk_code)
			<< "Ctrl+" << expected_vk;
		if (should_have_ctrl) {
			EXPECT_NE(records[0].Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED, 0u)
				<< "Ctrl+" << expected_vk;
		}
	}
}

TEST_F(TTYInputParserTest, CtrlC_WorksCorrectly) {
	auto records = FeedBytes({'\x03'});

	ASSERT_GE(records.size(), 2u);
	EXPECT_EQ(records[0].Event.KeyEvent.wVirtualKeyCode, WORD('C'));
	EXPECT_NE(records[0].Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED, 0u);
}

TEST_F(TTYInputParserTest, CtrlAt_MapsTo_Space_WithLeftCtrl) {
	auto records = FeedBytes({'\x00'});

	ASSERT_GE(records.size(), 2u);
	EXPECT_EQ(records[0].Event.KeyEvent.wVirtualKeyCode, WORD(VK_SPACE));
	EXPECT_NE(records[0].Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED, 0u);
}

TEST_F(TTYInputParserTest, Tab_Byte_Produces_VK_TAB) {
	auto records = FeedBytes({'\x09'});

	ASSERT_GE(records.size(), 2u);
	EXPECT_EQ(records[0].Event.KeyEvent.wVirtualKeyCode, WORD(VK_TAB));
	EXPECT_EQ(records[0].Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED, 0u);
}

TEST_F(TTYInputParserTest, Enter_Byte_Produces_VK_RETURN) {
	auto records = FeedBytes({'\x0d'});

	ASSERT_GE(records.size(), 2u);
	EXPECT_EQ(records[0].Event.KeyEvent.wVirtualKeyCode, WORD(VK_RETURN));
	EXPECT_EQ(records[0].Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED, 0u);
}
