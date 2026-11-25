#include <d8dx6.h>
#include <lgd3d.h>
#include <d8states.h>
#include <d6over.h>
#include <d6prim.h>
#include <d8prim.h>
#include <stdlib.h>
#include <lgassert.h>
#include <d6check.h>
#include <dbg.h>
#include <tmgr.h>
#include <comtools.h>
#include <dispapi.h>
#include <gshelapi.h>
#include <wdisp.h>
#include <wdspprov.h>
#include <wdspmode.h>
#include <wdispd3d.h>
#include <wd3dmode.h>

class cD8SuspendScene {
public:
    cD8SuspendScene(BOOL bRestartScene, IDirect3DSurface9* pSavedTarget)
    {
        m_bRestartScene = bRestartScene;
        if (m_bRestartScene)
            g_lpD3D9Device->EndScene();
        m_pSavedTarget = pSavedTarget;
        if (m_pSavedTarget)
            g_lpD3D9Device->SetRenderTarget(0, NULL);
    }
    ~cD8SuspendScene()
    {
        if (m_pSavedTarget)
            g_lpD3D9Device->SetRenderTarget(0, m_pSavedTarget);
        if (m_pSavedTarget)
            m_pSavedTarget->Release();
        m_pSavedTarget = NULL;
        if (m_bRestartScene)
            g_lpD3D9Device->BeginScene();
    }

    BOOL m_bRestartScene;
    IDirect3DSurface9* m_pSavedTarget;
};

cD8Renderer::cD8Renderer(unsigned long mode, unsigned long* pInitFlags)
    : m_bInScene(FALSE), m_nFrame(-1), m_bDeviceLost(FALSE), m_bPostProcess(FALSE),
      m_pSavedTarget(NULL), m_bTargetSwapped(FALSE)
{
    Init(mode, pInitFlags);
}

cD8Renderer::~cD8Renderer()
{
    if (m_bTargetSwapped)
    {
        HRESULT hr = g_lpD3D9Device->SetRenderTarget(0, m_pSavedTarget);
#line 98
        CheckHResult(hr, "cD8Renderer::EndFrame: SetRenderTarget failed");
        if (m_pSavedTarget)
            m_pSavedTarget->Release();
        m_pSavedTarget = NULL;
    }
    if (m_bInScene)
    {
        g_lpD3D9Device->EndScene();
        if (g_pD3DModeOps)
            g_pD3DModeOps->Clear2DDirty();
    }
}

void cD8Renderer::OnLostDevice()
{
    m_bDeviceLost = TRUE;
    m_bPostProcess = FALSE;
    if (m_bTargetSwapped)
    {
        m_bTargetSwapped = FALSE;
        HRESULT hr = g_lpD3D9Device->SetRenderTarget(0, m_pSavedTarget);
#line 123
        CheckHResult(hr, "cD8Renderer::EndFrame: SetRenderTarget failed");
        if (m_pSavedTarget)
            m_pSavedTarget->Release();
        m_pSavedTarget = NULL;
    }
    if (m_bInScene)
    {
        m_bInScene = FALSE;
        g_lpD3D9Device->EndScene();
        if (g_pD3DModeOps)
            g_pD3DModeOps->Clear2DDirty();
    }
}

BOOL cD8Renderer::OnResetDevice()
{
    m_bDeviceLost = FALSE;
    return TRUE;
}

cD6Renderer* cD8Renderer::Instance(unsigned long mode, unsigned long* pInitFlags)
{
    if (cD6Renderer::m_RendererInstance == NULL)
        cD6Renderer::m_RendererInstance = new cD8Renderer(mode, pInitFlags);
    return cD6Renderer::m_RendererInstance;
}

