


#pragma once

#include <lgd3d.h>


typedef struct d3d_cookie {
    union {
        struct {
            unsigned char wlog;
            unsigned char hlog;
            unsigned char flags;
            unsigned char palette;
        };
        unsigned int value;
    };
} d3d_cookie;

struct grs_bitmap;
typedef struct tdrv_texture_info tdrv_texture_info;

struct _D3DTLVERTEX;
struct r3ixs_info;
struct IWinDisplayDevice;


class cD6Frame;
class cD6Renderer;
class cD6Primitives;
class cD6States;
class cD6OverlayHandler;

struct sTextureData {
    IDirect3DTexture2* lpTexture;
    IDirectDrawSurface4* lpSurface;
    grs_bitmap* pTdrvBitmap;
    d3d_cookie TdrvCookie;
};

struct cliprect { float left; float right; float top; float bot; };

extern cD6Frame* pcFrame;
extern cD6Renderer* pcRenderer;
extern cD6Primitives* pcRenderBuffer;
extern cD6States* pcStates;
extern cD6OverlayHandler* pcOverlayHandler;
extern int lgd3d_g_bInitialized;
extern int g_bWFog;
extern int g_bUseDepthBuffer;
extern int g_bUseTableFog;
extern int g_bUseVertexFog;
extern int g_b8888supported;
extern int g_bPrefer_RGB;
extern int g_bTexSuspended;
extern int bSpewOn;
extern int zlinear;
extern D3DDEVICEDESC g_sD3DDevDesc;
extern DDSURFACEDESC2 g_sDescOfRenderBuffer;
extern DDPIXELFORMAT g_sDDPFDepth;
extern DDPIXELFORMAT g_RGBTextureFormat;
extern DDPIXELFORMAT g_AlphaTextureFormat;
extern DDPIXELFORMAT g_PalTextureFormat;
extern DDPIXELFORMAT g_TransRGBTextureFormat;
extern DDPIXELFORMAT g_8888TexFormat;
extern DDPIXELFORMAT* g_FormatList[5];
extern IDirect3D3* g_lpD3D;
extern IDirect3DDevice3* g_lpD3Ddevice;
extern IDirect3DViewport3* g_lpViewport;
extern IDirect3DMaterial3* g_lpBackgroundMaterial;
extern unsigned long g_hBackgroundMaterial;
extern IDirectDraw4* g_lpDD_ext;
extern IDirectDrawSurface4* g_lpRenderBuffer;
extern IDirectDrawSurface4* g_lpDepthBuffer;
extern sTextureData g_saTextures[1024];
extern grs_bitmap* default_bm;
extern unsigned char* texture_clut;
extern unsigned long g_dwRSChangeFlags;
extern float g_XOffset;
extern float g_YOffset;
extern unsigned long g_dwScreenWidth;
extern unsigned long g_dwScreenHeight;
extern double z1;
extern double z2;
extern double z2d;
extern double w2d;
extern double inv_z_far;
extern double z_near;
extern double z_far;
extern double zbias;
extern cliprect lgd3d_clip;

void SetLGD3DErrorCode(unsigned long code, long hError);
char* GetLgd3dErrorCode(unsigned long code);
char* GetDDErrorMsg(long hr);

#ifdef __cplusplus
extern "C" {
#endif

extern BOOL lgd3d_z_normal;
extern BOOL lgd3d_punt_d3d;
extern texture_manager* g_tmgr;

#ifdef __cplusplus
}
#endif

struct lgd3ds_device_info;
struct _GUID;
class cD6Frame {
public:
    unsigned long m_dwRequestedFlags;
    BOOL m_bDepthBuffer;
    unsigned long m_dwTextureOpCaps;
    IWinDisplayDevice* m_pWinDisplayDevice;
    cD6Frame(unsigned long dwWidth, unsigned long dwHeight, struct lgd3ds_device_info* psDeviceInfo);
    cD6Frame(struct ILGSurface* pILGSurface);
    ~cD6Frame();
    long CreateDepthBuffer();
    long Init(struct ILGSurface* pSurface);

private:
    void InitializeGlobals(unsigned long dwWidth, unsigned long dwHeight, unsigned long dwFlags);
    long GetDDstuffFromDisplay();
    void InitializeEnvironment(struct lgd3ds_device_info* pDevInfo);
    int CreateD3D(const struct _GUID& guid);
    void ExamineRenderingCapabilities();
};

