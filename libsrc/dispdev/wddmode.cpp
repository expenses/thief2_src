///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/wddmode.cpp $
// $Author: KEVIN $
// $Date: 1998/04/14 15:02:23 $
// $Revision: 1.15 $
//

// Core headers
#include <windows.h>
#include <ddraw.h>

#include <lg.h>

// Component APIs
#include <comtools.h>
#include <dispapi.h>
#include <gshelapi.h>

// Application aggregate
#include <appagg.h>

// Other required LG libraries
#include <dev2d.h>
#include <math.h>

// Windows display implementation headers
#include <wdisp.h>
#include <wdspprov.h>
#include <wdspmode.h>
#include <wdcreate.h>

#include <wdispdd.h>
#include <wddmode.h>
#include <d3d.h>
#include <lgd3d.h>
#include <config.h>

#ifdef MONO_SPEW
#define put_mono(c) (*((uchar *)0xb0078)) = (c)
#else
#define put_mono(c)
#endif

///////////////////////////////////////////////////////////////////////////////

#ifndef DEBUG_SETMODE
#define DEBUG_SETMODE 0
#else
#pragma message ("DEBUG_SETMODE defined")
#endif

#ifndef DEBUG_PALETTE
#define DEBUG_PALETTE 0
#else
#pragma message ("DEBUG_PALETTE defined")
#endif

#if DEBUG_PALETTE
#define DebugDumpPalette(paletteEntries, uStart, n) \
{ \
  for (int i = uStart; i < n; i++) \
    DebugMsgEx4(PALETTE, "fLogicalPalette.palPalEntry[%d] (r,g,b) = (%d,%d,%d)", i, paletteEntries[i].peRed, paletteEntries[i].peGreen, paletteEntries[i].peBlue ); \
}
#else
#define DebugDumpPalette(p, uStart, n)
#endif

///////////////////////////////////////////////////////////////////////////////
//
// class cDDModeOpsBase
//

cDDModeOpsBase::~cDDModeOpsBase()
{
    GetDD()->FlipToGDISurface();
    SafeRelease(m_pSecondarySurface);
    SafeRelease(m_pPrimarySurface);
    SafeRelease(m_pPalette);
}

///////////////////////////////////////

BOOL cDDModeOpsBase::DoSetPalette(LOGPALETTE & logicalPalette, unsigned uStart, unsigned n)
{
    BEGIN_DEBUG_MSG_TRUE_EX(PALETTE, m_pPalette, "cDDProvider::DoSetPalette()");
    HRESULT result;
    if (m_pPalette != NULL)
    {
        DebugDumpPalette(logicalPalette.palPalEntry, uStart, n);

        int iLock = m_pDisplayDevice->BreakLock();
        RestoreSurfaces();
        result = m_pPalette->SetEntries(0,uStart,n,logicalPalette.palPalEntry);
        DebugMsgIfErr("DoSetPalette: SetEntries", result);
        m_pDisplayDevice->RestoreLock(iLock);
    }
    else
    {
        DebugMsgEx(PALETTE, "No palette yet");
    }

    return TRUE;
    END_DEBUG;
}

///////////////////////////////////////

void cDDModeOpsBase::GetSurfaces(IDirectDrawSurface4 ** ppPrimarySurface,
                                 IDirectDrawSurface4 ** ppSecondarySurface)

{
    if (ppPrimarySurface)
    {
        if (m_pPrimarySurface)
        {
            *ppPrimarySurface = m_pPrimarySurface;
            m_pPrimarySurface->AddRef();
        }
        else
            ppPrimarySurface = NULL;
    }
    
    if (ppSecondarySurface)
    {
        if (m_pSecondarySurface && m_bFlip)
        {
            *ppSecondarySurface = m_pSecondarySurface;
            m_pSecondarySurface->AddRef();
        }
        else
            *ppSecondarySurface = NULL;
    }
}

///////////////////////////////////////

void cDDModeOpsBase::SetClipToWindow(BOOL fWantClipping)
{
    const BOOL fClippingNow = (m_Flags & kClippingToWindow);
    if (fWantClipping && !fClippingNow)
    {
        IDirectDrawClipper * pDirectDrawClipper;
        HRESULT result = GetDD()->CreateClipper(0, &pDirectDrawClipper, NULL);
        DebugMsgIfErr("DoSetPalette: SetEntries", result);
        if (result == S_OK)
        {
            pDirectDrawClipper->SetHWnd(0, m_pOuter->GetMainWnd());
            m_pPrimarySurface->SetClipper(pDirectDrawClipper);
            pDirectDrawClipper->Release();
            m_Flags |= kClippingToWindow;
        }
    }
    else if (!fWantClipping && fClippingNow)
    {
        m_pPrimarySurface->SetClipper(NULL);
        m_Flags &= ~kClippingToWindow;
    }
}

///////////////////////////////////////

void cDDModeOpsBase::SetModeInfoFromSurfaceDesc(const DDSURFACEDESC2 & renderTargetDesc,
                                                int flags,
                                                sGrModeCap * pReturnModeInfo)
{
    // Get the linear address of video
    DoLock(pReturnModeInfo);
    DoUnlock();

    // Fill out various info structures (2d and internal)


    AssertMsg(renderTargetDesc.dwFlags & DDSD_PIXELFORMAT, "Didn't get pixel format in descriptor!");

    if (flags & kGrModeIsWindowed)
    {
        m_ModeInfoEx.width  = pReturnModeInfo->w;
        m_ModeInfoEx.height = pReturnModeInfo->h;
    }
    else
    {
        m_ModeInfoEx.width  = pReturnModeInfo->w = (short) renderTargetDesc.dwWidth;
        m_ModeInfoEx.height = pReturnModeInfo->h = (short) renderTargetDesc.dwHeight;
    }

    m_ModeInfoEx.rowBytes  = pReturnModeInfo->rowBytes = renderTargetDesc.lPitch;

    m_ModeInfoEx.depth     = renderTargetDesc.ddpfPixelFormat.dwRGBBitCount;
    m_ModeInfoEx.mode      = 0;
    m_ModeInfoEx.flags     = flags;
    m_ModeInfoEx.redMask   = renderTargetDesc.ddpfPixelFormat.dwRBitMask;
    m_ModeInfoEx.greenMask = renderTargetDesc.ddpfPixelFormat.dwGBitMask;
    m_ModeInfoEx.blueMask  = renderTargetDesc.ddpfPixelFormat.dwBBitMask;

    int colorBitsPerPixel;

    if (m_ModeInfoEx.depth == 16)
    {
        if ((m_ModeInfoEx.redMask | m_ModeInfoEx.greenMask | m_ModeInfoEx.blueMask) & 0x8000)
            colorBitsPerPixel = 16;
        else
            colorBitsPerPixel = 15;
    }
    else
        colorBitsPerPixel = m_ModeInfoEx.depth;

    m_pDisplayDevice->ModeInfoToEnumMode(m_ModeInfoEx.width,
                                         m_ModeInfoEx.height,
                                         colorBitsPerPixel,
                                         &m_ModeInfoEx.mode);
    m_ModeInfoEx.flags |= gr_mode_info_from_mode(m_ModeInfoEx.mode)->flags;
}

///////////////////////////////////////

BOOL cDDModeOpsBase::WipeSurface(IDirectDrawSurface4 *pdds)
{
    if (pdds)
    {
        BEGIN_DEBUG_MSG_EX1(VERBOSE, "WipeSurface(0x%lx)", pdds);

        DDSURFACEDESC2 surfaceDesc = { 0 };
        surfaceDesc.dwSize = sizeof(surfaceDesc);
        HRESULT result = pdds->Lock(NULL, &surfaceDesc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);
        DebugMsgIfErr("WipeSurface Lock failed.", result);
        if (result != DD_OK)
            return FALSE;

        uchar * pRow = (uchar*)surfaceDesc.lpSurface;
        for (int y=0; y < surfaceDesc.dwHeight; y++)
        {
            memset(pRow, 0, surfaceDesc.dwWidth * (surfaceDesc.ddpfPixelFormat.dwRGBBitCount / 8));
            pRow += surfaceDesc.lPitch;
        }

        pdds->Unlock(NULL);

        END_DEBUG;
    }

    return TRUE;
}

///////////////////////////////////////

