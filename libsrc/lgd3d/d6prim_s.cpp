// $Header: x:/prj/tech/libsrc/lgd3d/RCS/render.c 1.26 1998/07/06 15:32:20 buzzard Exp $

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
extern "C" {
#include <g2.h>
}
#include <lgd3d.h>
#include <d6states.h>
#include <d6prim.h>
#include <r3d.h>
#include <memall.h>
#include <tmpalloc.h>
#include <lgassert.h>
#include <d6hrmsg.h>
#include <d6check.h>

static ushort hack_light_alpha_pal[16] =
{
   0x0fff, 0x1fff, 0x2fff, 0x3fff, 0x4fff, 0x5fff, 0x6fff, 0x7fff,
   0x8fff, 0x9fff, 0xafff, 0xbfff, 0xcfff, 0xdfff, 0xefff, 0xffff,
};

#define make_scolor(pColor, c0, intensity) \
do { \
   ulong __c0 = (c0); \
   float __i = (intensity); \
   *(pColor) = (((((ulong)(((__c0 >> 16) & 0xff) * __i)) << 8) + (ulong)(((__c0 >> 8) & 0xff) * __i)) << 8) + (ulong)((__c0 & 0xff) * __i) + (__c0 & 0xff000000); \
} while (0)


// Externs from dev2d and other modules
extern void (*gr_free)(void *p);
extern "C" void (*lgd3d_release_ip_func)(void);

// Global edge indices array for DrawStandardEdges (initialized in constructor)
static WORD waEdgeIndices[50];
cD6Primitives* pcRenderBuffer = NULL;

// Forward declaration from d6intfc.cpp
extern "C" void lgd3d_release_indexed_primitives(void);

#ifdef SHIP
#define PUNT_D3D 0
#else
#define PUNT_D3D lgd3d_punt_d3d
#endif



// Local struct definition for r3ixs_info (not in libsrc/r3d/r3ds.h)
struct r3ixs_info_local {
    union {
        unsigned long value;
        struct {
            short index;
            unsigned char flags;
            unsigned char pad;
        };
    };
};


cImBuffer* cImBuffer::m_Instance = NULL;

cD6Primitives::cD6Primitives():
    m_bFlushing(0),
    m_bPointMode(0),
    m_ePolyMode(1),
    m_dwNoCashedPoints(0),
    m_dwPointBufferSize(50),
    m_dwNoCashedVertices(0),
    m_dwVertexBufferSize(50),
    m_pVIB(NULL),
    m_hack_light_bm(NULL)
{
    memset(m_saVertexBuffer, 0, sizeof(m_saVertexBuffer));
    for (int i = 0; i < 50; i++) {
        waEdgeIndices[i] = (WORD)i;
    }
    CreateVertIndBuffer(16);
    lgd3d_release_ip_func = lgd3d_release_indexed_primitives;
}

cD6Primitives::cD6Primitives(int nAlpha)
{
}

void cD6Primitives::FlushPrimitives()
{
    if (m_bPrimitivesPending && !m_bFlushing) {
        m_bFlushing = 1;
        FlushIndPolies();
        FlushPoints();
        m_bPrimitivesPending = 0;
        m_bFlushing = 0;
    }
}
void cD6Primitives::EndIndexedRun()
{
    FlushIndPolies();
    InitializeVIBCounters();
}
int cD6Primitives::TrifanMTD(int count, r3s_point** points, LGD3D_tex_coord** tex_coords)
{
    return Trifan(count, points);
}
int cD6Primitives::LitTrifanMTD(int count, r3s_point** points, LGD3D_tex_coord** tex_coords)
{
    return LitTrifan(count, points);
}


cMSBuffer* cMSBuffer::m_Instance = NULL;

int cD6Primitives::RGBlitTrifanMTD(int count, r3s_point** points, LGD3D_tex_coord** tex_coords)
{
    return RGBlitTrifan(count, points);
}
int cD6Primitives::RGBAlitTrifanMTD(int count, r3s_point** points, LGD3D_tex_coord** tex_coords)
{
    return RGBAlitTrifan(count, points);
}
int cD6Primitives::RGBAFogLitTrifanMTD(int count, r3s_point** points, LGD3D_tex_coord** tex_coords)
{
    return RGBAFogLitTrifan(count, points);
}
int cD6Primitives::DiffuseSpecularLitTrifanMTD(int count, r3s_point** points, LGD3D_tex_coord** tex_coords)
{
    return DiffuseSpecularLitTrifan(count, points);
}
int cD6Primitives::g2UTrifanMTD(int count, g2s_point** points, LGD3D_tex_coord** tex_coords)
{
    return g2UTrifan(count, points);
}
int cD6Primitives::g2TrifanMTD(int count, g2s_point** points, LGD3D_tex_coord** tex_coords)
{
    return g2Trifan(count, points);
}

cD6Primitives::~cD6Primitives()
{
    if (m_hack_light_bm) {
        if (g_tmgr) {
            (*g_tmgr->unload_texture)(m_hack_light_bm);
        }
        (*gr_free)(m_hack_light_bm);
        m_hack_light_bm = NULL;
    }
    DeleteVertIndBuffer();
    lgd3d_release_ip_func = NULL;
}

void cD6Primitives::DrawStandardEdges(void* pVertera, unsigned long dwNoVeriteces) {
    if (g_bTexSuspended == 0) {
        FlushIndPolies();
        StartNonTexMode();
    }
    waEdgeIndices[dwNoVeriteces] = 0;
    int hr = ((LPDIRECT3DDEVICE3)g_lpD3Ddevice)->DrawIndexedPrimitive(
        D3DPT_LINESTRIP, D3DFVF_TLVERTEX, pVertera, dwNoVeriteces, waEdgeIndices, dwNoVeriteces + 1, D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS);
    waEdgeIndices[dwNoVeriteces] = (WORD)dwNoVeriteces;
#line 237
    CheckHResult(hr, "DrawStandardEdges failed");
}
void cD6Primitives::StartFrame()
{
    lgd3d_clip.left = (float)grd_canvas->gc.clip.f.left / 65536.0f + g_XOffset;
    lgd3d_clip.right = (float)grd_canvas->gc.clip.f.right / 65536.0f + g_XOffset;
    lgd3d_clip.top = (float)grd_canvas->gc.clip.f.top / 65536.0f + g_YOffset;
    lgd3d_clip.bot = (float)grd_canvas->gc.clip.f.bot / 65536.0f + g_YOffset;
    pcStates->EnableFog(0);
    pcStates->SetFogSpecularLevel(0.0f);
    m_dwNoCashedPoints = 0;
    g_bTexSuspended = 0;
}
void cD6Primitives::EndFrame()
{
    FlushPrimitives();
    EndIndexedRun();
}

