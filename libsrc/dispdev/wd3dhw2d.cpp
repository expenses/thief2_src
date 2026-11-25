#include <windows.h>
#include <d3d9.h>
#include <lg.h>
#include <ddraw.h>
#include <math.h>

#include <comtools.h>
#include <dispapi.h>
#include <appagg.h>

#include <dev2d.h>
#include <fonts.h>
#include <comdfcn.h>

#include <wdisp.h>
#include <wdspprov.h>
#include <wdspmode.h>

#include <wdispd3d.h>
#include <wd3dmode.h>
#include <wd3dhw2d.h>

IHW2DRenderer * g_pHW2D;
extern "C" BOOL g_b2DTexFilter = TRUE;
uint g_HW2DBilinear;
static uchar g_HW2DDummyBits[4];
static sHW2DVertex2 g_HW2DPolyVerts[HW2D_MAX_POLY_VERTS + 1];

BOOL D8_CreateBitmapTexture(IDirect3DDevice9 * pDevice, IDirect3DTexture9 ** ppTexture, DWORD dwFlags, grs_bitmap * pBm);

DEFINE_LG_GUID_UNCONDITIONAL(IID_IHW2DRenderer, 0x3a1);

static uchar * BitmapEnd(grs_bitmap * pBm)
{
    if (pBm->type == BMT_MONO)
        return pBm->bits + (pBm->h - 1) * pBm->row + (pBm->w >> 3);
    if (pBm->type == BMT_RSD8)
        return pBm->bits + ((pBm->h + 3) / 4) * pBm->row;
    return pBm->bits + (pBm->h - 1) * pBm->row + pBm->bpp * pBm->w;
}

///////////////////////////////////////

cD3DHW2DCache::cD3DHW2DCache(cD3DModeOps * pModeOps, int nCacheSize)
  : m_pModeOps(pModeOps),
    m_bActive(FALSE),
    m_State(0),
    m_ZMode(0),
    m_ZBias(0.0),
    m_bColorWrite(TRUE),
    m_bFirst(FALSE),
    m_pCanvasBits(NULL),
    m_CanvasRowBytes(0),
    m_pCurEntry(NULL),
    m_CurBits(0),
    m_CurW(0),
    m_CurH(0),
    m_bFullTexture(TRUE),
    m_Color(0),
    m_pSavedTarget(NULL),
    m_pSavedDepth(NULL),
    m_pDepthStencil(NULL),
    m_bStencil(FALSE),
    m_pReadSurface(NULL),
    m_ReadW(0),
    m_ReadH(0),
    m_pEntries(NULL),
    m_nFree(0),
    m_nBytes(0),
    m_nFrame(0),
    m_nPurgeFrame(0),
    m_nCacheSize(nCacheSize),
    m_nPrimType(0),
    m_nBatched(0)
{
    if (!g_pHW2D)
        g_pHW2D = this;
    memset(&m_Viewport, 0, sizeof(m_Viewport));
    m_Viewport.MaxZ = 1.0f;
    memset(m_Quad, 0, sizeof(m_Quad));
    for (int i = 0; i < 4; i++)
    {
        m_Quad[i].z = 1.0f;
        m_Quad[i].rhw = 1.0f;
    }
    m_Quad[1].u = 1.0f;
    m_Quad[2].u = 1.0f;
    m_Quad[2].v = 1.0f;
    m_Quad[3].v = 1.0f;
}

cD3DHW2DCache::~cD3DHW2DCache()
{
    if (g_pHW2D == this)
        g_pHW2D = NULL;
    m_nBatched = 0;
    Flush();
    while (m_pEntries)
        FreeEntry(m_pEntries);
    SafeRelease(m_pSavedTarget);
    SafeRelease(m_pSavedDepth);
    SafeRelease(m_pDepthStencil);
    SafeRelease(m_pReadSurface);
}

STDMETHODIMP cD3DHW2DCache::QueryInterface(REFIID id, void ** ppI)
{
    if (id == IID_IHW2DRenderer || id == IID_IUnknown)
    {
        *ppI = this;
        return S_OK;
    }
    return E_FAIL;
}

STDMETHODIMP_(ULONG) cD3DHW2DCache::AddRef()
{
    return 1;
}

STDMETHODIMP_(ULONG) cD3DHW2DCache::Release()
{
    return 1;
}

///////////////////////////////////////

void cD3DHW2DCache::FlushQuads()
{
    if (m_nBatched)
    {
        m_pModeOps->m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, m_nBatched, m_Batch, sizeof(sHW2DVertex));
        m_nBatched = 0;
    }
}

STDMETHODIMP_(void) cD3DHW2DCache::FlushBatch()
{
    FlushQuads();
}

void cD3DHW2DCache::AddQuad()
{
    if (m_nPrimType != 0)
    {
        FlushQuads();
        m_nPrimType = 0;
    }
    sHW2DVertex * pV = &m_Batch[m_nBatched * 3];
    m_nBatched += 2;
    pV[0] = m_Quad[0];
    pV[1] = m_Quad[1];
    pV[2] = m_Quad[2];
    pV[3] = m_Quad[0];
    pV[4] = m_Quad[2];
    pV[5] = m_Quad[3];
    if (m_nBatched >= HW2D_MAX_TRIS)
        FlushQuads();
}

void cD3DHW2DCache::AddLineQuad()
{
    if (m_nPrimType != 1)
    {
        FlushQuads();
        m_nPrimType = 1;
    }
    sHW2DVertex * pV = &m_Batch[m_nBatched * 3];
    m_nBatched += 2;
    pV[0] = m_Quad[0];
    pV[1] = m_Quad[1];
    pV[2] = m_Quad[2];
    pV[3] = m_Quad[0];
    pV[4] = m_Quad[2];
    pV[5] = m_Quad[3];
    if (m_nBatched >= HW2D_MAX_TRIS)
        FlushQuads();
}

void cD3DHW2DCache::AddFan(sHW2DVertex * pVerts, int n)
{
    if (m_nPrimType == 2)
    {
        if (m_nBatched + n - 2 >= HW2D_MAX_TRIS)
            FlushQuads();
    }
    else
    {
        FlushQuads();
        m_nPrimType = 2;
    }

    float dx = (float) m_Viewport.X - m_pModeOps->m_fTexelOffsetX;
    float dy = (float) m_Viewport.Y - m_pModeOps->m_fTexelOffsetY;
    sHW2DVertex * pV = &m_Batch[m_nBatched * 3];
    m_nBatched += n - 2;

    sHW2DVertex v0 = pVerts[0];
    v0.x += dx;
    v0.y += dy;
    sHW2DVertex prev = pVerts[1];
    prev.x += dx;
    prev.y += dy;
    for (int i = 2; i < n; i++)
    {
        sHW2DVertex cur = pVerts[i];
        cur.x += dx;
        cur.y += dy;
        pV[0] = v0;
        pV[1] = prev;
        pV[2] = cur;
        pV += 3;
        prev = cur;
    }
}

///////////////////////////////////////

void cD3DHW2DCache::Link(sHW2DEntry * pEntry)
{
    m_nFree++;
    m_nBytes += pEntry->nBytes;
    if (m_pEntries)
    {
        pEntry->pNext = m_pEntries;
        pEntry->pPrev = NULL;
        m_pEntries->pPrev = pEntry;
        m_pEntries = pEntry;
    }
    else
    {
        m_pEntries = pEntry;
        pEntry->pNext = NULL;
        pEntry->pPrev = NULL;
    }
}

