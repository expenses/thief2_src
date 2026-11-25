#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <direct.h>
#include <dos.H>

#include <lg.h>
#include <hashfns.h>
#include <config.h>
#include <hash.h>
#include <lgsprntf.H>
#include <cfgdbg.h>
#include <mprintf.h>
#include <lg_file.h>

#include <sys\stat.h>	// added by cnn - 8/29/96
#include <nameconv.h>	// added by cnn - 8/29/96
#include <recapi.h>	// added by njt - 8/28/97
#include <comtools.h>	// added by njt - 8/28/97
#include <appagg.h>	// added by njt - 8/28/97
#include <appapi.h>	// added by njt - 8/28/97

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

// -------
// DEFINES
// -------


#define INITIAL_TBLSIZE 10
#define LINE_BUFSIZE 8192
#define FN_BUFSIZE 15
#define DELIM_CHAR ' '
#define COMMENT_CHAR ';'
#define IS_DELIM(c) (isspace((unsigned char)(c)) || (c) == ',')
#define DEF_CHAR '+'
#define UNDEF_CHAR '-'
#define ALT_UNDEF_CHAR '/'

#define PRIORITY_COMMANDLINE ((uint)-1)

typedef struct _filename_rec
{
   uint refcount;
   char text[1];
} filename_rec;

typedef struct _config_elem
{
   char* var;
   char* val;
   int vallen;
   uint priority;
   filename_rec* origin;
} config_elem;



// -------
// GLOBALS
// -------

//bool (*config_writables)(char* var);
Hashtable config_table;
static int config_enabled = 0;
static char equals_and_colon[] = "=:";                // yep, that's an equals and a colon all right
static char *assign_str = equals_and_colon;
static char pszConfigElem[] = "config elem";


//-----------------------
// GOOFY FILENAME RECORD
//-----------------------

filename_rec* alloc_filename_rec(const char* fn)
{
   filename_rec* rec = malloc(sizeof(filename_rec)+strlen(fn));
   if (rec == NULL) return NULL;
   strcpy(rec->text,fn);
   rec->refcount = 1;
   return rec;
}

void ref_filename_rec(filename_rec* rec)
{
   rec->refcount++;
}

void free_filename_rec(filename_rec* rec)
{
   rec->refcount--;
   if (rec->refcount == 0)
      free(rec);
}

static char* config_strdup(const char* s)
{
   char* p;
   int len;
   if (s == NULL)
   {
      p = malloc(1);
      *p = '\0';
      return p;
   }
   len = strlen(s)+1;
   p = malloc(len);
   memcpy(p,s,len);
   return p;
}

static void config_set_elem_val(config_elem* e, const char* val)
{
   int len;
   if (val == NULL)
      val = "";
   if (e->val == NULL)
   {
      e->val = config_strdup(val);
      e->vallen = strlen(e->val);
      return;
   }
   len = strlen(val);
   if (len > e->vallen || e->vallen - len > 64)
   {
      free(e->val);
      e->val = malloc(len+1);
      e->vallen = len;
   }
   memcpy(e->val,val,len+1);
}

void config_copy_elem(config_elem* t, config_elem* s)
{
   t->var = config_strdup(s->var);
   t->val = NULL;
   t->vallen = 0;
   config_set_elem_val(t,s->val);
   t->priority = s->priority;
   t->origin = s->origin;
   if (t->origin != NULL)
      ref_filename_rec(t->origin);
}

void config_free_elem(config_elem* e)
{
   if (e->var != NULL)
      free(e->var);
   if (e->val != NULL)
      free(e->val);
   if (e->origin != NULL)
      free_filename_rec(e->origin);
}



// ---------
// INTERNALS
// ---------

char* fdgets(int fd, char* buf, int  bufsiz)
{
   char* s;
   for (s = buf; s < buf + bufsiz - 1 && !lg_eof(fd); s++)
   {
      lg_read(fd,s,1);
      if (*s == '\n')
         break;
   }
   *s = '\0';
   return buf;
}

int config_hashfunc(config_elem* e)
{
   return (int) HashStringCaseless(e->var);
}

int config_equfunc(config_elem* e1, config_elem* e2)
{
   return config_compare(e1->var,e2->var);
}

int config_compare(const char* var1, const char* var2)
{
   return stricmp(var1,var2);
}

