#ifndef __PG8RLIST_H
#define __PG8RLIST_H

#include "intsetbx.h"

/////////////////////////////////////////////////////////////////////////////
// cGatorCombo
//
// A combo box of the propagators that support a stimulus, by PropagatorID

class cGatorCombo : public cIntSetCombo
{
public:
   cGatorCombo();
   virtual ~cGatorCombo();

   void Fill(StimID stim); 
};

#endif // __PG8RLIST_H
