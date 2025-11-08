#include <farplug-wide.h>
#include <WinPort.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cwctype>
#include <farkeys.h>
#include <memory>
#include <cstdio>
#include <utils.h>

#define WINPORT_IMAGE_ID "image_viewer"

// Глобальные переменные для FAR API
PluginStartupInfo g_far;
FarStandardFunctions g_fsf;

// --- Структура для передачи данных в диалог ---
struct ViewerData {
    HCONSOLEIMAGE imageHandle;
    std::wstring fileName;
};


// Прототип нашей диалоговой процедуры
LONG_PTR WINAPI ViewerDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2);

// --- Вспомогательные функции ---

bool IsImageFile(const wchar_t* FileName) {
    if (!FileName) return false;
    std::wstring lowerName = FileName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);

    const std::vector<std::wstring> supported_exts = { L".png", L".jpg", L".jpeg", L".gif", L".bmp" };
    for (const auto& ext : supported_exts) {
        if (lowerName.length() >= ext.length() && lowerName.compare(lowerName.length() - ext.length(), ext.length(), ext) == 0) {
            return true;
        }
    }
    return false;
}

bool LoadAndLetterboxImage(const wchar_t* path_name, int target_w_cells, int target_h_cells) {
    fprintf(stderr, "\n--- ImageViewer: Starting image processing for '%ls' ---\n", path_name);
    fprintf(stderr, "Target cell grid size: %dx%d\n", target_w_cells, target_h_cells);
	if (target_w_cells <= 0 || target_h_cells <= 0) {
        fprintf(stderr, "ERROR: bad grid size.\n");
		return false;
	}

    // 1. Получаем оригинальные размеры картинки
    std::string cmd = "identify -format \"%w %h\" \"";
    cmd += Wide2MB(path_name);
    cmd += "\"";

    std::string dims_str;
    if (!POpen(dims_str, cmd.c_str())) {
        fprintf(stderr, "ERROR: ImageMagick 'identify' failed.\n");
        return false;
    }

    int orig_w = 0, orig_h = 0;
    if (sscanf(dims_str.c_str(), "%d %d", &orig_w, &orig_h) != 2 || orig_w <= 0 || orig_h <= 0) {
        fprintf(stderr, "ERROR: Failed to parse original dimensions. Got: '%s'\n", dims_str.c_str());
        return false;
    }

    // 2. Получаем соотношение сторон ячейки терминала
	WinportGraphicsInfo wgi{};

	if (!WINPORT(GetConsoleImageCaps)(NULL, sizeof(wgi), &wgi) || !wgi.Caps) {
        fprintf(stderr, "ERROR: GetConsoleImageCaps failed\n");
        return false;
	}

    fprintf(stderr, "Original image size: %dx%d pixels, PixPerCell=%dx%d\n", orig_w, orig_h, wgi.PixPerCell.X, wgi.PixPerCell.Y);

    double cell_aspect_ratio = 0.5;
    if (wgi.PixPerCell.Y > 0 && wgi.PixPerCell.X > 0) {
		cell_aspect_ratio = double(wgi.PixPerCell.X) / double(wgi.PixPerCell.Y);
        fprintf(stderr, "GetConsoleCellAspectRatio returned: %f\n", cell_aspect_ratio);
    } else {
        fprintf(stderr, "GetConsoleCellAspectRatio returned 0.0, using default: %f\n", cell_aspect_ratio);
    }

    // 3. Вычисляем итоговое соотношение сторон целевой области в пикселях
    double target_pixel_aspect = ((double)target_w_cells / (double)target_h_cells) * cell_aspect_ratio;
    fprintf(stderr, "Target pixel aspect ratio for the cell grid is %f\n", target_pixel_aspect);

    // 4. Вычисляем размер холста, который будет иметь нужные пропорции,
    //    но при этом будет не меньше оригинальной картинки, чтобы избежать апскейла.
    double img_aspect = (double)orig_w / orig_h;
    int canvas_w{}, canvas_h{};

    if (img_aspect > target_pixel_aspect) {
        // Изображение "шире" целевой области. Ширина холста = ширина картинки.
        canvas_w = orig_w;
        canvas_h = (int)(orig_w / target_pixel_aspect);
    } else {
        // Изображение "выше" или такое же по пропорциям. Высота холста = высота картинки.
        canvas_h = orig_h;
        canvas_w = (int)(orig_h * target_pixel_aspect);
    }
    fprintf(stderr, "Calculated canvas size for letterboxing: %dx%d pixels\n", canvas_w, canvas_h);

    // 5. Формируем команду для imagemagick: без ресайза, только центрирование и добавление полей.
    cmd = "convert \"";
    cmd += Wide2MB(path_name);
    cmd += "\" -background black -gravity center -extent " + std::to_string(canvas_w) + "x" + std::to_string(canvas_h);
    cmd += " -depth 8 rgba:-";
    
    fprintf(stderr, "Executing ImageMagick: %s\n", cmd.c_str());

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        fprintf(stderr, "ERROR: ImageMagick 'convert' failed.\n");
        return false;
    }

    std::vector<uint8_t> final_pixel_data(canvas_w * canvas_h * 4);
    size_t bytes_read = fread(final_pixel_data.data(), 1, final_pixel_data.size(), pipe);
    pclose(pipe);

    if (bytes_read != final_pixel_data.size()) {
        fprintf(stderr, "ERROR: Failed to read final pixel data from ImageMagick.\n");
        return false;
    }

    // 6. Создаем ConsoleImage с готовым битмапом.
    fprintf(stderr, "--- Image processing finished, creating ConsoleImage ---\n\n");
	COORD pos = {0, 0};
    return WINPORT(SetConsoleImage)(NULL, WINPORT_IMAGE_ID, 0, pos, canvas_w, canvas_h, final_pixel_data.data()) != FALSE;
}


