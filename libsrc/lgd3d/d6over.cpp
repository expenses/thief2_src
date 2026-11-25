
#include <windows.h>
#include <dbg.h>
#include <d3d.h>
#include <lgd3d.h>
#include <d6states.h>
#include <d6over.h>
#include <d6hrmsg.h>
#include <lgassert.h>
#include <lglog.h>
#include <lgassert.h>

extern "C" int lgd3d_is_zwrite_on(void);
extern "C" int lgd3d_is_zcompare_on(void);

struct sBaseOverData
{
    float fX0, fY0, fX1, fY1;
    float fU0, fV0, fU1, fV1;
    DWORD dwColor;
    int bReady;
};

class cD6AlphaOverlay : public cD6OvelayType
{
public:
    int m_bAlphaBlend;
    int m_bTextureLoaded;
    int m_nTexID;
    grs_bitmap* m_psBitmap;
    sBaseOverData m_sOverData;
    _D3DTLVERTEX m_saVertices[4];

    cD6AlphaOverlay():
        m_bAlphaBlend(0), m_bTextureLoaded(0),
        m_nTexID(0xffffffff), m_psBitmap(NULL)
    {
    }
    virtual ~cD6AlphaOverlay();
    virtual void DrawOverlay();
    void SetOverlayBaseData(sLGD3DOverlayInfo* psOverInfo);
    void UpdateOverlayBaseData(sLGD3DOverlayInfo* psOverInfo);
    int  PrepareOverlay();
    int  LockAndGetBitmap(grs_bitmap** ppsBitmap);
    int  UnLockAndReload();
    void SetPositionF(float fX, float fY);
    void SetColor(int nRed, int nGreen, int nBlue, int nAlpha);
    void SetAlpha(int nAlpha);
    void Move(float fDeltaX, float fDeltaY)
    {
        m_sOverData.fX0 += fDeltaX;
        m_sOverData.fX1 += fDeltaX;
        m_sOverData.fY0 += fDeltaY;
        m_sOverData.fY1 += fDeltaY;
    }

private:
    void SetBitmap(grs_bitmap* psBitmap);
};

cD6OverlayHandler* pcOverlayHandler;
unsigned long cD6OverlayHandler::s_dwIdGenerator = 0;

inline HLGD3DOVERLAY cD6OverlayHandler::MakeNewOverlayHandle(cD6OvelayType* pcOver)
{
    s_dwIdGenerator++;
    pcOver->m_dwID = s_dwIdGenerator;
    return pcOver->m_dwID;
}