void cD3DHW2DCache::Unlink(sHW2DEntry * pEntry)
{
    if (!(pEntry->flags & kHW2DEntryLocked))
    {
        m_nFree--;
        m_nBytes -= pEntry->nBytes;
    }
    if (pEntry == m_pCurEntry)
    {
        FlushQuads();
        SetTexture(NULL);
    }
    if (!pEntry->pPrev)
    {
        m_pEntries = m_pEntries->pNext;
        if (m_pEntries)
            m_pEntries->pPrev = NULL;
    }
    else
    {
        pEntry->pPrev->pNext = pEntry->pNext;
        if (pEntry->pNext)
            pEntry->pNext->pPrev = pEntry->pPrev;
    }
}

void cD3DHW2DCache::Touch(sHW2DEntry * pEntry)
{
    if (pEntry->pPrev)
    {
        pEntry->pPrev->pNext = pEntry->pNext;
        if (pEntry->pNext)
            pEntry->pNext->pPrev = pEntry->pPrev;
        pEntry->pNext = m_pEntries;
        pEntry->pPrev = NULL;
        m_pEntries->pPrev = pEntry;
        m_pEntries = pEntry;
    }
    pEntry->nFrame = m_nFrame;
}

void cD3DHW2DCache::FreeEntry(sHW2DEntry * pEntry)
{
    Unlink(pEntry);
    SafeRelease(pEntry->pTexture[0]);
    SafeRelease(pEntry->pTexture[1]);
    delete pEntry;
}

void cD3DHW2DCache::Purge()
{
    if (!m_pEntries)
        return;

    m_nPurgeFrame = m_nFrame;
    sHW2DEntry * pEntry = m_pEntries->pNext;
    if (!pEntry)
        return;
    while (pEntry->pNext)
        pEntry = pEntry->pNext;
    while (pEntry->pPrev)
    {
        sHW2DEntry * pPrev = pEntry->pPrev;
        if (!(pEntry->flags & kHW2DEntryLocked))
            FreeEntry(pEntry);
        pEntry = pPrev;
    }
}

void cD3DHW2DCache::FlushAll()
{
    Purge();
}

sHW2DEntry * cD3DHW2DCache::NewEntry(grs_bitmap * pBm, DWORD flags)
{
    sHW2DEntry * pEntry = new sHW2DEntry;

    pBm->exflags |= 4;
    pEntry->pBits = pBm->bits;
    pEntry->pEnd = BitmapEnd(pBm);
    pEntry->pBitmap = pBm;
    pEntry->w = pBm->w;
    pEntry->h = pBm->h;
    pEntry->pTexture[0] = NULL;
    pEntry->pTexture[1] = NULL;
    pEntry->nFrame = m_nFrame;
    if (pBm->type == BMT_FLAT32)
        pEntry->nBytes = pBm->w * pBm->h * 4;
    else if (pBm->type == BMT_RSD8)
        pEntry->nBytes = ((pBm->h + 3) / 4) * pBm->row;
    else
        pEntry->nBytes = pBm->w * pBm->h * (pBm->bpp ? pBm->bpp : 1);
    pEntry->flags = flags;
    return pEntry;
}

sHW2DEntry * cD3DHW2DCache::FindEntry(grs_bitmap * pBm)
{
    for (sHW2DEntry * pEntry = m_pEntries; pEntry; pEntry = pEntry->pNext)
        if (pEntry->pBitmap == pBm)
            return pEntry;
    return NULL;
}

STDMETHODIMP_(sHW2DEntry *) cD3DHW2DCache::Cache(grs_bitmap * pBm)
{
    if (!pBm)
        return NULL;

    uchar * pBits = pBm->bits;
    uchar * pEnd = BitmapEnd(pBm);
    sHW2DEntry * pEntry = m_pEntries;
    while (pEntry)
    {
        if (pBits >= pEntry->pBits && pEnd <= pEntry->pEnd)
        {
            if (pEntry->flags & kHW2DEntryStale)
                break;
            Touch(pEntry);
            if (pEntry->pBitmap == pBm && (pEntry->flags & (kHW2DEntryTarget | kHW2DEntryDirty)) == (kHW2DEntryTarget | kHW2DEntryDirty))
            {
                MakeTarget(pBm);
                return m_pEntries;
            }
            return pEntry;
        }
        if ((pBits < pEntry->pBits || pEnd > pEntry->pEnd) && pEnd >= pEntry->pBits && pBits <= pEntry->pEnd)
            break;
        pEntry = pEntry->pNext;
    }
    if (pEntry)
        FreeEntry(pEntry);

    if (pBm->exflags & 8)
    {
        Link(NewEntry(pBm, kHW2DEntryTarget));
        MakeTarget(pBm);
        return m_pEntries;
    }
    pEntry = NewEntry(pBm, 0);
    D8_CreateBitmapTexture(m_pModeOps->m_pDevice, &pEntry->pTexture[0], 0, pBm);
    if (!pEntry->pTexture[0])
        pEntry->nBytes = 0;
    Link(pEntry);
    return pEntry;
}

sHW2DEntry * cD3DHW2DCache::CacheExact(grs_bitmap * pBm)
{
    sHW2DEntry * pEntry;

    if (!pBm)
        return NULL;
    for (pEntry = m_pEntries; pEntry; pEntry = pEntry->pNext)
    {
        if (pEntry->pBitmap != pBm)
            continue;
        if (!(pEntry->flags & kHW2DEntryStale))
        {
            Touch(pEntry);
            if ((pEntry->flags & (kHW2DEntryTarget | kHW2DEntryDirty)) == (kHW2DEntryTarget | kHW2DEntryDirty))
            {
                MakeTarget(pBm);
                return m_pEntries;
            }
            return pEntry;
        }
        FreeEntry(pEntry);
        break;
    }
    pEntry = NewEntry(pBm, 0);
    D8_CreateBitmapTexture(m_pModeOps->m_pDevice, &pEntry->pTexture[0], 0, pBm);
    if (!pEntry->pTexture[0])
        pEntry->nBytes = 0;
    Link(pEntry);
    return pEntry;
}

STDMETHODIMP_(void) cD3DHW2DCache::Pin(grs_bitmap * pBm)
{
    sHW2DEntry * pEntry = Cache(pBm);
    if (pEntry && !(pEntry->flags & kHW2DEntryLocked))
    {
        pEntry->flags |= kHW2DEntryLocked;
        m_nFree--;
        m_nBytes -= pEntry->nBytes;
    }
}

