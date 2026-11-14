#pragma once

#include <sndseg.h>

struct IRes;

class cNRezSegment : public cSndSegment
{
public:
   cNRezSegment();
   virtual ~cNRezSegment();

   void Init(IRes *pIRes, unsigned long offset, sSndAttribs *pAttribs, int doDouble);

   virtual void *GetRawData(void *pDst, unsigned long nBytes);

   IRes *mpIRes;
};
