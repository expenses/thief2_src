// $Header: x:/prj/tech/libsrc/object/objcore/RCS/objproxy.h 1.1 1999/04/07 14:38:43 TOML Exp $
//
// Proxy object manager
//
// In a networked environment, each machine "owns" some of the objects in
// the game. All of the other machines maintain "proxies" for those objects:
// simplified versions of the objects, which have the same archetype and
// position information, but which aren't as fully simulated as the real
// object.
//
// The proxy manager keeps track of hash tables that map between the real
// host objIDs to the local proxies. Conceptually, it is closely related
// to objsys.
//

#ifndef _OBJPROXY_H
#define _OBJPROXY_H

#include <hashpp.h>

#ifdef TODO_ABSTRACT_OBJECT_NETWORKING
#include <netprops.h> // for sHostObjID
#endif

#ifdef TODO_ABSTRACT_OBJECT_NETWORKING
//////////

class cProxyManager
{
public:
   cProxyManager();
   ~cProxyManager();

   void RegisterProxy(ObjID host, NetObjID netObjID, ObjID proxyID);
   ObjID GetProxy(ObjID host, NetObjID netObjID);
   void DeleteProxy(ObjID host, NetObjID netObjID);

private:
   typedef cGenHashFunc<sHostObjID> cHostObjIDFunc;
   typedef cHashTable<sHostObjID *, ObjID, cHostObjIDFunc> cHashByHostObjID;

   cHashByHostObjID *m_pTable;
};
#endif

#endif // !_OBJPROXY_H
