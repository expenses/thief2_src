#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mprintf.h>
#include <lg_file.h>
#include <lg.h>
#include <datapath.h>

#define LG_FILE_NUM_STD 3


int g_lgFileLogLevel;
static int g_lgFileLastId;
static LGFILE *g_pLGFiles;
static int g_nLGFiles;
static LGFILE *g_pLGStdFiles[LG_FILE_NUM_STD];
static CRITICAL_SECTION g_LGFileLock;

static void LGFileInit(LGFILE *f)
{
   f->id = ++g_lgFileLastId;
   f->size = -1;
   f->open = 0;
   f->stdslot = 0;
}

static void LGFileLink(LGFILE *f)
{
   EnterCriticalSection(&g_LGFileLock);
   g_nLGFiles++;
   if (g_pLGFiles)
      g_pLGFiles->prev = f;
   f->prev = NULL;
   f->next = g_pLGFiles;
   g_pLGFiles = f;
   LeaveCriticalSection(&g_LGFileLock);
}

static void LGFileUnlink(LGFILE *f)
{
   EnterCriticalSection(&g_LGFileLock);
   g_nLGFiles--;
   if (g_pLGFiles == f)
   {
      if (f->next)
         f->next->prev = NULL;
      g_pLGFiles = f->next;
   }
   else
   {
      f->prev->next = f->next;
      if (f->next)
         f->next->prev = f->prev;
   }
   LeaveCriticalSection(&g_LGFileLock);
}

static LGFILE *LGFileFind(int id)
{
   LGFILE *f;
   if (g_pLGFiles->id == id)
      return g_pLGFiles;
   EnterCriticalSection(&g_LGFileLock);
   for (f = g_pLGFiles; f; f = f->next)
   {
      if (f->id == id)
      {
         if (g_pLGFiles != f)
         {
            if (f->next)
               f->next->prev = f->prev;
            f->prev->next = f->next;
            g_pLGFiles->prev = f;
            f->next = g_pLGFiles;
            f->prev = NULL;
            g_pLGFiles = f;
         }
         LeaveCriticalSection(&g_LGFileLock);
         return f;
      }
   }
   LeaveCriticalSection(&g_LGFileLock);
   return NULL;
}

static int LGFileVScanf(FILE *fp, const char *fmt, va_list args)
{
   const char *p;
   int nArgs = 0;
   void **argv;
   int i;
   for (p = fmt; *p; p++)
      if (p[0] == '%' && p[1] != '*' && p[1] != '%')
         nArgs++;
   argv = (void **)_alloca((nArgs + 2) * sizeof(void *));
   argv[0] = fp;
   argv[1] = (void *)fmt;
   for (i = 0; i < nArgs; i++)
      argv[i + 2] = va_arg(args, void *);
   switch (nArgs)
   {
      case 0: return fscanf(fp, fmt);
      case 1: return fscanf(fp, fmt, argv[2]);
      case 2: return fscanf(fp, fmt, argv[2], argv[3]);
      case 3: return fscanf(fp, fmt, argv[2], argv[3], argv[4]);
      case 4: return fscanf(fp, fmt, argv[2], argv[3], argv[4], argv[5]);
      case 5: return fscanf(fp, fmt, argv[2], argv[3], argv[4], argv[5], argv[6]);
      case 6: return fscanf(fp, fmt, argv[2], argv[3], argv[4], argv[5], argv[6], argv[7]);
      case 7: return fscanf(fp, fmt, argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8]);
      default: return fscanf(fp, fmt, argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9]);
   }
}

static int LGFileIllegalName(const char *name)
{
   const char *p = name + strlen(name);
   while (p > name)
   {
      if (*p == '\\' || *p == '/')
      {
         p++;
         break;
      }
      p--;
   }
   if (!strnicmp(p, "com", 3) && isdigit((unsigned char)p[3]) && (p[4] == '\0' || p[4] == '.'))
   {
#ifndef SHIP
      if (g_lgFileLogLevel >= 2)
         mprintf("lg_file: illegal filename \"%s\" detected, file ignored\n", name);
#endif
      return 1;
   }
   return 0;
}

