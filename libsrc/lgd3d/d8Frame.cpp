#include <windows.h>
extern "C" {
#include <g2.h>
}
#include <r3d.h>
#include <d8dx6.h>
#include <lgd3d.h>
#include <d8states.h>
#include <d8Effect.h>
#include <stdio.h>
#include <string.h>
#include <lgassert.h>
#include <d6check.h>
#include <appagg.h>
#include <wdispapi.h>
#include <config.h>
#include <dbg.h>
#include <tmgr.h>
#include <math.h>

IDirect3D9* g_lpD3D9;
IDirect3DDevice9* g_lpD3D9Device;
IDirect3DSurface9* g_lpSceneSurface;
IDirect3DTexture9* g_lpSceneTextures[kD8NumSceneTextures];
D3DSURFACE_DESC g_sSceneTextureDesc[kD8NumSceneTextures];
D3DSURFACE_DESC g_sBackBufferDesc;
D3DCAPS9 g_sD3D9Caps;
DWORD g_dwD8Flags;
DWORD g_dwD8CurrentFVF;
BOOL g_bD8ManagedTextures;
int g_bD8ScaleRects;
float g_fD8RectScaleX = 1.0f;
float g_fD8RectScaleY = 1.0f;
float g_fD8ScreenScaleX = 1.0f;
float g_fD8ScreenScaleY = 1.0f;
float g_fD8TexelCenterX;
float g_fD8TexelCenterY;

static cD8Effect_ScreenDistort g_DistortLow(kD8SceneLow);
static cD8Effect_ScreenDistort g_DistortFull(kD8SceneFull);
static cD8Effect_ScreenDistort g_DistortBlur(kD8SceneBlur);
static cD8Effect_FakeDOF g_FakeDOF;
cD8Effect_Blur2pass g_Blur2pass;
cD8Effect_PostProcess g_PostProcess;
cD8Effect_ColorCorrection g_ColorCorrection;

extern "C" const GUID IID_IDirect3D9 = { 0x81bdcbca, 0x64d4, 0x426d, { 0xae, 0x8d, 0xad, 0x01, 0x47, 0xf4, 0x27, 0x5c } };
extern "C" const GUID IID_IDirect3DTexture9 = { 0x85c31227, 0x3de5, 0x4f00, { 0x9b, 0x3a, 0xf1, 0x1a, 0xc3, 0x8c, 0x18, 0xb5 } };

cD8Effect* g_apD8Effects[] = {
    &g_DistortLow,
    &g_DistortFull,
    &g_DistortBlur,
    &g_FakeDOF,
    &g_Blur2pass,
    &g_PostProcess,
    &g_ColorCorrection,
    NULL
};

#define D8_BLUR_RADIUS  6
#define D8_BLUR_TAPS    (D8_BLUR_RADIUS * 2 + 1)
#define D8_MAX_BLUR_PASSES 16
#define D8_NUM_EFFECT_SLOTS 5

float g_fD8BloomThreshold = 0.6f;
float g_fD8BloomFalloff = 1.0f;
float g_fD8BloomIntensity = 5.0f;
float g_fD8BloomRadius = 2.0f;
float g_fD8BloomBrightScale = 0.7f;
BOOL g_bD8PostProcess;

static int s_nD8SavedTextureLevel;
static r3s_texture s_pD8SavedTexture;

static void D8SetBlurOffsets(BOOL bVertical, double fStep)
{
    float aOffsets[D8_BLUR_TAPS][4];
    for (int i = 0; i < D8_BLUR_TAPS; i++) {
        float fOffset = (float)((i - D8_BLUR_RADIUS) * fStep);
        aOffsets[i][0] = bVertical ? 0.0f : fOffset;
        aOffsets[i][1] = bVertical ? fOffset : 0.0f;
        aOffsets[i][2] = 0.0f;
        aOffsets[i][3] = 0.0f;
    }
    g_lpD3D9Device->SetPixelShaderConstantF(0, aOffsets[0], D8_BLUR_TAPS);
}

static void D8SetBlurWeights(double fSigma, double fTotal)
{
    float aWeights[D8_BLUR_TAPS][4];
    float afHalf[D8_BLUR_RADIUS + 1];
    double fMean = 0.0;
    double fMeanSq = fMean * fMean;
    double fDenom = -2.0 * fSigma * fSigma;
    double fNorm = 1.0 / sqrt(2.0 * 3.141592653589793 * fSigma * fSigma);
    int i;

    float fSum = (float)(exp((fMeanSq + fMeanSq) / fDenom) * fNorm);
    afHalf[D8_BLUR_RADIUS] = fSum;
    for (i = D8_BLUR_RADIUS; i > 0; i--) {
        float fWeight = (float)(exp((i * i + fMeanSq) / fDenom) * fNorm);
        afHalf[D8_BLUR_RADIUS - i] = fWeight;
        fSum += fWeight + fWeight;
    }
    fSum = (float)(fTotal / fSum);
    for (i = 0; i <= D8_BLUR_RADIUS; i++)
        afHalf[i] *= fSum;
    for (i = 0; i < D8_BLUR_TAPS; i++) {
        float fWeight = afHalf[i <= D8_BLUR_RADIUS ? i : D8_BLUR_TAPS - 1 - i];
        aWeights[i][0] = fWeight;
        aWeights[i][1] = fWeight;
        aWeights[i][2] = fWeight;
        aWeights[i][3] = 0.0f;
    }
    g_lpD3D9Device->SetPixelShaderConstantF(D8_BLUR_TAPS, aWeights[0], D8_BLUR_TAPS);
}

static void D8SetBlurConstants(BOOL bVertical, UINT nWidth, UINT nHeight, double fSigma, double fScale)
{
    D8SetBlurOffsets(bVertical, fScale / (bVertical ? nHeight : nWidth));
    D8SetBlurWeights(fSigma, 1.0);
}

static void D8SetBloomBlurConstants(BOOL bVertical, UINT nWidth, UINT nHeight, double fSigma, double fIntensity)
{
    D8SetBlurOffsets(bVertical, 1.0 / (bVertical ? nHeight : nWidth));
    D8SetBlurWeights(fSigma, fIntensity);
}

static float D8BlurSigma(float fRadius)
{
    if (fRadius >= 6.0f)
        return 3.0f;
    float fSigma = (float)((fRadius - 3.0) / 3.0 * 2.67 + 0.33);
    if (fSigma <= 0.33)
        return 0.33f;
    return fSigma;
}

