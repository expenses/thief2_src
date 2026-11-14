#include <windows.h>
#include <lg.h>
#include <mode.h>
#include <grspoint.h>
#include <bitmap.h>
struct grs_bitmap;
extern "C" int md_sizeof_mipmap_bits(grs_bitmap* mmap);
#include <mdutil.h>

int gResImageConvert8888to4444 = 1;
ushort gResImageChromaKey = 0;
ulong gResImageFlat16Format = 0;

static void FlagsXorMip(grs_bitmap* pBm, int flag);
static void FlagsOrMip(grs_bitmap* pBm, int16 flag);

void ImageHackAutoSetTransparency(grs_bitmap* pBm, uint8 mip)
{
	int16 flag = 0;

	switch (pBm->type)
	{
	case BMT_FLAT16:
	{
		uint16* pBits = (uint16*)&pBm->bits[2 * pBm->h * pBm->w];
		while (--pBits >= (uint16*)pBm->bits)
		{
			if (*pBits == gResImageChromaKey)
			{
				flag = 1;
				break;
			}
		}
		break;
	}
	case BMT_FLAT8:
	{
		uchar* pBits = &pBm->bits[pBm->h * pBm->w];
		while (--pBits >= pBm->bits)
		{
			if (!*pBits)
			{
				flag = 1;
				break;
			}
		}
		break;
	}
	}

	if (flag)
	{
		if (mip)
			FlagsOrMip(pBm, flag);
		else
			pBm->flags |= flag;
	}
}

static void FlagsOrMip(grs_bitmap* pBm, int16 flag)
{
	while (true)
	{
		pBm->flags |= flag;
		if (pBm->w == 1 && pBm->h == 1)
			break;
		++pBm;
	}
}

void ImageHackRemap16Bit(grs_bitmap* pBm, uint8 mip)
{
	if (pBm->type != BMT_FLAT16)
		return;

	ulong format = gResImageFlat16Format;
	if (!format)
	{
		uchar depth = grd_mode_info[grd_mode].bitDepth;
		if (depth <= 8)
			return;
		format = (depth != 15) ? BMF_RGB_565 : BMF_RGB_555;
		if (!format)
			return;
	}
	if (pBm->flags & format & 0x700)
		return;

	uint16* pStart = (uint16*)pBm->bits;
	uint16* pBits = pStart + pBm->h * pBm->w;
	if (mip)
		pBits = (uint16*)((uchar*)pStart + md_sizeof_mipmap_bits(pBm));

	if (pBm->flags & BMF_RGB_565)
	{
		while (--pBits >= pStart)
		{
			uint16 c = *pBits;
			c = ((c >> 1) & 0x7C00) | (c & 0x83FF);
			c = ((c >> 1) & 0x03E0) | (c & 0xFC1F);
			*pBits = c;
		}
	}
	else
	{
		while (--pBits >= pStart)
		{
			uint16 c = *pBits;
			c = ((c & 0xFC00) << 1) | (c & 0x07FF);
			int g = (c >> 5) & 0x1F;
			c = ((((g * 63) / 31) & 0x3F) << 5) | (c & 0xF81F);
			*pBits = c;
		}
	}

	if (mip)
		FlagsXorMip(pBm, 0x700);
	else
		pBm->flags ^= 0x700;
}

static void FlagsXorMip(grs_bitmap* pBm, int flag)
{
	while (true)
	{
		pBm->flags ^= flag;
		if (pBm->w == 1 && pBm->h == 1)
			break;
		++pBm;
	}
}
