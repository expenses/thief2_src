// $Header: r:/prj/cam/src/dark/RCS/drkgame.h 1.5 1998/11/05 21:09:44 mahk Exp $

#ifndef DRKGAME_H
#define DRKGAME_H

////////////////////////////////////////////////////////////
// DARK GAME CODE
//

EXTERN void dark_sim_update_frame(void);
EXTERN void dark_rend_update_frame(void);
EXTERN void dark_start_gamemode(BOOL resuming);
EXTERN void dark_end_gamemode(BOOL suspending);
EXTERN void dark_init_game(void);
EXTERN void dark_term_game(void);

// this has to be set so that the update_frame can decide what to do with 
//  the currently viewed object and highlighting
EXTERN BOOL world_use_in_progress;

#endif // DRKGAME_H

