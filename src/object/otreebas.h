// $Header: x:/prj/tech/libsrc/object/objedit/RCS/otreebas.h 1.1 1999/04/07 14:39:40 TOML Exp $
#pragma once  
#ifndef __OTREEBAS_H
#define __OTREEBAS_H

#include <otreetyp.h>

//
// Object Tree Editor Base Types
//

enum eObjTreeFlags
{
   kObjTreePermitConcretes    = 1 << 0,

   kObjTreeNoAddButton        = 1 << 1, 
   kObjTreeNoDelButton        = 1 << 2, 
   kObjTreeNoCreateButton     = 1 << 3,

   kObjTreeNoLoadButton       = 1 << 4,
   kObjTreeNoSaveButton       = 1 << 5,
   kObjTreeNoSaveLoadButtons  = kObjTreeNoLoadButton | kObjTreeNoSaveButton,
}; 

struct sObjTreeEditorDesc 
{
   char title[32];  // window title (optional)
   ulong flags; 
   sObjTreeDesc* trees; // Null terminated array of sObjTreeDescs
};

enum eTreeCBResult  // a bitmask for callback results
{
   kObjTreeIgnore = 0,        // Move along, nothing to see here
   kObjTreeCancel = 1 << 3,   // Cancel the effect of the button  
   kObjTreeNormal = 1 << 0,   // perform the default operation for the button
   kObjTreeChanged= 1 << 1,   // I did something that should impact the tree
   kObjTreeQuit   = 1 << 2,   // quit the tree editor 
}; 

struct sObjTreeDesc
{
   char treename[32];  // human-readable name of tree
   char rootname[16];  // obj name of root
   ulong flags;

   struct sObjTreeFuncs 
   { 
      // "add button" method, return true for success
      eTreeCBResult (LGAPI * add)(ObjID obj, const char* name); 
      // "del button" callback, return true to let obj be deleted normally
      eTreeCBResult (LGAPI * del)(ObjID obj); 
      // "create button" callback 
      eTreeCBResult (LGAPI * create)(ObjID obj);
      // "edit button" callback
      eTreeCBResult (LGAPI * edit)(ObjID obj);
      // "load button" callback
      eTreeCBResult (LGAPI * load)();
      // "save button" callback
      eTreeCBResult (LGAPI * save)();
   } func;
};

// The correct terminator for a treedesc list
#define NULL_TREEDESC { "" } 

#endif // __OTREEBAS_H

