///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/dddynf.cpp $
// $Author: KEVIN $
// $Date: 1997/10/10 15:09:44 $
// $Revision: 1.2 $
//

#include <windows.h>
#include <ddraw.h>

#include <dddynf.h>

//
// _DirectDrawCreate@12
//
ImplDynFunc(DirectDrawCreate, "ddraw.dll", "DirectDrawCreate", 0);
ImplDynFunc(DirectDrawEnumerateA, "ddraw.dll", "DirectDrawEnumerateA", 0);

#include <d3d9.h>
#include <stdlib.h>
#include <lgassert.h>
#include <dbg.h>

DeclDynFunc_(IDirect3D9 *, WINAPI, Direct3DCreate9, (UINT SDKVersion));
DeclDynFunc_(HRESULT, WINAPI, D3DXLoadSurfaceFromSurface, (IDirect3DSurface9 *, const PALETTEENTRY *, const RECT *, IDirect3DSurface9 *, const PALETTEENTRY *, const RECT *, DWORD, D3DCOLOR));
DeclDynFunc_(HRESULT, WINAPI, D3DXFilterTexture, (IDirect3DBaseTexture9 *, const PALETTEENTRY *, UINT, DWORD));
DeclDynFunc_(HRESULT, WINAPI, D3DXCreateEffect, (IDirect3DDevice9 *, LPCVOID, UINT, const void *, void *, DWORD, void *, void **, void **));

ImplDynFunc(Direct3DCreate9, "d3d9.dll", "Direct3DCreate9", 0);
ImplDynFunc(D3DXLoadSurfaceFromSurface, "d3dx9_43.dll", "D3DXLoadSurfaceFromSurface", 0);
ImplDynFunc(D3DXFilterTexture, "d3dx9_43.dll", "D3DXFilterTexture", 0);
ImplDynFunc(D3DXCreateEffect, "d3dx9_43.dll", "D3DXCreateEffect", 0);

static void D3DLoadFailed(const char * pszMsg)
{
    MessageBox(NULL, pszMsg, NULL, MB_ICONERROR);
    pExitMsg = NULL;
    exit(1);
}

BOOL LoadDirect3D9()
{
    if (!DynFunc(Direct3DCreate9).Load())
        D3DLoadFailed("Unable to locate or load d3d9.dll");
    if (!DynFunc(D3DXLoadSurfaceFromSurface).Load() ||
        !DynFunc(D3DXFilterTexture).Load() ||
        !DynFunc(D3DXCreateEffect).Load())
        D3DLoadFailed("Unable to locate or load d3dx9_43.dll");
    return TRUE;
}

BOOL LoadDirectDraw()
{
    return DynFunc(DirectDrawCreate).Load() && DynFunc(DirectDrawEnumerateA).Load();
}
