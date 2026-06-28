// Stub implementations for symbols defined in the far2l executable
// that WinPort references but the unit test binary never links.
// These are no-ops — the unit tests exercise WinPort in isolation.
#include <cstddef>

extern "C" {
    bool g_far2l_use_vs16 = true;
}

void VTShell_InjectRawInput(const char *, size_t) {}
