// $Header$

#ifndef __CSGERR_H
#define __CSGERR_H

#include <setjmp.h>
#include <mprintf.h>

EXTERN BOOL g_bCsgExceptionHandlerActive;
EXTERN jmp_buf g_CsgJmpBuf;

#if _MSC_VER >= 1400
#define CsgError(...) \
   do { \
      if (g_bCsgExceptionHandlerActive) { \
         mprintf("ERROR: " __VA_ARGS__); \
         longjmp(g_CsgJmpBuf, 1); \
      } \
      Error(1, __VA_ARGS__); \
   } while (0)
#else
EXTERN void CsgError(char *msg, ...);
#endif

#endif
