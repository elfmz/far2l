#include "easteregg.h"

#include <algorithm>
#include <random>
#include <vector>

FireEffect::FireEffect(int width, int height)
    : m_width(width), m_height(height), m_heat(width * height * 2, 0)
{
    InitPalette();
}

void FireEffect::InitPalette()
{
    m_palette.resize(256);

    for (int i = 0; i < 256; ++i) {
        // Simple gradient: black → red → yellow → white
        int r = std::min(255, i * 2);
        int g = std::min(255, i);
        int b = std::min(80, i / 3);

        m_palette[i] = wxColour(r, g, b);
    }
}

void FireEffect::Update()
{
    // Bottom row: random sparks
    for (int x = 0; x < m_width; ++x) {
        m_heat[(m_height - 1) * m_width + x] = (rand() % 256);
    }

    // Propagate fire upward
    for (int y = 0; y < m_height - 1; ++y) {
        for (int x = 0; x < m_width; ++x) {
            int src = (y + 1) * m_width + x;

            // Average of neighbors below
            int heat = m_heat[src];

            if (x > 0) heat += m_heat[src - 1];
            if (x < m_width - 1) heat += m_heat[src + 1];
            heat += m_heat[src + m_width];

            heat /= 4;

            // Decay
            heat = std::max(0, heat - (rand() % 3));

            m_heat[y * m_width + x] = heat;
        }
    }
}

wxImage FireEffect::Render()
{
    wxImage img(m_width, m_height);
    img.InitAlpha();

    unsigned char* alpha = img.GetAlpha();
	// for (int i = 0; i < m_width * m_height; ++i) alpha[i] = 70; // 0–255

    unsigned char* data = img.GetData();

    for (int i = 0; i < m_width * m_height; ++i) {
        wxColour c = m_palette[m_heat[i]];

        alpha[i] = m_heat[i] > 0 ? 70 : 0;

        data[i * 3 + 0] = c.Red();
        data[i * 3 + 1] = c.Green();
        data[i * 3 + 2] = c.Blue();
    }

    return img;
}

MarsEffect::MarsEffect(int width, int height)
    : m_width(width),
      m_height(height),
      m_heightmap(width * height * 2, 0)
{
    InitPalette();
    GenerateTerrain();
}

void MarsEffect::InitPalette()
{
    m_palette.resize(256);

    for (int i = 0; i < 256; ++i)
    {
        // Mars-like palette: dark red → orange → yellow → white
        int r = std::min(255, i * 2);
        int g = std::min(255, i);
        int b = std::min(80, i / 4);

        m_palette[i] = wxColour(r, g, b);
    }
}

void MarsEffect::GenerateTerrain()
{
    // Initial terrain: sine waves + noise
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            float fx = float(x) / m_width;
            float fy = float(y) / m_height;

            float v =
                128.0f +
                80.0f * std::sin(fx * 6.28f * 3.0f) +
                40.0f * std::sin(fy * 6.28f * 2.0f);

            m_heightmap[y * m_width + x] = std::clamp(int(v), 0, 255);
        }
    }
}

void MarsEffect::Update()
{
    // Scroll terrain downward (classic plasma motion)
    for (int y = m_height - 1; y > 0; --y) {
        for (int x = 0; x < m_width; ++x) {
            m_heightmap[y * m_width + x] = m_heightmap[(y - 1) * m_width + x];
        }
    }

    // Generate new top row using sine animation
    static float t = 0.0f;
    t += 0.05f;

    for (int x = 0; x < m_width; ++x) {
        float fx = float(x) / m_width;

        float v =
            128.0f +
            80.0f * std::sin(fx * 6.28f * 3.0f + t) +
            40.0f * std::sin(t * 2.0f);

        m_heightmap[x] = std::clamp(int(v), 0, 255);
    }
}

