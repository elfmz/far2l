#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include <dlfcn.h>

#if !defined(__APPLE__) && !defined(__FreeBSD__) && !defined(__DragonFly__)
# include <alloca.h>
#endif

#include <Python.h>

#include <windows.h>
#include <Threaded.h>
#include <utils.h>

#include <farplug-wide.h>
#include <farcolor.h>
#include <farkeys.h>

static struct PluginStartupInfo Info;
static FARSTANDARDFUNCTIONS FSF;
extern "C" {
#undef WINPORT_DECL_DEF
#define WINPORT_DECL_DEF(NAME, RV, ARGS) typedef RV (*ptrWINPORT_##NAME) ARGS;
#include "WinPortDecl.h"
#undef WINPORT_DECL_DEF
#define WINPORT_DECL_DEF(NAME, RV, ARGS) ptrWINPORT_##NAME v##NAME;
typedef struct _WINPORTDECL{
#include "WinPortDecl.h"
} WINPORTDECL;
static WINPORTDECL winportvar = {
#undef WINPORT_DECL_DEF
#define WINPORT_DECL_DEF(NAME, RV, ARGS) &WINPORT_##NAME,
#include "WinPortDecl.h"
};
}

static void python_log(const char *function, unsigned int line, const char *format, ...)
{
    va_list args;
    char *xformat = (char *)alloca(strlen(format) + strlen(function) + 64);
    sprintf(xformat, "[PYTHON %lu]: %s@%u%s%s",
        (unsigned long)GetProcessUptimeMSec(), function, line, (*format != '\n') ? " - " : "", format);

    va_start(args, format);
    vfprintf(stderr, xformat, args);
    va_end(args);
}

#define PYTHON_LOG(args...)  python_log(__FUNCTION__, __LINE__, args)

