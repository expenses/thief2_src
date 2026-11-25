#include <windows.h>
#include <stdlib.h>
#include <lg.h>
#include <storeapi.h>
#include <resapi.h>
#include <bitmap.h>
#include <grs.h>
#include <mprintf.h>
#include <lglog.h>
#include <resimage.h>
#include <resmtl.h>
#include <imgdds.h>

extern "C" BOOL UseD3DDisplay(void);

#define DDS_MAGIC               0x20534444
#define DDS_FOURCC_DXT1         0x31545844
#define DDS_FOURCC_DXT2         0x32545844
#define DDS_FOURCC_DXT3         0x33545844
#define DDS_FOURCC_DXT4         0x34545844
#define DDS_FOURCC_DXT5         0x35545844
#define DDSCAPS2_CUBEMAP        0x00000200
#define DDSCAPS2_CUBEMAP_ALL    0x0000fc00

struct sDdsPixelFormat
{
   ulong size;
   ulong flags;
   ulong fourCC;
   ulong rgbBitCount;
   ulong rMask;
   ulong gMask;
   ulong bMask;
   ulong aMask;
};

struct sDdsHeader
{
   ulong magic;
   ulong size;
   ulong flags;
   ulong height;
   ulong width;
   ulong pitch;
   ulong depth;
   ulong mipMapCount;
   ulong reserved1[11];
   sDdsPixelFormat pf;
   ulong caps;
   ulong caps2;
   ulong caps3;
   ulong caps4;
   ulong reserved2;
};

static const uchar g_DxtAlpha4To8[16] =
{
   0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
   0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
};

#define DXT_B(c) ((uchar)(((c) << 3) | (((c) >> 2) & 7)))
#define DXT_G(c) ((uchar)((((c) >> 3) & 0xfc) | (((c) >> 9) & 3)))
#define DXT_R(c) ((uchar)((((c) >> 8) & 0xf8) | (((c) >> 13) & 7)))

static void DxtDecodeBlockDXT1(const uchar *pBlock, uchar *pDst, int nCols, int nRows, int nStride)
{
   ushort c0 = ((const ushort *)pBlock)[0];
   ushort c1 = ((const ushort *)pBlock)[1];
   uchar col0[3], col1[3], col2[3], col3[3];
   int i, x, y;

   col0[0] = DXT_B(c0); col0[1] = DXT_G(c0); col0[2] = DXT_R(c0);
   col1[0] = DXT_B(c1); col1[1] = DXT_G(c1); col1[2] = DXT_R(c1);

   if (c0 > c1)
   {
      for (i = 0; i < 3; i++)
      {
         col2[i] = (2 * col0[i] + col1[i]) / 3;
         col3[i] = (col0[i] + 2 * col1[i]) / 3;
      }
   }
   else
   {
      col3[0] = col3[1] = col3[2] = 0;
      for (i = 0; i < 3; i++)
         col2[i] = (col0[i] + col1[i]) / 2;
   }

   for (y = 0; y < nRows; y++)
   {
      uchar *p = pDst;
      for (x = 0; x < nCols; x++)
      {
         switch ((*(const ulong *)(pBlock + 4) >> (y * 8 + x * 2)) & 3)
         {
            case 0: *p++ = col0[0]; *p++ = col0[1]; *p++ = col0[2]; *p++ = 0xff; break;
            case 1: *p++ = col1[0]; *p++ = col1[1]; *p++ = col1[2]; *p++ = 0xff; break;
            case 2: *p++ = col2[0]; *p++ = col2[1]; *p++ = col2[2]; *p++ = 0xff; break;
            case 3: *p++ = col3[0]; *p++ = col3[1]; *p++ = col3[2]; *p++ = 0xff; break;
         }
      }
      pDst += nStride;
   }
}

