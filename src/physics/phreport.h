///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/physics/RCS/phreport.h 1.1 1998/08/20 19:17:16 CCAROLLO Exp $
//
// Physics report generation header
//

#ifndef __PHREPORT_H
#define __PHREPORT_H

#ifdef REPORT

EXTERN void InitPhysReports();
EXTERN void TermPhysReports();

#else

#define InitPhysReports()
#define TermPhysReports()

#endif

#endif
