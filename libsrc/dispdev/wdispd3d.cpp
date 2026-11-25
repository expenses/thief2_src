#include <windows.h>
#include <d3d9.h>
#include <lg.h>
#include <ddraw.h>
#include <stdio.h>

#include <comtools.h>
#include <dispapi.h>
#include <gshelapi.h>

#include <appagg.h>

#include <dev2d.h>
#include <mprintf.h>
#include <config.h>
#include <cfgdbg.h>

#include <dddynf.h>
#include <wdisp.h>
#include <wdspprov.h>
#include <wdspmode.h>
#include <wdcreate.h>
#include <wdisptls.h>
#include <wdspgshl.h>

#include <wdispd3d.h>
#include <wd3dmode.h>

BOOL g_bWD3D9Device;
sGrModeInfo g_DesktopModeInfo;
sGrModeInfo g_D3DWindowedModeInfo;
RECT g_rcUIScaleView = { 0, 70, 640, 340 };
BOOL g_bUIScaleMode;
BOOL g_bUIScaleView;
BOOL g_bUIScaleDrawn;
int g_nD3DFrame;

#ifndef SM_CXVIRTUALSCREEN
#define SM_CXVIRTUALSCREEN 78
#define SM_CYVIRTUALSCREEN 79
#endif

static BOOL g_bInSetModeRetry;
static int g_nD3DDriverIndex;

void AddConfigDisplayModes(short * pModes, int desktopDepth, sGrModeInfo * pModeInfo);
void UpdateUIScaleMode();

static BOOL UIScaleModeEnabled()
{
    return config_is_defined("ui_scale_mode");
}

const char * WhatD3DError(HRESULT error)
{
    static char szUnknown[64];

    switch (error)
    {
        case D3D_OK:                            return "D3D_OK";
        case D3DERR_WRONGTEXTUREFORMAT:         return "D3DERR_WRONGTEXTUREFORMAT";
        case D3DERR_UNSUPPORTEDCOLOROPERATION:  return "D3DERR_UNSUPPORTEDCOLOROPERATION";
        case D3DERR_UNSUPPORTEDCOLORARG:        return "D3DERR_UNSUPPORTEDCOLORARG";
        case D3DERR_UNSUPPORTEDALPHAOPERATION:  return "D3DERR_UNSUPPORTEDALPHAOPERATION";
        case D3DERR_UNSUPPORTEDALPHAARG:        return "D3DERR_UNSUPPORTEDALPHAARG";
        case D3DERR_TOOMANYOPERATIONS:          return "D3DERR_TOOMANYOPERATIONS";
        case D3DERR_CONFLICTINGTEXTUREFILTER:   return "D3DERR_CONFLICTINGTEXTUREFILTER";
        case D3DERR_UNSUPPORTEDFACTORVALUE:     return "D3DERR_UNSUPPORTEDFACTORVALUE";
        case D3DERR_CONFLICTINGRENDERSTATE:     return "D3DERR_CONFLICTINGRENDERSTATE";
        case D3DERR_UNSUPPORTEDTEXTUREFILTER:   return "D3DERR_UNSUPPORTEDTEXTUREFILTER";
        case D3DERR_CONFLICTINGTEXTUREPALETTE:  return "D3DERR_CONFLICTINGTEXTUREPALETTE";
        case D3DERR_DRIVERINTERNALERROR:        return "D3DERR_DRIVERINTERNALERROR";
        case D3DERR_NOTFOUND:                   return "D3DERR_NOTFOUND";
        case D3DERR_MOREDATA:                   return "D3DERR_MOREDATA";
        case D3DERR_DEVICELOST:                 return "D3DERR_DEVICELOST";
        case D3DERR_DEVICENOTRESET:             return "D3DERR_DEVICENOTRESET";
        case D3DERR_NOTAVAILABLE:               return "D3DERR_NOTAVAILABLE";
        case D3DERR_OUTOFVIDEOMEMORY:           return "D3DERR_OUTOFVIDEOMEMORY";
        case D3DERR_INVALIDDEVICE:              return "D3DERR_INVALIDDEVICE";
        case D3DERR_INVALIDCALL:                return "D3DERR_INVALIDCALL";
        case D3DERR_DRIVERINVALIDCALL:          return "D3DERR_DRIVERINVALIDCALL";
        case E_INVALIDARG:                      return "E_INVALIDARG";
    }
    sprintf(szUnknown, "D3D Unknown Error Code %08X", error);
    return szUnknown;
}