static void DxtDecodeBlockDXT1A(const uchar *pBlock, uchar *pDst, int nCols, int nRows, int nStride)
{
   ushort c0 = ((const ushort *)pBlock)[0];
   ushort c1 = ((const ushort *)pBlock)[1];
   uchar col0[3], col1[3], col2[3], col3[4];
   int i, x, y;

   col0[0] = DXT_B(c0); col0[1] = DXT_G(c0); col0[2] = DXT_R(c0);
   col1[0] = DXT_B(c1); col1[1] = DXT_G(c1); col1[2] = DXT_R(c1);

   if (c0 > c1)
   {
      for (i = 0; i < 3; i++)
      {
         col2[i] = (2 * col0[i] + col1[i]) / 3;
         col3[i] = (col0[i] + 2 * col1[i]) / 3;
      }
      col3[3] = 0xff;
   }
   else
   {
      *(ulong *)col3 = 0;
      for (i = 0; i < 3; i++)
         col2[i] = (col0[i] + col1[i]) / 2;
   }

   for (y = 0; y < nRows; y++)
   {
      uchar *p = pDst;
      for (x = 0; x < nCols; x++)
      {
         switch ((*(const ulong *)(pBlock + 4) >> (y * 8 + x * 2)) & 3)
         {
            case 0: *p++ = col0[0]; *p++ = col0[1]; *p++ = col0[2]; *p++ = 0xff; break;
            case 1: *p++ = col1[0]; *p++ = col1[1]; *p++ = col1[2]; *p++ = 0xff; break;
            case 2: *p++ = col2[0]; *p++ = col2[1]; *p++ = col2[2]; *p++ = 0xff; break;
            case 3: *p++ = col3[0]; *p++ = col3[1]; *p++ = col3[2]; *p++ = col3[3]; break;
         }
      }
      pDst += nStride;
   }
}

static void DxtDecodeBlockDXT3(const uchar *pBlock, uchar *pDst, int nCols, int nRows, int nStride)
{
   ushort c0 = ((const ushort *)pBlock)[4];
   ushort c1 = ((const ushort *)pBlock)[5];
   uchar col0[3], col1[3], col2[3], col3[3];
   int i, x, y;

   col0[0] = DXT_B(c0); col0[1] = DXT_G(c0); col0[2] = DXT_R(c0);
   col1[0] = DXT_B(c1); col1[1] = DXT_G(c1); col1[2] = DXT_R(c1);
   for (i = 0; i < 3; i++)
   {
      col2[i] = (2 * col0[i] + col1[i]) / 3;
      col3[i] = (col0[i] + 2 * col1[i]) / 3;
   }

   for (y = 0; y < nRows; y++)
   {
      uchar *p = pDst;
      for (x = 0; x < nCols; x++)
      {
         int a = (int)(*(const unsigned __int64 *)pBlock >> (y * 16 + x * 4)) & 0xf;
         switch ((*(const ulong *)(pBlock + 12) >> (y * 8 + x * 2)) & 3)
         {
            case 0: *p++ = col0[0]; *p++ = col0[1]; *p++ = col0[2]; *p++ = g_DxtAlpha4To8[a]; break;
            case 1: *p++ = col1[0]; *p++ = col1[1]; *p++ = col1[2]; *p++ = g_DxtAlpha4To8[a]; break;
            case 2: *p++ = col2[0]; *p++ = col2[1]; *p++ = col2[2]; *p++ = g_DxtAlpha4To8[a]; break;
            case 3: *p++ = col3[0]; *p++ = col3[1]; *p++ = col3[2]; *p++ = g_DxtAlpha4To8[a]; break;
         }
      }
      pDst += nStride;
   }
}

static void DxtDecodeBlockDXT5(const uchar *pBlock, uchar *pDst, int nCols, int nRows, int nStride)
{
   ushort c0 = ((const ushort *)pBlock)[4];
   ushort c1 = ((const ushort *)pBlock)[5];
   uchar col0[3], col1[3], col2[3], col3[3];
   uchar alpha[8];
   unsigned __int64 alphaBits;
   int i, x, y;

   col0[0] = DXT_B(c0); col0[1] = DXT_G(c0); col0[2] = DXT_R(c0);
   col1[0] = DXT_B(c1); col1[1] = DXT_G(c1); col1[2] = DXT_R(c1);
   for (i = 0; i < 3; i++)
   {
      col2[i] = (2 * col0[i] + col1[i]) / 3;
      col3[i] = (col0[i] + 2 * col1[i]) / 3;
   }

   alpha[0] = pBlock[0];
   alpha[1] = pBlock[1];
   if (alpha[0] > alpha[1])
   {
      alpha[2] = (6 * alpha[0] + 1 * alpha[1]) / 7;
      alpha[3] = (5 * alpha[0] + 2 * alpha[1]) / 7;
      alpha[4] = (4 * alpha[0] + 3 * alpha[1]) / 7;
      alpha[5] = (3 * alpha[0] + 4 * alpha[1]) / 7;
      alpha[6] = (2 * alpha[0] + 5 * alpha[1]) / 7;
      alpha[7] = (1 * alpha[0] + 6 * alpha[1]) / 7;
   }
   else
   {
      alpha[2] = (4 * alpha[0] + 1 * alpha[1]) / 5;
      alpha[3] = (3 * alpha[0] + 2 * alpha[1]) / 5;
      alpha[4] = (2 * alpha[0] + 3 * alpha[1]) / 5;
      alpha[5] = (1 * alpha[0] + 4 * alpha[1]) / 5;
      alpha[6] = 0;
      alpha[7] = 0xff;
   }

   alphaBits = *(const unsigned __int64 *)pBlock >> 16;
   for (y = 0; y < nRows; y++)
   {
      uchar *p = pDst;
      for (x = 0; x < nCols; x++)
      {
         int a = (int)(alphaBits >> (y * 12 + x * 3)) & 7;
         switch ((*(const ulong *)(pBlock + 12) >> (y * 8 + x * 2)) & 3)
         {
            case 0: *p++ = col0[0]; *p++ = col0[1]; *p++ = col0[2]; *p++ = alpha[a]; break;
            case 1: *p++ = col1[0]; *p++ = col1[1]; *p++ = col1[2]; *p++ = alpha[a]; break;
            case 2: *p++ = col2[0]; *p++ = col2[1]; *p++ = col2[2]; *p++ = alpha[a]; break;
            case 3: *p++ = col3[0]; *p++ = col3[1]; *p++ = col3[2]; *p++ = alpha[a]; break;
         }
      }
      pDst += nStride;
   }
}