_D3DTLVERTEX* cD6Primitives::ReservePolySlots(int n)
{
    if (m_bPointMode != 0) {
        FlushPoints();
        m_bPointMode = 0;
    }
#line 283
    AssertMsg((DWORD)n <= m_dwVertexBufferSize, "ReservePolySlots(): poly too large!");
    _D3DTLVERTEX* result = m_saVertexBuffer;
    m_dwNoCashedVertices = n;
    return result;
}
void cD6Primitives::DrawPoly(int bSuspendTexturing)
{
    if ((m_ePolyMode & 2) != 0)
        bSuspendTexturing = 1;
    if (bSuspendTexturing != g_bTexSuspended) {
        FlushIndPolies();
        if (bSuspendTexturing != 0)
            StartNonTexMode();
        else
            EndNonTexMode();
    }
    if (!PUNT_D3D) {
    if ((m_ePolyMode & 3) != 0) {
        HRESULT hr;
        hr = ((LPDIRECT3DDEVICE3)g_lpD3Ddevice)->DrawPrimitive(D3DPT_TRIANGLEFAN,
            D3DFVF_TLVERTEX, m_saVertexBuffer, m_dwNoCashedVertices, D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS);
#line 319
        CheckHResult(hr, "DrawPrimitive failed");
    }
    if ((m_ePolyMode & 4) != 0) {
        DrawStandardEdges(m_saVertexBuffer, m_dwNoCashedVertices);
    }
    }
}
int cD6Primitives::Poly(int n, r3s_point** ppl)
{
    DWORD color = pcStates->get_color();
    _D3DTLVERTEX* pv = ReservePolySlots(n);
    for (int i = 0; i < n; i++)
    {
        pv[i].color = color;
        pv[i].specular = m_dcFogSpecular;
        _D3DTLVERTEX* pvv = &pv[i];
        r3s_point* p = ppl[i];
        int x = (p->grp).sx + 0x8000;
        int y = (p->grp).sy + 0x8000;
        if (x > (grd_canvas->gc).clip.f.right)
            x = (grd_canvas->gc).clip.f.right;
        if (x < (grd_canvas->gc).clip.f.left)
            x = (grd_canvas->gc).clip.f.left;
        if (y > (grd_canvas->gc).clip.f.bot)
            y = (grd_canvas->gc).clip.f.bot;
        if (y < (grd_canvas->gc).clip.f.top)
            y = (grd_canvas->gc).clip.f.top;
        pvv->sx = (float)x / 65536.0f + g_XOffset;
        pvv->sy = (float)y / 65536.0f + g_YOffset;
        _D3DTLVERTEX* pvz = pvv;
        if (zlinear)
        {
            pvz->sz = (float)z2d;
        }
        else
        {
            if (lgd3d_z_normal)
            {
                pvz->sz = (float)((p->p).z * inv_z_far);
            }
            else
            {
                pvz->sz = (float)(z1 - (p->grp).w * z2);
                pvz->rhw = (p->grp).w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                    }
                }
            }
        }
    }
    DrawPoly(1);
    return 0;
}
int cD6Primitives::SPoly(int n, r3s_point** ppl)
{
    DWORD base_color = pcStates->get_color();
    _D3DTLVERTEX* pv = ReservePolySlots(n);
    for (int i = 0; i < n; i++)
    {
        float inten = ppl[i]->grp.i;
        if (inten > 1.0f)
            inten = 1.0f;
        make_scolor(&pv[i].color, base_color, inten);
        pv[i].specular = m_dcFogSpecular;
        _D3DTLVERTEX* pvv = &pv[i];
        r3s_point* p = ppl[i];
        int x = (p->grp).sx + 0x8000;
        int y = (p->grp).sy + 0x8000;
        if (x > (grd_canvas->gc).clip.f.right)
            x = (grd_canvas->gc).clip.f.right;
        if (x < (grd_canvas->gc).clip.f.left)
            x = (grd_canvas->gc).clip.f.left;
        if (y > (grd_canvas->gc).clip.f.bot)
            y = (grd_canvas->gc).clip.f.bot;
        if (y < (grd_canvas->gc).clip.f.top)
            y = (grd_canvas->gc).clip.f.top;
        pvv->sx = (float)x / 65536.0f + g_XOffset;
        pvv->sy = (float)y / 65536.0f + g_YOffset;
        _D3DTLVERTEX* pvz = pvv;
        if (zlinear)
        {
            pvz->sz = (float)z2d;
        }
        else
        {
            if (lgd3d_z_normal)
            {
                pvz->sz = (float)((p->p).z * inv_z_far);
            }
            else
            {
                pvz->sz = (float)(z1 - (p->grp).w * z2);
                pvz->rhw = (p->grp).w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                    }
                }
            }
        }
    }
    DrawPoly(1);
    return 0;
}
int cD6Primitives::RGB_Poly(int n, r3s_point** ppl)
{
    int bc;
    int rc;
    DWORD c0;
    int gc;
    int j;
    _D3DTLVERTEX* vlist;

    c0 = pcStates->get_color();
    rc = (c0 >> 16) & 0xff;
    gc = (c0 >> 8) & 0xff;
    bc = c0 & 0xff;
    vlist = ReservePolySlots(n);
    for (j = 0; j < n; j++)
    {
        g2s_point* g2p = (g2s_point*)&ppl[j]->grp;
        int r = (int)(g2p->r * (float)rc);
        if (r > 0xff)
            r = 0xff;
        int g = (int)(g2p->g * (float)gc);
        if (g > 0xff)
            g = 0xff;
        int b = (int)(g2p->b * (float)bc);
        if (b > 0xff)
            b = 0xff;
        vlist[j].color = (m_nAlpha << 24) | (r << 16) | (g << 8) | b;
        vlist[j].specular = m_dcFogSpecular;
        _D3DTLVERTEX* pvv = &vlist[j];
        r3s_point* pplj = ppl[j];
        int _sx = pplj->grp.sx + 0x8000;
        int _sy = pplj->grp.sy + 0x8000;
        if (_sx > (grd_canvas->gc).clip.f.right)
            _sx = (grd_canvas->gc).clip.f.right;
        if (_sx < (grd_canvas->gc).clip.f.left)
            _sx = (grd_canvas->gc).clip.f.left;
        if (_sy > (grd_canvas->gc).clip.f.bot)
            _sy = (grd_canvas->gc).clip.f.bot;
        if (_sy < (grd_canvas->gc).clip.f.top)
            _sy = (grd_canvas->gc).clip.f.top;
        pvv->sx = (float)_sx / 65536.0f + g_XOffset;
        pvv->sy = (float)_sy / 65536.0f + g_YOffset;
        _D3DTLVERTEX* pvz = pvv;
        if (zlinear)
        {
            pvz->sz = (float)z2d;
        }
        else
        {
            if (lgd3d_z_normal)
            {
                pvz->sz = (float)(pplj->p.z * inv_z_far);
            }
            else
            {
                pvz->sz = (float)(z1 - pplj->grp.w * z2);
                pvz->rhw = pplj->grp.w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                    }
                }
            }
        }
    }
    DrawPoly(1);
    return 0;
}
int cD6Primitives::RGBA_Poly(int n, r3s_point** ppl)
{
    DWORD base_color = pcStates->get_color();
    int red = (base_color >> 16) & 0xff;
    int green = (base_color >> 8) & 0xff;
    int blue = base_color & 0xff;
    _D3DTLVERTEX* pv = ReservePolySlots(n);
    for (int i = 0; i < n; i++)
    {
        g2s_point* g2p = (g2s_point*)&ppl[i]->grp;
        int r = (int)(g2p->r * (float)red);
        if (r > 0xff)
            r = 0xff;
        int g = (int)(g2p->g * (float)green);
        if (g > 0xff)
            g = 0xff;
        int b = (int)(g2p->b * (float)blue);
        if (b > 0xff)
            b = 0xff;
        int a = (int)(g2p->a * (float)m_nAlpha);
        if (a > 0xff)
            a = 0xff;
        pv[i].color = (a << 24) | (r << 16) | (g << 8) | b;
        pv[i].specular = m_dcFogSpecular;
        _D3DTLVERTEX* pvv = &pv[i];
        r3s_point* p = ppl[i];
        int x = (p->grp).sx + 0x8000;
        int y = (p->grp).sy + 0x8000;
        if (x > (grd_canvas->gc).clip.f.right)
            x = (grd_canvas->gc).clip.f.right;
        if (x < (grd_canvas->gc).clip.f.left)
            x = (grd_canvas->gc).clip.f.left;
        if (y > (grd_canvas->gc).clip.f.bot)
            y = (grd_canvas->gc).clip.f.bot;
        if (y < (grd_canvas->gc).clip.f.top)
            y = (grd_canvas->gc).clip.f.top;
        pvv->sx = (float)x / 65536.0f + g_XOffset;
        pvv->sy = (float)y / 65536.0f + g_YOffset;
        _D3DTLVERTEX* pvz = pvv;
        if (zlinear)
        {
            pvz->sz = (float)z2d;
        }
        else
        {
            if (lgd3d_z_normal)
            {
                pvz->sz = (float)((p->p).z * inv_z_far);
            }
            else
            {
                pvz->sz = (float)(z1 - (p->grp).w * z2);
                pvz->rhw = (p->grp).w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                    }
                }
            }
        }
    }
    DrawPoly(1);
    return 0;
}
int cD6Primitives::Trifan(int n, r3s_point** ppl)
{
    DWORD color = ((DWORD)m_nAlpha << 24) + 0xFFFFFF;
    int i;
    _D3DTLVERTEX* pv = ReservePolySlots(n);
    for (i = 0; i < n; i++)
    {
        float u = (ppl[i]->grp).u;
        float v = (ppl[i]->grp).v;
        pv[i].tu = u;
        pv[i].tv = v;
        _D3DTLVERTEX* pvv = &pv[i];
        r3s_point* p = ppl[i];
        int x = (p->grp).sx + 0x8000;
        int y = (p->grp).sy + 0x8000;
        if (x > (grd_canvas->gc).clip.f.right)
            x = (grd_canvas->gc).clip.f.right;
        if (x < (grd_canvas->gc).clip.f.left)
            x = (grd_canvas->gc).clip.f.left;
        if (y > (grd_canvas->gc).clip.f.bot)
            y = (grd_canvas->gc).clip.f.bot;
        if (y < (grd_canvas->gc).clip.f.top)
            y = (grd_canvas->gc).clip.f.top;
        pvv->sx = (float)x / 65536.0f + g_XOffset;
        pvv->sy = (float)y / 65536.0f + g_YOffset;
        _D3DTLVERTEX* pvz = pvv;
        if (zlinear)
        {
            pvz->sz = (float)z2d;
        }
        else
        {
            if (lgd3d_z_normal)
            {
                pvz->sz = (float)((p->p).z * inv_z_far);
            }
            else
            {
                pvz->sz = (float)(z1 - (p->grp).w * z2);
                pvz->rhw = (p->grp).w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                    }
                }
            }
        }
        pv[i].color = color;
        pv[i].specular = m_dcFogSpecular;
    }
    DrawPoly(0);
    return 0;
}


int cD6Primitives::LitTrifan(int n, r3s_point** ppl)
{
    DWORD base_color = ((DWORD)m_nAlpha << 24) + 0xFFFFFF;
    _D3DTLVERTEX* pv = ReservePolySlots(n);
    for (int i = 0; i < n; i++)
    {
        float u = ppl[i]->grp.u;
        float v = ppl[i]->grp.v;
        float inten = ppl[i]->grp.i;
        if (inten > 1.0)
            inten = 1.0f;
        pv[i].tu = u;
        pv[i].tv = v;
        make_scolor(&pv[i].color, base_color, inten);
        pv[i].specular = m_dcFogSpecular;
        _D3DTLVERTEX* pvv = &pv[i];
        r3s_point* p = ppl[i];
        int x = (p->grp).sx + 0x8000;
        int y = (p->grp).sy + 0x8000;
        if (x > (grd_canvas->gc).clip.f.right)
            x = (grd_canvas->gc).clip.f.right;
        if (x < (grd_canvas->gc).clip.f.left)
            x = (grd_canvas->gc).clip.f.left;
        if (y > (grd_canvas->gc).clip.f.bot)
            y = (grd_canvas->gc).clip.f.bot;
        if (y < (grd_canvas->gc).clip.f.top)
            y = (grd_canvas->gc).clip.f.top;
        pvv->sx = (float)x / 65536.0f + g_XOffset;
        pvv->sy = (float)y / 65536.0f + g_YOffset;
        _D3DTLVERTEX* pvz = pvv;
        if (zlinear)
        {
            pvz->sz = (float)z2d;
        }
        else
        {
            if (lgd3d_z_normal)
            {
                pvz->sz = (float)((p->p).z * inv_z_far);
            }
            else
            {
                pvz->sz = (float)(z1 - (p->grp).w * z2);
                pvz->rhw = (p->grp).w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                    }
                }
            }
        }
    }
    DrawPoly(0);
    return 0;
}

int cD6Primitives::SetPolyMode(int mode) {
    if ((mode & 7) != 0) {
        if (m_ePolyMode != mode) {
            FlushPrimitives();
        }
        m_ePolyMode = mode;
        return 1;
    }
    return 0;
}

int cD6Primitives::RGBlitTrifan(int n, r3s_point** ppl)
{
    _D3DTLVERTEX* pv = ReservePolySlots(n);
    for (int i = 0; i < n; i++)
    {
        r3s_point* pp = ppl[i];
        float u = pp->grp.u; float v = pp->grp.v;
        pv[i].tu = u; pv[i].tv = v;
        g2s_point* g2p = (g2s_point*)&pp->grp;
        int r = (int)(255.0f * g2p->r);
        if (r > 0xff) r = 0xff;
        int g = (int)(255.0f * g2p->g);
        if (g > 0xff) g = 0xff;
        int b = (int)(255.0f * g2p->b);
        if (b > 0xff) b = 0xff;
        pv[i].color = (((m_nAlpha << 8) | r) << 8 | g) << 8 | b;
        pv[i].specular = m_dcFogSpecular;
        _D3DTLVERTEX* pvv = &pv[i];
        r3s_point* p = ppl[i];
        int x = (p->grp).sx + 0x8000;
        int y = (p->grp).sy + 0x8000;
        if (x > (grd_canvas->gc).clip.f.right)
            x = (grd_canvas->gc).clip.f.right;
        if (x < (grd_canvas->gc).clip.f.left)
            x = (grd_canvas->gc).clip.f.left;
        if (y > (grd_canvas->gc).clip.f.bot)
            y = (grd_canvas->gc).clip.f.bot;
        if (y < (grd_canvas->gc).clip.f.top)
            y = (grd_canvas->gc).clip.f.top;
        pvv->sx = (float)x / 65536.0f + g_XOffset;
        pvv->sy = (float)y / 65536.0f + g_YOffset;
        _D3DTLVERTEX* pvz = pvv;
        if (zlinear) {
            pvz->sz = (float)z2d;
        } else {
            if (lgd3d_z_normal) {
                pvz->sz = (float)((p->p).z * inv_z_far);
            } else {
                pvz->sz = (float)(z1 - (p->grp).w * z2);
                pvz->rhw = (p->grp).w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                    }
                }
            }
        }
    }
    DrawPoly(0);
    return 0;
}
int cD6Primitives::RGBAlitTrifan(int n, r3s_point** ppl)
{
    _D3DTLVERTEX* pv = ReservePolySlots(n);
    for (int i = 0; i < n; i++)
    {
        r3s_point* pp = ppl[i];
        float u = pp->grp.u; float v = pp->grp.v;
        pv[i].tu = u; pv[i].tv = v;
        g2s_point* g2p = (g2s_point*)&pp->grp;
        int r = (int)(255.0f * g2p->r);
        if (r > 0xff) r = 0xff;
        int g = (int)(255.0f * g2p->g);
        if (g > 0xff) g = 0xff;
        int b = (int)(255.0f * g2p->b);
        if (b > 0xff) b = 0xff;
        int a = (int)(255.0f * g2p->a);
        if (a > 0xff) a = 0xff;
        pv[i].color = (((a << 8) | r) << 8 | g) << 8 | b;
        pv[i].specular = m_dcFogSpecular;
        _D3DTLVERTEX* pvv = &pv[i];
        r3s_point* p = ppl[i];
        int x = (p->grp).sx + 0x8000;
        int y = (p->grp).sy + 0x8000;
        if (x > (grd_canvas->gc).clip.f.right)
            x = (grd_canvas->gc).clip.f.right;
        if (x < (grd_canvas->gc).clip.f.left)
            x = (grd_canvas->gc).clip.f.left;
        if (y > (grd_canvas->gc).clip.f.bot)
            y = (grd_canvas->gc).clip.f.bot;
        if (y < (grd_canvas->gc).clip.f.top)
            y = (grd_canvas->gc).clip.f.top;
        pvv->sx = (float)x / 65536.0f + g_XOffset;
        pvv->sy = (float)y / 65536.0f + g_YOffset;
        _D3DTLVERTEX* pvz = pvv;
        if (zlinear)
        {
            pvz->sz = (float)z2d;
        }
        else
        {
            if (lgd3d_z_normal)
            {
                pvz->sz = (float)((p->p).z * inv_z_far);
            }
            else
            {
                pvz->sz = (float)(z1 - (p->grp).w * z2);
                pvz->rhw = (p->grp).w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                    }
                }
            }
        }
    }
    DrawPoly(0);
    return 0;
}


#include <fix.h>
#include <lglog.h>
#include <lgassert.h>