// --- Основная логика плагина ---

void ShowImage(const wchar_t* FileName) {
    auto data = std::make_unique<ViewerData>();
    data->fileName = FileName;
    data->imageHandle = nullptr;

    SMALL_RECT Rect;
    g_far.AdvControl(g_far.ModuleNumber, ACTL_GETFARRECT, &Rect, 0);

    FarDialogItem DlgItems[] = {
        {DI_USERCONTROL, 0, 0, Rect.Right, Rect.Bottom, 0, {}, 0, 0, L"", 0},
    };

    HANDLE hDlg = g_far.DialogInit(g_far.ModuleNumber, -1, -1, Rect.Right+1, Rect.Bottom+1,
                             L"ImageViewer", DlgItems, sizeof(DlgItems)/sizeof(DlgItems[0]),
                             0, FDLG_NODRAWSHADOW|FDLG_NODRAWPANEL, ViewerDlgProc, (LONG_PTR)data.get());

    if (hDlg != INVALID_HANDLE_VALUE) {
        data.release();
        g_far.DialogRun(hDlg);
        g_far.DialogFree(hDlg);
    }
}

// --- Диалоговая процедура ---

LONG_PTR WINAPI ViewerDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
    ViewerData* data = (ViewerData*)g_far.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);

    switch(Msg) {
        case DN_INITDIALOG:
        {
            ViewerData* initData = (ViewerData*)Param2;
            g_far.SendDlgMessage(hDlg, DM_SETDLGDATA, 0, (LONG_PTR)initData);

            SMALL_RECT Rect;
            g_far.AdvControl(g_far.ModuleNumber, ACTL_GETFARRECT, &Rect, 0);
            int target_w_cells = Rect.Right > 1 ? Rect.Right - 1 : 1;
            int target_h_cells = Rect.Bottom > 3 ? Rect.Bottom - 3 : 1;

            bool ok = LoadAndLetterboxImage(initData->fileName.c_str(), target_w_cells, target_h_cells);

            if (!ok) {
                const wchar_t *MsgItems[]={L"Image Viewer", L"Failed to load image file:", initData->fileName.c_str()};
                g_far.Message(g_far.ModuleNumber, FMSG_WARNING|FMSG_ERRORTYPE, nullptr, MsgItems, sizeof(MsgItems)/sizeof(MsgItems[0]), 1);
                g_far.SendDlgMessage(hDlg, DM_CLOSE, -1, 0);
            }
            return TRUE;
        }

        case DN_KEY:
        {
            int Key = (int)Param2;
            if (Key == KEY_ESC || Key == KEY_F10 || Key == KEY_ENTER) {
                g_far.SendDlgMessage(hDlg, DM_CLOSE, 1, 0);
                return TRUE;
            }
            return TRUE;
        }

        case DN_CLOSE:
        {
            if (data) {
                WINPORT(DeleteConsoleImage)(NULL, WINPORT_IMAGE_ID);
                delete data;
                g_far.SendDlgMessage(hDlg, DM_SETDLGDATA, 0, 0);
            }
            return TRUE;
        }
    }

    return g_far.DefDlgProc(hDlg, Msg, Param1, Param2);
}


// --- Экспортируемые функции плагина ---

SHAREDSYMBOL void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info)
{
    g_far = *Info;
    g_fsf = *Info->FSF;
}

SHAREDSYMBOL void WINAPI GetPluginInfoW(struct PluginInfo *Info)
{
    Info->StructSize = sizeof(struct PluginInfo);
    Info->Flags = 0;

    static const wchar_t *PluginMenuStrings[1];
    PluginMenuStrings[0] = L"Image Viewer";
    Info->PluginMenuStrings = PluginMenuStrings;
    Info->PluginMenuStringsNumber = 1;
}

SHAREDSYMBOL HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item)
{
    if (OpenFrom == OPEN_PLUGINSMENU) {
        size_t size = g_far.Control(PANEL_ACTIVE, FCTL_GETCURRENTPANELITEM, 0, 0);
        if (size > 0) {
            std::vector<char> buf(size);
            PluginPanelItem* ppi = reinterpret_cast<PluginPanelItem*>(buf.data());
            g_far.Control(PANEL_ACTIVE, FCTL_GETCURRENTPANELITEM, size, (LONG_PTR)ppi);

            if (wcscmp(ppi->FindData.lpwszFileName, L"..") != 0 && !(ppi->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	        {
                std::wstring fn = ppi->FindData.lpwszFileName;

	            if (IsImageFile(fn.c_str())) {
	                ShowImage(fn.c_str());
	            } else {
	                const wchar_t *MsgItems[]={L"Image Viewer", L"This is not a supported image file."};
	                g_far.Message(g_far.ModuleNumber, FMSG_WARNING, nullptr, MsgItems, sizeof(MsgItems)/sizeof(MsgItems[0]), 1);
	            }
	        }
        }
    }

    return INVALID_HANDLE_VALUE;
}

SHAREDSYMBOL void WINAPI ClosePluginW(HANDLE hPlugin) {}
SHAREDSYMBOL int WINAPI ProcessEventW(HANDLE hPlugin, int Event, void *Param) { return FALSE; }