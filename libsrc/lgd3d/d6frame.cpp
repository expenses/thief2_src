
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
#include <config.h>
#include <lglog.h>
#include <lgassert.h>

unsigned long g_dwScreenHeight;
DDPIXELFORMAT g_RGBTextureFormat;
DDSURFACEDESC2 g_sDescOfRenderBuffer;
D3DDEVICEDESC g_sD3DDevDesc;
DDPIXELFORMAT g_sDDPFDepth;
IDirect3DDevice3* g_lpD3Ddevice;
DDPIXELFORMAT* g_FormatList[LGD3D_NUM_TEX_FORMATS];
sTextureData g_saTextures[LGD3D_MAX_TEXTURES];
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
float g_fXOffsetBase;
float g_fYOffsetBase;
float g_fTexelCenterX = 0.5f;
float g_fTexelCenterY = 0.5f;
float g_fMipmapBias;



static HRESULT WINAPI c_EnumZBufferFormats(LPDDPIXELFORMAT pDDPF, LPVOID pBuf);

cD6Frame::cD6Frame(unsigned long dwWidth, unsigned long dwHeight, lgd3ds_device_info* psDeviceInfo)
{
    InitializeGlobals(dwWidth, dwHeight, psDeviceInfo->flags);
    if (GetDDstuffFromDisplay() >= 0) {
        InitializeEnvironment(psDeviceInfo);
    } else {
        SetLGD3DErrorCode(LGD3D_EC_DD_KAPUT, 0);
        lgd3d_g_bInitialized = 0;
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
    pIlgdd4surface->GetRenderSurface(&g_lpRenderBuffer);
    SafeRelease(pIlgdd4surface);
    InitializeEnvironment(psDeviceInfo);
}

void cD6Frame::ReleaseRenderer()
{
    pcRenderer = cD6Renderer::DeInstance();
}

cD6Frame::~cD6Frame()
{
    if (g_lpBackgroundMaterial)
        g_lpBackgroundMaterial->Release();
    g_lpBackgroundMaterial = NULL;
    if (g_lpViewport)
        g_lpViewport->Release();
    g_lpViewport = NULL;
    if (g_lpD3Ddevice)
        g_lpD3Ddevice->Release();
    g_lpD3Ddevice = NULL;
    if (g_lpDepthBuffer && g_lpRenderBuffer)
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
    g_fXOffsetBase = 0.0f;
    g_fYOffsetBase = 0.0f;
    g_XOffset = g_fXOffsetBase - g_fTexelCenterX;
    g_YOffset = g_fYOffsetBase - g_fTexelCenterY;
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
    memset(&g_sDescOfRenderBuffer, 0, sizeof(g_sDescOfRenderBuffer));
    memset(&g_sDDPFDepth, 0, sizeof(g_sDDPFDepth));
    memset(&g_sD3DDevDesc, 0, sizeof(g_sD3DDevDesc));

    static BOOL bConfigRead = FALSE;
    if (!bConfigRead) {
        bConfigRead = TRUE;
        g_nAlphaTestMode = !config_is_defined("disable_alpha_test");
        if (g_nAlphaTestMode && config_is_defined("alpha_test_blend"))
            g_nAlphaTestMode = 2;
        g_nAlphaTestRef = 127;
        int cnt = 1;
        if (config_get_value("default_alphatest_ref", CONFIG_INT_TYPE, (config_valptr)&g_nAlphaTestRef, &cnt) && cnt > 0) {
            if (g_nAlphaTestRef < 1)
                g_nAlphaTestRef = 1;
            else if (g_nAlphaTestRef > 255)
                g_nAlphaTestRef = 255;
        }
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
        lgd3d_g_bInitialized = 0;
        return;
    }

    hResult = g_lpDD_ext->QueryInterface(IID_IDirect3D3, (void**)&g_lpD3D);
    if (hResult != 0)
    {
        SetLGD3DErrorCode(LGD3D_EC_QUERY_D3D, hResult);
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
        lgd3d_g_bInitialized = 0;
        return;
    }

    memset(&g_sDescOfRenderBuffer, 0, sizeof(g_sDescOfRenderBuffer));
    g_sDescOfRenderBuffer.dwSize = sizeof(g_sDescOfRenderBuffer);
    hResult = g_lpRenderBuffer->GetSurfaceDesc(&g_sDescOfRenderBuffer);

    if (hResult != 0)
    {
        SetLGD3DErrorCode(LGD3D_EC_GET_SURF_DESC, hResult);
        lgd3d_g_bInitialized = 0;
        return;
    }

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
        cD6Renderer::CleanRenderSurface(1.0);
        ApplyDisplayConfig(psDeviceInfo);
    }
}