extern "C" {
HRESULT lgd3d_aol_add(sLGD3DOverlayInfo* psOverInfo, HLGD3DOVERLAY* phOver)
{
#line 71
    AssertMsg(psOverInfo, "psOverInfo == NULL");
    AssertMsg(phOver, "phOver == NULL");
    if (!pcOverlayHandler)
        pcOverlayHandler = new cD6OverlayHandler;
    cD6AlphaOverlay* pOver = new cD6AlphaOverlay;
    if (!pOver)
        return E_LGD3DOEC_ALLOCATION;
    pcOverlayHandler->AddOverlay(pOver, phOver);
    pOver->SetOverlayBaseData(psOverInfo);
    return pOver->PrepareOverlay();
}
HRESULT lgd3d_aol_insert(sLGD3DOverlayInfo* psOverInfo, HLGD3DOVERLAY hAfterMe, HLGD3DOVERLAY* phOver)
{
    int _pad;
#line 98
    AssertMsg(pcOverlayHandler, "pcOverlayHandler == NULL");
#line 71
    AssertMsg(psOverInfo, "psOverInfo == NULL");
    AssertMsg(phOver, "phOver == NULL");
#line 101
    AssertMsg(hAfterMe, "hAfterMe == 0");
    cD6AlphaOverlay* pOver = NULL;
    cD6OvelayType* pAfter = NULL;
    if (pcOverlayHandler->OverlayFromHandle(hAfterMe, &pAfter))
    {
        pOver = new cD6AlphaOverlay;
        if (!pOver)
            return E_LGD3DOEC_ALLOCATION;
        pcOverlayHandler->InsertOverlayAfter(pOver, pAfter, phOver);
        pOver->SetOverlayBaseData(psOverInfo);
        return pOver->PrepareOverlay();
    }
    *phOver = 0;
    return E_LGD3DOEC_BAD_HANDLE;
}
HRESULT lgd3d_aol_remove(HLGD3DOVERLAY hOver)
{

#line 131
    AssertMsg(pcOverlayHandler, "pcOverlayHandler == NULL");
    AssertMsg(hOver, "hOver == 0");
    cD6OvelayType* pOver = NULL;
    if (pcOverlayHandler->OverlayFromHandle(hOver, &pOver) != 0)
    {
        pcOverlayHandler->RemoveOverlay(pOver);
        return 0;
    }
    else
    {
        return E_LGD3DOEC_BAD_HANDLE;
    }
}
void lgd3d_aol_remove_all()
{
    if (pcOverlayHandler)
        pcOverlayHandler->RemoveAllOverlays();
    else
        Warning(("lgd3d_aol_remove_all() ovrlays already freed!"));
}
HRESULT lgd3d_aol_switch(HLGD3DOVERLAY hOver, int bOn)
{
#line 163
    AssertMsg(pcOverlayHandler, "pcOverlayHandler == NULL");
    AssertMsg(hOver, "hOver == 0");
    cD6OvelayType* pOver = NULL;
    if (pcOverlayHandler->OverlayFromHandle(hOver, &pOver) != 0)
    {
        pOver->TurnOnOff(bOn);
        return 0;
    }
    else
    {
        return E_LGD3DOEC_BAD_HANDLE;
    }
}
HRESULT lgd3d_aol_update(HLGD3DOVERLAY hOver, sLGD3DOverlayInfo* psOverInfo)
{





    AssertMsg(pcOverlayHandler, "pcOverlayHandler == NULL");
    AssertMsg(psOverInfo, "psOverInfo == NULL");
    AssertMsg(hOver, "hOver == 0");
    cD6AlphaOverlay* pOver = NULL;
    if (pcOverlayHandler->AlphaOverlayFromHandle(hOver, &pOver) != 0)
    {
        pOver->UpdateOverlayBaseData(psOverInfo);
        return pOver->PrepareOverlay();
    }
    else
    {
        return E_LGD3DOEC_BAD_HANDLE;
    }
}
HRESULT lgd3d_aol_lock_bitmap_data(HLGD3DOVERLAY hOver, grs_bitmap** ppsBitmap)
{




    AssertMsg(pcOverlayHandler, "pcOverlayHandler == NULL");
    AssertMsg(hOver, "hOver == 0");
    cD6AlphaOverlay* pOver = NULL;
    if (pcOverlayHandler->AlphaOverlayFromHandle(hOver, &pOver) != 0)
    {
        return pOver->LockAndGetBitmap(ppsBitmap);
    }
    else
    {
        return E_LGD3DOEC_BAD_HANDLE;
    }
}
HRESULT lgd3d_aol_unlock_bitmap_data(HLGD3DOVERLAY hOver)
{



    AssertMsg(pcOverlayHandler, "pcOverlayHandler == NULL");
    AssertMsg(hOver, "hOver == 0");
    cD6AlphaOverlay* pOver = NULL;
    if (pcOverlayHandler->AlphaOverlayFromHandle(hOver, &pOver) != 0)
    {
        return pOver->UnLockAndReload();
    }
    else
    {
        return E_LGD3DOEC_BAD_HANDLE;
    }
}
HRESULT lgd3d_aol_set_position(HLGD3DOVERLAY hOver, float fScreenX, float fScreenY)
{



    cD6AlphaOverlay* pOver = NULL;
    if (pcOverlayHandler->AlphaOverlayFromHandle(hOver, &pOver) != 0)
    {
        pOver->SetPositionF(fScreenX, fScreenY);
        return pOver->PrepareOverlay();
    }
    else
    {
        return E_LGD3DOEC_BAD_HANDLE;
    }
}
HRESULT lgd3d_aol_move(HLGD3DOVERLAY hOver, float fDeltaX, float fDeltaY)
{
#line 258
    AssertMsg(pcOverlayHandler, "pcOverlayHandler == NULL");
    AssertMsg(hOver, "hOver == 0");
    cD6AlphaOverlay* pOver = NULL;
    if (pcOverlayHandler->AlphaOverlayFromHandle(hOver, &pOver) != 0)
    {
        pOver->Move(fDeltaX, fDeltaY);
        return pOver->PrepareOverlay();
    }
    else
    {
        return E_LGD3DOEC_BAD_HANDLE;
    }
}
HRESULT lgd3d_aol_set_color(HLGD3DOVERLAY hOver, int nRed, int nGreen, int nBlue, int nAlpha)
{

#line 256
    AssertMsg(pcOverlayHandler, "pcOverlayHandler == NULL");
    AssertMsg(hOver, "hOver == 0");

    AssertMsg(nRed >= 0 && nRed < 0x100, "nRed out of range");
    AssertMsg(nGreen >= 0 && nGreen < 0x100, "nGreen out of range");
    AssertMsg(nBlue >= 0 && nBlue < 0x100, "nBlue out of range");
    AssertMsg(nAlpha >= 0 && nAlpha < 0x100, "nAlpha out of range");
    cD6AlphaOverlay* pOver = NULL;
    if (pcOverlayHandler->AlphaOverlayFromHandle(hOver, &pOver) != 0)
    {
        pOver->SetColor(nRed, nGreen, nBlue, nAlpha);
        return pOver->PrepareOverlay();
    }
    else
    {
        return E_LGD3DOEC_BAD_HANDLE;
    }
}
HRESULT lgd3d_aol_set_alpha(HLGD3DOVERLAY hOver, int nAlpha)
{
    cD6AlphaOverlay* pOver = NULL;
    if (pcOverlayHandler->AlphaOverlayFromHandle(hOver, &pOver) != 0)
    {
        pOver->SetAlpha(nAlpha);
        return pOver->PrepareOverlay();
    }
    else
    {
        return E_LGD3DOEC_BAD_HANDLE;
    }
}
void lgd3d_aol_set_clip_rect(sOverRectangle* psInRect)
{
#line 329
    AssertMsg(pcOverlayHandler, "pcOverlayHandler == NULL");
    pcOverlayHandler->SetClipViewport(psInRect);
}
void lgd3d_aol_get_clip_rect(sOverRectangle* psOutRect)
{




    AssertMsg(pcOverlayHandler, "pcOverlayHandler == NULL");
    AssertMsg(psOutRect, "psOutRect == NULL");
    pcOverlayHandler->GetClipViewport(psOutRect);
}
}

