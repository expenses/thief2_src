//		GIF.C		Reads .GIF files
//		Rex E. Bradford, borrowed heavily from Jaemz' readgif.c in ipw lib
/*
 * $Source: x:/prj/tech/libsrc/gfile/RCS/gif.c $
 * $Revision: 1.5 $
 * $Author: TOML $
 * $Date: 1996/10/16 16:07:57 $
 */

#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <gif.h>
#include <resapi.h>
#include <storeapi.h>
#include <grs.h>
#include <bitmap.h>

typedef struct {
	char sig[6];
	ushort screenwidth;
	ushort screendepth;
	uchar flags;
	uchar background;
	uchar aspect;
} GifHeader;

typedef struct {
	ushort left;
	ushort top;
	ushort width;
	ushort depth;
	uchar flags;
} ImageBlock;

#define NO_CODE		-1

bool ResUnpackImage(IStoreStream *pStream, int bits, uchar *buffer, ImageBlock *pImg)
{
	short int bits2;  	    /* Bits plus 1 */
	short int codesize;       /* Current code size in bits */
	short int codesize2;      /* Next codesize */
	short int nextcode;       /* Next available table entry */
	short int thiscode;       /* Code being expanded */
	short int oldtoken;       /* Last symbol decoded */
	short int currentcode;    /* Code just read */
	short int oldcode;        /* Code read before this one */
	short int bitsleft;       /* Number of bits left in *p */
	short int blocksize;      /* Bytes in next block */
	short int line=0;	    /* next line to write */
	short int byte=0;         /* next byte to write */
	short int pass=0;	    /* pass number for interlaced pictures */

	uchar *p;	         /* Pointer to current byte in read buffer */
	uchar *q;            /* Pointer past last byte in read buffer */
	uchar b[255];        /* Read buffer */
	uchar *u;            /* Stack pointer into firstcodestack */
	uchar *linebuffer;   /* place to store the current line */

	/* Jesus Christ, allocate these */
	uchar *firstcodestack;  /* Stack for first codes */
	uchar *lastcodestack;   /* Statck for previous code */
	short int *codestack;  /* Stack for links */

	static short int wordmasktable[] = {  \
		0x0000,0x0001,0x0003,0x0007,
		0x000f,0x001f,0x003f,0x007f,
		0x00ff,0x01ff,0x03ff,0x07ff,
		0x0fff,0x1fff,0x3fff,0x7fff};

	static int inctable[] = { 8,8,4,2,0 }; /* interlace increments */
	static int startable[] = { 0,4,2,1,0 };  /* interlace starts */

	/* Allocate code table space */

	firstcodestack = (uchar *)malloc(4096);
	lastcodestack = (uchar *)malloc(4096);
	codestack = (short int *)malloc(2*4096);

	p=q=b;
	bitsleft = 8;

	if (bits < 2 || bits > 8) return FALSE;
	bits2 = 1 << bits;
	nextcode = bits2 + 2;
	codesize2 = 1 << (codesize = bits + 1);
	oldcode = oldtoken = NO_CODE;

	if ((linebuffer = (uchar *)malloc(pImg->width)) == NULL)
		return FALSE;

	/* loop until something breaks */

	while(1)
		{
		if (bitsleft == 8)
			{
			if (++p >= q &&
			(((blocksize = IStoreStream_Getc(pStream)) < 1) ||
			(q= (p=b) + IStoreStream_Read(pStream, blocksize, (char *)b)) < (b+blocksize)))
				{
				free(linebuffer);
				return FALSE;
				}
			bitsleft = 0;
			}

		thiscode = *p;
		if ((currentcode=(codesize+bitsleft)) <= 8)
			{
			*p >>= codesize;
			bitsleft = currentcode;
			}
		else
			{
			if (++p >= q &&
			  (((blocksize = IStoreStream_Getc(pStream)) < 1) ||
			  (q=(p=b)+IStoreStream_Read(pStream, blocksize, (char *)b)) < (b+blocksize)))
				{
				free(linebuffer);
				return FALSE;
				}
			thiscode |= *p << (8 - bitsleft);
			if (currentcode <= 16)
				*p >>= (bitsleft=currentcode-8);
			else
				{
				if (++p >= q &&
				  (((blocksize = IStoreStream_Getc(pStream)) < 1) ||
				  (q=(p=b) + IStoreStream_Read(pStream, blocksize, (char *)b)) < (b+blocksize)))
					{
					free(linebuffer);
   				return FALSE;
					}
				thiscode |= *p << (16 - bitsleft);
				*p >>= (bitsleft = currentcode - 16);
				}
			}
		thiscode &= wordmasktable[codesize];
		currentcode = thiscode;

		if (thiscode == (bits2+1)) break;	/* found EOI */
		if (thiscode > nextcode)
			{
			free(linebuffer);
			return FALSE;
			}

		if (thiscode == bits2)
			{
			nextcode = bits2 + 2;
			codesize2 = 1 << (codesize = (bits + 1));
			oldtoken = oldcode = NO_CODE;
			continue;
			}

		u = firstcodestack;

		if (thiscode==nextcode)
			{
			if (oldcode==NO_CODE)
				{
				free(linebuffer);
            return FALSE;
				}
			*u++ = oldtoken;
			thiscode = oldcode;
			}

		while (thiscode >= bits2)
			{
			*u++ = lastcodestack[thiscode];
			thiscode = codestack[thiscode];
			}

		oldtoken = thiscode;
		do {
			linebuffer[byte++]=thiscode;

			if (byte >= pImg->width)
				{
            // Hmmm, I think this means write out the line
				if (line < pImg->depth)
					memcpy(buffer + (long) line * (long) pImg->width,
						linebuffer, pImg->width);
				byte=0;

				/* check for interlaced image */
				if (pImg->flags & 0x40)
					{
					line+=inctable[pass];
					if(line >= pImg->depth)
					    line=startable[++pass];
					}
				else
					++line;
				}

			if (u <= firstcodestack) break;
			thiscode = *--u;
			} while(1);

		if (nextcode < 4096 && oldcode != NO_CODE)
			{
			codestack[nextcode] = oldcode;
			lastcodestack[nextcode] = oldtoken;
			if (++nextcode >= codesize2 && codesize < 12)
			    codesize2 = 1 << ++codesize;
			}
		oldcode = currentcode;
		}

	free(linebuffer);
	free(firstcodestack);
	free(lastcodestack);
	free(codestack);

	return TRUE;
}