///////////////////////////////////////

cD3DProvider::cD3DProvider(cWinDisplayDevice * pDisplayDevice)
  : cWinDisplayProvider(pDisplayDevice),
    m_pModeOps(NULL),
    m_bPersistentMode(FALSE),
    m_pD3D(NULL),
    m_nAdapter(0),
    m_WindowMode(0),
    m_NormalWindowStyle(0),
    m_Flags(0)
{
}

cD3DProvider::~cD3DProvider()
{
    if (m_pModeOps)
    {
        m_bPersistentMode = FALSE;
        DoClose();
    }
}

///////////////////////////////////////

int cD3DProvider::GetDisplayWidth()
{
    if (m_Flags & kDesktopRes)
        return g_DesktopModeInfo.w;
    return m_pModeOps->GetModeInfoEx().width;
}

int cD3DProvider::GetDisplayHeight()
{
    if (m_Flags & kDesktopRes)
        return g_DesktopModeInfo.h;
    return m_pModeOps->GetModeInfoEx().height;
}

///////////////////////////////////////

BOOL cD3DProvider::DoProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam, long * pRetVal)
{
    if (!m_pDisplayDevice->IsModeSet() || (m_pDisplayDevice->GetCreateFlags() & kDispDebug))
    {
        if (msg == WM_ACTIVATE || msg == WM_SIZE || msg == WM_ACTIVATEAPP || msg == WM_MOVE)
            ClipCursor(NULL);
        return FALSE;
    }

    switch (msg)
    {
        case WM_SIZE:
            if (wParam == SIZE_MINIMIZED)
            {
                ClipCursor(NULL);
                break;
            }
            if (m_pModeOps && (m_pModeOps->GetModeInfoEx().flags & kGrModeIsWindowed))
            {
                m_pModeOps->SetScaleFactor(AdjustWindow(GetMainWnd(), GetDisplayWidth(), GetDisplayHeight(), TRUE));
            }
            // fall through

        case WM_MOVE:
            if (GetActiveWindow() == GetMainWnd() && !IsIconic(GetMainWnd()))
                D3DClipCursorToWindow(GetMainWnd());
            break;

        case WM_ACTIVATE:
            if (wParam == WA_INACTIVE || IsIconic(GetMainWnd()))
                ClipCursor(NULL);
            else
                D3DClipCursorToWindow(GetMainWnd());
            break;

        case WM_ACTIVATEAPP:
            ClipCursor(NULL);
            break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            g_bD3DPaintPending = TRUE;
            HDC hdc = BeginPaint(GetMainWnd(), &ps);
            if (m_pDisplayDevice->IsFlushingPaused())
            {
                FillRect(hdc, &ps.rcPaint, (HBRUSH) GetStockObject(BLACK_BRUSH));
            }
            else
            {
                m_pDisplayDevice->FlushRect(ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);
            }
            EndPaint(GetMainWnd(), &ps);
            *pRetVal = 0;
            return TRUE;
        }
    }
    return FALSE;
}

///////////////////////////////////////

static BOOL FindMode(short * pModes, short mode)
{
    while (*pModes != -1)
    {
        if (*pModes == mode)
            return TRUE;
        pModes++;
    }
    return FALSE;
}

