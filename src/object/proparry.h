// $Header: x:/prj/tech/libsrc/object/property/RCS/proparry.h 1.2 1999/04/21 11:38:24 ccarollo Exp $
#pragma once  
#ifndef __PROPARRY_H
#define __PROPARRY_H

#include <propsto_.h>
#include <dataops_.h>
#include <propbase.h>
#include <packflag.h>
#include <iobjsys.h>
#include <objdef.h>

////////////////////////////////////////////////////////////
// ARRAY PROPERTY STORE IMPLEMENTATION 
//

template <class OPS> 
class cArrayPropertyStore: public cPropertyStore<OPS>
{
   
public:
   cArrayPropertyStore(ObjID min = gObjSys->GetLocalID(gObjSys->MaxObjID(kObjectAbstract)), 
                       ObjID max = gObjSys->GetLocalID(gObjSys->MaxObjID(kObjectConcrete)))
      : mArray(new sDatum[max-min]),
        mInUse(max-min),
        mMinObj(min),
        mMaxObj(max)
   {
      // translate array
      mArray -= mMinObj; 
   }

   ~cArrayPropertyStore()
   {
      mArray += mMinObj; 
      delete [] mArray; 
   }

   //
   // Helpers
   // 
   
   BOOL InUse(ObjID obj) const { return mInUse.IsSet(obj - mMinObj); };
   void SetInUse(ObjID obj)  { mInUse.Set(obj - mMinObj); }; 
   void ClearInUse(ObjID obj)  { mInUse.Clear(obj - mMinObj); }; 

   BOOL InBounds(ObjID obj) const
   { 
      return obj >= mMinObj && obj < mMaxObj;
   }      

   void ValidateHost(ObjID obj) const
   {
      AssertMsg(gObjSys->GetObjHost(obj) == gObjSys->GetHost(),
                "Array property store can only be used on locally created properties!");
   }

   //
   // FAST ACCESS PATH 
   //
   
   sDatum& operator[](int obj) const { Assert_(InBounds(obj)); return mArray[obj]; }; 


   //
   // METHODS 
   // 

   STDMETHOD_(const sPropertyStoreDesc*,Describe)() const 
   {
      static sPropertyStoreDesc desc = { "Array" }; 
      return &desc; 
   }

   STDMETHOD_(sDatum,Create)(ObjID obj) 
   {
      ValidateHost(obj);
      obj = gObjSys->GetLocalID(obj);

      sDatum val; 
      if (InBounds(obj) && !InUse(obj))
      {
         val = mOps.New(); 
         mArray[obj] = val; 
         SetInUse(obj); 
      }

      return val; 
   }

   STDMETHOD(Delete)(ObjID obj)
   {
      ValidateHost(obj);
      obj = gObjSys->GetLocalID(obj);

      if (InBounds(obj) && InUse(obj))
      {
         mOps.Delete(mArray[obj]); 
         mArray[obj] = sDatum(NULL); 
         ClearInUse(obj); 
         return S_OK; 
      }
      return S_FALSE; 
   }

   STDMETHOD(Release)(ObjID obj, sDatum *pval)
   {
      ValidateHost(obj);
      obj = gObjSys->GetLocalID(obj);

      if (InBounds(obj) && InUse(obj))
      {
         *pval = mArray[obj];
         mArray[obj] = sDatum(NULL); 
         ClearInUse(obj); 
         return S_OK; 
      }
      return S_FALSE; 
   }
      

   STDMETHOD_(BOOL,Relevant)(ObjID obj) const
   {
      if (gObjSys->GetObjHost(obj) != gObjSys->GetHost())
         return FALSE;

      obj = gObjSys->GetLocalID(obj);

      return InBounds(obj) && InUse(obj); 
   }

   STDMETHOD_(BOOL,Get)(ObjID obj, sDatum* pval) const
   {
      if (gObjSys->GetObjHost(obj) != gObjSys->GetHost())
         return FALSE;

      obj = gObjSys->GetLocalID(obj);

      if (InBounds(obj) && InUse(obj))
      {
         *pval = mArray[obj];
         return TRUE; 
      }
      return FALSE; 
   }

   STDMETHOD(Set)(ObjID obj, sDatum val)
   {
      ValidateHost(obj);
      obj = gObjSys->GetLocalID(obj);

      if (!InBounds(obj))
         return E_FAIL; 

      if (InUse(obj))
      {
         mOps.Copy(&mArray[obj],val); 
         return S_FALSE;
      }
      else
      {
         mArray[obj] = mOps.CopyNew(val);  
         SetInUse(obj); 
         return S_OK; 
      }
   }

   STDMETHOD_(sDatum,Copy)(ObjID targ, ObjID src) 
   {
      ValidateHost(targ);
      ValidateHost(src);

      targ = gObjSys->GetLocalID(targ);
      src = gObjSys->GetLocalID(src);

      if (!InBounds(targ) || !InBounds(src) || !InUse(src))
         return sDatum(); 

      sDatum val = mArray[src]; 

      if (InUse(targ))
      {
         mOps.Copy(&mArray[targ],val); 
      }
      else
      {
         mArray[targ] = mOps.CopyNew(val); 
         SetInUse(targ); 
      }
      return val; 
   }

   STDMETHOD(Reset)()
   {
      for (int i = mMinObj; i < mMaxObj; i++)
         if (InUse(i))
         {
            mOps.Delete(mArray[i]); 
            mArray[i] = sDatum(NULL); 
            ClearInUse(i); 
         }
      return S_OK; 
   }

   STDMETHOD(IterStart)(sPropertyObjIter* piter) const
   {
      piter->next = mMinObj; 

      return S_OK; 
   }

   STDMETHOD_(BOOL,IterNext)(sPropertyObjIter* piter, ObjID* obj, sDatum* value) const
   {
      while (piter->next < mMaxObj && !InUse(piter->next))
         piter->next++; 
      if (piter->next >= mMaxObj)
         return FALSE; 

      if (obj)
         *obj = piter->next;
      if (value)
         *value = mArray[piter->next]; 
      piter->next++; 

      return TRUE; 
   }

   STDMETHOD(IterStop)(sPropertyObjIter* ) const
   {
      return S_OK; 
   }

protected: 
   sDatum* mArray;
   cPackedBoolSet mInUse; 
   ObjID mMinObj;
   ObjID mMaxObj;
   
};

//------------------------------------------------------------
// Generic version
//

class cGenericArrayPropertyStore : public cArrayPropertyStore<cDelegatingDataOps>
{
public:
   cGenericArrayPropertyStore(ObjID min = gObjSys->GetLocalID(gObjSys->MaxObjID(kObjectAbstract)), 
                              ObjID max = gObjSys->GetLocalID(gObjSys->MaxObjID(kObjectConcrete)))
      : cArrayPropertyStore<cDelegatingDataOps>(min,max)
   {
      mOps.InitDelegation(this); 
   }

   STDMETHOD(SetOps)(IDataOps* ops)
   {  
      mOps.SetOps(ops); 
      return S_OK; 
   }

}; 



#endif // __PROPARRY_H