static PyObject *
far2l_CheckForInput(PyObject *self, PyObject *args)
{
    DWORD dwTimeout = INFINITE;
    if (!PyArg_ParseTuple(args, "|k", &dwTimeout))
        return NULL;

    if (WINPORT(WaitConsoleInput)(NULL, dwTimeout))
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject *
far2l_CheckForEscape(PyObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    WORD EscCode = VK_ESCAPE;
    if (WINPORT(CheckForKeyPress)(NULL, &EscCode, 1, CFKP_KEEP_OTHER_EVENTS))
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject *
far2l_WINPORT(PyObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    PyObject *pValue = PyLong_FromLong((long int)&winportvar);
    return pValue;
}

static PyMethodDef Far2lcMethods[] = {
    {"CheckForInput", far2l_CheckForInput, METH_VARARGS, "CheckForInput"},
    {"CheckForEscape", far2l_CheckForEscape, METH_VARARGS, "CheckForEscape"},
    {"WINPORT", far2l_WINPORT, METH_VARARGS, "WINPORT"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef far2lcmodule = {
    PyModuleDef_HEAD_INIT,
    "far2lc",   /* name of module */
    NULL, /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    Far2lcMethods
};

PyMODINIT_FUNC
PyInit_far2lc(void)
{
    return PyModule_Create(&far2lcmodule);
}


#define FAIL_IF_STATUS_EXCEPTION(status)                                       \
  if (PyStatus_Exception(status)) {                                            \
PYTHON_LOG("status exception\n"); \
    goto done;                                                                 \
  }

static bool init_python()
{
    PyStatus status;

    PyConfig config;
    PyConfig_InitIsolatedConfig(&config);

    //config.utf8_mode = 1;
    config.user_site_directory = 1;
    config.install_signal_handlers = 1;

    /* Set the program name before reading the configuration
       (decode byte string from the locale encoding).

       Implicitly preinitialize Python. */

    /* Read all configuration at once */
    status = PyConfig_Read(&config);
    FAIL_IF_STATUS_EXCEPTION(status);

    PyImport_AppendInittab("far2lc", PyInit_far2lc);

    status = Py_InitializeFromConfig(&config);
    FAIL_IF_STATUS_EXCEPTION(status);

    PyConfig_Clear(&config);
    return true;

done:
    PyConfig_Clear(&config);
    return false;
}

#ifdef PYPLUGIN_THREADED
class PythonHolder : Threaded
#else
class PythonHolder
#endif
{
    bool initok;
    std::string pluginPath;
    void *soPythonInterpreter = nullptr;
    PyObject *pyPluginModule = nullptr;
    PyObject *pyPluginManager = nullptr;


protected:
    virtual void *ThreadProc()
    {
        std::string syspath = "import sys";
        syspath += "\nsys.path.insert(1, '" + pluginPath + "')";

PYTHON_LOG("initial sys.path=%s\n", syspath.c_str());
        PyRun_SimpleString(syspath.c_str());

        PyObject *pName;
        pName = PyUnicode_DecodeFSDefault("far2l");
        pyPluginModule = PyImport_Import(pName);
        Py_DECREF(pName);

        if (pyPluginModule == NULL) {
            PyErr_Print();
            PYTHON_LOG("Failed to load \"far2l\"\n");
            return nullptr;
        }

        pyPluginManager = PyObject_GetAttrString(pyPluginModule, "_pluginmanager");
        if (pyPluginManager == NULL) {
            PYTHON_LOG("Failed to load \"far2l.pluginmanager\"\n");
            Py_DECREF(pyPluginModule);
            pyPluginModule = NULL;
            return nullptr;
        }

        PYTHON_LOG("complete\n");
        return nullptr;
    }

public:
    PythonHolder(const char *PluginPath)
        : pluginPath(PluginPath)
    {
        size_t pos = pluginPath.rfind(L'/');
        if (pos != std::string::npos) {
            pluginPath.resize(pos);
        }

        PYTHON_LOG("pluginpath: %s, python library used:%s\n", pluginPath.c_str(), PYTHON_LIBRARY);
        soPythonInterpreter = dlopen(PYTHON_LIBRARY, RTLD_NOW | RTLD_GLOBAL);
        if( !soPythonInterpreter ){
            PYTHON_LOG("error %u from dlopen('%s')\n", errno, PYTHON_LIBRARY);
            initok = false;
            return;
        }

        initok = init_python();
        if( initok )
        {
#ifdef PYPLUGIN_THREADED
            if (!StartThread()) {
                PYTHON_LOG("StartThread failed, fallback to synchronous initialization\n");
                ThreadProc();
            }
#else
            ThreadProc();
#endif
        }

    }

    virtual ~PythonHolder()
    {
#ifdef PYPLUGIN_THREADED
        WaitThread();
#endif

        if (pyPluginManager) {
            Py_XDECREF(pyPluginManager);
            pyPluginManager = nullptr;
        }

        if (pyPluginModule) {
            Py_XDECREF(pyPluginModule);
            pyPluginModule = nullptr;
        }

        if( initok )
            Py_Finalize();

        if( soPythonInterpreter != NULL ) {
            dlclose(soPythonInterpreter);
            soPythonInterpreter = NULL;
        }
    }

    PyObject *pycall(const char *func, int n, va_list args){
        PyObject *pyresult = NULL;
        PyObject *pFunc = PyObject_GetAttrString(pyPluginManager, func);
        if (pFunc == NULL) {
            return pyresult;
        }

        if (pFunc && PyCallable_Check(pFunc)) {
            PyObject *pArgs = PyTuple_New(n);

            for (int i = 0; i < n; ++i) {
                PyObject *pValue = PyLong_FromLong((long int)va_arg(args, void *));
                PyTuple_SetItem(pArgs, i, pValue);
            }

            pyresult = PyObject_CallObject(pFunc, pArgs);
            Py_DECREF(pArgs);
        }
        Py_XDECREF(pFunc);
        return pyresult;
    }

    void vcall(const char *func, int n, ...)
    {
#ifdef PYPLUGIN_THREADED
        WaitThread();
#endif
        if (pyPluginManager == NULL) {
            return;
        }

        // Acquire the GIL
        PyGILState_STATE gstate = PyGILState_Ensure();

        va_list args;
        va_start(args, n);
        PyObject *pyresult = this->pycall(func, n, args);
        va_end(args);
        if (pyresult == NULL || PyErr_Occurred()) {
            PyErr_Print();
            PYTHON_LOG("Failed to call \"%s\"\n", func);
        }
        Py_DECREF(pyresult);

        // Release the GIL. No Python API allowed beyond this point.
        PyGILState_Release(gstate);
    }

    int icall(int defresult, const char *func, int n, ...)
    {
#ifdef PYPLUGIN_THREADED
        WaitThread();
#endif
        if (pyPluginManager == NULL) {
            return defresult;
        }

        // Acquire the GIL
        PyGILState_STATE gstate = PyGILState_Ensure();

        int result = defresult;
        va_list args;
        va_start(args, n);
        PyObject *pyresult = this->pycall(func, n, args);
        va_end(args);
        if( pyresult != NULL && PyLong_Check(pyresult) )
            result = PyLong_AsLong(pyresult);
        if (pyresult == NULL || PyErr_Occurred()) {
            PyErr_Print();
            PYTHON_LOG("Failed to call \"%s\"\n", func);
            result = defresult;
        }
        Py_DECREF(pyresult);

        // Release the GIL. No Python API allowed beyond this point.
        PyGILState_Release(gstate);
        return result;
    }

    HANDLE hcall(HANDLE defresult, const char *func, int n, ...)
    {
#ifdef PYPLUGIN_THREADED
        WaitThread();
#endif
        if (pyPluginManager == NULL) {
            return defresult;
        }

        // Acquire the GIL
        PyGILState_STATE gstate = PyGILState_Ensure();

        HANDLE result = defresult;
        va_list args;
        va_start(args, n);
        PyObject *pyresult = this->pycall(func, n, args);
        va_end(args);
        if( pyresult != NULL && PyLong_Check(pyresult) )
            result = (HANDLE)PyLong_AsLong(pyresult);
        if (pyresult == NULL || PyErr_Occurred()) {
            PyErr_Print();
            PYTHON_LOG("Failed to call \"%s\"\n", func);
            result = defresult;
        }
        Py_DECREF(pyresult);

        // Release the GIL. No Python API allowed beyond this point.
        PyGILState_Release(gstate);
        return result;
    }

} *g_python_holder = nullptr;

#include <time.h>
extern "C"
SHAREDSYMBOL void PluginModuleOpen(const char *path)
{
#if defined(PYPLUGIN_MEASURE_STARTUP) && defined(PYPLUGIN_DEBUGLOG)
    clock_t t = clock();
    g_python_holder = new PythonHolder(path);
    t = clock() - t;
    double cpu_time_used = ((double) t) / CLOCKS_PER_SEC;
    PYTHON_LOG("startup time=%f\n", cpu_time_used);
#else
    g_python_holder = new PythonHolder(path);
#endif
}

SHAREDSYMBOL int WINAPI EXP_NAME(GetMinFarVersion)()
{
    PYTHON_LOG("\n");
    return FARMANAGERVERSION;
}

#define XPORT(type, name) \
SHAREDSYMBOL type WINAPI EXP_NAME(name)

XPORT(void, SetStartupInfo)(const struct PluginStartupInfo *Info)
{
    PYTHON_LOG("\n");
    ::Info=*Info;
    ::FSF=*Info->FSF;
    ::Info.FSF=&::FSF;
    g_python_holder->vcall("SetStartupInfo", 1, &::Info);
}

XPORT(void, GetPluginInfo)(struct PluginInfo *Info)
{
    PYTHON_LOG("\n");
    Info->StructSize = sizeof(*Info);
    Info->Flags = PF_DISABLEPANELS;
    Info->DiskMenuStringsNumber = 0;
    Info->PluginMenuStringsNumber = 0;
    Info->PluginConfigStringsNumber = 0;
    g_python_holder->vcall("GetPluginInfo", 1, Info);

    // enforce plugin preopening to be able to initialize python asynchronously
    Info->Flags|= PF_PREOPEN;
}

XPORT(HANDLE, OpenPlugin)(int OpenFrom,INT_PTR Item)
{
    PYTHON_LOG("OpenFrom=%d Item=%d\n", OpenFrom, Item);
    return g_python_holder->hcall(INVALID_HANDLE_VALUE, "OpenPlugin", 2, OpenFrom, Item);
}

XPORT(void, ClosePlugin)(HANDLE hPlugin) {
    PYTHON_LOG("\n");
    g_python_holder->vcall("ClosePlugin", 1, hPlugin);
}

XPORT(int, Compare)(HANDLE hPlugin,const PluginPanelItem *Item1,const PluginPanelItem *Item2,unsigned int Mode) {
    PYTHON_LOG("Mode=%d\n", Mode);
    return g_python_holder->icall(0, "Compare", 4, hPlugin, Item1, Item2, Mode);
}
XPORT(int, Configure)(int ItemNumber) {
    PYTHON_LOG("ItemNumber=%d\n", ItemNumber);
    return g_python_holder->icall(0, "Configure", 1, ItemNumber);
}
XPORT(int, DeleteFiles)(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode) {
    PYTHON_LOG("ItemsNumber=%d OpMode=%d\n", ItemsNumber, OpMode);
    return g_python_holder->icall(0, "DeleteFiles", 4, hPlugin, PanelItem, ItemsNumber, OpMode);
}
XPORT(void, ExitFAR)(void) {
    PYTHON_LOG("\n");
    g_python_holder->vcall("ExitFAR", 0);
    delete g_python_holder;
    g_python_holder = nullptr;
}
XPORT(void, FreeFindData)(HANDLE hPlugin,PluginPanelItem *PanelItem,int ItemsNumber) {
    PYTHON_LOG("ItemsNumber=%d\n", ItemsNumber);
    g_python_holder->vcall("FreeFindData", 3, hPlugin, PanelItem, ItemsNumber);
}
XPORT(void, FreeVirtualFindData)(HANDLE hPlugin,PluginPanelItem *PanelItem,int ItemsNumber) {
    PYTHON_LOG("ItemsNumber=%d\n", ItemsNumber);
    g_python_holder->vcall("FreeVirtualFindData", 3, hPlugin, PanelItem, ItemsNumber);
}
XPORT(int, GetFiles)(HANDLE hPlugin,PluginPanelItem *PanelItem,int ItemsNumber,int Move,const wchar_t **DestPath,int OpMode) {
    PYTHON_LOG("ItemsNumber=%d Move=%d OpMode=%d\n", ItemsNumber, Move, OpMode);
    return g_python_holder->icall(0, "GetFiles", 6, hPlugin, PanelItem, ItemsNumber, Move, DestPath, OpMode);
}
XPORT(int, GetFindData)(HANDLE hPlugin,PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode) {
    PYTHON_LOG("OpMode=%d\n", OpMode);
    return g_python_holder->icall(0, "GetFindData", 4, hPlugin, pPanelItem, pItemsNumber, OpMode);
}
XPORT(void, GetOpenPluginInfo)(HANDLE hPlugin, OpenPluginInfo *Info) {
    PYTHON_LOG("\n");
    memset(Info, 0, sizeof(*Info));
    Info->StructSize = sizeof(*Info);
    g_python_holder->icall(0, "GetOpenPluginInfo", 2, hPlugin, Info);
}
XPORT(int, GetVirtualFindData)(HANDLE hPlugin, PluginPanelItem **pPanelItem, int *pItemsNumber, const wchar_t *Path) {
    PYTHON_LOG("\n");
    return g_python_holder->icall(0, "GetVirtualFindData", 4, hPlugin, pPanelItem, pItemsNumber, Path);
}
XPORT(int, MakeDirectory)(HANDLE hPlugin,const wchar_t **Name,int OpMode) {
    PYTHON_LOG("OpMode=%d\n", OpMode);
    return g_python_holder->icall(0, "MakeDirectory", 3, hPlugin, Name, OpMode);
}
XPORT(HANDLE, OpenFilePlugin)(const wchar_t *Name,const unsigned char *Data,int DataSize,int OpMode) {
    PYTHON_LOG("OpMode=%d Name='%ls'\n", OpMode, Name);
    return g_python_holder->hcall(INVALID_HANDLE_VALUE, "OpenFilePlugin", 4, Name, Data, DataSize, OpMode);
}
XPORT(int, ProcessDialogEvent)(int Event,void *Param) {
    PYTHON_LOG("Event=%d\n", Event);
    return g_python_holder->icall(0, "ProcessDialogEvent", 2, Event, Param);
}
XPORT(int, ProcessEditorEvent)(int Event,void *Param) {
    PYTHON_LOG("Event=%d\n", Event);
    return g_python_holder->icall(0, "ProcessEditorEvent", 2, Event, Param);
}
XPORT(int, ProcessEditorInput)(const INPUT_RECORD *Rec) {
    PYTHON_LOG("keydown=%d vkey=%08X cstate=%08X ra=%d la=%d rc=%d lc=%d sh=%d\n",
        Rec->Event.KeyEvent.bKeyDown,
        Rec->Event.KeyEvent.wVirtualKeyCode,
        Rec->Event.KeyEvent.dwControlKeyState,
        (Rec->Event.KeyEvent.dwControlKeyState & RIGHT_ALT_PRESSED) != 0,
        (Rec->Event.KeyEvent.dwControlKeyState & LEFT_ALT_PRESSED) != 0,
        (Rec->Event.KeyEvent.dwControlKeyState & RIGHT_CTRL_PRESSED) != 0,
        (Rec->Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED) != 0,
        (Rec->Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED) != 0
    );
    return g_python_holder->icall(0, "ProcessEditorInput", 1, Rec);
}
XPORT(int, ProcessEvent)(HANDLE hPlugin,int Event,void *Param) {
    PYTHON_LOG("Event=%d\n", Event);
    return g_python_holder->icall(0, "ProcessEvent", 3, hPlugin, Event, Param);
}
XPORT(int, ProcessHostFile)(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode) {
    PYTHON_LOG("ItemsNumber=%d OpMode=%d\n", ItemsNumber, OpMode);
    return g_python_holder->icall(0, "ProcessHostFile", 4, hPlugin, PanelItem, ItemsNumber, OpMode);
}
XPORT(int, ProcessKey)(HANDLE hPlugin,int Key,unsigned int ControlState) {
    PYTHON_LOG("Key=%d ControlState=0x%x\n", Key, ControlState);
    return g_python_holder->icall(0, "ProcessKey", 3, hPlugin, Key, ControlState);
}
XPORT(int, ProcessSynchroEvent)(int Event,void *Param) {
    PYTHON_LOG("Event=%d\n", Event);
    return g_python_holder->icall(0, "ProcessSynchroEvent", 2, Event, Param);
}
XPORT(int, ProcessViewerEvent)(int Event,void *Param) {
    PYTHON_LOG("Event=%d\n", Event);
    return g_python_holder->icall(0, "ProcessViewerEvent", 2, Event, Param);
}
XPORT(int, PutFiles)(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int Move,const wchar_t *SrcPath, int OpMode) {
    PYTHON_LOG("ItemsNumber=%d Move=%d OpMode=%d SrcPath='%ls'\n", ItemsNumber, Move, OpMode, SrcPath);
    return g_python_holder->icall(0, "PutFiles", 6, hPlugin, PanelItem, ItemsNumber, Move, SrcPath, OpMode);
}
XPORT(int, SetDirectory)(HANDLE hPlugin,const wchar_t *Dir,int OpMode) {
    PYTHON_LOG("OpMode=%d Dir='%ls'\n", OpMode, Dir);
    return g_python_holder->icall(0, "SetDirectory", 3, hPlugin, Dir, OpMode);
}
XPORT(int, SetFindList)(HANDLE hPlugin,const PluginPanelItem *PanelItem,int ItemsNumber) {
    PYTHON_LOG("ItemsNumber=%d\n", ItemsNumber);
    return g_python_holder->icall(0, "SetFindList", 3, hPlugin, PanelItem, ItemsNumber);
}
XPORT(int, MayExitFAR)(void)
{
    PYTHON_LOG("\n");
    return g_python_holder->icall(1, "MayExitFAR", 0);
}

#if 0
XPORT(int, Analyse)(const AnalyseData *pData)
{
    PYTHON_LOG("\n");
    return (int)g_python_holder->vcall("Analyse", 1, pData);
}
XPORT(int, GetCustomData)(const wchar_t *FilePath, wchar_t **CustomData)
{
    PYTHON_LOG("FilePath=%ls\n", FilePath);
    return (int)g_python_holder->vcall("GetCustomData", 2, FilePath, CustomData);
}
XPORT(void, FreeCustomData)(wchar_t *CustomData)
{
    PYTHON_LOG("\n");
    g_python_holder->vcall("FreeCustomData", INVALID_HANDLE_VALUE, 1, CustomData);
}
#endif
