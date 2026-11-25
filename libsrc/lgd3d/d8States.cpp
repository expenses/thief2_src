#include <d8dx6.h>
#include <lgd3d.h>
#include <d8states.h>
#include <d6prim.h>
#include <d8prim.h>
#include <tdrv.h>
#include <tmgr.h>
#include <lglog.h>
#include <dbg.h>
#include <lgassert.h>
#include <d6check.h>
#include <dynfunc.h>
#include <config.h>
#include <comtools.h>
#include <appagg.h>
#include <resapi.h>
#include <storeapi.h>
#include <reslock.h>
#include <dev2d.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

extern "C" {
extern BOOL lgd3d_punt_d3d;
extern int mprintf(const char*, ...);
extern void setwbnf(double znear, double zfar);
ushort* texture_pal_opaque[256];
ushort* texture_pal_trans[256];
ushort default_alpha_pal[256];
ushort* alpha_pal;
}
extern texture_manager* g_tmgr;
int g_nMipmapMode;
BOOL g_bLightmapMipmapLinear;
DDPIXELFORMAT g_DXTTexFormat[5];
CRITICAL_SECTION g_ResManLock;
extern "C" grs_bitmap* default_bm;

extern "C" void InitDefaultTexture(int size)
{
    grs_bitmap* bm = gr_alloc_bitmap(BMT_FLAT8, 0, size, size);
    if (!bm)
        return;
    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++)
            bm->bits[size * i + j] = ((i + j) & 1) ? 1 : 2;
    default_bm = bm;
}
extern int D3DGetAlphaToCoverageMode(IDirect3DDevice9* pDevice);


#define PUNT_D3D lgd3d_punt_d3d

#define D8CheckedSetRS(state, val) \
    { HRESULT hResult = g_lpD3D9Device->SetRenderState((D3DRENDERSTATETYPE)(state), (val)); CheckHResult(hResult, "SetRenderStateForGlobal failed"); }
#define D8CheckedSetTSS(stage, type, val) \
    { HRESULT hResult = g_lpD3D9Device->SetTextureStageState((stage), (D3DTEXTURESTAGESTATETYPE)(type), (val)); CheckHResult(hResult, "SetTextureStageState failed"); }
#define D8CheckedSetSS(stage, type, val) \
    { HRESULT hResult = g_lpD3D9Device->SetSamplerState((stage), (D3DSAMPLERSTATETYPE)(type), (val)); CheckHResult(hResult, "SetSamplerState failed"); }

#define ATOC_NVIDIA_ATOC        1
#define ATOC_NVIDIA_SSAA        2
#define ATOC_ATI_A2M            3
#define FOURCC_NV_ATOC          MAKEFOURCC('A', 'T', 'O', 'C')
#define FOURCC_NV_SSAA          MAKEFOURCC('S', 'S', 'A', 'A')
#define FOURCC_ATI_A2M_ENABLE   MAKEFOURCC('A', '2', 'M', '1')
#define FOURCC_ATI_A2M_DISABLE  MAKEFOURCC('A', '2', 'M', '0')

typedef HRESULT (WINAPI* tD3DXFilterTexture)(LPDIRECT3DBASETEXTURE9, CONST PALETTEENTRY*, UINT, DWORD);
typedef HRESULT (WINAPI* tD3DXCreateEffect)(LPDIRECT3DDEVICE9, LPCVOID, UINT, CONST D3DXMACRO*, LPD3DXINCLUDE, DWORD, LPD3DXEFFECTPOOL, LPD3DXEFFECT*, LPD3DXBUFFER*);
typedef HRESULT (WINAPI* tD3DXLoadSurfaceFromSurface)(LPDIRECT3DSURFACE9, CONST PALETTEENTRY*, CONST RECT*, LPDIRECT3DSURFACE9, CONST PALETTEENTRY*, CONST RECT*, DWORD, D3DCOLOR);

DeclDynFunc_(HRESULT, WINAPI, D3DXLoadSurfaceFromSurface, (LPDIRECT3DSURFACE9, CONST PALETTEENTRY*, CONST RECT*, LPDIRECT3DSURFACE9, CONST PALETTEENTRY*, CONST RECT*, DWORD, D3DCOLOR));
DeclDynFunc_(HRESULT, WINAPI, D3DXFilterTexture, (LPDIRECT3DBASETEXTURE9, CONST PALETTEENTRY*, UINT, DWORD));
DeclDynFunc_(HRESULT, WINAPI, D3DXCreateEffect, (LPDIRECT3DDEVICE9, LPCVOID, UINT, CONST D3DXMACRO*, LPD3DXINCLUDE, DWORD, LPD3DXEFFECTPOOL, LPD3DXEFFECT*, LPD3DXBUFFER*));

tD3DXFilterTexture g_pfnD3DXFilterTexture;
tD3DXCreateEffect g_pfnD3DXCreateEffect;
tD3DXLoadSurfaceFromSurface g_pfnD3DXLoadSurfaceFromSurface;

D3DFORMAT g_aDXTFormats[5];
IDirect3DTexture9* g_apStagingTextures[8];
int g_nNextStagingTexture;
cD8States* g_pD8States;
int g_nD8ActiveEffect;
BOOL g_bD8EffectTexture;

static const D3DBLEND s_aModulateBlend[4] = {
    D3DBLEND_ZERO, D3DBLEND_ONE, D3DBLEND_SRCCOLOR, D3DBLEND_DESTCOLOR
};
static const D3DBLEND s_aModulateBlendEx[8] = {
    D3DBLEND_ZERO, D3DBLEND_ONE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA,
    D3DBLEND_SRCCOLOR, D3DBLEND_INVSRCCOLOR, D3DBLEND_DESTCOLOR, D3DBLEND_INVDESTCOLOR
};
#define ALPHA_MODULATE_RAW      0x10000
#define ALPHA_MODULATE_EXTENDED 0x20000

static const bool s_abBlendReadsDest[24] = {
    false, false, false, false, false, false, false, true, true, true, true
};

static BOOL BlendIsModulated(int nSrc, int nDst)
{
    if (nSrc == D3DBLEND_SRCALPHA && nDst == D3DBLEND_INVSRCALPHA)
        return FALSE;
    if (nDst == D3DBLEND_ZERO && !s_abBlendReadsDest[nSrc])
        return FALSE;
    return TRUE;
}

class cD8EffectInclude : public ID3DXInclude {
public:
    STDMETHOD(Open)(D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes);
    STDMETHOD(Close)(LPCVOID pData);

    IStore* m_pStore;
};

static cD8EffectInclude g_D8EffectInclude;

