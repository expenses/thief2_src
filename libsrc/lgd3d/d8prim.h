#pragma once

#include <d6states.h>

extern float g_fD8InvRectScale;
extern float g_fD8HalfTexelX;
extern float g_fD8HalfTexelY;
extern int g_nD8HalfTexelFixX;
extern int g_nD8HalfTexelFixY;

class cD8Primitives : public cD6Primitives
{
protected:
    cD8Primitives();
public:
    virtual ~cD8Primitives();
    virtual void FlushPoints();
    virtual void DrawPoly(int bSuspendTexturing);
    virtual int CubeTexTrifan(int n, _r3s_point** ppl, float* uvw, DWORD color);
    virtual void DrawTLVertices(int n, _D3DTLVERTEX* pVerts);
    virtual void DrawStandardEdges(void* pVerts, unsigned long n);
    virtual void FlushIndPolies();
    virtual int PolyInd(int, _r3s_point**, r3ixs_info*);
    virtual int SPolyInd(int, _r3s_point**, r3ixs_info*);
    virtual int RGB_PolyInd(int, _r3s_point**, r3ixs_info*);
    virtual int RGBA_PolyInd(int, _r3s_point**, r3ixs_info*);
    virtual int TrifanInd(int, _r3s_point**, r3ixs_info*);
    virtual int LitTrifanInd(int, _r3s_point**, r3ixs_info*);
    virtual int RGBlitTrifanInd(int, _r3s_point**, r3ixs_info*);
    virtual int RGBAlitTrifanInd(int, _r3s_point**, r3ixs_info*);
    virtual int RGBAFogLitTrifanInd(int, _r3s_point**, r3ixs_info*);
    virtual int DiffuseSpecularLitTrifanInd(int, _r3s_point**, r3ixs_info*);
    virtual void DrawMTVertices(int n, void* pVerts);
};

class cD8ImBuffer : public cD8Primitives
{
    static cD8ImBuffer* m_Instance;
    cD8ImBuffer() {}
public:
    static cD6Primitives* Instance();
    virtual cD6Primitives* DeInstance();
};

class cD8MSBuffer : public cD8Primitives
{
protected:
    static cD8MSBuffer* m_Instance;

    _MTVERTEX m_saMTVertices[50];
    int m_nMTVertices;
    int m_nMaxMTSlots;

    cD8MSBuffer() : m_nMaxMTSlots(50) {}

public:
    static cD6Primitives* Instance();
    virtual cD6Primitives* DeInstance();
    virtual void DrawPoly(int bSuspendTexturing);
    virtual int CubeTexTrifan(int n, _r3s_point** ppl, float* uvw, DWORD color);
    virtual int TexTrifanMT(int n, _r3s_point** ppl, float* uv, LGD3D_tex_coord** lm, DWORD color);
    virtual void DrawIndPolies();
    virtual void FlushIndPolies();
    virtual int TrifanMTD(int, _r3s_point**, LGD3D_tex_coord**);
    virtual int LitTrifanMTD(int, _r3s_point**, LGD3D_tex_coord**);
    virtual int RGBlitTrifanMTD(int, _r3s_point**, LGD3D_tex_coord**);
    virtual int RGBAlitTrifanMTD(int, _r3s_point**, LGD3D_tex_coord**);
    virtual int RGBAFogLitTrifanMTD(int, _r3s_point**, LGD3D_tex_coord**);
    virtual int DiffuseSpecularLitTrifanMTD(int, _r3s_point**, LGD3D_tex_coord**);
    virtual int g2UTrifanMTD(int, g2s_point**, LGD3D_tex_coord**);
    virtual int g2TrifanMTD(int, g2s_point**, LGD3D_tex_coord**);
    virtual _MTVERTEX* ReserveMTPolySlots(int n);
    virtual void DrawMTPoly();
};

#define D8_NUM_TL_BATCHES       4
#define D8_NUM_MT_BATCHES       8
#define D8_TL_BATCH_TRIS        1000
#define D8_ORDERED_BATCH_TRIS   4000
#define D8_MT_BATCH_TRIS        400
#define D8_BATCH_FREE           (-3)

struct sD8TLBatch {
    int nTexId;
    DWORD dwStamp;
    int nTris;
    _D3DTLVERTEX* pVerts;
};

struct sD8MTBatch {
    int naTexId[2];
    DWORD dwStamp;
    int nTris;
    _MTVERTEX* pVerts;
    grs_bitmap* pLightMap;
};

class cD8MSBatchBuffer : public cD8MSBuffer
{
public:
    cD8MSBatchBuffer();

    static void SelectTLBatch(int n);
    void SelectMTBatch(DWORD dwLevel, int n);
    void FlushBatches();

    virtual void FlushPoints();
    virtual _D3DTLVERTEX* ReservePolySlots(int n);
    virtual void DrawPoly(int bSuspendTexturing);
    virtual void DrawIndPolies();
    virtual void FlushIndPolies();
    virtual _MTVERTEX* ReserveMTPolySlots(int n);
    virtual void DrawMTPoly();

protected:
    void FlushTLBatch();
    void FlushMTBatch();

    int m_nPendingVerts;
    int m_nCurTL;
    int m_nCurMT;
    int m_nMaxTLTris;
    BOOL m_bOrdered;
    BOOL m_bMTMode;
    int m_nOrderedTL;
    sD8TLBatch m_aTL[D8_NUM_TL_BATCHES];
    sD8MTBatch m_aMT[D8_NUM_MT_BATCHES];
    union {
        _D3DTLVERTEX m_aTLVerts[D8_NUM_TL_BATCHES][D8_TL_BATCH_TRIS * 3];
        _MTVERTEX m_aMTVerts[D8_NUM_MT_BATCHES][D8_MT_BATCH_TRIS * 3];
    };
};