errtype config_set_raw_value(const char* var, char* val, uint priority)
{
   config_elem e;
   config_elem *r;
   if (var == NULL) return ERR_NULL;
   e.var = (char*)var;
   hash_lookup(&config_table,&e,&r);
   if (r == NULL)
   {
      e.var = config_strdup(e.var);
      e.val = NULL;
      e.vallen = 0;
      config_set_elem_val(&e,val);
      e.priority = priority;
      e.origin = NULL;
      return hash_set(&config_table,&e);
   }
   else
   {
      r->priority = max(r->priority,priority);
      if (r->origin != NULL)
         free_filename_rec(r->origin);
      r->origin = NULL;
      config_set_elem_val(r,val);
      return OK;
   }
}




void config_parse_line(char* buf, char** var, char** val,char** com)
{
   char *b,*s;
   *var = *val = *com = NULL;
   // Look for the comment character, and end the string there
   for (b = buf; *b != '\0'; b++)
      if (*b == COMMENT_CHAR)
      {
         *b = '\0';
         *com = b+1;
         break;
      }
   for (s = buf; isspace((unsigned char)*s); s++);
   if (*s != '\0')
   {
      char* v = s;
      while (!isspace((unsigned char)*s) && *s != '\0') s++;
      if (*s != '\0')
      {
         *s = '\0';
         s++;
         // go past the next whitespace.
         while(isspace((unsigned char)*s)) s++;
      }
      *var = v;
      *val = s;
   }
}


// -------------
// API FUNCTIONS
// -------------

errtype config_init(void)
{
   errtype err;
   if (config_enabled++ > 0) return ERR_NOEFFECT;
   err = hash_init(&config_table,sizeof(config_elem),INITIAL_TBLSIZE,config_hashfunc,config_equfunc);
   if (err != OK) return err;
//   config_writables = writeable;
   return OK;
}

#ifdef OLD_WRITABLES
errtype config_set_writables(bool (*writable)(char* var))
{
   config_writables = writable;
   return OK;
}
#endif

errtype config_set_priority(const char* var, uint priority)
{
   config_elem e;
   config_elem *out;
   e.var = (char*)(var ? var : "");
   hash_lookup(&config_table,&e,&out);
   if (out == NULL) return ERR_NOEFFECT;
   out->priority = priority;
   return OK;
}


uint config_get_priority(const char* var)
{
   config_elem e;
   config_elem *out;
   e.var = (char*)(var ? var : "");
   hash_lookup(&config_table,&e,&out);
   if (out == NULL) return PRIORITY_TRANSIENT;
   return out->priority;
}

errtype config_get_origin(const char* var, char* buf, int buflen)
{
   config_elem e;
   config_elem *out;
   e.var = (char*)(var ? var : "");
   hash_lookup(&config_table,&e,&out);
   if (out == NULL || out->origin == NULL)
   {
      buf[0] = '\0';
   }
   else
   {
      strncpy(buf,out->origin->text,buflen);
      buf[buflen-1]='\0';
   }
   return OK;
}

errtype config_set_from_file(const char* var, char* val, uint priority, filename_rec* rec)
{
   config_elem e;
   config_elem *r;
   if (var == NULL) return ERR_NULL;
   e.var = (char*)var;
   hash_lookup(&config_table,&e,&r);
   if (r == NULL)
   {
      e.var = config_strdup(var);
      e.val = NULL;
      e.vallen = 0;
      config_set_elem_val(&e,val);
      ref_filename_rec(rec);
      e.priority = priority;
      e.origin = rec;
      return hash_set(&config_table,&e);
   }
   else
   {
      if (r->priority > priority)
         return ERR_NOEFFECT;
      r->priority = priority;
      ref_filename_rec(rec);
      if (r->origin != NULL)
         free_filename_rec(r->origin);
      r->origin = rec;
      config_set_elem_val(r,val);
      return OK;
   }
}

errtype config_read_file(const char* fn, uint (*readfunc)(char* var))
{
   LGFILE *fp;
   filename_rec* rec;
   if (config_enabled <= 0) return ERR_NULL;

   fp = lg_fopen(fn,"r");
   if (fp == NULL) return ERR_FOPEN;

   rec = alloc_filename_rec(fn);


   while(!feof(fp->fp))
   {
      char *var, *val, *com;
      char buf[LINE_BUFSIZE];
      uint priority;
      if (!fgets(buf,LINE_BUFSIZE,fp->fp))
         break;
      buf[LINE_BUFSIZE-1] = '\0';
      if (buf[strlen(buf)-1] == '\n')
         buf[strlen(buf)-1] = '\0';
      config_parse_line(buf,&var,&val,&com);
      if (readfunc != NULL)
         priority = (*readfunc)(var);
      else
         priority = 1;
      if (priority == 0)
         continue;
      if (var != NULL)
         config_set_from_file(var,val,priority,rec);
   }
   lg_fclose(fp);
   free_filename_rec(rec);
   return OK;
}

