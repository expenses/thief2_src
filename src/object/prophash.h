// $Header: x:/prj/tech/libsrc/object/property/RCS/prophash.h 1.2 1999/04/21 11:38:47 ccarollo Exp $

#ifndef PROPHASH_H
#define PROPHASH_H
#include <propsto_.h>
#include <dataops_.h>
#include <propbase.h>
#include <hashpp.h>


////////////////////////////////////////////////////////////
//
// HASH TABLE PROPERTY STORE IMPLEMENTATION 
//
//

#define DEFAULT_TABLE_SIZE 3

class cPropertyHashTable : public cHashTable<ObjID,sDatum,cHashFunctions>
{
public:
   cPropertyHashTable() 
      : cHashTable<ObjID,sDatum,cHashFunctions>(DEFAULT_TABLE_SIZE)
   {} 
}; 



template <class OPS> 
class cHashPropertyStore: public cPropertyStore<OPS>
{
   
public:
   typedef cPropertyHashTable cTable; 

   //
   // FAST ACCESS PATH 
   //

   cTable& GetTable() { return mTable; }; 

   //
   // METHODS 
   // 

   STDMETHOD_(const sPropertyStoreDesc*,Describe)() const 
   {
      static sPropertyStoreDesc desc = { "Hash Table" }; 
      return &desc; 
   }

   STDMETHOD_(sDatum,Create)(ObjID obj) 
   {
      Assert_(obj != OBJ_NULL);
      sDatum val; 
      if (!mTable.Lookup(obj,&val))
      {
         val = mOps.New(); 
         mTable.Set(obj,val); 
      }

      return val; 
   }

   STDMETHOD(Delete)(ObjID obj)
   {
      sDatum val;
      if (mTable.Lookup(obj,&val))
      {
         mOps.Delete(val);    
         mTable.Delete(obj);
         return S_OK; 
         
      }
      return S_FALSE; 
   }

   STDMETHOD(Release)(ObjID obj, sDatum* pval)
   {
      if (mTable.Lookup(obj,pval))
      {
         mTable.Delete(obj);
         return S_OK; 
         
      }
      return S_FALSE; 
   }

   STDMETHOD_(BOOL,Relevant)(ObjID obj) const 
   {
      return mTable.HasKey(obj); 
   }

   STDMETHOD_(BOOL,Get)(ObjID obj, sDatum* pval) const 
   {
      return mTable.Lookup(obj,pval); 
   }

   STDMETHOD(Set)(ObjID obj, sDatum val)
   {
      Assert_(obj != OBJ_NULL);
      HRESULT retval = S_OK; 
      sDatum setval; 
      if (mTable.Lookup(obj,&setval))
      {
         mOps.Copy(&setval,val);
         retval = S_FALSE; 
      }
      else
         setval = mOps.CopyNew(val);
      mTable.Set(obj,setval);
      return retval; 
   }

   STDMETHOD_(sDatum,Copy)(ObjID targ, ObjID src) 
   {
      Assert_(targ != OBJ_NULL);
      sDatum srcval; 
      if (mTable.Lookup(src,&srcval))
      {
         sDatum targval = mOps.CopyNew(srcval); 
         mTable.Set(targ,targval); 
         return srcval;
      }
      return sDatum(0); 
   }

   STDMETHOD(Reset)()
   {
      cTable::cIter iter;  
      for (iter = mTable.Iter(); !iter.Done(); iter.Next())
      {
         mOps.Delete(iter.Value());
      }
      mTable.Clear(); 
      return S_OK; 
   }

   STDMETHOD(IterStart)(sPropertyObjIter* piter) const 
   {
      cTable::cIter& iter = *(cTable::cIter*)piter; 
      Assert_(sizeof(*piter) >= sizeof(iter)); 
      
      iter = mTable.Iter(); 
      return S_OK; 
   }

   STDMETHOD_(BOOL,IterNext)(sPropertyObjIter* piter, ObjID* obj, sDatum* value) const 
   {
      cTable::cIter& iter = *(cTable::cIter*)piter; 
      if (iter.Done())
         return FALSE; 
      if (obj)
         *obj = iter.Key(); 
      if (value)
         *value = iter.Value(); 
      iter.Next(); 
      return TRUE; 
   }

   STDMETHOD(IterStop)(sPropertyObjIter* ) const 
   {
      return S_OK; 
   }

protected: 
   cTable mTable; 

};

//------------------------------------------------------------
// Generic version
//

class cGenericHashPropertyStore : public cHashPropertyStore<cDelegatingDataOps>
{
public:
   cGenericHashPropertyStore()
   {
      mOps.InitDelegation(this); 
   }

   STDMETHOD(SetOps)(IDataOps* ops)
   {  
      mOps.SetOps(ops); 
      return S_OK; 
   }

}; 




#endif // PROPHASH_H