int cD6Frame::CreateD3D(const GUID& sDeviceGUID)
{
    long hResult;
    D3DVIEWPORT2 vdData;
    D3DMATERIAL sD3DMaterial;



#line 346
    AssertMsg(g_lpDD_ext != NULL, "cD6Frame::CreateD3D: DirectDraw Not Initialized.");
    AssertMsg(g_lpD3D != NULL, "cD6Frame::CreateD3D: D3D Not Initialized.");
    hResult = g_lpD3D->CreateDevice(sDeviceGUID, g_lpRenderBuffer, &g_lpD3Ddevice, NULL);
    if (hResult != 0)
    {
        SetLGD3DErrorCode(LGD3D_EC_CREATE_3D_DEVICE, hResult);
        lgd3d_g_bInitialized = 0;
        return 0;
    }

    hResult = g_lpD3D->CreateViewport(&g_lpViewport, NULL);
    if (hResult != 0)
    {
        SetLGD3DErrorCode(LGD3D_EC_CREATE_VIEWPORT, hResult);
        lgd3d_g_bInitialized = 0;
        return 0;
    }

    hResult = g_lpD3Ddevice->AddViewport(g_lpViewport);
    if (hResult != 0)
    {
        SetLGD3DErrorCode(LGD3D_EC_ADD_VIEWPORT, hResult);
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
        lgd3d_g_bInitialized = 0;
        return 0;
    }

    hResult = g_lpD3Ddevice->SetCurrentViewport(g_lpViewport);
    if (hResult != 0)
    {
        SetLGD3DErrorCode(LGD3D_EC_SET_CURR_VP, hResult);
        lgd3d_g_bInitialized = 0;
        return 0;
    }

    hResult = g_lpD3D->CreateMaterial(&g_lpBackgroundMaterial, NULL);
    if (hResult != 0)
    {
        SetLGD3DErrorCode(LGD3D_EC_CREATE_BK_MATERIAL, hResult);
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
        lgd3d_g_bInitialized = 0;
        return 0;
    }

    hResult = g_lpBackgroundMaterial->GetHandle(g_lpD3Ddevice, &g_hBackgroundMaterial);
    if (hResult != 0)
    {
        SetLGD3DErrorCode(LGD3D_EC_GET_BK_MAT_HANDLE, hResult);
        lgd3d_g_bInitialized = 0;
        return 0;
    }

    return 1;
}

void cD6Frame::ExamineRenderingCapabilities()
{
    D3DDEVICEDESC sD3DHELDevDesc;
    memset(&g_sD3DDevDesc, 0, sizeof(D3DDEVICEDESC));
    g_sD3DDevDesc.dwSize = sizeof(D3DDEVICEDESC);
    memset(&sD3DHELDevDesc, 0, sizeof(D3DDEVICEDESC));
    sD3DHELDevDesc.dwSize = sizeof(D3DDEVICEDESC);
    long hResult = g_lpD3Ddevice->GetCaps(&g_sD3DDevDesc, &sD3DHELDevDesc);
    if (hResult != 0)
    {
        SetLGD3DErrorCode(LGD3D_EC_GET_3D_CAPS, hResult);
        lgd3d_g_bInitialized = 0;
        return;
    }


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
    long hResult = m_pWinDisplayDevice->GetBitmapSurface(NULL, &g_lpRenderBuffer);



    return hResult;
}

void cD6Frame::ApplyDisplayConfig(lgd3ds_device_info* psDeviceInfo)
{
    float fAdjust[2];
    int cnt;

    g_fTexelCenterX = 0.5f;
    g_fTexelCenterY = 0.5f;
    fAdjust[0] = 0.5f;
    fAdjust[1] = 0.5f;
    cnt = 2;
    if (config_get_value("texel_center_adjust", CONFIG_FLOAT_TYPE, (config_valptr)fAdjust, &cnt) && cnt > 0) {
        if (cnt == 1)
            fAdjust[1] = fAdjust[0];
        g_fTexelCenterX = fAdjust[0];
        g_fTexelCenterY = fAdjust[1];
        if (fAdjust[0] < -1.0f)
            g_fTexelCenterX = -1.0f;
        else if (fAdjust[0] > 1.0f)
            g_fTexelCenterX = 1.0f;
        if (fAdjust[1] < -1.0f)
            g_fTexelCenterY = -1.0f;
        else if (fAdjust[1] > 1.0f)
            g_fTexelCenterY = 1.0f;
        g_XOffset = g_fXOffsetBase - g_fTexelCenterX;
        g_YOffset = g_fYOffsetBase - g_fTexelCenterY;
    }

    static BOOL bConfigRead = FALSE;
    if (bConfigRead) {
        if (pcStates)
            pcStates->SetTextureFilterMode(g_nTexFilterMode);
        if (pcStates)
            pcStates->SetLightmapFilterMode(g_nLightMapFilterMode);
        if (g_fMipmapBias == 0.0f)
            return;
    } else {
        bConfigRead = TRUE;
        BOOL bNoLightmapAniso = config_is_defined("disable_lightmap_aniso");
        BOOL bTrilinear = config_is_defined("tex_filter_trilinear");
        if (pcStates)
            pcStates->SetTrilinearFiltering(bTrilinear);
        if (config_is_defined("tex_filter_mode")) {
            int nMode;
            cnt = 1;
            config_get_value("tex_filter_mode", CONFIG_INT_TYPE, (config_valptr)&nMode, &cnt);
            int nLightMapMode = nMode;
            if (config_is_defined("lm_filter_mode"))
                config_get_int("lm_filter_mode", &nLightMapMode);
            else if (bNoLightmapAniso && nMode > 1)
                nLightMapMode = 1;
            if (pcStates) {
                pcStates->SetTextureFilterMode(nMode);
                if (pcStates)
                    pcStates->SetLightmapFilterMode(nLightMapMode);
            }
        } else if (config_is_defined("lm_filter_mode")) {
            int nLightMapMode;
            config_get_int("lm_filter_mode", &nLightMapMode);
            if (pcStates)
                pcStates->SetLightmapFilterMode(nLightMapMode);
        }
        if (!config_is_defined("mipmap_bias"))
            return;
        float fBias;
        cnt = 1;
        config_get_value("mipmap_bias", CONFIG_FLOAT_TYPE, (config_valptr)&fBias, &cnt);
        g_fMipmapBias = fBias;
    }
    if (pcStates)
        pcStates->SetMipmapBias(g_fMipmapBias);
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