static void D8InitScreenQuad(_D3DTLVERTEX* pQuad)
{
    memset(pQuad, 0, 4 * sizeof(_D3DTLVERTEX));
    pQuad[0].sx = pQuad[3].sx = -g_fD8TexelCenterX;
    pQuad[0].sy = pQuad[1].sy = -g_fD8TexelCenterY;
    for (int i = 0; i < 4; i++)
        pQuad[i].rhw = 1.0f;
    pQuad[1].tu = pQuad[2].tu = 1.0f;
    pQuad[2].tv = pQuad[3].tv = 1.0f;
}

static void D8SetScreenQuadSize(_D3DTLVERTEX* pQuad, UINT nWidth, UINT nHeight)
{
    D3DVIEWPORT9 vp;
    vp.X = 0;
    vp.Y = 0;
    vp.Width = nWidth;
    vp.Height = nHeight;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    g_lpD3D9Device->SetViewport(&vp);
    pQuad[1].sx = pQuad[2].sx = (float)nWidth - g_fD8TexelCenterX;
    pQuad[2].sy = pQuad[3].sy = (float)nHeight - g_fD8TexelCenterY;
}

BOOL D8_RenderBloom(IDirect3DTexture9* pSource, IDirect3DTexture9* pTemp, IDirect3DTexture9* pTarget)
{
    D3DSURFACE_DESC srcDesc, tempDesc, targetDesc;
    _D3DTLVERTEX quad[4];
    IDirect3DSurface9* pOldTarget = NULL;
    IDirect3DSurface9* pSurface = NULL;
    BOOL bOk = FALSE;

    if (!g_Blur2pass.Activate())
        return FALSE;
    if (FAILED(pSource->GetLevelDesc(0, &srcDesc)) || FAILED(pTemp->GetLevelDesc(0, &tempDesc))
        || FAILED(pTarget->GetLevelDesc(0, &targetDesc))) {
        g_Blur2pass.Deactivate();
        return FALSE;
    }
    D8InitScreenQuad(quad);
    g_lpD3D9Device->GetRenderTarget(0, &pOldTarget);
    bOk = TRUE;

    UINT nWidth = srcDesc.Width;
    UINT nHeight = srcDesc.Height;
    float fRadius = (float)(sqrt((double)(nWidth * nWidth + nHeight * nHeight)) * (g_Blur2pass.m_fBlurAmount / 100.0));
    for (int i = 0; i < D8_MAX_BLUR_PASSES && nWidth && nHeight; i++) {
        float fSigma = D8BlurSigma(fRadius);

        if (FAILED(pTemp->GetSurfaceLevel(0, &pSurface))) {
            bOk = FALSE;
            break;
        }
        D8SetBlurConstants(FALSE, nWidth, nHeight, fSigma, 1.0);
        g_lpD3D9Device->SetTexture(0, pSource);
        HRESULT hr = g_lpD3D9Device->SetRenderTarget(0, pSurface);
        if (pSurface)
            pSurface->Release();
        pSurface = NULL;
        if (FAILED(hr)) {
            bOk = FALSE;
            break;
        }
        if (i == 0)
            D8SetScreenQuadSize(quad, tempDesc.Width, tempDesc.Height);
        g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, quad, sizeof(_D3DTLVERTEX));
        g_lpD3D9Device->SetTexture(0, NULL);
        g_lpD3D9Device->SetRenderTarget(0, NULL);

        if (FAILED(pTarget->GetSurfaceLevel(0, &pSurface))) {
            bOk = FALSE;
            break;
        }
        D8SetBlurConstants(TRUE, nWidth, nHeight, fSigma, 1.0);
        g_lpD3D9Device->SetTexture(0, pTemp);
        hr = g_lpD3D9Device->SetRenderTarget(0, pSurface);
        if (pSurface)
            pSurface->Release();
        pSurface = NULL;
        if (FAILED(hr)) {
            bOk = FALSE;
            break;
        }
        if (i == 0)
            D8SetScreenQuadSize(quad, targetDesc.Width, targetDesc.Height);
        g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, quad, sizeof(_D3DTLVERTEX));
        g_lpD3D9Device->SetTexture(0, NULL);
        g_lpD3D9Device->SetRenderTarget(0, NULL);
        if (i == 0) {
            srcDesc = targetDesc;
            pSource = pTarget;
        }
        if (fRadius <= 6.0f)
            break;
        fRadius *= 0.5;
        nWidth >>= 1;
        nHeight >>= 1;
    }
    g_lpD3D9Device->SetRenderTarget(0, pOldTarget);
    if (pOldTarget)
        pOldTarget->Release();
    pOldTarget = NULL;
    g_Blur2pass.Deactivate();
    return bOk;
}

static void D8SetBrightPassConstants(UINT nWidth, UINT nHeight)
{
    float c[4][4];
    c[0][0] = c[0][1] = c[0][2] = c[0][3] = g_fD8BloomThreshold;
    g_lpD3D9Device->SetPixelShaderConstantF(0, c[0], 1);
    c[0][0] = c[0][1] = c[0][2] = c[0][3] = g_fD8BloomBrightScale;
    g_lpD3D9Device->SetPixelShaderConstantF(1, c[0], 1);
    float dx = 0.5f / nWidth;
    float dy = 0.5f / nHeight;
    c[0][0] = -dx; c[0][1] = -dy; c[0][2] = 0.0f; c[0][3] = 0.0f;
    c[1][0] = dx;  c[1][1] = -dy; c[1][2] = 0.0f; c[1][3] = 0.0f;
    c[2][0] = -dx; c[2][1] = dy;  c[2][2] = 0.0f; c[2][3] = 0.0f;
    c[3][0] = dx;  c[3][1] = dy;  c[3][2] = 0.0f; c[3][3] = 0.0f;
    g_lpD3D9Device->SetPixelShaderConstantF(2, c[0], 4);
}

