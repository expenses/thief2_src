// $Header: r:/prj/cam/src/ai/RCS/aiokbits.h 1.1 1998/04/30 11:27:14 MAT Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   aiokbits.h

   okBits tell you whether a given AI can follow a given link in the
   pathfinding database.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _AIOKBITS_H_
#define _AIOKBITS_H_

// As of 4/98 each path link has a uchar for okBits.  If this grows
// be sure to update kAIOK_All.

#define kAIOK_Walk      1
#define kAIOK_Fly       2
#define kAIOK_Swim      4
#define kAIOK_Climb     8
#define kAIOK_Hoist     16
#define kAIOK_JumpDown  32

// These are installed by the app at runtime and evaluated through
// callbacks.  Which makes them much more expensive than the others.
#define kAIOK_App1      64
#define kAIOK_App2      128

#define kAIOK_All       0xff
#define kAIOK_Normal    0x3f    // all but App1 & App2
#define kAIOK_AppAll    (kAIOK_App1 | kAIOK_App2)

#endif // ~_AIOKBITS_H_
