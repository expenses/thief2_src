
#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include <lgd3d.h>
#include <d6states.h>
#include <dev2d.h>
#include <string.h>
#include <lgassert.h>
#include <d6hrmsg.h>
#include <d6check.h>
#include <appagg.h>
#include <wdispapi.h>
#include <lgsurf.h>
#include <lgsurf_i.h>
#include <lglog.h>
#include <dbg.h>
#include <lglog.h>
#include <lgassert.h>

unsigned long g_dwScreenHeight;
DDPIXELFORMAT g_RGBTextureFormat;
DDSURFACEDESC2 g_sDescOfRenderBuffer;
D3DDEVICEDESC g_sD3DDevDesc;
DDPIXELFORMAT g_sDDPFDepth;
IDirect3DDevice3* g_lpD3Ddevice;
DDPIXELFORMAT* g_FormatList[5];
sTextureData g_saTextures[1024];
DDPIXELFORMAT g_TransRGBTextureFormat;
DDPIXELFORMAT g_PalTextureFormat;
DDPIXELFORMAT g_AlphaTextureFormat;
DDPIXELFORMAT g_8888TexFormat;
unsigned long g_hBackgroundMaterial;
unsigned long g_dwScreenWidth;
cD6Frame* pcFrame = NULL;
unsigned long g_dwRSChangeFlags = 0;
int g_bTexSuspended = FALSE;
int g_bPrefer_RGB = FALSE;
float g_XOffset = 0.0f;
float g_YOffset = 0.0f;
int g_bUseDepthBuffer = FALSE;
int g_bUseTableFog = FALSE;
int g_bUseVertexFog = FALSE;
IDirectDraw4* g_lpDD_ext = NULL;
IDirectDrawSurface4* g_lpRenderBuffer = NULL;
IDirectDrawSurface4* g_lpDepthBuffer = NULL;
IDirect3D3* g_lpD3D = NULL;
IDirect3DViewport3* g_lpViewport = NULL;
IDirect3DMaterial3* g_lpBackgroundMaterial = NULL;
int g_b8888supported = FALSE;
grs_bitmap* default_bm = NULL;
int g_bWFog = FALSE;



static HRESULT WINAPI c_EnumZBufferFormats(LPDDPIXELFORMAT pDDPF, LPVOID pBuf);

cD6Frame::cD6Frame(unsigned long dwWidth, unsigned long dwHeight, lgd3ds_device_info* psDeviceInfo)
{
    InitializeGlobals(dwWidth, dwHeight, psDeviceInfo->flags);
    if (GetDDstuffFromDisplay() < 0) {
        SetLGD3DErrorCode(LGD3D_EC_DD_KAPUT, 0);
#ifndef SHIP
        if (bSpewOn != 0) {
#line 121
            CriticalLGD3DError(LGD3D_EC_DD_KAPUT, 0);
        } else {
            WarnLGD3DError(LGD3D_EC_DD_KAPUT, 0);
        }
#endif
        lgd3d_g_bInitialized = 0;
    } else {
        InitializeEnvironment(psDeviceInfo);
    }
}

cD6Frame::cD6Frame(ILGSurface* pILGSurface)
{
    DWORD dwWidth, dwHeight;
    ILGDD4Surface* pIlgdd4surface;
    int nDeviceIndex;
    lgd3ds_device_info* psDeviceInfo;

    dwWidth = pILGSurface->GetWidth();
    dwHeight = pILGSurface->GetHeight();
    pIlgdd4surface = NULL;
    pILGSurface->QueryInterface(IID_ILGDD4Surface, (void**)&pIlgdd4surface);
    nDeviceIndex = pIlgdd4surface->GetDeviceInfoIndex();
    psDeviceInfo = lgd3d_get_device_info(nDeviceIndex);
    InitializeGlobals(dwWidth, dwHeight, psDeviceInfo->flags);
    pIlgdd4surface->GetDirectDraw(&g_lpDD_ext);
    if (g_lpDD_ext == NULL) {



        CriticalMsg("cD6Frame: GetDirectDraw failed");
    }
    pIlgdd4surface->GetRenderSurface(&g_lpRenderBuffer);
    if (g_lpRenderBuffer == NULL) {

        CriticalMsg("cD6Frame: GetRenderSurface failed");
    }
    SafeRelease(pIlgdd4surface);
    InitializeEnvironment(psDeviceInfo);
}