//	---------------------------------------------------------------
//
//	ResPutExtension() is called when the GIF decoder encounters an extension

void ResPutExtension(IStoreStream *pStream)
{
	int n;
	int header_size;
   int tail;

	switch ((short)IStoreStream_Getc(pStream))
		{
		case 0x0001:		/* plain text descriptor */
			header_size = 13;
			tail = 1;
			break;
		case 0x00f9:		/* graphic control block */
			header_size = 6;
			tail = 0;
			break;
		case 0x00fe:		/* comment extension */
			header_size = 0;
			tail = 1;
			break;
		case 0x00ff:		/* application extension */
			header_size = 12;
			tail = 1;
			break;
		default:		/* something else */
			header_size = 0;
			tail = 0;
			n=(short)IStoreStream_Getc(pStream);
			IStoreStream_SetPos(pStream, IStoreStream_GetPos(pStream) + n);
			break;
		}

	IStoreStream_SetPos(pStream, IStoreStream_GetPos(pStream) + header_size);

	if (tail == 1)
		{
		do {
			n=(short)IStoreStream_Getc(pStream);
			IStoreStream_SetPos(pStream, IStoreStream_GetPos(pStream) + n);
			} while(n > 0 && n != EOF);
 	 	}
}

//	-------------------------------------------------------
//		READ ROUTINE
//	-------------------------------------------------------
//
//	ResGifReadImage() reads in a GIF file.

grs_bitmap *ResGifReadImage(IStoreStream *pStream, IResMemOverride *pResMem)
{
	GifHeader gifHeader;
	ImageBlock imageBlock;
	grs_bitmap *pbm;
	int sizePall;
	int c;

//	Read header, if not valid GIF file return

	IStoreStream_Read(pStream, sizeof(GifHeader), (char *) &gifHeader);
	if (memcmp(gifHeader.sig, "GIF", 3))
		return NULL;

//	Skip palette if there is one

	if (gifHeader.flags & 0x80)
		{
		sizePall = (1 << ((gifHeader.flags & 7) + 1));
		IStoreStream_SetPos(pStream, IStoreStream_GetPos(pStream) + 3 * sizePall);
		}

//	Step through the blocks, looking for 1st image block

	c = (short)IStoreStream_Getc(pStream);
	while ((c != EOF) && ((c == ',') || (c == '!') || (c == 0)))
		{
		/* if it's an image block... */
		if (c == ',')
			{
			/* get the start of the image block */
			IStoreStream_Read(pStream, sizeof(ImageBlock), (char *) &imageBlock);

			/* skip the local colour map if there is one */
			if (imageBlock.flags & 0x80)
				IStoreStream_SetPos(pStream, IStoreStream_GetPos(pStream) + 3 * 256);

			/* get the initial code size */
			if ((c = (short)IStoreStream_Getc(pStream)) == EOF)
				return NULL;

			/* unpack the image */
			pbm = (grs_bitmap *) pResMem->ResMalloc(imageBlock.width * imageBlock.depth + sizeof(grs_bitmap));
			if (!ResUnpackImage(pStream, c, (uchar *) (pbm + 1), &imageBlock))
				{
				pResMem->ResFree(pbm);
				return NULL;
				}
			gr_init_bitmap(pbm, (uchar *) (pbm + 1), BMT_FLAT8, 0, imageBlock.width, imageBlock.depth);
			return pbm;
			}
		/* otherwise, it's an extension */
		if (c == '!')
			ResPutExtension(pStream);
		c = (short)IStoreStream_Getc(pStream);
		}

	return NULL;
}

//	-------------------------------------------------------
//
//	ResGifReadPalette() reads the global palette of a GIF file.

void *ResGifReadPalette(IStoreStream *pStream, IResMemOverride *pResMem)
{
	GifHeader gifHeader;
	IStoreStream_Read(pStream, sizeof(GifHeader), (char *) &gifHeader);
	if (memcmp(gifHeader.sig, "GIF", 3) || !(gifHeader.flags & 0x80))
		return NULL;

	int sizePall = (1 << ((gifHeader.flags & 7) + 1));
	uchar *ppall = (uchar *) pResMem->ResMalloc(3 * 256);
	memset(ppall, 0, 3 * 256);
	IStoreStream_Read(pStream, 3 * sizePall, (char *) ppall);
	return ppall;
}
