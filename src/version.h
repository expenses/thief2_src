// $Header: r:/prj/cam/src/RCS/version.h 1.4 1998/07/24 12:34:12 TOML Exp $

#ifndef __VERSION_H
#define __VERSION_H
#include <vernum.h>

typedef struct _Version
{
   char phase[16]; // e.g. "pre-alpha" "alpha" "beta" "final"
   VersionNum num; // version number
   char suffix[32]; // e.g. "german" "oem" etc.
} Version;


// App version accessors
EXTERN const Version* AppVersion(void);
EXTERN const char* VersionString(void); // e.g. "pre-Alpha 0.05G"

// convert a version to string.
EXTERN const char* Version2String(const Version*);  
// compare versions for equality
EXTERN bool VersionsEqual(const Version*, const Version*);


#endif // __VERSION_H