class cD6Renderer {
protected:
    cD6Renderer(unsigned long mode, unsigned long* pInitFlags);
    ~cD6Renderer();
public:
    static cD6Renderer* Instance(unsigned long mode, unsigned long* pInitFlags);
    static cD6Renderer* DeInstance();
    void EndFrame();
    void CleanDepthBuffer(int x0, int y0, int x1, int y1);
    void CleanRenderSurface(BOOL bDepthBuffToo);
    void StartFrame(int frame);
    BOOL SwitchOverlaysOnOff(BOOL bOverlaysOn);

    long InitViewport(void);

    size_t m_dwEntrySize;
    unsigned long m_dwNoEntries;
    unsigned long m_dwStackTop;
    char* m_pStackData;
    unsigned long* m_pdwRSCData;
    char* m_pCurrentEntry;
    char* m_pSetEntry;
    BOOL m_bOverlaysOn;

    static cD6Renderer* m_RendererInstance;

private:
    void DeletStatesStack();
    void CreateStatesStack(unsigned long depth, size_t size);
    void R_EnableAlphaBlending(BOOL val);
};

class cD6Primitives {
protected:
    cD6Primitives();
    cD6Primitives(int nAlpha);
    ~cD6Primitives();
public:
    virtual cD6Primitives* DeInstance() = 0;
    virtual void FlushPrimitives();
    virtual void StartFrame();
    virtual void EndFrame();
    virtual void Clear(int);
    virtual void StartNonTexMode();
    virtual void EndNonTexMode();
    virtual _D3DTLVERTEX* ReservePointSlots(int = 0);
    virtual void FlushPoints();
    void SetPointBufferSize(ulong dwSize);
    ulong GetPointBufferSize();
    virtual int DrawPoint(_r3s_point*);
    virtual void DrawPoly(int);
    virtual int Poly(int, _r3s_point**);
    virtual int SPoly(int, _r3s_point**);
    virtual int RGB_Poly(int, _r3s_point**);
    virtual int RGBA_Poly(int, _r3s_point**);
    virtual int Trifan(int, _r3s_point**);
    virtual int LitTrifan(int, _r3s_point**);
    virtual int RGBlitTrifan(int, _r3s_point**);
    virtual int RGBAlitTrifan(int, _r3s_point**);
    virtual int RGBAFogLitTrifan(int, _r3s_point**);
    virtual int DiffuseSpecularLitTrifan(int, _r3s_point**);
    virtual int g2UPoly(int, g2s_point**);
    virtual int g2Poly(int, g2s_point**);
    virtual int g2UTrifan(int, g2s_point**);
    virtual int g2Trifan(int, g2s_point**);
    virtual void InitializeVIBCounters();
    virtual int CreateVertIndBuffer(unsigned long size = 0);
    virtual void DeleteVertIndBuffer();
    virtual int ResizeVertIndBuffer(unsigned long);
    virtual void EndIndexedRun();
    virtual void FlushIfNoFit(int, int);
    virtual _D3DTLVERTEX* GetIndPolySlot(int, r3ixs_info*);
    virtual void DrawIndPolies();
    virtual void FlushIndPolies();
    virtual int PolyInd(int, _r3s_point**, r3ixs_info*);
    virtual int SPolyInd(int, _r3s_point**, r3ixs_info*);
    virtual int RGB_PolyInd(int, _r3s_point**, r3ixs_info*);
    virtual int RGBA_PolyInd(int, _r3s_point**, r3ixs_info*);
    virtual int TrifanInd(int, _r3s_point**, r3ixs_info*);
    virtual int LitTrifanInd(int, _r3s_point**, r3ixs_info*);
    virtual int RGBlitTrifanInd(int, _r3s_point**, r3ixs_info*);
    virtual int RGBAlitTrifanInd(int, _r3s_point**, r3ixs_info*);
    virtual int TrifanMTD(int, _r3s_point**, LGD3D_tex_coord**);
    virtual int LitTrifanMTD(int, _r3s_point**, LGD3D_tex_coord**);
    virtual int RGBlitTrifanMTD(int, _r3s_point**, LGD3D_tex_coord**);
    virtual int RGBAlitTrifanMTD(int, _r3s_point**, LGD3D_tex_coord**);
    virtual int RGBAFogLitTrifanMTD(int, _r3s_point**, LGD3D_tex_coord**);
    virtual int DiffuseSpecularLitTrifanMTD(int, _r3s_point**, LGD3D_tex_coord**);
    virtual int g2UTrifanMTD(int, g2s_point**, LGD3D_tex_coord**);
    virtual int g2TrifanMTD(int, g2s_point**, LGD3D_tex_coord**);

