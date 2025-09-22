#pragma once
#include <cassert>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <iostream>

#include <farplug-wide.h>

extern PluginStartupInfo _PSI;

namespace fardialog {

class Sizer;

class Dialog {
public:
    Sizer *contents;
	std::map<const char *, int> name2fdi;
    std::vector<FarDialogItem> fdi;
    PluginStartupInfo *plugin;
    int width;
    int height;
    const wchar_t *title;
    const wchar_t *helptopic;
    int flags;
    FARWINDOWPROC cb;
	HANDLE hDlg;
	LONG_PTR param;

    Dialog(PluginStartupInfo *plugin_, const wchar_t *title_, const wchar_t *helptopic_, int flags_, FARWINDOWPROC cb_, LONG_PTR param_)
        : plugin(plugin_), title(title_), helptopic(helptopic_), flags(flags_), cb(cb_), param(param_)
    {
    }
    void CreateFDI(int fdiCount) {
        fdi.resize(fdiCount+1);
    }
    void SetSize(int width_, int height_)
    {
        width = width_;
        height = height_;
    }

    void buildFDI(Sizer *contents);
    void show();
	HANDLE DialogInit();

	int getID(const char *name) {
		std::map<const char *,int>::iterator it = name2fdi.find(name);
  		if (it != name2fdi.end())
			return it->second;
		assert(false);
		return -1;
	}

	int DialogRun() {
		return plugin->DialogRun(hDlg);
	}
	void DialogFree() {
		plugin->DialogFree(hDlg);
	}
	void Close(int exitcode) {
        plugin->SendDlgMessage(hDlg, DM_CLOSE, exitcode, 0);
    }

    void EnableRedraw(bool on) {
        plugin->SendDlgMessage(hDlg, DM_ENABLEREDRAW, on ? 1 : 0, 0);
    }

    void RedrawDialog() {
        plugin->SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
    }

    int GetDlgData() {
        return (int)plugin->SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
    }

    void SetDlgData(int Data) {
        plugin->SendDlgMessage(hDlg, DM_SETDLGDATA, 0, Data);
    }

    int GetDlgItemData(int ID) {
        return (int)plugin->SendDlgMessage(hDlg, DM_GETITEMDATA, ID, 0);
    }

    void SetDlgItemData(int ID, int Data) {
        plugin->SendDlgMessage(hDlg, DM_SETITEMDATA, ID, Data);
    }

    int GetFocus() {
        return (int)plugin->SendDlgMessage(hDlg, DM_GETFOCUS, 0, 0);
    }

    void SetFocus(int ID) {
        plugin->SendDlgMessage(hDlg, DM_SETFOCUS, ID, 0);
    }

    void Enable(int ID) {
        plugin->SendDlgMessage(hDlg, DM_ENABLE, ID, 1);
    }

    void Disable(int ID) {
        plugin->SendDlgMessage(hDlg, DM_ENABLE, ID, 0);
    }

    int IsEnable(int ID) {
        return plugin->SendDlgMessage(hDlg, DM_ENABLE, ID, -1);
    }

    int GetTextLength(int ID) {
        return plugin->SendDlgMessage(hDlg, DM_GETTEXTPTR, ID, 0);
    }

    const wchar_t *GetText(int ID) {
        return (const wchar_t *)plugin->SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, ID, 0);
    }

    void SetText(int ID, const wchar_t *Str) {
        plugin->SendDlgMessage(hDlg, DM_SETTEXTPTR, ID, (LONG_PTR)Str);
    }

    int GetCheck(int ID) {
        return (int)plugin->SendDlgMessage(hDlg, DM_GETCHECK, ID, 0);
    }

    void SetCheck(int ID, int State) {
        plugin->SendDlgMessage(hDlg, DM_SETCHECK, ID, State);
    }

    void AddHistory(int ID, const std::wstring& Str) {
        plugin->SendDlgMessage(hDlg, DM_ADDHISTORY, ID, (LONG_PTR)Str.c_str());
    }

    void AddString(int ID, const std::wstring& Str) {
        plugin->SendDlgMessage(hDlg, DM_LISTADDSTR, ID, (LONG_PTR)Str.c_str());
    }

