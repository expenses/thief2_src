// $Header: x:/prj/tech/libsrc/object/property/RCS/propbool.h 1.2 1999/04/21 11:38:35 ccarollo Exp $
#pragma once  
#ifndef __PROPBOOL_H
#define __PROPBOOL_H

#include <propsto_.h>
#include <dataops_.h>
#include <packflag.h>
#include <iobjsys.h>
#include <objdef.h>

////////////////////////////////////////////////////////////
// Special "Bit Vector" property implementation for booleans
//
// Not templatized.  
// 
//

class cBoolPropertyStore: public cPropertyStore<cSimpleDataOps>
{
   
public:
   cBoolPropertyStore(ObjID min = gObjSys->GetLocalID(gObjSys->MaxObjID(kObjectAbstract)), 
                      ObjID max = gObjSys->GetLocalID(gObjSys->MaxObjID(kObjectConcrete)))
      : mValue(max-min),
        mInUse(max-min),
        mMinObj(min),
        mMaxObj(max)
   {
   }

   ~cBoolPropertyStore()
   {
   }

   //
   // Helpers
   // 
   
   BOOL InUse(ObjID obj) const { return mInUse.IsSet(obj - mMinObj); };
   void SetInUse(ObjID obj) { mInUse.Set(obj - mMinObj); }; 
   void ClearInUse(ObjID obj) { mInUse.Clear(obj - mMinObj); }; 

   BOOL Value(ObjID obj) const { return mValue.IsSet(obj - mMinObj); }; 
   void SetValue(ObjID obj, BOOL val) { mValue.Assign(obj - mMinObj,val); }; 

   BOOL InBounds(ObjID obj) const 
   { 
      return obj >= mMinObj && obj < mMaxObj;
   }      

   void ValidateHost(ObjID obj) const
   {
      AssertMsg(gObjSys->GetObjHost(obj) == gObjSys->GetHost(),
                "Bool property store can only be used on locally created properties!");
   }

   //
   // METHODS 
   // 

   STDMETHOD_(const sPropertyStoreDesc*,Describe)() const
   {
      static sPropertyStoreDesc desc = { "Bit Vector" }; 
      return &desc; 
   }

   STDMETHOD_(sDatum,Create)(ObjID obj) 
   {
      ValidateHost(obj);
      obj = gObjSys->GetLocalID(obj);

      sDatum val; 
      if (InBounds(obj) && !InUse(obj))
      {
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
         ClearInUse(obj);
         SetValue(obj,FALSE); 
      }
      return S_FALSE; 
   }

   STDMETHOD(Release)(ObjID obj, sDatum* pval)
   {
      ValidateHost(obj);
      obj = gObjSys->GetLocalID(obj);

      if (InBounds(obj) && InUse(obj))
      {
         pval->value = (void *)Value(obj);
         ClearInUse(obj);
         SetValue(obj,FALSE); 
      }
      return S_FALSE; 
   }

   STDMETHOD_(BOOL,Relevant)(ObjID obj)  const 
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
         pval->value = (void*)Value(obj); 
      }
      return FALSE; 
   }

   STDMETHOD(Set)(ObjID obj, sDatum val)
   {
      ValidateHost(obj);
      obj = gObjSys->GetLocalID(obj);

      if (!InBounds(obj))
         return E_FAIL; 

      HRESULT retval = InUse(obj) ? S_FALSE : S_OK; 

      SetInUse(obj);
      SetValue(obj,(BOOL)val.value); 

      return retval; 
   }

   STDMETHOD_(sDatum,Copy)(ObjID targ, ObjID src) 
   {
      ValidateHost(targ);
      ValidateHost(src);

      targ = gObjSys->GetLocalID(targ);
      src = gObjSys->GetLocalID(src);

      if (!InBounds(targ) || !InBounds(src) || !InUse(src))
         return sDatum(); 

      BOOL set = Value(src); 
      SetValue(targ,set); 
      SetInUse(targ); 
      return sDatum((void*)set); 
   }

   STDMETHOD(Reset)()
   {
      mInUse.ClearAll();
      mValue.ClearAll(); 
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
         value->value = (void*)Value(piter->next); 
      piter->next++; 

      return TRUE; 
   }

   STDMETHOD(IterStop)(sPropertyObjIter* ) const 
   {
      return S_OK; 
   }

protected: 
   cPackedBoolSet mValue;
   cPackedBoolSet mInUse; 
   ObjID mMinObj;
   ObjID mMaxObj;
   
};


#endif // __PROPBOOL_H