cD6Frame::~cD6Frame()
{
    pcRenderer = cD6Renderer::DeInstance();
    if (g_lpBackgroundMaterial)
        g_lpBackgroundMaterial->Release();
    g_lpBackgroundMaterial = NULL;
    if (g_lpViewport)
        g_lpViewport->Release();
    g_lpViewport = NULL;
    if (g_lpD3Ddevice)
        g_lpD3Ddevice->Release();
    g_lpD3Ddevice = NULL;
    g_lpRenderBuffer->DeleteAttachedSurface(0, g_lpDepthBuffer);
    if (g_lpRenderBuffer)
        g_lpRenderBuffer->Release();
    g_lpRenderBuffer = NULL;
    if (g_lpDepthBuffer)
        g_lpDepthBuffer->Release();
    g_lpDepthBuffer = NULL;
    if (g_lpD3D)
        g_lpD3D->Release();
    g_lpD3D = NULL;
    if (g_lpDD_ext)
        g_lpDD_ext->Release();
    g_lpDD_ext = NULL;
    if (m_pWinDisplayDevice)
        m_pWinDisplayDevice->Release();
    m_pWinDisplayDevice = NULL;
}



void cD6Frame::InitializeGlobals(unsigned long dwWidth, unsigned long dwHeight, unsigned long dwRequestedFlags)
{
    g_dwScreenWidth = dwWidth;
    g_dwScreenHeight = dwHeight;
    m_pWinDisplayDevice = NULL;
    g_XOffset = 0.0f;
    g_YOffset = 0.0f;
    g_lpDD_ext = NULL;
    g_lpRenderBuffer = NULL;
    g_lpDepthBuffer = NULL;
    g_lpD3D = NULL;
    g_lpD3Ddevice = NULL;
    g_lpViewport = NULL;
    g_lpBackgroundMaterial = NULL;
    g_hBackgroundMaterial = 0;
    g_b8888supported = 0;
    g_bUseDepthBuffer = (dwRequestedFlags & (LGD3DF_ZBUFFER | LGD3DF_WBUFFER)) != 0;
#ifndef SHIP
    bSpewOn = (dwRequestedFlags & LGD3DF_SPEW) != 0;
#endif
    g_bUseTableFog = 0;
    g_bUseVertexFog = 0;
    if ((dwRequestedFlags & LGD3DF_TABLE_FOG) != 0) {
        g_bUseTableFog = (dwRequestedFlags & LGD3DF_CAN_DO_TABLE_FOG) != 0;
    }
    if ((dwRequestedFlags & LGD3DF_VERTEX_FOG) != 0) {
        g_bUseVertexFog = (dwRequestedFlags & LGD3DF_CAN_DO_VERTEX_FOG) != 0;
    }
}



