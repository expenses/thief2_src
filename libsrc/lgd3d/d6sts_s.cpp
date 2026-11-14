// $Header: x:/prj/tech/libsrc/lgd3d/RCS/texture.c 1.45 1998/04/27 15:00:24 KEVIN Exp $
#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include <lgd3d.h>
#include <d6states.h>
#include <tdrv.h>
#include <tmgr.h>
#include <lglog.h>
#include <dbg.h>
#include <lgassert.h>
#include <d6hrmsg.h>
#include <d6check.h>
#include <malloc.h>
#include <stddef.h>
#include <string.h>
#include <appagg.h>
#include <wdispapi.h>
#include <wdispcb.h>
#include <cbchain.h>
#include <texture.h>
#include <lglog.h>
#include <lgassert.h>

cD6States* pcStates = NULL;

extern "C" {
extern BOOL lgd3d_punt_d3d;
#ifdef SHIP
#define PUNT_D3D 0
#else
#define PUNT_D3D lgd3d_punt_d3d
#endif
BOOL b_SS2_UseSageTexManager = FALSE;



static sTexBlendArgs sTexBlendArgsProtos[3][2] = {
    { { D3DTOP_MODULATE, 2, 0, D3DTOP_SELECTARG2, 2, 0 }, { D3DTOP_DISABLE, 2, 1, D3DTOP_DISABLE, 2, 0 } },
    { { D3DTOP_MODULATE, 2, 0, D3DTOP_MODULATE, 2, 0 }, { D3DTOP_DISABLE, 2, 1, D3DTOP_DISABLE, 2, 0 } },
    { { D3DTOP_BLENDDIFFUSEALPHA, 2, 0, D3DTOP_SELECTARG2, 2, 0 }, { D3DTOP_DISABLE, 2, 1, D3DTOP_DISABLE, 2, 0 } },
};
static sTexBlendArgs sMultiTexBlendArgsProtos[3][2] = {
    { { D3DTOP_SELECTARG1, 2, 0, D3DTOP_DISABLE, 2, 0 }, { D3DTOP_MODULATE, 2, 1, D3DTOP_DISABLE, 2, 0 } },
    { { D3DTOP_SELECTARG1, 2, 0, D3DTOP_SELECTARG2, 2, 0 }, { D3DTOP_SELECTARG2, 2, 1, D3DTOP_MODULATE, 2, 1 } },
    { { D3DTOP_MODULATE, 2, 0, D3DTOP_SELECTARG1, 2, 0 }, { D3DTOP_BLENDTEXTUREALPHA, 2, 1, D3DTOP_SELECTARG2, 2, 1 } },
};


extern texture_manager* g_tmgr;
extern ushort** grd_pal16_list;
static void calc_size(tdrv_texture_info* info, d3d_cookie cookie);
static void GetAvailableTexMem(DWORD *local, DWORD *agp);
static HRESULT WINAPI EnumTextureFormatsCallback(LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext);
static ushort* texture_pal_opaque[256];
static ushort* texture_pal_trans[256];
static IDirectDrawPalette* lpDDPalTexture[256];
extern void lgd3d_get_trans_texture_bitmask(grs_rgb_bitmask*);
extern void lgd3d_get_opaque_texture_bitmask(grs_rgb_bitmask*);
extern int mprintf(const char*, ...);
}


extern unsigned char *lgd3d_clut;

#define NONLOCAL_CAPS DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_NONLOCALVIDMEM | DDSCAPS_ALLOCONLOAD
#define LOCAL_CAPS DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM | DDSCAPS_ALLOCONLOAD
#define MEMORY_TYPE_MASK (DDSCAPS_SYSTEMMEMORY | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM)

#define PF_GENERIC   0
#define PF_RGB       1
#define PF_ALPHA     2
#define PF_MASK      3
#define PF_MAX       3
#define PF_TRANS     128

#define GBitMask15 0x3e0

extern D3DDEVICEDESC g_sD3DDevDesc;


static ushort default_alpha_pal[256]; static ushort* alpha_pal = NULL;
static int callback_id = 0;
static unsigned short** texture_pal_list[4] = { texture_pal_opaque, texture_pal_opaque, &alpha_pal, texture_pal_trans };
extern "C" BOOL lgd3d_blend_trans = TRUE;

#define SET_RENDER_STATE(dev, state, val) \
    ((LPDIRECT3DDEVICE3)(dev))->SetRenderState((state), (val))
#define SET_TSS(dev, stage, state, val) \
    ((LPDIRECT3DDEVICE3)(dev))->SetTextureStageState((stage), (state), (val))
#define SET_RS(dev, st, val) \
    IDirect3DDevice3_SetRenderState((IDirect3DDevice3*)(dev), (D3DRENDERSTATETYPE)(st), (val))
#define SET_STS(dev, st, ty, val) \
    IDirect3DDevice3_SetTextureStageState((IDirect3DDevice3*)(dev), (st), (D3DTEXTURESTAGESTATETYPE)(ty), (val))
#define D6CheckedSetRS(state, val) \
    { HRESULT hResult = SET_RS(g_lpD3Ddevice, state, val); CheckHResult(hResult, "SetRenderStateForGlobal failed"); }
#define D6CheckedSetTSS(stage, type, val) \
    { HRESULT hResult = SET_STS(g_lpD3Ddevice, stage, type, val); CheckHResult(hResult, "SetTextureStageState failed"); }
#define D6CheckedSetTexture(stage, tex) \
    { HRESULT hResult = ((LPDIRECT3DDEVICE3)g_lpD3Ddevice)->SetTexture(stage, (LPDIRECT3DTEXTURE2)(tex)); CheckHResult(hResult, "SetTexture failed"); }
#define D6CheckedSetTexBlend(stage, b) \
    D6CheckedSetTSS(stage, D3DTSS_COLOROP, (b)->eColorOperation); \
    D6CheckedSetTSS(stage, D3DTSS_COLORARG1, (b)->dwColorArg1); \
    D6CheckedSetTSS(stage, D3DTSS_COLORARG2, (b)->dwColorArg2); \
    D6CheckedSetTSS(stage, D3DTSS_ALPHAOP, (b)->eAlphaOperation); \
    D6CheckedSetTSS(stage, D3DTSS_ALPHAARG1, (b)->dwAlphaArg1); \
    D6CheckedSetTSS(stage, D3DTSS_ALPHAARG2, (b)->dwAlphaArg2)
#define AssertTexId(id) AssertMsg1((id) >= 0 && (id) < LGD3D_MAX_TEXTURES, "Invalid texture id: %i", id)


static inline ULONG ReleaseCOM(void* p)
{
    if (p)
        return ((ULONG(__stdcall*)(void*))(*(void***)p)[2])(p);
    return 0;
}

#define NUM_SIZE_ENTRIES 81
static int alpha_size_table[NUM_SIZE_ENTRIES];
static int generic_size_table[NUM_SIZE_ENTRIES];
static int rgb_size_table[NUM_SIZE_ENTRIES];
static int trgb_size_table[NUM_SIZE_ENTRIES];
static int b8888_size_table[NUM_SIZE_ENTRIES];

static int munge_size_tables(int** out_array);
static int init_size_tables(int** out_array);
static void init_size_table(int* table, uchar type);
static void CheckSurfaces(sWinDispDevCallbackInfo *info);
static void InitDefaultTexture(int size);
static void blit_8to16(tdrv_texture_info *info, ushort *dst, int drow, ushort *pal);
static void blit_8to16_trans(tdrv_texture_info *info, ushort *dst, int drow, ushort *pal);
static void blit_8to16_clut(tdrv_texture_info *info, ushort *dst, int drow, ushort *pal);
static void blit_8to16_scale(tdrv_texture_info *info, ushort *dst, int drow, ushort *pal);
static void blit_8to16_clut_scale(tdrv_texture_info *info, ushort *dst, int drow, ushort *pal);
static void blit_8to8(tdrv_texture_info *info, uchar *dst, int drow);
static void blit_8to8_clut(tdrv_texture_info *info, uchar *dst, int drow);
static void blit_8to8_scale(tdrv_texture_info *info, uchar *dst, int drow);
static void blit_8to8_clut_scale(tdrv_texture_info *info, uchar *dst, int drow);
static void blit_16to16(tdrv_texture_info *info, ushort *dst, int drow);
static void blit_16to16_scale(tdrv_texture_info *info, ushort *dst, int drow);
static void blit_32to32(tdrv_texture_info *info, ushort *dst, int drow);
static void blit_32to32_scale(tdrv_texture_info *info, ushort *dst, int drow);
static void blit_32to16(tdrv_texture_info *info, ushort *dst, uint drow);


cD6States::cD6States():
#ifdef NEWDARK
    nd_14(0), nd_18(0), nd_1C(0), nd_20(0), nd_24(0), nd_28(0), nd_30(0),
    m_psCurrentRS(0), m_psSetRS(0), m_bCanDither(0), m_bCanAntialias(0), m_bCanModulate(0),
#endif
    m_bTextureListInitialized(0), m_bTexture_RGB(0), m_bUsingLocalMem(0), m_bLocalMem_available(0),
    m_bAGP_available(0), m_bWBuffer(0), m_bSpecular(0)
{
}

int cD6States::Initialize(DWORD dwRequestedFlags)
{
    int i;
    IWinDisplayDevice* pWinDisplayDevice;

    m_texture_caps = g_sD3DDevDesc.dpcTriCaps.dwTextureCaps;

    m_bWBuffer = (dwRequestedFlags & LGD3DF_WBUFFER) ? 1 : 0;

    EnumerateTextureFormats();

    for (i = 0; i < 0x100; i++)
    {
        texture_pal_opaque[i] = 0;
        texture_pal_trans[i] = 0;
        lpDDPalTexture[i] = 0;
    }

    pcStates->SetPalSlotFlags(0, 0x100, grd_pal, 0, 3);
    alpha_pal = default_alpha_pal;

    pcStates->SetChromaKey(m_psCurrentRS->chroma_r,
                           m_psCurrentRS->chroma_g,
                           m_psCurrentRS->chroma_b);

    memset(&g_saTextures, 0, sizeof(g_saTextures));
    m_bTextureListInitialized = 1;

    pWinDisplayDevice = (IWinDisplayDevice*)_AppGetAggregated(IID_IWinDisplayDevice);
    pWinDisplayDevice->AddTaskSwitchCallback(CheckSurfaces);
    if (pWinDisplayDevice)
        pWinDisplayDevice->Release();
    pWinDisplayDevice = NULL;

    InitDefaultTexture(0x10);

    InitTextureManager();

    return SetDefaultsStates(dwRequestedFlags);
}

void cD6States::EnumerateTextureFormats()
{
    HRESULT hr;
    D3DDEVICEDESC hal;
    D3DDEVICEDESC hel;
    DDSURFACEDESC2 ddsd;
    IDirectDrawSurface4* pSurf;

    memset(&g_RGBTextureFormat, 0, sizeof(g_RGBTextureFormat));
    memset(&g_TransRGBTextureFormat, 0, sizeof(g_TransRGBTextureFormat));
    memset(&g_PalTextureFormat, 0, sizeof(g_PalTextureFormat));
    memset(&g_8888TexFormat, 0, sizeof(g_8888TexFormat));
    g_b8888supported = 0;

    hr = ((LPDIRECT3DDEVICE3)g_lpD3Ddevice)->EnumTextureFormats(EnumTextureFormatsCallback, NULL);
#line 383
    CheckHResult(hr, "EnumTextureFormats failed");

    m_bTexture_RGB = g_bPrefer_RGB;
    g_FormatList[0] = (m_bTexture_RGB != 0) ? &g_RGBTextureFormat : &g_PalTextureFormat;
    if (g_FormatList[0]->dwFlags == 0) {
        m_bTexture_RGB = !m_bTexture_RGB;
        g_FormatList[0] = (m_bTexture_RGB != 0) ? &g_RGBTextureFormat : &g_PalTextureFormat;
        if (g_FormatList[0]->dwFlags == 0) {





            CriticalMsg(D6Msg("Direct3D device does not support 8 bit palettized or 15 or 16 bit RGB textures"));
        }
    }
    g_FormatList[1] = &g_RGBTextureFormat;
    g_FormatList[2] = &g_AlphaTextureFormat;
    g_FormatList[3] = &g_TransRGBTextureFormat;
    g_FormatList[4] = &g_8888TexFormat;

#ifndef SHIP
    if (bSpewOn != 0) {
        mprintf("Using %s textures\n", (m_bTexture_RGB != 0) ? "16 bit RGB" : "Palettized");
    }
#endif

    if (g_AlphaTextureFormat.dwFlags == 0) {
#ifndef SHIP
        if (bSpewOn != 0) {
            mprintf("no alpha texture format available.\n");
        }
#endif
    }

    memset(&hal, 0, sizeof(hal));
    hal.dwSize = sizeof(hal);
    memset(&hel, 0, sizeof(hel));
    hel.dwSize = sizeof(hel);
    hr = ((LPDIRECT3DDEVICE3)g_lpD3Ddevice)->GetCaps(&hal, &hel);
#line 419
    CheckHResult(hr, "Failed to obtain device caps");
#ifndef SHIP
    if (hal.dwFlags & 2)
        ;
    else
#line 421
        CriticalMsg("No HAL device!");
#endif

    memset(&ddsd, 0, sizeof(ddsd));
    memcpy(&ddsd.ddpfPixelFormat, g_FormatList[0], sizeof(ddsd.ddpfPixelFormat));
    ddsd.dwHeight = 0x100;
    ddsd.dwWidth = 0x100;
    m_bAGP_available = 0;
    m_bLocalMem_available = 0;

    if (hal.dwDevCaps & D3DDEVCAPS_TEXTURENONLOCALVIDMEM) {
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
        ddsd.ddsCaps.dwCaps = NONLOCAL_CAPS;
        hr = ((LPDIRECTDRAW4)g_lpDD_ext)->CreateSurface(&ddsd, &pSurf, NULL);
        if ((hr == 0) && pSurf) {
            if (pSurf) {
                ((IUnknown*)pSurf)->Release();
            }
            pSurf = NULL;
            m_bAGP_available = 1;
            m_DeviceSurfaceCaps = NONLOCAL_CAPS;
#ifndef SHIP
            if (bSpewOn != 0) {
                mprintf("nonlocal videomemory textures available.\n");
            }
#endif
        }
    }
    if (hal.dwDevCaps & D3DDEVCAPS_TEXTUREVIDEOMEMORY) {
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
        ddsd.ddsCaps.dwCaps = LOCAL_CAPS;
        hr = ((LPDIRECTDRAW4)g_lpDD_ext)->CreateSurface(&ddsd, &pSurf, NULL);
        if ((hr == 0) && pSurf) {
            if (pSurf) {
                ((IUnknown*)pSurf)->Release();
            }
            pSurf = NULL;
            m_bLocalMem_available = 1;
            m_DeviceSurfaceCaps = LOCAL_CAPS;
#ifndef SHIP
            if (bSpewOn != 0) {
                mprintf("local videomemory textures available.\n");
            }
#endif
        }
    }
    if ((m_bAGP_available == 0) && (m_bLocalMem_available == 0)) {
#ifndef SHIP
        if (bSpewOn != 0) {
            mprintf("No local or nonlocal texture memory! Using system memory textures.\n");
        }
#endif
        m_DeviceSurfaceCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_ALLOCONLOAD;
    }
    m_bUsingLocalMem = m_bLocalMem_available;
}

