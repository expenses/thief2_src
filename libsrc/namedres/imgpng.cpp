#include <windows.h>
#include <png.h>
#include <imgpng.h>
#include <bitmap.h>
#include <mprintf.h>

#define PNG_MAX_DIM 4096

static void PngErrorFn(png_structp png_ptr, png_const_charp msg)
{
}

static void PngWarningFn(png_structp png_ptr, png_const_charp msg)
{
}

static png_voidp PngMallocFn(png_structp png_ptr, png_alloc_size_t size)
{
	return malloc(size);
}

static void PngFreeFn(png_structp png_ptr, png_voidp ptr)
{
	free(ptr);
}

static void PngStreamRead(png_structp png_ptr, png_bytep data, png_size_t length)
{
	IStoreStream* pStream = (IStoreStream*)png_get_io_ptr(png_ptr);
	if (pStream->Read(length, (char*)data) != (long)length)
		png_error(png_ptr, "PngStreamRead failed");
}

grs_bitmap* ResPngReadImage(IStoreStream* pStream, IResMemOverride* pResMem)
{
	png_structp png_ptr;
	png_infop info_ptr;
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;
	png_bytep rows[PNG_MAX_DIM];
	png_byte sig[8];
	grs_bitmap* pbm = NULL;

	pStream->SetPos(0);
	pStream->Read(sizeof(sig), (char*)sig);
	if (png_sig_cmp(sig, 0, sizeof(sig)))
		return NULL;

	png_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, NULL, PngErrorFn, PngWarningFn,
		NULL, PngMallocFn, PngFreeFn);
	if (!png_ptr)
		return NULL;

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return NULL;
	}

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		if (pbm)
			pResMem->ResFree(pbm);
		return NULL;
	}

	png_set_option(png_ptr, PNG_SKIP_sRGB_CHECK_PROFILE, PNG_OPTION_ON);
	png_set_read_fn(png_ptr, pStream, PngStreamRead);
	png_set_sig_bytes(png_ptr, sizeof(sig));
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);

	if (height > PNG_UINT_32_MAX / sizeof(png_bytep))
		png_error(png_ptr, "image is too tall");

	if (bit_depth < 8 || color_type == PNG_COLOR_TYPE_PALETTE || png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_expand(png_ptr);
	png_set_bgr(png_ptr);
	png_set_gray_to_rgb(png_ptr);
	if (interlace_type != PNG_INTERLACE_NONE)
		png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);

	if ((color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGB_ALPHA) &&
	    bit_depth == 8 && width <= PNG_MAX_DIM && height <= PNG_MAX_DIM)
	{
		bit_depth = ((color_type == PNG_COLOR_TYPE_RGB) ? 3 : 4) * 8;
		uint flags = (bit_depth == 32) ? (BMF_RGB_8888 | BMF_TRANS) : BMF_RGB_888;
		int npix = width * height;
		pbm = (grs_bitmap*)pResMem->ResMalloc(npix * 4 + sizeof(grs_bitmap));
		uchar* pBits = (uchar*)(pbm + 1);
		uchar* p = pBits;
		png_bytep* pRow = rows;
		for (png_uint_32 y = height; y; y--)
		{
			*pRow++ = p;
			p += (bit_depth / 8) * width;
		}

		png_read_image(png_ptr, rows);
		png_read_end(png_ptr, info_ptr);

		if (bit_depth == 24)
		{
			uchar* src = pBits + npix * 3 - 1;
			uchar* dst = pBits + npix * 4 - 1;
			for (; npix; npix--)
			{
				*dst-- = 0xff;
				*dst-- = *src--;
				*dst-- = *src--;
				*dst-- = *src--;
			}
		}

		gr_init_bitmap(pbm, pBits, BMT_FLAT32, flags, width, height);
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return pbm;
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	return NULL;
}