wxImage MarsEffect::Render()
{
    wxImage img(m_width, m_height);
    img.InitAlpha();

    unsigned char* data = img.GetData();
    unsigned char* alpha = img.GetAlpha();

    for (int i = 0; i < m_width * m_height; ++i) {
        wxColour c = m_palette[m_heightmap[i]];

        alpha[i] = m_heightmap[i] > 0 ? 70 : 0;

        data[i * 3 + 0] = c.Red();
        data[i * 3 + 1] = c.Green();
        data[i * 3 + 2] = c.Blue();
    }

    return img;
}

PotatoPlasmaEffect::PotatoPlasmaEffect(int width, int height)
    : m_width(width),
      m_height(height),
      m_field(width * height * 2, 0)
{
    InitPalette();
    GenerateInitialField();
}

void PotatoPlasmaEffect::InitPaletteOnFire()
{
    m_palette.resize(256);

    for (int i = 0; i < 256; ++i)
    {
        // Potato palette: brown → golden → yellow → light cream
        int r = 80 + (i / 2);        // warm brown to golden
        int g = 60 + (i / 3);        // earthy greenish tint
        int b = 30 + (i / 6);        // subtle brown/cream

        r = std::clamp(r, 0, 255);
        g = std::clamp(g, 0, 255);
        b = std::clamp(b, 0, 255);

        m_palette[i] = wxColour(r, g, b);
    }
}

void PotatoPlasmaEffect::InitPalette()
{
    m_palette.resize(256);

    for (int i = 0; i < 256; ++i)
    {
        // Inverted potato palette:
        // start dark → end bright golden/yellow
        int r = 20 + (i * 3);        // dark brown → bright gold
        int g = 15 + (i * 2);        // earthy greenish → warm yellow
        int b = 10 + (i / 2);        // subtle brown → light cream

        r = std::clamp(r, 0, 255);
        g = std::clamp(g, 0, 255);
        b = std::clamp(b, 0, 255);

        m_palette[i] = wxColour(r, g, b);
    }
}

void PotatoPlasmaEffect::GenerateInitialField()
{
    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            float fx = float(x) / m_width;
            float fy = float(y) / m_height;

            float v =
                128.0f +
                60.0f * std::sin(fx * 6.28f * 2.0f) +
                40.0f * std::sin(fy * 6.28f * 3.0f);

            m_field[y * m_width + x] = std::clamp(int(v), 0, 255);
        }
    }
}

void PotatoPlasmaEffect::Update()
{
    static float t = 0.0f;
    t += 0.03f; // slower, smoother potato motion

    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            float fx = float(x) / m_width;
            float fy = float(y) / m_height;

            float v =
                128.0f +
                50.0f * std::sin(fx * 6.28f * 2.0f + t) +
                30.0f * std::sin(fy * 6.28f * 3.0f - t * 0.5f) +
                20.0f * std::sin((fx + fy) * 6.28f + t * 1.5f);

            m_field[y * m_width + x] = std::clamp(int(v), 0, 255);
        }
    }
}

wxImage PotatoPlasmaEffect::Render()
{
    wxImage img(m_width, m_height);
    img.InitAlpha();
    unsigned char* data = img.GetData();
    unsigned char* alpha = img.GetAlpha();

    for (int i = 0; i < m_width * m_height; ++i) {
        wxColour c = m_palette[m_field[i]];

        alpha[i] = 50; // + m_field[i] > 0 ? 20 : 0;

        data[i * 3 + 0] = c.Red();
        data[i * 3 + 1] = c.Green();
        data[i * 3 + 2] = c.Blue();
    }
    return img;
}

#include <chrono>

bool IsEasterEggActive()
{
    using namespace std::chrono;

    // get local time
    auto now = system_clock::now();
    time_t tt = system_clock::to_time_t(now);
    tm local{};
    localtime_r(&tt, &local);   // or localtime_s on Windows

    int month = local.tm_mon + 1;   // tm_mon: 0 = Jan
    int day   = local.tm_mday;

    return (month == 8 && day >= 9 && day <= 15);
}


FireEffect fire(400, 200);
// MarsEffect terrain(320, 200);
// PotatoPlasmaEffect potato(320, 200);