#pragma once

#include <d3d9.h>
#include <ddraw.h>

struct IDirect3D3;
struct IDirect3DDevice3;
struct IDirect3DViewport3;
struct IDirect3DMaterial3;
struct IDirect3DTexture2;
typedef struct _D3DDeviceDesc D3DDEVICEDESC;

typedef struct _D3DTLVERTEX {
    float sx;
    float sy;
    float sz;
    float rhw;
    D3DCOLOR color;
    D3DCOLOR specular;
    float tu;
    float tv;
} D3DTLVERTEX;

typedef enum _D3DTEXTUREMINFILTER {
    D3DTFN_POINT = 1,
    D3DTFN_LINEAR = 2,
    D3DTFN_ANISOTROPIC = 3,
    D3DTFN_FORCE_DWORD = 0x7fffffff,
} D3DTEXTUREMINFILTER;