BOOL cDDModeOpsBase::DoSetGamma(double gamma)
{
    IDirectDrawGammaControl * pGammaControl = NULL;

    if (m_pPrimarySurface->QueryInterface(IID_IDirectDrawGammaControl, (void **)&pGammaControl) < 0)
        return FALSE;

    DDGAMMARAMP ramp;
    for (int i = 0; i < 256; i++)
        ramp.red[i] = ramp.green[i] = ramp.blue[i] = (WORD)(pow(i * (1.0 / 255.0), gamma) * 65535.0);

    HRESULT result = pGammaControl->SetGammaRamp(0, &ramp);
    SafeRelease(pGammaControl);
    return (result >= 0);
}

///////////////////////////////////////////////////////////////////////////////
//
// class cDirectDDModeOps
//

cDirectDDModeOps::~cDirectDDModeOps()
{
    // Jacobson, 3-4-96
    // Release the pause surface if it exists (it shouldn't usually)
    SafeRelease(m_pPauseSurface);
}

BOOL cDirectDDModeOps::DoStatVTrace()
{
    HRESULT result;
    BOOL bResult;
    result = GetDD()->GetVerticalBlankStatus(&bResult); // force synchoniousity
    DebugMsgIfErr("DoSetMode: GetVerticalBlankStatus", result);
    return bResult;
}

///////////////////////////////////////

BOOL cDirectDDModeOps::StartMode(const sGrModeInfo & modeInfo,
                                 int flags,
                                 sGrModeCap * pReturnModeInfo)
{
    HRESULT result;
    BEGIN_DEBUG_MSG_EX(SETMODE, "cDirectDDModeOps::StartMode()");

    // Release the previous mode, if any
    DebugMsgEx(SETMODE, "Release previous surfaces, if any,,,");
    SafeRelease(m_pPrimarySurface);          // Sets m_pPrimarySurface to null
    SafeRelease(m_pSecondarySurface);

    DDSURFACEDESC2 surfaceDesc;
    memset(&surfaceDesc, 0, sizeof(surfaceDesc));
    surfaceDesc.dwSize = sizeof(surfaceDesc);

#ifndef SHIP
    m_bHIW = (m_pDisplayDevice->GetCreateFlags() & kDispAttempt3D) && (m_pDisplayDevice->GetCreateFlags() & kDispAttemptFlippable)
             && (GetCoopFlags() & DDSCL_NORMAL);
    if (m_bHIW)
    {
        IDirectDrawClipper * piClipper;

        surfaceDesc.dwFlags = DDSD_CAPS;
        surfaceDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        result = GetDD()->CreateSurface(&surfaceDesc, &m_pPrimarySurface, NULL);
        if (result != DD_OK)
        {
            DebugMsgIfErr("DoSetMode: CreateSurface", result);
            return FALSE;
        }
        result = GetDD()->CreateClipper(0, &piClipper, NULL);
        if (result != DD_OK)
        {
            DebugMsgIfErr("DoSetMode: CreateClipper", result);
            return FALSE;
        }
        result = piClipper->SetHWnd(0, m_pOuter->GetMainWnd());
        if (result != DD_OK)
        {
            DebugMsgIfErr("DoSetMode: SetHWnd", result);
            return FALSE;
        }
        result = m_pPrimarySurface->SetClipper(piClipper);
        if (result != DD_OK)
        {
            DebugMsgIfErr("DoSetMode: SetClipper", result);
            return FALSE;
        }
        SafeRelease(piClipper);

        surfaceDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
        surfaceDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
        surfaceDesc.dwWidth = modeInfo.w;
        surfaceDesc.dwHeight = modeInfo.h;
        result = GetDD()->CreateSurface(&surfaceDesc, &m_pSecondarySurface, NULL);
        if (result != DD_OK)
        {
            DebugMsgIfErr("DoSetMode: CreateSurface", result);
            return FALSE;
        }

        RestoreSurfaces();
        result = m_pPrimarySurface->Lock(NULL, &surfaceDesc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);
        DebugMsgIfErr("StartMode: DirectDrawSurface->Lock", result);
        if (result == DDERR_SURFACELOST)
        {
            RestoreSurfaces();
            result = m_pPrimarySurface->Lock(NULL, &surfaceDesc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);
        }
        if (result != DD_OK)
        {
#line 386
            CriticalMsg1("Failed to lock primary DirectDraw surface (%s)", WhatDDError(result));
            return FALSE;
        }
        m_pPrimarySurface->Unlock(NULL);

        m_PrimaryDesc.dwSize = sizeof(m_PrimaryDesc);
        result = m_pPrimarySurface->GetSurfaceDesc(&m_PrimaryDesc);
        if (result != DD_OK)
        {
            DebugMsgIfErr("DoSetMode: GetSurfaceDesc", result);
            return FALSE;
        }
        m_SecondaryDesc.dwSize = sizeof(m_SecondaryDesc);
        result = m_pSecondarySurface->GetSurfaceDesc(&m_SecondaryDesc);
        if (result != DD_OK)
        {
            DebugMsgIfErr("DoSetMode::GetSurfaceDesc", result);
            return FALSE;
        }
        m_bFlip = TRUE;
        SetModeInfoFromSurfaceDesc(m_SecondaryDesc,
                                   (flags & kGrSetWindowed) ? (kGrModeIsWindowed | kGrModeIsDirect) : kGrModeIsDirect,
                                   pReturnModeInfo);
        pReturnModeInfo->vbase2 = NULL;
        return TRUE;
    }
#endif

    // If we're not running in a window, set the screen display mode
    if (!(GetCoopFlags() & DDSCL_NORMAL))
    {
        DebugMsgEx(SETMODE, "Setting display mode...");
        int realBitsPerPixel = (modeInfo.bitDepth != 15) ? modeInfo.bitDepth : 16;
        result = SetDisplayMode(modeInfo.w, modeInfo.h, realBitsPerPixel);

        if (result != DD_OK)
        {
            DebugMsgIfErr("DoSetMode: SetDisplayMode", result);
            return FALSE;
        }
    }
    else
        DebugMsgEx(SETMODE, "Didn't set display mode...");

    pReturnModeInfo->w = modeInfo.w;
    pReturnModeInfo->h = modeInfo.h;

    // Create the new surface(s)
    const BOOL fAttemptFlippable = (m_pDisplayDevice->GetCreateFlags() & kDispAttemptFlippable) && !(GetCoopFlags() & DDSCL_NORMAL);
    const BOOL fAttempt3D = (m_pDisplayDevice->GetCreateFlags() & kDispAttempt3D);

    DebugMsgEx(SETMODE, "Creating the surface(s)...");

    // Try flippable, if requested...
    DWORD caps = DDSCAPS_PRIMARYSURFACE;
    if (fAttempt3D)
        caps |= DDSCAPS_3DDEVICE;
    if (fAttemptFlippable)
    {
        DebugMsgEx(SETMODE, "Attempting to create flippable surface...");
        surfaceDesc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
        surfaceDesc.ddsCaps.dwCaps = caps | DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_VIDEOMEMORY;
        surfaceDesc.dwBackBufferCount = 1;

        result = GetDD()->CreateSurface(&surfaceDesc, &m_pPrimarySurface, NULL);
        DebugMsgIfErr("DoSetMode: CreateSurface", result);
        // Fall through in event of failure
    }

    // If we either failed or didn't want to get flippable surface, create a non-flippable one
    if (!m_pPrimarySurface)
    {
        surfaceDesc.dwFlags = DDSD_CAPS;
        surfaceDesc.ddsCaps.dwCaps = caps;

        result = GetDD()->CreateSurface(&surfaceDesc, &m_pPrimarySurface, NULL);
        if (result != DD_OK)
        {
            DebugMsgIfErr("DoSetMode: CreateSurface", result);
            return FALSE;
        }
    }

    // Make sure surfaces were not discarded, or are restored
    RestoreSurfaces();

    if (modeInfo.bitDepth == 8 && m_pPalette)
    {
        // Set the palette for the new surface(s)
        DebugMsgEx(SETMODE, "Setting the palettes...");
        result = m_pPrimarySurface->SetPalette(m_pPalette);
        DebugMsgIfErr("StartMode: SetPalette", result);

        // Set the entries of the palette
        DebugMsgEx(SETMODE, "Setting the palette entries...");
        result = m_pPalette->SetEntries(0, 0, 256, GetPaletteEntries());
        DebugMsgIfErr("StartMode: SetEntries", result);

        if (result != DD_OK)
            return FALSE;
    }

    // Jacobson, 3-5-96
    // Now confirm that we're actually able to perform direct writes
    // and that this is a valid mode by trying to lock the surface for
    // writing.

    result = m_pPrimarySurface->Lock(NULL, &surfaceDesc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);
    DebugMsgIfErr("StartMode: Direct write test failed. Can't do direct mode.",result);

    if (result == DDERR_SURFACELOST)
    {
        RestoreSurfaces();
        result = m_pPrimarySurface->Lock(NULL, &surfaceDesc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);
    }

    if (result != DD_OK)
    {
#line 532
        CriticalMsg1("Failed to lock primary DirectDraw surface (%s)", WhatDDError(result));
        return FALSE;
    }
    else
        m_pPrimarySurface->Unlock(NULL);

    // Get the back buffer, if any.  Store pointer in secondary surface
    DDSCAPS2 ddsCaps;
    ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
    result = m_pPrimarySurface->GetAttachedSurface(&ddsCaps, &m_pSecondarySurface);

    // DebugMsgIfErr("DoSetMode: Couldn't Find a back buffer", result);

    // Get the descriptors
    DebugMsgEx(SETMODE, "Getting the descriptors...");
    m_PrimaryDesc.dwSize = sizeof(m_PrimaryDesc);
    result = m_pPrimarySurface->GetSurfaceDesc(&m_PrimaryDesc);

    if (result != DD_OK)
    {
        DebugMsgIfErr("DoSetMode: GetSurfaceDesc", result);
        return FALSE;
    }

    DebugMsgDesc(m_PrimaryDesc);

    if (m_pSecondarySurface)
    {
        // Get the secondary descriptor
        m_SecondaryDesc.dwSize = sizeof(m_SecondaryDesc);
        result = m_pSecondarySurface->GetSurfaceDesc(&m_SecondaryDesc);

        if (result != DD_OK)
        {
            DebugMsgIfErr("DoSetMode::GetSurfaceDesc", result);
            return FALSE;
        }

        DebugMsgDesc(m_SecondaryDesc);
        m_bFlip = TRUE;
    }
    else
        m_bFlip = FALSE;

    SetModeInfoFromSurfaceDesc((!m_pSecondarySurface) ? m_PrimaryDesc : m_SecondaryDesc,
                               (flags & kGrSetWindowed) ? (kGrModeIsWindowed | kGrModeIsDirect) : kGrModeIsDirect,
                               pReturnModeInfo);

    DebugMsgEx(SETMODE, "Clearing the new surface...");
    if (!(m_ModeInfoEx.flags & kGrModeIsWindowed))
    {
        WipeSurface(m_pPrimarySurface);
        WipeSurface(m_pSecondarySurface);
    }

    //
    // @Note (toml 06-03-96): The flipping model used in this implementation
    // essentially hides the flipping from the client.  If a back buffer is
    // available, the game is pointed at that.  Otherwise, it points at
    // the front, visible buffer.
    //
    pReturnModeInfo->vbase2 = NULL;

    return TRUE;
    END_DEBUG;
}