static BOOL D8BloomBlurPass(IDirect3DTexture9* pTexture, IDirect3DTexture9* pTemp, _D3DTLVERTEX* pQuad, float fSigma,
                            UINT nWidth, UINT nHeight, float fIntensity)
{
    IDirect3DSurface9* pSurface = NULL;
    if (FAILED(pTemp->GetSurfaceLevel(0, &pSurface)))
        return FALSE;
    HRESULT hr = g_lpD3D9Device->SetRenderTarget(0, pSurface);
    if (pSurface)
        pSurface->Release();
    if (FAILED(hr)) {
        g_lpD3D9Device->SetTexture(0, NULL);
        return FALSE;
    }
    D8SetBloomBlurConstants(FALSE, nWidth, nHeight, fSigma, fIntensity);
    g_lpD3D9Device->SetTexture(0, pTexture);
    g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, pQuad, sizeof(_D3DTLVERTEX));
    g_lpD3D9Device->SetTexture(0, NULL);
    g_lpD3D9Device->SetRenderTarget(0, NULL);

    if (FAILED(pTexture->GetSurfaceLevel(0, &pSurface)))
        return FALSE;
    hr = g_lpD3D9Device->SetRenderTarget(0, pSurface);
    if (pSurface)
        pSurface->Release();
    if (FAILED(hr)) {
        g_lpD3D9Device->SetTexture(0, NULL);
        return FALSE;
    }
    D8SetBloomBlurConstants(TRUE, nWidth, nHeight, fSigma, 1.0);
    g_lpD3D9Device->SetTexture(0, pTemp);
    g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, pQuad, sizeof(_D3DTLVERTEX));
    g_lpD3D9Device->SetTexture(0, NULL);
    g_lpD3D9Device->SetRenderTarget(0, NULL);
    return TRUE;
}

static BOOL D8RenderPostProcess(IDirect3DTexture9* pScene)
{
    D3DSURFACE_DESC sceneDesc, midDesc, targetDesc;
    _D3DTLVERTEX quad[4];
    IDirect3DSurface9* pTarget = NULL;
    IDirect3DSurface9* pSurface = NULL;
    IDirect3DTexture9* pMid = g_lpSceneTextures[kD8SceneMid];

    if (!g_PostProcess.Activate())
        return FALSE;
    if (FAILED(pScene->GetLevelDesc(0, &sceneDesc)) || FAILED(pMid->GetLevelDesc(0, &midDesc))) {
        g_PostProcess.Deactivate();
        return FALSE;
    }
    g_lpD3D9Device->GetRenderTarget(0, &pTarget);
    if (!pTarget || FAILED(pTarget->GetDesc(&targetDesc))) {
        if (pTarget)
            pTarget->Release();
        pTarget = NULL;
        g_PostProcess.Deactivate();
        return FALSE;
    }
    D8InitScreenQuad(quad);

    if (g_fD8BloomIntensity > 0.0f) {
        if (FAILED(pMid->GetSurfaceLevel(0, &pSurface)))
            goto fail;
        UINT nWidth = midDesc.Width;
        UINT nHeight = midDesc.Height;
        D8SetScreenQuadSize(quad, nWidth, nHeight);
        D8SetBrightPassConstants(sceneDesc.Width, sceneDesc.Height);
        g_lpD3D9Device->SetTexture(0, pScene);
        HRESULT hr = g_lpD3D9Device->SetRenderTarget(0, pSurface);
        if (pSurface)
            pSurface->Release();
        pSurface = NULL;
        if (FAILED(hr))
            goto fail;
        g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, quad, sizeof(_D3DTLVERTEX));
        g_lpD3D9Device->SetTexture(0, NULL);
        g_lpD3D9Device->SetRenderTarget(0, pTarget);

        float fRadius = (float)(sqrt((double)(nWidth * nWidth + nHeight * nHeight)) * (g_fD8BloomRadius / 100.0));
        g_lpD3D9Device->SetPixelShader(g_PostProcess.m_pBlur);
        for (int i = 0; i < D8_MAX_BLUR_PASSES && nWidth && nHeight; i++) {
            float fSigma = D8BlurSigma(fRadius);
            float fIntensity = i ? g_fD8BloomFalloff : 1.0f;
            if (!D8BloomBlurPass(pMid, g_lpSceneTextures[kD8SceneMidTemp], quad, fSigma, nWidth, nHeight, fIntensity)) {
                g_lpD3D9Device->SetRenderTarget(0, pTarget);
                goto fail;
            }
            if (fRadius <= 6.0f)
                break;
            fRadius *= 0.5;
            nWidth >>= 1;
            nHeight >>= 1;
        }
        g_lpD3D9Device->SetRenderTarget(0, pTarget);
    }
    if (pTarget)
        pTarget->Release();
    pTarget = NULL;

    {
        int nCombine;
        if (g_fD8BloomIntensity > 0.0f)
            nCombine = g_PostProcess.m_fGamma == 1.0f ? 1 : 2;
        else
            nCombine = g_PostProcess.m_fGamma == 1.0f ? 1 : 0;
        g_lpD3D9Device->SetPixelShader(g_PostProcess.m_pCombine[nCombine]);

        float c[4];
        if (g_PostProcess.m_fGamma != 1.0f) {
            c[0] = c[1] = c[2] = c[3] = g_PostProcess.m_fGamma;
            g_lpD3D9Device->SetPixelShaderConstantF(1, c, 1);
        } else {
            c[0] = c[1] = c[2] = c[3] = g_fD8BloomIntensity;
            g_lpD3D9Device->SetPixelShaderConstantF(0, c, 1);
        }
    }
    D8SetScreenQuadSize(quad, targetDesc.Width, targetDesc.Height);
    g_lpD3D9Device->SetTexture(0, pScene);
    g_lpD3D9Device->SetTexture(1, g_fD8BloomIntensity > 0.0f ? pMid : NULL);
    g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, quad, sizeof(_D3DTLVERTEX));
    g_lpD3D9Device->SetTexture(0, NULL);
    g_lpD3D9Device->SetTexture(1, NULL);
    g_PostProcess.Deactivate();
    return TRUE;

fail:
    if (pTarget)
        pTarget->Release();
    pTarget = NULL;
    g_PostProcess.Deactivate();
    return FALSE;
}

BOOL D8_ColorCorrectionActive()
{
    return g_ColorCorrection.IsActive();
}

