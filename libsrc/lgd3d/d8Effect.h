#pragma once

#include <d8states.h>
#include <d6check.h>
#include <config.h>
#include <mprintf.h>
#include <lglog.h>

#define D8EF_CREATED 0x1
#define D8EF_ACTIVE  0x2

IDirect3DTexture9* D8_GetSceneTexture(int nWhich);

class cD8Effect;

class cD8EffectState {
public:
    cD8EffectState() : m_pStates(NULL), m_pRestore(NULL) {}
    ~cD8EffectState()
    {
        if (m_pStates)
            m_pStates->Release();
        m_pStates = NULL;
        if (m_pRestore)
            m_pRestore->Release();
        m_pRestore = NULL;
    }

    BOOL Create(cD8Effect* pEffect);

    BOOL Apply()
    {
        if (!m_pStates)
            return FALSE;
        HRESULT hr = m_pRestore->Capture();
#line 107
        CheckHResult(hr, "cD8EffectState: capture state block");
        if (FAILED(hr))
            return FALSE;
        hr = m_pStates->Apply();
#line 112
        CheckHResult(hr, "cD8EffectState: apply state block");
        return hr == S_OK;
    }

    BOOL Restore()
    {
        if (!m_pStates)
            return FALSE;
        HRESULT hr = m_pRestore->Apply();
#line 123
        CheckHResult(hr, "cD8EffectState: apply restore state block");
        return hr == S_OK;
    }

    IDirect3DStateBlock9* m_pStates;
    IDirect3DStateBlock9* m_pRestore;
};

class cD8Effect {
public:
    cD8Effect() : m_dwFlags(0), m_pState(NULL) {}
    virtual ~cD8Effect()
    {
        if (m_pState)
            delete m_pState;
        m_pState = NULL;
    }
    virtual BOOL IsEnabled() { return TRUE; }
    virtual DWORD GetTextureMask() { return 0; }
    virtual BOOL SetParam(int nParam, const void* pValue) { return FALSE; }
    virtual BOOL GetParam(int nParam, void* pValue) { return FALSE; }
    virtual BOOL Init() = 0;
    virtual void Term() = 0;
    virtual void OnLostDevice() = 0;
    virtual BOOL OnResetDevice() = 0;
    virtual BOOL Begin()
    {
        if (m_pState && !m_pState->Apply())
            return FALSE;
        return TRUE;
    }
    virtual void End()
    {
        if (m_pState)
            m_pState->Restore();
    }
    virtual BOOL SetupStates() { return FALSE; }

    BOOL Activate()
    {
        m_dwFlags |= D8EF_ACTIVE;
        if (!Begin()) {
            m_dwFlags &= ~D8EF_ACTIVE;
            return FALSE;
        }
        return TRUE;
    }

    void Deactivate()
    {
        End();
        m_dwFlags &= ~D8EF_ACTIVE;
    }

    BOOL CreateState()
    {
        m_pState = new cD8EffectState;
        if (!m_pState->Create(this)) {
            if (m_pState)
                delete m_pState;
            m_pState = NULL;
            return FALSE;
        }
        return TRUE;
    }

    DWORD m_dwFlags;
    cD8EffectState* m_pState;
};

inline BOOL cD8EffectState::Create(cD8Effect* pEffect)
{
    if (FAILED(g_lpD3D9Device->BeginStateBlock()))
        return FALSE;
    BOOL bOk = pEffect->SetupStates();
    if (FAILED(g_lpD3D9Device->EndStateBlock(&m_pStates)))
        return FALSE;
    if (bOk && SUCCEEDED(g_lpD3D9Device->BeginStateBlock())) {
        bOk = pEffect->SetupStates();
        if (SUCCEEDED(g_lpD3D9Device->EndStateBlock(&m_pRestore))) {
            if (bOk)
                return TRUE;
            if (m_pStates)
                m_pStates->Release();
            m_pStates = NULL;
            if (m_pRestore)
                m_pRestore->Release();
            m_pRestore = NULL;
            return FALSE;
        }
    }
    if (m_pStates)
        m_pStates->Release();
    m_pStates = NULL;
    return FALSE;
}