void cD6Frame::InitializeEnvironment(lgd3ds_device_info* psDeviceInfo)
{
    long hResult;
    DDCAPS sDDChdw;
    DDCAPS sDDChel;

    if (g_lpRenderBuffer->IsLost() == DDERR_SURFACELOST
        && (hResult = g_lpDD_ext->RestoreAllSurfaces(), hResult != 0))
    {
        SetLGD3DErrorCode(LGD3D_EC_RESTORE_ALL_SURFS, hResult);
#ifndef SHIP
        if (bSpewOn == 0) {
            WarnLGD3DError(LGD3D_EC_RESTORE_ALL_SURFS, hResult);
        } else {
#line 247
            CriticalLGD3DError(LGD3D_EC_RESTORE_ALL_SURFS, hResult);
        }
#endif
        lgd3d_g_bInitialized = 0;
        return;
    }

    hResult = g_lpDD_ext->QueryInterface(IID_IDirect3D3, (void**)&g_lpD3D);
    if (hResult != 0)
    {
        SetLGD3DErrorCode(LGD3D_EC_QUERY_D3D, hResult);
#ifndef SHIP
        if (bSpewOn == 0) {
            WarnLGD3DError(LGD3D_EC_QUERY_D3D, hResult);
        } else {
#line 253
            CriticalLGD3DError(LGD3D_EC_QUERY_D3D, hResult);
        }
#endif
        lgd3d_g_bInitialized = 0;
        return;
    }

    memset(&sDDChdw, 0, sizeof(sDDChdw));
    sDDChdw.dwSize = sizeof(sDDChdw);
    memset(&sDDChel, 0, sizeof(sDDChel));
    sDDChel.dwSize = sizeof(sDDChel);
    hResult = g_lpDD_ext->GetCaps(&sDDChdw, &sDDChel);
    if (hResult != 0)
    {
        SetLGD3DErrorCode(LGD3D_EC_GET_DD_CAPS, hResult);
#ifndef SHIP
        if (bSpewOn == 0) {
            WarnLGD3DError(LGD3D_EC_GET_DD_CAPS, hResult);
        } else {
#line 266
            CriticalLGD3DError(LGD3D_EC_GET_DD_CAPS, hResult);
        }
#endif
        lgd3d_g_bInitialized = 0;
        return;
    }

#line 268
    AssertMsg(sDDChdw.dwCaps & DDCAPS_3D, "Not an accelerator.");
    memset(&g_sDescOfRenderBuffer, 0, sizeof(g_sDescOfRenderBuffer));
    g_sDescOfRenderBuffer.dwSize = sizeof(g_sDescOfRenderBuffer);
    hResult = g_lpRenderBuffer->GetSurfaceDesc(&g_sDescOfRenderBuffer);

    if (hResult != 0)
    {
        SetLGD3DErrorCode(LGD3D_EC_GET_SURF_DESC, hResult);
#ifndef SHIP
        if (bSpewOn == 0) {
            WarnLGD3DError(LGD3D_EC_GET_SURF_DESC, hResult);
        } else {
#line 276
            CriticalLGD3DError(LGD3D_EC_GET_SURF_DESC, hResult);
        }
#endif
        lgd3d_g_bInitialized = 0;
        return;
    }

#ifndef SHIP
    if ((g_sDescOfRenderBuffer.dwFlags & DDSD_CAPS) == 0)
        Warning(("cD6Frame: flags indicate caps field is empty"));
    if ((g_sDescOfRenderBuffer.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) != 0)
        Warning(("cD6Frame: Rendering to primary surface"));
#endif
    m_bDepthBuffer = (sDDChdw.ddsCaps.dwCaps & DDSCAPS_ZBUFFER) != 0;
    if (g_bUseDepthBuffer != 0) {
        if (m_bDepthBuffer == 0) {

            Warning(("Z buffer not present"));

            g_bUseDepthBuffer = 0;
            psDeviceInfo->flags &= ~(LGD3DF_ZBUFFER | LGD3DF_WBUFFER);
            if ((psDeviceInfo->flags & LGD3DF_DEPTH_BUFFER_REQUIRED) != 0) {
                lgd3d_g_bInitialized = 0;
                return;
            }
        } else {
            hResult = g_lpD3D->EnumZBufferFormats(psDeviceInfo->device_guid, c_EnumZBufferFormats, &g_sDDPFDepth);
            if (hResult != 0) {
                SetLGD3DErrorCode(LGD3D_EC_ZBUFF_ENUMERATION, hResult);
#ifndef SHIP
                if (bSpewOn != 0) {



                    CriticalLGD3DError(LGD3D_EC_ZBUFF_ENUMERATION, hResult);
                } else {
                    WarnLGD3DError(LGD3D_EC_ZBUFF_ENUMERATION, hResult);
                }
#endif
                lgd3d_g_bInitialized = 0;
                return;
            }
            if (CreateDepthBuffer() != 0) {

                Warning(("Could not create Zbuffer"));

                psDeviceInfo->flags &= ~(LGD3DF_ZBUFFER | LGD3DF_WBUFFER);
                if ((psDeviceInfo->flags & LGD3DF_DEPTH_BUFFER_REQUIRED) != 0) {
                    lgd3d_g_bInitialized = 0;
                    return;
                }
                g_bUseDepthBuffer = 0;
            }
        }
    }
    if (CreateD3D(psDeviceInfo->device_guid) != 0) {
        m_dwRequestedFlags = psDeviceInfo->flags;
        ExamineRenderingCapabilities();
        psDeviceInfo->flags = m_dwRequestedFlags;
    }
}