#define setz(_dest, z, w) \
do { \
   LPD3DTLVERTEX __dest = _dest; \
   if (zlinear) \
      __dest->sz = z2d; \
   else if (lgd3d_z_normal) \
      __dest->sz = z * inv_z_far; \
   else { \
      __dest->sz = z1 - z2 * w; \
      __dest->rhw = w;  \
      if (__dest->sz > 1.0) \
         __dest->sz = 1.0;  \
      else if (__dest->sz < 0.0) \
         __dest->sz = 0.0;  }\
} while (0)


int cD6Primitives::RGBAFogLitTrifan(int n, r3s_point** ppl)
{
    _D3DTLVERTEX* pv = ReservePolySlots(n);
    for (int i = 0; i < n; i++)
    {
        r3s_point* pp = ppl[i];
        float u = pp->grp.u; float v = pp->grp.v;
        pv[i].tu = u; pv[i].tv = v;
        g2s_point* g2p = (g2s_point*)&pp->grp;
        int r = (int)(255.0f * g2p->r);
        if (r > 0xff) r = 0xff;
        int g = (int)(255.0f * g2p->g);
        if (g > 0xff) g = 0xff;
        int b = (int)(255.0f * g2p->b);
        if (b > 0xff) b = 0xff;
        int a = (int)(255.0f * g2p->a);
        if (a > 0xff) a = 0xff;
        int fog = (int)((1.0 - g2p->coord[6]) * 255.0);
        if (fog > 0xff) fog = 0xff;
        pv[i].color = (((a << 8) | r) << 8 | g) << 8 | b;
        pv[i].specular = (fog << 24);
        _D3DTLVERTEX* pvv = &pv[i];
        r3s_point* p = ppl[i];
        int x = (p->grp).sx + 0x8000;
        int y = (p->grp).sy + 0x8000;
        if (x > (grd_canvas->gc).clip.f.right)
            x = (grd_canvas->gc).clip.f.right;
        if (x < (grd_canvas->gc).clip.f.left)
            x = (grd_canvas->gc).clip.f.left;
        if (y > (grd_canvas->gc).clip.f.bot)
            y = (grd_canvas->gc).clip.f.bot;
        if (y < (grd_canvas->gc).clip.f.top)
            y = (grd_canvas->gc).clip.f.top;
        pvv->sx = (float)x / 65536.0f + g_XOffset;
        pvv->sy = (float)y / 65536.0f + g_YOffset;
        _D3DTLVERTEX* pvz = pvv;
        if (zlinear)
        {
            pvz->sz = (float)z2d;
        }
        else
        {
            if (lgd3d_z_normal)
            {
                pvz->sz = (float)((p->p).z * inv_z_far);
            }
            else
            {
                pvz->sz = (float)(z1 - (p->grp).w * z2);
                pvz->rhw = (p->grp).w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                    }
                }
            }
        }
    }
    DrawPoly(0);
    return 0;
}
int cD6Primitives::DiffuseSpecularLitTrifan(int n, r3s_point** ppl)
{
    _D3DTLVERTEX* pv = ReservePolySlots(n);
    for (int i = 0; i < n; i++)
    {
        r3s_point* pp = ppl[i];
        float u = pp->grp.u; float v = pp->grp.v;
        pv[i].tu = u; pv[i].tv = v;
        g2s_point* g2p = (g2s_point*)&pp->grp;
        int r = (int)(255.0f * g2p->r);
        if (r > 0xff) r = 0xff;
        int g = (int)(255.0f * g2p->g);
        if (g > 0xff) g = 0xff;
        int b = (int)(255.0f * g2p->b);
        if (b > 0xff) b = 0xff;
        int a = (int)(255.0f * g2p->a);
        if (a > 0xff) a = 0xff;
        pv[i].color = (((a << 8) | r) << 8 | g) << 8 | b;
        int spec_a = (int)(255.0f * g2p->coord[7]);
        if (spec_a > 0xff) spec_a = 0xff;
        int spec_r = (int)(255.0f * g2p->coord[8]);
        if (spec_r > 0xff) spec_r = 0xff;
        int spec_g = (int)(255.0f * g2p->coord[9]);
        if (spec_g > 0xff) spec_g = 0xff;
        int spec_b = (int)(255.0f * g2p->coord[10]);
        if (spec_b > 0xff) spec_b = 0xff;
        pv[i].specular = (((spec_a << 8) | spec_r) << 8 | spec_g) << 8 | spec_b;
        _D3DTLVERTEX* pvv = &pv[i];
        r3s_point* p = ppl[i];
        int x = (p->grp).sx + 0x8000;
        int y = (p->grp).sy + 0x8000;
        if (x > (grd_canvas->gc).clip.f.right)
            x = (grd_canvas->gc).clip.f.right;
        if (x < (grd_canvas->gc).clip.f.left)
            x = (grd_canvas->gc).clip.f.left;
        if (y > (grd_canvas->gc).clip.f.bot)
            y = (grd_canvas->gc).clip.f.bot;
        if (y < (grd_canvas->gc).clip.f.top)
            y = (grd_canvas->gc).clip.f.top;
        pvv->sx = (float)x / 65536.0f + g_XOffset;
        pvv->sy = (float)y / 65536.0f + g_YOffset;
        _D3DTLVERTEX* pvz = pvv;
        if (zlinear)
        {
            pvz->sz = (float)z2d;
        }
        else
        {
            if (lgd3d_z_normal)
            {
                pvz->sz = (float)((p->p).z * inv_z_far);
            }
            else
            {
                pvz->sz = (float)(z1 - (p->grp).w * z2);
                pvz->rhw = (p->grp).w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                    }
                }
            }
        }
    }
    DrawPoly(0);
    return 0;
}
int cD6Primitives::g2UPoly(int n, g2s_point** ppl)
{
    DWORD color = pcStates->get_color();
    _D3DTLVERTEX* pv = ReservePolySlots(n);
    for (int i = 0; i < n; i++)
    {
        pv[i].color = color;
        pv[i].specular = m_dcFogSpecular;
        int x = ppl[i]->sx + 0x8000;
        int y = ppl[i]->sy + 0x8000;
        if (x > (grd_canvas->gc).clip.f.right)
            x = (grd_canvas->gc).clip.f.right;
        if (x < (grd_canvas->gc).clip.f.left)
            x = (grd_canvas->gc).clip.f.left;
        if (y > (grd_canvas->gc).clip.f.bot)
            y = (grd_canvas->gc).clip.f.bot;
        if (y < (grd_canvas->gc).clip.f.top)
            y = (grd_canvas->gc).clip.f.top;
        pv[i].sx = (float)x / 65536.0f + g_XOffset;
        pv[i].sy = (float)y / 65536.0f + g_YOffset;
        pv[i].sz = (float)z2d;
        pv[i].rhw = (float)w2d;
    }
    DrawPoly(1);
    return 0;
}
int cD6Primitives::g2Poly(int n, g2s_point** ppl)
{
    g2s_point** clipped = NULL;
    int nclipped = g2_clip_poly(n, 0, ppl, &clipped);
    int result;
    if (nclipped < 3)
    {
        result = 0x10;
    }
    else
    {
        g2UPoly(nclipped, clipped);
        result = 0;
    }
    if (clipped && clipped != ppl)
    {
        temp_free(clipped);
    }
    return result;
}
int cD6Primitives::g2UTrifan(int n, g2s_point** ppl)
{
    unsigned int alpha = (m_nAlpha << 24) + 0xffffff;
    _D3DTLVERTEX* pv = ReservePolySlots(n);
    for (int i = 0; i < n; i++)
    {
        float intensity = ppl[i]->i;
        int x = ppl[i]->sx + 0x8000;
        int y = ppl[i]->sy + 0x8000;
        if (x > (grd_canvas->gc).clip.f.right)
            x = (grd_canvas->gc).clip.f.right;
        if (x < (grd_canvas->gc).clip.f.left)
            x = (grd_canvas->gc).clip.f.left;
        if (y > (grd_canvas->gc).clip.f.bot)
            y = (grd_canvas->gc).clip.f.bot;
        if (y < (grd_canvas->gc).clip.f.top)
            y = (grd_canvas->gc).clip.f.top;
        pv[i].sx = (float)x / 65536.0f + g_XOffset;
        pv[i].sy = (float)y / 65536.0f + g_YOffset;
        pv[i].sz = (float)z2d;
        pv[i].rhw = (float)w2d;
        unsigned int alpha_tmp = alpha;
        pv[i].color = (((((int)(((alpha_tmp >> 16) & 0xff) * intensity)) << 8) + (int)(((alpha_tmp >> 8) & 0xff) * intensity)) << 8) + (int)((alpha_tmp & 0xff) * intensity) + (alpha_tmp & 0xff000000);
        pv[i].specular = m_dcFogSpecular;
        pv[i].tu = ppl[i]->u;
        pv[i].tv = ppl[i]->v;
    }
    DrawPoly(0);
    return 0;
}
int cD6Primitives::g2Trifan(int n, g2s_point** ppl)
{
    g2s_point** clipped = NULL;
    int nclipped = g2_clip_poly(n, 4, ppl, &clipped);
    int result;
    if (nclipped < 3)
    {
        result = 0x10;
    }
    else
    {
        g2UTrifan(nclipped, clipped);
        result = 0;
    }
    if (clipped && clipped != ppl)
    {
        temp_free(clipped);
    }
    return result;
}
void cD6Primitives::InitializeVIBCounters()
{
    m_dwNoVIBEntries = 0;
    m_dwNoIndices = 0;
    m_dwTempIndCounter = 0;
    m_dwMinVIndex = 0x100;
    m_dwMaxVIndex = 0;
}
int cD6Primitives::CreateVertIndBuffer(unsigned long size)
{
    DeleteVertIndBuffer();
    m_dwVIBSizeInEntries = size;
#line 1019
    m_pVIB = (_D3DTLVERTEX*)malloc(m_dwVIBSizeInEntries * sizeof(_D3DTLVERTEX));
    InitializeVIBCounters();
    m_dwVIBmax = 0x100;
    return m_pVIB != NULL;
}

void cD6Primitives::init_hack_light_bm()
{
   uchar *bits;
   int i,j;

   m_hack_light_bm = gr_alloc_bitmap(BMT_FLAT8, BMF_TLUC, 32, 32);
   bits = m_hack_light_bm->bits;
   for (i=0; i<32; i++) {
      for (j=0; j<32; j++) {
         float alpha = 8.0 * 16.0 / ((i-15.5)*(i-15.5) + (j-15.5)*(j-15.5));
         uchar val;
         if (alpha > 15.0)
            val = 15;
         else {
            val = (uchar)((int)alpha);
         }
         bits[j] = val;
      }
      bits += 32;
   }
}
void cD6Primitives::DeleteVertIndBuffer()
{
    if (m_pVIB) {
        free(m_pVIB);
        m_pVIB = NULL;
    }
}


void cD6Primitives::HackLight(r3s_point *p, float r)
{
    if (r <= 1.0) {
        DrawPoint(p);
    } else {
        if (m_hack_light_bm == NULL)
            init_hack_light_bm();
        do_quad_light(p, r, m_hack_light_bm);
    }
}
int cD6Primitives::ResizeVertIndBuffer(unsigned long size)
{
    _D3DTLVERTEX* pNew = (_D3DTLVERTEX*)realloc(m_pVIB, size * sizeof(_D3DTLVERTEX));
    if (pNew) {
        m_pVIB = pNew;
        m_dwVIBSizeInEntries = size;
        return 1;
    }
    return 0;
}
void cD6Primitives::FlushIfNoFit(int nIndicesToAdd, int bSuspendTexturing)
{
    if (m_bPointMode)
    {
        FlushPoints();
        m_bPointMode = 0;
    }
    if (bSuspendTexturing != g_bTexSuspended)
    {
        FlushIndPolies();
        if (bSuspendTexturing != 0)
            StartNonTexMode();
        else
            EndNonTexMode();
    }
    if ((50 <= m_dwTempIndCounter + nIndicesToAdd) ||
        ((nIndicesToAdd - 3) * 3 + m_dwNoIndices + 3 >= m_dwVIBmax))
    {
        FlushIndPolies();
        m_dwTempIndCounter = 0;
    }
}
_D3DTLVERTEX* cD6Primitives::GetIndPolySlot(int nPolySize, r3ixs_info* psIndInfo)
{
    USHORT wIndex;
    _D3DTLVERTEX* pResult;

    wIndex = *(USHORT*)psIndInfo;
    if (wIndex >= m_dwVIBSizeInEntries)
    {
        if (ResizeVertIndBuffer(((wIndex >> 8) + 1) << 8) == 0)
        {
#line 852
            CriticalMsg(D6Msg("Could not reallocate memory for VIB"));
        }
    }
    if (wIndex > m_dwMaxVIndex)
        m_dwMaxVIndex = wIndex;
    if (wIndex < m_dwMinVIndex)
        m_dwMinVIndex = wIndex;
    m_waTempIndices[m_dwTempIndCounter] = wIndex;
    m_dwTempIndCounter++;
    if ((((unsigned char*)psIndInfo)[2] & 1) != 0)
        pResult = NULL;
    else
        pResult = m_pVIB + wIndex;
    return pResult;
}


