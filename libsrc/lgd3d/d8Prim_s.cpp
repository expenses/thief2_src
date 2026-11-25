#include <windows.h>
extern "C" {
#include <g2.h>
}
#include <r3d.h>
#include <d8dx6.h>
#include <lgd3d.h>
#include <d8states.h>
#include <d6prim.h>
#include <d8prim.h>
#include <lgassert.h>
#include <d6check.h>
#include <lglog.h>
#include <dbg.h>
#include <config.h>
#include <tmpalloc.h>
#include <string.h>

extern "C" BOOL lgd3d_punt_d3d;
#define PUNT_D3D lgd3d_punt_d3d

extern WORD waEdgeIndices[50];

#define D8_CUBEVERTEX_FVF (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE3(0))

struct sD8CubeVertex {
    float sx, sy, sz, rhw;
    DWORD color;
    DWORD specular;
    float tu, tv, tw;
};

cD8ImBuffer* cD8ImBuffer::m_Instance = NULL;
cD8MSBuffer* cD8MSBuffer::m_Instance = NULL;

float g_fD8InvRectScale = 1.0f;
float g_fD8HalfTexelX = 0.5f;
float g_fD8HalfTexelY = 0.5f;
int g_nD8HalfTexelFixX = 0x8000;
int g_nD8HalfTexelFixY = 0x8000;

#define make_scolor(pColor, c0, intensity) \
do { \
   ulong __c0 = (c0); \
   float __i = (intensity); \
   *(pColor) = (((((ulong)(((__c0 >> 16) & 0xff) * __i)) << 8) + (ulong)(((__c0 >> 8) & 0xff) * __i)) << 8) + (ulong)((__c0 & 0xff) * __i) + (__c0 & 0xff000000); \
} while (0)

static void D8ScaleTLVertices(_D3DTLVERTEX* pv, int n)
{
    for (; n; n--, pv++) {
        pv->sx *= g_fD8RectScaleX;
        pv->sy *= g_fD8RectScaleY;
    }
}

static void D8ScaleMTVertices(_MTVERTEX* pv, int n)
{
    for (; n; n--, pv++) {
        pv->sx *= g_fD8RectScaleX;
        pv->sy *= g_fD8RectScaleY;
    }
}

static void D8ScaleCubeVertices(sD8CubeVertex* pv, int n)
{
    for (; n; n--, pv++) {
        pv->sx *= g_fD8RectScaleX;
        pv->sy *= g_fD8RectScaleY;
    }
}

static void D8SetVertexPos(_D3DTLVERTEX* pv, r3s_point* p)
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
    if (zlinear)
        pv->sz = (float)z2d;
    else if (lgd3d_z_normal)
        pv->sz = (float)((p->p).z * inv_z_far);
    else {
        pv->sz = (float)(z1 - (p->grp).w * z2);
        if (pv->sz >= 1.0f)
            pv->sz = g_fMaxZ;
        else if (pv->sz < 0.0f)
            pv->sz = 0.0f;
    }
    pv->rhw = (p->grp).w;
}

static void D8SetIndVertexPos(_D3DTLVERTEX* pv, r3s_point* p)
{
    D8SetVertexPos(pv, p);
    pv->sx *= g_fD8RectScaleX;
    pv->sy *= g_fD8RectScaleY;
}

cD8Primitives::cD8Primitives()
{
    if (g_fD8RectScaleX != 1.0f) {
        g_fD8InvRectScale = 1.0f / g_fD8RectScaleX;
        g_fD8HalfTexelX = 0.5 / g_fD8RectScaleX;
        g_fD8HalfTexelY = 0.5 / g_fD8RectScaleY;
        g_nD8HalfTexelFixX = (int)(g_fD8HalfTexelX * 65536.0);
        g_nD8HalfTexelFixY = (int)(g_fD8HalfTexelY * 65536.0);
    } else {
        g_fD8InvRectScale = 1.0f;
        g_nD8HalfTexelFixX = 0x8000;
        g_nD8HalfTexelFixY = 0x8000;
        g_fD8HalfTexelX = 0.5f;
        g_fD8HalfTexelY = 0.5f;
    }
}

cD8Primitives::~cD8Primitives()
{
    g_fD8InvRectScale = 1.0f;
    g_fD8HalfTexelX = 0.5f;
    g_fD8HalfTexelY = g_fD8HalfTexelX;
    g_nD8HalfTexelFixX = 0x8000;
    g_nD8HalfTexelFixY = 0x8000;
}

void cD8Primitives::FlushPoints()
{
    if (m_dwNoCashedPoints) {
        if (!g_bTexSuspended)
            StartNonTexMode();
        D8SetFVF(D8_TLVERTEX_FVF);
        if (g_bD8ScaleRects)
            D8ScaleTLVertices(m_saPointBuffer, m_dwNoCashedPoints);
        HRESULT hr = g_lpD3D9Device->DrawPrimitiveUP(D3DPT_POINTLIST, m_dwNoCashedPoints, m_saPointBuffer,
                                                     sizeof(_D3DTLVERTEX));
#line 121
        CheckHResult(hr, "DrawPrimitive(points) failed");
        m_dwNoCashedPoints = 0;
    }
}

void cD8Primitives::DrawPoly(int bSuspendTexturing)
{
    HRESULT hr;
    if (g_nD8ActiveEffect) {
        if (!PUNT_D3D && (m_ePolyMode & 3)) {
            D8SetFVF(D8_TLVERTEX_FVF);
            if (g_bD8ScaleRects)
                D8ScaleTLVertices(m_saVertexBuffer, m_dwNoCashedVertices);
            hr = g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, m_dwNoCashedVertices - 2, m_saVertexBuffer,
                                                 sizeof(_D3DTLVERTEX));
#line 166
            CheckHResult(hr, "DrawPrimitive(poly) failed");
        }
        return;
    }
    if (m_ePolyMode & 2)
        bSuspendTexturing = 1;
    if (bSuspendTexturing != g_bTexSuspended) {
        FlushIndPolies();
        if (bSuspendTexturing)
            StartNonTexMode();
        else
            EndNonTexMode();
    }
    if (!PUNT_D3D) {
        if (g_bD8ScaleRects)
            D8ScaleTLVertices(m_saVertexBuffer, m_dwNoCashedVertices);
        if (m_ePolyMode & 3) {
            D8SetFVF(D8_TLVERTEX_FVF);
            hr = g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, m_dwNoCashedVertices - 2, m_saVertexBuffer,
                                                 sizeof(_D3DTLVERTEX));
#line 189
            CheckHResult(hr, "DrawPrimitive(poly) failed");
        }
        if (m_ePolyMode & 4)
            DrawStandardEdges(m_saVertexBuffer, m_dwNoCashedVertices);
    }
}