int cD6States::EnableMTMode(DWORD dwMTOn)
{
    return 1;
}

void cD6States::SetLightMapMode(ulong flag)
{
}

void cD6States::InitTextureManager()
{
    int n;

   if (g_tmgr!=NULL) {
      g_tmgr->shutdown();
      g_tmgr = NULL;
   }

    g_tmgr = (texture_manager*)get_dopey_texture_manager(pcStates);

    n = init_size_tables(&m_texture_size_list);

    g_tmgr->init(default_bm, LGD3D_MAX_TEXTURES, m_texture_size_list, n, 1);
}





unsigned long cD6States::GetRenderStatesSize()
{
    return 0xC0;
}

void cD6States::SetPointerToCurrentStates(char* pData)
{
    m_psCurrentRS = (sRenderStates*)pData;
}

void cD6States::SetPointerToSetStates(char* pData)
{
    m_psSetRS = (sRenderStates*)pData;
}



//  render states / texture stage states to their defaults


void cD6States::SetCommonDefaultStates(DWORD dwRequestedFlags, int bMultiTexture)
{
    DWORD dwPasses;
    HRESULT hR;

    memset(m_psCurrentRS, 0, sizeof(sRenderStates));

    m_bCanDither = (dwRequestedFlags & LGD3DF_CAN_DO_DITHER) != 0;
    m_psCurrentRS->bDitheringOn = (m_bCanDither && (dwRequestedFlags & LGD3DF_DITHER)) ? 1 : 0;





    D6CheckedSetRS(D3DRENDERSTATE_DITHERENABLE, m_psCurrentRS->bDitheringOn);

    m_bCanAntialias = (dwRequestedFlags & LGD3DF_CAN_DO_ANTIALIAS) != 0;
    m_psCurrentRS->bAntialiasingOn = (m_bCanAntialias && (dwRequestedFlags & LGD3DF_ANTIALIAS)) ? 1 : 0;




    D6CheckedSetRS(D3DRENDERSTATE_ANTIALIAS, (m_psCurrentRS->bAntialiasingOn ? 2 : 0));

    m_psCurrentRS->bUsePalette = 1;
    m_psCurrentRS->nAlphaColor = 0xff;
    pcRenderBuffer->PassAlphaColor(m_psCurrentRS->nAlphaColor);
    m_psCurrentRS->eShadeMode = D3DSHADE_GOURAUD;




    D6CheckedSetRS(D3DRENDERSTATE_SHADEMODE, m_psCurrentRS->eShadeMode);

    if (g_bUseDepthBuffer) {
        m_psCurrentRS->eZenable = m_bWBuffer ? D3DZB_USEW : D3DZB_TRUE;

        D6CheckedSetRS(D3DRENDERSTATE_ZENABLE, m_psCurrentRS->eZenable);

    m_psCurrentRS->bZWriteEnable = 0;
    D6CheckedSetRS(D3DRENDERSTATE_ZWRITEENABLE, m_psCurrentRS->bZWriteEnable);

    m_psCurrentRS->eZCompareFunc = D3DCMP_ALWAYS;
    D6CheckedSetRS(D3DRENDERSTATE_ZFUNC, m_psCurrentRS->eZCompareFunc);
    }

    m_psCurrentRS->bFogOn = 0;
    m_psCurrentRS->dwFogColor = 0;
    m_psCurrentRS->eFogMode = D3DFOG_EXP;
    m_psCurrentRS->fFogTableDensity = 0.025f;
    m_psCurrentRS->dcFogSpecular = 0xff000000;
    pcRenderBuffer->PassFogSpecularColor(m_psCurrentRS->dcFogSpecular);

    if (g_bUseTableFog) {



        D6CheckedSetRS(D3DRENDERSTATE_FOGENABLE, m_psCurrentRS->bFogOn);

#line 587
        D6CheckedSetRS(D3DRENDERSTATE_FOGTABLEMODE, m_psCurrentRS->eFogMode);
        D6CheckedSetRS(D3DRENDERSTATE_FOGCOLOR, m_psCurrentRS->dwFogColor);
        D6CheckedSetRS(D3DRENDERSTATE_FOGTABLEDENSITY, *(DWORD*)&m_psCurrentRS->fFogTableDensity);

    } else if (g_bUseVertexFog) {

        D6CheckedSetRS(D3DRENDERSTATE_FOGENABLE, m_psCurrentRS->bFogOn);
        D6CheckedSetRS(D3DRENDERSTATE_FOGCOLOR, m_psCurrentRS->dwFogColor);
    }

    m_psCurrentRS->naTextureId[0] = -1;
    m_psCurrentRS->naTextureId[1] = -1;
    D6CheckedSetTexture(0, NULL);

    m_psCurrentRS->bPerspectiveCorr = 1;
    D6CheckedSetRS(D3DRENDERSTATE_TEXTUREPERSPECTIVE, m_psCurrentRS->bPerspectiveCorr);

    m_psCurrentRS->baChromaKeying[0] = 0;
    D6CheckedSetRS(D3DRENDERSTATE_COLORKEYENABLE, m_psCurrentRS->baChromaKeying[0]);

    m_psCurrentRS->eWrap[0] = D3DTADDRESS_WRAP;
    D6CheckedSetTSS(0, D3DTSS_ADDRESS, m_psCurrentRS->eWrap[0]);

    m_psCurrentRS->eMagTexFilter = D3DTFN_LINEAR;
    m_psCurrentRS->eMinTexFilter = D3DTFN_LINEAR;
    D6CheckedSetTSS(0, D3DTSS_MAGFILTER, m_psCurrentRS->eMagTexFilter);
    D6CheckedSetTSS(0, D3DTSS_MINFILTER, m_psCurrentRS->eMinTexFilter);

    m_psCurrentRS->dwTexBlendMode[1] = 0;
    m_psCurrentRS->baTextureWithAlpha[0] = 0;
    m_psCurrentRS->baTextureWithAlpha[1] = 0;


    D6CheckedSetRS(D3DRENDERSTATE_SPECULARENABLE, this->m_bSpecular);
    D6CheckedSetRS(D3DRENDERSTATE_STENCILENABLE, 0);

    m_psSetRS->dwNoLightMapLevels = 2;

    memcpy(m_psCurrentRS->saTexBlend, sTexBlendArgsProtos, sizeof(m_psCurrentRS->saTexBlend));
#line 634
    D6CheckedSetTSS(0, D3DTSS_TEXCOORDINDEX, 0);
    D6CheckedSetTexBlend(0, m_psCurrentRS->saTexBlend);

    if (bMultiTexture) {
        D6CheckedSetTSS(1, D3DTSS_TEXCOORDINDEX, 1);
        D6CheckedSetTexBlend(1, &m_psCurrentRS->saTexBlend[1]);
    }

    hR = IDirect3DDevice3_ValidateDevice(g_lpD3Ddevice, &dwPasses);
    m_bCanModulate = (hR == 0);

    memcpy(m_psCurrentRS->saTexBlend, sTexBlendArgsProtos[1], sizeof(m_psCurrentRS->saTexBlend));




    D6CheckedSetTSS(0, D3DTSS_TEXCOORDINDEX, 0);
    D6CheckedSetTexBlend(0, m_psCurrentRS->saTexBlend);

    if (bMultiTexture) {
        D6CheckedSetTSS(1, D3DTSS_TEXCOORDINDEX, 1);
        D6CheckedSetTexBlend(1, &m_psCurrentRS->saTexBlend[1]);
    }

    hR = IDirect3DDevice3_ValidateDevice(g_lpD3Ddevice, &dwPasses);
    m_bCanModulateAlpha = (hR == 0);

    if (m_bCanModulate == 0 && m_bCanModulateAlpha == 0) {
        SetLGD3DErrorCode(LGD3D_EC_VD_MPASS_MT, hR);
#ifndef SHIP
        if (bSpewOn) {
#line 663
            CriticalLGD3DError(LGD3D_EC_VD_MPASS_MT, hR);
        } else {
            WarnLGD3DError(LGD3D_EC_VD_MPASS_MT, hR);
        }
#endif
        lgd3d_g_bInitialized = 0;
    } else if (dwRequestedFlags & LGD3DF_MODULATEALPHA) {
        m_psCurrentRS->dwTexBlendMode[0] = (m_bCanModulateAlpha != 0);
    } else {
        m_psCurrentRS->dwTexBlendMode[0] = (m_bCanModulate == 0);
    }
}


int cD6States::SetDefaultsStates(DWORD dwRequestedFlags)
{
    DWORD dwPasses;
    HRESULT hR;

    SetCommonDefaultStates(dwRequestedFlags, 0);

    memcpy(m_psCurrentRS->saTexBlend,
        sTexBlendArgsProtos[m_psCurrentRS->dwTexBlendMode[0]],
        sizeof(m_psCurrentRS->saTexBlend));
#line 717
    D6CheckedSetTSS(0, D3DTSS_TEXCOORDINDEX, 0);
    D6CheckedSetTexBlend(0, m_psCurrentRS->saTexBlend);

    D6CheckedSetRS(D3DRENDERSTATE_ALPHABLENDENABLE, 1);

    if ((dwRequestedFlags & LGD3DF_MULTITEXTURE_COLOR) != 0) {

        D6CheckedSetRS(D3DRENDERSTATE_SRCBLEND, D3DBLEND_DESTCOLOR);
        D6CheckedSetRS(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ZERO);

        hR = IDirect3DDevice3_ValidateDevice(g_lpD3Ddevice, &dwPasses);
        if (hR != 0) {
            SetLGD3DErrorCode(LGD3D_EC_VD_MPASS_MT, hR);
#ifndef SHIP
            if (bSpewOn == 0) {
                WarnLGD3DError(LGD3D_EC_VD_MPASS_MT, hR);
            }
            else {
#line 729
                CriticalLGD3DError(LGD3D_EC_VD_MPASS_MT, hR);
            }
#endif
            lgd3d_g_bInitialized = 0;
            return 0;
        }
    }

    m_psCurrentRS->eSrcAlpha = D3DBLEND_SRCALPHA;
#line 734
    D6CheckedSetRS(D3DRENDERSTATE_SRCBLEND, m_psCurrentRS->eSrcAlpha);
    m_psCurrentRS->eDstAlpha = D3DBLEND_INVSRCALPHA;

    D6CheckedSetRS(D3DRENDERSTATE_DESTBLEND, m_psCurrentRS->eDstAlpha);

    hR = IDirect3DDevice3_ValidateDevice(g_lpD3Ddevice, &dwPasses);
    if (hR != 0) {
        SetLGD3DErrorCode(LGD3D_EC_VD_S_DEFAULT, hR);
#ifndef SHIP
        if (bSpewOn == 0) {
            WarnLGD3DError(LGD3D_EC_VD_S_DEFAULT, hR);
        }
        else {
#line 741
            CriticalLGD3DError(LGD3D_EC_VD_S_DEFAULT, hR);
        }
#endif
        lgd3d_g_bInitialized = 0;
        return 0;
    }

    memcpy(m_psCurrentRS->saTexBlend,
        sTexBlendArgsProtos[m_psCurrentRS->dwTexBlendMode[0]],
        sizeof(m_psCurrentRS->saTexBlend));
#line 749
    D6CheckedSetTSS(0, D3DTSS_TEXCOORDINDEX, 0);
    D6CheckedSetTexBlend(0, m_psCurrentRS->saTexBlend);

    m_psCurrentRS->bAlphaOn = 0;
    D6CheckedSetRS(D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);

    memcpy(m_psSetRS, m_psCurrentRS, sizeof(sRenderStates));
    g_dwRSChangeFlags = 0;
    return 1;
}

void cD6States::synchronize()
{
    pcRenderBuffer->FlushPrimitives();
}

void cD6States::start_frame(int n)
{
    pcRenderer->StartFrame(n);
}

void cD6States::end_frame()
{
    pcRenderer->EndFrame();
}

void cD6States::release_texture_a(int n)
{
    sTextureData* pEntry;

#line 1349
    AssertMsg(n >= 0 && n <= 0x3ff, "Texture id out of range");

    pEntry = &g_saTextures[n];

    if (n == m_psCurrentRS->naTextureId[0]) {
        pcRenderBuffer->FlushPrimitives();
    }

    if (pEntry->lpTexture) {
        ((IUnknown*)pEntry->lpTexture)->Release();
    }
    pEntry->lpTexture = NULL;
    if (g_saTextures[n].lpSurface) {
        ((IUnknown*)g_saTextures[n].lpSurface)->Release();
    }
    g_saTextures[n].lpSurface = NULL;
    g_saTextures[n].pTdrvBitmap = NULL;
}

int cD6States::load_texture_a(tdrv_texture_info* info)
{
    int id;
    sTextureData* pEntry;

    if (((m_texture_caps & D3DPTEXTURECAPS_SQUAREONLY) == 0) || (info->w == info->h)) {
        id = info->id;
        if ((id < 0) || (id > 0x3ff)) {
#line 1388
            CriticalMsg("Texture id out of range");
        }
        pEntry = &g_saTextures[id];
        if (id == m_psCurrentRS->naTextureId[0]) {
            pcRenderBuffer->FlushPrimitives();
        }
        if (pEntry->lpTexture) {
            ((IUnknown*)pEntry->lpTexture)->Release();
        }
        pEntry->lpTexture = NULL;
        if (g_saTextures[id].lpSurface) {
            ((IUnknown*)g_saTextures[id].lpSurface)->Release();
        }
        g_saTextures[id].lpSurface = NULL;
        id = reload_texture_a(info);
    } else {
        id = -1;
    }
    return id;
}

