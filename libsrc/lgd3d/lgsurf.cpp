


#include <windows.h>
#include <ddraw.h>
#include <lgd3d.h>
#include <lgsurf.h>
#include <lgsurf_i.h>
#include <dev2d.h>
#include <lgassert.h>
#include <d6check.h>
#include <lglog.h>
#include <dbg.h>
#include <appagg.h>
#include <dispapi.h>
#include <wdispapi.h>
#include <lglog.h>
#include <lgassert.h>

#include <dddynf.h>
char* GetDDErrorMsg(long hr);
extern int lgd3d_g_bInitialized;
extern "C" void lgd3d_clean_render_surface(int zbuffer);
extern "C" int lgd3d_enumerate_devices(void);
extern "C" BOOL lgd3d_attach_to_lgsurface(ILGSurface* pILGSurface);
extern "C" lgd3ds_device_info* lgd3d_get_device_info(int nID);

EXTERN_C const GUID CDECL IID_ILGSurface = { 0x5E00025E, 0x7CDE, 0x142D, { 0x83, 0x48, 0x00, 0xAA, 0x00, 0xA8, 0x2B, 0x51 } };
EXTERN_C const GUID CDECL IID_ILGDD4Surface =
    { 0x6B000269, 0x7CE9, 0x1438, { 0x83, 0x48, 0x00, 0xAA, 0x00, 0xA8, 0x2B, 0x51 } };






BOOL CreateLGSurface(ILGSurface** ppILGSurf)
{
    *ppILGSurf = NULL;
    *ppILGSurf = new cLGSurface;
    return *ppILGSurf != NULL;

}

