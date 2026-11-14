#include "resutil.h"

EXTERN long ResBaseExtractCallback(void* pBuf, long nNumBytes, long nIx, void* pUntypedData)
{
	sExtractData* pData = (sExtractData*)pUntypedData;

	return pData->Callback(pData->pResource, pBuf, nNumBytes, nIx, pData->pCallbackData);
}
extern "C" void CanonicalizePathname(char *pPathname)
{
   char *p = pPathname;
   while (*p != '\0') {
      if (*p == '\\') {
         *p = '/';
      }
      p++;
   }
}