void cD6Primitives::DrawIndPolies()
{





    AssertMsg1(m_dwTempIndCounter > 2, "A poly has more than %d points!", m_dwTempIndCounter);

    m_saIndexBuffer[m_dwNoIndices] = m_waTempIndices[0];
    m_saIndexBuffer[m_dwNoIndices + 1] = m_waTempIndices[1];
    m_saIndexBuffer[m_dwNoIndices + 2] = m_waTempIndices[2];
    m_dwNoIndices += 3;

    for (DWORD j = 3; j < m_dwTempIndCounter; j++)
    {
        m_saIndexBuffer[m_dwNoIndices] = m_waTempIndices[0];
        m_saIndexBuffer[m_dwNoIndices + 1] = m_waTempIndices[j - 1];
        m_saIndexBuffer[m_dwNoIndices + 2] = m_waTempIndices[j];
        m_dwNoIndices += 3;
    }

    m_dwTempIndCounter = 0;
    m_bPrimitivesPending = 1;
}

void cD6Primitives::FlushIndPolies()
{
    DWORD i;
    int result;

    if (m_dwNoIndices != 0)
    {
        if (m_dwMinVIndex != 0) {
            for (i = 0; i < m_dwNoIndices; i++) {
#line 915
                AssertMsg((DWORD)m_saIndexBuffer[i] <= m_dwMaxVIndex, "Runaway Prim index!");
                m_saIndexBuffer[i] -= (WORD)m_dwMinVIndex;
            }
        }
#ifdef DEBUG
        else {
            for (i = 0; i < m_dwNoIndices; i++) {
#line 924
                AssertMsg((DWORD)m_saIndexBuffer[i] <= m_dwMaxVIndex, "Runaway Prim index!");
            }
        }
#endif
        DWORD vertex_count = m_dwMaxVIndex + 1 - m_dwMinVIndex;
        if (!PUNT_D3D)
        {
            result = ((LPDIRECT3DDEVICE3)g_lpD3Ddevice)->DrawIndexedPrimitive(
                D3DPT_TRIANGLELIST, D3DFVF_TLVERTEX,
                m_pVIB + m_dwMinVIndex, vertex_count,
                m_saIndexBuffer, m_dwNoIndices, D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS);
#line 942
            CheckHResult(result, "DrawIndexedPrimitive failed");

        }
        m_dwNoIndices = 0;
        m_dwMinVIndex = 0x100;
        m_dwMaxVIndex = 0;
    }
}

int cD6Primitives::PolyInd(int n, r3s_point** ppl, r3ixs_info* info)
{
    DWORD color = pcStates->get_color();
    r3ixs_info_local* pIter = (r3ixs_info_local*)info;
    FlushIfNoFit(n, 1);
    for (int i = 0; i < n; i++, pIter++)
    {
        _D3DTLVERTEX* pv = GetIndPolySlot(n, (r3ixs_info*)pIter);
        if (!pv)
            continue;
        pv->color = color;
        pv->specular = m_dcFogSpecular;
        _D3DTLVERTEX* pvv = pv;
        r3s_point* p = ppl[i];
        int x = (p->grp).sx + 0x8000;
        int y = (p->grp).sy + 0x8000;
        if (x > (grd_canvas->gc).clip.f.right)
            x = (grd_canvas->gc).clip.f.right;
        if (x < (grd_canvas->gc).clip.f.left)
            x = (grd_canvas->gc).clip.f.left;
        if (y > (grd_canvas->gc).clip.f.bot)
            y = (grd_canvas->gc).clip.f.bot;
        if (y < (grd_canvas->gc).clip.f.top)
            y = (grd_canvas->gc).clip.f.top;
        pvv->sx = (float)x / 65536.0f + g_XOffset;
        pvv->sy = (float)y / 65536.0f + g_YOffset;
        _D3DTLVERTEX* pvz = pvv;
        if (zlinear)
        {
            pvz->sz = (float)z2d;
        }
        else
        {
            if (lgd3d_z_normal)
            {
                pvz->sz = (float)((p->p).z * inv_z_far);
            }
            else
            {
                pvz->sz = (float)(z1 - (p->grp).w * z2);
                pvz->rhw = (p->grp).w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                        do {} while (0);
                        do {} while (0);
                    }
                }
            }
        }
    }
    DrawIndPolies();
    return 0;
}

int cD6Primitives::SPolyInd(int n, r3s_point** ppl, r3ixs_info* info)
{
    DWORD base_color = pcStates->get_color();
    r3ixs_info_local* pIter = (r3ixs_info_local*)info;
    FlushIfNoFit(n, 1);
    for (int i = 0; i < n; i++, pIter++)
    {
        _D3DTLVERTEX* pv = GetIndPolySlot(n, (r3ixs_info*)pIter);
        if (!pv)
            continue;
        float inten = ppl[i]->grp.i;
        if (inten > 1.0f)
            inten = 1.0f;
        make_scolor(&pv->color, base_color, inten);
        pv->specular = m_dcFogSpecular;
        _D3DTLVERTEX* pvv = pv;
        r3s_point* p = ppl[i];
        int x = (p->grp).sx + 0x8000;
        int y = (p->grp).sy + 0x8000;
        if (x > (grd_canvas->gc).clip.f.right)
            x = (grd_canvas->gc).clip.f.right;
        if (x < (grd_canvas->gc).clip.f.left)
            x = (grd_canvas->gc).clip.f.left;
        if (y > (grd_canvas->gc).clip.f.bot)
            y = (grd_canvas->gc).clip.f.bot;
        if (y < (grd_canvas->gc).clip.f.top)
            y = (grd_canvas->gc).clip.f.top;
        pvv->sx = (float)x / 65536.0f + g_XOffset;
        pvv->sy = (float)y / 65536.0f + g_YOffset;
        _D3DTLVERTEX* pvz = pvv;
        if (zlinear)
        {
            pvz->sz = (float)z2d;
        }
        else
        {
            if (lgd3d_z_normal)
            {
                pvz->sz = (float)((p->p).z * inv_z_far);
            }
            else
            {
                pvz->sz = (float)(z1 - (p->grp).w * z2);
                pvz->rhw = (p->grp).w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                        do {} while (0);
                        do {} while (0);
                    }
                }
            }
        }
    }
    DrawIndPolies();
    return 0;
}

int cD6Primitives::RGB_PolyInd(int n, r3s_point** ppl, r3ixs_info* info)
{
    unsigned int color = pcStates->get_color();
    r3ixs_info_local* pIter = (r3ixs_info_local*)info;
    int r = (color >> 16) & 0xff;
    int g = (color >> 8) & 0xff;
    int b = color & 0xff;
    FlushIfNoFit(n, 1);
    for (int i = 0; i < n; i++, pIter++)
    {
        _D3DTLVERTEX* pv = GetIndPolySlot(n, (r3ixs_info*)pIter);
        if (!pv)
            continue;
        g2s_point* g2p = (g2s_point*)&ppl[i]->grp;
        int vr = (int)(r * g2p->r);
        if (vr > 0xff) vr = 0xff;
        int vg = (int)(g * g2p->g);
        if (vg > 0xff) vg = 0xff;
        int vb = (int)(b * g2p->b);
        if (vb > 0xff) vb = 0xff;
        pv->color = (m_nAlpha << 24) | (vr << 16) | (vg << 8) | vb;
        pv->specular = m_dcFogSpecular;
        r3s_point* p = ppl[i];
        r3s_point* pclip = p;
        int x = pclip->grp.sx + 0x8000;
        int y = pclip->grp.sy + 0x8000;
        if (x > (grd_canvas->gc).clip.f.right)
            x = (grd_canvas->gc).clip.f.right;
        if (x < (grd_canvas->gc).clip.f.left)
            x = (grd_canvas->gc).clip.f.left;
        if (y > (grd_canvas->gc).clip.f.bot)
            y = (grd_canvas->gc).clip.f.bot;
        if (y < (grd_canvas->gc).clip.f.top)
            y = (grd_canvas->gc).clip.f.top;
        pv->sx = (float)x / 65536.0f + g_XOffset;
        pv->sy = (float)y / 65536.0f + g_YOffset;
        _D3DTLVERTEX* pvz = pv;
        if (zlinear)
        {
            pvz->sz = (float)z2d;
        }
        else
        {
            if (lgd3d_z_normal)
            {
                pvz->sz = (float)((p->p).z * inv_z_far);
            }
            else
            {
                pvz->sz = (float)(z1 - (p->grp).w * z2);
                pvz->rhw = (p->grp).w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                        do {} while (0);
                        do {} while (0);
                    }
                }
            }
        }
    }
    DrawIndPolies();
    return 0;
}

int cD6Primitives::RGBA_PolyInd(int n, r3s_point** ppl, r3ixs_info* info)
{
    DWORD base_color = pcStates->get_color();
    r3ixs_info_local* pIter = (r3ixs_info_local*)info;
    int red = (base_color >> 16) & 0xff;
    int green = (base_color >> 8) & 0xff;
    int blue = base_color & 0xff;
    FlushIfNoFit(n, 1);
    for (int i = 0; i < n; i++, pIter++)
    {
        _D3DTLVERTEX* pv = GetIndPolySlot(n, (r3ixs_info*)pIter);
        if (!pv)
            continue;
        g2s_point* g2p = (g2s_point*)&ppl[i]->grp;
        int r = (int)(g2p->r * (float)red);
        if (r > 0xff)
            r = 0xff;
        int g = (int)(g2p->g * (float)green);
        if (g > 0xff)
            g = 0xff;
        int b = (int)(g2p->b * (float)blue);
        if (b > 0xff)
            b = 0xff;
        int a = (int)(g2p->a * (float)m_nAlpha);
        if (a > 0xff)
            a = 0xff;
        pv->color = (a << 24) | (r << 16) | (g << 8) | b;
        pv->specular = m_dcFogSpecular;
        _D3DTLVERTEX* pvv = pv;
        r3s_point* p = ppl[i];
        int x = (p->grp).sx + 0x8000;
        int y = (p->grp).sy + 0x8000;
        if (x > (grd_canvas->gc).clip.f.right)
            x = (grd_canvas->gc).clip.f.right;
        if (x < (grd_canvas->gc).clip.f.left)
            x = (grd_canvas->gc).clip.f.left;
        if (y > (grd_canvas->gc).clip.f.bot)
            y = (grd_canvas->gc).clip.f.bot;
        if (y < (grd_canvas->gc).clip.f.top)
            y = (grd_canvas->gc).clip.f.top;
        pvv->sx = (float)x / 65536.0f + g_XOffset;
        pvv->sy = (float)y / 65536.0f + g_YOffset;
        _D3DTLVERTEX* pvz = pvv;
        if (zlinear)
        {
            pvz->sz = (float)z2d;
        }
        else
        {
            if (lgd3d_z_normal)
            {
                pvz->sz = (float)((p->p).z * inv_z_far);
            }
            else
            {
                pvz->sz = (float)(z1 - (p->grp).w * z2);
                pvz->rhw = (p->grp).w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                        do {} while (0);
                        do {} while (0);
                    }
                }
            }
        }
    }
    DrawIndPolies();
    return 0;
}