    int GetCurPos(int ID) {
        return (int)plugin->SendDlgMessage(hDlg, DM_LISTGETCURPOS, ID, 0);
    }

    void ClearList(int ID) {
        plugin->SendDlgMessage(hDlg, DM_LISTDELETE, ID, 0);
    }

    void SortUp(int ID) {
        plugin->SendDlgMessage(hDlg, DM_LISTSORT, ID, 0);
    }

    void SortDown(int ID) {
        plugin->SendDlgMessage(hDlg, DM_LISTSORT, ID, 1);
    }

    int GetItemData(int ID, int Index) {
        return plugin->SendDlgMessage(hDlg, DM_LISTGETDATA, ID, (LONG_PTR)Index);
    }

    COORD GetCursorPos(int ID) {
        COORD cpos = {0, 0};
        plugin->SendDlgMessage(hDlg, DM_GETCURSORPOS, ID, (LONG_PTR)&cpos);
        return cpos;
    }

    void SetCursorPos(int ID, int X, int Y) {
        COORD cpos = {(SHORT)X, (SHORT)Y};
        plugin->SendDlgMessage(hDlg, DM_SETCURSORPOS, ID, (LONG_PTR)&cpos);
    }

    void ShowItem(int ID, bool On) {
        plugin->SendDlgMessage(hDlg, DM_SHOWITEM, ID, On ? 1 : 0);
    }

    void SetCursorSize(int ID, bool On, int Size) {
        plugin->SendDlgMessage(hDlg, DM_SETCURSORSIZE, ID, On | (Size << 16));
    }
};

template <typename TClass, typename TCall>
class DialogT : public Dialog {
public:
	TClass &inst;
	TCall cb;
    DialogT(
		PluginStartupInfo *plugin_,
		const wchar_t *title_, 
		const wchar_t *helptopic_,
		int flags_,
		TClass &inst_,
		TCall cb_
	)
		:Dialog(plugin_, title_, helptopic_, flags_, &dlgcb, (LONG_PTR)this),
		inst(inst_),
		cb(cb_)
	{}
	static LONG_PTR WINAPI dlgcb(HANDLE dlg, int msg, int param1, LONG_PTR param2)
    {
		DialogT<TClass, TCall>* instance = nullptr;
		if (msg != DN_INITDIALOG)
			instance = reinterpret_cast<DialogT<TClass, TCall>*>(_PSI.SendDlgMessage(dlg, DM_GETDLGDATA, 0, 0));
		else {
			instance = reinterpret_cast<DialogT<TClass, TCall>*>(param2);
			_PSI.SendDlgMessage(dlg, DM_SETDLGDATA, 0, (LONG_PTR)instance);
		}
		assert(instance);
		return ((instance->inst).*(instance->cb))(dlg, msg, param1, param2);
    }
};

enum Orientation {
    horizontal = 0,
    vertical = 1
};

struct Position {
    int x, y;

	Position(int x = 0, int y = 0) : x(x), y(y) {}
};

struct Size {
    int width, height;

	Size(int w = 0, int h = 0) : width(w), height(h) {}
};

class Screen {
public:

	Screen(int width_, int height_):
        width(width_), height(height_)
    {
		buffer = new char[width * height];
        clear();
    }
    void clear(){
        memset(buffer, '.', width * height);
    }
    void write(int row, int col, std::wstring data){
        int len = data.length();
        std::string str(data.begin(), data.end());
        for (int i = 0; i < len && col + i < width; i++) {
            buffer[row * width + col + i] = str[i];
        }
    }
    void show() {
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                std::cout << buffer[i * width + j];
            }
            std::cout << std::endl;
        }
    }
private:
    char *buffer;
    int width;
    int height;
};

struct Border {
    int left, top, right, bottom;

	Border(int left_ = 0, int top_ = 0, int right_ = 0, int bottom_ = 0)
        : left(left_), top(top_), right(right_), bottom(bottom_) {}
};

class Window {
public:
    Position pos;
    Size size;

