#pragma once

#include <Common.h>

class TConfiguration;
class TStoredSessionList;
extern TStoredSessionList * StoredSessions;

void CoreInitialize();
void CoreFinalize();
void CoreSetResourceModule(void * ResourceHandle);
void CoreMaintenanceTask();
TConfiguration * GetConfiguration();

UnicodeString NeonVersion();
UnicodeString ExpatVersion();