static bool config_write_iter_success=FALSE;

void config_write_line(int fd, const char* var, char* val, char* com)
{
   char buf[LINE_BUFSIZE];
   int len;
   strncpy(buf,var,LINE_BUFSIZE);
   buf[LINE_BUFSIZE-1] = '\0';
   buf[strlen(var)] = DELIM_CHAR;
   strncpy(buf+strlen(var)+1,val,LINE_BUFSIZE-(strlen(var)+1));
   buf[LINE_BUFSIZE-1] = '\0';
   if (com != NULL)
   {
      int s = strlen(buf);
      if (s < LINE_BUFSIZE-2)
      {
         buf[s] = COMMENT_CHAR;
         strncpy(buf+s+1,com,LINE_BUFSIZE-1-s);
      }
   }
   strncat(buf,"\n",LINE_BUFSIZE-1);
   buf[LINE_BUFSIZE-2] = '\n';
   buf[LINE_BUFSIZE-1] = '\0';
   len = strlen(buf);
   if (lg_write(fd,buf,len) != len)
      config_write_iter_success=FALSE;
}

typedef struct _config_iter_struct
{
   int fd;
   Hashtable *tbl;
   char* fn;
   writefunc writable;
} citer;

bool config_write_iter(config_elem* e, citer* iter)
{
   if (e->priority == PRIORITY_TRANSIENT
      || e->priority == PRIORITY_COMMANDLINE) return FALSE;
   if (iter->writable != NULL && !iter->writable(iter->fn,e->var)) return FALSE;
   config_write_line(iter->fd,e->var,e->val,NULL);
   return FALSE;
}

#define IS_DIRCHAR(c) ((c) == '/' || (c) == '\\' || (c) == ':')

void split_fname(const char* fname, char* dirbuf, char* fbuf)
{
   const char* s;
   const char* last = fname;
   int len;
   for (s = fname; *s != '\0'; s++)
      if (IS_DIRCHAR(*s))
         last = s+1;
   len = last - fname;
   if (len)
      memcpy(dirbuf,fname,len);
   dirbuf[len] = '\0';
   strcpy(fbuf,last);
}

bool config_write_to_same_file( char* filename, char* var )
{

   char buf[_MAX_PATH];
   /* I will assume that I'm not about to stricmp() buffers bigger than PATH_MAX... */

   config_get_origin( var, buf, _MAX_PATH );
   return (!stricmp( buf, filename ));
}


#ifdef NEED_CHANGE_DRIVE
#define DRIVE_TO_DRNUM(x)  (toupper(*(x)) - 'A' + 1)

#define DRIVE_CHAR ':'

void _change_dir(char* path)
{
   uint drnum = 0;
   char* colon;
   while(isspace((unsigned char)*path)) path++;

   colon = strchr(path,DRIVE_CHAR);
   if (colon != NULL)
   {
      *colon = '\0';
      drnum = DRIVE_TO_DRNUM(path);
      *colon = DRIVE_CHAR;
      colon++;
   }
   else
      colon = path;
   chdir(path);
   if (drnum > 0)
   {
      uint tot;
      _dos_setdrive(drnum,&tot);
   }
}
#endif // NEED_CHANGE_DRIVE

