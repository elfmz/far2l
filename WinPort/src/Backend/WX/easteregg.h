#pragma once

#include <cstdint>

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <wx/wx.h>
#include <vector>

class FireEffect
{
public:
    FireEffect(int width, int height);

    // Update fire simulation
    void Update();

    // Render fire into wxImage (RGBA)
    wxImage Render();

private:
    int m_width;
    int m_height;

    // Heat buffer (one byte per pixel)
    std::vector<uint8_t> m_heat;

    // Palette (256 colors)
    std::vector<wxColour> m_palette;

    void InitPalette();
};

class MarsEffect
{
public:
    MarsEffect(int width, int height);

    // Update terrain animation
    void Update();

    // Render terrain into wxImage (RGB)
    wxImage Render();

private:
    int m_width;
    int m_height;

    // Heightmap buffer (0–255)
    std::vector<uint8_t> m_heightmap;

    // Color palette (256 colors)
    std::vector<wxColour> m_palette;

    void InitPalette();
    void GenerateTerrain();
};

class PotatoPlasmaEffect
{
public:
    PotatoPlasmaEffect(int width, int height);

    // Update animation
    void Update();

    // Render into wxImage (RGB)
    wxImage Render();

private:
    int m_width;
    int m_height;

    // Plasma buffer (0–255)
    std::vector<uint8_t> m_field;

    // Potato-themed palette
    std::vector<wxColour> m_palette;

    void InitPalette();
    void InitPaletteOnFire();
    void GenerateInitialField();
};


extern FireEffect fire;
// extern MarsEffect terrain;
// extern PotatoPlasmaEffect potato;

bool IsEasterEggActive();
bool IsEasterEggAnimationActive();

