/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/init.cpp,v 1.107 2000/02/25 22:58:18 mwhite Exp $

#include <float.h>

#include <comtools.h>
#include <recapi.h>
#include <gshelapi.h>
#include <dispapi.h>
#include <appapi.h>
#include <appagg.h>
#include <loopapi.h>
#include <config.h>
#include <cfg.h>
#include <fault.h>
#include <mprintf.h>
#include <kbcook.h>
#include <stdlib.h>
#include <res.h>
#include <resapi.h>
#include <inpapi.h>
#include <inpinit.h>
#include <tminit.h>
#include <timer.h>
#include <resagg.h>
#include <string.h>
#include <cfgtool.h>
#include <breakkey.h>
#include <hchkthrd.h>
#include <iobjsys.h>
#include <scrptapi.h>
#include <scrnmode.h>

#include <appname.h>
#include <contexts.h>
#include <init.h>
#include <imgdds.h>
#include <direct.h>
#include <str.h>
#include <lglog.h>
#include <mprintf.h>
#include <datapath.h>
#include <version.h>

extern int g_nMipmapMode;
extern "C" BOOL UseD3DDisplay(void);
BOOL g_bFamPal32Bit = TRUE;
BOOL g_bLegacy32BitTxtPal;
cAnsiStr g_AppVersionStr;
#include <2dapp.h>
#include <resapp.h>
#include <gameinfo.h>
#include <loopapp.h>
#include <uiapp.h>
#include <r3app.h>
#include <portapp.h>
#include <dispatch.h>
#include <dispbase.h>
#include <propman.h>
#include <pickgame.h>
#include <traitman.h>
#include <linkman.h>
#include <trcache.h>
#include <propag8n.h>
#include <stimuli.h>
#include <stimsens.h>
#include <reaction.h>
#include <stimul8r.h>
#include <stimsrc.h>
#include <dxload.h>
#include <simtime.h>
#include <gametool.h>
#include <aiapi.h>
#include <movieapi.h>
#include <contain.h>
#include <gen_bind.h>
#include <keysys.h>
#include <simman.h>
#include <questapi.h>
#include <gamestr.h>
#include <vocore.h>
#include <campaign.h>
#include <netman.h>
#include <iobjnet.h>
#include <movie.h>
#include <arqapi.h>
#include <diskfree.h>
#include <dxwrndlg.h>
#include <cdplayer.h>
#include <dyntex.h>
#include <scrptne_.h>
#include <random.h>

#include <md.h>

// must be last header
#include <dbmem.h>

// huh?
#ifdef __WATCOMC__
#pragma warning 555 9
#endif

EXTERN BOOL CheckForCD(void);

//------------------------------------------------------------
// CONFIG INITIALIZATION
//

#define CONFIG_FILE "cam.cfg"
#define GAME_CFG_VAR "game"
#define INCLUDE_PREFIX "include_"
void AppShutdownConfig(void);

//----------------------------------------

static uint app_read_cfg(char* )
{
   return CONFIG_DFT_HI_PRI;
}


//------------------------------------------------------------
// MONOCHROME
//


static void init_monochrome()
{
   static initialized = FALSE;

   if (initialized)
      return;

   initialized = TRUE;

   char monofname[256];
   monofname[0] = '\0';

#ifdef WIN32
#ifndef SHIP
   {
      bool is_mono=!config_is_defined("mono_no_screen");
      char *buf;
      // for now, we know NT cant do Mono's
      if ((buf=getenv("OS"))!=NULL)
         if (stricmp(buf,"Windows_NT")==0)
            is_mono=FALSE;
      mono_win_init(is_mono);
   }
#else
   mono_win_init(FALSE);
#endif
#else
   mono_init();
#endif

   mono_clear();
   mono_set_flags(MONO_FLG_WRAPCLEAR,NULL);
   config_get_raw("monolog",monofname,256);
   if (strcmp(monofname,"")==0)
      strcpy(monofname,"monolog.txt");
   if (config_is_defined("monolog"))
      mono_logon(monofname, MONO_LOG_NEW, MONO_LOG_ALLWIN);
   mono_to_debugger = config_is_defined("monodebug");
}

//------------------------------------------------------------
// GAME SELECTION
//

