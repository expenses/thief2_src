#pragma once

#include <ddraw.h>
#include <lgsurf.h>

DEFINE_LG_GUID(IID_ILGDD4Surface, 0x269);

#undef INTERFACE
#define INTERFACE ILGDD4Surface
DECLARE_INTERFACE_(ILGDD4Surface, IUnknown)
{
    STDMETHOD_(int, GetDeviceInfoIndex)(THIS) PURE;
    STDMETHOD_(int, GetDirectDraw)(THIS_ IDirectDraw4**) PURE;
    STDMETHOD_(int, GetRenderSurface)(THIS_ IDirectDrawSurface4**) PURE;
};

struct cLGSurface : ILGSurface, ILGDD4Surface
{
    DECLARE_UNAGGREGATABLE();
    int             m_nFrameCount;
    IDirectDraw4*   m_lpDD4;
    IDirectDrawSurface4* m_lpScreen;
    IDirectDrawSurface4* m_lpTheSurface;
    int             m_nD3DDevID;
    eLGSurfState    m_eSurfState;
    int             m_bZbuffer;
    int             m_b3DAttached;
    sLGSurfaceDescription m_sDescription;
    grs_canvas      m_sCanvas;

    cLGSurface();
    ~cLGSurface();

    // ILGSurface methods
    STDMETHOD(CreateInternalScreenSurface)(THIS_ HWND);
    STDMETHOD(ChangeClipper)(THIS_ HWND);
    STDMETHOD(SetAs3dHdwTarget)(THIS_ DWORD, DWORD, DWORD, DWORD, DWORD*, grs_canvas**);
    STDMETHOD(Resize3dHdw)(THIS_ DWORD, DWORD, DWORD, DWORD, DWORD*, grs_canvas**);
    STDMETHOD(BlitToScreen)(THIS_ DWORD, DWORD, DWORD, DWORD);
    STDMETHOD_(void, CleanSurface)(THIS);
    STDMETHOD_(void, Start3D)(THIS);
    STDMETHOD_(void, End3D)(THIS);
    STDMETHOD(LockFor2D)(THIS_ grs_canvas**, eLGSBlit);
    STDMETHOD(UnlockFor2D)(THIS);
    STDMETHOD_(DWORD, GetWidth)(THIS);
    STDMETHOD_(DWORD, GetHeight)(THIS);
    STDMETHOD_(DWORD, GetPitch)(THIS);
    STDMETHOD_(DWORD, GetBitDepth)(THIS);
    STDMETHOD_(eLGSurfState, GetSurfaceState)(THIS);
    STDMETHOD(GetSurfaceDescription)(THIS_ sLGSurfaceDescription*);

    // ILGDD4Surface methods
    STDMETHOD_(int, GetDeviceInfoIndex)(THIS);
    STDMETHOD_(int, GetDirectDraw)(THIS_ IDirectDraw4**);
    STDMETHOD_(int, GetRenderSurface)(THIS_ IDirectDrawSurface4**);

    // Internal
    void ReleaseContent();
    void RecordSurfaceDescription(_DDSURFACEDESC2* lpDDSD);
    void FixThePitchInCanvas() { m_sCanvas.bm.row = m_sDescription.dwPitch; }
    void SetBitsInCanvas(void* pvData) { m_sCanvas.bm.bits = (uchar*)pvData; }
};
