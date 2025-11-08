#include <farplug-wide.h>
#include <WinPort.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cwctype>
#include <farkeys.h>
#include <memory>
#include <cstdio>
#include <set>
#include <dirent.h>
#include <utils.h>

#define WINPORT_IMAGE_ID "image_viewer"

// Глобальные переменные для FAR API
PluginStartupInfo g_far;
FarStandardFunctions g_fsf;

// --- Структура для передачи данных в диалог ---
struct ViewerData {
    std::wstring fileName;
    HCONSOLEIMAGE imageHandle{nullptr};
	enum Result
	{
		CLOSE,
		NEXT,
		PREV,
		ERROR
	} result {CLOSE};
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
	int canvas_w = target_w_cells * wgi.PixPerCell.X;
	int canvas_h = target_h_cells * wgi.PixPerCell.Y;

    fprintf(stderr, "Image pixels size, original: %dx%d canvas: %dx%d\n", orig_w, orig_h, canvas_w, canvas_h);

    // 5. Формируем команду для imagemagick: без ресайза, только центрирование и добавление полей.
    cmd = "convert \"";
    cmd += Wide2MB(path_name);
    cmd += "\" -background black -gravity center";
    cmd += " -resize " + std::to_string(canvas_w) + "x" + std::to_string(canvas_h);
    cmd += " -extent " + std::to_string(canvas_w) + "x" + std::to_string(canvas_h);
    cmd += " -depth 8 rgba:-";

    
    fprintf(stderr, "Executing ImageMagick: %s\n", cmd.c_str());

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        fprintf(stderr, "ERROR: ImageMagick start failed.\n");
        return false;
    }

    std::vector<uint8_t> final_pixel_data(canvas_w * canvas_h * 4);
    size_t bytes_read = fread(final_pixel_data.data(), 1, final_pixel_data.size(), pipe);
    if (pclose(pipe) != 0) {
        fprintf(stderr, "ERROR: ImageMagick 'convert' failed.\n");
        return false;
	}

    if (bytes_read != final_pixel_data.size()) {
        fprintf(stderr, "ERROR: Failed to read final pixel data from ImageMagick.\n");
        return false;
    }

    // 6. Создаем ConsoleImage с готовым битмапом.
    fprintf(stderr, "--- Image processing finished, creating ConsoleImage ---\n\n");
	COORD pos = {1, 1};
    return WINPORT(SetConsoleImage)(NULL, WINPORT_IMAGE_ID, 0, pos, canvas_w, canvas_h, final_pixel_data.data()) != FALSE;
}


// --- Основная логика плагина ---

static ViewerData::Result ShowImage(const wchar_t* FileName)
{
    ViewerData data;
    data.fileName = FileName;
//	data.annotate = annotate;

    SMALL_RECT Rect;
    g_far.AdvControl(g_far.ModuleNumber, ACTL_GETFARRECT, &Rect, 0);

    FarDialogItem DlgItems[] = {
		{ DI_DOUBLEBOX, 0, 0, Rect.Right, Rect.Bottom, FALSE, {}, 0, 0, FileName, 0 },
        { DI_USERCONTROL, 0, 0, Rect.Right, Rect.Bottom, 0, {}, 0, 0, L"", 0},
    };

    HANDLE hDlg = g_far.DialogInit(g_far.ModuleNumber, 0, 0, Rect.Right, Rect.Bottom,
                             L"ImageViewer", DlgItems, sizeof(DlgItems)/sizeof(DlgItems[0]),
                             0, FDLG_NODRAWSHADOW|FDLG_NODRAWPANEL, ViewerDlgProc, (LONG_PTR)&data);

    if (hDlg != INVALID_HANDLE_VALUE) {
        g_far.DialogRun(hDlg);
        g_far.DialogFree(hDlg);
    }
	return data.result;
}

static void ShowImageLoop(std::wstring file_name)
{
	std::set<std::string> all_files;
	for (ViewerData::Result res = ViewerData::ERROR;;) {
		auto cur_res = ShowImage(file_name.c_str());
		if (cur_res == ViewerData::CLOSE) {
			break;
		}
		if (cur_res != ViewerData::ERROR) {
			res = cur_res;
		} else if (res == ViewerData::ERROR) {
            const wchar_t *MsgItems[]={L"Image Viewer", L"Failed to load image file:", file_name.c_str()};
            g_far.Message(g_far.ModuleNumber, FMSG_WARNING|FMSG_ERRORTYPE, nullptr, MsgItems, sizeof(MsgItems)/sizeof(MsgItems[0]), 1);
			break;
		}

		if (all_files.empty()) {
			DIR *d = opendir(".");
			if (d) {
				for (;;) {
					auto *de = readdir(d);
					if (!de) break;
					if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
						all_files.insert(de->d_name);
					}
				}
				closedir(d);
			}
		}
		auto it = all_files.find(StrWide2MB(file_name));
		if (it == all_files.end()) {
			break;
		}
		if (res == ViewerData::NEXT) {
			++it;
			if (it == all_files.end()) {
				break;
			}
		} else if (it == all_files.begin()) {
			break;
		} else {
			--it;
		}
		file_name = StrMB2Wide(*it);
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
            int target_h_cells = Rect.Bottom > 1 ? Rect.Bottom - 1 : 1;

            bool ok = LoadAndLetterboxImage(initData->fileName.c_str(), target_w_cells, target_h_cells);
            if (!ok) {
				data->result = ViewerData::ERROR;
                g_far.SendDlgMessage(hDlg, DM_CLOSE, -1, 0);
            }
            return TRUE;
        }

        case DN_KEY:
        {
            int Key = (int)Param2;
            if (Key == KEY_ESC || Key == KEY_F10 || Key == KEY_ENTER) {
				data->result = ViewerData::CLOSE;
                g_far.SendDlgMessage(hDlg, DM_CLOSE, 1, 0);
                return TRUE;
            }
            if (Key == KEY_SPACE || Key == KEY_RIGHT || Key == KEY_BS || Key == KEY_LEFT) {
				data->result = (Key == KEY_BS || Key == KEY_LEFT) ? ViewerData::PREV : ViewerData::NEXT;
                g_far.SendDlgMessage(hDlg, DM_CLOSE, 1, 0);
                return TRUE;
			}
            return TRUE;
        }

        case DN_CLOSE:
        {
            if (data) {
                WINPORT(DeleteConsoleImage)(NULL, WINPORT_IMAGE_ID);
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
	                ShowImageLoop(fn.c_str());
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
