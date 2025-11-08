#pragma once
#include <stdint.h>
#include "../WinCompat.h"
#include <vector>

struct ConsoleImage
{
    uint32_t id;             // Уникальный ID изображения.

    std::vector<uint8_t> pixel_data; // Сырые пиксели в формате RGBA.
    uint32_t width;          // Ширина в пикселях.
    uint32_t height;         // Высота в пикселях.

    COORD grid_origin;       // Координаты (колонка, строка) на сетке консоли.
    COORD grid_size;         // Запрошенный размер в ячейках (ширина, высота).
    // ... другие поля для будущих расширений (z_index, etc.)
};

