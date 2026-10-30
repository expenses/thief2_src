// $Header: x:/prj/tech/libsrc/object/RCS/edittool.h 1.3 1999/01/27 16:00:33 mahk Exp $
#pragma once  
#ifndef __EDITTOOL_H
#define __EDITTOOL_H

#include <comtools.h>
#include <objtype.h>

////////////////////////////////////////////////////////////
// EDITOR TOOLS COMPONENT
//
// This "editing tools" interface is a legacy from the Dark engine.  
// It collects together a few useful functions that the editing dialogs need
//

F_DECLARE_INTERFACE(IEditTools);

#undef INTERFACE
#define INTERFACE IEditTools

DECLARE_INTERFACE_(IEditTools,IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   //
   // Get a human-readable name for an object 
   //
   STDMETHOD_(const char*,ObjName)(THIS_ ObjID obj) PURE;

   // 
   // Parse an object by name
   //
   STDMETHOD_(ObjID,GetObjNamed)(THIS_ const char* name) PURE; 

   //
   // Edit an object with a registered type name
   // 
   STDMETHOD(EditTypedData)(THIS_ const char* title, const char* type, void* data) PURE;

};

#undef INTERFACE

//
// Factory 
//

EXTERN void EditToolsCreate(void); 




#endif // __EDITTOOL_H
