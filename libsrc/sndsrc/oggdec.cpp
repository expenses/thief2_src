////////////////////////////////////////////////////////////////////////
//
// Module name: Ogg Vorbis decompressor
// File name: oggdec.cpp
//
////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <errno.h>
#include <lg.h>
#include <lgsound.h>
#include <oggdec.h>

static size_t OggDecodeRead( void *ptr, size_t size, size_t nmemb, void *datasource )
{
   sOggDecodeSrc *pSrc = (sOggDecodeSrc *) datasource;
   long n = size * nmemb;

   if ( n > pSrc->bytesLeft )
      n = pSrc->bytesLeft;
   if ( n <= 0 ) {
      if ( n < 0 )
         errno = EINVAL;
      return 0;
   }
   memcpy( ptr, pSrc->pData + pSrc->pos, n );
   pSrc->pos += n;
   pSrc->bytesLeft -= n;
   return n / size;
}

cOggDecoder::~cOggDecoder()
{
   if ( mOpen )
      ov_clear( &mVF );
}

long
cOggDecoder::DecompressBlock( BOOL isStereo, char *pIn, short *pOut, long nSamps )
{
   long hdrLen;

   if ( mOpen )
      return Decompress( pIn, pOut, nSamps );

   mSrc.pData = pIn;
   if ( strncmp( pIn, "DrkS", 4 ) == 0 ) {
      mSrc.pos = sizeof(sDrkSHeader);
      mSrc.bytesLeft = ((sDrkSHeader *) pIn)->dataLen + sizeof(sDrkSHeader);
   } else {
      mSrc.pos = 0;
      mSrc.bytesLeft = 0x7fffffff;
   }

   ov_callbacks callbacks = { OggDecodeRead, NULL, NULL, NULL };
   if ( ov_open_callbacks( &mSrc, &mVF, NULL, 0, callbacks ) < 0 )
      return 0;

   hdrLen = mSrc.pos;
   mOpen = TRUE;
   return Decompress( pIn + hdrLen, pOut, nSamps ) + hdrLen;
}

long
cOggDecoder::Decompress( char *pIn, short *pOut, long nSamps )
{
   char *pDst = (char *) pOut;
   int bitstream;
   long nBytes, nRead;

   if ( !mOpen )
      return 0;

   nBytes = 2 * nSamps * mVF.vi->channels;
   mSrc.pData = pIn;
   mSrc.pos = 0;
   while ( nBytes > 0 ) {
      nRead = ov_read( &mVF, pDst, nBytes, 0, 2, 1, &bitstream );
      if ( bitstream == 0 ) {
         if ( nRead <= 0 )
            break;
         pDst += nRead;
         nBytes -= nRead;
      }
   }
   return mSrc.pos;
}
