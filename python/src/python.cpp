#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <dlfcn.h>

#include "../../etc/plugs.h"

#include "python.hpp"

#include <Python.h>

std::wstring gPluginPath;
PyObject *pyPluginModule = NULL;
PyObject *pyPluginManager = NULL;
void *gPython_Interpreter = NULL;

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


extern "C"
SHAREDSYMBOL void WINPORT_DllStartup(const char *path)
{
    std::string str1 = path;
    std::string::size_type pos = str1.rfind("/");
    std::string str2 = str1.substr(0, pos);
    gPluginPath = std::wstring(str2.begin(), str2.end());
}

SHAREDSYMBOL int WINAPI EXP_NAME(GetMinFarVersion)()
{
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    return FARMANAGERVERSION;
}

static PyObject *vcall(const char *func, int n, ...)
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
    }
    else {
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

#define XPORT(type, name) \
SHAREDSYMBOL type WINAPI EXP_NAME(name)

XPORT(void, SetStartupInfo)(const struct PluginStartupInfo *Info)
{
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    ::Info=*Info;
    ::FSF=*Info->FSF;
    ::Info.FSF=&::FSF;
    std::wstring progname = gPluginPath + L"/python/bin/python";

    gPython_Interpreter = dlopen(PYTHON_LIBRARY, RTLD_NOW | RTLD_GLOBAL);
    if( gPython_Interpreter == NULL ){
        flog(_T("gPython_Interpreter=NULL\n"));
        return;
    }
    Py_SetProgramName((wchar_t *)progname.c_str());
    Py_Initialize();
    PyEval_InitThreads();

    std::string pp( gPluginPath.begin(), gPluginPath.end() );;
    std::string syspath = "import sys";
    syspath += "\nsys.path.insert(1, '" + pp + "')";
    syspath += "\nsys.path.insert(1, '~/.config/far2l/python')";
    flog(_T("PyRun_SimpleString=%s\n"), syspath.c_str());
    PyRun_SimpleString(syspath.c_str());

    PyObject *pName;
    pName = PyUnicode_DecodeFSDefault("far2l");
    pyPluginModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pyPluginModule == NULL) {
        PyErr_Print();
        flog(_T("Failed to load \"far2l\"\n"));
        return;
    }

    pyPluginManager = PyObject_GetAttrString(pyPluginModule, "pluginmanager");
    if (pyPluginManager == NULL) {
        flog(_T("Failed to load \"far2l.pluginmanager\"\n"));
        Py_DECREF(pyPluginModule);
        pyPluginModule = NULL;
        return;
    }

    PyObject *pyresult = vcall("SetStartupInfo", 1, &::Info);
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
    PyObject *pyresult = vcall("GetPluginInfo", 1, Info);
    PYTHON_VOID()
}

XPORT(HANDLE, OpenPlugin)(int OpenFrom,INT_PTR Item)
{
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    HANDLE result = INVALID_HANDLE_VALUE;
    PyObject *pyresult = vcall("OpenPlugin", 2, OpenFrom, Item);
    PYTHON_HANDLE(INVALID_HANDLE_VALUE)
    return result;
}

XPORT(void, ClosePlugin)(HANDLE hPlugin) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    PyObject *pyresult = vcall("ClosePlugin", 1, hPlugin);
    PYTHON_VOID()
}

