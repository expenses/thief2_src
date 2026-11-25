/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/dbfile.cpp,v 2.38 2000/03/07 19:57:07 toml Exp $

#include <windows.h>
#include <lg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <config.h>

#include <dispbase.h>
#include <dbasemsg.h>
#include <dbfile.h>
#include <dbtagfil.h>
#include <tagfile.h>
#include <vernum.h>
#include <filetool.h>
#include <status.h>
#include <command.h>
#include <editsave.h>  // for secret load variables
#include <appsfx.h>
#include <cfgtool.h>

#include <osysbase.h>
#include <objspace.h>

#include <mprintf.h>

#include <filemenu.h>

#include <comtools.h>
#include <wappapi.h>
#include <loopapi.h>
#include <appagg.h>
#include <winui.h>

#include <simtime.h>
#include <simman.h>

#include <btffact.h>
#include <stdlib.h>
#include <math.h>
#include <direct.h>
#include <str.h>
#include <datapath.h>
#include <lg_file.h>
#include <objremap.h>
#include <resapi.h>
#include <questapi.h>
#include <property.h>
#include <propman.h>
#include <iobjsys.h>
#include <wrtype.h>
#include <objpos.h>
#include <filevar.h>
#include <propbase.h>
#include <propstor.h>
#include <dataops.h>
#include <propface.h>
#include <nameprop.h>
#include <linkman.h>
#include <relation.h>
#include <linkbase.h>
#include <linkid.h>
#include <lnkquery.h>
#include <traitman.h>
#include <stimuli.h>
#include <stimsens.h>
#include <stimsrc.h>
#include <senstype.h>
#include <sensbase.h>
#include <receptro.h>
#include <reaction.h>
#include <reacbase.h>
#include <propag8n.h>
#include <propag8r.h>
#include <pg8rbase.h>
#include <ssrcbase.h>
#include <storeapi.h>
#include <respaths.h>
#include <scrptprp.h>
#include <sdesbase.h>
#include <sdesc.h>
#include <sdestool.h>

// must be last header
#include <dbmem.h>

#define LOAD_FILE_VAR "world"
#define SAVE_FILE_VAR "world"
#define LOAD_PATH_VAR "load_path"

static char gCurrGameFile[256] = "";
static char gCurrMapFile[256] = "";

static char gCurrFilename[256]; // so people who want to query can find out

// We count the files that get loaded, from the reset on up. This is mainly
// needed for networking, to have an easy way to synchronize which objects
// were loaded from which file:
static int gCurrFilenum = 0;

// @TODO: when maps split off from missions, change this
#define MISSION_FILE_TYPE (kFiletypeMIS | kFiletypeMAP)

struct sFiletypeExtension
{  
   edbFiletype Filetype;
   char ext[4];
};

// Table of file types and their extensions

static sFiletypeExtension file_type_extensions[] = 
{
   { kFiletypeMIS|kFiletypeMAP, "MIS"},
   { kFiletypeMIS, "MIS"},
   { kFiletypeAll, "COW"},
   { kFiletypeGAM, "GAM"},
   { kFiletypeMAP, "MAP"},
   { 0, "COW"} // terminator, default
};

////////////////////////////////////////////////////////////

void reset_objid_space()
{
   AutoAppIPtr(ObjIDManager);
   sObjBounds bounds = { gMinObjID, gMaxObjID }; 

   config_get_int("obj_min",&bounds.min);
   config_get_int("obj_max",&bounds.max);

   pObjIDManager->ResizeObjIDSpace(bounds); 
}

////////////////////////////////////////////////////////////
// DATABASE FILE OPS/COMMANDS
//


static void munge_path(char* fullname, char* filename)
{
   char base[256] = ".";
   char name[256];
   attachPostfix(name,filename,"."DB_MISSION_SUFFIX);
   config_get_raw(DB_FILE_DIR_VAR,base,sizeof(base));
   if (is_full_path(name))
      strcpy(fullname,name);
   else
      diskPathMunge(fullname,base,NULL,name);
}

static BOOL find_for_load(char* fullname, char* filename)
{
   if (find_file_in_config_path(fullname,filename,DB_FILE_DIR_VAR))
      return TRUE;
   if (find_file_in_config_path(fullname,filename,LOAD_PATH_VAR))
      return TRUE;
   return FALSE;
}

BOOL dbFind(const char* filename, char* fullname)
{
   char buf[256];
   if (fullname == NULL) fullname = buf; 
   return find_for_load(fullname,(char*)filename); 
}

void dbCurrentFile(char *buf, int len)
{
   strncpy(buf, gCurrFilename, len);
}

static const char* strip_pathname(const char* filename)
{
   const char* fn = filename;
   // skip ':' and '\' and '/'
   static char skip_these[] = ":/\\";
   for (char* skip = skip_these; *skip != '\0'; skip++)
   {
      const char* found = strrchr(fn,*skip);
      if (found != NULL) fn = found+1;
   }
   return fn;
}

void dbSetCurrentFile(char *buf)
{
   strncpy(gCurrFilename, (char *)strip_pathname(buf),sizeof(gCurrFilename));
   gCurrFilename[sizeof(gCurrFilename)-1] = '\0'; 
}
//----------------------------------------

static void set_file_var(char* var, char* filename, bool save)
{
   char name[256];
   attachPostfix(name,filename,"."DB_FILE_SUFFIX);
   config_set_string(var,name);
   if (save)
      config_set_priority(var,CONFIG_DFT_LO_PRI);   
   else
      config_set_priority(var,PRIORITY_TRANSIENT);
}

//----------------------------------------

static edbFiletype guess_Filetype_from_extension(const char* filename)
{
   const char* dot = strrchr(filename,'.');
   if (dot != NULL)
   {
      dot++;
      // Search through the associative array of file types/extensions
      sFiletypeExtension* fte = file_type_extensions;
      for (; fte->Filetype != 0; fte++)
         if (stricmp(dot,fte->ext) == 0) // does extension match
            return fte->Filetype;
   }   
   return 0;
}



static const char* extension_for_filetype(edbFiletype ftype)
{
   sFiletypeExtension* fte = file_type_extensions;
   for (; fte->Filetype != 0; fte++)
      if (fte->Filetype == ftype) // does extension match
         break;
   return fte->ext;
}


// Try every likely extension
static edbFiletype find_for_load_extensionless(char* fullname, char* filename)
{
   sFiletypeExtension* fte = file_type_extensions;
   for (; fte->Filetype != 0; fte++)
   {
      char filebuf[256];
      sprintf(filebuf,"%s.%s",filename,fte->ext);
      if (find_for_load(fullname,filebuf)) 
         return fte->Filetype;
   }
   return 0;
}




//----------------------------------------

static void set_gamesys_file(char* fn)
{
   while(isspace((uchar)*fn)) fn++;
   strcpy(gCurrGameFile, strip_pathname(fn));
   StatusField(SF_GAMESYSNAME, gCurrGameFile);
}

////////////////////////////////////////////////////////////
// MISSION HEADER
//
// contains pointers to .MAP and .GAM files 
// (outdated, present for back-compatibility)
//

TagFileTag mhTag = { "MIS_HEADER" };
TagVersion mhVersion = { 0, 1,};

typedef struct
{
   char mapName[256];      // Full path names
   char gamesysName[256];
} MissionHeader;



//------------------------------------------------------------------
// Read the header to get the .MAP and .GAM filenames.
//------------------------------------------------------------------
BOOL ReadMissionHeader(ITagFile *file, char *mapName, char *gamName)
{
   TagVersion v = mhVersion;
   MissionHeader mh;

   if (FAILED(ITagFile_OpenBlock(file, &mhTag, &v)))
      return FALSE;
   ITagFile_Read(file,(char *)&mh, sizeof(mh));
   strcpy(mapName, mh.mapName);
   strcpy(gamName, mh.gamesysName);
   ITagFile_CloseBlock(file);
   return TRUE;
}

////////////////////////////////////////////////////////////
// FILE TYPE TAG 
//
// Contains the mask of what file types we contain
//

static TagFileTag dbTypeTag = { "FILE_TYPE" };
static TagVersion dbTypeVersion = { 0, 1 };

static BOOL write_file_type(ITagFile* file, edbFiletype Filetype)
{
   TagVersion v = dbTypeVersion;
   if (FAILED(file->OpenBlock(&dbTypeTag,&v)))
      return FALSE;
   file->Write((char*)&Filetype,sizeof(Filetype));
   file->CloseBlock();
   return TRUE;
}

static BOOL read_file_type(ITagFile* file, edbFiletype* Filetype)
{
   TagVersion v = dbTypeVersion;
   if (FAILED(file->OpenBlock(&dbTypeTag,&v)))
      return FALSE;
   BOOL retval = file->Read((char*)Filetype,sizeof(*Filetype)) == sizeof(Filetype);
   file->CloseBlock();
   return retval;
}

////////////////////////////////////////////////////////////
// FILE POINTER TAGS
//

struct sFNameTag
{
   ulong Filetype;
   TagFileTag tag;
};

const TagVersion FileNameVersion = { 0, 1};

const sFNameTag FileNameTags[] = 
{
   { kFiletypeMIS, { "MIS_FILE" }},
   { kFiletypeMAP, { "MAP_FILE" }},
   { kFiletypeGAM, { "GAM_FILE"}},
   { 0},
};

static const TagFileTag* FileNameTag(ulong ftype)
{
   for (int i = 0; FileNameTags[i].Filetype != 0; i++)
      if (FileNameTags[i].Filetype == ftype)
         return &FileNameTags[i].tag;
   return NULL;
}

#define FILEBUF_SIZE 256

static BOOL write_file_ref_tag(ITagFile* file, ulong ftype, const char* filename)
{
   const TagFileTag* tag = FileNameTag(ftype);
   if (tag == NULL) return FALSE;

   TagVersion v = FileNameVersion;
   if (FAILED(file->OpenBlock(tag,&v)))
      return FALSE;

   char buf[FILEBUF_SIZE];
   strncpy(buf,strip_pathname(filename),sizeof(buf));
   buf[sizeof(buf)-1] = '\0';
      
   file->Write(buf,sizeof(buf));
   file->CloseBlock();
   return TRUE;
}

static BOOL read_file_ref_tag(ITagFile* file, ulong ftype, char* filename)
{ 
   const TagFileTag* tag = FileNameTag(ftype);
   if (tag == NULL) return FALSE;

   TagVersion v = FileNameVersion;
   if (FAILED(file->OpenBlock(tag,&v)))
      return FALSE;

   char buf[FILEBUF_SIZE];
   BOOL success = file->Read(buf,sizeof(buf)) == sizeof(buf);
   buf[sizeof(buf)-1] = '\0';

   strcpy(filename,buf);
   file->CloseBlock();
   return success;
}


////////////////////////////////////////////////////////////
// FILE FACTORY 
// 

static cDBFileFactory* file_factory = NULL; 

ITagFile* dbOpenFile(const char* filename, TagFileOpenMode mode)
{
   if (file_factory)
      return file_factory->Open(filename,mode);
   else
      return BufTagFileOpen(filename,mode); 
}



cDBFileFactory* dbSetFileFactory(cDBFileFactory* new_factory)
{
   cDBFileFactory* retval = file_factory;
   file_factory = new_factory;
   return retval; 
}


////////////////////////////////////////////////////////////
// API FUNCTIONS
//

edbFiletype dbSave(const char* filename, edbFiletype filetype)
{
   char fullname[256];
   munge_path(fullname,(char*)filename);


   cAutoIPtr<ITagFile> file(dbOpenFile(fullname,kTagOpenWrite));

   if (file == NULL)
      return 0;

   return dbSaveTagFile(file,filetype);
}


edbFiletype dbSaveTagFile(ITagFile* file, edbFiletype filetype)
{


   // Write out file type
   if (!write_file_type(file,filetype))
      return 0;

   // If we are not saving out a game sys, save out the file ref
   if (!(filetype & kFiletypeGAM))
      write_file_ref_tag(file,kFiletypeGAM,strip_pathname(gCurrGameFile));

   // Likewise, save out the map ref
   if (!(filetype & kFiletypeMAP))
      write_file_ref_tag(file,kFiletypeMAP,strip_pathname(gCurrMapFile));

   DispatchData msg = { filetype | kDatabaseSave, file };
   DispatchMsg(kMsgDatabase,&msg);
   
   return filetype;
}

////////////////////////////////////////


// @TODO: change this to three phases when map/mis splits up
static ulong load_order[] = { kFiletypeGAM, kFiletypeMAP|kFiletypeMIS};
static char* phase_names[] = { "gamesys", "mission"};

typedef BOOL (*find_file_func)(ulong ftype, const char* fname, char* filebuf);
static find_file_func special_file_find = NULL;

#define LOAD_ORDER_PHASES (sizeof(load_order)/sizeof(load_order[0]))

struct sDbPatchEntry;
enum eDbPatchPass
{
   kDbPatchPreLoad,
   kDbPatchLoad,
   kDbPatchPostLoad,
   kDbPatchScripts,
   kDbPatchEnum
};
static void dbPatchRequest(sDbPatchEntry **ppQueue, const char *pszFile, ulong flags, ulong *pFound,
                           int pass, sDbPatchEntry **ppScripts);
static BOOL dbPatchApplyQueue(sDbPatchEntry **ppQueue, sDbPatchEntry *pList, ulong flags, int pass,
                              sDbPatchEntry **ppScripts);
static void dbPatchFreeList(sDbPatchEntry **ppList);
static void dbPatchSetMission(const char *pszName, ulong flags);
static void dbPatchLoadDone(void);

edbFiletype dbLoad(const char* filename, ulong loadtype)
{
   char fullname[256];
   const char* name = filename; 
   if (!file_factory)
   {
      if (!find_for_load(fullname,(char*)filename))
         return 0;
      name = fullname;
   }

   cAutoIPtr<ITagFile> file ( dbOpenFile(name,kTagOpenRead) );
   if (file == NULL)
      return 0;

   edbFiletype retval = dbLoadTagFile(file,loadtype); 

   // So user knows what file he just loaded.
   strncpy(gCurrFilename, (char *)strip_pathname(name),sizeof(gCurrFilename));
   gCurrFilename[sizeof(gCurrFilename)-1] = '\0'; 
   StatusField(SF_FILENAME, gCurrFilename); 

   return retval; 
}

edbFiletype dbMergeLoadTagFile(ITagFile* file, edbFiletype loadtype);


edbFiletype dbMergeLoad(const char* filename, ulong loadtype)
{
   char fullname[256];
   const char* name = filename; 
   if (!file_factory)
   {
      if (!find_for_load(fullname,(char*)filename))
         return 0;
      name = fullname;
   }

   cAutoIPtr<ITagFile> file ( dbOpenFile(name,kTagOpenRead) );
   if (file == NULL)
      return 0;

   edbFiletype retval = dbMergeLoadTagFile(file,loadtype); 

   // So user knows what file he just loaded.
   StatusField(SF_FILENAME, (char*)strip_pathname(fullname));

   return retval; 
}


edbFiletype dbLoadTagFile(ITagFile* file, edbFiletype loadtype)
{
   // We are starting from the ground up:
   gCurrFilenum = 0;

   // Send a reset message
   DispatchData msg = { kDatabaseReset, NULL } ;
   DispatchMsg(kMsgDatabase,&msg);
   
   reset_objid_space();

   edbFiletype retval = dbMergeLoadTagFile(file, loadtype);

   // Post-load all database parts that weren't loaded
   if (~retval & kFiletypeAll)
   {
      DispatchData postload = { kDatabasePostLoad | (kFiletypeAll & ~retval), NULL};
      DispatchMsg(kMsgDatabase,&postload);
   }

   // send a generic post load
   DispatchData postload = { kDatabasePostLoad, NULL};
   DispatchMsg(kMsgDatabase,&postload);

   // Suspend the sim
   AutoAppIPtr_(SimManager,pSim); 
   pSim->SuspendSim();

   return retval; 

}


edbFiletype dbMergeLoadTagFile(ITagFile* file, edbFiletype loadtype)
{
   // The actual type in the file, rather than just whether we've loaded it.
   ulong Filetype = 0;

   //  filenames of the various pieces, corresponding to load order
   char pieceNames[LOAD_ORDER_PHASES][256];
   // and whether we've found them 
   BOOL pieceFound[LOAD_ORDER_PHASES]; 
   memset(pieceFound,0,sizeof(pieceFound));
   
   // Figure out what kinds of stuff are in the file
   if (!read_file_type(file,&Filetype))
   {
      // @TODO: this is back-compatibility stuff, remove when obsolete
      // look for a mission header 
      if (ReadMissionHeader(file,NULL,pieceNames[0]))
      {
         Filetype = MISSION_FILE_TYPE;
         pieceFound[0] = TRUE;
      }

#ifdef NO_MISSION_HEADER_OR_FILETYPE
      else
      {
         Filetype = guess_Filetype_from_extension(fullname);
      }
#else
      else
      {
         // @HACK: I guess we're a cow file 
         Filetype = kFiletypeAll; 
      }
#endif 
   }

   edbFiletype retval = 0;

   // burn through the load order. 
   for (int i = 0; i < LOAD_ORDER_PHASES; i++)
   {
      char msgbuf[256];

      // what piece are we loading now?
      int piece = load_order[i] ;

      // only load the pieces we want
      piece &= loadtype;

      if (piece == 0)
         continue; 

      // okay, we're loading another file:
      gCurrFilenum++;

      // where to load it from
      ITagFile* use_file = NULL;

      // send status message
      sprintf(msgbuf,"Loading %s",phase_names[i]);
      Status(msgbuf);

      // do we have it in the file? 
      // Note: the & DB_PARTITION_MASK is for back-compatibility . 
      // old files don't have obj partitions in their type. 
      if ((Filetype & (piece & DB_PARTITION_MASK)) == (piece & DB_PARTITION_MASK))
      {
         file->AddRef();
         use_file = file;
      }
      else // look for it in the file ref tag
      {
         BOOL success = FALSE;
         char fullpiecename[256] = "";

         pieceFound[i] = pieceFound[i] || read_file_ref_tag(file,piece,pieceNames[i]);
         // we found a file, now lets try to open it
         if (pieceFound[i])
         {
            BOOL found = find_for_load(fullpiecename,pieceNames[i]);
            if (!found && special_file_find != NULL)
               found = special_file_find(piece,pieceNames[i],fullpiecename);

            if (found)
            {
               use_file = dbOpenFile(fullpiecename, kTagOpenRead);
               success = use_file != NULL;

               strcpy(pieceNames[i],strip_pathname(fullpiecename));
            }
         }
         if (!success)
         {
            sprintf(msgbuf,"Failed to open %s file %s",phase_names[i],fullpiecename);
            Status(msgbuf);
            retval = 0;
            break;
         }
      }

      // now use_file is valid.  Load the our piece from it
      DispatchData msg = { kDatabaseLoad | piece, use_file};
      DispatchMsg(kMsgDatabase,&msg);

      SafeRelease(use_file);

      if (piece & kDBMission)
         dbPatchSetMission(strip_pathname(pieceFound[i] ? pieceNames[i] : gCurrFilename), piece);
      sDbPatchEntry *pPostLoad = NULL;
      dbPatchRequest(&pPostLoad, strip_pathname(pieceFound[i] ? pieceNames[i] : gCurrFilename), piece, NULL,
                     kDbPatchLoad, NULL);

      // Now send a post-load for that piece
      msg.subtype = kDatabasePostLoad | piece;
      msg.data = NULL;
      DispatchMsg(kMsgDatabase,&msg);

      dbPatchApplyQueue(NULL, pPostLoad, piece, kDbPatchPostLoad, NULL);
      dbPatchFreeList(&pPostLoad);
      retval |= piece;
   }



   dbPatchLoadDone();

   if (pieceFound[0])
   {
      // So user knows what gamesys he is using.
      set_gamesys_file(pieceNames[0]);
   }


   return retval;
}

////////////////////////////////////////

void dbReset(void)
{
   gCurrFilenum = 0;

   DispatchData msg = { kDatabaseReset, NULL};
   DispatchMsg(kMsgDatabase,&msg);
   
   reset_objid_space();

   // Clear out current game file.
   set_gamesys_file("");

   StatusField(SF_FILENAME, "");

   // This might want to move out.
   if (config_is_defined(LOAD_FILE_VAR))
      config_unset(LOAD_FILE_VAR);
}


void dbBuildDefault(void)
{
   DispatchData msg = { kDatabaseDefault, NULL};
   dbReset();
   DispatchMsg(kMsgDatabase,&msg);
}

//------------------------------------------------------------
// What's the current file number?
// Mainly interesting to the network code

