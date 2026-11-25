/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ui/movie.cpp,v 1.9 2000/03/16 17:16:11 patmac Exp $
//
// Movie playing APIs
//

#include <windows.h>
#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <filespec.h>

#include <movieapi.h>
#include <dispapi.h>
#include <2d.h>
#include <keydefs.h>
#include <gamma.h>
#include <config.h>
#include <string.h>
#include <ctype.h>

#include <movie.h>
#include <mprintf.h>

// Must be last header 
#include <dbmem.h>


EXTERN RECT g_rcUIScaleView;
EXTERN BOOL g_bUIScaleMode;
EXTERN BOOL g_bUIScaleView;

BOOL MovieInConfigList(const char* fn, const char* var)
{
   char list[1024];

   if (!fn || !*fn)
      return FALSE;

   if (!config_get_raw(var, list, sizeof(list)))
      return FALSE;

   const char* base = strrchr(fn, '/');
   const char* bslash = strrchr(fn, '\\');
   if (bslash > base)
      base = bslash;
   base = base ? base + 1 : fn;

   for (char* p = list; p; )
   {
      char* next = NULL;
      char* plus = strchr(p, '+');
      if (plus)
      {
         *plus = '\0';
         next = plus + 1;
      }

      while (iswspace(*p))
         p++;
      if (!*p)
         continue;

      for (char* end = p + strlen(p) - 1; end > p && iswspace(*end); end--)
         *end = '\0';
      if (!*p)
         continue;

      if (!stricmp(base, p))
         return TRUE;
      p = next;
   }
   return FALSE;
}

static BOOL MovieCropExcluded(const char* fn)
{
   return MovieInConfigList(fn, "movie_crop_exclude") || MovieInConfigList(fn, "fm_movie_crop_exclude");
}

static void MovieSetupCropRect()
{
   int cnt = 4;

   if (config_get_value("crop_movies", CONFIG_INT_TYPE, (config_valptr)&g_rcUIScaleView, &cnt) && (cnt == 4)
       && g_rcUIScaleView.right && g_rcUIScaleView.bottom && (g_rcUIScaleView.bottom <= g_rcUIScaleView.right)
       && (g_rcUIScaleView.left <= 640) && (g_rcUIScaleView.top <= 480)
       && (g_rcUIScaleView.left + g_rcUIScaleView.right <= 640) && (g_rcUIScaleView.top + g_rcUIScaleView.bottom <= 480))
   {
      g_rcUIScaleView.left = grd_canvas->bm.w * g_rcUIScaleView.left / 640;
      g_rcUIScaleView.top = grd_canvas->bm.h * g_rcUIScaleView.top / 480;
      g_rcUIScaleView.right = grd_canvas->bm.w * g_rcUIScaleView.right / 640;
      g_rcUIScaleView.bottom = grd_canvas->bm.h * g_rcUIScaleView.bottom / 480;
   }
   else
   {
      g_rcUIScaleView.right = grd_canvas->bm.w;
      g_rcUIScaleView.bottom = grd_canvas->bm.w * 34 / 64;
      g_rcUIScaleView.left = 0;
      g_rcUIScaleView.top = (grd_canvas->bm.h - g_rcUIScaleView.bottom) / 2;
   }
}

EXTERN BOOL MoviePlaySynchronous(const char * pszFile, int volume)
{
   static char termlist[] = { KEY_ENTER, KEY_ESC, KEY_SPACE, KEY_BS, '\0'};

   AutoAppIPtr(MoviePlayer1);
   AutoAppIPtr(DisplayDevice);
   cFileSpec fileSpec(pszFile);
   float oldGamma;

   if (!pMoviePlayer1)
      return FALSE;
      
   fileSpec.MakeFullPath();
   if (!fileSpec.FileExists())
      return FALSE;
   
   // remember old gamma setting, set gamma to default
   //  this is to keep movies from looking bad if user has cranked up gamma
   oldGamma = g_gamma;
   g_gamma = DEFAULT_GAMMA;
   config_get_float("gamma_movie", &g_gamma);
   gamma_update();

   // @TODO (toml 06-16-97): This is hacked e3 logic and should be distrusted as long as this comment is here
   int iDisplayLock = pDisplayDevice->BreakLock();

   gr_push_canvas(grd_visible_canvas);

   g_bUIScaleMode = !config_is_defined("scale_movie_to_ui");
   if (g_bUIScaleMode)
   {
      g_bUIScaleView = config_is_defined("crop_movies") && !MovieCropExcluded(pszFile);
      MovieSetupCropRect();
   }

   pMoviePlayer1->SetTermKeys(termlist);
   if (pMoviePlayer1->Open(fileSpec.GetName(), 0, 0, grd_canvas->bm.w, grd_canvas->bm.h))
   {
      pMoviePlayer1->SetVolume(volume);
      pMoviePlayer1->Play(kMoviePlayBlock);
   }
   pMoviePlayer1->Close();

   g_bUIScaleMode = FALSE;
   g_bUIScaleView = FALSE;

   gr_pop_canvas();
   gr_clear(0);
    
   pDisplayDevice->RestoreLock(iDisplayLock);

   // restore old gamma setting
   g_gamma = oldGamma;
   gamma_update();

   return TRUE;
}


// Assumes canvas you want played to is already the current one
BOOL MoviePlayAsynchronous(const char *pszFile,int x,int y,int w,int h, int volume)
{
   AutoAppIPtr(MoviePlayer1);
   AutoAppIPtr(DisplayDevice);
   cFileSpec fileSpec(pszFile);
   float oldGamma;
   
   if (!pMoviePlayer1)
      return FALSE;
      
   fileSpec.MakeFullPath();
   if (!fileSpec.FileExists())
      return FALSE;
   
   // remember old gamma setting, set gamma to default
   //  this is to keep movies from looking bad if user has cranked up gamma
   oldGamma = g_gamma;
   g_gamma = DEFAULT_GAMMA;
   gamma_update();

   // @TODO (toml 06-16-97): This is hacked e3 logic and should be distrusted as long as this comment is here
   int iDisplayLock = pDisplayDevice->BreakLock();

   pMoviePlayer1->Open(fileSpec.GetName(), x, y, w, h);
   pMoviePlayer1->SetVolume(volume);
   // Play asynchronously.
   pMoviePlayer1->Play(kMoviePlayNoStop);
    
   pDisplayDevice->RestoreLock(iDisplayLock);

   // restore old gamma setting
   g_gamma = oldGamma;
   gamma_update();

   return TRUE;
}



void MovieOnExit()
{
   AutoAppIPtr(MoviePlayer1);
   pMoviePlayer1->Stop();
   pMoviePlayer1->Close();
}

// Get the current status
eMP1State MovieStatus()
{
   AutoAppIPtr(MoviePlayer1);
   return pMoviePlayer1->GetState();
}