static void lock_bitmap_from_surface(grs_bitmap *bm, LPDIRECTDRAWSURFACE4 surf)
{
   DDSURFACEDESC2 surfaceDesc;
   HRESULT result;

   surfaceDesc.dwSize = sizeof(surfaceDesc);

   result = surf->GetSurfaceDesc(&surfaceDesc);

   DebugMsgIfErr("lock_bitmap_from_surface(): GetSurfaceDesc failed.", result);

   if (surfaceDesc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)
       // Hack Hack Hack Hack!!!!
       memcpy(bm, grd_screen_canvas, sizeof(*bm));
   else
       gr_init_bitmap(bm, NULL, 
           BMT_OLD_DEVICE_VIDMEM_TO_FLAT, 0,
           bm->w, bm->h);

   result = surf->Lock(NULL, &surfaceDesc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);

   DebugMsgIfErr("lock_bitmap_from_surface(): Lock failed.", result);

   bm->bits = (uchar *)surfaceDesc.lpSurface;
}
///////////////////////////////////////


// when our apps can repaint themselves, we should get rid of this
#define PAUSE_BLIT

void cDirectDDModeOps::PauseBlit(LPDIRECTDRAWSURFACE4 pDest,LPDIRECTDRAWSURFACE4 pSource)
{
#ifdef PAUSE_BLIT
    // Jacobson, 3-4-96
    // This function blits back and forth from the pause blit buffer.

    BEGIN_DEBUG_MSG2("cDirectDDModeOps::PauseBlit(0x%x, 0x%x)", pDest, pSource);

    if (pDest && pSource)
    {

        // Blit from secondary surface to primary surface
        RECT blitRect;

        blitRect.left = 0;
        blitRect.top = 0;
        blitRect.right = m_PauseDesc.dwWidth;
        blitRect.bottom = m_PauseDesc.dwHeight;

        int iLock = m_pDisplayDevice->BreakLock();
        HRESULT result = pDest->Blt(&blitRect, pSource, &blitRect, DDBLT_WAIT, NULL);

        if (result == DDERR_NOBLTHW) { // use 2d to blit
            grs_bitmap src_bm, dst_bm;
            grs_canvas dst_cnv;
            src_bm.w = dst_bm.w = (short )m_PauseDesc.dwWidth;
            src_bm.h = dst_bm.h = (short )m_PauseDesc.dwHeight;

            lock_bitmap_from_surface(&dst_bm, pDest);
            lock_bitmap_from_surface(&src_bm, pSource);

            gr_make_canvas(&dst_bm, &dst_cnv);
            gr_push_canvas(&dst_cnv);
            gr_bitmap(&src_bm, 0, 0);
            gr_pop_canvas();
            pSource->Unlock(NULL);
            pDest->Unlock(NULL);
        }

        m_pDisplayDevice->RestoreLock(iLock);

        DebugMsgIfErr("Pause Blit -> 1", result);
    }

    END_DEBUG;
#endif
}

///////////////////////////////////////

BOOL cDirectDDModeOps::DoSetFlip(int bFlip)
{
    int count = m_pDisplayDevice->BreakLock();
#ifndef SHIP
    if (m_bHIW)
        m_bFlip = TRUE;
    else
#endif
        m_bFlip = bFlip && m_pSecondarySurface != NULL;
    m_pDisplayDevice->RestoreLock(count);
    return m_bFlip;
}

///////////////////////////////////////

BOOL cDirectDDModeOps::OnTaskSwitch(BOOL bActive)
{
    static BOOL bRestored;

    if (!bActive)
    {
        if (bRestored)
            bRestored = FALSE;
        return FALSE;
    }
    if (bRestored)
        return FALSE;
    bRestored = TRUE;
    RestoreSurfaces();
    return TRUE;
}

///////////////////////////////////////

BOOL cDirectDDModeOps::DoStatHTrace()
{
    DoPageFlip();
    return FALSE;
}

///////////////////////////////////////

BOOL cDirectDDModeOps::DoPageFlip()
{
    DebugMsgEx(VERBOSE, "PageFlip");

#ifndef SHIP
    if (m_bHIW)
    {
        HRESULT result;
        int iLock;
        RECT sScreenRect;

        RestoreSurfaces();
        GetClientRect(m_pOuter->GetMainWnd(), &sScreenRect);
        ClientToScreen(m_pOuter->GetMainWnd(), (POINT *)&sScreenRect.left);
        ClientToScreen(m_pOuter->GetMainWnd(), (POINT *)&sScreenRect.right);
        iLock = m_pDisplayDevice->BreakLock();
        while (1)
        {
            result = m_pPrimarySurface->Blt(&sScreenRect, m_pSecondarySurface, NULL, DDFLIP_WAIT, NULL);
            if (result == DD_OK)
                break;
            if (result != DDERR_WASSTILLDRAWING)
            {
                DebugMsgIfErr("DoPageFlip: Blt", result);
#line 836
                CriticalMsg("Blt failed!");
                break;
            }
        }
        m_pDisplayDevice->RestoreLock(iLock);
        return NO_ERROR;
    }
#endif

    if (m_pSecondarySurface)
    {
        if (m_bFlip)
        {
            HRESULT result;
            int iLock;

            RestoreSurfaces();
            iLock = m_pDisplayDevice->BreakLock();
            while (1)
            {
                result = m_pPrimarySurface->Flip(NULL, DDFLIP_WAIT);
                if (result == DD_OK)
                    break;
                if (result != DDERR_WASSTILLDRAWING)
                {
                    DebugMsgIfErr("DoPageFlip: Flip", result);
#line 861
                    CriticalMsg("Flip failed!");
                    break;
                }
            }
            m_pDisplayDevice->RestoreLock(iLock);
        }
    }
    return NO_ERROR;
}

