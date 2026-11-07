/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#ifdef EDITOR
#line 1 "r:\\prj\\thief2\\skup\\thief2\\src\\ui\\winui.c"
#else
#line 1 "s:\\prj\\thief2\\src\\ui\\winui.c"
#endif
#line 1
// $Header: r:/t2repos/thief2/src/ui/winui.c,v 1.2 2000/02/19 13:28:24 toml Exp $
// windows UI utility elements of Gedit

// quarantine the virus
#include <windows.h>
// quarantine the virus


#include <winui.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// get y or n response
BOOL winui_GetYorN(char *msg)
{
   int rv=MessageBox(NULL,msg,"Dromed Yes or No Question",MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION);
   return rv==IDYES;
}

// draw a big menu
void winui_Text(char *msg)
{
   MessageBox(NULL,msg,"Dromed Info",MB_OK);
}
