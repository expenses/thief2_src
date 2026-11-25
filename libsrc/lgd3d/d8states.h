#pragma once

#include <d8dx6.h>
#include <d6states.h>
#include <d3dx9effect.h>

#define D8F_NO_SCENE_TEXTURE    0x1
#define D8F_NO_LOW_TEXTURE      0x2
#define D8F_NO_BLUR_TEXTURE     0x4
#define D8F_NO_MID_TEXTURE      0x8
#define D8F_NO_SCENE_COPY       0x10

enum eD8SceneTexture {
    kD8SceneFull,
    kD8SceneMid,
    kD8SceneLow,
    kD8SceneBlur,
    kD8SceneMidTemp,
    kD8SceneBlurTemp,
    kD8NumSceneTextures
};

#define D8_SCENE_BIT(n) (1 << (n))

extern IDirect3D9* g_lpD3D9;
extern IDirect3DDevice9* g_lpD3D9Device;
extern IDirect3DSurface9* g_lpSceneSurface;
extern IDirect3DTexture9* g_lpSceneTextures[kD8NumSceneTextures];
extern D3DSURFACE_DESC g_sSceneTextureDesc[kD8NumSceneTextures];
extern D3DSURFACE_DESC g_sBackBufferDesc;
extern D3DCAPS9 g_sD3D9Caps;
extern DWORD g_dwD8Flags;
extern DWORD g_dwD8CurrentFVF;

#define D8_TLVERTEX_FVF (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1)
#define D8_MT_TLVERTEX_FVF (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2)

inline void D8SetFVF(DWORD dwFVF)
{
    if (g_dwD8CurrentFVF != dwFVF) {
        g_dwD8CurrentFVF = dwFVF;
        g_lpD3D9Device->SetFVF(dwFVF);
    }
}
#define PF_GENERIC      0
#define PF_RGB          1
#define PF_ALPHA        2
#define PF_TRANS        0x80
#define PF_ALPHA_1BIT   0x20
#define PF_PALETTIZED   0x40
#define PF_FORMAT_MASK  0x07
#define PF_ALPHA_FORMAT(f) ((f) == 2 || (f) == 3 || (f) == 4)

extern int g_nD8ActiveEffect;
extern BOOL g_bD8EffectTexture;
extern BOOL g_bD8ManagedTextures;
extern int g_bD8ScaleRects;
extern float g_fD8RectScaleX;
extern float g_fD8RectScaleY;
extern float g_fD8ScreenScaleX;
extern float g_fD8ScreenScaleY;
extern float g_fD8TexelCenterX;
extern float g_fD8TexelCenterY;

extern const DWORD g_fxColorCorrection[0x12ac / sizeof(DWORD)];
HRESULT D8_CreateColorCorrectionEffect(ID3DXEffect** ppEffect);
BOOL D8_ColorCorrectionActive();
BOOL D8_PostProcessEnabled();
BOOL D8_BloomEnabled();
BOOL D8_RunPostProcess(IDirect3DTexture9* pScene);
BOOL D8_ApplyColorCorrection(IDirect3DSurface9* pSource);
BOOL D8_RenderBloom(IDirect3DTexture9* pSource, IDirect3DTexture9* pBloom, IDirect3DTexture9* pTarget);

class cD8States : public cD6States {
public:
    virtual int  Initialize(DWORD dwRequestedFlags);
    virtual void ReleaseAllTextures();
    virtual int  load_texture(tdrv_texture_info* info);
    virtual void release_texture(int n);
    virtual void set_texture_id(int n);
    virtual int  reload_texture(tdrv_texture_info* info);
    virtual void TurnOffTexuring(int bTexOff);
    virtual void SetTextureNow(void);
    virtual void EnableDepthBuffer(int val);
    virtual void SetZWrite(BOOL val);
    virtual void SetZCompare(BOOL val);
    virtual void SetFogDensity(float density);
    virtual int  UseLinearTableFog(BOOL bUseIt);
    virtual void SetFogStartAndEnd(float fStart, float fEnd);
    virtual void SetAlphaColor(float alpha);
    virtual void EnableAlphaBlending(BOOL enable);
    virtual void SetAlphaModulateHack(uint mode);
    virtual void ResetDefaultAlphaModulate();
    virtual void EnableFog(BOOL enable);
    virtual void SetFogColor(int r, int g, int b);
    virtual void SetTexturePalette(uint start, uint n, uchar* pal, int slot);
    virtual BOOL SetTexWrapping(DWORD dwLevel, BOOL bSetSmooth);
    virtual void ApplyTextureFilter();
    virtual void ApplyLightmapFilter();
    virtual void SetCullMode(DWORD dwCull);
    virtual void ResetAlphaTest();
    virtual void SetAlphaTestRef(int nRef);
    virtual void SetTrilinearFiltering(BOOL bOn);
    virtual void SetMipmapBias(float fBias);
    virtual void SetTextureMipmapBias(float fBias);
    virtual void SetTextureFilterMode(int nMode);
    virtual void SetLightmapFilterMode(int nMode);
    virtual void SetColorWriteEnable(DWORD dwMask);
    virtual void OnLostDevice();
    virtual void OnResetDevice();
    virtual void SetRenderStates(sRenderStates* pStates);
    virtual void SetAlphaToCoverage(BOOL bOn);

