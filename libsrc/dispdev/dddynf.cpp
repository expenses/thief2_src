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


BOOL LoadDirectDraw()
{
    HRESULT result;
    LPDIRECTDRAW pDirectDraw;
    LPVOID pDD4;

    if (DynFunc(DirectDrawCreate).Load() == 0 ||
        DynFunc(DirectDrawEnumerateA).Load() == 0)
    {
        return 0;
    }

    result = DynDirectDrawCreate(NULL, &pDirectDraw, NULL);
    if (result != 0 ||
        pDirectDraw == NULL)
    {
        return 0;
    }

    result = pDirectDraw->QueryInterface(IID_IDirectDraw4, &pDD4);
    pDirectDraw->Release();
    if (result != 0 ||
        pDD4 == NULL)
    {
        return 0;
    }

    ((IUnknown *)pDD4)->Release();
    return 1;
}