int cD6Primitives::TrifanInd(int n, r3s_point** ppl, r3ixs_info* info)
{
    DWORD color = ((DWORD)m_nAlpha << 24) + 0xFFFFFF;
    r3ixs_info_local* pIter = (r3ixs_info_local*)info;
    FlushIfNoFit(n, 0);
    for (int i = 0; i < n; i++, pIter++)
    {
        _D3DTLVERTEX* pv = GetIndPolySlot(n, (r3ixs_info*)pIter);
        if (pv)
        {
            float v = ppl[i]->grp.v;
            float u = ppl[i]->grp.u;
            pv->tu = u;
            pv->tv = v;
            r3s_point* p = ppl[i];
            int x = (p->grp).sx + 0x8000;
            int y = (p->grp).sy + 0x8000;
            if (x > (grd_canvas->gc).clip.f.right)
                x = (grd_canvas->gc).clip.f.right;
            if (x < (grd_canvas->gc).clip.f.left)
                x = (grd_canvas->gc).clip.f.left;
            if (y > (grd_canvas->gc).clip.f.bot)
                y = (grd_canvas->gc).clip.f.bot;
            if (y < (grd_canvas->gc).clip.f.top)
                y = (grd_canvas->gc).clip.f.top;
            pv->sx = (float)x / 65536.0f + g_XOffset;
            pv->sy = (float)y / 65536.0f + g_YOffset;
            _D3DTLVERTEX* pvz = pv;
            if (zlinear)
            {
                pvz->sz = (float)z2d;
            }
            else
            {
                if (lgd3d_z_normal)
                {
                    pvz->sz = (float)((p->p).z * inv_z_far);
                }
                else
                {
                    pvz->sz = (float)(z1 - (p->grp).w * z2);
                    pvz->rhw = (p->grp).w;
                    if (pvz->sz > 1.0)
                    {
                        pvz->sz = 1.0;
                    }
                    else
                    {
                        if (pvz->sz < 0.0)
                        {
                            pvz->sz = 0.0;
                        }
                    }
                }
            }
            pv->color = color;
            pv->specular = m_dcFogSpecular;
        }
    }
    DrawIndPolies();
    return 0;
}

int cD6Primitives::LitTrifanInd(int n, r3s_point** ppl, r3ixs_info* info)
{
    DWORD base_color = ((DWORD)m_nAlpha << 24) + 0xFFFFFF;
    r3ixs_info_local* pIter = (r3ixs_info_local*)info;
    FlushIfNoFit(n, 0);
    for (int i = 0; i < n; i++, pIter++)
    {
        _D3DTLVERTEX* pv = GetIndPolySlot(n, (r3ixs_info*)pIter);
        if (!pv)
            continue;
        float u = ppl[i]->grp.u;
        float v = ppl[i]->grp.v;
        float inten = ppl[i]->grp.i;
        if (inten > 1.0)
            inten = 1.0f;
        pv->tu = u;
        pv->tv = v;
        make_scolor(&pv->color, base_color, inten);
        pv->specular = m_dcFogSpecular;
        _D3DTLVERTEX* pvv = pv;
        r3s_point* p = ppl[i];
        int x = (p->grp).sx + 0x8000;
        int y = (p->grp).sy + 0x8000;
        if (x > (grd_canvas->gc).clip.f.right)
            x = (grd_canvas->gc).clip.f.right;
        if (x < (grd_canvas->gc).clip.f.left)
            x = (grd_canvas->gc).clip.f.left;
        if (y > (grd_canvas->gc).clip.f.bot)
            y = (grd_canvas->gc).clip.f.bot;
        if (y < (grd_canvas->gc).clip.f.top)
            y = (grd_canvas->gc).clip.f.top;
        pvv->sx = (float)x / 65536.0f + g_XOffset;
        pvv->sy = (float)y / 65536.0f + g_YOffset;
        _D3DTLVERTEX* pvz = pvv;
        if (zlinear)
        {
            pvz->sz = (float)z2d;
        }
        else
        {
            if (lgd3d_z_normal)
            {
                pvz->sz = (float)((p->p).z * inv_z_far);
            }
            else
            {
                pvz->sz = (float)(z1 - (p->grp).w * z2);
                pvz->rhw = (p->grp).w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                        do {} while (0);
                        do {} while (0);
                    }
                }
            }
        }
    }
    DrawIndPolies();
    return 0;
}

int cD6Primitives::RGBlitTrifanInd(int n, r3s_point** ppl, r3ixs_info* info)
{
    r3ixs_info_local* pIter = (r3ixs_info_local*)info;
    FlushIfNoFit(n, 0);
    for (int i = 0; i < n; i++, pIter++)
    {
        _D3DTLVERTEX* pv = GetIndPolySlot(n, (r3ixs_info*)pIter);
        if (!pv)
            continue;
        r3s_point* pp = ppl[i];
        float u = pp->grp.u; float v = pp->grp.v;
        pv->tu = u; pv->tv = v;
        g2s_point* g2p = (g2s_point*)&pp->grp;
        int r = (int)(255.0f * g2p->r);
        if (r > 0xff) r = 0xff;
        int g = (int)(255.0f * g2p->g);
        if (g > 0xff) g = 0xff;
        int b = (int)(255.0f * g2p->b);
        if (b > 0xff) b = 0xff;
        pv->color = (((m_nAlpha << 8) | r) << 8 | g) << 8 | b;
        pv->specular = m_dcFogSpecular;
        _D3DTLVERTEX* pvv = pv;
        r3s_point* p = ppl[i];
        int x = (p->grp).sx + 0x8000;
        int y = (p->grp).sy + 0x8000;
        if (x > (grd_canvas->gc).clip.f.right)
            x = (grd_canvas->gc).clip.f.right;
        if (x < (grd_canvas->gc).clip.f.left)
            x = (grd_canvas->gc).clip.f.left;
        if (y > (grd_canvas->gc).clip.f.bot)
            y = (grd_canvas->gc).clip.f.bot;
        if (y < (grd_canvas->gc).clip.f.top)
            y = (grd_canvas->gc).clip.f.top;
        pvv->sx = (float)x / 65536.0f + g_XOffset;
        pvv->sy = (float)y / 65536.0f + g_YOffset;
        _D3DTLVERTEX* pvz = pvv;
        if (zlinear) {
            pvz->sz = (float)z2d;
        } else {
            if (lgd3d_z_normal) {
                pvz->sz = (float)((p->p).z * inv_z_far);
            } else {
                pvz->sz = (float)(z1 - (p->grp).w * z2);
                pvz->rhw = (p->grp).w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                        do {} while (0);
                        do {} while (0);
                    }
                }
            }
        }
    }
    DrawIndPolies();
    return 0;
}

int cD6Primitives::RGBAlitTrifanInd(int n, r3s_point** ppl, r3ixs_info* info)
{
    r3ixs_info_local* pIter = (r3ixs_info_local*)info;
    FlushIfNoFit(n, 0);
    for (int i = 0; i < n; i++, pIter++)
    {
        _D3DTLVERTEX* pv = GetIndPolySlot(n, (r3ixs_info*)pIter);
        if (!pv)
            continue;
        r3s_point* pp = ppl[i];
        float u = pp->grp.u; float v = pp->grp.v;
        pv->tu = u; pv->tv = v;
        g2s_point* g2p = (g2s_point*)&pp->grp;
        int r = (int)(255.0f * g2p->r);
        if (r > 0xff) r = 0xff;
        int g = (int)(255.0f * g2p->g);
        if (g > 0xff) g = 0xff;
        int b = (int)(255.0f * g2p->b);
        if (b > 0xff) b = 0xff;
        int a = (int)(255.0f * g2p->a);
        if (a > 0xff) a = 0xff;
        pv->color = (((a << 8) | r) << 8 | g) << 8 | b;
        pv->specular = m_dcFogSpecular;
        _D3DTLVERTEX* pvv = pv;
        r3s_point* p = ppl[i];
        int x = (p->grp).sx + 0x8000;
        int y = (p->grp).sy + 0x8000;
        if (x > (grd_canvas->gc).clip.f.right)
            x = (grd_canvas->gc).clip.f.right;
        if (x < (grd_canvas->gc).clip.f.left)
            x = (grd_canvas->gc).clip.f.left;
        if (y > (grd_canvas->gc).clip.f.bot)
            y = (grd_canvas->gc).clip.f.bot;
        if (y < (grd_canvas->gc).clip.f.top)
            y = (grd_canvas->gc).clip.f.top;
        pvv->sx = (float)x / 65536.0f + g_XOffset;
        pvv->sy = (float)y / 65536.0f + g_YOffset;
        _D3DTLVERTEX* pvz = pvv;
        if (zlinear)
        {
            pvz->sz = (float)z2d;
        }
        else
        {
            if (lgd3d_z_normal)
            {
                pvz->sz = (float)((p->p).z * inv_z_far);
            }
            else
            {
                pvz->sz = (float)(z1 - (p->grp).w * z2);
                pvz->rhw = (p->grp).w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                        do {} while (0);
                        do {} while (0);
                    }
                }
            }
        }
    }
    DrawIndPolies();
    return 0;
}

int cD6Primitives::RGBAFogLitTrifanInd(int n, r3s_point** ppl, r3ixs_info* info)
{
    r3ixs_info_local* pIter = (r3ixs_info_local*)info;
    FlushIfNoFit(n, 0);
    for (int i = 0; i < n; i++, pIter++)
    {
        _D3DTLVERTEX* pv = GetIndPolySlot(n, (r3ixs_info*)pIter);
        if (!pv)
            continue;
        r3s_point* pp = ppl[i];
        float u = pp->grp.u; float v = pp->grp.v;
        pv->tu = u; pv->tv = v;
        g2s_point* g2p = (g2s_point*)&pp->grp;
        int r = (int)(255.0f * g2p->r);
        if (r > 0xff) r = 0xff;
        int g = (int)(255.0f * g2p->g);
        if (g > 0xff) g = 0xff;
        int b = (int)(255.0f * g2p->b);
        if (b > 0xff) b = 0xff;
        int a = (int)(255.0f * g2p->a);
        if (a > 0xff) a = 0xff;
        int fog = (int)((1.0 - g2p->coord[6]) * 255.0);
        if (fog > 0xff) fog = 0xff;
        pv->color = (((a << 8) | r) << 8 | g) << 8 | b;
        pv->specular = (fog << 24);
        _D3DTLVERTEX* pvv = pv;
        r3s_point* p = ppl[i];
        int x = (p->grp).sx + 0x8000;
        int y = (p->grp).sy + 0x8000;
        if (x > (grd_canvas->gc).clip.f.right)
            x = (grd_canvas->gc).clip.f.right;
        if (x < (grd_canvas->gc).clip.f.left)
            x = (grd_canvas->gc).clip.f.left;
        if (y > (grd_canvas->gc).clip.f.bot)
            y = (grd_canvas->gc).clip.f.bot;
        if (y < (grd_canvas->gc).clip.f.top)
            y = (grd_canvas->gc).clip.f.top;
        pvv->sx = (float)x / 65536.0f + g_XOffset;
        pvv->sy = (float)y / 65536.0f + g_YOffset;
        _D3DTLVERTEX* pvz = pvv;
        if (zlinear)
        {
            pvz->sz = (float)z2d;
        }
        else
        {
            if (lgd3d_z_normal)
            {
                pvz->sz = (float)((p->p).z * inv_z_far);
            }
            else
            {
                pvz->sz = (float)(z1 - (p->grp).w * z2);
                pvz->rhw = (p->grp).w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                        do {} while (0);
                        do {} while (0);
                    }
                }
            }
        }
    }
    DrawIndPolies();
    return 0;
}