static void pick_game(void)
{
   char buf[80];
   if (config_get_raw(GAME_CFG_VAR,buf,sizeof(buf)))
      AppSelectGame(buf);  // select the game we named
   else
      AppSelectGame(NULL); // select the default game

#ifndef SHIP
   // for programmers only, load prog.cfg so we can find game.cfg and other
   // stuff 
   config_load("prog.cfg");
#endif 

   //
   // Include "game.cfg" 
   //
   char game[128]; 
   config_get_raw("game",game,sizeof(game)); 

   // punt trailing space
   char* s; 
   for (s = game + strlen(game) - 1; s >= game && isspace(*s); s--)
      *s = '\0'; 
   s++; 
   strcat(game,".cfg");
   
   char path[256]; 
   Verify(find_file_in_config_path(path,game,"include_path")); 
   config_load(path); 
}



//------------------------------------------------------------
// CUSTOM PURE-VIRTUAL TRAP

#if defined(_MSC_VER)
EXTERN int __cdecl _purecall(void)
{
   CriticalMsg("Pure-virtual function call!");
   exit(1);
   return 0;
}
#endif

//------------------------------------------------------------
// DEBUG/SPEW SUPPORT
//

#ifdef DBG_ON
int KeyGetch ()
{
   ushort code;

   kb_flush_bios();

   while (TRUE)
   {
      if (kb_get_cooked (&code) && (code & KB_FLAG_DOWN))
      {
         return code ^ KB_FLAG_DOWN;
      }
   }
}
#endif

#ifndef SHIP 
#define ScriptPrintString mprintf
#else 
#define ScriptPrintString NULL
#endif

//------------------------------------------------------------
// MINIMUM DISK SPACE
//

#define MIN_STARTUP_DISK_MB 35

//------------------------------------------------------------
// CORE ENGINE AGGREGATE OBJECT CREATION
//

