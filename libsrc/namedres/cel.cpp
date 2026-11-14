//		CEL.C		Reads 1st frame from .CEL or .FLC or .FLI file
//		Rex E. Bradford (REX)
//
//	The header files: "pjstypes.h" and "pjfli.h" come from the
//	Autodesk developer's kit, and the .fli/.flc format is more fully
//	described in an accompanying document entitled:
//		"Animation Support Libraries: File Formats Reference"
/*
* $Header: r:/prj/lib/src/gfile/rcs/cel.c 1.3 1994/05/26 18:00:36 rex Exp $
* $Log: cel.c $
 * Revision 1.3  1994/05/26  18:00:36  rex
 * Allocated bitmap buffer larger than need be to avoid crash, also took
 * out warning of condition that should be met but isn't always
 * 
 * Revision 1.2  1993/10/26  13:21:50  rex
 * Fixed bug in CelExpandWordDelta()
 * 
 * Revision 1.1  1993/09/27  19:21:41  rex
 * Initial revision
 * 
*/

#include <string.h>

#include <windows.h>
#include <lg.h>
#include <cel.h>
#include <pjfli.h>
#include <bitmap.h>

//	----------------------------------------------------------
//
//	CelExpandBrun() expands a FLI_BRUN chunk.

void CelExpandBrun(uchar *ps, uchar *bits, short w, short h)
{
	uchar *pd;
	short y;
	short pixels;
	signed char npix;
	short i;

//	Init destination

	pd = bits;

//	Byte-Run-length is always full-screen

	for (y = 0; y < h; y++)
		{
		ps++;									// skip packet count;
		pixels = 0;							// init pixel count

//	Get pixels till hit end of line

		while (pixels < w)
			{
			npix = *(signed char *)ps;	// get signed count
			ps++;
			if (npix < 0)					// if negative count, copy
				{
				npix = -npix;
				for (i = 0; i < npix; i++)
					*pd++ = *ps++;
				}
			else								// if positive count, run
				{
				for (i = 0; i < npix; i++)
					*pd++ = *ps;
				ps++;
				}
			pixels += npix;				// bump pixel count
			}

//	At end of line, check for errors

		if (pixels > w)
			{
			Warning(("CelExpandBrun: decompress error\n"));
			return;
			}
		}
}

//	----------------------------------------------------------
//
//	CelExpandByteDelta() expands a FLI_LC chunk.

void CelExpandByteDelta(uchar *ps, uchar *bits, short w)
{
	uchar *pd;
	short nskip;
	short nlines;
	short y;
	short xpos;
	short i;
	short numPackets;
	short ipacket;
	signed char npix;

//	Init dest

	pd = bits;

//	First word is # lines to skip

	nskip = *(short *)ps;
	ps += 2;

	for (y = 0; y < nskip; y++)
		{
		for (i = 0; i < w; i++)
			*pd++ = 0;
		}

//	Next word is # lines in the chunk

	nlines = *(short *)ps;
	ps += 2;

//	Iterate thru lines

	for (y = 0; y < nlines; y++)
		{

//	First byte is packet count

		numPackets = *ps++;
		xpos = 0;

//	Loop thru packets

		for (ipacket = 0; ipacket < numPackets; ipacket++)
			{

//	First byte is # pixels to skip

			nskip = *ps++;
			for (i = 0; i < nskip; i++)
				*pd++ = 0;
			xpos += nskip;

//	Next byte is count

			npix = *(signed char *)ps;
			ps++;

//	If count negative, pixel run

			if (npix < 0)
				{
				npix = -npix;
				for (i = 0; i < npix; i++)
					*pd++ = *ps;
				ps++;
				}

//	Else if count positive, pixel copy

			else
				{
				for (i = 0; i < npix; i++)
					*pd++ = *ps++;
				}

//	Update pixel pos

			xpos += npix;
			}

//	At end of packets, check for errors & fill in rest of line with skip

		if (xpos > w)
			{
			Warning(("CelExpandByteDelta: decompress error\n"));
			return;
			}
		else if (xpos < w)
			{
			for (i = xpos; i < w; i++)
				*pd++ = 0;
			}
		}
}

//	----------------------------------------------------------
//
//	CelExpandWordDelta() expands a FLI_SS2 chunk.

