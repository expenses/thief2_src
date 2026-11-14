#include <string.h>

#include <windows.h>
#include <tga.h>
#include <bitmap.h>

extern int gResImageConvert8888to4444;

#define TGA_TYPE_COLORMAPPED    1
#define TGA_TYPE_TRUECOLOR      2
#define TGA_DESC_RIGHT_TO_LEFT  0x10
#define TGA_DESC_TOP_TO_BOTTOM  0x20

struct sTgaHeader
{
	uchar ID_field_length;
	uchar color_map_type;
	uchar image_type;
	ushort first_color_map;
	ushort color_map_length;
	uchar color_map_size;
	ushort x_origin;
	ushort y_origin;
	ushort width;
	ushort height;
	uchar bits_per_pixel;
	uchar image_descriptor_bits;
};

grs_bitmap* ResTgaReadImage(IStoreStream* pStream, IResMemOverride* pResMem)
{
	sTgaHeader tga;
	grs_bitmap* pbm;
	uchar* pBits;
	int idel, jstart, jend, jdel, j;

	pStream->Read(sizeof(tga), (char*)&tga);

	if (tga.image_descriptor_bits & TGA_DESC_RIGHT_TO_LEFT)
		idel = -1;
	else
		idel = 1;

	if (tga.image_descriptor_bits & TGA_DESC_TOP_TO_BOTTOM)
	{
		jstart = 0;
		jend = tga.height;
		jdel = 1;
	}
	else
	{
		jstart = tga.height - 1;
		jend = -1;
		jdel = -1;
	}

	switch (tga.image_type)
	{
	case TGA_TYPE_COLORMAPPED:
	{
			if (tga.bits_per_pixel != 8)
				return NULL;

			pStream->SetPos(pStream->GetPos() + tga.color_map_length * 3);
			pbm = (grs_bitmap*)pResMem->ResMalloc(tga.width * tga.height + sizeof(grs_bitmap));
			pBits = (uchar*)&pbm[1];
			for (j = jstart; j != jend; j += jdel)
			{
				char* buff = (char*)&pBits[tga.width * j];
				pStream->Read(tga.width, buff);
				if (idel == -1)
				{
					char* other = &buff[tga.width - 1];
					while (other > buff)
					{
						char tmp = *buff;
						*buff = *other;
						*other-- = tmp;
						++buff;
					}
				}
			}
			gr_init_bitmap(pbm, pBits, BMT_FLAT8, 0, tga.width, tga.height);
			return pbm;
	}
	case TGA_TYPE_TRUECOLOR:
		if (tga.bits_per_pixel == 32)
		{
			int Bpp = tga.bits_per_pixel / 8;
			int readrow = Bpp * tga.width;
			int bmrow = (gResImageConvert8888to4444 ? 2 : 4) * tga.width;
			pbm = (grs_bitmap*)pResMem->ResMalloc(tga.height * bmrow + sizeof(grs_bitmap));
			uchar* p = (uchar*)&pbm[1];
			uint* read_buff = new uint[tga.width];
			for (j = jstart; j != jend; j += jdel)
			{
				if (gResImageConvert8888to4444)
				{
					uint* start_buff = read_buff;
					start_buff += (idel == -1) ? tga.width - 1 : 0;
					pStream->Read(readrow, (char*)read_buff);
					uint* pSrc = start_buff;
					ushort* pDst = (ushort*)&p[2 * tga.width * j];
					for (int i = 0; i < tga.width; ++i)
					{
						*pDst++ = (ushort)(((*pSrc >> 16) & 0xf000) | ((*pSrc >> 12) & 0xf00) | ((*pSrc >> 8) & 0xf0) | ((*pSrc >> 4) & 0xf));
						pSrc += idel;
					}
				}
				else
					pStream->Read(bmrow, (char*)&p[4 * tga.width * j]);
			}
			delete read_buff;
			if (gResImageConvert8888to4444)
				gr_init_bitmap(pbm, p, BMT_FLAT16, BMF_RGB_4444, tga.width, tga.height);
			else
				gr_init_bitmap(pbm, p, BMT_FLAT32, BMF_RGB_8888, tga.width, tga.height);
		}
		else
		{
			if (tga.bits_per_pixel != 24 && tga.bits_per_pixel != 16)
				return NULL;

			int nBytes = tga.bits_per_pixel / 8;
			int row = nBytes * tga.width;
			pbm = (grs_bitmap*)pResMem->ResMalloc(tga.height * row + sizeof(grs_bitmap));
			pBits = (uchar*)&pbm[1];
			for (j = jstart; j != jend; j += jdel)
			{
				uchar* pLo = &pBits[j * row];
				pStream->Read(row, (char*)pLo);
				if (idel == -1)
				{
					uchar* pHi = pLo + (tga.width - 1) * nBytes;
					while (pHi > pLo)
					{
						uchar tmp[4];
						memcpy(tmp, pLo, nBytes);
						memcpy(pLo, pHi, nBytes);
						memcpy(pHi, tmp, nBytes);
						pHi -= nBytes;
						pLo += nBytes;
					}
				}
			}
			gr_init_bitmap(pbm, pBits, (nBytes != 2) ? BMT_FLAT24 : BMT_FLAT16, (nBytes != 2) ? BMF_RGB_888 : BMF_RGB_555, tga.width, tga.height);
		}
		return pbm;
	}
	return NULL;
}

void* ResTgaReadPalette(IStoreStream* pStream, IResMemOverride* pResMem)
{
	sTgaHeader tga;
	pStream->Read(sizeof(tga), (char*)&tga);
	if (tga.image_type != TGA_TYPE_COLORMAPPED)
		return NULL;
	if (tga.bits_per_pixel != 8)
		return NULL;

	uint8* pPall = (uint8*)pResMem->ResMalloc(256 * 3);
	memset(pPall, 0, 256 * 3);
	pStream->Read(tga.color_map_length * 3, (char*)(pPall + tga.first_color_map * 3));
	return pPall;
}