STDMETHODIMP_(void) cD3DHW2DCache::AddFont(grs_font * pFont)
{
    grs_bitmap bm;

    if (!pFont)
        return;
    for (sHW2DEntry * pEntry = m_pEntries; pEntry; pEntry = pEntry->pNext)
    {
        if (pEntry->pBitmap == (grs_bitmap *) pFont)
        {
            if (!(pEntry->flags & kHW2DEntryStale))
            {
                Touch(pEntry);
                return;
            }
            FreeEntry(pEntry);
            break;
        }
    }

    int type = (pFont->id && (pFont->id < 3 || pFont->id == 0xcccc)) ? BMT_FLAT8 : BMT_MONO;
    gr_init_bitmap(&bm, (uchar *) pFont + pFont->buf, type, BMF_TRANS, pFont->w, pFont->h);
    if (pFont->id == 0xcccc)
        bm.align = pFont->pal_id;
    if (pFont->id == 1)
        bm.flags |= 0xf00;
    else if (pFont->id == 2)
        bm.flags = (bm.flags & ~0x100) | 0xe00;

    sHW2DEntry * pEntry = NewEntry(&bm, 0);
    pEntry->pBitmap = (grs_bitmap *) pFont;
    pEntry->nBytes = (pFont->id == 0xcccc) ? bm.w * bm.h * 4 : bm.h * bm.row;
    D8_CreateBitmapTexture(m_pModeOps->m_pDevice, &pEntry->pTexture[0], 0, &bm);
    if (!pEntry->pTexture[0])
        pEntry->nBytes = 0;
    Link(pEntry);
    pEntry->flags |= kHW2DEntryLocked;
    m_nFree--;
    m_nBytes -= pEntry->nBytes;
    gr_close_bitmap(&bm);
}

STDMETHODIMP_(void) cD3DHW2DCache::Invalidate(grs_bitmap * pBm)
{
    if (!pBm)
        return;

    if (pBm->exflags & 4)
    {
        sHW2DEntry * pEntry = FindEntry(pBm);
        pBm->exflags &= ~4;
        if (!pEntry)
            return;
        if (!(pEntry->flags & (kHW2DEntryUploaded | kHW2DEntryTarget)))
        {
            FreeEntry(pEntry);
            return;
        }
        if (pEntry->flags & kHW2DEntryLocked)
        {
            pEntry->flags &= ~kHW2DEntryLocked;
            m_nFree++;
            m_nBytes += pEntry->nBytes;
        }
        if (pEntry->flags & kHW2DEntryTarget)
            pEntry->flags |= kHW2DEntryDirty;
        pEntry->flags |= kHW2DEntryStale;
        return;
    }

    uchar * pBits = pBm->bits;
    uchar * pEnd = BitmapEnd(pBm);
    sHW2DEntry * pEntry = m_pEntries;
    while (pEntry)
    {
        sHW2DEntry * pNext = pEntry->pNext;
        if (pBits <= pEntry->pEnd && pEnd >= pEntry->pBits)
        {
            if (!(pEntry->flags & (kHW2DEntryUploaded | kHW2DEntryTarget)))
                FreeEntry(pEntry);
            else
            {
                if (pEntry->flags & kHW2DEntryLocked)
                {
                    pEntry->flags &= ~kHW2DEntryLocked;
                    m_nFree++;
                    m_nBytes += pEntry->nBytes;
                }
                if (pEntry->flags & kHW2DEntryTarget)
                    pEntry->flags |= kHW2DEntryDirty;
                pEntry->flags |= kHW2DEntryStale;
            }
        }
        pEntry = pNext;
    }
}

STDMETHODIMP_(void) cD3DHW2DCache::Discard(grs_bitmap * pBm)
{
    if (!pBm)
        return;
    sHW2DEntry * pEntry = FindEntry(pBm);
    if (pEntry)
        FreeEntry(pEntry);
}

STDMETHODIMP_(void) cD3DHW2DCache::Update(grs_bitmap * pBm)
{
    if (!pBm)
        return;

    if (pBm->exflags & 4)
    {
        sHW2DEntry * pEntry = FindEntry(pBm);
        if (!pEntry)
        {
            pBm->exflags &= ~4;
            return;
        }
        if (pEntry->flags & kHW2DEntryTarget)
        {
            pEntry->flags |= kHW2DEntryDirty;
            return;
        }
        pBm->exflags &= ~4;
        if (!(pEntry->flags & kHW2DEntryUploaded))
        {
            FreeEntry(pEntry);
            return;
        }
        if (pEntry->flags & kHW2DEntryLocked)
        {
            pEntry->flags &= ~kHW2DEntryLocked;
            m_nFree++;
            m_nBytes += pEntry->nBytes;
        }
        pEntry->flags |= kHW2DEntryStale;
        return;
    }

    uchar * pBits = pBm->bits;
    uchar * pEnd = BitmapEnd(pBm);
    sHW2DEntry * pEntry = m_pEntries;
    while (pEntry)
    {
        sHW2DEntry * pNext = pEntry->pNext;
        if (pBits <= pEntry->pEnd && pEnd >= pEntry->pBits)
        {
            if (pEntry->flags & kHW2DEntryTarget)
            {
                pEntry->flags |= kHW2DEntryDirty;
                return;
            }
            if (!(pEntry->flags & kHW2DEntryUploaded))
                FreeEntry(pEntry);
            else
            {
                if (pEntry->flags & kHW2DEntryLocked)
                {
                    pEntry->flags &= ~kHW2DEntryLocked;
                    m_nFree++;
                    m_nBytes += pEntry->nBytes;
                }
                pEntry->flags |= kHW2DEntryStale;
            }
        }
        pEntry = pNext;
    }
}

///////////////////////////////////////

void cD3DHW2DCache::GetCanvasOrigin(int * pX, int * pY)
{
    cD3DModeOps * pOps = m_pModeOps;
    uchar * pBase = pOps->m_pDisplayDevice->m_pModeInfo ? pOps->m_pDisplayDevice->m_pModeInfo->vbase : g_HW2DDummyBits;
    int offset = grd_canvas->bm.bits - pBase;
    *pY = offset / grd_canvas->bm.row;
    *pX = (offset - *pY * grd_canvas->bm.row) / grd_canvas->bm.bpp;
}

void cD3DHW2DCache::SetViewport()
{
    if ((uchar *) m_pCanvasBits != grd_canvas->bm.bits)
    {
        FlushQuads();
        m_pCanvasBits = grd_canvas->bm.bits;
        m_Viewport.Width = grd_canvas->bm.w;
        m_Viewport.Height = grd_canvas->bm.h;
        GetCanvasOrigin((int *) &m_Viewport.X, (int *) &m_Viewport.Y);
        m_pModeOps->m_pDevice->SetViewport(&m_Viewport);
        return;
    }
    if (m_Viewport.Width != (DWORD) grd_canvas->bm.w || m_Viewport.Height != (DWORD) grd_canvas->bm.h)
    {
        FlushQuads();
        m_Viewport.Width = grd_canvas->bm.w;
        m_Viewport.Height = grd_canvas->bm.h;
        m_pModeOps->m_pDevice->SetViewport(&m_Viewport);
    }
}

void cD3DHW2DCache::SetTexture(sHW2DEntry * pEntry)
{
    if (m_pCurEntry == pEntry)
        return;
    m_pCurEntry = pEntry;
    FlushQuads();
    if (!pEntry)
    {
        m_pModeOps->m_pDevice->SetTexture(0, NULL);
        return;
    }
    m_pModeOps->m_pDevice->SetTexture(0, pEntry->pTexture[pEntry->flags & kHW2DEntryFlip]);
    m_CurBits = 0;
    m_CurW = 0;
    m_CurH = 0;
}