static void D8SetColorCorrectionParams(UINT nWidth, UINT nHeight)
{
    static int s_nNoiseFrame;
    ID3DXEffect* pEffect = g_ColorCorrection.m_pEffect;
    if (!pEffect)
        return;
    float afFilter[4];
    afFilter[0] = g_ColorCorrection.m_fColorFilter[0];
    afFilter[1] = g_ColorCorrection.m_fColorFilter[1];
    afFilter[2] = g_ColorCorrection.m_fColorFilter[2];
    afFilter[3] = 1.0f;
    float afScreen[2];
    afScreen[0] = (float)nWidth;
    afScreen[1] = (float)nHeight;
    pEffect->SetFloat("g_fGamma", g_ColorCorrection.m_fGamma);
    pEffect->SetFloat("g_fBrightness", g_ColorCorrection.m_fBrightness);
    pEffect->SetFloat("g_fContrast", g_ColorCorrection.m_fContrast);
    pEffect->SetFloat("g_fSaturation", g_ColorCorrection.m_fSaturation);
    pEffect->SetFloatArray("g_fColorFilter", afFilter, 4);
    pEffect->SetFloatArray("g_fScreenSize", afScreen, 2);
    s_nNoiseFrame = (s_nNoiseFrame + 1) % 60;
    pEffect->SetFloat("g_fNoiseTimer", (float)s_nNoiseFrame / 60.0f);
}

static const char* D8ColorCorrectionTechnique()
{
    const char* apszTechniques[8] = {
        "TeqCopy", "TeqGamma", "TeqSat", "TeqSatGamma", "TeqBr", "TeqBrGamma", "TeqBrSat", "TeqBrSatGamma"
    };
    int n = g_ColorCorrection.m_fGamma != 1.0f;
    if (g_ColorCorrection.m_fSaturation != 1.0f || g_ColorCorrection.m_fColorFilter[0] != 1.0f
        || g_ColorCorrection.m_fColorFilter[1] != 1.0f || g_ColorCorrection.m_fColorFilter[2] != 1.0f)
        n |= 2;
    if (g_ColorCorrection.m_fBrightness != 0.0f || g_ColorCorrection.m_fContrast != 1.0f)
        n |= 4;
    return apszTechniques[n];
}

BOOL D8_ApplyColorCorrection(IDirect3DSurface9* pSource)
{
    IDirect3DSurface9* pTarget = NULL;
    D3DSURFACE_DESC desc;
    _D3DTLVERTEX quad[4];
    UINT nPasses;

    g_lpD3D9Device->GetRenderTarget(0, &pTarget);
    if (!pTarget) {
        g_ColorCorrection.Deactivate();
        return FALSE;
    }
    if (!D8_ColorCorrectionActive()) {
        if (pSource != pTarget) {
            HRESULT hr = g_lpD3D9Device->StretchRect(pSource, NULL, pTarget, NULL, D3DTEXF_POINT);
            if (pTarget)
                pTarget->Release();
            return hr == S_OK;
        }
        if (pTarget)
            pTarget->Release();
        return TRUE;
    }
    if (!((cD8Renderer*)pcRenderer)->UpdateSceneTexture(D8_SCENE_BIT(kD8SceneFull), pSource, TRUE) || !g_ColorCorrection.Activate()) {
        if (pTarget)
            pTarget->Release();
        return FALSE;
    }
    if (FAILED(pTarget->GetDesc(&desc)) || FAILED(g_lpD3D9Device->BeginScene()))
        goto fail;
    {
        ID3DXEffect* pEffect = g_ColorCorrection.m_pEffect;
        if (FAILED(pEffect->SetTechnique(D8ColorCorrectionTechnique()))) {
            g_lpD3D9Device->EndScene();
            goto fail;
        }
        D8SetColorCorrectionParams(desc.Width, desc.Height);
        g_lpD3D9Device->SetTexture(0, g_lpSceneTextures[kD8SceneFull]);
        if (FAILED(pEffect->Begin(&nPasses, 0))) {
            g_lpD3D9Device->EndScene();
            goto fail;
        }
        D8InitScreenQuad(quad);
        D8SetScreenQuadSize(quad, desc.Width, desc.Height);
        for (UINT i = 0; i < nPasses; i++) {
            if (SUCCEEDED(pEffect->BeginPass(i))) {
                pEffect->CommitChanges();
                g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, quad, sizeof(_D3DTLVERTEX));
            }
            pEffect->EndPass();
        }
        pEffect->End();
        g_lpD3D9Device->EndScene();
        if (pTarget)
            pTarget->Release();
        g_ColorCorrection.Deactivate();
        return TRUE;
    }
fail:
    if (pTarget)
        pTarget->Release();
    pTarget = NULL;
    g_ColorCorrection.Deactivate();
    return FALSE;
}

BOOL D8_PostProcessEnabled()
{
    if (!g_lpD3D9Device)
        return FALSE;
    return (g_PostProcess.m_dwFlags & D8EF_CREATED) ? g_bD8PostProcess : FALSE;
}

BOOL D8_BloomEnabled()
{
    return g_lpD3D9Device && (g_Blur2pass.m_dwFlags & D8EF_CREATED);
}

BOOL D8_RunPostProcess(IDirect3DTexture9* pScene)
{
    if (!g_lpD3D9Device || !(g_PostProcess.m_dwFlags & D8EF_CREATED))
        return FALSE;
    BOOL bOk = FALSE;
    if (SUCCEEDED(g_lpD3D9Device->BeginScene())) {
        bOk = D8RenderPostProcess(pScene);
        g_lpD3D9Device->EndScene();
    }
    return bOk;
}

extern "C" BOOL lgd3d_set_effect_param(int nEffect, int nParam, const void* pValue)
{
    if (g_lpD3D9Device && pcStates && (g_apD8Effects[nEffect - 1]->m_dwFlags & D8EF_CREATED))
        return g_apD8Effects[nEffect - 1]->SetParam(nParam, pValue);
    return FALSE;
}

extern "C" BOOL lgd3d_get_effect_param(int nEffect, int nParam, void* pValue)
{
    if (g_lpD3D9Device && pcStates && (g_apD8Effects[nEffect - 1]->m_dwFlags & D8EF_CREATED))
        return g_apD8Effects[nEffect - 1]->GetParam(nParam, pValue);
    return FALSE;
}

extern "C" BOOL lgd3d_begin_effect(int nEffect)
{
    if (!g_lpD3D9Device || !pcRenderer || !nEffect || nEffect >= D8_NUM_EFFECT_SLOTS || g_nD8ActiveEffect)
        return FALSE;
    if (!(g_apD8Effects[nEffect - 1]->m_dwFlags & D8EF_CREATED))
        return FALSE;
    pcRenderBuffer->FlushPrimitives();
    s_nD8SavedTextureLevel = pcStates->GetTextureLevel();
    s_pD8SavedTexture = r3d_state.cur_con ? r3d_state.cur_con->prim.bm : NULL;
    if (!g_apD8Effects[nEffect - 1]->Activate())
        return FALSE;
    g_nD8ActiveEffect = nEffect;
    return TRUE;
}

