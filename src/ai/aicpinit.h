///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/ai/RCS/aicpinit.h 1.2 1998/07/15 18:12:17 TOML Exp $
//
//
//

#ifndef __AICPINIT_H
#define __AICPINIT_H

#include <comtools.h>

F_DECLARE_INTERFACE(IAIManager);

BOOL AIInitStandardComponents(IAIManager * pManager);
BOOL AITermStandardComponents();

#endif /* !__AICPINIT_H */