cD6AlphaOverlay::~cD6AlphaOverlay()
{
    if (m_bTextureLoaded) {
        if (g_tmgr) {
            g_tmgr->unload_texture(m_psBitmap);
        }
    }
}

void cD6AlphaOverlay::SetOverlayBaseData(sLGD3DOverlayInfo* psOverInfo)
{
    DWORD dwFlags;
    m_sOverData.fX0 = psOverInfo->fX0;
    m_sOverData.fY0 = psOverInfo->fY0;
    m_psBitmap = psOverInfo->pBitmap;
    dwFlags = psOverInfo->dwFlags;
    if (dwFlags & LGD3DOI_LOWER_RIGHT_CORNER)
    {
        m_sOverData.fX1 = psOverInfo->fX1;
        m_sOverData.fY1 = psOverInfo->fY1;
    }
    else
    {
        int iWidth = 1 << m_psBitmap->wlog;
        float width = (float)iWidth;
        int iHeight = 1 << m_psBitmap->hlog;
        float height = (float)iHeight;
        m_sOverData.fX1 = width + psOverInfo->fX0;
        m_sOverData.fY1 = height + psOverInfo->fY0;
    }
    if (dwFlags & LGD3DOI_TEXTURE_COORDINATES)
    {
        m_sOverData.fU0 = psOverInfo->tu0;
        m_sOverData.fU1 = psOverInfo->tu1;
        m_sOverData.fV0 = psOverInfo->tv0;
        m_sOverData.fV1 = psOverInfo->tv1;
    }
    else
    {
        m_sOverData.fU0 = 0.0f;
        m_sOverData.fV0 = 0.0f;
        m_sOverData.fU1 = 1.0f;
        m_sOverData.fV1 = 1.0f;
    }
    int alpha;
    if (dwFlags & LGD3DOI_ALPHA)
        alpha = psOverInfo->nAlpha;
    else
        alpha = 0xff;
    m_bAlphaBlend = (uint)(alpha < 0xff);
    if (dwFlags & LGD3DOI_RGB)
        m_sOverData.dwColor = (alpha << 24) | (psOverInfo->nRed << 16) | (psOverInfo->nGreen << 8) | psOverInfo->nBlue;
    else
        m_sOverData.dwColor = (alpha << 24) | 0xffffff;
}