HRESULT cD8EffectInclude::Open(D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
{
    *ppData = NULL;
    *pBytes = 0;
    if (m_pStore && m_pStore->Exists()) {
        EnterCriticalSection(&g_ResManLock);
        IStoreStream* pStream = m_pStore->OpenStream(pFileName, 0);
        LeaveCriticalSection(&g_ResManLock);
        if (pStream) {
            void* pData = NULL;
            BOOL bOk = TRUE;
            UINT nSize = pStream->GetSize();
            if (nSize) {
                pData = new char[nSize];
                bOk = pStream->Read(nSize, (char*)pData) == (long)nSize;
                if (!bOk) {
                    delete[] (char*)pData;
                    pData = NULL;
                }
            }
            pStream->Close();
            pStream->Release();
            if (!bOk) {
                mprintf("ERROR: D3D effect include file \"%s\" failed to load\n", pFileName);
                return E_FAIL;
            }
            if (nSize) {
                *ppData = pData;
                *pBytes = nSize;
            }
            return S_OK;
        }
    }
    mprintf("ERROR: D3D effect include file \"%s\" not found\n", pFileName);
    return E_FAIL;
}

HRESULT cD8EffectInclude::Close(LPCVOID pData)
{
    if (pData)
        delete[] (char*)pData;
    return S_OK;
}

HRESULT D8_CreateColorCorrectionEffect(ID3DXEffect** ppEffect)
{
    const char* pszName = "cc.fx";
    char szPath[256];
    if (!g_lpD3D9Device || !pcStates)
        return E_FAIL;

    void* pData = NULL;
    UINT nSize = 0;
    sprintf(szPath, "shaders\\%s", pszName);
    IStoreStream* pStream = ResOpenConfigFile(szPath, NULL, &g_D8EffectInclude.m_pStore);
    if (pStream) {
        BOOL bOk = FALSE;
        nSize = pStream->GetSize();
        if (nSize) {
            pData = new char[nSize];
            bOk = pStream->Read(nSize, (char*)pData) == (long)nSize;
            if (!bOk) {
                delete[] (char*)pData;
                pData = NULL;
            }
        }
        pStream->Close();
        pStream->Release();
        if (!bOk || !nSize) {
            SafeRelease(g_D8EffectInclude.m_pStore);
            g_D8EffectInclude.m_pStore = NULL;
        }
    }

    ID3DXBuffer* pErrors = NULL;
    ID3DXInclude* pInclude;
    const void* pSource;
    if (pData) {
        pInclude = &g_D8EffectInclude;
        pSource = pData;
    } else {
        pSource = g_fxColorCorrection;
        nSize = sizeof(g_fxColorCorrection);
        pInclude = NULL;
    }
    HRESULT hr = g_pfnD3DXCreateEffect(g_lpD3D9Device, pSource, nSize, NULL, pInclude, 0, NULL, ppEffect, &pErrors);
    SafeRelease(g_D8EffectInclude.m_pStore);
    g_D8EffectInclude.m_pStore = NULL;
    if (pData)
        delete[] (char*)pData;
    if (FAILED(hr) && pErrors) {
        const char* pszErrors = (const char*)pErrors->GetBufferPointer();
        if (pszErrors) {
            mprintf("ERROR: D3D effect \"%s\" failed to compile: %s\n", pszName, pszErrors);
            LogMsg2("ERROR: D3D effect \"%s\" failed to compile: %s", pszName, pszErrors);
        }
        SafeRelease(pErrors);
    }
    return hr;
}

static void D8SetTexBlend(DWORD dwStage, sTexBlendArgs* pCur, sTexBlendArgs* pSet)
{
    pCur += dwStage;
    pSet += dwStage;
    if (pCur->eColorOperation != pSet->eColorOperation) {
        pSet->eColorOperation = pCur->eColorOperation;
#line 385
        D8CheckedSetTSS(dwStage, D3DTSS_COLOROP, pCur->eColorOperation);
    }
    if (pCur->dwColorArg1 != pSet->dwColorArg1) {
        pSet->dwColorArg1 = pCur->dwColorArg1;
#line 386
        D8CheckedSetTSS(dwStage, D3DTSS_COLORARG1, pCur->dwColorArg1);
    }
    if (pCur->dwColorArg2 != pSet->dwColorArg2) {
        pSet->dwColorArg2 = pCur->dwColorArg2;
#line 387
        D8CheckedSetTSS(dwStage, D3DTSS_COLORARG2, pCur->dwColorArg2);
    }
    if (pCur->eAlphaOperation != pSet->eAlphaOperation) {
        pSet->eAlphaOperation = pCur->eAlphaOperation;
#line 389
        D8CheckedSetTSS(dwStage, D3DTSS_ALPHAOP, pCur->eAlphaOperation);
    }
    if (pCur->dwAlphaArg1 != pSet->dwAlphaArg1) {
        pSet->dwAlphaArg1 = pCur->dwAlphaArg1;
#line 390
        D8CheckedSetTSS(dwStage, D3DTSS_ALPHAARG1, pCur->dwAlphaArg1);
    }
    if (pCur->dwAlphaArg2 != pSet->dwAlphaArg2) {
        pSet->dwAlphaArg2 = pCur->dwAlphaArg2;
#line 391
        D8CheckedSetTSS(dwStage, D3DTSS_ALPHAARG2, pCur->dwAlphaArg2);
    }
}

#define D8CheckedSetTexBlend(stage, b) \
    D8CheckedSetTSS(stage, D3DTSS_COLOROP, (b)->eColorOperation); \
    D8CheckedSetTSS(stage, D3DTSS_COLORARG1, (b)->dwColorArg1); \
    D8CheckedSetTSS(stage, D3DTSS_COLORARG2, (b)->dwColorArg2); \
    D8CheckedSetTSS(stage, D3DTSS_ALPHAOP, (b)->eAlphaOperation); \
    D8CheckedSetTSS(stage, D3DTSS_ALPHAARG1, (b)->dwAlphaArg1); \
    D8CheckedSetTSS(stage, D3DTSS_ALPHAARG2, (b)->dwAlphaArg2)

static float FogCoef(float fDist)
{
    return (float)((z_far - (z_near * z_far) / fDist) / (z_far - z_near));
}

void cD8States::EnableFog(BOOL enable)
{
    if ((g_bUseTableFog || g_bUseVertexFog) && m_psSetRS->bFogOn != enable) {
        pcRenderBuffer->FlushPrimitives();
        m_psCurrentRS->bFogOn = enable;
        m_psSetRS->bFogOn = enable;
#line 420
        D8CheckedSetRS(D3DRS_FOGENABLE, m_psCurrentRS->bFogOn);
    }
}

void cD8States::ResetDefaultAlphaModulate()
{
    pcRenderBuffer->FlushPrimitives();
    m_psCurrentRS->eSrcAlpha = (D3DBLEND)D3DBLEND_SRCALPHA;
    m_psSetRS->eSrcAlpha = (D3DBLEND)D3DBLEND_SRCALPHA;
    m_psCurrentRS->eDstAlpha = (D3DBLEND)D3DBLEND_INVSRCALPHA;
    m_psSetRS->eDstAlpha = (D3DBLEND)D3DBLEND_INVSRCALPHA;
    if (!PUNT_D3D) {
#line 435
        D8CheckedSetRS(D3DRS_SRCBLEND, m_psCurrentRS->eSrcAlpha);
#line 436
        D8CheckedSetRS(D3DRS_DESTBLEND, m_psCurrentRS->eDstAlpha);
    }
    m_bAlphaModulated = FALSE;
}

void cD8States::SetAlphaColor(float alpha)
{
    int val = (int)(alpha * 255);
    if ((unsigned)val > 0xFF)
        val = val < 0 ? 0 : 0xFF;
    if (m_psCurrentRS->nAlphaColor != val) {
        pcRenderBuffer->FlushPrimitives();
        m_psCurrentRS->nAlphaColor = val;
        m_psSetRS->nAlphaColor = val;
        pcRenderBuffer->PassAlphaColor(val);
        if (m_psSetRS->bAlphaTestOn) {
            if (m_psSetRS->bAlphaOn)
                SetAlphaRef(m_psSetRS->nAlphaColor * g_nAlphaTestRef / 255);
            else
                SetAlphaRef(g_nAlphaTestRef);
        }
    }
}

void cD8States::SetAlphaTestRef(int nRef)
{
    if (nRef < 0)
        nRef = 0;
    else if (nRef > 255)
        nRef = 255;
    if (g_nAlphaTestRef != nRef) {
        if (!g_nAlphaTestMode) {
            g_nAlphaTestRef = nRef;
            return;
        }
        pcRenderBuffer->FlushPrimitives();
        BOOL bToggled = (nRef == 0 || g_nAlphaTestRef == 0);
        g_nAlphaTestRef = nRef;
        if (m_psSetRS->bAlphaTestOn) {
            if (m_psSetRS->bAlphaOn)
                SetAlphaRef(m_psSetRS->nAlphaColor * g_nAlphaTestRef / 255);
            else
                SetAlphaRef(g_nAlphaTestRef);
            if (bToggled && m_nAlphaToCoverage)
                SetAlphaTestEnable(TRUE);
        }
    }
}

void cD8States::EnableAlphaBlending(BOOL enable)
{
    if (enable != m_psCurrentRS->bAlphaOn || enable != m_psSetRS->bAlphaOn) {
        pcRenderBuffer->FlushPrimitives();
        m_psCurrentRS->bAlphaOn = enable;
        m_psSetRS->bAlphaOn = enable;
        if (!PUNT_D3D) {
#line 506
            D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
            if (m_psSetRS->bAlphaTestOn) {
                if (m_psSetRS->bAlphaOn)
                    SetAlphaRef(m_psSetRS->nAlphaColor * g_nAlphaTestRef / 255);
                else
                    SetAlphaRef(g_nAlphaTestRef);
            }
        }
    }
}

void cD8States::ResetAlphaTest()
{
    if (m_psSetRS->bAlphaTestOn) {
        pcRenderBuffer->FlushPrimitives();
        m_psSetRS->bAlphaTestOn = FALSE;
        SetAlphaTestEnable(m_psSetRS->bAlphaTestOn);
    }
}

void cD8States::SetAlphaModulateHack(uint mode)
{
    pcRenderBuffer->FlushPrimitives();
    if (mode & ALPHA_MODULATE_RAW) {
        m_psCurrentRS->eSrcAlpha = (D3DBLEND)(mode & 0xff);
        m_psSetRS->eSrcAlpha = m_psCurrentRS->eSrcAlpha;
        m_psCurrentRS->eDstAlpha = (D3DBLEND)(((int)mode >> 8) & 0xff);
    } else if (mode & ALPHA_MODULATE_EXTENDED) {
        m_psCurrentRS->eSrcAlpha = (D3DBLEND)s_aModulateBlendEx[mode & 7];
        m_psSetRS->eSrcAlpha = m_psCurrentRS->eSrcAlpha;
        m_psCurrentRS->eDstAlpha = (D3DBLEND)s_aModulateBlendEx[((int)mode >> 3) & 7];
    } else {
        m_psCurrentRS->eSrcAlpha = (D3DBLEND)s_aModulateBlend[mode & 3];
        m_psSetRS->eSrcAlpha = m_psCurrentRS->eSrcAlpha;
        m_psCurrentRS->eDstAlpha = (D3DBLEND)s_aModulateBlend[((int)mode >> 2) & 3];
    }
    m_psSetRS->eDstAlpha = m_psCurrentRS->eDstAlpha;
    if (!PUNT_D3D) {
#line 579
        D8CheckedSetRS(D3DRS_SRCBLEND, m_psCurrentRS->eSrcAlpha);
#line 580
        D8CheckedSetRS(D3DRS_DESTBLEND, m_psCurrentRS->eDstAlpha);
    }
    m_bAlphaModulated = BlendIsModulated(m_psCurrentRS->eSrcAlpha, m_psCurrentRS->eDstAlpha);
}

void cD8States::SetFogStartAndEnd(float fStart, float fEnd)
{
    if (g_bUseTableFog || g_bUseVertexFog) {
        float start;
        float end;
        if (g_bWFog) {
            start = fStart;
            end = fEnd;
        } else {
            start = FogCoef(fStart);
            end = FogCoef(fEnd);
        }
        m_psSetRS->fFogStart = m_psCurrentRS->fFogStart = start;
#line 600
        D8CheckedSetRS(D3DRS_FOGSTART, *(DWORD*)&m_psCurrentRS->fFogStart);
        m_psSetRS->fFogEnd = m_psCurrentRS->fFogEnd = end;
#line 605
        D8CheckedSetRS(D3DRS_FOGEND, *(DWORD*)&m_psCurrentRS->fFogEnd);
    }
}

void cD8States::SetFogColor(int r, int g, int b)
{
    if (g_bUseTableFog || g_bUseVertexFog) {
        pcRenderBuffer->FlushPrimitives();
        if (r > 255)
            r = 255;
        else if (r < 0)
            r = 0;
        if (g > 255)
            g = 255;
        else if (g < 0)
            g = 0;
        if (b > 255)
            b = 255;
        else if (b < 0)
            b = 0;
        m_psCurrentRS->dwFogColor = (r << 16) | (g << 8) | b;
        m_psSetRS->dwFogColor = m_psCurrentRS->dwFogColor;
        if (!PUNT_D3D) {
#line 630
            D8CheckedSetRS(D3DRS_FOGCOLOR, m_psCurrentRS->dwFogColor);
        }
    }
}

BOOL cD8States::SetTexWrapping(DWORD dwLevel, BOOL bSetSmooth)
{
    int old = (m_psCurrentRS->eWrap[dwLevel] == D3DTADDRESS_WRAP);
    if (old != bSetSmooth) {
        pcRenderBuffer->FlushPrimitives();
        m_psCurrentRS->eWrap[dwLevel] = bSetSmooth ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
        m_psSetRS->eWrap[dwLevel] = m_psCurrentRS->eWrap[dwLevel];
        if (!PUNT_D3D) {
#line 649
            D8CheckedSetSS(dwLevel, D3DSAMP_ADDRESSU, m_psCurrentRS->eWrap[dwLevel]);
#line 650
            D8CheckedSetSS(dwLevel, D3DSAMP_ADDRESSV, m_psCurrentRS->eWrap[dwLevel]);
        }
    }
    return old;
}

void cD8States::SetTrilinearFiltering(BOOL bOn)
{
    g_bTrilinearFiltering = bOn;
    if (bOn) {
#line 662
        D8CheckedSetSS(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
#line 663
        D8CheckedSetSS(1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
    } else {
#line 667
        D8CheckedSetSS(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
#line 668
        D8CheckedSetSS(1, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
    }
}

void cD8States::SetMipmapBias(float fBias)
{
    if (fBias < -16.0f)
        fBias = -16.0f;
    else if (fBias > 16.0f)
        fBias = 16.0f;
#line 679
    D8CheckedSetSS(0, D3DSAMP_MIPMAPLODBIAS, *(DWORD*)&fBias);
#line 680
    D8CheckedSetSS(1, D3DSAMP_MIPMAPLODBIAS, *(DWORD*)&fBias);
}

void cD8States::SetTextureMipmapBias(float fBias)
{
    if (fBias < -16.0f)
        fBias = -16.0f;
    else if (fBias > 16.0f)
        fBias = 16.0f;
#line 690
    D8CheckedSetSS(0, D3DSAMP_MIPMAPLODBIAS, *(DWORD*)&fBias);
}

void cD8States::SetTexturePalette(uint start, uint n, uchar* pal, int slot)
{
    PALETTEENTRY peColorTable[256];
    int i;
    for (i = 0; i < (int)n; i++) {
        peColorTable[i].peRed = pal[3 * i];
        peColorTable[i].peGreen = pal[3 * i + 1];
        peColorTable[i].peFlags = 0xff;
        peColorTable[i].peBlue = pal[3 * i + 2];
    }
    for (; i < 256; i++) {
        peColorTable[i].peFlags = 0xff;
        peColorTable[i].peRed = 0;
        peColorTable[i].peGreen = 0;
        peColorTable[i].peBlue = 0;
    }
    HRESULT hRes = g_lpD3D9Device->SetPaletteEntries(slot, peColorTable);
#line 714
    CheckHResult(hRes, "SetPaletteEntries failed");
}

int cD8States::UseLinearTableFog(BOOL bUseIt)
{
    if (!g_bUseTableFog)
        return 0;
    int old = (m_psCurrentRS->eFogMode == D3DFOG_LINEAR);
    if (old != bUseIt) {
        pcRenderBuffer->FlushPrimitives();
        m_psCurrentRS->eFogMode = bUseIt ? (D3DFOGMODE)D3DFOG_LINEAR : (D3DFOGMODE)D3DFOG_EXP;
        m_psSetRS->eFogMode = m_psCurrentRS->eFogMode;
#line 728
        D8CheckedSetRS(D3DRS_FOGTABLEMODE, m_psCurrentRS->eFogMode);
    }
    return old;
}

void cD8States::ApplyTextureFilter()
{
    m_psCurrentRS->eMagTexFilter = (D3DTEXTUREMINFILTER)g_dwTexMagFilter;
    m_psSetRS->eMagTexFilter = (D3DTEXTUREMINFILTER)g_dwTexMagFilter;
    m_psCurrentRS->eMinTexFilter = (D3DTEXTUREMINFILTER)g_dwTexMinFilter;
    m_psSetRS->eMinTexFilter = (D3DTEXTUREMINFILTER)g_dwTexMinFilter;
#line 738
    D8CheckedSetSS(0, D3DSAMP_MAGFILTER, m_psCurrentRS->eMagTexFilter);
#line 739
    D8CheckedSetSS(0, D3DSAMP_MINFILTER, m_psCurrentRS->eMinTexFilter);
}

void cD8States::ApplyLightmapFilter()
{
    m_psCurrentRS->eLightMapMagFilter = (D3DTEXTUREMINFILTER)g_dwLightMapMagFilter;
    m_psSetRS->eLightMapMagFilter = (D3DTEXTUREMINFILTER)g_dwLightMapMagFilter;
    m_psCurrentRS->eLightMapMinFilter = (D3DTEXTUREMINFILTER)g_dwLightMapMinFilter;
    m_psSetRS->eLightMapMinFilter = (D3DTEXTUREMINFILTER)g_dwLightMapMinFilter;
#line 747
    D8CheckedSetSS(1, D3DSAMP_MAGFILTER, m_psCurrentRS->eLightMapMagFilter);
#line 748
    D8CheckedSetSS(1, D3DSAMP_MINFILTER, m_psCurrentRS->eLightMapMinFilter);
}

void cD8States::SetTextureFilterMode(int nMode)
{
    g_nTexFilterMode = nMode;
    if (nMode == 0) {
        g_dwTexMagFilter = D3DTEXF_POINT;
        g_dwTexMinFilter = D3DTEXF_POINT;
    } else if (nMode == 1) {
        g_dwTexMagFilter = D3DTEXF_LINEAR;
        g_dwTexMinFilter = D3DTEXF_LINEAR;
    } else {
        g_dwTexMagFilter = D3DTEXF_ANISOTROPIC;
        g_dwTexMinFilter = D3DTEXF_ANISOTROPIC;
        if ((DWORD)nMode > g_sD3D9Caps.MaxAnisotropy)
            nMode = g_sD3D9Caps.MaxAnisotropy;
#line 771
        D8CheckedSetSS(0, D3DSAMP_MAXANISOTROPY, nMode);
    }
    ApplyTextureFilter();
}

void cD8States::SetLightmapFilterMode(int nMode)
{
    g_nLightMapFilterMode = nMode;
    if (nMode == 0) {
        g_dwLightMapMagFilter = D3DTEXF_POINT;
        g_dwLightMapMinFilter = D3DTEXF_POINT;
    } else if (nMode == 1) {
        g_dwLightMapMagFilter = D3DTEXF_LINEAR;
        g_dwLightMapMinFilter = D3DTEXF_LINEAR;
    } else {
        g_dwLightMapMagFilter = D3DTEXF_ANISOTROPIC;
        g_dwLightMapMinFilter = D3DTEXF_ANISOTROPIC;
        if ((DWORD)nMode > g_sD3D9Caps.MaxAnisotropy)
            nMode = g_sD3D9Caps.MaxAnisotropy;
#line 797
        D8CheckedSetSS(1, D3DSAMP_MAXANISOTROPY, nMode);
    }
    ApplyLightmapFilter();
}

int D8_DeviceReset(BOOL bRestore)
{
    if (g_pD8States) {
        if (!bRestore) {
            if (!g_bD8ManagedTextures) {
                for (int i = 0; i < LGD3D_MAX_TEXTURES; i++) {
                    if (g_saTextures[i].lpTexture) {
                        ((IUnknown*)g_saTextures[i].lpTexture)->Release();
                        g_saTextures[i].lpTexture = NULL;
                        if (g_saTextures[i].pTdrvBitmap)
                            g_tmgr->unload_texture(g_saTextures[i].pTdrvBitmap);
                    }
                }
            }
            g_pD8States->OnLostDevice();
        } else
            g_pD8States->OnResetDevice();
    }
    return TRUE;
}

void cD8States::OnLostDevice()
{
    ((cD8Frame*)pcFrame)->OnLostDevice();
}

void cD8States::OnResetDevice()
{
    m_pCurTexture[0] = NULL;
    m_pCurTexture[1] = NULL;
    ((cD8Frame*)pcFrame)->OnResetDevice();
    g_dwD8CurrentFVF = 0;
    setwbnf(z_near, z_far);
#line 872
    D8CheckedSetRS(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
#line 873
    D8CheckedSetRS(D3DRS_SPECULARENABLE, m_bSpecular);
#line 874
    D8CheckedSetRS(D3DRS_STENCILENABLE, FALSE);
    for (int i = 0; i < 2; i++) {
#line 876
        D8CheckedSetSS(i, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
    }
    if (g_fMipmapBias != 0.0f)
        SetMipmapBias(g_fMipmapBias);
    SetTextureFilterMode(g_nTexFilterMode);
    SetLightmapFilterMode(g_nLightMapFilterMode);
    SetTrilinearFiltering(g_bTrilinearFiltering);
#line 884
    D8CheckedSetRS(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
    SetRenderStates(m_psSetRS);
    if (!lgd3d_g_bInitialized)
        lgd3d_g_bInitialized = TRUE;
}

void cD8States::SetRenderStates(sRenderStates* pStates)
{
    if (!pStates)
        return;
    m_bAlphaModulated = BlendIsModulated(pStates->eSrcAlpha, pStates->eDstAlpha);
    if (!g_nAlphaTestMode) {
        m_nCurAlphaRef = g_nAlphaTestRef;
#line 937
        D8CheckedSetRS(D3DRS_ALPHAREF, g_nAlphaTestRef);
        SetAlphaTestEnable(FALSE);
    } else {
        BOOL bAlphaTest = pStates->bAlphaTestOn;
        SetAlphaTestEnable(bAlphaTest);
        if (!bAlphaTest) {
            m_nCurAlphaRef = g_nAlphaTestRef;
#line 923
            D8CheckedSetRS(D3DRS_ALPHAREF, g_nAlphaTestRef);
        } else {
            m_nCurAlphaRef = pStates->nAlphaColor * g_nAlphaTestRef / 255;
#line 919
            D8CheckedSetRS(D3DRS_ALPHAREF, m_nCurAlphaRef);
        }
    }
#line 941
    D8CheckedSetRS(D3DRS_SHADEMODE, pStates->eShadeMode);
#line 942
    D8CheckedSetRS(D3DRS_DITHERENABLE, pStates->bDitheringOn);
#line 944
    D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, pStates->bAlphaOn);
#line 945
    D8CheckedSetRS(D3DRS_SRCBLEND, pStates->eSrcAlpha);
#line 946
    D8CheckedSetRS(D3DRS_DESTBLEND, pStates->eDstAlpha);
#line 947
    D8CheckedSetRS(D3DRS_ZENABLE, pStates->eZenable);
#line 948
    D8CheckedSetRS(D3DRS_ZWRITEENABLE, pStates->bZWriteEnable);
#line 949
    D8CheckedSetRS(D3DRS_ZFUNC, pStates->eZCompareFunc);
#line 950
    D8CheckedSetRS(D3DRS_FOGENABLE, pStates->bFogOn);
#line 951
    D8CheckedSetRS(D3DRS_FOGCOLOR, pStates->dwFogColor);
#line 952
    D8CheckedSetRS(D3DRS_FOGTABLEMODE, pStates->eFogMode);
#line 953
    D8CheckedSetRS(D3DRS_FOGDENSITY, *(DWORD*)&pStates->fFogTableDensity);
#line 954
    D8CheckedSetRS(D3DRS_FOGSTART, *(DWORD*)&pStates->fFogStart);
#line 955
    D8CheckedSetRS(D3DRS_FOGEND, *(DWORD*)&pStates->fFogEnd);
    for (int i = 0; i < 2; i++) {
#line 959
        D8CheckedSetTSS(i, D3DTSS_TEXCOORDINDEX, i);
#line 961
        D8CheckedSetTSS(i, D3DTSS_COLOROP, pStates->saTexBlend[i].eColorOperation);
#line 962
        D8CheckedSetTSS(i, D3DTSS_COLORARG1, pStates->saTexBlend[i].dwColorArg1);
#line 963
        D8CheckedSetTSS(i, D3DTSS_COLORARG2, pStates->saTexBlend[i].dwColorArg2);
#line 965
        D8CheckedSetTSS(i, D3DTSS_ALPHAOP, pStates->saTexBlend[i].eAlphaOperation);
#line 966
        D8CheckedSetTSS(i, D3DTSS_ALPHAARG1, pStates->saTexBlend[i].dwAlphaArg1);
#line 967
        D8CheckedSetTSS(i, D3DTSS_ALPHAARG2, pStates->saTexBlend[i].dwAlphaArg2);
#line 969
        D8CheckedSetSS(i, D3DSAMP_ADDRESSU, pStates->eWrap[i]);
#line 970
        D8CheckedSetSS(i, D3DSAMP_ADDRESSV, pStates->eWrap[i]);
    }
#line 972
    D8CheckedSetSS(0, D3DSAMP_MAGFILTER, pStates->eMagTexFilter);
#line 973
    D8CheckedSetSS(0, D3DSAMP_MINFILTER, pStates->eMinTexFilter);
#line 974
    D8CheckedSetSS(1, D3DSAMP_MAGFILTER, pStates->eLightMapMagFilter);
#line 975
    D8CheckedSetSS(1, D3DSAMP_MINFILTER, pStates->eLightMapMinFilter);
#line 977
    D8CheckedSetRS(D3DRS_CULLMODE, pStates->dwCullMode);
    if (pStates != m_psSetRS)
        *m_psSetRS = *pStates;
}

int cD8States::Initialize(DWORD dwRequestedFlags)
{
    g_pfnD3DXLoadSurfaceFromSurface = DynFunc(D3DXLoadSurfaceFromSurface).GetProcAddress();
    if (!g_pfnD3DXLoadSurfaceFromSurface)
        return FALSE;
    g_pfnD3DXFilterTexture = DynFunc(D3DXFilterTexture).GetProcAddress();
    if (!g_pfnD3DXFilterTexture)
        return FALSE;
    g_pfnD3DXCreateEffect = DynFunc(D3DXCreateEffect).GetProcAddress();
    if (!g_pfnD3DXCreateEffect)
        return FALSE;

    g_bLightmapMipmapLinear = 0;
    config_get_int("lm_mipmap_mode", &g_bLightmapMipmapLinear);
    g_bLightmapMipmapLinear = (g_bLightmapMipmapLinear == 1);

    m_texture_caps = g_sD3D9Caps.TextureCaps;
    m_bWBuffer = (dwRequestedFlags & LGD3DF_WBUFFER) ? 1 : 0;
    EnumerateTextureFormats();

    for (int i = 0; i < 0x100; i++) {
        texture_pal_opaque[i] = 0;
        texture_pal_trans[i] = 0;
    }
    SetPalSlotFlags(0, 0x100, grd_pal, 0, 3);
    alpha_pal = default_alpha_pal;
    pcStates->SetChromaKey(m_psCurrentRS->chroma_r, m_psCurrentRS->chroma_g, m_psCurrentRS->chroma_b);

    memset(g_saTextures, 0, sizeof(g_saTextures));
    m_bTextureListInitialized = 1;
    g_pD8States = this;
    m_dwRequestedFlags = dwRequestedFlags;

    InitDefaultTexture(0x10);
    InitTextureManager();

    return SetDefaultsStates(dwRequestedFlags);
}

void cD8States::release_texture(int n)
{
    sTextureData* pEntry = &g_saTextures[n];
    if (n == m_psCurrentRS->naTextureId[0])
        pcRenderBuffer->FlushPrimitives();
    for (int i = 0; i < 2; i++) {
        if (m_pCurTexture[i] == (IDirect3DBaseTexture9*)pEntry->lpTexture && m_pCurTexture[i]) {
            m_pCurTexture[i] = NULL;
            HRESULT hResult = g_lpD3D9Device->SetTexture(i, NULL);
#line 1256
            CheckHResult(hResult, "SetTexture failed");
        }
    }
    if (pEntry->lpTexture)
        ((IUnknown*)pEntry->lpTexture)->Release();
    pEntry->pTdrvBitmap = NULL;
    pEntry->lpTexture = NULL;
}

void cD8States::SetCullMode(DWORD dwCull)
{
    if (m_psCurrentRS->dwCullMode != dwCull) {
        pcRenderBuffer->FlushPrimitives();
        m_psCurrentRS->dwCullMode = dwCull;
        m_psSetRS->dwCullMode = dwCull;
#line 1105
        D8CheckedSetRS(D3DRS_CULLMODE, dwCull);
    }
}

int cD8States::load_texture(tdrv_texture_info* info)
{
    return reload_texture(info);
}

void cD8States::UpdateAlphaTest(BOOL bTexAlpha)
{
    m_psCurrentRS->bAlphaTestOn = bTexAlpha;
    if (m_psSetRS->bAlphaTestOn != m_psCurrentRS->bAlphaTestOn) {
        SetAlphaTestEnable(bTexAlpha);
        m_psSetRS->bAlphaTestOn = m_psCurrentRS->bAlphaTestOn;
        if (bTexAlpha) {
            if (m_psSetRS->bAlphaOn)
                SetAlphaRef(m_psSetRS->nAlphaColor * g_nAlphaTestRef / 255);
            else
                SetAlphaRef(g_nAlphaTestRef);
        }
    }
    if (!m_psSetRS->bAlphaTestOn || g_nAlphaTestMode == 2 || m_psCurrentRS->nAlphaColor < 0xff) {
        if (!m_psSetRS->bAlphaOn) {
#line 1163
            D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, TRUE);
            m_psSetRS->bAlphaOn = TRUE;
            if (bTexAlpha)
                SetAlphaRef(m_psSetRS->nAlphaColor * g_nAlphaTestRef / 255);
        }
    } else if (m_psSetRS->bAlphaOn && g_nAlphaTestRef && !m_bAlphaModulated) {
#line 1172
        D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, FALSE);
        m_psSetRS->bAlphaOn = FALSE;
        if (bTexAlpha)
            SetAlphaRef(g_nAlphaTestRef);
    }
}

void cD8States::SetAlphaTestEnable(BOOL bOn)
{
#line 1180
    D8CheckedSetRS(D3DRS_ALPHATESTENABLE, bOn);
    if (m_nAlphaToCoverage) {
        if (bOn && !g_nAlphaTestRef)
            bOn = FALSE;
        SetAlphaToCoverage(bOn);
    }
}

void cD8States::SetAlphaToCoverage(BOOL bOn)
{
    if (!m_nAlphaToCoverage)
        return;
    if (bOn) {
        if (m_nAlphaToCoverage == ATOC_NVIDIA_ATOC) {
#line 1208
            D8CheckedSetRS(D3DRS_ADAPTIVETESS_Y, FOURCC_NV_ATOC);
        } else if (m_nAlphaToCoverage == ATOC_NVIDIA_SSAA) {
#line 1212
            D8CheckedSetRS(D3DRS_ADAPTIVETESS_Y, FOURCC_NV_SSAA);
        } else if (m_nAlphaToCoverage == ATOC_ATI_A2M) {
#line 1216
            D8CheckedSetRS(D3DRS_POINTSIZE, FOURCC_ATI_A2M_ENABLE);
        }
    } else if (m_nAlphaToCoverage < ATOC_ATI_A2M) {
#line 1228
        D8CheckedSetRS(D3DRS_ADAPTIVETESS_Y, D3DFMT_UNKNOWN);
    } else if (m_nAlphaToCoverage == ATOC_ATI_A2M) {
#line 1232
        D8CheckedSetRS(D3DRS_POINTSIZE, FOURCC_ATI_A2M_DISABLE);
    }
}

void cD8States::SetAlphaRef(int nRef)
{
    if (m_nCurAlphaRef != nRef) {
        m_nCurAlphaRef = nRef;
#line 1247
        D8CheckedSetRS(D3DRS_ALPHAREF, nRef);
    }
}

void cD8States::SetTexture(DWORD dwStage, IDirect3DBaseTexture9* pTexture)
{
    if (m_pCurTexture[dwStage] != pTexture) {
        m_pCurTexture[dwStage] = pTexture;
        HRESULT hResult = g_lpD3D9Device->SetTexture(dwStage, pTexture);
#line 1256
        CheckHResult(hResult, "SetTexture failed");
    }
}

void cD8States::set_texture_id(int n)
{
    if (g_nD8ActiveEffect || PUNT_D3D)
        return;
    m_psCurrentRS->naTextureId[0] = n;
    if (m_psSetRS->naTextureId[0] == m_psCurrentRS->naTextureId[0]) {
        if (m_psCurrentRS->baTextureWithAlpha[0]) {
            if (g_nAlphaTestMode) {
                UpdateAlphaTest(TRUE);
                return;
            }
            if (!m_psSetRS->bAlphaOn) {
#line 1280
                D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, TRUE);
                m_psSetRS->bAlphaOn = TRUE;
            }
            return;
        }
        if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn) {
#line 1288
            D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
            m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
        }
        if (g_nAlphaTestMode && m_psSetRS->bAlphaTestOn) {
            SetAlphaTestEnable(FALSE);
            m_psSetRS->bAlphaTestOn = FALSE;
        }
        return;
    }

    pcRenderBuffer->FlushPrimitives();
    if (m_psCurrentRS->naTextureId[0] == -2) {
        g_tmgr->set_texture_callback();
        return;
    }
    if (m_psCurrentRS->naTextureId[0] == -1) {
        SetTexture(0, NULL);
        m_psSetRS->naTextureId[0] = -1;
        if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn) {
#line 1315
            D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
            m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
        }
        if (g_nAlphaTestMode && m_psSetRS->bAlphaTestOn) {
            SetAlphaTestEnable(FALSE);
            m_psSetRS->bAlphaTestOn = FALSE;
        }
    } else {
        sTextureData* psTD = &g_saTextures[m_psCurrentRS->naTextureId[0]];
        uchar pixel_flags = psTD->TdrvCookie.flags;
        m_psCurrentRS->baChromaKeying[0] = (pixel_flags & PF_TRANS) != 0;
        if (!m_psCurrentRS->baChromaKeying[0]) {
            m_psCurrentRS->eMagTexFilter = (D3DTEXTUREMINFILTER)g_dwTexMagFilter;
            m_psCurrentRS->eMinTexFilter = (D3DTEXTUREMINFILTER)g_dwTexMinFilter;
        } else {
            m_psCurrentRS->eMagTexFilter = (D3DTEXTUREMINFILTER)D3DTEXF_POINT;
            m_psCurrentRS->eMinTexFilter = (D3DTEXTUREMINFILTER)D3DTEXF_POINT;
        }
        if (m_psSetRS->baChromaKeying[0] != m_psCurrentRS->baChromaKeying[0]) {
            m_psSetRS->baChromaKeying[0] = m_psCurrentRS->baChromaKeying[0];
#line 1347
            D8CheckedSetSS(0, D3DSAMP_MAGFILTER, m_psCurrentRS->eMagTexFilter);
            m_psSetRS->eMagTexFilter = m_psCurrentRS->eMagTexFilter;
#line 1350
            D8CheckedSetSS(0, D3DSAMP_MAGFILTER, m_psCurrentRS->eMinTexFilter);
            m_psSetRS->eMinTexFilter = m_psCurrentRS->eMinTexFilter;
        }
        pixel_flags &= PF_FORMAT_MASK;
        m_psCurrentRS->baTextureWithAlpha[0] = PF_ALPHA_FORMAT(pixel_flags);
        if (!m_psCurrentRS->baTextureWithAlpha[0]) {
            if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn) {
#line 1371
                D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
                m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
            }
            if (g_nAlphaTestMode && m_psSetRS->bAlphaTestOn) {
                SetAlphaTestEnable(FALSE);
                m_psSetRS->bAlphaTestOn = FALSE;
            }
        } else if (!g_nAlphaTestMode) {
            if (!m_psSetRS->bAlphaOn) {
#line 1363
                D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, TRUE);
                m_psSetRS->bAlphaOn = TRUE;
            }
        } else
            UpdateAlphaTest(TRUE);

        if (m_psSetRS->dwTexBlendMode[0] != m_psCurrentRS->dwTexBlendMode[0] ||
            m_psSetRS->baTextureWithAlpha[0] != m_psCurrentRS->baTextureWithAlpha[0]) {
            m_psCurrentRS->saTexBlend[0] = sTexBlendArgsProtos[m_psCurrentRS->dwTexBlendMode[0]][0];
            if (m_psCurrentRS->dwTexBlendMode[0] == 0 && m_psCurrentRS->baTextureWithAlpha[0])
                m_psCurrentRS->saTexBlend[0].eAlphaOperation = (D3DTEXTUREOP)D3DTOP_SELECTARG1;
            D8SetTexBlend(0, m_psCurrentRS->saTexBlend, m_psSetRS->saTexBlend);
            m_psSetRS->dwTexBlendMode[0] = m_psCurrentRS->dwTexBlendMode[0];
            m_psSetRS->baTextureWithAlpha[0] = m_psCurrentRS->baTextureWithAlpha[0];
        }
        SetTextureNow();
    }
    m_psSetRS->naTextureId[0] = m_psCurrentRS->naTextureId[0];
}

