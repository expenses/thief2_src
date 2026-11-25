#ifndef __IMGDDS_H
#define __IMGDDS_H

struct grs_bitmap;
struct IStoreStream;
struct IResMemOverride;

enum eResDXTMode
{
   kResDXTHardware,
   kResDXTDecompress,
   kResDXTAuto,
};

extern int g_ResDXTMode;
EXTERN int g_bForce32BitTextures;

grs_bitmap *ResDdsReadImage(IStoreStream *pStream, IResMemOverride *pResMem);

#endif