errtype config_write_file(const char* input_fn,writefunc writable)
{
   errtype err;
   int ofd,ifd;
   char fn[_MAX_PATH];
   char dbuf[_MAX_PATH],fbuf[_MAX_PATH];
   citer iter;
   Hashtable seen_vars;

   err = hash_copy(&seen_vars,&config_table);
   if (err != OK) return err;
   if (config_enabled <= 0) return ERR_NULL;
   split_fname(input_fn,dbuf,fbuf);

   // Construct a temporary filename
   strcpy(fn,dbuf);
   strcat(fn,"_");
   strcat(fn,fbuf+1);
   ofd = lg_open(fn,O_WRONLY|O_CREAT|O_TRUNC|O_TEXT,S_IWRITE);
   if (ofd == -1) return ERR_FOPEN;
   config_write_iter_success=TRUE;

   ifd = lg_open(input_fn,O_RDONLY|O_TEXT);
   if (ifd != -1)
   {
      while(!lg_eof(ifd))
      {
         config_elem e;
         config_elem *out;
         char *var, *val, *com;
         char  buf[LINE_BUFSIZE];
         char rbuf[LINE_BUFSIZE];

         fdgets(ifd,buf,LINE_BUFSIZE);
         strncpy(rbuf,buf,LINE_BUFSIZE);
         rbuf[LINE_BUFSIZE-1] = '\0';

         config_parse_line(buf,&var,&val,&com);
         e.var = (char*)(var ? var : "");
         hash_lookup(&config_table,&e,&out);

         if (var != NULL && out != NULL
             && out->priority != PRIORITY_TRANSIENT
             && out->priority != PRIORITY_COMMANDLINE)
         {
            config_write_line(ofd,var,out->val,com);
            config_unset(var);
         }
         else
         {
            strcat(rbuf,"\n");
            lg_write(ofd,rbuf,strlen(rbuf));
         }
      }
      lg_close(ifd);
   }
   iter.fd = ofd;
   iter.writable = writable;
   iter.fn = (char *)input_fn;
   hash_iter(&config_table,(HashIterFunc)config_write_iter,&iter);
   lg_close(ofd);
   hash_destroy(&config_table);
   config_table = seen_vars;
   if (config_write_iter_success)
   {
      remove(input_fn);
      rename(fn,input_fn);
   }
   else
      remove(fn);
   if (config_write_iter_success)
      return OK;
   else
      return ERR_FOPEN;
}

uint config_default_priority = CONFIG_DFT_LO_PRI;

errtype config_set_value(const char* varname, int type, void* fillvec, int cnt)
{
   int i;
   char buf[LINE_BUFSIZE];
   char *s = buf;
   for (i = 0; i < cnt; i++)
   {
      switch(type)
      {
         case CONFIG_STRING_TYPE:
            strcpy(s,((char**)fillvec)[i]);
            break;
         case CONFIG_INT_TYPE:
            itoa(((int*)fillvec)[i],s,10);
            break;
         case CONFIG_FLOAT_TYPE:
            gcvt(((float *)fillvec)[i],10,s);
            break;
      }
      if (i < cnt-1)
      {
         s+= strlen(s);
         if (s >= buf + LINE_BUFSIZE - 1)
         {
            buf[LINE_BUFSIZE-1] = '\0';
            break;
         }
         *(s++) = DELIM_CHAR;
      }
   }
   return config_set_raw_value(varname,buf,config_default_priority);
}

errtype config_set_single_value(const char* varname, int type, config_valtype value)
{
   char buf[LINE_BUFSIZE];
   char *s = buf;
   switch(type)
      {
         case CONFIG_STRING_TYPE:
            strncpy(s,(char*)value,LINE_BUFSIZE);
            buf[LINE_BUFSIZE-1] = '\0';
            break;
         case CONFIG_INT_TYPE:
            itoa((int)value,s,10);
            break;
         case CONFIG_FLOAT_TYPE:
            gcvt(*(float *)(&value),10,s);
            break;

      }
   return config_set_raw_value(varname,buf,config_default_priority);
}



bool config_get_value(const char* varname, int type, void* fillvec, int* cnt)
{
   config_elem e;
   config_elem *r;
   char* s;
   int max = (*cnt > 0) ? *cnt : -1;
   *cnt = 0;
   e.var = (char*)(varname ? varname : "");
   hash_lookup(&config_table,&e,&r);
   if (r == NULL) return FALSE;
   s = r->val;
   while(isspace((unsigned char)*s)) s++;
   while(*s != '\0' && max > 0)
   {
      char* v;
      char tmp;
      v = s;
      while(!IS_DELIM(*v) && *v != '\0') v++;
      tmp = *v;
      *v = '\0';
      switch(type)
      {
         case CONFIG_STRING_TYPE:
            ((char**)fillvec)[*cnt] = malloc(strlen(s)+1);
            strcpy(((char**)fillvec)[*cnt],s);
            break;
         case CONFIG_INT_TYPE:
            ((int*)fillvec)[*cnt] = atoi(s);
            break;
         case CONFIG_FLOAT_TYPE:
            ((float*)fillvec)[*cnt] = atof(s);
            break;
      }
      *v = tmp;
      (*cnt)++;
      if (max > 0) max--;
      s = v;  // skip past delimeter
      if (*s != '\0' && IS_DELIM(*s)) s++; // skip past delimeter
      while(*s != '\0' && isspace((unsigned char)*s)) s++;
   }
   return TRUE;
}