inline IDirect3DTexture9* D8_GetSceneTexture(int nWhich)
{
    IDirect3DTexture9* pTexture = g_lpSceneTextures[nWhich];
    if (pTexture)
        return pTexture;
    switch (nWhich) {
        case kD8SceneFull:
            return g_lpSceneTextures[kD8SceneMid] ? g_lpSceneTextures[kD8SceneMid] : g_lpSceneTextures[kD8SceneLow];
        case kD8SceneMid:
            return g_lpSceneTextures[kD8SceneFull] ? g_lpSceneTextures[kD8SceneFull] : g_lpSceneTextures[kD8SceneLow];
        case kD8SceneLow:
            return g_lpSceneTextures[kD8SceneMid] ? g_lpSceneTextures[kD8SceneMid] : g_lpSceneTextures[kD8SceneFull];
    }
    return NULL;
}

#define D8SetSamplerState(stage, type, val) \
    { HRESULT hr = g_lpD3D9Device->SetSamplerState((stage), (type), (val)); CheckHResult(hr, "SetSamplerState failed"); }
#define D8SetRenderState(state, val) \
    { HRESULT hr = g_lpD3D9Device->SetRenderState((state), (val)); CheckHResult(hr, "SetRenderStateForGlobal failed"); }

class cD8Effect_ScreenDistort : public cD8Effect {
public:
    cD8Effect_ScreenDistort(int nTexture) : m_pShader(NULL), m_nTexture(nTexture)
    {
        m_fScale[0] = 0.25f;
        m_fScale[1] = 0.25f;
    }
    virtual BOOL IsEnabled() { return config_is_nonzero("d3d_disp_enable_distortionfx"); }
    virtual DWORD GetTextureMask() { return D8_SCENE_BIT(m_nTexture); }
    virtual BOOL SetParam(int nParam, const void* pValue)
    {
        if (nParam != 0)
            return FALSE;
        m_fScale[0] = ((const float*)pValue)[0];
        m_fScale[1] = ((const float*)pValue)[1];
        UpdateConstants();
        return TRUE;
    }
    virtual BOOL GetParam(int nParam, void* pValue)
    {
        if (nParam != 0)
            return FALSE;
        ((float*)pValue)[0] = m_fScale[0];
        ((float*)pValue)[1] = m_fScale[1];
        return TRUE;
    }
    virtual BOOL Init()
    {
        if (!D8_GetSceneTexture(m_nTexture))
            return FALSE;
        return OnResetDevice();
    }
    virtual void Term() { OnLostDevice(); }
    virtual void OnLostDevice()
    {
        if (m_pShader)
            m_pShader->Release();
        m_pShader = NULL;
    }
    virtual BOOL OnResetDevice();
    virtual BOOL Begin()
    {
        if (!m_pShader || !D8_GetSceneTexture(m_nTexture))
            return FALSE;
        if (m_pState && !m_pState->Apply())
            return FALSE;
        UpdateConstants();
        return TRUE;
    }
    virtual BOOL SetupStates();

    void UpdateConstants()
    {
        if (m_dwFlags & D8EF_ACTIVE) {
            float c[4] = { m_fScale[0], m_fScale[1], m_fScale[0], m_fScale[1] };
            g_lpD3D9Device->SetPixelShaderConstantF(0, c, 1);
        }
    }

    IDirect3DPixelShader9* m_pShader;
    int m_nTexture;
    float m_fScale[2];
};

class cD8Effect_FakeDOF : public cD8Effect {
public:
    cD8Effect_FakeDOF() : m_pShader(NULL) {}
    virtual BOOL IsEnabled() { return config_is_nonzero("d3d_disp_enable_distortionfx"); }
    virtual DWORD GetTextureMask() { return D8_SCENE_BIT(kD8SceneBlur); }
    virtual BOOL Init()
    {
        if (!g_lpSceneTextures[kD8SceneBlur])
            return FALSE;
        return OnResetDevice();
    }
    virtual void Term() { OnLostDevice(); }
    virtual void OnLostDevice()
    {
        if (m_pShader)
            m_pShader->Release();
        m_pShader = NULL;
    }
    virtual BOOL OnResetDevice();
    virtual BOOL Begin()
    {
        if (!m_pShader || !g_lpSceneTextures[kD8SceneBlur])
            return FALSE;
        if (m_pState && !m_pState->Apply())
            return FALSE;
        return TRUE;
    }
    virtual BOOL SetupStates();