static D3DFORMAT FormatFromPixelFormat(DDPIXELFORMAT* pf)
{
    switch (pf->dwFourCC) {
        case D3DFMT_DXT1:
        case D3DFMT_DXT2:
        case D3DFMT_DXT3:
        case D3DFMT_DXT4:
        case D3DFMT_DXT5:
            return (D3DFORMAT)pf->dwFourCC;
    }
    switch (pf->dwRGBBitCount) {
        case 8:
            return D3DFMT_P8;
        case 16:
            if (pf->dwRGBAlphaBitMask)
                return pf->dwGBitMask == 0xf0 ? D3DFMT_A4R4G4B4 : D3DFMT_A1R5G5B5;
            return pf->dwGBitMask == 0x7e0 ? D3DFMT_R5G6B5 : D3DFMT_X1R5G5B5;
        case 32:
            return pf->dwRGBAlphaBitMask ? D3DFMT_A8R8G8B8 : D3DFMT_X8R8G8B8;
    }
    return D3DFMT_UNKNOWN;
}

static void SetupLoadDesc(DDSURFACEDESC2* pDesc, int w, int h, D3DLOCKED_RECT* pLock, DDPIXELFORMAT* pf, int nMips)
{
    memset(pDesc, 0, sizeof(*pDesc));
    pDesc->dwWidth = w;
    pDesc->dwHeight = h;
    pDesc->lPitch = pLock->Pitch;
    pDesc->lpSurface = pLock->pBits;
    pDesc->ddpfPixelFormat = *pf;
    pDesc->dwMipMapCount = nMips;
}