void cD8Renderer::StartFrame(int frame)
{
    if (m_bDeviceLost || m_bInScene)
        return;
    int nLastFrame = m_nFrame;
    m_bInScene = TRUE;
    m_nFrame = frame;
    if (g_pD3DModeOps)
        g_pD3DModeOps->SetFrame(frame);
    if (nLastFrame != frame)
    {
        if (!g_lpSceneSurface)
        {
            if (g_lpSceneTextures[kD8SceneFull])
                m_bPostProcess = TRUE;
        }
        else if (g_lpSceneTextures[kD8SceneFull])
        {
            HRESULT hr = g_lpD3D9Device->GetRenderTarget(0, &m_pSavedTarget);
#line 185
            CheckHResult(hr, "cD8Renderer::StartFrame: GetRenderTarget failed");
            if (SUCCEEDED(hr))
            {
                hr = g_lpD3D9Device->SetRenderTarget(0, g_lpSceneSurface);
#line 191
                CheckHResult(hr, "cD8Renderer::StartFrame: SetRenderTarget failed");
                if (FAILED(hr))
                {
                    if (m_pSavedTarget)
                        m_pSavedTarget->Release();
                    m_pSavedTarget = NULL;
                }
                else
                {
                    m_bTargetSwapped = TRUE;
                    m_bPostProcess = TRUE;
                }
            }
        }
    }
    HRESULT hr = g_lpD3D9Device->BeginScene();
#line 213
    CheckSucceeded(hr, "cD8Renderer::StartFrame: BeginScene failed");
    pcRenderBuffer->StartFrame();
    if (g_tmgr)
        g_tmgr->start_frame(frame);
}

void cD8Renderer::EndFrame()
{
    if (!m_bInScene)
    {
        if (g_tmgr)
            g_tmgr->end_frame();
        return;
    }
    m_bInScene = FALSE;
    pcRenderBuffer->EndFrame();
    if (g_lpD3D9Device && pcRenderer)
        ((cD8Renderer*)pcRenderer)->FlushPostProcessing(D8_ColorCorrectionActive());
    if (m_bOverlaysOn && pcOverlayHandler)
        pcOverlayHandler->DrawOverlays();
    g_lpD3D9Device->EndScene();
    if (g_tmgr)
        g_tmgr->end_frame();
    if (g_pD3DModeOps)
        g_pD3DModeOps->Clear2DDirty();
}

void cD8Renderer::FlushPostProcessing(BOOL bKeepTarget)
{
    if (!m_bTargetSwapped)
    {
        if (m_bPostProcess)
        {
            m_bPostProcess = FALSE;
            if (D8_PostProcessEnabled())
            {
                pcRenderBuffer->EndFrame();
                g_lpD3D9Device->EndScene();
                if (UpdateSceneTexture(D8_SCENE_BIT(kD8SceneFull), NULL, TRUE))
                    D8_RunPostProcess(g_lpSceneTextures[kD8SceneFull]);
                g_lpD3D9Device->BeginScene();
            }
        }
        return;
    }
    if (!bKeepTarget)
        m_bTargetSwapped = FALSE;
    if (!g_lpSceneSurface)
        return;
    pcRenderBuffer->EndFrame();
    if (m_bPostProcess && D8_PostProcessEnabled())
    {
        g_lpD3D9Device->EndScene();
        if (!bKeepTarget)
        {
            HRESULT hr = g_lpD3D9Device->SetRenderTarget(0, m_pSavedTarget);
#line 283
            CheckHResult(hr, "cD8Renderer::EndFrame: SetRenderTarget failed");
            if (m_pSavedTarget)
                m_pSavedTarget->Release();
            m_pSavedTarget = NULL;
        }
        if (UpdateSceneTexture(D8_SCENE_BIT(kD8SceneFull), g_lpSceneSurface, TRUE))
            D8_RunPostProcess(g_lpSceneTextures[kD8SceneFull]);
        else
        {
            HRESULT hr = g_lpD3D9Device->StretchRect(g_lpSceneSurface, NULL, m_pSavedTarget, NULL, D3DTEXF_POINT);
#line 298
            CheckSucceeded(hr, "cD8Renderer::FlushPostProcessing: fallback StretchRect failed");
        }
    }
    else
    {
        if (bKeepTarget)
        {
            m_bPostProcess = FALSE;
            return;
        }
        g_lpD3D9Device->EndScene();
        HRESULT hr = g_lpD3D9Device->StretchRect(g_lpSceneSurface, NULL, m_pSavedTarget, NULL, D3DTEXF_POINT);
#line 311
        CheckSucceeded(hr, "cD8Renderer::FlushPostProcessing: StretchRect failed");
        hr = g_lpD3D9Device->SetRenderTarget(0, m_pSavedTarget);
#line 316
        CheckHResult(hr, "cD8Renderer::EndFrame: SetRenderTarget failed");
        if (m_pSavedTarget)
            m_pSavedTarget->Release();
        m_pSavedTarget = NULL;
    }
    g_lpD3D9Device->BeginScene();
    m_bPostProcess = FALSE;
}