    IDirect3DPixelShader9* m_pShader;
};

class cD8Effect_Blur2pass : public cD8Effect {
public:
    cD8Effect_Blur2pass() : m_pShader(NULL), m_fBlurAmount(1.0f) {}
    virtual BOOL IsEnabled() { return config_is_nonzero("d3d_disp_enable_distortionfx"); }
    virtual DWORD GetTextureMask() { return D8_SCENE_BIT(kD8SceneBlur); }
    virtual BOOL SetParam(int nParam, const void* pValue)
    {
        if (nParam != 0)
            return FALSE;
        m_fBlurAmount = *(const float*)pValue;
        return TRUE;
    }
    virtual BOOL GetParam(int nParam, void* pValue)
    {
        if (nParam != 0)
            return FALSE;
        *(float*)pValue = m_fBlurAmount;
        return TRUE;
    }
    virtual BOOL Init()
    {
        if (!g_lpSceneTextures[kD8SceneBlur])
            return FALSE;
        return OnResetDevice();
    }
    virtual void Term() { OnLostDevice(); }
    virtual void OnLostDevice()
    {
        if (m_pShader)
            m_pShader->Release();
        m_pShader = NULL;
    }
    virtual BOOL OnResetDevice();
    virtual BOOL Begin()
    {
        if (!m_pShader || !g_lpSceneTextures[kD8SceneBlur])
            return FALSE;
        if (m_pState && !m_pState->Apply())
            return FALSE;
        return TRUE;
    }
    virtual BOOL SetupStates();

    IDirect3DPixelShader9* m_pShader;
    float m_fBlurAmount;
};

class cD8Effect_PostProcess : public cD8Effect {
public:
    enum { kNumCombineShaders = 3 };

    cD8Effect_PostProcess() : m_pBrightPass(NULL), m_pBlur(NULL), m_fGamma(1.0f)
    {
        for (int i = 0; i < kNumCombineShaders; i++)
            m_pCombine[i] = NULL;
    }
    virtual BOOL IsEnabled() { return config_is_nonzero("postprocess"); }
    virtual DWORD GetTextureMask() { return D8_SCENE_BIT(kD8SceneFull) | D8_SCENE_BIT(kD8SceneMid); }
    virtual BOOL SetParam(int nParam, const void* pValue)
    {
        if (nParam != 0)
            return FALSE;
        m_fGamma = *(const float*)pValue;
        return TRUE;
    }
    virtual BOOL GetParam(int nParam, void* pValue)
    {
        if (nParam != 0)
            return FALSE;
        *(float*)pValue = m_fGamma;
        return TRUE;
    }
    virtual BOOL Init()
    {
        if (!g_lpSceneTextures[kD8SceneFull] || !g_lpSceneTextures[kD8SceneMid])
            return FALSE;
        return OnResetDevice();
    }
    virtual void Term() { OnLostDevice(); }
    virtual void OnLostDevice()
    {
        if (m_pBrightPass)
            m_pBrightPass->Release();
        m_pBrightPass = NULL;
        if (m_pBlur)
            m_pBlur->Release();
        m_pBlur = NULL;
        for (int i = 0; i < kNumCombineShaders; i++) {
            if (m_pCombine[i])
                m_pCombine[i]->Release();
            m_pCombine[i] = NULL;
        }
    }
    virtual BOOL OnResetDevice();
    virtual BOOL Begin()
    {
        if (!m_pBrightPass || !m_pBlur || !g_lpSceneTextures[kD8SceneFull] || !g_lpSceneTextures[kD8SceneMid])
            return FALSE;
        for (int i = 0; i < kNumCombineShaders; i++)
            if (!m_pCombine[i])
                return FALSE;
        return cD8Effect::Begin();
    }
    virtual BOOL SetupStates();