int lg_open(const char *name, int oflag, ...)
{
   char mode[8];
   char *p;
   int bUmask = 0;
   int oldMask = 0;
   int pmode;
   int bGetSize = 0;
   LGFILE *f;
   va_list ap;

   if (oflag & _O_APPEND)
   {
      mode[0] = 'a';
      p = mode + 1;
   }
   else if (oflag & _O_CREAT)
   {
      va_start(ap, oflag);
      pmode = va_arg(ap, int);
      va_end(ap);
      if (oflag & _O_EXCL)
      {
         if (lg_fileattr(name) == 0)
         {
            errno = EEXIST;
            return -1;
         }
         mode[0] = 'w';
         p = mode + 1;
      }
      else if (!(oflag & _O_TRUNC) && lg_fileattr(name) == 0)
      {
         mode[0] = 'r';
         mode[1] = '+';
         p = mode + 2;
      }
      else
      {
         mode[0] = 'w';
         p = mode + 1;
      }
      if (pmode && p[-1] == 'w')
      {
         oldMask = _umask(~pmode & (_S_IREAD | _S_IWRITE));
         bUmask = 1;
      }
      if (oflag & _O_TEMPORARY)
         *p++ = 'T';
      else if (oflag & _O_SHORT_LIVED)
         *p++ = 'D';
   }
   else if (oflag & _O_RDWR)
   {
      mode[0] = (oflag & _O_TRUNC) ? 'w' : 'r';
      mode[1] = '+';
      p = mode + 2;
   }
   else if (oflag & _O_WRONLY)
   {
      if (oflag & _O_TRUNC)
      {
         mode[0] = 'w';
         p = mode + 1;
      }
      else
      {
         mode[0] = 'r';
         mode[1] = '+';
         p = mode + 2;
      }
   }
   else
   {
      mode[0] = 'r';
      bGetSize = 1;
      p = mode + 1;
   }
   *p++ = (oflag & _O_TEXT) ? 't' : 'b';
   if (oflag & _O_RANDOM)
      *p++ = 'R';
   else if (oflag & _O_SEQUENTIAL)
      *p++ = 'S';
   if (oflag & _O_NOINHERIT)
      *p++ = 'N';
   *p = '\0';

   f = lg_fopen(name, mode);
   if (bUmask)
   {
      int err = errno;
      _umask(oldMask);
      errno = err;
   }
   if (!f)
      return -1;
   if (bGetSize)
      f->size = _filelengthi64(_fileno(f->fp));
   f->open = 1;
   return f->id;
}

int lg_close(int fd)
{
   LGFILE *f = LGFileFind(fd);
   if (!f)
   {
      errno = EBADF;
      return -1;
   }
   return lg_fclose(f);
}

int lg_read(int fd, void *buf, unsigned int count)
{
   LGFILE *f = LGFileFind(fd);
   if (!f)
   {
      errno = EBADF;
      return -1;
   }
   return fread(buf, 1, count, f->fp);
}

int lg_write(int fd, const void *buf, unsigned int count)
{
   LGFILE *f = LGFileFind(fd);
   if (!f)
   {
      errno = EBADF;
      return -1;
   }
   return fwrite(buf, 1, count, f->fp);
}

long lg_filelength(int fd)
{
   LGFILE *f = LGFileFind(fd);
   if (!f)
   {
      errno = EBADF;
      return -1;
   }
   return _filelength(_fileno(f->fp));
}

int lg_lseek(int fd, long offset, int origin)
{
   LGFILE *f = LGFileFind(fd);
   if (!f)
   {
      errno = EBADF;
      return -1;
   }
   return fseek(f->fp, offset, origin);
}

__int64 lg_filelengthi64(int fd)
{
   LGFILE *f = LGFileFind(fd);
   if (!f)
   {
      errno = EBADF;
      return -1;
   }
   return _filelengthi64(_fileno(f->fp));
}

int lg_eof(int fd)
{
   __int64 pos, size;
   LGFILE *f = LGFileFind(fd);
   if (!f)
   {
      errno = EBADF;
      return -1;
   }
   pos = _ftelli64(f->fp);
   if (pos == -1)
      return -1;
   size = f->size;
   if (size == -1)
   {
      size = lg_filelengthi64(fd);
      if (size == -1)
         return -1;
   }
   return pos >= size;
}

LGFILE *lg_fopen(const char *name, const char *mode)
{
   char fmode[32];
   char path[MAX_PATH];
   const char *m;
   char *p;
   int n;
   FILE *fp;
   LGFILE *f;

   if (!name || !*name || !mode || !*mode)
   {
      errno = EINVAL;
      return NULL;
   }
   if (LGFileIllegalName(name))
   {
      errno = ENOENT;
      return NULL;
   }
   p = fmode;
   for (m = mode, n = 0; *m && n < 32; n++)
   {
      char c = *m++;
      if (c != 'M')
      {
         *p++ = c;
         continue;
      }
      if (*name && (name[1] == ':' || *name == '\\' || *name == '/'))
      {
#ifndef SHIP
         if (g_lgFileLogLevel >= 2)
            mprintf("lg_file: lg_fopen called with mode M but filname is not relative \"%s\"\n", name);
#endif
         errno = ENOENT;
         return NULL;
      }
      name = FMPathPrepend(name, path);
   }
   *p = '\0';

   fp = fopen(name, fmode);
   if (!fp)
   {
#ifndef SHIP
      if (g_lgFileLogLevel >= 5)
         mprintf("lg_file: lg_fopen: fopen(\"%s\", \"%s\") failed (%d)\n", name, mode, errno);
#endif
      return NULL;
   }
   f = (LGFILE *)malloc(sizeof(LGFILE));
   if (f)
      LGFileInit(f);
   f->fp = fp;
   LGFileLink(f);
#ifndef SHIP
   if (g_lgFileLogLevel >= 10)
      mprintf("lg_file: lg_fopen(\"%s\", \"%s\") succeeded\n", name, mode);
#endif
   return f;
}

