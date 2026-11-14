#pragma once

#include <d6states.h>

struct _MTVERTEX {
    float sx;
    float sy;
    float sz;
    float rhw;
    DWORD color;
    DWORD specular;
    float tu;
    float tv;
    float tu2;
    float tv2;
};

class cImBuffer : public cD6Primitives
{
    static cImBuffer* m_Instance;

    cImBuffer();
    ~cImBuffer();

public:
    static cD6Primitives* Instance();
    virtual cD6Primitives* DeInstance();
};

class cMSBuffer : public cD6Primitives
{
    static cMSBuffer* m_Instance;

    _MTVERTEX m_saMTVertices[50];
    int m_nMTVertices;
    int m_nMaxMTSlots;

    cMSBuffer();
    ~cMSBuffer();

    void DrawPoly(int);
    void DrawIndPolies();
    void FlushIndPolies();
    void DrawMTPoly();
    _MTVERTEX* ReserveMTPolySlots(int);

public:
    static cD6Primitives* Instance();
    virtual cD6Primitives* DeInstance();
    int TrifanMTD(int, _r3s_point**, LGD3D_tex_coord**);
    int LitTrifanMTD(int, _r3s_point**, LGD3D_tex_coord**);
    int RGBlitTrifanMTD(int, _r3s_point**, LGD3D_tex_coord**);
    int RGBAlitTrifanMTD(int, _r3s_point**, LGD3D_tex_coord**);
    int RGBAFogLitTrifanMTD(int, _r3s_point**, LGD3D_tex_coord**);
    int DiffuseSpecularLitTrifanMTD(int, _r3s_point**, LGD3D_tex_coord**);
    int g2TrifanMTD(int, g2s_point**, LGD3D_tex_coord**);
    int g2UTrifanMTD(int, g2s_point**, LGD3D_tex_coord**);
};