void cD8Renderer::ApplyGamma()
{
    if (m_bTargetSwapped)
    {
        m_bTargetSwapped = FALSE;
        HRESULT hr = g_lpD3D9Device->SetRenderTarget(0, m_pSavedTarget);
#line 359
        CheckHResult(hr, "cD8Renderer::EndFrame: SetRenderTarget failed");
        if (m_pSavedTarget)
            m_pSavedTarget->Release();
        m_pSavedTarget = NULL;
        if (g_lpSceneSurface && g_lpD3D9Device)
            D8_ApplyColorCorrection(g_lpSceneSurface);
    }
    else
    {
        IDirect3DSurface9* pTarget = NULL;
        g_lpD3D9Device->GetRenderTarget(0, &pTarget);
        if (pTarget)
        {
            if (g_lpD3D9Device)
                D8_ApplyColorCorrection(pTarget);
            if (pTarget)
                pTarget->Release();
        }
    }
}

HRESULT cD8Renderer::Clear(DWORD dwCount, D3DRECT* pRects, DWORD dwFlags, D3DCOLOR dwColor, float dvZ, DWORD dwStencil)
{
    D3DRECT rects[16];
    if (g_bD8ScaleRects)
    {
        for (DWORD i = 0; i < dwCount; i++)
        {
            rects[i].x1 = (long)(pRects[i].x1 * g_fD8RectScaleX);
            rects[i].y1 = (long)(pRects[i].y1 * g_fD8RectScaleY);
            rects[i].x2 = (long)(pRects[i].x2 * g_fD8RectScaleX);
            rects[i].y2 = (long)(pRects[i].y2 * g_fD8RectScaleY);
        }
        pRects = rects;
    }
    return g_lpD3D9Device->Clear(dwCount, pRects, dwFlags, dwColor, dvZ, dwStencil);
}

HRESULT cD8Renderer::SetTransform(int nWhich, D3DMATRIX* pMatrix)
{
    D3DTRANSFORMSTATETYPE type;
    switch (nWhich) {
        case 0:
            type = D3DTS_WORLD;
            break;
        case 1:
            type = D3DTS_VIEW;
            break;
        case 2:
            type = D3DTS_PROJECTION;
            break;
        default:
            return E_INVALIDARG;
    }
    return g_lpD3D9Device->SetTransform(type, pMatrix);
}

static BOOL CopyTargetToTexture(IDirect3DTexture9* pTexture, IDirect3DSurface9* pSource, D3DTEXTUREFILTERTYPE filter)
{
    IDirect3DSurface9* pLevel;
    if (!pSource)
        return FALSE;
    HRESULT hr = pTexture->GetSurfaceLevel(0, &pLevel);
    if (FAILED(hr))
    {
#line 437
        CriticalHResult(hr, "cD8Renderer::CopyTargetToTexture: GetSurfaceLevel failed");
        return FALSE;
    }
    hr = g_lpD3D9Device->StretchRect(pSource, NULL, pLevel, NULL, filter);
#line 445
    CheckSucceeded(hr, "cD8Renderer::CopyTargetToTexture: StretchRect failed");
    if (pLevel)
        pLevel->Release();
    return hr == S_OK;
}

