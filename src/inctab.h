// $Header: r:/prj/cam/src/RCS/inctab.h 1.1 1997/05/29 17:14:57 JON Exp $

#ifndef INCTAB__H
#define INCTAB__H

#define INC_TAB_INT 1
#define INC_TAB_FLOAT 2

EXTERN void IncTabsInit();
EXTERN void IncTabsShutdown();
EXTERN void IncTabAddInt(char *define, int val);
EXTERN void IncTabAddFloat(char *define, float val);
EXTERN BOOL IncTabLookupInt(char *define, int *val);
EXTERN BOOL IncTabLookupFloat(char *define, float *val);
EXTERN int IncTabLookup(char *define, void *val);

#endif