STDMETHODIMP cLGSurface::QueryInterface(REFIID id, LPVOID* ppI)
{
    if (id == IID_ILGSurface || id == IID_IUnknown)
    {
        *ppI = (ILGSurface*)this;
    }
    else if (id == IID_ILGDD4Surface)
    {
        *ppI = (ILGDD4Surface*)this;
    }
    else
    {
        *ppI = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP_(unsigned long) cLGSurface::AddRef()
{
    return __m_ulRefs.AddRef();
}

STDMETHODIMP_(unsigned long) cLGSurface::Release()
{
    if (__m_ulRefs.Release() != 0) {
        return (unsigned long)__m_ulRefs;
    }
    this->OnFinalRelease();
    return 0;
}

STDMETHODIMP cLGSurface::CreateInternalScreenSurface(HWND hMainWindow)
{
    HRESULT hr;
    IDirectDraw* lpDD;
    DDSURFACEDESC2 ddsd;
    IUnknown* pUnk;

    if (m_eSurfState != 0)
        return E_LGSURF_ALREADY_INITIALIZED;

    if (LoadDirectDraw() == 0)
        return E_LGSURF_HARDWARE_ERROR;

    lpDD = NULL;
    pUnk = NULL;
    hr = DynDirectDrawCreate(NULL, &lpDD, NULL);
    if (hr != 0) {
#line 150
        CheckHResult(hr, "DirectDraw creation .");
        goto error;
    }
    hr = lpDD->QueryInterface(IID_IDirectDraw4, (void**)&m_lpDD4);
    if (hr != 0) {


        CheckHResult(hr, "QueryInterface for DD4 .");
        goto error;
    }
    hr = m_lpDD4->SetCooperativeLevel(hMainWindow, DDSCL_NORMAL);
    if (hr != 0) {



        CheckHResult(hr, "SetCooperativeLevel for DD4 .");
        goto error;
    }
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = m_lpDD4->CreateSurface(&ddsd, (IDirectDrawSurface4**)&m_lpScreen, NULL);
    if (hr != 0) {
#line 180
        CheckHResult(hr, "CreateSurface screen .");
        goto error;
    }
    ChangeClipper(hMainWindow);

    if (lpDD)
        lpDD->Release();

    m_eSurfState = kLGSSInitialized;
    return S_OK;

error:
    if (lpDD)
        lpDD->Release();
    lpDD = NULL;
    if (m_lpScreen) {
        m_lpScreen->Release();
    }
    m_lpScreen = NULL;
    if (m_lpDD4) {
        m_lpDD4->Release();
    }
    m_lpDD4 = NULL;
    return E_LGSURF_HARDWARE_ERROR;
}

STDMETHODIMP cLGSurface::ChangeClipper(HWND hMainWindow)
{
    HRESULT hr;
    LPDIRECTDRAWCLIPPER clip;
    HWND hwndClip;

    clip = NULL;
    hr = 0;

    m_lpScreen->GetClipper(&clip);
    if (clip) {
        hwndClip = NULL;
        clip->GetHWnd(&hwndClip);
        if (hwndClip && hwndClip == hMainWindow)
            return S_OK;

        m_lpScreen->SetClipper(NULL);
        if (clip)
            clip->Release();
        clip = NULL;
    }

    if (hMainWindow == NULL)
        return S_OK;

    hr = m_lpDD4->CreateClipper(0, &clip, NULL);
    if (hr != 0) {
        if (hr == 0); else {



            CriticalHResult(hr, "CreateClipper failed.");
            goto error;
        }
    }
    hr = clip->SetHWnd(0, hMainWindow);
    if (hr != 0) {
        if (hr == 0); else {

            CriticalHResult(hr, "SetHWnd failed.");
            goto error;
        }
    }
    hr = m_lpScreen->SetClipper(clip);
    if (hr != 0) {
        if (hr == 0); else {
            CriticalHResult(hr, "SetClipper failed.");
            goto error;
        }
    }

    if (clip)
        clip->Release();
    clip = NULL;
    return S_OK;

error:
    if (clip)
        clip->Release();
    clip = NULL;
    return E_LGSURF_HARDWARE_ERROR;
}

STDMETHODIMP cLGSurface::SetAs3dHdwTarget(DWORD dwRequestedFlags, DWORD dwWidth, DWORD dwHeight, DWORD dwBitDepth, DWORD* pdwCapabilityFlags, grs_canvas** ppsCanvas)
{
    HRESULT hr;
    IDisplayDevice* pDispDev;
    GUID* pDevGuid;
    lgd3ds_device_info* pDevInfo;
    DDSURFACEDESC2 ddsd;

    if (lgd3d_g_bInitialized != 0)
        return E_LGSURF_3DHDW_INUSE;

    *pdwCapabilityFlags = 0;
    GUID* pGuid = NULL;

    if (m_eSurfState == kLGSSUninitialized) {
        IWinDisplayDevice* pWinDisp = AppGetObj(IWinDisplayDevice);
        pWinDisp->GetDirectDraw((IDirectDraw4**)&m_lpDD4);
        if (m_lpDD4 == NULL)
            return E_LGSURF_2D_DRIVER_ERROR;

        hr = pWinDisp->GetBitmapSurface(NULL, (IDirectDrawSurface4**)&m_lpScreen);
        if (pWinDisp) pWinDisp->Release();
        if (hr == 0) {
            ReleaseContent();
            return E_LGSURF_2D_DRIVER_ERROR;
        }
        if (m_lpScreen == NULL) {
#line 307
            CriticalMsg("No render surface available.");
        }

        pDispDev = AppGetObj(IDisplayDevice);
        pDispDev->GetKind2(NULL, NULL, &pGuid);
        if (pDispDev) pDispDev->Release();
    }

    int nDevices = lgd3d_enumerate_devices();
    m_nD3DDevID = -1;

    for (int i = 0; i < nDevices; i++) {
        pDevInfo = lgd3d_get_device_info(i);
        pDevGuid = pDevInfo->p_ddraw_guid;
        if (pGuid == NULL) {
            if (pDevGuid == NULL) {
                m_nD3DDevID = i;
                break;
            }
        } else if (pDevGuid && IsEqualGUID(*pGuid, *pDevGuid)) {
            m_nD3DDevID = i;
            break;
        }
    }

    if (m_nD3DDevID == -1)
        return E_LGSURF_2D_DRIVER_ERROR;

    pDevInfo = lgd3d_get_device_info(m_nD3DDevID);
    pDevInfo->flags |= dwRequestedFlags;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_3DDEVICE | DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY | DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY;
    ddsd.dwWidth = dwWidth;
    ddsd.dwHeight = dwHeight;

    hr = m_lpDD4->CreateSurface(&ddsd, (IDirectDrawSurface4**)&m_lpTheSurface, NULL);
    if (hr != 0) {
        ReleaseContent();
        if (hr == DDERR_OUTOFVIDEOMEMORY)
            return E_LGSURF_OUT_OF_VIDEO_MEMORY;
        ReleaseContent();
        Warning((pcDXef, "cLGSurface::CreateSurface (%d): %s", HRESULT_CODE(hr), GetDDErrorMsg(hr)));
        return E_LGSURF_HARDWARE_ERROR;
    }

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    hr = m_lpTheSurface->GetSurfaceDesc(&ddsd);
    if (hr != 0) {
#line 395
        CriticalHResult(hr, "cLGSurface::GetSurfaceDesc .");
        return E_LGSURF_HARDWARE_ERROR;
    }

    RecordSurfaceDescription(&ddsd);

    i = lgd3d_attach_to_lgsurface((ILGSurface*)this);
    if (i == 0) {
        ReleaseContent();
        return E_LGSURF_HARDWARE_ERROR;
    }

    m_b3DAttached = 1;
    *pdwCapabilityFlags = pDevInfo->flags;
    m_bZbuffer = (*pdwCapabilityFlags & (LGD3DF_ZBUFFER | LGD3DF_WBUFFER)) != 0;
    gr_init_canvas(&m_sCanvas, 0, 3, dwWidth, dwHeight);
    FixThePitchInCanvas();
    SetBitsInCanvas(NULL);
    *ppsCanvas = &m_sCanvas;
    m_eSurfState = kLGSSInitialized;
    return 0;
}

STDMETHODIMP cLGSurface::Resize3dHdw(DWORD dwRequestedFlags, DWORD dwWidth, DWORD dwHeight, DWORD dwBitDepth, DWORD* pdwCapabilityFlags, grs_canvas** ppsCanvas)
{
    ReleaseContent();
    return SetAs3dHdwTarget(dwRequestedFlags, dwWidth, dwHeight, dwBitDepth, pdwCapabilityFlags, ppsCanvas);
}

STDMETHODIMP cLGSurface::BlitToScreen(DWORD dwXScreen, DWORD dwYScreen, DWORD dwWidth, DWORD dwHeight)
{
    RECT destRect;
    HRESULT hr;
    int iLockCount;
    RECT srcRect;
    IDisplayDevice* pDispDev;

    if (!m_b3DAttached) {
#line 487
        CriticalMsg("cLGSurface::BlitToScreen: 3dhardware not ready");
    }

    if (m_eSurfState != kLGSSUnlocked)
        return E_LGSURF_SURFACE_LOCKED;

    if (dwHeight > m_sDescription.dwHeight || dwWidth > m_sDescription.dwWidth)
        return E_LGSURF_INPUT_PARAMETERS_INVALID;

    srcRect.left = 0;
    srcRect.top = 0;
    srcRect.right = dwWidth;
    srcRect.bottom = dwHeight;

    destRect.left = dwXScreen;
    destRect.top = dwYScreen;
    destRect.right = dwXScreen + dwWidth;
    destRect.bottom = dwYScreen + dwHeight;

    pDispDev = AppGetObj(IDisplayDevice);

    iLockCount = pDispDev->BreakLock();

    hr = m_lpScreen->Blt(&destRect, m_lpTheSurface, &srcRect, DDBLT_WAIT, NULL);
    if (hr != 0)
    {
        Warning((pcDXef, "BlitToScreen failed (%d): %s", HRESULT_CODE(hr), GetDDErrorMsg(hr)));
        return E_LGSURF_HARDWARE_ERROR;
    }

    pDispDev->RestoreLock(iLockCount);
    if (pDispDev)
        pDispDev->Release();
    pDispDev = NULL;
    return S_OK;
}

STDMETHODIMP_(void) cLGSurface::CleanSurface()
{
#line 534
    AssertMsg(m_b3DAttached, "cLGSurface::CleanSurface 3dhardware not initialized");
    AssertMsg(m_eSurfState == kLGSSUnlocked, "cLGSurface::CleanSurface surface was not in unlocked state");
    lgd3d_clean_render_surface(m_bZbuffer);
}

STDMETHODIMP_(void) cLGSurface::Start3D()
{
#line 547
    AssertMsg(m_b3DAttached, "cLGSurface::CleanSurface 3dhardware not initialized");
    AssertMsg(m_eSurfState == kLGSSUnlocked, "cLGSurface::Start3D surface was locked");
    lgd3d_start_frame(m_nFrameCount++);
    m_eSurfState = kLGSS3DRenderining;
}

STDMETHODIMP_(void) cLGSurface::End3D()
{




    AssertMsg(m_b3DAttached, "cLGSurface::End3D 3dhardware not initialized");
    AssertMsg(m_eSurfState == kLGSS3DRenderining, "cLGSurface::End3D surface was not in 3d render state");
    lgd3d_end_frame();
    m_eSurfState = kLGSSUnlocked;
}

STDMETHODIMP cLGSurface::LockFor2D(grs_canvas** ppsCanvas, eLGSBlit eBlitFlags)
{
    DWORD dwFlags;
    DDSURFACEDESC2 ddsd;
    HRESULT hr;

    if (m_eSurfState != kLGSSUnlocked)
        return E_LGSURF_SURFACE_LOCKED;

    dwFlags = DDLOCK_NOSYSLOCK | DDLOCK_WAIT;
    if ((eBlitFlags & kLGSBReadWrite) == 0)
    {
        if ((eBlitFlags & kLGSBRead) != 0)
            dwFlags |= DDLOCK_READONLY;
        else
            dwFlags |= DDLOCK_WRITEONLY;
    }

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);

    hr = m_lpTheSurface->Lock(NULL, &ddsd, dwFlags, NULL);
    if (hr != 0)
    {
        Warning((pcDXef, "cLGSurface::LockFor2D: Lock failed (%d): %s", HRESULT_CODE(hr), GetDDErrorMsg(hr)));
        return E_LGSURF_HARDWARE_ERROR;
    }

    m_eSurfState = kLGSS2DLocked;
    RecordSurfaceDescription(&ddsd);
    FixThePitchInCanvas();
    SetBitsInCanvas(ddsd.lpSurface);
    *ppsCanvas = &m_sCanvas;
    return S_OK;
}

STDMETHODIMP cLGSurface::UnlockFor2D()
{
    if (m_eSurfState != kLGSS2DLocked)
        return E_LGSURF_SURFACE_WASNOT_LOCKED;

    HRESULT hr = m_lpTheSurface->Unlock(NULL);
    if (hr != 0)
    {
        Warning((pcDXef, "cLGSurface::UnlockFor2D: Unlock failed (%d): %s", HRESULT_CODE(hr), GetDDErrorMsg(hr)));
        if (hr == DDERR_NOTLOCKED)
            return E_LGSURF_SURFACE_WASNOT_LOCKED;
        return E_LGSURF_HARDWARE_ERROR;
    }

    m_eSurfState = kLGSSUnlocked;
    SetBitsInCanvas(NULL);
    return S_OK;
}

STDMETHODIMP_(DWORD) cLGSurface::GetWidth()
{
    return m_sDescription.dwWidth;
}

STDMETHODIMP_(DWORD) cLGSurface::GetHeight()
{
    return m_sDescription.dwHeight;
}

STDMETHODIMP_(DWORD) cLGSurface::GetPitch()
{
    return m_sDescription.dwPitch;
}

STDMETHODIMP_(DWORD) cLGSurface::GetBitDepth()
{
    return m_sDescription.dwBitDepth;
}

STDMETHODIMP_(eLGSurfState) cLGSurface::GetSurfaceState()
{
    return m_eSurfState;
}

STDMETHODIMP cLGSurface::GetSurfaceDescription(sLGSurfaceDescription* psDsc)
{
    if (!psDsc)
        return E_LGSURF_INPUT_PARAMETERS_INVALID;
    *psDsc = m_sDescription;
    return S_OK;
}

STDMETHODIMP_(int) cLGSurface::GetDeviceInfoIndex()
{
    return m_nD3DDevID;
}

STDMETHODIMP_(int) cLGSurface::GetDirectDraw(IDirectDraw4** ppDD)
{
    *ppDD = m_lpDD4;
    if (m_lpDD4 == NULL)
        return 0;
    m_lpDD4->AddRef();
    return 1;
}

STDMETHODIMP_(int) cLGSurface::GetRenderSurface(IDirectDrawSurface4** ppSurf)
{
    *ppSurf = m_lpTheSurface;
    if (m_lpTheSurface == NULL)
        return 0;
    m_lpTheSurface->AddRef();
    return 1;
}

cLGSurface::cLGSurface():
    m_nFrameCount(0), m_lpDD4(NULL), m_lpScreen(NULL),
    m_lpTheSurface(NULL), m_nD3DDevID(-1), m_eSurfState(kLGSSUninitialized),
    m_bZbuffer(0), m_b3DAttached(0)
{
    memset(&m_sDescription, 0, sizeof(m_sDescription));
}

cLGSurface::~cLGSurface()
{
    ReleaseContent();
}

void cLGSurface::ReleaseContent()
{
    if (m_b3DAttached)
        lgd3d_shutdown();
    if (m_lpTheSurface)
        m_lpTheSurface->Release();
    m_lpTheSurface = NULL;
    if (m_lpScreen)
        m_lpScreen->Release();
    m_lpScreen = NULL;
    if (m_lpDD4)
        m_lpDD4->Release();
    m_lpDD4 = NULL;
    memset(&m_sDescription, 0, sizeof(m_sDescription));
    memset(&m_sCanvas, 0, sizeof(m_sCanvas));
}

void cLGSurface::RecordSurfaceDescription(_DDSURFACEDESC2* lpDDSD)
{
    memset(&m_sDescription, 0, sizeof(m_sDescription));
    m_sDescription.dwWidth = lpDDSD->dwWidth;
    m_sDescription.dwHeight = lpDDSD->dwHeight;
    m_sDescription.dwPitch = lpDDSD->dwLinearSize;
    m_sDescription.dwBitDepth = lpDDSD->ddpfPixelFormat.dwRGBBitCount;
    m_sDescription.sBitmask.red = lpDDSD->ddpfPixelFormat.dwRBitMask;
    m_sDescription.sBitmask.green = lpDDSD->ddpfPixelFormat.dwGBitMask;
    m_sDescription.sBitmask.blue = lpDDSD->ddpfPixelFormat.dwBBitMask;
    m_sDescription.sBitmask.alpha = lpDDSD->ddpfPixelFormat.dwRGBAlphaBitMask;
}

void cLGSurface::OnFinalRelease()
{
    delete this;
}