BOOL cD8Renderer::UpdateSceneTexture(DWORD dwWhat, IDirect3DSurface9* pSource, BOOL bKeepScene)
{
    if (!dwWhat)
        return TRUE;
    if (m_bInScene && !bKeepScene)
        pcRenderBuffer->EndFrame();
    HRESULT hr = S_OK;
    IDirect3DSurface9* pTarget = NULL;
    if (!pSource)
    {
        hr = g_lpD3D9Device->GetRenderTarget(0, &pTarget);
        if (FAILED(hr))
        {
#line 480
            CriticalHResult(hr, "cD8Renderer::UpdateSceneTexture: GetRenderTarget failed");
            return FALSE;
        }
    }
    cD8SuspendScene suspend(m_bInScene && !bKeepScene, pTarget);
    if (!pSource)
        pSource = pTarget;

    D3DSURFACE_DESC desc;
    BOOL bMultiSampled = !(g_dwD8Flags & D8F_NO_SCENE_COPY)
                         && SUCCEEDED(pSource->GetDesc(&desc))
                         && desc.MultiSampleType != D3DMULTISAMPLE_NONE;
    IDirect3DSurface9* pSceneLevel = NULL;
    if (bMultiSampled && g_lpSceneTextures[kD8SceneFull] && SUCCEEDED(g_lpSceneTextures[kD8SceneFull]->GetSurfaceLevel(0, &pSceneLevel)))
        dwWhat |= D8_SCENE_BIT(kD8SceneFull);
    if ((dwWhat & D8_SCENE_BIT(kD8SceneFull)) && (!g_lpSceneTextures[kD8SceneFull] || !CopyTargetToTexture(g_lpSceneTextures[kD8SceneFull], pSource, D3DTEXF_POINT)))
    {
        dwWhat &= ~D8_SCENE_BIT(kD8SceneFull);
        hr = E_FAIL;
    }
    if ((dwWhat & D8_SCENE_BIT(kD8SceneLow)) && (!g_lpSceneTextures[kD8SceneLow] || !CopyTargetToTexture(g_lpSceneTextures[kD8SceneLow], pSource, D3DTEXF_LINEAR)))
    {
        dwWhat &= ~D8_SCENE_BIT(kD8SceneLow);
        hr = E_FAIL;
    }
    if (dwWhat & D8_SCENE_BIT(kD8SceneBlur))
    {
        IDirect3DTexture9* pBloomSource;
        if ((dwWhat & D8_SCENE_BIT(kD8SceneLow)) && g_sSceneTextureDesc[kD8SceneLow].Width == g_sSceneTextureDesc[kD8SceneBlur].Width
            && g_sSceneTextureDesc[kD8SceneLow].Height == g_sSceneTextureDesc[kD8SceneBlur].Height)
            pBloomSource = g_lpSceneTextures[kD8SceneLow];
        else if (g_lpSceneTextures[kD8SceneBlur] && CopyTargetToTexture(g_lpSceneTextures[kD8SceneBlur], pSource, D3DTEXF_LINEAR))
            pBloomSource = g_lpSceneTextures[kD8SceneBlur];
        else
        {
            if (pSceneLevel)
                pSceneLevel->Release();
            pSceneLevel = NULL;
            return FALSE;
        }
        g_lpD3D9Device->BeginScene();
        BOOL bOk = D8_BloomEnabled() ? D8_RenderBloom(pBloomSource, g_lpSceneTextures[kD8SceneBlurTemp], g_lpSceneTextures[kD8SceneBlur]) : FALSE;
        g_lpD3D9Device->EndScene();
        if (!bOk)
            hr = E_FAIL;
    }
    if (pSceneLevel)
        pSceneLevel->Release();
    pSceneLevel = NULL;
    return hr == S_OK;
}

void cD8Renderer::CreateStates(unsigned long mode, unsigned long* pInitFlags)
{
    if (mode != 0) {
        if (mode == 1) {
            pcRenderBuffer = cD8MSBuffer::Instance();
            pcStates = cD8MSStates::Instance();
        }
    } else {
        pcRenderBuffer = cD8ImBuffer::Instance();
        pcStates = cD8ImStates::Instance();
    }
}