void cD6AlphaOverlay::UpdateOverlayBaseData(sLGD3DOverlayInfo* psOverInfo)
{
    DWORD dwFlags = psOverInfo->dwFlags;
    if (dwFlags & LGD3DOI_UPPER_LEFT_CORNER)
    {
        m_sOverData.fX0 = psOverInfo->fX0;
        m_sOverData.fY0 = psOverInfo->fY0;
    }
    if (dwFlags & LGD3DOI_LOWER_RIGHT_CORNER)
    {
        m_sOverData.fX1 = psOverInfo->fX1;
        m_sOverData.fY1 = psOverInfo->fY1;
    }
    if (dwFlags & LGD3DOI_TEXTURE_COORDINATES)
    {
        m_sOverData.fU0 = psOverInfo->tu0;
        m_sOverData.fU1 = psOverInfo->tu1;
        m_sOverData.fV0 = psOverInfo->tv0;
        m_sOverData.fV1 = psOverInfo->tv1;
    }
    if (dwFlags & (LGD3DOI_ALPHA | LGD3DOI_RGB))
    {
        int nAlpha;
        if (dwFlags & LGD3DOI_ALPHA)
            nAlpha = (int)(psOverInfo->nAlpha >> 24);
        else
            nAlpha = (int)(m_sOverData.dwColor & 0xff000000);
        if (dwFlags & LGD3DOI_RGB)
            m_sOverData.dwColor = (psOverInfo->nRed << 16) | (psOverInfo->nGreen << 8) | psOverInfo->nBlue;
        else
            m_sOverData.dwColor = m_sOverData.dwColor & 0x00ffffff;
        m_sOverData.dwColor = m_sOverData.dwColor | nAlpha;
        m_bAlphaBlend = (uint)((int)nAlpha < (int)0xFF000000);
    }
}

