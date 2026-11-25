///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/wdcreate.cpp $
// $Author: TOML $
// $Date: 1997/10/16 13:20:07 $
// $Revision: 1.11 $
//
// This is the one file where every concrete type is known.
//

#include <windows.h>
#include <ddraw.h>

#include <lg.h>
#include <comtools.h>
#include <aggmemb.h>
#include <appagg.h>
#include <wappapi.h>
#include <dispapi.h>
#include <dev2d.h>

#include <dddynf.h>
#include <wdisp.h>
#include <wdspprov.h>
#include <wdspmode.h>
#include <wdispgdi.h>
#include <wdispdd.h>
#include <config.h>
#include <wdispd3d.h>

extern "C" BOOL UseD3DDisplay(void)
{
   static BOOL bRead = FALSE;
   static BOOL bUse = FALSE;
   if (!bRead)
   {
      bUse = config_is_defined("use_d3d_display");
      bRead = TRUE;
   }
   return bUse;
}
#line 27

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Low-level Display Device create method
//

tResult LGAPI _DisplayDeviceCreate(REFIID, IDisplayDevice ** /* ppDisplayDevice */, IUnknown * pOuterUnknown, eDisplayDeviceKind kind, int flags)
{
    if (new cWinDisplayDevice(pOuterUnknown, kind, flags) != 0)
        return NOERROR;
    return E_FAIL;
}


///////////////////////////////////////////////////////////////////////////////
//
// cWinDisplayDevice calls this function to create a concrete implementation
// for mon-modal IDisplayDevice functionality.
//

class cWinDisplayProvider;

cWinDisplayProvider * CreateProvider(eDisplayDeviceKind kind, cWinDisplayDevice * pDisplayDevice)
{
    switch (kind)
    {
        case kDispDefault:
        case kDispWindowed:
        case kDispFullScreen:
        case kDispUndefined:
            if (UseD3DDisplay())
            {
                if (!LoadDirect3D9())
                    return NULL;
                return new cD3DProvider(pDisplayDevice);
            }
#line 63
            if (LoadDirectDraw())
                return new cDDProvider(pDisplayDevice);
            // else
            // fall through...

        case kDispDebug:
            return new cGDIProvider(pDisplayDevice);
    }
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////