int cD8Primitives::CubeTexTrifan(int n, r3s_point** ppl, float* uvw, DWORD color)
{
    sD8CubeVertex verts[50];
    HRESULT hr;
    for (int i = 0; i < n; i++, uvw += 3) {
        sD8CubeVertex* pv = &verts[i];
        pv->tu = uvw[0];
        pv->tv = uvw[1];
        pv->tw = uvw[2];
        D8SetVertexPos((_D3DTLVERTEX*)pv, ppl[i]);
        pv->color = color;
        pv->specular = m_dcFogSpecular;
    }
    if (g_nD8ActiveEffect) {
        if (!PUNT_D3D && (m_ePolyMode & 3)) {
            D8SetFVF(D8_CUBEVERTEX_FVF);
            if (g_bD8ScaleRects)
                D8ScaleCubeVertices(verts, n);
            hr = g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, n - 2, verts, sizeof(sD8CubeVertex));
#line 223
            CheckHResult(hr, "DrawPrimitive(poly) failed");
        }
        return 0;
    }
    FlushPrimitives();
    int bSuspend = (m_ePolyMode & 2) != 0;
    if (bSuspend != g_bTexSuspended) {
        if (bSuspend)
            StartNonTexMode();
        else
            EndNonTexMode();
    }
    if (!PUNT_D3D && (m_ePolyMode & 3)) {
        D8SetFVF(D8_CUBEVERTEX_FVF);
        if (g_bD8ScaleRects)
            D8ScaleCubeVertices(verts, n);
        hr = g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, n - 2, verts, sizeof(sD8CubeVertex));
#line 249
        CheckHResult(hr, "DrawPrimitive(poly) failed");
    }
    return 0;
}

void cD8Primitives::DrawTLVertices(int n, _D3DTLVERTEX* pVerts)
{
    HRESULT hr;
    D8SetFVF(D8_TLVERTEX_FVF);
    if (g_bD8ScaleRects) {
        _D3DTLVERTEX verts[16];
        for (int i = 0; i < n; i++) {
            verts[i] = pVerts[i];
            verts[i].sx *= g_fD8RectScaleX;
            verts[i].sy *= g_fD8RectScaleY;
        }
        hr = g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, n - 2, verts, sizeof(_D3DTLVERTEX));
#line 282
        CheckHResult(hr, "DrawPrimitive(poly) failed");
    } else {
        hr = g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, n - 2, pVerts, sizeof(_D3DTLVERTEX));
#line 287
        CheckHResult(hr, "DrawPrimitive(poly) failed");
    }
}

void cD8Primitives::DrawMTVertices(int n, void* pVerts)
{
    HRESULT hr;
    D8SetFVF(D8_MT_TLVERTEX_FVF);
    if (g_bD8ScaleRects) {
        _MTVERTEX verts[16];
        _MTVERTEX* pSrc = (_MTVERTEX*)pVerts;
        for (int i = 0; i < n; i++) {
            verts[i] = pSrc[i];
            verts[i].sx *= g_fD8RectScaleX;
            verts[i].sy *= g_fD8RectScaleY;
        }
        hr = g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, n - 2, verts, sizeof(_MTVERTEX));
#line 313
        CheckHResult(hr, "DrawPrimitive(poly) failed");
    } else {
        hr = g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, n - 2, pVerts, sizeof(_MTVERTEX));
#line 318
        CheckHResult(hr, "DrawPrimitive(poly) failed");
    }
}

void cD8Primitives::FlushIndPolies()
{
    if (m_dwNoIndices) {
        if (!PUNT_D3D) {
            D8SetFVF(D8_TLVERTEX_FVF);
            HRESULT hr = g_lpD3D9Device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, m_dwMinVIndex,
                                                                m_dwMaxVIndex - m_dwMinVIndex + 1, m_dwNoIndices / 3,
                                                                m_saIndexBuffer, D3DFMT_INDEX16, m_pVIB,
                                                                sizeof(_D3DTLVERTEX));
#line 339
            CheckHResult(hr, "DrawIndexedPrimitive(inMT) failed");
        }
        m_dwNoIndices = 0;
        m_dwMinVIndex = 0xc00;
        m_dwMaxVIndex = 0;
    }
}

int cD8Primitives::PolyInd(int n, r3s_point** ppl, r3ixs_info* info)
{
    DWORD color = pcStates->get_color();
    ulong* pInfo = (ulong*)info;
    FlushIfNoFit(n, 1);
    for (int i = 0; i < n; i++) {
        _D3DTLVERTEX* pv = GetIndPolySlot(n, (r3ixs_info*)&pInfo[i]);
        if (!pv)
            continue;
        pv->color = color;
        pv->specular = m_dcFogSpecular;
        D8SetIndVertexPos(pv, ppl[i]);
    }
    DrawIndPolies();
    return 0;
}

int cD8Primitives::SPolyInd(int n, r3s_point** ppl, r3ixs_info* info)
{
    DWORD base_color = pcStates->get_color();
    ulong* pInfo = (ulong*)info;
    FlushIfNoFit(n, 1);
    for (int i = 0; i < n; i++) {
        _D3DTLVERTEX* pv = GetIndPolySlot(n, (r3ixs_info*)&pInfo[i]);
        if (!pv)
            continue;
        float inten = ppl[i]->grp.i;
        if (inten > 1.0f)
            inten = 1.0f;
        make_scolor(&pv->color, base_color, inten);
        pv->specular = m_dcFogSpecular;
        D8SetIndVertexPos(pv, ppl[i]);
    }
    DrawIndPolies();
    return 0;
}

int cD8Primitives::RGB_PolyInd(int n, r3s_point** ppl, r3ixs_info* info)
{
    unsigned int color = pcStates->get_color();
    ulong* pInfo = (ulong*)info;
    int r = (color >> 16) & 0xff;
    int g = (color >> 8) & 0xff;
    int b = color & 0xff;
    FlushIfNoFit(n, 1);
    for (int i = 0; i < n; i++) {
        _D3DTLVERTEX* pv = GetIndPolySlot(n, (r3ixs_info*)&pInfo[i]);
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
        D8SetIndVertexPos(pv, ppl[i]);
    }
    DrawIndPolies();
    return 0;
}

int cD8Primitives::RGBA_PolyInd(int n, r3s_point** ppl, r3ixs_info* info)
{
    DWORD base_color = pcStates->get_color();
    ulong* pInfo = (ulong*)info;
    int red = (base_color >> 16) & 0xff;
    int green = (base_color >> 8) & 0xff;
    int blue = base_color & 0xff;
    FlushIfNoFit(n, 1);
    for (int i = 0; i < n; i++) {
        _D3DTLVERTEX* pv = GetIndPolySlot(n, (r3ixs_info*)&pInfo[i]);
        if (!pv)
            continue;
        g2s_point* g2p = (g2s_point*)&ppl[i]->grp;
        int r = (int)(g2p->r * (float)red);
        if (r > 0xff) r = 0xff;
        int g = (int)(g2p->g * (float)green);
        if (g > 0xff) g = 0xff;
        int b = (int)(g2p->b * (float)blue);
        if (b > 0xff) b = 0xff;
        int a = (int)(g2p->a * (float)m_nAlpha);
        if (a > 0xff) a = 0xff;
        pv->color = (a << 24) | (r << 16) | (g << 8) | b;
        pv->specular = m_dcFogSpecular;
        D8SetIndVertexPos(pv, ppl[i]);
    }
    DrawIndPolies();
    return 0;
}