int cD6AlphaOverlay::PrepareOverlay()
{
    // Compute screen coordinates with offset
    float fX0 = g_XOffset + m_sOverData.fX0;
    float fY0 = g_YOffset + m_sOverData.fY0;
    float fX1 = g_XOffset + m_sOverData.fX1;
    float fY1 = g_YOffset + m_sOverData.fY1;
    float fU0 = m_sOverData.fU0;
    float fV0 = m_sOverData.fV0;
    float fU1 = m_sOverData.fU1;
    float fV1 = m_sOverData.fV1;

    m_bVisible = 0;

    sOverRectangle clipRect;
    m_pcParent->GetClipViewport(&clipRect);

    // Check if fully clipped
    if (fX1 < clipRect.fX0 || fX0 > clipRect.fX1 ||
        fY1 < clipRect.fY0 || fY0 > clipRect.fY1)
    {
        return S_LGD3DOEC_INVISIBLE;
    }

    // Determine which edges need clipping
    int clipFlags = 0;
    if (fX0 < clipRect.fX0) clipFlags |= 1;
    if (fX1 > clipRect.fX1) clipFlags |= 4;
    if (fY0 < clipRect.fY0) clipFlags |= 2;
    if (fY1 > clipRect.fY1) clipFlags |= 8;

    if (clipFlags != 0)
    {
        float du_dx = (fU1 - fU0) / (fX1 - fX0);
        float dv_dy = (fV1 - fV0) / (fY1 - fY0);

        if (clipFlags & 1)
        {
            float fDelta = clipRect.fX0 - fX0;
            fX0 = clipRect.fX0;
            fU0 = fDelta * du_dx + fU0;
        }
        if (clipFlags & 4)
        {
            float fDelta = fX1 - clipRect.fX1;
            fX1 = clipRect.fX1;
            fU1 = fU1 - fDelta * du_dx;
        }
        if (clipFlags & 2)
        {
            float fDelta = clipRect.fY0 - fY0;
            fY0 = clipRect.fY0;
            fV0 = fDelta * dv_dy + fV0;
        }
        if (clipFlags & 8)
        {
            float fDelta = fY1 - clipRect.fY1;
            fY1 = clipRect.fY1;
            fV1 = fV1 - fDelta * dv_dy;
        }
    }

    // Set vertices
    _D3DTLVERTEX* pv = m_saVertices;
    pv->sx = fX0;
    pv->sy = fY0;
    pv->sz = 0.01f;
    pv->rhw = 100.0f;
    pv->color = m_sOverData.dwColor;
    pv->specular = 0xFF000000;
    pv->tu = fU0;
    pv->tv = fV0;

    pv = pv + 1;
    pv->sx = fX1;
    pv->sy = fY0;
    pv->sz = 0.01f;
    pv->rhw = 100.0f;
    pv->color = m_sOverData.dwColor;
    pv->specular = 0xFF000000;
    pv->tu = fU1;
    pv->tv = fV0;

    pv = pv + 1;
    pv->sx = fX1;
    pv->sy = fY1;
    pv->sz = 0.01f;
    pv->rhw = 100.0f;
    pv->color = m_sOverData.dwColor;
    pv->specular = 0xFF000000;
    pv->tu = fU1;
    pv->tv = fV1;

    pv = pv + 1;
    pv->sx = fX0;
    pv->sy = fY1;
    pv->sz = 0.01f;
    pv->rhw = 100.0f;
    pv->color = m_sOverData.dwColor;
    pv->specular = 0xFF000000;
    pv->tu = fU0;
    pv->tv = fV1;

    m_bVisible = 1;
    return 0;
}

int cD6AlphaOverlay::LockAndGetBitmap(grs_bitmap** ppsBitmap)
{
    if (m_nTexID != -1)
        return E_LGD3DOEC_BITMAP_LOCKED;
    if (m_bTextureLoaded != 0)
    {
        m_nTexID = (int)m_psBitmap->bits;
        g_tmgr->restore_bits(m_psBitmap);
        *ppsBitmap = m_psBitmap;
        return 0;
    }
    return E_LGD3DOEC_BITMAP_MIA;
}

int cD6AlphaOverlay::UnLockAndReload()
{
    if (m_nTexID == -1)
        return E_LGD3DOEC_BITMAP_UNLOCKED;
    if (m_bTextureLoaded != 0)
    {
        m_psBitmap->bits = (uchar*)m_nTexID;
        g_tmgr->reload_texture(m_psBitmap);
        m_nTexID = -1;
        return 0;
    }
    return E_LGD3DOEC_BITMAP_MIA;
}