void cD3DProvider::DoGetInfo(sGrDeviceInfo * pGrDeviceInfo, sGrModeInfo * pModeInfo)
{
    static const D3DFORMAT formats[] = { D3DFMT_X8R8G8B8, D3DFMT_R5G6B5 };
    D3DCAPS9 caps;
    short * pModes;
    int i;

    memset(&caps, 0, sizeof(caps));
    m_pD3D->GetDeviceCaps(m_nAdapter, D3DDEVTYPE_HAL, &caps);

    LogIncIndent();
    LogMsg1("D3DProvider: Enumerating adapter %d modes", m_nAdapter);

    YieldDisplay(m_pDisplayDevice);

    pModes = pGrDeviceInfo->modes;
    *pModes = -1;
    for (i = 0; i < 2; i++)
    {
        D3DFORMAT fmt = formats[i];
        UINT nModes = m_pD3D->GetAdapterModeCount(m_nAdapter, fmt);
        int lastMode = -1;
        for (UINT j = 0; j < nModes; j++)
        {
            D3DDISPLAYMODE dm;
            int depth;

            if (FAILED(m_pD3D->EnumAdapterModes(m_nAdapter, fmt, j, &dm)))
                continue;
            switch (dm.Format)
            {
                case D3DFMT_R8G8B8:
                    depth = 24;
                    break;
                case D3DFMT_A8R8G8B8:
                case D3DFMT_X8R8G8B8:
                case D3DFMT_A2R10G10B10:
                    depth = 32;
                    break;
                case D3DFMT_R5G6B5:
                    depth = 16;
                    break;
                default:
                    continue;
            }
            int mode = gr_mode_from_info(dm.Width, dm.Height, depth);
            if (mode < 0 || mode == lastMode)
                continue;
            lastMode = mode;
            if (FindMode(pGrDeviceInfo->modes, mode))
                continue;
            *pModes++ = mode;
            *pModes = -1;
            LogMsg3("%d x %d x %d", dm.Width, dm.Height, depth);
        }
    }
    *pModes = -1;

    LogDecIndent();
    LogMsg("D3DProvider: Enumeration done");

    RegainDisplay(m_pDisplayDevice);

    pModes = pGrDeviceInfo->modes;
    while (*pModes != -1)
    {
        pModeInfo[*pModes].flags |= kGrModeIsNative;
        pModes++;
    }

    HDC hdc = GetDC(NULL);
    int desktopDepth = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(NULL, hdc);

    DWORD desktopWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    DWORD desktopHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (!desktopWidth)
        desktopWidth = GetSystemMetrics(SM_CXSCREEN);
    if (!desktopHeight)
        desktopHeight = GetSystemMetrics(SM_CYSCREEN);

    pModes = pGrDeviceInfo->modes;
    while (*pModes != -1)
    {
        pModeInfo[*pModes].flags |= kGrModeCanFullscreen;
        pModes++;
    }

    pModes = pGrDeviceInfo->modes;
    while (*pModes != -1)
    {
        int modePixelSize = (pModeInfo[*pModes].bitDepth == 15) ? 16 : pModeInfo[*pModes].bitDepth;
        if ((DWORD)pModeInfo[*pModes].w < desktopWidth &&
            (DWORD)pModeInfo[*pModes].h < desktopHeight &&
            modePixelSize == desktopDepth)
            pModeInfo[*pModes].flags |= kGrModeCanWindow;
        pModes++;
    }

    AddConfigDisplayModes(pGrDeviceInfo->modes, desktopDepth, pModeInfo);
}

///////////////////////////////////////

BOOL cD3DProvider::DoOpen(sGrModeCap *, int)
{
    if (m_bPersistentMode)
        return TRUE;

    m_WindowMode = 0;
    m_pD3D = DynDirect3DCreate9(D3D_SDK_VERSION);
    if (!m_pD3D || m_pD3D->GetAdapterCount() <= 0)
        return FALSE;

    config_get_int("d3d_driver_index", &g_nD3DDriverIndex);
    m_nAdapter = 0;
    if ((UINT)g_nD3DDriverIndex < m_pD3D->GetAdapterCount())
        m_nAdapter = g_nD3DDriverIndex;

    m_NormalWindowStyle = GetWindowLong(GetMainWnd(), GWL_STYLE);
    m_NormalWindowExStyle = GetWindowLong(GetMainWnd(), GWL_EXSTYLE);

    YieldDisplay(m_pDisplayDevice);
    BringWindowToTop(GetMainWnd());
    RegainDisplay(m_pDisplayDevice);

    D3DSetCritMsgHandler(this);
    g_bWD3D9Device = TRUE;
    return TRUE;
}