    IDirect3DPixelShader9* m_pBrightPass;
    IDirect3DPixelShader9* m_pBlur;
    IDirect3DPixelShader9* m_pCombine[kNumCombineShaders];
    float m_fGamma;
};

class cD8Effect_ColorCorrection : public cD8Effect {
public:
    enum eParam {
        kGamma,
        kBrightness,
        kContrast,
        kSaturation,
        kColorFilter
    };

    cD8Effect_ColorCorrection()
        : m_pEffect(NULL), m_fGamma(1.0f), m_fContrast(1.0f), m_fBrightness(0.0f), m_fSaturation(1.0f), m_nMode(-1)
    {
        m_fColorFilter[0] = 1.0f;
        m_fColorFilter[1] = 1.0f;
        m_fColorFilter[2] = 1.0f;
    }
    virtual BOOL IsEnabled() { return GetMode(); }
    virtual DWORD GetTextureMask() { return D8_SCENE_BIT(kD8SceneFull); }
    virtual BOOL SetParam(int nParam, const void* pValue)
    {
        switch (nParam) {
            case kGamma:
                m_fGamma = *(const float*)pValue;
                return TRUE;
            case kBrightness:
                m_fBrightness = *(const float*)pValue;
                return TRUE;
            case kContrast:
                m_fContrast = *(const float*)pValue;
                return TRUE;
            case kSaturation:
                m_fSaturation = *(const float*)pValue;
                return TRUE;
            case kColorFilter:
                m_fColorFilter[0] = ((const float*)pValue)[0];
                m_fColorFilter[1] = ((const float*)pValue)[1];
                m_fColorFilter[2] = ((const float*)pValue)[2];
                return TRUE;
        }
        return FALSE;
    }
    virtual BOOL GetParam(int nParam, void* pValue)
    {
        switch (nParam) {
            case kGamma:
                *(float*)pValue = m_fGamma;
                return TRUE;
            case kBrightness:
                *(float*)pValue = m_fBrightness;
                return TRUE;
            case kContrast:
                *(float*)pValue = m_fContrast;
                return TRUE;
            case kSaturation:
                *(float*)pValue = m_fSaturation;
                return TRUE;
            case kColorFilter:
                ((float*)pValue)[0] = m_fColorFilter[0];
                ((float*)pValue)[1] = m_fColorFilter[1];
                ((float*)pValue)[2] = m_fColorFilter[2];
                return TRUE;
        }
        return FALSE;
    }
    virtual BOOL Init()
    {
        if (!g_lpSceneTextures[kD8SceneFull])
            return FALSE;
        if (FAILED(D8_CreateColorCorrectionEffect(&m_pEffect)))
            return FALSE;
        if (!CreateState()) {
            if (m_pEffect)
                m_pEffect->Release();
            m_pEffect = NULL;
            return FALSE;
        }
        return TRUE;
    }
    virtual void Term()
    {
        if (m_pEffect)
            m_pEffect->Release();
        m_pEffect = NULL;
    }
    virtual void OnLostDevice()
    {
        if (m_pEffect)
            m_pEffect->OnLostDevice();
    }
    virtual BOOL OnResetDevice()
    {
        if (m_pEffect && FAILED(m_pEffect->OnResetDevice()))
            return FALSE;
        return CreateState();
    }
    virtual BOOL Begin()
    {
        if (!m_pEffect || !g_lpSceneTextures[kD8SceneFull])
            return FALSE;
        if (m_pState && !m_pState->Apply())
            return FALSE;
        return TRUE;
    }
    virtual BOOL SetupStates();

    int GetMode()
    {
        if (m_nMode == -1) {
            m_nMode = config_is_nonzero("d3d_disp_sw_cc");
            if (m_nMode) {
                int nMode;
                int cnt = 1;
                if (config_get_value("d3d_disp_sw_cc", CONFIG_INT_TYPE, (config_valptr)&nMode, &cnt) && cnt > 0 && nMode > 1)
                    m_nMode = 2;
            }
        }
        return m_nMode;
    }

    BOOL IsActive()
    {
        return GetMode() && m_pEffect && g_lpSceneTextures[kD8SceneFull]
            && (m_nMode == 2 || m_fGamma != 1.0f || m_fContrast != 1.0f || m_fBrightness != 0.0f
                || m_fSaturation != 1.0f || m_fColorFilter[0] != 1.0f || m_fColorFilter[1] != 1.0f
                || m_fColorFilter[2] != 1.0f);
    }

