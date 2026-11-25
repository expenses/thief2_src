///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/wddmode.h $
// $Author: KEVIN $
// $Date: 1997/10/21 16:18:06 $
// $Revision: 1.8 $
//

#ifndef __WDDMODE_H
#define __WDDMODE_H

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDDModeOpsBase
//
// This intermediate base class encapsulates the common
// elements of the "real" DirectDraw modes and the
// "spoofed" modes.  It contains the needed pointers
// to the DirectDraw interfaces, but no specific
// knowledge about how they are used.
//

class cDDModeOpsBase : public cWinDisplayModeOperations
{
public:
    cDDModeOpsBase(cWinDisplayDevice * pDisplayDevice, cDDProvider * pOuter, LPDIRECTDRAWPALETTE pPalette)
      : m_pOuter(pOuter),
        m_pPrimarySurface(0),
        m_pSecondarySurface(0),
        m_pPalette(pPalette),
        m_Flags(0),
        cWinDisplayModeOperations(pDisplayDevice)

    {
        if (m_pPalette)
            m_pPalette->AddRef();
    }
    virtual ~cDDModeOpsBase();

    ///////////////////////////////

    virtual BOOL StartMode(const sGrModeInfo &, int flags, sGrModeCap * pReturnModeInfo) = 0;
    virtual BOOL DoSetPalette(LOGPALETTE & palette, unsigned uStart, unsigned n);
    virtual BOOL DoSetGamma(double gamma);

    ///////////////////////////////

    void AttachPalette(LPDIRECTDRAWPALETTE pNewPalette)
    {
        if (m_pPalette)
            m_pPalette->Release();
        if (pNewPalette)
            pNewPalette->AddRef();
        m_pPalette = pNewPalette;
    }

    ///////////////////////////////

    LPDIRECTDRAWPALETTE GetPalette()
    {
        if (m_pPalette)
            m_pPalette->AddRef();
        return m_pPalette;
    }

    ///////////////////////////////
    
    void GetSurfaces(IDirectDrawSurface4 ** ppPrimarySurface,
                     IDirectDrawSurface4 ** ppSecondarySurface);

    ///////////////////////////////
    virtual void RestoreSurfaces()
    {
        if (m_pPrimarySurface->IsLost())
            m_pPrimarySurface->Restore();
        if (m_pSecondarySurface && m_pSecondarySurface->IsLost())
            m_pSecondarySurface->Restore();
    }

    ///////////////////////////////

    void SetClipToWindow(BOOL fWantClipping);

protected:
    ///////////////////////////////
    // Utility functions to simplify access to outer object
    DWORD GetCoopFlags()
    {
        return m_pOuter->m_DDCoopFlags;
    }

    LPDIRECTDRAW4 GetDD()
    {
        return m_pOuter->m_pDD;
    }

    HRESULT SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP)
    {
        YieldDisplay(m_pDisplayDevice);

        m_pOuter->SetCooperativeLevel(cDDProvider::kDDWindowedCoopFlags);
        SetWindowPos(m_pOuter->GetMainWnd(),
                     HWND_TOPMOST,
                     0, 0,
                     dwWidth, dwHeight,
                     SWP_SHOWWINDOW	| SWP_NOCOPYBITS);
        m_pOuter->SetCooperativeLevel(cDDProvider::kDDFullScreenCoopFlags);

        RegainDisplay(m_pDisplayDevice);
        return m_pOuter->m_pDD->SetDisplayMode(dwWidth, dwHeight, dwBPP, 0, 0);
    }

    PALETTEENTRY * GetPaletteEntries()
    {
        return m_pDisplayDevice->GetPaletteEntries();
    }

    void SetModeInfoFromSurfaceDesc(const DDSURFACEDESC2 &, int flags, sGrModeCap * pReturnModeInfo);

    BOOL IsModeX()
    {
        return (m_PrimaryDesc.dwWidth < 400);
    }

    BOOL WipeSurface(IDirectDrawSurface4 *pdds);

    ///////////////////////////////

    cDDProvider * m_pOuter;

    LPDIRECTDRAWSURFACE4 m_pPrimarySurface;  // DirectDraw primary surface
    LPDIRECTDRAWSURFACE4 m_pSecondarySurface;     // DirectDraw back surface
    LPDIRECTDRAWPALETTE m_pPalette;       // DirectDraw palette
    DDSURFACEDESC2       m_PrimaryDesc;
    DDSURFACEDESC2       m_SecondaryDesc;
    int                 m_Flags;
    BOOL                m_bFlip;

    enum eFlags
    {
        kClippingToWindow = 0x01
    };
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDirectDDModeOps
//
// This class implements the direct DirectDraw operations, where the locked
// pointer points to the visible screen
//