int cD8States::reload_texture(tdrv_texture_info* info)
{
    grs_bitmap* bm = info->bm;
    d3d_cookie cookie;
    cookie.value = info->cookie;
    sTextureData* psTD = &g_saTextures[info->id];
    DDPIXELFORMAT* pixel_format;

    if (bm->type == BMT_DXT) {
        int nDXT = (bm->compression & 0xf) - 1;
        if ((unsigned)nDXT > 4)
            nDXT = 4;
        pixel_format = &g_DXTTexFormat[nDXT];
    } else
        pixel_format = g_FormatList[cookie.flags & PF_FORMAT_MASK];
    if ((cookie.flags & PF_FORMAT_MASK) == 4 && !g_b8888supported && bm->type != BMT_DXT)
        pixel_format = g_FormatList[2];
    if (!pixel_format->dwFlags)
        return TDRV_FAILURE;

    BOOL bAutoMip = g_nMipmapMode && (bm->flags & BMF_MIPMAP)
                    && !(bm->extra && (*(DWORD*)bm->extra & 0x100))
                    && !(bm->exflags & BMEF_MIPMAPPED);
    int nFilterMode = g_nMipmapMode;
    if (bAutoMip && (bm->flags & BMF_DYNAMIC) && g_nMipmapMode && g_bLightmapMipmapLinear)
        nFilterMode = 1;

    UINT nLevels = 1;
    D3DFORMAT fmt = FormatFromPixelFormat(pixel_format);
    if (fmt == D3DFMT_UNKNOWN)
        return TDRV_FAILURE;
    if (!bAutoMip || (info->w < 2 && info->h < 2)) {
        if ((bm->exflags & BMEF_MIPMAPPED) && bm->mips)
            nLevels = bm->mips + 1;
    } else {
        for (int w = info->w, h = info->h; w > 1 || h > 1; w >>= 1, h >>= 1)
            nLevels++;
    }

    IDirect3DBaseTexture9* pStaging = NULL;
    BOOL bCube = (bm->exflags & BMEF_CUBEMAP) != 0;
    HRESULT hr;
    if (!g_bD8ManagedTextures) {
        if ((bm->flags & BMF_DYNAMIC) && g_apStagingTextures[g_nNextStagingTexture]) {
            pStaging = g_apStagingTextures[g_nNextStagingTexture];
            pStaging->AddRef();
            g_nNextStagingTexture = (g_nNextStagingTexture + 1) % 8;
        } else if (!bCube) {
            hr = g_lpD3D9Device->CreateTexture(info->w, info->h, nLevels, 0, fmt, D3DPOOL_SYSTEMMEM,
                                               (IDirect3DTexture9**)&pStaging, NULL);
#line 1846
            CheckHResult(hr, "CreateTexture (sysmem) failed");
            if (!pStaging) {
                if (LogGetOptions() & kLogToAll)
                    LogMsg6("CreateTexture (sysmem) failed (%d) [%dx%d fmt=%d lvl=%d dyn=%d]", hr, info->w, info->h, fmt, nLevels, (bm->flags & BMF_DYNAMIC) != 0);
                return TDRV_FAILURE;
            }
            if (bm->flags & BMF_DYNAMIC) {
                g_apStagingTextures[g_nNextStagingTexture] = (IDirect3DTexture9*)pStaging;
                pStaging->AddRef();
                g_nNextStagingTexture = (g_nNextStagingTexture + 1) % 8;
            }
        } else {
            hr = g_lpD3D9Device->CreateCubeTexture(info->w, nLevels, 0, fmt, D3DPOOL_SYSTEMMEM,
                                                   (IDirect3DCubeTexture9**)&pStaging, NULL);
#line 1830
            CheckHResult(hr, "CreateCubeTexture (sysmem) failed");
            if (!pStaging) {
                if (LogGetOptions() & kLogToAll)
                    LogMsg5("CreateCubeTexture (sysmem) failed (%d) [%dx%d fmt=%d lvl=%d]", hr, info->w, info->h, fmt, nLevels);
                return TDRV_FAILURE;
            }
        }
    }

    if (fmt == D3DFMT_P8)
        info->cookie |= PF_PALETTIZED << 16;
    IDirect3DBaseTexture9** ppTexture = (IDirect3DBaseTexture9**)&psTD->lpTexture;
    if (*ppTexture) {
        D3DSURFACE_DESC desc;
        IDirect3DTexture9* pOld = (IDirect3DTexture9*)*ppTexture;
        pOld->PreLoad();
        pOld->GetLevelDesc(0, &desc);
        pOld->GetLevelCount();
        if (desc.Format != fmt || (desc.Type != D3DRTYPE_CUBETEXTURE) != !bCube) {
            SafeRelease(*ppTexture);
            *ppTexture = NULL;
        }
    }
    if (!*ppTexture) {
        D3DPOOL pool = g_bD8ManagedTextures ? D3DPOOL_MANAGED : D3DPOOL_DEFAULT;
        const char* pszErr;
        if (!bCube) {
            hr = g_lpD3D9Device->CreateTexture(info->w, info->h, nLevels, 0, fmt, pool,
                                               (IDirect3DTexture9**)ppTexture, NULL);
            pszErr = "CreateTexture failed";
        } else {
            hr = g_lpD3D9Device->CreateCubeTexture(info->w, nLevels, 0, fmt, pool,
                                                   (IDirect3DCubeTexture9**)ppTexture, NULL);
            pszErr = "CreateCubeTexture failed";
        }
        if (hr != S_OK) {
            if (LogGetOptions() & kLogToAll)
                LogMsg6("%s (%d) [%dx%d fmt=%d lvl=%d]", pszErr, hr, info->w, info->h, fmt, nLevels);
            if (bSpewOn)
                mprintf("couldn't load texture: error %i.\n", HRESULT_CODE(hr));
            SafeRelease(pStaging);
            *ppTexture = NULL;
            return TDRV_FAILURE;
        }
    }

    IDirect3DBaseTexture9* pTarget = pStaging ? pStaging : *ppTexture;
    D3DLOCKED_RECT lock;
    DDSURFACEDESC2 ddsd;
    if (!bCube) {
        IDirect3DTexture9* pTex = (IDirect3DTexture9*)pTarget;
        hr = pTex->LockRect(0, &lock, NULL, 0);
#line 1982
        CheckHResult(hr, "LockRect failed");
        SetupLoadDesc(&ddsd, info->w, info->h, &lock, pixel_format, nLevels);
        LoadSurface(info, &ddsd);
        hr = pTex->UnlockRect(0);
#line 1994
        CheckHResult(hr, "UnlockRect failed");
        if (nLevels >= 2 && !bAutoMip) {
            tdrv_texture_info sub = *info;
            grs_bitmap* pLevel = bm;
            for (UINT nLevel = 1; nLevel < nLevels; nLevel++) {
                pLevel++;
                hr = pTex->LockRect(nLevel, &lock, NULL, 0);
#line 2004
                CheckHResult(hr, "LockRect failed for mipmap");
                sub.bm = pLevel;
                sub.w = pLevel->w;
                sub.h = pLevel->h;
                sub.bits = pLevel->bits;
                SetupLoadDesc(&ddsd, sub.w, sub.h, &lock, pixel_format, nLevels);
                LoadSurface(&sub, &ddsd);
                hr = pTex->UnlockRect(nLevel);
#line 2021
                CheckHResult(hr, "UnlockRect failed for mipmap");
            }
        }
    } else {
        IDirect3DCubeTexture9* pCube = (IDirect3DCubeTexture9*)pTarget;
        int nFaceLevels = nLevels < 2 ? 1 : nLevels;
        tdrv_texture_info sub = *info;
        grs_bitmap* pLevel = bm;
        for (int nFace = 0; nFace < 6; nFace++) {
            for (int nLevel = 0; nLevel < nFaceLevels; nLevel++, pLevel++) {
                hr = pCube->LockRect((D3DCUBEMAP_FACES)nFace, nLevel, &lock, NULL, 0);
#line 1954
                CheckHResult(hr, "LockRect failed for cube face");
                sub.bm = pLevel;
                sub.w = pLevel->w;
                sub.h = pLevel->h;
                sub.bits = pLevel->bits;
                SetupLoadDesc(&ddsd, sub.w, sub.h, &lock, pixel_format, nLevels);
                LoadSurface(&sub, &ddsd);
                hr = pCube->UnlockRect((D3DCUBEMAP_FACES)nFace, nLevel);
#line 1971
                CheckHResult(hr, "UnlockRect failed for cube face");
            }
            if (bm->exflags & BMEF_MIPMAPPED)
                pLevel++;
        }
    }

    if (nLevels > 1 && bAutoMip && !(info->cookie & (PF_PALETTIZED << 16))) {
        if (nFilterMode == 1) {
            hr = g_pfnD3DXFilterTexture(pTarget, NULL, 0, D3DX_DEFAULT);
#line 2041
            CheckHResult(hr, "D3DXFilterTexture (mipmap creation)");
        } else {
            IDirect3DTexture9* pTex = (IDirect3DTexture9*)pTarget;
            IDirect3DSurface9* pSrc = NULL;
            pTex->GetSurfaceLevel(0, &pSrc);
            if (pSrc) {
                for (UINT nLevel = 1; nLevel < nLevels; nLevel++) {
                    IDirect3DSurface9* pDst;
                    hr = pTex->GetSurfaceLevel(nLevel, &pDst);
                    if (hr != S_OK) {
#line 2059
                        CriticalHResult(hr, "GetSurfaceLevel for mipmap");
                        break;
                    }
                    hr = g_pfnD3DXLoadSurfaceFromSurface(pDst, NULL, NULL, pSrc, NULL, NULL, D3DX_FILTER_BOX, 0);
                    if (hr != S_OK) {
#line 2065
                        CriticalHResult(hr, "CopyMipMap (mipmap creation)");
                        break;
                    }
                    pSrc->Release();
                    pSrc = pDst;
                }
                pSrc->Release();
            }
        }
    }

    if (pStaging) {
        hr = g_lpD3D9Device->UpdateTexture(pStaging, *ppTexture);
#line 2080
        CheckHResult(hr, "UpdateTexture failed");
        pStaging->Release();
    }
    psTD->pTdrvBitmap = bm;
    psTD->TdrvCookie.value = info->cookie;
    return TDRV_SUCCESS;
}

D3DFORMAT g_D8MonoBitmapFormat = D3DFMT_A8;

#define D8BT_MANAGED    0x1
#define D8BT_DYNAMIC    0x2
#define D8BT_STAGING    0x4
#define D8BT_NO_UPLOAD  0x8

void LoadSurfaceBits(DDSURFACEDESC2 *ddsd, tdrv_texture_info *info);

BOOL D8_CreateBitmapTexture(IDirect3DDevice9* pDevice, IDirect3DTexture9** ppTexture, DWORD dwFlags, grs_bitmap* pBm)
{
    D3DFORMAT fmt;
    BOOL bManaged = (dwFlags & D8BT_MANAGED) != 0;
    DWORD dwUsage = ((dwFlags & D8BT_DYNAMIC) && !bManaged) ? D3DUSAGE_DYNAMIC : 0;

    switch (pBm->type) {
        case BMT_FLAT8:
            if ((pBm->flags & BMF_RGB_MASK) != 0xe00 && (pBm->flags & BMF_RGB_MASK) != 0xf00) {
                fmt = D3DFMT_A8R8G8B8;
                break;
            }
        case BMT_MONO:
            fmt = g_D8MonoBitmapFormat;
            if (!fmt)
                return FALSE;
            break;
        case BMT_FLAT16:
            switch (pBm->flags & BMF_RGB_MASK) {
                case BMF_RGB_4444:
                    fmt = D3DFMT_A4R4G4B4;
                    break;
                case BMF_RGB_1555:
                    fmt = D3DFMT_A1R5G5B5;
                    break;
                case BMF_RGB_555:
                    fmt = D3DFMT_X1R5G5B5;
                    break;
                default:
                    fmt = D3DFMT_R5G6B5;
                    break;
            }
            break;
        case BMT_FLAT24:
            fmt = D3DFMT_R8G8B8;
            break;
        case BMT_FLAT32:
            fmt = D3DFMT_A8R8G8B8;
            break;
        case BMT_DXT:
            switch (pBm->flags & BMF_RGB_MASK) {
                case BMF_DXT1: fmt = D3DFMT_DXT1; break;
                case BMF_DXT2: fmt = D3DFMT_DXT2; break;
                case BMF_DXT3: fmt = D3DFMT_DXT3; break;
                case BMF_DXT4: fmt = D3DFMT_DXT4; break;
                case BMF_DXT5: fmt = D3DFMT_DXT5; break;
                default: return FALSE;
            }
            break;
        default:
            return FALSE;
    }

    IDirect3DTexture9* pStaging = NULL;
    HRESULT hr;
    if ((!bManaged && !dwUsage) || (dwFlags & D8BT_STAGING)) {
        hr = pDevice->CreateTexture(pBm->w, pBm->h, 1, 0, fmt, D3DPOOL_SYSTEMMEM, &pStaging, NULL);
        if (!pStaging) {
            if (LogGetOptions() & kLogToAll)
                LogMsg5("CreateTexture (sysmem) failed (%d) [%dx%d fmt=%d lvl=%d]", hr, pBm->w, pBm->h, fmt, 1);
            return FALSE;
        }
        if ((dwFlags & D8BT_STAGING) && (dwFlags & D8BT_NO_UPLOAD)) {
            *ppTexture = pStaging;
            return TRUE;
        }
    }
    if (!(dwFlags & D8BT_STAGING) && !*ppTexture) {
        hr = pDevice->CreateTexture(pBm->w, pBm->h, 1, dwUsage, fmt, bManaged ? D3DPOOL_MANAGED : D3DPOOL_DEFAULT,
                                    ppTexture, NULL);
        if (hr != S_OK) {
            if (LogGetOptions() & kLogToAll)
                LogMsg7("CreateTexture failed (%d) [%dx%d fmt=%d lvl=%d usage=%d managed=%d]", hr, pBm->w, pBm->h, fmt, 1, dwUsage, bManaged);
            if (bSpewOn)
                mprintf("couldn't load texture: error %i.\n", HRESULT_CODE(hr));
            SafeRelease(pStaging);
            *ppTexture = NULL;
            return FALSE;
        }
    }

    IDirect3DTexture9* pTarget = pStaging ? pStaging : *ppTexture;
    D3DLOCKED_RECT lock;
    hr = pTarget->LockRect(0, &lock, NULL, dwUsage ? D3DLOCK_DISCARD : 0);
#line 2248
    CheckHResult(hr, "LockRect failed");
    tdrv_texture_info info;
    memset(&info, 0, sizeof(info));
    info.bm = pBm;
    info.id = 1;
    info.scale_w = 1;
    info.w = pBm->w;
    info.h = pBm->h;
    DDSURFACEDESC2 ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwWidth = pBm->w;
    ddsd.dwHeight = pBm->h;
    ddsd.lPitch = lock.Pitch;
    ddsd.lpSurface = lock.pBits;
    ddsd.dwMipMapCount = 1;
    if (g_D8MonoBitmapFormat == D3DFMT_A8)
        ddsd.ddpfPixelFormat.dwRGBBitCount = 8;
    else
        ddsd.ddpfPixelFormat.dwRGBBitCount = (g_D8MonoBitmapFormat != D3DFMT_A8L8) ? 32 : 16;
    LoadSurfaceBits(&ddsd, &info);
    hr = pTarget->UnlockRect(0);
#line 2261
    CheckHResult(hr, "UnlockRect failed");
    if (pStaging) {
        if (!(dwFlags & D8BT_STAGING)) {
            hr = pDevice->UpdateTexture(pStaging, *ppTexture);
#line 2272
            CheckHResult(hr, "UpdateTexture failed");
            pStaging->Release();
            return *ppTexture != NULL;
        }
        *ppTexture = pStaging;
    }
    return TRUE;
}