int cD6Primitives::DiffuseSpecularLitTrifanInd(int n, r3s_point** ppl, r3ixs_info* info)
{
    r3ixs_info_local* pIter = (r3ixs_info_local*)info;
    FlushIfNoFit(n, 0);
    for (int i = 0; i < n; i++, pIter++)
    {
        _D3DTLVERTEX* pv = GetIndPolySlot(n, (r3ixs_info*)pIter);
        if (!pv)
            continue;
        r3s_point* pp = ppl[i];
        float u = pp->grp.u; float v = pp->grp.v;
        pv->tu = u; pv->tv = v;
        g2s_point* g2p = (g2s_point*)&pp->grp;
        int r = (int)(255.0f * g2p->r);
        if (r > 0xff) r = 0xff;
        int g = (int)(255.0f * g2p->g);
        if (g > 0xff) g = 0xff;
        int b = (int)(255.0f * g2p->b);
        if (b > 0xff) b = 0xff;
        int a = (int)(255.0f * g2p->a);
        if (a > 0xff) a = 0xff;
        pv->color = (((a << 8) | r) << 8 | g) << 8 | b;
        int spec_a = (int)(255.0f * g2p->coord[7]);
        if (spec_a > 0xff) spec_a = 0xff;
        int spec_r = (int)(255.0f * g2p->coord[8]);
        if (spec_r > 0xff) spec_r = 0xff;
        int spec_g = (int)(255.0f * g2p->coord[9]);
        if (spec_g > 0xff) spec_g = 0xff;
        int spec_b = (int)(255.0f * g2p->coord[10]);
        if (spec_b > 0xff) spec_b = 0xff;
        pv->specular = (((spec_a << 8) | spec_r) << 8 | spec_g) << 8 | spec_b;
        _D3DTLVERTEX* pvv = pv;
        r3s_point* p = ppl[i];
        int x = (p->grp).sx + 0x8000;
        int y = (p->grp).sy + 0x8000;
        if (x > (grd_canvas->gc).clip.f.right)
            x = (grd_canvas->gc).clip.f.right;
        if (x < (grd_canvas->gc).clip.f.left)
            x = (grd_canvas->gc).clip.f.left;
        if (y > (grd_canvas->gc).clip.f.bot)
            y = (grd_canvas->gc).clip.f.bot;
        if (y < (grd_canvas->gc).clip.f.top)
            y = (grd_canvas->gc).clip.f.top;
        pvv->sx = (float)x / 65536.0f + g_XOffset;
        pvv->sy = (float)y / 65536.0f + g_YOffset;
        _D3DTLVERTEX* pvz = pvv;
        if (zlinear)
        {
            pvz->sz = (float)z2d;
        }
        else
        {
            if (lgd3d_z_normal)
            {
                pvz->sz = (float)((p->p).z * inv_z_far);
            }
            else
            {
                pvz->sz = (float)(z1 - (p->grp).w * z2);
                pvz->rhw = (p->grp).w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                        do {} while (0);
                        do {} while (0);
                    }
                }
            }
        }
    }
    DrawIndPolies();
    return 0;
}

void cD6Primitives::DrawLine(r3s_point *p0, r3s_point *p1)
{
    D3DCOLOR color = (m_nAlpha << 24) | pcStates->get_color();
    int left = grd_canvas->gc.clip.f.left;
    int right = grd_canvas->gc.clip.f.right;
    int top = grd_canvas->gc.clip.f.top;
    int bot = grd_canvas->gc.clip.f.bot;

    if (bot - top >= 0x10000 && right - left >= 0x10000) {
        _D3DTLVERTEX* pv = ReservePolySlots(4);
        for (int i = 0; i < 4; i++) {
            _D3DTLVERTEX* pvv = &pv[i];
            pvv->color = color;
            pvv->specular = m_dcFogSpecular;
            if (zlinear) {
                pvv->sz = (float)z2d;
            } else if (lgd3d_z_normal) {
                pvv->sz = (float)(p0->p.z * inv_z_far);
            } else {
                pvv->sz = (float)(z1 - p0->grp.w * z2);
                pvv->rhw = p0->grp.w;
                if (pvv->sz > 1.0) {
                    pvv->sz = 1.0;
                } else if (pvv->sz < 0.0) {
                    pvv->sz = 0.0;
                }
            }
            pvv->tv = 0.0;
            pvv->tu = 0.0;
        }

        int dx = p1->grp.sx - p0->grp.sx;
        int dy = p1->grp.sy - p0->grp.sy;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;

        if (dy > dx) {
            left += 0x8000;
            right = right + (-0x8000);
            if (p0->grp.sy > p1->grp.sy) {
                r3s_point* tmp = p0;
                p0 = p1;
                p1 = tmp;
            }
        } else {
            top += 0x8000;
            bot = bot + (-0x8000);
            if (p0->grp.sx > p1->grp.sx) {
                r3s_point* tmp = p0;
                p0 = p1;
                p1 = tmp;
            }
        }

        int x = p0->grp.sx + 0x8000;
        if (x < left) x = left;
        if (x > right) x = right;
        float sx1 = (float)x / 65536.0f + g_XOffset;

        x = p1->grp.sx + 0x8000;
        if (x < left) x = left;
        if (x > right) x = right;
        float sx2 = (float)x / 65536.0f + g_XOffset;

        int y = p0->grp.sy + 0x8000;
        if (y < top) y = top;
        if (y > bot) y = bot;
        float sy1 = (float)y / 65536.0f + g_YOffset;

        y = p1->grp.sy + 0x8000;
        if (y < top) y = top;
        if (y > bot) y = bot;
        float sy2 = (float)y / 65536.0f + g_YOffset;

        if (dy > dx) {
            pv[0].sx = sx1 - 0.5;
            pv[1].sx = sx1 + 0.5;
            pv[2].sx = sx2 + 0.5;
            pv[3].sx = sx2 - 0.5;
            pv[0].sy = sy1;
            pv[1].sy = sy1;
            pv[2].sy = sy2;
            pv[3].sy = sy2;
        } else {
            pv[0].sy = sy1 + 0.5;
            pv[1].sy = sy1 - 0.5;
            pv[2].sy = sy2 - 0.5;
            pv[3].sy = sy2 + 0.5;
            pv[0].sx = sx1;
            pv[1].sx = sx1;
            pv[2].sx = sx2;
            pv[3].sx = sx2;
        }

        DrawPoly(1);
    }
}
void cD6Primitives::Clear(int c)
{
    _D3DTLVERTEX* pv;
    float x0, y0, x1, y1;
    int i;
    int fc_save = gr_get_fcolor();

    pv = ReservePolySlots(4);

    gr_set_fcolor(c);

    c = pcStates->get_color();

    gr_set_fcolor(fc_save);

    x0 = fix_float(grd_gc.clip.f.left) + g_XOffset;
    x1 = fix_float(grd_gc.clip.f.right) + g_XOffset;
    y0 = fix_float(grd_gc.clip.f.top) + g_YOffset;
    y1 = fix_float(grd_gc.clip.f.bot) + g_YOffset;

    for (i = 0; i < 4; i++)
    {
        pv[i].sz = z2d;
        pv[i].rhw = w2d;
        pv[i].color = c;
        pv[i].specular = m_dcFogSpecular;
    }

    pv[0].sx = x0;
    pv[0].sy = y0;
    pv[1].sx = x1;
    pv[1].sy = y0;
    pv[2].sx = x1;
    pv[2].sy = y1;
    pv[3].sx = x0;
    pv[3].sy = y1;

    DrawPoly(1);
}
void cD6Primitives::StartNonTexMode()
{
    m_iSavedTexId = pcStates->get_texture_id();
    if (m_iSavedTexId != -1)
        pcStates->TurnOffTexuring(TRUE);
}
void cD6Primitives::EndNonTexMode()
{
    if (m_iSavedTexId != -1)
        pcStates->TurnOffTexuring(FALSE);
}
_D3DTLVERTEX* cD6Primitives::ReservePointSlots(int n)
{
    _D3DTLVERTEX* pResult;

    if (m_bPointMode == 0) {
        FlushIndPolies();
        m_bPointMode = 1;
    }
    if (m_dwNoCashedPoints + n > m_dwPointBufferSize)
        FlushPoints();
#line 1552
    AssertMsg(m_dwNoCashedPoints + n <= m_dwPointBufferSize, "ReservePointSlots(): too many points!");


    pResult = &m_saPointBuffer[m_dwNoCashedPoints];
    m_dwNoCashedPoints = m_dwNoCashedPoints + n;
    m_bPrimitivesPending = 1;
    return pResult;
}
void cD6Primitives::FlushPoints()
{

    if (m_dwNoCashedPoints != 0)
    {
        if (g_bTexSuspended == 0)
            StartNonTexMode();
        int hr = ((LPDIRECT3DDEVICE3)g_lpD3Ddevice)->DrawPrimitive(D3DPT_POINTLIST, D3DFVF_TLVERTEX,
            m_saPointBuffer, m_dwNoCashedPoints, D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS);
#line 1581
        CheckHResult(hr, "DrawPrimitive(points) failed");
        m_dwNoCashedPoints = 0;
    }
}


int cD6Primitives::DrawPoint(r3s_point* p)
{
    int x = (p->grp).sx + 0x8000;
    int y = (p->grp).sy + 0x8000;
    if ((x > (grd_canvas->gc).clip.f.right) || (x < (grd_canvas->gc).clip.f.left) ||
        (y > (grd_canvas->gc).clip.f.bot) || (y < (grd_canvas->gc).clip.f.top))
    {
        return 0x10;
    }
    _D3DTLVERTEX* pv = ReservePointSlots(1);
    DWORD color = pcStates->get_color() | 0xff000000;
    pv->sx = (float)x / 65536.0f + g_XOffset;
    pv->sy = (float)y / 65536.0f + g_YOffset;
    pv->color = color;
    pv->specular = m_dcFogSpecular;
    setz(pv, p->p.z, p->grp.w);
    return 0;
}

void cD6Primitives::do_quad_light(r3s_point *p, float r, grs_bitmap *bm)
{
   float x = fix_float(p->grp.sx)+g_XOffset;
   float y = fix_float(p->grp.sy)+g_YOffset;
   float x_right, x_left, y_top, y_bot;
   float u_right, u_left, v_top, v_bot;
   LPD3DTLVERTEX vl;
   D3DCOLOR c, spec;

   if ((x+r < lgd3d_clip.left) || (x-r > lgd3d_clip.right) ||
       (y+r < lgd3d_clip.top) || (y-r > lgd3d_clip.bot))
      return;

   c = pcStates->get_color()|0xff000000;
   spec = m_dcFogSpecular;

   gr_set_fill_type(FILL_BLEND);
   pcStates->SetAlphaPalette(hack_light_alpha_pal);
   lgd3d_set_texture(bm);

   vl = ReservePolySlots(4);
   x_right = x+r; x_left = x-r;
   u_left = 0; u_right = 1.0;
   if (x_left < lgd3d_clip.left) {
      u_left = (lgd3d_clip.left - x_left) / (2*r);
      x_left = lgd3d_clip.left;
   }
   if (x_right > lgd3d_clip.right) {
      u_right = u_left + (u_right - u_left)*(lgd3d_clip.right - x_left) / (x_right - x_left);
      x_right = lgd3d_clip.right;
   }
   y_bot = y+r; y_top = y-r;
   v_top = 0; v_bot = 1.0;
   if (y_top < lgd3d_clip.top) {
      v_top = (lgd3d_clip.top - y_top) / (2*r);
      y_top = lgd3d_clip.top;
   }
   if (y_bot > lgd3d_clip.bot) {
      v_bot = v_top + (1.0 - v_top)*(lgd3d_clip.bot - y_top) / (y_bot - y_top);
      y_bot = lgd3d_clip.bot;
   }

   vl[0].tu = u_left;
   vl[0].tv = v_top;
   vl[0].sx = x_left;
   vl[0].sy = y_top;
   vl[0].color = c;
   vl[0].specular = spec;
   setz(&vl[0], p->p.z, p->grp.w);
   vl[1].tu = u_right;
   vl[1].tv = v_top;
   vl[1].sx = x_right;
   vl[1].sy = y_top;
   vl[1].color = c;
   vl[1].specular = spec;
   setz(&vl[1], p->p.z, p->grp.w);
   vl[2].tu = u_right;
   vl[2].tv = v_bot;
   vl[2].sx = x_right;
   vl[2].sy = y_bot;
   vl[2].color = c;
   vl[2].specular = spec;
   setz(&vl[2], p->p.z, p->grp.w);
   vl[3].tu = u_left;
   vl[3].tv = v_bot;
   vl[3].sx = x_left;
   vl[3].sy = y_bot;
   vl[3].color = c;
   vl[3].specular = spec;
   setz(&vl[3], p->p.z, p->grp.w);

   DrawPoly(0);

   gr_set_fill_type(FILL_NORM);
}