tResult LGAPI CoreEngineCreateObjects(int argc, const char *argv[])
{
   char buffer[40];

   g_argv = argv;
   g_argc = argc;

   //
   // config sys initializes NOW so that it can be used to create objects
   //
   config_init();
   config_read_file(CONFIG_FILE,app_read_cfg);

   pick_game();

   // must come after pick_game
   process_config_includes(INCLUDE_PREFIX);
#ifdef EDITOR
   process_config_includes("editor_"INCLUDE_PREFIX); 
#endif 

   config_parse_commandline(g_argc,g_argv,NULL);

   AtExit(AppShutdownConfig);

   // now that we have config, lets go make sure we can run

   if (!config_is_defined("skip_writeaccess_check"))
      CheckForWriteAccessAndMessage();

   if (!config_is_defined("skip_starting_checks"))
   {
      if (!CheckForDiskspaceAndMessage(NULL,MIN_STARTUP_DISK_MB))
         Exit(1,NULL); // CheckFor does a message box

      if (!LoadDirectX())
         Exit(1,NULL);  // message already printed in LoadDirectX itself
   }

   // umm, ship only or something?
   init_monochrome();

#ifndef DONT_CHECK_CD
   // Is the CD in the drive? - in ship, we always check this
#ifndef SHIP
   if (!config_is_defined("skip_starting_checks"))
#endif
      if (!CheckForCD())
         Exit(1,NULL);
#endif

   //
   //  Next let's do fault
   //

#ifdef FAULT
   if (!config_is_defined("nofault"))
      ex_startup(EXM_DIVIDE_ERR);
#endif

#if defined(_MSC_VER) && !defined(SHIP)
   int cw = _controlfp( 0, 0 );
   
   if (config_is_defined("fp_fault_over"))
      cw &= ~EM_OVERFLOW;
   if (config_is_defined("fp_fault_under"))
      cw &= ~EM_UNDERFLOW;
   if (config_is_defined("fp_fault_divzero"))
      cw &= ~EM_ZERODIVIDE;
   if (config_is_defined("fp_fault_denorm"))
      cw &= ~EM_DENORMAL;
      
   _controlfp( cw, MCW_EM );
#endif

   //
   // From this point on, all we're doing is adding COM objects to the
   // app aggregate.  They initialize in constrained priority order.
   //


   // Get display device from config
   eDisplayDeviceKind dispkind = kDispDebug;
   config_get_int("display",&dispkind);

   // Get other options from config
   int opt = kGameShellDefault & ~(kLockFrame | kFlushOnEndFrame);
   if (config_is_defined("multithread"))
      opt |= kMultithreadedShell;

   g_ResDXTMode = config_is_nonzero("dxt_to_rgb32") ? kResDXTDecompress : kResDXTAuto;

   config_get_int("mipmap_mode", &g_nMipmapMode);
   if (g_nMipmapMode < 0)
      g_nMipmapMode = 0;

   if (UseD3DDisplay() && !config_is_nonzero("force_32bit"))
      config_set_int("force_32bit", TRUE);
   if (config_is_nonzero("force_32bit") && config_is_nonzero("force_32bit_textures"))
   {
      g_bForce32BitTextures = TRUE;
      g_bFamPal32Bit = !config_is_nonzero("disable_32bit_fampal");
      g_bLegacy32BitTxtPal = config_is_nonzero("legacy_32bit_txtpal");
   }

   if (LogGetOptions() & kLogToAll)
   {
      static const char *s_LogConfigVars[] =
      {
      "resname_base", "cd_path", "load_path", "script_module_path",
      "movie_path", "include_path", "mod_path", "uber_mod_path",
      "fm_path", "data_path", "variant_path", "install_path",
      "script_path", "language", "fm", "fm_language",
      "fm_language_forced", "game_screen_size", "game_screen_depth", "game_screen_flags",
      "game_full_screen", "screen_size", "screen_depth", "d3d_driver_index",
      "game_hardware", "vsync_mode", "force_windowed", "single_display_mode",
      "ui_scale_mode", "use_d3d_display", "d3d_disp_scaled_2d_overlay", "framerate_cap",
      "SlowFrame", "SlowFrameUI", "force_32bit", "force_32bit_textures",
      "mipmap_mode", "lm_mipmap_mode", "lm_init_texmem", "lm_filter_margin",
      "disable_lightmap_aniso", "tex_edge_padding", "disable_alpha_test", "alpha_test_as_opaque",
      "default_alphatest_ref", "d3d_disp_enable_atoc", "d3d_disp_sw_cc", "d3d_disp_enable_hdr",
      "multisampletype", "postprocess", "d3d_disp_enable_distortionfx", "sfx_device",
      "sfx_channels", "mixer_rate", "sfx_vol_music", "sfx_vol_2d",
      "sfx_vol_3d", "snd_speaker_config", "submerged_sound_occlusion", "force_underwater_reverb",
      "sfx_mixer_timeout", "load_with_threaded_mixer", "game", "dark1",
      "obj_min", "obj_max", "max_refs", "gui_font",
      "use_hi_res_timer", "disable_simd_optimizations", "phys_freq", "enhanced_sky",
      "skip_intro", "no_movies", "crop_movies", "new_mantle",
      "new_mantle_fwd", "swim_chest_offset", "apply_dbmods", "dbmod_no_remap",
      "z_far", "wr_render_zcomp", "render_weapon_particles", "legacy_force_match_unrefs",
      "model_lod_dist_factor", "model_lod_force_low", "dark_zcomp_arm", "edit_screen_size",
      "edit_screen_depth", "editor_disable_gdi", "game_mode_backup", "song_source_path",
      "schema_base_path", "vbrush_dir", "d3d_disp_hw2d_msaa",
      };
      char buf[8192];

      if (_getcwd(buf, sizeof(buf)))
         mprint(_LogFmt("CWD: %s", buf));
      const char *pszFMPath = *g_szFMPath ? g_szFMPath : NULL;
      if (pszFMPath)
         mprint(_LogFmt("FM Path: %s", pszFMPath));
      mprint("-----------------------------------------------------------");
      g_AppVersionStr.FmtStr("%s %s", APPNAME, VersionString());
      mprint(_LogFmt("App Version: %s", (const char *)g_AppVersionStr));
      mprint("--------------------- misc config -------------------------");
      for (unsigned i = 0; i < sizeof(s_LogConfigVars) / sizeof(s_LogConfigVars[0]); i++)
      {
         const char *pszVar = s_LogConfigVars[i];
         if (config_get_raw(pszVar, buf, sizeof(buf)))
         {
            buf[sizeof(buf) - 1] = '\0';
            if (*buf)
               mprint(_LogFmt("%s %s", pszVar, buf));
            else
               mprint(pszVar);
         }
      }
      mprint("-----------------------------------------------------------");
   }

   // Use the convenience macro to create all the basic game components
   CoreGameLibrariesCreate(APPNAME, argc, argv, dispkind, opt);
   ScriptManCreate(GetSimTime, ScriptPrintString);
   Gr2dCreate();
   ResSysCreate();
   LoopManagerCreate();
   LoopAppCreate();
   uiSysCreate();
   r3SysCreate();
   PortalSysCreate();
   InputManagerCreate();
   InputCreate();
   ResCreate();
   Res2Create();
   PropertyManagerCreate();
   LinkManagerCreate();
   TraitManagerCreate();
   ObjectSystemCreate();
   DonorCacheCreate();
   PropagationCreate();
   StimuliCreate();
   StimSensorsCreate();
   ReactionsCreate();
   StimulatorCreate();
   StimSourcesCreate();
   GameToolsCreate(); 
   AIManagerCreate();
   MoviePlayer1Create(); 
   SimManagerCreate(); 
   QuestDataCreate();
   GameStringsCreate(); 
   VoiceOverCreate(); 
   CampaignCreate(); 
   InputBinderCreate (&g_pInputBinder);
   AsyncReadQueueCreate();
   CDPlayerCreate();
   DynTextureCreate();

   // @TODO: move these out to an "engine features" app object creation function,
   // that is called by the systems that use engfeat? 
   ContainSysCreate();
   KeySysCreate();
#ifdef NEW_NETWORK_ENABLED
   NetManagerCreate();
   ObjectNetworkingCreate();
   ScriptNetworkingCreate();
#endif

   ResSharedCacheCreate();


   // start recording or playing back
   if (config_get_raw("record",buffer,38)) {
      RecorderCreate(kRecRecord, buffer);
   } else if (config_get_raw("playback",buffer,38)) {
      RecorderCreate(kRecPlayback, buffer);
   }

   // set the API switch 
   md_use_lgd3d();

   return NOERROR;
}


