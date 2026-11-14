#include "vernum.h"

#include <cstdio>

static char vbuf[32];

const char* VersionNum2String(const VersionNum* v) // reversed
{
    sprintf(vbuf, "%d.%02d", v->major, v->minor);
    
    return vbuf;
}

int VersionNumsCompare(const VersionNum* v1, const VersionNum* v2) // reversed
{
    int diff = v1->major - v2->major;
    if (diff != 0)
        return diff;

    diff = v1->minor - v2->minor;
    if (diff != 0)
        return diff;

    return 0;
}