int dbCurrentFilenum()
{
   return gCurrFilenum;
}

////////////////////////////////
// look, IFDEF EDITOR, dont put game code below this!!!

#ifdef EDITOR

////////////////////////////////////////////////////////////
// COMMANDS
//

//
// File dialog filters
//

static sFileMenuFilter db_file_save_filters[] = 
{
   { "DromEd Mission Files (*.mis)", "*.mis"},
   { "DromEd World Files (*.cow)", "*.cow"},
   { "DromEd GameSys Files (*.gam)", "*.gam"},
   { "DromEd Save Files (*.cow, *.mis, *.gam)", "*.mis;*.cow;*.gam"},
   { "All Files (*.*)", "*.*"},
   { NULL }
};

static sFileMenuFilter db_file_load_filters[] = 
{
   { "DromEd Save Files (*.cow, *.mis, *.gam)", "*.mis;*.cow;*.gam"},
   { "All Files (*.*)", "*.*"},
   { NULL }
};

// little helper for getting the filename

// ok, hack to try and modularize this
// the idea is you either are passing in a string
// or passing null and want the user to pick something

static char _maybe_buf[256]="";  // since it will be used for null inputs only

// @TODO: have a way to give a base filename that isnt null to this
static char *get_filename_maybe_dialog(char *str, eFileMenuMode fMode, const char *type_ext)
{
   if (str != NULL)
      while (isspace(*str)) str++;
   if (str == NULL || *str == '\0')
   {
      sFileMenuFilter* filter = (fMode == kFileMenuSave) ? db_file_save_filters : db_file_load_filters; 
      str=FileDialog(fMode, filter, type_ext, _maybe_buf, sizeof(_maybe_buf));
      if (str == NULL || *str == '\0')
         return NULL;
   }
   return str;
}

void dbIncrementalBackup(const char *pszFile);

static void save_cmd_guts(char* filename, edbFiletype ftype)
{
   const char *type_ext=extension_for_filetype(ftype);
   filename=get_filename_maybe_dialog(filename,kFileMenuSave,type_ext);
   if (filename==NULL) return;

   const char* ext = strrchr(filename,'.');
   if (ext == NULL)
   {
      strcat(filename,".");
      strcat(filename,type_ext);
      ext = type_ext;
   }
   else
   { 
      ext++;
      if (stricmp(ext,type_ext) != 0) // different extentions
      {
         char statbuf[256];
         sprintf(statbuf,"Using .%s extension instead of standard .%s",ext,type_ext);
         Status(statbuf);
      }
   }

   // copy fname into our local buffer
   char filebuf[256];
   munge_path(filebuf,filename);
   filebuf[sizeof(filebuf)-1] = '\0';
   filename = filebuf;

   char backup[16];
   sprintf(backup,"backup.%s",ext);
   CopyFileNamed(backup,filename);   
   dbIncrementalBackup(filename);

   edbFiletype result = dbSave(filename,ftype);
   // So user knows what file he just saved.
   StatusField(SF_FILENAME, (char*)strip_pathname(filename));

   if (result > 0)
   {
      Status("Save successful");
      set_file_var(SAVE_FILE_VAR,filename,TRUE);
      set_file_var(LOAD_FILE_VAR,filename,FALSE);
   }
   else
      Status("Save Failed");
}

//
// Last file sim time check
//

static BOOL safe_to_save(void)
{
   int delta = GetSimTime() - GetSimFileTime(); 
   if (delta != 0)
   {
      mprintf("You are trying to save a mission in which sim time has run.\n If you are in full screen hit space or enter to cancel.\n");
      char msg[256];
      sprintf(msg,"The simulation has run for %.02f seconds since this file was last saved or loaded.\nYour file will need *immediate* surgery in order to be used as a shipping level\nSave anyway?",delta*1.0/SIM_TIME_SECOND); 

      return winui_GetYorN(msg); 
   }
   return TRUE; 
}


EXTERN void save_world(char* arg)
{
   if (safe_to_save())
      save_cmd_guts(arg,kFiletypeAll);
}

static void save_mission(char* arg)
{
   if (safe_to_save())
      save_cmd_guts(arg,MISSION_FILE_TYPE);
}

static void save_gamesys(char* arg)
{
   if (safe_to_save())
      save_cmd_guts(arg,kFiletypeGAM);
}



//---------------------------------------------------------------------
// Lets the user choose a .GAM file for the current mission.
//---------------------------------------------------------------------
static void set_gamesys(char *filename)
{  // how to do suffix
   filename=get_filename_maybe_dialog(filename,kFileMenuSave,DB_GAMESYS_SUFFIX);
   if (filename==NULL) return;

   char *ext = strchr(filename, '.');
   if (ext == NULL)
      strcat(filename, "."DB_GAMESYS_SUFFIX);
   set_gamesys_file((char *)strip_pathname(filename));
}

//------------------------------------------------------------
// Loading commands 
//

#ifndef SHIP
static int  ld_snd_hnd=SFX_NO_HND;

static void secret_load_start_sounds(void)
{
   if (config_is_defined("load_snds"))
   {
      sfx_parm loop_parm={64,SFXFLG_LOOP,0,0,0,100,0,0,NULL};
      char snd_name[128];

      if (!config_get_raw("load_snds",snd_name,128))
         strcpy(snd_name,"load_snd");
      if (snd_name[0]=='\0')
         strcpy(snd_name,"load_snd");
      if (strnicmp(snd_name,"null",4))  // if we arent null
         ld_snd_hnd=SFX_Play_Raw(SFX_STATIC, &loop_parm, snd_name);
   }
}

static void secret_load_end_sounds(void)
{
   if (ld_snd_hnd!=SFX_NO_HND)
   {
      SFX_Kill_Hnd(ld_snd_hnd);
      ld_snd_hnd=SFX_NO_HND;
   }
}
#else   // SHIP
#define secret_load_start_sounds()
#define secret_load_end_sounds()
#endif

EXTERN void load_brushes(char* str);

static BOOL load_cmd_file_find(ulong filetype, const char* filename, char* buf)
{
   char msgbuf[256];
   // find a good phase name
   for (int i = 0; i < LOAD_ORDER_PHASES; i++)
      if (load_order[i] & filetype)
         break;
   if (i < LOAD_ORDER_PHASES)
      sprintf(msgbuf,"Can't find %s file '%s'.\nBrowse?",phase_names[i],filename);
   else
      sprintf(msgbuf,"Can't find file '%s'.\nBrowse?",filename);
      
   if (winui_GetYorN(msgbuf))
   {
      char* result=FileDialog(kFileMenuLoad, db_file_load_filters, extension_for_filetype(filetype), _maybe_buf, sizeof(_maybe_buf));
      strcpy(buf,_maybe_buf);
      return result != NULL && *result != '\0';
   }
   return FALSE;
}

static void load_cmd_guts(char* filename, edbFiletype ftype)
{
   filename=get_filename_maybe_dialog(filename,kFileMenuLoad,extension_for_filetype(ftype));
   if (filename==NULL) return;

   // Look for the file in likely places with likely extensions
   BOOL found = FALSE;
   char filebuf[256];
   char* ext = strrchr(filename,'.');
   if (ext == NULL)  // No extension case
   {
      edbFiletype foundtype = find_for_load_extensionless(filebuf,filename);
      if (foundtype > 0)
      {
         found = TRUE;
      }
   }

   if (!found)
      found = find_for_load(filebuf,filename);
   if (!found)
   {
      char statbuf[256];
      sprintf(statbuf,"can't find %s",filename);
      Status(statbuf);  
      return;
   }

   secret_load_start_sounds();   

   if (!config_is_defined("no_inprog"))
      dbSave("inprog." DB_FILE_SUFFIX, kFiletypeAll);

   filename = filebuf;
   find_file_func oldfunc = special_file_find;
   special_file_find = load_cmd_file_find;
   edbFiletype result = dbLoad(filename,ftype);
   special_file_find = oldfunc;
   if (result > 0)
   {
      Status("Load successful");
      //      set_file_var(SAVE_FILE_VAR,filename,TRUE);
      set_file_var(LOAD_FILE_VAR,filename,TRUE);
   }
   else
      Status("Load Failed");
   secret_load_end_sounds();  
}

//---------------------------------------------------------------------
// For compatability, checks input before loading the .COW file.
//---------------------------------------------------------------------

EXTERN void load_file(char *str)
{
   load_cmd_guts(str,kFiletypeAll);
}

EXTERN void load_gamesys(char* str)
{
   char buf[256];
   attachPostfix(buf,str,"."DB_GAMESYS_SUFFIX);
   load_cmd_guts(buf,kFiletypeGAM);
   set_gamesys_file(buf); 
}

// for init_edit
EXTERN void new_world(void)
{
   char buf[80];
   if (config_get_raw("default_gamesys",buf,sizeof(buf)))
   {
      char fname[256];
      attachPostfix(fname,buf,"."DB_GAMESYS_SUFFIX);
      char fpath[256]; 
      if (find_for_load(fpath,fname))
      {
         dbLoad(fpath,kFiletypeGAM);
         set_gamesys_file(fpath); 
         return; 
      }
   }
   dbBuildDefault(); 
}

//////////////////////////////////////////

//
// Debugging command for dispatch
//

#pragma off(unreferenced)
void dispatch_noise(char* arg)
{
#ifndef SHIP
   ILoop * pLoop = AppGetObj(ILoop);

   uint diags;
   ulong messes;

   pLoop->GetDiagnostics(&diags, &messes);
   
   ulong bits;
   sscanf(arg,"%lx",&bits);

   messes ^= bits;
   mprintf("Loopmode noise now %X\n",messes);
   pLoop->SetDiagnostics(diags,messes);
   SafeRelease(pLoop);

#endif
}
#pragma on(unreferenced)

////////////////////////////////////////

Command dbfile_commands[] = 
{
   { "save_cow",  FUNC_STRING, save_world, "Save world to file" }, 
   { "save_mission",  FUNC_STRING, save_mission, "Save mission to file" }, 
   { "save_gamesys", FUNC_STRING, save_gamesys, "Save gamesys info only" }, 
   { "load_file", FUNC_STRING, load_file, "Load .MIS, .COW, or .GAM files" },
   { "load_gamesys", FUNC_STRING, load_gamesys, "Load .GAM file - Resets the level" },
   { "set_gamesys", FUNC_STRING, set_gamesys, "Set gamesys file for current mission" }, 
   { "clear_world", FUNC_VOID,   dbBuildDefault, "Reset to the empty world"},
   { "dispatch_noise",FUNC_STRING, dispatch_noise, "Toggle dispatch noise for hex message mask\n"},
   { "file_menu", FUNC_VOID, FileMenu, "Bring up the file menu" },
   { "new_world", FUNC_VOID, new_world, "Clear the world and load the gamesys" },
};
#endif // ~EDITOR

void dbfile_setup_commands(void)
{
#ifdef EDITOR
   COMMANDS(dbfile_commands,HK_ALL);
#endif
}

#define DBPATCH_LOG(level, fmt, ...)                                      \
   do {                                                                   \
      if (g_dbPatchLogLevel >= level && g_dbPatchLogFile)                 \
         lg_fprintf(g_dbPatchLogFile, fmt, __VA_ARGS__);                  \
   } while (0)

#define DBPATCH_ERROR(fmt, ...)                                           \
   do {                                                                   \
      if (g_dbPatchLogLevel > 0 && g_dbPatchLogFile)                      \
         lg_fprintf(g_dbPatchLogFile, fmt, __VA_ARGS__);                  \
   } while (0)

#define DBPATCH_INFO  2
#define DBPATCH_DIAG  5
#define DBPATCH_TRACE 10



int g_dbModMode;
int g_dbPatchLogLevel;
BOOL g_dbPatchRemapLinks;
static LGFILE *g_dbPatchLogFile;
static int g_dbPatchLogState = 1;
static char g_dbPatchTokEnd;

struct sDbPatchEntry
{
   sDbPatchEntry(const char *name, const char *path, sDbPatchEntry **ppList);

   cAnsiStr name;
   cAnsiStr path;
   sDbPatchEntry *next;
};

sDbPatchEntry::sDbPatchEntry(const char *pszName, const char *pszPath, sDbPatchEntry **ppList)
{
   name = pszName;
   if (pszPath)
      path = pszPath;
   next = NULL;
   if (*ppList == NULL)
      *ppList = this;
   else
   {
      sDbPatchEntry *p = *ppList;
      while (p->next)
         p = p->next;
      p->next = this;
   }
}

static void dbPatchFreeList(sDbPatchEntry **ppList)
{
   sDbPatchEntry *p = *ppList;
   while (p)
   {
      sDbPatchEntry *next = p->next;
      delete p;
      p = next;
   }
   *ppList = NULL;
}

static void dbPatchOpenLog()
{
   char cwd[_MAX_PATH];

   if (g_dbPatchLogLevel <= 0)
      return;
#line 1945
   AssertMsg(g_dbPatchLogFile == NULL, "dbPatchLogFile == NULL");
   g_dbPatchLogFile = lg_fopen("dbmod.log", g_dbPatchLogState == 1 ? "w" : "a");
   if (g_dbPatchLogState == 1 && g_dbPatchLogFile)
   {
      g_dbPatchLogState = 2;
      DBPATCH_LOG(DBPATCH_DIAG, "DIAG: cwd is '%s'\n\n", _getcwd(cwd, _MAX_PATH));
   }
}

static void dbPatchCloseLog()
{
   if (g_dbPatchLogFile)
   {
      lg_fclose(g_dbPatchLogFile);
      g_dbPatchLogFile = NULL;
   }
}

static BOOL dbPatchSkipSpace(char **ppText)
{
   for (;;)
   {
      while (**ppText && isspace((uchar)**ppText))
         ++*ppText;
      char *p = *ppText;
      if (*p != '/')
         return *p != '\0';
      if (p[1] == '/')
      {
         *ppText = p + 2;
         while (**ppText && **ppText != '\r' && **ppText != '\n')
            ++*ppText;
      }
      else if (p[1] == '*')
      {
         do
         {
            *ppText = p + 2;
            while (**ppText && **ppText != '*')
               ++*ppText;
            p = *ppText;
         } while (*p && p[1] != '/');
         if (*p == '*')
            *ppText = p + 2;
      }
      else
         return TRUE;
   }
}

static BOOL dbPatchToken(char **ppText, char **ppToken, BOOL *pQuoted)
{
   *ppToken = NULL;
   if (!dbPatchSkipSpace(ppText))
      return FALSE;
   char *start = *ppText;
   if (*start != '"')
   {
      while (**ppText && !isspace((uchar)**ppText))
         ++*ppText;
      g_dbPatchTokEnd = **ppText;
      **ppText = '\0';
      ++*ppText;
      *ppToken = start;
      return TRUE;
   }
   if (pQuoted)
      *pQuoted = TRUE;
   start = ++*ppText;
   while (**ppText && **ppText != '"')
      ++*ppText;
   if (!**ppText)
      return FALSE;
   g_dbPatchTokEnd = **ppText;
   **ppText = '\0';
   ++*ppText;
   *ppToken = start;
   return TRUE;
}

static BOOL dbPatchFieldToken(char **ppText, char **ppToken, int *pIndex)
{
   *ppToken = NULL;
   *pIndex = 0;
   if (!dbPatchSkipSpace(ppText))
      return FALSE;
   char *start = *ppText;
   if (*start != '"')
   {
      while (**ppText && !isspace((uchar)**ppText))
         ++*ppText;
      g_dbPatchTokEnd = **ppText;
      **ppText = '\0';
      ++*ppText;
      *ppToken = start;
      return TRUE;
   }
   start = ++*ppText;
   while (**ppText && **ppText != '"')
      ++*ppText;
   if (!**ppText)
      return FALSE;
   g_dbPatchTokEnd = **ppText;
   **ppText = '\0';
   ++*ppText;
   *ppToken = start;
   if (**ppText != '[')
      return TRUE;
   char *index = ++*ppText;
   while (**ppText && **ppText != ']')
   {
      if (!isdigit((uchar)**ppText))
      {
         DBPATCH_ERROR("SYNTAX ERROR: invalid [] indexing syntax\n");
         return FALSE;
      }
      ++*ppText;
   }
   if (!**ppText)
      return FALSE;
   g_dbPatchTokEnd = **ppText;
   **ppText = '\0';
   ++*ppText;
   *pIndex = atoi(index);
   return TRUE;
}

static BOOL dbPatchValue(char **ppText, char **ppValue, BOOL bOptional)
{
   if (g_dbPatchTokEnd != '\r' && g_dbPatchTokEnd != '\n' && g_dbPatchTokEnd != '\0')
   {
      if (**ppText)
         while (isspace((uchar)**ppText) && **ppText != '\r' && **ppText != '\n' && *++*ppText)
            ;
      char c = **ppText;
      if (c == '\0')
      {
         DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
         return FALSE;
      }
      if (c != '\r' && c != '\n')
      {
         *ppValue = *ppText;
         char *end = *ppText;
         BOOL bInQuote = FALSE;
         while (**ppText && **ppText != '\r' && **ppText != '\n')
         {
            char *p = *ppText;
            if (*p == '"')
               bInQuote = !bInQuote;
            else if (!bInQuote && p[0] == '/' && (p[1] == '/' || p[1] == '*'))
            {
               *p = '\0';
               dbPatchSkipSpace(ppText);
               break;
            }
            ++*ppText;
            ++end;
         }
         **ppText = '\0';
         ++*ppText;
         uchar *last = (uchar *)end - 1;
         while (last >= (uchar *)*ppValue && isspace(*last))
            *last-- = '\0';
         if (**ppValue == '"' && (char *)last > *ppValue && *last == '"')
         {
            ++*ppValue;
            *last = '\0';
            return TRUE;
         }
         if (**ppValue)
            return TRUE;
      }
   }
   if (!bOptional)
   {
      DBPATCH_ERROR("SYNTAX ERROR: parameter value missing\n");
      return FALSE;
   }
   *ppValue = NULL;
   return TRUE;
}

static const sFieldDesc *dbPatchFindField(const sStructDesc *pDesc, const char *pszName, int index)
{
   for (int i = 0; i < pDesc->nfields; i++)
   {
      if (!strnicmp(pszName, pDesc->fields[i].name, sizeof(pDesc->fields[i].name)))
      {
         if (index == 0)
            return &pDesc->fields[i];
         index--;
      }
   }
   return NULL;
}

static ObjID dbPatchRemapObj(ObjID obj)
{
   if (g_dbModMode && obj > 0)
   {
      ObjID newObj = ObjRemapOnLoad(obj);
      if (newObj != obj)
         DBPATCH_LOG(DBPATCH_DIAG, "INFO:     remapped object id %d to %d\n", obj, newObj);
      return newObj;
   }
   return obj;
}

static BOOL dbPatchCheckObj(ObjID obj, ulong flags, BOOL bAllowAbstract)
{
   if (obj == OBJ_NULL)
   {
      DBPATCH_ERROR("WARNING: invalid object id 0, skipping\n");
      return FALSE;
   }
   if (obj > 0)
   {
      if (flags & kDBMission)
         return TRUE;
      DBPATCH_ERROR("WARNING: incorrect dbmod context for concrete objects (ObjID %d), skipping\n", obj);
      return FALSE;
   }
   if (bAllowAbstract || (flags & kDBGameSys))
      return TRUE;
   DBPATCH_ERROR("WARNING: incorrect dbmod context for abstract objects (ObjID %d), skipping\n", obj);
   return FALSE;
}

static int dbPatchFindFiletypes(ulong mask, ulong *pFound)
{
   ulong found = 0;
   if (pFound)
   {
      if (*pFound != (ulong)-1)
         return *pFound & mask & (kDBMission | kDBGameSys);
      *pFound = 0;
   }
   const char *pattern = "*.*";
   if (!pFound)
   {
      if ((mask & (kDBMission | kDBGameSys)) == kDBMission)
         pattern = "*.mis";
      else if ((mask & (kDBMission | kDBGameSys)) == kDBGameSys)
         pattern = "*.gam";
   }
   Datapath dp;
   memset(&dp, 0, sizeof(dp));
   DatapathAdd(&dp, *g_szFMPath ? g_szFMPath : NULL);
   DatapathDir *dir = DatapathOpenDir(&dp, (char *)pattern, 0);
   char *name;
   while ((name = DatapathReadDir(dir)) != NULL)
   {
      char *ext = strrchr(name, '.');
      if (!ext)
         continue;
      if (!stricmp(ext + 1, "mis"))
         found |= kDBMission;
      else if (!stricmp(ext + 1, "gam"))
         found |= kDBGameSys;
      if (pFound)
      {
         *pFound = found;
         if (found == (kDBMission | kDBGameSys))
            break;
      }
      else if (mask & (kDBMission | kDBGameSys) & found)
         break;
   }
   DatapathCloseDir(dir);
   DatapathFree(&dp);
   return found & mask & (kDBMission | kDBGameSys);
}