class cDirectDDModeOps : public cDDModeOpsBase
{
public:
    cDirectDDModeOps(cWinDisplayDevice * pDisplayDevice, cDDProvider * pOuter, LPDIRECTDRAWPALETTE pPalette)
      : cDDModeOpsBase(pDisplayDevice, pOuter, pPalette),
        m_pPauseSurface(NULL),
        m_pPauseSurface2(NULL),
        m_bHIW(FALSE)
    {
    }

    virtual ~cDirectDDModeOps();
    virtual BOOL StartMode(const sGrModeInfo &, int flags, sGrModeCap * pReturnModeInfo);
    virtual BOOL OnTaskSwitch(BOOL bActive);
    virtual BOOL DoStatVTrace();
    virtual BOOL DoStatHTrace();
    virtual BOOL DoPageFlip();
    virtual BOOL DoLock(sGrModeCap *);
    virtual BOOL DoUnlock();
    virtual BOOL DoSetFlip(int bFlip);

private:
    // Jacobson, 3-4-96
    // These objects here are used to store off the contents of the
    // screen when the pause key is hit.
    LPDIRECTDRAWSURFACE4 m_pPauseSurface;  // DirectDraw temp surface
    LPDIRECTDRAWSURFACE4 m_pPauseSurface2; // DirectDraw temp surface
    DDSURFACEDESC2      m_PauseDesc;
    int                 m_bHIW;
    void                PauseBlit(LPDIRECTDRAWSURFACE4 pDest,LPDIRECTDRAWSURFACE4 pSource);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhoney512ModeOps
//
// This class implements the faked 512x384 for Dark.
// @TBD (toml 12-09-96): This must be generalized for all faked "subregion" modes
//

class cPhoney512ModeOps : public cDirectDDModeOps
{
public:
    cPhoney512ModeOps(cWinDisplayDevice * pDisplayDevice, cDDProvider * pOuter, LPDIRECTDRAWPALETTE pPalette)
      : cDirectDDModeOps(pDisplayDevice, pOuter, pPalette)
    {
    }

    virtual BOOL StartMode(const sGrModeInfo &, int flags, sGrModeCap * pReturnModeInfo);
    virtual BOOL DoLock(sGrModeCap *);
    virtual BOOL DoUnlock();
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cOffVideoDDModeOps
//
// This class implements the offscreen DirectDraw operations, where the locked
// pointer is a DirectDraw surface in system memory
//

class cOffVideoDDModeOps : public cDDModeOpsBase
{
public:
    cOffVideoDDModeOps(cWinDisplayDevice * pDisplayDevice, cDDProvider * pOuter, LPDIRECTDRAWPALETTE pPalette)
      : cDDModeOpsBase(pDisplayDevice, pOuter, pPalette)
    {
    }