    ID3DXEffect* m_pEffect;
    float m_fGamma;
    float m_fContrast;
    float m_fBrightness;
    float m_fSaturation;
    float m_fColorFilter[3];
    int m_nMode;
};

extern cD8Effect* g_apD8Effects[];

#include <d8shaders.h>

inline void D8_SetScreenPassStates(IDirect3DPixelShader9* pShader)
{
    ((cD8States*)pcStates)->SetAlphaToCoverage(FALSE);
    g_lpD3D9Device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1);
    g_lpD3D9Device->SetVertexShader(NULL);
    g_lpD3D9Device->SetPixelShader(pShader);

    D3DVIEWPORT9 vp;
    vp.X = 0;
    vp.Y = 0;
    vp.Width = 4;
    vp.Height = 4;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    g_lpD3D9Device->SetViewport(&vp);

    D3DMATRIX ident;
    ident._12 = ident._13 = ident._14 = 0.0f;
    ident._21 = ident._23 = ident._24 = 0.0f;
    ident._31 = ident._32 = ident._34 = 0.0f;
    ident._41 = ident._42 = ident._43 = 0.0f;
    ident._11 = ident._22 = ident._33 = ident._44 = 1.0f;
    g_lpD3D9Device->SetTransform(D3DTS_WORLD, &ident);
    g_lpD3D9Device->SetTransform(D3DTS_VIEW, &ident);
    g_lpD3D9Device->SetTransform(D3DTS_PROJECTION, &ident);
}


inline BOOL cD8Effect_ScreenDistort::OnResetDevice()
{
    HRESULT hr = g_lpD3D9Device->CreatePixelShader(g_psScreenDistort, &m_pShader);
    if (FAILED(hr)) {
        ReportHResult(hr, "cD8Effect_ScreenDistort: CreatePixelShader failed");
        return FALSE;
    }
    return CreateState();
}

inline BOOL cD8Effect_ScreenDistort::SetupStates()
{
#line 498
    D8SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    D8SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    D8SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    D8SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);

    D8SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
    D8SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);
    D8SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    D8SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    D8SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    g_lpD3D9Device->SetTexture(0, NULL);
    g_lpD3D9Device->SetTexture(1, D8_GetSceneTexture(m_nTexture));


    D8SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    D8SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    D8SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    D8SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    D8SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    D8SetRenderState(D3DRS_ALPHAREF, 0);
    ((cD8States*)pcStates)->SetAlphaToCoverage(FALSE);
    g_lpD3D9Device->SetPixelShader(m_pShader);
    return TRUE;
}

inline BOOL cD8Effect_FakeDOF::OnResetDevice()
{
    HRESULT hr = g_lpD3D9Device->CreatePixelShader(g_psFakeDOF, &m_pShader);
    if (FAILED(hr)) {
        ReportHResult(hr, "cD8Effect_FakeDOF: CreatePixelShader failed");
        return FALSE;
    }
    return CreateState();
}

inline BOOL cD8Effect_FakeDOF::SetupStates()
{
#line 667
    D8SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    D8SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    D8SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    D8SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    g_lpD3D9Device->SetTexture(0, g_lpSceneTextures[kD8SceneBlur]);
    g_lpD3D9Device->SetTexture(1, NULL);


    D8SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    D8SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    D8SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    D8SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    D8SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    D8SetRenderState(D3DRS_ALPHAREF, 0);
    ((cD8States*)pcStates)->SetAlphaToCoverage(FALSE);
    g_lpD3D9Device->SetPixelShader(m_pShader);
    return TRUE;
}

inline BOOL cD8Effect_Blur2pass::OnResetDevice()
{
    HRESULT hr = g_lpD3D9Device->CreatePixelShader(g_psBlur, &m_pShader);
    if (FAILED(hr)) {
        ReportHResult(hr, "cD8Effect_Blur2pass: CreatePixelShader failed");
        return FALSE;
    }
    return CreateState();
}

