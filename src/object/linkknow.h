// $Header: x:/prj/tech/libsrc/object/link/RCS/linkknow.h 1.1 1999/04/07 14:38:21 TOML Exp $

#ifndef __LINKKNOW_H
#define __LINKKNOW_H
#include <linktype.h>

////////////////////////////////////////////////////////////
// cLinkManagerKnower
//
// Cheesy little class for relations and other things that
// want a quick easy pointer to the link manager.  
// Not for external use. 
//

class cLinkManagerKnower
{
   static ILinkManagerInternal *linkMan;

protected:
   static void SetManager(ILinkManagerInternal* m) { linkMan = m; } ;
   static ILinkManagerInternal* LinkMan(void) { return linkMan;}; 
};

#endif // __LINKKNOW_H