int cD8Primitives::TrifanInd(int n, r3s_point** ppl, r3ixs_info* info)
{
    DWORD color = ((DWORD)m_nAlpha << 24) + 0xFFFFFF;
    ulong* pInfo = (ulong*)info;
    FlushIfNoFit(n, 0);
    for (int i = 0; i < n; i++) {
        _D3DTLVERTEX* pv = GetIndPolySlot(n, (r3ixs_info*)&pInfo[i]);
        if (!pv)
            continue;
        pv->tu = ppl[i]->grp.u;
        pv->tv = ppl[i]->grp.v;
        D8SetIndVertexPos(pv, ppl[i]);
        pv->color = color;
        pv->specular = m_dcFogSpecular;
    }
    DrawIndPolies();
    return 0;
}

int cD8Primitives::LitTrifanInd(int n, r3s_point** ppl, r3ixs_info* info)
{
    DWORD base_color = ((DWORD)m_nAlpha << 24) + 0xFFFFFF;
    ulong* pInfo = (ulong*)info;
    FlushIfNoFit(n, 0);
    for (int i = 0; i < n; i++) {
        _D3DTLVERTEX* pv = GetIndPolySlot(n, (r3ixs_info*)&pInfo[i]);
        if (!pv)
            continue;
        float inten = ppl[i]->grp.i;
        if (inten > 1.0)
            inten = 1.0f;
        pv->tu = ppl[i]->grp.u;
        pv->tv = ppl[i]->grp.v;
        make_scolor(&pv->color, base_color, inten);
        pv->specular = m_dcFogSpecular;
        D8SetIndVertexPos(pv, ppl[i]);
    }
    DrawIndPolies();
    return 0;
}

int cD8Primitives::RGBlitTrifanInd(int n, r3s_point** ppl, r3ixs_info* info)
{
    ulong* pInfo = (ulong*)info;
    FlushIfNoFit(n, 0);
    for (int i = 0; i < n; i++) {
        _D3DTLVERTEX* pv = GetIndPolySlot(n, (r3ixs_info*)&pInfo[i]);
        if (!pv)
            continue;
        r3s_point* pp = ppl[i];
        pv->tu = pp->grp.u;
        pv->tv = pp->grp.v;
        g2s_point* g2p = (g2s_point*)&pp->grp;
        int r = (int)(255.0f * g2p->r);
        if (r > 0xff) r = 0xff;
        int g = (int)(255.0f * g2p->g);
        if (g > 0xff) g = 0xff;
        int b = (int)(255.0f * g2p->b);
        if (b > 0xff) b = 0xff;
        pv->color = (((m_nAlpha << 8) | r) << 8 | g) << 8 | b;
        pv->specular = m_dcFogSpecular;
        D8SetIndVertexPos(pv, pp);
    }
    DrawIndPolies();
    return 0;
}

int cD8Primitives::RGBAlitTrifanInd(int n, r3s_point** ppl, r3ixs_info* info)
{
    ulong* pInfo = (ulong*)info;
    FlushIfNoFit(n, 0);
    for (int i = 0; i < n; i++) {
        _D3DTLVERTEX* pv = GetIndPolySlot(n, (r3ixs_info*)&pInfo[i]);
        if (!pv)
            continue;
        r3s_point* pp = ppl[i];
        pv->tu = pp->grp.u;
        pv->tv = pp->grp.v;
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
        D8SetIndVertexPos(pv, pp);
    }
    DrawIndPolies();
    return 0;
}

int cD8Primitives::RGBAFogLitTrifanInd(int n, r3s_point** ppl, r3ixs_info* info)
{
    ulong* pInfo = (ulong*)info;
    FlushIfNoFit(n, 0);
    for (int i = 0; i < n; i++) {
        _D3DTLVERTEX* pv = GetIndPolySlot(n, (r3ixs_info*)&pInfo[i]);
        if (!pv)
            continue;
        r3s_point* pp = ppl[i];
        pv->tu = pp->grp.u;
        pv->tv = pp->grp.v;
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
        D8SetIndVertexPos(pv, pp);
    }
    DrawIndPolies();
    return 0;
}

int cD8Primitives::DiffuseSpecularLitTrifanInd(int n, r3s_point** ppl, r3ixs_info* info)
{
    ulong* pInfo = (ulong*)info;
    FlushIfNoFit(n, 0);
    for (int i = 0; i < n; i++) {
        _D3DTLVERTEX* pv = GetIndPolySlot(n, (r3ixs_info*)&pInfo[i]);
        if (!pv)
            continue;
        r3s_point* pp = ppl[i];
        pv->tu = pp->grp.u;
        pv->tv = pp->grp.v;
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
        D8SetIndVertexPos(pv, pp);
    }
    DrawIndPolies();
    return 0;
}

void cD8Primitives::DrawStandardEdges(void* pVerts, unsigned long n)
{
    if (!g_bTexSuspended) {
        FlushIndPolies();
        StartNonTexMode();
    }
    waEdgeIndices[n] = 0;
    D8SetFVF(D8_TLVERTEX_FVF);
    HRESULT hr = g_lpD3D9Device->DrawIndexedPrimitiveUP(D3DPT_LINESTRIP, 0, n, n, waEdgeIndices, D3DFMT_INDEX16,
                                                        pVerts, sizeof(_D3DTLVERTEX));
    waEdgeIndices[n] = (WORD)n;
#line 732
    CheckHResult(hr, "DrawStandardEdges failed");
}

cD6Primitives* cD8ImBuffer::Instance()
{
    if (!m_Instance)
        m_Instance = new cD8ImBuffer();
    return m_Instance;
}

cD6Primitives* cD8ImBuffer::DeInstance()
{
    if (m_Instance) {
        delete m_Instance;
        m_Instance = NULL;
    }
    return m_Instance;
}

cD6Primitives* cD8MSBuffer::Instance()
{
    if (!m_Instance) {
        if (config_is_nonzero("d3d_disp_no_batch"))
            m_Instance = new cD8MSBuffer();
        else
            m_Instance = new cD8MSBatchBuffer();
    }
    return m_Instance;
}

cD6Primitives* cD8MSBuffer::DeInstance()
{
    if (m_Instance) {
        delete m_Instance;
        m_Instance = NULL;
    }
    return m_Instance;
}