    virtual BOOL StartMode(const sGrModeInfo &, int flags, sGrModeCap * pReturnModeInfo);
    virtual void DoFlush();
    virtual void DoFlushRect(int x0, int y0, int x1, int y1);
    virtual BOOL DoLock(sGrModeCap *);
    virtual BOOL DoUnlock();
    virtual BOOL DoSetFlip(int bFlip);

private:
    int                 m_nBackBuffers;
    BOOL                m_bFlipOnFlush;
    BOOL                m_bLocked;
};

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cFixedDDModeOps
//

struct IDirect3D3;
struct IDirect3DDevice3;
struct IDirect3DViewport3;
struct IDirect3DTexture2;
F_DECLARE_INTERFACE(IGameShell);

class cFixedDDModeOps : public cDDModeOpsBase
{
public:
    cFixedDDModeOps(cWinDisplayDevice * pDisplayDevice, cDDProvider * pOuter, LPDIRECTDRAWPALETTE pPalette,
                    const sGrModeInfo & desktopMode, int nScaleMode, BOOL bPersistent)
      : cDDModeOpsBase(pDisplayDevice, pOuter, pPalette),
        m_bOwnSecondary(FALSE),
        m_pScaleSurface(NULL),
        m_pOverlaySurface(NULL),
        m_nReserved(0),
        m_bLockBack(FALSE),
        m_DesktopMode(desktopMode),
        m_nScaleMode(nScaleMode),
        m_bPersistent(bPersistent),
        m_nFrameCapUs(0),
        m_LastFrameUs(0),
        m_pGameShell(NULL),
        m_pBackBuffer(NULL),
        m_pD3D(NULL),
        m_pD3DDevice(NULL),
        m_pViewport(NULL),
        m_pTexSurface(NULL),
        m_pTexture(NULL),
        m_pRenderTarget(NULL),
        m_fTexelAdjustX(0.5f),
        m_fTexelAdjustY(0.5f)
    {
        m_bFlip = FALSE;
    }
    virtual ~cFixedDDModeOps();
    virtual BOOL StartMode(const sGrModeInfo &, int flags, sGrModeCap * pReturnModeInfo);
    virtual BOOL OnTaskSwitch(BOOL bActive);
    virtual BOOL DoStatHTrace();
    virtual BOOL DoPageFlip();
    virtual BOOL DoLock(sGrModeCap *);
    virtual BOOL DoUnlock();
    virtual void DoFlush();
    virtual BOOL DoSetFlip(int bFlip);
    virtual void RestoreSurfaces();

    BOOL IsDesktopMode(const sGrModeInfo & mode)
    {
        return m_DesktopMode.w == mode.w && m_DesktopMode.h == mode.h && m_DesktopMode.bitDepth == mode.bitDepth &&
               !((m_DesktopMode.flags ^ mode.flags) & kGrModeIsNative);
    }

private:
    void ThrottleFrame();
    BOOL InitFilteredBlit();
    void FilteredBlit(RECT * pDest);
    void ReleaseFilteredBlit();

    BOOL                    m_bOwnSecondary;
    LPDIRECTDRAWSURFACE4    m_pScaleSurface;
    LPDIRECTDRAWSURFACE4    m_pOverlaySurface;
    int                     m_nReserved;
    DDSURFACEDESC2          m_ScaleDesc;
    BOOL                    m_bLockBack;
    sGrModeInfo             m_DesktopMode;
    int                     m_nScaleMode;
    BOOL                    m_bPersistent;
    DWORD                   m_nFrameCapUs;
    unsigned __int64        m_LastFrameUs;
    IGameShell *            m_pGameShell;
    LPDIRECTDRAWSURFACE4    m_pBackBuffer;
    IDirect3D3 *            m_pD3D;
    IDirect3DDevice3 *      m_pD3DDevice;
    IDirect3DViewport3 *    m_pViewport;
    LPDIRECTDRAWSURFACE4    m_pTexSurface;
    IDirect3DTexture2 *     m_pTexture;
    LPDIRECTDRAWSURFACE4    m_pRenderTarget;
    float                   m_fTexelAdjustX;
    float                   m_fTexelAdjustY;
};


#endif /* !__WDDMODE_H */