void cD8States::TurnOffTexuring(int bTexOff)
{
    if (bTexOff) {
        SetTexture(0, NULL);
        m_psSetRS->naTextureId[0] = -1;
        g_bTexSuspended = TRUE;
        if (g_nAlphaTestMode && m_psSetRS->bAlphaTestOn) {
            SetAlphaTestEnable(FALSE);
            m_psSetRS->bAlphaTestOn = FALSE;
        }
    } else
        SetTextureNow();
}

void cD8States::SetTextureNow()
{
    int n = m_psCurrentRS->naTextureId[0];
    g_bTexSuspended = FALSE;
    if (n != -1) {
        sTextureData* pEntry = &g_saTextures[n];
        SetTexture(0, (IDirect3DBaseTexture9*)pEntry->lpTexture);
        if (pEntry->TdrvCookie.flags & PF_PALETTIZED) {
            HRESULT hr = g_lpD3D9Device->SetCurrentTexturePalette(pEntry->TdrvCookie.palette);
#line 2315
            CheckHResult(hr, "SetCurrentTexturePalette failed");
        }
    }
}

void cD8States::EnableDepthBuffer(int val)
{
    pcRenderBuffer->FlushPrimitives();
    m_psCurrentRS->eZenable = val ? D3DZB_TRUE : D3DZB_FALSE;
    if (m_psSetRS->eZenable != m_psCurrentRS->eZenable && !PUNT_D3D) {
        m_psSetRS->eZenable = m_psCurrentRS->eZenable;
        if (!PUNT_D3D) {
#line 2334
            D8CheckedSetRS(D3DRS_ZENABLE, m_psCurrentRS->eZenable);
        }
    }
}

void cD8States::SetZWrite(BOOL val)
{
    pcRenderBuffer->FlushPrimitives();
    m_psCurrentRS->bZWriteEnable = val;
    if (m_psSetRS->bZWriteEnable != m_psCurrentRS->bZWriteEnable && !PUNT_D3D) {
        m_psSetRS->bZWriteEnable = val;
        if (!PUNT_D3D) {
#line 2351
            D8CheckedSetRS(D3DRS_ZWRITEENABLE, m_psCurrentRS->bZWriteEnable);
        }
    }
}

void cD8States::SetZCompare(BOOL val)
{
    pcRenderBuffer->FlushPrimitives();
    m_psCurrentRS->eZCompareFunc = val ? D3DCMP_LESSEQUAL : D3DCMP_ALWAYS;
    if (m_psSetRS->eZCompareFunc != m_psCurrentRS->eZCompareFunc && !PUNT_D3D) {
        m_psSetRS->eZCompareFunc = m_psCurrentRS->eZCompareFunc;
        if (!PUNT_D3D) {
#line 2369
            D8CheckedSetRS(D3DRS_ZFUNC, m_psCurrentRS->eZCompareFunc);
        }
    }
}

void cD8States::SetColorWriteEnable(DWORD dwMask)
{
#line 2376
    D8CheckedSetRS(D3DRS_COLORWRITEENABLE, dwMask);
}

void cD8States::SetFogDensity(float density)
{
    if (g_bUseTableFog || g_bUseVertexFog) {
        pcRenderBuffer->FlushPrimitives();
        if (!g_bWFog)
            density = density * 0.5 * z_far;
        m_psSetRS->fFogTableDensity = m_psCurrentRS->fFogTableDensity = density;
        if (!PUNT_D3D) {
#line 2394
            D8CheckedSetRS(D3DRS_FOGDENSITY, *(DWORD*)&m_psCurrentRS->fFogTableDensity);
        }
    }
}

cD8States::cD8States():
    m_bAlphaModulated(FALSE), m_dwRequestedFlags(0), m_nAlphaToCoverage(0), m_nCurAlphaRef(0)
{
    m_pCurTexture[0] = NULL;
    m_pCurTexture[1] = NULL;
}

cD8States::~cD8States()
{
    if (default_bm) {
        gr_free(default_bm);
        default_bm = NULL;
    }
    for (int i = 0; i < 0x100; i++) {
        if (texture_pal_opaque[i]) {
            gr_free(texture_pal_opaque[i]);
            texture_pal_opaque[i] = NULL;
        }
        if (texture_pal_trans[i]) {
            gr_free(texture_pal_trans[i]);
            texture_pal_trans[i] = NULL;
        }
    }
    if (m_texture_size_list) {
        free(m_texture_size_list);
        m_texture_size_list = NULL;
    }
    g_pD8States = NULL;
}

void cD8States::ReleaseAllTextures()
{
    if (m_bTextureListInitialized) {
        if (g_tmgr) {
            g_tmgr->shutdown();
            g_tmgr = NULL;
        }
        sTextureData* psTD = g_saTextures;
        for (int i = 0; i < LGD3D_MAX_TEXTURES; i++, psTD++) {
            SafeRelease(psTD->lpTexture);
            SafeRelease(psTD->lpSurface);
        }
        m_bTextureListInitialized = TRUE;
    }
}

cD8ImStates* cD8ImStates::m_Instance = NULL;

cD6States* cD8ImStates::Instance()
{
    if (m_Instance == NULL)
        m_Instance = new cD8ImStates();
    return m_Instance;
}

cD6States* cD8ImStates::DeInstance()
{
    if (m_Instance) {
        m_Instance->ReleaseAllTextures();
        delete m_Instance;
        m_Instance = NULL;
    }
    return m_Instance;
}

cD8ImStates::~cD8ImStates()
{
}

cD8MSStates* cD8MSStates::m_Instance = NULL;

cD6States* cD8MSStates::Instance()
{
    if (m_Instance == NULL) {
        if (config_is_nonzero("d3d_disp_no_batch"))
            m_Instance = new cD8MSStates();
        else
            m_Instance = new cD8MSBatchStates();
    }
    return m_Instance;
}

cD6States* cD8MSStates::DeInstance()
{
    if (m_Instance) {
        m_Instance->ReleaseAllTextures();
        delete m_Instance;
        m_Instance = NULL;
    }
    return m_Instance;
}

cD8MSStates::~cD8MSStates()
{
    for (int i = 0; i < 8; i++) {
        SafeRelease(g_apStagingTextures[i]);
        g_apStagingTextures[i] = NULL;
    }
    g_nNextStagingTexture = 0;
}

DWORD cD8MSStates::GetTextureLevel()
{
    return m_dwCurrentTexLevel;
}

static void D8SetPixelFormat(DDPIXELFORMAT* pf, DWORD dwFlags, DWORD dwBits, DWORD dwR, DWORD dwG, DWORD dwB, DWORD dwA)
{
    pf->dwSize = sizeof(DDPIXELFORMAT);
    pf->dwFlags = dwFlags;
    pf->dwRGBBitCount = dwBits;
    pf->dwRBitMask = dwR;
    pf->dwGBitMask = dwG;
    pf->dwBBitMask = dwB;
    pf->dwRGBAlphaBitMask = dwA;
}

static HRESULT D8EnumTextureFormats()
{
    D3DDEVICE_CREATION_PARAMETERS params;
    D3DDISPLAYMODE mode;
    HRESULT hr;

    hr = g_lpD3D9Device->GetCreationParameters(&params);
    if (FAILED(hr))
        return hr;
    hr = g_lpD3D9Device->GetDisplayMode(0, &mode);
    if (FAILED(hr))
        return hr;

#define D8CheckFormat(fmt) \
    SUCCEEDED(g_lpD3D9->CheckDeviceFormat(params.AdapterOrdinal, params.DeviceType, mode.Format, 0, D3DRTYPE_TEXTURE, (fmt)))

    if (D8CheckFormat(D3DFMT_A8R8G8B8)) {
        D8SetPixelFormat(&g_8888TexFormat, DDPF_RGB | DDPF_ALPHA, 32, 0xff0000, 0xff00, 0xff, 0xff000000);
        g_b8888supported = 1;
    }
    if (D8CheckFormat(D3DFMT_A4R4G4B4))
        D8SetPixelFormat(&g_AlphaTextureFormat, DDPF_RGB | DDPF_ALPHA, 16, 0xf00, 0xf0, 0xf, 0xf000);
    if (D8CheckFormat(D3DFMT_A1R5G5B5))
        D8SetPixelFormat(&g_TransRGBTextureFormat, DDPF_RGB | DDPF_ALPHA, 16, 0x7c00, 0x3e0, 0x1f, 0x8000);
    if (D8CheckFormat(D3DFMT_R5G6B5) && !g_RGBTextureFormat.dwFlags)
        D8SetPixelFormat(&g_RGBTextureFormat, DDPF_RGB, 16, 0xf800, 0x7e0, 0x1f, 0);
    if (D8CheckFormat(D3DFMT_P8)) {
        g_PalTextureFormat.dwSize = sizeof(DDPIXELFORMAT);
        g_PalTextureFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
        g_PalTextureFormat.dwRGBBitCount = 8;
    }
    for (int i = 0; i < 5; i++) {
        D3DFORMAT fmt = (D3DFORMAT)(D3DFMT_DXT1 + (i << 24));
        if (D8CheckFormat(fmt))
            g_aDXTFormats[i] = fmt;
    }
#undef D8CheckFormat

    if (!g_aDXTFormats[1])
        g_aDXTFormats[1] = g_aDXTFormats[2];
    if (!g_aDXTFormats[3])
        g_aDXTFormats[3] = g_aDXTFormats[4];
    for (int i = 0; i < 5; i++) {
        if (g_aDXTFormats[i]) {
            D8SetPixelFormat(&g_DXTTexFormat[i], DDPF_RGB | DDPF_ALPHA, 32, 0xff0000, 0xff00, 0xff, 0xff000000);
            g_DXTTexFormat[i].dwFourCC = g_aDXTFormats[i];
        }
    }
    return S_OK;
}

void cD8States::EnumerateTextureFormats()
{
    D3DCAPS9 caps;
    HRESULT hr;

    memset(&g_RGBTextureFormat, 0, sizeof(g_RGBTextureFormat));
    memset(&g_TransRGBTextureFormat, 0, sizeof(g_TransRGBTextureFormat));
    memset(&g_PalTextureFormat, 0, sizeof(g_PalTextureFormat));
    memset(&g_8888TexFormat, 0, sizeof(g_8888TexFormat));
    g_b8888supported = 0;
    memset(g_DXTTexFormat, 0, sizeof(g_DXTTexFormat));

    if (FAILED(D8EnumTextureFormats())) {
#line 2566
        CriticalMsg("EnumTextureFormats failed\n");
    }

    m_bTexture_RGB = g_bPrefer_RGB;
    g_FormatList[0] = (m_bTexture_RGB != 0) ? &g_RGBTextureFormat : &g_PalTextureFormat;
    if (g_FormatList[0]->dwFlags == 0) {
        m_bTexture_RGB = !m_bTexture_RGB;
        g_FormatList[0] = (m_bTexture_RGB != 0) ? &g_RGBTextureFormat : &g_PalTextureFormat;
    }
    g_FormatList[1] = &g_RGBTextureFormat;
    g_FormatList[2] = &g_AlphaTextureFormat;
    g_FormatList[3] = &g_TransRGBTextureFormat;
    g_FormatList[4] = &g_8888TexFormat;
    g_FormatList[5] = &g_8888TexFormat;

    if (bSpewOn)
        mprintf("Using %s textures\n", (m_bTexture_RGB != 0) ? "16 bit RGB" : "Palettized");
    if (g_AlphaTextureFormat.dwFlags == 0) {
        if (bSpewOn)
            mprintf("no alpha texture format available.\n");
    }

    memset(&caps, 0, sizeof(caps));
    hr = g_lpD3D9Device->GetDeviceCaps(&caps);
#line 2612
    CheckHResult(hr, "Failed to obtain device caps");

    m_bAGP_available = 0;
    m_bLocalMem_available = 0;
    if (caps.DevCaps & D3DDEVCAPS_TEXTURENONLOCALVIDMEM) {
        m_bAGP_available = 1;
        if (bSpewOn)
            mprintf("nonlocal videomemory textures available.\n");
    }
    if (caps.DevCaps & D3DDEVCAPS_TEXTUREVIDEOMEMORY) {
        m_bLocalMem_available = 1;
        if (bSpewOn)
            mprintf("local videomemory textures available.\n");
    }
    if (!m_bAGP_available && !m_bLocalMem_available) {
        if (bSpewOn)
            mprintf("No local or nonlocal texture memory! Using system memory textures.\n");
    }
    m_bUsingLocalMem = m_bLocalMem_available;
}

