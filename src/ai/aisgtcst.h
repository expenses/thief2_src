//////////////////////////////////////////////////////////////////////////////
//$Header: r:/prj/cam/src/ai/RCS/aisgtcst.h 1.4 1998/10/25 01:45:40 TOML Exp $
//
// line-of-sight check from pStartLoc to pEndLoc, accounting for those
// doors which are set to block visibility
//
// We actually cull our list of doors, using only those within a fixed
// radius of pStartLoc.
//

#ifndef __AISGTCST_H
#define __AISGTCST_H

struct Location;

void AIInitSightCast();
void AITermSightCast();

BOOL AISightCast(Location *pStartLoc, Location *pEndLoc);

// preperatory func, on entering sim: gather ObjIDs of all objects
// doors want to avoid seeing through
void AISightCastCollectRefs();

#endif /* !__AISGTCST_H */
