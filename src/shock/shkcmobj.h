// $Header: r:/prj/cam/src/shock/RCS/shkcmobj.h 1.1 1999/02/09 09:45:22 JON Exp $

#ifndef __SHKCMOBJ_H
#define __SHKCMOBJ_H

#include <comtools.h>
#include <simtime.h>

//------------------------------------
// Camera Objects API
//

F_DECLARE_INTERFACE(ICameraObjects); 

#undef INTERFACE
#define INTERFACE ICameraObjects

DECLARE_INTERFACE_(ICameraObjects,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   // Update
   STDMETHOD(Frame)(THIS_ tSimTime timeDelta) PURE;
   // Fire gun
};

#undef INTERFACE

void ShockCameraObjectsCreate(void);

#endif // __SHKCMOBJ_H