int cD6States::reload_texture_a(tdrv_texture_info* info)
{
    DDPIXELFORMAT* pFormat;
    int result;
    IDirectDrawSurface4* pSysmem;
    DDSURFACEDESC2 ddsd;
    sTextureData* pData;
    IDirectDrawSurface4** ppDeviceSurface;

    result = info->id;
    pData = &g_saTextures[result];
    ppDeviceSurface = &g_saTextures[result].lpSurface;

    pFormat = g_FormatList[((unsigned char*)&info->cookie)[2] & 7];
    if (pFormat->dwFlags == 0) {
        return -1;
    }

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_TEXTURESTAGE;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    ddsd.dwHeight = info->h;
    ddsd.dwWidth = info->w;
    memcpy(&ddsd.ddpfPixelFormat, pFormat, sizeof(DDPIXELFORMAT));

    result = this->CreateDDSurface(*(d3d_cookie*)&info->cookie, &ddsd, &pSysmem);
#line 1446
    CheckHResult(result, "CreateDDSurface() failed.");

    if (pSysmem == NULL) {
#line 1447
        CriticalMsg("NULL SysmemSurface");
    }

    if (pData->lpTexture == NULL) {
        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
        ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
        ddsd.dwTextureStage = (((unsigned char*)&info->cookie)[2] & 0x40) != 0;
        result = this->CreateDDSurface(*(d3d_cookie*)&info->cookie, &ddsd, ppDeviceSurface);



        CheckHResult(result, "CreateDDSurface() failed.");
        result = ((IDirectDrawSurface4*)*ppDeviceSurface)->QueryInterface(IID_IDirect3DTexture2, (void**)&pData->lpTexture);

        CheckHResult(result, "Failed to obtain D3D texture interface.");
    }

    result = pSysmem->Lock(NULL, &ddsd, 0, NULL);



    CheckHResult(result, "Failed to lock sysmem surface.");

    this->LoadSurface(info, &ddsd);

    result = pSysmem->Unlock(NULL);
    CheckHResult(result, "Failed to unlock sysmem surface.");

    result = ((IDirectDrawSurface4*)*ppDeviceSurface)->Blt(NULL, pSysmem, NULL, DDBLT_WAIT, NULL);

    CheckHResult(result, "Failed to blit sysmem surface.");

    if (pSysmem) {
        ((IUnknown*)pSysmem)->Release();
    }

    g_saTextures[info->id].pTdrvBitmap = info->bm;
    g_saTextures[info->id].TdrvCookie.value = info->cookie;

    return 0;
}

int cD6States::load_texture(tdrv_texture_info* info)
{
    int n;
    DWORD local_start;
    DWORD agp_start;
    int size;
    int result;

    n = info->id;
    local_start = 0;
    agp_start = 0;
    if (info->size_index >= 0) {
        size = m_texture_size_list[info->size_index];
    } else {
        size = 0;
    }

    if ((m_texture_caps & D3DPTEXTURECAPS_SQUAREONLY) && (info->w != info->h)) {
        return -1;
    }

    if (b_SS2_UseSageTexManager == 0) {
        GetAvailableTexMem(m_bLocalMem_available ? &local_start : NULL, m_bAGP_available ? &agp_start : NULL);
        if ((local_start < (DWORD)size) && (m_bUsingLocalMem != 0) && (m_bAGP_available != 0)) {
#ifndef SHIP
            if (bSpewOn != 0) {
                mprintf("using nonlocal vidmem textures...\n");
            }
#endif
            m_bUsingLocalMem = 0;
            m_DeviceSurfaceCaps = NONLOCAL_CAPS;
        }
        if ((m_bUsingLocalMem ? local_start : agp_start) < (DWORD)size) {
            return -1;
        }
    }
#line 837
    AssertMsg(n >= 0 && n < LGD3D_MAX_TEXTURES, "Texture id out of range");
    result = reload_texture(info);
    if ((result != 0) && (m_bUsingLocalMem != 0) && (m_bAGP_available != 0)) {
#ifndef SHIP
        if (bSpewOn != 0) {
            mprintf("using nonlocal vidmem textures...\n");
        }
#endif
        m_bUsingLocalMem = 0;
        m_DeviceSurfaceCaps = NONLOCAL_CAPS;
        result = reload_texture(info);
    }
    if (result != 0) {
        return result;
    }

    if ((b_SS2_UseSageTexManager == 0) && (size == 0)) {
        DWORD local_end;
        DWORD agp_end;
        GetAvailableTexMem(m_bLocalMem_available ? &local_end : NULL, m_bAGP_available ? &agp_end : NULL);
        if ((m_bLocalMem_available != 0) && (local_start > local_end)) {
            info->size_index = local_start - local_end;
            if (m_bAGP_available != 0) {
                info->size_index += agp_start - agp_end;
            }
        } else {
            if ((m_bAGP_available != 0) && (agp_start != agp_end)) {
                info->size_index = agp_start - agp_end;
            } else {
                Warning(("no AGP memory left"));
                if ((info->w == 1) && (info->h == 1)) {
                    info->size_index = 0x10;
                    return 0;
                }
                Error(1, "Direct3d device driver does not accurately report texture memory usage.\n  Contact your 3d accelerator vendor for updated drivers.");
                return -1;
            }
        }
    }
    return 0;
}


void cD6States::release_texture(int n)
{
    DDSCAPS2 caps;
    sTextureData* pEntry;

#line 914
    AssertMsg(n >= 0 && n < LGD3D_MAX_TEXTURES, "Texture id out of range");

    pEntry = &g_saTextures[n];

    if (n == m_psCurrentRS->naTextureId[0]) {
        pcRenderBuffer->FlushPrimitives();
    }

    if (pEntry->lpSurface == NULL) {
        Warning(("texture %i already released", n));
    }

    if ((pEntry->lpSurface && (m_bLocalMem_available != 0)) &&
        (m_bUsingLocalMem == 0) &&
        (((IDirectDrawSurface4*)pEntry->lpSurface)->GetCaps(&caps), (caps.dwCaps & DDSCAPS_LOCALVIDMEM) != 0))
    {
#ifndef SHIP
        if (bSpewOn != 0) {
            mprintf("Using Local vidmem textures\n");
        }
#endif
        m_DeviceSurfaceCaps = LOCAL_CAPS;
        m_bUsingLocalMem = 1;
    }

    if (pEntry->lpTexture) {
        ((IUnknown*)pEntry->lpTexture)->Release();
    }
    pEntry->lpTexture = NULL;
    if (pEntry->lpSurface) {
        ((IUnknown*)pEntry->lpSurface)->Release();
    }
    pEntry->lpSurface = NULL;
    pEntry->pTdrvBitmap = NULL;
}

int cD6States::reload_texture(tdrv_texture_info* info)
{
    IDirectDrawSurface4* DeviceSurface;
    sTextureData* psTexData;
    DDPIXELFORMAT* pixel_format;
    IDirectDrawSurface4* SysmemSurface;
    IDirect3DTexture2* SysmemTexture;
    HRESULT LastError;
    DDSURFACEDESC2 ddsd;
    IDirect3DTexture2** pDeviceTexture;
    IDirectDrawSurface4** pDeviceSurface;
    d3d_cookie cookie;
    int fmt;

    psTexData = &g_saTextures[info->id];
    pDeviceTexture = (IDirect3DTexture2**)&psTexData->lpTexture;
    pDeviceSurface = &psTexData->lpSurface;
    cookie.value = info->cookie;
    fmt = cookie.flags & 7;
    pixel_format = g_FormatList[fmt];
    if (fmt == 4 && !g_b8888supported)
        pixel_format = g_FormatList[2];
    if (!pixel_format->dwFlags)
        return -1;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    ddsd.dwHeight = info->h;
    ddsd.dwWidth = info->w;
    memcpy(&ddsd.ddpfPixelFormat, pixel_format, sizeof(ddsd.ddpfPixelFormat));

    LastError = CreateDDSurface(cookie, &ddsd, &SysmemSurface);



    CheckHResult(LastError, "CreateDDSurface() failed");
    AssertMsg(SysmemSurface, "NULL SysmemSurface");

    SysmemTexture = NULL;
    LastError = SysmemSurface->QueryInterface(IID_IDirect3DTexture2, (void**)&SysmemTexture);



    CheckHResult(LastError, "Failed to obtain D3D texture interface for sysmem surface");

    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    if (!*pDeviceTexture)
    {
        ddsd.ddsCaps.dwCaps = m_DeviceSurfaceCaps;
        LastError = CreateDDSurface(cookie, &ddsd, pDeviceSurface);
        if (LastError && m_bUsingLocalMem && m_bAGP_available)
        {
#ifndef SHIP
            if (bSpewOn)
                mprintf("using nonlocal vidmem textures...\n");
#endif
            m_bUsingLocalMem = 0;
            m_DeviceSurfaceCaps = NONLOCAL_CAPS;
            ddsd.ddsCaps.dwCaps = m_DeviceSurfaceCaps;
            LastError = CreateDDSurface(cookie, &ddsd, pDeviceSurface);
        }
        if (LastError)
        {
#ifndef SHIP
            if (bSpewOn)
                mprintf("couldn't load texture: error %i.\n", HRESULT_CODE(LastError));
#endif
            if (SysmemTexture)
                SysmemTexture->Release();
            SysmemTexture = NULL;
            if (SysmemSurface)
                SysmemSurface->Release();
            SysmemSurface = NULL;
            *pDeviceSurface = NULL;
            return -1;
        }
        DeviceSurface = *pDeviceSurface;
#ifndef SHIP
        if ((m_DeviceSurfaceCaps & MEMORY_TYPE_MASK) != (ddsd.ddsCaps.dwCaps & MEMORY_TYPE_MASK) && bSpewOn)
            mprintf("device texture not created in requested memory location\nRequested %x Received %x\n",
                    m_DeviceSurfaceCaps & MEMORY_TYPE_MASK, ddsd.ddsCaps.dwCaps & MEMORY_TYPE_MASK);
#endif
        LastError = DeviceSurface->QueryInterface(IID_IDirect3DTexture2, (void**)pDeviceTexture);
#line 1043
        CheckHResult(LastError, "Failed to obtain D3D texture interface for device surface.");
    }

    LastError = SysmemSurface->Lock(NULL, &ddsd, 0, NULL);

    CheckHResult(LastError, "Failed to lock sysmem surface.");
    LoadSurface(info, &ddsd);
    LastError = SysmemSurface->Unlock(NULL);


    CheckHResult(LastError, "Failed to unlock sysmem surface.");
    LastError = (*pDeviceTexture)->Load(SysmemTexture);
#line 1061
    CheckHResult(LastError, "Failed to load device texture from sysmem texture.");

    if (SysmemTexture)
        SysmemTexture->Release();
    SysmemTexture = NULL;
    if (SysmemSurface)
        SysmemSurface->Release();
    SysmemSurface = NULL;

    psTexData->pTdrvBitmap = info->bm;
    psTexData->TdrvCookie.value = info->cookie;
    return 0;
}


void cD6States::cook_info(tdrv_texture_info* info)
{
    d3d_cookie cook;
    int i;
    int w;
    int h;

    cook.palette = 0;

    if (grd_canvas->gc.fill_type == FILL_BLEND) {
        cook.flags = 2;
    } else switch (info->bm->type) {
        case BMT_FLAT32:
            cook.flags = 4;
            break;
        case BMT_FLAT16:
            if (gr_test_bitmap_format(info->bm, BMF_RGB_4444)) {
                cook.flags = 2;
            } else if ((info->bm->flags & BMF_TRANS) != 0) {
                if (g_TransRGBTextureFormat.dwFlags != 0 &&
                    (m_psCurrentRS->dwTexBlendMode[0] != 0 || m_psCurrentRS->nAlphaColor >= 0xff)) {
                    cook.flags = 3;
                } else {
                    cook.flags = 0x81;
                }
            } else {
                cook.flags = 1;
            }
            break;
        case BMT_FLAT8:
            cook.palette = info->bm->align;
            if ((info->bm->flags & BMF_TRANS) != 0) {
                if (texture_pal_trans[cook.palette] &&
                    g_TransRGBTextureFormat.dwFlags != 0 &&
                    (m_psCurrentRS->dwTexBlendMode[0] != 0 || m_psCurrentRS->nAlphaColor >= 0xff)) {
                    cook.flags = 3;
                } else {
                    cook.flags = 0x80;
                }
            } else {
                cook.flags = 0;
            }
            break;
    }

    calc_size(info, cook);

    w = info->w;
    h = info->h;
    cook.hlog = 0;
    cook.wlog = 0;
    for (i = 2; i <= w; i += i) cook.wlog++;
    for (i = 2; i <= h; i += i) cook.hlog++;
#line 1167
    AssertMsg(!(info->h != (1 << cook.hlog) || info->w != (1 << cook.wlog)), "hlog/wlog does not match texture width/height");
    info->cookie = cook.value;
}
void cD6States::unload_texture(int n)
{
#line 1178
    AssertMsg(n >= 0 && n < LGD3D_MAX_TEXTURES, "Texture id out of range");
    g_saTextures[n].pTdrvBitmap = NULL;
}
HRESULT cD6States::CreateDDSurface(d3d_cookie dwCookie, DDSURFACEDESC2* pddsd, IDirectDrawSurface4** ppSurface)
{
    DWORD dwResult;
    IDirectDrawSurface4* pSurface;
    DWORD chroma_key;
    DDCOLORKEY ddck;


    dwResult = g_lpDD_ext->CreateSurface(pddsd, ppSurface, NULL);
    if (dwResult == 0) {
        pSurface = *ppSurface;
        if ((pddsd->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) != 0) {
            dwResult = pSurface->SetPalette(
                lpDDPalTexture[dwCookie.palette]);
#line 1203
            CheckHResult(dwResult, "SetPalette failed while creating surface.");
            chroma_key = 0;
        } else {
            chroma_key = m_psCurrentRS->chroma_key;
        }
        if ((dwCookie.flags & PF_TRANS) != 0) {
            ddck.dwColorSpaceLowValue = chroma_key;
            ddck.dwColorSpaceHighValue = chroma_key;
            dwResult = pSurface->SetColorKey(
                DDCKEY_SRCBLT, &ddck);



            CheckHResult(dwResult, "SetColorKey failed while creating surface.");
        }
        dwResult = 0;
    }
    return dwResult;
}

void cD6States::LoadSurface(tdrv_texture_info* info, DDSURFACEDESC2* ddsd)
{
    uchar* src;
    grs_bitmap* bm;
    uchar zero_save;
    d3d_cookie cookie;

    bm = info->bm;
    zero_save = 0;
    cookie.value = info->cookie;

    if (texture_clut && (bm->flags & BMF_TRANS)) {
        zero_save = *texture_clut;
        *texture_clut = 0;
    }

    if (ddsd->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
        uchar* dst;
        dst = (uchar*)ddsd->lpSurface;
        if (info->scale_w | info->scale_h) {
            if (texture_clut)
                blit_8to8_clut_scale(info, dst, ddsd->lPitch);
            else
                blit_8to8_scale(info, dst, ddsd->lPitch);
        } else {
            if (texture_clut)
                blit_8to8_clut(info, dst, ddsd->lPitch);
            else
                blit_8to8(info, dst, ddsd->lPitch);
        }
    } else {
        int drow;
        ushort* dst;
        uchar type;
        dst = (ushort*)ddsd->lpSurface;
        drow = ddsd->lPitch / 2;
        type = cookie.flags & 7;

        if (bm->type == BMT_FLAT8) {
            ushort* pal;
            ushort zero_save;
            pal = texture_pal_list[cookie.flags & PF_MASK][cookie.palette];

            if (pal == NULL)
                pal = grd_pal16_list[cookie.palette];

            if (cookie.flags & PF_TRANS) {
                zero_save = *pal;
                *pal = (ushort)m_psCurrentRS->chroma_key;
            }





            AssertMsg(pal, "Hey! trying to use NULL 16 bit palette to load texture!");

            if (info->scale_w | info->scale_h) {
                if (type != 2 && texture_clut)
                    blit_8to16_clut_scale(info, dst, drow, pal);
                else
                    blit_8to16_scale(info, dst, drow, pal);
            } else {
                if (type != 2 && texture_clut)
                    blit_8to16_clut(info, dst, drow, pal);
                else if (type == 3 && lgd3d_blend_trans)
                    blit_8to16_trans(info, dst, drow, pal);
                else
                    blit_8to16(info, dst, drow, pal);
                if (cookie.flags & PF_TRANS)
                    *pal = zero_save;
            }
        } else if (bm->type == BMT_FLAT32) {
            if (g_b8888supported) {
                if (info->scale_w | info->scale_h)
                    blit_32to32_scale(info, dst, drow);
                else
                    blit_32to32(info, dst, drow);
            } else
                blit_32to16(info, dst, drow);
        } else {
            if (info->scale_w | info->scale_h)
                blit_16to16_scale(info, dst, drow);
            else
                blit_16to16(info, dst, drow);
        }
    }

    if (zero_save)
        *texture_clut = zero_save;
}


