// $Header: x:/prj/tech/libsrc/object/property/RCS/propfac_.h 1.4 1999/04/22 14:54:34 JAEMZ Exp $
#pragma once  
#ifndef __PROPFAC__H
#define __PROPFAC__H

#include <propface.h>
#include <propert_.h>
#include <dataops_.h>
#include <matrixs.h>
#include <matrixds.h>
#include <mxang.h>
#include <srgba.h>
#include <propbool.h>

////////////////////////////////////////////////////////////
// 
// IMPLEMENTATION CLASSES FOR STANDARD PROPERTY INTERFACES 
//

class cGenPropStorePtr : public cAutoIPtr<IPropertyStore>
{
public:
   cGenPropStorePtr(ePropertyImpl impl) : 
      cAutoIPtr<IPropertyStore> (CreateGenericPropertyStore(impl))
   {
   }
};

#define GENSTORE(x) cGenPropStorePtr(x)

//
// BOOL
//

class cGenericBoolProperty : public cGenericProperty<IBoolProperty,&IID_IBoolProperty,BOOL>
{
   typedef cGenericProperty<IBoolProperty,&IID_IBoolProperty,BOOL> cParent; 

   static cSimpleDataOps gOps; 

public:
   cGenericBoolProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cParent(desc,store)
   {
      SetOps(&gOps); 
   }

   cGenericBoolProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cParent(desc,GENSTORE(impl))
   {
      SetOps(&gOps); 
   }

   STANDARD_DESCRIBE_TYPE(BOOL); 
};

// "Non-generic" version that uses the cBoolPropertyStore

class cBoolProperty : public cSpecificProperty<IBoolProperty,&IID_IBoolProperty,BOOL,cBoolPropertyStore>
{
   typedef cSpecificProperty<IBoolProperty,&IID_IBoolProperty,BOOL,cBoolPropertyStore> cParent; 
public:
   cBoolProperty(const sPropertyDesc* desc)
      : cParent(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(BOOL); 

      
}; 


//
// INT
//

class cGenericIntProperty : public cGenericProperty<IIntProperty,&IID_IIntProperty,int>
{
   typedef cGenericProperty<IIntProperty,&IID_IIntProperty,int> cParent; 

   static cSimpleDataOps gOps; 
public:

   cGenericIntProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cParent(desc,store)
   {
      SetOps(&gOps); 
   }

   cGenericIntProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cParent(desc,GENSTORE(impl))
   {
      SetOps(&gOps); 
   }

   STANDARD_DESCRIBE_TYPE(int); 
};

//
// FLOAT
//

class cGenericFloatProperty : public cGenericProperty<IFloatProperty,&IID_IFloatProperty,float>
{
   typedef cGenericProperty<IFloatProperty,&IID_IFloatProperty,float> cParent; 

   static cSimpleDataOps gOps; 

public:
   cGenericFloatProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cParent(desc,store)
   {
      SetOps(&gOps); 
   }

   cGenericFloatProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cParent(desc,GENSTORE(impl))
   {
      SetOps(&gOps); 
   }

   STANDARD_DESCRIBE_TYPE(float); 
};



//
// Label
//

class cGenericLabelProperty : public cGenericProperty<ILabelProperty,&IID_ILabelProperty,Label*>
{
   typedef cGenericProperty<ILabelProperty,&IID_ILabelProperty,Label*> cParent; 

   static cClassDataOps<Label> gOps; 
public:

   cGenericLabelProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cParent(desc,store)
   {
      SetOps(&gOps); 
   }

   cGenericLabelProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cParent(desc,GENSTORE(impl))
   {
      SetOps(&gOps); 
   }

   STANDARD_DESCRIBE_TYPE(Label); 
};


//
// Vector
//

typedef mxs_vector Vector; 

class cGenericVectorProperty : public cGenericProperty<IVectorProperty,&IID_IVectorProperty,Vector*>
{
   typedef cGenericProperty<IVectorProperty,&IID_IVectorProperty,Vector*> cParent; 

   static cClassDataOps<Vector> gOps; 
public:

   cGenericVectorProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cParent(desc,store)
   {
      SetOps(&gOps); 
   }

   cGenericVectorProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cParent(desc,GENSTORE(impl))
   {
      SetOps(&gOps); 
   }

   STANDARD_DESCRIBE_TYPE(Vector); 
};


//
// RGB
//
typedef sRGBA RGB;


class cGenericRGBProperty : public cGenericProperty<IRGBProperty,&IID_IRGBProperty,RGB*>
{
   typedef cGenericProperty<IRGBProperty,&IID_IRGBProperty,RGB*> cParent; 

   static cClassDataOps<RGB> gOps; 
public:

   cGenericRGBProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cParent(desc,store)
   {
      SetOps(&gOps); 
   }

   cGenericRGBProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cParent(desc,GENSTORE(impl))
   {
      SetOps(&gOps); 
   }

   STANDARD_DESCRIBE_TYPE(RGB); 
};


//
// DoubleVec
//

typedef mxds_vector DoubleVec; 

class cGenericDoubleVecProperty : public cGenericProperty<IDoubleVecProperty,&IID_IDoubleVecProperty,DoubleVec*>
{
   typedef cGenericProperty<IDoubleVecProperty,&IID_IDoubleVecProperty,DoubleVec*> cParent; 

   static cClassDataOps<DoubleVec> gOps; 
public:

   cGenericDoubleVecProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cParent(desc,store)
   {
      SetOps(&gOps); 
   }

   cGenericDoubleVecProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cParent(desc,GENSTORE(impl))
   {
      SetOps(&gOps); 
   }

   STANDARD_DESCRIBE_TYPE(DoubleVec); 
};


//
// AngVec property
// 

typedef mxs_angvec AngVec; 

class cGenericAngVecProperty : public cGenericProperty<IAngVecProperty,&IID_IAngVecProperty,AngVec*>
{
   typedef cGenericProperty<IAngVecProperty,&IID_IAngVecProperty,AngVec*> cParent; 

   static cClassDataOps<AngVec> gOps; 
public:

   cGenericAngVecProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cParent(desc,store)
   {
      SetOps(&gOps); 
   }

   cGenericAngVecProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cParent(desc,GENSTORE(impl))
   {
      SetOps(&gOps); 
   }

   STANDARD_DESCRIBE_TYPE(AngVec); 
};

#undef GENSTORE

#endif // __PROPFAC__H