int cD8MSBuffer::TrifanMTD(int count, _r3s_point** points, LGD3D_tex_coord** tex_coords)
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
                float z = (float)(z1 - p->grp.w * z2);
                pvz->sz = z;
                if (z >= 1.0)
                    pvz->sz = g_fMaxZ;
                else if (z < 0.0)
                    pvz->sz = 0.0;
            }
        }
        pvz->rhw = p->grp.w;
        pv[i].color = color;
        pv[i].specular = m_dcFogSpecular;
    }
    DrawMTPoly();
    return 0;
}

int cD8MSBuffer::LitTrifanMTD(int count, _r3s_point** points, LGD3D_tex_coord** tex_coords)
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
                float z = (float)(z1 - p->grp.w * z2);
                pvz->sz = z;
                if (z >= 1.0)
                    pvz->sz = g_fMaxZ;
                else if (z < 0.0)
                    pvz->sz = 0.0;
            }
        }
        pvz->rhw = p->grp.w;
    }
    DrawMTPoly();
    return 0;
}

int cD8MSBuffer::RGBlitTrifanMTD(int count, _r3s_point** points, LGD3D_tex_coord** tex_coords)
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
                float z = (float)(z1 - p->grp.w * z2);
                pvz->sz = z;
                if (z >= 1.0)
                    pvz->sz = g_fMaxZ;
                else if (z < 0.0)
                    pvz->sz = 0.0;
            }
        }
        pvz->rhw = p->grp.w;
    }
    DrawMTPoly();
    return 0;
}

int cD8MSBuffer::RGBAlitTrifanMTD(int count, _r3s_point** points, LGD3D_tex_coord** tex_coords)
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
                float z = (float)(z1 - p->grp.w * z2);
                pvz->sz = z;
                if (z >= 1.0)
                    pvz->sz = g_fMaxZ;
                else if (z < 0.0)
                    pvz->sz = 0.0;
            }
        }
        pvz->rhw = p->grp.w;
    }
    DrawMTPoly();
    return 0;
}

int cD8MSBuffer::RGBAFogLitTrifanMTD(int count, _r3s_point** points, LGD3D_tex_coord** tex_coords)
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
                float z = (float)(z1 - p->grp.w * z2);
                pvz->sz = z;
                if (z >= 1.0)
                    pvz->sz = g_fMaxZ;
                else if (z < 0.0)
                    pvz->sz = 0.0;
            }
        }
        pvz->rhw = p->grp.w;
    }
    DrawMTPoly();
    return 0;
}

int cD8MSBuffer::DiffuseSpecularLitTrifanMTD(int count, _r3s_point** points, LGD3D_tex_coord** tex_coords)
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
                float z = (float)(z1 - p->grp.w * z2);
                pvz->sz = z;
                if (z >= 1.0)
                    pvz->sz = g_fMaxZ;
                else if (z < 0.0)
                    pvz->sz = 0.0;
            }
        }
        pvz->rhw = p->grp.w;
    }
    DrawMTPoly();
    return 0;
}

int cD8MSBuffer::g2UTrifanMTD(int n, g2s_point** vpl, LGD3D_tex_coord** pptc)
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

int cD8MSBuffer::g2TrifanMTD(int count, g2s_point** points, LGD3D_tex_coord** tex_coords)
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

int cD8MSBuffer::CubeTexTrifan(int n, r3s_point** ppl, float* uvw, DWORD color)
{
    sD8CubeVertex verts[50];
    HRESULT hr;
    for (int i = 0; i < n; i++, uvw += 3) {
        sD8CubeVertex* pv = &verts[i];
        pv->tu = uvw[0];
        pv->tv = uvw[1];
        pv->tw = uvw[2];
        D8SetVertexPos((_D3DTLVERTEX*)pv, ppl[i]);
        pv->color = color;
        pv->specular = m_dcFogSpecular;
    }
    if (g_nD8ActiveEffect) {
        if (!PUNT_D3D && (m_ePolyMode & 3)) {
            D8SetFVF(D8_CUBEVERTEX_FVF);
            if (g_bD8ScaleRects)
                D8ScaleCubeVertices(verts, n);
            hr = g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, n - 2, verts, sizeof(sD8CubeVertex));
#line 1084
            CheckHResult(hr, "DrawPrimitive(poly) failed");
        }
        return 0;
    }
    FlushPrimitives();
    int bSuspend = (m_ePolyMode & 2) != 0;
    if (bSuspend != g_bTexSuspended) {
        if (bSuspend)
            StartNonTexMode();
        else
            EndNonTexMode();
    }
    if (!g_bTexSuspended && !pcStates->EnableMTMode(0))
        return 0;
    if (!PUNT_D3D && (m_ePolyMode & 3)) {
        D8SetFVF(D8_CUBEVERTEX_FVF);
        if (g_bD8ScaleRects)
            D8ScaleCubeVertices(verts, n);
        hr = g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, n - 2, verts, sizeof(sD8CubeVertex));
#line 1110
        CheckHResult(hr, "DrawPrimitive(poly) failed");
    }
    return 0;
}

int cD8MSBuffer::TexTrifanMT(int n, r3s_point** ppl, float* uv, LGD3D_tex_coord** lm, DWORD color)
{
    _MTVERTEX* pv = ReserveMTPolySlots(n);
    for (int i = 0; i < n; i++, uv += 2) {
        pv[i].tu = uv[0];
        pv[i].tv = uv[1];
        pv[i].tu2 = lm[i]->u;
        pv[i].tv2 = lm[i]->v;
        D8SetVertexPos((_D3DTLVERTEX*)&pv[i], ppl[i]);
        pv[i].color = color;
        pv[i].specular = m_dcFogSpecular;
    }
    DrawMTPoly();
    return 0;
}

void cD8MSBuffer::DrawPoly(int bSuspendTexturing)
{
    HRESULT hr;
    if (g_nD8ActiveEffect) {
        if (!PUNT_D3D && (m_ePolyMode & 3)) {
            D8SetFVF(D8_TLVERTEX_FVF);
            if (g_bD8ScaleRects)
                D8ScaleTLVertices(m_saVertexBuffer, m_dwNoCashedVertices);
            hr = g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, m_dwNoCashedVertices - 2, m_saVertexBuffer,
                                                 sizeof(_D3DTLVERTEX));
#line 1168
            CheckHResult(hr, "DrawPrimitive(poly) failed");
        }
        return;
    }
    if (m_ePolyMode & 2)
        bSuspendTexturing = 1;
    if (bSuspendTexturing != g_bTexSuspended) {
        FlushIndPolies();
        if (bSuspendTexturing)
            StartNonTexMode();
        else
            EndNonTexMode();
    }
    if (!g_bTexSuspended && !pcStates->EnableMTMode(0))
        return;
    if (!PUNT_D3D) {
        if (g_bD8ScaleRects)
            D8ScaleTLVertices(m_saVertexBuffer, m_dwNoCashedVertices);
        if (m_ePolyMode & 3) {
            D8SetFVF(D8_TLVERTEX_FVF);
            hr = g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, m_dwNoCashedVertices - 2, m_saVertexBuffer,
                                                 sizeof(_D3DTLVERTEX));
#line 1193
            CheckHResult(hr, "DrawPrimitive(poly) failed");
        }
        if (m_ePolyMode & 4)
            DrawStandardEdges(m_saVertexBuffer, m_dwNoCashedVertices);
    }
}

