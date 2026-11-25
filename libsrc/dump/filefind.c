/*
 * $Source: x:/prj/tech/libsrc/dump/RCS/filefind.c $
 * $Revision: 1.5 $
 * $Author: dc $
 * $Date: 1997/08/11 08:17:54 $
 *
 * Screen dumping file finder (actually quite generic)
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>     // _MAX_PATH
#include <fcntl.h>
#include <io.h>
#include <sys\stat.h>
#include <nameconv.h>
#include <windows.h>
#include <lg_file.h>
#include <config.h>

/* Finds the free file in a sequence like
 * "<prefix>000.<suff>", tries to open it,
 * and returns you a pointer to it.
 * maximum prefix is 5 letters long.
 * maximum suffix is 3 letters long.
 * numbers the files in decimal.
 * if successful and buff non zero copies the name into buff
 */

// modified by dc to use max path
int dmp_find_free_file(char *buff,char *prefix,char *suffix)
{
   char fname[_MAX_PATH];
   int fp, ps;
   int num=0;
   
   ps = strlen(prefix);
   strcpy(fname,prefix);
   if (config_is_defined("screenshot_timestamp"))
   {
      SYSTEMTIME st;
      GetLocalTime(&st);
      sprintf(fname+strlen(fname),"%04d%02d%02d%02d%02d%02d0.",
              st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
      ps = strlen(fname)-4;
      num = st.wSecond*10;
   }
   else
      strcat(fname,"000.");
   strcat(fname,suffix);
   
   /* Look for files like uwpic000.gif */
   while ( (fp = lg_open(fname,O_BINARY|O_RDONLY)) != -1)
   {  /* Check next slot */
      lg_close(fp);                    // good idea to, like, close the opened file
      ++num;
      fname[ps+2] = '0'+(num%10);
      fname[ps+1] = '0'+((num/10)%10);
      fname[ps] = '0'+((num/100)%10);
   }
   
   if ((fp = lg_open(fname,O_CREAT|O_BINARY|O_WRONLY,S_IREAD|S_IWRITE))==-1)
      return 0;
   strcpy(buff,fname);
   lg_close(fp);
   return 1;
}