int lg_fclose(LGFILE *f)
{
   int result;
   if (!f)
   {
      errno = EINVAL;
      return -1;
   }
   if (f->stdslot)
   {
      g_pLGStdFiles[f->stdslot - 1] = NULL;
      LGFileUnlink(f);
      free(f);
      return 0;
   }
   result = fclose(f->fp);
   LGFileUnlink(f);
   free(f);
   return result;
}

int lg_fprintf(LGFILE *f, const char *fmt, ...)
{
   va_list args;
   if (!f)
   {
      errno = EINVAL;
      return -1;
   }
   va_start(args, fmt);
   return vfprintf(f->fp, fmt, args);
}

int lg_fscanf(LGFILE *f, const char *fmt, ...)
{
   va_list args;
   if (!f)
   {
      errno = EINVAL;
      return -1;
   }
   va_start(args, fmt);
   return LGFileVScanf(f->fp, fmt, args);
}

int lg_fputs(const char *s, LGFILE *f)
{
   if (!f)
   {
      errno = EINVAL;
      return -1;
   }
   return fputs(s, f->fp);
}

int lg_fgetc(LGFILE *f)
{
   if (!f)
   {
      errno = EINVAL;
      return -1;
   }
   return fgetc(f->fp);
}

size_t lg_fread(void *buf, size_t size, size_t count, LGFILE *f)
{
   if (!f)
   {
      errno = EINVAL;
      return 0;
   }
   return fread(buf, size, count, f->fp);
}

size_t lg_fwrite(const void *buf, size_t size, size_t count, LGFILE *f)
{
   if (!f)
   {
      errno = EINVAL;
      return 0;
   }
   return fwrite(buf, size, count, f->fp);
}

int lg_fflush(LGFILE *f)
{
   if (!f)
   {
      errno = EINVAL;
      return EOF;
   }
   return fflush(f->fp);
}

int lg_fseek(LGFILE *f, long offset, int origin)
{
   if (!f)
   {
      errno = EINVAL;
      return -1;
   }
   return fseek(f->fp, offset, origin);
}

long lg_ftell(LGFILE *f)
{
   if (!f)
   {
      errno = EINVAL;
      return -1;
   }
   return ftell(f->fp);
}

int lg_findfirst_error(const char *name)
{
#ifndef SHIP
   if (errno != ENOENT)
   {
      if (g_lgFileLogLevel >= 1)
         mprintf("lg_file: lg_findfirst error %d \"%s\"\n", errno, name ? name : "<NULL>");
   }
   else if (g_lgFileLogLevel >= 5)
      mprintf("lg_file: lg_findfirst file not found \"%s\"\n", name ? name : "<NULL>");
#endif
   return -1;
}

int lg_fileattr(const char *name)
{
   struct _finddata64i32_t data;
   intptr_t h;
   if (name && *name && LGFileIllegalName(name))
   {
      errno = ENOENT;
      return -1;
   }
   h = _findfirst64i32(name, &data);
   if (h != -1)
   {
      _findclose(h);
      return 0;
   }
#ifndef SHIP
   if (errno != ENOENT)
   {
      if (g_lgFileLogLevel >= 1)
         mprintf("lg_file: lg_fileattr error %d \"%s\"\n", errno, name ? name : "<NULL>");
   }
   else if (g_lgFileLogLevel >= 5)
      mprintf("lg_file: lg_fileattr file not found \"%s\"\n", name ? name : "<NULL>");
#endif
   return -1;
}

int lg_stat(const char *name, struct _stat64i32 *buf)
{
   if (name && *name && LGFileIllegalName(name))
   {
      errno = ENOENT;
      return -1;
   }
   if (_stat64i32(name, buf) == 0)
      return 0;
#ifndef SHIP
   if (errno != ENOENT)
   {
      if (g_lgFileLogLevel >= 1)
         mprintf("lg_file: lg_stat error %d \"%s\"\n", errno, name ? name : "<NULL>");
   }
   else if (g_lgFileLogLevel >= 5)
      mprintf("lg_file: lg_stat file not found \"%s\"\n", name ? name : "<NULL>");
#endif
   return -1;
}

int lg_fstat(int fd, struct _stat64i32 *buf)
{
   LGFILE *f = LGFileFind(fd);
   if (!f)
   {
      errno = EBADF;
      return -1;
   }
   if (_fstat64i32(_fileno(f->fp), buf) == 0)
      return 0;
#ifndef SHIP
   if (errno != ENOENT && g_lgFileLogLevel >= 1)
      mprintf("lg_file: lg_fstat error %d\n", errno);
#endif
   return -1;
}

void lg_fcloseall(void)
{
   int i;
   for (i = 0; i < LG_FILE_NUM_STD; i++)
      if (g_pLGStdFiles[i])
         lg_fclose(g_pLGStdFiles[i]);
   EnterCriticalSection(&g_LGFileLock);
   while (g_pLGFiles)
   {
      LGFILE *next = g_pLGFiles->next;
      fclose(g_pLGFiles->fp);
      free(g_pLGFiles);
      g_nLGFiles--;
      g_pLGFiles = next;
   }
   LeaveCriticalSection(&g_LGFileLock);
}
