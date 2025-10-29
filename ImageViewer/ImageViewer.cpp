#include <farplug-wide.h>
#include <src/WinPortGraphics.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cwctype>
#include <farkeys.h>
#include <memory>
#include <cstdio>  // Для popen, pclose, fread
#include <utils.h> // Для Wide2MB, POpen

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

HCONSOLEIMAGE LoadAndScaleImage(const wchar_t* path_name, int target_w_cells, int target_h_cells) {
    // 1. Получаем оригинальные размеры картинки
    std::string cmd = "identify -format \"%w %h\" \"";
    cmd += Wide2MB(path_name);
    cmd += "\"";

    std::string dims_str;
    if (!POpen(dims_str, cmd.c_str())) {
        fprintf(stderr, "ImageViewer: ImageMagick 'identify' failed for '%ls'\n", path_name);
        return nullptr;
    }

    int orig_w = 0, orig_h = 0;
    if (sscanf(dims_str.c_str(), "%d %d", &orig_w, &orig_h) != 2 || orig_w <= 0 || orig_h <= 0) {
        fprintf(stderr, "ImageViewer: Failed to parse original dimensions. Got: '%s'\n", dims_str.c_str());
        return nullptr;
    }

    // 2. Вычисляем целевые размеры в пикселях
    // TODO: Получить реальные размеры шрифта из API, когда они будут доступны.
    // Пока используем типичные значения.
    int font_w = 8, font_h = 16;
    int target_w_px = target_w_cells * font_w;
    int target_h_px = target_h_cells * font_h;

    // 3. Рассчитываем пропорции и итоговый размер отмасштабированной картинки
    double img_aspect = (double)orig_w / orig_h;
    double target_aspect = (double)target_w_px / target_h_px;

    int scaled_w = target_w_px;
    int scaled_h = target_h_px;

    if (img_aspect > target_aspect) { // Картинка шире, чем область
        scaled_h = (int)(target_w_px / img_aspect);
    } else { // Картинка выше, чем область
        scaled_w = (int)(target_h_px * img_aspect);
    }

    // 4. Запрашиваем у imagemagick уже отмасштабированную картинку
    cmd = "convert \"";
    cmd += Wide2MB(path_name);
    cmd += "\" -resize ";
    cmd += std::to_string(scaled_w) + "x" + std::to_string(scaled_h) + "! "; // '!' - игнорировать пропорции, т.к. мы их уже посчитали
    cmd += "-depth 8 rgba:-";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        fprintf(stderr, "ImageViewer: ImageMagick 'convert' failed for '%ls'\n", path_name);
        return nullptr;
    }

    std::vector<uint8_t> scaled_pixel_data(scaled_w * scaled_h * 4);
    size_t bytes_read = fread(scaled_pixel_data.data(), 1, scaled_pixel_data.size(), pipe);
    pclose(pipe);
    if (bytes_read != scaled_pixel_data.size()) {
        fprintf(stderr, "ImageViewer: Failed to read scaled pixel data for '%ls'\n", path_name);
        return nullptr;
    }

    // 5. Создаем финальный буфер (холст) и центрируем на нем картинку (letterboxing)
    std::vector<uint8_t> final_buffer(target_w_px * target_h_px * 4, 0); // Заполняем черным цветом (RGBA=0,0,0,0 - прозрачный черный)
    for(size_t i = 0; i < final_buffer.size() / 4; ++i) final_buffer[i*4+3] = 255; // Делаем фон непрозрачным черным (RGBA=0,0,0,255)

    int x_offset = (target_w_px - scaled_w) / 2;
    int y_offset = (target_h_px - scaled_h) / 2;

    for (int y = 0; y < scaled_h; ++y) {
        uint8_t* dest_row = &final_buffer[((y_offset + y) * target_w_px + x_offset) * 4];
        uint8_t* src_row = &scaled_pixel_data[y * scaled_w * 4];
        memcpy(dest_row, src_row, scaled_w * 4);
    }

    return WINPORT(CreateConsoleImageFromBuffer)(final_buffer.data(), target_w_px, target_h_px, 0);
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

// --- Диалоговая процедура для нашего "окна" просмотра ---

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
            int target_h_cells = Rect.Bottom > 1 ? Rect.Bottom - 1 : 1;

            initData->imageHandle = LoadAndScaleImage(initData->fileName.c_str(), target_w_cells, target_h_cells);

            if (initData->imageHandle) {
                ConsoleImage* img = static_cast<ConsoleImage*>(initData->imageHandle);
                img->grid_origin.X = 1;
                img->grid_origin.Y = 1;
                // grid_size теперь не нужен, т.к. битмап уже правильного размера
                img->grid_size = {0, 0};

                WINPORT(DisplayConsoleImage)(initData->imageHandle);
            } else {
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
                if (data->imageHandle) {
                    WINPORT(DeleteConsoleImage)(data->imageHandle, 0);
                }
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
                PanelInfo pinfo = {};
                g_far.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pinfo);

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