////////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/physics/RCS/phmotion.h 1.3 1998/06/16 18:44:54 CCAROLLO Exp $
//
// Physics motion class header
//

#ifndef __PHMOTION_H
#define __PHMOTION_H

#include <matrixs.h>
#include <dynarray.h>

#include <phmoapi.h>

////////////////////////////////////////////////////////////////////////////////

class cPlayerMotion
{
public:
   
   /////////////////////////////////////
   //
   // Constructor / Destructor
   //
   cPlayerMotion();
   ~cPlayerMotion();

   /////////////////////////////////////
   //
   // Active motion accessors
   //
   void          Activate(ePlayerMotion motion);
   ePlayerMotion GetActive();

   void          ActivateList(ePlayerMotion *motions, int size);

   void          SetRest(ePlayerMotion restMotion);
   ePlayerMotion GetRest();

   void SetOffset(short submod, mxs_vector *offset);
   void GetOffset(short submod, mxs_vector *offset);

   /////////////////////////////////////
   //
   // Update
   //
   void Update(mxs_real dt);

private:
   
   ePlayerMotion m_activeMotion;
   mxs_real      m_timeDuration;
   mxs_real      m_timeHoldLength;

   ePlayerMotion m_restMotion;

   cDynArray<ePlayerMotion> m_motionList;

   cDynArray<mxs_vector>   m_curOffset;
   cDynArray<mxs_vector>   m_targOffset;

   mxs_real      m_timeActive;

};

////////////////////////////////////////////////////////////////////////////////

#endif // __PHMOTION_H