int cD6Frame::CreateD3D(const GUID& sDeviceGUID)
{
    long hResult;
    D3DVIEWPORT2 vdData;
    D3DMATERIAL sD3DMaterial;



    AssertMsg(g_lpDD_ext != NULL, "cD6Frame::CreateD3D: DirectDraw Not Initialized.");
    AssertMsg(g_lpD3D != NULL, "cD6Frame::CreateD3D: D3D Not Initialized.");
    hResult = g_lpD3D->CreateDevice(sDeviceGUID, g_lpRenderBuffer, &g_lpD3Ddevice, NULL);
    if (hResult != 0)
    {
        SetLGD3DErrorCode(LGD3D_EC_CREATE_3D_DEVICE, hResult);
#ifndef SHIP
        if (bSpewOn != 0) {
#line 353
            CriticalLGD3DError(LGD3D_EC_CREATE_3D_DEVICE, hResult);
        } else {
            WarnLGD3DError(LGD3D_EC_CREATE_3D_DEVICE, hResult);
        }
#endif
        lgd3d_g_bInitialized = 0;
        return 0;
    }

    hResult = g_lpD3D->CreateViewport(&g_lpViewport, NULL);
    if (hResult != 0)
    {
        SetLGD3DErrorCode(LGD3D_EC_CREATE_VIEWPORT, hResult);
#ifndef SHIP
        if (bSpewOn != 0) {
#line 358
            CriticalLGD3DError(LGD3D_EC_CREATE_VIEWPORT, hResult);
        } else {
            WarnLGD3DError(LGD3D_EC_CREATE_VIEWPORT, hResult);
        }
#endif
        lgd3d_g_bInitialized = 0;
        return 0;
    }

    hResult = g_lpD3Ddevice->AddViewport(g_lpViewport);
    if (hResult != 0)
    {
        SetLGD3DErrorCode(LGD3D_EC_ADD_VIEWPORT, hResult);
#ifndef SHIP
        if (bSpewOn != 0) {
#line 362
            CriticalLGD3DError(LGD3D_EC_ADD_VIEWPORT, hResult);
        } else {
            WarnLGD3DError(LGD3D_EC_ADD_VIEWPORT, hResult);
        }
#endif
        lgd3d_g_bInitialized = 0;
        return 0;
    }

    memset(&vdData, 0, sizeof(vdData));
    vdData.dwSize = sizeof(vdData);
    vdData.dwWidth = g_dwScreenWidth;
    vdData.dwHeight = g_dwScreenHeight;
    vdData.dvClipX = -1.0f;
    vdData.dvClipY = -1.0f;
    vdData.dvClipWidth = 2.0f;
    vdData.dvClipHeight = 2.0f;
    //vp.dvMinZ = 0.0f;
    vdData.dvMaxZ = 1.0f;
    hResult = g_lpViewport->SetViewport2(&vdData);
    if (hResult != 0)
    {
        SetLGD3DErrorCode(LGD3D_EC_SET_VIEWPORT, hResult);
#ifndef SHIP
        if (bSpewOn != 0) {
#line 380
            CriticalLGD3DError(LGD3D_EC_SET_VIEWPORT, hResult);
        } else {
            WarnLGD3DError(LGD3D_EC_SET_VIEWPORT, hResult);
        }
#endif
        lgd3d_g_bInitialized = 0;
        return 0;
    }

    hResult = g_lpD3Ddevice->SetCurrentViewport(g_lpViewport);
    if (hResult != 0)
    {
        SetLGD3DErrorCode(LGD3D_EC_SET_CURR_VP, hResult);
#ifndef SHIP
        if (bSpewOn != 0) {
#line 384
            CriticalLGD3DError(LGD3D_EC_SET_CURR_VP, hResult);
        } else {
            WarnLGD3DError(LGD3D_EC_SET_CURR_VP, hResult);
        }
#endif
        lgd3d_g_bInitialized = 0;
        return 0;
    }

    hResult = g_lpD3D->CreateMaterial(&g_lpBackgroundMaterial, NULL);
    if (hResult != 0)
    {
        SetLGD3DErrorCode(LGD3D_EC_CREATE_BK_MATERIAL, hResult);
#ifndef SHIP
        if (bSpewOn != 0) {
#line 389
            CriticalLGD3DError(LGD3D_EC_CREATE_BK_MATERIAL, hResult);
        } else {
            WarnLGD3DError(LGD3D_EC_CREATE_BK_MATERIAL, hResult);
        }
#endif
        lgd3d_g_bInitialized = 0;
        return 0;
    }

    memset(&sD3DMaterial, 0, sizeof(sD3DMaterial));
    sD3DMaterial.dwSize = sizeof(sD3DMaterial);
    sD3DMaterial.dwRampSize = 1;
    hResult = g_lpBackgroundMaterial->SetMaterial(&sD3DMaterial);
    if (hResult != 0)
    {
        SetLGD3DErrorCode(LGD3D_EC_SET_BK_MATERIAL, hResult);
#ifndef SHIP
        if (bSpewOn != 0) {
#line 399
            CriticalLGD3DError(LGD3D_EC_SET_BK_MATERIAL, hResult);
        } else {
            WarnLGD3DError(LGD3D_EC_SET_BK_MATERIAL, hResult);
        }
#endif
        lgd3d_g_bInitialized = 0;
        return 0;
    }

    hResult = g_lpBackgroundMaterial->GetHandle(g_lpD3Ddevice, &g_hBackgroundMaterial);
    if (hResult != 0)
    {
        SetLGD3DErrorCode(LGD3D_EC_GET_BK_MAT_HANDLE, hResult);
#ifndef SHIP
        if (bSpewOn != 0) {
#line 403
            CriticalLGD3DError(LGD3D_EC_GET_BK_MAT_HANDLE, hResult);
        } else {
            WarnLGD3DError(LGD3D_EC_GET_BK_MAT_HANDLE, hResult);
        }
#endif
        lgd3d_g_bInitialized = 0;
        return 0;
    }

    return 1;
}

