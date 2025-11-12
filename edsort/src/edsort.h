#include "farapi.h"
#include "fardialog.h"

class EdSort
{
public:
	EdSort();
	void Run();

private:
	void SortLines();
	LONG_PTR WINAPI dlg_proc(HANDLE dlg, int Msg, int Param1, LONG_PTR Param2);

	size_t column;
	bool reverse;
	
	fardialog::Dialog *myDialog;
};
