#ifndef __WD3DHW2D_H
#define __WD3DHW2D_H

#include <d3d9.h>
#include <hw2dapi.h>

class cD3DModeOps;

struct sHW2DVertex
{
    float x, y, z, rhw;
    D3DCOLOR color;
    float u, v;
};

struct sHW2DVertex2
{
    float x, y, z, rhw;
    D3DCOLOR color;
    float u0, v0;
    float u1, v1;
};

#define HW2D_FVF            (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)
#define HW2D_FVF2           (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX2)
#define HW2D_MAX_TRIS       0x300
#define HW2D_MAX_POLY_VERTS 0x100

enum eHW2DFlags
{
    kHW2DBlendMask      = 0x000f,
    kHW2DBlendNone      = 0,
    kHW2DBlendAlpha     = 2,
    kHW2DBlendAdd       = 3,
    kHW2DTextured       = 0x0010,
    kHW2DAlphaTest      = 0x0020,
    kHW2DAlphaTexture   = 0x0040,
    kHW2DFiltered       = 0x0080,
    kHW2DWrap           = 0x0100,
    kHW2DMultiTex       = 0x0200,
    kHW2DMTOpMask       = 0x0c00,
    kHW2DMTModulate2    = 0x1000,
    kHW2DDiffuse        = 0x2000,
    kHW2DPolygon        = 0x4000,
    kHW2DBilinear       = 0x0080
};

enum eHW2DEntryFlags
{
    kHW2DEntryFlip      = 0x001,
    kHW2DEntryFont      = 0x002,
    kHW2DEntryAlpha     = 0x004,
    kHW2DEntryRawFont   = 0x008,
    kHW2DEntryUploaded  = 0x010,
    kHW2DEntryLocked    = 0x020,
    kHW2DEntryTarget    = 0x040,
    kHW2DEntryDirty     = 0x080,
    kHW2DEntryStale     = 0x100
};


struct sHW2DEntry
{
    uchar *                 pBits;
    uchar *                 pEnd;
    sHW2DEntry *            pNext;
    sHW2DEntry *            pPrev;
    IDirect3DTexture9 *     pTexture[2];
    grs_bitmap *            pBitmap;
    int                     w;
    int                     h;
    int                     nFrame;
    int                     nBytes;
    DWORD                   flags;
};

class cD3DHW2DCache : public IHW2DRenderer
{
public:
    cD3DHW2DCache(cD3DModeOps * pModeOps, int nCacheSize);
    virtual ~cD3DHW2DCache();

    STDMETHOD(QueryInterface)(REFIID, void **);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    STDMETHOD_(void, ColorFill)(int x0, int y0, int x1, int y1, D3DCOLOR color);
    STDMETHOD_(void, ClearDepth)(int x0, int y0, int x1, int y1);
    STDMETHOD_(void, FillRect)(int x0, int y0, int x1, int y1, D3DCOLOR color, uint flags);
    STDMETHOD_(void, DrawLine)(int x0, int y0, int x1, int y1, D3DCOLOR color, uint flags);
    STDMETHOD_(void, DrawPixel)(int x, int y, D3DCOLOR color, uint flags);
    STDMETHOD_(void, Blit)(grs_bitmap * pBm, int x, int y, uint flags);
    STDMETHOD_(void, ScaleBlit)(grs_bitmap * pBm, int x, int y, int w, int h, uint flags);
    STDMETHOD_(void, DrawEllipse)(int x, int y, int rx, int ry, D3DCOLOR color);
    STDMETHOD_(void, FillEllipse)(int x, int y, int rx, int ry, D3DCOLOR color);
    STDMETHOD_(void, DrawPoly)(grs_bitmap * pBm, sHW2DVertex * pVerts, int n, uint flags);
    STDMETHOD_(void, DrawPolyMT)(grs_bitmap * pBm, grs_bitmap * pBm2, sHW2DVertex2 * pVerts, int n, uint mtOp, uint flags);
    STDMETHOD_(BOOL, SetBilinear)(BOOL bOn);
    STDMETHOD_(uint, SetZMode)(uint mode);
    STDMETHOD_(double, SetZBias)(double bias);
    STDMETHOD_(BOOL, SetColorWrite)(BOOL bOn);
    STDMETHOD_(void, FlushBatch)();
    STDMETHOD_(sHW2DEntry *, Cache)(grs_bitmap * pBm);
    STDMETHOD_(void, Pin)(grs_bitmap * pBm);
    STDMETHOD_(void, AddFont)(struct grs_font * pFont);
    STDMETHOD_(void, Invalidate)(grs_bitmap * pBm);
    STDMETHOD_(void, Discard)(grs_bitmap * pBm);
    STDMETHOD_(void, Update)(grs_bitmap * pBm);
    STDMETHOD_(void, CopyFromScreen)(grs_bitmap * pBm, int x, int y);
    STDMETHOD_(void, MakeTarget)(grs_bitmap * pBm);
    STDMETHOD_(void, BeginCanvas)(grs_canvas * pCanvas);
    STDMETHOD_(void, EndCanvas)(grs_canvas * pCanvas);
    STDMETHOD_(BOOL, ReadBack)(grs_bitmap * pSrc, grs_bitmap * pDest);