void cD3DHW2DCache::SetTextureCoords(sHW2DEntry * pEntry, grs_bitmap * pBm)
{
    if (m_pCurEntry == pEntry)
    {
        if (!pEntry)
            return;
        if ((int) pBm->bits == m_CurBits && pBm->w == m_CurW && pBm->h == m_CurH && !(pEntry->flags & kHW2DEntryRawFont))
            return;
    }
    else
    {
        m_pCurEntry = pEntry;
        FlushQuads();
        if (!pEntry)
        {
            m_pModeOps->m_pDevice->SetTexture(0, NULL);
            return;
        }
        m_pModeOps->m_pDevice->SetTexture(0, pEntry->pTexture[pEntry->flags & kHW2DEntryFlip]);
    }

    m_CurBits = (int) pBm->bits;
    m_CurW = pBm->w;
    m_CurH = pBm->h;
    if (pBm->bits == pEntry->pBits && pBm->w == pEntry->w && pBm->h == pEntry->h)
    {
        if (!m_bFullTexture)
        {
            m_bFullTexture = TRUE;
            m_Quad[0].u = 0.0f;
            m_Quad[0].v = 0.0f;
            m_Quad[1].u = 1.0f;
            m_Quad[1].v = 0.0f;
            m_Quad[2].u = 1.0f;
            m_Quad[2].v = 1.0f;
            m_Quad[3].u = 0.0f;
            m_Quad[3].v = 1.0f;
        }
        return;
    }

    m_bFullTexture = FALSE;
    DWORD offset = pBm->bits - pEntry->pBits;
    DWORD row, col;
    if (!(pEntry->flags & kHW2DEntryFont))
    {
        if (pBm->type == BMT_MONO)
        {
            row = offset / pBm->row;
            col = pBm->align + (offset - pBm->row * row) * 8;
        }
        else if (pBm->type == BMT_RSD8)
        {
            col = offset % pBm->row;
            row = (offset / pBm->row) * 4;
            col = ((pBm->flags & 0xf00) == 0x100) ? (col >> 3) * 4 : (col >> 4) * 4;
        }
        else
        {
            DWORD rowBytes = pEntry->pBitmap->row;
            row = offset / rowBytes;
            col = (offset - rowBytes * row) / pEntry->pBitmap->bpp;
        }
    }
    else
    {
        short fontRow = ((grs_font *) pEntry->pBitmap)->w;
        row = offset / fontRow;
        col = offset - fontRow * row;
        if (pEntry->flags & kHW2DEntryRawFont)
            col = pBm->align + col * 8;
    }

    float u0 = (float) col / (float) pEntry->w;
    float v0 = (float) row / (float) pEntry->h;
    float u1 = (float) pBm->w / (float) pEntry->w + u0;
    float v1 = (float) pBm->h / (float) pEntry->h + v0;
    m_Quad[0].u = u0;
    m_Quad[0].v = v0;
    m_Quad[1].u = u1;
    m_Quad[1].v = v0;
    m_Quad[2].u = u1;
    m_Quad[2].v = v1;
    m_Quad[3].u = u0;
    m_Quad[3].v = v1;
}

void cD3DHW2DCache::SetState(uint flags)
{
    IDirect3DDevice9 * pDev;

    if (flags == m_State)
        return;
    FlushQuads();
    uint changed = m_State ^ flags;
    m_State = flags;
    pDev = m_pModeOps->m_pDevice;

    if (changed & (kHW2DBlendMask | kHW2DAlphaTest))
    {
        DWORD ref = ((flags & kHW2DPolygon) && (flags & kHW2DAlphaTest)) ? 0x7f : 0;
        switch (flags & kHW2DBlendMask)
        {
            case 0:
            case 1:
            case 4:
                pDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
                pDev->SetRenderState(D3DRS_ALPHATESTENABLE, (flags >> 5) & 1);
                pDev->SetRenderState(D3DRS_ALPHAREF, ref);
                break;
            case kHW2DBlendAlpha:
                pDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
                pDev->SetRenderState(D3DRS_ALPHATESTENABLE, (flags >> 5) & 1);
                pDev->SetRenderState(D3DRS_ALPHAREF, ref);
                pDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR);
                pDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCALPHA);
                break;
            case kHW2DBlendAdd:
                pDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
                pDev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
                pDev->SetRenderState(D3DRS_ALPHAREF, 0);
                pDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
                pDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
                break;
        }
    }

    if (changed & (kHW2DTextured | kHW2DAlphaTest | kHW2DAlphaTexture | kHW2DDiffuse))
    {
        DWORD alphaArg;
        if (!(flags & kHW2DDiffuse))
        {
            if (changed & kHW2DDiffuse)
            {
                pDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
                pDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
            }
            if (!(flags & (kHW2DTextured | kHW2DAlphaTest | kHW2DAlphaTexture)))
            {
                SetTexture(NULL);
                pDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
                alphaArg = D3DTA_DIFFUSE;
            }
            else
            {
                pDev->SetTextureStageState(0, D3DTSS_COLORARG1, ~(flags >> 5) & 2);
                alphaArg = (flags >> 4) & 2;
            }
        }
        else if (!(flags & (kHW2DTextured | kHW2DAlphaTest | kHW2DAlphaTexture)))
        {
            pDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
            pDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
            SetTexture(NULL);
            pDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
            alphaArg = D3DTA_DIFFUSE;
        }
        else
        {
            if (flags & kHW2DAlphaTexture)
            {
                pDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
                pDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
            }
            else
            {
                pDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
                pDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            }
            if (flags & kHW2DAlphaTest)
            {
                pDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
                alphaArg = D3DTA_TEXTURE;
            }
            else
            {
                pDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
                alphaArg = D3DTA_DIFFUSE;
            }
        }
        pDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, alphaArg);
    }

    if (changed & kHW2DMultiTex)
    {
        if (!(flags & kHW2DMultiTex))
        {
            pDev->SetTexture(1, NULL);
            pDev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
            pDev->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
            pDev->SetFVF(HW2D_FVF);
        }
        else
        {
            pDev->SetTextureStageState(1, D3DTSS_COLOROP, ((flags >> 10) & 3) + D3DTOP_MODULATE);
            pDev->SetTextureStageState(1, D3DTSS_ALPHAOP, (flags & kHW2DMTModulate2) ? D3DTOP_SELECTARG2 : D3DTOP_MODULATE);
            pDev->SetFVF(HW2D_FVF2);
        }
    }
    else if (changed & (kHW2DMTOpMask | kHW2DMTModulate2))
    {
        pDev->SetTextureStageState(1, D3DTSS_COLOROP, ((flags >> 10) & 3) + D3DTOP_MODULATE);
        pDev->SetTextureStageState(1, D3DTSS_ALPHAOP, (flags & kHW2DMTModulate2) ? D3DTOP_SELECTARG2 : D3DTOP_MODULATE);
    }

    if (changed & kHW2DFiltered)
    {
        DWORD filter = ((flags & kHW2DFiltered) && g_b2DTexFilter) ? D3DTEXF_LINEAR : D3DTEXF_POINT;
        pDev->SetSamplerState(0, D3DSAMP_MAGFILTER, filter);
        pDev->SetSamplerState(0, D3DSAMP_MINFILTER, filter);
    }
    if (changed & kHW2DWrap)
    {
        DWORD address = (flags & kHW2DWrap) ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
        pDev->SetSamplerState(0, D3DSAMP_ADDRESSU, address);
        pDev->SetSamplerState(0, D3DSAMP_ADDRESSV, address);
    }
    if ((changed & kHW2DPolygon) && !(changed & (kHW2DBlendMask | kHW2DAlphaTest)))
    {
        DWORD ref = ((flags & kHW2DPolygon) && (flags & kHW2DAlphaTest) && flags != kHW2DBlendAdd) ? 0x7f : 0;
        pDev->SetRenderState(D3DRS_ALPHAREF, ref);
    }
}

