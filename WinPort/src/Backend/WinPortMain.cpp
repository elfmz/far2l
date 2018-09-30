#include "Backend.h"
#include "ConsoleOutput.h"
#include "ConsoleInput.h"
#include "WinPortHandle.h"
#include "PathHelpers.h"


ConsoleOutput g_winport_con_out;
ConsoleInput g_winport_con_in;

bool WinPortMainWX(int argc, char **argv, int(*AppMain)(int argc, char **argv), int *result);
extern "C" void WinPortInitRegistry();


extern "C" int WinPortMain(int argc, char **argv, int(*AppMain)(int argc, char **argv))
{
	WinPortInitRegistry();
	WinPortInitWellKnownEnv();
//      g_winport_con_out.WriteString(L"Hello", 5);

	int result = -1;
	if (!WinPortMainWX(argc, argv, AppMain, &result) ) {
		fprintf(stderr, "Cannot use WX backend\n");
	}

	WinPortHandle_FinalizeApp();
	return result;
}