XPORT(int, Compare)(HANDLE hPlugin,const PluginPanelItem *Item1,const PluginPanelItem *Item2,unsigned int Mode) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = vcall("Compare", 4, hPlugin, Item1, Item2, Mode);
    PYTHON_INT(0)
    return result;
}
XPORT(int, Configure)(int ItemNumber) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = vcall("Configure", 1, ItemNumber);
    PYTHON_INT(0)
    return result;
}
XPORT(int, DeleteFiles)(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = vcall("DeleteFiles", 4, hPlugin, PanelItem, ItemsNumber, OpMode);
    PYTHON_INT(0)
    return result;
}
XPORT(void, ExitFAR)(void) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    PyObject *pyresult = vcall("ExitFAR", 0);
    PYTHON_VOID()

    Py_XDECREF(pyPluginManager); pyPluginManager = NULL;
    Py_XDECREF(pyPluginModule); pyPluginModule = NULL;
    Py_Finalize();
    if( gPython_Interpreter != NULL ) {
        dlclose(gPython_Interpreter);
        gPython_Interpreter = NULL;
    }
}
XPORT(void, FreeFindData)(HANDLE hPlugin,PluginPanelItem *PanelItem,int ItemsNumber) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    PyObject *pyresult = vcall("FreeFindData", 3, hPlugin, PanelItem, ItemsNumber);
    PYTHON_VOID()
}
XPORT(void, FreeVirtualFindData)(HANDLE hPlugin,PluginPanelItem *PanelItem,int ItemsNumber) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    PyObject *pyresult = vcall("FreeVirtualFindData", 3, hPlugin, PanelItem, ItemsNumber);
    PYTHON_VOID()
}
XPORT(int, GetFiles)(HANDLE hPlugin,PluginPanelItem *PanelItem,int ItemsNumber,int Move,const wchar_t **DestPath,int OpMode) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = vcall("GetFiles", 6, hPlugin, PanelItem, ItemsNumber, Move, DestPath, OpMode);
    PYTHON_INT(0)
    return result;
}
XPORT(int, GetFindData)(HANDLE hPlugin,PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = vcall("GetFindData", 4, hPlugin, pPanelItem, pItemsNumber, OpMode);
    PYTHON_INT(0)
    return result;
}
XPORT(void, GetOpenPluginInfo)(HANDLE hPlugin, OpenPluginInfo *Info) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    memset(Info, 0, sizeof(*Info));
    Info->StructSize = sizeof(*Info);
    PyObject *pyresult = vcall("GetOpenPluginInfo", 2, hPlugin, Info);
    PYTHON_VOID()
}
XPORT(int, GetVirtualFindData)(HANDLE hPlugin, PluginPanelItem **pPanelItem, int *pItemsNumber, const wchar_t *Path) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = vcall("GetVirtualFindData", 4, hPlugin, pPanelItem, pItemsNumber, Path);
    PYTHON_INT(0)
    return result;
}
XPORT(int, MakeDirectory)(HANDLE hPlugin,const wchar_t **Name,int OpMode) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = vcall("MakeDirectory", 3, hPlugin, Name, OpMode);
    PYTHON_INT(0)
    return result;
}
XPORT(HANDLE, OpenFilePlugin)(const wchar_t *Name,const unsigned char *Data,int DataSize,int OpMode) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    HANDLE result = INVALID_HANDLE_VALUE;
    PyObject *pyresult = vcall("OpenFilePlugin", 4, Name, Data, DataSize, OpMode);
    PYTHON_HANDLE(INVALID_HANDLE_VALUE)
    return result;
}
XPORT(int, ProcessDialogEvent)(int Event,void *Param) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = vcall("ProcessDialogEvent", 2, Event, Param);
    PYTHON_INT(0)
    return result;
}
XPORT(int, ProcessEditorEvent)(int Event,void *Param) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = vcall("ProcessEditorEvent", 2, Event, Param);
    PYTHON_INT(0)
    return result;
}
XPORT(int, ProcessEditorInput)(const INPUT_RECORD *Rec) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = vcall("ProcessEditorInput", 1, Rec);
    PYTHON_INT(0)
    return result;
}
XPORT(int, ProcessEvent)(HANDLE hPlugin,int Event,void *Param) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = vcall("ProcessEvent", 3, hPlugin, Event, Param);
    PYTHON_INT(0)
    return result;
}
XPORT(int, ProcessHostFile)(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = vcall("ProcessHostFile", 4, hPlugin, PanelItem, ItemsNumber, OpMode);
    PYTHON_INT(0)
    return result;
}
XPORT(int, ProcessKey)(HANDLE hPlugin,int Key,unsigned int ControlState) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = vcall("ProcessKey", 3, hPlugin, Key, ControlState);
    PYTHON_INT(0)
    return result;
}
XPORT(int, ProcessSynchroEvent)(int Event,void *Param) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = vcall("ProcessSynchroEvent", 2, Event, Param);
    PYTHON_INT(0)
    return result;
}
XPORT(int, ProcessViewerEvent)(int Event,void *Param) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = vcall("ProcessViewerEvent", 2, Event, Param);
    PYTHON_INT(0)
    return result;
}
XPORT(int, PutFiles)(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int Move,const wchar_t *SrcPath, int OpMode) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = vcall("PutFiles", 6, hPlugin, PanelItem, ItemsNumber, Move, SrcPath, OpMode);
    PYTHON_INT(0)
    return result;
}
XPORT(int, SetDirectory)(HANDLE hPlugin,const wchar_t *Dir,int OpMode) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = vcall("SetDirectory", 3, hPlugin, Dir, OpMode);
    PYTHON_INT(0)
    return result;
}
XPORT(int, SetFindList)(HANDLE hPlugin,const PluginPanelItem *PanelItem,int ItemsNumber) {
    flog(_T("%s:%d\n"), __FUNCTION__, __LINE__);
    int result = 0;
    PyObject *pyresult = vcall("SetFindList", 3, hPlugin, PanelItem, ItemsNumber);
    PYTHON_INT(0)
    return result;
}
