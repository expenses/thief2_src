// $Header: r:/prj/cam/src/physics/RCS/phlisbas.h 1.1 1999/02/09 09:39:34 JON Exp $

#ifndef __PHLISBAS_H
#define __PHLISBAS_H

#include <types.h>

#include <comtools.h>

#include <phscrpt.h>

//
// Physics listeners get these
//
struct sPhysListenMsg : sPhysMsgBase
{
   ePhysScriptMsgType type;

   // Collision
   sPhysListenMsg(int submod, ePhysCollisionType type, ObjID coll_object, 
      int coll_submod, const mxs_vector &normal, mxs_real momentum, const mxs_vector &pt):
      type(kCollisionMsg),
      sPhysMsgBase(submod, type, coll_object, coll_submod, normal, momentum, pt)
   {
   }
};

#endif