static BOOL D8ValidateDevice(int nErrorCode)
{
    DWORD dwPasses;
    D8SetFVF(D8_TLVERTEX_FVF);
    HRESULT hR = g_lpD3D9Device->ValidateDevice(&dwPasses);
    if (hR != D3D_OK && !config_is_nonzero("d3d_disp_forcevalid")) {
        SetLGD3DErrorCode(nErrorCode, hR);
        lgd3d_g_bInitialized = FALSE;
        return FALSE;
    }
    return TRUE;
}

int cD8ImStates::SetDefaultsStates(DWORD dwRequestedFlags)
{
    SetCommonDefaultStates(dwRequestedFlags, 0);
    memcpy(m_psCurrentRS->saTexBlend, sTexBlendArgsProtos[m_psCurrentRS->dwTexBlendMode[0]],
           sizeof(m_psCurrentRS->saTexBlend));
#line 2406
    D8CheckedSetTSS(0, D3DTSS_TEXCOORDINDEX, 0);
    D8CheckedSetTSS(0, D3DTSS_COLOROP, m_psCurrentRS->saTexBlend[0].eColorOperation);
    D8CheckedSetTSS(0, D3DTSS_COLORARG1, m_psCurrentRS->saTexBlend[0].dwColorArg1);
    D8CheckedSetTSS(0, D3DTSS_COLORARG2, m_psCurrentRS->saTexBlend[0].dwColorArg2);
    D8CheckedSetTSS(0, D3DTSS_ALPHAOP, m_psCurrentRS->saTexBlend[0].eAlphaOperation);
    D8CheckedSetTSS(0, D3DTSS_ALPHAARG1, m_psCurrentRS->saTexBlend[0].dwAlphaArg1);
    D8CheckedSetTSS(0, D3DTSS_ALPHAARG2, m_psCurrentRS->saTexBlend[0].dwAlphaArg2);
    D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, TRUE);
    if (dwRequestedFlags & LGD3DF_MULTITEXTURE_COLOR) {
#line 2417
        D8CheckedSetRS(D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR);
        D8CheckedSetRS(D3DRS_DESTBLEND, D3DBLEND_ZERO);
        if (!D8ValidateDevice(LGD3D_EC_VD_MPASS_MT))
            return 0;
    }
    m_psCurrentRS->eSrcAlpha = (D3DBLEND)D3DBLEND_SRCALPHA;
#line 2431
    D8CheckedSetRS(D3DRS_SRCBLEND, m_psCurrentRS->eSrcAlpha);
    m_psCurrentRS->eDstAlpha = (D3DBLEND)D3DBLEND_INVSRCALPHA;
    D8CheckedSetRS(D3DRS_DESTBLEND, m_psCurrentRS->eDstAlpha);
    m_bAlphaModulated = FALSE;
    if (!D8ValidateDevice(LGD3D_EC_VD_S_DEFAULT))
        return 0;
    memcpy(m_psCurrentRS->saTexBlend, sTexBlendArgsProtos[m_psCurrentRS->dwTexBlendMode[0]],
           sizeof(m_psCurrentRS->saTexBlend));
#line 2451
    D8CheckedSetTSS(0, D3DTSS_TEXCOORDINDEX, 0);
    D8CheckedSetTSS(0, D3DTSS_COLOROP, m_psCurrentRS->saTexBlend[0].eColorOperation);
    D8CheckedSetTSS(0, D3DTSS_COLORARG1, m_psCurrentRS->saTexBlend[0].dwColorArg1);
    D8CheckedSetTSS(0, D3DTSS_COLORARG2, m_psCurrentRS->saTexBlend[0].dwColorArg2);
    D8CheckedSetTSS(0, D3DTSS_ALPHAOP, m_psCurrentRS->saTexBlend[0].eAlphaOperation);
    D8CheckedSetTSS(0, D3DTSS_ALPHAARG1, m_psCurrentRS->saTexBlend[0].dwAlphaArg1);
    D8CheckedSetTSS(0, D3DTSS_ALPHAARG2, m_psCurrentRS->saTexBlend[0].dwAlphaArg2);
    m_psCurrentRS->bAlphaOn = FALSE;
#line 2460
    D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
    memcpy(m_psSetRS, m_psCurrentRS, sizeof(sRenderStates));
    g_dwRSChangeFlags = 0;
    return 1;
}