void CelExpandWordDelta(uchar *ps, uchar *bits, short w)
{
	uchar *pd;
	short nlines;
	short nskip;
	short y;
	short word;
	short xpos;
	short i,j;
	short numPackets;
	short ipacket;
	signed char nwords;
	short lastByte;

//	Init dest

	pd = bits;

//	First word is # lines in chunk

	nlines = *(short *)ps;
	ps += 2;

//	Iterate thru lines

	for (y = 0; y < nlines; y++)
		{

//	Check for special options

		while (TRUE)
			{
			word = *(short *)ps;
			ps += 2;
			if ((word & 0xC000) == 0xC000)	// 11xxxxxx: skip lines
				{
				for (i = 0; i < -word; i++)
					for (j = 0; j < w; j++)
						*pd++ = 0;
				}
			else if ((word & 0xC000) == 0x8000)	// 10xxxxxx: single byte patch
				{
				lastByte = word & 0xFF;
				numPackets = *(short *)ps;
				ps += 2;
				break;
				}
			else if ((word & 0xC000) == 0x4000)
				{
				Error(1, "CelExpandWordDelta: 0x4000 case illegal\n");
				}
			else										// 00xxxxxx: num packets (following)
				{
				numPackets = word;
				break;
				}
			}

//	Get packets

		xpos = 0;
		for (ipacket = 0; ipacket < numPackets; ipacket++)
			{

//	First byte is pixels to skip

			nskip = *ps++;
			for (i = 0; i < nskip; i++)
				*pd++ = 0;
			xpos += nskip;

//	Next byte is count of words

			nwords = *(signed char *)ps;
			ps++;

//	If count negative, word pixel run

			if (nwords < 0)
				{
				nwords = -nwords;
				for (i = 0; i < nwords; i++)
					{
					*pd++ = *ps;
					*pd++ = *(ps + 1);
					}
				ps += 2;
				}

//	Else if count positive, word pixel copy

			else
				{
				for (i = 0; i < nwords; i++)
					{
					*pd++ = *ps++;
					*pd++ = *ps++;
					}
				}

//	Update pixel pos

			xpos += (nwords << 1);
			}

//	At end of packets, check for errors & fill in rest of line with skip

		if (xpos > w)
			{
			Warning(("CelExpandWordDelta: decompress error\n"));
			return;
			}
		else if (xpos < w)
			{
			for (i = xpos; i < w; i++)
				*pd++ = 0;
			}
		if (lastByte >= 0)
			*(pd - 1) = lastByte;
		}
}


//	----------------------------------------------------------
//		INTERNAL ROUTINES
//	----------------------------------------------------------
//
//	CelGetPall() gets pallette

void CelGetPall(uchar *pall, uchar *buff, int shifter)
{
	uchar *pd;
	short index;
	short numPackets;
	short ipacket;
	short count;
	short i;

	pd = pall;
	index = 0;

//	First word is # color packets

	numPackets = *(short *)buff;
	buff += 2;

	for (ipacket = 0; ipacket < numPackets; ipacket++)
		{
//	Each packet is index,count,rgb[]

		index += *buff++;
		count = *buff++;
		if (count == 0)
			count = 256;

		if ((index != 0) || (count != 256))
			{
			Warning(("CelGetPall: can't handle partial pallettes\n"));
			}

		for (i = 0; i < (count * 3); i++)
			*pd++ = *buff++ << shifter;
		}
}

//	----------------------------------------------------------
//
//	CelReadFrame() reads in a frame from the .cel file.