static BOOL dbPatchParse(BOOL *pbQueued, sDbPatchEntry **ppScripts, char *pText, ulong flags, int pass);
static BOOL dbPatchFingerprint(char *pText, ulong flags, sDbPatchEntry **ppScripts);

static int dbPatchLoadFile(const char *pszName, const char *pszPath, ulong flags, int pass,
                           BOOL *pbQueued, sDbPatchEntry **ppScripts)
{
   if (!pszPath)
      pszPath = "";
   DBPATCH_LOG(DBPATCH_DIAG, "DIAG: looking for file '%s' in path '%s' (%x)\n", pszName, pszPath, flags);

   AutoAppIPtr(ResMan);
   IStore *pStore = pResMan->GetStore(pszPath);
   if (!pStore)
   {
      DBPATCH_LOG(DBPATCH_DIAG, "DIAG:     path not found\n");
      return -1;
   }
   IStoreStream *pStream = ResOpenStoreStream(pStore, pszName);
   pStore->Release();
   if (!pStream)
   {
      DBPATCH_LOG(DBPATCH_DIAG, "DIAG:     file not found\n");
      return -1;
   }
   DBPATCH_ERROR("\nINFO: found file '%s' in path '%s', loading... (%x)\n", pszName, pszPath, flags);

   char *pData = NULL;
   BOOL bRead = FALSE;
   long size = pStream->GetSize();
   if (size)
   {
      pData = new char[size + 2];
#line 5309
      AssertMsg(pData != NULL, "data != NULL");
      bRead = pStream->Read(size, pData) == size;
      pData[size] = '\0';
      pData[size + 1] = '\0';
   }
   pStream->Close();
   pStream->Release();

   if (!size)
   {
      DBPATCH_LOG(DBPATCH_INFO, "INFO:     file was empty\n");
      return pass != kDbPatchEnum;
   }
   if (!bRead)
   {
      DBPATCH_ERROR("WARNING: file '%s' in path '%s' failed to load\n", pszName, pszPath);
      delete[] pData;
      return 0;
   }
   if (strncmp(pData, "DML1", 4))
   {
      DBPATCH_ERROR("SYNTAX ERROR: file '%s' in path '%s' does not have correct format (invalid header)\n", pszName, pszPath);
      delete[] pData;
      return 0;
   }

   g_dbPatchRemapLinks = (flags & kDBMission) != 0;
   int result;
   if (pass == kDbPatchEnum)
   {
      result = dbPatchFingerprint(pData + 4, flags, ppScripts) != 0;
      if (!result)
         DBPATCH_LOG(DBPATCH_INFO, "INFO: patch rejected by fingerprint identification\n");
      else
      {
         *pbQueued = TRUE;
         DBPATCH_LOG(DBPATCH_INFO, "INFO: patch queued\n");
      }
   }
   else
   {
      result = dbPatchParse(pbQueued, ppScripts, pData + 4, flags, pass);
      if (!result)
         DBPATCH_ERROR("ERROR: file '%s' in path '%s' failed to parse correctly\n", pszName, pszPath);
      else
         DBPATCH_LOG(DBPATCH_INFO, "INFO: patch done\n");
   }
   delete[] pData;
   return result;
}

static void dbPatchQueueFound(int result, BOOL bQueued, int pass, const char *pszName, const char *pszPath,
                              sDbPatchEntry **ppQueue)
{
   if (pass == kDbPatchEnum)
   {
      if (result > 0)
         new sDbPatchEntry(pszName, pszPath, ppQueue);
   }
   else if (bQueued && ppQueue)
      new sDbPatchEntry(pszName, pszPath, ppQueue);
}

static void dbPatchRequest(sDbPatchEntry **ppQueue, const char *pszFile, ulong flags, ulong *pFound,
                           int pass, sDbPatchEntry **ppScripts)
{
   char name[256];
   BOOL bQueued;
   int result;

   if (!g_dbModMode || !pszFile || !*pszFile || (pass == kDbPatchEnum && !ppQueue))
      return;
   if ((flags & kDBSaveGame) && g_dbModMode < 2)
   {
      if (g_dbPatchLogLevel > 4 && pass != kDbPatchScripts && pass != kDbPatchEnum)
      {
         dbPatchOpenLog();
         DBPATCH_LOG(DBPATCH_DIAG, "DIAG: patch request for savegame ignored, '%s' (%x)\n", pszFile, flags);
         dbPatchCloseLog();
      }
      return;
   }
   strcpy(name, pszFile);
   strcat(name, ".dml");

   dbPatchOpenLog();
   if (pass == kDbPatchScripts)
      DBPATCH_LOG(DBPATCH_DIAG, "DIAG: processing script dependencies of patches for '%s' (%x)\n", pszFile, flags);
   else if (pass == kDbPatchEnum)
      DBPATCH_LOG(DBPATCH_DIAG, "DIAG: enumerating patches for '%s' (%x)\n", pszFile, flags);
   else
      DBPATCH_LOG(DBPATCH_DIAG, "DIAG: patch request for '%s' (%x)\n", pszFile, flags);

   Datapath *pMods = g_pModDatapath;
   BOOL bSearchLoadPath = g_iModDatapathFM < 0;
   if (!bSearchLoadPath)
   {
      const char *pszPath = pMods->datapath[g_iModDatapathFM];
      bQueued = FALSE;
      result = dbPatchLoadFile(name, pszPath, flags, pass, &bQueued, ppScripts);
      dbPatchQueueFound(result, bQueued, pass, name, pszPath, ppQueue);
      if (result < 0 && !dbPatchFindFiletypes(flags, pFound))
         bSearchLoadPath = TRUE;
   }
   if (bSearchLoadPath)
   {
      bQueued = FALSE;
      result = dbPatchLoadFile(name, NULL, flags, pass, &bQueued, ppScripts);
      if (result < 0)
      {
         Datapath dp;
         char buf[256];
         result = 0;
         memset(&dp, 0, sizeof(dp));
         if (config_get_raw(LOAD_PATH_VAR, buf, sizeof(buf)))
            DatapathAdd(&dp, buf);
         if (dp.numDatapaths)
            DBPATCH_LOG(DBPATCH_DIAG, "DIAG: searching in \"load_path\" paths...\n");
         for (int i = 0; i < dp.numDatapaths; i++)
         {
            const char *pszPath = dp.datapath[i];
            if (pszPath[0] == '.' && (pszPath[1] == '\0' || ((pszPath[1] == '/' || pszPath[1] == '\\') && pszPath[2] == '\0')))
               continue;
            bQueued = FALSE;
            if (dbPatchLoadFile(name, pszPath, flags, pass, &bQueued, ppScripts) > 0)
            {
               if ((bQueued && ppQueue) || pass == kDbPatchEnum)
                  new sDbPatchEntry(name, pszPath, ppQueue);
               result = 1;
               break;
            }
         }
         DatapathFree(&dp);
      }
      else
         dbPatchQueueFound(result, bQueued, pass, name, NULL, ppQueue);
   }

   if (pMods && pMods->numDatapaths)
   {
      DBPATCH_LOG(DBPATCH_DIAG, "DIAG: searching FM/mod paths for patches...\n");
      for (int i = pMods->numDatapaths - 1; i >= 0; i--)
      {
         if (i == g_iModDatapathFM)
            continue;
         bQueued = FALSE;
         int r = dbPatchLoadFile(name, pMods->datapath[i], flags, pass, &bQueued, ppScripts);
         if (r == 0)
            result = 0;
         dbPatchQueueFound(r, bQueued, pass, name, pMods->datapath[i], ppQueue);
      }
   }

   if (pass == kDbPatchEnum)
      DBPATCH_LOG(DBPATCH_DIAG, "DIAG: ------ patch enum request finished ------\n\n");
   else
      DBPATCH_LOG(DBPATCH_DIAG, "DIAG: ------ patch request finished ------\n\n");
   dbPatchCloseLog();
}

static BOOL dbPatchApplyQueue(sDbPatchEntry **ppQueue, sDbPatchEntry *pList, ulong flags, int pass,
                              sDbPatchEntry **ppScripts)
{
   if (!g_dbModMode || !pList || ((flags & kDBSaveGame) && g_dbModMode < 2))
      return FALSE;

   dbPatchOpenLog();
   if (pass == kDbPatchPostLoad)
   {
      DBPATCH_LOG(DBPATCH_DIAG, "DIAG: applying post-load patch components\n");
      ppQueue = NULL;
   }
   else
      DBPATCH_LOG(DBPATCH_DIAG, "DIAG: applying patch queue\n");

   BOOL bOk = TRUE;
   for (sDbPatchEntry *p = pList; p; p = p->next)
   {
      BOOL bQueued = FALSE;
      const char *pszPath = p->path.GetLength() ? (const char *)p->path : NULL;
      if (!dbPatchLoadFile(p->name, pszPath, flags, pass, &bQueued, ppScripts))
         bOk = FALSE;
      if (bQueued && ppQueue)
         new sDbPatchEntry(p->name, p->path.GetLength() ? (const char *)p->path : NULL, ppQueue);
   }

   if (pass == kDbPatchPostLoad)
      DBPATCH_LOG(DBPATCH_DIAG, "DIAG: ------ post-load patches finished ------\n\n");
   else
      DBPATCH_LOG(DBPATCH_DIAG, "DIAG: ------ patch queue finished ------\n\n");
   dbPatchCloseLog();
   return bOk;
}

static BOOL dbPatchParseInt(const char *pszToken, int *pValue)
{
   const char *p = pszToken;
   if (*p == '-' || *p == '+')
      p++;
   for (; *p; p++)
   {
      if (!isdigit((uchar)*p))
      {
         DBPATCH_ERROR("SYNTAX ERROR: expected int val (%s)\n", pszToken);
         return FALSE;
      }
   }
   *pValue = atoi(pszToken);
   return TRUE;
}

static BOOL dbPatchParseObjID(const char *pszToken, ObjID *pObj)
{
   int obj;
   if (!dbPatchParseInt(pszToken, &obj))
      return FALSE;
   *pObj = dbPatchRemapObj(obj);
   return TRUE;
}

enum eDbPatchCompare
{
   kDbPatchEQ,
   kDbPatchNE,
   kDbPatchLT,
   kDbPatchGT,
   kDbPatchLE,
   kDbPatchGE
};

static const char *g_apszDbPatchCompare[] = { "==", "!=", "<", ">", "<=", ">=" };

static BOOL dbPatchFingerprintQVar(char **ppText, BOOL bNot, BOOL *pbMatch, IQuestData *pQuest)
{
   char *pszName;
   char *pszTok;
   int mask = -1;
   int op;
   int value;

   *pbMatch = FALSE;
   if (!dbPatchToken(ppText, &pszName, NULL))
   {
      DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
      return FALSE;
   }
   if (!pszName || !*pszName)
   {
      DBPATCH_ERROR("SYNTAX ERROR: expected qvar name\n");
      return FALSE;
   }
   BOOL bExists = pQuest->ExistsType(pszName, kQuestDataMission);
   if (!bNot)
   {
      if (!bExists)
      {
         DBPATCH_LOG(12, "DIAG:         fingerprint did not match, mission qvar \"%s\" does not to exist but is required\n", pszName);
         return TRUE;
      }
   }
   else if (!bExists)
   {
      *pbMatch = TRUE;
      return TRUE;
   }

   while (**ppText && isspace((uchar)**ppText))
      ++*ppText;
   if (**ppText == '\0')
   {
      if (bNot)
      {
         DBPATCH_LOG(12, "DIAG:         fingerprint did not match, mission qvar \"%s\" exists but isn't allowed to exist\n", pszName);
         return TRUE;
      }
      *pbMatch = TRUE;
      return TRUE;
   }

   if (**ppText == '&')
   {
      ++*ppText;
      if (!dbPatchToken(ppText, &pszTok, NULL))
      {
         DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
         return FALSE;
      }
      if (!dbPatchParseInt(pszTok, &mask))
         return FALSE;
      while (**ppText && isspace((uchar)**ppText))
         ++*ppText;
   }

   char *p = *ppText;
   switch (*p)
   {
      case '=':
         *ppText = p + 1;
         if (p[1] != '=')
            goto bad_op;
         *ppText = p + 2;
         op = kDbPatchEQ;
         break;
      case '!':
         *ppText = p + 1;
         if (p[1] != '=')
            goto bad_op;
         *ppText = p + 2;
         op = kDbPatchNE;
         break;
      case '<':
         *ppText = p + 1;
         if (p[1] == '=')
         {
            *ppText = p + 2;
            op = kDbPatchLE;
         }
         else
            op = kDbPatchLT;
         break;
      case '>':
         *ppText = p + 1;
         if (p[1] == '=')
         {
            *ppText = p + 2;
            op = kDbPatchGE;
         }
         else
            op = kDbPatchGT;
         break;
      default:
      bad_op:
         DBPATCH_ERROR("SYNTAX ERROR: expected a comparison operator (\"==\" / \"!=\" / \"<\" / \">\" / \"<=\" / \">=\")\n");
         return FALSE;
   }

   if (!dbPatchToken(ppText, &pszTok, NULL))
   {
      DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
      return FALSE;
   }
   if (!dbPatchParseInt(pszTok, &value))
      return FALSE;

   int qvar = pQuest->Get(pszName);
   int masked = qvar & mask;
   switch (op)
   {
      case kDbPatchEQ: *pbMatch = masked == value; break;
      case kDbPatchNE: *pbMatch = masked != value; break;
      case kDbPatchLT: *pbMatch = masked < value; break;
      case kDbPatchGT: *pbMatch = masked > value; break;
      case kDbPatchLE: *pbMatch = masked <= value; break;
      case kDbPatchGE: *pbMatch = masked >= value; break;
   }
   if (!*pbMatch && g_dbPatchLogLevel >= 12)
   {
      if (mask == -1)
         DBPATCH_LOG(12, "DIAG:         fingerprint did not match, qvar \"%s\" (%d) did not satisfy: %s %d\n",
                     pszName, qvar, g_apszDbPatchCompare[op], value);
      else
         DBPATCH_LOG(12, "DIAG:         fingerprint did not match, qvar \"%s\" (%d & %d = %d) did not satisfy: %s %d\n",
                     pszName, qvar, mask, masked, g_apszDbPatchCompare[op], value);
   }
   return TRUE;
}

static BOOL dbPatchFingerprintObj(char **ppText, BOOL bGamesys, ObjID *pObj)
{
   char *pszTok;
   if (!dbPatchToken(ppText, &pszTok, NULL))
   {
      DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
      return FALSE;
   }
   if (!dbPatchParseObjID(pszTok, pObj))
      return FALSE;
   if (bGamesys && *pObj >= 0)
   {
      DBPATCH_ERROR("ERROR: incorrect dbmod context for concrete object (ObjID %d) in fingerprint\n", *pObj);
      return FALSE;
   }
   return TRUE;
}

static BOOL dbPatchFingerprintProp(BOOL bGamesys, char **ppText, BOOL bNot, BOOL *pbMatch, IPropertyManager *pPropMan)
{
   ObjID obj;
   char *pszProp;

   *pbMatch = FALSE;
   if (!dbPatchFingerprintObj(ppText, bGamesys, &obj))
      return FALSE;
   if (!dbPatchToken(ppText, &pszProp, NULL))
   {
      DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
      return FALSE;
   }
   if (!pszProp || !*pszProp)
   {
      DBPATCH_ERROR("SYNTAX ERROR: expected property name\n");
      return FALSE;
   }
   IProperty *pProp = pPropMan->GetPropertyNamed(pszProp);
   if (pProp->GetID() == PROPID_NULL)
   {
      DBPATCH_ERROR("ERROR: no property named \"%s\"\n", pszProp);
      SafeRelease(pProp);
      return FALSE;
   }
   BOOL bHas = pProp->IsSimplyRelevant(obj);
   if (!bNot)
   {
      if (bHas)
         *pbMatch = TRUE;
      else
         DBPATCH_LOG(12, "DIAG:         fingerprint did not match, object %d does not have property \"%s\" but is required\n", obj, pszProp);
   }
   else
   {
      if (!bHas)
         *pbMatch = TRUE;
      else
         DBPATCH_LOG(12, "DIAG:         fingerprint did not match, object %d has property \"%s\" but isn't allowed to\n", obj, pszProp);
   }
   SafeRelease(pProp);
   return TRUE;
}

static BOOL dbPatchFingerprintName(BOOL bGamesys, char **ppText, BOOL bNot, BOOL *pbMatch, IObjectSystem *pObjSys)
{
   ObjID obj;
   char *pszName;

   *pbMatch = FALSE;
   if (!dbPatchFingerprintObj(ppText, bGamesys, &obj))
      return FALSE;
   if (!dbPatchToken(ppText, &pszName, NULL))
   {
      DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
      return FALSE;
   }
   if (!pszName || !*pszName)
   {
      DBPATCH_ERROR("SYNTAX ERROR: expected object name\n");
      return FALSE;
   }
   const char *pszObjName = pObjSys->GetName(obj);
   BOOL bNamed = pszObjName && !stricmp(pszObjName, pszName);
   if (!bNot)
   {
      if (!bNamed)
      {
         DBPATCH_LOG(12, "DIAG:         fingerprint did not match, object %d is not named \"%s\" but is required\n", obj, pszName);
         return TRUE;
      }
   }
   else if (bNamed)
   {
      DBPATCH_LOG(12, "DIAG:         fingerprint did not match, object %d is named \"%s\" but isn't allowed to\n", obj, pszName);
      return TRUE;
   }
   *pbMatch = TRUE;
   return TRUE;
}

static BOOL dbPatchFingerprintGam(char **ppText, BOOL bNot, BOOL *pbMatch)
{
   char *pszName;

   *pbMatch = FALSE;
   if (!dbPatchToken(ppText, &pszName, NULL))
   {
      DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
      return FALSE;
   }
   if (!pszName || !*pszName)
   {
      DBPATCH_ERROR("SYNTAX ERROR: expected gamesys name\n");
      return FALSE;
   }
   const char *pszGam = *gCurrGameFile ? gCurrGameFile : NULL;
   BOOL bNamed = pszGam && !stricmp(pszGam, pszName);
   if (!bNot)
   {
      if (!bNamed)
      {
         DBPATCH_LOG(12, "DIAG:         fingerprint did not match, gamesys is not named \"%s\" but is required\n", pszName);
         return TRUE;
      }
   }
   else if (bNamed)
   {
      DBPATCH_LOG(12, "DIAG:         fingerprint did not match, gamesys is named \"%s\" but isn't allowed to\n", pszName);
      return TRUE;
   }
   *pbMatch = TRUE;
   return TRUE;
}

#define DBPATCH_MAX_FP_POS    15
#define DBPATCH_MIN_FP_POS    8
#define DBPATCH_FP_POS_SLACK  3
#define DBPATCH_FP_POS_TOLERANCE 3.0f

static BOOL dbPatchFingerprintPos(char *pszObj, char **ppText, BOOL *pbMatch, float tolerance, ObjID *pSeen, int nSeen)
{
   ObjID obj;
   char *pszTok;
   mxs_vector pos;

   *pbMatch = FALSE;
   if (!dbPatchParseObjID(pszObj, &obj))
      return FALSE;
   if (obj <= 0)
   {
      DBPATCH_ERROR("SYNTAX ERROR: object position fingerprint rule must specify a concrete object ID\n");
      return FALSE;
   }
   for (int i = 0; i < nSeen; i++)
   {
      if (pSeen[i] == obj)
      {
         DBPATCH_ERROR("SYNTAX ERROR: fingerprint contains duplicate object ID %d, no cheating allowed!\n", obj);
         return FALSE;
      }
   }
   while (**ppText && isspace((uchar)**ppText))
      ++*ppText;
   if (**ppText != '[')
   {
      DBPATCH_ERROR("SYNTAX ERROR: expected [\n");
      return FALSE;
   }
   ++*ppText;
   if (!dbPatchToken(ppText, &pszTok, NULL))
      goto eof;
   pos.x = (float)atof(pszTok);
   if (!dbPatchToken(ppText, &pszTok, NULL))
      goto eof;
   pos.y = (float)atof(pszTok);
   if (!dbPatchToken(ppText, &pszTok, NULL))
      goto eof;
   {
      char *pEnd = strchr(pszTok, ']');
      if (pEnd)
         *pEnd = '\0';
      pos.z = (float)atof(pszTok);
      if (!pEnd)
      {
         if (!dbPatchToken(ppText, &pszTok, NULL))
            goto eof;
         if (*pszTok != ']')
         {
            DBPATCH_ERROR("SYNTAX ERROR: expected ]\n");
            return FALSE;
         }
      }
   }
   {
      ObjPos *pPos = ObjPosGet(obj);
      if (pPos
          && fabs(pPos->loc.vec.x - pos.x) <= tolerance
          && fabs(pPos->loc.vec.y - pos.y) <= tolerance
          && fabs(pPos->loc.vec.z - pos.z) <= tolerance)
         *pbMatch = TRUE;
      else
         DBPATCH_LOG(12, "DIAG:         obj %d position doesn't match (or object doesn't exist)\n", obj);
   }
   pSeen[nSeen] = obj;
   return TRUE;

eof:
   DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
   return FALSE;
}

