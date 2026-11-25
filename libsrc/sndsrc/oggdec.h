////////////////////////////////////////////////////////////////////////
//
// Module name: Ogg Vorbis decompressor
// File name: oggdec.h
//
////////////////////////////////////////////////////////////////////////

#ifndef OGGDEC_H
#define OGGDEC_H

#include <imaadpcm.h>
#pragma pack(push, 8)
#include <vorbis/vorbisfile.h>
#pragma pack(pop)

struct sOggDecodeSrc
{
   char  *pData;
   long  pos;
   long  bytesLeft;
};

class cOggDecoder : public cSndDecoder {
public:
   cOggDecoder() : mOpen(FALSE) {}
   virtual        ~cOggDecoder();

   virtual long   DecompressBlock( BOOL isStereo, char *pIn, short *pOut, long nSamps );
   virtual long   Decompress( char *pIn, short *pOut, long nSamps );

private:
   sOggDecodeSrc  mSrc;
   OggVorbis_File mVF;
   BOOL           mOpen;
};

#endif // OGGDEC_H
