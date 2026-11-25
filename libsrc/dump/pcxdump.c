/*
 * $Source: x:/prj/tech/libsrc/dump/RCS/pcxdump.c $
 * $Revision: 1.5 $
 * $Author: JAEMZ $
 * $Date: 1996/12/19 12:15:43 $
 *
 * PCX dumper
 * 
 */

#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <lg.h>
#include <dev2d.h>
#include <dump.h>
#include <lg_file.h>

#define  RGB_RED     0
#define  RGB_GREEN   1
#define  RGB_BLUE 2
#define  RGB_SIZE 3

int dmp_pcx_file(char *buff,char *prefix)
{
   int fd = dmp_find_free_file(buff,prefix,"pcx");
   if (fd == 0) {
       return 0;
   }

   return dmp_pcx_dump_screen(buff);
}

typedef struct {
   char manufacturer;
   char version;
   char encoding;              
   char bits_per_pixel;       // better be 8 for us
   short int xmin,ymin;       // usually zero unless zany
   short int xmax,ymax;       // xmax-xmin+1 = w, ymax-ymin+1 = h
   short int hres;            // unreliable      
   short int vres;            // unreliable
   char palette[48];
   char reserved;
   char colour_planes;        // 3 for rgb, 1 for 8 bit
   short int bytes_per_line;  // use this for width when unpacking
   short int palette_type;    // everyone punts this, ignore, should mean monochrome or color
   char filler[58];
}  dps_head;

static int writepcxline(uchar *p,int fp,int bytes);

// Dumps an 8 bit image
int  dmp_pcx_dump_screen(char *name)
{
   dps_head pcx;
   int i,j,bytes;
   char c;
   uchar *p;
   grs_bitmap *bmbuff;
   int fd;

   fd = lg_open(name,O_CREAT|O_BINARY|O_WRONLY,S_IREAD|S_IWRITE);
   if (fd == -1) {
       return 0;
   }

   pcx.manufacturer = 10;
   pcx.version = 5;
   pcx.encoding = 1;
   pcx.bits_per_pixel = 8;
   pcx.xmin = 0;
   pcx.ymin = 0;
   pcx.xmax = grd_bm.w -1;
   pcx.ymax = grd_bm.h -1;
   pcx.hres = grd_bm.w;
   pcx.vres = grd_bm.h;
   pcx.colour_planes = (grd_mode_info[grd_mode].bitDepth==8)?1:3;
   pcx.bytes_per_line = grd_bm.w;
   pcx.palette_type = 0;

   lg_write(fd,&pcx,sizeof(dps_head));

   bytes=pcx.bytes_per_line;

   // 24 bit or 8 bit
   if (pcx.colour_planes==3) {
      ushort col;
      ulong gmask,bshift;
      ushort *src;
      uchar *dst;

      switch(grd_mode_info[grd_mode].bitDepth)
      {
         case 15:
         {
            gmask = 0x1F;
            bshift = 10;
            break;
         }
         case 16:
         {
            gmask = 0x3F;
            bshift = 11;
            break;
         }
         default:
         {
            return 0;
         }
      }

      // allocate a line buffer for pcx insanity
      if ((p=Malloc(RGB_SIZE*bytes)) == NULL) return 0;

      // allocate a bitmap for non-flat8 weirdness
      bmbuff = gr_alloc_bitmap(BMT_FLAT16,0,bytes,1);
   
      for(i=0;i<grd_bm.h;++i) {
         // split rgb into three planes
         gr_get_bitmap(bmbuff,0,i);
         src = (ushort *)(bmbuff->bits);
         for(j=0;j<grd_bm.w;++j) {
            col = *src;
            dst = p+j;
            // could table-ize these for speed increase
            *dst = (((col>>bshift)&0x1F) * 0xFF)/0x1F;
            dst += bytes;
            *dst = (((col>>5)&gmask) * 0xFF)/gmask;
            dst += bytes;
            *dst = ((col&0x1F) * 0xFF)/0x1F;
            src++;
         }
   
         // We've split a line, spew to disk
         for(j=0;j<RGB_SIZE;++j) {
            if(writepcxline(p+(j*bytes),fd,bytes) != bytes) {
               free(p);
               return 0;
            }
         }
      }
      gr_free(bmbuff);
      Free(p);

   } else {

      // allocate a bitmap for non-flat8 weirdness
      bmbuff = gr_alloc_bitmap(BMT_FLAT8,0,bytes,1);

      for(i=0;i<grd_bm.h;++i) {
         gr_get_bitmap(bmbuff,0,i);
         if(writepcxline(bmbuff->bits,fd,bytes) != bytes) {
            return 0;
         }
      }

      gr_free(bmbuff);

      // flag for people to know the palette is next
      c = 12;
      lg_write(fd,&c,sizeof(char));

      lg_write(fd,grd_pal,768);
   }

   lg_close(fd);

   return 1;
}

// write and encode into a PCX line
static int writepcxline(uchar *p,int fp,int bytes)
{
   int n=0,i,j=0;
   uchar c;
   uchar buf[10000];

   do {
      c = p[n++];
      i = 1;
      while ( (c == p[n]) && (i<0x3f) && (n<bytes) ) {
         ++i;
         ++n;
      }
      if ( (i>1) || (c>0x3f) )
         buf[j++] = (0xc0 + i);
      buf[j++] = c;
   } while(n < bytes);
   lg_write(fp,buf,j);
   return n;
}
