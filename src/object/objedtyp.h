// $Header: x:/prj/tech/libsrc/object/objedit/RCS/objedtyp.h 1.1 1999/04/07 14:39:40 TOML Exp $
#pragma once  
#ifndef __OBJEDTYP_H
#define __OBJEDTYP_H

#include <comtools.h>
F_DECLARE_INTERFACE(IObjEditor);
F_DECLARE_INTERFACE(IObjEditors); 
F_DECLARE_INTERFACE(IEditTrait); 
F_DECLARE_INTERFACE(IProperty); 



//
// Object editor types
//

enum eObjEdModality_
{
   kObjEdModal,
   kObjEdModeless, 
};

typedef ulong eObjEdModality; 

typedef struct sObjEditorDesc sObjEditorDesc;
typedef struct sObjEditorEvent sObjEditorEvent; 

typedef void* ObjEditCBData;
typedef void (LGAPI* ObjEditCB)(sObjEditorEvent* event, ObjEditCBData data);

typedef struct sEditTraitIter sEditTraitIter; 

typedef IEditTrait* (LGAPI* EditTraitPropFactory)(IProperty* prop); 


#endif // __OBJEDTYP_H

