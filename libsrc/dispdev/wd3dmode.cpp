#include <windows.h>
#include <d3d9.h>
#include <lg.h>
#include <ddraw.h>
#include <math.h>
#include <mmsystem.h>

#include <comtools.h>
#include <dispapi.h>
#include <gshelapi.h>
#include <appagg.h>

#include <dev2d.h>
#include <config.h>
#include <comdfcn.h>
#include <lgassert.h>

#include <wdisp.h>
#include <wdspprov.h>
#include <wdspmode.h>
#include <wdisptls.h>

#include <wdispd3d.h>
#include <wd3dmode.h>
#include <wd3dhw2d.h>

D3DFORMAT g_D3DAlphaTextureFormat = D3DFMT_A8R8G8B8;
DWORD g_D3DWindowMode;
BOOL g_bD3DPaintPending;
BOOL g_bD3DClipCursor;
BOOL g_bD3DDeviceRestored;
HMONITOR g_hD3DMonitor;
cD3DModeOps * g_pD3DModeOps;

int g_nD3DVSync;
static BOOL g_bD3DActive;
static BOOL g_bD3DLocked;
static IDirect3DDevice9 * g_pAToCDevice;
static int g_nAToCMode;

extern float g_fMaxZ;
EXTERN IDirect3DSurface9 * g_pD3DDesktopSurface = NULL;
int D8_DeviceReset(BOOL bRestore);

static const DWORD g_GammaShader[] =
{
    0xffff0200, 0x0200001f, 0x80000000, 0xb0030000, 0x0200001f, 0x90000000, 0xa00f0800,
    0x03000042, 0x800f0000, 0xb0e40000, 0xa0e40800, 0x0200000f, 0x80010000, 0x80000000,
    0x0200000f, 0x80020000, 0x80550000, 0x0200000f, 0x80040000, 0x80aa0000, 0x03000005,
    0x80070000, 0x80e40000, 0xa0000000, 0x0200000e, 0x80010000, 0x80000000, 0x0200000e,
    0x80020000, 0x80550000, 0x0200000e, 0x80040000, 0x80aa0000, 0x02000001, 0x800f0800,
    0x80e40000, 0x0000ffff
};

struct sD3DBlitVertex
{
    float x, y, z, rhw;
    D3DCOLOR color;
    float u, v;
};

#define D3D_BLIT_FVF (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)

///////////////////////////////////////

void D3DClipCursorToWindow(HWND hwnd)
{
    RECT rect;
    POINT tl, br;

    if (!g_bD3DClipCursor)
        return;
    GetClientRect(hwnd, &rect);
    tl.x = rect.left;
    tl.y = rect.top;
    br.x = rect.right;
    br.y = rect.bottom;
    ClientToScreen(hwnd, &tl);
    ClientToScreen(hwnd, &br);
    rect.left = tl.x;
    rect.top = tl.y;
    rect.right = br.x;
    rect.bottom = br.y;
    ClipCursor(&rect);
}

int D3DGetAlphaToCoverageMode(IDirect3DDevice9 * pDevice)
{
    IDirect3D9 * pD3D;
    D3DDEVICE_CREATION_PARAMETERS params;
    D3DDISPLAYMODE mode;
    D3DADAPTER_IDENTIFIER9 id;
    int nForce;

    if (g_pAToCDevice == pDevice)
        return g_nAToCMode;
    g_nAToCMode = 0;
    g_pAToCDevice = pDevice;
    if (!config_is_defined("d3d_disp_enable_atoc"))
        return 0;
    if (FAILED(pDevice->GetDirect3D(&pD3D)))
    {
        g_pAToCDevice = NULL;
        return 0;
    }
    D3DCAPS9 caps;
    HRESULT hr = pDevice->GetDeviceCaps(&caps);
    if (FAILED(hr) || !(caps.PrimitiveMiscCaps & D3DPMISCCAPS_SEPARATEALPHABLEND)
        || FAILED(pDevice->GetCreationParameters(&params))
        || FAILED(pDevice->GetDisplayMode(0, &mode))
        || FAILED(pD3D->GetAdapterIdentifier(params.AdapterOrdinal, 0, &id)))
    {
        SafeRelease(pD3D);
        return 0;
    }

    switch (id.VendorId)
    {
        case 0x10de:
            nForce = 1;
            config_get_int("d3d_disp_enable_atoc", &nForce);
            if (nForce > 1 &&
                pD3D->CheckDeviceFormat(params.AdapterOrdinal, D3DDEVTYPE_HAL, mode.Format, 0,
                                        D3DRTYPE_SURFACE, (D3DFORMAT) MAKEFOURCC('S', 'S', 'A', 'A')) == D3D_OK)
            {
                g_nAToCMode = 2;
                break;
            }
            if (!g_nAToCMode &&
                pD3D->CheckDeviceFormat(params.AdapterOrdinal, D3DDEVTYPE_HAL, mode.Format, 0,
                                        D3DRTYPE_SURFACE, (D3DFORMAT) MAKEFOURCC('A', 'T', 'O', 'C')) == D3D_OK)
                g_nAToCMode = 1;
            break;

        case 0x1002:
        {
            DWORD old;
            pDevice->GetRenderState(D3DRS_POINTSIZE, &old);
            if (pDevice->SetRenderState(D3DRS_POINTSIZE, MAKEFOURCC('A', '2', 'M', '1')) == D3D_OK)
            {
                g_nAToCMode = 3;
                pDevice->SetRenderState(D3DRS_POINTSIZE, MAKEFOURCC('A', '2', 'M', '0'));
            }
            pDevice->SetRenderState(D3DRS_POINTSIZE, old);
            break;
        }

        case 0x8086:
            if (!g_nAToCMode &&
                pD3D->CheckDeviceFormat(params.AdapterOrdinal, D3DDEVTYPE_HAL, mode.Format, 0,
                                        D3DRTYPE_SURFACE, (D3DFORMAT) MAKEFOURCC('A', 'T', 'O', 'C')) == D3D_OK)
                g_nAToCMode = 1;
            break;
    }
    SafeRelease(pD3D);
    return g_nAToCMode;
}

static void LogD3DResult(const char * pszWhat, HRESULT hr, D3DPRESENT_PARAMETERS * pp)
{
    LogIncIndent();
    LogMsg(_LogFmt("%s: res = %s ; %dx%d fmt=%d zfmt=%d cnt=%d ms=%d swap=%d fs=%d hz=%d pres=%d flags=%X",
                   pszWhat, WhatD3DError(hr), pp->BackBufferWidth, pp->BackBufferHeight, pp->BackBufferFormat,
                   pp->AutoDepthStencilFormat, pp->BackBufferCount, pp->MultiSampleType, pp->SwapEffect,
                   !pp->Windowed, pp->FullScreen_RefreshRateInHz, pp->PresentationInterval, pp->Flags));
    LogDecIndent();
}

BOOL D3DFormatToMasks(D3DFORMAT fmt, DWORD * pRed, DWORD * pGreen, DWORD * pBlue)
{
    switch (fmt)
    {
        case D3DFMT_R8G8B8:
            *pRed = 0xff;
            *pGreen = 0xff;
            *pBlue = 0xff;
            return TRUE;
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
            *pRed = 0xff0000;
            *pGreen = 0xff00;
            *pBlue = 0xff;
            return TRUE;
        case D3DFMT_R5G6B5:
            *pRed = 0xf800;
            *pGreen = 0x7e0;
            *pBlue = 0x1f;
            return TRUE;
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
            *pRed = 0x7c00;
            *pGreen = 0x3e0;
            *pBlue = 0x1f;
            return TRUE;
        case D3DFMT_A4R4G4B4:
            *pRed = 0xf00;
            *pGreen = 0xf0;
            *pBlue = 0xf;
            return TRUE;
    }
    *pBlue = 0;
    *pGreen = 0;
    *pRed = 0;
    return FALSE;
}

static DWORD NextPow2(DWORD n)
{
    if (n & (n - 1))
    {
        DWORD bit = 0x80000000;
        if (!(n & bit))
        {
            do
                bit >>= 1;
            while (!(n & bit));
        }
        n = bit << 1;
    }
    return n;
}

static void ZeroAligned(void * p, size_t n)
{
    memset(p, 0, n);
}

///////////////////////////////////////

cD3DModeOpsBase::cD3DModeOpsBase(cWinDisplayDevice * pDisplayDevice, cD3DProvider * pProvider)
  : cWinDisplayModeOperations(pDisplayDevice),
    m_pProvider(pProvider),
    m_pDevice(NULL),
    m_pSecondarySurface(NULL),
    m_pSecondaryTexture(NULL),
    m_fState(0),
    m_nLockDepth(0),
    m_bStarted(FALSE),
    m_bHWGamma(FALSE)
{
    D3DCAPS9 caps;

    memset(&caps, 0, sizeof(caps));
    pProvider->m_pD3D->GetDeviceCaps(pProvider->m_nAdapter, D3DDEVTYPE_HAL, &caps);
    if ((caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL) || !(caps.TextureCaps & D3DPTEXTURECAPS_POW2))
        m_fState |= kD3DNonPow2;
    if (caps.RasterCaps & D3DPRASTERCAPS_SCISSORTEST)
        m_fState |= kD3DCanScissor;
    if (caps.PrimitiveMiscCaps & D3DPMISCCAPS_SEPARATEALPHABLEND)
        m_fState |= kD3DCanSRGB;
    if (caps.PrimitiveMiscCaps & D3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS)
        m_fState |= kD3DCanAToC;
    if (caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL)
        LogMsg("D3DProvider: D3D device only supports limited non-POW2 textures");

    if (!g_pD3DModeOps)
        g_pD3DModeOps = (cD3DModeOps *) this;
}