void cD6Primitives::HackLightExtra(r3s_point *p, float r, grs_bitmap *bm)
{
    if (r <= 1.0) {
        float fStack4 = r * r;
        DrawPoint(p);
        (void)fStack4;
    } else {
        do_quad_light(p, r, bm);
    }
}

void cD6Primitives::PassFogSpecularColor(DWORD color)
{
    m_dcFogSpecular = color;
}

void cD6Primitives::PassAlphaColor(int val)
{
    m_nAlpha = val;
}

cD6Primitives* cImBuffer::Instance()
{
    if (m_Instance == 0)
        m_Instance = new cImBuffer();
    return m_Instance;
}

cD6Primitives* cImBuffer::DeInstance()
{
    if (m_Instance != 0) {
        delete m_Instance;
        m_Instance = 0;
    }
    return m_Instance;
}
// Helper: shared position/zlogic for indexed primitives
static void WriteIndPolyVertex(_D3DTLVERTEX* pv, r3s_point* p)
{
    int x = (p->grp).sx + 0x8000;
    int y = (p->grp).sy + 0x8000;
    if (x > (grd_canvas->gc).clip.f.right)
        x = (grd_canvas->gc).clip.f.right;
    if (x < (grd_canvas->gc).clip.f.left)
        x = (grd_canvas->gc).clip.f.left;
    if (y > (grd_canvas->gc).clip.f.bot)
        y = (grd_canvas->gc).clip.f.bot;
    if (y < (grd_canvas->gc).clip.f.top)
        y = (grd_canvas->gc).clip.f.top;
    pv->sx = (float)x / 65536.0f + g_XOffset;
    pv->sy = (float)y / 65536.0f + g_YOffset;
    if (zlinear == 0)
    {
        if (lgd3d_z_normal == 0)
        {
            pv->sz = (float)(z1 - (p->grp).w * z2);
            pv->rhw = (p->grp).w;
            if (pv->sz <= 1.0f)
            {
                if (pv->sz < 0.0f)
                    pv->sz = 0.0f;
            }
            else
            {
                pv->sz = 1.0f;
            }
        }
        else
        {
            pv->sz = (float)((p->p).z * inv_z_far);
        }
    }
    else
    {
        pv->sz = (float)z2d;
    }
}

cImBuffer::cImBuffer()
{
}

cImBuffer::~cImBuffer()
{
    cD6Primitives::~cD6Primitives();
}

cD6Primitives* cMSBuffer::Instance()
{
    if (m_Instance == 0)
        m_Instance = new cMSBuffer();
    return m_Instance;
}

cD6Primitives* cMSBuffer::DeInstance()
{
    if (m_Instance != 0) {
        delete m_Instance;
        m_Instance = 0;
    }
    return m_Instance;
}

cMSBuffer::cMSBuffer() : m_nMaxMTSlots(50)
{
}

cMSBuffer::~cMSBuffer()
{
    cD6Primitives::~cD6Primitives();
}

void cMSBuffer::DrawPoly(int bSuspendTexturing)
{
    if ((m_ePolyMode & 2) != 0)
        bSuspendTexturing = 1;
    if (bSuspendTexturing != g_bTexSuspended)
    {
        FlushIndPolies();
        if (bSuspendTexturing != 0)
            StartNonTexMode();
        else
            EndNonTexMode();
    }
    if (((g_bTexSuspended != 0) || pcStates->EnableMTMode(0) != 0) && !PUNT_D3D)
    {
        if ((m_ePolyMode & 3) != 0)
        {
            int hr = ((LPDIRECT3DDEVICE3)g_lpD3Ddevice)->DrawPrimitive(D3DPT_TRIANGLEFAN,
                D3DFVF_TLVERTEX, m_saVertexBuffer, m_dwNoCashedVertices, D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS);
#line 1953
            CheckHResult(hr, "DrawPrimitive(poly) failed");
        }
        if ((m_ePolyMode & 4) != 0)
            DrawStandardEdges(m_saVertexBuffer, m_dwNoCashedVertices);
    
    }
}

void cMSBuffer::DrawIndPolies()
{
    DWORD j;

#line 1972
    AssertMsg1(m_dwTempIndCounter > 2, "A poly has more than %d points!", m_dwTempIndCounter);


    if ((g_bTexSuspended != 0) || pcStates->EnableMTMode(0) != 0)
    {
        m_saIndexBuffer[m_dwNoIndices] = m_waTempIndices[0];
        m_saIndexBuffer[m_dwNoIndices + 1] = m_waTempIndices[1];
        m_saIndexBuffer[m_dwNoIndices + 2] = m_waTempIndices[2];
        m_dwNoIndices += 3;

        for (j = 3; j < m_dwTempIndCounter; j++)
        {
            m_saIndexBuffer[m_dwNoIndices] = m_waTempIndices[0];
            m_saIndexBuffer[m_dwNoIndices + 1] = m_waTempIndices[j - 1];
            m_saIndexBuffer[m_dwNoIndices + 2] = m_waTempIndices[j];
            m_dwNoIndices += 3;
        }

        m_dwTempIndCounter = 0;
        m_bPrimitivesPending = 1;
    }
}

void cMSBuffer::FlushIndPolies()
{
    DWORD i;
    int result;

    if (m_dwNoIndices != 0)
    {
        if (m_dwMinVIndex != 0) {
            for (i = 0; i < m_dwNoIndices; i++) {
#line 2016
                AssertMsg((DWORD)m_saIndexBuffer[i] <= m_dwMaxVIndex, "Runaway Prim index!");
                m_saIndexBuffer[i] -= (WORD)m_dwMinVIndex;
            }
        }
#ifdef DEBUG
        else {
            for (i = 0; i < m_dwNoIndices; i++) {
#line 2016
                AssertMsg((DWORD)m_saIndexBuffer[i] <= m_dwMaxVIndex, "Runaway Prim index!");
            }
        }
#endif
        {
            DWORD vertex_count = m_dwMaxVIndex + 1 - m_dwMinVIndex;
            if (!PUNT_D3D) {
                result = ((LPDIRECT3DDEVICE3)g_lpD3Ddevice)->DrawIndexedPrimitive(
                    D3DPT_TRIANGLELIST, D3DFVF_TLVERTEX,
                    m_pVIB + m_dwMinVIndex, vertex_count,
                    m_saIndexBuffer, m_dwNoIndices, D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS);
#line 2052
                CheckHResult(result, "DrawIndexedPrimitive(inMT) failed");
            }
        }
        m_dwNoIndices = 0;
        m_dwMinVIndex = 0x100;
        m_dwMaxVIndex = 0;
    }
}

_MTVERTEX* cMSBuffer::ReserveMTPolySlots(int n)
{





    AssertMsg((unsigned int)n <= (unsigned int)m_nMaxMTSlots, "ReservePolySlots(): poly too large!");
    m_nMTVertices = n;
    return m_saMTVertices;
}

void cMSBuffer::DrawMTPoly()
{
    if (g_bTexSuspended != 0) {
        EndNonTexMode();
    }
    if (pcStates->EnableMTMode(1) != 0 && !PUNT_D3D) {
        unsigned int hResult = ((LPDIRECT3DDEVICE3)g_lpD3Ddevice)->DrawPrimitive(
            D3DPT_TRIANGLEFAN, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2,
            m_saMTVertices,
            m_nMTVertices,
            D3DDP_DONOTUPDATEEXTENTS | D3DDP_DONOTCLIP);
#line 2101
        CheckHResult(hResult, "DrawPrimitive(MT) failed");
    }
}

int cMSBuffer::TrifanMTD(int count, _r3s_point** points, LGD3D_tex_coord** tex_coords)
{
    DWORD color = ((DWORD)m_nAlpha << 24) + 0xFFFFFF;
    _MTVERTEX* pv = ReserveMTPolySlots(count);
    for (int i = 0; i < count; i++)
    {
        r3s_point* pp = points[i];
        float u = pp->grp.u; float v = pp->grp.v;
        pv[i].tu = u; pv[i].tv = v;
        float u2 = tex_coords[i]->u; float v2 = tex_coords[i]->v;
        pv[i].tu2 = u2; pv[i].tv2 = v2;
        _MTVERTEX* pvv = &pv[i];
        r3s_point* p = points[i];
        int x = p->grp.sx + 0x8000;
        int y = p->grp.sy + 0x8000;
        if (x > grd_canvas->gc.clip.f.right) x = grd_canvas->gc.clip.f.right;
        if (x < grd_canvas->gc.clip.f.left) x = grd_canvas->gc.clip.f.left;
        if (y > grd_canvas->gc.clip.f.bot) y = grd_canvas->gc.clip.f.bot;
        if (y < grd_canvas->gc.clip.f.top) y = grd_canvas->gc.clip.f.top;
        pvv->sx = (float)x / 65536.0f + g_XOffset;
        pvv->sy = (float)y / 65536.0f + g_YOffset;
        _MTVERTEX* pvz = pvv;
        if (zlinear)
        {
            pvz->sz = (float)z2d;
        }
        else
        {
            if (lgd3d_z_normal)
            {
                pvz->sz = (float)(p->p.z * inv_z_far);
            }
            else
            {
                pvz->sz = (float)(z1 - p->grp.w * z2);
                pvz->rhw = p->grp.w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                    }
                }
            }
        }
        pv[i].color = color;
        pv[i].specular = m_dcFogSpecular;
    }
    DrawMTPoly();
    return 0;
}

int cMSBuffer::LitTrifanMTD(int count, _r3s_point** points, LGD3D_tex_coord** tex_coords)
{
    DWORD base_color = ((DWORD)m_nAlpha << 24) + 0xFFFFFF;
    _MTVERTEX* pv = ReserveMTPolySlots(count);
    for (int i = 0; i < count; i++)
    {
        float u = points[i]->grp.u;
        float v = points[i]->grp.v;
        float inten = points[i]->grp.i;
        if (inten > 1.0)
            inten = 1.0f;
        pv[i].tu = u;
        pv[i].tv = v;
        float u2 = tex_coords[i]->u; float v2 = tex_coords[i]->v;
        pv[i].tu2 = u2; pv[i].tv2 = v2;
        make_scolor(&pv[i].color, base_color, inten);
        pv[i].specular = m_dcFogSpecular;
        _MTVERTEX* pvv = &pv[i];
        r3s_point* p = points[i];
        int x = p->grp.sx + 0x8000;
        int y = p->grp.sy + 0x8000;
        if (x > grd_canvas->gc.clip.f.right) x = grd_canvas->gc.clip.f.right;
        if (x < grd_canvas->gc.clip.f.left) x = grd_canvas->gc.clip.f.left;
        if (y > grd_canvas->gc.clip.f.bot) y = grd_canvas->gc.clip.f.bot;
        if (y < grd_canvas->gc.clip.f.top) y = grd_canvas->gc.clip.f.top;
        pvv->sx = (float)x / 65536.0f + g_XOffset;
        pvv->sy = (float)y / 65536.0f + g_YOffset;
        _MTVERTEX* pvz = pvv;
        if (zlinear)
        {
            pvz->sz = (float)z2d;
        }
        else
        {
            if (lgd3d_z_normal)
            {
                pvz->sz = (float)(p->p.z * inv_z_far);
            }
            else
            {
                pvz->sz = (float)(z1 - p->grp.w * z2);
                pvz->rhw = p->grp.w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                    }
                }
            }
        }
    }
    DrawMTPoly();
    return 0;
}