static BOOL dbPatchFingerprint(char *pText, ulong flags, sDbPatchEntry **ppScripts)
{
   if (!pText || !*pText)
      return FALSE;

   BOOL bGamesys = !(flags & kDBMission);
   AutoAppIPtr(QuestData);
   AutoAppIPtr(PropertyManager);
   AutoAppIPtr(ObjectSystem);

   BOOL bHaveBlocks = FALSE;
   BOOL bNeedMatch = FALSE;
   BOOL bMatched = FALSE;
   sDbPatchEntry *pScripts = NULL;
   char *pszTok;
   char *pszParams;

   while (*pText)
   {
      if (!dbPatchToken(&pText, &pszTok, NULL))
         break;
      if (stricmp(pszTok, "FINGERPRINT") && stricmp(pszTok, "!FINGERPRINT"))
      {
         if (stricmp(pszTok, "#script"))
            break;
         char *pszScript;
         if (!dbPatchToken(&pText, &pszScript, NULL))
         {
            DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
            return FALSE;
         }
         if (!pszScript || !*pszScript)
         {
            DBPATCH_ERROR("SYNTAX ERROR: expected OSM name after #script statement\n");
            return FALSE;
         }
         if (ppScripts)
            new sDbPatchEntry(pszScript, NULL, &pScripts);
         continue;
      }

      BOOL bNotBlock = *pszTok == '!';
      bHaveBlocks = TRUE;
      if (!bNotBlock)
         bNeedMatch = TRUE;
      DBPATCH_LOG(12, "DIAG:     evaluating \"%sFINGERPRINT\" block...\n", bNotBlock ? "!" : "");

      int nPos = 0;
      int nPosMatched = 0;
      BOOL bAll = TRUE;
      ObjID seen[DBPATCH_MAX_FP_POS];

      if (!dbPatchToken(&pText, &pszTok, NULL))
         goto eof;
      if (pszTok[0] != '{' || pszTok[1] != '\0')
      {
         DBPATCH_ERROR("SYNTAX ERROR: expected {\n");
         return FALSE;
      }
      for (;;)
      {
         BOOL bMatch;
         if (!dbPatchToken(&pText, &pszTok, NULL) || !dbPatchValue(&pText, &pszParams, TRUE))
            goto eof;
         if (pszTok[0] == '}' && pszTok[1] == '\0')
            break;
         if (!pszParams)
         {
            DBPATCH_ERROR("SYNTAX ERROR: parameters missing for fingerprint rule\n");
            return FALSE;
         }
         BOOL bNot = *pszTok == '!';
         if (!stricmp(pszTok, "QVAR") || !stricmp(pszTok, "!QVAR"))
         {
            if (bGamesys)
            {
               DBPATCH_ERROR("SYNTAX ERROR: QVAR fingerprint rules may not be used in a gamesys dbmod\n");
               return FALSE;
            }
            if (!dbPatchFingerprintQVar(&pszParams, bNot, &bMatch, pQuestData))
               return FALSE;
         }
         else if (!stricmp(pszTok, "PROP") || !stricmp(pszTok, "!PROP"))
         {
            if (!dbPatchFingerprintProp(bGamesys, &pszParams, bNot, &bMatch, pPropertyManager))
               return FALSE;
         }
         else if (!stricmp(pszTok, "NAME") || !stricmp(pszTok, "!NAME"))
         {
            if (!dbPatchFingerprintName(bGamesys, &pszParams, bNot, &bMatch, pObjectSystem))
               return FALSE;
         }
         else if (!stricmp(pszTok, "GAM") || !stricmp(pszTok, "!GAM"))
         {
            if (!dbPatchFingerprintGam(&pszParams, bNot, &bMatch))
               return FALSE;
         }
         else
         {
            if (bGamesys)
            {
               DBPATCH_ERROR("SYNTAX ERROR: object position fingerprint rules may not be used in a gamesys dbmod\n");
               return FALSE;
            }
            if (nPos >= DBPATCH_MAX_FP_POS)
            {
               DBPATCH_ERROR("WARNING: too many object positions in fingerprint, only the first %d are used, the rest are ignored\n", DBPATCH_MAX_FP_POS);
               continue;
            }
            if (!dbPatchFingerprintPos(pszTok, &pszParams, &bMatch, DBPATCH_FP_POS_TOLERANCE, seen, nPos))
               return FALSE;
            if (bMatch)
               nPosMatched++;
            nPos++;
            continue;
         }
         if (!bMatch)
            bAll = FALSE;
      }

      if (!bGamesys)
      {
         if (nPos > 0 && nPos < DBPATCH_MIN_FP_POS)
         {
            DBPATCH_ERROR("SYNTAX ERROR: fingerprints must contain at least %d object positions, or none at all\n", DBPATCH_MIN_FP_POS);
            return FALSE;
         }
         if (!bAll)
            continue;
         if (nPos)
         {
            int nRequired = nPos - DBPATCH_FP_POS_SLACK;
            if (nRequired < DBPATCH_MIN_FP_POS)
               nRequired = DBPATCH_MIN_FP_POS - 1;
            if (nPosMatched < nRequired)
            {
               DBPATCH_LOG(12, "DIAG:         fingerprint did not match, only %d out of %d obj positions matched, %d were required\n",
                           nPosMatched, nPos, nRequired);
               continue;
            }
         }
      }
      if (bAll)
      {
         if (bNotBlock)
         {
            DBPATCH_LOG(12, "DIAG:         \"!FINGERPRINT\" block successfully matched\n");
            DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     patch file rejected, !FINGERPRINT match\n");
            dbPatchFreeList(&pScripts);
            return FALSE;
         }
         bMatched = TRUE;
         DBPATCH_LOG(12, "DIAG:         \"FINGERPRINT\" block successfully matched\n");
      }
   }

   if (bNeedMatch && !bMatched)
   {
      DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     patch file rejected, no fingerprint match\n");
      dbPatchFreeList(&pScripts);
      return FALSE;
   }
   if (pScripts)
   {
      for (sDbPatchEntry *p = pScripts; p; p = p->next)
      {
         DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     requires script module \"%s\"\n", (const char *)p->name);
         new sDbPatchEntry(p->name, NULL, ppScripts);
      }
      dbPatchFreeList(&pScripts);
   }
   return TRUE;

eof:
   DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
   dbPatchFreeList(&pScripts);
   return FALSE;
}

#define DBPATCH_SKIP_TAGBLOCK   0x01
#define DBPATCH_SKIP_OBJPROP    0x02
#define DBPATCH_SKIP_METAPROP   0x04
#define DBPATCH_SKIP_LINK       0x08
#define DBPATCH_SKIP_RECEPTRON  0x10
#define DBPATCH_SKIP_STIMSOURCE 0x20
#define DBPATCH_SKIP_CREATE     0x40
#define DBPATCH_SKIP_POSTLOAD   (DBPATCH_SKIP_METAPROP | DBPATCH_SKIP_LINK | DBPATCH_SKIP_RECEPTRON | DBPATCH_SKIP_STIMSOURCE)

struct sDbPatchContext
{
   ulong flags;
   ulong skip;
   BOOL *pbQueued;
   BOOL bNamesValid;

   IStructDescTools *pTools;
   IObjectSystem *pObjSys;
   IPropertyManager *pPropMan;
   ILinkManager *pLinkMan;
   IStimSensors *pSensors;
   IStimSources *pSources;
   IReactions *pReactions;
   IPropagation *pPropagation;
   IStimuli *pStimuli;
   ITraitManager *pTraitMan;

   ObjID objSensor;
   ObjID objSource;
   IRelation *pSourceRel;
   IRelation *pReceptronRel;
   IRelation *pNullRel;
   IPropagator *pNullGator;

   BOOL bInCreate;
   BOOL bSkipCreate;
   ObjID createObj;
};

static void dbPatchEndCreate(sDbPatchContext *ctx)
{
   if (ctx->createObj == OBJ_NULL)
      return;
   ctx->pObjSys->EndCreate(ctx->createObj);
   RebuildSymNameTable();
   if (ctx->pStimuli->IsStimulus(ctx->createObj))
      ctx->pStimuli->AddStimulus(ctx->createObj);
   DBPATCH_LOG(DBPATCH_TRACE, "DIAG:   ...completed creation of %d\n", ctx->createObj);
   ctx->createObj = OBJ_NULL;
}

static BOOL dbPatchObjRef(sDbPatchContext *ctx, char **ppText, ObjID *pObj)
{
   char *pszTok;
   BOOL bQuoted = FALSE;
   if (!dbPatchToken(ppText, &pszTok, &bQuoted))
   {
      DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
      return FALSE;
   }
   if (!bQuoted)
      return dbPatchParseObjID(pszTok, pObj);
   if (!*pszTok)
   {
      *pObj = OBJ_NULL;
      return TRUE;
   }
   if (!ctx->bNamesValid)
   {
      ctx->bNamesValid = TRUE;
      RebuildSymNameTable();
   }
   *pObj = ctx->pObjSys->GetObjectNamed(pszTok);
   if (*pObj == OBJ_NULL)
      DBPATCH_ERROR("WARNING: no object named \"%s\" was found, skipping\n", pszTok);
   return TRUE;
}

static BOOL dbPatchParseHex(const char *pszToken, ulong *pValue)
{
   for (const char *p = pszToken; *p; p++)
   {
      if (!isxdigit((uchar)*p))
      {
         DBPATCH_ERROR("SYNTAX ERROR: expected (positive) hex val (%s)\n", pszToken);
         return FALSE;
      }
   }
   *pValue = strtoul(pszToken, NULL, 16);
   return TRUE;
}

static LinkID dbPatchRemapLink(ILinkManager *pLinkMan, LinkID id)
{
   if (!g_dbPatchRemapLinks)
      return id;
   LinkID newId;
   if (LINKID_IS_INVERTED(id))
      newId = LINKID_INVERT(pLinkMan->RemapOnLoad(LINKID_INVERT(id)));
   else
      newId = pLinkMan->RemapOnLoad(id);
   if (newId != id)
      DBPATCH_LOG(DBPATCH_DIAG, "INFO:     remapped link id %08X to %08X\n", id, newId);
   return newId;
}

static BOOL dbPatchOptionalId(char **ppText, ILinkManager *pLinkMan, LinkID *pId)
{
   char *pszTok;
   *pId = LINKID_NULL;
   if (g_dbPatchTokEnd == '\0' || g_dbPatchTokEnd == '\r' || g_dbPatchTokEnd == '\n')
      return TRUE;
   char *p = *ppText;
   while (*p && isspace((uchar)*p) && *p != '\r' && *p != '\n')
      p++;
   *ppText = p;
   if (!*p || *p == '\r' || *p == '\n' || *p == '{')
      return TRUE;
   if (!dbPatchToken(ppText, &pszTok, NULL))
   {
      DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
      return FALSE;
   }
   ulong id;
   if (!dbPatchParseHex(pszTok, &id))
      return FALSE;
   *pId = dbPatchRemapLink(pLinkMan, (LinkID)id);
   return TRUE;
}

static BOOL dbPatchCheckPair(ulong flags, ObjID obj, const char *pszWhat, const char *pszAction)
{
   if (obj <= 0)
   {
      if (flags & kDBGameSys)
         return TRUE;
      DBPATCH_ERROR("WARNING: incorrect dbmod context for %s on abstract (ObjID %d), %s\n", pszWhat, obj, pszAction);
      return FALSE;
   }
   if (flags & kDBMission)
      return TRUE;
   DBPATCH_ERROR("WARNING: incorrect dbmod context for %s on concrete (ObjID %d), %s\n", pszWhat, obj, pszAction);
   return FALSE;
}

static BOOL dbPatchCheckLink(sDbPatchContext *ctx, LinkID id, sLink *pLink, const char *pszAction)
{
   if (id == LINKID_NULL || !ctx->pLinkMan->Get(id, pLink))
   {
      DBPATCH_ERROR("WARNING: invalid link id %08X, %s\n", id, pszAction);
      return FALSE;
   }
   if (pLink->source <= 0 && pLink->dest <= 0)
   {
      if (ctx->flags & kDBGameSys)
         return TRUE;
      DBPATCH_ERROR("WARNING: incorrect dbmod context for link between abstract objects (ObjID %d -> %d), %s\n",
                  pLink->source, pLink->dest, pszAction);
      return FALSE;
   }
   if (ctx->flags & kDBMission)
      return TRUE;
   DBPATCH_ERROR("WARNING: incorrect dbmod context for link with concrete (ObjID %d -> %d), %s\n",
               pLink->source, pLink->dest, pszAction);
   return FALSE;
}

static BOOL dbPatchCheckReceptron(sDbPatchContext *ctx, ReceptronID id, sObjStimPair *pElems, const char *pszAction)
{
   if (id == 0)
   {
      DBPATCH_ERROR("WARNING: invalid receptron id %08X, %s\n", id, pszAction);
      return FALSE;
   }
   *pElems = ctx->pSensors->GetReceptronElems(id);
   if (pElems->obj == OBJ_NULL && pElems->stim == OBJ_NULL)
   {
      DBPATCH_ERROR("WARNING: invalid receptron id %08X, %s\n", id, pszAction);
      return FALSE;
   }
   return dbPatchCheckPair(ctx->flags, pElems->obj, "receptron", pszAction);
}

static BOOL dbPatchCheckSource(sDbPatchContext *ctx, StimSourceID id, sObjStimPair *pElems, const char *pszAction)
{
   if (id == 0)
   {
      DBPATCH_ERROR("WARNING: invalid stim source id %08X, %s\n", id, pszAction);
      return FALSE;
   }
   *pElems = ctx->pSources->GetSourceElems(id);
   if (pElems->obj == OBJ_NULL && pElems->stim == OBJ_NULL)
   {
      DBPATCH_ERROR("WARNING: invalid stim source id %08X, %s\n", id, pszAction);
      return FALSE;
   }
   return dbPatchCheckPair(ctx->flags, pElems->obj, "stim source", pszAction);
}

static BOOL dbPatchSingleValue(char **ppText, char **ppField, char **ppValue)
{
   **ppField = '\0';
   return dbPatchValue(ppText, ppValue, FALSE);
}

static int dbPatchNextField(char **ppText, BOOL *pbSingle, BOOL *pbDone, char **ppField, int *pIndex, char **ppValue)
{
   if (*pbDone)
      return 0;
   if (*pbSingle)
   {
      *pbDone = TRUE;
      return 1;
   }
   if (!dbPatchFieldToken(ppText, ppField, pIndex))
   {
      DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
      return -1;
   }
   if ((*ppField)[0] == '}' && (*ppField)[1] == '\0')
      return 0;
   if (!dbPatchValue(ppText, ppValue, FALSE))
      return -1;
   return 1;
}

static BOOL dbPatchBeginFields(char **ppText, BOOL *pbSingle, char **ppField, int *pIndex, char **ppValue)
{
   char *pszTok;
   *pbSingle = FALSE;
   *pIndex = 0;
   if (!dbPatchToken(ppText, &pszTok, NULL))
   {
      DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
      return FALSE;
   }
   if (*pszTok == '=')
   {
      *pbSingle = TRUE;
      *ppField = pszTok;
      return dbPatchSingleValue(ppText, ppField, ppValue);
   }
   if (pszTok[0] != '{' || pszTok[1] != '\0')
   {
      DBPATCH_ERROR("SYNTAX ERROR: expected {\n");
      return FALSE;
   }
   return TRUE;
}

static BOOL dbPatchSkipFields(char **ppText)
{
   BOOL bSingle, bDone = FALSE;
   char *pszField, *pszValue;
   int index, r;
   if (!dbPatchBeginFields(ppText, &bSingle, &pszField, &index, &pszValue))
      return FALSE;
   while ((r = dbPatchNextField(ppText, &bSingle, &bDone, &pszField, &index, &pszValue)) > 0)
      ;
   return r == 0;
}

static BOOL dbPatchCheckField(const sFieldDesc *pField, const sStructDesc *pDesc, int line)
{
   if (pField->offset + pField->size > pDesc->size)
      _CriticalMsg("invalid field offset", __FILE__, line);
   if (pField->type == kFieldTypeStringPtr || pField->type == kFieldTypeVoidPtr)
      return FALSE;
   return TRUE;
}

static BOOL dbPatchSetTagField(cFileVarBase *pVar, const sFieldDesc *pField, const char *pszValue,
                               const TagVersion *pVersion, IStructDescTools *pTools, const sStructDesc *pDesc)
{
   const char *pszTag = pVar->mDesc->tag.label;
   if (!dbPatchCheckField(pField, pDesc, 2505))
   {
      DBPATCH_ERROR("WARNING: pointer field type is not supported (%s::\"%s\" in object %s)\n",
                  pDesc->name, pField->name, pszTag);
      return FALSE;
   }
   if (pField->flags & kFieldFlagNotEdit)
   {
      DBPATCH_ERROR("WARNING: cannot change read-only field (%s::\"%s\" in object %s)\n",
                  pDesc->name, pField->name, pszTag);
      return FALSE;
   }
   if (!pVar->SetField(pField, pszValue, pVersion, pTools))
   {
      DBPATCH_ERROR("WARNING: could assign value %s::\"%s\" = \"%s\" in object %s\n",
                  pDesc->name, pField->name, pszValue, pszTag);
      return FALSE;
   }
   DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     assigned tag value %s::\"%s\" = \"%s\" in object %s\n",
               pDesc->name, pField->name, pszValue, pszTag);
   return TRUE;
}

static BOOL dbPatchSetPropField(void *pStruct, const char *pszValue, ObjID obj, IStructDescTools *pTools,
                                const char *pszProp, const sFieldDesc *pField, const sStructDesc *pDesc)
{
   if (!dbPatchCheckField(pField, pDesc, 2534))
   {
      DBPATCH_ERROR("WARNING: pointer field type is not supported (%s::\"%s\" on object %d)\n",
                  pDesc->name, pField->name, obj);
      return FALSE;
   }
   if (pField->flags & kFieldFlagNotEdit)
   {
      DBPATCH_ERROR("WARNING: cannot change read-only field (%s::\"%s\" on object %d)\n",
                  pDesc->name, pField->name, obj);
      return FALSE;
   }
   if (pTools->ParseField(pField, pszValue, pStruct) != S_OK)
   {
      DBPATCH_ERROR("WARNING: could not assign value %s::\"%s\" = \"%s\" on object %d\n",
                  pDesc->name, pField->name, pszValue, obj);
      return FALSE;
   }
   DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     assigned prop value \"%s\" -> %s::\"%s\" = \"%s\" on object %d\n",
               pszProp, pDesc->name, pField->name, pszValue, obj);
   return TRUE;
}

static BOOL dbPatchSetDataField(void *pStruct, const char *pszValue, ulong id, ObjID obj, IStructDescTools *pTools,
                                const char *pszType, const char *pszWhat, const sFieldDesc *pField,
                                const sStructDesc *pDesc)
{
   if (!dbPatchCheckField(pField, pDesc, 2563))
   {
      DBPATCH_ERROR("WARNING: pointer field type is not supported (%s::\"%s\" on %s %08X, obj %d)\n",
                  pDesc->name, pField->name, pszWhat, id, obj);
      return FALSE;
   }
   if (pField->flags & kFieldFlagNotEdit)
   {
      DBPATCH_ERROR("WARNING: cannot change read-only field (%s::\"%s\" on %s %08X, obj %d)\n",
                  pDesc->name, pField->name, pszWhat, id, obj);
      return FALSE;
   }
   if (pTools->ParseField(pField, pszValue, pStruct) != S_OK)
   {
      DBPATCH_ERROR("WARNING: could not assign value %s::\"%s\" = \"%s\" on %s %08X, obj %d\n",
                  pDesc->name, pField->name, pszValue, pszWhat, id, obj);
      return FALSE;
   }
   DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     assigned value \"%s\" -> %s::\"%s\" = \"%s\" on %s %08X, obj %d\n",
               pszType, pDesc->name, pField->name, pszValue, pszWhat, id, obj);
   return TRUE;
}