void cD8MSBuffer::FlushIndPolies()
{
    if (m_dwNoIndices) {
        if (!PUNT_D3D) {
            D8SetFVF(D8_TLVERTEX_FVF);
            HRESULT hr = g_lpD3D9Device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, m_dwMinVIndex,
                                                                m_dwMaxVIndex - m_dwMinVIndex + 1, m_dwNoIndices / 3,
                                                                m_saIndexBuffer, D3DFMT_INDEX16, m_pVIB,
                                                                sizeof(_D3DTLVERTEX));
#line 1219
            CheckHResult(hr, "DrawIndexedPrimitive(inMT) failed");
        }
        m_dwNoIndices = 0;
        m_dwMinVIndex = 0xc00;
        m_dwMaxVIndex = 0;
    }
}

void cD8MSBuffer::DrawIndPolies()
{
    if (g_bTexSuspended || pcStates->EnableMTMode(0)) {
        m_saIndexBuffer[m_dwNoIndices] = m_waTempIndices[0];
        m_saIndexBuffer[m_dwNoIndices + 1] = m_waTempIndices[1];
        m_saIndexBuffer[m_dwNoIndices + 2] = m_waTempIndices[2];
        m_dwNoIndices += 3;
        for (DWORD j = 3; j < m_dwTempIndCounter; j++) {
            m_saIndexBuffer[m_dwNoIndices] = m_waTempIndices[0];
            m_saIndexBuffer[m_dwNoIndices + 1] = m_waTempIndices[j - 1];
            m_saIndexBuffer[m_dwNoIndices + 2] = m_waTempIndices[j];
            m_dwNoIndices += 3;
        }
        m_dwTempIndCounter = 0;
        m_bPrimitivesPending = 1;
    }
}

_MTVERTEX* cD8MSBuffer::ReserveMTPolySlots(int n)
{
    m_nMTVertices = n;
    return m_saMTVertices;
}

void cD8MSBuffer::DrawMTPoly()
{
    HRESULT hr;
    if (g_nD8ActiveEffect) {
        if (PUNT_D3D)
            return;
        D8SetFVF(D8_MT_TLVERTEX_FVF);
        if (g_bD8ScaleRects)
            D8ScaleMTVertices(m_saMTVertices, m_nMTVertices);
        hr = g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, m_nMTVertices - 2, m_saMTVertices, sizeof(_MTVERTEX));
#line 1271
        CheckHResult(hr, "DrawPrimitive(MT) failed");
        return;
    }
    if (g_bTexSuspended)
        EndNonTexMode();
    if (!pcStates->EnableMTMode(1))
        return;
    if (PUNT_D3D)
        return;
    D8SetFVF(D8_MT_TLVERTEX_FVF);
    if (g_bD8ScaleRects)
        D8ScaleMTVertices(m_saMTVertices, m_nMTVertices);
    hr = g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, m_nMTVertices - 2, m_saMTVertices, sizeof(_MTVERTEX));
#line 1285
    CheckHResult(hr, "DrawPrimitive(MT) failed");
}

static DWORD g_dwD8BatchStamp;

static BOOL D8NeedsOrderedBatch()
{
    sRenderStates* rs = pcStates->m_psSetRS;
    if (rs->eZenable && rs->bZWriteEnable && rs->eZCompareFunc == D3DCMP_LESSEQUAL)
        return FALSE;
    return TRUE;
}

static BOOL D8IsTranslucentTexture(int n)
{
    if (pcStates->m_psSetRS->nAlphaColor >= 0xff) {
        if (n <= -1)
            return FALSE;
        sTextureData* psTD = &g_saTextures[n];
        uchar flags = psTD->TdrvCookie.flags;
        int nFormat = flags & PF_FORMAT_MASK;
        if (!PF_ALPHA_FORMAT(nFormat))
            return FALSE;
        if (!g_nAlphaTestMode || !(flags & (PF_TRANS | PF_ALPHA_1BIT)))
            return TRUE;
        return g_nAlphaTestRef == 0;
    }
    return TRUE;
}

static void D8FanToList(_D3DTLVERTEX* pv, int n)
{
    if (n > 3) {
        _D3DTLVERTEX* pSrc = &pv[n - 1];
        _D3DTLVERTEX* pDst = &pv[(n - 2) * 3 - 1];
        for (int i = n - 3; i > 0; i--) {
            pDst[0] = *pSrc--;
            pDst[-1] = *pSrc;
            pDst[-2] = pv[0];
            pDst -= 3;
        }
    }
}

static void D8FanToListMT(_MTVERTEX* pv, int n)
{
    if (n > 3) {
        _MTVERTEX* pSrc = &pv[n - 1];
        _MTVERTEX* pDst = &pv[(n - 2) * 3 - 1];
        for (int i = n - 3; i > 0; i--) {
            pDst[0] = *pSrc--;
            pDst[-1] = *pSrc;
            pDst[-2] = pv[0];
            pDst -= 3;
        }
    }
}

static void D8SetBatchTexture(int n)
{
    if (pcStates->m_psSetRS->naTextureId[0] != n || pcStates->m_psCurrentRS->naTextureId[0] != n)
        ((cD8MSStates*)pcStates)->cD8MSStates::set_texture_id(n);
}

cD8MSBatchBuffer::cD8MSBatchBuffer()
{
    int i;
    m_nPendingVerts = 0;
    m_nCurTL = 0;
    m_nCurMT = 0;
    m_nMaxTLTris = D8_TL_BATCH_TRIS;
    m_bOrdered = FALSE;
    m_bMTMode = FALSE;
    m_nOrderedTL = 0;
    for (i = 0; i < D8_NUM_TL_BATCHES; i++) {
        m_aTL[i].nTexId = D8_BATCH_FREE;
        m_aTL[i].dwStamp = 0;
        m_aTL[i].nTris = 0;
        m_aTL[i].pVerts = m_aTLVerts[i];
    }
    for (i = 0; i < D8_NUM_MT_BATCHES; i++) {
        m_aMT[i].naTexId[0] = D8_BATCH_FREE;
        m_aMT[i].naTexId[1] = D8_BATCH_FREE;
        m_aMT[i].dwStamp = 0;
        m_aMT[i].nTris = 0;
        m_aMT[i].pVerts = m_aMTVerts[i];
        m_aMT[i].pLightMap = NULL;
    }
}