cD3DModeOpsBase::~cD3DModeOpsBase()
{
    g_bD3DClipCursor = FALSE;
    ClipCursor(NULL);
    SafeRelease(m_pSecondaryTexture);
    SafeRelease(m_pSecondarySurface);
    SafeRelease(m_pDevice);
    g_hD3DMonitor = NULL;
    if (g_pD3DModeOps == (cD3DModeOps *) this)
        g_pD3DModeOps = NULL;
}

BOOL cD3DModeOpsBase::DoSetGamma(double gamma)
{
    D3DGAMMARAMP ramp;

    if (!m_pDevice)
        return FALSE;
    if (m_bHWGamma)
        gamma = 1.0;
    for (int i = 0; i < 256; i++)
    {
        WORD val = (WORD)(pow(i / 255.0, 1.0 / gamma) * 65535.0);
        ramp.red[i] = val;
        ramp.green[i] = val;
        ramp.blue[i] = val;
    }
    m_pDevice->SetGammaRamp(0, 0, &ramp);
    return TRUE;
}

///////////////////////////////////////

cD3DModeOps::cD3DModeOps(cWinDisplayDevice * pDisplayDevice, cD3DProvider * pProvider, const sGrModeInfo & modeInfo,
                         BOOL bUIScale, BOOL bPersistent)
  : cD3DModeOpsBase(pDisplayDevice, pProvider),
    m_ModeInfo(modeInfo),
    m_bUIScale(bUIScale),
    m_bPersistent(bPersistent),
    m_n2DSurfMode(kD3D2DRenderTarget),
    m_nFrameCapUs(0),
    m_LastFrameUs(0),
    m_pGameShell(NULL),
    m_pMSAASurface(NULL),
    m_bPersistLocks(FALSE),
    m_nGPUQuery(0),
    m_nGPUQueriesPending(0),
    m_nGPUQueries(0),
    m_pBlitStateBlock(NULL),
    m_pSavedStateBlock(NULL),
    m_fTexelOffsetX(0.5f),
    m_fTexelOffsetY(0.5f),
    m_bFilterScale2D(TRUE),
    m_Gamma(1.0),
    m_pGammaShader(NULL),
    m_p2DCache(NULL),
    m_pLost2DCache(NULL),
    m_n2DCacheSize(0x1000000)
{
    m_LockedRect.Pitch = 0;
    m_LockedRect.pBits = NULL;
    memset(m_apGPUQueries, 0, sizeof(m_apGPUQueries));
}

cD3DModeOps::~cD3DModeOps()
{
    if (m_LockedRect.pBits)
        UnlockSurface();
    if (m_p2DCache)
    {
        delete m_p2DCache;
        m_p2DCache = NULL;
    }
    if (m_pLost2DCache)
    {
        delete m_pLost2DCache;
        m_pLost2DCache = NULL;
    }
    ReleaseBlitResources();
    SafeRelease(m_pMSAASurface);
    D8_DeviceReset(FALSE);
    for (int i = 0; i < D3D_NUM_GPU_QUERIES; i++)
        SafeRelease(m_apGPUQueries[i]);
    g_fMaxZ = 1.0f;
}

///////////////////////////////////////

void cD3DModeOps::ReleaseSurfaces(BOOL bDevice)
{
    if (m_p2DCache)
    {
        delete m_p2DCache;
        m_p2DCache = NULL;
    }
    ReleaseBlitResources();
    SafeRelease(m_pMSAASurface);
    SafeRelease(m_pSecondaryTexture);
    SafeRelease(m_pSecondarySurface);
    if (bDevice)
    {
        SafeRelease(m_pDevice);
        m_fState &= ~(kD3DHasDepth | kD3DHasStencil);
        g_hD3DMonitor = NULL;
    }
}

UINT cD3DModeOps::FindRefreshRate(int width, int height, D3DFORMAT fmt)
{
    int nRequested = 0;
    UINT nBest = 0;

    if (!config_get_int("d3d_disp_refreshrate", &nRequested) || nRequested < 10)
        return 0;

    IDirect3D9 * pD3D = m_pProvider->m_pD3D;
    UINT nAdapter = m_pProvider->m_nAdapter;
    UINT nModes = pD3D->GetAdapterModeCount(nAdapter, fmt);
    if (nModes)
    {
        for (UINT i = 0; i < nModes; i++)
        {
            D3DDISPLAYMODE dm;
            if (FAILED(pD3D->EnumAdapterModes(nAdapter, fmt, i, &dm)))
                continue;
            if ((int)dm.Width != width || (int)dm.Height != height)
                continue;
            if (dm.RefreshRate == (UINT)nRequested)
                return dm.RefreshRate;
            if (nRequested > 60 && dm.RefreshRate > 60 && dm.RefreshRate < (UINT)nRequested && dm.RefreshRate > nBest)
                nBest = dm.RefreshRate;
        }
        if (nBest)
        {
            LogIncIndent();
            LogMsg2("Requested refresh rate %d Hz is not supported, using closest available %d Hz", nRequested, nBest);
            LogDecIndent();
            return nBest;
        }
    }
    LogIncIndent();
    LogMsg1("Requested refresh rate %d Hz is not supported, using default", nRequested);
    LogDecIndent();
    return nBest;
}

D3DMULTISAMPLE_TYPE cD3DModeOps::FindMultiSampleType(D3DFORMAT fmt, BOOL bWindowed)
{
    int nSamples = 0;

    if (!config_get_int("multisampletype", &nSamples))
        return D3DMULTISAMPLE_NONE;
    if (nSamples < 2)
        return D3DMULTISAMPLE_NONE;
    if (nSamples > 16)
        nSamples = 16;
    for (; nSamples > 1; nSamples--)
    {
        if (SUCCEEDED(m_pProvider->m_pD3D->CheckDeviceMultiSampleType(m_pProvider->m_nAdapter, D3DDEVTYPE_HAL, fmt,
                                                                      bWindowed, (D3DMULTISAMPLE_TYPE) nSamples, NULL)))
            break;
    }
    if (nSamples == 1)
        return D3DMULTISAMPLE_NONE;
    return (D3DMULTISAMPLE_TYPE) nSamples;
}

D3DFORMAT cD3DModeOps::FindDepthStencilFormat(D3DFORMAT fmt)
{
    D3DFORMAT formats[] =
    {
        D3DFMT_D24S8, D3DFMT_D24X4S4, D3DFMT_D24FS8, D3DFMT_D32, D3DFMT_D24X8, D3DFMT_D16
    };
    UINT nAdapter = m_pProvider->m_nAdapter;

    for (int i = 0; i < 6; i++)
    {
        if (SUCCEEDED(m_pProvider->m_pD3D->CheckDeviceFormat(nAdapter, D3DDEVTYPE_HAL, fmt,
                                                             D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, formats[i])) &&
            SUCCEEDED(m_pProvider->m_pD3D->CheckDepthStencilMatch(nAdapter, D3DDEVTYPE_HAL, fmt,
                                                                  fmt, formats[i])))
            return formats[i];
    }
    return D3DFMT_UNKNOWN;
}

BOOL cD3DModeOps::CanUseRGB10(BOOL bWindowed)
{
    if (config_is_defined("d3d_disp_no_rgb10_buf"))
        return FALSE;
    UINT nAdapter = m_pProvider->m_nAdapter;
    if (m_pProvider->m_pD3D->CheckDeviceType(nAdapter, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DFMT_A2R10G10B10, bWindowed) &&
        m_pProvider->m_pD3D->CheckDeviceType(nAdapter, D3DDEVTYPE_HAL, D3DFMT_A2R10G10B10, D3DFMT_A2R10G10B10, bWindowed))
        return FALSE;
    return TRUE;
}

void cD3DModeOps::ChooseAlphaTextureFormat()
{
    D3DDEVICE_CREATION_PARAMETERS params;
    D3DDISPLAYMODE mode;

    g_D3DAlphaTextureFormat = D3DFMT_A8R8G8B8;
    if (FAILED(m_pDevice->GetCreationParameters(&params)) || FAILED(m_pDevice->GetDisplayMode(0, &mode)))
        return;
    if (!config_is_nonzero("d3d_disp_force_rgba_atex"))
    {
        IDirect3D9 * pD3D = m_pProvider->m_pD3D;
        if (SUCCEEDED(pD3D->CheckDeviceFormat(params.AdapterOrdinal, params.DeviceType, mode.Format, 0,
                                              D3DRTYPE_TEXTURE, D3DFMT_A8)))
        {
            g_D3DAlphaTextureFormat = D3DFMT_A8;
            return;
        }
        if (config_is_nonzero("d3d_disp_allow_a8l8_atex") &&
            SUCCEEDED(pD3D->CheckDeviceFormat(params.AdapterOrdinal, params.DeviceType, mode.Format, 0,
                                              D3DRTYPE_TEXTURE, D3DFMT_A8L8)))
        {
            g_D3DAlphaTextureFormat = D3DFMT_A8L8;
            return;
        }
    }
    g_D3DAlphaTextureFormat = D3DFMT_A8R8G8B8;
}