static cFileVarBase *dbPatchFindFileVar(eWhichVarFile file, const char *pszTag)
{
   cFileVarList *pList = cFileVarBase::AllFileVars();
   if (!pList)
      return NULL;
   for (cFileVarList::cIter iter = pList->Iter(); !iter.Done(); iter.Next())
   {
      cFileVarBase *pVar = iter.Value();
      if (pVar->mDesc->file == file && !strcmp(pVar->mDesc->tag.label, pszTag))
         return pVar;
   }
   return NULL;
}

static BOOL dbPatchTagBlock(sDbPatchContext *ctx, char **ppText)
{
   char *pszTag;
   if (!dbPatchToken(ppText, &pszTag, NULL))
   {
      DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
      return FALSE;
   }
   if (ctx->skip & DBPATCH_SKIP_TAGBLOCK)
   {
      *ctx->pbQueued = TRUE;
      return dbPatchSkipFields(ppText);
   }

   cFileVarBase *pVar = NULL;
   if (ctx->flags & kDBMission)
      pVar = dbPatchFindFileVar(kMissionVar, pszTag);
   if (!pVar && (ctx->flags & kDBGameSys))
      pVar = dbPatchFindFileVar(kGameSysVar, pszTag);
   if (!pVar)
   {
      DBPATCH_ERROR("WARNING: cannot find tag object '%s', skipping\n", pszTag);
      return dbPatchSkipFields(ppText);
   }
   const sFileVarDesc *pVarDesc = pVar->mDesc;
   const sStructDesc *pDesc = ctx->pTools->Lookup(pVarDesc->type);
   if (!pDesc)
   {
      DBPATCH_ERROR("WARNING: cannot find sdesc '%s' for tag object '%s', skipping\n", pVarDesc->type, pszTag);
      return dbPatchSkipFields(ppText);
   }
   if (!pDesc->nfields)
   {
      DBPATCH_ERROR("WARNING: sdesc '%s' has no fields (object '%s'), skipping\n", pVarDesc->type, pszTag);
      return dbPatchSkipFields(ppText);
   }

   BOOL bSingle, bDone = FALSE;
   char *pszField, *pszValue;
   int index, r;
   if (!dbPatchBeginFields(ppText, &bSingle, &pszField, &index, &pszValue))
      return FALSE;
   while ((r = dbPatchNextField(ppText, &bSingle, &bDone, &pszField, &index, &pszValue)) > 0)
   {
      const sFieldDesc *pField = dbPatchFindField(pDesc, pszField, index);
      if (!pField)
      {
         DBPATCH_ERROR("WARNING: cannot find field %s::\"%s\" (tag object '%s'), skipping\n",
                     pVarDesc->type, pszField, pszTag);
         continue;
      }
      TagVersion version = pVarDesc->version;
      dbPatchSetTagField(pVar, pField, pszValue, &version, ctx->pTools, pDesc);
   }
   return r == 0;
}

static BOOL dbPatchObjPropFields(sDbPatchContext *ctx, char **ppText, ObjID obj, IProperty *pProp, const char *pszProp,
                                 BOOL bAdd)
{
   const sPropertyTypeDesc *pTypeDesc = pProp->DescribeType();
   const sStructDesc *pDesc = ctx->pTools->Lookup(pTypeDesc->type);
   IStringProperty *pStrProp = NULL;
   if (!pDesc)
   {
      if (FAILED(pProp->QueryInterface(IID_IStringProperty, (void **)&pStrProp)))
      {
         DBPATCH_ERROR("WARNING: cannot find sdesc '%s' for property \"%s\" (ObjID %d), skipping\n",
                     pTypeDesc->type, pszProp, obj);
         return dbPatchSkipFields(ppText);
      }
   }
   else if (!pDesc->nfields)
   {
      DBPATCH_ERROR("WARNING: sdesc '%s' has no fields (property \"%s\" ObjID %d), skipping\n",
                  pTypeDesc->type, pszProp, obj);
      return dbPatchSkipFields(ppText);
   }

   IPropertyStore *pStore = NULL;
   if (FAILED(pProp->QueryInterface(IID_IPropertyStore, (void **)&pStore)))
   {
      CriticalMsg("QueryInterface IID_IPropertyStore failed");
      SafeRelease(pStrProp);
      return dbPatchSkipFields(ppText);
   }

   BOOL bResult;
   sDatum dat;
   dat.value = NULL;
   if (!pStore->Get(obj, &dat))
   {
      if (!bAdd)
      {
         DBPATCH_ERROR("WARNING: property \"%s\" does not exist for object %d, skipping\n", pszProp, obj);
         bResult = dbPatchSkipFields(ppText);
         goto done;
      }
      if (FAILED(pProp->Create(obj)) || !pStore->Get(obj, &dat))
      {
         DBPATCH_ERROR("WARNING: property \"%s\" could not be created for object %d, skipping\n", pszProp, obj);
         bResult = dbPatchSkipFields(ppText);
         goto done;
      }
      DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     added property \"%s\" to object %d\n", pszProp, obj);
   }

   {
      IDataOps *pOps = pStore->GetOps();
      if (!pOps)
      {
         DBPATCH_ERROR("WARNING: cannot get IDataOps for property \"%s\" (ObjID %d), skipping\n", pszProp, obj);
         bResult = dbPatchSkipFields(ppText);
         goto done;
      }
      void *pStruct = pOps->BlockSize(dat) < 0 ? (void *)&dat : dat.value;

      BOOL bSingle, bDone = FALSE;
      char *pszField, *pszValue;
      int index, r;
      if (!dbPatchBeginFields(ppText, &bSingle, &pszField, &index, &pszValue))
      {
         SafeRelease(pOps);
         bResult = FALSE;
         goto done;
      }
      while ((r = dbPatchNextField(ppText, &bSingle, &bDone, &pszField, &index, &pszValue)) > 0)
      {
         if (!pStrProp)
         {
            const sFieldDesc *pField = dbPatchFindField(pDesc, pszField, index);
            if (!pField)
               DBPATCH_ERROR("WARNING: cannot find field %s::\"%s\" (prop \"%s\" ObjID %d), skipping\n",
                           pTypeDesc->type, pszField, pszProp, obj);
            else if (dbPatchSetPropField(pStruct, pszValue, obj, ctx->pTools, pszProp, pField, pDesc))
            {
               pStore->Set(obj, dat);
               if (ctx->flags & kDBGameSys)
                  pProp->Touch(obj);
            }
         }
         else if (!*pszField)
         {
            pStrProp->Set(obj, pszValue);
            DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     assigned prop \"%s\" = \"%s\" on object %d\n", pszProp, pszValue, obj);
         }
         else
            DBPATCH_ERROR("WARNING: field \"%s\" does not exist for property \"%s\" (ObjID %d), skipping\n",
                        pszField, pszProp, obj);
      }
      SafeRelease(pOps);
      bResult = r == 0;
   }

done:
   SafeRelease(pStore);
   SafeRelease(pStrProp);
   return bResult;
}

static BOOL dbPatchObjProp(sDbPatchContext *ctx, char **ppText, const char *pszCmd)
{
   BOOL bAdd = *pszCmd == '+';
   BOOL bDelete = *pszCmd == '-';
   ObjID obj = ctx->createObj;
   char *pszProp;

   if (!ctx->bInCreate && !dbPatchObjRef(ctx, ppText, &obj))
      return FALSE;
   if (!dbPatchToken(ppText, &pszProp, NULL))
   {
      DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
      return FALSE;
   }

   BOOL bApply;
   if (ctx->bInCreate)
      bApply = !ctx->bSkipCreate;
   else if (ctx->skip & DBPATCH_SKIP_OBJPROP)
   {
      *ctx->pbQueued = TRUE;
      bApply = FALSE;
   }
   else
      bApply = TRUE;

   if (bApply && dbPatchCheckObj(obj, ctx->flags, FALSE))
   {
      if (!ctx->pObjSys->Exists(obj))
         DBPATCH_ERROR("WARNING: cannot find object %d, skipping\n", obj);
      else
      {
         IProperty *pProp = ctx->pPropMan->GetPropertyNamed(pszProp);
         BOOL bResult = TRUE;
         if (pProp->GetID() == PROPID_NULL)
         {
            DBPATCH_ERROR("WARNING: cannot find property \"%s\" (ObjID %d), skipping\n", pszProp, obj);
            bApply = FALSE;
         }
         else if (bDelete)
         {
            if (!pProp->IsSimplyRelevant(obj))
               DBPATCH_ERROR("WARNING: property \"%s\" does not exist for object %d, skipping delete\n", pszProp, obj);
            else if (FAILED(pProp->Delete(obj)))
               DBPATCH_ERROR("WARNING: failed to delete property \"%s\" from object %d\n", pszProp, obj);
            else
               DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     deleted property \"%s\" from object %d\n", pszProp, obj);
         }
         else
         {
            pProp->Describe();
            bResult = dbPatchObjPropFields(ctx, ppText, obj, pProp, pszProp, bAdd);
         }
         SafeRelease(pProp);
         if (bApply)
            return bResult;
      }
   }
   if (bDelete)
      return TRUE;
   return dbPatchSkipFields(ppText);
}

static BOOL dbPatchMetaProp(sDbPatchContext *ctx, char **ppText, const char *pszCmd)
{
   BOOL bAdd = pszCmd[0] == '+';
   BOOL bForce = bAdd && pszCmd[1] == '+';
   BOOL bRemove = pszCmd[0] == '-';
   ObjID obj, meta;

   if (!dbPatchObjRef(ctx, ppText, &obj) || !dbPatchObjRef(ctx, ppText, &meta))
      return FALSE;
   if (ctx->skip & DBPATCH_SKIP_METAPROP)
   {
      *ctx->pbQueued = TRUE;
      return TRUE;
   }
   if (!dbPatchCheckObj(obj, ctx->flags, FALSE) || meta == OBJ_NULL)
      return TRUE;

   ITraitManager *pTraitMan = ctx->pTraitMan;
   if (!pTraitMan->IsMetaProperty(meta))
   {
      DBPATCH_ERROR("WARNING: cannot add meta property, %d is not a meta property\n", meta);
      return TRUE;
   }
   if (pTraitMan->IsMetaProperty(obj))
   {
      DBPATCH_ERROR("WARNING: object %d is a meta property, cannot add meta property to another\n", obj, meta);
      return TRUE;
   }
   if (bRemove)
   {
      if (!pTraitMan->ObjHasDonorIntrinsically(obj, meta))
         DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     meta property %d not removed from object %d because object does not have it\n", meta, obj);
      else if (FAILED(pTraitMan->RemoveObjMetaProperty(obj, meta)))
         DBPATCH_ERROR("WARNING: failed to remove meta property %d from object %d\n", meta, obj);
      else
         DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     removed meta property %d from object %d\n", meta, obj);
      return TRUE;
   }
   if (!bForce)
   {
      if (!bAdd)
      {
         if (pTraitMan->ObjHasDonor(obj, meta))
         {
            DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     meta property %d not added to object %d because object or one of its archetypes has it\n", meta, obj);
            return TRUE;
         }
      }
      else if (pTraitMan->ObjHasDonorIntrinsically(obj, meta))
      {
         DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     meta property %d not added to object %d because object has it\n", meta, obj);
         return TRUE;
      }
   }
   if (FAILED(pTraitMan->AddObjMetaProperty(obj, meta)))
      DBPATCH_ERROR("WARNING: failed to add meta property %d to object %d\n", meta, obj);
   else
      DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     added meta property %d to object %d\n", meta, obj);
   return TRUE;
}

static BOOL dbPatchCreate(sDbPatchContext *ctx, char **ppText, const char *pszCmd)
{
   BOOL bArch = !stricmp(pszCmd, "CreateArch");
   const char *pszKind = bArch ? "archetype" : "object";
   ObjID arch;
   char *pszName = NULL;

   if (!dbPatchObjRef(ctx, ppText, &arch))
      return FALSE;
   if (g_dbPatchTokEnd != '\0' && g_dbPatchTokEnd != '\r' && g_dbPatchTokEnd != '\n')
   {
      while (**ppText && isspace((uchar)**ppText) && **ppText != '\r' && **ppText != '\n')
         ++*ppText;
      if (**ppText && **ppText != '\r' && **ppText != '\n' && **ppText != '{'
          && !dbPatchToken(ppText, &pszName, NULL))
      {
         DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
         return FALSE;
      }
   }
   while (**ppText && isspace((uchar)**ppText))
      ++*ppText;
   if (**ppText == '{')
   {
      ++*ppText;
      ctx->bInCreate = TRUE;
      ctx->bSkipCreate = FALSE;
   }

   if (ctx->skip & DBPATCH_SKIP_CREATE)
   {
      ctx->bSkipCreate = ctx->bInCreate;
      *ctx->pbQueued = TRUE;
      return TRUE;
   }
   if (!bArch)
   {
      if (!(ctx->flags & kDBMission))
      {
         DBPATCH_ERROR("WARNING: incorrect dbmod context for 'Create' command, skipping\n");
         ctx->bSkipCreate = ctx->bInCreate;
         return TRUE;
      }
   }
   else
   {
      if (!(ctx->flags & kDBGameSys))
      {
         DBPATCH_ERROR("WARNING: incorrect dbmod context for 'CreateArch' command, skipping\n");
         ctx->bSkipCreate = ctx->bInCreate;
         return TRUE;
      }
      if (arch > 0)
      {
         DBPATCH_ERROR("WARNING: 'CreateArch' does not take a concrete object (ObjID %d) as archetype argument, skipping\n", arch);
         ctx->bSkipCreate = ctx->bInCreate;
         return TRUE;
      }
   }
   if (!ctx->bNamesValid)
   {
      ctx->bNamesValid = TRUE;
      RebuildSymNameTable();
   }

   ObjID obj;
   if (pszName && (obj = ctx->pObjSys->GetObjectNamed(pszName)) != OBJ_NULL)
   {
      DBPATCH_ERROR("WARNING: %s \"%s\" not created, an object (%d) with that name already exists\n",
                  pszKind, pszName, obj);
      ctx->bSkipCreate = ctx->bInCreate;
      return TRUE;
   }
   obj = ctx->pObjSys->BeginCreate(arch, bArch ? kObjectAbstract : kObjectConcrete);
   if (obj == OBJ_NULL)
   {
      ctx->pObjSys->EndCreate(OBJ_NULL);
      return TRUE;
   }
   ctx->createObj = obj;
   if (!pszName)
      DBPATCH_LOG(DBPATCH_TRACE, "DIAG:   creating %s %d (from archetype %d)...\n", pszKind, obj, arch);
   else
      DBPATCH_LOG(DBPATCH_TRACE, "DIAG:   creating %s %s (%d) (from archetype %d)...\n", pszKind, pszName, obj, arch);
   if (obj < 0)
   {
      IProperty *pDonorType = ctx->pPropMan->GetPropertyNamed("DonorType");
      if (pDonorType->IsSimplyRelevant(arch))
         pDonorType->Copy(obj, arch);
      SafeRelease(pDonorType);
   }
   if (pszName)
      ctx->pObjSys->NameObject(obj, pszName);
   if (!ctx->bInCreate)
      dbPatchEndCreate(ctx);
   return TRUE;
}

static BOOL dbPatchLink(sDbPatchContext *ctx, char **ppText, const char *pszCmd)
{
   BOOL bAdd = pszCmd[0] == '+';
   BOOL bForce = bAdd && pszCmd[1] == '+';
   BOOL bDelete = pszCmd[0] == '-';
   ObjID src, dst;
   char *pszFlavor;
   LinkID id = LINKID_NULL;
   sLink link;

   if (!dbPatchObjRef(ctx, ppText, &src) || !dbPatchObjRef(ctx, ppText, &dst))
      return FALSE;
   if (!dbPatchToken(ppText, &pszFlavor, NULL))
   {
      DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
      return FALSE;
   }
   if (!bForce && !dbPatchOptionalId(ppText, ctx->pLinkMan, &id))
      return FALSE;

   if (ctx->skip & DBPATCH_SKIP_LINK)
   {
      *ctx->pbQueued = TRUE;
      return bDelete ? TRUE : dbPatchSkipFields(ppText);
   }
   if (!dbPatchCheckObj(src, ctx->flags, FALSE) || !dbPatchCheckObj(dst, ctx->flags, TRUE))
      return bDelete ? TRUE : dbPatchSkipFields(ppText);

   if (id != LINKID_NULL)
   {
      const char *pszAction = bAdd ? "creating new" : "skipping";
      if (!dbPatchCheckLink(ctx, id, &link, pszAction))
      {
         if (!bAdd)
            return bDelete ? TRUE : dbPatchSkipFields(ppText);
         id = LINKID_NULL;
      }
      else if (link.source != src || link.dest != dst)
      {
         DBPATCH_ERROR("WARNING: link %08X does not link %d -> %d as expected, %s\n", id, src, dst, pszAction);
         if (!bAdd)
            return bDelete ? TRUE : dbPatchSkipFields(ppText);
         id = LINKID_NULL;
      }
   }

   IRelation *pRel = ctx->pLinkMan->GetRelationNamed(pszFlavor);
   if (!pRel || pRel == ctx->pNullRel)
   {
      DBPATCH_ERROR("WARNING: invalid link type \"%s\", skipping\n", pszFlavor);
      SafeRelease(pRel);
      return bDelete ? TRUE : dbPatchSkipFields(ppText);
   }

   BOOL bResult = TRUE;
   RelationID flavor = pRel->GetID();
   const sRelationDesc *pRelDesc = pRel->Describe();
   AssertMsg(pRelDesc != NULL, "desc != NULL");
   if (id != LINKID_NULL && link.flavor != flavor)
      DBPATCH_ERROR("WARNING: link %08X is not of type \"%s\" as expected, skipping\n", id, pRelDesc->name);
   else if (!strcmp(pRelDesc->name, "Receptron"))
      DBPATCH_ERROR("WARNING: link of type \"Receptron\" not supported, use 'Receptron' command instead, skipping\n");
   else if (!strcmp(pRelDesc->name, "arSrcDesc"))
      DBPATCH_ERROR("WARNING: link of type \"arSrcDesc\" not supported, use 'StimSource' command instead, skipping\n");
   else if (pRelDesc->flags & kRelationNoEdit)
      DBPATCH_ERROR("WARNING: link type \"%s\" cannot be modified, skipping\n", pRelDesc->name);
   else
   {
      if (id == LINKID_NULL && !bForce)
      {
         ILinkQuery *pQuery = pRel->Query(src, dst);
         for (; !pQuery->Done(); pQuery->Next())
         {
            sLink found;
            pQuery->Link(&found);
            if (found.flavor == flavor)
            {
               id = pQuery->ID();
               break;
            }
         }
         SafeRelease(pQuery);
         if (id == LINKID_NULL && !bAdd)
         {
            DBPATCH_ERROR("WARNING: no \"%s\" link found for %d -> %d, skipping\n", pRelDesc->name, src, dst);
            SafeRelease(pRel);
            return bDelete ? TRUE : dbPatchSkipFields(ppText);
         }
      }

      if (bDelete)
      {
         if (FAILED(ctx->pLinkMan->Remove(id)))
            DBPATCH_ERROR("WARNING: failed to delete \"%s\" link %08X (%d -> %d)\n", pRelDesc->name, id, src, dst);
         else
            DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     deleted \"%s\" link %08X (%d -> %d)\n", pRelDesc->name, id, src, dst);
         SafeRelease(pRel);
         return TRUE;
      }

      const sRelationDataDesc *pDataDesc = pRel->DescribeData();
      if (pDataDesc)
      {
         if (id == LINKID_NULL)
         {
            if (pDataDesc->size == 0)
               id = pRel->Add(src, dst);
            else
            {
               void *pData = new char[pDataDesc->size];
               memset(pData, 0, pDataDesc->size);
               id = pRel->AddFull(src, dst, pData);
               delete [] (char *)pData;
            }
            if (id == LINKID_NULL)
            {
               DBPATCH_ERROR("WARNING: failed to create link '%d -> %d' of type \"%s\", skipping\n", src, dst, pRelDesc->name);
               SafeRelease(pRel);
               return dbPatchSkipFields(ppText);
            }
            DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     added link '%d -> %d' of type \"%s\" (LinkID %08X)\n", src, dst, pRelDesc->name, id);
         }
         if (pDataDesc->size)
         {
            const sStructDesc *pDesc = ctx->pTools->Lookup(pDataDesc->type);
            if (!pDesc)
               DBPATCH_ERROR("WARNING: cannot find sdesc '%s' for link \"%s\", skipping\n", pDataDesc->type, pRelDesc->name);
            else if (!pDesc->nfields)
               DBPATCH_ERROR("WARNING: sdesc '%s' has no fields (link \"%s\"), skipping\n", pDataDesc->type, pRelDesc->name);
            else
            {
               void *pData = ctx->pLinkMan->GetData(id);
               if (!pData)
               {
                  pData = new char[pDataDesc->size];
                  memset(pData, 0, pDataDesc->size);
                  pRel->SetData(id, pData);
                  DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     created missing instance data for link %08X\n", id);
               }
               BOOL bChanged = FALSE;
               BOOL bSingle, bDone = FALSE;
               char *pszField, *pszValue;
               int index, r;
               if (!dbPatchBeginFields(ppText, &bSingle, &pszField, &index, &pszValue))
               {
                  SafeRelease(pRel);
                  return FALSE;
               }
               while ((r = dbPatchNextField(ppText, &bSingle, &bDone, &pszField, &index, &pszValue)) > 0)
               {
                  const sFieldDesc *pField = dbPatchFindField(pDesc, pszField, index);
                  if (!pField)
                     DBPATCH_ERROR("WARNING: cannot find field %s::\"%s\" (link \"%s\" LinkID %d), skipping\n",
                                 pDataDesc->type, pszField, pRelDesc->name, id);
                  else if (dbPatchSetDataField(pData, pszValue, id, src, ctx->pTools, pRelDesc->name, "link", pField, pDesc))
                     bChanged = TRUE;
               }
               if (bChanged)
                  pRel->SetData(id, pData);
               SafeRelease(pRel);
               return r == 0;
            }
         }
      }
   }
   SafeRelease(pRel);
   if (bDelete)
      return TRUE;
   return dbPatchSkipFields(ppText) && bResult;
}