extern "C" void lgd3d_end_effect()
{
    if (!g_lpD3D9Device || !pcRenderer || !g_nD8ActiveEffect)
        return;
    pcRenderBuffer->FlushPrimitives();
    g_apD8Effects[g_nD8ActiveEffect - 1]->Deactivate();
    g_nD8ActiveEffect = 0;
    pcStates->SetTextureLevel(s_nD8SavedTextureLevel);
    if (r3d_state.cur_con)
        r3d_state.cur_con->prim.bm = s_pD8SavedTexture;
    g_dwD8CurrentFVF = 0;
}

extern "C" void lgd3d_run_effects(int* pEffects, int nEffects)
{
    for (int i = 0; i < nEffects; i++) {
        if (lgd3d_begin_effect(pEffects[i]))
            lgd3d_end_effect();
    }
}

#define D8_COLOR_CORRECTION_EFFECT 7

void D8_FlushPostProcessing(BOOL bColorCorrect)
{
    if (!g_lpD3D9Device || !pcRenderBuffer)
        return;
    pcRenderBuffer->FlushPrimitives();
    if (bColorCorrect && D8_ColorCorrectionActive()) {
        int nEffect = D8_COLOR_CORRECTION_EFFECT;
        lgd3d_run_effects(&nEffect, 1);
    }
}

extern "C" void lgd3d_set_effect_texture(grs_bitmap* pBm)
{
    if (!g_lpD3D9Device || !pcStates || !g_nD8ActiveEffect)
        return;
    g_bD8EffectTexture = TRUE;
    if (lgd3d_g_bInitialized)
        pcStates->SetTextureLevel(0);
    if (g_tmgr)
        g_tmgr->set_texture(pBm);
    g_bD8EffectTexture = FALSE;
}

cD6Frame* D8_CreateFrame(unsigned long dwWidth, unsigned long dwHeight, lgd3ds_device_info* psDeviceInfo)
{
    return new cD8Frame(dwWidth, dwHeight, psDeviceInfo);
}

cD8Frame::cD8Frame(unsigned long dwWidth, unsigned long dwHeight, lgd3ds_device_info* psDeviceInfo)
{
    InitializeGlobals(dwWidth, dwHeight, psDeviceInfo->flags);
    if (GetDDstuffFromDisplay() >= 0) {
        InitializeEnvironment(psDeviceInfo);
    } else {
        SetLGD3DErrorCode(LGD3D_EC_DD_KAPUT, 0);
        lgd3d_g_bInitialized = 0;
    }
}

cD8Frame::~cD8Frame()
{
    for (int i = 0; g_apD8Effects[i]; i++) {
        cD8Effect* pEffect = g_apD8Effects[i];
        if (pEffect->m_dwFlags & D8EF_CREATED) {
            if (pEffect->m_pState) {
                delete pEffect->m_pState;
                pEffect->m_pState = NULL;
            }
            pEffect->Term();
            g_apD8Effects[i]->m_dwFlags &= ~(D8EF_CREATED | D8EF_ACTIVE);
        }
    }
    SafeRelease(g_lpSceneSurface);
    for (int j = 0; j < kD8NumSceneTextures; j++)
        SafeRelease(g_lpSceneTextures[j]);
    SafeRelease(g_lpD3D9Device);
    SafeRelease(g_lpD3D9);
    g_fD8ScreenScaleX = 1.0f;
    g_fD8ScreenScaleY = 1.0f;
}

void cD8Frame::InitializeGlobals(unsigned long dwWidth, unsigned long dwHeight, unsigned long dwRequestedFlags)
{
    cD6Frame::InitializeGlobals(dwWidth, dwHeight, dwRequestedFlags);

    static BOOL bConfigRead = FALSE;
    if (!bConfigRead) {
        bConfigRead = TRUE;
        g_dwTexMagFilter = D3DTEXF_LINEAR;
        g_dwTexMinFilter = D3DTEXF_LINEAR;
        int cnt = 1;
        config_get_value("d3d_disp_post_flags", CONFIG_INT_TYPE, (config_valptr)&g_dwD8Flags, &cnt);
        float f;
        cnt = 1;
        if (config_get_value("d3d_disp_sw_cc_sat", CONFIG_FLOAT_TYPE, (config_valptr)&f, &cnt) && cnt > 0) {
            if (f < 0.0f)
                f = 0.0f;
            g_ColorCorrection.m_fSaturation = f;
        }
        cnt = 1;
        if (config_get_value("d3d_disp_sw_cc_bright", CONFIG_FLOAT_TYPE, (config_valptr)&f, &cnt) && cnt > 0)
            g_ColorCorrection.m_fBrightness = f;
        cnt = 1;
        if (config_get_value("d3d_disp_sw_cc_contr", CONFIG_FLOAT_TYPE, (config_valptr)&f, &cnt) && cnt > 0) {
            if (f < 0.0f)
                f = 0.0f;
            g_ColorCorrection.m_fContrast = f;
        }
        float rgb[3];
        cnt = 3;
        if (config_get_value("d3d_disp_sw_cc_rgbfilter", CONFIG_FLOAT_TYPE, (config_valptr)rgb, &cnt) && cnt > 2) {
            if (rgb[0] < 0.0f)
                rgb[0] = 0.0f;
            if (rgb[1] < 0.0f)
                rgb[1] = 0.0f;
            if (rgb[2] < 0.0f)
                rgb[2] = 0.0f;
            g_ColorCorrection.m_fColorFilter[0] = rgb[0];
            g_ColorCorrection.m_fColorFilter[1] = rgb[1];
            g_ColorCorrection.m_fColorFilter[2] = rgb[2];
        }
    }
    g_dwD8CurrentFVF = 0;
    memset(&g_sD3D9Caps, 0, sizeof(g_sD3D9Caps));
}