///////////////////////////////////////

BOOL cDirectDDModeOps::DoLock(sGrModeCap * pReturnModeInfo)
{
#ifndef SHIP
    static BOOL bShowLockFailWarning = TRUE;
#endif
    AssertMsg(m_pPrimarySurface != NULL, "Attempt to lock uninitialized display device");
    DebugMsgEx(VERBOSE, "cDirectDDModeOps::DoLock()");

    // Make sure surfaces were not discarded, or are restored
    RestoreSurfaces();

    IDirectDrawSurface4 * pClientTarget;
    DDSURFACEDESC2 surfaceDesc;
    HRESULT result;

    memset(&surfaceDesc, 0, sizeof(surfaceDesc));
    surfaceDesc.dwSize = sizeof(surfaceDesc);

#ifndef SHIP
    if (m_bHIW)
    {
        pClientTarget = m_pSecondarySurface;
        result = pClientTarget->Lock(NULL, &surfaceDesc, DDLOCK_WAIT, NULL);
        if (result == DDERR_SURFACELOST)
        {
            RestoreSurfaces();
            result = pClientTarget->Lock(NULL, &surfaceDesc, DDLOCK_WAIT, NULL);
        }
        if (result != DD_OK)
            return FALSE;
        pReturnModeInfo->vbase = (uchar *)surfaceDesc.lpSurface;
        pReturnModeInfo->vbase2 = NULL;
        return !!pReturnModeInfo->vbase;
    }
#endif

    pClientTarget = (m_pSecondarySurface && m_bFlip) ? m_pSecondarySurface : m_pPrimarySurface;

    result = pClientTarget->Lock(NULL, &surfaceDesc, DDLOCK_WAIT, NULL);
    if (result == DDERR_SURFACELOST)
    {
        put_mono('?');
        RestoreSurfaces();
        result = pClientTarget->Lock(NULL, &surfaceDesc, DDLOCK_WAIT, NULL);
    }
    put_mono('.');

    if (result != DD_OK)
    {
#ifndef SHIP
        if (bShowLockFailWarning)
        {
            Warning(("Failed to lock primary DirectDraw surface (%s)\n", WhatDDError(result)));
            bShowLockFailWarning = FALSE;
        }
#endif
        // No way to recover...
        return FALSE;
    }
#ifndef SHIP
    else
        bShowLockFailWarning = TRUE;
#endif


    if (!(m_ModeInfoEx.flags & kGrModeIsWindowed))
    {
        pReturnModeInfo->vbase = (uchar *)surfaceDesc.lpSurface;
    }
    else
    {
        POINT originClient = { 0, 0 };
        ClientToScreen(m_pOuter->GetMainWnd(), &originClient);

        pReturnModeInfo->vbase = (((uchar *)surfaceDesc.lpSurface) + (originClient.y * m_ModeInfoEx.rowBytes)) + (originClient.x * (m_ModeInfoEx.depth >> 3));
    }

    pReturnModeInfo->vbase2   = NULL;

    return !!pReturnModeInfo->vbase;
}

///////////////////////////////////////