	Window() : pos(0, 0), size(0, 0) {}
    virtual ~Window() {}
    virtual void move(int l, int t, int w, int h) {
        pos = Position(l, t);
        size = Size(w, h);
    }
    virtual Size get_best_size() const = 0;
    virtual int fdiCount() const {return 1;};
    virtual void makeItem(Dialog *dlg, int& no) = 0;
    virtual void show(int indent) {
        std::cout << std::string(indent, ' ') << "Window at (" << pos.x << "," << pos.y << ") size (" << size.width << "x" << size.height << ")\n";
    }
    virtual void write(Screen& scr) {}
};

class Box {
public:
    Window* window;
    Border border;

	Box(Window* w, Border b) : window(w), border(b) {}
    void size(int left, int top, int right, int bottom) {
        int w = right - left;
        int h = bottom - top;
        window->move(left, top, w, h);
    }
    Size get_best_size() const {
        Size s = window->get_best_size();
        return Size(
            s.width + border.left + border.right,
            s.height + border.top + border.bottom
        );
    }
    void show(int indent) const {
        window->show(indent);
    }
    int fdiCount() const {
        return window->fdiCount();
    }
    void makeItem(Dialog *dlg, int& no) {
        window->makeItem(dlg, no);
    }
    void write(Screen& scr) const {
        window->write(scr);
    }
};


class Element : public Window {
public:
    FarDialogItem *fdi;
    DialogItemTypes dialogType;
	const char *elemname;
	const wchar_t *text;
    DWORD flags;
    int focus;
    int DefaultButton;
    size_t MaxLen;

	Element(DialogItemTypes dialogType_, const char *elemname_ = nullptr, const wchar_t* text_ = nullptr, DWORD flags_ = 0, int focus_ = 0, int DefaultButton_ = 0, size_t MaxLen_ = 0)
        : Window(), dialogType(dialogType_), elemname(elemname_), text(text_), flags(flags_), focus(focus_), DefaultButton(DefaultButton_), MaxLen(MaxLen_) {}
    void makeItem(Dialog *dlg, int& no) override {
        fdi = &dlg->fdi[no];
		dlg->name2fdi.insert(std::pair<const char *,int>(elemname, no));
        dlg->fdi[no] = {
            dialogType,
            pos.x, pos.y, pos.x + size.width - 1, pos.y + size.height - 1,
            focus,
            {}, //Reserved, Selected, History, Mask, ListItems, ListPos, VBuf
            flags,
            DefaultButton,
            text,
            MaxLen
        };
        no += 1;
    }
};

class Spacer : public Element {
public:

    Spacer(int width = 1, int height = 1) : Element(DI_TEXT) {
        size = Size(width, height);
    }
    Size get_best_size() const override { return size; }
    int fdiCount() const override {return 0;};
    void makeItem(Dialog *dlg, int& no) override {}
};

class DlgTEXT : public Element {
public:

	DlgTEXT(const char *elemname_, const wchar_t *text_)
        : Element(DI_TEXT, elemname_, text_) {}
    Size get_best_size() const override {
        std::wstring t(text);
        t.erase(std::remove(t.begin(), t.end(), '&'), t.end());
        return Size(t.length(), 1);
    }
    void write(Screen& scr) override {
        std::wstring t(text);
        t.erase(std::remove(t.begin(), t.end(), '&'), t.end());
        scr.write(pos.y, pos.x, t);
    }
};

class DlgEDIT : public Element {
public:
    int width;

	DlgEDIT(const char *elemname_, int width_, int maxlength = 0)
        : Element(DI_EDIT, elemname_), width(width_) {
		MaxLen = ( maxlength > 0 ) ? maxlength : width_;
	}
    Size get_best_size() const override {
        return Size(width, 1);
    }
};

class DlgPASSWORD : public Element {
public:
    int width;

	DlgPASSWORD(const char *elemname_, int width, int maxlength = 0)
		: Element(DI_PSWEDIT, elemname_), width(width) {
		MaxLen = ( maxlength > 0 ) ? maxlength : width;
	}
    Size get_best_size() const override {
        return Size(width, 1);
    }
    void write(Screen& scr) override {
        std::wstring t(L"");
        for(int i=0; i<size.width; i++) t.push_back('*');
        scr.write(pos.y, pos.x, t);
    }
};


