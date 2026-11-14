#include <lg.h>
#include <nrezseg.h>
#include <resapi.h>

cNRezSegment::cNRezSegment()
{
}

cNRezSegment::~cNRezSegment()
{
}

void cNRezSegment::Init(IRes *pIRes, unsigned long offset, sSndAttribs *pAttribs, int doDouble)
{
   (void)pIRes;
   (void)offset;
   (void)pAttribs;
   (void)doDouble;
}

void *cNRezSegment::GetRawData(void *pDst, unsigned long nBytes)
{
   mpIRes->ExtractPartial(mSrcOffset, mSrcOffset - 1 + nBytes, pDst);
   return pDst;
}