void cD8States::SetCommonDefaultStates(DWORD dwRequestedFlags, int bMultiTexture)
{
    DWORD dwPasses;
    HRESULT hR;

    memset(m_psCurrentRS, 0, sizeof(sRenderStates));
    memset(m_psSetRS, 0, sizeof(sRenderStates));
    SetTrilinearFiltering(g_bTrilinearFiltering);
#line 2646
    D8CheckedSetRS(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
    m_psSetRS->dwCullMode = D3DCULL_CCW;
    m_psCurrentRS->dwCullMode = D3DCULL_CCW;
#line 2649
    D8CheckedSetRS(D3DRS_CULLMODE, m_psCurrentRS->dwCullMode);
#line 2651
    D8CheckedSetRS(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
    m_nCurAlphaRef = g_nAlphaTestRef;
    D8CheckedSetRS(D3DRS_ALPHAREF, g_nAlphaTestRef);
    if (g_nAlphaTestMode)
        SetAlphaTestEnable(FALSE);

    m_bCanDither = (dwRequestedFlags & LGD3DF_CAN_DO_DITHER) != 0;
    m_psCurrentRS->bDitheringOn = (m_bCanDither && (dwRequestedFlags & LGD3DF_DITHER)) ? 1 : 0;
#line 2661
    D8CheckedSetRS(D3DRS_DITHERENABLE, m_psCurrentRS->bDitheringOn);
    m_bCanAntialias = (dwRequestedFlags & LGD3DF_CAN_DO_ANTIALIAS) != 0;
    m_psCurrentRS->bAntialiasingOn = (m_bCanAntialias && (dwRequestedFlags & LGD3DF_ANTIALIAS)) ? 1 : 0;

    m_psCurrentRS->bUsePalette = 1;
    m_psCurrentRS->nAlphaColor = 0xff;
    pcRenderBuffer->PassAlphaColor(m_psCurrentRS->nAlphaColor);
    m_psCurrentRS->eShadeMode = D3DSHADE_GOURAUD;
#line 2674
    D8CheckedSetRS(D3DRS_SHADEMODE, m_psCurrentRS->eShadeMode);

    if (g_bUseDepthBuffer) {
        m_psCurrentRS->eZenable = m_bWBuffer ? D3DZB_USEW : D3DZB_TRUE;
        m_psSetRS->eZenable = m_psCurrentRS->eZenable;
#line 2678
        D8CheckedSetRS(D3DRS_ZENABLE, m_psSetRS->eZenable);
        m_psCurrentRS->bZWriteEnable = 0;
        m_psSetRS->bZWriteEnable = m_psCurrentRS->bZWriteEnable;
#line 2681
        D8CheckedSetRS(D3DRS_ZWRITEENABLE, m_psSetRS->bZWriteEnable);
        m_psCurrentRS->eZCompareFunc = D3DCMP_ALWAYS;
        m_psSetRS->eZCompareFunc = m_psCurrentRS->eZCompareFunc;
#line 2684
        D8CheckedSetRS(D3DRS_ZFUNC, m_psSetRS->eZCompareFunc);
    }

    m_psCurrentRS->bFogOn = 0;
    m_psCurrentRS->dwFogColor = 0;
    m_psCurrentRS->eFogMode = D3DFOG_LINEAR;
    m_psCurrentRS->fFogTableDensity = 0.025f;
    m_psCurrentRS->fFogStart = 10000.0f;
    m_psCurrentRS->fFogEnd = 10000.0f;
    m_psCurrentRS->dcFogSpecular = 0xff000000;
    pcRenderBuffer->PassFogSpecularColor(m_psCurrentRS->dcFogSpecular);
    if (g_bUseTableFog || g_bUseVertexFog) {
#line 2697
        D8CheckedSetRS(D3DRS_FOGENABLE, m_psCurrentRS->bFogOn);
        D8CheckedSetRS(D3DRS_FOGCOLOR, m_psCurrentRS->dwFogColor);
        D8CheckedSetRS(D3DRS_FOGDENSITY, *(DWORD*)&m_psCurrentRS->fFogTableDensity);
        if (g_bUseTableFog) {
#line 2703
            D8CheckedSetRS(D3DRS_FOGTABLEMODE, m_psCurrentRS->eFogMode);
        }
    }

    m_psCurrentRS->naTextureId[0] = -1;
    m_psCurrentRS->naTextureId[1] = -1;
    SetTexture(0, NULL);
    m_psCurrentRS->bPerspectiveCorr = 1;
    m_psCurrentRS->baChromaKeying[0] = 0;
    m_psCurrentRS->eWrap[0] = D3DTADDRESS_WRAP;
#line 2720
    D8CheckedSetSS(0, D3DSAMP_ADDRESSU, m_psCurrentRS->eWrap[0]);
    D8CheckedSetSS(0, D3DSAMP_ADDRESSV, m_psCurrentRS->eWrap[0]);
    m_psCurrentRS->eMagTexFilter = (D3DTEXTUREMINFILTER)D3DTEXF_LINEAR;
    m_psCurrentRS->eMinTexFilter = (D3DTEXTUREMINFILTER)D3DTEXF_LINEAR;
#line 2726
    D8CheckedSetSS(0, D3DSAMP_MAGFILTER, m_psCurrentRS->eMagTexFilter);
    D8CheckedSetSS(0, D3DSAMP_MINFILTER, m_psCurrentRS->eMinTexFilter);
    m_psCurrentRS->eLightMapMagFilter = (D3DTEXTUREMINFILTER)D3DTEXF_LINEAR;
    m_psCurrentRS->eLightMapMinFilter = (D3DTEXTUREMINFILTER)D3DTEXF_LINEAR;
#line 2731
    D8CheckedSetSS(1, D3DSAMP_MAGFILTER, m_psCurrentRS->eLightMapMagFilter);
    D8CheckedSetSS(1, D3DSAMP_MINFILTER, m_psCurrentRS->eLightMapMinFilter);
    m_psCurrentRS->dwTexBlendMode[1] = 0;
    m_psCurrentRS->baTextureWithAlpha[0] = 0;
    m_psCurrentRS->baTextureWithAlpha[1] = 0;
#line 2737
    D8CheckedSetRS(D3DRS_SPECULARENABLE, m_bSpecular);
    D8CheckedSetRS(D3DRS_STENCILENABLE, FALSE);
    m_psSetRS->dwNoLightMapLevels = 2;

    memcpy(m_psCurrentRS->saTexBlend, sTexBlendArgsProtos[0], sizeof(m_psCurrentRS->saTexBlend));
#line 2743
    D8CheckedSetTSS(0, D3DTSS_TEXCOORDINDEX, 0);
    D8CheckedSetTSS(0, D3DTSS_COLOROP, m_psCurrentRS->saTexBlend[0].eColorOperation);
    D8CheckedSetTSS(0, D3DTSS_COLORARG1, m_psCurrentRS->saTexBlend[0].dwColorArg1);
    D8CheckedSetTSS(0, D3DTSS_COLORARG2, m_psCurrentRS->saTexBlend[0].dwColorArg2);
    D8CheckedSetTSS(0, D3DTSS_ALPHAOP, m_psCurrentRS->saTexBlend[0].eAlphaOperation);
    D8CheckedSetTSS(0, D3DTSS_ALPHAARG1, m_psCurrentRS->saTexBlend[0].dwAlphaArg1);
    D8CheckedSetTSS(0, D3DTSS_ALPHAARG2, m_psCurrentRS->saTexBlend[0].dwAlphaArg2);
    if (bMultiTexture) {
#line 2753
        D8CheckedSetTSS(1, D3DTSS_TEXCOORDINDEX, 1);
        D8CheckedSetTSS(1, D3DTSS_COLOROP, m_psCurrentRS->saTexBlend[1].eColorOperation);
        D8CheckedSetTSS(1, D3DTSS_COLORARG1, m_psCurrentRS->saTexBlend[1].dwColorArg1);
        D8CheckedSetTSS(1, D3DTSS_COLORARG2, m_psCurrentRS->saTexBlend[1].dwColorArg2);
        D8CheckedSetTSS(1, D3DTSS_ALPHAOP, m_psCurrentRS->saTexBlend[1].eAlphaOperation);
        D8CheckedSetTSS(1, D3DTSS_ALPHAARG1, m_psCurrentRS->saTexBlend[1].dwAlphaArg1);
        D8CheckedSetTSS(1, D3DTSS_ALPHAARG2, m_psCurrentRS->saTexBlend[1].dwAlphaArg2);
    }
    D8SetFVF(D8_TLVERTEX_FVF);
    hR = g_lpD3D9Device->ValidateDevice(&dwPasses);
    m_bCanModulate = (hR == D3D_OK);
    if (!m_bCanModulate && config_is_nonzero("d3d_disp_forcevalid"))
        m_bCanModulate = TRUE;

    memcpy(m_psCurrentRS->saTexBlend, sTexBlendArgsProtos[1], sizeof(m_psCurrentRS->saTexBlend));
#line 2777
    D8CheckedSetTSS(0, D3DTSS_TEXCOORDINDEX, 0);
    D8CheckedSetTSS(0, D3DTSS_COLOROP, m_psCurrentRS->saTexBlend[0].eColorOperation);
    D8CheckedSetTSS(0, D3DTSS_COLORARG1, m_psCurrentRS->saTexBlend[0].dwColorArg1);
    D8CheckedSetTSS(0, D3DTSS_COLORARG2, m_psCurrentRS->saTexBlend[0].dwColorArg2);
    D8CheckedSetTSS(0, D3DTSS_ALPHAOP, m_psCurrentRS->saTexBlend[0].eAlphaOperation);
    D8CheckedSetTSS(0, D3DTSS_ALPHAARG1, m_psCurrentRS->saTexBlend[0].dwAlphaArg1);
    D8CheckedSetTSS(0, D3DTSS_ALPHAARG2, m_psCurrentRS->saTexBlend[0].dwAlphaArg2);
    if (bMultiTexture) {
#line 2787
        D8CheckedSetTSS(1, D3DTSS_TEXCOORDINDEX, 1);
        D8CheckedSetTSS(1, D3DTSS_COLOROP, m_psCurrentRS->saTexBlend[1].eColorOperation);
        D8CheckedSetTSS(1, D3DTSS_COLORARG1, m_psCurrentRS->saTexBlend[1].dwColorArg1);
        D8CheckedSetTSS(1, D3DTSS_COLORARG2, m_psCurrentRS->saTexBlend[1].dwColorArg2);
        D8CheckedSetTSS(1, D3DTSS_ALPHAOP, m_psCurrentRS->saTexBlend[1].eAlphaOperation);
        D8CheckedSetTSS(1, D3DTSS_ALPHAARG1, m_psCurrentRS->saTexBlend[1].dwAlphaArg1);
        D8CheckedSetTSS(1, D3DTSS_ALPHAARG2, m_psCurrentRS->saTexBlend[1].dwAlphaArg2);
    }
    hR = g_lpD3D9Device->ValidateDevice(&dwPasses);
    if (hR != D3D_OK && config_is_nonzero("d3d_disp_forcevalid"))
        hR = D3D_OK;
    m_bCanModulateAlpha = (hR == D3D_OK);
    if (!m_bCanModulate && !m_bCanModulateAlpha) {
        SetLGD3DErrorCode(LGD3D_EC_VD_S_DEFAULT, hR);
        lgd3d_g_bInitialized = FALSE;
    } else if (dwRequestedFlags & LGD3DF_MODULATEALPHA)
        m_psCurrentRS->dwTexBlendMode[0] = (m_bCanModulateAlpha != 0);
    else
        m_psCurrentRS->dwTexBlendMode[0] = (m_bCanModulate == 0);
}

int cD8MSStates::Initialize(DWORD dwRequestedFlags)
{
    g_bPrefer_RGB = 1;
    return cD8States::Initialize(dwRequestedFlags);
}

void cD8MSStates::set_texture_id(int n)
{
    if (!g_nD8ActiveEffect) {
        m_bTexturePending = TRUE;
        if (m_psSetRS->naTextureId[m_dwCurrentTexLevel] == n) {
            m_psCurrentRS->naTextureId[m_dwCurrentTexLevel] = n;
            return;
        }
        pcRenderBuffer->FlushPrimitives();
        m_psCurrentRS->naTextureId[m_dwCurrentTexLevel] = n;
        if (m_dwCurrentTexLevel == 1)
            m_LastLightMapBm = g_saTextures[n].pTdrvBitmap;
        if (m_psCurrentRS->naTextureId[m_dwCurrentTexLevel] == -2)
            g_tmgr->set_texture_callback();
    } else if (g_bD8EffectTexture && !PUNT_D3D) {
        if (n == -2) {
            g_tmgr->set_texture_callback();
            return;
        }
        HRESULT hr;
        if (n == -1) {
            hr = g_lpD3D9Device->SetTexture(m_dwCurrentTexLevel, NULL);
#line 3059
            CheckHResult(hr, "SetTexture NULL failed for effect");
        } else {
            hr = g_lpD3D9Device->SetTexture(m_dwCurrentTexLevel, (IDirect3DBaseTexture9*)g_saTextures[n].lpTexture);
#line 3055
            CheckHResult(hr, "SetTexture failed for effect");
        }
    }
}

void cD8MSStates::SetLightMapMode(ulong flag)
{
    if (flag >= 3) {
        Warning(("cMSStates::SetLightMapMode -- invalid mode", flag));
    } else {
        m_psCurrentRS->dwTexBlendMode[1] = flag;
    }
}

void cD8MSStates::SetTextureLevel(int n)
{
    m_dwCurrentTexLevel = n;
}

void cD8MSStates::TurnOffTexuring(int bTexOff)
{
    if (!bTexOff) {
        g_bTexSuspended = FALSE;
        m_bTexturePending = TRUE;
        return;
    }
    m_psCurrentRS->dwNoLightMapLevels = 0;
    SetTexture(0, NULL);
    m_psSetRS->naTextureId[0] = -1;
    m_psCurrentRS->baTextureWithAlpha[0] = 0;
    if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn) {
#line 3612
        D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
        m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
    }
    if (g_nAlphaTestMode && m_psSetRS->bAlphaTestOn) {
        SetAlphaTestEnable(FALSE);
        m_psSetRS->bAlphaTestOn = FALSE;
    }
    if (m_psSetRS->dwNoLightMapLevels != m_psCurrentRS->dwNoLightMapLevels ||
        m_psSetRS->dwTexBlendMode[0] != m_psCurrentRS->dwTexBlendMode[0] ||
        m_psSetRS->baTextureWithAlpha[0] != m_psCurrentRS->baTextureWithAlpha[0]) {
        memcpy(m_psCurrentRS->saTexBlend, sTexBlendArgsProtos[m_psCurrentRS->dwTexBlendMode[0]],
               sizeof(m_psCurrentRS->saTexBlend));
        if (m_psCurrentRS->dwTexBlendMode[0] == 0 && m_psCurrentRS->baTextureWithAlpha[0])
            m_psCurrentRS->saTexBlend[0].eAlphaOperation = (D3DTEXTUREOP)D3DTOP_SELECTARG1;
        m_psSetRS->dwTexBlendMode[0] = m_psCurrentRS->dwTexBlendMode[0];
        m_psSetRS->baTextureWithAlpha[0] = m_psCurrentRS->baTextureWithAlpha[0];
        for (int i = 0; i < 2; i++)
            D8SetTexBlend(i, m_psCurrentRS->saTexBlend, m_psSetRS->saTexBlend);
        m_psSetRS->dwNoLightMapLevels = m_psCurrentRS->dwNoLightMapLevels;
    }
    m_bTexturePending = TRUE;
    g_bTexSuspended = TRUE;
}

void cD8MSStates::cook_info(tdrv_texture_info* info)
{
    d3d_cookie cookie;
    int v;
    int w;
    int h;

    if (m_dwCurrentTexLevel == 0) {
        cD6States::cook_info(info);
        return;
    }
    cookie.value = 0;
    if (grd_canvas->gc.fill_type == FILL_BLEND) {
        if (info->bm->type == BMT_FLAT32 ||
            (info->bm->type == BMT_DXT && gr_test_bitmap_format(info->bm, BMF_RGB_8888)))
            cookie.flags = 4;
        else
            cookie.flags = PF_ALPHA;
    } else switch (info->bm->type) {
        case BMT_FLAT8:
            cookie.palette = info->bm->align;
            if ((info->bm->flags & BMF_TRANS) != 0) {
                if (texture_pal_trans[cookie.palette] && g_TransRGBTextureFormat.dwFlags != 0) {
                    cookie.flags = 3;
                    if (g_TransRGBTextureFormat.dwRGBAlphaBitMask == 0x8000)
                        cookie.flags |= PF_ALPHA_1BIT;
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
                    if (g_TransRGBTextureFormat.dwRGBAlphaBitMask == 0x8000)
                        cookie.flags |= PF_ALPHA_1BIT;
                } else {
                    cookie.flags = PF_TRANS | PF_RGB;
                }
            } else {
                cookie.flags = PF_RGB;
            }
            break;
        case BMT_FLAT32:
        case BMT_DXT:
            if (gr_test_bitmap_format(info->bm, BMF_RGB_8888))
                cookie.flags = 4;
            else if ((info->bm->flags & BMF_TRANS) != 0 && gr_test_bitmap_format(info->bm, BMF_RGB_INDEX))
                cookie.flags = texture_pal_trans[info->bm->align] ? (4 | PF_ALPHA_1BIT) : (PF_TRANS | 5);
            else
                cookie.flags = 5;
            break;
    }
    calc_size(info, cookie);
    w = info->w;
    h = info->h;
    cookie.hlog = 0;
    cookie.wlog = 0;
    for (v = 2; v <= w; v += v) cookie.wlog++;
    for (v = 2; v <= h; v += v) cookie.hlog++;
    info->cookie = cookie.value;
}

int cD8MSStates::SetDefaultsStates(DWORD dwRequestedFlags)
{
    DWORD dwPasses;
    HRESULT hR;
    int i;

    SetCommonDefaultStates(dwRequestedFlags, 1);
    m_nAlphaToCoverage = D3DGetAlphaToCoverageMode(g_lpD3D9Device);
    m_psCurrentRS->eSrcAlpha = (D3DBLEND)D3DBLEND_SRCALPHA;
    m_psCurrentRS->eDstAlpha = (D3DBLEND)D3DBLEND_INVSRCALPHA;
    m_psCurrentRS->bAlphaOn = 0;
    m_psCurrentRS->bAlphaTestOn = 0;
    m_bAlphaModulated = FALSE;
    m_psCurrentRS->eWrap[1] = D3DTADDRESS_CLAMP;
#line 2934
    D8CheckedSetRS(D3DRS_SRCBLEND, m_psCurrentRS->eSrcAlpha);
    D8CheckedSetRS(D3DRS_DESTBLEND, m_psCurrentRS->eDstAlpha);
#line 2938
    D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
#line 2940
    D8CheckedSetSS(1, D3DSAMP_MAGFILTER, m_psCurrentRS->eLightMapMagFilter);
    D8CheckedSetSS(1, D3DSAMP_MINFILTER, m_psCurrentRS->eLightMapMinFilter);
#line 2947
    D8CheckedSetSS(1, D3DSAMP_ADDRESSU, m_psCurrentRS->eWrap[1]);
    D8CheckedSetSS(1, D3DSAMP_ADDRESSV, m_psCurrentRS->eWrap[1]);

    if (dwRequestedFlags & LGD3DF_MULTITEXTURE_COLOR) {
        memcpy(m_psCurrentRS->saTexBlend, sMultiTexBlendArgsProtos[0], sizeof(m_psCurrentRS->saTexBlend));
        for (i = 0; i < 2; i++) {
#line 2961
            D8CheckedSetTSS(i, D3DTSS_TEXCOORDINDEX, i);

            D8CheckedSetTSS(i, D3DTSS_COLOROP, m_psCurrentRS->saTexBlend[i].eColorOperation);
            D8CheckedSetTSS(i, D3DTSS_COLORARG1, m_psCurrentRS->saTexBlend[i].dwColorArg1);
            D8CheckedSetTSS(i, D3DTSS_COLORARG2, m_psCurrentRS->saTexBlend[i].dwColorArg2);

            D8CheckedSetTSS(i, D3DTSS_ALPHAOP, m_psCurrentRS->saTexBlend[i].eAlphaOperation);
            D8CheckedSetTSS(i, D3DTSS_ALPHAARG1, m_psCurrentRS->saTexBlend[i].dwAlphaArg1);
            D8CheckedSetTSS(i, D3DTSS_ALPHAARG2, m_psCurrentRS->saTexBlend[i].dwAlphaArg2);
        }
        D8SetFVF(D8_MT_TLVERTEX_FVF);
        hR = g_lpD3D9Device->ValidateDevice(&dwPasses);
        if (hR != D3D_OK && !config_is_nonzero("d3d_disp_forcevalid")) {
            SetLGD3DErrorCode(LGD3D_EC_VD_SPASS_MT, hR);
            lgd3d_g_bInitialized = FALSE;
            return 0;
        }
    }
    if (dwRequestedFlags & LGD3DF_MT_BLENDDIFFUSE) {
        memcpy(m_psCurrentRS->saTexBlend, sMultiTexBlendArgsProtos[2], sizeof(m_psCurrentRS->saTexBlend));
        for (i = 0; i < 2; i++) {
#line 2990
            D8CheckedSetTSS(i, D3DTSS_TEXCOORDINDEX, i);

            D8CheckedSetTSS(i, D3DTSS_COLOROP, m_psCurrentRS->saTexBlend[i].eColorOperation);
            D8CheckedSetTSS(i, D3DTSS_COLORARG1, m_psCurrentRS->saTexBlend[i].dwColorArg1);
            D8CheckedSetTSS(i, D3DTSS_COLORARG2, m_psCurrentRS->saTexBlend[i].dwColorArg2);

            D8CheckedSetTSS(i, D3DTSS_ALPHAOP, m_psCurrentRS->saTexBlend[i].eAlphaOperation);
            D8CheckedSetTSS(i, D3DTSS_ALPHAARG1, m_psCurrentRS->saTexBlend[i].dwAlphaArg1);
            D8CheckedSetTSS(i, D3DTSS_ALPHAARG2, m_psCurrentRS->saTexBlend[i].dwAlphaArg2);
        }
        D8SetFVF(D8_MT_TLVERTEX_FVF);
        hR = g_lpD3D9Device->ValidateDevice(&dwPasses);
        if (hR != D3D_OK && !config_is_nonzero("d3d_disp_forcevalid"))
            return 0;
        m_psCurrentRS->dwTexBlendMode[1] = 2;
    }
    memcpy(m_psCurrentRS->saTexBlend, sTexBlendArgsProtos[m_psCurrentRS->dwTexBlendMode[0]],
           sizeof(m_psCurrentRS->saTexBlend));
    for (i = 0; i < 2; i++) {
#line 3022
        D8CheckedSetTSS(i, D3DTSS_TEXCOORDINDEX, i);

        D8CheckedSetTSS(i, D3DTSS_COLOROP, m_psCurrentRS->saTexBlend[i].eColorOperation);
        D8CheckedSetTSS(i, D3DTSS_COLORARG1, m_psCurrentRS->saTexBlend[i].dwColorArg1);
        D8CheckedSetTSS(i, D3DTSS_COLORARG2, m_psCurrentRS->saTexBlend[i].dwColorArg2);

        D8CheckedSetTSS(i, D3DTSS_ALPHAOP, m_psCurrentRS->saTexBlend[i].eAlphaOperation);
        D8CheckedSetTSS(i, D3DTSS_ALPHAARG1, m_psCurrentRS->saTexBlend[i].dwAlphaArg1);
        D8CheckedSetTSS(i, D3DTSS_ALPHAARG2, m_psCurrentRS->saTexBlend[i].dwAlphaArg2);

        D8CheckedSetSS(i, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
    }
    memcpy(m_psSetRS, m_psCurrentRS, sizeof(sRenderStates));
    g_dwRSChangeFlags = 0;
    return 1;
}

#define PF_ALPHA_TESTED (PF_TRANS | PF_ALPHA_1BIT)

int cD8MSStates::EnableMTMode(DWORD dwMTOn)
{
    DWORD dwLevel;
    BOOL bSetTexture;
    sTextureData* psTD;
    uchar pixel_flags;

    if (!m_bTexturePending) {
        if (m_psCurrentRS->naTextureId[0] != -1) {
            if (m_psCurrentRS->baTextureWithAlpha[0] || m_psCurrentRS->baTextureWithAlpha[1]) {
                if (!g_nAlphaTestMode) {
                    if (!m_psSetRS->bAlphaOn) {
#line 3232
                        D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, TRUE);
                        m_psSetRS->bAlphaOn = TRUE;
                    }
                } else if ((m_psCurrentRS->baTextureWithAlpha[0] &&
                            (g_saTextures[m_psCurrentRS->naTextureId[0]].TdrvCookie.flags & PF_ALPHA_TESTED)) ||
                           (m_psCurrentRS->baTextureWithAlpha[1] &&
                            (g_saTextures[m_psCurrentRS->naTextureId[1]].TdrvCookie.flags & PF_ALPHA_TESTED)))
                    UpdateAlphaTest(TRUE);
                else
                    UpdateAlphaTest(FALSE);
            } else {
                if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn) {
#line 3240
                    D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
                    m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
                }
                if (g_nAlphaTestMode && m_psSetRS->bAlphaTestOn) {
                    SetAlphaTestEnable(FALSE);
                    m_psSetRS->bAlphaTestOn = FALSE;
                }
            }
        }
        return TRUE;
    }

    m_bTexturePending = FALSE;
    bSetTexture = FALSE;
    if (dwMTOn) {
        m_psCurrentRS->dwNoLightMapLevels = 1;
        if (m_psCurrentRS->naTextureId[0] == -1) {
            Warning(("\nMulti texturing with no textures?!\n\n"));
            return FALSE;
        }
        if (m_psCurrentRS->naTextureId[1] == -1) {
            lgd3d_set_texture_level(1);
            if (g_tmgr)
                g_tmgr->set_texture(m_LastLightMapBm);
            lgd3d_set_texture_level(0);
        }
        if (m_psSetRS->naTextureId[0] != m_psCurrentRS->naTextureId[0] ||
            m_psSetRS->naTextureId[1] != m_psCurrentRS->naTextureId[1]) {
            DWORD dwAlphaTestMask = 0;
            bSetTexture = TRUE;
            for (dwLevel = 0; dwLevel < 2; dwLevel++) {
                if (m_psSetRS->naTextureId[dwLevel] != m_psCurrentRS->naTextureId[dwLevel]) {
                    psTD = &g_saTextures[m_psCurrentRS->naTextureId[dwLevel]];
                    m_psCurrentRS->baChromaKeying[dwLevel] = (psTD->TdrvCookie.flags & PF_TRANS) != 0;
                    if (m_psSetRS->baChromaKeying[dwLevel] != m_psCurrentRS->baChromaKeying[dwLevel]) {
                        m_psSetRS->baChromaKeying[dwLevel] = m_psCurrentRS->baChromaKeying[dwLevel];
                        if (dwLevel == 0) {
                            if (!m_psCurrentRS->baChromaKeying[0]) {
                                m_psSetRS->eMagTexFilter = m_psCurrentRS->eMagTexFilter = (D3DTEXTUREMINFILTER)g_dwTexMagFilter;
                                m_psSetRS->eMinTexFilter = m_psCurrentRS->eMinTexFilter = (D3DTEXTUREMINFILTER)g_dwTexMinFilter;
                            } else {
                                m_psSetRS->eMagTexFilter = m_psCurrentRS->eMagTexFilter = (D3DTEXTUREMINFILTER)D3DTEXF_POINT;
                                m_psSetRS->eMinTexFilter = m_psCurrentRS->eMinTexFilter = (D3DTEXTUREMINFILTER)D3DTEXF_POINT;
                            }
#line 3318
                            D8CheckedSetSS(0, D3DSAMP_MAGFILTER, m_psCurrentRS->eMagTexFilter);
                            D8CheckedSetSS(0, D3DSAMP_MINFILTER, m_psCurrentRS->eMinTexFilter);
                        }
                    }
                }
                if (g_saTextures[m_psCurrentRS->naTextureId[dwLevel]].TdrvCookie.flags & PF_ALPHA_TESTED)
                    dwAlphaTestMask |= 1 << dwLevel;
            }
            m_psCurrentRS->bAlphaTestOn = dwAlphaTestMask != 0;
            pixel_flags = g_saTextures[m_psCurrentRS->naTextureId[0]].TdrvCookie.flags & PF_FORMAT_MASK;
            m_psCurrentRS->baTextureWithAlpha[0] = PF_ALPHA_FORMAT(pixel_flags);
        }
        if (m_psSetRS->dwNoLightMapLevels != m_psCurrentRS->dwNoLightMapLevels ||
            m_psSetRS->dwTexBlendMode[1] != m_psCurrentRS->dwTexBlendMode[1] ||
            m_psSetRS->baTextureWithAlpha[0] != m_psCurrentRS->baTextureWithAlpha[0]) {
            memcpy(m_psCurrentRS->saTexBlend, sMultiTexBlendArgsProtos[m_psCurrentRS->dwTexBlendMode[1]],
                   sizeof(m_psCurrentRS->saTexBlend));
            m_psSetRS->dwTexBlendMode[1] = m_psCurrentRS->dwTexBlendMode[1];
            m_psSetRS->baTextureWithAlpha[0] = m_psCurrentRS->baTextureWithAlpha[0];
            bSetTexture = TRUE;
        }
        if (bSetTexture) {
            BOOL bAlphaTestChanged = FALSE;
            if (m_psSetRS->bAlphaTestOn != m_psCurrentRS->bAlphaTestOn) {
                SetAlphaTestEnable(m_psCurrentRS->bAlphaTestOn);
                m_psSetRS->bAlphaTestOn = m_psCurrentRS->bAlphaTestOn;
                bAlphaTestChanged = m_psSetRS->bAlphaTestOn;
            }
            if (!m_psSetRS->bAlphaTestOn || g_nAlphaTestMode == 2) {
                if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn) {
#line 3376
                    D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
                    m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
                    if (bAlphaTestChanged && m_psSetRS->bAlphaOn)
                        SetAlphaRef(m_psSetRS->nAlphaColor * g_nAlphaTestRef / 255);
                }
            } else if (m_psSetRS->bAlphaOn && !m_bAlphaModulated) {
#line 3383
                D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, FALSE);
                m_psSetRS->bAlphaOn = FALSE;
                if (bAlphaTestChanged)
                    SetAlphaRef(g_nAlphaTestRef);
            } else if (!g_nAlphaTestMode && m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn) {
#line 3390
                D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
                m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
            }
            for (dwLevel = 0; dwLevel < 2; dwLevel++) {
                D8SetTexBlend(dwLevel, m_psCurrentRS->saTexBlend, m_psSetRS->saTexBlend);
                SetTexture(dwLevel, (IDirect3DBaseTexture9*)g_saTextures[m_psCurrentRS->naTextureId[dwLevel]].lpTexture);
                m_psSetRS->naTextureId[dwLevel] = m_psCurrentRS->naTextureId[dwLevel];
            }
            psTD = &g_saTextures[m_psCurrentRS->naTextureId[0]];
            if (psTD->TdrvCookie.flags & PF_PALETTIZED) {
                HRESULT hr = g_lpD3D9Device->SetCurrentTexturePalette(psTD->TdrvCookie.palette);
#line 3411
                CheckHResult(hr, "SetCurrentTexturePalette failed");
            }
            m_psSetRS->dwNoLightMapLevels = m_psCurrentRS->dwNoLightMapLevels;
        }
        return TRUE;
    }

    m_psCurrentRS->dwNoLightMapLevels = 0;
    if (m_psSetRS->naTextureId[0] == m_psCurrentRS->naTextureId[0]) {
        if (m_psCurrentRS->baTextureWithAlpha[0]) {
            if (!g_nAlphaTestMode) {
                if (!m_psSetRS->bAlphaOn) {
#line 3430
                    D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, TRUE);
                    m_psSetRS->bAlphaOn = TRUE;
                }
            } else
                UpdateAlphaTest(TRUE);
        } else {
            if (g_nAlphaTestMode && m_psSetRS->bAlphaTestOn) {
                SetAlphaTestEnable(FALSE);
                m_psSetRS->bAlphaTestOn = FALSE;
            }
            if (!m_psSetRS->bAlphaTestOn || g_nAlphaTestMode == 2) {
                if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn) {
#line 3445
                    D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
                    m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
                }
            } else if (m_psSetRS->bAlphaOn && !m_bAlphaModulated) {
#line 3451
                D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, FALSE);
                m_psSetRS->bAlphaOn = FALSE;
            }
        }
    } else if (m_psCurrentRS->naTextureId[0] == -1) {
        SetTexture(0, NULL);
        m_psSetRS->naTextureId[0] = -1;
        m_psCurrentRS->baTextureWithAlpha[0] = FALSE;
        if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn) {
#line 3467
            D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
            m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
        }
        if (g_nAlphaTestMode && m_psSetRS->bAlphaTestOn) {
            SetAlphaTestEnable(FALSE);
            m_psSetRS->bAlphaTestOn = FALSE;
        }
    } else {
        bSetTexture = TRUE;
        psTD = &g_saTextures[m_psCurrentRS->naTextureId[0]];
        pixel_flags = psTD->TdrvCookie.flags;
        m_psCurrentRS->baChromaKeying[0] = (pixel_flags & PF_TRANS) != 0;
        if (!m_psCurrentRS->baChromaKeying[0]) {
            m_psCurrentRS->eMagTexFilter = (D3DTEXTUREMINFILTER)g_dwTexMagFilter;
            m_psCurrentRS->eMinTexFilter = (D3DTEXTUREMINFILTER)g_dwTexMinFilter;
        } else {
            m_psCurrentRS->eMagTexFilter = (D3DTEXTUREMINFILTER)D3DTEXF_POINT;
            m_psCurrentRS->eMinTexFilter = (D3DTEXTUREMINFILTER)D3DTEXF_POINT;
        }
        if (m_psSetRS->baChromaKeying[0] != m_psCurrentRS->baChromaKeying[0]) {
            m_psSetRS->baChromaKeying[0] = m_psCurrentRS->baChromaKeying[0];
#line 3502
            D8CheckedSetSS(0, D3DSAMP_MAGFILTER, m_psCurrentRS->eMagTexFilter);
            m_psSetRS->eMagTexFilter = m_psCurrentRS->eMagTexFilter;
#line 3505
            D8CheckedSetSS(0, D3DSAMP_MINFILTER, m_psCurrentRS->eMinTexFilter);
            m_psSetRS->eMinTexFilter = m_psCurrentRS->eMinTexFilter;
        }
        pixel_flags &= PF_FORMAT_MASK;
        m_psCurrentRS->baTextureWithAlpha[0] = PF_ALPHA_FORMAT(pixel_flags);
        if (m_psCurrentRS->baTextureWithAlpha[0]) {
            if (!g_nAlphaTestMode) {
                if (!m_psSetRS->bAlphaOn) {
#line 3518
                    D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, TRUE);
                    m_psSetRS->bAlphaOn = TRUE;
                }
            }
        } else if (m_psSetRS->bAlphaOn != m_psCurrentRS->bAlphaOn) {
#line 3526
            D8CheckedSetRS(D3DRS_ALPHABLENDENABLE, m_psCurrentRS->bAlphaOn);
            m_psSetRS->bAlphaOn = m_psCurrentRS->bAlphaOn;
        }
        if (g_nAlphaTestMode && m_psSetRS->bAlphaTestOn && !m_psCurrentRS->baTextureWithAlpha[0]) {
            SetAlphaTestEnable(FALSE);
            m_psSetRS->bAlphaTestOn = FALSE;
        }
    }
    if (m_psSetRS->dwNoLightMapLevels != m_psCurrentRS->dwNoLightMapLevels ||
        m_psSetRS->dwTexBlendMode[0] != m_psCurrentRS->dwTexBlendMode[0] ||
        m_psSetRS->baTextureWithAlpha[0] != m_psCurrentRS->baTextureWithAlpha[0]) {
        memcpy(m_psCurrentRS->saTexBlend, sTexBlendArgsProtos[m_psCurrentRS->dwTexBlendMode[0]],
               sizeof(m_psCurrentRS->saTexBlend));
        if (m_psCurrentRS->dwTexBlendMode[0] == 0 && m_psCurrentRS->baTextureWithAlpha[0])
            m_psCurrentRS->saTexBlend[0].eAlphaOperation = (D3DTEXTUREOP)D3DTOP_SELECTARG1;
        m_psSetRS->dwTexBlendMode[0] = m_psCurrentRS->dwTexBlendMode[0];
        m_psSetRS->baTextureWithAlpha[0] = m_psCurrentRS->baTextureWithAlpha[0];
        bSetTexture = TRUE;
    }
    if (bSetTexture) {
        for (dwLevel = 0; dwLevel < 2; dwLevel++)
            D8SetTexBlend(dwLevel, m_psCurrentRS->saTexBlend, m_psSetRS->saTexBlend);
        m_psSetRS->dwNoLightMapLevels = m_psCurrentRS->dwNoLightMapLevels;
        if (m_psCurrentRS->naTextureId[0] != -1) {
            psTD = &g_saTextures[m_psCurrentRS->naTextureId[0]];
            SetTexture(0, (IDirect3DBaseTexture9*)psTD->lpTexture);
            m_psSetRS->naTextureId[0] = m_psCurrentRS->naTextureId[0];
            if (psTD->TdrvCookie.flags & PF_PALETTIZED) {
                HRESULT hr = g_lpD3D9Device->SetCurrentTexturePalette(psTD->TdrvCookie.palette);
#line 3582
                CheckHResult(hr, "SetCurrentTexturePalette failed");
            }
            m_psCurrentRS->naTextureId[1] = -1;
            SetTexture(1, NULL);
            m_psSetRS->naTextureId[1] = m_psCurrentRS->naTextureId[1];
        }
    }
    return TRUE;
}

