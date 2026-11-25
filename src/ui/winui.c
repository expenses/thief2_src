/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/winui.c,v 1.2 2000/02/19 13:28:24 toml Exp $
// windows UI utility elements of Gedit

// quarantine the virus
#include <windows.h>
// quarantine the virus


#include <winui.h>
#include <comtools.h>
#include <appagg.h>
#include <wappapi.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

HWND winui_GetMainWnd(void)
{
   IWinApp *pWinApp = AppGetObj(IWinApp);
   HWND hWnd = IWinApp_GetMainWnd(pWinApp);
   SafeRelease(pWinApp);
   return hWnd;
}

// get y or n response
BOOL winui_GetYorN(char *msg)
{
   int rv=MessageBox(winui_GetMainWnd(),msg,"Dromed Yes or No Question",MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION);
   return rv==IDYES;
}

int winui_GetYesNoCancel(char *msg)
{
   int rv=MessageBox(winui_GetMainWnd(),msg,"Dromed Yes or No Question",MB_YESNOCANCEL|MB_DEFBUTTON2|MB_ICONQUESTION);
   if (rv==IDYES)
      return 1;
   if (rv==IDNO)
      return 0;
   return -1;
}

// draw a big menu
void winui_Text(char *msg)
{
   MessageBox(NULL,msg,"Dromed Info",MB_OK);
}