inline BOOL cD8Effect_Blur2pass::SetupStates()
{
#line 835
    D8SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    D8SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    D8SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    D8SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    D8SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    g_lpD3D9Device->SetTexture(0, NULL);
    g_lpD3D9Device->SetTexture(1, NULL);


    D8SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    D8SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    D8SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    D8SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    D8SetRenderState(D3DRS_ZENABLE, FALSE);
    D8SetRenderState(D3DRS_STENCILENABLE, FALSE);
    D8SetRenderState(D3DRS_STENCILWRITEMASK, 0);
    D8SetRenderState(D3DRS_FOGENABLE, FALSE);
    D8SetRenderState(D3DRS_COLORWRITEENABLE, 0xffffffff);
    D8_SetScreenPassStates(m_pShader);
    return TRUE;
}

inline BOOL cD8Effect_PostProcess::OnResetDevice()
{
    HRESULT hr = g_lpD3D9Device->CreatePixelShader(g_psBrightPass, &m_pBrightPass);
    if (FAILED(hr)) {
        ReportHResult(hr, "cD8Effect_PostProcess: CreatePixelShader 1 failed");
        return FALSE;
    }
    hr = g_lpD3D9Device->CreatePixelShader(g_psBlur, &m_pBlur);
    if (FAILED(hr)) {
        ReportHResult(hr, "cD8Effect_PostProcess: CreatePixelShader 2 failed");
        if (m_pBrightPass)
            m_pBrightPass->Release();
        m_pBrightPass = NULL;
        return FALSE;
    }
    const DWORD* apCombine[kNumCombineShaders] = { g_psBloomGamma, g_psBloom, g_psBloomScaleGamma };
    for (int i = 0; i < kNumCombineShaders; i++) {
        hr = g_lpD3D9Device->CreatePixelShader(apCombine[i], &m_pCombine[i]);
        if (FAILED(hr)) {
            char msg[128];
            sprintf(msg, "cD8Effect_PostProcess: CreatePixelShader %d failed", i + 3);
            ReportHResult(hr, msg);
            if (m_pBrightPass)
                m_pBrightPass->Release();
            m_pBrightPass = NULL;
            if (m_pBlur)
                m_pBlur->Release();
            m_pBlur = NULL;
            for (int j = 0; j < i; j++) {
                if (m_pCombine[j])
                    m_pCombine[j]->Release();
                m_pCombine[j] = NULL;
            }
            return FALSE;
        }
    }
    return CreateState();
}

inline BOOL cD8Effect_PostProcess::SetupStates()
{
#line 1442
    D8SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    D8SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    D8SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    D8SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    D8SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

    D8SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    D8SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    D8SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    D8SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    D8SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    g_lpD3D9Device->SetTexture(0, NULL);
    g_lpD3D9Device->SetTexture(1, NULL);


    D8SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    D8SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    D8SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    D8SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    D8SetRenderState(D3DRS_ZENABLE, FALSE);
    D8SetRenderState(D3DRS_STENCILENABLE, FALSE);
    D8SetRenderState(D3DRS_STENCILWRITEMASK, 0);
    D8SetRenderState(D3DRS_FOGENABLE, FALSE);
    D8SetRenderState(D3DRS_COLORWRITEENABLE, 0xffffffff);
    D8_SetScreenPassStates(m_pBrightPass);
    return TRUE;
}

inline BOOL cD8Effect_ColorCorrection::SetupStates()
{
#line 2317
    D8SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    D8SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    D8SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    D8SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    D8SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    g_lpD3D9Device->SetTexture(0, NULL);
    g_lpD3D9Device->SetTexture(1, NULL);


    D8SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    D8SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    D8SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    D8SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    D8SetRenderState(D3DRS_ZENABLE, FALSE);
    D8SetRenderState(D3DRS_STENCILENABLE, FALSE);
    D8SetRenderState(D3DRS_STENCILWRITEMASK, 0);
    D8SetRenderState(D3DRS_FOGENABLE, FALSE);
    D8SetRenderState(D3DRS_COLORWRITEENABLE, 0xffffffff);
    D8_SetScreenPassStates(NULL);
    return TRUE;
}