bool config_get_single_value(const char* varname, int type, config_valtype* value)
{
   void *v = value;
   int cnt = 1;
   bool result;
   result = config_get_value(varname,type,v,&cnt);
   return result && cnt > 0;
}

bool config_get_raw(const char* varname, char* buf, int bufsize)
{
   config_elem e;
   config_elem *r;
   e.var = (char*)(varname ? varname : "");
   hash_lookup(&config_table,&e,&r);
   if (r == NULL) return FALSE;
   if (bufsize > 0 && buf != NULL)
   {
      strncpy(buf,r->val,bufsize);
      buf[bufsize-1] = '\0';
   }
   return TRUE;
}

typedef struct _config_iter_rawall_struct
{
   char *buffer;
   int bufsize;
   ConfigIterFunc ifunc;
} citer_rawall;

bool config_get_raw_all_iter(config_elem* e, citer_rawall *iter)
{
   strncpy(iter->buffer,e->val,iter->bufsize);
   iter->buffer[iter->bufsize-1] = '\0';
   return (iter->ifunc)(e->var);
}

void config_get_raw_all(ConfigIterFunc ifunc, char* buf, int bufsize)
{
   citer_rawall iter;

   iter.buffer = buf;
   iter.bufsize = bufsize;
   iter.ifunc = ifunc;

   hash_iter(&config_table,(HashIterFunc)config_get_raw_all_iter,&iter);
}

void config_set_assign_characters (char* str)
{
   assign_str = str;
}

errtype config_parse_commandline(int argc, const char* argv[], bool (*do_switch)(char c))
{
   int i;
   for (i = 1; i < argc; i++)
   {
      char* s = (char*)argv[i];
      if (do_switch != NULL && isSwitch(*s))
      {
         for (s++;*s != '\0';s++)
            if (do_switch(*s))
            {
               char sw[2] = "X";
               const char* val = NULL;
               sw[0] = *s;
               s++;
               if (*s != 0)
                  val = s;
               else if (++i < argc)
               {
                  val = argv[i];
               }
               config_set_raw_value(sw,(char*)val,PRIORITY_COMMANDLINE);
               break;
            }

      }
      else switch (*s)
      {

         case DEF_CHAR:
            s++;
            config_set_raw_value(s,"",PRIORITY_COMMANDLINE);
            break;
         case UNDEF_CHAR:
         case ALT_UNDEF_CHAR:
            s++;
            config_unset(s);
            break;
         default:
         {
            char* e = strpbrk((char*)argv[i],assign_str);
            if (e && *e)
            {
               char tmp;
               char *v = s;
               while(v < e && !isspace((unsigned char)*v)) v++;
               tmp = *v;
               *v = '\0';
               e++;
               if (*e != '\0')
                  config_set_raw_value(s,e,PRIORITY_COMMANDLINE);
               else
                  config_unset(s);
               *v = tmp;
            }
         }
      }
   }
   return OK;
}


errtype config_unset(const char* varname)
{
   config_elem e;
   config_elem *r;
   e.var = (char*)(varname ? varname : "");
   hash_lookup(&config_table,&e,&r);

   if (r==NULL) {
#ifdef DBG_ON
     mprintf("Config unset failed!");
#endif
      return -1;
   }

   return hash_delete(&config_table,r);
}


#pragma disable_message(202)
bool config_shutdown_iter(config_elem* e, void* data)
{
#ifndef NO_DUMMIES
   void* dummy = data;
   data = dummy;
#endif // NO_DUMMIES
//   free(e->val);
   return FALSE;
}
#pragma enable_message(202)

errtype config_shutdown(void)
{
   if (--config_enabled > 0) return ERR_NOEFFECT;
   config_enabled = 0;
   hash_iter(&config_table,config_shutdown_iter,NULL);
   return hash_destroy(&config_table);
}

// STANDARD CLIENT IMPLEMENTATION


#define SWITCH_PREFIX "@SWITCH:"


static config_write_spec* cfg_write_table = NULL;
bool config_default_writability = FALSE;


void config_set_writable_table(config_write_spec* tbl)
{
   cfg_write_table = tbl;
}

void config_set_int(const char *varname, int val)
{
   config_set_single_value(varname, CONFIG_INT_TYPE, (config_valtype)val);
}

void config_set_string(const char *varname, const char *val)
{
   config_set_single_value(varname, CONFIG_STRING_TYPE, (config_valtype)val);
}