void cD8MSBatchBuffer::SelectTLBatch(int n)
{
    cD8MSBatchBuffer* pBuf = (cD8MSBatchBuffer*)pcRenderBuffer;
    int i;
    DWORD dwOldest;

    if (pBuf->m_bMTMode) {
        pBuf->SelectMTBatch(0, n);
        return;
    }
    if (pBuf->m_aTL[pBuf->m_nCurTL].nTexId == n)
        return;
    if (D8NeedsOrderedBatch())
        goto flush;
    if (pBuf->m_nMaxTLTris > D8_TL_BATCH_TRIS) {
        if (pBuf->m_aTL[pBuf->m_nCurTL].nTris >= D8_TL_BATCH_TRIS) {
            pBuf->FlushBatches();
            D8SetBatchTexture(n);
            return;
        }
        pBuf->m_nMaxTLTris = D8_TL_BATCH_TRIS;
    }
    for (i = 0; i < D8_NUM_TL_BATCHES; i++) {
        if (pBuf->m_aTL[i].nTexId == n) {
            pBuf->m_aTL[i].dwStamp = g_dwD8BatchStamp++;
            pBuf->m_nCurTL = i;
            return;
        }
    }
    {
        BOOL bOrdered = pBuf->m_bOrdered;
        BOOL bStartOrdered = FALSE;
        BOOL bTranslucent = D8IsTranslucentTexture(n);
        if (!bOrdered) {
            if (bTranslucent)
                bStartOrdered = TRUE;
        } else if (bTranslucent)
            goto flush;

        if (!bOrdered) {
            pBuf->m_nCurTL = 0;
            dwOldest = pBuf->m_aTL[0].dwStamp;
            for (i = 1; i < D8_NUM_TL_BATCHES; i++) {
                if (pBuf->m_aTL[i].dwStamp < dwOldest) {
                    pBuf->m_nCurTL = i;
                    dwOldest = pBuf->m_aTL[i].dwStamp;
                }
            }
        } else {
            pBuf->m_nCurTL = pBuf->m_nOrderedTL == 0;
            dwOldest = pBuf->m_aTL[pBuf->m_nCurTL].dwStamp;
            for (i = 0; i < D8_NUM_TL_BATCHES; i++) {
                if (i != pBuf->m_nOrderedTL && pBuf->m_aTL[i].dwStamp < dwOldest) {
                    pBuf->m_nCurTL = i;
                    dwOldest = pBuf->m_aTL[i].dwStamp;
                }
            }
        }
        pBuf->FlushTLBatch();
        pBuf->m_aTL[pBuf->m_nCurTL].nTexId = n;
        pBuf->m_aTL[pBuf->m_nCurTL].dwStamp = g_dwD8BatchStamp++;
        if (bStartOrdered) {
            pBuf->m_bOrdered = TRUE;
            pBuf->m_nOrderedTL = pBuf->m_nCurTL;
        }
        return;
    }
flush:
    pBuf->FlushBatches();
    D8SetBatchTexture(n);
}

void cD8MSBatchBuffer::SelectMTBatch(DWORD dwLevel, int n)
{
    cD8MSBatchStates* ps = (cD8MSBatchStates*)pcStates;
    int i;

    if (!m_bMTMode) {
        pcStates->SetTextureLevel(0);
        FlushBatches();
        pcStates->SetTextureLevel(1);
        if (pcStates->m_psSetRS->naTextureId[1] != n || pcStates->m_psCurrentRS->naTextureId[1] != n)
            ps->cD8MSStates::set_texture_id(n);
        return;
    }
    if (m_aMT[m_nCurMT].naTexId[dwLevel] == n)
        return;
    if (!m_bOrdered && !D8NeedsOrderedBatch()) {
        DWORD dwOther = dwLevel ^ 1;
        int nOther = m_aMT[m_nCurMT].naTexId[dwOther];
        for (i = 0; i < D8_NUM_MT_BATCHES; i++) {
            sD8MTBatch* pb = &m_aMT[i];
            if (pb->naTexId[dwOther] == nOther) {
                if (pb->naTexId[dwLevel] == n || pb->naTexId[dwLevel] == D8_BATCH_FREE || !pb->nTris) {
                    pb->naTexId[dwLevel] = n;
                    pb->dwStamp = g_dwD8BatchStamp++;
                    m_nCurMT = i;
                    return;
                }
            }
        }
        DWORD dwOldest = m_aMT[0].dwStamp;
        m_nCurMT = 0;
        for (i = 1; i < D8_NUM_MT_BATCHES; i++) {
            if (m_aMT[i].dwStamp < dwOldest) {
                m_nCurMT = i;
                dwOldest = m_aMT[i].dwStamp;
            }
        }
        FlushMTBatch();
        sD8MTBatch* pb = &m_aMT[m_nCurMT];
        pb->naTexId[dwLevel] = n;
        pb->naTexId[dwOther] = nOther;
        pb->dwStamp = g_dwD8BatchStamp++;
        pb->pLightMap = ps->m_LastLightMapBm;
        return;
    }
    FlushBatches();
    if (pcStates->m_psSetRS->naTextureId[dwLevel] != n || pcStates->m_psCurrentRS->naTextureId[dwLevel] != n)
        ps->cD8MSStates::set_texture_id(n);
}

void cD8MSBatchBuffer::FlushTLBatch()
{
    cD8MSBatchStates* ps = (cD8MSBatchStates*)pcStates;
    sD8TLBatch* pb = &m_aTL[m_nCurTL];
    if (pb->nTris) {
        BOOL bBatching = ps->m_bBatching;
        ps->m_bBatching = FALSE;
        if (!g_bTexSuspended && !g_nD8ActiveEffect) {
            m_bFlushing++;
            D8SetBatchTexture(pb->nTexId);
            pcStates->EnableMTMode(0);
            m_bFlushing--;
        }
        D8SetFVF(D8_TLVERTEX_FVF);
        HRESULT hr = g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, pb->nTris, pb->pVerts, sizeof(_D3DTLVERTEX));
#line 1639
        CheckHResult(hr, "DrawPrimitive(trilist) failed");
        pb->nTris = 0;
        ps->m_bBatching = bBatching;
    }
    pb->nTexId = D8_BATCH_FREE;
    if (m_nCurTL == m_nOrderedTL)
        m_bOrdered = FALSE;
}