void cD8Frame::InitializeEnvironment(lgd3ds_device_info* psDeviceInfo)
{
    D3DCAPS9 caps;
    HRESULT hResult = g_lpD3D9Device->GetDeviceCaps(&caps);
    if (hResult != 0) {
        SetLGD3DErrorCode(LGD3D_EC_GET_DD_CAPS, hResult);
        lgd3d_g_bInitialized = 0;
        return;
    }
    m_bDepthBuffer = TRUE;
    if (g_bUseDepthBuffer) {
        IDirect3DSurface9* pDepth;
        D3DSURFACE_DESC desc;
        hResult = g_lpD3D9Device->GetDepthStencilSurface(&pDepth);
        if (hResult == 0) {
            hResult = pDepth->GetDesc(&desc);
            SafeRelease(pDepth);
        }
        if (hResult != 0) {
            psDeviceInfo->flags &= ~(LGD3DF_ZBUFFER | LGD3DF_WBUFFER);
            if ((psDeviceInfo->flags & LGD3DF_DEPTH_BUFFER_REQUIRED) != 0) {
                lgd3d_g_bInitialized = 0;
                return;
            }
            g_bUseDepthBuffer = 0;
        } else {
            g_sDDPFDepth.dwSize = 0;
            g_sDDPFDepth.dwFlags = 0;
            g_sDDPFDepth.dwFourCC = 0;
            g_sDDPFDepth.dwStencilBitDepth = 0;
            g_sDDPFDepth.dwStencilBitMask = 0;
            g_sDDPFDepth.dwRGBZBitMask = 0;
            switch (desc.Format) {
                case D3DFMT_D16_LOCKABLE:
                case D3DFMT_D16:
                    g_sDDPFDepth.dwZBufferBitDepth = 16;
                    g_sDDPFDepth.dwZBitMask = 0xffff;
                    break;
                case D3DFMT_D15S1:
                    g_sDDPFDepth.dwZBufferBitDepth = 16;
                    g_sDDPFDepth.dwZBitMask = 0xfffe;
                    g_sDDPFDepth.dwStencilBitDepth = 1;
                    g_sDDPFDepth.dwStencilBitMask = 1;
                    break;
                case D3DFMT_D24S8:
                    g_sDDPFDepth.dwZBitMask = 0xffffff00;
                    g_sDDPFDepth.dwStencilBitDepth = 8;
                    g_sDDPFDepth.dwStencilBitMask = 0xff;
                    g_sDDPFDepth.dwZBufferBitDepth = 32;
                    break;
                case D3DFMT_D24X8:
                    g_sDDPFDepth.dwZBitMask = 0xffffff00;
                    g_sDDPFDepth.dwZBufferBitDepth = 32;
                    break;
                case D3DFMT_D24X4S4:
                    g_sDDPFDepth.dwZBitMask = 0xffffff00;
                    g_sDDPFDepth.dwStencilBitDepth = 4;
                    g_sDDPFDepth.dwStencilBitMask = 0xf;
                    g_sDDPFDepth.dwZBufferBitDepth = 32;
                    break;
                default:
                    g_sDDPFDepth.dwZBitMask = 0xffffffff;
                    g_sDDPFDepth.dwZBufferBitDepth = 32;
                    break;
            }
        }
    }
    if (CreateD3D()) {
        m_dwRequestedFlags = psDeviceInfo->flags;
        ExamineRenderingCapabilities();
        psDeviceInfo->flags = m_dwRequestedFlags;
        cD6Renderer::CleanRenderSurface(1.0);
        ApplyDisplayConfig(psDeviceInfo);

        g_bD8ManagedTextures = config_is_defined("d3d_disp_managed_textures");
        g_fD8TexelCenterX = g_fTexelCenterX;
        g_fD8TexelCenterY = g_fTexelCenterY;
        if (g_bD8ScaleRects) {
            g_fTexelCenterX /= g_fD8RectScaleX;
            g_fTexelCenterY /= g_fD8RectScaleY;
            g_XOffset = g_fXOffsetBase - g_fTexelCenterX;
            g_YOffset = g_fYOffsetBase - g_fTexelCenterY;
        }

        IDirect3DStateBlock9* pSaved = NULL;
        g_lpD3D9Device->CreateStateBlock(D3DSBT_ALL, &pSaved);

        int i;
        DWORD dwWanted = 0;
        for (i = 0; g_apD8Effects[i]; i++)
            if (g_apD8Effects[i]->IsEnabled())
                dwWanted |= g_apD8Effects[i]->GetTextureMask();
        CreateSceneTextures(&g_sBackBufferDesc, dwWanted);
        for (i = 0; g_apD8Effects[i]; i++)
            if (g_apD8Effects[i]->IsEnabled() && g_apD8Effects[i]->Init())
                g_apD8Effects[i]->m_dwFlags |= D8EF_CREATED;
        DWORD dwUsed = 0;
        for (i = 0; g_apD8Effects[i]; i++)
            if (g_apD8Effects[i]->m_dwFlags & D8EF_CREATED)
                dwUsed |= g_apD8Effects[i]->GetTextureMask();
        if (dwWanted != dwUsed) {
            for (i = 0; i < kD8NumSceneTextures; i++) {
                if ((dwUsed ^ dwWanted) & dwWanted & D8_SCENE_BIT(i)) {
                    SafeRelease(g_lpSceneTextures[i]);
                    memset(&g_sSceneTextureDesc[i], 0, sizeof(g_sSceneTextureDesc[i]));
                    if (i == kD8SceneMid) {
                        SafeRelease(g_lpSceneTextures[kD8SceneMidTemp]);
                        memset(&g_sSceneTextureDesc[kD8SceneMidTemp], 0, sizeof(g_sSceneTextureDesc[kD8SceneMidTemp]));
                    } else if (i == kD8SceneBlur) {
                        SafeRelease(g_lpSceneTextures[kD8SceneBlurTemp]);
                        memset(&g_sSceneTextureDesc[kD8SceneBlurTemp], 0, sizeof(g_sSceneTextureDesc[kD8SceneBlurTemp]));
                    }
                }
            }
        }
        if (pSaved) {
            pSaved->Apply();
            SafeRelease(pSaved);
        }
    }
}

long cD8Frame::GetDDstuffFromDisplay()
{
    m_pWinDisplayDevice = AppGetObj(IWinDisplayDevice);
    IUnknown* pD3D = NULL;
    m_pWinDisplayDevice->GetDirectDraw((IDirectDraw4**)&pD3D);
    if (!pD3D)
        return E_FAIL;
    pD3D->QueryInterface(IID_IDirect3D9, (void**)&g_lpD3D9);
    SafeRelease(pD3D);
    if (!g_lpD3D9)
        return E_FAIL;
    IUnknown* pSurface = NULL;
    m_pWinDisplayDevice->GetBitmapSurface(NULL, (IDirectDrawSurface4**)&pSurface);
    if (!pSurface)
        return E_FAIL;
    IDirect3DTexture9* pTexture = NULL;
    HRESULT hResult = pSurface->QueryInterface(IID_IDirect3DTexture9, (void**)&pTexture);
    SafeRelease(pSurface);
    if (!pTexture)
        return hResult;
    hResult = pTexture->GetDevice(&g_lpD3D9Device);
    SafeRelease(pTexture);
    return g_lpD3D9Device ? S_OK : hResult;
}

