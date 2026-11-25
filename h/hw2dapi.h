#ifndef __HW2DAPI_H
#define __HW2DAPI_H

#include <comtools.h>
#include <dev2d.h>

#ifndef D3DCOLOR_DEFINED
typedef DWORD D3DCOLOR;
#define D3DCOLOR_DEFINED
#endif

typedef struct sHW2DVertex sHW2DVertex;
typedef struct sHW2DVertex2 sHW2DVertex2;
typedef struct sHW2DEntry sHW2DEntry;

F_DECLARE_INTERFACE(IHW2DRenderer);

#undef INTERFACE
#define INTERFACE IHW2DRenderer

DECLARE_INTERFACE_(IHW2DRenderer, IUnknown)
{
    DECLARE_UNKNOWN_PURE();

    STDMETHOD_(void, ColorFill)(THIS_ int x0, int y0, int x1, int y1, D3DCOLOR color) PURE;
    STDMETHOD_(void, ClearDepth)(THIS_ int x0, int y0, int x1, int y1) PURE;
    STDMETHOD_(void, FillRect)(THIS_ int x0, int y0, int x1, int y1, D3DCOLOR color, uint flags) PURE;
    STDMETHOD_(void, DrawLine)(THIS_ int x0, int y0, int x1, int y1, D3DCOLOR color, uint flags) PURE;
    STDMETHOD_(void, DrawPixel)(THIS_ int x, int y, D3DCOLOR color, uint flags) PURE;
    STDMETHOD_(void, Blit)(THIS_ grs_bitmap * pBm, int x, int y, uint flags) PURE;
    STDMETHOD_(void, ScaleBlit)(THIS_ grs_bitmap * pBm, int x, int y, int w, int h, uint flags) PURE;
    STDMETHOD_(void, DrawEllipse)(THIS_ int x, int y, int rx, int ry, D3DCOLOR color) PURE;
    STDMETHOD_(void, FillEllipse)(THIS_ int x, int y, int rx, int ry, D3DCOLOR color) PURE;
    STDMETHOD_(void, DrawPoly)(THIS_ grs_bitmap * pBm, sHW2DVertex * pVerts, int n, uint flags) PURE;
    STDMETHOD_(void, DrawPolyMT)(THIS_ grs_bitmap * pBm, grs_bitmap * pBm2, sHW2DVertex2 * pVerts, int n,
                                 uint mtOp, uint flags) PURE;
    STDMETHOD_(BOOL, SetBilinear)(THIS_ BOOL bOn) PURE;
    STDMETHOD_(uint, SetZMode)(THIS_ uint mode) PURE;
    STDMETHOD_(double, SetZBias)(THIS_ double bias) PURE;
    STDMETHOD_(BOOL, SetColorWrite)(THIS_ BOOL bOn) PURE;
    STDMETHOD_(void, FlushBatch)(THIS) PURE;
    STDMETHOD_(sHW2DEntry *, Cache)(THIS_ grs_bitmap * pBm) PURE;
    STDMETHOD_(void, Pin)(THIS_ grs_bitmap * pBm) PURE;
    STDMETHOD_(void, AddFont)(THIS_ struct grs_font * pFont) PURE;
    STDMETHOD_(void, Invalidate)(THIS_ grs_bitmap * pBm) PURE;
    STDMETHOD_(void, Discard)(THIS_ grs_bitmap * pBm) PURE;
    STDMETHOD_(void, Update)(THIS_ grs_bitmap * pBm) PURE;
    STDMETHOD_(void, CopyFromScreen)(THIS_ grs_bitmap * pBm, int x, int y) PURE;
    STDMETHOD_(void, MakeTarget)(THIS_ grs_bitmap * pBm) PURE;
    STDMETHOD_(void, BeginCanvas)(THIS_ grs_canvas * pCanvas) PURE;
    STDMETHOD_(void, EndCanvas)(THIS_ grs_canvas * pCanvas) PURE;
    STDMETHOD_(BOOL, ReadBack)(THIS_ grs_bitmap * pSrc, grs_bitmap * pDest) PURE;
#ifdef __cplusplus
    virtual ~IHW2DRenderer() {}
#endif
};

EXTERN IHW2DRenderer * g_pHW2D;
EXTERN uint g_HW2DBilinear;

#define IHW2DRenderer_Blit(p, a, b, c, d)                COMCall4(p, Blit, a, b, c, d)
#define IHW2DRenderer_Invalidate(p, a)                   COMCall1(p, Invalidate, a)
#define IHW2DRenderer_CopyFromScreen(p, a, b, c)         COMCall3(p, CopyFromScreen, a, b, c)

#endif
