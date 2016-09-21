#pragma once


#include "plugin.hpp"
#include <windows.h>

class Plugin {
public:
    static Plugin& getInstance();

public:
    Plugin();
public:
    int getVersion();
    void setStartupInfo(const struct PluginStartupInfo * psi);
    void exitFar();
    void getPluginInfo(PluginInfo * pi);
    int configure(int item);
    HANDLE openPlugin(int openFrom, intptr_t item);
    void closePlugin(HANDLE Plugin);
    int getOpenPluginInfo(HANDLE Plugin, OpenPluginInfo * pluginInfo);
    int getFindData(HANDLE Plugin, PluginPanelItem ** PanelItem, int * itemsNumber, int OpMode);
    int freeFindData(HANDLE Plugin, PluginPanelItem * PanelItem, int itemsNumber);
    int processHostFile(HANDLE Plugin, struct PluginPanelItem * PanelItem, int ItemsNumber, int OpMode);
    int processKey(HANDLE Plugin, int key, unsigned int controlState);
    int rocessEvent(HANDLE Plugin, int Event, void * Param);
    int setDirectory(HANDLE Plugin, const wchar_t * Dir, int OpMode);
    int makeDirectory(HANDLE Plugin, const wchar_t ** Name, int OpMode);
    int deleteFiles(HANDLE Plugin, PluginPanelItem * PanelItem, int itemsNumber, int OpMode);
    int getFiles(HANDLE Plugin, PluginPanelItem * PanelItem, int itemsNumber, int Move, const wchar_t ** destPath, int OpMode);
    int putFiles(HANDLE Plugin, PluginPanelItem * PanelItem, int itemsNumber, int Move, const wchar_t * SrcPath, int OpMode);
    int processEditorEvent(int Event, void * Param);
    int processEditorInput(const INPUT_RECORD * Rec);

};