void cD6Frame::ExamineRenderingCapabilities()
{
    D3DDEVICEDESC sD3DHELDevDesc;
#line 416
    AssertMsg(g_lpDD_ext != NULL, "cD6Frame::CreateD3D: DirectDraw Not Initialized.");
    AssertMsg(g_lpRenderBuffer != NULL, "cD6Frame::CreateD3D: Render surface Not Initialized.");
    AssertMsg(g_lpD3D != NULL, "cD6Frame::CreateD3D: D3D Not Initialized.");
    AssertMsg(g_lpD3Ddevice != NULL, "cD6Frame::CreateD3D: D3D device Not Initialized.");
    memset(&g_sD3DDevDesc, 0, sizeof(D3DDEVICEDESC));
    g_sD3DDevDesc.dwSize = sizeof(D3DDEVICEDESC);
    memset(&sD3DHELDevDesc, 0, sizeof(D3DDEVICEDESC));
    sD3DHELDevDesc.dwSize = sizeof(D3DDEVICEDESC);
    long hResult = g_lpD3Ddevice->GetCaps(&g_sD3DDevDesc, &sD3DHELDevDesc);
    if (hResult != 0)
    {
        SetLGD3DErrorCode(LGD3D_EC_GET_3D_CAPS, hResult);
#ifndef SHIP
        if (bSpewOn != 0) {





            CriticalLGD3DError(LGD3D_EC_GET_3D_CAPS, hResult);
        } else {
            WarnLGD3DError(LGD3D_EC_GET_3D_CAPS, hResult);
        }
#endif
        lgd3d_g_bInitialized = 0;
        return;
    }

#line 441
    AssertMsg((g_sD3DDevDesc.dwFlags & D3DDD_DEVCAPS) != 0, "Device Description: invalid caps");
    AssertMsg((g_sD3DDevDesc.dwFlags & D3DDD_TRICAPS) != 0, "Device Description: invalid caps");

    g_bWFog = (g_sD3DDevDesc.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_WFOG) != 0 ? 1 : 0;
    if ((g_sD3DDevDesc.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_WBUFFER) == 0)
        m_dwRequestedFlags &= ~LGD3DF_WBUFFER;
    m_dwTextureOpCaps = g_sD3DDevDesc.dwTextureOpCaps;
    DWORD dwLGMode;
    if ((m_dwRequestedFlags & LGD3DF_MULTI_TEXTURING) != 0) {
        if (g_sD3DDevDesc.wMaxTextureBlendStages < 2 || g_sD3DDevDesc.wMaxSimultaneousTextures < 2 || (unsigned short)g_sD3DDevDesc.dwFVFCaps < 2) {
            dwLGMode = 0;
            m_dwRequestedFlags &= ~LGD3DF_MULTI_TEXTURING;
        } else {
            dwLGMode = 1;
        }
    } else {
        dwLGMode = 0;
    }
    pcRenderer = cD6Renderer::Instance(dwLGMode, &m_dwRequestedFlags);
}