///////////////////////////////////////

BOOL cD3DProvider::DoClose()
{
    if (m_bPersistentMode)
        return TRUE;

    D3DSetCritMsgHandler(NULL);

    if (m_pModeOps)
    {
        delete m_pModeOps;
        m_pModeOps = NULL;
    }
    SafeRelease(m_pD3D);

    SetWindowLong(GetMainWnd(), GWL_STYLE, m_NormalWindowStyle);
    SetWindowLong(GetMainWnd(), GWL_EXSTYLE, m_NormalWindowExStyle);
    SetWindowPos(GetMainWnd(), NULL, 0, 0, 0, 0,
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
    InvalidateRect(NULL, NULL, TRUE);
    ShowWindow(GetMainWnd(), SW_SHOW);
    SetWindowPos(GetMainWnd(), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    g_bWD3D9Device = FALSE;
    return TRUE;
}

///////////////////////////////////////

void cD3DProvider::SetWindowMode(DWORD mode)
{
    if (m_WindowMode == mode)
        return;

    YieldDisplay(m_pDisplayDevice);
    if (mode == kD3DWindowedStyle)
    {
        SetWindowLong(GetMainWnd(), GWL_STYLE, m_NormalWindowStyle);
        SetWindowPos(GetMainWnd(), NULL, 0, 0, 0, 0,
                     SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    else
    {
        SetWindowLong(GetMainWnd(), GWL_STYLE, WS_POPUP);
        SetWindowPos(GetMainWnd(), HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        SetForegroundWindow(GetMainWnd());
    }
    g_D3DWindowMode = mode;
    m_WindowMode = mode;
    RegainDisplay(m_pDisplayDevice);
}

cWinDisplayModeOperations * cD3DProvider::DoSetMode(const sGrModeInfo & modeInfo, int flags, sGrModeCap * pReturnModeInfo)
{
    const sGrModeInfo * pPersistInfo;
    BOOL bForceWindowed;
    BOOL bUIScale;
    BOOL bScaled;

    if (modeInfo.bitDepth == 8)
        return NULL;

    bForceWindowed = config_is_defined("force_windowed");
    if (bForceWindowed)
        flags |= kDispWindowed;

    m_Flags &= ~kDesktopRes;
    pPersistInfo = NULL;
    bUIScale = FALSE;

    if (!(g_DesktopModeInfo.flags & kGrModeIsNative))
    {
        if (config_is_defined("single_display_mode"))
        {
            int mode = gr_mode_from_info(g_DesktopModeInfo.w, g_DesktopModeInfo.h, g_DesktopModeInfo.bitDepth);
            if (mode != -1)
            {
                g_DesktopModeInfo.flags |= grd_mode_info[mode].flags & 0x80;
                g_DesktopModeInfo.bankShift = grd_mode_info[mode].bankShift;
                m_Flags |= kDesktopRes;
                pPersistInfo = &g_DesktopModeInfo;
                bUIScale = UIScaleModeEnabled();
            }
        }
    }
    else
    {
        if (!(flags & kDispFullScreen) && modeInfo.bitDepth < 32)
        {
            g_D3DWindowedModeInfo = modeInfo;
            g_D3DWindowedModeInfo.bitDepth = 32;
            pPersistInfo = &g_D3DWindowedModeInfo;
        }
        if (!(flags & kDispFullScreen))
            flags |= kDispWindowed;
    }

    if (!(flags & (kDispWindowed | kDispFullScreen)))
    {
        switch (m_pDisplayDevice->GetKind())
        {
            case kDispDefault:
            case kDispFullScreen:
            case kDispUndefined:
                flags |= kDispFullScreen;
                break;
            case kDispWindowed:
            case kDispDebug:
                flags |= kDispWindowed;
                break;
            default:
                CriticalMsg("Unknown display device kind");
        }
    }

    if ((flags & kDispWindowed) && !(modeInfo.flags & kGrModeCanWindow))
        flags &= ~kDispWindowed;

    if (m_bPersistentMode)
    {
        if (!pPersistInfo ||
            ((flags & kDispWindowed) != 0) != (m_WindowMode == kD3DWindowedStyle) ||
            m_pModeOps->m_ModeInfo.w != pPersistInfo->w ||
            m_pModeOps->m_ModeInfo.h != pPersistInfo->h ||
            m_pModeOps->m_ModeInfo.bitDepth != pPersistInfo->bitDepth ||
            ((m_pModeOps->m_ModeInfo.flags ^ pPersistInfo->flags) & kGrModeIsNative))
        {
            m_bPersistentMode = FALSE;
            DoClose();
            DoOpen(NULL, 0);
        }
    }

    if (!m_bPersistentMode)
        SetWindowMode((flags & kDispWindowed) ? kD3DWindowedStyle : kD3DFullScreenStyle);

    if (m_pModeOps && !m_bPersistentMode)
    {
        delete m_pModeOps;
        m_pModeOps = NULL;
    }

    bScaled = !((m_Flags & kTriedFallback) == 0 && (modeInfo.flags & kGrModeIsNative) &&
                ((flags & kDispWindowed) || modeInfo.h >= 400));

    if (!pPersistInfo)
    {
        sGrModeInfo info = modeInfo;
        info.flags = (info.flags & 0xff) | (g_DesktopModeInfo.flags & 0x80);
        if (modeInfo.flags & kGrModeIsModeX)
        {
            int mode = gr_mode_from_info(g_DesktopModeInfo.w, g_DesktopModeInfo.h, g_DesktopModeInfo.bitDepth);
            if (mode != -1)
            {
                m_Flags |= kDesktopRes;
                g_DesktopModeInfo.flags |= grd_mode_info[mode].flags & 0x80;
                g_DesktopModeInfo.bankShift = grd_mode_info[mode].bankShift;
                info = g_DesktopModeInfo;
            }
        }
        if (modeInfo.w == 512 && !(modeInfo.flags & kGrModeIsNative))
            m_pModeOps = new cD3DModeOps(m_pDisplayDevice, this, info, bUIScale, modeInfo.flags & kGrModeIsNative);
        else
            m_pModeOps = new cD3DModeOps(m_pDisplayDevice, this, info, bUIScale, FALSE);
    }
    else if (!m_pModeOps)
    {
        if (!(g_DesktopModeInfo.flags & kGrModeIsNative) && !UIScaleModeEnabled())
            m_bPersistentMode = TRUE;
        m_pModeOps = new cD3DModeOps(m_pDisplayDevice, this, *pPersistInfo, bUIScale, m_bPersistentMode);
    }

    if (!IsWindowVisible(GetMainWnd()))
    {
        YieldDisplay(m_pDisplayDevice);
        m_pModeOps->SetScaleFactor(AdjustWindow(GetMainWnd(), GetDisplayWidth(), GetDisplayHeight()));
        ShowMainWnd();
        RegainDisplay(m_pDisplayDevice);
    }

    if (!m_pModeOps->StartMode(modeInfo, flags, pReturnModeInfo))
    {
        if (!g_bInSetModeRetry && !bScaled)
        {
            g_bInSetModeRetry = TRUE;
            if (flags & kDispWindowed)
                flags &= ~kDispWindowed;
            else
            {
                if (bForceWindowed)
                {
                    g_bInSetModeRetry = FALSE;
                    return NULL;
                }
                m_Flags |= kTriedFallback;
            }
            cWinDisplayModeOperations * pResult = DoSetMode(modeInfo, flags, pReturnModeInfo);
            g_bInSetModeRetry = FALSE;
            return pResult;
        }
        if (m_pModeOps)
            delete m_pModeOps;
        m_pModeOps = NULL;
        m_bPersistentMode = FALSE;
    }

    if (m_pModeOps)
    {
        YieldDisplay(m_pDisplayDevice);
        if (m_pModeOps->GetModeInfoEx().flags & kGrModeIsWindowed)
            m_pModeOps->SetScaleFactor(AdjustWindow(GetMainWnd(), GetDisplayWidth(), GetDisplayHeight()));
        else
        {
            m_pModeOps->SetScaleFactor(0);
            AdjustWindow(GetMainWnd(), GetDisplayWidth(), GetDisplayHeight());
        }
        UpdateUIScaleMode();
        RegainDisplay(m_pDisplayDevice);
    }
    return m_pModeOps;
}

///////////////////////////////////////

BOOL cD3DProvider::DoGetDirectDraw(IDirectDraw4 ** ppDD)
{
    *ppDD = (IDirectDraw4 *) m_pD3D;
    if (!m_pD3D)
        return FALSE;
    m_pD3D->AddRef();
    return TRUE;
}

///////////////////////////////////////

BOOL cD3DProvider::DoGetBitmapSurface(sGrBitmap *, IDirectDrawSurface4 ** ppDDS)
{
    IUnknown * pSurface = m_pModeOps->m_pSecondarySurface;
    if (!pSurface)
        pSurface = m_pModeOps->m_pSecondaryTexture;
    if (pSurface)
    {
        pSurface->AddRef();
        *ppDDS = (IDirectDrawSurface4 *) pSurface;
    }
    return TRUE;
}

///////////////////////////////////////

BOOL cD3DProvider::DoIsPersistentMode()
{
    return m_bPersistentMode;
}

///////////////////////////////////////

void AddConfigDisplayModes(short * pModes, int desktopDepth, sGrModeInfo * pModeInfo)
{
    short * pEnd = pModes;
    int nModes = 0;

    while (*pEnd != -1)
    {
        pEnd++;
        nModes++;
    }
    if (nModes >= GRD_MODES)
        return;

    BOOL bForceWindowed = config_is_defined("force_windowed");
    int screenW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int screenH = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (!screenW)
        screenW = GetSystemMetrics(SM_CXSCREEN);
    if (!screenH)
        screenH = GetSystemMetrics(SM_CYSCREEN);

    for (short mode = 2; mode < GRD_MODES; mode++)
    {
        grs_mode_info * pInfo = &grd_mode_info[mode];
        if (!(pInfo->flags & kGrModeSupported) || pInfo->w < 640 || pInfo->h < 480 ||
            pInfo->w > screenW || pInfo->h > screenH || pInfo->bitDepth != desktopDepth)
            continue;
        if (FindMode(pModes, mode))
            continue;
        nModes++;
        *pEnd++ = mode;
        *pEnd = -1;
        if (bForceWindowed)
        {
            pInfo->flags |= kGrModeCanFullscreen | kGrModeCanWindow;
            pModeInfo[mode].flags |= kGrModeCanFullscreen | kGrModeCanWindow;
        }
        else
        {
            pInfo->flags = (pInfo->flags & ~kGrModeCanFullscreen) | kGrModeCanWindow;
            pModeInfo[mode].flags = (pModeInfo[mode].flags & ~kGrModeCanFullscreen) | kGrModeCanWindow;
        }
        pModeInfo[mode].flags |= kGrModeIsNative;
        if (nModes >= GRD_MODES)
            return;
    }
}

void UpdateUIScaleMode()
{
    AutoAppIPtr(GameShell);
    pGameShell->BeginFrame();
}

static cD3DProvider * g_pCritMsgProvider;

static void LGAPI D3DCritMsgNotificationHandler(eCritMsgNotification notification)
{
    if (!g_pCritMsgProvider || !g_pD3DModeOps)
        return;
    if (notification == kCritMsgEnter)
        ClipCursor(NULL);
}

void D3DSetCritMsgHandler(cD3DProvider * pProvider)
{
    if (pProvider && !g_pCritMsgProvider)
        CritMsgSetHandler(D3DCritMsgNotificationHandler);
    else if (!pProvider && g_pCritMsgProvider)
        CritMsgSetHandler(NULL);
    g_pCritMsgProvider = pProvider;
}