BOOL cD3DHW2DCache::Begin(uint flags)
{
    IDirect3DDevice9 * pDev = m_pModeOps->m_pDevice;

    if (!m_bActive)
    {
        IDirect3DSurface9 * pTarget = m_pModeOps->m_pMSAASurface;
        if (pTarget)
            pTarget->AddRef();
        else if (FAILED(m_pModeOps->m_pSecondaryTexture->GetSurfaceLevel(0, &pTarget)))
            return FALSE;

        pDev->GetRenderTarget(0, &m_pSavedTarget);
        HRESULT hr = pDev->SetRenderTarget(0, pTarget);
        SafeRelease(pTarget);
        if (FAILED(hr))
        {
            SafeRelease(m_pSavedTarget);
            return FALSE;
        }
        pDev->GetDepthStencilSurface(&m_pSavedDepth);
        pDev->SetDepthStencilSurface(m_pDepthStencil);

        m_nFrame++;
        m_bActive = TRUE;
        m_State = 0;
        m_pCurEntry = NULL;
        m_CurBits = 0;
        m_CurW = 0;
        m_CurH = 0;
        m_nPrimType = 0;

        m_pModeOps->m_pSavedStateBlock->Capture();
        m_pModeOps->m_pBlitStateBlock->Apply();
        pDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        pDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        pDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        pDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        pDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
        pDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
        pDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        if ((m_pModeOps->m_ModeInfo.flags & kGrModeIsNative) && m_bFirst && m_pModeOps->m_n2DSurfMode != kD3D2DRenderTargetMSAA)
            pDev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
        pDev->SetRenderState(D3DRS_COLORWRITEENABLE, m_bColorWrite ? 0xffffffff : 7);

        m_pCanvasBits = m_pModeOps->m_pDisplayDevice->m_pModeInfo ? m_pModeOps->m_pDisplayDevice->m_pModeInfo->vbase : g_HW2DDummyBits;
        m_CanvasRowBytes = grd_canvas->bm.row;
        m_Viewport.X = 0;
        m_Viewport.Y = 0;
        m_Viewport.Width = m_pModeOps->m_SecondaryDesc.Width;
        m_Viewport.Height = m_pModeOps->m_SecondaryDesc.Height;
        pDev->SetViewport(&m_Viewport);

        uint zMode = m_ZMode;
        m_ZMode = !zMode;
        SetZMode(zMode);
        double zBias = m_ZBias;
        m_ZBias = zBias + 1.0;
        SetZBias(zBias);

        if (g_pDev2dDisplayDevice)
            g_pDev2dDisplayDevice->SetMonitor((tGrDispBits **) &m_pCanvasBits, (ushort *) &m_CanvasRowBytes, kGrDispPtrMonitorOn);
        if (!(m_pModeOps->m_fState & kD3D2DDirty))
            pDev->BeginScene();
    }
    SetViewport();
    SetState(flags);
    return TRUE;
}

STDMETHODIMP_(void) cD3DHW2DCache::EndCanvas(grs_canvas *)
{
    Flush();
}

BOOL cD3DHW2DCache::Flush()
{
    if (!m_bActive)
        return FALSE;

    FlushQuads();
    m_bActive = FALSE;
    if (g_pDev2dDisplayDevice)
        g_pDev2dDisplayDevice->SetMonitor((tGrDispBits **) &m_pCanvasBits, NULL, kGrDispPtrMonitorOff);
    m_pModeOps->m_pSavedStateBlock->Apply();
    if (!(m_pModeOps->m_fState & kD3D2DDirty))
        m_pModeOps->m_pDevice->EndScene();
    m_pModeOps->m_pDevice->SetRenderTarget(0, m_pSavedTarget);
    m_pModeOps->m_pDevice->SetDepthStencilSurface(m_pSavedDepth);
    SafeRelease(m_pSavedTarget);
    SafeRelease(m_pSavedDepth);
    return TRUE;
}

STDMETHODIMP_(void) cD3DHW2DCache::BeginCanvas(grs_canvas *)
{
    Begin(m_State);
}

