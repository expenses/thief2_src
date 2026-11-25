#ifndef __WDISPD3D_H
#define __WDISPD3D_H

#include <d3d9.h>

class cD3DModeOps;

const char * WhatD3DError(HRESULT error);

EXTERN BOOL g_bWD3D9Device;
EXTERN sGrModeInfo g_DesktopModeInfo;
EXTERN sGrModeInfo g_D3DWindowedModeInfo;
EXTERN RECT g_rcUIScaleView;
EXTERN BOOL g_bUIScaleMode;
EXTERN BOOL g_bUIScaleView;
EXTERN BOOL g_bUIScaleDrawn;
EXTERN int g_nD3DFrame;

class cD3DProvider : public cWinDisplayProvider
{
public:
    cD3DProvider(cWinDisplayDevice * pDisplayDevice);
    virtual ~cD3DProvider();

    enum
    {
        kD3DFullScreenStyle = (DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE | DDSCL_ALLOWMODEX),
        kD3DWindowedStyle   = DDSCL_NORMAL
    };

    IDirect3D9 * GetD3D() { return m_pD3D; }
    UINT GetAdapter() { return m_nAdapter; }

private:
    virtual BOOL DoProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam, long * pRetVal);
    virtual void DoGetInfo(sGrDeviceInfo *, sGrModeInfo *);
    virtual BOOL DoOpen(sGrModeCap *, int flags);
    virtual BOOL DoClose();
    virtual cWinDisplayModeOperations * DoSetMode(const sGrModeInfo &, int flags, sGrModeCap * pReturnModeInfo);
    virtual BOOL DoGetDirectDraw(IDirectDraw4 **);
    virtual BOOL DoGetBitmapSurface(sGrBitmap *, IDirectDrawSurface4 **);
    virtual BOOL DoIsPersistentMode();

    void SetWindowMode(DWORD mode);
    int GetDisplayWidth();
    int GetDisplayHeight();

    friend class cD3DModeOpsBase;
    friend class cD3DModeOps;

    cD3DModeOps *       m_pModeOps;
    BOOL                m_bPersistentMode;
    IDirect3D9 *        m_pD3D;
    UINT                m_nAdapter;
    DWORD               m_WindowMode;
    long                m_NormalWindowStyle;
    long                m_NormalWindowExStyle;

    enum eFlags
    {
        kTriedFallback  = 0x01,
        kDesktopRes     = 0x02
    };

    int                 m_Flags;
};

#endif /* !__WDISPD3D_H */