BOOL cDirectDDModeOps::DoUnlock()
{
    DebugMsgEx(VERBOSE, "cDirectDDModeOps::DoUnlock()");

    // Make sure surfaces were not discarded, or are restored
    RestoreSurfaces();

    HRESULT result;

    IDirectDrawSurface4 * pClientTargetSurface = (m_pSecondarySurface && m_bFlip) ? m_pSecondarySurface : m_pPrimarySurface;
    if (pClientTargetSurface != NULL)
    {
        result = pClientTargetSurface->Unlock(NULL);
        DebugMsgIfErr("DoLock::Unlock client surface", result);

        if (result != DD_OK)
        {



            CriticalMsg1("Lock count of DirectDraw surfaces appears corrupted (%s)", WhatDDError(result));
            // No way to recover...
            SafeRelease(m_pSecondarySurface);
            SafeRelease(m_pPrimarySurface);
            SafeRelease(m_pPalette);
            exit(0);
        }
    }

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//
// class cPhoney512ModeOps
//

BOOL cPhoney512ModeOps::StartMode(const sGrModeInfo & modeInfo, int flags, sGrModeCap * pReturnModeInfo)
{
    sGrModeInfo realModeInfo = modeInfo;

    realModeInfo.w = 640;
    realModeInfo.h = 480;

    if (cDirectDDModeOps::StartMode(realModeInfo, flags, pReturnModeInfo))
    {
        pReturnModeInfo->w = modeInfo.w;
        pReturnModeInfo->h = modeInfo.h;
        return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////

BOOL cPhoney512ModeOps::DoLock(sGrModeCap * pReturnModeInfo)
{
    pReturnModeInfo->vbase = NULL;
    pReturnModeInfo->vbase2 = NULL;
    if (cDirectDDModeOps::DoLock(pReturnModeInfo))
    {
        pReturnModeInfo->vbase = (((uchar *)pReturnModeInfo->vbase) + (48 * m_ModeInfoEx.rowBytes)) + (64 * (m_ModeInfoEx.depth >> 3));
        return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////

BOOL cPhoney512ModeOps::DoUnlock()
{
    return cDirectDDModeOps::DoUnlock();
}

///////////////////////////////////////////////////////////////////////////////
//
// class cOffVideoDDModeOps
//

BOOL cOffVideoDDModeOps::StartMode(const sGrModeInfo & modeInfo, int flags, sGrModeCap * pReturnModeInfo)
{
    HRESULT result;
    BEGIN_DEBUG_MSG("cOffVideoDDModeOps::StartMode()");

    const int realBitsPerPixel = (modeInfo.bitDepth != 15) ? modeInfo.bitDepth : 16;

    // Release the previous mode, if any
    DebugMsgEx(SETMODE, "Releasing previous surfaces, if any...");
    SafeRelease(m_pPrimarySurface);          // Sets m_pPrimarySurface to null
    SafeRelease(m_pSecondarySurface);

    // If we're not running in a window, set the screen display mode to the best available
    if (!(GetCoopFlags() & DDSCL_NORMAL))
    {
        int targetMode = -1;

        // First look for an exact match, if we're not trying a ModeX mode
        if (modeInfo.w > 400)
        {
            targetMode = cDisplayDevice::GetAvailableMode(modeInfo.w, modeInfo.h, modeInfo.bitDepth);
            DebugMsgTrueEx(SETMODE, targetMode != -1, "Picked exact match");
        }

        // Next try a perfect doubling
        if (targetMode == -1)
        {
            targetMode = cDisplayDevice::GetAvailableMode(modeInfo.w * 2, modeInfo.h * 2, modeInfo.bitDepth);
            DebugMsgTrueEx(SETMODE, targetMode != -1, "Picked double w and double h");
        }

        // Next try a double of the horizontal axis
        if (targetMode == -1)
        {
            targetMode = cDisplayDevice::GetAvailableMode(modeInfo.w * 2, modeInfo.h, modeInfo.bitDepth);
            DebugMsgTrueEx(SETMODE, targetMode != -1, "Picked double w and double h");
        }

        // Finally, just choose 640x480
        if (targetMode == -1)
        {
            targetMode = cDisplayDevice::GetAvailableMode(640, 480, modeInfo.bitDepth);
            DebugMsgTrueEx(SETMODE, targetMode != -1, "Picked highest res");
        }

        if (targetMode == -1)
            return FALSE;

        // Now set the mode
        const sGrModeInfo & targetModeInfo = cDisplayDevice::EnumModeToModeInfo(targetMode);
        result = SetDisplayMode(targetModeInfo.w, targetModeInfo.h, targetModeInfo.bitDepth);

        if (result != DD_OK)
        {
            DebugMsgIfErr("DoSetMode::SetDisplayMode", result);
            return FALSE;
        }
    }

    // Create the primary ("real") surface
    DebugMsgEx(SETMODE, "Creating primary surface...");

    DDSURFACEDESC2 surfaceDesc;
    memset(&surfaceDesc, 0, sizeof(surfaceDesc));
    surfaceDesc.dwSize = sizeof(surfaceDesc);

    surfaceDesc.dwFlags = DDSD_CAPS;
    surfaceDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    result = GetDD()->CreateSurface(&surfaceDesc, &m_pPrimarySurface, NULL);
    if (result != DD_OK)
    {
        DebugMsgIfErr("DoSetMode::CreateSurface (primary)", result);
        return FALSE;
    }

    // Get the primary descriptor
    m_PrimaryDesc.dwSize = sizeof(m_PrimaryDesc);
    result = m_pPrimarySurface->GetSurfaceDesc(&m_PrimaryDesc);

    if (result != DD_OK)
    {
        DebugMsgIfErr("DoSetMode::GetSurfaceDesc", result);
        return FALSE;
    }

    DebugMsgDesc(m_PrimaryDesc);

    // Create the secondary ("spoofed") surface
    //
    // What about a 2 surface flip-on-endframe approach?
    //
    DebugMsgEx(SETMODE, "Creating secondary surface...");

    memset(&surfaceDesc, 0, sizeof(surfaceDesc));
    surfaceDesc.dwSize = sizeof(surfaceDesc);
    surfaceDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    surfaceDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

    surfaceDesc.dwWidth = modeInfo.w;
    surfaceDesc.dwHeight = modeInfo.h;

    surfaceDesc.dwFlags |= DDSD_PIXELFORMAT;
    surfaceDesc.ddpfPixelFormat.dwSize = sizeof(surfaceDesc.ddpfPixelFormat);
    result = m_pPrimarySurface->GetPixelFormat(&surfaceDesc.ddpfPixelFormat);
    DebugMsgIfErr("m_pPrimarySurface->GetPixelFormat()", result);

    result = GetDD()->CreateSurface(&surfaceDesc, &m_pSecondarySurface, NULL);
    DebugMsgIfErr("DoSetMode::CreateSurface (secondary 1)", result);

    if (result != DD_OK)
    {
        Warning(("Failed to create offscreen DirectDraw surface (%d: %s)\n", result, WhatDDError(result)));
        return FALSE;
    }

    // Get the secondary descriptor
    m_SecondaryDesc.dwSize = sizeof(m_SecondaryDesc);
    result = m_pSecondarySurface->GetSurfaceDesc(&m_SecondaryDesc);

    if (result != DD_OK)
    {
        DebugMsgIfErr("DoSetMode::GetSurfaceDesc", result);
        return FALSE;
    }

    DebugMsgDesc(m_SecondaryDesc);

    // Store info about the render target
    SetModeInfoFromSurfaceDesc(m_SecondaryDesc,
                               (flags & kGrSetWindowed) ? kGrModeIsWindowed : 0,
                               pReturnModeInfo);

    // Make sure surfaces were not discarded, or are restored
    RestoreSurfaces();

    // Set the palette for the new surface(s)
    if (m_pPalette)
    {
        DebugMsgEx(SETMODE, "Setting palettes...");
        result = m_pPrimarySurface->SetPalette(m_pPalette);
        DebugMsgIfErr("DoSetMode::SetPalette (primary)", result);

        // Set the entries of the palette
        result = m_pPalette->SetEntries(0, 0, 256, GetPaletteEntries());
    }


    if (result != DD_OK)
    {
        DebugMsgIfErr("DoSetMode::SetEntries", result);
        return FALSE;
    }

    // Note: The paint request to the primary surface may fail for
    // certain hardware boards that are unable to lock the primary
    // surface. But that's ok here.
    if (!(m_ModeInfoEx.flags & kGrModeIsWindowed))
    {
        if (!IsModeX())
            WipeSurface(m_pPrimarySurface);
        WipeSurface(m_pSecondarySurface);
    }
    else
        SetClipToWindow(TRUE);

    // Get the linear address of the video
    pReturnModeInfo->vbase2   = NULL;

    return TRUE;
    END_DEBUG;
}

///////////////////////////////////////

void cOffVideoDDModeOps::DoFlushRect(int x0Source, int y0Source, int x1Source, int y1Source)
{
    DebugMsgEx(VERBOSE, "DoFlushRect()");

    // Make sure surfaces were not discarded, or are restored
    RestoreSurfaces();

    // Blit from secondary surface to primary surface
    RECT sourceRect;
    RECT destRect;

    // Adjust rectangle to look good when window is halved or doubled
    sourceRect.left   = x0Source - (x0Source & 1);
    sourceRect.top    = y0Source - (y0Source & 1);
    sourceRect.right  = x1Source + (x1Source & 1);
    sourceRect.bottom = y1Source + (y1Source & 1);

    // Clip the rectangles
    if (sourceRect.left < 0)
        sourceRect.left = 0;
    if (sourceRect.top < 0)
        sourceRect.top = 0;
    if (sourceRect.right > m_ModeInfoEx.width)
        sourceRect.right = m_ModeInfoEx.width;
    if (sourceRect.bottom > m_ModeInfoEx.height)
        sourceRect.bottom = m_ModeInfoEx.height;

    if (sourceRect.right - sourceRect.left > 0 && sourceRect.bottom - sourceRect.top > 0)
    {
        destRect.left   = Scale(sourceRect.left);
        destRect.top    = Scale(sourceRect.top);
        destRect.right  = Scale(sourceRect.right);
        destRect.bottom = Scale(sourceRect.bottom);

        // Move destination to right place on screen
        if (m_ModeInfoEx.flags & kGrModeIsWindowed)
        {
            POINT originClient = { 0, 0 };
            ClientToScreen(m_pOuter->GetMainWnd(), &originClient);

            destRect.left   += originClient.x;
            destRect.top    += originClient.y;
            destRect.right  += originClient.x;
            destRect.bottom += originClient.y;
        }
        else
        {
            const int xScale = (m_PrimaryDesc.dwWidth / m_SecondaryDesc.dwWidth) - 1;
            const int yScale = (m_PrimaryDesc.dwHeight / m_SecondaryDesc.dwHeight) - 1;

            if (xScale < 0)
            {
                destRect.left >>= -xScale;
                destRect.right >>= -xScale;
            }
            else
            {
                destRect.left <<= xScale;
                destRect.right <<= xScale;
            }

            if (yScale < 0)
            {
                destRect.top >>= -yScale;
                destRect.bottom >>= -yScale;
            }
            else
            {
                destRect.top <<= yScale;
                destRect.bottom <<= yScale;
            }
        }

        // Now blit!
        int iLock = m_pDisplayDevice->BreakLock();

        // Hack to address jerky Windows mouse cursor (see http://www.microsoft.com/mediadev/overviews/winfaq.htm (toml 12-30-96))
        {
            RECT rectClient;
            POINT pt;

            GetClientRect(m_pOuter->GetMainWnd(), &rectClient);
            MapWindowPoints(m_pOuter->GetMainWnd(), NULL, (POINT *) &rectClient, 2);
            GetCursorPos(&pt);

            if (!PtInRect(&rectClient, pt))
            {
                SetCursorPos(pt.x, pt.y);
            }
        }

        HRESULT result = m_pPrimarySurface->Blt(&destRect, m_pSecondarySurface, &sourceRect, DDBLT_WAIT, NULL);
        if (result == DDERR_SURFACELOST)
        {
            RestoreSurfaces();
            result = m_pPrimarySurface->Blt(&destRect, m_pSecondarySurface, &sourceRect, DDBLT_WAIT, NULL);
        }
        m_pDisplayDevice->RestoreLock(iLock);

        DebugMsgIfErr("Blit SecondaryBuffer to Primary", result);
    }
}

///////////////////////////////////////

void cOffVideoDDModeOps::DoFlush()
{
    DoFlushRect(0, 0, m_SecondaryDesc.dwWidth, m_SecondaryDesc.dwHeight);
}

///////////////////////////////////////

BOOL cOffVideoDDModeOps::DoLock(sGrModeCap * pReturnModeInfo)
{
    AssertMsg(m_pPrimarySurface != NULL, "Attempt to lock uninitialized display device");
    AssertMsg(m_pSecondarySurface != NULL, "Attempt to lock uninitialized display device");

    DebugMsgEx(VERBOSE, "cOffVideoDDModeOps::DoLock()");

    // Make sure surfaces were not discarded, or are restored
    RestoreSurfaces();

    HRESULT result;
    DDSURFACEDESC2 surfaceDesc;
    memset(&surfaceDesc, 0, sizeof(surfaceDesc));
    surfaceDesc.dwSize = sizeof(surfaceDesc);

    result = m_pSecondarySurface->Lock(NULL, &surfaceDesc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);
#if 1
    m_pSecondarySurface->Unlock(NULL);
#endif
    DebugMsgIfErr("DoLock::Lock SecondaryBuffer", result);

    if (result == DDERR_SURFACELOST)
    {
        RestoreSurfaces();
        result = m_pSecondarySurface->Lock(NULL, &surfaceDesc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);
#if 1
        m_pSecondarySurface->Unlock(NULL);
#endif
    }

    if (result != DD_OK)
    {
        CriticalMsg("Lock count of DirectDraw surfaces appears corrupted");
        // No way to recover...
        SafeRelease(m_pSecondarySurface);
        SafeRelease(m_pPrimarySurface);
        SafeRelease(m_pPalette);
        exit(0);
    }
    pReturnModeInfo->vbase    = (uchar *)surfaceDesc.lpSurface;
    pReturnModeInfo->vbase2   = NULL;

    return !!pReturnModeInfo->vbase;
}

///////////////////////////////////////

BOOL cOffVideoDDModeOps::DoUnlock()
{
    RestoreSurfaces();
    if (m_bLocked && m_pSecondarySurface)
        m_pSecondarySurface->Unlock(NULL);
    return TRUE;
}

///////////////////////////////////////

BOOL cOffVideoDDModeOps::DoSetFlip(int bFlip)
{
    int count = m_pDisplayDevice->BreakLock();
    m_bFlipOnFlush = bFlip;
    m_pDisplayDevice->RestoreLock(count);
    return m_bFlipOnFlush;
}

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cFixedDDModeOps
//


extern int g_nD3DVSync;
EXTERN RECT g_rcUIScaleView;
EXTERN BOOL g_bUIScaleMode;
EXTERN BOOL g_bUIScaleView;
EXTERN BOOL g_bUIScaleDrawn;
BOOL D3DFitWindowToMonitor(HWND hwnd, int height);

static BOOL g_bFixedRestored;
static BOOL g_bFixedLockOK;

cFixedDDModeOps::~cFixedDDModeOps()
{
    ReleaseFilteredBlit();
    SafeRelease(m_pOverlaySurface);
    SafeRelease(m_pBackBuffer);
    SafeRelease(m_pScaleSurface);
    SafeRelease(m_pGameShell);
}

void cFixedDDModeOps::ReleaseFilteredBlit()
{
    SafeRelease(m_pTexture);
    SafeRelease(m_pTexSurface);
    SafeRelease(m_pViewport);
    SafeRelease(m_pD3DDevice);
    SafeRelease(m_pRenderTarget);
    SafeRelease(m_pD3D);
}

BOOL cFixedDDModeOps::OnTaskSwitch(BOOL bActive)
{
    if (!bActive)
    {
        if (g_bFixedRestored)
            g_bFixedRestored = FALSE;
        return FALSE;
    }
    if (g_bFixedRestored)
        return FALSE;
    g_bFixedRestored = TRUE;
    RestoreSurfaces();
    return TRUE;
}

BOOL cFixedDDModeOps::DoStatHTrace()
{
    if (m_pBackBuffer)
        DoPageFlip();
    return FALSE;
}

void cFixedDDModeOps::RestoreSurfaces()
{
    if (m_pPrimarySurface->IsLost())
        m_pPrimarySurface->Restore();
    if (m_pSecondarySurface && m_pSecondarySurface->IsLost())
        m_pSecondarySurface->Restore();
    if (m_pScaleSurface && m_pScaleSurface->IsLost())
        m_pScaleSurface->Restore();
    if (m_pBackBuffer && m_pBackBuffer->IsLost())
        m_pBackBuffer->Restore();
    if (m_pTexSurface && m_pTexSurface->IsLost())
    {
        SafeRelease(m_pTexture);
        if (m_pTexSurface->Restore() == DD_OK)
            m_pTexSurface->QueryInterface(IID_IDirect3DTexture2, (void **) &m_pTexture);
    }
    if (m_pRenderTarget && m_pRenderTarget->IsLost())
        m_pRenderTarget->Restore();
}

void cFixedDDModeOps::ThrottleFrame()
{
    if (!m_nFrameCapUs)
        return;
    unsigned __int64 now = (unsigned __int64) timeGetTime() * 1000;
    DWORD elapsed = (now >= m_LastFrameUs) ? (DWORD)(now - m_LastFrameUs) : (DWORD)(now - m_LastFrameUs) - 999;
    if (elapsed + 1000 < m_nFrameCapUs)
    {
        Sleep((m_nFrameCapUs - elapsed) / 1000);
        now = (unsigned __int64) timeGetTime() * 1000;
    }
    m_LastFrameUs = now;
}

BOOL cFixedDDModeOps::StartMode(const sGrModeInfo & modeInfo, int flags, sGrModeCap * pReturnModeInfo)
{
    DDSURFACEDESC2 desc;
    DDSCAPS2 caps;
    float fps;
    int nCount;
    int nBackBuffers;

    m_bOwnSecondary = FALSE;
    ReleaseFilteredBlit();
    if (!m_bPersistent)
    {
        SafeRelease(m_pBackBuffer);
        SafeRelease(m_pPrimarySurface);
    }
    SafeRelease(m_pSecondarySurface);
    SafeRelease(m_pScaleSurface);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);

    m_nFrameCapUs = 0;
    nCount = 1;
    if (config_get_value("framerate_cap", CONFIG_FLOAT_TYPE, (config_valptr) &fps, &nCount) && nCount > 0 && fps >= 1.0f)
    {
        m_nFrameCapUs = (DWORD)(1000000.0f / fps);
        if (m_nFrameCapUs)
        {
            if (!m_pGameShell)
                m_pGameShell = AppGetObj(IGameShell);
            unsigned __int64 now = (unsigned __int64) timeGetTime() * 1000;
            m_LastFrameUs = now - m_nFrameCapUs;
            LogMsg1("sw framerate cap requested (%d us)", m_nFrameCapUs);
        }
    }

    if (!m_pPrimarySurface)
    {
        if (!(GetCoopFlags() & DDSCL_FULLSCREEN))
            D3DFitWindowToMonitor(m_pOuter->GetMainWnd(), m_DesktopMode.h);
        else
        {
            int depth = m_DesktopMode.bitDepth;
            if (depth == 15)
                depth = 16;
            if (SetDisplayMode(m_DesktopMode.w, m_DesktopMode.h, depth) != DD_OK)
                return FALSE;
        }
    }

    pReturnModeInfo->w = modeInfo.w;
    pReturnModeInfo->h = modeInfo.h;

    BOOL bFlipChain = !!(GetCoopFlags() & DDSCL_FULLSCREEN);
    nBackBuffers = 0;
    if (bFlipChain)
    {
        nCount = 1;
        config_get_value("single_display_mode", CONFIG_INT_TYPE, (config_valptr) &nBackBuffers, &nCount);
        if (nBackBuffers < 2)
        {
            bFlipChain = FALSE;
            nBackBuffers = 0;
        }
        else
        {
            if (nBackBuffers > 3)
                nBackBuffers = 3;
            nBackBuffers--;
        }
    }

    BOOL bWindowedScale = !(m_pDisplayDevice->GetCreateFlags() & kGrSetWindowed) && m_nScaleMode == 2 &&
                          (modeInfo.w != m_DesktopMode.w || modeInfo.h != m_DesktopMode.h);

    if (!m_pPrimarySurface)
    {
        for (;;)
        {
            desc.dwFlags = DDSD_CAPS;
            desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
            if (bFlipChain)
            {
                desc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
                desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_VIDEOMEMORY;
                desc.dwBackBufferCount = nBackBuffers;
            }
            if (GetDD()->CreateSurface(&desc, &m_pPrimarySurface, NULL) == DD_OK)
                break;
            if (!bFlipChain)
                return FALSE;
            bFlipChain = FALSE;
            nBackBuffers = 0;
        }
        if (bFlipChain)
        {
            memset(&caps, 0, sizeof(caps));
            caps.dwCaps = DDSCAPS_BACKBUFFER;
            m_pPrimarySurface->GetAttachedSurface(&caps, &m_pBackBuffer);
        }
    }

    m_PrimaryDesc.dwSize = sizeof(m_PrimaryDesc);
    if (m_pPrimarySurface->GetSurfaceDesc(&m_PrimaryDesc) != DD_OK)
        return FALSE;

    if (!m_pSecondarySurface)
    {
        DDSURFACEDESC2 sdesc;
        memset(&sdesc, 0, sizeof(sdesc));
        sdesc.dwSize = sizeof(sdesc);
        sdesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
        if (!bWindowedScale)
            sdesc.ddsCaps.dwCaps = nBackBuffers ? DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY : DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE;
        else
            sdesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE;
        sdesc.dwWidth = modeInfo.w;
        sdesc.dwHeight = modeInfo.h;
        sdesc.ddpfPixelFormat = m_PrimaryDesc.ddpfPixelFormat;
        int depth = modeInfo.bitDepth;
        if (depth == 15)
            depth = 16;
        if (!bWindowedScale && depth == 16 && sdesc.ddpfPixelFormat.dwRGBBitCount != 16)
        {
            sdesc.ddpfPixelFormat.dwRGBBitCount = 16;
            sdesc.ddpfPixelFormat.dwRBitMask = 0xf800;
            sdesc.ddpfPixelFormat.dwGBitMask = 0x7e0;
            sdesc.ddpfPixelFormat.dwBBitMask = 0x1f;
        }
        GetDD()->CreateSurface(&sdesc, &m_pSecondarySurface, NULL);
        if (!bWindowedScale && !nBackBuffers &&
            (depth != m_DesktopMode.bitDepth || modeInfo.w != m_DesktopMode.w || modeInfo.h != m_DesktopMode.h))
        {
            sdesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE | DDSCAPS_LOCALVIDMEM;
            sdesc.ddpfPixelFormat = m_PrimaryDesc.ddpfPixelFormat;
            GetDD()->CreateSurface(&sdesc, &m_pScaleSurface, NULL);
        }
        m_bOwnSecondary = TRUE;
    }

    if (!m_pSecondarySurface)
    {
        memset(&m_SecondaryDesc, 0, sizeof(m_SecondaryDesc));
        m_bFlip = FALSE;
    }
    else
    {
        m_SecondaryDesc.dwSize = sizeof(m_SecondaryDesc);
        if (m_pSecondarySurface->GetSurfaceDesc(&m_SecondaryDesc) != DD_OK)
            return FALSE;
        m_bFlip = !m_bOwnSecondary;
    }

    LogMsg10("DDProviderFixed StartMode: smode %dx%dx%d ; mode %dx%dx%d (%x %x %d %dD)",
             m_DesktopMode.w, m_DesktopMode.h, m_DesktopMode.bitDepth, modeInfo.w, modeInfo.h, modeInfo.bitDepth,
             m_pDisplayDevice->GetCreateFlags(), flags, m_pScaleSurface != NULL, bWindowedScale + 2);

    SetModeInfoFromSurfaceDesc(m_SecondaryDesc, (flags & kGrSetWindowed) | kGrSetFullScreen, pReturnModeInfo);
    RestoreSurfaces();

    if (modeInfo.bitDepth == 8 && m_pPalette)
    {
        m_pPrimarySurface->SetPalette(m_pPalette);
        if (m_pPalette->SetEntries(0, 0, 256, GetPaletteEntries()) != DD_OK)
            return FALSE;
    }

    if (nBackBuffers)
        InitFilteredBlit();

    if (m_Flags & kClippingToWindow)
        SetClipToWindow(TRUE);

    pReturnModeInfo->vbase = NULL;
    return TRUE;
}

BOOL cFixedDDModeOps::InitFilteredBlit()
{
    DDSURFACEDESC2 desc;
    D3DVIEWPORT2 vp;
    D3DMATRIX mat;
    float fAdjust[2];
    int nCount;
    int iDriver;

    if (m_pD3D)
        return TRUE;

    int nDrivers = lgd3d_enumerate_devices();
    if (nDrivers < 1)
        return FALSE;
    nCount = 1;
    iDriver = 0;
    if (config_get_value("d3d_driver_index", CONFIG_INT_TYPE, (config_valptr) &iDriver, &nCount) && nCount > 0)
    {
        if (iDriver < 0)
            iDriver += nDrivers;
        if (iDriver < 0 || iDriver >= nDrivers)
            iDriver = 0;
    }

    if (GetDD()->QueryInterface(IID_IDirect3D3, (void **) &m_pD3D) != DD_OK)
        return FALSE;

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    desc.dwWidth = m_DesktopMode.w;
    desc.dwHeight = m_DesktopMode.h;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE;
    desc.ddpfPixelFormat = m_PrimaryDesc.ddpfPixelFormat;
    if (GetDD()->CreateSurface(&desc, &m_pRenderTarget, NULL) != DD_OK ||
        m_pD3D->CreateDevice(lgd3d_get_device_info(iDriver)->device_guid, m_pRenderTarget, &m_pD3DDevice, NULL) != D3D_OK ||
        m_pD3D->CreateViewport(&m_pViewport, NULL) != D3D_OK ||
        m_pD3DDevice->AddViewport(m_pViewport) != D3D_OK)
        goto fail;

    memset(&vp, 0, sizeof(vp));
    vp.dwSize = sizeof(vp);
    vp.dwWidth = desc.dwWidth;
    vp.dvClipX = -1.0f;
    vp.dvClipY = -1.0f;
    vp.dvClipWidth = 2.0f;
    vp.dvClipHeight = 2.0f;
    vp.dvMaxZ = 1.0f;
    vp.dwHeight = desc.dwHeight;
    if (m_pViewport->SetViewport2(&vp) != D3D_OK || m_pD3DDevice->SetCurrentViewport(m_pViewport) != D3D_OK)
        goto fail;

    memset(&mat, 0, sizeof(mat));
    mat._11 = mat._22 = mat._33 = mat._44 = 1.0f;
    m_pD3DDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &mat);
    m_pD3DDevice->SetTransform(D3DTRANSFORMSTATE_VIEW, &mat);
    m_pD3DDevice->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &mat);
    m_pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);
    m_pD3DDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_NONE);
    m_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    m_pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    m_pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
    m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    m_pD3DDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
    m_pD3DDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
    m_pD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    m_pD3DDevice->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    desc.dwHeight = m_SecondaryDesc.dwHeight;
    desc.dwWidth = m_SecondaryDesc.dwWidth;
    desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    desc.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
    desc.ddpfPixelFormat = m_SecondaryDesc.ddpfPixelFormat;
    if (GetDD()->CreateSurface(&desc, &m_pTexSurface, NULL) != DD_OK ||
        m_pTexSurface->QueryInterface(IID_IDirect3DTexture2, (void **) &m_pTexture) != DD_OK)
        goto fail;

    m_fTexelAdjustX = 0.5f;
    m_fTexelAdjustY = 0.5f;
    fAdjust[0] = 0.5f;
    fAdjust[1] = 0.5f;
    nCount = 2;
    if (config_get_value("texel_center_adjust", CONFIG_FLOAT_TYPE, (config_valptr) fAdjust, &nCount) && nCount > 0)
    {
        if (nCount == 1)
            fAdjust[1] = fAdjust[0];
        m_fTexelAdjustX = fAdjust[0];
        m_fTexelAdjustY = fAdjust[1];
        if (fAdjust[0] < 0.0f)
            m_fTexelAdjustX = 0.0f;
        else if (fAdjust[0] > 1.0f)
            m_fTexelAdjustX = 1.0f;
        if (fAdjust[1] < 0.0f)
            m_fTexelAdjustY = 0.0f;
        else if (fAdjust[1] > 1.0f)
            m_fTexelAdjustY = 1.0f;
    }
    return TRUE;

fail:
    ReleaseFilteredBlit();
    return FALSE;
}

