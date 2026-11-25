#ifndef __RESLOCK_H
#define __RESLOCK_H

#include <windows.h>

extern CRITICAL_SECTION g_ResManLock;

#define ResManLock()   EnterCriticalSection(&g_ResManLock)
#define ResManUnlock() LeaveCriticalSection(&g_ResManLock)

#endif
