



#include <windows.h>
#include <ddraw.h>
#include <d3d.h>

#include <mprintf.h>
#include <stdio.h>
#include <math.h>
#include <memall.h>
#include <tmpalloc.h>
#include <d3dmacs.h>
#include <r3ds.h>
#include <g2.h>
#include <lgassert.h>
#include <d6hrmsg.h>
#include <d6check.h>
#include <lgd3d.h>
#include <d6states.h>
#include <tmgr.h>
#include <tdrv.h>
#define USE_D3D2_API 1

#include <comtools.h>
#include <appagg.h>
#include <wdispapi.h>
#include <dbg.h>
#include <setup.h>
#include <texture.h>
#include <dev2d.h>
extern "C" BOOL g_bWD3D9Device;
extern "C" void setwbnf(double znear, double zfar);
cD6Frame* D8_CreateFrame(unsigned long dwWidth, unsigned long dwHeight, lgd3ds_device_info* psDeviceInfo);
#define LGD3D_INIT_ASSERT_LINE 603


#ifdef SHIP
static void __stdcall _LogWriteMsgFileLine(const char *, const char *, unsigned) {}
#else
extern void __stdcall _LogWriteMsgFileLine(const char *, const char *, unsigned);
#endif
#define DebugMsg(msg) _LogWriteMsgFileLine(msg, __FILE__, __LINE__)

#ifdef MONO_SPEW
#define put_mono(c) \
do { \
   uchar *p_mono = (uchar *)0xb0080; \
   *p_mono = c;                      \
} while (0)
#else
#define put_mono(c)
#endif

float g_fMaxZ = 1.0f;
extern "C" BOOL lgd3d_z_normal = TRUE;

static DWORD dwZBiasStackPtr = 0;
static double drZBiasStock[8];
static double drZBiasTable[17] =
{
   0.0,
   1.0/32768, 1.0/16384, 1.0/8192, 1.0/4096, 1.0/2048, 1.0/1024, 1.0/512, 1.0/256,
   1.0/128, 1.0/64, 1.0/32, 1.0/16, 1.0/8, 1.0/4, 1.0/2, 1.0,
};

struct ILGSurface;
typedef struct grs_rgb_bitmask grs_rgb_bitmask;
struct r3ixs_info;

#define CheckHResult(hRes, msg)
#define start(s)
#define end()

#define VTABLE_RELEASE(ptr) \
    (*(void(**)(void*))((int**)ptr)[0][2])(ptr)

#define VTABLE_CALL3(ptr, slot) \
    (*(void(**)(void*, int, void*))((int**)ptr)[0][slot / 4])

extern "C" void (*lgd3d_release_ip_func)(void) = NULL;











inline BOOL cD6Renderer::SwitchOverlaysOnOff(BOOL bOverlaysOn)
{
    BOOL old = m_bOverlaysOn;
    m_bOverlaysOn = bOverlaysOn;
    return old;
}



BOOL cD6States::GetTexWrapping(DWORD dwLevel)
{
    return m_psCurrentRS->eWrap[dwLevel] == 1;
}


int (*lgd3d_draw_poly_func)(int n, r3s_phandle *pl);
int (*lgd3d_draw_poly_indexed_func)(int n, r3s_phandle *pl, r3ixs_info *info);