void cFixedDDModeOps::FilteredBlit(RECT * pDest)
{
    D3DTLVERTEX v[4];
    float u0 = m_fTexelAdjustX / m_SecondaryDesc.dwWidth;
    float v0 = m_fTexelAdjustY / m_SecondaryDesc.dwHeight;

    m_pTexSurface->Blt(NULL, m_pSecondarySurface, NULL, DDBLT_WAIT, NULL);
    memset(v, 0, sizeof(v));
    for (int i = 0; i < 4; i++)
    {
        v[i].rhw = 1.0f;
        v[i].color = 0xffffffff;
    }
    v[0].sx = v[3].sx = (float) pDest->left;
    v[1].sx = v[2].sx = (float) pDest->right;
    v[0].sy = v[1].sy = (float) pDest->top;
    v[2].sy = v[3].sy = (float) pDest->bottom;
    v[0].tu = v[3].tu = u0;
    v[1].tu = v[2].tu = 1.0f + u0;
    v[0].tv = v[1].tv = v0;
    v[2].tv = v[3].tv = 1.0f + v0;

    m_pD3DDevice->SetTexture(0, m_pTexture);
    if (m_pD3DDevice->BeginScene() == D3D_OK)
    {
        m_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, v, 4, D3DDP_WAIT);
        m_pD3DDevice->EndScene();
    }
    m_pD3DDevice->SetTexture(0, NULL);
}

