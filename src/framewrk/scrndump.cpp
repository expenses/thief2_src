/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/scrndump.cpp,v 1.2 2000/02/19 13:16:34 toml Exp $
#include <comtools.h>
#include <appagg.h>
#include <scrndump.h>
#include <string.h>

// for screen dump 
#include <dispapi.h>
#include <dump.h>
#include <status.h>
#include <config.h>
#include <lg.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

EXTERN BOOL FMMakeDir(const char *pszName, char *pszOut);
void D3DCreateDesktopCanvas();
void D3DReleaseDesktopCanvas();

static BOOL dump_now = FALSE; 
static char dump_fname[16]; 


void SetScreenDump(const char* fname)
{
   if (fname && *fname)
   {
      strncpy(dump_fname,fname,sizeof(dump_fname)); 
      dump_fname[sizeof(dump_fname)-1] = '\0'; 
   }
   else 
      strcpy(dump_fname,"dump"); 
   dump_now = TRUE; 
}


void DumpScreenMaybe(void)
{
   if (!dump_now)
      return; 
   dump_now = FALSE; 

   AutoAppIPtr_(DisplayDevice,pDisp); 

   pDisp->Lock(); 

   char buffer[1024];
   char path[1024];
   char *prefix = dump_fname;
   if (FMMakeDir("screenshots", path))
   {
      strcat(path, "\\");
      strcat(path, dump_fname);
      prefix = path;
   }

   D3DCreateDesktopCanvas();

   int rv;
   BOOL have_format = config_get_raw("screenshot_format", buffer, 16);
   if (have_format && !strcmp(buffer, "png"))
      rv = dmp_find_free_file(buffer, prefix, "png") ? dmp_png_dump_screen(buffer) : 0;
   else if (have_format && !strcmp(buffer, "pcx"))
      rv = dmp_find_free_file(buffer, prefix, "pcx") ? dmp_pcx_dump_screen(buffer) : 0;
   else
      rv = dmp_bmp_file(buffer, prefix);

   D3DReleaseDesktopCanvas();

   pDisp->Unlock(); 

   if (rv==-1)
      Status("Couldn't Screen Dump");
   else
   {
      strcat(buffer," has been saved");
      Status(buffer);
   }
}



