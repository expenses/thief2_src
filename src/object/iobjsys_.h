// $Header: x:/prj/tech/libsrc/object/objcore/RCS/iobjsys_.h 1.1 1999/04/07 14:38:42 TOML Exp $
#ifndef __IOBJSYS__H
#define __IOBJSYS__H
#include <iobjsys.h>
#include <objdef.h>
#include <aggmemb.h>
#include <nameprop.h>
#include <dlistsim.h>
#include <hashset.h>
#include <dynarray.h>

#include <objproxy.h>

F_DECLARE_INTERFACE(IPropertyManager);
F_DECLARE_INTERFACE(ILinkManager);
F_DECLARE_INTERFACE(ITraitManager);
F_DECLARE_INTERFACE(ITagFile);
#ifdef TODO_ABSTRACT_OBJECT_NETWORKING
F_DECLARE_INTERFACE(INetManager);
F_DECLARE_INTERFACE(IObjectNetworking);
#endif

////////////////////////////////////////

typedef cSimpleDListNode<ObjID> cObjListNode;

class cObjList: public cSimpleDList<ObjID>
{
}; 

class cListeners : public cDynArray<sObjListenerDesc>
{
}; 

template <class NODE>
class cObjIDHashSet : public cHashSet<NODE, ObjID, cHashFunctions> {};

class cActiveTable : public cObjIDHashSet<cObjListNode *>
{
   virtual tHashSetKey GetKey(tHashSetNode p) const
   {
      return (tHashSetKey)(((cObjListNode *)p)->Value());
   }
};

////////////////////////////////////////

const HostID  kHostMax   = 0x7FFF;
const int     kHostShift = 16;
const ObjID   kHostMask  = ((ObjID)kHostMax) << kHostShift;

const ObjID   kObjMax   = 0x0000FFFF;
const ObjID   kObjMask  = 0x0000FFFF;


inline ObjID HostObj(HostID host, ObjID obj)
{
   Assert_(OBJ_IS_CONCRETE(obj));

   return ((ObjID)(host) << kHostShift) | (obj & kObjMask);
}

////////////////////////////////////////

//
// OBJECT SYSTEM IMPLEMENTATION CLASS
// 

class cObjectSystem : public IObjectSystem
{
   DECLARE_AGGREGATION(cObjectSystem);

protected: 

   static cObjectSystem* TheObjSys;

   // Helper functions
   void notify_obj(ObjID id,ulong msg);
   void notify_obj_all(eObjConcreteness which,ulong msg); 
   void send_post_load(eObjPartition part); 
   
   void remove_obj(ObjID obj); 

   void build_obj_space();
   void build_free_lists(); 

   ObjID get_next_obj(eObjConcreteness concrete);

   ObjID begin_create(ObjID objID, ObjID archtype);

   void    SaveActiveObjects(ITagFile *file, eObjPartition partition);
   HRESULT LoadActiveObjects(ITagFile *file, eObjPartition partition);

public:
   cObjectSystem(IUnknown* pOuter);
   virtual ~cObjectSystem();

   //
   // IObjectSystem methods
   //
   STDMETHOD_(ObjID,Create)(ObjID archetype, eObjConcreteness concrete = kObjectConcrete);
   STDMETHOD_(ObjID,CreateID)(ObjID objID, ObjID archetype);
   STDMETHOD_(ObjID,BeginCreate)(ObjID archetype, eObjConcreteness concrete);
   STDMETHOD_(ObjID,BeginCreateID)(ObjID objID, ObjID archetype);
   STDMETHOD(EndCreate)(ObjID obj);
   STDMETHOD(Destroy)(ObjID obj);
   STDMETHOD(Reset)();
   STDMETHOD_(BOOL,Exists)(ObjID obj);
   STDMETHOD(SetHost)(HostID host);
   STDMETHOD_(HostID,GetHost)();
   STDMETHOD_(HostID,GetObjHost)(ObjID obj);
   STDMETHOD_(ObjID,GetLocalID)(ObjID obj);
   STDMETHOD_(IObjectQuery*,Iter)(eObjConcreteness which);
   STDMETHOD(DatabaseNotify)(eObjNotifyMsg msg, ObjNotifyData data);
   STDMETHOD(NameObject)(ObjID obj, const char* name);
   STDMETHOD_(const char*,GetName)(ObjID obj);
   STDMETHOD_(ObjID,GetObjectNamed)(const char* name);
   STDMETHOD_(IProperty*,GetCoreProperty)(eObjSysProperty which);
   STDMETHOD(SetObjTransience)(ObjID obj, BOOL transient);
   STDMETHOD_(BOOL,ObjIsTransient)(ObjID obj);
   STDMETHOD(CloneObject)(ObjID to, ObjID from);

   STDMETHOD_(tObjListenerHandle,Listen)(sObjListenerDesc* desc) ; 
   STDMETHOD(Unlisten)(tObjListenerHandle handle) ; 

   STDMETHOD_(ObjID,MaxObjID)(eObjConcreteness concrete) ;
   STDMETHOD_(ObjID,MinObjID)(eObjConcreteness concrete) ;

   STDMETHOD(Lock)(); 
   STDMETHOD(Unlock)(); 

   STDMETHOD_(BOOL,IsObjSavePartition)(ObjID obj, eObjPartition part); 
   STDMETHOD_(BOOL,IsObjLoadPartition)(ObjID obj, eObjPartition part); 
   STDMETHOD_(ObjID,RemapOnLoad)(ObjID src);
   STDMETHOD(SetSubPartitionFilter)(eObjPartition subpart, const sObjPartitionFilter* filter); 

   STDMETHOD_(eObjPartition,ObjDefaultPartition)(ObjID obj); 

#ifndef SHIP
   STDMETHOD(SpewObjList)(eObjConcreteness which);
#endif

protected: 

   void HideTransients(IBoolProperty *transient);
   void UnhideTransients(IBoolProperty *transient);

   enum { kNumSubPartitions = 8, }; 

   struct sFilter : public sObjPartitionFilter
   {
      sFilter(const sObjPartitionFilter& f) { *(sObjPartitionFilter*)this = f; }; 
      sFilter() { func = NULL; } ;
   };

   //
   // Aggregate Protocol
   //
   HRESULT Init();
   HRESULT End();

   IPropertyManager* PropMan;
   ILinkManager* LinkMan;
   ITraitManager* TraitMan;
#ifdef TODO_ABSTRACT_OBJECT_NETWORKING
   INetManager* NetMan;
   IObjectNetworking* ObjNet;
#endif
   IInvStringProperty* SymName;
   IBoolProperty* Transient;
   IBoolProperty* Immobile;

   cObjList mActiveConcreteObjs;
   cObjList mActiveAbstractObjs;
   cObjList mFreeConcreteObjs;
   cObjList mFreeAbstractObjs;

   cObjList mLoadedObjs; 
   cObjList mDeletedObjs;
   
   int mLockCount; 

   HostID mHostID;

   ObjID mMinConcrete;
   ObjID mMaxConcrete;

   ObjID mMinAbstract;
   ObjID mMaxAbstract;

   ObjID mTopObj;
   ObjID mBottomObj;

   class cActiveTable mActiveObjTable;

   cListeners mListeners; 
   sFilter mSubPartFilters[kNumSubPartitions]; 
   

};



#endif // __IOBJSYS__H
