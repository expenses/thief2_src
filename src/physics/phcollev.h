// $Header: r:/prj/cam/src/physics/RCS/phcollev.h 1.2 1998/07/29 08:26:43 CCAROLLO Exp $
#pragma once  
#ifndef __PHCOLLEV_H
#define __PHCOLLEV_H

#include <chainevt.h>
#include <chevkind.h>
#include <phclsn.h>

#include <mprintf.h>

//
// A phys collision chained event.  This is in no way a flat
// structure, and thus can't be deep-copied correctly.  So all uses of
// this event have to be on the stack, which is perilous.  
//

struct sPhysClsnEvent : public sChainedEvent
{
   cPhysClsn *collision; 

   sPhysClsnEvent(cPhysClsn* coll, sChainedEvent *ev = NULL)
      : sChainedEvent(kEventKindCollision, sizeof(*this), ev)
   {
      collision = new cPhysClsn(coll);
   } 

   ~sPhysClsnEvent()
   {
      delete collision;
   }
}; 

#endif // __PHCOLLEV_H