bool CelReadFrame(IStoreStream *pStream, uchar *bits, short w, short h, bool palOnly)
{
	Fli_frame frameInfo;
	Chunk_id chunk;
	int ichunk;
	long nextOffset;
	bool frameDone;
	long fullLen;
	uchar *buff;

//	Buffer not allocated yet

	buff = NULL;

READFRAME:

//	Read frame info header, check for errors, report if verbose

	pStream->Read(sizeof(frameInfo), (char *) &frameInfo);
	if (frameInfo.type != FCID_FRAME)
		{
		Warning(("CelReadFrame: missing frame chunk\n"));
		if (buff)
			{
			Free(buff);
			return FALSE;
			}
		}

//	Allocate buffer

	if (buff == NULL)
		{
		fullLen = (long) w * (long) h;
		if (fullLen < frameInfo.size)
			buff = (uchar *) Malloc(fullLen);
		else
			buff = (uchar *) Malloc(frameInfo.size);
		if (buff == NULL)
			return FALSE;
		}

//	If no frame chunks, add time to previous frame & retry

	if (frameInfo.chunks == 0)
		{
		pStream->SetPos(pStream->GetPos() - sizeof(frameInfo) + frameInfo.size);
		goto READFRAME;
		}

//	Else process all chunks in frame

	else
		{
		frameDone = FALSE;

		for (ichunk = 0; ichunk < frameInfo.chunks; ichunk++)
			{
			pStream->Read(sizeof(chunk), (char *) &chunk);
			nextOffset = pStream->GetPos() + chunk.size - sizeof(chunk);
			switch (chunk.type)
				{
				case FLI_COLOR256:
					pStream->Read(chunk.size - sizeof(chunk), (char *) buff);
					if (palOnly)
						{
						CelGetPall(bits, buff, 0);
						return TRUE;
						}
					break;

				case FLI_SS2:
					pStream->Read(chunk.size - sizeof(chunk), (char *) buff);
					CelExpandWordDelta(buff, bits, w);
					frameDone = TRUE;
					break;

				case FLI_COLOR:
					pStream->Read(chunk.size - sizeof(chunk), (char *) buff);
					if (palOnly)
						{
						CelGetPall(bits, buff, 2);
						return TRUE;
						}
					break;

				case FLI_LC:
					pStream->Read(chunk.size - sizeof(chunk), (char *) buff);
					CelExpandByteDelta(buff, bits, w);
					frameDone = TRUE;
					break;

				case FLI_COLOR_0:
					memset(bits, 0, fullLen);
					frameDone = TRUE;
					break;

				case FLI_BRUN:
					pStream->Read(chunk.size - sizeof(chunk), (char *) buff);
					CelExpandBrun(buff, bits, w, h);
					frameDone = TRUE;
					break;

				case FLI_COPY:
#ifdef OMIT		// amazingly, this relation isn't always true!
					if ((chunk.size - sizeof(chunk)) != fullLen)
						{
						Warning(("CelReadFrame: invalid FLI_COPY chunk size,\n\
chunk.size - sizeof(chunk) = %d, fullLen = %d\n",
chunk.size - sizeof(chunk), fullLen));
						goto BADRET;
						}
#endif
					pStream->Read(fullLen, (char *) bits);
					frameDone = TRUE;
					break;

				default:
					break;
				}

			pStream->SetPos(nextOffset);
			}
		}

//	Make sure got some data in that chunk, bump current frame & return

	Free(buff);
	if (frameDone && !palOnly)
		return TRUE;
	return FALSE;
}

grs_bitmap *ResCelReadImage(IStoreStream *pStream, IResMemOverride *pResMem)
{
	Fli_head celHead;
	grs_bitmap *bm;
	uchar *bits;

//	Read header, check for error

	pStream->Read(sizeof(celHead), (char *) &celHead);
	if ((celHead.type != FLIHR_MAGIC) && (celHead.type != FLIH_MAGIC))
		return NULL;

//	Seek to first frame, set up bitmap, read it

	bm = (grs_bitmap *) pResMem->ResMalloc(celHead.width * celHead.height + sizeof(grs_bitmap));
	bits = (uchar *) (bm + 1);
	pStream->SetPos(celHead.frame1_oset);
	if (!CelReadFrame(pStream, bits, celHead.width, celHead.height, FALSE))
		{
		pResMem->ResFree(bm);
		return NULL;
		}
	gr_init_bitmap(bm, bits, BMT_FLAT8, 0, celHead.width, celHead.height);
	return bm;
}

void *ResCelReadPalette(IStoreStream *pStream, IResMemOverride *pResMem)
{
	Fli_head celHead;
	uint8 *pal;

	pStream->Read(sizeof(celHead), (char *) &celHead);
	if ((celHead.type != FLIHR_MAGIC) && (celHead.type != FLIH_MAGIC))
		return NULL;

	pal = (uint8 *) pResMem->ResMalloc(256 * 3);
	pStream->SetPos(celHead.frame1_oset);
	if (!CelReadFrame(pStream, pal, celHead.width, celHead.height, TRUE))
		{
		pResMem->ResFree(pal);
		return NULL;
		}
	return pal;
}