HRESULT cD3DModeOps::CreateSecondaryTexture()
{
    HRESULT hr;

    if (m_n2DSurfMode == kD3D2DSysmem)
        return D3D_OK;
    if (!m_pSecondarySurface && m_n2DSurfMode == kD3D2DSysmemTexture)
        return E_FAIL;

    if (m_n2DSurfMode == kD3D2DRenderTargetMSAA)
    {
        hr = m_pDevice->CreateRenderTarget(m_nTexWidth, m_nTexHeight, m_SecondaryDesc.Format,
                                           m_BackBufferDesc.MultiSampleType, m_BackBufferDesc.MultiSampleQuality,
                                           FALSE, &m_pMSAASurface, NULL);
        if (FAILED(hr))
            return hr;
        hr = m_pDevice->CreateTexture(m_nTexWidth, m_nTexHeight, 1, D3DUSAGE_RENDERTARGET, m_SecondaryDesc.Format,
                                      D3DPOOL_DEFAULT, &m_pSecondaryTexture, NULL);
        if (FAILED(hr))
        {
            SafeRelease(m_pMSAASurface);
            return hr;
        }
        return D3D_OK;
    }

    DWORD usage = 0;
    if (m_n2DSurfMode == kD3D2DDynamicTexture)
        usage = D3DUSAGE_DYNAMIC;
    else if (m_n2DSurfMode == kD3D2DRenderTarget)
        usage = D3DUSAGE_RENDERTARGET;
    return m_pDevice->CreateTexture(m_nTexWidth, m_nTexHeight, 1, usage, m_SecondaryDesc.Format,
                                    D3DPOOL_DEFAULT, &m_pSecondaryTexture, NULL);
}

///////////////////////////////////////

BOOL cD3DModeOps::SetDefaultStates(IDirect3DStateBlock9 ** ppStateBlock)
{
    D3DVIEWPORT9 vp;
    D3DMATRIX mat;

    int nAToC = D3DGetAlphaToCoverageMode(m_pDevice);
    if (FAILED(m_pDevice->BeginStateBlock()))
        return FALSE;

    vp.X = 0;
    vp.Y = 0;
    vp.Width = m_BackBufferDesc.Width;
    vp.Height = m_BackBufferDesc.Height;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    m_pDevice->SetViewport(&vp);

    memset(&mat, 0, sizeof(mat));
    mat._11 = mat._22 = mat._33 = mat._44 = 1.0f;
    m_pDevice->SetTransform(D3DTS_WORLD, &mat);
    m_pDevice->SetTransform(D3DTS_VIEW, &mat);
    m_pDevice->SetTransform(D3DTS_PROJECTION, &mat);

    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    m_pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    m_pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    m_pDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    m_pDevice->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
    m_pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    m_pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    m_pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    m_pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    m_pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    m_pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    m_pDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    m_pDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    m_pDevice->SetTexture(0, NULL);
    m_pDevice->SetTexture(1, NULL);
    m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    BOOL bNative = (m_ModeInfo.flags & kGrModeIsNative) != 0;
    if (bNative)
    {
        m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
        m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
    }
    else
    {
        m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    }
    m_pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, !bNative);
    if (nAToC)
    {
        if (nAToC < 3)
            m_pDevice->SetRenderState(D3DRS_ADAPTIVETESS_Y, D3DFMT_UNKNOWN);
        else
            m_pDevice->SetRenderState(D3DRS_POINTSIZE, MAKEFOURCC('A', '2', 'M', '0'));
    }
    m_pDevice->SetRenderState(D3DRS_ALPHAREF, 0);
    m_pDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
    m_pDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);
    m_pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0xffffffff);
    m_pDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
    m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    m_pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    m_pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
    m_pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    m_pDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    m_pDevice->SetRenderState(D3DRS_CLIPPING, FALSE);
    m_pDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
    if ((m_fState & kD3DCanScissor) && m_PresentParams.MultiSampleType)
        m_pDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
    m_pDevice->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
    m_pDevice->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    m_pDevice->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    m_pDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    m_pDevice->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    m_pDevice->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    m_pDevice->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
    if (m_ModeInfo.flags & kGrModeIsNative)
        m_pDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
    m_pDevice->SetPixelShader(NULL);
    m_pDevice->SetVertexShader(NULL);
    m_pDevice->SetFVF(D3D_BLIT_FVF);
    return SUCCEEDED(m_pDevice->EndStateBlock(ppStateBlock));
}

void cD3DModeOps::ReleaseBlitResources()
{
    SafeRelease(m_pGammaShader);
    SafeRelease(m_pBlitStateBlock);
    SafeRelease(m_pSavedStateBlock);
}

BOOL cD3DModeOps::CreateBlitResources()
{
    m_pDevice->SetRenderState(D3DRS_WRAP0, 0);
    m_pDevice->SetRenderState(D3DRS_WRAP1, 0);
    if (!SetDefaultStates(&m_pBlitStateBlock) || !SetDefaultStates(&m_pSavedStateBlock))
    {
        ReleaseBlitResources();
        return FALSE;
    }

    float offsets[2];
    int nOffsets = 2;
    m_fTexelOffsetX = 0.5f;
    m_fTexelOffsetY = 0.5f;
    offsets[0] = 0.5f;
    offsets[1] = 0.5f;
    if (config_get_value("texel_center_adjust", CONFIG_FLOAT_TYPE, (config_valptr)offsets, &nOffsets) && nOffsets >= 1)
    {
        if (nOffsets == 1)
            offsets[1] = offsets[0];
        m_fTexelOffsetX = offsets[0];
        m_fTexelOffsetY = offsets[1];
        if (m_fTexelOffsetX < -1.0f)
            m_fTexelOffsetX = -1.0f;
        else if (m_fTexelOffsetX > 1.0f)
            m_fTexelOffsetX = 1.0f;
        if (m_fTexelOffsetY < -1.0f)
            m_fTexelOffsetY = -1.0f;
        else if (m_fTexelOffsetY > 1.0f)
            m_fTexelOffsetY = 1.0f;
    }

    m_bHWGamma = FALSE;
    if (config_is_nonzero("d3d_disp_sw_cc"))
    {
        if (FAILED(m_pDevice->CreatePixelShader(g_GammaShader, &m_pGammaShader)))
            return FALSE;
        m_bHWGamma = config_is_nonzero("d3d_disp_sw_cc_hwgamma");
    }
    return TRUE;
}

///////////////////////////////////////