void cD6AlphaOverlay::DrawOverlay()
{
    if (m_bVisible == 0) return;
    if (m_bTurnedOff != 0) return;
    if (m_nTexID != -1)
    {
        Warning(("Draw(alpha)Overlay: the bitmap is locked!\n"));
        return;
    }
    if (m_bAlphaBlend != 0)
    {
        lgd3d_set_blend(1);
    }
    lgd3d_set_texture_level(0);
    if (g_tmgr)
    {
        g_tmgr->set_texture(m_psBitmap);
    }
    m_bTextureLoaded = 1;
    if (pcStates->EnableMTMode(0) != 0)
    {
        ((LPDIRECT3DDEVICE3)g_lpD3Ddevice)->DrawPrimitive(
            D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, m_saVertices, 4, D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS);
    }
}

void cD6AlphaOverlay::SetPositionF(float fX, float fY)
{
    float fDiffX = m_sOverData.fX1 - m_sOverData.fX0;
    float fDiffY = m_sOverData.fY1 - m_sOverData.fY0;
    m_sOverData.fX0 = fX;
    m_sOverData.fX1 = fX + fDiffX;
    m_sOverData.fY0 = fY;
    m_sOverData.fY1 = fY + fDiffY;
}

void cD6AlphaOverlay::SetColor(int nRed, int nGreen, int nBlue, int nAlpha)
{
    m_sOverData.dwColor = (nAlpha << 24) | (nRed << 16) | (nGreen << 8) | nBlue;
    m_bAlphaBlend = (nAlpha < 0xFF);
}

void cD6AlphaOverlay::SetAlpha(int nAlpha)
{
    m_sOverData.dwColor = (m_sOverData.dwColor & 0x00FFFFFF) | (nAlpha << 24);
    m_bAlphaBlend = (nAlpha < 0xFF);
}

cD6OverlayHandler::cD6OverlayHandler()
{
    m_viewRect.fX0 = 0.0f;
    m_viewRect.fY0 = 0.0f;
    m_viewRect.fX1 = (float)g_dwScreenWidth;
    m_viewRect.fY1 = (float)g_dwScreenHeight;
    m_cListHead.m_pcParent = NULL;
    m_cListHead.m_pcNext = NULL;
    m_cListHead.m_pcPrevious = NULL;
}

cD6OverlayHandler::~cD6OverlayHandler()
{
    RemoveAllOverlays();
}

void cD6OverlayHandler::AddOverlay(cD6OvelayType* pcNewOver, HLGD3DOVERLAY* phOver)
{
    cD6OvelayType* pCurrent = &m_cListHead;
    while (pCurrent->m_pcNext)
        pCurrent = pCurrent->m_pcNext;
    pCurrent->m_pcNext = pcNewOver;
    pcNewOver->m_pcNext = NULL;
    pcNewOver->m_pcPrevious = pCurrent;
    pcNewOver->m_pcParent = this;
    *phOver = MakeNewOverlayHandle(pcNewOver);
}

void cD6OverlayHandler::InsertOverlayAfter(cD6OvelayType* pcNewOver, cD6OvelayType* pcAfterOver, HLGD3DOVERLAY* phOver)
{
#line 861
    AssertMsg(pcNewOver != NULL, "pcNewOver == NULL");
    AssertMsg(pcAfterOver != NULL, "pcAfterOver == NULL");
    pcNewOver->m_pcNext = pcAfterOver->m_pcNext;
    if (pcNewOver->m_pcNext)
    {
        pcNewOver->m_pcNext->m_pcPrevious = pcNewOver;
    }
    pcAfterOver->m_pcNext = pcNewOver;
    pcNewOver->m_pcPrevious = pcAfterOver;
    pcNewOver->m_pcParent = this;
    *phOver = MakeNewOverlayHandle(pcNewOver);
}