static BOOL dbPatchSkipReceptronBlock(char **ppText)
{
   char *pszTok, *pszKey, *pszValue;
   if (!dbPatchToken(ppText, &pszTok, NULL))
   {
      DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
      return FALSE;
   }
   if (pszTok[0] != '{' || pszTok[1] != '\0')
   {
      DBPATCH_ERROR("SYNTAX ERROR: expected {\n");
      return FALSE;
   }
   while (dbPatchToken(ppText, &pszKey, NULL) && dbPatchValue(ppText, &pszValue, TRUE))
   {
      if (pszKey[0] == '}' && pszKey[1] == '\0')
         return TRUE;
      if (!pszValue)
      {
         DBPATCH_ERROR("SYNTAX ERROR: parameter value missing\n");
         return FALSE;
      }
      if (!stricmp(pszKey, "Effect") || !stricmp(pszKey, "Propagator"))
      {
         while (**ppText && isspace((uchar)**ppText))
            ++*ppText;
         if (**ppText == '{')
         {
            ++*ppText;
            if (!dbPatchSkipReceptronBlock(ppText))
               return FALSE;
         }
      }
      else if (!stricmp(pszKey, "Shape") || !stricmp(pszKey, "Life"))
      {
         if (!dbPatchSkipFields(ppText))
            return FALSE;
      }
   }
   DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
   return FALSE;
}

static BOOL dbPatchReactObj(sDbPatchContext *ctx, const char *pszValue, ObjID *pObj)
{
   if (!stricmp(pszValue, "Me"))
      *pObj = ctx->objSensor;
   else if (!stricmp(pszValue, "Source"))
      *pObj = ctx->objSource;
   else
      return dbPatchParseInt(pszValue, pObj);
   return TRUE;
}

static BOOL dbPatchReceptron(sDbPatchContext *ctx, char **ppText, const char *pszCmd)
{
   BOOL bAdd = pszCmd[0] == '+';
   BOOL bForce = bAdd && pszCmd[1] == '+';
   BOOL bDelete = pszCmd[0] == '-';
   ObjID obj;
   char *pszStim;
   ReceptronID id = 0;
   sObjStimPair elems;
   sReceptron rec;

   if (!dbPatchObjRef(ctx, ppText, &obj))
      return FALSE;
   if (!dbPatchToken(ppText, &pszStim, NULL))
   {
      DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
      return FALSE;
   }
   StimID stim = ctx->pStimuli->GetStimulusNamed(pszStim);
   if (!bForce && !dbPatchOptionalId(ppText, ctx->pLinkMan, (LinkID *)&id))
      return FALSE;

   if (ctx->skip & DBPATCH_SKIP_RECEPTRON)
   {
      *ctx->pbQueued = TRUE;
      return bDelete ? TRUE : dbPatchSkipReceptronBlock(ppText);
   }
   if (stim == OBJ_NULL)
   {
      DBPATCH_ERROR("WARNING: \"%s\" is not a valid stimulus (receptron for obj %d), skipping\n", pszStim, obj);
      return bDelete ? TRUE : dbPatchSkipReceptronBlock(ppText);
   }
   if (!dbPatchCheckObj(obj, ctx->flags, FALSE) || !dbPatchCheckObj(stim, ctx->flags, TRUE))
      return bDelete ? TRUE : dbPatchSkipReceptronBlock(ppText);

   const char *pszAction = bAdd ? "creating new" : "skipping";
   if (id != 0)
   {
      if (!dbPatchCheckReceptron(ctx, id, &elems, pszAction))
      {
         if (!bAdd)
            return bDelete ? TRUE : dbPatchSkipReceptronBlock(ppText);
         id = 0;
      }
      else if (elems.obj != obj || elems.stim != stim)
      {
         DBPATCH_ERROR("WARNING: receptron %08X does not link obj %d and stim \"%s\" as expected, %s\n", id, obj, pszStim, pszAction);
         if (!bAdd)
            return bDelete ? TRUE : dbPatchSkipReceptronBlock(ppText);
         id = 0;
      }
   }
   else if (!bForce)
   {
      RelationID flavor = ctx->pReceptronRel->GetID();
      ILinkQuery *pQuery = ctx->pReceptronRel->Query(obj, stim);
      for (; !pQuery->Done(); pQuery->Next())
      {
         sLink found;
         pQuery->Link(&found);
         if (found.flavor == flavor)
         {
            ReceptronID candidate = pQuery->ID();
            if (SUCCEEDED(ctx->pSensors->GetReceptron(candidate, &rec)))
            {
               id = candidate;
               elems = ctx->pSensors->GetReceptronElems(id);
               break;
            }
         }
      }
      SafeRelease(pQuery);
      if (id == 0 && !bAdd)
      {
         DBPATCH_ERROR("WARNING: no receptron found for obj %d and stim \"%s\", skipping\n", obj, pszStim);
         return bDelete ? TRUE : dbPatchSkipReceptronBlock(ppText);
      }
   }

   if (bDelete)
   {
      if (FAILED(ctx->pLinkMan->Remove(id)))
         DBPATCH_ERROR("WARNING: failed to delete receptron %08X (obj %d stim \"%s\")\n", id, obj, pszStim);
      else
         DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     deleted receptron %08X (obj %d stim \"%s\")\n", id, obj, pszStim);
      return TRUE;
   }

   BOOL bChanged = FALSE;
   if (id != 0 && FAILED(ctx->pSensors->GetReceptron(id, &rec)))
   {
      DBPATCH_ERROR("WARNING: failed to get receptron data for receptron %08X (obj %d stim \"%s\"), %s\n", id, obj, pszStim, pszAction);
      if (!bAdd)
         return dbPatchSkipReceptronBlock(ppText);
      id = 0;
   }
   if (id == 0)
   {
      elems.obj = obj;
      elems.stim = stim;
      memset(&rec, 0, sizeof(rec));
      bChanged = TRUE;
      DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     preparing to add receptron (temp id 00000000) for stim \"%s\" on object %d\n", pszStim, obj);
   }

   char *pszTok, *pszKey, *pszValue;
   if (!dbPatchToken(ppText, &pszTok, NULL))
   {
      DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
      return FALSE;
   }
   if (pszTok[0] != '{' || pszTok[1] != '\0')
   {
      DBPATCH_ERROR("SYNTAX ERROR: expected {\n");
      return FALSE;
   }
   for (;;)
   {
      if (!dbPatchToken(ppText, &pszKey, NULL) || !dbPatchValue(ppText, &pszValue, TRUE))
      {
         DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
         return FALSE;
      }
      if (pszKey[0] == '}' && pszKey[1] == '\0')
         break;
      if (!pszValue)
      {
         DBPATCH_ERROR("SYNTAX ERROR: parameter value missing\n");
         return FALSE;
      }
      if (!stricmp(pszKey, "Min"))
      {
         if (!stricmp(pszValue, "None"))
            rec.trigger.flags |= kStimTrigNoMin;
         else
         {
            rec.trigger.flags &= ~kStimTrigNoMin;
            rec.trigger.min = (tStimLevel)atof(pszValue);
         }
         bChanged = TRUE;
         DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     assigned value Min = \"%s\" to receptron %08X, obj %d\n", pszValue, id, elems.obj);
      }
      else if (!stricmp(pszKey, "Max"))
      {
         if (!stricmp(pszValue, "None"))
            rec.trigger.flags |= kStimTrigNoMax;
         else
         {
            rec.trigger.flags &= ~kStimTrigNoMax;
            rec.trigger.max = (tStimLevel)atof(pszValue);
         }
         bChanged = TRUE;
         DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     assigned value Max = \"%s\" to receptron %08X, obj %d\n", pszValue, id, elems.obj);
      }
      else if (!stricmp(pszKey, "Target"))
      {
         if (!dbPatchReactObj(ctx, pszValue, &rec.effect.param.obj[kReactDirectObj]))
            return FALSE;
         bChanged = TRUE;
         DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     assigned value Target = \"%s\" to receptron %08X, obj %d\n", pszValue, id, elems.obj);
      }
      else if (!stricmp(pszKey, "Agent"))
      {
         if (!dbPatchReactObj(ctx, pszValue, &rec.effect.param.obj[kReactIndirectObj]))
            return FALSE;
         bChanged = TRUE;
         DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     assigned value Agent = \"%s\" to receptron %08X, obj %d\n", pszValue, id, elems.obj);
      }
      else if (!stricmp(pszKey, "Effect"))
      {
         char szEffect[64];
         strncpy(szEffect, pszValue, sizeof(szEffect));
         szEffect[sizeof(szEffect) - 1] = '\0';
         ReactionID kind = ctx->pReactions->GetReactionNamed(szEffect);
         if (rec.effect.kind != kind)
         {
            rec.effect.kind = kind;
            if (kind == REACTION_NULL)
            {
               DBPATCH_ERROR("WARNING: invalid effect name \"%s\" on receptron %08X, obj %d), skipping\n", szEffect, id, elems.obj);
               return FALSE;
            }
            bChanged = TRUE;
            DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     assigned effect type \"%s\" to receptron %08X, obj %d\n", szEffect, id, elems.obj);
         }
         while (**ppText && isspace((uchar)**ppText))
            ++*ppText;
         if (**ppText != '{')
            continue;
         const sReactionDesc *pReactDesc = ctx->pReactions->DescribeReaction(rec.effect.kind);
         const char *pszType = pReactDesc ? pReactDesc->param_type.name : NULL;
         const sStructDesc *pDesc = NULL;
         if (pszType && *pszType)
         {
            pDesc = ctx->pTools->Lookup(pszType);
            if (!pDesc)
               DBPATCH_ERROR("WARNING: cannot find sdesc '%s' for receptron effect \"%s\", skipping\n", pszType, szEffect);
            else if (!pDesc->nfields)
            {
               DBPATCH_ERROR("WARNING: sdesc '%s' has no fields (receptron effect \"%s\"), skipping\n", pszType, szEffect);
               pDesc = NULL;
            }
         }
         if (!pDesc)
         {
            if (!dbPatchSkipFields(ppText))
               return FALSE;
            continue;
         }
         BOOL bSingle, bDone = FALSE;
         char *pszField, *pszFieldValue;
         int index, r;
         if (!dbPatchBeginFields(ppText, &bSingle, &pszField, &index, &pszFieldValue))
            return FALSE;
         while ((r = dbPatchNextField(ppText, &bSingle, &bDone, &pszField, &index, &pszFieldValue)) > 0)
         {
            const sFieldDesc *pField = dbPatchFindField(pDesc, pszField, index);
            if (!pField)
               DBPATCH_ERROR("WARNING: cannot find field %s::\"%s\" (effect \"%s\" on receptron %08X, obj %d), skipping\n",
                           pszType, pszField, szEffect, id, elems.obj);
            else if (dbPatchSetDataField(&rec.effect.param.data, pszFieldValue, id, elems.obj, ctx->pTools, szEffect,
                                         "receptron", pField, pDesc))
               bChanged = TRUE;
         }
         if (r < 0)
            return FALSE;
      }
      else
      {
         DBPATCH_ERROR("SYNTAX ERROR: unknown/unexpected keyword '%s'\n", pszKey);
         return FALSE;
      }
   }

   if (!bChanged)
      return TRUE;
   if (rec.effect.kind == REACTION_NULL)
   {
      rec.effect.param.obj[kReactDirectObj] = OBJ_NULL;
      rec.effect.param.obj[kReactIndirectObj] = OBJ_NULL;
   }
   else
   {
      const sReactionDesc *pReactDesc = ctx->pReactions->DescribeReaction(rec.effect.kind);
      if (pReactDesc)
      {
         if (!(pReactDesc->flags & kReactionHasDirectObj))
            rec.effect.param.obj[kReactDirectObj] = OBJ_NULL;
         if (!(pReactDesc->flags & kReactionHasIndirectObj))
            rec.effect.param.obj[kReactIndirectObj] = OBJ_NULL;
      }
   }
   if (id == 0)
   {
      id = ctx->pSensors->AddReceptron(elems.obj, elems.stim, &rec);
      if (id == 0)
         DBPATCH_ERROR("WARNING: failed to create receptron for stim \"%s\" on object %d, skipping\n", pszStim, obj);
      else
         DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     added receptron for stim \"%s\" on object %d (LinkID %08X)\n", pszStim, obj, id);
   }
   else
      ctx->pSensors->SetReceptron(id, &rec);
   return TRUE;
}

static BOOL dbPatchSourceParams(sDbPatchContext *ctx, char **ppText, sStimSourceDesc *pSrc, IPropagator *pGator,
                                const char *pszGator, StimSourceID id, ObjID obj, BOOL *pbChanged)
{
   char *pszTok, *pszKey, *pszValue;
   if (!dbPatchToken(ppText, &pszTok, NULL))
   {
      DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
      return FALSE;
   }
   if (pszTok[0] != '{' || pszTok[1] != '\0')
   {
      DBPATCH_ERROR("SYNTAX ERROR: expected {\n");
      return FALSE;
   }
   for (;;)
   {
      if (!dbPatchToken(ppText, &pszKey, NULL) || !dbPatchValue(ppText, &pszValue, TRUE))
      {
         DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
         return FALSE;
      }
      if (pszKey[0] == '}' && pszKey[1] == '\0')
         return TRUE;
      BOOL bLife;
      if (!stricmp(pszKey, "Shape"))
         bLife = FALSE;
      else if (!stricmp(pszKey, "Life"))
         bLife = TRUE;
      else
      {
         DBPATCH_ERROR("SYNTAX ERROR: unknown/unexpected keyword '%s'\n", pszKey);
         return FALSE;
      }
      const char *pszPart = bLife ? "life" : "shape";
      const sStructDesc *pDesc = NULL;
      if (pGator && (pSrc->valid_fields & (bLife ? kStimSrcLifeValid : kStimSrcShapeValid)))
      {
         pDesc = bLife ? pGator->DescribeLifeCycles() : pGator->DescribeShapes();
         if (!pDesc)
            DBPATCH_ERROR("WARNING: type not found for propagator %s params for propagator \"%s\", skipping\n", pszPart, pszGator);
         else if (!pDesc->nfields)
         {
            DBPATCH_ERROR("WARNING: sdesc '%s' has no fields (propagator %s params for propagator \"%s\"), skipping\n",
                        pDesc->name, pszPart, pszGator);
            pDesc = NULL;
         }
      }
      if (!pDesc)
      {
         if (!dbPatchSkipFields(ppText))
            return FALSE;
         continue;
      }
      void *pStruct = bLife ? (void *)&pSrc->life : (void *)&pSrc->shape;
      BOOL bSingle, bDone = FALSE;
      char *pszField, *pszFieldValue;
      int index, r;
      if (!dbPatchBeginFields(ppText, &bSingle, &pszField, &index, &pszFieldValue))
         return FALSE;
      while ((r = dbPatchNextField(ppText, &bSingle, &bDone, &pszField, &index, &pszFieldValue)) > 0)
      {
         const sFieldDesc *pField = dbPatchFindField(pDesc, pszField, index);
         if (!pField)
            DBPATCH_ERROR("WARNING: cannot find field %s::\"%s\" (%s params for propagator \"%s\" on stim source %08X, obj %d), skipping\n",
                        pDesc->name, pszField, pszPart, pszGator, id, obj);
         else if (dbPatchSetDataField(pStruct, pszFieldValue, id, obj, ctx->pTools, pszGator, "stim source", pField, pDesc))
            *pbChanged = TRUE;
      }
      if (r < 0)
         return FALSE;
   }
}