BOOL cD3DModeOps::StartMode(const sGrModeInfo & modeInfo, int flags, sGrModeCap * pReturnModeInfo)
{
    BOOL bResetDevice = FALSE;
    BOOL bWindowed;
    int n2DSurfMode;
    int nNumber;
    BOOL bGame;

    m_bPersistLocks = !config_is_defined("d3d_disp_no_persist_locks");
    g_bD3DDeviceRestored = FALSE;
    g_bD3DClipCursor = FALSE;
    ClipCursor(NULL);
    if (m_LockedRect.pBits)
        UnlockSurface();
    m_fState &= ~(kD3DDeviceLost | kD3DDirty | kD3DNewFrame | kD3D2DDirty);
    m_LockedRect.pBits = NULL;

    bGame = m_pDisplayDevice->GetCreateFlags() & kDispFullScreen;
    n2DSurfMode = kD3D2DRenderTarget;
    nNumber = 1;
    if (config_get_value("d3d_disp_2d_surf_mode", CONFIG_INT_TYPE, (config_valptr) &n2DSurfMode, &nNumber) && nNumber > 0)
    {
        if (n2DSurfMode < 0)
            n2DSurfMode = 0;
        else if (n2DSurfMode > kD3D2DRenderTarget)
            n2DSurfMode = kD3D2DRenderTarget;
    }
    if (n2DSurfMode == kD3D2DRenderTarget && ((!bGame && !(m_ModeInfo.flags & kGrModeIsNative)) || !(m_fState & kD3DNonPow2)))
        n2DSurfMode = kD3D2DSysmem;

    if (m_p2DCache)
    {
        delete m_p2DCache;
        m_p2DCache = NULL;
    }
    if (m_pLost2DCache)
    {
        delete m_pLost2DCache;
        m_pLost2DCache = NULL;
    }
    if (m_n2DSurfMode != n2DSurfMode)
    {
        m_n2DSurfMode = n2DSurfMode;
        SafeRelease(m_pMSAASurface);
        SafeRelease(m_pSecondaryTexture);
        SafeRelease(m_pSecondarySurface);
    }

    if (n2DSurfMode == kD3D2DRenderTarget)
    {
        int nSizeMB = (m_ModeInfo.flags & kGrModeIsNative) ? 64 : 16;
        nNumber = 1;
        if (config_get_value(m_pDisplayDevice->GetCreateFlags() & 1 ? "d3d_disp_hw2d_cachesize_editor" : "d3d_disp_hw2d_cachesize",
                             CONFIG_INT_TYPE, (config_valptr) &nSizeMB, &nNumber) && nNumber > 0)
        {
            if (nSizeMB < 8)
                nSizeMB = 8;
            else if (nSizeMB >= 0x800)
                nSizeMB = 0x7ff;
        }
        m_n2DCacheSize = nSizeMB << 20;
    }

    if (!m_bPersistent)
    {
        if (m_p2DCache)
        {
            delete m_p2DCache;
            m_p2DCache = NULL;
        }
        ReleaseBlitResources();
        SafeRelease(m_pMSAASurface);
        SafeRelease(m_pSecondaryTexture);
        SafeRelease(m_pSecondarySurface);
        SafeRelease(m_pDevice);
        m_fState &= ~(kD3DHasDepth | kD3DHasStencil);
        g_hD3DMonitor = NULL;
    }

    m_bStarted = FALSE;
    m_nFrameCapUs = 0;
    float fFrameCap;
    nNumber = 1;
    if (config_get_value("framerate_cap", CONFIG_FLOAT_TYPE, (config_valptr) &fFrameCap, &nNumber) && nNumber > 0 && fFrameCap >= 1.0f)
    {
        m_nFrameCapUs = (DWORD)(1000000.0f / fFrameCap);
        if (m_nFrameCapUs)
        {
            if (!m_pGameShell)
                m_pGameShell = AppGetObj(IGameShell);
            DWORD now = timeGetTime();
            m_LastFrameUs = (unsigned __int64) now * 1000 - m_nFrameCapUs;
            LogMsg(_LogFmt("sw framerate cap requested (%d us)", m_nFrameCapUs));
        }
    }

    m_bFilterScale2D = !config_is_defined("d3d_disp_force_filter_scale2d");
    pReturnModeInfo->w = modeInfo.w;
    pReturnModeInfo->h = modeInfo.h;

    bWindowed = !(m_pProvider->m_WindowMode & DDSCL_FULLSCREEN) && !(flags & kDispFullScreen);
    int nAdapter = 0;
    if (bWindowed)
    {
        nNumber = 1;
        config_get_value("d3d_disp_adapter", CONFIG_INT_TYPE, (config_valptr) &nAdapter, &nNumber);
        if (nAdapter < 2)
            nAdapter = 1;
        else
        {
            if (nAdapter > 3)
                nAdapter = 3;
            nAdapter--;
        }
    }

    LogMsg(_LogFmt("D3DProvider StartMode: smode %dx%dx%d ; mode %dx%dx%d (%x %x %d %dD) ; 2d_surf_mode: %d",
                   m_ModeInfo.w, m_ModeInfo.h, m_ModeInfo.bitDepth, modeInfo.w, modeInfo.h, modeInfo.bitDepth,
                   m_pDisplayDevice->GetCreateFlags(), flags, m_pDevice != NULL, bGame + 2, m_n2DSurfMode));

    HWND hwnd = m_pProvider->GetMainWnd();
    UINT nD3DAdapter = m_pProvider->m_nAdapter;
    BOOL bPresentWindowed = (!bGame) != !(m_fState & kD3DPresentWindowed);
    DWORD brushBuildType = bGame ? (g_nD3DVSync & 1) : (g_nD3DVSync & 2);

    D3DPRESENT_PARAMETERS pp;
    memset(&pp, 0, sizeof(pp));
    pp.BackBufferWidth = m_ModeInfo.w;
    pp.BackBufferHeight = m_ModeInfo.h;
    pp.Windowed = (flags & kDispWindowed) != 0;
    if (m_ModeInfo.bitDepth == 16)
        pp.BackBufferFormat = D3DFMT_R5G6B5;
    else if (m_ModeInfo.bitDepth == 24)
        pp.BackBufferFormat = D3DFMT_R8G8B8;
    else
        pp.BackBufferFormat = (m_ModeInfo.bitDepth == 32) ? D3DFMT_X8R8G8B8 : D3DFMT_UNKNOWN;

    if (!pp.BackBufferFormat)
    {
        LogIncIndent();
        LogMsg1("failed, unsupported backbuffer depth (%d)", m_ModeInfo.bitDepth);
        LogDecIndent();
        return FALSE;
    }
    if (pp.BackBufferFormat == D3DFMT_X8R8G8B8 && !(m_fState & kD3DNoRGB10) && CanUseRGB10(pp.Windowed))
        pp.BackBufferFormat = D3DFMT_A2R10G10B10;

    pp.MultiSampleType = FindMultiSampleType(pp.BackBufferFormat, pp.Windowed);
    pp.BackBufferCount = 1;
    pp.hDeviceWindow = hwnd;
    if (!pp.Windowed)
        pp.FullScreen_RefreshRateInHz = FindRefreshRate(pp.BackBufferWidth, pp.BackBufferHeight, pp.BackBufferFormat);
    pp.PresentationInterval = brushBuildType ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
    if (brushBuildType && !pp.Windowed)
    {
        switch ((g_nD3DVSync >> 2) & 3)
        {
            case 1:
                pp.PresentationInterval = D3DPRESENT_INTERVAL_TWO;
                break;
            case 2:
                pp.PresentationInterval = D3DPRESENT_INTERVAL_THREE;
                break;
            case 3:
                pp.PresentationInterval = D3DPRESENT_INTERVAL_FOUR;
                break;
        }
    }
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;

    D3DFORMAT depthFmt = D3DFMT_UNKNOWN;
    if (bGame || m_bPersistent)
    {
        depthFmt = FindDepthStencilFormat(pp.BackBufferFormat);
        if (!depthFmt)
            return FALSE;
        pp.Flags |= D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
        pp.EnableAutoDepthStencil = TRUE;
        pp.AutoDepthStencilFormat = depthFmt;
    }

    HRESULT hr;
    if (m_pDevice)
    {
        if (memcmp(&m_PresentParams, &pp, sizeof(pp)) == 0)
            goto device_ready;
        bResetDevice = TRUE;
        if (m_p2DCache)
        {
            delete m_p2DCache;
            m_p2DCache = NULL;
        }
        if (m_pLost2DCache)
        {
            delete m_pLost2DCache;
            m_pLost2DCache = NULL;
        }
        ReleaseBlitResources();
        SafeRelease(m_pMSAASurface);
        SafeRelease(m_pSecondaryTexture);
        m_fState &= ~(kD3DHasDepth | kD3DHasStencil);
        m_PresentParams = pp;
        hr = m_pDevice->Reset(&pp);
        LogD3DResult("Reset", hr, &pp);
        if (FAILED(hr))
            return FALSE;
        goto device_ready;
    }

    bResetDevice = TRUE;
    g_hD3DMonitor = m_pProvider->m_pD3D->GetAdapterMonitor(nD3DAdapter);
    if (pp.Windowed)
        D3DFitWindowToMonitor(hwnd, pp.BackBufferHeight);

    g_fMaxZ = 1.0f;
    m_PresentParams = pp;
    hr = m_pProvider->m_pD3D->CreateDevice(nD3DAdapter, D3DDEVTYPE_HAL, hwnd,
                                           D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
                                           &pp, &m_pDevice);
    LogD3DResult("CreateDevice", hr, &pp);
    while (FAILED(hr))
    {
        pp = m_PresentParams;
        hr = m_pProvider->m_pD3D->CreateDevice(nD3DAdapter, D3DDEVTYPE_HAL, hwnd,
                                               D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
                                               &pp, &m_pDevice);
        LogD3DResult("CreateDevice (SW VP)", hr, &pp);
        if (SUCCEEDED(hr))
        {
            g_fMaxZ = 0.9999999f;
            break;
        }
        if (m_PresentParams.MultiSampleType == D3DMULTISAMPLE_2_SAMPLES)
        {
            m_PresentParams.MultiSampleType = D3DMULTISAMPLE_NONMASKABLE;
            m_PresentParams.BackBufferCount = 0;
        }
        else
        {
            if (m_PresentParams.BackBufferFormat != D3DFMT_A2R10G10B10)
            {
                g_hD3DMonitor = NULL;
                return FALSE;
            }
            m_PresentParams.BackBufferFormat = D3DFMT_X8R8G8B8;
            if (m_PresentParams.Windowed == FALSE)
                m_PresentParams.FullScreen_RefreshRateInHz =
                    FindRefreshRate(m_PresentParams.BackBufferWidth, m_PresentParams.BackBufferHeight, D3DFMT_X8R8G8B8);
            m_fState |= kD3DNoRGB10;
        }
        pp = m_PresentParams;
        hr = m_pProvider->m_pD3D->CreateDevice(nD3DAdapter, D3DDEVTYPE_HAL, hwnd,
                                               D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
                                               &pp, &m_pDevice);
        LogD3DResult("CreateDevice", hr, &pp);
    }

device_ready:
    if (depthFmt)
    {
        m_fState |= kD3DHasDepth;
        if (depthFmt == D3DFMT_D24S8 || depthFmt == D3DFMT_D24FS8 || depthFmt == D3DFMT_D24X4S4)
            m_fState |= kD3DHasStencil;
    }

    IDirect3DSurface9 * pBackBuffer;
    hr = m_pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    if (hr != D3D_OK)
    {
        LogIncIndent();
        LogMsg(_LogFmt("%s (%s)", "StartMode: GetBackBuffer primary", WhatD3DError(hr)));
        LogDecIndent();
        return FALSE;
    }
    hr = pBackBuffer->GetDesc(&m_BackBufferDesc);
    pBackBuffer->Release();
    if (hr != D3D_OK)
        return FALSE;

    if (m_n2DSurfMode == kD3D2DRenderTarget && (m_ModeInfo.flags & kGrModeIsNative) &&
        m_BackBufferDesc.MultiSampleType && config_is_nonzero("d3d_disp_hw2d_msaa"))
    {
        m_n2DSurfMode = kD3D2DRenderTargetMSAA;
        if (m_p2DCache)
        {
            delete m_p2DCache;
            m_p2DCache = NULL;
        }
        SafeRelease(m_pMSAASurface);
        SafeRelease(m_pSecondaryTexture);
        SafeRelease(m_pSecondarySurface);
    }

    {
        int oldTexWidth = m_nTexWidth;
        int oldTexHeight = m_nTexHeight;
        D3DFORMAT fmt;

        switch (modeInfo.bitDepth)
        {
            case 16:
                fmt = D3DFMT_R5G6B5;
                break;
            case 24:
                fmt = D3DFMT_R8G8B8;
                break;
            case 32:
                fmt = D3DFMT_X8R8G8B8;
                break;
            default:
                LogIncIndent();
                LogMsg1("failed, unsupported bit depth (%d)", modeInfo.bitDepth);
                LogDecIndent();
                return FALSE;
        }
        if (bWindowed || m_bPersistent)
        {
            if (modeInfo.bitDepth != 32)
            {
                LogIncIndent();
                LogMsg1("failed, unsupported bit depth (%d) for current mode", modeInfo.bitDepth);
                LogDecIndent();
                return FALSE;
            }
            fmt = D3DFMT_A8R8G8B8;
        }

        int w = modeInfo.w;
        int h = modeInfo.h;
        m_nWidth = w;
        m_nHeight = h;
        if (!(m_fState & kD3DNonPow2))
        {
            w = NextPow2(w);
            h = NextPow2(h);
            if (w > 0x1000 || h > 0x1000)
                return FALSE;
        }
        m_nTexWidth = w;
        m_nTexHeight = h;

        if (m_n2DSurfMode == kD3D2DDynamicTexture || m_n2DSurfMode == kD3D2DRenderTarget)
        {
            SafeRelease(m_pSecondarySurface);
            if (!m_pSecondaryTexture || m_SecondaryDesc.Format != fmt || w != oldTexWidth || h != oldTexHeight)
            {
                if (m_p2DCache)
                {
                    delete m_p2DCache;
                    m_p2DCache = NULL;
                }
                SafeRelease(m_pMSAASurface);
                SafeRelease(m_pSecondaryTexture);
            }
        }
        else if (!m_pSecondarySurface || m_SecondaryDesc.Format != fmt || w != oldTexWidth || h != oldTexHeight)
        {
            if (m_p2DCache)
            {
                delete m_p2DCache;
                m_p2DCache = NULL;
            }
            SafeRelease(m_pMSAASurface);
            SafeRelease(m_pSecondaryTexture);
            SafeRelease(m_pSecondarySurface);
            hr = m_pDevice->CreateTexture(w, h, 1, 0, fmt, (D3DPOOL)((m_n2DSurfMode != kD3D2DSysmem) + D3DPOOL_MANAGED),
                                          &m_pSecondarySurface, NULL);
            if (hr != D3D_OK)
            {
                LogIncIndent();
                LogMsg(_LogFmt("%s (%s)", "failed to create secondary surf", WhatD3DError(hr)));
                LogDecIndent();
                return FALSE;
            }
        }
        m_SecondaryDesc.Format = fmt;
    }

    m_bStarted = TRUE;
    m_nLockDepth = 0;
    if (!m_pSecondaryTexture && m_n2DSurfMode != kD3D2DSysmem && (hr = CreateSecondaryTexture()) != D3D_OK)
    {
        LogIncIndent();
        LogMsg(_LogFmt("%s (%s)", "failed to create secondary hw surf", WhatD3DError(hr)));
        LogDecIndent();
        return FALSE;
    }

    {
        IDirect3DTexture9 * pTex = m_pSecondarySurface ? m_pSecondarySurface : m_pSecondaryTexture;
        if (m_pMSAASurface)
        {
            if (m_pMSAASurface->GetDesc(&m_SecondaryDesc) != D3D_OK)
                return FALSE;
        }
        else
        {
            if (!pTex)
                return FALSE;
            if (pTex->GetLevelDesc(0, &m_SecondaryDesc) != D3D_OK)
                return FALSE;
        }
        m_SecondaryDesc.Width = m_nWidth;
        m_SecondaryDesc.Height = m_nHeight;

        if (bPresentWindowed)
            m_fState |= kD3DPresentWindowed;
        else
            m_fState &= ~kD3DPresentWindowed;

        D3DSURFACE_DESC * pDesc = pTex ? &m_SecondaryDesc : &m_BackBufferDesc;
        memset(&m_ModeInfoEx, 0, sizeof(m_ModeInfoEx));
        m_ModeInfoEx.mode = gr_mode_from_info(pDesc->Width, pDesc->Height, modeInfo.bitDepth);
        m_ModeInfoEx.width = pDesc->Width;
        m_ModeInfoEx.height = pDesc->Height;
        m_ModeInfoEx.depth = modeInfo.bitDepth;
        m_ModeInfoEx.flags = ((flags & kDispWindowed) | 8) << 6;
        D3DFormatToMasks(pDesc->Format, &m_ModeInfoEx.redMask, &m_ModeInfoEx.greenMask, &m_ModeInfoEx.blueMask);
    }

    ChooseAlphaTextureFormat();
    if (bResetDevice)
        CreateBlitResources();

    if ((m_n2DSurfMode == kD3D2DRenderTarget || m_n2DSurfMode == kD3D2DRenderTargetMSAA) && !m_p2DCache)
        m_p2DCache = new cD3DHW2DCache(this, m_n2DCacheSize);

    {
        DWORD clearFlags = D3DCLEAR_TARGET;
        if (m_PresentParams.EnableAutoDepthStencil)
            clearFlags = (m_fState & kD3DHasStencil) | D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
        hr = m_pDevice->Clear(0, NULL, clearFlags, 0, 1.0f, 0);
        if (hr != D3D_OK)
        {
            m_pDevice->Present(NULL, NULL, NULL, NULL);
            if (m_PresentParams.MultiSampleType == D3DMULTISAMPLE_2_SAMPLES)
                m_pDevice->Clear(0, NULL, clearFlags, 0, 1.0f, 0);
        }
    }
    ClearSecondary();
    pReturnModeInfo->vbase = NULL;

    if (m_ModeInfo.flags & kGrModeIsNative)
        return TRUE;

    switch (GetPrivateProfileInt("WinShell", "ClipCursor", 1, ".\\lg.ini"))
    {
        case 1:
            if (m_PresentParams.Windowed)
                break;
            g_bD3DClipCursor = TRUE;
            break;
        case 2:
            if (!m_PresentParams.Windowed && bWindowed)
                g_bD3DClipCursor = TRUE;
            break;
        case 3:
            g_bD3DClipCursor = TRUE;
            break;
        case 4:
            if (bWindowed)
                g_bD3DClipCursor = TRUE;
            break;
    }
    if (g_bD3DClipCursor)
        D3DClipCursorToWindow(m_pProvider->GetMainWnd());
    return TRUE;
}