BOOL cD3DHW2DCache::CreateDepthStencil()
{
    D3DFORMAT fmt = m_pModeOps->FindDepthStencilFormat(m_pModeOps->m_SecondaryDesc.Format);
    cD3DModeOps * pOps = m_pModeOps;

    pOps->m_pDevice->CreateDepthStencilSurface(pOps->m_nTexWidth, pOps->m_nTexHeight, fmt,
                                               pOps->m_SecondaryDesc.MultiSampleType,
                                               pOps->m_SecondaryDesc.MultiSampleQuality, FALSE, &m_pDepthStencil, NULL);
    if (m_pDepthStencil)
    {
        m_bStencil = (fmt == D3DFMT_D24S8 || fmt == D3DFMT_D24X4S4 || fmt == D3DFMT_D15S1);
        m_pModeOps->m_pDevice->SetDepthStencilSurface(m_pDepthStencil);
        float zBias = m_ZBias;
        m_ZBias += 1.0;
        SetZBias(zBias);
        return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////

void cD3DHW2DCache::SetQuad(int x0, int y0, int x1, int y1, D3DCOLOR color)
{
    float left = (float)(int)(x0 + m_Viewport.X) - m_pModeOps->m_fTexelOffsetX;
    float top = (float)(int)(y0 + m_Viewport.Y) - m_pModeOps->m_fTexelOffsetY;
    float right = (float)(int)(x1 + m_Viewport.X) - m_pModeOps->m_fTexelOffsetX;
    float bottom = (float)(int)(y1 + m_Viewport.Y) - m_pModeOps->m_fTexelOffsetY;

    for (int i = 0; i < 4; i++)
        m_Quad[i].color = color;

    m_Quad[0].x = left;
    m_Quad[0].y = top;
    m_Quad[1].x = right;
    m_Quad[1].y = top;
    m_Quad[2].x = right;
    m_Quad[2].y = bottom;
    m_Quad[3].x = left;
    m_Quad[3].y = bottom;
}

void cD3DHW2DCache::SetLine(int x0, int y0, int x1, int y1, D3DCOLOR color)
{
    float dx = (float) m_Viewport.X - m_pModeOps->m_fTexelOffsetX + 0.5f;
    float dy = (float) m_Viewport.Y - m_pModeOps->m_fTexelOffsetY + 0.5f;
    float lx = (float)(x1 - x0);
    float ly = (float)(y1 - y0);
    float len = (float) sqrt(lx * lx + ly * ly);
    float nx = (len > 0.0f) ? -ly * 0.5f / len : 0.5f;
    float ny = (len > 0.0f) ? lx * 0.5f / len : 0.0f;

    m_Quad[0].x = (float) x0 + dx + nx;
    m_Quad[0].y = (float) y0 + dy + ny;
    m_Quad[1].x = (float) x1 + dx + nx;
    m_Quad[1].y = (float) y1 + dy + ny;
    m_Quad[2].x = (float) x1 + dx - nx;
    m_Quad[2].y = (float) y1 + dy - ny;
    m_Quad[3].x = (float) x0 + dx - nx;
    m_Quad[3].y = (float) y0 + dy - ny;

    for (int i = 0; i < 4; i++)
        m_Quad[i].color = color;
}

STDMETHODIMP_(void) cD3DHW2DCache::ColorFill(int x0, int y0, int x1, int y1, D3DCOLOR color)
{
    if (!Begin(kHW2DBlendAlpha + 2))
        return;
    FlushQuads();

    RECT rect;
    rect.left = m_Viewport.X + x0;
    rect.top = m_Viewport.Y + y0;
    rect.right = m_Viewport.X + x1;
    rect.bottom = m_Viewport.Y + y1;
    if (m_pModeOps->m_pMSAASurface)
    {
        m_pModeOps->m_pDevice->ColorFill(m_pModeOps->m_pMSAASurface, &rect, color);
        return;
    }
    IDirect3DSurface9 * pSurf;
    if (SUCCEEDED(m_pModeOps->m_pSecondaryTexture->GetSurfaceLevel(0, &pSurf)))
    {
        m_pModeOps->m_pDevice->ColorFill(pSurf, &rect, color);
        SafeRelease(pSurf);
    }
}

STDMETHODIMP_(void) cD3DHW2DCache::ClearDepth(int x0, int y0, int x1, int y1)
{
    if (!m_pDepthStencil)
        return;
    FlushQuads();

    D3DRECT rect;
    if (x1 > x0 && y1 > y0)
    {
        rect.x1 = x0;
        rect.y1 = y0;
        rect.x2 = x1;
        rect.y2 = y1;
    }
    else
    {
        int x, y;
        GetCanvasOrigin(&x, &y);
        rect.x1 = x;
        rect.y1 = y;
        rect.x2 = grd_canvas->bm.w + x;
        rect.y2 = grd_canvas->bm.h + y;
    }
    m_pModeOps->m_pDevice->Clear(1, &rect, (m_bStencil ? D3DCLEAR_STENCIL : 0) | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
}

STDMETHODIMP_(void) cD3DHW2DCache::FillRect(int x0, int y0, int x1, int y1, D3DCOLOR color, uint flags)
{
    if (!Begin(flags))
        return;
    FlushQuads();
    SetQuad(x0, y0, x1, y1, color);
    m_pModeOps->m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, m_Quad, sizeof(sHW2DVertex));
}

STDMETHODIMP_(void) cD3DHW2DCache::DrawLine(int x0, int y0, int x1, int y1, D3DCOLOR color, uint flags)
{
    if (!Begin(flags))
        return;
    if (x0 != x1 && y0 != y1)
        SetLine(x0, y0, x1, y1, color);
    else
        SetQuad(x0, y0, x1 + 1, y1 + 1, color);
    AddLineQuad();
}

STDMETHODIMP_(void) cD3DHW2DCache::DrawPixel(int x, int y, D3DCOLOR color, uint flags)
{
    if (!Begin(flags))
        return;
    FlushQuads();
    SetQuad(x, y, x + 1, y + 1, color);
    m_pModeOps->m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, m_Quad, sizeof(sHW2DVertex));
}

STDMETHODIMP_(void) cD3DHW2DCache::Blit(grs_bitmap * pBm, int x, int y, uint flags)
{
    if (pBm)
        ScaleBlit(pBm, x, y, pBm->w, pBm->h, flags);
}

STDMETHODIMP_(void) cD3DHW2DCache::ScaleBlit(grs_bitmap * pBm, int x, int y, int w, int h, uint flags)
{
    sHW2DEntry * pEntry = Cache(pBm);
    if (!pEntry)
        return;

    if (!(pBm->flags & BMF_TRANS) && (pBm->flags & 0xf00) != 0x600 && !(pEntry->flags & kHW2DEntryFont))
        flags |= kHW2DTextured;
    else
        flags |= (g_HW2DBilinear ? kHW2DBlendAdd : 0) | kHW2DAlphaTest;
    if ((pEntry->flags & kHW2DEntryAlpha) || pBm->type == BMT_MONO)
        flags |= kHW2DAlphaTexture;
    if (pBm->w != w || pBm->h != h)
        flags |= kHW2DFiltered;

    if (!Begin(flags))
        return;
    SetTextureCoords(pEntry, pBm);
    SetQuad(x, y, x + w, y + h, 0xFFFFFFFF);
    AddQuad();
}

static int EllipseSegments(int rx, int ry)
{
    int n = (int) sqrt((double)(rx + ry));
    if (n < 8)
        return 8;
    if (n > 64)
        return 64;
    return (n + 1) & ~1;
}

STDMETHODIMP_(void) cD3DHW2DCache::DrawEllipse(int x, int y, int rx, int ry, D3DCOLOR color)
{
    sHW2DVertex verts[65];
    int n;
    int i;

    if (!Begin(m_State))
        return;
    FlushQuads();

    n = EllipseSegments(rx, ry);
    double cx = (double)(m_Viewport.X + x);
    double cy = (double)(m_Viewport.Y + y);
    sHW2DVertex * pV = verts;
    for (i = 0; i <= n / 2; i++, pV++)
    {
        double a = (i * 3.14159265358979) / (n / 2);
        pV->x = (float)(floor(sin(a) * rx + 0.5) + cx);
        pV->y = (float)(cos(a) * ry + cy);
        pV->z = 1.0f;
        pV->rhw = 1.0f;
        pV->color = color;
    }
    for (; i < n; i++, pV++)
    {
        pV->x = (float)(cx + cx - pV[2 * (n / 2) - 2 * i].x);
        pV->y = pV[2 * (n / 2) - 2 * i].y;
        pV->z = 1.0f;
        pV->rhw = 1.0f;
        pV->color = color;
    }
    *pV = verts[0];
    m_pModeOps->m_pDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, n, verts, sizeof(sHW2DVertex));
}

STDMETHODIMP_(void) cD3DHW2DCache::FillEllipse(int x, int y, int rx, int ry, D3DCOLOR color)
{
    sHW2DVertex verts[65];
    int n;
    int i;

    if (!Begin(m_State))
        return;
    FlushQuads();

    n = EllipseSegments(rx, ry);
    double cx = (double)(m_Viewport.X + x) - m_pModeOps->m_fTexelOffsetX;
    double cy = (double)(m_Viewport.Y + y) - m_pModeOps->m_fTexelOffsetY;
    sHW2DVertex * pV = verts;
    for (i = 0; i <= n / 2; i++, pV++)
    {
        double a = (i * 3.14159265358979) / (n / 2);
        pV->x = (float)(floor(sin(a) * rx + 0.5) + cx);
        pV->y = (float)(cos(a) * ry + cy);
        pV->z = 1.0f;
        pV->rhw = 1.0f;
        pV->color = color;
    }
    for (; i < n; i++, pV++)
    {
        pV->x = (float)(cx + cx - pV[2 * (n / 2) - 2 * i].x);
        pV->y = pV[2 * (n / 2) - 2 * i].y;
        pV->z = 1.0f;
        pV->rhw = 1.0f;
        pV->color = color;
    }
    m_pModeOps->m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, n - 2, verts, sizeof(sHW2DVertex));
}

