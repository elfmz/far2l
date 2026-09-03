#pragma once
#include <string>

enum class SDLFontDialogStatus
{
    Chosen,
    Cancelled,
    Unsupported,
    Failed
};

struct SDLFontSelection
{
    std::string path;
    std::string fc_name;
    float point_size{0.0f};
    int face_index{-1};
};

SDLFontDialogStatus SDLShowFontPicker(SDLFontSelection &selection);
void SDLInstallMacFontMenu(void (*callback)(void *), void *context);