#pragma off(unreferenced)
bool config_default_writable(char* filename, char* var)
{
   config_write_spec* spec = cfg_write_table;
   char buf[32];

   if (strncmp(SWITCH_PREFIX,var,strlen(SWITCH_PREFIX)) == 0)
   {
      config_set_single_value(var+strlen(SWITCH_PREFIX),CONFIG_INT_TYPE,0);
      return FALSE;
   }

   for(;spec != NULL && spec->var != NULL; spec++)
   {
      if (strcmp(spec->var,var) == 0) return spec->writable;
      // This is a variable that was undefined by a switch
   }
   config_get_raw(var,buf,sizeof(buf));
   if (strcmp(buf,SWITCH_PREFIX)==0) return FALSE;
   return config_default_writability;
}
#pragma on(unreferenced)



static config_switchdef* switchtable = NULL;


void config_set_switch_table(config_switchdef* table)
{
   switchtable = table;
}

bool config_default_switchfunc(char sw)
{
   config_switchdef* def = switchtable;
   if (def == NULL) return FALSE;
   for(;def->sw != '\0';def++)
      if (sw == def->sw)
      {
         bool already_set = config_get_raw(def->var,NULL,0);
         switch(def->type)
         {
         case CFGS_DEFINE:
            if (!already_set)
            {
               config_set_single_value(def->var,CONFIG_STRING_TYPE,"");
               config_set_priority(def->var,PRIORITY_TRANSIENT);
            }

            break;
         case CFGS_UNDEFINE:
            if (already_set)
            {
               char buf[256];
               strcpy(buf,SWITCH_PREFIX);
               strcat(buf,def->var);
               config_set_single_value(buf,CONFIG_INT_TYPE,0);
            }
            config_unset(def->var);
            break;
         case CFGS_VARIABLE:
            return TRUE;
         }
      }
   return FALSE;
}


// the config_elem are saved intact, including their meaningless origin pointers
bool config_save_iter(config_elem* e, IRecorder *recorder)
{
   if (e->var[0] != '\0' || e->val[0] != '\0')
      IRecorder_AddToStream(recorder, e, sizeof(*e), pszConfigElem);
   return FALSE;
}

void config_add_to_or_extract_from_recording( void )
{
   IRecorder *recorder = AppGetObj(IRecorder);
   if (recorder) {
      switch(IRecorder_GetMode(recorder)) {
         case kRecRecord: {
            config_elem e;
            hash_iter(&config_table,(HashIterFunc)config_save_iter,recorder);
            // the end of the config recording is signalled by a 0 elem
            e.var = "";
            e.val = "";
            IRecorder_AddToStream(recorder, &e, sizeof(e), pszConfigElem);
            break;
         }
         case kRecPlayback: {
            config_elem e;
            config_shutdown();
            config_init();
            for (;;) {
               IRecorder_ExtractFromStream(recorder,&e,sizeof(e),pszConfigElem);
               if (e.var[0] == '\0' && e.val[0] == '\0')
                  break;
               // need to nuke the origin, since it's dirty
               e.origin = NULL;
               hash_set(&config_table, &e);
            }
            break;
         }
         default: {
            break;
         }
      }
      SafeRelease(recorder);
   }
}

//////////////////////////////////////////////////
// WACKY DEBUG SYSTEM
//////////////////////////////////////////////////

bool config_spew_on = TRUE;
static char* _spewsrc = NULL;

void config_set_float(const char *varname, float val)
{
   config_set_single_value(varname, CONFIG_FLOAT_TYPE, (*(config_valtype *)(&val)));
}

bool config_is_nonzero(const char *pVarName)
{
   char pBuf[64];
   bool raw;
   pBuf[0] = '\0';
   raw = config_get_raw(pVarName, pBuf, 64);
   return (raw && strcmp(pBuf, "0") != 0);
}

bool config_is_zero(const char *pVarName)
{
   char pBuf[64];
   bool raw;
   pBuf[0] = '\0';
   raw = config_get_raw(pVarName, pBuf, 64);
   return (raw && strcmp(pBuf, "0") == 0);
}

bool CfgSpewTest(char* var)
{
   _spewsrc = var;
   return config_is_defined(var);
}

void CfgDoSpew(char* msg, ...)
{
   char buf[1024];
   va_list ap;
   va_start(ap,msg);
   lg_vsprintf(buf,msg,ap);
#ifdef DBG_ON
   mprintf("%s",buf);
#endif
   va_end(ap);
}

