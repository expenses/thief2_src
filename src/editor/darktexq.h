#pragma once

#ifndef __DARKTEXQ_H
#define __DARKTEXQ_H

struct grs_bitmap;
struct IRes;
struct IStoreStream;

struct sDarkTexInfo
{
   grs_bitmap *pBitmap;
   IRes *pRes;
   int loadid;
   int archetype;
};

struct sDarkTexAnimInfo
{
   int nFrames;
   IRes **ppFrames;
   int flags;
   int rate;
};

typedef void (*tDarkTexFamilyCallback)(const char *pszFamily, BOOL bLoaded, void *pData);
typedef void (*tDarkTexTextureCallback)(const char *pszTexture, BOOL bLoaded, void *pData);

class IDarkTexQuery
{
public:
   virtual ~IDarkTexQuery() {}
   virtual void FreeMem(void *p) = 0;
   virtual int GetNumTextures() = 0;
   virtual BOOL GetTexture(int idx, sDarkTexInfo *pInfo) = 0;
   virtual BOOL GetTextureAnim(int idx, sDarkTexAnimInfo *pInfo) = 0;
   virtual uchar *GetTexturePalette(int idx) = 0;
   virtual BOOL IsTextureBitmap(int idx, grs_bitmap *bm) = 0;
   virtual BOOL RenderTexture(int idx, int w, int h, uchar *pBits, int row) = 0;
   virtual void Select(int val) = 0;
   virtual void Detach() = 0;
   virtual void Close() = 0;
   virtual BOOL ExecCommand(const char *pszCmd, const char *pszArgs) = 0;
   virtual void CountTextureUse(int *pTable, int nSize, int flags) = 0;
   virtual const char *GetCurrentFaceName() = 0;
   virtual void EnumFamilies(tDarkTexFamilyCallback pfnCallback, void *pData) = 0;
   virtual void EnumFamilyTextures(const char *pszFamily, tDarkTexTextureCallback pfnCallback, void *pData) = 0;
   virtual void RefreshStorage() = 0;
   virtual IStoreStream *OpenMaterial(int idx) = 0;
};

enum eTexPalCmd
{
   kTexPalCreate,
   kTexPalDestroy,
   kTexPalUpdate,
   kTexPalSelect,
};

struct sTexPalCmd
{
   int cmd;
   int arg[5];
};

#define kDarkTexCountBrushes 0x1

#endif
