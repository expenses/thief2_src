// $Header: x:/prj/tech/libsrc/object/RCS/netprops.h 1.5 1999/01/27 16:00:42 mahk Exp $

#ifndef __NETPROPS_H
#define __NETPROPS_H

#include <objtype.h>

#include <property.h>
#include <propface.h>
#include <objtype.h>  // for NetObjID
#include <iobjnet.h>  // for eNetworkCategory

////////////////////////////////////////////////////////////
//
// NETWORK CATEGORY PROPERTY
//

// The possible values for the network category; these are the values
// that can be returned by ObjNetworkCategory.
// When the property is not set, it is assumed to be kLocalOnly
enum eNetworkCategoryEnum {
   // Object is created on all machines, not synchronized or hosted:
   kLocalOnly,
   // Objects loaded from missions are hosted by the DefaultHost, run-time
   // objects are hosted by whatever object is in the proxy table.
   kHosted,
};
typedef int eNetworkCategory;

#define PROP_NETWORK_CATEGORY_NAME "NetworkCategory"

EXTERN eNetworkCategory NetworkCategory(ObjID obj);
EXTERN void             SetNetworkCategory(ObjID obj, eNetworkCategory cat);

////////////////////////////////////////////////////////////
//
// HEARTBEAT RATE PROPERTY 
//
// The value of this property determines how often this object will send
// out "heartbeat" updates to the other players. It should be smaller for
// objects whose movement is less predictable and more relevant (ie, AIs
// that are close to players), and larger for objects whose movement is
// more predictable (ie, projectiles). If set to zero, the object will
// never send a heartbeat. The unit is simulation time, and is more or
// less milliseconds.
//

// property name
#define PROP_HEARTBEAT_NAME "Heartbeat"

// Accessors

EXTERN int ObjGetHeartbeat(ObjID obj);
EXTERN void ObjSetHeartbeat(ObjID obj, int rate);

///////////////////////////////////////
//
// HostObjID PROPERTY
//
// A "host object ID", which describes who owns the real object
// that this proxy belongs to, and its objID on that host.

typedef struct sHostObjID {
   ObjID host;
   NetObjID netObjID; // ObjID on m_host, not necessarily a valid ObjID here.
} sHostObjID;

#undef  INTERFACE
#define INTERFACE IHostObjIDProperty
DECLARE_PROPERTY_INTERFACE(IHostObjIDProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(sHostObjID*); 
}; 

// Make this property visible directly, rather than through accessors, since
// objsys provides the a more general API called ObjHostObjID().
EXTERN IHostObjIDProperty *gHostObjIDProp;

/////////////////////////////////////////////////
// AvatarHeightOffset Property
//

#define PROP_AVATAR_HEIGHT_OFFSET_NAME "AvatarZOffset"
EXTERN IFloatProperty *gAvatarHeightOffsetProp;

/////////////////////////////
//
// Initialization
//

EXTERN void NetPropsInit(void);
EXTERN void NetPropsShutdown(void);

#endif // __NETPROPS_H