STDMETHODIMP_(void) cD3DHW2DCache::DrawPoly(grs_bitmap * pBm, sHW2DVertex * pVerts, int n, uint flags)
{
    if (n < 1)
        return;
    if (n > HW2D_MAX_POLY_VERTS)
        n = HW2D_MAX_POLY_VERTS;

    if (!pBm)
    {
        if (!Begin((flags & kHW2DBlendMask) | kHW2DPolygon))
            return;
    }
    else
    {
        sHW2DEntry * pEntry = Cache(pBm);
        if (!pEntry)
            return;
        BOOL bTrans = (pBm->flags & BMF_TRANS) || (pBm->flags & 0xf00) == 0x600;
        flags |= (bTrans ? kHW2DAlphaTest : kHW2DTextured) | g_HW2DBilinear | kHW2DDiffuse | kHW2DPolygon;
        if (pBm->type == BMT_MONO)
            flags |= kHW2DAlphaTexture;
        if (!Begin(flags))
            return;
        SetTexture(pEntry);
    }

    if (n > 2)
    {
        AddFan(pVerts, n);
        return;
    }

    FlushQuads();
    float dx = (float) m_Viewport.X - m_pModeOps->m_fTexelOffsetX;
    float dy = (float) m_Viewport.Y - m_pModeOps->m_fTexelOffsetY;
    sHW2DVertex * pOut = (sHW2DVertex *) g_HW2DPolyVerts;
    for (int i = 0; i < n; i++)
    {
        pOut[i] = pVerts[i];
        pOut[i].x += dx;
        pOut[i].y += dy;
    }
    if (n == 2)
        m_pModeOps->m_pDevice->DrawPrimitiveUP(D3DPT_LINELIST, 1, pOut, sizeof(sHW2DVertex));
    else
        m_pModeOps->m_pDevice->DrawPrimitiveUP(D3DPT_POINTLIST, 1, pOut, sizeof(sHW2DVertex));
}

STDMETHODIMP_(void) cD3DHW2DCache::DrawPolyMT(grs_bitmap * pBm, grs_bitmap * pBm2, sHW2DVertex2 * pVerts, int n,
                                              uint mtOp, uint flags)
{
    if (n < 1)
        return;
    if (n > HW2D_MAX_POLY_VERTS)
        n = HW2D_MAX_POLY_VERTS;

    sHW2DEntry * pEntry = CacheExact(pBm);
    if (!pEntry)
        return;
    sHW2DEntry * pEntry2 = CacheExact(pBm2);

    BOOL bTrans = (pBm->flags & BMF_TRANS) || (pBm->flags & 0xf00) == 0x600;
    flags |= (bTrans ? kHW2DAlphaTest : kHW2DTextured) | ((mtOp & 7) << 10) | g_HW2DBilinear | kHW2DMultiTex | kHW2DDiffuse | kHW2DPolygon;
    if (pBm->type == BMT_MONO)
        flags |= kHW2DAlphaTexture;
    if (!Begin(flags))
        return;

    FlushQuads();
    SetTexture(pEntry);
    m_pModeOps->m_pDevice->SetTexture(1, pEntry2 ? pEntry2->pTexture[pEntry2->flags & kHW2DEntryFlip] : NULL);

    float dx = (float) m_Viewport.X - m_pModeOps->m_fTexelOffsetX;
    float dy = (float) m_Viewport.Y - m_pModeOps->m_fTexelOffsetY;
    for (int i = 0; i < n; i++)
    {
        g_HW2DPolyVerts[i] = pVerts[i];
        g_HW2DPolyVerts[i].x += dx;
        g_HW2DPolyVerts[i].y += dy;
    }
    if (n > 2)
        m_pModeOps->m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, n - 2, g_HW2DPolyVerts, sizeof(sHW2DVertex2));
    else if (n == 2)
        m_pModeOps->m_pDevice->DrawPrimitiveUP(D3DPT_LINELIST, 1, g_HW2DPolyVerts, sizeof(sHW2DVertex2));
    else
        m_pModeOps->m_pDevice->DrawPrimitiveUP(D3DPT_POINTLIST, 1, g_HW2DPolyVerts, sizeof(sHW2DVertex2));
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cD3DHW2DCache::SetBilinear(BOOL bOn)
{
    BOOL bWas = g_HW2DBilinear != 0;
    g_HW2DBilinear = bOn ? kHW2DBilinear : 0;
    return bWas;
}

STDMETHODIMP_(uint) cD3DHW2DCache::SetZMode(uint mode)
{
    uint old = m_ZMode;
    D3DMATRIX mat;

    if (old == mode)
        return mode;
    m_ZMode = mode;
    if (!m_bActive)
        return old;

    FlushQuads();
    IDirect3DDevice9 * pDev = m_pModeOps->m_pDevice;
    if (mode)
    {
        if (!m_pDepthStencil && !CreateDepthStencil())
            return 0;
        pDev->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
        pDev->SetRenderState(D3DRS_ZWRITEENABLE, mode & 1);
        pDev->SetRenderState(D3DRS_ZFUNC, (mode & 2) ? D3DCMP_LESSEQUAL : D3DCMP_ALWAYS);
        if (!old)
        {
            memset(&mat, 0, sizeof(mat));
            mat._11 = 1.0f;
            pDev->SetTransform(D3DTS_PROJECTION, &mat);
            return 0;
        }
    }
    else if (m_pDepthStencil)
    {
        pDev->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
        pDev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        pDev->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
        if (old)
        {
            memset(&mat, 0, sizeof(mat));
            mat._11 = 1.0f;
            pDev->SetTransform(D3DTS_PROJECTION, &mat);
        }
    }
    return old;
}

STDMETHODIMP_(double) cD3DHW2DCache::SetZBias(double bias)
{
    double old = bias;
    if (bias != m_ZBias)
    {
        old = m_ZBias;
        m_ZBias = bias;
        if (m_bActive && m_pDepthStencil)
        {
            FlushQuads();
            float fBias = (float) bias;
            m_pModeOps->m_pDevice->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD *) &fBias);
        }
    }
    return old;
}

STDMETHODIMP_(BOOL) cD3DHW2DCache::SetColorWrite(BOOL bOn)
{
    BOOL old = m_bColorWrite;
    if (old == (bOn != 0))
        return bOn;
    m_bColorWrite = (bOn != 0);
    if (m_bActive)
    {
        FlushQuads();
        m_pModeOps->m_pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, bOn ? 0xffffffff : 7);
    }
    return old;
}

///////////////////////////////////////