long cD6Frame::GetDDstuffFromDisplay()
{
    m_pWinDisplayDevice = AppGetObj(IWinDisplayDevice);
    m_pWinDisplayDevice->GetDirectDraw(&g_lpDD_ext);
    if (g_lpDD_ext == NULL)
#line 503
        CriticalMsg("cD6Frame: GetDirectDraw failed");
    long hResult = m_pWinDisplayDevice->GetBitmapSurface(NULL, &g_lpRenderBuffer);




    AssertMsg(hResult >= 0, "GetBitmapSurface failed.");
    AssertMsg(g_lpRenderBuffer != NULL, "No render surface available.");
    return hResult;
}

long cD6Frame::CreateDepthBuffer()
{
    DDSURFACEDESC2 sDDSD;
    long hResult;

    memset(&sDDSD, 0, sizeof(sDDSD));
    memcpy(&sDDSD, &g_sDescOfRenderBuffer, sizeof(sDDSD));
    sDDSD.dwFlags = 0x1007;
    sDDSD.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | DDSCAPS_VIDEOMEMORY;
    memcpy(&sDDSD.ddpfPixelFormat, &g_sDDPFDepth, sizeof(DDPIXELFORMAT));
    hResult = g_lpDD_ext->CreateSurface(&sDDSD, &g_lpDepthBuffer, NULL);
    if (hResult == 0) {
        hResult = g_lpRenderBuffer->AddAttachedSurface(g_lpDepthBuffer);
        if (hResult != 0) {
            Warning((pcDXef, "Failed to attach depth buffer (%d): %s", HRESULT_CODE(hResult), GetDDErrorMsg(hResult)));
        }
    } else {
        Warning((pcDXef, "Failed to create depth buffer (%d): %s", HRESULT_CODE(hResult), GetDDErrorMsg(hResult)));
    }
    return hResult;
}


static HRESULT WINAPI c_EnumZBufferFormats(LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext)
{
    if (lpDDPixFmt->dwZBufferBitDepth == 16) {
        LPDDPIXELFORMAT psTargetPixelFormat = (LPDDPIXELFORMAT)lpContext;
        memcpy(psTargetPixelFormat, lpDDPixFmt, sizeof(DDPIXELFORMAT));
        return D3DENUMRET_CANCEL;
    }
    return D3DENUMRET_OK;
}