    BOOL Flush();
    void FlushAll();

private:
    BOOL Begin(uint flags);
    void FlushQuads();
    void SetState(uint flags);
    void SetTexture(sHW2DEntry * pEntry);
    void SetTextureCoords(sHW2DEntry * pEntry, grs_bitmap * pBm);
    void SetViewport();
    void GetCanvasOrigin(int * pX, int * pY);
    void SetQuad(int x0, int y0, int x1, int y1, D3DCOLOR color);
    void SetLine(int x0, int y0, int x1, int y1, D3DCOLOR color);
    void AddQuad();
    void AddLineQuad();
    void AddFan(sHW2DVertex * pVerts, int n);
    BOOL CreateDepthStencil();
    sHW2DEntry * FindEntry(grs_bitmap * pBm);
    sHW2DEntry * CacheExact(grs_bitmap * pBm);
    sHW2DEntry * NewEntry(grs_bitmap * pBm, DWORD flags);
    void Link(sHW2DEntry * pEntry);
    void Unlink(sHW2DEntry * pEntry);
    void Touch(sHW2DEntry * pEntry);
    void FreeEntry(sHW2DEntry * pEntry);
    void Purge();

    cD3DModeOps *           m_pModeOps;
    BOOL                    m_bActive;
    uint                    m_State;
    uint                    m_ZMode;
    double                  m_ZBias;
    BOOL                    m_bColorWrite;
    BOOL                    m_bFirst;
    uchar *                 m_pCanvasBits;
    int                     m_CanvasRowBytes;
    D3DVIEWPORT9            m_Viewport;
    sHW2DEntry *            m_pCurEntry;
    int                     m_CurBits;
    int                     m_CurW;
    int                     m_CurH;
    BOOL                    m_bFullTexture;
    D3DCOLOR                m_Color;
    IDirect3DSurface9 *     m_pSavedTarget;
    IDirect3DSurface9 *     m_pSavedDepth;
    IDirect3DSurface9 *     m_pDepthStencil;
    BOOL                    m_bStencil;
    IDirect3DSurface9 *     m_pReadSurface;
    int                     m_ReadW;
    int                     m_ReadH;
    sHW2DEntry *            m_pEntries;
    int                     m_nFree;
    int                     m_nBytes;
    int                     m_nFrame;
    int                     m_nPurgeFrame;
    int                     m_nCacheSize;
    int                     m_nPrimType;
    int                     m_nBatched;
    sHW2DVertex             m_Quad[4];
    sHW2DVertex             m_Batch[(HW2D_MAX_TRIS + 1) * 3];
};

class cD3DLost2DCache : public IHW2DRenderer
{
public:
    cD3DLost2DCache();
    virtual ~cD3DLost2DCache();

    STDMETHOD(QueryInterface)(REFIID, void **);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    STDMETHOD_(void, ColorFill)(int, int, int, int, D3DCOLOR) {}
    STDMETHOD_(void, ClearDepth)(int, int, int, int) {}
    STDMETHOD_(void, FillRect)(int, int, int, int, D3DCOLOR, uint) {}
    STDMETHOD_(void, DrawLine)(int, int, int, int, D3DCOLOR, uint) {}
    STDMETHOD_(void, DrawPixel)(int, int, D3DCOLOR, uint) {}
    STDMETHOD_(void, Blit)(grs_bitmap *, int, int, uint) {}
    STDMETHOD_(void, ScaleBlit)(grs_bitmap *, int, int, int, int, uint) {}
    STDMETHOD_(void, DrawEllipse)(int, int, int, int, D3DCOLOR) {}
    STDMETHOD_(void, FillEllipse)(int, int, int, int, D3DCOLOR) {}
    STDMETHOD_(void, DrawPoly)(grs_bitmap *, sHW2DVertex *, int, uint) {}
    STDMETHOD_(void, DrawPolyMT)(grs_bitmap *, grs_bitmap *, sHW2DVertex2 *, int, uint, uint) {}
    STDMETHOD_(BOOL, SetBilinear)(BOOL) { return FALSE; }
    STDMETHOD_(uint, SetZMode)(uint) { return 0; }
    STDMETHOD_(double, SetZBias)(double) { return 0.0; }
    STDMETHOD_(BOOL, SetColorWrite)(BOOL) { return FALSE; }
    STDMETHOD_(void, FlushBatch)() {}
    STDMETHOD_(sHW2DEntry *, Cache)(grs_bitmap *) { return NULL; }
    STDMETHOD_(void, Pin)(grs_bitmap *) {}
    STDMETHOD_(void, AddFont)(struct grs_font *) {}
    STDMETHOD_(void, Invalidate)(grs_bitmap *) {}
    STDMETHOD_(void, Discard)(grs_bitmap *) {}
    STDMETHOD_(void, Update)(grs_bitmap *) {}
    STDMETHOD_(void, CopyFromScreen)(grs_bitmap *, int, int) {}
    STDMETHOD_(void, MakeTarget)(grs_bitmap *) {}
    STDMETHOD_(void, BeginCanvas)(grs_canvas *) {}
    STDMETHOD_(void, EndCanvas)(grs_canvas *) {}
    STDMETHOD_(BOOL, ReadBack)(grs_bitmap *, grs_bitmap *) { return FALSE; }
};

#endif /* !__WD3DHW2D_H */