class DlgMASKED : public Element {
public:
	const wchar_t *mask;

	DlgMASKED(const char *elemname_, const wchar_t *text_, const wchar_t *mask_, DWORD flags_=0)
        : Element(DI_FIXEDIT, elemname_, text_, flags_), mask(mask_){}
    Size get_best_size() const override {
        return Size(wcslen(mask), 1);
    }
    void makeItem(Dialog *dlg, int& no) override {
        Element::makeItem(dlg, no);
        dlg->fdi[no-1].Param.Mask = mask;
    };
};


class DlgBUTTON : public Element {
public:

	DlgBUTTON(const char *elemname_, const wchar_t *text_, DWORD flags_ = 0, int focus_ = 0, int DefaultButton_ = 0)
        : Element(DI_BUTTON, elemname_, text_, flags_, focus_, DefaultButton_) {}
    Size get_best_size() const override {
        std::wstring t(text);
        t.erase(std::remove(t.begin(), t.end(), '&'), t.end());
        return Size(4 + t.length(), 1);
    }
    void write(Screen& scr) override {
        std::wstring t(text);
        t.erase(std::remove(t.begin(), t.end(), '&'), t.end());
        t = L"[ " + t + L" ]";
        scr.write(pos.y, pos.x, t);
    }
};

class DlgCHECKBOX : public Element {
public:
	bool checked;

	DlgCHECKBOX(const char *elemname_, const wchar_t *text_, bool checked_)
        : Element(DI_CHECKBOX, elemname_, text_), checked(checked_) {}

	Size get_best_size() const override {
        std::wstring t(text);
        t.erase(std::remove(t.begin(), t.end(), '&'), t.end());
        return Size(4 + t.length(), 1);
    }

    void makeItem(Dialog *dlg, int& no) override {
        Element::makeItem(dlg, no);
        dlg->fdi[no-1].Param.Selected = checked ? 1:0;
    };

	void write(Screen& scr) override {
		std::wstring t(text);
		t.erase(std::remove(t.begin(), t.end(), '&'), t.end());
		t = (checked ? L"[x] " : L"[ ] ") + t;
		scr.write(pos.y, pos.x, t);
	}
};

class DlgRADIOBUTTON : public Element{
public:
	bool selected;

	DlgRADIOBUTTON(const char *elemname_, const wchar_t *text_, DWORD flags_=0, bool selected_=false)
        : Element(DI_RADIOBUTTON, elemname_, text_, flags_), selected(selected_) {}

	Size get_best_size() const override {
        std::wstring t(text);
        t.erase(std::remove(t.begin(), t.end(), '&'), t.end());
        return Size(4 + t.length(), 1);
    }

    void makeItem(Dialog *dlg, int& no) override {
        Element::makeItem(dlg, no);
        dlg->fdi[no-1].Param.Selected = selected ? 1:0;
    };

	void write(Screen& scr) override {
		std::wstring t(text);
		t.erase(std::remove(t.begin(), t.end(), '&'), t.end());
		t = (selected ? L"(x) " : L"( ) " ) + t;
		scr.write(pos.y, pos.x, t);
	}
};


class DlgCOMBOBOX : public Element{
public:
	FarList &items;
	int width;

	DlgCOMBOBOX(const char *elemname_, FarList &items_, int width_=0)
        : Element(DI_COMBOBOX, elemname_), items(items_) {
		if(width_ > 0) width = width_;
		else {
			for(int i=0; i<items.ItemsNumber; i++)
				width = std::max(width, (int)wcslen(items.Items[i].Text));
		}
	}

	Size get_best_size() const override {
        return Size(width + 2, 1);
    }

    void makeItem(Dialog *dlg, int& no) override {
        Element::makeItem(dlg, no);
        dlg->fdi[no-1].Param.ListItems = &items;
    };

