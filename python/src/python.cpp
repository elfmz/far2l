#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <dlfcn.h>

#include <Python.h>

#include "plugin.hpp"
#include "farcolor.hpp"
#include "farkeys.hpp"
#include "../../etc/plugs.h"

#include "python.hpp"
#include <Threaded.h>
#include <utils.h>

PyMODINIT_FUNC PyInit__pyfar(void);

#define PLUGIN_DEBUG 0
#if PLUGIN_DEBUG
void flog(const wchar_t *format, ...) {
   FILE *stream = fopen("/tmp/far2.py.log", "at");
   va_list args;

   va_start(args, format);
   vfwprintf(stream, format, args);
   va_end(args);
   fclose(stream);
}
#else
#define flog(...)
#endif


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



class PythonHolder : Threaded
{
    std::string gPluginPath;
    void *gPython_Interpreter = nullptr;
    PyObject *pyPluginModule = nullptr;
    PyObject *pyPluginManager = nullptr;


protected:
    virtual void *ThreadProc()
    {
        PyInit__pyfar();

        std::string syspath = "import sys";
        syspath += "\nsys.path.insert(1, '" + gPluginPath + "')";
        flog(_T("PyRun_SimpleString=%s\n"), syspath.c_str());
        PyRun_SimpleString(syspath.c_str());

        PyObject *pName;
        pName = PyUnicode_DecodeFSDefault("far2l");
        pyPluginModule = PyImport_Import(pName);
        Py_DECREF(pName);

        if (pyPluginModule == NULL) {
            PyErr_Print();
            flog(_T("Failed to load \"far2l\"\n"));
            return nullptr;
        }

        pyPluginManager = PyObject_GetAttrString(pyPluginModule, "pluginmanager");
        if (pyPluginManager == NULL) {
            flog(_T("Failed to load \"far2l.pluginmanager\"\n"));
            Py_DECREF(pyPluginModule);
            pyPluginModule = NULL;
            return nullptr;
        }

        return nullptr;
    }

public:
    PythonHolder(const char *PluginPath)
		:gPluginPath(PluginPath)
    {
        size_t pos = gPluginPath.rfind(L'/');
        if (pos != std::string::npos) {
            gPluginPath.resize(pos);
        }

        std::wstring progname;
        StrMB2Wide(gPluginPath, progname);
        progname+= L"/python/bin/python";

        gPython_Interpreter = dlopen(PYTHON_LIBRARY, RTLD_NOW | RTLD_GLOBAL);
        if( !gPython_Interpreter ){
            flog(_T("gPython_Interpreter=NULL\n"));
            return;
        }
        Py_SetProgramName((wchar_t *)progname.c_str());
        Py_Initialize();
        PyEval_InitThreads();
        if (!StartThread()) {
            // fallback to synchronous initialization
            ThreadProc();
        }
    }

    virtual ~PythonHolder()
    {
        WaitThread();

        if (pyPluginManager) {
            Py_XDECREF(pyPluginManager);
            pyPluginManager = nullptr;
        }

        if (pyPluginModule) {
            Py_XDECREF(pyPluginModule);
            pyPluginModule = nullptr;
        }

        Py_Finalize();

        if( gPython_Interpreter != NULL ) {
            dlclose(gPython_Interpreter);
            gPython_Interpreter = NULL;
        }
    }

