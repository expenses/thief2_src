// $Header: x:/prj/tech/libsrc/object/objedit/RCS/iobjed.h 1.1 1999/04/07 14:39:39 TOML Exp $
#pragma once  
#ifndef __IOBJED_H
#define __IOBJED_H

#include <comtools.h>
#include <objtype.h>
#include <objedtyp.h>
#include <otreetyp.h>

//
// Object Editor Interface
//

#undef INTERFACE
#define INTERFACE IObjEditor
DECLARE_INTERFACE_(IObjEditor,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   //
   // Start the editor.  
   // If Modal, returns TRUE iff the ok button was pushed. 
   // If Modeless, returns FALSE immediately
   //
   STDMETHOD_(BOOL,Go)(THIS_ eObjEdModality modal) PURE;

#define IObjEditor_Go(p, a) COMCall1(p, Go, a) 

   //
   // Set the callback to be called when buttons get pushed
   //
   STDMETHOD(SetCallback)(THIS_ ObjEditCB cb, ObjEditCBData data) PURE;

#define IObjEditor_SetCallback(p, a, b) COMCall2(p, SetCallback, a, b)

   //
   // Accessors
   //
   STDMETHOD_(const sObjEditorDesc*,Describe)(THIS) PURE;
   STDMETHOD_(ObjID,Object)(THIS) PURE; 

#define IObjEditor_Describe(p)         COMCall0(p, Describe)
#define IObjEditor_Object(p)           COMCall0(p, Object)

};

#undef INTERFACE 

// 
// Object Editing Service Agg Member
//

F_DECLARE_INTERFACE(IEditTrait); 
F_DECLARE_INTERFACE(IProperty);
F_DECLARE_INTERFACE(IRelation);

#define INTERFACE IObjEditors
DECLARE_INTERFACE_(IObjEditors,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   //
   // Registry of "traits" that can be edited
   //
   STDMETHOD(AddTrait)(THIS_ IEditTrait* trait) PURE; 
   STDMETHOD(RemoveTrait)(THIS_ IEditTrait* trait) PURE; 

   // Make a trait out of a property, and add it. 
   STDMETHOD_(IEditTrait*, AddProperty)(THIS_ IProperty* prop) PURE; 
   
   // Iteration
   STDMETHOD_(IEditTrait*,FirstTrait)(THIS_ sEditTraitIter* iterstate) PURE;
   STDMETHOD_(IEditTrait*,NextTrait)(THIS_ sEditTraitIter* iterstate) PURE; 

   //
   // Create an object editor
   //
   STDMETHOD_(IObjEditor*,Create)(THIS_ sObjEditorDesc* desc, ObjID obj) PURE;

#define IObjEditors_Create(p, a, b)       COMCall2(p, Create, a, b) 

   // Make a trait out of a relation, and add it.  You probably want to do this
   // With the relation's inverse, as well 
   STDMETHOD_(IEditTrait*, AddRelation)(THIS_ IRelation* rel) PURE;

   // Register a factory that makes edittraits from properties that 
   // conform to a particular interface.  It will be used by 
   // AddProperty.
   STDMETHOD(AddPropertyTraitFactory)(THIS_ REFIID iid, EditTraitPropFactory factory) PURE; 

   // Edit an object hierarchy in a modal dialog
   // optionally, try to start focused on an object 
   STDMETHOD(EditObjTree)(THIS_ const sObjTreeEditorDesc* desc, ObjID obj DEFAULT_TO(0)) PURE; 

}; 

#undef INTERFACE 

// create the IObjEditors for an editor exe
EXTERN void ObjEditorsRealCreate(void); 
// create an empty IObjEditors
EXTERN void ObjEditorsNullCreate(void); 

#ifdef EDITOR
#define ObjEditorsCreate ObjEditorsRealCreate
#else
#define ObjEditorsCreate ObjEditorsNullCreate
#endif 


#endif // __IOBJED_H




