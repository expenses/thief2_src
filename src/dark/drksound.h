// $Header: r:/prj/cam/src/dark/RCS/drksound.h 1.2 1998/08/25 23:24:50 dc Exp $
#pragma once
#ifndef __DRKSOUND_H
#define __DRKSOUND_H


////////////////////////////////////////////////////////////
// DARK-SPECFIC SOUND STUFF
//

EXTERN void DarkSoundInit(void); 
EXTERN void DarkSoundTerm(void); 

// run ms of a dark specific sound frame
// really, for now, this is just the "am i stopping to listen" code
EXTERN void DrkSoundFrame(void);

#endif // __DRKSOUND_H
