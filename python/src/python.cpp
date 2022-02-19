#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include <dlfcn.h>

#if !defined(__APPLE__) && !defined(__FreeBSD__)
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

// #define PYPLUGIN_DEBUGLOG "/tmp/far2.py.log"
// #define PYPLUGIN_DEBUGLOG "" /* to stderr */
// #define PYPLUGIN_THREADED
// #define PYPLUGIN_MEASURE_STARTUP

static void python_log(const char *function, unsigned int line, const char *format, ...)
{
#ifdef PYPLUGIN_DEBUGLOG
    va_list args;
    char *xformat = (char *)alloca(strlen(format) + strlen(function) + 64);
    sprintf(xformat, "[PYTHON %lu]: %s@%u%s%s",
        (unsigned long)GetProcessUptimeMSec(), function, line, (*format != '\n') ? " - " : "", format);

    FILE *stream = nullptr;
    if (PYPLUGIN_DEBUGLOG[0]) {
        stream = fopen(PYPLUGIN_DEBUGLOG, "at");
    }
    if (!stream) {
        stream = stderr;
    }

    va_start(args, format);
    vfprintf(stream, xformat, args);
    va_end(args);

    if (stream != stderr) {
        fclose(stream);
    }
#endif
}

#define PYTHON_LOG(args...)  python_log(__FUNCTION__, __LINE__, args)

#define PYTHON_VOID() \
    if (pyresult != NULL) { \
        Py_DECREF(pyresult); \
    }

#define PYTHON_HANDLE(default) \
    if (pyresult != NULL) { \
        if( PyLong_Check(pyresult) ) { \
            result = (HANDLE)PyLong_AsLong(pyresult); \
            if (PyErr_Occurred()) { \
                PyErr_Print(); \
                result = default; \
            } \
        } \
        Py_DECREF(pyresult); \
    }

#define PYTHON_INT(default) \
    if (pyresult != NULL) { \
        if( PyLong_Check(pyresult) ) { \
            result = PyLong_AsLong(pyresult); \
            if (PyErr_Occurred()) { \
                PyErr_Print(); \
                result = default; \
            } \
        } \
        Py_DECREF(pyresult); \
    }



