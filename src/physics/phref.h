////////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/physics/RCS/phref.h 1.8 1998/07/08 18:10:55 mahk Exp $
//
// Physics ref system header
//

#ifndef __PHREF_H
#define __PHREF_H

#include <refsys.h>
#include <objtype.h>

#ifdef __cplusplus
class cPhysModel;
#else
typedef int cPhysModel;
#endif


typedef ObjRefID tPhysRef;

EXTERN int  PhysRefSystemInit();
EXTERN void PhysRefSystemClear(); 
EXTERN void PhysRefSystemResize();
EXTERN void PhysRefSystemRebuild(); 
EXTERN void PhysRefSystemTerm();

EXTERN int PhysUpdateRefs(cPhysModel *pModel);
EXTERN int PhysUpdateRefsObj(ObjID objID);

EXTERN int PhysRefSystemID;

#endif // __PHREF_H