    void SetTexture(DWORD dwStage, IDirect3DBaseTexture9* pTexture);

protected:
    cD8States();
    ~cD8States();
    void EnumerateTextureFormats();
    void SetCommonDefaultStates(DWORD dwRequestedFlags, int bMultiTexture);
    void SetAlphaTestEnable(BOOL bOn);
    void UpdateAlphaTest(BOOL bOn);
    void SetAlphaRef(int nRef);

    BOOL m_bAlphaModulated;
    DWORD m_dwRequestedFlags;
    int m_nAlphaToCoverage;
    IDirect3DBaseTexture9* m_pCurTexture[2];
    int m_nCurAlphaRef;

    friend int D8_DeviceReset(BOOL bRestore);
};

class cD8ImStates : public cD8States {
public:
    static cD6States* Instance();
    virtual cD6States* DeInstance();
protected:
    virtual int  SetDefaultsStates(DWORD dwRequestedFlags);
    cD8ImStates() {}
    virtual ~cD8ImStates();
private:
    static cD8ImStates* m_Instance;
};

class cD8MSStates : public cD8States {
public:
    static cD6States* Instance();
    virtual cD6States* DeInstance();
    virtual int  Initialize(DWORD dwRequestedFlags);
    virtual void set_texture_id(int n);
    virtual void cook_info(tdrv_texture_info* info);
    virtual void SetLightMapMode(ulong flag);
    virtual void SetTextureLevel(int n);
    virtual int  EnableMTMode(DWORD dwMTOn);
    virtual void TurnOffTexuring(int bTexOff);
    virtual DWORD GetTextureLevel();
protected:
    virtual int  SetDefaultsStates(DWORD dwRequestedFlags);
    cD8MSStates() : m_dwCurrentTexLevel(0), m_bTexturePending(0), m_LastLightMapBm(NULL) {}
    virtual ~cD8MSStates();

    DWORD m_dwCurrentTexLevel;
    int m_bTexturePending;
    grs_bitmap* m_LastLightMapBm;
    static cD8MSStates* m_Instance;

    friend class cD8MSBatchBuffer;
};

class cD8MSBatchStates : public cD8MSStates {
public:
    cD8MSBatchStates() : m_bBatching(FALSE) {}
    virtual void set_texture_id(int n);
    virtual void SetAlphaColor(float alpha);

    BOOL m_bBatching;
};

int D8_DeviceReset(BOOL bRestore);
BOOL D8_CreateBitmapTexture(IDirect3DDevice9* pDevice, IDirect3DTexture9** ppTexture, DWORD dwFlags, grs_bitmap* pBm);

class cD8Frame : public cD6Frame {
public:
    cD8Frame(unsigned long dwWidth, unsigned long dwHeight, struct lgd3ds_device_info* psDeviceInfo);
    virtual ~cD8Frame();
protected:
    virtual void InitializeGlobals(unsigned long dwWidth, unsigned long dwHeight, unsigned long dwFlags);
    virtual void InitializeEnvironment(struct lgd3ds_device_info* pDevInfo);
    virtual long GetDDstuffFromDisplay();
public:
    virtual void OnLostDevice();
    virtual BOOL OnResetDevice();
    virtual BOOL CreateSceneTextures(D3DSURFACE_DESC* pDesc, DWORD dwMask);
protected:
    BOOL CreateD3D();
    void ExamineRenderingCapabilities();
};


class cD8Renderer : public cD6Renderer {
protected:
    cD8Renderer(unsigned long mode, unsigned long* pInitFlags);
public:
    static cD6Renderer* Instance(unsigned long mode, unsigned long* pInitFlags);

    virtual void StartFrame(int frame);
    virtual void EndFrame();
    virtual HRESULT SetTransform(int nWhich, D3DMATRIX* pMatrix);
    virtual ~cD8Renderer();
    virtual void CreateStates(unsigned long mode, unsigned long* pInitFlags);
    virtual HRESULT Clear(DWORD dwCount, D3DRECT* pRects, DWORD dwFlags, D3DCOLOR dwColor, float dvZ, DWORD dwStencil);
    virtual BOOL UpdateSceneTexture(DWORD dwWhat, IDirect3DSurface9* pSource, BOOL bKeepScene);
    virtual void OnLostDevice();
    virtual BOOL OnResetDevice();

    void FlushPostProcessing(BOOL bKeepTarget);
    void ApplyGamma();

    BOOL m_bInScene;
    int m_nFrame;
    BOOL m_bDeviceLost;
    BOOL m_bPostProcess;
    IDirect3DSurface9* m_pSavedTarget;
    BOOL m_bTargetSwapped;
};