///////////////////////////////////////

BOOL cD3DModeOps::RestoreSurfaces()
{
    return CheckDevice();
}

BOOL cD3DModeOps::CheckDevice()
{
    if (!m_pDevice)
        return FALSE;

    HRESULT hr = m_pDevice->TestCooperativeLevel();
    if (hr == D3D_OK)
        return TRUE;

    g_bD3DDeviceRestored = FALSE;
    if (!(m_fState & kD3DDeviceLost))
    {
        IDirect3DTexture9 * pTex = m_pSecondarySurface ? m_pSecondarySurface : m_pSecondaryTexture;
        if (m_LockedRect.pBits && pTex && !m_pDisplayDevice->IsLocked())
        {
            pTex->UnlockRect(0);
            m_LockedRect.pBits = NULL;
        }
        if (m_p2DCache)
        {
            delete m_p2DCache;
            m_p2DCache = NULL;
            m_pLost2DCache = new cD3DLost2DCache;
        }
        ReleaseBlitResources();
        SafeRelease(m_pMSAASurface);
        SafeRelease(m_pSecondaryTexture);
        D8_DeviceReset(FALSE);
        m_fState &= ~(kD3DDirty | kD3DNeedsClear | kD3D2DDirty);
        if (!m_pSecondarySurface)
            m_fState &= ~kD3DDirty;
        ClipCursor(NULL);
        m_fState |= kD3DDeviceLost;
    }
    m_fState |= kD3DDeviceLost;

    if (hr == D3DERR_DEVICENOTRESET)
    {
        D3DPRESENT_PARAMETERS pp = m_PresentParams;
        if (SUCCEEDED(m_pDevice->Reset(&pp)))
        {
            g_hD3DMonitor = m_pProvider->m_pD3D->GetAdapterMonitor(m_pProvider->m_nAdapter);
            if (!m_pSecondaryTexture)
            {
                if (CreateSecondaryTexture())
                    return FALSE;
                if (!m_pSecondarySurface)
                    m_fState |= kD3DNeedsClear;
            }
            m_fState &= ~kD3DDeviceLost;
            CreateBlitResources();
            if (m_pLost2DCache)
            {
                delete m_pLost2DCache;
                m_pLost2DCache = NULL;
            }
            if ((m_n2DSurfMode == kD3D2DRenderTarget || m_n2DSurfMode == kD3D2DRenderTargetMSAA) && !m_p2DCache)
                m_p2DCache = new cD3DHW2DCache(this, m_n2DCacheSize);
            D8_DeviceReset(TRUE);
            D3DClipCursorToWindow(m_pProvider->GetMainWnd());
            g_bD3DDeviceRestored = TRUE;
            return TRUE;
        }
    }
    return FALSE;
}

///////////////////////////////////////

BOOL cD3DModeOps::OnTaskSwitch(BOOL bActive)
{
    if (!bActive)
    {
        if (g_bD3DActive)
        {
            g_bD3DActive = FALSE;
            return FALSE;
        }
        return FALSE;
    }
    if (g_bD3DActive)
        return FALSE;
    g_bD3DActive = TRUE;
    RestoreSurfaces();
    return TRUE;
}

void cD3DModeOps::ThrottleFrame()
{
    if (!m_nFrameCapUs)
        return;

    unsigned __int64 now = (unsigned __int64) timeGetTime() * 1000;
    DWORD elapsed;
    if (now < m_LastFrameUs)
    {
        elapsed = (DWORD)(now - m_LastFrameUs) - 999;
    }
    else
    {
        elapsed = (DWORD)(now - m_LastFrameUs);
    }
    if (elapsed + 1000 <= m_nFrameCapUs)
    {
        Sleep((m_nFrameCapUs - elapsed) / 1000);
        now = (unsigned __int64) timeGetTime() * 1000;
    }
    m_LastFrameUs = now;
}