#ifdef PYPLUGIN_THREADED
class PythonHolder : Threaded
#else
class PythonHolder
#endif
{
    std::string pluginPath;
    void *soPythonInterpreter = nullptr;
    PyObject *pyPluginModule = nullptr;
    PyObject *pyPluginManager = nullptr;


protected:
    virtual void *ThreadProc()
    {
        std::string syspath = "import sys";
        syspath += "\nsys.path.insert(1, '" + pluginPath + "')";
        PYTHON_LOG("syspath=%s\n", syspath.c_str());

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

        pyPluginManager = PyObject_GetAttrString(pyPluginModule, "pluginmanager");
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

        std::wstring progname;
        StrMB2Wide(pluginPath, progname);
        progname+= L"/python/bin/python";

        soPythonInterpreter = dlopen(PYTHON_LIBRARY, RTLD_NOW | RTLD_GLOBAL);
        if( !soPythonInterpreter ){
            PYTHON_LOG("error %u from dlopen('%s')\n", errno, PYTHON_LIBRARY);
            return;
        }
        Py_SetProgramName((wchar_t *)progname.c_str());
        Py_Initialize();
        PyEval_InitThreads();
        TranslateInstallPath_Lib2Share(pluginPath);
#ifdef PYPLUGIN_THREADED
        if (!StartThread()) {
            PYTHON_LOG("StartThread failed, fallback to synchronous initialization\n");
            ThreadProc();
        }
#else
        ThreadProc();
#endif
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

        Py_Finalize();

        if( soPythonInterpreter != NULL ) {
            dlclose(soPythonInterpreter);
            soPythonInterpreter = NULL;
        }
    }

    PyObject *vcall(const char *func, int n, ...)
    {
#ifdef PYPLUGIN_THREADED
        WaitThread();
#endif

        PyObject *pFunc;
        PyObject *result = NULL;

        if (pyPluginManager == NULL) {
            return result;
        }

        // Acquire the GIL
        PyGILState_STATE gstate = PyGILState_Ensure();

        pFunc = PyObject_GetAttrString(pyPluginManager, func);
        if (pFunc == NULL) {
            goto eof;
        }

        if (pFunc && PyCallable_Check(pFunc)) {
            PyObject *pArgs = PyTuple_New(n);

            va_list args;
            va_start(args, n);
            for (int i = 0; i < n; ++i) {
                PyObject *pValue = PyLong_FromLong((long int)va_arg(args, void *));
                PyTuple_SetItem(pArgs, i, pValue);
            }
            va_end(args);

            result = PyObject_CallObject(pFunc, pArgs);
            Py_DECREF(pArgs);
            if (result == NULL) {
                PyErr_Print();
                goto eofr;
            }

        } else {
            if (PyErr_Occurred())
                PyErr_Print();
        }

eofr:
        Py_XDECREF(pFunc);
eof:
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
    PyObject *pyresult = g_python_holder->vcall("SetStartupInfo", 1, &::Info);
    PYTHON_VOID()
}

XPORT(void, GetPluginInfo)(struct PluginInfo *Info)
{
    PYTHON_LOG("\n");
    Info->StructSize = sizeof(*Info);
    Info->Flags = PF_DISABLEPANELS;
    Info->DiskMenuStringsNumber = 0;
    Info->PluginMenuStringsNumber = 0;
    Info->PluginConfigStringsNumber = 0;
    PyObject *pyresult = g_python_holder->vcall("GetPluginInfo", 1, Info);
    PYTHON_VOID()

    // enforce plugin preopening to be able to initialize python asynchronously
    Info->Flags|= PF_PREOPEN;
}

XPORT(HANDLE, OpenPlugin)(int OpenFrom,INT_PTR Item)
{
    PYTHON_LOG("OpenFrom=%d Item=%d\n", OpenFrom, Item);
    HANDLE result = INVALID_HANDLE_VALUE;
    PyObject *pyresult = g_python_holder->vcall("OpenPlugin", 2, OpenFrom, Item);
    PYTHON_HANDLE(INVALID_HANDLE_VALUE)
    return result;
}

XPORT(void, ClosePlugin)(HANDLE hPlugin) {
    PYTHON_LOG("\n");
    PyObject *pyresult = g_python_holder->vcall("ClosePlugin", 1, hPlugin);
    PYTHON_VOID()
}

XPORT(int, Compare)(HANDLE hPlugin,const PluginPanelItem *Item1,const PluginPanelItem *Item2,unsigned int Mode) {
    PYTHON_LOG("Mode=%d\n", Mode);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("Compare", 4, hPlugin, Item1, Item2, Mode);
    PYTHON_INT(0)
    return result;
}
XPORT(int, Configure)(int ItemNumber) {
    PYTHON_LOG("ItemNumber=%d\n", ItemNumber);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("Configure", 1, ItemNumber);
    PYTHON_INT(0)
    return result;
}
XPORT(int, DeleteFiles)(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode) {
    PYTHON_LOG("ItemsNumber=%d OpMode=%d\n", ItemsNumber, OpMode);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("DeleteFiles", 4, hPlugin, PanelItem, ItemsNumber, OpMode);
    PYTHON_INT(0)
    return result;
}
XPORT(void, ExitFAR)(void) {
    PYTHON_LOG("\n");
    PyObject *pyresult = g_python_holder->vcall("ExitFAR", 0);
    PYTHON_VOID()
    delete g_python_holder;
    g_python_holder = nullptr;
}
XPORT(void, FreeFindData)(HANDLE hPlugin,PluginPanelItem *PanelItem,int ItemsNumber) {
    PYTHON_LOG("ItemsNumber=%d\n", ItemsNumber);
    PyObject *pyresult = g_python_holder->vcall("FreeFindData", 3, hPlugin, PanelItem, ItemsNumber);
    PYTHON_VOID()
}
XPORT(void, FreeVirtualFindData)(HANDLE hPlugin,PluginPanelItem *PanelItem,int ItemsNumber) {
    PYTHON_LOG("ItemsNumber=%d\n", ItemsNumber);
    PyObject *pyresult = g_python_holder->vcall("FreeVirtualFindData", 3, hPlugin, PanelItem, ItemsNumber);
    PYTHON_VOID()
}
XPORT(int, GetFiles)(HANDLE hPlugin,PluginPanelItem *PanelItem,int ItemsNumber,int Move,const wchar_t **DestPath,int OpMode) {
    PYTHON_LOG("ItemsNumber=%d Move=%d OpMode=%d\n", ItemsNumber, Move, OpMode);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("GetFiles", 6, hPlugin, PanelItem, ItemsNumber, Move, DestPath, OpMode);
    PYTHON_INT(0)
    return result;
}
XPORT(int, GetFindData)(HANDLE hPlugin,PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode) {
    PYTHON_LOG("OpMode=%d\n", OpMode);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("GetFindData", 4, hPlugin, pPanelItem, pItemsNumber, OpMode);
    PYTHON_INT(0)
    return result;
}
XPORT(void, GetOpenPluginInfo)(HANDLE hPlugin, OpenPluginInfo *Info) {
    PYTHON_LOG("\n");
    memset(Info, 0, sizeof(*Info));
    Info->StructSize = sizeof(*Info);
    PyObject *pyresult = g_python_holder->vcall("GetOpenPluginInfo", 2, hPlugin, Info);
    PYTHON_VOID()
}
XPORT(int, GetVirtualFindData)(HANDLE hPlugin, PluginPanelItem **pPanelItem, int *pItemsNumber, const wchar_t *Path) {
    PYTHON_LOG("\n");
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("GetVirtualFindData", 4, hPlugin, pPanelItem, pItemsNumber, Path);
    PYTHON_INT(0)
    return result;
}
XPORT(int, MakeDirectory)(HANDLE hPlugin,const wchar_t **Name,int OpMode) {
    PYTHON_LOG("OpMode=%d\n", OpMode);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("MakeDirectory", 3, hPlugin, Name, OpMode);
    PYTHON_INT(0)
    return result;
}
XPORT(HANDLE, OpenFilePlugin)(const wchar_t *Name,const unsigned char *Data,int DataSize,int OpMode) {
    PYTHON_LOG("OpMode=%d Name='%ls'\n", OpMode, Name);
    HANDLE result = INVALID_HANDLE_VALUE;
    PyObject *pyresult = g_python_holder->vcall("OpenFilePlugin", 4, Name, Data, DataSize, OpMode);
    PYTHON_HANDLE(INVALID_HANDLE_VALUE)
    return result;
}
XPORT(int, ProcessDialogEvent)(int Event,void *Param) {
    PYTHON_LOG("Event=%d\n", Event);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("ProcessDialogEvent", 2, Event, Param);
    PYTHON_INT(0)
    return result;
}
XPORT(int, ProcessEditorEvent)(int Event,void *Param) {
    PYTHON_LOG("Event=%d\n", Event);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("ProcessEditorEvent", 2, Event, Param);
    PYTHON_INT(0)
    return result;
}
XPORT(int, ProcessEditorInput)(const INPUT_RECORD *Rec) {
    PYTHON_LOG("\n");
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("ProcessEditorInput", 1, Rec);
    PYTHON_INT(0)
    return result;
}
XPORT(int, ProcessEvent)(HANDLE hPlugin,int Event,void *Param) {
    PYTHON_LOG("Event=%d\n", Event);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("ProcessEvent", 3, hPlugin, Event, Param);
    PYTHON_INT(0)
    return result;
}
XPORT(int, ProcessHostFile)(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode) {
    PYTHON_LOG("ItemsNumber=%d OpMode=%d\n", ItemsNumber, OpMode);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("ProcessHostFile", 4, hPlugin, PanelItem, ItemsNumber, OpMode);
    PYTHON_INT(0)
    return result;
}
XPORT(int, ProcessKey)(HANDLE hPlugin,int Key,unsigned int ControlState) {
    PYTHON_LOG("Key=%d ControlState=0x%x\n", Key, ControlState);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("ProcessKey", 3, hPlugin, Key, ControlState);
    PYTHON_INT(0)
    return result;
}
XPORT(int, ProcessSynchroEvent)(int Event,void *Param) {
    PYTHON_LOG("Event=%d\n", Event);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("ProcessSynchroEvent", 2, Event, Param);
    PYTHON_INT(0)
    return result;
}
XPORT(int, ProcessViewerEvent)(int Event,void *Param) {
    PYTHON_LOG("Event=%d\n", Event);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("ProcessViewerEvent", 2, Event, Param);
    PYTHON_INT(0)
    return result;
}
XPORT(int, PutFiles)(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int Move,const wchar_t *SrcPath, int OpMode) {
    PYTHON_LOG("ItemsNumber=%d Move=%d OpMode=%d SrcPath='%ls'\n", ItemsNumber, Move, OpMode, SrcPath);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("PutFiles", 6, hPlugin, PanelItem, ItemsNumber, Move, SrcPath, OpMode);
    PYTHON_INT(0)
    return result;
}
XPORT(int, SetDirectory)(HANDLE hPlugin,const wchar_t *Dir,int OpMode) {
    PYTHON_LOG("OpMode=%d Dir='%ls'\n", OpMode, Dir);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("SetDirectory", 3, hPlugin, Dir, OpMode);
    PYTHON_INT(0)
    return result;
}
XPORT(int, SetFindList)(HANDLE hPlugin,const PluginPanelItem *PanelItem,int ItemsNumber) {
    PYTHON_LOG("ItemsNumber=%d\n", ItemsNumber);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("SetFindList", 3, hPlugin, PanelItem, ItemsNumber);
    PYTHON_INT(0)
    return result;
}