void cD8MSBatchBuffer::FlushMTBatch()
{
    cD8MSBatchStates* ps = (cD8MSBatchStates*)pcStates;
    sD8MTBatch* pb = &m_aMT[m_nCurMT];
    if (pb->nTris) {
        BOOL bBatching = ps->m_bBatching;
        ps->m_bBatching = FALSE;
        if (!g_bTexSuspended && !g_nD8ActiveEffect) {
            m_bFlushing++;
            DWORD dwLevel = pcStates->GetTextureLevel();
            pcStates->SetTextureLevel(1);
            if (pcStates->m_psSetRS->naTextureId[1] != pb->naTexId[1] ||
                pcStates->m_psCurrentRS->naTextureId[1] != pb->naTexId[1])
                ps->cD8MSStates::set_texture_id(pb->naTexId[1]);
            pcStates->SetTextureLevel(0);
            D8SetBatchTexture(pb->naTexId[0]);
            pcStates->SetTextureLevel(dwLevel);
            ps->m_LastLightMapBm = pb->pLightMap;
            pcStates->EnableMTMode(1);
            m_bFlushing--;
        }
        D8SetFVF(D8_MT_TLVERTEX_FVF);
        HRESULT hr = g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, pb->nTris, pb->pVerts, sizeof(_MTVERTEX));
#line 1683
        CheckHResult(hr, "DrawPrimitive(trilist) failed");
        pb->nTris = 0;
        ps->m_bBatching = bBatching;
    }
    pb->naTexId[0] = D8_BATCH_FREE;
    pb->naTexId[1] = D8_BATCH_FREE;
    pb->pLightMap = NULL;
}

void cD8MSBatchBuffer::FlushPoints()
{
    if (m_dwNoCashedPoints) {
        FlushIndPolies();
        cD8Primitives::FlushPoints();
    }
}

void cD8MSBatchBuffer::FlushIndPolies()
{
    FlushBatches();
    cD8MSBuffer::FlushIndPolies();
}

void cD8MSBatchBuffer::FlushBatches()
{
    cD8MSBatchStates* ps = (cD8MSBatchStates*)pcStates;
    int i, nFirst;

    if (!ps->m_bBatching)
        return;
    ps->m_bBatching = FALSE;
    grs_bitmap* pLightMap = ps->m_LastLightMapBm;
    if (!m_bMTMode) {
        int nTex = pcStates->m_psCurrentRS->naTextureId[0];
        if (!m_bOrdered || m_nMaxTLTris != D8_ORDERED_BATCH_TRIS) {
            nFirst = -1;
            for (i = 0; i < D8_NUM_TL_BATCHES; i++) {
                m_nCurTL = (m_nCurTL + 1) % D8_NUM_TL_BATCHES;
                if (m_aTL[m_nCurTL].nTexId == nTex) {
                    FlushTLBatch();
                    nFirst = m_nCurTL;
                }
            }
            for (i = 0; i < D8_NUM_TL_BATCHES; i++) {
                m_nCurTL = (m_nCurTL + 1) % D8_NUM_TL_BATCHES;
                if (m_nCurTL != nFirst)
                    FlushTLBatch();
            }
        } else {
            for (i = 0; i < D8_NUM_TL_BATCHES; i++) {
                m_nCurTL = (m_nCurTL + 1) % D8_NUM_TL_BATCHES;
                if (m_nCurTL != m_nOrderedTL && m_aTL[m_nCurTL].nTexId == nTex)
                    FlushTLBatch();
            }
            for (i = 0; i < D8_NUM_TL_BATCHES; i++) {
                m_nCurTL = (m_nCurTL + 1) % D8_NUM_TL_BATCHES;
                if (m_nCurTL != m_nOrderedTL)
                    FlushTLBatch();
            }
            m_nCurTL = m_nOrderedTL;
            FlushTLBatch();
        }
        if (!g_nD8ActiveEffect) {
            if (!g_bTexSuspended)
                D8SetBatchTexture(m_iSavedTexId);
            else
                pcStates->m_psCurrentRS->naTextureId[0] = m_iSavedTexId;
        }
        m_nCurTL = 0;
        m_bOrdered = FALSE;
        m_nOrderedTL = 0;
    }
    int nTex0 = pcStates->m_psCurrentRS->naTextureId[0];
    int nTex1 = pcStates->m_psCurrentRS->naTextureId[1];
    nFirst = -1;
    for (i = 0; i < D8_NUM_MT_BATCHES; i++) {
        m_nCurMT = (m_nCurMT + 1) % D8_NUM_MT_BATCHES;
        if (m_aMT[m_nCurMT].naTexId[0] == nTex0 && m_aMT[m_nCurMT].naTexId[1] == nTex1) {
            FlushMTBatch();
            nFirst = m_nCurMT;
        }
    }
    for (i = 0; i < D8_NUM_MT_BATCHES; i++) {
        m_nCurMT = (m_nCurMT + 1) % D8_NUM_MT_BATCHES;
        if (m_nCurMT != nFirst)
            FlushMTBatch();
    }
    if (g_nD8ActiveEffect)
        return;
    if (!pcStates->GetTextureLevel()) {
        pcStates->SetTextureLevel(1);
        if (pcStates->m_psSetRS->naTextureId[1] != nTex1 || pcStates->m_psCurrentRS->naTextureId[1] != nTex1)
            ps->cD8MSStates::set_texture_id(nTex1);
        pcStates->SetTextureLevel(0);
        D8SetBatchTexture(m_iSavedTexId);
    } else {
        pcStates->SetTextureLevel(0);
        D8SetBatchTexture(m_iSavedTexId);
        pcStates->SetTextureLevel(1);
        if (pcStates->m_psSetRS->naTextureId[1] != nTex1 || pcStates->m_psCurrentRS->naTextureId[1] != nTex1)
            ps->cD8MSStates::set_texture_id(nTex1);
    }
    ps->m_LastLightMapBm = pLightMap;
}

_D3DTLVERTEX* cD8MSBatchBuffer::ReservePolySlots(int n)
{
    if (m_bPointMode) {
        FlushPoints();
        m_bPointMode = 0;
    }
    if (m_bMTMode && ((cD8MSBatchStates*)pcStates)->m_bBatching)
        FlushIndPolies();
    sD8TLBatch* pb = &m_aTL[m_nCurTL];
    m_nPendingVerts = n;
    if (pb->nTris + n - 2 > m_nMaxTLTris) {
        if (m_bOrdered && m_nCurTL == m_nOrderedTL) {
            FlushIndPolies();
            return m_aTL[m_nCurTL].pVerts;
        }
        int nTex = pb->nTexId;
        FlushTLBatch();
        pb->nTexId = nTex;
    }
    return pb->pVerts + pb->nTris * 3;
}