void cD8MSBatchStates::SetAlphaColor(float alpha)
{
    int val = (int)(alpha * 255);
    if ((unsigned)val > 0xFF)
        val = val < 0 ? 0 : 0xFF;
    if (m_psCurrentRS->nAlphaColor != val) {
        if (pcRenderBuffer->m_bPrimitivesPending && (val == 0xFF || m_psCurrentRS->nAlphaColor == 0xFF))
            pcRenderBuffer->FlushPrimitives();
        m_psCurrentRS->nAlphaColor = val;
        m_psSetRS->nAlphaColor = val;
        pcRenderBuffer->PassAlphaColor(val);
        if (m_psSetRS->bAlphaTestOn) {
            int nRef = g_nAlphaTestRef;
            if (m_psSetRS->bAlphaOn)
                nRef = m_psSetRS->nAlphaColor * g_nAlphaTestRef / 255;
            if (nRef != m_nCurAlphaRef) {
                pcRenderBuffer->FlushPrimitives();
                SetAlphaRef(nRef);
            }
        }
    }
}

void cD8MSBatchStates::set_texture_id(int n)
{
    if (n == -2) {
        g_tmgr->set_texture_callback();
        return;
    }
    if (!g_nD8ActiveEffect) {
        if (!m_bBatching) {
            if (m_dwCurrentTexLevel == 0)
                pcRenderBuffer->m_iSavedTexId = n;
            cD8MSStates::set_texture_id(n);
            return;
        }
        if (m_dwCurrentTexLevel == 0) {
            pcRenderBuffer->m_iSavedTexId = n;
            cD8MSBatchBuffer::SelectTLBatch(n);
            return;
        }
        if (m_dwCurrentTexLevel == 1)
            m_LastLightMapBm = g_saTextures[n].pTdrvBitmap;
        ((cD8MSBatchBuffer*)pcRenderBuffer)->SelectMTBatch(1, n);
    } else if (g_bD8EffectTexture && !PUNT_D3D) {
        pcRenderBuffer->FlushIndPolies();
        HRESULT hr;
        if (n == -1) {
            hr = g_lpD3D9Device->SetTexture(m_dwCurrentTexLevel, NULL);
#line 3721
            CheckHResult(hr, "SetTexture NULL failed for effect");
        } else {
            hr = g_lpD3D9Device->SetTexture(m_dwCurrentTexLevel, (IDirect3DBaseTexture9*)g_saTextures[n].lpTexture);
#line 3717
            CheckHResult(hr, "SetTexture failed for effect");
        }
    }
}