typedef void (*tDxtBlockDecoder)(const uchar *pBlock, uchar *pDst, int nCols, int nRows, int nStride);

#define DXT_READ_BLOCKS(name, decoder, blockSize) \
static void name(IStoreStream *pStream, uchar **ppDst, int w, int h, uint nBlocksW, uint nBlocksH, uchar *pBuf) \
{ \
   int nStride = w * 4; \
   int xRem = w & 3; \
   int yRem = h & 3; \
   uint nFullW = xRem ? nBlocksW - 1 : nBlocksW; \
   uint nFullH = yRem ? nBlocksH - 1 : nBlocksH; \
   uint x, y; \
   const uchar *pSrc; \
   uchar *pDst; \
   for (y = 0; y < nFullH; y++) \
   { \
      pStream->Read(nBlocksW * blockSize, (char *)pBuf); \
      pSrc = pBuf; \
      pDst = *ppDst; \
      for (x = 0; x < nFullW; x++) \
      { \
         decoder(pSrc, pDst, 4, 4, nStride); \
         pSrc += blockSize; \
         pDst += 16; \
      } \
      if (xRem) \
         decoder(pSrc, pDst, xRem, 4, nStride); \
      *ppDst += nStride * 4; \
   } \
   if (yRem) \
   { \
      pStream->Read(nBlocksW * blockSize, (char *)pBuf); \
      pSrc = pBuf; \
      pDst = *ppDst; \
      for (x = 0; x < nFullW; x++) \
      { \
         decoder(pSrc, pDst, 4, yRem, nStride); \
         pSrc += blockSize; \
         pDst += 16; \
      } \
      if (xRem) \
         decoder(pSrc, pDst, xRem, yRem, nStride); \
      *ppDst += yRem * nStride; \
   } \
}

DXT_READ_BLOCKS(DxtReadDXT1A, DxtDecodeBlockDXT1A, 8)
DXT_READ_BLOCKS(DxtReadDXT1, DxtDecodeBlockDXT1, 8)
DXT_READ_BLOCKS(DxtReadDXT3, DxtDecodeBlockDXT3, 16)
DXT_READ_BLOCKS(DxtReadDXT5, DxtDecodeBlockDXT5, 16)

static void DdsReadRGB24(IStoreStream *pStream, uchar **ppDst, uchar *pBuf, int w, int h)
{
   int x, y;
   for (y = 0; y < h; y++)
   {
      pStream->Read(w * 3, (char *)pBuf);
      const uchar *pSrc = pBuf;
      uchar *pDst = *ppDst;
      for (x = 0; x < w; x++)
      {
         *pDst++ = *pSrc++;
         *pDst++ = *pSrc++;
         *pDst++ = *pSrc++;
         *pDst++ = 0xff;
      }
      *ppDst = pDst;
   }
}

#define DDS_DIM(v, i)              ((((v) >> (i))) ? ((v) >> (i)) : 1)
#define DDS_BLOCKS(v, i, rnd, sh)  (((((v) >> (i)) + (rnd)) >> (sh)) ? ((((v) >> (i)) + (rnd)) >> (sh)) : 1)