_MTVERTEX* cD8MSBatchBuffer::ReserveMTPolySlots(int n)
{
    if (m_bPointMode) {
        FlushPoints();
        m_bPointMode = 0;
    }
    if (!m_bMTMode && ((cD8MSBatchStates*)pcStates)->m_bBatching)
        FlushIndPolies();
    sD8MTBatch* pb = &m_aMT[m_nCurMT];
    m_nPendingVerts = n;
    if (pb->nTris + n - 2 > D8_MT_BATCH_TRIS) {
        int nTex0 = pb->naTexId[0];
        int nTex1 = pb->naTexId[1];
        FlushMTBatch();
        pb->naTexId[1] = nTex1;
        pb->naTexId[0] = nTex0;
    }
    return pb->pVerts + pb->nTris * 3;
}

void cD8MSBatchBuffer::DrawPoly(int bSuspendTexturing)
{
    cD8MSBatchStates* ps = (cD8MSBatchStates*)pcStates;
    sD8TLBatch* pb;
    int n;
    _D3DTLVERTEX* pv;

    if (g_nD8ActiveEffect) {
        if (!ps->m_bBatching || m_bMTMode) {
            FlushIndPolies();
            ps->m_bBatching = TRUE;
            m_nOrderedTL = m_nCurTL;
            m_bMTMode = FALSE;
            m_bOrdered = TRUE;
            m_nMaxTLTris = D8_ORDERED_BATCH_TRIS;
            m_aTL[m_nCurTL].dwStamp = g_dwD8BatchStamp++;
            m_aTL[m_nCurTL].nTexId = -1;
            m_bPrimitivesPending = TRUE;
        }
        if (PUNT_D3D || !(m_ePolyMode & 3))
            return;
        n = m_nPendingVerts;
        pb = &m_aTL[m_nCurTL];
        pv = pb->pVerts + pb->nTris * 3;
        if (g_bD8ScaleRects)
            D8ScaleTLVertices(pv, n);
        D8FanToList(pv, n);
        pb->nTris += n - 2;
        m_bPrimitivesPending = TRUE;
        return;
    }
    n = m_nPendingVerts;
    pb = &m_aTL[m_nCurTL];
    pv = pb->pVerts + pb->nTris * 3;
    if (m_ePolyMode & 2)
        bSuspendTexturing = 1;
    if (bSuspendTexturing != g_bTexSuspended) {
        FlushIndPolies();
        if (bSuspendTexturing)
            StartNonTexMode();
        else
            EndNonTexMode();
    } else {
        if (ps->m_bBatching)
            goto draw;
        FlushIndPolies();
    }
    if (!g_bTexSuspended)
        pcStates->EnableMTMode(0);
    ps->m_bBatching = TRUE;
    m_bMTMode = FALSE;
    {
        sRenderStates* rs = pcStates->m_psSetRS;
        if (!rs->bAlphaOn && rs->eZenable && rs->bZWriteEnable && rs->eZCompareFunc == D3DCMP_LESSEQUAL)
            m_bOrdered = FALSE;
        else {
            m_bOrdered = TRUE;
            m_nOrderedTL = m_nCurTL;
        }
        m_nMaxTLTris = D8_ORDERED_BATCH_TRIS;
        pb = &m_aTL[m_nCurTL];
        pb->dwStamp = g_dwD8BatchStamp++;
        pb->nTexId = rs->naTextureId[0];
    }
    if (pv != pb->pVerts) {
        memmove(pb->pVerts, pv, n * sizeof(_D3DTLVERTEX));
        pv = pb->pVerts;
    }
    m_bPrimitivesPending = TRUE;
draw:
    if (PUNT_D3D)
        return;
    if (g_bD8ScaleRects)
        D8ScaleTLVertices(pv, n);
    if (m_ePolyMode & 4) {
        FlushIndPolies();
        if (m_ePolyMode & 3) {
            D8SetFVF(D8_TLVERTEX_FVF);
            HRESULT hr = g_lpD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, n - 2, pv, sizeof(_D3DTLVERTEX));
#line 2053
            CheckHResult(hr, "DrawPrimitive(poly) failed");
        }
        DrawStandardEdges(pv, n);
    }
    if (m_ePolyMode & 3) {
        D8FanToList(pv, n);
        pb->nTris += n - 2;
        m_bPrimitivesPending = TRUE;
    }
}

void cD8MSBatchBuffer::DrawIndPolies()
{
    if (((cD8MSBatchStates*)pcStates)->m_bBatching)
        FlushBatches();
    cD8MSBuffer::DrawIndPolies();
}

void cD8MSBatchBuffer::DrawMTPoly()
{
    cD8MSBatchStates* ps = (cD8MSBatchStates*)pcStates;
    sD8MTBatch* pb;
    int n;
    _MTVERTEX* pv;

    if (g_nD8ActiveEffect) {
        if (!ps->m_bBatching || !m_bMTMode) {
            FlushIndPolies();
            ps->m_bBatching = TRUE;
            m_bMTMode = TRUE;
            m_bOrdered = TRUE;
            pb = &m_aMT[m_nCurMT];
            pb->dwStamp = g_dwD8BatchStamp++;
            pb->naTexId[0] = -1;
            pb->naTexId[1] = -1;
            pb->pLightMap = NULL;
            m_bPrimitivesPending = TRUE;
        }
        if (PUNT_D3D)
            return;
        n = m_nPendingVerts;
        pb = &m_aMT[m_nCurMT];
        pv = pb->pVerts + pb->nTris * 3;
        if (g_bD8ScaleRects)
            D8ScaleMTVertices(pv, n);
        D8FanToListMT(pv, n);
        pb->nTris += n - 2;
        m_bPrimitivesPending = TRUE;
        return;
    }
    n = m_nPendingVerts;
    pb = &m_aMT[m_nCurMT];
    pv = pb->pVerts + pb->nTris * 3;
    if (!g_bTexSuspended) {
        if (ps->m_bBatching)
            goto draw;
        FlushIndPolies();
    } else {
        FlushIndPolies();
        EndNonTexMode();
    }
    pcStates->EnableMTMode(1);
    ps->m_bBatching = TRUE;
    m_bMTMode = TRUE;
    {
        sRenderStates* rs = pcStates->m_psSetRS;
        m_bOrdered = !(!rs->bAlphaOn && rs->eZenable && rs->bZWriteEnable && rs->eZCompareFunc == D3DCMP_LESSEQUAL);
        pb = &m_aMT[m_nCurMT];
        pb->naTexId[0] = rs->naTextureId[0];
        pb->naTexId[1] = rs->naTextureId[1];
        pb->dwStamp = g_dwD8BatchStamp++;
        pb->pLightMap = ps->m_LastLightMapBm;
    }
    if (pv != pb->pVerts) {
        memmove(pb->pVerts, pv, n * sizeof(_MTVERTEX));
        pv = pb->pVerts;
    }
    m_bPrimitivesPending = TRUE;
draw:
    if (PUNT_D3D)
        return;
    if (g_bD8ScaleRects)
        D8ScaleMTVertices(pv, n);
    D8FanToListMT(pv, n);
    pb->nTris += n - 2;
    m_bPrimitivesPending = TRUE;
}
