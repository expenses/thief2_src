// $Header: x:/prj/tech/libsrc/object/rcs/iobjnet.h 1.6 1999/02/08 12:18:33 ccarollo Exp $
//
// Object Networking
//
// This system serves as the interface between the Object System and the
// Network Manager. It deals with mapping objects between different machines,
// and keeping track of which machine "owns" which objects.
//

#ifndef __IOBJNET_H
#define __IOBJNET_H

#include <iobjsys.h>
#include <comtools.h>
#include <nettype.h>

F_DECLARE_INTERFACE(IObjectNetworking);

#undef INTERFACE
#define INTERFACE IObjectNetworking
DECLARE_INTERFACE_(IObjectNetworking,IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   ////////
   //
   // General-interest methods
   //

   //
   // Establish the mapping from a remote ObjID to the local one. External
   // systems shouldn't usually need to call this, except in ususual
   // circumstances (eg, when the remote and local objects have different
   // archetypes).
   //
   STDMETHOD_(void, ObjRegisterProxy)(THIS_ 
                                      ObjID hostPlayer, 
                                      NetObjID objIdOnHost, 
                                      ObjID proxy) PURE;

   //
   // Given the host of an object and the ID on that host, returns the proxy objid that 
   // can be used on this machine.  
   //
   STDMETHOD_(ObjID,ObjGetProxy)(THIS_ 
                                 ObjID hostPlayer, 
                                 NetObjID objIdOnHost) PURE;

   //
   // Is this object a proxy for an object owned on another machine?
   // (False for kLocalOnly objects).
   //
   STDMETHOD_(BOOL,ObjIsProxy)(THIS_ ObjID obj) PURE;

   //
   // Is this object hosted on this machine (changes get broadcast).
   // (Also False for kLocalOnly objects).
   //
   STDMETHOD_(BOOL,ObjHostedHere)(THIS_ ObjID obj) PURE;

   //
   // Get the player that hosts this obj.  HostPlayer will be OBJ_NULL for kLocalOnly
   // objects, and PlayerObj() for global objects that are hosted here.
   STDMETHOD_(ObjID,ObjHostPlayer)(THIS_ ObjID obj) PURE;

   //
   // Get the ObjID of obj on obj's host player's machine.
   STDMETHOD_(NetObjID,ObjHostObjID)(THIS_ ObjID obj) PURE;

   //
   // Get both the Host player and the ID of 'obj' on that player's machine.
   // (Faster than getting one at a time).
   STDMETHOD_(void,ObjHostInfo)(THIS_ 
                                ObjID obj, 
                                ObjID *hostPlayer,
                                NetObjID *objIdOnHost) PURE;

   //
   // Object handoff method. This method tells the object system to
   // take over responsibility for the specified object.
   STDMETHOD_(void,ObjTakeOver)(THIS_ ObjID obj) PURE;

   //
   // These two methods are an unspeakably ugly hack, whose wisdom should
   // be re-examined once things are working.
   //
   // The problem is that scripts, broadly speaking, only run on the host
   // machine for an object. But there are occasions (such as frobbing a
   // FrobLocally object) when we specifically want to allow scripts to run
   // on a client. This method allows you to enable and disable local script
   // running. It should tightly bracket the relevant ScriptMan calls.
   STDMETHOD_(void,AllowClientScripts)(THIS_ BOOL bAllow) PURE;

   ////////
   //
   // ObjSys methods
   //
   // These methods are mainly here for use by the Object System itself.
   //
   STDMETHOD_(void, BroadcastBeginCreate)(THIS_ 
                                          ObjID exemplar, 
                                          ObjID obj) PURE;
   STDMETHOD_(void, BroadcastEndCreate)(THIS_ ObjID obj) PURE;
   STDMETHOD_(void, BroadcastDestroy)(THIS_ ObjID obj) PURE;
   STDMETHOD_(BOOL, CreatingProxy)(THIS) PURE;
};

#undef INTERFACE 

EXTERN tResult LGAPI ObjectNetworkingCreate(void);

#endif // __IOBJNET_H