static BOOL dbPatchStimSource(sDbPatchContext *ctx, char **ppText, const char *pszCmd)
{
   BOOL bAdd = pszCmd[0] == '+';
   BOOL bForce = bAdd && pszCmd[1] == '+';
   BOOL bDelete = pszCmd[0] == '-';
   ObjID obj;
   char *pszStim;
   StimSourceID id = 0;
   sObjStimPair elems;
   sStimSourceDesc src;

   if (!dbPatchObjRef(ctx, ppText, &obj))
      return FALSE;
   if (!dbPatchToken(ppText, &pszStim, NULL))
   {
      DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
      return FALSE;
   }
   StimID stim = ctx->pStimuli->GetStimulusNamed(pszStim);
   if (!bForce && !dbPatchOptionalId(ppText, ctx->pLinkMan, (LinkID *)&id))
      return FALSE;

   if (ctx->skip & DBPATCH_SKIP_STIMSOURCE)
   {
      *ctx->pbQueued = TRUE;
      return bDelete ? TRUE : dbPatchSkipReceptronBlock(ppText);
   }
   if (stim == OBJ_NULL)
   {
      DBPATCH_ERROR("WARNING: \"%s\" is not a valid stimulus (stim source for obj %d), skipping\n", pszStim, obj);
      return bDelete ? TRUE : dbPatchSkipReceptronBlock(ppText);
   }
   if (!dbPatchCheckObj(obj, ctx->flags, FALSE) || !dbPatchCheckObj(stim, ctx->flags, TRUE))
      return bDelete ? TRUE : dbPatchSkipReceptronBlock(ppText);

   const char *pszAction = bAdd ? "creating new" : "skipping";
   if (id != 0)
   {
      if (!dbPatchCheckSource(ctx, id, &elems, pszAction))
      {
         if (!bAdd)
            return bDelete ? TRUE : dbPatchSkipReceptronBlock(ppText);
         id = 0;
      }
      else if (elems.obj != obj || elems.stim != stim)
      {
         DBPATCH_ERROR("WARNING: stim source %08X does not link obj %d and stim \"%s\" as expected, %s\n", id, obj, pszStim, pszAction);
         if (!bAdd)
            return bDelete ? TRUE : dbPatchSkipReceptronBlock(ppText);
         id = 0;
      }
   }
   else if (!bForce)
   {
      RelationID flavor = ctx->pSourceRel->GetID();
      ILinkQuery *pQuery = ctx->pSourceRel->Query(obj, stim);
      for (; !pQuery->Done(); pQuery->Next())
      {
         sLink found;
         pQuery->Link(&found);
         if (found.flavor == flavor)
         {
            StimSourceID candidate = pQuery->ID();
            if (SUCCEEDED(ctx->pSources->DescribeSource(candidate, &src)))
            {
               id = candidate;
               elems = ctx->pSources->GetSourceElems(id);
               break;
            }
         }
      }
      SafeRelease(pQuery);
      if (id == 0 && !bAdd)
      {
         DBPATCH_ERROR("WARNING: no stim source found for obj %d and stim \"%s\", skipping\n", obj, pszStim);
         return bDelete ? TRUE : dbPatchSkipReceptronBlock(ppText);
      }
   }

   if (bDelete)
   {
      if (FAILED(ctx->pSources->RemoveSource(id)))
         DBPATCH_ERROR("WARNING: failed to delete stim source %08X, obj %d\n", id, elems.obj);
      else
         DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     deleted stim source %08X, obj %d\n", id, elems.obj);
      return TRUE;
   }

   BOOL bChanged = FALSE;
   if (id != 0 && FAILED(ctx->pSources->DescribeSource(id, &src)))
   {
      DBPATCH_ERROR("WARNING: failed to get stim source data for source %08X (obj %d stim \"%s\"), %s\n", id, obj, pszStim, pszAction);
      if (!bAdd)
         return dbPatchSkipReceptronBlock(ppText);
      id = 0;
   }
   if (id == 0)
   {
      elems.obj = obj;
      elems.stim = stim;
      memset(&src, 0, sizeof(src));
      bChanged = TRUE;
      DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     preparing to add stim source (temp id 00000000) for stim \"%s\" on object %d\n", pszStim, obj);
   }

   char *pszTok, *pszKey, *pszValue;
   if (!dbPatchToken(ppText, &pszTok, NULL))
   {
      DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
      return FALSE;
   }
   if (pszTok[0] != '{' || pszTok[1] != '\0')
   {
      DBPATCH_ERROR("SYNTAX ERROR: expected {\n");
      return FALSE;
   }
   for (;;)
   {
      if (!dbPatchToken(ppText, &pszKey, NULL) || !dbPatchValue(ppText, &pszValue, TRUE))
      {
         DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
         return FALSE;
      }
      if (pszKey[0] == '}' && pszKey[1] == '\0')
         break;
      if (!pszValue)
      {
         DBPATCH_ERROR("SYNTAX ERROR: parameter value missing\n");
         return FALSE;
      }
      if (!stricmp(pszKey, "Intensity"))
      {
         src.level = (tStimLevel)atof(pszValue);
         bChanged = TRUE;
         DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     assigned value Intensity = \"%s\" to stim source %08X, obj %d\n", pszValue, id, elems.obj);
         continue;
      }
      if (stricmp(pszKey, "Propagator"))
      {
         DBPATCH_ERROR("SYNTAX ERROR: unknown/unexpected keyword '%s'\n", pszKey);
         return FALSE;
      }

      IPropagator *pGator = NULL;
      PropagatorID gatorID = 0;
      const char *pszGator = NULL;
      sStimSourceDesc init;
      if (stricmp(pszValue, "None") && stricmp(pszValue, "Null"))
      {
         pGator = ctx->pPropagation->GetPropagatorNamed(pszValue);
         if (!pGator || pGator == ctx->pNullGator)
         {
            SafeRelease(pGator);
            DBPATCH_ERROR("SYNTAX ERROR: invalid propagator name \"%s\" (stim source %08X, obj %d)\n", pszValue, id, elems.obj);
            return FALSE;
         }
         gatorID = pGator->GetID();
         pGator->InitSource(&init);
         const sPropagatorDesc *pGatorDesc = pGator->Describe();
         AssertMsg(pGatorDesc != NULL, "g8rdesc != NULL");
         pszGator = pGatorDesc->name;
      }
      else
         memset(&init, 0, sizeof(init));
      if (src.propagator != gatorID)
      {
         src.valid_fields = init.valid_fields;
         src.shape = init.shape;
         src.life = init.life;
         src.propagator = gatorID;
         bChanged = TRUE;
         DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     assigned propagator type \"%s\" to stim source %08X, obj %d\n",
                     pszGator ? pszGator : "None", id, elems.obj);
      }
      while (**ppText && isspace((uchar)**ppText))
         ++*ppText;
      if (**ppText == '{')
      {
         if (!dbPatchSourceParams(ctx, ppText, &src, pGator, pszGator ? pszGator : "None", id, elems.obj, &bChanged))
         {
            SafeRelease(pGator);
            return FALSE;
         }
      }
      SafeRelease(pGator);
   }

   if (!bChanged)
      return TRUE;
   if (id == 0)
   {
      id = ctx->pSources->AddSource(elems.obj, elems.stim, &src);
      if (id == 0)
         DBPATCH_ERROR("WARNING: failed to create stim source for stim \"%s\" on object %d, skipping\n", pszStim, obj);
      else
         DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     added stim source for stim \"%s\" on object %d (LinkID %08X)\n", pszStim, obj, id);
   }
   else
      ctx->pSourceRel->SetData(id, &src);
   return TRUE;
}

static BOOL dbPatchCmdIs(const char *pszTok, const char *pszCmd, BOOL bForce)
{
   if (!stricmp(pszTok, pszCmd))
      return TRUE;
   if (pszTok[0] == '+' && !stricmp(pszTok + 1, pszCmd))
      return TRUE;
   if (bForce && pszTok[0] == '+' && pszTok[1] == '+' && !stricmp(pszTok + 2, pszCmd))
      return TRUE;
   return pszTok[0] == '-' && !stricmp(pszTok + 1, pszCmd);
}

static BOOL dbPatchParse(BOOL *pbQueued, sDbPatchEntry **ppScripts, char *pText, ulong flags, int pass)
{
   char *pszTok;
   BOOL bFingerprint = FALSE;
   BOOL bBody = FALSE;

   if (!pText || !*pText)
      return FALSE;

   do
   {
      if (!dbPatchToken(&pText, &pszTok, NULL))
         break;
      if (!stricmp(pszTok, "FINGERPRINT") || !stricmp(pszTok, "!FINGERPRINT"))
      {
         bFingerprint = TRUE;
         while (*pText && isspace((uchar)*pText))
            pText++;
         if (*pText != '{')
         {
            DBPATCH_ERROR("SYNTAX ERROR: expected {\n");
            return FALSE;
         }
         pText++;
         do
         {
            if (!dbPatchToken(&pText, &pszTok, NULL))
            {
               DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
               return FALSE;
            }
         } while (*pszTok != '}');
         continue;
      }
      if (stricmp(pszTok, "#script"))
      {
         bBody = TRUE;
         break;
      }
      char *pszScript;
      if (!dbPatchToken(&pText, &pszScript, NULL))
      {
         DBPATCH_ERROR("SYNTAX ERROR: unexpected EOF\n");
         return FALSE;
      }
      if (!pszScript || !*pszScript)
      {
         DBPATCH_ERROR("SYNTAX ERROR: expected OSM name after #script statement\n");
         return FALSE;
      }
      if (ppScripts && !bFingerprint)
      {
         DBPATCH_LOG(DBPATCH_TRACE, "DIAG:     requires script module \"%s\"\n", pszScript);
         new sDbPatchEntry(pszScript, NULL, ppScripts);
      }
   } while (*pText);

   if (pass == kDbPatchScripts || !cFileVarBase::AllFileVars())
      return FALSE;

   sDbPatchContext ctx;
   memset(&ctx, 0, sizeof(ctx));
   ctx.flags = flags;
   ctx.pbQueued = pbQueued;
   ctx.bNamesValid = pass != kDbPatchLoad;
   if (pass == kDbPatchLoad)
      ctx.skip = DBPATCH_SKIP_POSTLOAD;
   else if (pass == kDbPatchPostLoad)
      ctx.skip = ~DBPATCH_SKIP_POSTLOAD;

   ctx.pTools = AppGetObj(IStructDescTools);
   ctx.pObjSys = AppGetObj(IObjectSystem);
   ctx.pPropMan = AppGetObj(IPropertyManager);
   ctx.pLinkMan = AppGetObj(ILinkManager);
   ctx.pSensors = AppGetObj(IStimSensors);
   ctx.pSources = AppGetObj(IStimSources);
   ctx.pReactions = AppGetObj(IReactions);
   ctx.pPropagation = AppGetObj(IPropagation);
   ctx.pStimuli = AppGetObj(IStimuli);
   ctx.pTraitMan = AppGetObj(ITraitManager);

   BOOL bResult = FALSE;
   if (!ctx.pTools || !ctx.pObjSys || !ctx.pPropMan || !ctx.pSensors || !ctx.pReactions
       || !ctx.pPropagation || !ctx.pStimuli || !ctx.pTraitMan)
   {
      CriticalMsg("dbPatch: failed to resolve a subsystem");
      goto cleanup;
   }

   ctx.objSource = ctx.pSensors->ObjParam(kObjParamSource);
   ctx.objSensor = ctx.pSensors->ObjParam(kObjParamSensor);
   ctx.pSourceRel = ctx.pLinkMan->GetRelationNamed("arSrcDesc");
   ctx.pReceptronRel = ctx.pLinkMan->GetRelationNamed("Receptron");
   ctx.pNullGator = ctx.pPropagation->GetPropagatorNamed("");
   ctx.pNullRel = ctx.pLinkMan->GetRelationNamed("");
   if (!ctx.objSensor || !ctx.objSource || !ctx.pSourceRel || !ctx.pReceptronRel || !ctx.pNullGator || !ctx.pNullRel)
   {
      CriticalMsg("dbPatch: failed to resolve an internal archetype");
      goto cleanup;
   }

   bResult = TRUE;
   while (bBody || (*pText && dbPatchToken(&pText, &pszTok, NULL)))
   {
      BOOL bOk;
      bBody = FALSE;
      if (ctx.bInCreate && pszTok[0] == '}' && pszTok[1] == '\0')
      {
         dbPatchEndCreate(&ctx);
         ctx.bInCreate = FALSE;
         ctx.bSkipCreate = FALSE;
         continue;
      }
      if (!ctx.bInCreate && !stricmp(pszTok, "TagBlock"))
         bOk = dbPatchTagBlock(&ctx, &pText);
      else if (!stricmp(pszTok, "ObjProp") || !stricmp(pszTok, "+ObjProp") || !stricmp(pszTok, "-ObjProp"))
         bOk = dbPatchObjProp(&ctx, &pText, pszTok);
      else if (ctx.bInCreate)
      {
         DBPATCH_ERROR("SYNTAX ERROR: unknown/unexpected token '%s'\n", pszTok);
         bOk = FALSE;
      }
      else if (dbPatchCmdIs(pszTok, "Link", TRUE))
         bOk = dbPatchLink(&ctx, &pText, pszTok);
      else if (dbPatchCmdIs(pszTok, "Receptron", TRUE))
         bOk = dbPatchReceptron(&ctx, &pText, pszTok);
      else if (dbPatchCmdIs(pszTok, "StimSource", TRUE))
         bOk = dbPatchStimSource(&ctx, &pText, pszTok);
      else if (dbPatchCmdIs(pszTok, "MetaProp", TRUE))
         bOk = dbPatchMetaProp(&ctx, &pText, pszTok);
      else if (!stricmp(pszTok, "Create") || !stricmp(pszTok, "CreateArch"))
         bOk = dbPatchCreate(&ctx, &pText, pszTok);
      else
      {
         DBPATCH_ERROR("SYNTAX ERROR: unknown/unexpected token '%s'\n", pszTok);
         bOk = FALSE;
      }
      if (!bOk)
      {
         bResult = FALSE;
         break;
      }
   }
   if (ctx.createObj)
   {
      if (bResult)
         dbPatchEndCreate(&ctx);
      else
         ctx.pObjSys->EndCreate(ctx.createObj);
   }

cleanup:
   SafeRelease(ctx.pNullRel);
   SafeRelease(ctx.pNullGator);
   SafeRelease(ctx.pReceptronRel);
   SafeRelease(ctx.pSourceRel);
   SafeRelease(ctx.pTraitMan);
   SafeRelease(ctx.pStimuli);
   SafeRelease(ctx.pPropagation);
   SafeRelease(ctx.pReactions);
   SafeRelease(ctx.pSources);
   SafeRelease(ctx.pSensors);
   SafeRelease(ctx.pLinkMan);
   SafeRelease(ctx.pPropMan);
   SafeRelease(ctx.pObjSys);
   SafeRelease(ctx.pTools);
   return bResult;
}

static const char *g_apszDbDumpTypeNames[] =
{
   "int", "bool", "short", "bitflags", "enum", "string", "char*", "void*", "Point", "vector",
   "float", "sfloat", "sfloatvec", "float", "rgb", "vector", "ang", "angvec",
   "uint", "ushort", "packednormal",
};

#define DBDUMP_TYPE_UINT         18
#define DBDUMP_TYPE_USHORT       19
#define DBDUMP_TYPE_PACKEDNORMAL 20

static int dbDumpFieldIndex(const sStructDesc *pDesc, int iField)
{
   const char *pszName = pDesc->fields[iField].name;
   int index = 0;
   for (int i = 0; i < iField; i++)
      if (!strnicmp(pDesc->fields[i].name, pszName, sizeof(pDesc->fields[i].name)))
         index++;
   return index;
}

static BOOL dbDumpFieldVisible(const sStructDesc *pDesc, const sFieldDesc *pField, BOOL bSimple)
{
   AssertMsg(pField->size != 0, "fd->size != 0");
   if (pField->type == kFieldTypeStringPtr || pField->type == kFieldTypeVoidPtr || pField->size == 0)
      return FALSE;
   if (pField->flags & kFieldFlagNotEdit)
      return FALSE;
   return pField->name[0] != '\0' || bSimple;
}

static void dbDumpStruct(LGFILE *f, const sStructDesc *pDesc, BOOL bBraceOnly, const char *pszIndent)
{
   BOOL bSimple = pDesc->nfields == 1 && pDesc->fields[0].name[0] == '\0';
   if (!bBraceOnly)
      lg_fprintf(f, "%s%s\n{\n", pszIndent, pDesc->name);
   else
      lg_fprintf(f, "%s{\n", pszIndent);
   for (int i = 0; i < pDesc->nfields; i++)
   {
      const sFieldDesc *pField = &pDesc->fields[i];
      if (!dbDumpFieldVisible(pDesc, pField, bSimple))
         continue;
      int type = pField->type;
      BOOL bHex = (pField->flags & kFieldFlagHex) && (type == kFieldTypeInt || type == kFieldTypeShort);
      if ((pField->flags & kFieldFlagPackedNormal) && type == kFieldTypeInt)
         type = DBDUMP_TYPE_PACKEDNORMAL;
      else if (pField->flags & kFieldFlagUnsigned)
      {
         if (type == kFieldTypeInt)
            type = DBDUMP_TYPE_UINT;
         else if (type == kFieldTypeShort)
            type = DBDUMP_TYPE_USHORT;
      }
      int index = i ? dbDumpFieldIndex(pDesc, i) : 0;
      if (!index)
         lg_fprintf(f, "%s\t\"%s\" : %s%s", pszIndent, pField->name, g_apszDbDumpTypeNames[type], bHex ? "_hex" : "");
      else
         lg_fprintf(f, "%s\t\"%s\"[%d] : %s%s", pszIndent, pField->name, index, g_apszDbDumpTypeNames[type], bHex ? "_hex" : "");

      const char **ppszNames = (const char **)pField->data;
      if (pField->type == kFieldTypeEnum && pField->max > 0 && ppszNames)
      {
         lg_fputs("    // enums: ", f);
         for (ulong n = 0; n < pField->datasize; n++)
            lg_fprintf(f, n ? ", \"%s\"" : "\"%s\"", ppszNames[n]);
      }
      else if (pField->type == kFieldTypeBits && pField->max > 0 && ppszNames)
      {
         lg_fputs("    // flags: ", f);
         for (int n = 0; n < pField->max - pField->min; n++)
            lg_fprintf(f, n ? ", \"%s\"" : "\"%s\"", ppszNames[n]);
      }
      else if (pField->type == kFieldTypeString && pField->size)
         lg_fprintf(f, "    // max %d characters long", pField->size - 1);
      lg_fputs("\n", f);
   }
   lg_fprintf(f, "%s}\n", pszIndent);
}

static void dbDumpStructValues(LGFILE *f, const sStructDesc *pDesc, const void *pStruct, IStructDescTools *pTools)
{
   char buf[1024];
   BOOL bSimple = pDesc->nfields == 1 && pDesc->fields[0].name[0] == '\0';
   lg_fprintf(f, "%s{\n", "");
   for (int i = 0; i < pDesc->nfields; i++)
   {
      const sFieldDesc *pField = &pDesc->fields[i];
      if (!dbDumpFieldVisible(pDesc, pField, bSimple))
         continue;
      if (FAILED(pTools->UnparseField(pField, pStruct, buf, sizeof(buf))))
         continue;
      int index = i ? dbDumpFieldIndex(pDesc, i) : 0;
      if (!index)
         lg_fprintf(f, "%s\t\"%s\" %s", "", pField->name, buf);
      else
         lg_fprintf(f, "%s\t\"%s\"[%d] %s", "", pField->name, index, buf);
      lg_fputs("\n", f);
   }
   lg_fprintf(f, "%s}\n", "");
}

static void dbDumpTypeDefinitions(LGFILE *f)
{
   static const char *names[] =
   {
      "int", "uint", "int_hex", "uint_hex", "bool", "short", "ushort", "short_hex", "ushort_hex",
      "bitflags", "enum", "Point", "vector", "float", "sfloat", "sfloatvec", "rgb", "ang", "angvec",
      "packednormal", "packednormal_hex", NULL
   };
   static const char *descs[] =
   {
      "integer number",
      "positive integer number",
      "hexadecimal integer number (0xABCD0123)",
      "positive hexadecimal number (0xABCD0123)",
      "one of: 0, 1, true, false, yes, no, on, off",
      "integer number -32768 to 32767",
      "positive integer number 0 to 65535",
      "integer hexadecimal -0x8000 to 0x7FFF",
      "positive hexadecimal number 0x0 to 0xFFFF",
      "comma separated list of unquoted flag/bit names",
      "enum/value name",
      "comma separated x, y int pair",
      "comma separated x, y, z float triplet (if color then r g b values 0.0 - 1.0)",
      "floating point number",
      "floating point number -32768.0 to 32767.0",
      "space separated x y z sfloat triplet",
      "comma separated r, g, b triplet with int values 0 - 255",
      "angle 0.0 to 360.0",
      "comma separated heading, pitch, bank ang triplet",
      "32-bit integer number with an encoded unit length vector, similar to normal maps, with 3 MSB aligned 10-bit signed vals [X:10 Y:10 Z:10 unused:2]",
      "same as packednormal but as a hexadecimal number",
   };
   lg_fputs("\nType definitions:\n\n", f);
   for (int i = 0; names[i]; i++)
      lg_fprintf(f, "%-12s = %s\n", names[i], descs[i]);
}

static void dbDumpTagBlock(LGFILE *f, const sFileVarDesc *pVarDesc, cFileVarBase *pVar, int mode)
{
   char name[32];
   sprintf(name, "\"%s\"", pVarDesc->tag.label);
   if (mode == 2 && pVar)
   {
      lg_fprintf(f, "// %s\nTagBlock %s\n", pVarDesc->friendly_name, name);
      AutoAppIPtr(StructDescTools);
      const sStructDesc *pDesc = pStructDescTools->Lookup(pVarDesc->type);
      if (pDesc)
         dbDumpStructValues(f, pDesc, pVar->mBlock, pStructDescTools);
      return;
   }
   if (!mode)
   {
      lg_fprintf(f, "TagBlock %-18s : %-25s // %s\n", name, pVarDesc->type, pVarDesc->friendly_name);
      return;
   }
   lg_fprintf(f, "TagBlock %-18s // type %-25s ; %s\n", name, pVarDesc->type, pVarDesc->friendly_name);
   AutoAppIPtr(StructDescTools);
   const sStructDesc *pDesc = pStructDescTools->Lookup(pVarDesc->type);
   if (pDesc)
      dbDumpStruct(f, pDesc, TRUE, "");
}

static void dbDumpPropUI(char *buf, const sPropertyDesc *pDesc)
{
   buf[0] = '\0';
   if (!pDesc->ui.friendly_name)
      return;
   if (!pDesc->ui.category || !*pDesc->ui.category)
      sprintf(buf, " , editor name: \"%s\"", pDesc->ui.friendly_name);
   else
      sprintf(buf, " , editor name: \"%s: %s\"", pDesc->ui.category, pDesc->ui.friendly_name);
   if (pDesc->ui.help_text)
      sprintf(buf + strlen(buf), " , help text: \"%s\"", pDesc->ui.help_text);
}

