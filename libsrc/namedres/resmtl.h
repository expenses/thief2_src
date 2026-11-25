#pragma once

#include <comtools.h>

struct grs_bitmap;
F_DECLARE_INTERFACE(IRes);

struct sMtlUVXform
{
   float m[2][3];
};

enum eMtlWaveType
{
   kMtlWaveSine,
   kMtlWaveAbsSine,
   kMtlWaveTriangle,
   kMtlWaveSquare,
   kMtlWaveSawtooth,
   kMtlWaveInvSawtooth,
   kMtlWaveTurb,
};

enum eMtlFunc
{
   kMtlFuncConst,
   kMtlFuncIncidence,
   kMtlFuncWave,
};

struct sMtlWave
{
   int type;
   int period;
   int phase;
   float base;
   float amplitude;
   float steps;
   float stepScale;
};

struct sMtlFrame
{
   grs_bitmap *pBitmap;
   IRes *pRes;
};

#define kMtlLayerTranslucent  0x01
#define kMtlLayerShaded       0x02
#define kMtlLayerTexture      0x04
#define kMtlLayerLightmap     0x08
#define kMtlLayerReplaceAlpha 0x10
#define kMtlLayerAnimated     0x20
#define kMtlLayerAdditive     0x40
#define kMtlLayerLocEnvTex    0x80
#define kMtlLayerLocEnvTex2   0x100

enum eMtlLayerWave
{
   kMtlWaveRGB,
   kMtlWaveAlpha,
   kMtlWaveUScale,
   kMtlWaveVScale,
   kMtlWaveUOffset,
   kMtlWaveVOffset,
   kMtlWaveRotate,
   kMtlNumWaves
};

#define kMtlMaxAniFrames 1024
#define kMtlDefAniRate   250

struct sMtlTexLayer
{
   ulong flags;
   grs_bitmap *pBitmap;
   int mipmapBias;
   int srcBlend;
   int dstBlend;
   int uvSource;
   BOOL bUVClamp;
   int rgbFunc;
   int alphaFunc;
   float rgb[3];
   float alpha;
   ulong packedColor;
   int uvFunc[5];
   sMtlWave waves[kMtlNumWaves];
   float pivot[2];
   float scale[2];
   float rotation;
   float offset[2];
   sMtlUVXform xform;
   grs_bitmap *apLUT[2];
   int nFrames;
   int aniRate;
   int aniMode;
   void *pFrames;
   IRes *apLUTRes[2];
};

#define kMtlMaxLayers 6

#define kMtlAniRate          0x02
#define kMtlTerrainScale     0x10

struct sMaterial
{
   ulong flags;
   int aniRate;
   ulong aniMode;
   int aniFrames;
   int reserved;
   int nLayers;
   sMtlTexLayer layers[kMtlMaxLayers];
   int terrainScale[2];
   int tileFactor[2];
   float tileScale[2];
   grs_bitmap *pBitmap;
};

#define kMtlHasLayers         0x0001
#define kMtlLegacyChromaKey   0x0004
#define kMtlChromaKeyToAlpha  0x0008
#define kMtlForceOpaque       0x0020
#define kMtlForceFullAlpha    0x0040
#define kMtlForceAlphaKey     0x0080
#define kMtlNoMipmap          0x0100
#define kMtlUVClamp           0x0200
#define kMtlEdgePadding       0x0400
#define kMtlNoEdgePadding     0x0800
#define kMtlAnimated          0x1000
#define kMtlRenderMtlOnly     0x2000
#define kMtlForceLocalPal     0x4000
#define kMtlForceAniSettings  0x8000

#define kTexBmPadded   0x02
#define kTexBmNoPad    0x10

struct sTexBitmap
{
   grs_bitmap bm;
   ushort reserved;
   uchar scaleWLog;
   uchar scaleHLog;
   short scaleW;
   short scaleH;
   ushort texFlags;
   uchar bTerrain;
   uchar pad;
   sMaterial *pMtl;
};

sMaterial *MtlLoadForRes(IRes *pRes);
void MtlMaterialUnloadTextures(sMaterial *pMtl);
void MtlMaterialFree(sMaterial *pMtl);
void MtlTexEdgePad(sTexBitmap *pTex);

EXTERN BOOL g_bMaterialsEnabled;
EXTERN sMaterial *g_pResImageMaterial;
