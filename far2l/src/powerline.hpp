#pragma once

#include <farplug-wide.h>

std::string GetPowerlinePrompt(const std::string& target_cwd);

struct TextState {
    FarTrueColor fg = {255, 255, 255};
    FarTrueColor bg = {0, 0, 0};
    bool bold = false;
};

struct TextSegment {
	TextState colors;
	wchar_t c;
};

std::vector<TextSegment> ParseColorizedText(const std::wstring& input);