STDMETHODIMP_(void) cD3DHW2DCache::CopyFromScreen(grs_bitmap * pBm, int x, int y)
{
    FlushQuads();

    sHW2DEntry * pEntry = NULL;
    if (pBm)
    {
        for (pEntry = m_pEntries; pEntry; pEntry = pEntry->pNext)
        {
            if (pEntry->pBitmap != pBm)
                continue;
            Touch(pEntry);
            if (pEntry->pTexture[0] && pEntry->pBitmap->w == pEntry->w && pEntry->pBitmap->h == pEntry->h &&
                (pEntry->flags & kHW2DEntryUploaded))
            {
                pEntry->flags ^= kHW2DEntryFlip;
                if (pEntry->pBits != pBm->bits)
                {
                    uchar * pOld = pEntry->pBits;
                    pEntry->pBits = pBm->bits;
                    pEntry->pEnd = pBm->bits + (pEntry->pEnd - pOld);
                }
                break;
            }
            FreeEntry(pEntry);
            pEntry = NULL;
            break;
        }
    }
    if (!pEntry)
    {
        pEntry = NewEntry(pBm, 0);
        pEntry->nBytes = ((m_pModeOps->GetSignificantDepth() + 7) >> 3) * pBm->h * pBm->w;
        Link(pEntry);
    }

    int iTex = pEntry->flags & kHW2DEntryFlip;
    if (!pEntry->pTexture[iTex])
    {
        m_pModeOps->m_pDevice->CreateTexture(pEntry->w, pEntry->h, 1, D3DUSAGE_RENDERTARGET,
                                             m_pModeOps->m_SecondaryDesc.Format, D3DPOOL_DEFAULT,
                                             &pEntry->pTexture[iTex], NULL);
        if (iTex)
        {
            if (!(pEntry->flags & kHW2DEntryLocked))
                m_nBytes += pEntry->nBytes;
            pEntry->nBytes <<= 1;
        }
    }
    if (!pEntry->pTexture[iTex])
        return;

    if (!(pEntry->flags & kHW2DEntryLocked))
    {
        pEntry->flags |= kHW2DEntryLocked;
        m_nFree--;
        m_nBytes -= pEntry->nBytes;
    }
    pEntry->flags &= ~kHW2DEntryStale;
    pBm->exflags |= 4;

    if (m_bActive)
        m_pModeOps->m_pDevice->EndScene();

    IDirect3DSurface9 * pSrc = m_pModeOps->m_pMSAASurface;
    if (pSrc)
        pSrc->AddRef();
    else
        m_pModeOps->m_pSecondaryTexture->GetSurfaceLevel(0, &pSrc);
    if (pSrc)
    {
        if (m_bActive && pEntry == m_pCurEntry)
            SetTexture(NULL);
        IDirect3DSurface9 * pDest;
        if (SUCCEEDED(pEntry->pTexture[iTex]->GetSurfaceLevel(0, &pDest)))
        {
            int ox, oy;
            GetCanvasOrigin(&ox, &oy);
            RECT src, dst;
            dst.left = 0;
            dst.top = 0;
            dst.right = pBm->w;
            dst.bottom = pBm->h;
            src.left = x + ox;
            src.top = y + oy;
            src.right = src.left + pBm->w;
            src.bottom = src.top + pBm->h;
            int maxW = m_pModeOps->m_SecondaryDesc.Width;
            int maxH = m_pModeOps->m_SecondaryDesc.Height;
            if (src.left < 0)
            {
                dst.left = -src.left;
                src.left = 0;
            }
            if (src.top < 0)
            {
                dst.top = -src.top;
                src.top = 0;
            }
            if (src.right > maxW)
            {
                dst.right += maxW - src.right;
                src.right = maxW;
            }
            if (src.bottom > maxH)
            {
                dst.bottom += maxH - src.bottom;
                src.bottom = maxH;
            }
            if (src.top < src.bottom && src.left < src.right)
                m_pModeOps->m_pDevice->StretchRect(pSrc, &src, pDest, &dst, D3DTEXF_NONE);
            SafeRelease(pDest);
        }
        SafeRelease(pSrc);
    }
    if (m_bActive)
        m_pModeOps->m_pDevice->BeginScene();
}

STDMETHODIMP_(void) cD3DHW2DCache::MakeTarget(grs_bitmap * pBm)
{
    FlushQuads();

    sHW2DEntry * pEntry = NULL;
    if (pBm)
    {
        for (pEntry = m_pEntries; pEntry; pEntry = pEntry->pNext)
        {
            if (pEntry->pBitmap != pBm)
                continue;
            Touch(pEntry);
            if ((pEntry->flags & kHW2DEntryTarget) && pEntry->w == pBm->w && pEntry->h == pBm->h)
            {
                pEntry->flags ^= kHW2DEntryFlip;
                if ((pEntry->flags & kHW2DEntryFlip) && !pEntry->pTexture[1])
                {
                    if (!(pEntry->flags & kHW2DEntryLocked))
                        m_nBytes += pEntry->nBytes;
                    pEntry->nBytes <<= 1;
                }
                goto have_entry;
            }
            FreeEntry(pEntry);
            break;
        }
    }
    pEntry = NewEntry(pBm, 0);
    Link(pEntry);

have_entry:
    pBm->exflags |= 4;
    pEntry->flags &= ~(kHW2DEntryDirty | kHW2DEntryStale);
    D8_CreateBitmapTexture(m_pModeOps->m_pDevice, &pEntry->pTexture[pEntry->flags & kHW2DEntryFlip], 2, pBm);
}

STDMETHODIMP_(BOOL) cD3DHW2DCache::ReadBack(grs_bitmap * pSrc, grs_bitmap * pDest)
{
    BOOL bResult = FALSE;

    if (!pSrc || !pDest || pDest->type != BMT_FLAT32)
        return FALSE;
    sHW2DEntry * pEntry = Cache(pSrc);
    if (!pEntry || !Begin(m_State))
        return FALSE;
    FlushQuads();

    IDirect3DDevice9 * pDev = m_pModeOps->m_pDevice;
    IDirect3DSurface9 * pTarget = m_pReadSurface;
    if (pTarget && (m_ReadW != pDest->w || m_ReadH != pDest->h))
    {
        SafeRelease(m_pReadSurface);
        pTarget = NULL;
    }
    if (!pTarget)
    {
        m_ReadW = pDest->w;
        m_ReadH = pDest->h;
        if (FAILED(pDev->CreateRenderTarget(m_ReadW, m_ReadH, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE,
                                            &m_pReadSurface, NULL)))
            return FALSE;
    }

    IDirect3DSurface9 * pOldTarget = NULL;
    pDev->GetRenderTarget(0, &pOldTarget);
    pDev->EndScene();
    if (SUCCEEDED(pDev->SetRenderTarget(0, m_pReadSurface)))
    {
        pDev->BeginScene();
        D3DVIEWPORT9 vp = { 0, 0, (DWORD) pDest->w, (DWORD) pDest->h, 0.0f, 1.0f };
        pDev->SetViewport(&vp);
        SetTextureCoords(pEntry, pSrc);
        SetQuad(0, 0, pDest->w, pDest->h, 0xFFFFFFFF);
        pDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, m_Quad, sizeof(sHW2DVertex));
        pDev->EndScene();
        pDev->SetRenderTarget(0, pOldTarget);
        pDev->BeginScene();
        pDev->SetViewport(&m_Viewport);

        D3DLOCKED_RECT lr;
        if (SUCCEEDED(m_pReadSurface->LockRect(&lr, NULL, D3DLOCK_READONLY)))
        {
            uchar * pDst = pDest->bits;
            uchar * pRow = (uchar *) lr.pBits;
            for (int y = 0; y < pDest->h; y++)
            {
                memcpy(pDst, pRow, pDest->w * 4);
                pDst += pDest->row;
                pRow += lr.Pitch;
            }
            m_pReadSurface->UnlockRect();
            bResult = TRUE;
        }
    }
    SafeRelease(pOldTarget);
    return bResult;
}

///////////////////////////////////////

cD3DLost2DCache::cD3DLost2DCache()
{
    if (!g_pHW2D)
        g_pHW2D = this;
}

cD3DLost2DCache::~cD3DLost2DCache()
{
    if (g_pHW2D == this)
        g_pHW2D = NULL;
}

STDMETHODIMP cD3DLost2DCache::QueryInterface(REFIID id, void ** ppI)
{
    if (id == IID_IHW2DRenderer || id == IID_IUnknown)
    {
        *ppI = this;
        return S_OK;
    }
    return E_FAIL;
}

STDMETHODIMP_(ULONG) cD3DLost2DCache::AddRef()
{
    return 1;
}

STDMETHODIMP_(ULONG) cD3DLost2DCache::Release()
{
    return 1;
}
