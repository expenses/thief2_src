///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/ai/RCS/aisched.h 1.1 1999/04/09 11:30:50 TOML Exp $
//
//
//

#ifndef __AISCHED_H
#define __AISCHED_H

struct sAIScheduleSettings
{
   BOOL fActive;
   long budget;
};

extern sAIScheduleSettings g_AIScheduleSettings;

#endif /* !__AISCHED_H */
