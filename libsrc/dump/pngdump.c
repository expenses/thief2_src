#include <stdlib.h>
#include <png.h>
#include <lg.h>
#include <dev2d.h>
#include <lg_file.h>
#include <dump.h>
#include <memall.h>

#define PNG_DUMP_MAX_H 8192

static void PngErrorFn(png_structp png_ptr, png_const_charp msg)
{
}

static void PngWarningFn(png_structp png_ptr, png_const_charp msg)
{
}

static png_voidp PngMallocFn(png_structp png_ptr, png_alloc_size_t size)
{
   return Malloc(size);
}

static void PngFreeFn(png_structp png_ptr, png_voidp ptr)
{
   Free(ptr);
}

static void PngFileWrite(png_structp png_ptr, png_bytep data, png_size_t length)
{
   if (lg_fwrite(data, 1, length, (LGFILE *)png_get_io_ptr(png_ptr)) != length)
      png_error(png_ptr, "PngFileWrite failed");
}

static void PngFileFlush(png_structp png_ptr)
{
   lg_fflush((LGFILE *)png_get_io_ptr(png_ptr));
}

BOOL dmp_png_dump_screen(char *name)
{
   png_structp png_ptr;
   png_infop info_ptr;
   png_bytep rows[PNG_DUMP_MAX_H];
   png_color pal[256];
   grs_bitmap *bm = NULL;
   uchar *buf = NULL;
   LGFILE *fp;
   int w = grd_bm.w;
   int h = grd_bm.h;
   int x, y;

   if (grd_bm.type != BMT_FLAT8 && grd_bm.type != BMT_FLAT16 && grd_bm.type != BMT_FLAT24 && grd_bm.type != BMT_FLAT32)
      return FALSE;
   if (h > PNG_DUMP_MAX_H)
      return FALSE;

   fp = lg_fopen(name, "wb");
   if (!fp)
      return FALSE;

   png_ptr = png_create_write_struct_2(PNG_LIBPNG_VER_STRING, NULL, PngErrorFn, PngWarningFn,
                                       NULL, PngMallocFn, PngFreeFn);
   if (!png_ptr)
   {
      lg_fclose(fp);
      return FALSE;
   }
   info_ptr = png_create_info_struct(png_ptr);
   if (!info_ptr)
   {
      png_destroy_write_struct(&png_ptr, NULL);
      lg_fclose(fp);
      return FALSE;
   }
   if (setjmp(png_jmpbuf(png_ptr)))
   {
      png_destroy_write_struct(&png_ptr, &info_ptr);
      lg_fclose(fp);
      return FALSE;
   }

   png_set_write_fn(png_ptr, fp, PngFileWrite, PngFileFlush);
   png_set_IHDR(png_ptr, info_ptr, w, h, 8,
                grd_bm.type == BMT_FLAT8 ? PNG_COLOR_TYPE_PALETTE : PNG_COLOR_TYPE_RGB,
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

   if (grd_bm.type == BMT_FLAT8)
   {
      uchar *p = grd_pal;
      for (x = 0; x < 256; x++)
      {
         pal[x].red = *p++;
         pal[x].green = *p++;
         pal[x].blue = *p++;
      }
      png_set_PLTE(png_ptr, info_ptr, pal, 256);
   }

   if (grd_bm.type == BMT_FLAT32)
   {
      uchar *dst = buf = (uchar *)operator new(w * h * 3);
      bm = gr_alloc_bitmap(BMT_FLAT32, 0, w, 1);
      for (y = 0; y < h; y++)
      {
         uchar *src;
         rows[y] = dst;
         gd_get_bitmap(bm, 0, y);
         src = bm->bits;
         for (x = 0; x < w; x++, src += 4)
         {
            *dst++ = src[2];
            *dst++ = src[1];
            *dst++ = src[0];
         }
      }
   }
   else if (grd_bm.type == BMT_FLAT24)
   {
      uchar *src;
      png_set_bgr(png_ptr);
      bm = gr_alloc_bitmap(BMT_FLAT24, 0, w, h);
      gd_get_bitmap(bm, 0, 0);
      src = bm->bits;
      for (y = 0; y < h; y++, src += w * 3)
         rows[y] = src;
   }
   else if (grd_bm.type == BMT_FLAT16)
   {
      uchar *dst = buf = (uchar *)operator new(w * h * 3);
      bm = gr_alloc_bitmap(BMT_FLAT16, grd_bm.flags & BMF_RGB_MASK, w, 1);
      for (y = 0; y < h; y++)
      {
         ushort *src;
         rows[y] = dst;
         gd_get_bitmap(bm, 0, y);
         src = (ushort *)bm->bits;
         for (x = 0; x < w; x++, src++)
         {
            ushort c = *src;
            *dst++ = ((c >> 8) & ~7) | ((c >> 13) & 7);
            *dst++ = ((c >> 3) & ~3) | ((c >> 9) & 3);
            *dst++ = (c << 3) | ((c >> 2) & 7);
         }
      }
   }
   else if (grd_bm.type == BMT_FLAT8)
   {
      uchar *src;
      bm = gr_alloc_bitmap(BMT_FLAT8, 0, w, h);
      gd_get_bitmap(bm, 0, 0);
      src = bm->bits;
      for (y = 0; y < h; y++, src += w)
         rows[y] = src;
   }

   png_write_info(png_ptr, info_ptr);
   png_write_image(png_ptr, rows);
   png_write_end(png_ptr, info_ptr);
   png_destroy_write_struct(&png_ptr, NULL);
   lg_fclose(fp);

   if (bm)
   {
      gr_close_bitmap(bm);
      gr_free(bm);
   }
   if (buf)
      operator delete(buf);
   return TRUE;
}