int cD6States::get_texture_id()
{
    return m_psCurrentRS->naTextureId[0];
}

void cD6States::TurnOffTexuring(int bTexOff)
{
    HRESULT hr;

    if (bTexOff) {
        hr = ((LPDIRECT3DDEVICE3)(g_lpD3Ddevice))->SetTexture(0, NULL);
#line 1507
        CheckHResult(hr, "SetTexture failed");
        m_psSetRS->naTextureId[0] = -1;
        g_bTexSuspended = 1;
    } else {
        SetTextureNow();
    }
}

void cD6States::SetDithering(BOOL bOn)
{
    if (m_bCanDither) {
        m_psCurrentRS->bDitheringOn = bOn;
        if (m_psSetRS->bDitheringOn != m_psCurrentRS->bDitheringOn) {
            pcRenderBuffer->FlushPrimitives();
            {
                HRESULT hr = SET_RENDER_STATE(g_lpD3Ddevice, D3DRENDERSTATE_DITHERENABLE, m_psCurrentRS->bDitheringOn);

#line 1534
                CheckHResult(hr, "SetRenderStateForGlobal failed");
            }
            m_psSetRS->bDitheringOn = m_psCurrentRS->bDitheringOn;
        }
    }
}

void cD6States::SetAntialiasing(int bOn)
{
    if (m_bCanAntialias) {
        m_psCurrentRS->bAntialiasingOn = bOn;
        if (m_psSetRS->bAntialiasingOn != m_psCurrentRS->bAntialiasingOn) {
            pcRenderBuffer->FlushPrimitives();
            DWORD dwVal = (m_psCurrentRS->bAntialiasingOn ? -1 : 0) & 2;
            HRESULT hr = SET_RENDER_STATE(g_lpD3Ddevice, D3DRENDERSTATE_ANTIALIAS, dwVal);
#line 1558
            CheckHResult(hr, "SetRenderStateForGlobal failed");
            m_psSetRS->bAntialiasingOn = m_psCurrentRS->bAntialiasingOn;
        }
    }
}

void cD6States::SetTextureLevel(int n)
{

}

void cD6States::EnableDepthBuffer(int val)
{
    pcRenderBuffer->FlushPrimitives();
    m_psCurrentRS->eZenable = (D3DZBUFFERTYPE)val;
    m_psSetRS->eZenable = (D3DZBUFFERTYPE)val;
    if (PUNT_D3D) {
    } else {
        HRESULT hr = SET_RENDER_STATE(g_lpD3Ddevice, D3DRENDERSTATE_ZENABLE, m_psCurrentRS->eZenable);
#line 1575
        CheckHResult(hr, "SetRenderStateForGlobal failed");
    }
}

void cD6States::SetZWrite(BOOL val)
{
    pcRenderBuffer->FlushPrimitives();
    m_psCurrentRS->bZWriteEnable = val;
    m_psSetRS->bZWriteEnable = val;

    if (!PUNT_D3D) {
        HRESULT hr;
        hr = SET_RENDER_STATE(g_lpD3Ddevice, D3DRENDERSTATE_ZWRITEENABLE, m_psCurrentRS->bZWriteEnable);




        CheckHResult(hr, "SetRenderStateForGlobal failed");
    }
}

int cD6States::EnableSpecular(BOOL enable)
{
    int old = m_bSpecular;
    m_bSpecular = enable;
    HRESULT hr = SET_RENDER_STATE(g_lpD3Ddevice, D3DRENDERSTATE_SPECULARENABLE, m_bSpecular);
#line 2380
    CheckHResult(hr, "SetRenderStateForGlobal failed");
    return old;
}

void cD6States::SetZCompare(BOOL val)
{
    pcRenderBuffer->FlushPrimitives();
    m_psCurrentRS->eZCompareFunc = val ? D3DCMP_LESSEQUAL : D3DCMP_ALWAYS;
    m_psSetRS->eZCompareFunc = m_psCurrentRS->eZCompareFunc;

    if (PUNT_D3D) {
    } else {
        HRESULT hr = SET_RENDER_STATE(g_lpD3Ddevice, D3DRENDERSTATE_ZFUNC, m_psCurrentRS->eZCompareFunc);
#line 1609
        CheckHResult(hr, "SetRenderStateForGlobal failed");
    }
}

int cD6States::SetSmoothShading(BOOL bSmooth)
{
    int old = IsSmoothShadingOn();
    if (bSmooth != old) {
        pcRenderBuffer->FlushPrimitives();
        m_psCurrentRS->eShadeMode = bSmooth ? D3DSHADE_GOURAUD : D3DSHADE_FLAT;
        m_psSetRS->eShadeMode = m_psCurrentRS->eShadeMode;
        if (!PUNT_D3D) {
            HRESULT hr = SET_RENDER_STATE(g_lpD3Ddevice, D3DRENDERSTATE_SHADEMODE, m_psCurrentRS->eShadeMode);
#line 1903
            CheckHResult(hr, "SetRenderStateForGlobal failed");
        }
    }
    return old;
}

void cD6States::SetFogDensity(float density)
{
        if (g_bUseTableFog || g_bUseVertexFog) {

        pcRenderBuffer->FlushPrimitives();
        m_psSetRS->fFogTableDensity = m_psCurrentRS->fFogTableDensity =
            !g_bWFog ? (float)(density * z_far * 0.5) : density;
        if (PUNT_D3D) {
        } else {
            HRESULT hr = SET_RENDER_STATE(g_lpD3Ddevice, D3DRENDERSTATE_FOGTABLEDENSITY, *(DWORD*)&m_psCurrentRS->fFogTableDensity);
#line 1633
            CheckHResult(hr, "SetRenderStateForGlobal failed");
        }

            }
}

int cD6States::UseLinearTableFog(BOOL bUseIt)
{
    int old;
    if (g_bUseTableFog) {

        old = (m_psCurrentRS->eFogMode == D3DFOG_LINEAR);
        if (old != bUseIt) {
            pcRenderBuffer->FlushPrimitives();
            m_psCurrentRS->eFogMode = bUseIt ? D3DFOG_LINEAR : D3DFOG_EXP;
            m_psSetRS->eFogMode = m_psCurrentRS->eFogMode;
            HRESULT hr = SET_RENDER_STATE(g_lpD3Ddevice, D3DRENDERSTATE_FOGTABLEMODE, m_psCurrentRS->eFogMode);
#line 1657
            CheckHResult(hr, "SetRenderStateForGlobal failed");
        }

    return old;
    }
    return 0;
}





float cD6States::LinearWorldIntoFogCoef(float param)
{
    float fParam = param;
    if (g_bWFog)
        return fParam;




    AssertMsg(z_far > z_near, "Z far and near values are incorrect");
    return (float)((z_far - (z_far * z_near) / fParam) / (z_far - z_near));
}

void cD6States::SetFogStartAndEnd(float fStart, float fEnd)
{


    if (g_bUseTableFog) {
        float start = LinearWorldIntoFogCoef(fStart);
        float end = LinearWorldIntoFogCoef(fEnd);
        m_psSetRS->fFogStart = m_psCurrentRS->fFogStart = start;
        HRESULT hr = ((LPDIRECT3DDEVICE3)(g_lpD3Ddevice))->SetRenderState(D3DRENDERSTATE_FOGTABLESTART, *(DWORD*)&m_psCurrentRS->fFogStart);
#line 1698
        CheckHResult(hr, "SetRenderStateForGlobal failed");
        m_psSetRS->fFogEnd = m_psCurrentRS->fFogEnd = end;
        hr = ((LPDIRECT3DDEVICE3)(g_lpD3Ddevice))->SetRenderState(D3DRENDERSTATE_FOGTABLEEND, *(DWORD*)&m_psCurrentRS->fFogEnd);

        CheckHResult(hr, "SetRenderStateForGlobal failed");
    }
}

void cD6States::SetLinearFogDistance(float param)
{
    float start;
    float end;

    if (g_bWFog)
    {
        SetFogStartAndEnd(1.0f, param);
    }
    else
    {
        end = param;
        if (z_far < param)
            end = (float)z_far;
        if (z_near > param)
            start = (float)z_near;
        else
            start = (float)((param - z_near) * 0.2 + z_near);
        SetFogStartAndEnd(start, end);
    }
}


void cD6States::SetAlphaColor(float alpha)
{
    int val = (int)(alpha * 255);
    if (val > 0xFF) val = 0xFF;
    if (val < 0)    val = 0;
    if (m_psCurrentRS->nAlphaColor != val) {
        pcRenderBuffer->FlushPrimitives();
        m_psCurrentRS->nAlphaColor = val;
        m_psSetRS->nAlphaColor = val;
        pcRenderBuffer->PassAlphaColor(val);
    }
}