BOOL cD8Frame::CreateD3D()
{
    unsigned long dwWidth = g_dwScreenWidth;
    unsigned long dwHeight = g_dwScreenHeight;
    IDirect3DSurface9* pBackBuffer;
    if (SUCCEEDED(g_lpD3D9Device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer))) {
        D3DSURFACE_DESC desc;
        HRESULT hResult = pBackBuffer->GetDesc(&desc);
        SafeRelease(pBackBuffer);
        if (SUCCEEDED(hResult)) {
            dwWidth = desc.Width;
            dwHeight = desc.Height;
        }
    }
    g_fD8RectScaleX = 1.0f;
    g_bD8ScaleRects = FALSE;
    g_fD8RectScaleY = 1.0f;
    if (dwWidth != g_dwScreenWidth) {
        g_fD8RectScaleX = (float)((double)dwWidth / (double)g_dwScreenWidth);
        g_bD8ScaleRects = TRUE;
        g_fD8ScreenScaleX = g_fD8RectScaleX;
    }
    if (dwHeight != g_dwScreenHeight) {
        g_fD8RectScaleY = (float)((double)dwHeight / (double)g_dwScreenHeight);
        g_bD8ScaleRects = TRUE;
        g_fD8ScreenScaleY = g_fD8RectScaleY;
    }
    return OnResetDevice();
}

void cD8Frame::ExamineRenderingCapabilities()
{
    HRESULT hResult = g_lpD3D9Device->GetDeviceCaps(&g_sD3D9Caps);
    if (hResult != 0) {
        SetLGD3DErrorCode(LGD3D_EC_GET_3D_CAPS, hResult);
        lgd3d_g_bInitialized = 0;
        return;
    }
    g_bWFog = (g_sD3D9Caps.RasterCaps & D3DPRASTERCAPS_WFOG) != 0;
    if ((g_sD3D9Caps.RasterCaps & D3DPRASTERCAPS_WBUFFER) == 0)
        m_dwRequestedFlags &= ~LGD3DF_WBUFFER;
    m_dwTextureOpCaps = g_sD3D9Caps.TextureOpCaps;
    if ((m_dwRequestedFlags & LGD3DF_MULTI_TEXTURING) != 0) {
        if (g_sD3D9Caps.MaxTextureBlendStages > 1 && g_sD3D9Caps.MaxSimultaneousTextures > 1
            && (unsigned short)g_sD3D9Caps.FVFCaps > 1) {
            pcRenderer = cD8Renderer::Instance(1, &m_dwRequestedFlags);
            return;
        }
        m_dwRequestedFlags &= ~LGD3DF_MULTI_TEXTURING;
    }
    pcRenderer = cD8Renderer::Instance(0, &m_dwRequestedFlags);
}

void cD8Frame::OnLostDevice()
{
    ((cD8Renderer*)pcRenderer)->OnLostDevice();
    SafeRelease(g_lpSceneSurface);
    for (int j = 0; j < kD8NumSceneTextures; j++)
        SafeRelease(g_lpSceneTextures[j]);
    for (int i = 0; g_apD8Effects[i]; i++) {
        cD8Effect* pEffect = g_apD8Effects[i];
        if (pEffect->m_dwFlags & D8EF_CREATED) {
            if (pEffect->m_pState) {
                delete pEffect->m_pState;
                pEffect->m_pState = NULL;
            }
            pEffect->OnLostDevice();
        }
    }
}

BOOL cD8Frame::OnResetDevice()
{
    unsigned long dwWidth = g_dwScreenWidth;
    unsigned long dwHeight = g_dwScreenHeight;
    IDirect3DSurface9* pBackBuffer = NULL;
    memset(&g_sBackBufferDesc, 0, sizeof(g_sBackBufferDesc));
    if (SUCCEEDED(g_lpD3D9Device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer))) {
        HRESULT hr = pBackBuffer->GetDesc(&g_sBackBufferDesc);
        SafeRelease(pBackBuffer);
        if (SUCCEEDED(hr)) {
            dwWidth = g_sBackBufferDesc.Width;
            dwHeight = g_sBackBufferDesc.Height;
        }
    }

    D3DVIEWPORT9 vp;
    vp.X = 0;
    vp.Y = 0;
    vp.Width = dwWidth;
    vp.Height = dwHeight;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    HRESULT hResult = g_lpD3D9Device->SetViewport(&vp);
    if (hResult != 0) {
        SetLGD3DErrorCode(LGD3D_EC_SET_VIEWPORT, hResult);
        lgd3d_g_bInitialized = 0;
        return FALSE;
    }
    D3DMATERIAL9 mat;
    memset(&mat, 0, sizeof(mat));
    hResult = g_lpD3D9Device->SetMaterial(&mat);
    if (hResult != 0) {
        SetLGD3DErrorCode(LGD3D_EC_SET_BK_MATERIAL, hResult);
        lgd3d_g_bInitialized = 0;
        return FALSE;
    }

    if (config_is_defined("d3d_disp_enable_hdr")) {
        int nBits = 16;
        config_get_int("d3d_disp_enable_hdr", &nBits);
        D3DFORMAT fmt = nBits == 32 ? D3DFMT_A32B32G32R32F : D3DFMT_A16B16G16R16F;
        HRESULT hr = g_lpD3D9Device->CreateRenderTarget(dwWidth, dwHeight, fmt, g_sBackBufferDesc.MultiSampleType,
                                                        g_sBackBufferDesc.MultiSampleQuality, FALSE, &g_lpSceneSurface, NULL);
        if (hr == 0
            || (fmt != D3DFMT_A16B16G16R16F
                && SUCCEEDED(g_lpD3D9Device->CreateRenderTarget(dwWidth, dwHeight, D3DFMT_A16B16G16R16F,
                                                                g_sBackBufferDesc.MultiSampleType, g_sBackBufferDesc.MultiSampleQuality,
                                                                FALSE, &g_lpSceneSurface, NULL))))
            g_lpSceneSurface->GetDesc(&g_sBackBufferDesc);
    }

    float fGamma = 1.0f;
    int cnt = 1;
    if (config_get_value("postprocess_gamma", CONFIG_FLOAT_TYPE, (config_valptr)&fGamma, &cnt) && cnt > 0) {
        if (fGamma < 0.001)
            fGamma = 0.001f;
        g_PostProcess.m_fGamma = fGamma;
    }

    int i;
    DWORD dwMask = 0;
    for (i = 0; g_apD8Effects[i]; i++)
        if (g_apD8Effects[i]->m_dwFlags & D8EF_CREATED)
            dwMask |= g_apD8Effects[i]->GetTextureMask();
    CreateSceneTextures(&g_sBackBufferDesc, dwMask);
    for (i = 0; g_apD8Effects[i]; i++)
        if (g_apD8Effects[i]->m_dwFlags & D8EF_CREATED)
            g_apD8Effects[i]->OnResetDevice();

    if (pcRenderer && !((cD8Renderer*)pcRenderer)->OnResetDevice())
        return FALSE;
    return TRUE;
}