int cMSBuffer::RGBlitTrifanMTD(int count, _r3s_point** points, LGD3D_tex_coord** tex_coords)
{
    _MTVERTEX* pv = ReserveMTPolySlots(count);
    for (int i = 0; i < count; i++)
    {
        r3s_point* pp = points[i];
        float u = pp->grp.u; float v = pp->grp.v;
        pv[i].tu = u; pv[i].tv = v;
        float u2 = tex_coords[i]->u; float v2 = tex_coords[i]->v;
        pv[i].tu2 = u2; pv[i].tv2 = v2;
        g2s_point* g2p = (g2s_point*)&pp->grp;
        int r = (int)(255.0f * g2p->r);
        if (r > 0xff) r = 0xff;
        int g = (int)(255.0f * g2p->g);
        if (g > 0xff) g = 0xff;
        int b = (int)(255.0f * g2p->b);
        if (b > 0xff) b = 0xff;
        pv[i].color = (((m_nAlpha << 8) | r) << 8 | g) << 8 | b;
        pv[i].specular = m_dcFogSpecular;
        _MTVERTEX* pvv = &pv[i];
        r3s_point* p = points[i];
        int x = p->grp.sx + 0x8000;
        int y = p->grp.sy + 0x8000;
        if (x > grd_canvas->gc.clip.f.right) x = grd_canvas->gc.clip.f.right;
        if (x < grd_canvas->gc.clip.f.left) x = grd_canvas->gc.clip.f.left;
        if (y > grd_canvas->gc.clip.f.bot) y = grd_canvas->gc.clip.f.bot;
        if (y < grd_canvas->gc.clip.f.top) y = grd_canvas->gc.clip.f.top;
        pvv->sx = (float)x / 65536.0f + g_XOffset;
        pvv->sy = (float)y / 65536.0f + g_YOffset;
        _MTVERTEX* pvz = pvv;
        if (zlinear)
        {
            pvz->sz = (float)z2d;
        }
        else
        {
            if (lgd3d_z_normal)
            {
                pvz->sz = (float)(p->p.z * inv_z_far);
            }
            else
            {
                pvz->sz = (float)(z1 - p->grp.w * z2);
                pvz->rhw = p->grp.w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                    }
                }
            }
        }
    }
    DrawMTPoly();
    return 0;
}

int cMSBuffer::RGBAlitTrifanMTD(int count, _r3s_point** points, LGD3D_tex_coord** tex_coords)
{
    _MTVERTEX* pv = ReserveMTPolySlots(count);
    for (int i = 0; i < count; i++)
    {
        r3s_point* pp = points[i];
        float u = pp->grp.u; float v = pp->grp.v;
        pv[i].tu = u; pv[i].tv = v;
        float u2 = tex_coords[i]->u; float v2 = tex_coords[i]->v;
        pv[i].tu2 = u2; pv[i].tv2 = v2;
        g2s_point* g2p = (g2s_point*)&pp->grp;
        int r = (int)(255.0f * g2p->r);
        if (r > 0xff) r = 0xff;
        int g = (int)(255.0f * g2p->g);
        if (g > 0xff) g = 0xff;
        int b = (int)(255.0f * g2p->b);
        if (b > 0xff) b = 0xff;
        int a = (int)(255.0f * g2p->a);
        if (a > 0xff) a = 0xff;
        pv[i].color = (((a << 8) | r) << 8 | g) << 8 | b;
        pv[i].specular = m_dcFogSpecular;
        _MTVERTEX* pvv = &pv[i];
        r3s_point* p = points[i];
        int x = p->grp.sx + 0x8000;
        int y = p->grp.sy + 0x8000;
        if (x > grd_canvas->gc.clip.f.right) x = grd_canvas->gc.clip.f.right;
        if (x < grd_canvas->gc.clip.f.left) x = grd_canvas->gc.clip.f.left;
        if (y > grd_canvas->gc.clip.f.bot) y = grd_canvas->gc.clip.f.bot;
        if (y < grd_canvas->gc.clip.f.top) y = grd_canvas->gc.clip.f.top;
        pvv->sx = (float)x / 65536.0f + g_XOffset;
        pvv->sy = (float)y / 65536.0f + g_YOffset;
        _MTVERTEX* pvz = pvv;
        if (zlinear)
        {
            pvz->sz = (float)z2d;
        }
        else
        {
            if (lgd3d_z_normal)
            {
                pvz->sz = (float)(p->p.z * inv_z_far);
            }
            else
            {
                pvz->sz = (float)(z1 - p->grp.w * z2);
                pvz->rhw = p->grp.w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                    }
                }
            }
        }
    }
    DrawMTPoly();
    return 0;
}

int cMSBuffer::RGBAFogLitTrifanMTD(int count, _r3s_point** points, LGD3D_tex_coord** tex_coords)
{
    _MTVERTEX* pv = ReserveMTPolySlots(count);
    for (int i = 0; i < count; i++)
    {
        r3s_point* pp = points[i];
        float u = pp->grp.u; float v = pp->grp.v;
        pv[i].tu = u; pv[i].tv = v;
        float u2 = tex_coords[i]->u; float v2 = tex_coords[i]->v;
        pv[i].tu2 = u2; pv[i].tv2 = v2;
        g2s_point* g2p = (g2s_point*)&pp->grp;
        int r = (int)(255.0f * g2p->r);
        if (r > 0xff) r = 0xff;
        int g = (int)(255.0f * g2p->g);
        if (g > 0xff) g = 0xff;
        int b = (int)(255.0f * g2p->b);
        if (b > 0xff) b = 0xff;
        int a = (int)(255.0f * g2p->a);
        if (a > 0xff) a = 0xff;
        int fog = (int)((1.0 - g2p->coord[6]) * 255.0);
        if (fog > 0xff) fog = 0xff;
        pv[i].color = (((a << 8) | r) << 8 | g) << 8 | b;
        pv[i].specular = (fog << 24);
        _MTVERTEX* pvv = &pv[i];
        r3s_point* p = points[i];
        int x = p->grp.sx + 0x8000;
        int y = p->grp.sy + 0x8000;
        if (x > grd_canvas->gc.clip.f.right) x = grd_canvas->gc.clip.f.right;
        if (x < grd_canvas->gc.clip.f.left) x = grd_canvas->gc.clip.f.left;
        if (y > grd_canvas->gc.clip.f.bot) y = grd_canvas->gc.clip.f.bot;
        if (y < grd_canvas->gc.clip.f.top) y = grd_canvas->gc.clip.f.top;
        pvv->sx = (float)x / 65536.0f + g_XOffset;
        pvv->sy = (float)y / 65536.0f + g_YOffset;
        _MTVERTEX* pvz = pvv;
        if (zlinear)
        {
            pvz->sz = (float)z2d;
        }
        else
        {
            if (lgd3d_z_normal)
            {
                pvz->sz = (float)(p->p.z * inv_z_far);
            }
            else
            {
                pvz->sz = (float)(z1 - p->grp.w * z2);
                pvz->rhw = p->grp.w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                    }
                }
            }
        }
    }
    DrawMTPoly();
    return 0;
}

int cMSBuffer::DiffuseSpecularLitTrifanMTD(int count, _r3s_point** points, LGD3D_tex_coord** tex_coords)
{
    _MTVERTEX* pv = ReserveMTPolySlots(count);
    for (int i = 0; i < count; i++)
    {
        r3s_point* pp = points[i];
        float u = pp->grp.u; float v = pp->grp.v;
        pv[i].tu = u; pv[i].tv = v;
        float u2 = tex_coords[i]->u; float v2 = tex_coords[i]->v;
        pv[i].tu2 = u2; pv[i].tv2 = v2;
        g2s_point* g2p = (g2s_point*)&pp->grp;
        int r = (int)(255.0f * g2p->r);
        if (r > 0xff) r = 0xff;
        int g = (int)(255.0f * g2p->g);
        if (g > 0xff) g = 0xff;
        int b = (int)(255.0f * g2p->b);
        if (b > 0xff) b = 0xff;
        int a = (int)(255.0f * g2p->a);
        if (a > 0xff) a = 0xff;
        pv[i].color = (((a << 8) | r) << 8 | g) << 8 | b;
        int spec_a = (int)(255.0f * g2p->coord[7]);
        if (spec_a > 0xff) spec_a = 0xff;
        int spec_r = (int)(255.0f * g2p->coord[8]);
        if (spec_r > 0xff) spec_r = 0xff;
        int spec_g = (int)(255.0f * g2p->coord[9]);
        if (spec_g > 0xff) spec_g = 0xff;
        int spec_b = (int)(255.0f * g2p->coord[10]);
        if (spec_b > 0xff) spec_b = 0xff;
        pv[i].specular = (((spec_a << 8) | spec_r) << 8 | spec_g) << 8 | spec_b;
        _MTVERTEX* pvv = &pv[i];
        r3s_point* p = points[i];
        int x = p->grp.sx + 0x8000;
        int y = p->grp.sy + 0x8000;
        if (x > grd_canvas->gc.clip.f.right) x = grd_canvas->gc.clip.f.right;
        if (x < grd_canvas->gc.clip.f.left) x = grd_canvas->gc.clip.f.left;
        if (y > grd_canvas->gc.clip.f.bot) y = grd_canvas->gc.clip.f.bot;
        if (y < grd_canvas->gc.clip.f.top) y = grd_canvas->gc.clip.f.top;
        pvv->sx = (float)x / 65536.0f + g_XOffset;
        pvv->sy = (float)y / 65536.0f + g_YOffset;
        _MTVERTEX* pvz = pvv;
        if (zlinear)
        {
            pvz->sz = (float)z2d;
        }
        else
        {
            if (lgd3d_z_normal)
            {
                pvz->sz = (float)(p->p.z * inv_z_far);
            }
            else
            {
                pvz->sz = (float)(z1 - p->grp.w * z2);
                pvz->rhw = p->grp.w;
                if (pvz->sz > 1.0)
                {
                    pvz->sz = 1.0;
                }
                else
                {
                    if (pvz->sz < 0.0)
                    {
                        pvz->sz = 0.0;
                    }
                }
            }
        }
    }
    DrawMTPoly();
    return 0;
}

int cMSBuffer::g2UTrifanMTD(int n, g2s_point** vpl, LGD3D_tex_coord** pptc)
{
    unsigned int c0 = (m_nAlpha << 24) + 0xffffff;
    _MTVERTEX* vlist = ReserveMTPolySlots(n);
    for (int i = 0; i < n; i++)
    {
        float intensity;
        int _sx, _sy;
        intensity = vpl[i]->i;
        _sx = vpl[i]->sx + 0x8000;
        _sy = vpl[i]->sy + 0x8000;

        if (_sx > grd_canvas->gc.clip.f.right) _sx = grd_canvas->gc.clip.f.right;
        if (_sx < grd_canvas->gc.clip.f.left) _sx = grd_canvas->gc.clip.f.left;
        if (_sy > grd_canvas->gc.clip.f.bot) _sy = grd_canvas->gc.clip.f.bot;
        if (_sy < grd_canvas->gc.clip.f.top) _sy = grd_canvas->gc.clip.f.top;
        vlist[i].sx = (float)_sx / 65536.0f + g_XOffset;
        vlist[i].sy = (float)_sy / 65536.0f + g_YOffset;
        vlist[i].sz = (float)z2d;
        vlist[i].rhw = (float)w2d;
        unsigned int c0_tmp = c0;
        vlist[i].color = (((((int)(((c0_tmp >> 16) & 0xff) * intensity)) << 8) + (int)(((c0_tmp >> 8) & 0xff) * intensity)) << 8) + (int)((c0_tmp & 0xff) * intensity) + (c0_tmp & 0xff000000);
        vlist[i].specular = m_dcFogSpecular;
        float u = vpl[i]->u; float v = vpl[i]->v;
        vlist[i].tu = u; vlist[i].tv = v;
        float u2 = pptc[i]->u; float v2 = pptc[i]->v;
        vlist[i].tu2 = u2; vlist[i].tv2 = v2;
    }
    DrawMTPoly();
    return 0;
}

int cMSBuffer::g2TrifanMTD(int count, g2s_point** points, LGD3D_tex_coord** tex_coords)
{
    g2s_point** clipped = NULL;
    int nclipped = g2_clip_poly(count, 4, points, &clipped);
    int result;
    if (nclipped < 3)
    {
        result = 0x10;
    }
    else
    {
        g2UTrifanMTD(nclipped, clipped, tex_coords);
        result = 0;
    }
    if (clipped && clipped != points)
    {
        temp_free(clipped);
    }
    return result;
}
