// $Header: r:/prj/cam/src/ai/RCS/aireport.h 1.1 1998/10/17 21:01:58 dc Exp $
// woo woo

#ifndef __AIREPORT_H
#define __AIREPORT_H

#ifdef REPORT
EXTERN void AIReportInit(void);
EXTERN void AIReportTerm(void);
#else
#define AIReportInit()
#define AIReportTerm()
#endif

#endif  // __AIREPORT_H