    PyObject *vcall(const char *func, int n, ...)
    {
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

extern "C"
SHAREDSYMBOL void WINPORT_DllStartup(const char *path)
{
    g_python_holder = new PythonHolder(path);;
}

SHAREDSYMBOL int WINAPI EXP_NAME(GetMinFarVersion)()
{
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    return FARMANAGERVERSION;
}

#define XPORT(type, name) \
SHAREDSYMBOL type WINAPI EXP_NAME(name)

XPORT(void, SetStartupInfo)(const struct PluginStartupInfo *Info)
{
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    ::Info=*Info;
    ::FSF=*Info->FSF;
    ::Info.FSF=&::FSF;
    PyObject *pyresult = g_python_holder->vcall("SetStartupInfo", 1, &::Info);
       PYTHON_VOID()
}

XPORT(void, GetPluginInfo)(struct PluginInfo *Info)
{
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
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
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    HANDLE result = INVALID_HANDLE_VALUE;
    PyObject *pyresult = g_python_holder->vcall("OpenPlugin", 2, OpenFrom, Item);
    PYTHON_HANDLE(INVALID_HANDLE_VALUE)
    return result;
}

XPORT(void, ClosePlugin)(HANDLE hPlugin) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    PyObject *pyresult = g_python_holder->vcall("ClosePlugin", 1, hPlugin);
    PYTHON_VOID()
}

XPORT(int, Compare)(HANDLE hPlugin,const PluginPanelItem *Item1,const PluginPanelItem *Item2,unsigned int Mode) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("Compare", 4, hPlugin, Item1, Item2, Mode);
    PYTHON_INT(0)
    return result;
}
XPORT(int, Configure)(int ItemNumber) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("Configure", 1, ItemNumber);
    PYTHON_INT(0)
    return result;
}
XPORT(int, DeleteFiles)(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("DeleteFiles", 4, hPlugin, PanelItem, ItemsNumber, OpMode);
    PYTHON_INT(0)
    return result;
}
XPORT(void, ExitFAR)(void) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    PyObject *pyresult = g_python_holder->vcall("ExitFAR", 0);
    PYTHON_VOID()
    delete g_python_holder;
    g_python_holder = nullptr;
}
XPORT(void, FreeFindData)(HANDLE hPlugin,PluginPanelItem *PanelItem,int ItemsNumber) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    PyObject *pyresult = g_python_holder->vcall("FreeFindData", 3, hPlugin, PanelItem, ItemsNumber);
    PYTHON_VOID()
}
XPORT(void, FreeVirtualFindData)(HANDLE hPlugin,PluginPanelItem *PanelItem,int ItemsNumber) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    PyObject *pyresult = g_python_holder->vcall("FreeVirtualFindData", 3, hPlugin, PanelItem, ItemsNumber);
    PYTHON_VOID()
}
XPORT(int, GetFiles)(HANDLE hPlugin,PluginPanelItem *PanelItem,int ItemsNumber,int Move,const wchar_t **DestPath,int OpMode) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("GetFiles", 6, hPlugin, PanelItem, ItemsNumber, Move, DestPath, OpMode);
    PYTHON_INT(0)
    return result;
}
XPORT(int, GetFindData)(HANDLE hPlugin,PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("GetFindData", 4, hPlugin, pPanelItem, pItemsNumber, OpMode);
    PYTHON_INT(0)
    return result;
}
XPORT(void, GetOpenPluginInfo)(HANDLE hPlugin, OpenPluginInfo *Info) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    memset(Info, 0, sizeof(*Info));
    Info->StructSize = sizeof(*Info);
    PyObject *pyresult = g_python_holder->vcall("GetOpenPluginInfo", 2, hPlugin, Info);
    PYTHON_VOID()
}
XPORT(int, GetVirtualFindData)(HANDLE hPlugin, PluginPanelItem **pPanelItem, int *pItemsNumber, const wchar_t *Path) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("GetVirtualFindData", 4, hPlugin, pPanelItem, pItemsNumber, Path);
    PYTHON_INT(0)
    return result;
}
XPORT(int, MakeDirectory)(HANDLE hPlugin,const wchar_t **Name,int OpMode) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("MakeDirectory", 3, hPlugin, Name, OpMode);
    PYTHON_INT(0)
    return result;
}
XPORT(HANDLE, OpenFilePlugin)(const wchar_t *Name,const unsigned char *Data,int DataSize,int OpMode) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    HANDLE result = INVALID_HANDLE_VALUE;
    PyObject *pyresult = g_python_holder->vcall("OpenFilePlugin", 4, Name, Data, DataSize, OpMode);
    PYTHON_HANDLE(INVALID_HANDLE_VALUE)
    return result;
}
XPORT(int, ProcessDialogEvent)(int Event,void *Param) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("ProcessDialogEvent", 2, Event, Param);
    PYTHON_INT(0)
    return result;
}
XPORT(int, ProcessEditorEvent)(int Event,void *Param) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("ProcessEditorEvent", 2, Event, Param);
    PYTHON_INT(0)
    return result;
}
XPORT(int, ProcessEditorInput)(const INPUT_RECORD *Rec) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("ProcessEditorInput", 1, Rec);
    PYTHON_INT(0)
    return result;
}
XPORT(int, ProcessEvent)(HANDLE hPlugin,int Event,void *Param) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("ProcessEvent", 3, hPlugin, Event, Param);
    PYTHON_INT(0)
    return result;
}
XPORT(int, ProcessHostFile)(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("ProcessHostFile", 4, hPlugin, PanelItem, ItemsNumber, OpMode);
    PYTHON_INT(0)
    return result;
}
XPORT(int, ProcessKey)(HANDLE hPlugin,int Key,unsigned int ControlState) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("ProcessKey", 3, hPlugin, Key, ControlState);
    PYTHON_INT(0)
    return result;
}
XPORT(int, ProcessSynchroEvent)(int Event,void *Param) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("ProcessSynchroEvent", 2, Event, Param);
    PYTHON_INT(0)
    return result;
}
XPORT(int, ProcessViewerEvent)(int Event,void *Param) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("ProcessViewerEvent", 2, Event, Param);
    PYTHON_INT(0)
    return result;
}
XPORT(int, PutFiles)(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int Move,const wchar_t *SrcPath, int OpMode) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("PutFiles", 6, hPlugin, PanelItem, ItemsNumber, Move, SrcPath, OpMode);
    PYTHON_INT(0)
    return result;
}
XPORT(int, SetDirectory)(HANDLE hPlugin,const wchar_t *Dir,int OpMode) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("SetDirectory", 3, hPlugin, Dir, OpMode);
    PYTHON_INT(0)
    return result;
}
XPORT(int, SetFindList)(HANDLE hPlugin,const PluginPanelItem *PanelItem,int ItemsNumber) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = g_python_holder->vcall("SetFindList", 3, hPlugin, PanelItem, ItemsNumber);
    PYTHON_INT(0)
    return result;
}
