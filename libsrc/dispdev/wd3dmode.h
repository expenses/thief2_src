#ifndef __WD3DMODE_H
#define __WD3DMODE_H

#include <d3d9.h>

class cD3DProvider;
class cD3DHW2DCache;
struct IHW2DRenderer;
F_DECLARE_INTERFACE(IGameShell);

enum eD3DModeState
{
    kD3DHasDepth        = 0x00000002,
    kD3DHasStencil      = 0x00000004,
    kD3DNonPow2         = 0x00000008,
    kD3DDeviceLost      = 0x00000010,
    kD3DCanScissor      = 0x00000020,
    kD3DCanSRGB         = 0x00000040,
    kD3DCanAToC         = 0x00000080,
    kD3DDirty           = 0x00010000,
    kD3DNewFrame        = 0x00020000,
    kD3DNeedsClear      = 0x00040000,
    kD3DNoRGB10         = 0x00080000,
    kD3D2DDirty         = 0x00100000,
    kD3DPresentWindowed = 0x00200000
};

enum eD3D2DSurfMode
{
    kD3D2DSysmem,
    kD3D2DSysmemTexture,
    kD3D2DDynamicTexture,
    kD3D2DRenderTarget,
    kD3D2DRenderTargetMSAA
};

#define D3D_NUM_GPU_QUERIES 3

class cD3DModeOpsBase : public cWinDisplayModeOperations
{
public:
    cD3DModeOpsBase(cWinDisplayDevice * pDisplayDevice, cD3DProvider * pProvider);

    virtual BOOL DoSetGamma(double gamma);

protected:
    virtual ~cD3DModeOpsBase();

    friend class cD3DProvider;
    friend class cD3DHW2DCache;
    friend void D3DCreateDesktopCanvas();

    cD3DProvider *          m_pProvider;
    IDirect3DDevice9 *      m_pDevice;
    IDirect3DTexture9 *     m_pSecondarySurface;
    IDirect3DTexture9 *     m_pSecondaryTexture;
    D3DSURFACE_DESC         m_BackBufferDesc;
    D3DSURFACE_DESC         m_SecondaryDesc;
    D3DPRESENT_PARAMETERS   m_PresentParams;
    DWORD                   m_fState;
    int                     m_nLockDepth;
    BOOL                    m_bStarted;
    BOOL                    m_bHWGamma;
};

class cD3DModeOps : public cD3DModeOpsBase
{
public:
    cD3DModeOps(cWinDisplayDevice * pDisplayDevice, cD3DProvider * pProvider, const sGrModeInfo & modeInfo,
                BOOL bUIScale, BOOL bPersistent);

    virtual BOOL OnTaskSwitch(BOOL bActive);
    virtual BOOL DoStatHTrace();
    virtual BOOL DoStatVTrace();
    virtual BOOL DoPageFlip();
    virtual BOOL DoLock(sGrModeCap *);
    virtual BOOL DoUnlock();
    virtual void DoFlush();
    virtual void DoFlushRect(int x0, int y0, int x1, int y1);
    virtual BOOL DoSetGamma(double gamma);
    virtual BOOL DoSetFlip(int bFlip);
    virtual ~cD3DModeOps();
    virtual BOOL StartMode(const sGrModeInfo &, int flags, sGrModeCap * pReturnModeInfo);
    virtual BOOL RestoreSurfaces();
    virtual void SetFrame(int nFrame);
    virtual void Clear2DDirty();
    virtual void Flush2DCache();

    IDirect3DDevice9 * GetDevice() { return m_pDevice; }

protected:
    friend class cD3DProvider;
    friend class cD3DHW2DCache;

    BOOL CheckDevice();
    BOOL UnlockSurface();
    void Present();
    void BlitToBackBuffer(BOOL bForce);
    void Blit2D(RECT * pDest, RECT * pSrc, RECT * pBars, int nBars);
    void ClearSecondary();
    BOOL CreateSecondarySurface();
    HRESULT CreateSecondaryTexture();
    BOOL SetDefaultStates(IDirect3DStateBlock9 ** ppStateBlock);
    BOOL CreateBlitResources();
    void ReleaseBlitResources();
    void ChooseAlphaTextureFormat();
    UINT FindRefreshRate(int width, int height, D3DFORMAT fmt);
    D3DMULTISAMPLE_TYPE FindMultiSampleType(D3DFORMAT fmt, BOOL bWindowed);
    D3DFORMAT FindDepthStencilFormat(D3DFORMAT fmt);
    BOOL CanUseRGB10(BOOL bWindowed);
    void ThrottleFrame();
    HRESULT PollGPUQueries(BOOL bWait);
    void ReleaseSurfaces(BOOL bDevice);

    sGrModeInfo             m_ModeInfo;
    BOOL                    m_bUIScale;
    BOOL                    m_bPersistent;
    int                     m_n2DSurfMode;
    DWORD                   m_nFrameCapUs;
    unsigned __int64        m_LastFrameUs;
    IGameShell *            m_pGameShell;
    int                     m_nWidth;
    int                     m_nHeight;
    int                     m_nTexWidth;
    int                     m_nTexHeight;
    IDirect3DSurface9 *     m_pMSAASurface;
    BOOL                    m_bPersistLocks;
    D3DLOCKED_RECT          m_LockedRect;
    int                     m_nGPUQuery;
    int                     m_nGPUQueriesPending;
    int                     m_nGPUQueries;
    IDirect3DQuery9 *       m_apGPUQueries[D3D_NUM_GPU_QUERIES];
    IDirect3DStateBlock9 *  m_pBlitStateBlock;
    IDirect3DStateBlock9 *  m_pSavedStateBlock;
    float                   m_fTexelOffsetX;
    float                   m_fTexelOffsetY;
    BOOL                    m_bFilterScale2D;
    double                  m_Gamma;
    IDirect3DPixelShader9 * m_pGammaShader;
    cD3DHW2DCache *         m_p2DCache;
    IHW2DRenderer *         m_pLost2DCache;
    int                     m_n2DCacheSize;
};

EXTERN D3DFORMAT g_D3DAlphaTextureFormat;
EXTERN DWORD g_D3DWindowMode;
EXTERN BOOL g_bD3DPaintPending;
EXTERN BOOL g_bD3DClipCursor;
EXTERN BOOL g_bD3DDeviceRestored;
EXTERN IDirect3DSurface9 * g_pD3DDesktopSurface;
EXTERN cD3DModeOps * g_pD3DModeOps;

void D3DClipCursorToWindow(HWND hwnd);
BOOL D3DFitWindowToMonitor(HWND hwnd, int height);
int D3DGetAlphaToCoverageMode(IDirect3DDevice9 * pDevice);
void D3DSetCritMsgHandler(cD3DProvider * pProvider);
void D3DCreateDesktopCanvas();
void D3DDestroyDesktopCanvas();
void D3DReleaseDesktopCanvas();

#endif /* !__WD3DMODE_H */