HRESULT cD3DModeOps::PollGPUQueries(BOOL bWait)
{
    HRESULT hr = m_nGPUQueries;

    if (!m_nGPUQueries)
        return hr;
    if (!bWait)
    {
        hr = m_apGPUQueries[m_nGPUQuery]->Issue(D3DISSUE_END);
        if (SUCCEEDED(hr))
        {
            int next = m_nGPUQuery + 1;
            m_nGPUQueriesPending++;
            m_nGPUQuery = next % m_nGPUQueries;
            return next / m_nGPUQueries;
        }
    }
    else if (m_nGPUQueriesPending == m_nGPUQueries)
    {
        BOOL bData = FALSE;
        DWORD start = timeGetTime();
        hr = m_apGPUQueries[m_nGPUQuery]->GetData(&bData, sizeof(bData), D3DGETDATA_FLUSH);
        while (hr == S_FALSE)
        {
            Sleep(0);
            hr = timeGetTime() - start;
            if ((DWORD)hr > 65)
                break;
            hr = m_apGPUQueries[m_nGPUQuery]->GetData(&bData, sizeof(bData), D3DGETDATA_FLUSH);
        }
        m_nGPUQueriesPending--;
    }
    return hr;
}

void cD3DModeOps::Present()
{
    m_pDevice->Present(NULL, NULL, NULL, NULL);
    PollGPUQueries(FALSE);
    PollGPUQueries(TRUE);
    if ((m_fState & kD3DNewFrame) && (m_pDisplayDevice->GetCreateFlags() & kDispFullScreen) &&
        ((m_n2DSurfMode != kD3D2DRenderTarget && m_n2DSurfMode != kD3D2DRenderTargetMSAA) || (m_fState & kD3DDirty)))
        m_fState |= kD3DNeedsClear;
    if ((m_fState & kD3DNeedsClear) || (m_p2DCache && (m_ModeInfo.flags & kGrModeIsNative)))
        m_fState &= ~kD3DDirty;
    m_fState &= ~kD3DNewFrame;
    g_bD3DPaintPending = FALSE;
}

BOOL cD3DModeOps::DoPageFlip()
{
    DWORD createFlags = m_pDisplayDevice->GetCreateFlags();

    if ((m_pSecondarySurface || m_pSecondaryTexture) && m_nLockDepth)
    {
        m_nLockDepth = 0;
        DoFlush();
        m_nLockDepth = 1;
    }
    else if (m_p2DCache && m_p2DCache->Flush())
        m_fState |= kD3DDirty;

    if (m_PresentParams.MultiSampleType == D3DMULTISAMPLE_2_SAMPLES)
    {
        RestoreSurfaces();
        if (!(m_fState & kD3DDeviceLost))
        {
            if (createFlags & kDispFullScreen)
                ThrottleFrame();
            if (!m_p2DCache || !(m_ModeInfo.flags & kGrModeIsNative) || (m_fState & kD3DDirty) || g_bD3DPaintPending)
            {
                int iMutex = m_pDisplayDevice->BreakLock();
                Present();
                m_pDisplayDevice->RestoreLock(iMutex);
            }
        }
    }
    return FALSE;
}

void cD3DModeOps::DoFlush()
{
    if ((!m_pSecondarySurface && !m_pSecondaryTexture) || m_nLockDepth)
        return;

    RestoreSurfaces();
    if (m_fState & kD3DDeviceLost)
        return;

    if (m_p2DCache && m_p2DCache->Flush())
        m_fState |= kD3DDirty;

    BOOL bBlit = (m_fState & kD3DDirty) || !(m_fState & kD3DNewFrame);
    if (m_p2DCache && (m_ModeInfo.flags & kGrModeIsNative) && !(m_fState & kD3DDirty) && !g_bD3DPaintPending)
        return;

    if (m_LockedRect.pBits && bBlit)
        UnlockSurface();
    int iMutex = m_pDisplayDevice->BreakLock();
    if (bBlit)
        BlitToBackBuffer(FALSE);
    if (m_PresentParams.MultiSampleType != D3DMULTISAMPLE_2_SAMPLES)
    {
        if (m_pDisplayDevice->GetCreateFlags() & kDispFullScreen)
            ThrottleFrame();
        Present();
    }
    m_pDisplayDevice->RestoreLock(iMutex);
}

///////////////////////////////////////

BOOL cD3DModeOps::DoLock(sGrModeCap * pModeCap)
{
    pModeCap->vbase = NULL;
    pModeCap->vbase2 = NULL;
    RestoreSurfaces();

    IDirect3DTexture9 * pTex = m_pSecondarySurface;
    if ((m_fState & kD3DDeviceLost) || (!pTex && !(pTex = m_pSecondaryTexture)))
        return FALSE;

    if (m_LockedRect.pBits)
    {
        pModeCap->rowBytes = m_LockedRect.Pitch;
        pModeCap->vbase = (uchar *) m_LockedRect.pBits;
        if (m_n2DSurfMode != kD3D2DRenderTarget && m_n2DSurfMode != kD3D2DRenderTargetMSAA)
            m_fState |= kD3DDirty;
        return TRUE;
    }

    DWORD lockFlags = (m_n2DSurfMode == kD3D2DDynamicTexture && m_bPersistent) ? D3DLOCK_DISCARD : 0;
    if (m_n2DSurfMode == kD3D2DRenderTarget || m_n2DSurfMode == kD3D2DRenderTargetMSAA)
    {
        m_LockedRect.pBits = (void *) 0xffeeffee;
        m_LockedRect.Pitch = ((GetSignificantDepth() + 7) / 8) * m_nWidth;
    }
    else if (pTex->LockRect(0, &m_LockedRect, NULL, lockFlags | D3DLOCK_NOSYSLOCK) != D3D_OK)
    {
        if (g_bD3DLocked)
            g_bD3DLocked = FALSE;
        return FALSE;
    }
    g_bD3DLocked = TRUE;

    if (m_n2DSurfMode != kD3D2DRenderTarget && m_n2DSurfMode != kD3D2DRenderTargetMSAA)
        m_fState |= kD3DDirty;
    pModeCap->rowBytes = m_LockedRect.Pitch;
    pModeCap->vbase = (uchar *) m_LockedRect.pBits;

    if (m_fState & kD3DNeedsClear)
    {
        m_fState &= ~kD3DNeedsClear;
        if (m_n2DSurfMode == kD3D2DRenderTarget || m_n2DSurfMode == kD3D2DRenderTargetMSAA)
        {
            if (m_pMSAASurface)
                m_pDevice->ColorFill(m_pMSAASurface, NULL, 0);
            else
            {
                IDirect3DSurface9 * pSurf = NULL;
                if (SUCCEEDED(m_pSecondaryTexture->GetSurfaceLevel(0, &pSurf)))
                {
                    m_pDevice->ColorFill(pSurf, NULL, 0);
                    pSurf->Release();
                }
            }
        }
        else
        {
            int rowBytes = ((GetSignificantDepth() + 7) / 8) * m_nWidth;
            if (rowBytes)
            {
                if (pModeCap->rowBytes == (ulong) rowBytes)
                    ZeroAligned(pModeCap->vbase, m_nHeight * rowBytes);
                else
                {
                    uchar * p = pModeCap->vbase;
                    for (int y = 0; y < m_nHeight; y++)
                    {
                        memset(p, 0, rowBytes);
                        p += pModeCap->rowBytes;
                    }
                }
            }
        }
    }
    return pModeCap->vbase != NULL;
}

BOOL cD3DModeOps::UnlockSurface()
{
    IDirect3DTexture9 * pTex = m_pSecondarySurface;
    if (!pTex && !(pTex = m_pSecondaryTexture))
        return FALSE;

    if (m_LockedRect.pBits)
    {
        RestoreSurfaces();
        if (m_fState & kD3DDeviceLost)
            return FALSE;
        if (m_n2DSurfMode == kD3D2DRenderTarget || m_n2DSurfMode == kD3D2DRenderTargetMSAA)
            m_LockedRect.pBits = NULL;
        else
        {
            HRESULT hr = pTex->UnlockRect(0);
            m_LockedRect.pBits = NULL;
            if (hr != D3D_OK)
            {
#line 2802
                CriticalMsg1("Lock count of DirectDraw surfaces appears corrupted (%s)", WhatD3DError(hr));
                ReleaseSurfaces(TRUE);
                g_bD3DClipCursor = FALSE;
                ClipCursor(NULL);
                CriticalMsg("Lock count of DirectDraw surfaces appears corrupted");
            }
            if (m_pSecondaryTexture && m_pSecondarySurface && m_n2DSurfMode == kD3D2DSysmemTexture)
            {
                m_pDevice->UpdateTexture(m_pSecondarySurface, m_pSecondaryTexture);
                return TRUE;
            }
        }
    }
    return TRUE;
}

BOOL cD3DModeOps::DoUnlock()
{
    if (!m_pSecondarySurface && !m_pSecondaryTexture)
        return FALSE;
    if (m_p2DCache && (m_fState & kD3D2DDirty) && m_p2DCache->Flush())
        m_fState |= kD3DDirty;
    if (m_bPersistLocks)
        return TRUE;
    return UnlockSurface();
}

void cD3DModeOps::ClearSecondary()
{
    if (!m_pSecondarySurface && !m_pSecondaryTexture)
        return;

    m_fState &= ~kD3DNeedsClear;
    if (m_n2DSurfMode == kD3D2DRenderTarget)
    {
        IDirect3DSurface9 * pSurf = NULL;
        if (m_pSecondaryTexture && SUCCEEDED(m_pSecondaryTexture->GetSurfaceLevel(0, &pSurf)))
        {
            m_pDevice->ColorFill(pSurf, NULL, 0);
            pSurf->Release();
        }
    }
    else if (m_n2DSurfMode == kD3D2DRenderTargetMSAA)
    {
        if (m_pMSAASurface)
            m_pDevice->ColorFill(m_pMSAASurface, NULL, 0);
    }
    else
    {
        int rowBytes = ((GetSignificantDepth() + 7) / 8) * m_nWidth;
        sGrModeCap cap;
        if (rowBytes && DoLock(&cap) && cap.vbase)
        {
            if (cap.rowBytes == (ulong) rowBytes)
                ZeroAligned(cap.vbase, m_nHeight * rowBytes);
            else
            {
                uchar * p = cap.vbase;
                for (int y = 0; y < m_nHeight; y++)
                {
                    memset(p, 0, rowBytes);
                    p += cap.rowBytes;
                }
            }
            UnlockSurface();
        }
    }
}