static void DdsInitMip(grs_bitmap *bm, uchar *pBits, int type, int flags, int i, int w, int h, int nMips)
{
   if (i == 0)
   {
      gr_init_bitmap(bm, pBits, type, flags, w, h);
      bm->mips = nMips - 1;
   }
   else
   {
      gr_init_bitmap(bm, pBits, type, flags, DDS_DIM(bm[-1].w, 1), DDS_DIM(bm[-1].h, 1));
      bm->mips = bm[-1].mips - 1;
   }
}

grs_bitmap *ResDdsReadImage(IStoreStream *pStream, IResMemOverride *pResMem)
{
   sDdsHeader hdr;
   int type, flags, compression, bpp;
   int blockShift, blockRound;
   int nMips, nFaces, nHeaders, mode;
   int w, h, i, face;
   BOOL bTerminate;
   ulong nSize;
   grs_bitmap *pResult, *bm;
   uchar *pBits, *pBuf;

   pStream->SetPos(0);
   pStream->Read(sizeof(hdr), (char *)&hdr);
   if (hdr.magic != DDS_MAGIC)
      return NULL;

   type = BMT_FLAT32;
   compression = 0;
   blockShift = 0;
   blockRound = 0;
   bpp = 2;

   switch (hdr.pf.fourCC)
   {
      case DDS_FOURCC_DXT1:
         blockShift = 2;
         flags = BMF_RGB_888;
         compression = (1 << 8) | BMEF_MIPMAPPED | BMEF_NATIVE;
         bpp = 8;
         goto dxt;
      case DDS_FOURCC_DXT2:
         compression = (2 << 8) | BMEF_MIPMAPPED | BMEF_NATIVE;
         goto dxtn;
      case DDS_FOURCC_DXT3:
         compression = (3 << 8) | BMEF_MIPMAPPED | BMEF_NATIVE;
         goto dxtn;
      case DDS_FOURCC_DXT4:
         compression = (4 << 8) | BMEF_MIPMAPPED | BMEF_NATIVE;
         goto dxtn;
      case DDS_FOURCC_DXT5:
         compression = (5 << 8) | BMEF_MIPMAPPED | BMEF_NATIVE;
      dxtn:
         blockShift = 2;
         bpp = 16;
         flags = BMF_RGB_8888 | BMF_TRANS;
      dxt:
         blockRound = 3;
         type = BMT_DXT;
         break;

      default:
         if (hdr.pf.rgbBitCount == 32
             && hdr.pf.rMask == 0xff0000 && hdr.pf.gMask == 0xff00 && hdr.pf.bMask == 0xff)
         {
            flags = (hdr.pf.aMask == 0xff000000) ? (BMF_RGB_8888 | BMF_TRANS) : BMF_RGB_888;
            bpp = 4;
            break;
         }
         if (hdr.pf.rgbBitCount == 24
             && hdr.pf.rMask == 0xff0000 && hdr.pf.gMask == 0xff00 && hdr.pf.bMask == 0xff)
         {
            flags = BMF_RGB_888;
            bpp = 3;
            break;
         }
         if (hdr.pf.rgbBitCount == 16)
         {
            if (hdr.pf.rMask == 0xf800 && hdr.pf.gMask == 0x7e0 && hdr.pf.bMask == 0x1f)
            {
               type = BMT_FLAT16;
               flags = BMF_RGB_565;
               break;
            }
            if (hdr.pf.rMask == 0xf00 && hdr.pf.gMask == 0xf0 && hdr.pf.bMask == 0xf
                && hdr.pf.aMask == 0xf000)
            {
               type = BMT_FLAT16;
               flags = BMF_RGB_4444;
               break;
            }
         }
         mprintf("ERROR: unsupported DDS format (%d-bit, %08x %08x %08x %08x)\n",
                 hdr.pf.rgbBitCount, hdr.pf.rMask, hdr.pf.gMask, hdr.pf.bMask, hdr.pf.aMask);
         mprint(_LogFmt("ERROR: unsupported DDS format (%d-bit, %08x %08x %08x %08x)\n",
                        hdr.pf.rgbBitCount, hdr.pf.rMask, hdr.pf.gMask, hdr.pf.bMask, hdr.pf.aMask));
         return NULL;
   }

   if (hdr.caps2 & DDSCAPS2_CUBEMAP)
   {
      if ((hdr.caps2 & DDSCAPS2_CUBEMAP_ALL) != DDSCAPS2_CUBEMAP_ALL)
      {
         mprintf("ERROR: invalid dds cubemap file, all sides are required\n");
         return NULL;
      }
      if (UseD3DDisplay())
         compression |= BMEF_CUBEMAP | BMEF_MIPMAPPED | BMEF_NATIVE;
   }

   h = hdr.height;
   w = hdr.width;
   nMips = (hdr.mipMapCount > 1) ? hdr.mipMapCount : 1;
   nFaces = (compression & BMEF_CUBEMAP) ? 6 : 1;
   if (nMips > 1)
      compression |= BMEF_MIPMAPPED | BMEF_NATIVE;
   bTerminate = compression & BMEF_MIPMAPPED;
   nHeaders = bTerminate ? nMips + 1 : nMips;

   mode = 0;
   if (type == BMT_DXT)
   {
      if (g_ResDXTMode)
         mode = 1;
   }
   else if (type == BMT_FLAT32 && bpp == 3)
      mode = 2;

   nSize = 0;
   if (mode)
   {
      for (i = 0; i < nMips; i++)
         nSize += DDS_DIM(h, i) * DDS_DIM(w, i) * 4;
   }
   else
   {
      for (i = 0; i < nMips; i++)
         nSize += DDS_BLOCKS(h, i, blockRound, blockShift) * DDS_BLOCKS(w, i, blockRound, blockShift) * bpp;
   }

   pResult = (grs_bitmap *)pResMem->ResMalloc((nHeaders * sizeof(grs_bitmap) + nSize) * nFaces);
   bm = pResult;
   pBits = (uchar *)pResult + nHeaders * nFaces * sizeof(grs_bitmap);

   if (mode == 0)
   {
      for (face = 0; face < nFaces; face++)
      {
         for (i = 0; i < nMips; i++)
         {
            DdsInitMip(bm, pBits, type, (compression << 16) | flags, i, w, h, nMips);
            bm++;
            ulong nLevel = DDS_BLOCKS(w, i, blockRound, blockShift) * DDS_BLOCKS(h, i, blockRound, blockShift) * bpp;
            pStream->Read(nLevel, (char *)pBits);
            pBits += nLevel;
         }
         if (bTerminate)
         {
            memset(bm, 0, sizeof(*bm));
            bm++;
         }
      }
      return pResult;
   }

   if (mode == 1)
   {
      flags = (hdr.pf.fourCC == DDS_FOURCC_DXT1) ? BMF_RGB_888 : (BMF_RGB_8888 | BMF_TRANS);
      compression &= ~0xf00;
      pBuf = (uchar *)malloc(((w + 3) / 4) * bpp);
      for (face = 0; face < nFaces; face++)
      {
         for (i = 0; i < nMips; i++)
         {
            DdsInitMip(bm, pBits, BMT_FLAT32, (compression << 16) | flags, i, w, h, nMips);
            bm++;
            int mw = DDS_DIM(w, i);
            int mh = DDS_DIM(h, i);
            uint bw = DDS_BLOCKS(w, i, blockRound, blockShift);
            uint bh = DDS_BLOCKS(h, i, blockRound, blockShift);
            switch (hdr.pf.fourCC)
            {
               case DDS_FOURCC_DXT1:
                  if (g_pResImageMaterial && (g_pResImageMaterial->flags & (kMtlForceFullAlpha | kMtlForceAlphaKey)))
                     DxtReadDXT1A(pStream, &pBits, mw, mh, bw, bh, pBuf);
                  else
                     DxtReadDXT1(pStream, &pBits, mw, mh, bw, bh, pBuf);
                  break;
               case DDS_FOURCC_DXT2:
               case DDS_FOURCC_DXT3:
                  DxtReadDXT3(pStream, &pBits, mw, mh, bw, bh, pBuf);
                  break;
               case DDS_FOURCC_DXT4:
               case DDS_FOURCC_DXT5:
                  DxtReadDXT5(pStream, &pBits, mw, mh, bw, bh, pBuf);
                  break;
            }
         }
         if (bTerminate)
         {
            memset(bm, 0, sizeof(*bm));
            bm++;
         }
      }
   }
   else
   {
      pBuf = (uchar *)malloc(w * bpp);
      for (face = 0; face < nFaces; face++)
      {
         for (i = 0; i < nMips; i++)
         {
            DdsInitMip(bm, pBits, type, (compression << 16) | flags, i, w, h, nMips);
            bm++;
            DdsReadRGB24(pStream, &pBits, pBuf, DDS_DIM(w, i), DDS_DIM(h, i));
         }
         if (bTerminate)
         {
            memset(bm, 0, sizeof(*bm));
            bm++;
         }
      }
   }
   free(pBuf);
   return pResult;
}