static void dbDumpObjProp(LGFILE *f, const sPropertyDesc *pDesc, const char *pszType, BOOL bFull)
{
   char ui[256];
   char name[32];
   dbDumpPropUI(ui, pDesc);
   sprintf(name, "\"%s\"", pDesc->name);
   if (!bFull)
   {
      lg_fprintf(f, "ObjProp %-20s : %-25s // flags 0x%04X%s\n", name, pszType, pDesc->flags, ui);
      return;
   }
   lg_fprintf(f, "ObjProp %-20s // type %-25s , flags 0x%04X%s\n", name, pszType, pDesc->flags, ui);
   AutoAppIPtr(StructDescTools);
   const sStructDesc *pStruct = pStructDescTools->Lookup(pszType);
   if (pStruct)
      dbDumpStruct(f, pStruct, TRUE, "");
}

static void dbDumpStringProp(LGFILE *f, const sPropertyDesc *pDesc, BOOL bFull)
{
   char ui[256];
   char name[32];
   dbDumpPropUI(ui, pDesc);
   sprintf(name, "\"%s\"", pDesc->name);
   if (!bFull)
   {
      lg_fprintf(f, "ObjProp %-20s : %-25s // flags 0x%04X%s\n", name, "string", pDesc->flags, ui);
      return;
   }
   lg_fprintf(f, "ObjProp %-20s // type %-25s , flags 0x%04X%s\n", name, "string", pDesc->flags, ui);
   sFieldDesc field;
   memset(&field, 0, sizeof(field));
   field.type = kFieldTypeString;
   field.size = 2048;
   sStructDesc desc;
   memset(&desc, 0, sizeof(desc));
   strcpy(desc.name, "string");
   desc.size = 2048;
   desc.nfields = 1;
   desc.fields = &field;
   dbDumpStruct(f, &desc, TRUE, "");
}

static void dbDumpLink(LGFILE *f, const sRelationDesc *pDesc, const sRelationDataDesc *pDataDesc, BOOL bFull)
{
   char name[48];
   sprintf(name, "\"%s\"", pDesc->name);
   const char *pszType = pDataDesc->size ? pDataDesc->type : "None";
   if (!pDataDesc->size || !*pDataDesc->type || !bFull)
   {
      lg_fprintf(f, "Link %-22s : %-25s // flags 0x%04X\n", name, pszType, pDesc->flags);
      return;
   }
   lg_fprintf(f, "Link %-22s // type %-25s , flags 0x%04X\n", name, pszType, pDesc->flags);
   AutoAppIPtr(StructDescTools);
   const sStructDesc *pStruct = pStructDescTools->Lookup(pDataDesc->type);
   if (pStruct)
      dbDumpStruct(f, pStruct, TRUE, "");
}

static void dbDumpEffect(LGFILE *f, const sReactionDesc *pDesc, BOOL bFull)
{
   char ui[256];
   char name[48];
   ui[0] = '\0';
   if (pDesc->friendly_name[0])
      sprintf(ui, " , editor name: \"%s\"", pDesc->friendly_name);
   sprintf(name, "\"%s\"", pDesc->name);
   const char *pszType = pDesc->param_type.name;
   if (!*pszType)
      pszType = "None";
   else if (bFull)
   {
      lg_fprintf(f, "Effect %-20s // type %-25s , flags 0x%04X%s\n", name, pszType, pDesc->flags, ui);
      AutoAppIPtr(StructDescTools);
      const sStructDesc *pStruct = pStructDescTools->Lookup(pszType);
      if (pStruct)
         dbDumpStruct(f, pStruct, TRUE, "");
      return;
   }
   lg_fprintf(f, "Effect %-20s : %-25s // flags 0x%04X%s\n", name, pszType, pDesc->flags, ui);
}

static void dbDumpPropagator(LGFILE *f, const sPropagatorDesc *pDesc, const sStructDesc *pShape,
                             const sStructDesc *pLife, BOOL bFull)
{
   char name[48];
   sprintf(name, "\"%s\"", pDesc->name);
   lg_fprintf(f, "Propagator %-20s // flags 0x%04X\n", name, pDesc->flags);
   if (!pShape && !pLife)
      return;
   lg_fputs("{\n", f);
   const sStructDesc *descs[2] = { pShape, pLife };
   const char *parts[2] = { "Shape", "Life" };
   for (int i = 0; i < 2; i++)
   {
      if (!descs[i])
         continue;
      if (!bFull)
         lg_fprintf(f, "\t%s : %s\n", parts[i], descs[i]->name);
      else
      {
         lg_fprintf(f, "\t%s // type %s\n", parts[i], descs[i]->name);
         dbDumpStruct(f, descs[i], TRUE, "\t");
      }
   }
   lg_fputs("}\n", f);
}

static void dbDumpProperties(BOOL bFull)
{
   AutoAppIPtr(PropertyManager);
   AutoAppIPtr(StructDescTools);
   if (!pPropertyManager || !pStructDescTools)
      return;
   sPropertyIter iter;
   if (FAILED(pPropertyManager->BeginIter(&iter)))
      return;
   LGFILE *f = lg_fopen("proplist.txt", "w");
   if (!f)
      return;

   IProperty *pProp;
   while ((pProp = pPropertyManager->NextIter(&iter)) != NULL)
   {
      const sPropertyTypeDesc *pType = pProp->DescribeType();
      const sPropertyDesc *pDesc = pProp->Describe();
      if (pDesc && pType && !(pDesc->flags & kPropertyTransient))
      {
         if (pStructDescTools->Lookup(pType->type))
         {
            dbDumpObjProp(f, pDesc, pType->type, bFull);
            if (bFull)
               lg_fputs("\n", f);
         }
         else
         {
            IStringProperty *pStrProp;
            if (SUCCEEDED(pProp->QueryInterface(IID_IStringProperty, (void **)&pStrProp)))
            {
               dbDumpStringProp(f, pDesc, bFull);
               if (bFull)
                  lg_fputs("\n", f);
               pStrProp->Release();
            }
         }
      }
      pProp->Release();
   }

   AutoAppIPtr(LinkManager);
   if (pLinkManager != NULL)
   {
      lg_fputs("\n------------------------------------------------------------------------------\n", f);
      lg_fputs("LINK TYPES\n", f);
      lg_fputs("------------------------------------------------------------------------------\n\n", f);
      sRelationIter relIter;
      RelationID id;
      pLinkManager->IterStart(&relIter);
      while (pLinkManager->IterNext(&relIter, &id))
      {
         IRelation *pRel = pLinkManager->GetRelation(id);
         const sRelationDesc *pDesc = pRel->Describe();
         const sRelationDataDesc *pDataDesc = pRel->DescribeData();
         if (pDesc && !(pDesc->flags & (kRelationTransient | kRelationNoEdit)))
         {
            dbDumpLink(f, pDesc, pDataDesc, bFull);
            if (bFull)
               lg_fputs("\n", f);
         }
         pRel->Release();
      }
      pLinkManager->IterStop(&relIter);
   }

   AutoAppIPtr(Reactions);
   if (pReactions != NULL)
   {
      lg_fputs("\n------------------------------------------------------------------------------\n", f);
      lg_fputs("RECEPTRON EFFECTS\n", f);
      lg_fputs("------------------------------------------------------------------------------\n\n", f);
      IReactionQuery *pQuery = pReactions->QueryAll();
      if (SUCCEEDED(pQuery->Start()))
      {
         for (; !pQuery->Done(); pQuery->Next())
         {
            const sReactionDesc *pDesc = pQuery->Reaction();
            if (pDesc)
            {
               dbDumpEffect(f, pDesc, bFull);
               if (bFull)
                  lg_fputs("\n", f);
            }
         }
      }
      pQuery->Release();
   }

   AutoAppIPtr(Propagation);
   if (pPropagation != NULL)
   {
      lg_fputs("\n------------------------------------------------------------------------------\n", f);
      lg_fputs("STIM SOURCE PROPAGATORS\n", f);
      lg_fputs("------------------------------------------------------------------------------\n\n", f);
      IPropagatorQuery *pQuery = pPropagation->GetPropagators(OBJ_NULL);
      if (SUCCEEDED(pQuery->Start()))
      {
         for (; !pQuery->Done(); pQuery->Next())
         {
            IPropagator *pGator = pQuery->Propagator();
            const sPropagatorDesc *pDesc = pGator->Describe();
            const sStructDesc *pShape = pGator->DescribeShapes();
            const sStructDesc *pLife = pGator->DescribeLifeCycles();
            if (pDesc)
            {
               dbDumpPropagator(f, pDesc, pShape, pLife, bFull);
               if (bFull)
                  lg_fputs("\n", f);
            }
            pGator->Release();
         }
      }
      pQuery->Release();
   }

   if (bFull)
   {
      lg_fputs("\n------------------------------------------------------------------------------\n", f);
      dbDumpTypeDefinitions(f);
   }
   lg_fclose(f);
}

static void dbDumpTagBlocks(int mode)
{
   cFileVarList *pList = cFileVarBase::AllFileVars();
   if (!pList)
      return;
   AutoAppIPtr(StructDescTools);
   if (!pStructDescTools)
      return;
   LGFILE *f = lg_fopen(mode == 2 ? "taglist_vals.txt" : "taglist.txt", "w");
   if (!f)
      return;

   static const eWhichVarFile files[2] = { kMissionVar, kGameSysVar };
   static const char *headers[2] = { "MISSION:\n--------\n\n", "\n\nGAMESYS:\n--------\n\n" };
   for (int i = 0; i < 2; i++)
   {
      lg_fputs(headers[i], f);
      for (cFileVarList::cIter iter = pList->Iter(); !iter.Done(); iter.Next())
      {
         cFileVarBase *pVar = iter.Value();
         if (pVar->mDesc->file != files[i] || !pStructDescTools->Lookup(pVar->mDesc->type))
            continue;
         dbDumpTagBlock(f, pVar->mDesc, pVar, mode);
         if (mode)
            lg_fputs("\n", f);
      }
   }
   if (mode == 1)
   {
      lg_fputs("\n------------------------------------------------------------------------------\n", f);
      dbDumpTypeDefinitions(f);
   }
   lg_fclose(f);
}

void dbDumpPropertyList(void)
{
   dbDumpProperties(FALSE);
}

void dbDumpPropertyListFull(void)
{
   dbDumpProperties(TRUE);
}

void dbDumpTagList(void)
{
   dbDumpTagBlocks(0);
}

void dbDumpTagListFull(void)
{
   dbDumpTagBlocks(1);
}

void dbDumpTagListValues(void)
{
   dbDumpTagBlocks(2);
}

#define DBPATCH_SEARCH_FLAGS 0x2000
#define DBPATCH_MAX_STORES   64

static char g_dbPatchMission[256];
static ulong g_dbPatchMissionFlags;
static ulong *g_pdbPatchFound;
static sDbPatchEntry *g_pdbPatchScripts;
static BOOL g_dbPatchLoading;

static BOOL dbPatchHasExt(const char *pszName, const char *pszExt)
{
   if (!pszName || !*pszName)
      return FALSE;
   int nExt = strlen(pszExt);
   int nName = strlen(pszName);
   if (nName < nExt + 1)
      return FALSE;
   const char *p = pszName + nName - nExt;
   return p[-1] == '.' && !stricmp(p, pszExt);
}

static void dbPatchLoadScripts(sDbPatchEntry **ppScripts)
{
   for (sDbPatchEntry *p = *ppScripts; p; p = p->next)
      ScriptPropEnsureModule(p->name);
   dbPatchFreeList(ppScripts);
}

struct sDbPatchDirNames
{
   cAnsiStr path;
   sDbPatchEntry *pNames;
};

static void dbPatchInsertSorted(sDbPatchEntry **ppList, const char *pszName)
{
   sDbPatchEntry **pp = ppList;
   while (*pp && stricmp(pszName, (*pp)->name) >= 0)
      pp = &(*pp)->next;
   sDbPatchEntry *pNext = *pp;
   *pp = NULL;
   new sDbPatchEntry(pszName, NULL, pp);
   (*pp)->next = pNext;
}

static void dbPatchRequestDir(sDbPatchEntry **ppQueue, const char *pszDir, ulong flags, int pass,
                              sDbPatchEntry **ppScripts)
{
   if (!g_dbModMode || (pass == kDbPatchEnum && !ppQueue))
      return;
   if ((flags & kDBSaveGame) && g_dbModMode < 2)
   {
      if (g_dbPatchLogLevel > 4 && pass != kDbPatchEnum)
      {
         dbPatchOpenLog();
         DBPATCH_LOG(DBPATCH_DIAG, "DIAG: patch request for savegame ignored, '%s*.dml' (%x)\n", pszDir, flags);
         dbPatchCloseLog();
      }
      return;
   }

   dbPatchOpenLog();
   if (pass == kDbPatchScripts)
      DBPATCH_LOG(DBPATCH_DIAG, "DIAG: processing script dependencies of patches for '%s*.dml' (%x)\n", pszDir, flags);
   else if (pass == kDbPatchEnum)
      DBPATCH_LOG(DBPATCH_DIAG, "DIAG: enumerating patches in '%s*.dml' (%x)\n", pszDir, flags);
   else
      DBPATCH_LOG(DBPATCH_DIAG, "DIAG: patch request for '%s*.dml' (%x)\n", pszDir, flags);

   void *pCookie = gContextPath->BeginContents("*.dml", DBPATCH_SEARCH_FLAGS, pszDir);
   if (pCookie)
   {
      sDbPatchDirNames dirs[DBPATCH_MAX_STORES];
      int nDirs = 0;
      IStore *pLastStore = NULL;
      IStore *pStore;
      IStore *pCanon;
      char szName[256];
      for (int i = 0; i < DBPATCH_MAX_STORES; i++)
         dirs[i].pNames = NULL;
      while (gContextPath->Next(pCookie, &pStore, szName, &pCanon))
      {
         if (dbPatchHasExt(szName, "dml"))
         {
            if (pStore != pLastStore && nDirs < DBPATCH_MAX_STORES)
            {
               pLastStore = pStore;
               dirs[nDirs].path = pStore->GetFullPathName();
               nDirs++;
            }
            if (nDirs)
               dbPatchInsertSorted(&dirs[nDirs - 1].pNames, szName);
         }
         SafeRelease(pCanon);
         SafeRelease(pStore);
      }
      gContextPath->EndContents(pCookie);

      while (nDirs > 0)
      {
         sDbPatchDirNames *pDir = &dirs[--nDirs];
         for (sDbPatchEntry *p = pDir->pNames; p; p = p->next)
         {
            BOOL bQueued = FALSE;
            int result = dbPatchLoadFile(p->name, pDir->path, flags, pass, &bQueued, ppScripts);
            if (ppQueue)
            {
               if (pass == kDbPatchEnum)
               {
                  if (result > 0)
                     new sDbPatchEntry(p->name, pDir->path, ppQueue);
               }
               else if (bQueued)
                  new sDbPatchEntry(p->name, pDir->path, ppQueue);
            }
         }
         dbPatchFreeList(&pDir->pNames);
      }
   }

   if (pass == kDbPatchEnum)
      DBPATCH_LOG(DBPATCH_DIAG, "DIAG: ------ patch enum request finished ------\n\n");
   else
      DBPATCH_LOG(DBPATCH_DIAG, "DIAG: ------ patch request finished ------\n\n");
   dbPatchCloseLog();
}

void dbPatchRequestScripts(void)
{
   char szDir[256];
   if (!(g_dbPatchMissionFlags & kDBMission) || !g_dbPatchMission[0] || !g_dbPatchLoading)
      return;
   if (!(g_dbPatchMissionFlags & kDBSaveGame) || g_dbModMode >= 2)
   {
      sprintf(szDir, "dbmods\\%s\\", g_dbPatchMission);
      dbPatchRequest(NULL, "miss_all", g_dbPatchMissionFlags, g_pdbPatchFound, kDbPatchScripts, &g_pdbPatchScripts);
      dbPatchRequest(NULL, g_dbPatchMission, g_dbPatchMissionFlags, g_pdbPatchFound, kDbPatchScripts, &g_pdbPatchScripts);
      dbPatchRequestDir(NULL, "dbmods\\miss_all\\", g_dbPatchMissionFlags, kDbPatchScripts, &g_pdbPatchScripts);
      dbPatchRequestDir(NULL, szDir, g_dbPatchMissionFlags, kDbPatchScripts, &g_pdbPatchScripts);
   }
   dbPatchLoadScripts(&g_pdbPatchScripts);
}

static void dbPatchSetMission(const char *pszName, ulong flags)
{
   strncpy(g_dbPatchMission, pszName, sizeof(g_dbPatchMission) - 1);
   g_dbPatchMission[sizeof(g_dbPatchMission) - 1] = '\0';
   g_dbPatchMissionFlags = flags;
   g_dbPatchLoading = TRUE;
}

static void dbPatchLoadDone(void)
{
   dbPatchFreeList(&g_pdbPatchScripts);
   g_dbPatchLoading = FALSE;
}

#define DB_INCREMENTAL_BACKUP_TRIES 16

void dbIncrementalBackup(const char *pszFile)
{
   char szDir[256];
   char szTarget[256];
   char szExt[256];

   if (!pszFile || !*pszFile)
      return;
   if (!config_is_defined("incremental_save_dir") || !config_get_raw("incremental_save_dir", szDir, sizeof(szDir)))
      return;
   if (!szDir[0] || !dir_exists(szDir))
   {
      mprintf("WARNING: no incremental save done, incremental save directory '%s' not found\n", szDir);
      return;
   }
   int len = strlen(szDir);
   if (szDir[len - 1] != '\\' && szDir[len - 1] != '/')
      strcat(szDir, "\\");

   SYSTEMTIME time;
   GetLocalTime(&time);
   for (int i = 0; i <= DB_INCREMENTAL_BACKUP_TRIES; i++)
   {
      strcpy(szTarget, szDir);
      strcat(szTarget, strip_pathname(pszFile));
      char *pDot = strrchr(szTarget, '.');
      if (!pDot)
         return;
      strcpy(szExt, pDot);
      sprintf(pDot + 1, "%04d%02d%02d%02d%02d%02d", time.wYear, time.wMonth, time.wDay, time.wHour,
              time.wMinute, time.wSecond + i);
      strcat(szTarget, szExt);
      if (_access(szTarget, 0) == -1)
      {
         if (!CopyFileNamed(szTarget, pszFile))
            mprintf("WARNING: failed to backup incremental save '%s'\n", szTarget);
         else
            mprintf("Made backup incremental backup '%s'\n", szTarget);
         return;
      }
   }
   mprintf("WARNING: failed to backup incremental save for '%s', couldn't find unused filename\n", pszFile);
}

static sFileMenuFilter dbPatchFileFilters[] =
{
   { "DbMod Files (*.dml)", "*.dml" },
   { "All Files (*.*)", "*.*" },
   { NULL, NULL },
};

static char g_dbPatchManualBuf[_MAX_PATH];

void dbPatchManual(char *pszArg)
{
   char szDir[_MAX_PATH];
   char szCwd[_MAX_PATH];

   if (!pszArg || !*pszArg)
   {
      pszArg = FileDialog(kFileMenuLoad, dbPatchFileFilters, "DML", g_dbPatchManualBuf, sizeof(g_dbPatchManualBuf));
      if (!pszArg || !*pszArg)
         return;
   }

   memset(szDir, 0, sizeof(szDir));
   char *pSlash = strrchr(pszArg, '\\');
   char *pFwd = strrchr(pSlash ? pSlash : pszArg, '/');
   if (pFwd && (!pSlash || pFwd > pSlash))
      pSlash = pFwd;
   char *pszName = pszArg;
   if (pSlash)
   {
      pszName = pSlash + 1;
      int nDir = pszName - pszArg;
      memcpy(szDir, pszArg, nDir);
      szDir[nDir] = '\0';
      for (char *p = szDir; *p; p++)
         if (*p == '/')
            *p = '\\';
      _getcwd(szCwd, sizeof(szCwd));
      int nCwd = strlen(szCwd);
      if (szCwd[nCwd - 1] == '\\')
         szCwd[--nCwd] = '\0';
      if (nCwd < nDir && !strnicmp(szDir, szCwd, nCwd))
      {
         szDir[0] = '.';
         memmove(szDir + 1, szDir + nCwd, nDir - nCwd + 1);
      }
   }
   if (!*pszName)
      return;

   BOOL bQueued;
   dbPatchOpenLog();
   DBPATCH_LOG(DBPATCH_DIAG, "DIAG: manual patch request for '%s' (%x)\n", pszArg, kDBMission | kDBGameSys);
   dbPatchLoadFile(pszName, szDir[0] ? szDir : NULL, kDBMission | kDBGameSys, kDbPatchPreLoad, &bQueued, &g_pdbPatchScripts);
   DBPATCH_LOG(DBPATCH_DIAG, "DIAG: ------ manual patch request finished ------\n\n");
   dbPatchCloseLog();
   dbPatchLoadScripts(&g_pdbPatchScripts);
}

