// $Header: r:/prj/cam/src/physics/RCS/phlistyp.h 1.1 1999/02/09 09:40:09 JON Exp $

#ifndef __PHLISTYP_H
#define __PHLISTYP_H

#include <types.h>

#include <objtype.h>

// Handle for listener
typedef int PhysListenerHandle;

// Data For listener function
typedef void *PhysListenerData;

// Our actual message
typedef struct sPhysListenMsg sPhysListenMsg;  

// Message sets
typedef int PhysListenMsgSet;

// Listener function: 
typedef void (LGAPI * PhysListenFunc)(ObjID, const sPhysListenMsg*, PhysListenerData);

#endif