static void GetSceneTextureSize(const char* pszVar, UINT* pWidth, UINT* pHeight)
{
    int size[2];
    int cnt = 2;
    if (config_get_value(pszVar, CONFIG_INT_TYPE, (config_valptr)size, &cnt) && cnt == 2
        && size[0] > 15 && size[1] > 15 && size[0] <= 4096 && size[1] <= 4096) {
        *pWidth = size[0];
        *pHeight = size[1];
    }
}

BOOL cD8Frame::CreateSceneTextures(D3DSURFACE_DESC* pDesc, DWORD dwMask)
{
    if (dwMask == 0)
        return TRUE;
    if (pDesc->Format == D3DFMT_UNKNOWN)
        return FALSE;

    BOOL bOk = TRUE;
    HRESULT hr;
    UINT w, h;
    if ((dwMask & D8_SCENE_BIT(kD8SceneFull)) && !(g_dwD8Flags & D8F_NO_SCENE_TEXTURE)) {
        hr = g_lpD3D9Device->CreateTexture(pDesc->Width, pDesc->Height, 1, D3DUSAGE_RENDERTARGET, pDesc->Format,
                                           D3DPOOL_DEFAULT, &g_lpSceneTextures[kD8SceneFull], NULL);
        if (FAILED(hr)) {
#line 1101
            CriticalHResult(hr, "CreateTexture failed for scene texture");
            bOk = FALSE;
        } else
            g_lpSceneTextures[kD8SceneFull]->GetLevelDesc(0, &g_sSceneTextureDesc[kD8SceneFull]);
    }
    if ((dwMask & D8_SCENE_BIT(kD8SceneLow)) && !(g_dwD8Flags & D8F_NO_LOW_TEXTURE)) {
        w = pDesc->Width >> 2;
        h = pDesc->Height >> 2;
        GetSceneTextureSize("d3d_disp_scenelow_size", &w, &h);
        hr = g_lpD3D9Device->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, pDesc->Format, D3DPOOL_DEFAULT,
                                           &g_lpSceneTextures[kD8SceneLow], NULL);
        if (FAILED(hr)) {
#line 1130
            CriticalHResult(hr, "CreateTexture failed for low scene texture");
            bOk = FALSE;
        } else
            g_lpSceneTextures[kD8SceneLow]->GetLevelDesc(0, &g_sSceneTextureDesc[kD8SceneLow]);
    }
    if ((dwMask & D8_SCENE_BIT(kD8SceneBlur)) && !(g_dwD8Flags & D8F_NO_BLUR_TEXTURE)) {
        w = pDesc->Width >> 2;
        h = pDesc->Height >> 2;
        GetSceneTextureSize("d3d_disp_sceneblur_size", &w, &h);
        hr = g_lpD3D9Device->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, pDesc->Format, D3DPOOL_DEFAULT,
                                           &g_lpSceneTextures[kD8SceneBlurTemp], NULL);
        if (FAILED(hr)) {
#line 1161
            CriticalHResult(hr, "CreateTexture failed for blurry temp scene texture");
            bOk = FALSE;
        } else {
            hr = g_lpD3D9Device->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, pDesc->Format, D3DPOOL_DEFAULT,
                                               &g_lpSceneTextures[kD8SceneBlur], NULL);
            if (FAILED(hr)) {
#line 1173
                CriticalHResult(hr, "CreateTexture failed for blurry scene texture");
                SafeRelease(g_lpSceneTextures[kD8SceneBlurTemp]);
                bOk = FALSE;
            } else {
                g_lpSceneTextures[kD8SceneBlur]->GetLevelDesc(0, &g_sSceneTextureDesc[kD8SceneBlur]);
                g_lpSceneTextures[kD8SceneBlurTemp]->GetLevelDesc(0, &g_sSceneTextureDesc[kD8SceneBlurTemp]);
            }
        }
    }
    if ((dwMask & D8_SCENE_BIT(kD8SceneMid)) && !(g_dwD8Flags & D8F_NO_MID_TEXTURE)) {
        w = pDesc->Width >> 1;
        h = pDesc->Height >> 1;
        GetSceneTextureSize("d3d_disp_scenemid_size", &w, &h);
        hr = g_lpD3D9Device->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, pDesc->Format, D3DPOOL_DEFAULT,
                                           &g_lpSceneTextures[kD8SceneMidTemp], NULL);
        if (FAILED(hr)) {
#line 1207
            CriticalHResult(hr, "CreateTexture failed for mid temp scene texture");
            return FALSE;
        }
        hr = g_lpD3D9Device->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, pDesc->Format, D3DPOOL_DEFAULT,
                                           &g_lpSceneTextures[kD8SceneMid], NULL);
        if (FAILED(hr)) {
#line 1219
            CriticalHResult(hr, "CreateTexture failed for mid scene texture");
            SafeRelease(g_lpSceneTextures[kD8SceneMidTemp]);
            return FALSE;
        }
        g_lpSceneTextures[kD8SceneMid]->GetLevelDesc(0, &g_sSceneTextureDesc[kD8SceneMid]);
        g_lpSceneTextures[kD8SceneMidTemp]->GetLevelDesc(0, &g_sSceneTextureDesc[kD8SceneMidTemp]);
    }
    return bOk;
}
