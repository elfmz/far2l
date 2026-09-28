#include "fardialog.h"

namespace fardialog {

const Border dborder = Border(2, 1, 2, 1);

void Dialog::buildFDI(Sizer *contents){
	this->contents = contents;
	DlgVSizer vbox1({contents}, dborder);
	Size size = vbox1.get_best_size();
	size.width = std::max(size.width + 2, (int)wcslen(title) + 2);
	vbox1.size(3, 1, size.width, size.height);
	width = size.width;
	height = size.height;
	int fdiCount = contents->fdiCount();
	CreateFDI(fdiCount);
	fdi[0] = {
		DI_DOUBLEBOX,
		3, 1, width, height,
		0,
		{},
		0,
		0,
		title,
		0
	};
	int no = 1;
	contents->makeItem(this, no);
}

HANDLE Dialog::DialogInit() {
	hDlg = _PSI.DialogInit(
		_PSI.ModuleNumber,
		-1, -1, width + dborder.left + dborder.right, height + dborder.top + dborder.bottom,
		helptopic,
		fdi.data(), fdi.size(),
		0, // Reserved
		flags,
		cb,
		param);
	return hDlg;
}

void Dialog::show(){
	std::cout << "Showing dialog:" << std::endl;
	std::cout << "width:" << width << std::endl;
	std::cout << "height:" << height << std::endl;
	std::cout << "title:"; std::wcout << title << std::endl;
	std::cout << "helptopic:"; std::wcout << helptopic << std::endl;
	std::cout << "flags:" << flags << std::endl;
	Screen scr(width, height);
	contents->write(scr);
	scr.show();
}

}