void cD6States::EnableAlphaBlending(BOOL enable)
{
    if (enable != m_psCurrentRS->bAlphaOn || enable != m_psSetRS->bAlphaOn) {
        pcRenderBuffer->FlushPrimitives();
        m_psCurrentRS->bAlphaOn = enable;
        m_psSetRS->bAlphaOn = enable;
        if (PUNT_D3D) {
        } else {
            HRESULT hr = SET_RENDER_STATE(g_lpD3Ddevice, D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
#line 1776
            CheckHResult(hr, "SetRenderStateForGlobal failed");
        }
    }
}

void cD6States::SetAlphaModulateHack(uint mode)
{
    static D3DBLEND table[] = {
        D3DBLEND_ZERO, D3DBLEND_ONE, D3DBLEND_SRCCOLOR, D3DBLEND_DESTCOLOR
    };
    pcRenderBuffer->FlushPrimitives();
    m_psCurrentRS->eSrcAlpha = table[mode & 3];
    m_psSetRS->eSrcAlpha = m_psCurrentRS->eSrcAlpha;
    m_psCurrentRS->eDstAlpha = table[((int)mode >> 2) & 3];
    m_psSetRS->eDstAlpha = m_psCurrentRS->eDstAlpha;
    if (PUNT_D3D) {
    } else {
        HRESULT hr = SET_RENDER_STATE(g_lpD3Ddevice, D3DRENDERSTATE_SRCBLEND, m_psCurrentRS->eSrcAlpha);
#line 1811
        CheckHResult(hr, "SetRenderStateForGlobal failed");
        hr = SET_RENDER_STATE(g_lpD3Ddevice, D3DRENDERSTATE_DESTBLEND, m_psCurrentRS->eDstAlpha);
#line 1812
        CheckHResult(hr, "SetRenderStateForGlobal failed");
    }
}

void cD6States::ResetDefaultAlphaModulate()
{
    pcRenderBuffer->FlushPrimitives();
    m_psCurrentRS->eSrcAlpha = D3DBLEND_SRCALPHA;
    m_psSetRS->eSrcAlpha = D3DBLEND_SRCALPHA;
    m_psCurrentRS->eDstAlpha = D3DBLEND_INVSRCALPHA;
    m_psSetRS->eDstAlpha = D3DBLEND_INVSRCALPHA;
    if (PUNT_D3D) {
    } else {
        HRESULT hr = SET_RENDER_STATE(g_lpD3Ddevice, D3DRENDERSTATE_SRCBLEND, m_psCurrentRS->eSrcAlpha);
#line 1832
        CheckHResult(hr, "SetRenderStateForGlobal failed");
        hr = SET_RENDER_STATE(g_lpD3Ddevice, D3DRENDERSTATE_DESTBLEND, m_psCurrentRS->eDstAlpha);
#line 1833
        CheckHResult(hr, "SetRenderStateForGlobal failed");
    }
}

void cD6States::EnableFog(BOOL enable)
{
    if (g_bUseTableFog || g_bUseVertexFog) {
        pcRenderBuffer->FlushPrimitives();
        m_psCurrentRS->bFogOn = enable;
        m_psSetRS->bFogOn = enable;
        HRESULT hr = SET_RENDER_STATE(g_lpD3Ddevice, D3DRENDERSTATE_FOGENABLE, m_psCurrentRS->bFogOn);
#line 1926
        CheckHResult(hr, "SetRenderStateForGlobal failed");
    }
}

void cD6States::SetFogSpecularLevel(float level)
{
    DWORD dwVal = ((int)((1.0f - level) * 255.0f)) << 24;
    if (m_psCurrentRS->dcFogSpecular != (int)dwVal) {
        pcRenderBuffer->FlushPrimitives();
        m_psCurrentRS->dcFogSpecular = dwVal;
        m_psSetRS->dcFogSpecular = dwVal;
        pcRenderBuffer->PassFogSpecularColor(m_psCurrentRS->dcFogSpecular);
    }
}

void cD6States::SetFogColor(int r, int g, int b)
{
    if (g_bUseTableFog || g_bUseVertexFog) {
        pcRenderBuffer->FlushPrimitives();
        if (r > 0xFF) r = 0xFF; else if (r < 0) r = 0;
        if (g > 0xFF) g = 0xFF; else if (g < 0) g = 0;
        if (b > 0xFF) b = 0xFF; else if (b < 0) b = 0;
        m_psCurrentRS->dwFogColor = (r << 16) | (g << 8) | b;
        m_psSetRS->dwFogColor = m_psCurrentRS->dwFogColor;
        if (!PUNT_D3D) {
            HRESULT hr = SET_RENDER_STATE(g_lpD3Ddevice, D3DRENDERSTATE_FOGCOLOR, m_psCurrentRS->dwFogColor);
#line 1986
            CheckHResult(hr, "SetRenderStateForGlobal failed");
        }
    }
}

DWORD cD6States::get_color()
{
    int lgd3d_alpha;
    DWORD color;

    lgd3d_alpha = m_psCurrentRS->nAlphaColor;
    if (m_psCurrentRS->bUsePalette != 0) {
        int index;
        index = grd_canvas->gc.fcolor & 0xff;
        switch (grd_canvas->gc.fill_type) {
        default:
            Warning(("lgd3d: unsupported fill type (%i)", grd_canvas->gc.fill_type));
        case FILL_NORM:
            break;
        case FILL_CLUT:
            index = (uint)((uchar*)grd_canvas->gc.fill_parm)[index];
            break;
        case FILL_SOLID:
            index = grd_canvas->gc.fill_parm;
        }
        if (lgd3d_clut) {
            index = lgd3d_clut[index];
        }
        int i = index * 3;
        color = (lgd3d_alpha << 24) | (grd_pal[i] << 16) | (grd_pal[i + 1] << 8) | grd_pal[i + 2];
    } else {
        switch (grd_canvas->gc.fill_type) {
        default:
            Warning(("lgd3d: unsupported fill type (%i)", grd_canvas->gc.fill_type));
        case FILL_NORM:
            color = grd_canvas->gc.fcolor;
            break;
        case FILL_SOLID:
            color = grd_canvas->gc.fill_parm;
        }
        color = (color & 0xffffff) + lgd3d_alpha * 0x1000000;
    }
    return color;
}





void cD6States::GetTexBlendingModes(ulong* p0, ulong* p1)
{
    if (p0) *p0 = m_psCurrentRS->saTexBlend[1].dwColorArg2;
    if (p1) *p1 = m_psCurrentRS->saTexBlend[1].eAlphaOperation;
}

void cD6States::SetTextureMapMode(ulong flag)
{
    if (flag >= 3)
    {
        Warning(("cD6States::SetTextureMapMode -- invalid mode %d", flag));
        return;
    }
    ulong local_flag = flag;
    ulong mode;
    if (local_flag != 0)
    {
        if (local_flag != 1)
        {
            mode = local_flag;
        }
        else
        {
            if (m_bCanModulateAlpha != 0)
                mode = local_flag;
            else
                mode = 0;
        }
    }
    else
    {
        if (m_bCanModulate != 0)
            mode = local_flag;
        else
            mode = 1;
    }
    m_psCurrentRS->dwTexBlendMode[0] = mode;
}

void cD6States::SetTextureNow()
{
    HRESULT hr;
    g_bTexSuspended = 0;
    sTextureData *pEntry = &g_saTextures[m_psCurrentRS->naTextureId[0]];
    hr = ((LPDIRECT3DDEVICE3)(g_lpD3Ddevice))->SetTexture(0, (LPDIRECT3DTEXTURE2)pEntry->lpTexture);
#line 2055
    CheckHResult(hr, "SetTexture failed");
}


void cD6States::set_texture_id(int n)
{
    uchar pixel_flags;
    sTextureData* psTD;

    if (PUNT_D3D) {
    } else {
        m_psCurrentRS->naTextureId[0] = n;
        if (m_psSetRS->naTextureId[0] != m_psCurrentRS->naTextureId[0]) {
            pcRenderBuffer->FlushPrimitives();
            if (m_psCurrentRS->naTextureId[0] == -2) {
                g_tmgr->set_texture_callback();
            } else {
            if (m_psCurrentRS->naTextureId[0] == -1) {
#line 2083
                D6CheckedSetTexture(0, NULL);
                m_psSetRS->naTextureId[0] = -1;
                if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn) {



                    D6CheckedSetRS(D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
                    m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
                }
                goto sync_state;
            }



            AssertTexId(m_psCurrentRS->naTextureId[0]);

            psTD = &g_saTextures[m_psCurrentRS->naTextureId[0]];
            pixel_flags = psTD->TdrvCookie.flags;
            m_psCurrentRS->baChromaKeying[0] = (pixel_flags & PF_TRANS) != 0;
            if (m_psCurrentRS->baChromaKeying[0] != 0) {
                m_psCurrentRS->eMagTexFilter = D3DTFN_POINT;
                m_psCurrentRS->eMinTexFilter = D3DTFN_POINT;
            } else {
                m_psCurrentRS->eMagTexFilter = D3DTFN_LINEAR;
                m_psCurrentRS->eMinTexFilter = D3DTFN_LINEAR;
            }
            if (m_psSetRS->baChromaKeying[0] != m_psCurrentRS->baChromaKeying[0]) {
#line 2123
                D6CheckedSetRS(D3DRENDERSTATE_COLORKEYENABLE, m_psCurrentRS->baChromaKeying[0]);
                m_psSetRS->baChromaKeying[0] = m_psCurrentRS->baChromaKeying[0];

                D6CheckedSetTSS(0, D3DTSS_MAGFILTER, m_psCurrentRS->eMagTexFilter);
                m_psSetRS->eMagTexFilter = m_psCurrentRS->eMagTexFilter;

                D6CheckedSetTSS(0, D3DTSS_MAGFILTER, m_psCurrentRS->eMinTexFilter);
                m_psSetRS->eMinTexFilter = m_psCurrentRS->eMinTexFilter;
            }

            pixel_flags &= 7;
            m_psCurrentRS->baTextureWithAlpha[0] = (pixel_flags == 2 || pixel_flags == 3 || pixel_flags == 4) ? 1 : 0;
            if (m_psCurrentRS->baTextureWithAlpha[0] != 0) {
                if (m_psSetRS->bAlphaOn == 0) {





                    D6CheckedSetRS(D3DRENDERSTATE_ALPHABLENDENABLE, 1);
                    m_psSetRS->bAlphaOn = 1;
                }
            } else {
                if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn) {

                    D6CheckedSetRS(D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
                    m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
                }
            }

            if ((m_psSetRS->dwTexBlendMode[0] == m_psCurrentRS->dwTexBlendMode[0]) &&
                (m_psSetRS->baTextureWithAlpha[0] == m_psCurrentRS->baTextureWithAlpha[0]))
                goto tss_done;
            memcpy(m_psCurrentRS->saTexBlend, sTexBlendArgsProtos[m_psCurrentRS->dwTexBlendMode[0]], sizeof(sTexBlendArgs));
            if ((m_psCurrentRS->dwTexBlendMode[0] == 0) && (m_psCurrentRS->baTextureWithAlpha[0] != 0)) {
                m_psCurrentRS->saTexBlend[0].eAlphaOperation = D3DTOP_SELECTARG1;
            }



            D6CheckedSetTSS(0, D3DTSS_TEXCOORDINDEX, 0);
            D6CheckedSetTexBlend(0, m_psCurrentRS->saTexBlend);
            m_psSetRS->dwTexBlendMode[0] = m_psCurrentRS->dwTexBlendMode[0];
            m_psSetRS->baTextureWithAlpha[0] = m_psCurrentRS->baTextureWithAlpha[0];
tss_done:
            SetTextureNow();
sync_state:
            m_psSetRS->naTextureId[0] = m_psCurrentRS->naTextureId[0];
            }
        } else {
            if (m_psCurrentRS->baTextureWithAlpha[0] != 0) {
                if (m_psSetRS->bAlphaOn == 0) {
#line 2189
                    D6CheckedSetRS(D3DRENDERSTATE_ALPHABLENDENABLE, 1);
                    m_psSetRS->bAlphaOn = 1;
                }
                } else {
            if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn) {

                D6CheckedSetRS(D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
                m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
            }
            }
        }
    }
}

void cD6States::EnablePalette(BOOL enable)
{
    pcRenderBuffer->FlushPrimitives();
    m_psCurrentRS->bUsePalette = enable;
}

void cD6States::SetPalSlotFlags(uint start, uint n, uchar* pal, int slot, int flags)
{
    if (g_lpDD_ext == NULL) return;
    {
        ulong bitmask[4];
        if ((flags & 2) != 0 && (lgd3d_get_trans_texture_bitmask((grs_rgb_bitmask*)bitmask), ((grs_rgb_bitmask*)bitmask)->red != 0)) {
            if (texture_pal_trans[slot] == NULL) {
                texture_pal_trans[slot] = (ushort*)(*gr_malloc)(0x200);
            }
            lgd3d_get_trans_texture_bitmask((grs_rgb_bitmask*)bitmask);
            gr_make_pal16(start, n, texture_pal_trans[slot], pal, (grs_rgb_bitmask*)bitmask);
            {
                texture_pal_trans[slot][0] &= (ushort)(~((grs_rgb_bitmask*)bitmask)->alpha);
                for (int i = 1; i < 0x100; i++) {
                    texture_pal_trans[slot][i] |= ((grs_rgb_bitmask*)bitmask)->alpha;
                }
            }
        }
        if ((flags & 1) != 0 || ((flags & 2) != 0 && ((grs_rgb_bitmask*)bitmask)->red == 0)) {
            if (m_bTexture_RGB == 0) {
                SetTexturePalette(start, n, pal, slot);
            } else {
                if (texture_pal_opaque[slot] == NULL) {
                    texture_pal_opaque[slot] = (ushort*)(*gr_malloc)(0x200);
                }
                lgd3d_get_opaque_texture_bitmask((grs_rgb_bitmask*)bitmask);
                gr_make_pal16(start, n, texture_pal_opaque[slot], pal, (grs_rgb_bitmask*)bitmask);
            }
        }
    }
}

void cD6States::SetTexturePalette(uint start, uint n, uchar *pal, int slot)
{
   PALETTEENTRY         peColorTable[256];
   HRESULT              hRes;
   int i;

   if (lpDDPalTexture[slot] == NULL)
   {
      hRes = ((LPDIRECTDRAW)g_lpDD_ext)->CreatePalette(DDPCAPS_8BIT | DDPCAPS_ALLOW256,
                                 peColorTable,
                                 &lpDDPalTexture[slot],
                                 NULL);
#line 2284
      CheckHResult(hRes, "CreatePalette failed.");
   }

   for (i = 0; i < (int)n; i++) {
      peColorTable[i].peFlags = D3DPAL_READONLY | PC_RESERVED;
      peColorTable[i].peRed = pal[3*i];
      peColorTable[i].peGreen = pal[3*i+1];
      peColorTable[i].peBlue = pal[3*i+2];
   }

   hRes = lpDDPalTexture[slot]->SetEntries(0, start, n, peColorTable);



   CheckHResult(hRes, "SetEntries failed.");
}


cImStates* cImStates::m_Instance = NULL;

void cD6States::SetAlphaPalette(ushort* pal)
{
    pcRenderBuffer->FlushPrimitives();
    alpha_pal = pal;
}

void cD6States::SetChromaKey(int r, int g, int b)
{
    pcRenderBuffer->FlushPrimitives();
    m_psCurrentRS->chroma_r = r;
    m_psCurrentRS->chroma_g = g;
    m_psCurrentRS->chroma_b = b;
    if (g_RGBTextureFormat.dwRBitMask == 0x1f) {
        r = m_psCurrentRS->chroma_b;
        b = m_psCurrentRS->chroma_r;
    }
    m_psCurrentRS->chroma_key = b >> 3;
    if (g_RGBTextureFormat.dwGBitMask == GBitMask15)
        m_psCurrentRS->chroma_key += ((g >> 3) << 5) + ((r >> 3) << 10);
    else
        m_psCurrentRS->chroma_key += ((g >> 2) << 5) + ((r >> 3) << 11);
}

BOOL cD6States::SetTexWrapping(DWORD dwLevel, BOOL bSetSmooth)
{
    int old = (m_psCurrentRS->eWrap[dwLevel] == D3DTADDRESS_WRAP);
    if (old != bSetSmooth) {
        pcRenderBuffer->FlushPrimitives();
        m_psCurrentRS->eWrap[dwLevel] = bSetSmooth ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
        m_psSetRS->eWrap[dwLevel] = bSetSmooth ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
        if (!PUNT_D3D) {
            HRESULT hr = ((LPDIRECT3DDEVICE3)(g_lpD3Ddevice))->SetTextureStageState(dwLevel, D3DTSS_ADDRESS, m_psCurrentRS->eWrap[dwLevel]);
#line 2360
            CheckHResult(hr, "SetTextureStageState failed");
        }
    }
    return old;
}

cD6States* cImStates::Instance()
{
    if (m_Instance == NULL)
        m_Instance = new cImStates();
    return m_Instance;
}





cMSStates* cMSStates::m_Instance = NULL;

cD6States* cImStates::DeInstance()
{
    if (m_Instance) {
        delete m_Instance;
        m_Instance = NULL;
    }
    return m_Instance;
}

cImStates::cImStates() {}

cImStates::~cImStates()
{
    int j;
    IWinDisplayDevice* pDev;
    sTextureData* psTD;
    int i;

    if (m_bTextureListInitialized != 0) {
        if (g_tmgr) {
            g_tmgr->shutdown();
            g_tmgr = NULL;
        }
        pDev = (IWinDisplayDevice*)_AppGetAggregated(IID_IWinDisplayDevice);
        pDev->RemoveTaskSwitchCallback(callback_id);
        if (pDev)
            pDev->Release();
        pDev = NULL;
        psTD = g_saTextures;
        i = 0;
        for (; i < 1024; i++, psTD++) {
            if (psTD->lpTexture) {
                psTD->lpTexture->Release();
            }
            psTD->lpTexture = NULL;
            if (psTD->lpSurface) {
                psTD->lpSurface->Release();
            }
            psTD->lpSurface = NULL;
        }
        m_bTextureListInitialized = 0;
    }
    if (default_bm) {
        (*gr_free)(default_bm);
        default_bm = NULL;
    }
    for (j = 0; j < 0x100; j++) {
        if (texture_pal_opaque[j]) {
            (*gr_free)(texture_pal_opaque[j]);
            texture_pal_opaque[j] = NULL;
        }
        if (texture_pal_trans[j]) {
            (*gr_free)(texture_pal_trans[j]);
            texture_pal_trans[j] = NULL;
        }
        if (lpDDPalTexture[j]) {
            ((IUnknown*)lpDDPalTexture[j])->Release();
        }
        lpDDPalTexture[j] = NULL;
    }
    if (m_texture_size_list != 0) {
        free(m_texture_size_list);
        m_texture_size_list = 0;
    }
}

cD6States* cMSStates::Instance()
{
    if (m_Instance == NULL)
        m_Instance = new cMSStates();
    return m_Instance;
}

cD6States* cMSStates::DeInstance()
{
    if (m_Instance) {
        delete m_Instance;
        m_Instance = NULL;
    }
    return m_Instance;
}

cMSStates::cMSStates():
    m_dwCurrentTexLevel(0), m_bTexturePending(0)
{
}
void cMSStates::SetTextureLevel(int level)
{
    m_dwCurrentTexLevel = level;
}

cMSStates::~cMSStates()
{
    int j;
    IWinDisplayDevice* pDev;
    sTextureData* psTD;
    int i;

    if (m_bTextureListInitialized != 0) {
        if (g_tmgr) {
            g_tmgr->shutdown();
            g_tmgr = NULL;
        }
        pDev = (IWinDisplayDevice*)_AppGetAggregated(IID_IWinDisplayDevice);
        pDev->RemoveTaskSwitchCallback(callback_id);
        if (pDev)
            pDev->Release();
        pDev = NULL;
        psTD = g_saTextures;
        i = 0;
        for (; i < 1024; i++, psTD++) {
            if (psTD->lpTexture) {
                psTD->lpTexture->Release();
            }
            psTD->lpTexture = NULL;
            if (psTD->lpSurface) {
                psTD->lpSurface->Release();
            }
            psTD->lpSurface = NULL;
        }
        m_bTextureListInitialized = 0;
    }
    if (default_bm) {
        (*gr_free)(default_bm);
        default_bm = NULL;
    }
    for (j = 0; j < 0x100; j++) {
        if (texture_pal_opaque[j]) {
            (*gr_free)(texture_pal_opaque[j]);
            texture_pal_opaque[j] = NULL;
        }
        if (texture_pal_trans[j]) {
            (*gr_free)(texture_pal_trans[j]);
            texture_pal_trans[j] = NULL;
        }
        if (lpDDPalTexture[j]) {
            ((IUnknown*)lpDDPalTexture[j])->Release();
        }
        lpDDPalTexture[j] = NULL;
    }
    if (m_texture_size_list != 0) {
        free(m_texture_size_list);
        m_texture_size_list = 0;
    }
}

int cMSStates::Initialize(DWORD dwRequestedFlags)
{
    g_bPrefer_RGB = 1;
    return cD6States::Initialize(dwRequestedFlags);
}
int cMSStates::SetDefaultsStates(DWORD dwRequestedFlags)
{
    DWORD dwPasses;
    HRESULT hR;

    this->SetCommonDefaultStates(dwRequestedFlags, 1);

    m_psCurrentRS->eSrcAlpha = D3DBLEND_SRCALPHA;
    m_psCurrentRS->eDstAlpha = D3DBLEND_INVSRCALPHA;
    m_psCurrentRS->bAlphaOn = 0;

#line 2656
    D6CheckedSetRS(D3DRENDERSTATE_SRCBLEND, m_psCurrentRS->eSrcAlpha);
    D6CheckedSetRS(D3DRENDERSTATE_DESTBLEND, m_psCurrentRS->eDstAlpha);
    D6CheckedSetRS(D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);


    D6CheckedSetTSS(1, D3DTSS_MAGFILTER, m_psCurrentRS->eMagTexFilter);
    D6CheckedSetTSS(1, D3DTSS_MINFILTER, m_psCurrentRS->eMinTexFilter);

    m_psCurrentRS->eWrap[1] = D3DTADDRESS_WRAP;
    D6CheckedSetTSS(1, D3DTSS_ADDRESS, m_psCurrentRS->eWrap[0]);


    AssertMsg(dwRequestedFlags & LGD3DF_MULTI_TEXTURING, "Error 666");



    if (dwRequestedFlags & LGD3DF_MULTITEXTURE_COLOR) {
        memcpy(m_psCurrentRS->saTexBlend, sMultiTexBlendArgsProtos, sizeof(m_psCurrentRS->saTexBlend));
#line 2684
        D6CheckedSetTSS(0, D3DTSS_TEXCOORDINDEX, 0);
        D6CheckedSetTexBlend(0, m_psCurrentRS->saTexBlend);

        D6CheckedSetTSS(1, D3DTSS_TEXCOORDINDEX, 1);
        D6CheckedSetTexBlend(1, &m_psCurrentRS->saTexBlend[1]);

        hR = IDirect3DDevice3_ValidateDevice(g_lpD3Ddevice, &dwPasses);
        if (hR != 0) {
            SetLGD3DErrorCode(LGD3D_EC_VD_SPASS_MT, hR);
#ifndef SHIP
            if (bSpewOn != 0) {
#line 2692
                CriticalLGD3DError(LGD3D_EC_VD_SPASS_MT, hR);
            } else {
                WarnLGD3DError(LGD3D_EC_VD_SPASS_MT, hR);
            }
#endif
            lgd3d_g_bInitialized = 0;
            return 0;
        }
    }

    if (dwRequestedFlags & LGD3DF_MT_BLENDDIFFUSE) {
        memcpy(m_psCurrentRS->saTexBlend, sMultiTexBlendArgsProtos[2], sizeof(m_psCurrentRS->saTexBlend));
#line 2701
        D6CheckedSetTSS(0, D3DTSS_TEXCOORDINDEX, 0);
        D6CheckedSetTexBlend(0, m_psCurrentRS->saTexBlend);

        D6CheckedSetTSS(1, D3DTSS_TEXCOORDINDEX, 1);
        D6CheckedSetTexBlend(1, &m_psCurrentRS->saTexBlend[1]);

        hR = IDirect3DDevice3_ValidateDevice(g_lpD3Ddevice, &dwPasses);
        if (hR != 0) {
            return 0;
        }
        m_psCurrentRS->dwTexBlendMode[1] = 2;
    }

    memcpy(m_psCurrentRS->saTexBlend,
        sTexBlendArgsProtos[m_psCurrentRS->dwTexBlendMode[1]],
        sizeof(m_psCurrentRS->saTexBlend));




    D6CheckedSetTSS(0, D3DTSS_TEXCOORDINDEX, 0);
    D6CheckedSetTexBlend(0, m_psCurrentRS->saTexBlend);

    D6CheckedSetTSS(1, D3DTSS_TEXCOORDINDEX, 1);
    D6CheckedSetTexBlend(1, &m_psCurrentRS->saTexBlend[1]);

    memcpy(m_psSetRS, m_psCurrentRS, sizeof(sRenderStates));
    g_dwRSChangeFlags = 0;
    return 1;
}
void cMSStates::set_texture_id(int n)
{
    m_bTexturePending = 1;
    if (m_psSetRS->naTextureId[m_dwCurrentTexLevel] != n) {
        pcRenderBuffer->FlushPrimitives();
        m_psCurrentRS->naTextureId[m_dwCurrentTexLevel] = n;
        if (m_dwCurrentTexLevel == 1)
            m_LastLightMapBm = g_saTextures[n].pTdrvBitmap;
        if (m_psCurrentRS->naTextureId[m_dwCurrentTexLevel] == -2)
            g_tmgr->set_texture_callback();
    } else
        m_psCurrentRS->naTextureId[m_dwCurrentTexLevel] = n;
}

int cMSStates::EnableMTMode(DWORD dwMTOn)
{
    DWORD dwLevel;
    sTexBlendArgs (*sTexArgs)[2];
    int bSetTexture;
    DWORD dwNoLMLevels;
    sTextureData* psTD;
    uchar pixel_flags;

    if (!m_bTexturePending) {
        if (m_psCurrentRS->naTextureId[0] != -1) {
            if (m_psCurrentRS->baTextureWithAlpha[0] || m_psCurrentRS->baTextureWithAlpha[1]) {
                if (!m_psSetRS->bAlphaOn) {
#line 2784
                    D6CheckedSetRS(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
                    m_psSetRS->bAlphaOn = TRUE;
                }
            } else if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn) {



                D6CheckedSetRS(D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
                m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
            }
        }
        return TRUE;
    }

    m_bTexturePending = FALSE;
    if (dwMTOn) {
        dwNoLMLevels = 1;
        sTexArgs = sMultiTexBlendArgsProtos;
    } else {
        dwNoLMLevels = 0;
        sTexArgs = sTexBlendArgsProtos;
    }
    m_psCurrentRS->dwNoLightMapLevels = dwNoLMLevels;
    bSetTexture = FALSE;
    if (dwMTOn) {
        if (m_psCurrentRS->naTextureId[0] == -1) {
            Warning(("\nMulti texturing with no textures?!\n\n"));
            return FALSE;
        }
        if (m_psCurrentRS->naTextureId[1] == -1) {
            lgd3d_set_texture_level(1);
            if (g_tmgr)
                g_tmgr->set_texture(m_LastLightMapBm);
            lgd3d_set_texture_level(0);
            Warning(("\nSwap-out inside texure pair!, Trying to reload bitmap %p\n\n", m_LastLightMapBm));
        }
        if (m_psSetRS->naTextureId[0] != m_psCurrentRS->naTextureId[0]
            || m_psSetRS->naTextureId[1] != m_psCurrentRS->naTextureId[1]) {
            bSetTexture = TRUE;
            for (dwLevel = 0; dwLevel < 2; ++dwLevel) {
                if (m_psSetRS->naTextureId[dwLevel] != m_psCurrentRS->naTextureId[dwLevel]) {
#line 2864
                    AssertTexId(m_psCurrentRS->naTextureId[dwLevel]);
                    psTD = &g_saTextures[m_psCurrentRS->naTextureId[dwLevel]];
                    pixel_flags = psTD->TdrvCookie.flags;
                    (&m_psCurrentRS->baChromaKeying[0])[dwLevel] = (pixel_flags & PF_TRANS) != 0;
                    if ((&m_psSetRS->baChromaKeying[0])[dwLevel] != (&m_psCurrentRS->baChromaKeying[0])[dwLevel]) {
#line 2875
                        D6CheckedSetRS(D3DRENDERSTATE_COLORKEYENABLE, (&m_psCurrentRS->baChromaKeying[0])[dwLevel]);
                        (&m_psSetRS->baChromaKeying[0])[dwLevel] = (&m_psCurrentRS->baChromaKeying[0])[dwLevel];
                    }
                }
            }
        }
        if (m_psSetRS->dwNoLightMapLevels != m_psCurrentRS->dwNoLightMapLevels
            || m_psSetRS->dwTexBlendMode[0] != m_psCurrentRS->dwTexBlendMode[0]) {
            bSetTexture = TRUE;
            memcpy(m_psCurrentRS->saTexBlend, sTexArgs[m_psCurrentRS->dwTexBlendMode[dwNoLMLevels]], sizeof(m_psCurrentRS->saTexBlend));
        }
        if (bSetTexture) {
            if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn) {
#line 2901
                D6CheckedSetRS(D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
                m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
            }



            AssertTexId(m_psCurrentRS->naTextureId[0]);
            psTD = &g_saTextures[m_psCurrentRS->naTextureId[0]];


            D6CheckedSetTSS(0, D3DTSS_TEXCOORDINDEX, 0);
            D6CheckedSetTexBlend(0, m_psCurrentRS->saTexBlend);
            D6CheckedSetTexture(0, psTD->lpTexture);
            m_psSetRS->naTextureId[0] = m_psCurrentRS->naTextureId[0];



            AssertTexId(m_psCurrentRS->naTextureId[1]);
            psTD = &g_saTextures[m_psCurrentRS->naTextureId[1]];

            D6CheckedSetTSS(1, D3DTSS_TEXCOORDINDEX, 1);
            D6CheckedSetTexBlend(1, &m_psCurrentRS->saTexBlend[1]);
            D6CheckedSetTexture(1, psTD->lpTexture);
            m_psSetRS->naTextureId[1] = m_psCurrentRS->naTextureId[1];
            m_psSetRS->dwNoLightMapLevels = m_psCurrentRS->dwNoLightMapLevels;
        }
    } else {
        if (m_psSetRS->naTextureId[0] != m_psCurrentRS->naTextureId[0]) {
            if (m_psCurrentRS->naTextureId[0] == -1) {
#line 2940
                D6CheckedSetTexture(0, NULL);
                m_psSetRS->naTextureId[0] = -1;
                m_psCurrentRS->baTextureWithAlpha[0] = FALSE;
                if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn) {


                    D6CheckedSetRS(D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
                    m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
                }
            } else {
                bSetTexture = TRUE;
#line 2958
                AssertTexId(m_psCurrentRS->naTextureId[0]);
                psTD = &g_saTextures[m_psCurrentRS->naTextureId[0]];
                pixel_flags = psTD->TdrvCookie.flags;
                m_psCurrentRS->baChromaKeying[0] = (pixel_flags & PF_TRANS) != 0;
                if (m_psCurrentRS->baChromaKeying[0]) {
                    m_psCurrentRS->eMagTexFilter = D3DTFN_POINT;
                    m_psCurrentRS->eMinTexFilter = D3DTFN_POINT;
                } else {
                    m_psCurrentRS->eMagTexFilter = D3DTFN_LINEAR;
                    m_psCurrentRS->eMinTexFilter = D3DTFN_LINEAR;
                }
                if (m_psSetRS->baChromaKeying[0] != m_psCurrentRS->baChromaKeying[0]) {
#line 2980
                    D6CheckedSetRS(D3DRENDERSTATE_COLORKEYENABLE, m_psCurrentRS->baChromaKeying[0]);
                    m_psSetRS->baChromaKeying[0] = m_psCurrentRS->baChromaKeying[0];

                    D6CheckedSetTSS(0, D3DTSS_MAGFILTER, m_psCurrentRS->eMagTexFilter);
                    m_psSetRS->eMagTexFilter = m_psCurrentRS->eMagTexFilter;

                    D6CheckedSetTSS(0, D3DTSS_MAGFILTER, m_psCurrentRS->eMinTexFilter);
                    m_psSetRS->eMinTexFilter = m_psCurrentRS->eMinTexFilter;
                }
                pixel_flags &= 7;
                m_psCurrentRS->baTextureWithAlpha[0] = pixel_flags == 2 || pixel_flags == 3 || pixel_flags == 4;
                if (m_psCurrentRS->baTextureWithAlpha[0]) {
                    if (!m_psSetRS->bAlphaOn) {
#line 2999
                        D6CheckedSetRS(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
                        m_psSetRS->bAlphaOn = TRUE;
                    }
                } else if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn) {



                    D6CheckedSetRS(D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
                    m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
                }
            }
        } else {
            if (m_psCurrentRS->baTextureWithAlpha[0]) {
                if (!m_psSetRS->bAlphaOn) {
#line 3019
                    D6CheckedSetRS(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
                    m_psSetRS->bAlphaOn = TRUE;
                }
            } else if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn) {


                D6CheckedSetRS(D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
                m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
            }
        }
        if (m_psSetRS->dwNoLightMapLevels != m_psCurrentRS->dwNoLightMapLevels
            || m_psSetRS->dwTexBlendMode[0] != m_psCurrentRS->dwTexBlendMode[0]
            || m_psSetRS->baTextureWithAlpha[0] != m_psCurrentRS->baTextureWithAlpha[0]) {
            bSetTexture = TRUE;
            memcpy(m_psCurrentRS->saTexBlend, sTexArgs[m_psCurrentRS->dwTexBlendMode[dwNoLMLevels]], sizeof(m_psCurrentRS->saTexBlend));
            if (!m_psCurrentRS->dwTexBlendMode[0] && m_psCurrentRS->baTextureWithAlpha[0])
                m_psCurrentRS->saTexBlend[0].eAlphaOperation = D3DTOP_SELECTARG1;
            m_psSetRS->baTextureWithAlpha[0] = m_psCurrentRS->baTextureWithAlpha[0];
        }
        if (bSetTexture) {
#line 3054
            D6CheckedSetTSS(0, D3DTSS_TEXCOORDINDEX, 0);
            D6CheckedSetTexBlend(0, m_psCurrentRS->saTexBlend);
            D6CheckedSetTSS(1, D3DTSS_TEXCOORDINDEX, 1);
            D6CheckedSetTexBlend(1, &m_psCurrentRS->saTexBlend[1]);
            m_psSetRS->dwNoLightMapLevels = m_psCurrentRS->dwNoLightMapLevels;
            if (m_psCurrentRS->naTextureId[0] != -1) {
#line 3067
                AssertTexId(m_psCurrentRS->naTextureId[0]);
                psTD = &g_saTextures[m_psCurrentRS->naTextureId[0]];

                D6CheckedSetTexture(0, psTD->lpTexture);
                m_psSetRS->naTextureId[0] = m_psCurrentRS->naTextureId[0];
                m_psCurrentRS->naTextureId[1] = -1;


                D6CheckedSetTexture(1, NULL);
                m_psSetRS->naTextureId[1] = m_psCurrentRS->naTextureId[1];
            }
        }
    }
    return TRUE;
}

void cMSStates::TurnOffTexuring(int bTexOff)
{
    HRESULT hr;
    HRESULT hrDev;

    if (bTexOff) {
        m_psCurrentRS->dwNoLightMapLevels = 0;
        hr = ((LPDIRECT3DDEVICE3)(g_lpD3Ddevice))->SetTexture(0, NULL);
#line 3097
        CheckHResult(hr, "SetTexture failed");
        m_psSetRS->naTextureId[0] = -1;
        m_psCurrentRS->baTextureWithAlpha[0] = 0;
        if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn) {
            hrDev = SET_RS(g_lpD3Ddevice, D3DRENDERSTATE_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);

            CheckHResult(hrDev, "SetRenderStateForGlobal failed");
            m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
        }
        if ((m_psSetRS->dwNoLightMapLevels != m_psCurrentRS->dwNoLightMapLevels) ||
            (m_psSetRS->dwTexBlendMode[0] != m_psCurrentRS->dwTexBlendMode[0]) ||
            (m_psSetRS->baTextureWithAlpha[0] != m_psCurrentRS->baTextureWithAlpha[0])) {
            memcpy(m_psCurrentRS->saTexBlend, sTexBlendArgsProtos[m_psCurrentRS->dwTexBlendMode[0]], sizeof(m_psCurrentRS->saTexBlend));
            if ((m_psCurrentRS->dwTexBlendMode[0] == 0) && (m_psCurrentRS->baTextureWithAlpha[0] != 0)) {
                m_psCurrentRS->saTexBlend[0].eAlphaOperation = D3DTOP_SELECTARG1;
            }
            m_psSetRS->baTextureWithAlpha[0] = m_psCurrentRS->baTextureWithAlpha[0];
#line 3125
            D6CheckedSetTSS(0, D3DTSS_TEXCOORDINDEX, 0);
            D6CheckedSetTexBlend(0, m_psCurrentRS->saTexBlend);
            D6CheckedSetTSS(1, D3DTSS_TEXCOORDINDEX, 1);
            D6CheckedSetTexBlend(1, &m_psCurrentRS->saTexBlend[1]);
            m_psSetRS->dwNoLightMapLevels = m_psCurrentRS->dwNoLightMapLevels;
        }
        m_bTexturePending = 1;
        g_bTexSuspended = 1;
    } else {
        g_bTexSuspended = 0;
        m_bTexturePending = 1;
    }
}

int cMSStates::reload_texture(tdrv_texture_info* info)
{
    DWORD cookie;
    unsigned int cookieType;
    IDirectDrawSurface4** pDeviceSurface;
    IDirect3DTexture2* pDeviceTexture;
    DDSURFACEDESC2 ddsd;
    DWORD LastError;
    IDirect3DTexture2* SysmemTexture;
    IDirectDrawSurface4* SysmemSurface;
    DDPIXELFORMAT* pixel_format;
    sTextureData* psTexData;
    DWORD dwPFFlags;
    IDirectDrawSurface4* DeviceSurface;

    psTexData = &g_saTextures[info->id];
    pDeviceTexture = NULL;
    pDeviceSurface = &psTexData->lpSurface;
    cookie = info->cookie;
    cookieType = ((unsigned char*)&cookie)[2];
    pixel_format = g_FormatList[cookieType & 7];
    if ((cookieType & 7) == 4 && g_b8888supported == 0) {
        pixel_format = g_FormatList[2];
    }

    dwPFFlags = pixel_format->dwFlags;
    if (dwPFFlags == 0) {
        return -1;
    }

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    ddsd.dwHeight = info->h;
    ddsd.dwWidth = info->w;
    memcpy(&ddsd.ddpfPixelFormat, pixel_format, sizeof(DDPIXELFORMAT));

    LastError = this->CreateDDSurface(*(d3d_cookie*)&cookie, &ddsd, &SysmemSurface);





    CheckHResult(LastError, "CreateDDSurface() failed");
    AssertMsg(!(SysmemSurface == NULL), "NULL SysmemSurface");

    SysmemTexture = NULL;
    LastError = SysmemSurface->QueryInterface(IID_IDirect3DTexture2, (void**)&SysmemTexture);
#ifndef SHIP


    CheckHResult(LastError, "Failed to obtain D3D texture interface for sysmem surface");
#endif

    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_TEXTURESTAGE;
    ddsd.dwTextureStage = (cookieType >> 6) & 1;
    if (psTexData->lpTexture == NULL) {
        ddsd.ddsCaps.dwCaps = m_DeviceSurfaceCaps;
        LastError = this->CreateDDSurface(*(d3d_cookie*)&cookie, &ddsd, pDeviceSurface);

        if (LastError != 0 && m_bUsingLocalMem != 0 && m_bAGP_available != 0) {
#ifndef SHIP
            if (bSpewOn != 0) {
                mprintf("using nonlocal vidmem textures...\n");
            }
#endif
            m_bUsingLocalMem = 0;
            m_DeviceSurfaceCaps = NONLOCAL_CAPS;
            ddsd.ddsCaps.dwCaps = m_DeviceSurfaceCaps;
            LastError = this->CreateDDSurface(*(d3d_cookie*)&cookie, &ddsd, pDeviceSurface);
        }

        if (LastError != 0) {
#ifndef SHIP
            if (bSpewOn != 0) {
                mprintf("couldn't load texture: error %i.\n", LastError & 0xFFFF);
            }
#endif
            if (SysmemTexture) {
                SysmemTexture->Release();
            }
            SysmemTexture = NULL;
            if (SysmemSurface) {
                SysmemSurface->Release();
            }
            *pDeviceSurface = NULL;
            return -1;
        }

        DeviceSurface = *pDeviceSurface;
        if ((m_DeviceSurfaceCaps & MEMORY_TYPE_MASK) != (ddsd.ddsCaps.dwCaps & MEMORY_TYPE_MASK)) {
#ifndef SHIP
            if (bSpewOn != 0) {
                mprintf("device texture not created in requested memory location\nRequested %x Received %x\n",
                    m_DeviceSurfaceCaps & MEMORY_TYPE_MASK, ddsd.ddsCaps.dwCaps & MEMORY_TYPE_MASK);
            }
#endif
        }

        LastError = DeviceSurface->QueryInterface(IID_IDirect3DTexture2, (void**)&psTexData->lpTexture);



        CheckHResult(LastError, "Failed to obtain D3D texture interface for device surface.");
    }

    LastError = SysmemSurface->Lock(NULL, &ddsd, 0, NULL);

    CheckHResult(LastError, "Failed to lock sysmem surface.");

    this->LoadSurface(info, &ddsd);

    LastError = SysmemSurface->Unlock(NULL);
    CheckHResult(LastError, "Failed to unlock sysmem surface.");

    LastError = ((IDirect3DTexture2*)psTexData->lpTexture)->Load(SysmemTexture);





    CheckHResult(LastError, "Failed to load device texture from sysmem texture.");

    if (SysmemTexture) {
        SysmemTexture->Release();
    }
    SysmemTexture = NULL;
    if (SysmemSurface) {
        SysmemSurface->Release();
    }

    psTexData->pTdrvBitmap = info->bm;
    psTexData->TdrvCookie.value = info->cookie;

    return 0;
}

void cMSStates::cook_info(tdrv_texture_info* info)
{
    if (m_dwCurrentTexLevel == 0) {
        cD6States::cook_info(info);
    } else {
        d3d_cookie cookie;
        int v;
        int w;
        int h;

        cookie.palette = 0;
        if (grd_canvas->gc.fill_type == FILL_BLEND) {
            cookie.flags = PF_ALPHA;
        } else switch (info->bm->type) {
            case BMT_FLAT8:
                cookie.palette = info->bm->align;
                if ((info->bm->flags & BMF_TRANS) != 0) {
                    if (texture_pal_trans[cookie.palette] && g_TransRGBTextureFormat.dwFlags != 0) {
                        cookie.flags = 3;
                    } else {
                        cookie.flags = PF_TRANS;
                    }
                } else {
                    cookie.flags = PF_GENERIC;
                }
                break;
            case BMT_FLAT16:
                if (gr_test_bitmap_format(info->bm, BMF_RGB_4444)) {
                    cookie.flags = PF_ALPHA;
                } else if ((info->bm->flags & BMF_TRANS) != 0) {
                    if (g_TransRGBTextureFormat.dwFlags != 0) {
                        cookie.flags = 3;
                    } else {
                        cookie.flags = PF_TRANS | PF_RGB;
                    }
                } else {
                    cookie.flags = PF_RGB;
                }
                break;
            case BMT_FLAT32:
                cookie.flags = 4;
                break;
        }
        cookie.flags |= 0x40;
        calc_size(info, cookie);
        w = info->w;
        h = info->h;
        cookie.hlog = 0;
        cookie.wlog = 0;
        for (v = 2; v <= w; v += v) cookie.wlog++;
        for (v = 2; v <= h; v += v) cookie.hlog++;
#ifndef SHIP
        if (info->h == (1 << cookie.hlog) && info->w == (1 << cookie.wlog)) {
            ;
        } else {
#line 3369
            CriticalMsg("hlog/wlog does not match texture width/height");
        }
#endif
        info->cookie = cookie.value;
    }
}

void cMSStates::SetLightMapMode(ulong flag)
{
    if (flag >= 3) {
        Warning(("cMSStates::SetLightMapMode -- invalid mode", flag));
    } else {
        m_psCurrentRS->dwTexBlendMode[1] = flag;
    }
}

static void calc_size(tdrv_texture_info* info, d3d_cookie cookie)
{
    int* table;
    int w;
    int h;
    int w_power;
    int si;
    int h_power;
    grs_bitmap* bm;

    bm = info->bm;
    info->scale_h = 0;
    info->scale_w = 0;

    switch (cookie.flags & 7) {
        case 2: table = alpha_size_table; break;
        case 1: table = rgb_size_table; break;
        case 3: table = trgb_size_table; break;
        case 0: table = generic_size_table; break;
        case 4: table = b8888_size_table; break;
    }

    w_power = 0;
    for (w = 1; w_power < 9; w_power = w_power + 1, w = w + w) {
        if (w == bm->w) break;
    }

    h_power = 0;
    for (h = 1; h_power < 9; h_power = h_power + 1, h = h + h) {
        if (h == bm->h) break;
    }

    if (w_power >= 9 || h_power >= 9) {
#ifndef SHIP
        if (bSpewOn)
            mprintf("Unsupported texture size: w=%i h=%i\n", bm->w, bm->h);
#endif
        si = -1;
    } else {
        si = table[w_power * 9 + h_power];
        while (si < 0) {
            if (w_power < h_power) {
                w_power++;
                info->scale_w++;
            } else {
                h_power++;
                info->scale_h++;
                if (h_power >= 9) {
#ifndef SHIP
                    if (bSpewOn)
                        mprintf("Unsupported texture size: w=%i h=%i\n", bm->w, bm->h);
#endif
                    break;
                }
            }
            si = table[w_power * 9 + h_power];
        }
    }

    info->w = bm->w << info->scale_w;
    info->h = bm->h << info->scale_h;
    info->size_index = si;
}


static void GetAvailableTexMem(DWORD *local, DWORD *agp)
{
   DDSCAPS2 ddscaps;
   DWORD total;

#line 3475
   AssertMsg(NULL!=g_lpDD_ext, "No DirectDraw object!");
   memset(&ddscaps, 0, sizeof(ddscaps));
   if (local!=NULL) {
      ddscaps.dwCaps = LOCAL_CAPS;
      IDirectDraw4_GetAvailableVidMem((LPDIRECTDRAW4)g_lpDD_ext, &ddscaps, &total, local);
   } 
   if (agp!=NULL) {
      ddscaps.dwCaps = NONLOCAL_CAPS;
      IDirectDraw4_GetAvailableVidMem((LPDIRECTDRAW4)g_lpDD_ext, &ddscaps, &total, agp);
   } 
}

static void blit_8to16(tdrv_texture_info *info, ushort *dst, int drow, ushort *pal)
{
   int i, j;
   grs_bitmap *bm = info->bm;
   uchar *src = info->bits;

   for (i=0; i<bm->h; i++) {
      for (j=0; j<bm->w; j++)
         dst[j] = pal[src[j]];
      src += bm->row;
      dst += drow;
   }
}

static void blit_8to16_trans(tdrv_texture_info *info, ushort *dst, int drow, ushort *pal)
{
   int i, j;
   grs_bitmap *bm = info->bm;
   uchar *src = info->bits;
   int row = bm->row;
   int w = bm->w;
   int h = bm->h;

   for (i=0; i<h; i++) {
      for (j=0; j<w; j++) {
         uint k = src[j];
         if (k == 0) {
            if (0 < j)
               k = src[j-1];
            if (k == 0) {
               if (0 < i)
                  k = src[j-row];
               if (k == 0) {
                  if (j < w-1)
                     k = src[j+1];
                  if ((k == 0) && (i < h-1))
                     k = src[j+row];
               }
            }
            dst[j] = pal[k] & 0x7fff;
         } else
            dst[j] = pal[k];
      }
      src += row;
      dst += drow;
   }
}

static void blit_8to16_clut(tdrv_texture_info *info, ushort *dst, int drow, ushort *pal)
{
   int i, j;
   grs_bitmap *bm = info->bm;
   uchar *src = info->bits;

   for (i=0; i<bm->h; i++) {
      for (j=0; j<bm->w; j++)
         dst[j] = pal[texture_clut[src[j]]];
      src += bm->row;
      dst += drow;
   }
}

static void blit_8to16_scale(tdrv_texture_info *info, ushort *dst, int drow, ushort *pal)
{
   int i, j, k;
   int scale_w = info->scale_w;
   int scale_h = info->scale_h;
   int step_w = 1<<(scale_w-1);
   int step_h = (1<<scale_h)-1;
   grs_bitmap *bm = info->bm;
   uchar *src = info->bits;

   for (i=0; i<bm->h; i++) {
      if (scale_w>0)
      {
         ulong *base = (ulong *)dst;
         for (j=0; j<bm->w; j++)
         {
            ulong c32 = pal[src[j]];
            c32 += (c32<<16);
            for (k=0; k<step_w; k++)
               base[k] = c32;
            base += step_w;
         }
      } else {
         for (j=0; j<bm->w; j++)
            dst[j] = pal[src[j]];
      }
      for (j=0; j<step_h; j++) {
         memcpy(dst + drow, dst, 2*(bm->w<<scale_w));
         dst += drow;
      }
      src += bm->row;
      dst += drow;
   }
}

static void blit_8to16_clut_scale(tdrv_texture_info *info, ushort *dst, int drow, ushort *pal)
{
   int i, j, k;
   int scale_w = info->scale_w;
   int scale_h = info->scale_h;
   int step_w = 1<<(scale_w-1);
   int step_h = (1<<scale_h)-1;
   grs_bitmap *bm = info->bm;
   uchar *src = info->bits;

   for (i=0; i<bm->h; i++) {
      if (scale_w>0)
      {
         ulong *base = (ulong *)dst;
         for (j=0; j<bm->w; j++)
         {
            ulong c32 = pal[texture_clut[src[j]]];
            c32 += (c32<<16);
            for (k=0; k<step_w; k++)
               base[k] = c32;
            base += step_w;
         }
      } else {
         for (j=0; j<bm->w; j++)
            dst[j] = pal[texture_clut[src[j]]];
      }
      for (j=0; j<step_h; j++) {
         memcpy(dst + drow, dst, 2*(bm->w<<scale_w));
         dst += drow;
      }
      src += bm->row;
      dst += drow;
   }
}
static void blit_8to8(tdrv_texture_info *info, uchar *dst, int drow)
{
   int i;
   grs_bitmap *bm = info->bm;
   uchar *src = info->bits;

   for (i=0; i<bm->h; i++) {
      memcpy(dst, src, bm->w);
      src += bm->row;
      dst += drow;
   }
}
static void blit_8to8_clut(tdrv_texture_info *info, uchar *dst, int drow)
{
   int i, j;
   grs_bitmap *bm = info->bm;
   uchar *src = info->bits;

   for (i=0; i<bm->h; i++) {
      for (j=0; j<bm->w; j++)
         dst[j] = texture_clut[src[j]];
      src += bm->row;
      dst += drow;
   }
}

static void blit_8to8_scale(tdrv_texture_info *info, uchar *dst, int drow)
{
   int i, j, k;
   int scale_w = info->scale_w;
   int scale_h = info->scale_h;
   int step_w = 1<<scale_w;
   int step_h = (1<<scale_h) - 1;
   grs_bitmap *bm = info->bm;
   uchar *src = info->bits;

   for (i=0; i<bm->h; i++) {
      if (scale_w > 0) {
         uchar *base = dst;
         for (j=0; j<bm->w; j++) {
            uchar c = src[j];
            for (k=0; k<step_w; k++)
               base[k] = c;
            base += step_w;
         }
      } else
         memcpy(dst, src, bm->w);
      for (j = 0; j<step_h; j++) {
         memcpy(dst + drow, dst, bm->w);
         dst += drow;
      }
      src += bm->row;
      dst += drow;
   }
}

static void blit_8to8_clut_scale(tdrv_texture_info *info, uchar *dst, int drow)
{
   int i, j, k;
   int scale_w = info->scale_w;
   int scale_h = info->scale_h;
   int step_w = 1<<scale_w;
   int step_h = (1<<scale_h) - 1;
   grs_bitmap *bm = info->bm;
   uchar *src = info->bits;

   for (i=0; i<bm->h; i++) {
      uchar *base = dst;
      for (j=0; j<bm->w; j++) {
         uchar c = texture_clut[src[j]];
         for (k=0; k<step_w; k++)
            base[k] = c;
         base += step_w;
      }
      for (j = 0; j<step_h; j++) {
         memcpy(dst + drow, dst, bm->w);
         dst += drow;
      }
      src += bm->row;
      dst += drow;
   }
}

static void blit_16to16(tdrv_texture_info *info, ushort *dst, int drow)
{
   int i;
   grs_bitmap *bm = info->bm;
   uchar *src = info->bits;

   for (i=0; i<bm->h; i++) {
      memcpy(dst, src, 2*bm->w);
      src += bm->row;
      dst += drow;
   }
}

static void blit_16to16_scale(tdrv_texture_info *info, ushort *dst, int drow)
{
   int i, j, k;
   int scale_w = info->scale_w;
   int scale_h = info->scale_h;
   int step_w = 1<<scale_w;
   int step_h = (1<<scale_h) - 1;
   grs_bitmap *bm = info->bm;
   int srow = bm->row>>1;
   ushort *src = (ushort *)info->bits;

   for (i=0; i<bm->h; i++) {
      if (step_w > 1) {
         ushort *base = dst;
         for (j=0; j<bm->w; j++) {
            ushort c = src[j];
            for (k=0; k<step_w; k++)
               base[k] = c;
            base += step_w;
         }
      } else
         memcpy(dst, src, 2*bm->w);
      for (j = 0; j<step_h; j++) {
         memcpy(dst + drow, dst, 2*bm->w);
         dst += drow;
      }
      src += srow;
      dst += drow;
   }
}

static void blit_32to32(tdrv_texture_info *info, ushort *dst, int drow)
{
   int i;
   grs_bitmap *bm = info->bm;
   uchar *src = info->bits;

   for (i=0; i<bm->h; i++) {
      memcpy(dst, src, 4*bm->w);
      src += bm->row;
      dst += drow;
   }
}

static void blit_32to32_scale(tdrv_texture_info *info, ushort *dst, int drow)
{
   ulong *pDst;
   int i, j, k;
   int scale_w = info->scale_w;
   int scale_h = info->scale_h;
   int step_w = 1<<scale_w;
   int step_h = (1<<scale_h)-1;
   int DstRow = drow>>1;
   grs_bitmap *bm = info->bm;
   ulong *src;
   int srow = bm->row>>2;

   src = (ulong *)info->bits;
   pDst = (ulong *)dst;
   for (i=0; i<bm->h; i++) {
      if (step_w>1)
      {
         ulong *base = pDst;
         for (j=0; j<bm->w; j++)
         {
            ulong c = src[j];
            for (k=0; k<step_w; k++)
               base[k] = c;
            base += step_w;
         }
      } else
         memcpy(pDst, src, 4*bm->w);
      for (j=0; j<step_h; j++) {
         memcpy(pDst + DstRow, pDst, 4*bm->w);
         pDst += DstRow;
      }
      src += srow;
      pDst += DstRow;
   }
}

static void blit_32to16(tdrv_texture_info *info, ushort *dst, uint drow)
{
    grs_bitmap *bm;
    unsigned int* pS;
    unsigned int y;
    int h;
    int w;
    unsigned int x;

    bm = info->bm;
    pS = (unsigned int*)info->bits;
    w = bm->w;
    h = bm->h;
    for (y = 0; y < (unsigned int)h; y++) {
        for (x = 0; x < (unsigned int)w; x++, pS++, dst++) {
            *dst = (unsigned short)((*pS & 0xf0000000) >> 0x10) |
                   (unsigned short)((*pS & 0xf00000) >> 0xc) |
                   (unsigned short)((*pS & 0xf000) >> 8) |
                   (unsigned short)((*pS & 0xf0) >> 4);
        }
        if (drow > (unsigned int)w) {
            dst = dst + (drow - w);
        }
    }
}





static int FindClosestColor(float r, float g, float b);
static int get_num_sizes(int* dest, int count, int* source_table);
static int munge_size_tables(int** out_array);





static HRESULT WINAPI EnumTextureFormatsCallback(LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext)
{
    DDPIXELFORMAT* pf = lpDDPixFmt;
#ifndef SHIP
    if ((pf->dwFlags & DDPF_PALETTEINDEXED8) != 0) {
        if (bSpewOn)
            mprintf("8 bit palette indexed format\n");
    } else if ((pf->dwFlags & DDPF_ALPHA) != 0) {
        if (bSpewOn)
            mprintf("Alpha format: bitdepth %i\n", pf->dwRGBBitCount);
    } else if ((pf->dwFlags & DDPF_RGB) != 0) {
        if (bSpewOn)
            mprintf("RGB format: bitdepth %i\n", pf->dwRGBBitCount);
        if (bSpewOn)
            mprintf("bitmasks: A %x, R %x G %x B %x\n",
                pf->dwRGBAlphaBitMask, pf->dwRBitMask,
                pf->dwGBitMask, pf->dwBBitMask);
    } else {
        if (bSpewOn)
            mprintf("other format: %i\n", pf->dwFlags);
    }
#endif

    if ((pf->dwFlags & DDPF_RGB) != 0 && pf->dwRGBBitCount == 16) {
        if (pf->dwRGBAlphaBitMask == 0xF000 &&
            pf->dwRBitMask == 0xF00 &&
            pf->dwGBitMask == 0xF0 &&
            pf->dwBBitMask == 0xF)
        {
            memcpy(&g_AlphaTextureFormat, pf, sizeof(DDPIXELFORMAT));
        } else if (pf->dwRGBAlphaBitMask == 0x8000 && pf->dwGBitMask == 0x3E0) {
            memcpy(&g_TransRGBTextureFormat, pf, sizeof(DDPIXELFORMAT));
        } else if (pf->dwGBitMask == 0x7E0 ||
                   (pf->dwGBitMask == 0x3E0 && g_RGBTextureFormat.dwFlags == 0))
        {
            memcpy(&g_RGBTextureFormat, pf, sizeof(DDPIXELFORMAT));
        }
    } else if ((pf->dwFlags & DDPF_RGB) != 0 && pf->dwRGBBitCount == 32) {
        if (pf->dwRGBAlphaBitMask == 0xFF000000 &&
            pf->dwRBitMask == 0xFF0000 &&
            pf->dwGBitMask == 0xFF00 &&
            pf->dwBBitMask == 0xFF)
        {
            memcpy(&g_8888TexFormat, pf, sizeof(DDPIXELFORMAT));
            g_b8888supported = 1;
        }
    } else if ((pf->dwFlags & DDPF_PALETTEINDEXED8) != 0) {
        memcpy(&g_PalTextureFormat, pf, sizeof(DDPIXELFORMAT));
    }

    return DDENUMRET_OK;
}


static void CheckSurfaces(sWinDispDevCallbackInfo *info)
{
   static void (*chain)(sWinDispDevCallbackInfo *);
   int i;

   switch (info->message) {
   case kWinDispDevCallbackActive:
      for (i=0; i<LGD3D_MAX_TEXTURES; i++) {
         LPDIRECTDRAWSURFACE4 lpTS = g_saTextures[i].lpSurface;
         if ((lpTS == NULL)||
             (lpTS->IsLost() != DDERR_SURFACELOST))
            continue;

         if (lpTS->Restore() != DD_OK)
            Warning(("Could not restore lost surface %i!\n", i));
         if (g_saTextures[i].pTdrvBitmap) {
#line 3967
            AssertMsg(g_tmgr != NULL, "Hmmm.  Should have a non-NULL texture manager here.");
            g_tmgr->unload_texture(g_saTextures[i].pTdrvBitmap);
         }
      }
      break;
   case kWinDispDevCallbackUpdateChain:
      GenericCallbackChainHandler(&callback_id, (callback_chain_func **)&chain, &info->chain_info);
      return;
   }
   if (chain!=NULL)
      chain(info);
}

static void InitDefaultTexture(int size)
{
   grs_bitmap *bm;
   int c0, c1, i, j;

   bm = gr_alloc_bitmap(BMT_FLAT8, 0, size, size);
   c0 = FindClosestColor(180.0, 10.0, 10.0);
   c1 = FindClosestColor(10.0, 180.0, 10.0);
   for (i=0; i<size; i++)
      for (j=0; j<size; j++)
         bm->bits[size*i+j] = ((i+j)&1) ? c0:c1;

   default_bm = bm;
}

static int FindClosestColor(float r, float g, float b)
{
   float best=3*256*256;
   uchar *pal = grd_pal;
   int i, color = -1;

   for (i=0; i<256; i++) {
      float test, dr, dg, db;
      dr = r - pal[0];
      dg = g - pal[1];
      db = b - pal[2];
      pal += 3;
      test = dr*dr + dg*dg + db*db;
      if (test < best) {
         best = test;
         color = i;
      }
   }
#line 4051
   AssertMsg(color>=0,"Couldn't fit color.");
   return color;
}

static int init_size_tables(int** out_array)
{
    init_size_table(alpha_size_table, 2);
    init_size_table(generic_size_table, 0);
    init_size_table(rgb_size_table, 1);
    init_size_table(trgb_size_table, 3);
    init_size_table(b8888_size_table, 4);
    return munge_size_tables(out_array);
}

static void init_size_table(int* table, uchar type)
{
    int bm_type;
    int j;
    int i;
    tdrv_texture_info info;
    int h;
    int w;
    unsigned int cookie;

    info.id = 0;
    *((unsigned char*)&cookie + 2) = type;
    *((unsigned char*)&cookie + 3) = 0;
    w = 1;
    for (i = 0; i < 0x51; w += w, i += 9) {
        h = 1;
        for (j = 0; j < 9; h += h, j++) {


            if ((type == 0) || (type == 2)) {
                bm_type = 2;
            } else if (type == 4) {
                bm_type = 10;
            } else {
                bm_type = 3;
            }
            info.bm = gr_alloc_bitmap(bm_type, 0, w, h);
            info.scale_w = info.scale_h = 0;
            info.w = (int)info.bm->w;
            info.h = (int)info.bm->h;
            info.size_index = -1;
            info.bits = info.bm->bits;
            *((unsigned char*)&cookie + 0) = info.bm->wlog;
            *((unsigned char*)&cookie + 1) = info.bm->hlog;
            info.cookie = cookie;
            if (pcStates->load_texture(&info) == -1) {
                *(int*)(table + (i + j)) = -1;
            } else {
                if (b_SS2_UseSageTexManager != 0) {
                    *(int*)(table + (i + j)) = (i + j) * 5 + (int)type;
                } else {
                    *(int*)(table + (i + j)) = info.size_index;
                }
                pcStates->release_texture(info.id);
            }
            (*gr_free)(info.bm);
        }
    }
}

static int munge_size_tables(int **p_size_list)
{
    int *size_list;
    int num_sizes;

#line 3364
    size_list = (int *)malloc(5 * NUM_SIZE_ENTRIES * sizeof(int));
    num_sizes = get_num_sizes(size_list, 0, alpha_size_table);
    num_sizes = get_num_sizes(size_list, num_sizes, generic_size_table);
    num_sizes = get_num_sizes(size_list, num_sizes, rgb_size_table);
    num_sizes = get_num_sizes(size_list, num_sizes, trgb_size_table);
    num_sizes = get_num_sizes(size_list, num_sizes, b8888_size_table);
    *p_size_list = (int *)Realloc(size_list, num_sizes * sizeof(int));
    return num_sizes;
}

static int get_num_sizes(int* dest, int count, int* source_table)
{
    // Deduplicate entries from source_table into dest
    for (int si = 0; si < NUM_SIZE_ENTRIES; si++) {
        if (source_table[si] == -1) continue;
        int di;
        for (di = 0; di < count; di++) {
            if (dest[di] == source_table[si])
                break;
        }
        if (di == count) {
            dest[count] = source_table[si];
            count++;
        }
        source_table[si] = di;  // reassign to index
    }
    return count;
}
