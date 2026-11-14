//	PCX.C		Reads & process .PCX files
//	Rex E. Bradford (REX)
//
//	This file implements a .PCX file reader.  The Gfile access routines
//	call the function: PcxRead() in this file.

/*
* $Header: x:/prj/tech/libsrc/gfile/RCS/pcx.c 1.9 1997/08/15 10:31:19 JAEMZ Exp $
* $Log: pcx.c $
 * Revision 1.9  1997/08/15  10:31:19  JAEMZ
 * *** empty log message ***
 * 
 * Revision 1.8  1996/05/09  12:22:34  TONY
 * Ignore manufacturer field when reading, Windows 95 Paint can
 * put unspec'ed values there.
 * 
 * Revision 1.7  1994/09/01  18:05:56  rex
 * Fixed bug related to when rowbytes is set
 * 
 * Revision 1.6  1994/08/22  17:40:02  rex
 * Added 24/bit support
 * 
 * Revision 1.5  1994/02/01  12:17:57  rex
 * Added better warnings and hack to fix a single stupid pcx file with odd width
 * 
 * Revision 1.4  1993/10/27  20:15:39  jak
 * pcx code sets pcxheader's 'palette_type' to 2, which makes pstyler think
 * the file is greyscale when it's not.  So now it sets it to 0.
 * 
 * Revision 1.3  1993/10/19  15:12:08  xemu
 * got rid of warnigns
 * 
 * Revision 1.2  1993/10/19  15:09:27  xemu
 * added writiing stuff, changed the structure names some
 * 
 * Revision 1.1  1993/09/27  16:38:52  rex
 * Initial revision
 * 
*/

#include <string.h>

#include <windows.h>
#include <pcx.h>
#include <resapi.h>
#include <storeapi.h>
#include <grs.h>
#include <bitmap.h>

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
}  PCXHEAD;

typedef PCXHEAD PcxHeader;

#define READBUFF_SIZE 4096
static uchar *readBuff;
static short readBuffIndex;

//	--------------------------------------------------------
//		INTERNAL ROUTINES
//	--------------------------------------------------------
//
//	PcxReadHeader() reads the header from a PCX file.

#define IGNORE_MANUFACTURER      // Windows 95 Paint writes an 'illegal'
                                 // value here...

bool PcxReadHeader(IStoreStream *pStream, PcxHeader *phead)
{
	pStream->SetPos(0);
	pStream->Read(sizeof(PcxHeader), (char *) phead);

#ifndef IGNORE_MANUFACTURER
	if (phead->manufacturer != 10)
		{
		Warning(("Bad PCX header: Manufacturer id\n"));
		return FALSE;
		}
#endif
	if (phead->version != 5)
		{
		Warning(("Bad PCX header: not version 3.0\n"));
		return FALSE;
		}
	if (phead->encoding != 1)
		{
		Warning(("Bad PCX header: not compressed\n"));
		return FALSE;
		}

	if ((phead->xmin != 0) || (phead->ymin != 0))
		{
		Warning(("Bad PCX header: picture not at 0,0\n"));
		return FALSE;
		}
	if (phead->bits_per_pixel != 8)
		{
		Warning(("PCX file must have 8 bits per pixel\n"));
		return FALSE;
		}

	return TRUE;
}

//	-------------------------------------------------
//
//	PcxReadRow() reads a compressed row.

bool PcxReadRow(IStoreStream *pStream, uchar *p, short npixels)
{
	uchar count,value;
	uchar *pstart;
	uchar *pend;
	
	pstart = p;
	pend = (pstart + npixels);

	while (p < pend)
		{
		if (readBuffIndex >= READBUFF_SIZE)
			{
			pStream->Read(READBUFF_SIZE, (char *) readBuff);
			readBuffIndex = 0;
			}
		value = readBuff[readBuffIndex++];
		if ((value & 0xC0) != 0xC0)
			{
			*p++ = value;
			}
		else
			{
			count = value & 0x3F;
			if (readBuffIndex >= READBUFF_SIZE)
				{
				pStream->Read(READBUFF_SIZE, (char *) readBuff);
				readBuffIndex = 0;
				}
			value = readBuff[readBuffIndex++];
#line 133
			AssertMsg1((p + count) <= pend, "PCX decompression error, xpixel: %d\n", p - pstart);
			memset(p, value, count);
			p += count;
			}
		}

	return TRUE;
}

//	-------------------------------------------------------
//
//	PcxReadBody() reads in the body.

