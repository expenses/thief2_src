#ifndef __LG_FILE_H
#define __LG_FILE_H

#include <stdio.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LGFILE
{
   int id;
   FILE *fp;
   __int64 size;
   int open;
   int stdslot;
   struct LGFILE *prev;
   struct LGFILE *next;
} LGFILE;

LGFILE *lg_fopen(const char *name, const char *mode);
int lg_fclose(LGFILE *f);
int lg_fprintf(LGFILE *f, const char *fmt, ...);
int lg_fscanf(LGFILE *f, const char *fmt, ...);
int lg_fputs(const char *s, LGFILE *f);
int lg_fgetc(LGFILE *f);
size_t lg_fread(void *buf, size_t size, size_t count, LGFILE *f);
size_t lg_fwrite(const void *buf, size_t size, size_t count, LGFILE *f);
int lg_fflush(LGFILE *f);
int lg_fseek(LGFILE *f, long offset, int origin);
long lg_ftell(LGFILE *f);
void lg_fcloseall(void);

int lg_open(const char *name, int oflag, ...);
int lg_close(int fd);
int lg_read(int fd, void *buf, unsigned int count);
int lg_write(int fd, const void *buf, unsigned int count);
long lg_filelength(int fd);
__int64 lg_filelengthi64(int fd);
int lg_lseek(int fd, long offset, int origin);
int lg_eof(int fd);
int lg_fstat(int fd, struct _stat64i32 *buf);

int lg_findfirst_error(const char *name);
int lg_fileattr(const char *name);
int lg_stat(const char *name, struct _stat64i32 *buf);

#ifdef __cplusplus
}
#endif

#endif
