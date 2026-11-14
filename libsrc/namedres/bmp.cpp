#include <string.h>

#include <windows.h>
#include <dbg.h>
#include <bmp.h>
#include <resapi.h>
#include <storeapi.h>
#include <grs.h>
#include <bitmap.h>

void __cdecl _DbgReportWarning(const char* pszFmt, ...);


uchar ResBmpReadHeader(IStoreStream* pStream, BmpHead* pBmpHead, BmpInfo* pBmpInfo)
{
	pStream->Read(sizeof(BmpHead), (char*)pBmpHead);
	if (pBmpHead->bfType != 0x4d42)
	{
		Warning(("BmpRead: not a Windows 3.x bitmap\n"));
		return FALSE;
	}

	pStream->Read(sizeof(BmpInfo), (char*)pBmpInfo);
	if (pBmpInfo->biSize != sizeof(BITMAPINFOHEADER) || pBmpInfo->biPlanes != 1)
	{
		Warning(("BmpRead: bad bitmap info struct\n"));
		return FALSE;
	}

	if (pBmpInfo->biBitCount != 4 && pBmpInfo->biBitCount != 8)
	{
		Warning(("BmpRead: can't read file with %d bits per pixel\n", pBmpInfo->biBitCount));
		return FALSE;
	}

	if (pBmpInfo->biCompression != 0)
	{
		Warning(("BmpRead: can't do compressed bmp\n"));
		return FALSE;
	}

	return TRUE;
}

///////////////////////////////////////
//
// ResBmpReadImage
//
grs_bitmap* ResBmpReadImage(IStoreStream* pStream, IResMemOverride* pResMem)
{
	BmpHead bmpHead;
	BmpInfo bmpInfo;
	uchar buff[1024];
	grs_bitmap* pbm;
	uchar* pImg;
	uchar* pbits;
	uchar* p;
	int widthBytes;
	int x;
	int y;

	if (!ResBmpReadHeader(pStream, &bmpHead, &bmpInfo))
		return NULL;

	pStream->SetPos(bmpHead.bmOffBits);

	pbm = (grs_bitmap*)pResMem->ResMalloc(bmpInfo.biWidth * bmpInfo.biHeight + sizeof(grs_bitmap));
	pImg = (uchar*)&pbm[1];

	if (bmpInfo.biBitCount == 4)
	{
		widthBytes = ((ushort)((bmpInfo.biWidth + 1) >> 1) + 3) & 0xfffc;
		for (y = 0, pbits = pImg; y < bmpInfo.biHeight; ++y)
		{
			pStream->Read(widthBytes, (char*)buff);
			for (x = 0, p = pbits; x < bmpInfo.biWidth; ++x)
			{
				if (x & 1)
					*p++ = buff[x >> 1] & 0xf;
				else
					*p++ = buff[x >> 1] >> 4;
			}
			pbits += bmpInfo.biWidth;
		}
	}
	else
	{
		widthBytes = ((ushort)bmpInfo.biWidth + 3) & 0xfffc;
		for (y = 0, pbits = &pImg[(bmpInfo.biHeight - 1) * bmpInfo.biWidth]; y < bmpInfo.biHeight; ++y)
		{
			pStream->Read(bmpInfo.biWidth, (char*)pbits);
			if (widthBytes - bmpInfo.biWidth)
				pStream->Read(widthBytes - bmpInfo.biWidth, (char*)buff);
			pbits -= bmpInfo.biWidth;
		}
	}

	gr_init_bitmap(pbm, pImg, BMT_FLAT8, 0, bmpInfo.biWidth, bmpInfo.biHeight);
	return pbm;
}

///////////////////////////////////////
//
// ResBmpReadPalette
//
void* ResBmpReadPalette(IStoreStream *pStream, IResMemOverride *pResMem)
{
   BmpHead fh;
   BmpInfo ih;
   uint8 *pal;
   uchar bgr[1024];
   uchar *src;
   uint8 *dst;
   int palCount;

   if (!ResBmpReadHeader(pStream, &fh, &ih)) return NULL;

   pal = (uint8 *)pResMem->ResMalloc(0x300);
   if (!pal) return NULL;
   memset(pal, 0, 0x300);

   palCount = ih.biClrUsed;
   if (!palCount) palCount = (ih.biBitCount == 4) ? 16 : 256;

   pStream->Read(palCount * 4, (char *)bgr);


   dst = pal;
   if (palCount > 0)
   {
      src = bgr;
      do
      {
         *dst++ = src[2];
         *dst++ = src[1];
         *dst++ = src[0];
         src += 4;
      } while (--palCount);
   }

   return pal;
}