///////////////////////////////////////

void cD3DModeOps::Blit2D(RECT * pDest, RECT * pSrc, RECT * pBars, int nBars)
{
    IDirect3DTexture9 * pTex = m_pSecondaryTexture ? m_pSecondaryTexture : m_pSecondarySurface;
    sD3DBlitVertex v[4];
    float u0, v0, u1, v1;
    int srcW, srcH;

    if (!pTex || !m_pBlitStateBlock || !m_pSavedStateBlock)
        return;
    if (m_LockedRect.pBits)
        UnlockSurface();
    if (m_p2DCache)
        m_p2DCache->Flush();

    if (m_pMSAASurface)
    {
        IDirect3DSurface9 * pSurf = NULL;
        if (FAILED(m_pSecondaryTexture->GetSurfaceLevel(0, &pSurf)))
            return;
        if (m_nTexWidth == m_nWidth && m_nTexHeight == m_nHeight)
            m_pDevice->StretchRect(m_pMSAASurface, NULL, pSurf, NULL, D3DTEXF_NONE);
        else
        {
            RECT rect = { 0, 0, m_nWidth, m_nHeight };
            m_pDevice->StretchRect(m_pMSAASurface, &rect, pSurf, &rect, D3DTEXF_NONE);
        }
        SafeRelease(pSurf);
    }

    m_pDevice->BeginScene();
    m_pSavedStateBlock->Capture();
    m_pBlitStateBlock->Apply();

    if (pSrc)
    {
        srcW = pSrc->right - pSrc->left;
        srcH = pSrc->bottom - pSrc->top;
    }
    else
    {
        srcW = m_nWidth;
        srcH = m_nHeight;
    }
    if ((pDest->right - pDest->left) % srcW == 0 && (pDest->bottom - pDest->top) % srcH == 0 &&
        (m_bFilterScale2D || (pDest->right - pDest->left == srcW && pDest->bottom - pDest->top == srcH)))
        m_pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);

    if (!(m_pDisplayDevice->GetCreateFlags() & kDispFullScreen))
    {
        m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
        m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
        m_pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    }
    if (m_pGammaShader && !(m_pDisplayDevice->GetCreateFlags() & kDispFullScreen) && m_Gamma != 1.0)
    {
        float c[4];
        c[0] = c[1] = c[2] = c[3] = (float) m_Gamma;
        m_pDevice->SetPixelShader(m_pGammaShader);
        m_pDevice->SetPixelShaderConstantF(0, c, 1);
    }

    u0 = 0.0f;
    v0 = 0.0f;
    u1 = 1.0f;
    v1 = 1.0f;
    if (pSrc)
    {
        u0 = (float) pSrc->left / (float) m_nTexWidth;
        v0 = (float) pSrc->top / (float) m_nTexHeight;
        u1 = (float) pSrc->right / (float) m_nTexWidth;
        v1 = (float) pSrc->bottom / (float) m_nTexHeight;
    }
    else
    {
        if (m_nTexWidth != m_nWidth)
            u1 = (float) m_nWidth / (float) m_nTexWidth;
        if (m_nTexHeight != m_nHeight)
            v1 = (float) m_nHeight / (float) m_nTexHeight;
    }

    D3DCOLOR color = 0xffffffff;
    if (m_pDisplayDevice->GetCreateFlags() & kDispFullScreen)
        color = ((int)(0.01f * 255.0) << 24) | 0xffffff;

    float x0 = (float) pDest->left - m_fTexelOffsetX;
    float y0 = (float) pDest->top - m_fTexelOffsetY;
    float x1 = (float) pDest->right - m_fTexelOffsetX;
    float y1 = (float) pDest->bottom - m_fTexelOffsetY;
    v[0].x = x0; v[0].y = y0; v[0].z = 1.0f; v[0].rhw = 1.0f; v[0].color = color; v[0].u = u0; v[0].v = v0;
    v[1].x = x1; v[1].y = y0; v[1].z = 1.0f; v[1].rhw = 1.0f; v[1].color = color; v[1].u = u1; v[1].v = v0;
    v[2].x = x1; v[2].y = y1; v[2].z = 1.0f; v[2].rhw = 1.0f; v[2].color = color; v[2].u = u1; v[2].v = v1;
    v[3].x = x0; v[3].y = y1; v[3].z = 1.0f; v[3].rhw = 1.0f; v[3].color = color; v[3].u = u0; v[3].v = v1;

    m_pDevice->SetTexture(0, pTex);
    m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(sD3DBlitVertex));
    m_pDevice->SetTexture(0, NULL);

    if (pBars && nBars > 0)
    {
        m_pDevice->SetPixelShader(NULL);
        m_pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
        m_pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
        m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        m_pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        for (int i = 0; i < 4; i++)
            v[i].color = 0xff000000;
        for (; nBars; nBars--, pBars++)
        {
            v[0].x = v[3].x = (float) pBars->left - m_fTexelOffsetX;
            v[0].y = v[1].y = (float) pBars->top - m_fTexelOffsetY;
            v[1].x = v[2].x = (float) pBars->right - m_fTexelOffsetX;
            v[2].y = v[3].y = (float) pBars->bottom - m_fTexelOffsetY;
            m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(sD3DBlitVertex));
        }
    }

    m_pSavedStateBlock->Apply();
    m_pDevice->EndScene();
}

void cD3DModeOps::BlitToBackBuffer(BOOL)
{
    BOOL bGame = (m_pDisplayDevice->GetCreateFlags() & kDispFullScreen) != 0;
    if (bGame && (m_fState & kD3DNeedsClear))
        return;

    int bbW = m_BackBufferDesc.Width;
    int bbH = m_BackBufferDesc.Height;
    RECT dest = { 0, 0, bbW, bbH };
    RECT src;
    RECT bars[4];
    RECT * pSrc = NULL;
    int nBars = 0;

    if (bGame || (m_ModeInfo.w == m_ModeInfoEx.width && (DWORD) m_ModeInfo.h == m_ModeInfoEx.height))
    {
        Blit2D(&dest, NULL, NULL, 0);
        return;
    }

    if (g_rcUIScaleView.left && g_rcUIScaleView.top)
    {
        src.left = g_rcUIScaleView.left;
        src.top = g_rcUIScaleView.top;
        src.right = g_rcUIScaleView.left + g_rcUIScaleView.right;
        src.bottom = g_rcUIScaleView.top + g_rcUIScaleView.bottom;
        pSrc = &src;
        int h = (g_rcUIScaleView.bottom * bbW) / g_rcUIScaleView.right;
        if (h < bbH)
        {
            dest.top = (bbH - h) / 2;
            dest.bottom = h + dest.top;
            bars[0].left = 0; bars[0].top = 0; bars[0].right = bbW; bars[0].bottom = dest.top;
            bars[1].left = 0; bars[1].top = dest.bottom; bars[1].right = bbW; bars[1].bottom = bbH;
        }
        else
        {
            int w = (g_rcUIScaleView.right * bbH) / g_rcUIScaleView.bottom;
            dest.left = (bbW - w) / 2;
            dest.right = w + dest.left;
            bars[0].left = 0; bars[0].top = 0; bars[0].right = dest.left; bars[0].bottom = bbH;
            bars[1].left = dest.right; bars[1].top = 0; bars[1].right = bbW; bars[1].bottom = bbH;
        }
        nBars = 2;
    }
    else if (m_bUIScale == 1 && !g_rcUIScaleView.left)
    {
        int scale = (bbW * 3 < bbH * 4) ? bbW / 640 : bbH / 480;
        if (!scale)
            scale = 1;
        dest.left = (bbW - scale * 640) / 2;
        dest.right = scale * 640 + dest.left;
        dest.top = (bbH - scale * 480) / 2;
        dest.bottom = scale * 480 + dest.top;
        bars[0].left = 0; bars[0].top = 0; bars[0].right = bbW; bars[0].bottom = dest.top;
        bars[1].left = 0; bars[1].top = dest.bottom; bars[1].right = bbW; bars[1].bottom = bbH;
        bars[2].left = 0; bars[2].top = dest.top; bars[2].right = dest.left; bars[2].bottom = dest.bottom;
        bars[3].left = dest.right; bars[3].top = dest.top; bars[3].right = bbW; bars[3].bottom = dest.bottom;
        nBars = 4;
    }
    else
    {
        if (m_ModeInfo.h * 4 == m_ModeInfo.w * 3)
        {
            Blit2D(&dest, NULL, NULL, 0);
            return;
        }
        if (bbW * 3 < bbH * 4)
        {
            int h = (bbW * 3) / 4;
            dest.top = (bbH - h) / 2;
            dest.bottom = h + dest.top;
            bars[0].left = 0; bars[0].top = 0; bars[0].right = bbW; bars[0].bottom = dest.top;
            bars[1].left = 0; bars[1].top = dest.bottom; bars[1].right = bbW; bars[1].bottom = bbH;
        }
        else
        {
            int w = (bbH * 4) / 3;
            dest.left = (bbW - w) / 2;
            dest.right = w + dest.left;
            bars[0].left = 0; bars[0].top = 0; bars[0].right = dest.left; bars[0].bottom = bbH;
            bars[1].left = dest.right; bars[1].top = 0; bars[1].right = bbW; bars[1].bottom = bbH;
        }
        nBars = 2;
    }
    Blit2D(&dest, pSrc, bars, nBars);
}

