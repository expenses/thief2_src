#include <lgd3d.h>
#include <d3d.h>
#include <d6states.h>
#include <d6over.h>
#include <stdlib.h>
#include <lgassert.h>
#include <d6hrmsg.h>
#include <d6check.h>
#include <dbg.h>
#include <lglog.h>
#include <lgassert.h>

cD6Renderer* pcRenderer;
cD6Renderer* cD6Renderer::m_RendererInstance = NULL;

#include <d6prim.h>

cD6Renderer* cD6Renderer::Instance(unsigned long mode, unsigned long* pInitFlags)
{
    if (cD6Renderer::m_RendererInstance == NULL)
        cD6Renderer::m_RendererInstance = new cD6Renderer(mode, pInitFlags);
    return cD6Renderer::m_RendererInstance;
}

cD6Renderer* cD6Renderer::DeInstance()
{
    if (m_RendererInstance)
    {
        delete m_RendererInstance;
        m_RendererInstance = NULL;
    }
    return m_RendererInstance;
}

cD6Renderer::cD6Renderer(unsigned long mode, unsigned long* pInitFlags)
{
    m_bOverlaysOn = 1;
    switch (mode) {
        case 1:
            pcRenderBuffer = cMSBuffer::Instance();
            pcStates = cMSStates::Instance();
            break;
        case 0:
            pcRenderBuffer = cImBuffer::Instance();
            pcStates = cImStates::Instance();
            break;
    }
    CreateStatesStack(10, pcStates->GetRenderStatesSize());
    if (pcStates->Initialize(*pInitFlags) == 0)
    {
        if (mode == 1)
        {
            pcStates->DeInstance();
            pcRenderBuffer->DeInstance();
            DeletStatesStack();
            *pInitFlags &= ~LGD3DF_MULTI_TEXTURING;
            pcRenderBuffer = cImBuffer::Instance();
            pcStates = cImStates::Instance();
            CreateStatesStack(10, pcStates->GetRenderStatesSize());
            if (pcStates->Initialize(*pInitFlags) != 0)
            {
                Warning(("Can not use multitextures.  Reverting to single texture mode.\n"));
                return;
            }
        }
        lgd3d_g_bInitialized = 0;
        Warning(("Could not Initialize D3D states\n"));
    }
}

cD6Renderer::~cD6Renderer()
{
    pcStates->DeInstance();
    pcRenderBuffer->DeInstance();
    DeletStatesStack();
    if (pcOverlayHandler)
    {
        delete pcOverlayHandler;
        pcOverlayHandler = NULL;
    }
}

void cD6Renderer::StartFrame(int frame)
{
    int iResult = IDirect3DDevice3_BeginScene(g_lpD3Ddevice);
#line 166
    AssertMsg(iResult >= 0, "BeginScene failed");
    pcRenderBuffer->StartFrame();
    if (g_tmgr)
    {
        g_tmgr->start_frame(frame);
    }
}

void cD6Renderer::EndFrame()
{
    pcRenderBuffer->EndFrame();
    if (m_bOverlaysOn != 0 && pcOverlayHandler)
    {
        pcOverlayHandler->DrawOverlays();
    }
    int result = IDirect3DDevice3_EndScene(g_lpD3Ddevice);
    if (g_tmgr)
    {
        g_tmgr->end_frame();
    }
}

void cD6Renderer::CleanDepthBuffer(int x0, int y0, int x1, int y1)
{
    float left = (float)x0 + g_XOffset;
    float right = (float)x1 + g_XOffset;
    float top = (float)y0 + g_YOffset;
    float bottom = (float)y1 + g_YOffset;

    if (left < 0.0f) left = 0.0f;
    if (right > g_dwScreenWidth) right = (float)g_dwScreenWidth;
    if (top < 0.0f) top = 0.0f;
    if (bottom > g_dwScreenHeight) bottom = (float)g_dwScreenHeight;

    pcRenderBuffer->FlushPrimitives();

    int bZWrite = pcStates->IsZWriteOn();
    int bZCompare = pcStates->IsZCompareOn();

    if (!bZWrite)
        pcStates->SetZWrite(1);
    if (bZCompare)
        pcStates->SetZCompare(0);

    int bAlpha = pcStates->IsAlphaBlendingOn();
    pcStates->EnableAlphaBlending(1);

    _D3DTLVERTEX* pv = pcRenderBuffer->ReservePolySlots(4);
    memset(pv, 0, 4 * sizeof(_D3DTLVERTEX));
    for (int i = 0; i < 4; i++)
    {
        pv[i].sz = 1.0f;
        pv[i].rhw = 1.0f;
    }

    pv[0].sx = left;
    pv[0].sy = top;
    pv[1].sx = right;
    pv[1].sy = top;
    pv[2].sx = right;
    pv[2].sy = bottom;
    pv[3].sx = left;
    pv[3].sy = bottom;

    pcRenderBuffer->DrawPoly(1);

    if (!bZWrite)
        pcStates->SetZWrite(0);
    if (bZCompare)
        pcStates->SetZCompare(1);
    if (!bAlpha)
        pcStates->EnableAlphaBlending(0);
}


void cD6Renderer::CleanRenderSurface(BOOL bDepthBuffToo)
{
    DWORD dwFlags = D3DCLEAR_TARGET;
    if (bDepthBuffToo) dwFlags |= D3DCLEAR_ZBUFFER;
    D3DRECT rect;
    rect.x1 = 0;
    rect.y1 = 0;
    rect.x2 = g_dwScreenWidth;
    rect.y2 = g_dwScreenHeight;
    HRESULT hr = IDirect3DViewport3_Clear((LPDIRECT3DVIEWPORT3)g_lpViewport, 1, &rect, dwFlags);
#line 326
    CheckHResult(hr, "CleanRenderSurface failed.");
}

void cD6Renderer::CreateStatesStack(unsigned long depth, size_t size)
{
    m_dwEntrySize = size;
    m_dwNoEntries = depth;
#line 192
    m_pStackData = (char*)malloc(size * depth);
    m_pSetEntry = (char*)malloc(size);
    m_pdwRSCData = (unsigned long*)malloc(depth * 4);
    if (m_pStackData == NULL || m_pSetEntry == NULL || m_pdwRSCData == NULL) {
#line 346
        CriticalMsg("Memory Allocation failure!");
    }
    m_dwStackTop = 0;
    m_pCurrentEntry = m_pStackData;
    pcStates->SetPointerToCurrentStates(m_pCurrentEntry);
    pcStates->SetPointerToSetStates(m_pSetEntry);
}

long cD6Renderer::InitViewport(void) {
    D3DVIEWPORT2 Viewport;
    long lResult;

#line 421
    AssertMsg(g_lpViewport, "hey, no_viewport!");

    memset(&Viewport, 0, sizeof(Viewport));
    Viewport.dwSize = sizeof(Viewport);
    Viewport.dwWidth = g_dwScreenWidth;
    Viewport.dwHeight = g_dwScreenHeight;
    Viewport.dvClipX = -1.0f;
    Viewport.dvClipWidth = 2.0f;
    Viewport.dvClipY = -1.0f;
    Viewport.dvClipHeight = 2.0f;
    Viewport.dvMaxZ = 1.0f;

    lResult = g_lpViewport->SetViewport2(&Viewport);
    return lResult;

}

void cD6Renderer::DeletStatesStack()
{
#line 309
    free(m_pSetEntry);
    free(m_pStackData);
    free(m_pdwRSCData);
    m_pSetEntry = NULL;
    m_pStackData = NULL;
    m_pdwRSCData = NULL;
}