extern "C" {



void lgd3d_set_RGB(void)
{
}

void lgd3d_set_hardware(void)
{
}

void lgd3d_set_software(void)
{
}

void lgd3d_texture_set_RGB(bool is_RGB)
{
    g_bPrefer_RGB = is_RGB;
}



BOOL lgd3d_is_RGB(void)
{
    return TRUE;
}

BOOL lgd3d_is_hardware(void)
{
    return TRUE;
}



int lgd3d_init(lgd3ds_device_info* device_info)
{
    if (pcFrame)
    {
#ifdef WARN_ON
        Warning(("lgd3d_init():: library already initialized!\n"));
#endif
#line LGD3D_INIT_ASSERT_LINE
        AssertMsg(lgd3d_g_bInitialized == 1, "Missuse of the variable\"lgd3d_g_bInitialized\"\n");
        return 1;
    }
    else
    {
        z2d = g_fMaxZ;
        lgd3d_g_bInitialized = 1;
        w2d = 1.0;
        if (g_bWD3D9Device)
            pcFrame = D8_CreateFrame(grd_cap->w, grd_cap->h, device_info);
        else
            pcFrame = new cD6Frame(grd_cap->w, grd_cap->h, device_info);
        if (lgd3d_g_bInitialized)
            setwbnf(z_near, z_far);
        if (!lgd3d_g_bInitialized)
            lgd3d_shutdown();
        return lgd3d_g_bInitialized;
    }
}

int lgd3d_attach_to_lgsurface(ILGSurface* pILGSurface)
{
    if (!pILGSurface)
    {
#ifdef WARN_ON
        Warning(("lgd3d_attach_to_lgsurface::called with NULL"));
#endif
        return 0;
    }
    if (lgd3d_g_bInitialized)
        lgd3d_shutdown();
    lgd3d_g_bInitialized = 1;
    pcFrame = new cD6Frame(pILGSurface);
    if (!lgd3d_g_bInitialized)
        lgd3d_shutdown();
    return lgd3d_g_bInitialized;
}

void lgd3d_clean_render_surface(BOOL bDepthBuffToo)
{
    pcRenderer->CleanRenderSurface(bDepthBuffToo);
}

void lgd3d_shutdown(void)
{
    if (pcFrame) {
        pcFrame->ReleaseRenderer();
#line 161
        delete pcFrame;
    }
    pcFrame = NULL;
    lgd3d_g_bInitialized = 0;
}

void lgd3d_start_frame(int frame)
{
    if (!pcFrame) return;
    pcRenderer->StartFrame(frame);
}

void lgd3d_end_frame(void)
{
    if (!pcFrame) return;
    pcRenderer->EndFrame();
}

BOOL lgd3d_overlays_master_switch(BOOL bOverlaysOn)
{
    return pcRenderer->SwitchOverlaysOnOff(bOverlaysOn);
}

void lgd3d_release_indexed_primitives(void)
{
    pcRenderBuffer->EndIndexedRun();
}

extern "C++" int lgd3d_rgb_indexed_poly(int n, r3s_point** vpl, r3ixs_info* info)
{
    return pcRenderBuffer->RGB_PolyInd(n, vpl, (r3ixs_info*)info);
}

extern "C++" int lgd3d_rgba_indexed_poly(int n, r3s_point** vpl, r3ixs_info* info)
{
    return pcRenderBuffer->RGBA_PolyInd(n, vpl, (r3ixs_info*)info);
}



void lgd3d_blit()
{
}



int lgd3d_indexed_poly(int n, r3s_point** vpl, r3ixs_info* info)
{
    return pcRenderBuffer->PolyInd(n, vpl, (r3ixs_info*)info);
}

void lgd3d_zclear(void)
{
    Warning(("zclear not yet implemented"));
}

typedef float D3DMATRIX_FLAT[16];

static inline void set1(D3DMATRIX_FLAT mat);

int lgd3d_indexed_spoly(int n, r3s_point** vpl, r3ixs_info* info)
{
    return pcRenderBuffer->SPolyInd(n, vpl, (r3ixs_info*)info);
}

void lgd3d_get_znearfar(double* pdZNear, double* pdZFar)
{
    if (pdZNear) *pdZNear = z_near;
    if (pdZFar) *pdZFar = z_far;
}

extern "C++" int lgd3d_indexed_trifan(int n, r3s_point** vpl, r3ixs_info* info)
{
    return pcRenderBuffer->TrifanInd(n, vpl, (r3ixs_info*)info);
}

int lgd3d_lit_indexed_trifan(int n, r3s_point** vpl, r3ixs_info* info)
{
    return pcRenderBuffer->LitTrifanInd(n, vpl, (r3ixs_info*)info);
}


void lgd3d_tmap_setup(grs_bitmap *bm)
{
    if (!pcFrame) return;
    g_tmgr->set_texture(bm);
    lgd3d_draw_poly_func = lgd3d_trifan;
    lgd3d_draw_poly_indexed_func = lgd3d_indexed_trifan;
}

static int lgd3d_rgblit_trifan(int n, r3s_point **ppl);
static int lgd3d_rgblit_indexed_trifan(int n, r3s_point** vpl, r3ixs_info* info);
static int lgd3d_rgbalit_trifan(int n, r3s_point **ppl);
static int lgd3d_rgbalit_indexed_trifan(int n, r3s_point** vpl, r3ixs_info* info);
static int lgd3d_rgbafoglit_trifan(int n, r3s_point **ppl);
static int lgd3d_rgbafoglit_indexed_trifan(int n, r3s_point** vpl, r3ixs_info* info);
static int lgd3d_diffspecular_trifan(int n, r3s_point **ppl);
static int lgd3d_diffspecular_indexed_trifan(int n, r3s_point** vpl, r3ixs_info* info);
static int lgd3d_rgb_poly(int n, r3s_point **ppl);
static int lgd3d_rgba_poly(int n, r3s_point **ppl);

void lgd3d_rgblit_tmap_setup(grs_bitmap *bm)
{
    if (!pcFrame) return;
    g_tmgr->set_texture(bm);
    lgd3d_draw_poly_func = lgd3d_rgblit_trifan;
    lgd3d_draw_poly_indexed_func = lgd3d_rgblit_indexed_trifan;
}

int lgd3d_get_depth_buffer_state(void)
{
    if (pcFrame)
        return pcStates->GetDepthBufferState();
    return -1;
}

static int lgd3d_rgblit_trifan(int n, r3s_point **ppl)
{
    return pcRenderBuffer->RGBlitTrifan(n, ppl);
}

static int lgd3d_rgblit_indexed_trifan(int n, r3s_point** vpl, r3ixs_info* info)
{
    return pcRenderBuffer->RGBlitTrifanInd(n, vpl, (r3ixs_info*)info);
}


int lgd3d_set_shading(BOOL bSmoothShading)
{
    if (pcFrame)
        return pcStates->SetSmoothShading(bSmoothShading);
    return -1;
}

BOOL lgd3d_is_smooth_shading_on(void)
{
    if (pcFrame)
        return pcStates->IsSmoothShadingOn();
    return -1;
}


int lgd3d_is_alpha_blending_on(void)
{
    if (pcFrame)
        return pcStates->IsAlphaBlendingOn();
    return -1;
}

void lgd3d_rgbalit_tmap_setup(grs_bitmap* bm)
{
    if (!pcFrame) return;
    g_tmgr->set_texture(bm);
    lgd3d_draw_poly_func = lgd3d_rgbalit_trifan;
    lgd3d_draw_poly_indexed_func = lgd3d_rgbalit_indexed_trifan;
}

static int lgd3d_rgbalit_trifan(int n, r3s_point **ppl)
{
    return pcRenderBuffer->RGBAlitTrifan(n, ppl);
}

static int lgd3d_rgbalit_indexed_trifan(int n, r3s_point** vpl, r3ixs_info* info)
{
    return pcRenderBuffer->RGBAlitTrifanInd(n, vpl, (r3ixs_info*)info);
}

void lgd3d_rgbafoglit_tmap_setup(grs_bitmap* bm)
{
    if (!pcFrame) return;
    g_tmgr->set_texture(bm);
    lgd3d_draw_poly_func = lgd3d_rgbafoglit_trifan;
    lgd3d_draw_poly_indexed_func = lgd3d_rgbafoglit_indexed_trifan;
}

static int lgd3d_rgbafoglit_trifan(int n, r3s_point **ppl)
{
    return pcRenderBuffer->RGBAFogLitTrifan(n, ppl);
}

static int lgd3d_rgbafoglit_indexed_trifan(int n, r3s_point** vpl, r3ixs_info* info)
{
    return pcRenderBuffer->RGBAFogLitTrifanInd(n, vpl, (r3ixs_info*)info);
}

void lgd3d_diffspecular_tmap_setup(grs_bitmap* bm)
{
    if (!pcFrame) return;
    g_tmgr->set_texture(bm);
    lgd3d_draw_poly_func = lgd3d_diffspecular_trifan;
    lgd3d_draw_poly_indexed_func = lgd3d_diffspecular_indexed_trifan;
}

static int lgd3d_diffspecular_trifan(int n, r3s_point **ppl)
{
    return pcRenderBuffer->DiffuseSpecularLitTrifan(n, ppl);
}

static int lgd3d_diffspecular_indexed_trifan(int n, r3s_point** vpl, r3ixs_info* info)
{
    return pcRenderBuffer->DiffuseSpecularLitTrifanInd(n, vpl, (r3ixs_info*)info);
}


BOOL lgd3d_enable_specular(BOOL bUseIt)
{
    return pcStates->EnableSpecular(bUseIt);
}


void lgd3d_set_fog_level(float fl)
{
    if (!pcFrame) return;
    pcStates->SetFogSpecularLevel(fl);
}

void lgd3d_lit_tmap_setup(grs_bitmap *bm)
{
    if (!pcFrame) return;
    g_tmgr->set_texture(bm);
    lgd3d_draw_poly_func = lgd3d_lit_trifan;
    lgd3d_draw_poly_indexed_func = lgd3d_lit_indexed_trifan;
}

void lgd3d_poly_setup(grs_bitmap* bm)
{
    if (!pcFrame) return;
    pcStates->set_texture_id(0xffffffff);
    lgd3d_draw_poly_func = lgd3d_poly;
    lgd3d_draw_poly_indexed_func = lgd3d_indexed_poly;
}

void lgd3d_set_fog_density(float density)
{
    if (!pcFrame) return;
    pcStates->SetFogDensity(density);
}

void lgd3d_spoly_setup(grs_bitmap* bm)
{
    if (!pcFrame) return;
    pcStates->set_texture_id(0xffffffff);
    lgd3d_draw_poly_func = lgd3d_spoly;
    lgd3d_draw_poly_indexed_func = lgd3d_indexed_spoly;
}

void lgd3d_rgb_poly_setup(grs_bitmap* bm)
{
    if (!pcFrame) return;
    pcStates->set_texture_id(0xffffffff);
    lgd3d_draw_poly_func = lgd3d_rgb_poly;
    lgd3d_draw_poly_indexed_func = lgd3d_rgb_indexed_poly;
}

void lgd3d_set_fog_start_end(float fStart, float fEnd)
{
    if (!pcFrame) return;
    pcStates->SetFogStartAndEnd(fStart, fEnd);
}

static int lgd3d_rgb_poly(int n, r3s_point **ppl)
{
    return pcRenderBuffer->RGB_Poly(n, ppl);
}

void lgd3d_rgba_poly_setup(grs_bitmap* bm)
{
    if (!pcFrame) return;
    pcStates->set_texture_id(0xffffffff);
    lgd3d_draw_poly_func = lgd3d_rgba_poly;
    lgd3d_draw_poly_indexed_func = lgd3d_rgba_indexed_poly;
}

void lgd3d_get_texblending_modes(ulong* pulLevel0Mode, ulong* pulLevel1Mode)
{
    if (!pcFrame) return;
    pcStates->GetTexBlendingModes(pulLevel0Mode, pulLevel1Mode);
}

void lgd3d_hack_light(r3s_point* p, float r)
{
    if (!pcFrame) return;
    pcRenderBuffer->HackLight(p, r);
}

static int lgd3d_rgba_poly(int n, r3s_point **ppl)
{
    return pcRenderBuffer->RGBA_Poly(n, ppl);
}


BOOL lgd3d_get_texture_wrapping(DWORD dwLevel)
{
    if (pcFrame)
        return pcStates->GetTexWrapping(dwLevel);
    return -1;
}


int lgd3d_draw_point(r3s_point *p)
{
    return pcRenderBuffer->DrawPoint(p);
}


void lgd3d_set_pal(uint start, uint n, uchar* pal_data)
{
    lgd3d_set_pal_slot(start, n, pal_data, 0);
}

void lgd3d_set_zwrite(int bOn)
{
    if (!pcFrame) return;
    pcStates->SetZWrite(bOn);
}

void lgd3d_set_zcompare(int bOn)
{
    if (!pcFrame) return;
    pcStates->SetZCompare(bOn);
}

void lgd3d_set_znearfar(double znear, double zfar)
{
    z_near = znear;
    z_far = zfar;
    inv_z_far = 1.0 / z_far;
    z1 = zfar / (zfar - znear);
    z2 = znear * z1;
    z1 = z1 - zbias;
    setwbnf(znear, zfar);
}

extern "C" void setwbnf(double znear, double zfar)
{
    D3DMATRIX_FLAT mat1, mat2, mat3;
    if (!pcRenderer)
        return;
    set1(mat1);
    set1(mat2);
    set1(mat3);
    if (zfar <= znear)
        return;
    pcRenderer->SetTransform(0, (D3DMATRIX*)&mat1);
    pcRenderer->SetTransform(1, (D3DMATRIX*)&mat2);
    float t = (float)(zfar / (zfar - znear));
    mat3[10] = t;
    mat3[14] = (float)(-znear * t);
    mat3[11] = 1.0f;
    mat3[15] = 0.0f;
    pcRenderer->SetTransform(2, (D3DMATRIX*)&mat3);
}

void lgd3d_get_alpha_texture_bitmask(grs_rgb_bitmask* bitmask)
{
    extern DDPIXELFORMAT g_AlphaTextureFormat;
    if (g_AlphaTextureFormat.dwFlags == 0) {
        g_AlphaTextureFormat.dwBBitMask = 0;
        g_AlphaTextureFormat.dwGBitMask = g_AlphaTextureFormat.dwBBitMask;
        g_AlphaTextureFormat.dwRBitMask = g_AlphaTextureFormat.dwGBitMask;
        g_AlphaTextureFormat.dwRGBAlphaBitMask = g_AlphaTextureFormat.dwRBitMask;
    }
    ((DWORD*)bitmask)[3] = g_AlphaTextureFormat.dwRGBAlphaBitMask;
    ((DWORD*)bitmask)[0] = g_AlphaTextureFormat.dwRBitMask;
    ((DWORD*)bitmask)[1] = g_AlphaTextureFormat.dwGBitMask;
    ((DWORD*)bitmask)[2] = g_AlphaTextureFormat.dwBBitMask;
}

void lgd3d_set_texture_clut(uchar* clut)
{
    texture_clut = clut;
    if (g_tmgr)
        g_tmgr->set_clut(clut);
}

uchar* lgd3d_set_clut(uchar* clut)
{
    uchar *old_clut = lgd3d_clut;
    lgd3d_clut = clut;
    lgd3d_set_texture_clut(clut);
    return old_clut;
}

void lgd3d_set_pal_slot(uint start, uint n, uchar* pal_data, int slot)
{
    lgd3d_set_pal_slot_flags(start, n, pal_data, slot, 3);
}

void lgd3d_set_pal_slot_flags(uint start, uint n, uchar* pal, int slot, int flags)
{
    if (!pcFrame) return;
    pcStates->SetPalSlotFlags(start, n, pal, slot, flags);
}

void lgd3d_get_trans_texture_bitmask(grs_rgb_bitmask* bitmask)
{
    extern DDPIXELFORMAT g_TransRGBTextureFormat;
    if (g_TransRGBTextureFormat.dwFlags == 0) {
        g_TransRGBTextureFormat.dwBBitMask = 0;
        g_TransRGBTextureFormat.dwGBitMask = g_TransRGBTextureFormat.dwBBitMask;
        g_TransRGBTextureFormat.dwRBitMask = g_TransRGBTextureFormat.dwGBitMask;
        g_TransRGBTextureFormat.dwRGBAlphaBitMask = g_TransRGBTextureFormat.dwRBitMask;
    }
    ((DWORD*)bitmask)[3] = g_TransRGBTextureFormat.dwRGBAlphaBitMask;
    ((DWORD*)bitmask)[0] = g_TransRGBTextureFormat.dwRBitMask;
    ((DWORD*)bitmask)[1] = g_TransRGBTextureFormat.dwGBitMask;
    ((DWORD*)bitmask)[2] = g_TransRGBTextureFormat.dwBBitMask;
}

void lgd3d_get_opaque_texture_bitmask(grs_rgb_bitmask* bitmask)
{
    extern DDPIXELFORMAT g_RGBTextureFormat;
    if (g_RGBTextureFormat.dwFlags == 0) {
        g_RGBTextureFormat.dwBBitMask = 0;
        g_RGBTextureFormat.dwGBitMask = g_RGBTextureFormat.dwBBitMask;
        g_RGBTextureFormat.dwRBitMask = g_RGBTextureFormat.dwGBitMask;
        g_RGBTextureFormat.dwRGBAlphaBitMask = g_RGBTextureFormat.dwRBitMask;
    }
    ((DWORD*)bitmask)[3] = g_RGBTextureFormat.dwRGBAlphaBitMask;
    ((DWORD*)bitmask)[0] = g_RGBTextureFormat.dwRBitMask;
    ((DWORD*)bitmask)[1] = g_RGBTextureFormat.dwGBitMask;
    ((DWORD*)bitmask)[2] = g_RGBTextureFormat.dwBBitMask;
}

void lgd3d_set_fog_color(int r, int g, int b)
{
    if (!pcFrame) return;
    pcStates->SetFogColor(r, g, b);
}

void lgd3d_get_offsets(int* x, int* y)
{
    if (x) *x = (int)g_XOffset;
    if (y) *y = (int)g_YOffset;
}

void lgd3d_set_fog_enable(BOOL enable)
{
    if (!pcFrame) return;
    pcStates->EnableFog(enable);
}


BOOL lgd3d_set_poly_mode(ePolyMode eNewMode)
{
    if (pcFrame)
        return pcRenderBuffer->SetPolyMode(eNewMode);
    return 0;
}

int cD6Primitives::GetPolyMode()
{
    return m_ePolyMode;
}

ePolyMode lgd3d_get_poly_mode()
{
    if (pcFrame)
        return (ePolyMode)pcRenderBuffer->GetPolyMode();
    return (ePolyMode)0;
}

int lgd3d_use_linear_table_fog(int bUseIt)
{
    return pcStates->UseLinearTableFog(bUseIt);
}

void lgd3d_set_linear_fog_distance(float fDistance)
{
    if (!pcFrame) return;
    pcStates->SetLinearFogDistance(fDistance);
}

void lgd3d_set_alpha(float alpha)
{
    if (!pcFrame) return;
    pcStates->SetAlphaColor(alpha);
}

void lgd3d_set_blend(BOOL blend_enable)
{
    if (!pcFrame) return;
    pcStates->EnableAlphaBlending(blend_enable);
}

void lgd3d_blend_multiply(int blend_mode)
{
    if (!pcFrame) return;
    pcStates->SetAlphaModulateHack(blend_mode);
}

int lgd3d_g2utrifan(int n, g2s_point **vpl)
{
    return pcRenderBuffer->g2UTrifan(n, vpl);
}

void lgd3d_clear_z_rect(int x0, int y0, int x1, int y1)
{
    if (!pcFrame) return;
    pcRenderer->CleanDepthBuffer(x0, y0, x1, y1);
}

void lgd3d_blend_normal(void)
{
    if (!pcFrame) return;
    pcStates->ResetDefaultAlphaModulate();
}


void lgd3d_set_offsets(int x, int y)
{
    g_fXOffsetBase = (float)x;
    g_fYOffsetBase = (float)y;
    g_XOffset = (float)x - g_fTexelCenterX;
    g_YOffset = (float)y - g_fTexelCenterY;
}

void lgd3d_set_z(float z)
{
    if (lgd3d_z_normal)
    {
        z2d = z;
        if (z2d >= 1.0)
            z2d = g_fMaxZ;
    }
    else
    {
        z2d = z1 - (z2 / z);
        if (z2d >= 1.0)
            z2d = g_fMaxZ;
        else if (z2d < 0.0)
            z2d = 0.0;
    }
    w2d = 1.0 / z;
}

void lgd3d_disable_palette(void)
{
    if (!pcFrame) return;
    pcStates->EnablePalette(FALSE);
}

void lgd3d_enable_palette(void)
{
    if (!pcFrame) return;
    pcStates->EnablePalette(TRUE);
}

void lgd3d_hack_light_extra(r3s_point* p, float r, grs_bitmap* bm)
{
    if (!pcFrame) return;
    pcRenderBuffer->HackLightExtra(p, r, bm);
}

void lgd3d_draw_line(r3s_point *p0, r3s_point *p1)
{
    if (!pcFrame) return;
    pcRenderBuffer->DrawLine(p0, p1);
}

extern int lgd3d_indexed_poly(int n, r3s_point** vpl, r3ixs_info* info);
extern int lgd3d_indexed_spoly(int n, r3s_point** vpl, r3ixs_info* info);
extern int lgd3d_lit_indexed_trifan(int n, r3s_point** vpl, r3ixs_info* info);
extern "C++" int lgd3d_rgb_indexed_poly(int n, r3s_point** vpl, r3ixs_info* info);
extern "C++" int lgd3d_rgba_indexed_poly(int n, r3s_point** vpl, r3ixs_info* info);
extern "C++" int lgd3d_indexed_trifan(int n, r3s_point** vpl, r3ixs_info* info);
extern int lgd3d_rgblit_indexed_trifan(int n, r3s_point** vpl, r3ixs_info* info);
extern int lgd3d_rgbalit_indexed_trifan(int n, r3s_point** vpl, r3ixs_info* info);
extern int lgd3d_rgbafoglit_indexed_trifan(int n, r3s_point** vpl, r3ixs_info* info);
extern int lgd3d_diffspecular_indexed_trifan(int n, r3s_point** vpl, r3ixs_info* info);


int lgd3d_trifan(int n, r3s_point **ppl)
{
    return pcRenderBuffer->Trifan(n, ppl);
}

int lgd3d_lit_trifan(int n, r3s_point **ppl)
{
    return pcRenderBuffer->LitTrifan(n, ppl);
}

int lgd3d_poly(int n, r3s_point **ppl)
{
    return pcRenderBuffer->Poly(n, ppl);
}

int lgd3d_spoly(int n, r3s_point **ppl)
{
    return pcRenderBuffer->SPoly(n, ppl);
}

int lgd3d_g2upoly(int n, g2s_point **vpl)
{
    return pcRenderBuffer->g2UPoly(n, vpl);
}

int lgd3d_g2poly(int n, g2s_point **vpl)
{
    return pcRenderBuffer->g2Poly(n, vpl);
}

int lgd3d_g2trifan(int n, g2s_point **vpl)
{
    return pcRenderBuffer->g2Trifan(n, vpl);
}

void lgd3d_set_light_map_method(ulong flag)
{
    pcStates->SetLightMapMode(flag);
}

int lgd3d_TrifanMTD(int n, r3s_point** ppl, LGD3D_tex_coord** pptc)
{
    return pcRenderBuffer->TrifanMTD(n, ppl, pptc);
}

void lgd3d_set_texture_level(int n)
{
    if (!pcFrame) return;
    pcStates->SetTextureLevel(n);
}




double lgd3d_set_zbias(double new_bias)
{
    double old_bias = zbias;
    z1 += old_bias - new_bias;
    zbias = new_bias;
    return old_bias;
}

void lgd3d_set_zbias_table(int nScale, double fRange)
{
    if (nScale < 1)
        nScale = 1;
    for (int i = 1; i <= 10; i++)
        drZBiasTable[i] = (double)(nScale << i) / fRange;
}

void lgd3d_pop_zbias(void)
{
    if (dwZBiasStackPtr == 0) {
#line 504
        CriticalMsg("Z-Bias stack underflow!");
        return;
    }
    zbias = drZBiasStock[dwZBiasStackPtr--];
    z1 = z_far / (z_far - z_near) - zbias;
}

void lgd3d_push_zbias_i(int nZBias)
{
    if (dwZBiasStackPtr > 6) {
#line 512
        CriticalMsg("Z-Bias stack overflow!");
        return;
    }
    drZBiasStock[++dwZBiasStackPtr] = zbias;
#line 520
    AssertMsg1(nZBias >= 0 && nZBias <= 16, "Z-Bias value %i is out of the [0,16] range", nZBias);
    zbias = drZBiasTable[nZBias];
    z1 = z_far / (z_far - z_near) - zbias;
}

int lgd3d_set_alpha_test_ref(int nRef)
{
    int nOld = g_nAlphaTestRef;
    if (pcStates)
        pcStates->SetAlphaTestRef(nRef);
    else
        g_nAlphaTestRef = nRef;
    return nOld;
}


void lgd3d_set_chromakey(int r, int g, int b)
{
    if (!pcFrame) return;
    pcStates->SetChromaKey(r, g, b);
}

void lgd3d_set_alpha_pal(ushort* pal)
{
    if (!pcFrame) return;
    pcStates->SetAlphaPalette(pal);
}

void lgd3d_clear(int c)
{
    if (!pcFrame) return;
    pcRenderBuffer->Clear(c);
}

int lgd3d_is_zwrite_on(void)
{
    if (pcFrame)
        return pcStates->IsZWriteOn();
    return -1;
}

int lgd3d_is_zcompare_on(void)
{
    if (pcFrame)
        return pcStates->IsZCompareOn();
    return -1;
}

int lgd3d_is_fog_on(void)
{
    if (pcFrame)
        return pcStates->IsFogOn();
    return -1;
}


void lgd3d_set_dithering(int bOn)
{
    if (!pcFrame) return;
    pcStates->SetDithering(bOn);
}

int lgd3d_is_dithering_on(void)
{
    return pcStates->GetDitheringState();
}


void lgd3d_set_texture_map_method(ulong flag)
{
    pcStates->SetTextureMapMode(flag);
}


void lgd3d_set_antialiasing(int bOn)
{
    if (!pcFrame) return;
    pcStates->SetAntialiasing(bOn);
}

int lgd3d_is_antialiasing_on(void)
{
    return pcStates->GetAntialiasingState();
}

int lgd3d_LitTrifanMTD(int n, r3s_point** ppl, LGD3D_tex_coord** pptc)
{
    return pcRenderBuffer->LitTrifanMTD(n, ppl, pptc);
}

int lgd3d_RGBlitTrifanMTD(int n, r3s_point** ppl, LGD3D_tex_coord** pptc)
{
    return pcRenderBuffer->RGBlitTrifanMTD(n, ppl, pptc);
}

int lgd3d_RGBAlitTrifanMTD(int n, r3s_point** ppl, LGD3D_tex_coord** pptc)
{
    return pcRenderBuffer->RGBAlitTrifanMTD(n, ppl, pptc);
}

int lgd3d_RGBAFoglitTrifanMTD(int n, r3s_point** ppl, LGD3D_tex_coord** pptc)
{
    return pcRenderBuffer->RGBAFogLitTrifanMTD(n, ppl, pptc);
}

int lgd3d_DiffuseSpecularMTD(int n, r3s_point** ppl, LGD3D_tex_coord** pptc)
{
    return pcRenderBuffer->DiffuseSpecularLitTrifanMTD(n, ppl, pptc);
}

int lgd3d_g2UTrifanMTD(int n, g2s_point** vpl, LGD3D_tex_coord** vptc)
{
    return pcRenderBuffer->g2UTrifanMTD(n, vpl, vptc);
}

int lgd3d_g2TrifanMTD(int n, g2s_point** vpl, LGD3D_tex_coord** vptc)
{
    return pcRenderBuffer->g2TrifanMTD(n, vpl, vptc);
}


typedef float D3DMATRIX_FLAT[16];

static inline void set1(D3DMATRIX_FLAT mat)
{
    mat[7] = 0.0f;
    mat[6] = 0.0f;
    mat[4] = 0.0f;
    mat[3] = 0.0f;
    mat[2] = 0.0f;
    mat[1] = 0.0f;
    mat[14] = 0.0f;
    mat[13] = 0.0f;
    mat[12] = 0.0f;
    mat[11] = 0.0f;
    mat[9] = 0.0f;
    mat[8] = 0.0f;
    mat[15] = 1.0f;
    mat[10] = 1.0f;
    mat[5] = 1.0f;
    mat[0] = 1.0f;
}

BOOL lgd3d_set_texture_wrapping(DWORD dwLevel, BOOL bSetSmooth)
{
    if (pcFrame)
        return pcStates->SetTexWrapping(dwLevel, bSetSmooth);
    return -1;
}

} // extern "C"
