// $Header: r:/prj/cam/src/RCS/_animtxt.h 1.2 1998/08/14 00:39:30 dc Exp $
//
// Internals for anim_txt.c
// Published here just so resapp.h can see the structure definition...

#ifndef ___ANIMTXT_H
#define ___ANIMTXT_H

#define MAX_FRAMES 20

typedef struct {
   void  *animsys;
   IRes  *frame_hnd[MAX_FRAMES];
   short flags;
   short cnt;
   int   lock_cnt;
   int   cur;
   int   txt_id;
   int   last_time;
   int   rate;
} ectsAnimInfo;

#endif // ___ANIMTXT_H