    // Non-virtual:
    int GetPolyMode();
    int SetPolyMode(int mode);
    void DrawLine(_r3s_point* p0, _r3s_point* p1);
    _D3DTLVERTEX* ReservePolySlots(int = 0);
    void HackLight(_r3s_point* p, float r);
    void HackLightExtra(_r3s_point* p, float r, grs_bitmap* bm);
    void PassFogSpecularColor(DWORD);
    void PassAlphaColor(int val);
    void init_hack_light_bm();
    void do_quad_light(_r3s_point* p, float r, grs_bitmap* bm);
    int RGBAFogLitTrifanInd(int n, _r3s_point** ppl, r3ixs_info* info);
    int DiffuseSpecularLitTrifanInd(int n, _r3s_point** ppl, r3ixs_info* info);
    void DrawStandardEdges(void* param, unsigned long len);

    int m_bPrimitivesPending;
    int m_bFlushing;
    int m_nAlpha;
    DWORD m_dcFogSpecular;
    int m_bPointMode;
    int m_iSavedTexId;
    int m_ePolyMode;
    DWORD m_dwNoCashedPoints;
    DWORD m_dwPointBufferSize;
    _D3DTLVERTEX m_saPointBuffer[50];
    _D3DTLVERTEX m_saVertexBuffer[50];
    DWORD m_dwNoCashedVertices;
    DWORD m_dwVertexBufferSize;
    _D3DTLVERTEX* m_pVIB;
    DWORD m_dwNoVIBEntries;
    DWORD m_dwVIBSizeInEntries;
    WORD m_saIndexBuffer[256];
    DWORD m_dwVIBmax;
    DWORD m_dwNoIndices;
    WORD m_waTempIndices[50];
    DWORD m_dwMinVIndex;
    DWORD m_dwMaxVIndex;
    DWORD m_dwTempIndCounter;
    grs_bitmap* m_hack_light_bm;
};

struct sTexBlendArgs {
    D3DTEXTUREOP eColorOperation;
    ulong dwColorArg1;
    ulong dwColorArg2;
    D3DTEXTUREOP eAlphaOperation;
    ulong dwAlphaArg1;
    ulong dwAlphaArg2;
};

struct sRenderStates {
    BOOL bUsePalette;
    int nAlphaColor;
    D3DSHADEMODE eShadeMode;
    BOOL bDitheringOn;
    BOOL bAntialiasingOn;
    BOOL bAlphaOn;
    D3DBLEND eSrcAlpha;
    D3DBLEND eDstAlpha;
    D3DZBUFFERTYPE eZenable;
    BOOL bZWriteEnable;
    D3DCMPFUNC eZCompareFunc;
    BOOL bFogOn;
    ulong dwFogColor;
    D3DFOGMODE eFogMode;
    float fFogTableDensity;
    float fFogStart;
    float fFogEnd;
    ulong dcFogSpecular;
    BOOL bPerspectiveCorr;
    ulong dwNoLightMapLevels;
    int dwTexBlendMode[2];
    int naTextureId[2];
    BOOL baTextureWithAlpha[2];
    BOOL baChromaKeying[2];
    sTexBlendArgs saTexBlend[2];
    D3DTEXTUREADDRESS eWrap[2];
    int chroma_r;
    int chroma_g;
    int chroma_b;
    ulong chroma_key;
    D3DTEXTUREMINFILTER eMagTexFilter;
    D3DTEXTUREMINFILTER eMinTexFilter;
};

class cD6States {
public:
    virtual int  Initialize(DWORD dwRequestedFlags);
protected:
    virtual int  SetDefaultsStates(DWORD dwRequestedFlags);
public:
    virtual void set_texture_id(int n);
    virtual int  reload_texture(tdrv_texture_info *info);
    virtual void cook_info(tdrv_texture_info *info);
    virtual void SetLightMapMode(ulong flag);
    virtual void SetTextureLevel(int n);
    virtual int  EnableMTMode(DWORD dwMTOn);
    virtual void TurnOffTexuring(int bTexOff);
    virtual cD6States* DeInstance() = 0;
    virtual void EnableDepthBuffer(int val);
    virtual void SetZWrite(BOOL val);
    virtual void SetZCompare(BOOL val);
    virtual void SetFogDensity(float density);
    virtual void SetChromaKey(int r, int g, int b);

    // Texture driver interface (non-virtual — called through g_driver on cD6States*)
    int load_texture(tdrv_texture_info *info);
    void release_texture(int n);
    void unload_texture(int n);
    void synchronize(void);
    void start_frame(int n);
    void end_frame(void);
    int load_texture_a(tdrv_texture_info *info);
    int reload_texture_a(tdrv_texture_info *info);
    void release_texture_a(int n);
    void SetTextureNow(void);

