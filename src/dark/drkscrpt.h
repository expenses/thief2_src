///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/dark/RCS/drkscrpt.h 1.27 1998/10/20 21:16:59 CCAROLLO Exp $
//
// Dark engine specific script stuff
//

#ifndef __DRKSCRPT_H
#define __DRKSCRPT_H

#include <scrptapi.h>
#include <scrptsrv.h>

#include <engscrpt.h>
#include <damgscrp.h>
#include <damgscrs.h>
#include <pickscrs.h>
#include <repscrpt.h>
#include <diffscrm.h>
#include <drkuiscr.h>
#include <drkscrs.h>

///////////////////////////////////////////////////////////////////////////////
//
// SERVICES
//

///////////////////////////////////////////////////////////////////////////////
//
// MESSAGES
//


///////////////////////////////////////////////////////////////////////////////
//
// Base scripts
//

#ifdef SCRIPT

///////////////////////////////////////
//
// SCRIPT: RootScript
//

BEGIN_SCRIPT(RootScript, EngineRootScript)

   MESSAGE_DEFS:
      DAMAGE_MESSAGE_DEFAULTS()
      REPORT_MESSAGE_DEFAULTS()
      DIFFICULTY_MESSAGE_DEFAULTS()
     
   SCRIPT_BEGIN_MESSAGE_MAP()
      DAMAGE_MESSAGE_MAP_ENTRIES()
      REPORT_MESSAGE_MAP_ENTRIES()
      DIFFICULTY_MESSAGE_MAP_ENTRIES()
   SCRIPT_END_MESSAGE_MAP()


END_SCRIPT(RootScript)

#endif

///////////////////////////////////////////////////////////////////////////////

#include <aiscrpt.h>
#include <rooscrpt.h>
#include <drscrpt.h>
#include <weapscpt.h>
#include <bowscrpt.h>
#include <plyscrpt.h>
#include <phmtscpt.h>
#include <phppscpt.h>
#include <phscrpt.h>
#include <drkinvsc.h>
#include <drkpwscr.h>
#include <pgrpscpt.h>
#include <atkscrpt.h>

#endif /* !__DRKSCRPT_H */

