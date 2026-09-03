#include <farplug-wide.h>

// Plugin initialization code

#ifdef __cplusplus
#define EXTERN_C_BEGIN extern "C" {
#define EXTERN_C_END                                                                                           \
	}                                                                                                          \
	;
#endif
#if 0
#if defined(__GNUC__)
EXTERN_C_BEGIN
	BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
EXTERN_C_END

BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
	(void) hDll;
	(void) dwReason;
	(void) lpReserved;
	return TRUE;
}
#endif
#endif

// **************** General ****************

#if FARMANAGERVERSION_MAJOR >= 2
#define FAR2
#define WIDE
#endif
#if FARMANAGERVERSION_MAJOR >= 3
#define FAR3
#endif

#ifdef FAR3
#define INITSIZE(x) (x).StructSize = sizeof(x)
#else
#define INITSIZE(x)
#endif

#ifdef WIDE
#define WIDE_SUFFIX(x) x##W
#else
#define WIDE_SUFFIX(x) x
#endif

// ********** Exported plugin API **********

// GetGlobalInfo

#ifdef FAR3
#define ADD_GETGLOBALINFO                                                                                      \
	SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)                                           \
	{                                                                                                          \
		INITSIZE(*Info);                                                                                       \
		Info->MinFarVersion = FARMANAGERVERSION;                                                               \
		Info->Version = PLUGIN_VERSION;                                                                        \
		Info->Guid = MainGuid;                                                                                 \
		Info->Title = PLUGIN_NAME;                                                                             \
		Info->Description = PLUGIN_DESC;                                                                       \
		Info->Author = PLUGIN_AUTHOR;                                                                          \
	}
#else
#define ADD_GETGLOBALINFO
#endif

// GetPluginInfo

#ifdef FAR3
#define PluginMenuStringsNumber PluginMenu.Count
#endif

// Open / OpenPlugin

#ifdef FAR3
#define OPENPLUGIN SHAREDSYMBOL HANDLE WINAPI OpenW(const struct OpenInfo *OInfo)
#else
#define OPENPLUGIN SHAREDSYMBOL HANDLE WINAPI WIDE_SUFFIX(OpenPlugin)(int OpenFrom, INT_PTR Item)
#endif

// ProcessEditorInput

#ifdef FAR3
#define PROCESSEDITORINPUT                                                                                     \
	SHAREDSYMBOL intptr_t WINAPI ProcessEditorInputW(const struct ProcessEditorInputInfo *_Info)
#define PROCESSEDITORINPUT_REC (&(_Info->Rec))
#else
#define PROCESSEDITORINPUT     SHAREDSYMBOL int WINAPI WIDE_SUFFIX(ProcessEditorInput)(const INPUT_RECORD *_Rec)
#define PROCESSEDITORINPUT_REC _Rec
#endif

// ********** Far service API **********

#ifdef FAR3
#define EditorControl(cmd, str) EditorControl(-1, cmd, 0, str)
#endif