    void SetAntialiasing(int bOn);
    void SetAlphaColor(float alpha);
    void SetLinearFogDistance(float fDistance);
    int SetSmoothShading(BOOL bSmooth);
    void SetAlphaPalette(ushort* pal);
    void SetPalSlotFlags(uint start, uint n, uchar* pal, int slot, int flags);
    void SetTexturePalette(uint start, uint n, uchar* pal, int slot);
    void SetFogColor(int r, int g, int b);
    void EnableFog(BOOL enable);
    void EnableAlphaBlending(BOOL enable);
    void EnablePalette(BOOL enable);
    BOOL IsPaletteOn();
    int EnableSpecular(BOOL enable);
    void SetAlphaModulateHack(uint mode);
    void ResetDefaultAlphaModulate();
    void SetFogSpecularLevel(float level);
    void SetDithering(BOOL bOn);
    void GetTexBlendingModes(ulong* p0, ulong* p1);
    int UseLinearTableFog(BOOL bUseIt);
    void SetFogStartAndEnd(float fStart, float fEnd);
    void SetTextureMapMode(ulong flag);
    int  IsZCompareOn() { return m_psCurrentRS->eZCompareFunc == 4; }
    int  IsZWriteOn() { return m_psCurrentRS->bZWriteEnable; }
    void SetPointerToSetStates(char* pData);
    void* GetCurrentStates();
    int  GetAntialiasingState() { return m_psCurrentRS->bAntialiasingOn; }
    int  GetDepthBufferState() { return m_psCurrentRS->eZenable; }
    int  GetDitheringState() { return m_psCurrentRS->bDitheringOn; }
    BOOL GetTexWrapping(DWORD dwLevel);
    int  IsAlphaBlendingOn() { return m_psCurrentRS->bAlphaOn; }
    int  IsFogOn() { return m_psCurrentRS->bFogOn; }
    int  IsSmoothShadingOn() { return m_psCurrentRS->eShadeMode == 2; }
    BOOL SetTexWrapping(DWORD dwLevel, BOOL bSetSmooth);

    HRESULT CreateDDSurface(d3d_cookie dwCookie, DDSURFACEDESC2* pddsd, IDirectDrawSurface4** ppSurface);
    void LoadSurface(tdrv_texture_info* info, _DDSURFACEDESC2* ddsd);

    int get_texture_id();
    DWORD get_color();
protected:
    cD6States();
public:
    void SetPointerToCurrentStates(char* pData);
    ulong m_DeviceSurfaceCaps;
    int* m_texture_size_list;
    ulong m_texture_caps;
#ifdef NEWDARK
    int nd_14, nd_18, nd_1C, nd_20, nd_24, nd_28, nd_2C, nd_30, nd_34, nd_38;
#endif
    BOOL m_bTextureListInitialized;
    sRenderStates* m_psCurrentRS;
    sRenderStates* m_psSetRS;
    BOOL m_bTexture_RGB;
    BOOL m_bUsingLocalMem;
    BOOL m_bLocalMem_available;
    BOOL m_bAGP_available;
    BOOL m_bWBuffer;
    BOOL m_bCanDither;
    BOOL m_bCanAntialias;
    BOOL m_bSpecular;
    BOOL m_bCanModulate;
    BOOL m_bCanModulateAlpha;

    float LinearWorldIntoFogCoef(float param);

    unsigned long GetRenderStatesSize();

protected:
    void EnumerateTextureFormats();
    void InitTextureManager();
    void SetCommonDefaultStates(DWORD dwRequestedFlags, int bMultiTexture);
};

class cImStates : public cD6States
{
public:
    static cD6States* Instance();
    virtual cD6States* DeInstance();
protected:
    cImStates();
    ~cImStates();
private:
    static cImStates* m_Instance;
};

class cMSStates : public cD6States
{
public:
    static cD6States* Instance();
    virtual cD6States* DeInstance();
    int Initialize(DWORD);
    int SetDefaultsStates(DWORD dwRequestedFlags);
    void set_texture_id(int);
    int EnableMTMode(DWORD);
    void TurnOffTexuring(int bTexOff);
    void SetTextureNow();
    int reload_texture(tdrv_texture_info* info);
    void cook_info(tdrv_texture_info* info);
    void SetLightMapMode(ulong);
    void SetTextureLevel(int);

    DWORD m_dwCurrentTexLevel;
    int m_bTexturePending;
    grs_bitmap* m_LastLightMapBm;
protected:
    cMSStates();
    ~cMSStates();
private:
    static cMSStates* m_Instance;
};