BOOL cFixedDDModeOps::DoPageFlip()
{
    BOOL bFullscreen = !!(m_pDisplayDevice->GetCreateFlags() & kGrSetFullScreen);
    BOOL bVSync = bFullscreen ? (g_nD3DVSync & 1) : (g_nD3DVSync & 2);

    if (m_pSecondarySurface && m_bOwnSecondary)
    {
        if (!m_pBackBuffer)
        {
            if (bFullscreen)
                ThrottleFrame();
            if (bVSync)
                GetDD()->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, NULL);
        }
        m_bOwnSecondary = FALSE;
        DoFlush();
        m_bOwnSecondary = TRUE;
    }

    if (m_pBackBuffer)
    {
        if (bFullscreen)
            ThrottleFrame();
        RestoreSurfaces();
        DWORD flipFlags = bVSync ? DDFLIP_WAIT : DDFLIP_WAIT | DDFLIP_NOVSYNC;
        if (bVSync && (g_nD3DVSync & 0xc))
            flipFlags = (DDFLIP_INTERVAL2 << (((g_nD3DVSync >> 2) & 3) - 1)) | DDFLIP_WAIT;
        int count = m_pDisplayDevice->BreakLock();
        HRESULT result;
        while ((result = m_pPrimarySurface->Flip(NULL, flipFlags)) != DD_OK)
        {
            if (result != DDERR_WASSTILLDRAWING)
            {
                CriticalMsg("Flip failed!");
                break;
            }
        }
        m_pDisplayDevice->RestoreLock(count);
    }
    return FALSE;
}

