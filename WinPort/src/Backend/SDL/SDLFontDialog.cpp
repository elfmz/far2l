#include "SDLFontDialog.h"

#include <SDL.h>
#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

constexpr int kMinPointSize = 6;
constexpr int kMaxPointSize = 96;

struct FontEntry {
    std::string path;
    int face_index{-1};
    std::string label;
    std::string fc_name;
};

struct Color {
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;
};

static void FillRect(SDL_Surface *surface, int x, int y, int w, int h, const Color &color)
{
    if (!surface || w <= 0 || h <= 0) {
        return;
    }
    const int x0 = std::max(0, x);
    const int y0 = std::max(0, y);
    const int x1 = std::min(surface->w, x + w);
    const int y1 = std::min(surface->h, y + h);
    if (x0 >= x1 || y0 >= y1) {
        return;
    }
    Uint8 *base = static_cast<Uint8 *>(surface->pixels);
    for (int yy = y0; yy < y1; ++yy) {
        Uint8 *row = base + yy * surface->pitch + x0 * 4;
        for (int xx = x0; xx < x1; ++xx) {
            row[0] = color.r;
            row[1] = color.g;
            row[2] = color.b;
            row[3] = color.a;
            row += 4;
        }
    }
}

static bool NextCodepoint(const std::string &text, size_t &pos, uint32_t &out)
{
    if (pos >= text.size()) {
        return false;
    }
    const unsigned char c0 = static_cast<unsigned char>(text[pos++]);
    if (c0 < 0x80) {
        out = c0;
        return true;
    }
    if ((c0 & 0xE0) == 0xC0 && pos < text.size()) {
        const unsigned char c1 = static_cast<unsigned char>(text[pos++]);
        out = ((c0 & 0x1F) << 6) | (c1 & 0x3F);
        return true;
    }
    if ((c0 & 0xF0) == 0xE0 && pos + 1 < text.size()) {
        const unsigned char c1 = static_cast<unsigned char>(text[pos++]);
        const unsigned char c2 = static_cast<unsigned char>(text[pos++]);
        out = ((c0 & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
        return true;
    }
    if ((c0 & 0xF8) == 0xF0 && pos + 2 < text.size()) {
        const unsigned char c1 = static_cast<unsigned char>(text[pos++]);
        const unsigned char c2 = static_cast<unsigned char>(text[pos++]);
        const unsigned char c3 = static_cast<unsigned char>(text[pos++]);
        out = ((c0 & 0x07) << 18) | ((c1 & 0x3F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
        return true;
    }
    out = '?';
    return true;
}

static int MeasureText(FT_Face face, const std::string &text)
{
    if (!face) {
        return 0;
    }
    int width = 0;
    size_t pos = 0;
    while (pos < text.size()) {
        uint32_t cp = 0;
        if (!NextCodepoint(text, pos, cp)) {
            break;
        }
        if (FT_Load_Char(face, cp, FT_LOAD_RENDER) != 0) {
            continue;
        }
        width += (face->glyph->advance.x >> 6);
    }
    return width;
}

static int RenderText(SDL_Surface *surface, FT_Face face, int x, int y, const std::string &text, const Color &color)
{
    if (!surface || !face) {
        return 0;
    }
    const int ascender = (face->size->metrics.ascender >> 6);
    int pen_x = x;
    int pen_y = y + ascender;
    size_t pos = 0;
    while (pos < text.size()) {
        uint32_t cp = 0;
        if (!NextCodepoint(text, pos, cp)) {
            break;
        }
        if (FT_Load_Char(face, cp, FT_LOAD_RENDER) != 0) {
            continue;
        }
        const FT_GlyphSlot glyph = face->glyph;
        const FT_Bitmap &bmp = glyph->bitmap;
        int gx = pen_x + glyph->bitmap_left;
        const int gy = pen_y - glyph->bitmap_top;
        for (int row = 0; row < static_cast<int>(bmp.rows); ++row) {
            const int dst_y = gy + row;
            if (dst_y < 0 || dst_y >= surface->h) {
                continue;
            }
            const unsigned char *src = bmp.buffer + row * bmp.pitch;
            int start_col = 0;
            if (gx < 0) {
                start_col = -gx;
                gx = 0;
            }
            Uint8 *dst = static_cast<Uint8 *>(surface->pixels) + dst_y * surface->pitch + gx * 4;
            for (int col = start_col; col < static_cast<int>(bmp.width); ++col) {
                const int dst_x = gx + (col - start_col);
                if (dst_x < 0) {
                    dst += 4;
                    continue;
                }
                if (dst_x >= surface->w) {
                    break;
                }
                const Uint8 alpha = src[col];
                if (alpha) {
                    const Uint8 inv = 255 - alpha;
                    dst[0] = static_cast<Uint8>((color.r * alpha + dst[0] * inv) / 255);
                    dst[1] = static_cast<Uint8>((color.g * alpha + dst[1] * inv) / 255);
                    dst[2] = static_cast<Uint8>((color.b * alpha + dst[2] * inv) / 255);
                    dst[3] = static_cast<Uint8>(std::min<int>(255, dst[3] + alpha));
                }
                dst += 4;
            }
        }
        pen_x += (glyph->advance.x >> 6);
    }
    return pen_x - x;
}

static std::vector<FontEntry> CollectFonts()
{
    std::vector<FontEntry> fonts;
    if (!FcInit()) {
        return fonts;
    }

    FcPattern *pattern = FcPatternCreate();
    if (!pattern) {
        return fonts;
    }
    FcPatternAddInteger(pattern, FC_SPACING, FC_MONO);
    FcPatternAddBool(pattern, FC_OUTLINE, FcTrue);

    FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    FcResult result = FcResultMatch;
    FcFontSet *set = FcFontSort(nullptr, pattern, FcTrue, nullptr, &result);
    if (!set) {
        FcPatternDestroy(pattern);
        return fonts;
    }

    std::unordered_set<std::string> seen;
    for (int i = 0; i < set->nfont; ++i) {
        FcPattern *font = set->fonts[i];
        FcChar8 *file = nullptr;
        int index = 0;
        if (FcPatternGetString(font, FC_FILE, 0, &file) != FcResultMatch) {
            continue;
        }
        FcPatternGetInteger(font, FC_INDEX, 0, &index);
        FcChar8 *family = nullptr;
        FcChar8 *style = nullptr;
        FcPatternGetString(font, FC_FAMILY, 0, &family);
        FcPatternGetString(font, FC_STYLE, 0, &style);

        std::string key = std::string(reinterpret_cast<const char *>(file)) + "#" + std::to_string(index);
        if (!seen.insert(key).second) {
            continue;
        }

        std::string label;
        if (family) {
            label += reinterpret_cast<const char *>(family);
        }
        if (style && std::string(reinterpret_cast<const char *>(style)) != "Regular") {
            if (!label.empty()) {
                label += " ";
            }
            label += reinterpret_cast<const char *>(style);
        }
        if (label.empty()) {
            label = reinterpret_cast<const char *>(file);
        }

        FontEntry entry;
        entry.path = reinterpret_cast<const char *>(file);
        entry.face_index = index;
        entry.label = std::move(label);
        if (family) {
            std::string desc = std::string("fc:") + reinterpret_cast<const char *>(family);
            if (style) {
                desc += ":style=";
                desc += reinterpret_cast<const char *>(style);
            }
            int slant = 0;
            if (FcPatternGetInteger(font, FC_SLANT, 0, &slant) == FcResultMatch) {
                desc += ":slant=" + std::to_string(slant);
            }
            int weight = 0;
            if (FcPatternGetInteger(font, FC_WEIGHT, 0, &weight) == FcResultMatch) {
                desc += ":weight=" + std::to_string(weight);
            }
            int width = 0;
            if (FcPatternGetInteger(font, FC_WIDTH, 0, &width) == FcResultMatch) {
                desc += ":width=" + std::to_string(width);
            }
            int spacing = 0;
            if (FcPatternGetInteger(font, FC_SPACING, 0, &spacing) == FcResultMatch) {
                desc += ":spacing=" + std::to_string(spacing);
            }
            entry.fc_name = std::move(desc);
        }
        fonts.push_back(std::move(entry));
    }

    FcFontSetDestroy(set);
    FcPatternDestroy(pattern);

    return fonts;
}

static bool InitFace(FT_Library lib, const FontEntry &entry, int pixel_size, FT_Face &out)
{
    if (FT_New_Face(lib, entry.path.c_str(), entry.face_index, &out) != 0) {
        return false;
    }
    if (FT_Set_Pixel_Sizes(out, 0, pixel_size) != 0) {
        FT_Done_Face(out);
        out = nullptr;
        return false;
    }
    return true;
}

static int ClampPointSize(int size)
{
    if (size < kMinPointSize) return kMinPointSize;
    if (size > kMaxPointSize) return kMaxPointSize;
    return size;
}

} // namespace

SDLFontDialogStatus SDLShowFontPicker(SDLFontSelection &selection)
{
    std::vector<FontEntry> fonts = CollectFonts();
    if (fonts.empty()) {
        return SDLFontDialogStatus::Failed;
    }

    const int win_w = 720;
    const int win_h = 540;
    SDL_Window *window = SDL_CreateWindow("Select Font", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                         win_w, win_h, SDL_WINDOW_SHOWN);
    if (!window) {
        return SDLFontDialogStatus::Failed;
    }
    SDL_ShowWindow(window);
    SDL_RaiseWindow(window);
    SDL_FlushEvent(SDL_KEYDOWN);
    SDL_FlushEvent(SDL_KEYUP);
    SDL_FlushEvent(SDL_TEXTINPUT);
    SDL_FlushEvent(SDL_MOUSEBUTTONDOWN);
    SDL_FlushEvent(SDL_MOUSEBUTTONUP);
    SDL_FlushEvent(SDL_MOUSEWHEEL);

    FT_Library ft = nullptr;
    if (FT_Init_FreeType(&ft) != 0) {
        SDL_DestroyWindow(window);
        return SDLFontDialogStatus::Failed;
    }

    const int ui_font_px = 16;
    FT_Face ui_face = nullptr;
    if (!InitFace(ft, fonts.front(), ui_font_px, ui_face)) {
        FT_Done_FreeType(ft);
        SDL_DestroyWindow(window);
        return SDLFontDialogStatus::Failed;
    }

    int point_size = ClampPointSize(static_cast<int>(selection.point_size > 0.0f ? selection.point_size : 18));
    int selected = 0;
    int scroll = 0;

    const int pad = 12;
    const int header_h = 48;
    const int footer_h = 44;
    const int btn_size = 24;
    const int action_btn_w = 90;
    const int action_btn_h = 28;
    const int line_h = std::max(18, static_cast<int>(ui_face->size->metrics.height >> 6));
    const int list_y = header_h;
    const int list_h = win_h - header_h - footer_h;
    const int visible_rows = std::max(1, list_h / line_h);

    bool running = true;
    SDLFontDialogStatus status = SDLFontDialogStatus::Cancelled;

    const Uint32 dialog_start_ticks = SDL_GetTicks();
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            const bool ignore_submit = (SDL_GetTicks() - dialog_start_ticks) < 200;
            if (!ignore_submit && ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
                status = SDLFontDialogStatus::Cancelled;
                break;
            }
            if (!ignore_submit && ev.type == SDL_KEYDOWN && (ev.key.keysym.sym == SDLK_RETURN || ev.key.keysym.sym == SDLK_KP_ENTER)) {
                running = false;
                status = SDLFontDialogStatus::Chosen;
                break;
            }
            if (ev.type == SDL_QUIT) {
                running = false;
                status = SDLFontDialogStatus::Cancelled;
            } else if (ev.type == SDL_KEYDOWN) {
                switch (ev.key.keysym.sym) {
                case SDLK_ESCAPE:
                    running = false;
                    status = SDLFontDialogStatus::Cancelled;
                    break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    running = false;
                    status = SDLFontDialogStatus::Chosen;
                    break;
                case SDLK_UP:
                    if (selected > 0) {
                        selected--;
                    }
                    break;
                case SDLK_DOWN:
                    if (selected + 1 < static_cast<int>(fonts.size())) {
                        selected++;
                    }
                    break;
                case SDLK_PAGEUP:
                    selected = std::max(0, selected - visible_rows);
                    break;
                case SDLK_PAGEDOWN:
                    selected = std::min(static_cast<int>(fonts.size()) - 1, selected + visible_rows);
                    break;
                case SDLK_HOME:
                    selected = 0;
                    break;
                case SDLK_END:
                    selected = static_cast<int>(fonts.size()) - 1;
                    break;
                case SDLK_EQUALS:
                case SDLK_PLUS:
                case SDLK_KP_PLUS:
                    point_size = ClampPointSize(point_size + 1);
                    break;
                case SDLK_MINUS:
                case SDLK_KP_MINUS:
                    point_size = ClampPointSize(point_size - 1);
                    break;
                default:
                    break;
                }
            } else if (ev.type == SDL_MOUSEWHEEL) {
                if (ev.wheel.y > 0) {
                    selected = std::max(0, selected - 1);
                } else if (ev.wheel.y < 0) {
                    selected = std::min(static_cast<int>(fonts.size()) - 1, selected + 1);
                }
            } else if (ev.type == SDL_MOUSEBUTTONDOWN) {
                const int mx = ev.button.x;
                const int my = ev.button.y;
                const int size_w = MeasureText(ui_face, "Size: 99  (+/-)");
                const int size_x = win_w - pad - size_w;
                const int size_y = pad;
                const SDL_Rect minus_rect{size_x + size_w - btn_size * 2 - 4, size_y, btn_size, btn_size};
                const SDL_Rect plus_rect{size_x + size_w - btn_size, size_y, btn_size, btn_size};
                const SDL_Rect list_rect{0, list_y, win_w, list_h};
                const SDL_Rect ok_rect{win_w - pad - action_btn_w, win_h - footer_h + 8, action_btn_w, action_btn_h};
                const SDL_Rect cancel_rect{win_w - pad - action_btn_w * 2 - 8, win_h - footer_h + 8, action_btn_w, action_btn_h};

                auto hit = [&](const SDL_Rect &r) {
                    return mx >= r.x && mx < (r.x + r.w) && my >= r.y && my < (r.y + r.h);
                };

                if (hit(minus_rect)) {
                    point_size = ClampPointSize(point_size - 1);
                } else if (hit(plus_rect)) {
                    point_size = ClampPointSize(point_size + 1);
                } else if (hit(ok_rect)) {
                    running = false;
                    status = SDLFontDialogStatus::Chosen;
                } else if (hit(cancel_rect)) {
                    running = false;
                    status = SDLFontDialogStatus::Cancelled;
                } else if (hit(list_rect)) {
                    const int row = (my - list_y) / line_h;
                    const int idx = scroll + row;
                    if (idx >= 0 && idx < static_cast<int>(fonts.size())) {
                        selected = idx;
                        if (ev.button.clicks >= 2) {
                            running = false;
                            status = SDLFontDialogStatus::Chosen;
                        }
                    }
                }
            }
        }

        if (selected < scroll) {
            scroll = selected;
        } else if (selected >= scroll + visible_rows) {
            scroll = selected - visible_rows + 1;
        }
        scroll = std::max(0, std::min(scroll, std::max(0, static_cast<int>(fonts.size()) - visible_rows)));

        SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, win_w, win_h, 32, SDL_PIXELFORMAT_RGBA32);
        if (!surface) {
            status = SDLFontDialogStatus::Failed;
            break;
        }

        FillRect(surface, 0, 0, win_w, win_h, Color{22, 24, 28, 255});
        FillRect(surface, 0, 0, win_w, header_h, Color{30, 32, 38, 255});
        FillRect(surface, 0, win_h - footer_h, win_w, footer_h, Color{30, 32, 38, 255});

        RenderText(surface, ui_face, pad, pad, "Select Font", Color{230, 230, 230, 255});

        std::string size_line = "Size: " + std::to_string(point_size);
        const int size_w = MeasureText(ui_face, size_line);
        const int size_x = win_w - pad - size_w - btn_size * 2 - 8;
        RenderText(surface, ui_face, size_x, pad, size_line, Color{200, 200, 200, 255});
        const int btn_y = pad;
        const SDL_Rect minus_rect{size_x + size_w + 4, btn_y, btn_size, btn_size};
        const SDL_Rect plus_rect{size_x + size_w + 4 + btn_size + 4, btn_y, btn_size, btn_size};
        FillRect(surface, minus_rect.x, minus_rect.y, minus_rect.w, minus_rect.h, Color{50, 54, 62, 255});
        FillRect(surface, plus_rect.x, plus_rect.y, plus_rect.w, plus_rect.h, Color{50, 54, 62, 255});
        RenderText(surface, ui_face, minus_rect.x + 7, minus_rect.y + 2, "-", Color{230, 230, 230, 255});
        RenderText(surface, ui_face, plus_rect.x + 6, plus_rect.y + 2, "+", Color{230, 230, 230, 255});

        const int list_x = pad;
        for (int i = 0; i < visible_rows; ++i) {
            const int idx = scroll + i;
            if (idx >= static_cast<int>(fonts.size())) {
                break;
            }
            const int y = list_y + i * line_h;
            if (idx == selected) {
                FillRect(surface, 0, y, win_w, line_h, Color{60, 80, 120, 255});
            }
            RenderText(surface, ui_face, list_x, y, fonts[idx].label, Color{240, 240, 240, 255});
        }

        RenderText(surface, ui_face, pad, win_h - footer_h + 10,
                   "Enter=OK  Esc=Cancel  Up/Down=Select  PgUp/PgDn=Scroll", Color{180, 180, 180, 255});

        const SDL_Rect ok_rect{win_w - pad - action_btn_w, win_h - footer_h + 8, action_btn_w, action_btn_h};
        const SDL_Rect cancel_rect{win_w - pad - action_btn_w * 2 - 8, win_h - footer_h + 8, action_btn_w, action_btn_h};
        FillRect(surface, cancel_rect.x, cancel_rect.y, cancel_rect.w, cancel_rect.h, Color{50, 54, 62, 255});
        FillRect(surface, ok_rect.x, ok_rect.y, ok_rect.w, ok_rect.h, Color{70, 100, 150, 255});
        RenderText(surface, ui_face, cancel_rect.x + 14, cancel_rect.y + 4, "Cancel", Color{230, 230, 230, 255});
        RenderText(surface, ui_face, ok_rect.x + 26, ok_rect.y + 4, "OK", Color{240, 240, 240, 255});

        SDL_Surface *window_surface = SDL_GetWindowSurface(window);
        if (window_surface) {
            SDL_BlitSurface(surface, nullptr, window_surface, nullptr);
            SDL_UpdateWindowSurface(window);
        }
        SDL_FreeSurface(surface);

        SDL_Delay(16);
    }

    if (status == SDLFontDialogStatus::Chosen) {
        selection.path = fonts[selected].path;
        selection.face_index = fonts[selected].face_index;
        if (!fonts[selected].fc_name.empty()) {
            selection.fc_name = fonts[selected].fc_name;
        }
        selection.point_size = static_cast<float>(point_size);
    }

    FT_Done_Face(ui_face);
    FT_Done_FreeType(ft);
    SDL_DestroyWindow(window);
    SDL_FlushEvent(SDL_MOUSEMOTION);
    SDL_FlushEvent(SDL_MOUSEBUTTONDOWN);
    SDL_FlushEvent(SDL_MOUSEBUTTONUP);
    SDL_FlushEvent(SDL_MOUSEWHEEL);
    SDL_FlushEvent(SDL_TEXTINPUT);
    SDL_FlushEvent(SDL_KEYDOWN);
    SDL_FlushEvent(SDL_KEYUP);
    SDL_Event wake{};
    wake.type = SDL_USEREVENT;
    SDL_PushEvent(&wake);

    return status;
}

void SDLInstallMacFontMenu(void (*)(void *), void *)
{
}
