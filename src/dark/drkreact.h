// $Header: r:/prj/cam/src/dark/RCS/drkreact.h 1.2 1997/10/13 20:31:43 mahk Exp $

#ifndef __DRKREACT_H
#define __DRKREACT_H

////////////////////////////////////////////////////////////
// ACT/REACT REACTIONS FOR DARK
//

#define REACTION_SET_MODEL "set_model"
#define REACTION_LIGHT_OFF "light_off"
#define REACTION_LIGHT_ON  "light_on"

EXTERN void InitDarkReactions(void);
EXTERN void DarkReactionsPostLoad(void);


#endif // __DRKREACT_H