void cFixedDDModeOps::DoFlush()
{
    RECT dest;
    RECT src;
    RECT * pSrc;
    POINT origin = { 0, 0 };

    if (!m_pSecondarySurface || m_bOwnSecondary)
        return;

    RestoreSurfaces();
    HWND hwnd = m_pOuter->GetMainWnd();
    ClientToScreen(hwnd, &origin);
    GetClientRect(hwnd, &dest);
    OffsetRect(&dest, origin.x, origin.y);

    int count = m_pDisplayDevice->BreakLock();
    LPDIRECTDRAWSURFACE4 pTarget = m_pBackBuffer ? m_pBackBuffer : m_pPrimarySurface;
    LPDIRECTDRAWSURFACE4 pSource = m_pSecondarySurface;
    pSrc = NULL;

    if (m_DesktopMode.w != GetModeInfoEx().width || m_DesktopMode.h != GetModeInfoEx().height)
    {
        int w = dest.right - dest.left;
        int h = dest.bottom - dest.top;
        if (g_bUIScaleMode && g_bUIScaleView)
        {
            src = g_rcUIScaleView;
            src.right += src.left;
            src.bottom += src.top;
            pSrc = &src;
            g_bUIScaleDrawn = TRUE;
            int fitH = (g_rcUIScaleView.bottom * w) / g_rcUIScaleView.right;
            if (fitH < h)
            {
                int top = dest.top + (h - fitH) / 2;
                dest.bottom = top + fitH;
                dest.top = top;
            }
            else
            {
                int fitW = (g_rcUIScaleView.right * h) / g_rcUIScaleView.bottom;
                dest.left += (w - fitW) / 2;
                dest.right = dest.left + fitW;
            }
        }
        else if (m_nScaleMode == 1 && !g_bUIScaleMode)
        {
            int scale = (h * 3 < w * 4) ? h / 480 : w / 640;
            if (!scale)
                scale = 1;
            dest.left += (w - scale * 640) / 2;
            dest.right = dest.left + scale * 640;
            dest.top += (h - scale * 480) / 2;
            dest.bottom = dest.top + scale * 480;
        }
        else if (m_DesktopMode.h * 4 != m_DesktopMode.w * 3)
        {
            if (w * 3 < h * 4)
            {
                int fitH = (w * 3) / 4;
                dest.top += (h - fitH) / 2;
                dest.bottom = dest.top + fitH;
            }
            else
            {
                int fitW = (h * 4) / 3;
                dest.left += (w - fitW) / 2;
                dest.right = dest.left + fitW;
            }
        }
        if (m_pD3DDevice)
        {
            FilteredBlit(&dest);
            m_pDisplayDevice->RestoreLock(count);
            return;
        }
    }

    if (m_pScaleSurface)
    {
        m_pScaleSurface->Blt(NULL, pSource, NULL, DDBLT_WAIT, NULL);
        pSource = m_pScaleSurface;
    }
    pTarget->Blt(&dest, pSource, pSrc, DDBLT_WAIT, NULL);
    m_pDisplayDevice->RestoreLock(count);
}

BOOL cFixedDDModeOps::DoLock(sGrModeCap * pModeInfo)
{
    DDSURFACEDESC2 desc;
    HRESULT result;

    pModeInfo->vbase = NULL;
    pModeInfo->vbase2 = NULL;
    RestoreSurfaces();
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);

    LPDIRECTDRAWSURFACE4 pSurface = m_pSecondarySurface;
    if (!pSurface)
        pSurface = m_pBackBuffer ? m_pBackBuffer : m_pPrimarySurface;

    result = pSurface->Lock(NULL, &desc, DDLOCK_WAIT, NULL);
    if (result == DDERR_SURFACELOST)
    {
        RestoreSurfaces();
        result = pSurface->Lock(NULL, &desc, DDLOCK_WAIT, NULL);
    }
    if (result != DD_OK)
    {
        if (!m_bLockBack && g_bFixedLockOK)
            g_bFixedLockOK = FALSE;
        return FALSE;
    }
    pModeInfo->rowBytes = desc.lPitch;
    pModeInfo->vbase = (uchar *) desc.lpSurface;
    if (!m_bLockBack)
        g_bFixedLockOK = TRUE;
    return desc.lpSurface != NULL;
}

BOOL cFixedDDModeOps::DoUnlock()
{
    RestoreSurfaces();
    LPDIRECTDRAWSURFACE4 pSurface = m_pSecondarySurface;
    if (!pSurface)
        pSurface = m_pBackBuffer ? m_pBackBuffer : m_pPrimarySurface;
    if (pSurface && pSurface->Unlock(NULL) != DD_OK)
    {
        CriticalMsg1("Lock count of DirectDraw surfaces appears corrupted (%s)", WhatDDError(DDERR_GENERIC));
        SafeRelease(m_pSecondarySurface);
        SafeRelease(m_pBackBuffer);
        SafeRelease(m_pPrimarySurface);
        SafeRelease(m_pPalette);
        exit(0);
    }
    return TRUE;
}

BOOL cFixedDDModeOps::DoSetFlip(int bFlip)
{
    int count = m_pDisplayDevice->BreakLock();
    m_bFlip = m_bLockBack ? TRUE : bFlip;
    m_pDisplayDevice->RestoreLock(count);
    return bFlip;
}