	void write(Screen& scr) override {
		std::wstring t(items.Items[0].Text);
		scr.write(pos.y, pos.x, t.substr(0, width));
	}
};

class DlgLISTBOX : public Element{
public:
	FarList &items;
	int width;
	int height;

	DlgLISTBOX(const char *elemname_, FarList &items_, int width_=0, int height_=0)
        : Element(DI_LISTBOX, elemname_), items(items_), height(height_) {
		if(width_ > 0) width = width_;
		else {
			for(int i=0; i<items.ItemsNumber; i++)
				width = std::max(width, (int)wcslen(items.Items[i].Text));
		}
		if(height_ > 0) height = height_;
		else {
			height = items.ItemsNumber + 2;
		}
	}

	Size get_best_size() const override {
        return Size(width + 4, height);
    }

    void makeItem(Dialog *dlg, int& no) override {
        Element::makeItem(dlg, no);
        dlg->fdi[no-1].Param.ListItems = &items;
    };

	void write(Screen& scr) override {
		std::wstring t(items.Items[0].Text);
		scr.write(pos.y, pos.x, t.substr(0, width));
	}
};

class USERCONTROL : public Element {
public:
    int width;
    int height;

	USERCONTROL(const char *elemname_, int width_, int height_)
        : Element(DI_USERCONTROL, elemname_), width(width_), height(height_) {}
	Size get_best_size() const override {
        return Size(width, height);
    }
};

class DlgHLine : public Element {
public:
    DlgHLine(const char *elemname_=nullptr, const wchar_t *text_=nullptr)
		: Element(DI_TEXT, elemname_, text_, DIF_SEPARATOR) {}
    Size get_best_size() const override {
        return Size(1, 1);
    }
    void write(Screen& scr) override {
        std::wstring t(L"");
        for(int i=0; i<size.width; i++) t.push_back('-');
        scr.write(pos.y, pos.x, t);
    }
};

class Sizer : public Window {
public:
    Orientation orientation;
    std::vector<Box> boxes;
    Border border;
    Sizer(Orientation o, const std::vector<Window*>& controls, Border b)
        : orientation(o), border(b) {
        for (auto c : controls) {
            boxes.emplace_back(c, b);
        }
    }
    void move(int x, int y, int w, int h) override {
        size(x, y, w, h);
    }
    void size(int l, int t, int r, int b) {
        l += border.left;
        t += border.top;
        r -= border.right;
        b -= border.bottom;
        int hoffset = l, voffset = t;
        for (auto& box : boxes) {
            Size s = box.get_best_size();
            if (orientation == horizontal) {
                box.size(hoffset, voffset, hoffset + s.width, voffset + s.height);
                hoffset += s.width;
            } else {
                box.size(hoffset, voffset, hoffset + s.width, voffset + s.height);
                voffset += s.height;
            }
        }
    }
    Size get_best_size() const override {
        int b_x = 0, b_y = 0;
        for (const auto& box : boxes) {
            Size s = box.get_best_size();
            if (orientation == horizontal) {
                b_x += s.width;
                b_y = std::max(s.height, b_y);
            } else {
                b_x = std::max(s.width, b_x);
                b_y += s.height;
            }
        }
        return Size(b_x, b_y);
    }
    int fdiCount() const override{
        int n = 0;
        for (const auto& box : boxes) {
            n += box.fdiCount();
        }
        return n;
    }
    void makeItem(Dialog *dlg, int& no) override{
        for (auto& box : boxes) {
            box.makeItem(dlg, no);
        }
    }
    void show(int indent) override {
        for (auto& box : boxes) {
            box.show(indent + 4);
        }
    }
    virtual void write(Screen& scr) override {
        for (auto& box : boxes) {
            box.write(scr);
        }
    }
};

class DlgHSizer : public Sizer {
public:
    DlgHSizer(const std::vector<Window*>& controls, Border b = Border(0, 0, 1, 0))
        : Sizer(horizontal, controls, b) {}
};

class DlgVSizer : public Sizer {
public:
    DlgVSizer(const std::vector<Window*>& controls, Border b = Border())
        : Sizer(vertical, controls, b) {}
};

}
