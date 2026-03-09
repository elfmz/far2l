#include "SDLFontDialog.h"

SDLFontDialogStatus SDLShowFontPicker(SDLFontSelection &)
{
    return SDLFontDialogStatus::Unsupported;
}

void SDLInstallMacFontMenu(void (*)(void *), void *)
{
}