///////////////////////////////////////

BOOL cD3DModeOps::DoSetGamma(double gamma)
{
    m_Gamma = gamma;
    if (m_pDevice && m_pGammaShader && !(m_pDisplayDevice->GetCreateFlags() & kDispFullScreen) && gamma != 1.0)
        return cD3DModeOpsBase::DoSetGamma(1.0);
    return cD3DModeOpsBase::DoSetGamma(gamma);
}

void cD3DModeOps::SetFrame(int nFrame)
{
    if (m_p2DCache && m_p2DCache->Flush())
        m_fState |= kD3DDirty;
    m_fState |= kD3DNewFrame | kD3D2DDirty;
    if (g_nD3DFrame == nFrame)
    {
        if (!(m_fState & kD3DDirty))
            return;
        RestoreSurfaces();
        if (m_fState & kD3DDeviceLost)
            return;
        if (m_LockedRect.pBits)
            UnlockSurface();
        m_pDisplayDevice->BreakLock();
        BlitToBackBuffer(FALSE);
    }
    else
    {
        g_nD3DFrame = nFrame;
        if (!(m_fState & kD3DDirty))
            return;
    }
    m_fState = (m_fState & ~kD3DDirty) | kD3DNeedsClear;
}

void cD3DModeOps::Clear2DDirty()
{
    m_fState &= ~kD3D2DDirty;
}

void cD3DModeOps::Flush2DCache()
{
    if (m_p2DCache)
        m_p2DCache->FlushAll();
}

///////////////////////////////////////

BOOL D3DFitWindowToMonitor(HWND hwnd, int height)
{
    RECT window, client;
    MONITORINFO mi;
    int screenHeight;

    GetWindowRect(hwnd, &window);
    GetClientRect(hwnd, &client);
    if (window.right - window.left == client.right - client.left &&
        window.bottom - window.top == client.bottom - client.top)
        return FALSE;

    memset(&mi, 0, sizeof(mi));
    mi.cbSize = sizeof(mi);
    if (g_hD3DMonitor && GetMonitorInfo(g_hD3DMonitor, &mi))
        screenHeight = mi.rcMonitor.bottom - mi.rcMonitor.top;
    else
    {
        screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        if (!screenHeight)
            screenHeight = GetSystemMetrics(SM_CYSCREEN);
    }
    if ((window.bottom - window.top) + height - (client.bottom - client.top) <= screenHeight)
        return FALSE;
    if (GetWindowLong(hwnd, GWL_STYLE) != (LONG)(WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS))
    {
        SetWindowLong(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS);
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    return TRUE;
}

void D3DCenterOnMonitor(POINT * pPt, int w, int h)
{
    MONITORINFO mi;

    memset(&mi, 0, sizeof(mi));
    mi.cbSize = sizeof(mi);
    if (g_hD3DMonitor && GetMonitorInfo(g_hD3DMonitor, &mi))
    {
        pPt->x = ((mi.rcMonitor.right - mi.rcMonitor.left) - w) / 2 + mi.rcMonitor.left;
        pPt->y = ((mi.rcMonitor.bottom - mi.rcMonitor.top) - h) / 2 + mi.rcMonitor.top;
        return;
    }
    pPt->x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    pPt->y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
}

///////////////////////////////////////

BOOL cD3DModeOps::DoStatHTrace()
{
    if (m_PresentParams.SwapEffect == D3DSWAPEFFECT_FLIP)
        DoPageFlip();
    return FALSE;
}

BOOL cD3DModeOps::DoStatVTrace()
{
    D3DRASTER_STATUS status;
    return SUCCEEDED(m_pDevice->GetRasterStatus(0, &status));
}

void cD3DModeOps::DoFlushRect(int, int, int, int)
{
    DoFlush();
}

BOOL cD3DModeOps::DoSetFlip(int bFlip)
{
    int nLocks = m_pDisplayDevice->BreakLock();
    m_nLockDepth = bFlip;
    m_pDisplayDevice->RestoreLock(nLocks);
    return bFlip;
}

///////////////////////////////////////

static grs_canvas g_D3DDesktopCanvas;
static grs_bitmap g_D3DDesktopBitmap;
static uchar g_D3DSavedModeDepth;

static uchar D3DLog2(int n)
{
    uchar log = 0;
    for (int i = 2; i <= n; i *= 2)
        log++;
    return log;
}

void D3DDestroyDesktopCanvas()
{
    gr_close_canvas(&g_D3DDesktopCanvas);
    if (grd_canvas == &g_D3DDesktopCanvas)
        grd_canvas = NULL;
    memset(&g_D3DDesktopCanvas, 0, sizeof(g_D3DDesktopCanvas));
    if (gdd_active)
    {
        if ((g_D3DDesktopBitmap.flags & BMF_RELOCATABLE) && g_pDev2dDisplayDevice)
            g_pDev2dDisplayDevice->SetMonitor((tGrDispBits **) &g_D3DDesktopBitmap, NULL, kGrDispPtrMonitorOff);
        if ((g_D3DDesktopBitmap.exflags & 4) && g_pHW2D)
            g_pHW2D->Invalidate(&g_D3DDesktopBitmap);
    }
    memset(&g_D3DDesktopBitmap, 0, sizeof(g_D3DDesktopBitmap));
    g_pD3DDesktopSurface->UnlockRect();
    SafeRelease(g_pD3DDesktopSurface);
}

void D3DReleaseDesktopCanvas()
{
    if (g_pD3DModeOps && g_pD3DDesktopSurface)
    {
        grd_mode_info[grd_mode].bitDepth = g_D3DSavedModeDepth;
        gr_pop_canvas();
        D3DDestroyDesktopCanvas();
    }
}

void D3DCreateDesktopCanvas()
{
    D3DDISPLAYMODE mode;
    D3DDEVICE_CREATION_PARAMETERS params;
    D3DLOCKED_RECT lr;
    RECT rect;

    if (!g_pD3DModeOps || g_pD3DDesktopSurface)
        return;
    IDirect3D9 * pD3D = g_pD3DModeOps->m_pProvider->GetD3D();
    IDirect3DDevice9 * pDevice = g_pD3DModeOps->m_pDevice;
    if (!pD3D || !pDevice)
        return;
    if (FAILED(pDevice->GetDisplayMode(0, &mode)) || FAILED(pDevice->GetCreationParameters(&params)) || !mode.Width)
        return;
    if (FAILED(pDevice->CreateOffscreenPlainSurface(mode.Width, mode.Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH,
                                                    &g_pD3DDesktopSurface, NULL)))
        return;
    if (FAILED(pDevice->GetFrontBufferData(0, g_pD3DDesktopSurface)))
        goto fail;

    GetClientRect(params.hFocusWindow, &rect);
    if (g_pD3DModeOps->m_PresentParams.Windowed)
    {
        POINT pt = { 0, 0 };
        MONITORINFO mi;
        ClientToScreen(params.hFocusWindow, &pt);
        OffsetRect(&rect, pt.x, pt.y);
        memset(&mi, 0, sizeof(mi));
        mi.cbSize = sizeof(mi);
        HMONITOR hMonitor = pD3D->GetAdapterMonitor(params.AdapterOrdinal);
        if (hMonitor && GetMonitorInfo(hMonitor, &mi))
        {
            if (rect.left < mi.rcMonitor.left || rect.right > mi.rcMonitor.right ||
                rect.top < mi.rcMonitor.top || rect.bottom > mi.rcMonitor.bottom)
            {
                RECT client = rect;
                if (!IntersectRect(&rect, &client, &mi.rcMonitor) || IsRectEmpty(&rect))
                    goto fail;
            }
            OffsetRect(&rect, -mi.rcMonitor.left, -mi.rcMonitor.top);
        }
    }

    if (FAILED(g_pD3DDesktopSurface->LockRect(&lr, &rect, D3DLOCK_READONLY)))
        goto fail;

    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;
    memset(&g_D3DDesktopBitmap, 0, sizeof(g_D3DDesktopBitmap));
    g_D3DDesktopBitmap.bits = (uchar *) lr.pBits;
    g_D3DDesktopBitmap.type = BMT_FLAT32;
    g_D3DDesktopBitmap.w = w;
    g_D3DDesktopBitmap.h = h;
    g_D3DDesktopBitmap.wlog = D3DLog2(w);
    g_D3DDesktopBitmap.hlog = D3DLog2(h);
    g_D3DDesktopBitmap.bpp = 4;
    g_D3DDesktopBitmap.hlog0 = g_D3DDesktopBitmap.hlog;
    if (w * 4 == (1 << D3DLog2(w * 4)) && !(h & (h - 1)) && !(w & (w - 1)))
        g_D3DDesktopBitmap.flags = 0x620;
    else
        g_D3DDesktopBitmap.flags = 0x600;
    g_D3DDesktopBitmap.row = (ushort) lr.Pitch;
    g_D3DDesktopBitmap.rowlog = D3DLog2((ushort) lr.Pitch);
    g_D3DDesktopBitmap.wlog0 = g_D3DDesktopBitmap.wlog;
    g_D3DDesktopBitmap.w0 = g_D3DDesktopBitmap.w;
    g_D3DDesktopBitmap.h0 = g_D3DDesktopBitmap.h;

    gr_make_canvas(&g_D3DDesktopBitmap, &g_D3DDesktopCanvas);
    if (gr_push_canvas(&g_D3DDesktopCanvas) == 0)
    {
        g_D3DSavedModeDepth = grd_mode_info[grd_mode].bitDepth;
        grd_mode_info[grd_mode].bitDepth = 32;
        return;
    }
    D3DDestroyDesktopCanvas();
    return;

fail:
    SafeRelease(g_pD3DDesktopSurface);
}
