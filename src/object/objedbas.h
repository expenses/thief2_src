// $Header: x:/prj/tech/libsrc/object/objedit/RCS/objedbas.h 1.1 1999/04/07 14:39:40 TOML Exp $
#pragma once  
#ifndef __OBJEDBAS_H
#define __OBJEDBAS_H

#include <objedtyp.h>
//------------------------------------------------------------
// OBJECT EDITING BASE TYPES
//

//
// Editor Flags
// 

enum eObjEditorFlags
{
   kObjEdShowAll     = 1 << 0,   // Show all traits, including invisible ones

};

//
// Editor descriptor structure
//

struct sObjEditorDesc 
{
   char title[32]; // window title
   ulong flags;
};

//
// Editor event kind
//

enum eObjEditEvent_
{
   kObjEdDone,
   kObjEdCancel,
};

typedef ulong eObjEditEvent;

//
// Event structure
//

struct sObjEditorEvent
{
   eObjEditEvent kind; 
   IObjEditor* editor;    
};

//
// Trait iterator
//
struct sEditTraitIter 
{
   int idx; 
}; 


#endif // __OBJEDBAS_H



