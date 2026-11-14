// $Header: x:/prj/tech/libsrc/object/property/RCS/propnet.h 1.1 1999/04/07 14:40:22 TOML Exp $
#pragma once  
#ifndef __PROPNET_H
#define __PROPNET_H

#ifdef TODO_ABSTRACT_OBJECT_NETWORKING

////////////////////////////////////////////////////////////
// PROPERTY NETWORKING API 
//

#undef INTERFACE 
#define INTERFACE IPropertyNetworking
DECLARE_INTERFACE_(IPropertyNetworking,IUnknown)
{
   //
   // Standard IUnknown Stuff
   //
   DECLARE_UNKNOWN_PURE(); 

   //
   // Send network message regarding a change to this property.  If type's 
   // kListenPropRequest bit is set, then this will send a change request of to obj's
   // host machine, otherwise it will send a broadcast of a change that has happened.
   // Returns FALSE when it is already processing a receive for this property.
   //
   STDMETHOD_(BOOL,SendPropertyMsg)(ObjID obj, sDatum value, ePropertyListenMsg type) PURE;

   // 
   // Process an incoming message regarding this property.
   //
   STDMETHOD_(void,ReceivePropertyMsg)(const sNetMsg_Generic *pMsg, ulong size, ObjID fromPlayer) PURE;
}; 

#endif

#endif // __PROPNET_H
