// $Header: r:/prj/cam/src/RCS/vismsg.h 1.1 1997/01/08 22:51:50 mahk Exp $

#ifndef VISMSG_H
#define VISMSG_H

#include <looptype.h>


////////////////////////////////////////////////////////////
//
// VISUAL DISPATCH MESSAGES
//

#define kMsgVisual   kMsgApp1  // see the botton of dispatch.cc 

//
// Visual message subtypes
//

typedef enum msgVisualKind
{
   kPaletteChange,
   kNumVisualKinds,
} msgVisualKind;


#endif // VISMSG_H