//------------------------------------------------------------
// CORE INIT FUNCTION
//
tResult LGAPI CoreEngineAppInit()
{
   pGameShell = AppGetObj(IGameShell);

   init_monochrome();

#ifdef WE_CARED_ABOUT_DBG
   DbgInit();
   DbgInstallGetch (KeyGetch);

   if (config_is_defined ("dbg"))
   {
      DbgMonoConfig ();
   }
#endif

   if (config_is_defined("breakkey"))
      BreakKeyActivate(VK_F12,VK_F11);

   if (config_is_defined("heapcheck"))
      HeapCheckActivate(VK_F9);

   tm_init();

   // set default screen mode
   sScrnMode mode = { 0 }; 
   ScrnModeGetConfig(&mode,"");
   ScrnModeSetDefault(&mode); 

   DispatchInit();
   DispatchMsgAllClients(kMsgAppInit,NULL,kDispatchForward);


   //input binding stuff. load all contexts from "default.bnd" and "<game>.bnd"
   g_pInputBinder->Init (NULL, NULL);
   InitIBVars ();

   //  pop dialog box about dxdrivers
   if (config_is_defined("dx_warn_dlg"))
      DxWarnDlg();

   // initialize the random number lib
   // note fullwise: we retardedly could be calling this twice,
   // since the net lib wants to init it as well
   // if we didn't suck we would give this a better seed
   RandInit(tm_get_millisec());

   return NOERROR;
}

//------------------------------------------------------------
// CORE ENGINE APP EXIT
//

tResult LGAPI CoreEngineAppExit()
{
   MovieOnExit();
   g_pInputBinder->Term ();
   SetGameIBVarsFunc(NULL);
   g_pInputBinder = NULL;
   DispatchMsgAllClients(kMsgAppTerm,NULL,kDispatchReverse);
   DispatchShutdown();
   SafeRelease(pGameShell);
   return NOERROR;
}


//------------------------------------------------------------
// Config Sys Shutdown
//

static config_write_spec ConfigWritableTable[] =
{
   { NULL, }
};

static bool write_func(char* filename, char* var)
{
   if (config_write_to_same_file(filename,var))
      return TRUE;
   else
   {
      char buf[2]; // don't really need the whole filename
      // did the var come from a file
      config_get_origin(var,buf,sizeof(buf));
      if (buf[0] == '\0')
         return TRUE;
   }
   return config_default_writable(filename,var);
}

void AppShutdownConfig(void)
{
   config_set_writable_table(ConfigWritableTable);
   config_write_file(CONFIG_FILE,write_func);
   config_shutdown();
}

//------------------------------------------------------------
// INITIALIZATION GLOBALS
//

const char ** g_argv;
int g_argc;

sLoopInstantiator* gPrimordialMode = NULL;




/*
Local Variables:
typedefs:("config_write_spec" "uint" "ushort")
End:
*/