bool PcxReadBody(IStoreStream *pStream, uchar *bits, uchar type, short w, short h, ushort row, ushort npix)
{
	uchar *p,*buff24,*pd;
	int y,i,plane;

//	Seek to start of color data

	pStream->SetPos(128);

//	If 24-bit, alloc buffer

	if (type == BMT_FLAT24)
		{
		buff24 = (uchar *) malloc(npix * 3);
		if (buff24 == NULL)
			{
			Warning(("PCX reader can't alloc memory\n"));
			return FALSE;
			}
		}

//	Iterate across rows

	p = bits;
	for (y = 0; y < h; y++)
		{

//	If 8-bit, read row directly into bitmap

		if (type == BMT_FLAT8)
			{
			if (!PcxReadRow(pStream, p, npix))
				{
				Warning(("PCX decompression error on line: %d\n", y));
				return FALSE;
				}
			}

//	If 24-bit, read 3 planes into row buffer, then interleave into bitmap

		else
			{
			for (plane = 0; plane < 3; plane++)
				{
				if (!PcxReadRow(pStream, buff24 + (plane * w), npix))
					{
					Warning(("PCX decompression error on line: %d\n", y));
					return FALSE;
					}
				}
			for (i = 0, pd = p; i < w; i++)
				{
				*pd++ = buff24[i];
				*pd++ = buff24[w + i];
				*pd++ = buff24[(w * 2) + i];
				}
			}

//	Advance bitmap ptr to next row

		p += row;
		}

//	If 24-bit, free row buffer

	if (type == BMT_FLAT24)
		free(buff24);

	return TRUE;
}

//	-------------------------------------------------------
//		READ ROUTINE
//	-------------------------------------------------------
//
//	ResPcxReadImage() reads in a PCX file.

grs_bitmap *ResPcxReadImage(IStoreStream *pStream, IResMemOverride *pResMem)
{
	PcxHeader pcxHeader;

//	Read header, if not valid PCX file return

	if (!PcxReadHeader(pStream, &pcxHeader))
		return NULL;

//	Fill in bitmap

	short w = (pcxHeader.xmax - pcxHeader.xmin) + 1;
	short h = (pcxHeader.ymax - pcxHeader.ymin) + 1;
	uchar type;
	ushort rowbytes;
	int extra;
	if (pcxHeader.colour_planes == 1)
		{
		type = BMT_FLAT8;
		rowbytes = w;
		extra = pcxHeader.bytes_per_line - rowbytes;
		}
	else if (pcxHeader.colour_planes == 3)
		{
		type = BMT_FLAT24;
		rowbytes = w * 3;
		extra = pcxHeader.bytes_per_line * 3 - rowbytes;
		}
	else
		{
		Warning(("PCX file has %d planes!\n", pcxHeader.colour_planes));
		return NULL;
		}

//	Allocate bitmap bits

	grs_bitmap *bm = (grs_bitmap *) pResMem->ResMalloc(h * rowbytes + sizeof(grs_bitmap) + extra);
	if (!bm)
		{
		Warning(("Can't allocate bitmap bits in pcx reader\n"));
		return NULL;
		}
	uchar *bits = (uchar *) (bm + 1);

//	Allocate read buffer

	readBuff = (uchar *) malloc(READBUFF_SIZE);
	if (readBuff == NULL)
		{
		Warning(("Can't allocate buffer in pcx reader\n"));
		goto BADRET;
		}
	readBuffIndex = READBUFF_SIZE;

//	Read body and return

	if (!PcxReadBody(pStream, bits, type, w, h, rowbytes, pcxHeader.bytes_per_line))
		{
		Warning(("Unable to read PCX file!\n"));
		free(readBuff);
		pResMem->ResFree(bm);
		return NULL;
		}
	else
		{
		free(readBuff);
		gr_init_bitmap(bm, bits, type, 0, w, h);
		return bm;
		}

//	If error, free up properly

BADRET:

	pResMem->ResFree(bm);
	return NULL;
}

//	-------------------------------------------------------
//
//	ResPcxReadPalette() reads in the pallette.

void *ResPcxReadPalette(IStoreStream *pStream, IResMemOverride *pResMem)
{
	PcxHeader pcxHeader;

	if (PcxReadHeader(pStream, &pcxHeader) && (pcxHeader.colour_planes == 1))
		{
		uchar *pall = (uchar *) pResMem->ResMalloc(256 * 3);
		if (pall)
			{
			uchar pallPresent;

			pStream->SetPos(pStream->GetSize() - 769);
			pStream->Read(1, (char *) &pallPresent);
			if (pallPresent == 12)
				{
				pStream->Read(256 * 3, (char *) pall);
				return pall;
				}
			Warning(("Bad PCX trailer: pallette not found\n"));
			}
		}
	return NULL;
}