void cD6OverlayHandler::RemoveOverlay(cD6OvelayType* pcOver)
{
#line 885
    AssertMsg(pcOver->m_pcPrevious, "pcOver->m_pcPrevious == NULL");
    pcOver->m_pcPrevious->m_pcNext = pcOver->m_pcNext;
    if (pcOver->m_pcNext)
    {
        pcOver->m_pcNext->m_pcPrevious = pcOver->m_pcPrevious;
    }
    delete pcOver;
}

void cD6OverlayHandler::RemoveAllOverlays()
{
    cD6OvelayType* pOver = m_cListHead.m_pcNext;
    if (pOver)
        KillBranch(pOver);
}

void cD6OverlayHandler::KillBranch(cD6OvelayType* pcOver)
{
    if (pcOver->m_pcNext)
        KillBranch(pcOver->m_pcNext);
    delete pcOver;
}

void cD6OverlayHandler::SetClipViewport(sOverRectangle* psInRect)
{
    if (psInRect)
    {
#line 928
        AssertMsg(psInRect->fX0 < psInRect->fX1, "psInRect->fX0 < psInRect->fX1");
        AssertMsg(psInRect->fY0 < psInRect->fY1, "psInRect->fY0 < psInRect->fY1");
        m_viewRect.fX0 = psInRect->fX0;
        m_viewRect.fX1 = psInRect->fX1;
        m_viewRect.fY0 = psInRect->fY0;
        m_viewRect.fY1 = psInRect->fY1;
    }
    else
    {
        m_viewRect.fX0 = 0.0f;
        m_viewRect.fY0 = 0.0f;
        m_viewRect.fX1 = (float)g_dwScreenWidth;
        m_viewRect.fY1 = (float)g_dwScreenHeight;
    }
}

void cD6OverlayHandler::GetClipViewport(sOverRectangle* psOutRect)
{


    AssertMsg(psOutRect != NULL, "psOutRect == NULL");
    psOutRect->fX0 = m_viewRect.fX0;
    psOutRect->fX1 = m_viewRect.fX1;
    psOutRect->fY0 = m_viewRect.fY0;
    psOutRect->fY1 = m_viewRect.fY1;
}

void cD6OverlayHandler::DrawOverlays()
{
    cD6OvelayType* pOver = m_cListHead.m_pcNext;
    if (pOver)
    {
        int varA = lgd3d_is_zwrite_on();
        int varB = lgd3d_is_zcompare_on();
        lgd3d_blend_normal();
        if (varA)
            lgd3d_set_zwrite(0);
        if (varB)
            lgd3d_set_zcompare(0);
        while (pOver) {
            pOver->DrawOverlay();
            pOver = pOver->m_pcNext;
        }
        if (varA)
            lgd3d_set_zwrite(1);
        if (varB)
            lgd3d_set_zcompare(1);
        lgd3d_set_blend(0);
    }
}

BOOL cD6OverlayHandler::OverlayFromHandle(HLGD3DOVERLAY hOver, cD6OvelayType** ppcOver)
{
    cD6OvelayType* pOver = m_cListHead.m_pcNext;
    *ppcOver = NULL;
    while (pOver)
    {
        if (pOver->m_dwID == hOver)
        {
            *ppcOver = pOver;
            break;
        }
        pOver = pOver->m_pcNext;
    }
#line 1017
    AssertMsg1(*ppcOver != NULL, "invalid overlay handle : %d", hOver);
    return *ppcOver != NULL;
}

BOOL cD6OverlayHandler::AlphaOverlayFromHandle(HLGD3DOVERLAY hOver, cD6AlphaOverlay** ppcAlphaOver)
{
    cD6OvelayType* pOver;
    *ppcAlphaOver = NULL;
    if (OverlayFromHandle(hOver, &pOver))
        *ppcAlphaOver = (cD6AlphaOverlay*)pOver;
    return *ppcAlphaOver != NULL;
}
