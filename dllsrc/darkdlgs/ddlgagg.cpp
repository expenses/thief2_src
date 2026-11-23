// ddlgagg.cpp
//
// The editor hands us its application aggregate via DarkDlgsSetAggregate();
// _AppGetAggregated() is what AppGetObj()/AutoAppIPtr() resolve to in this DLL.
//

#include "stdafx.h"
#include <comtools.h>
#include <appagg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static IUnknown* g_pAggregate = NULL;

IUnknown* LGAPI _AppGetAggregated(REFIID iid)
{
   IUnknown* pReturn;
   if (g_pAggregate != NULL && g_pAggregate->QueryInterface(iid,(void**)&pReturn) == S_OK)
      return pReturn;
   return NULL;
}

extern "C" __declspec(dllexport) void LGAPI DarkDlgsSetAggregate(IUnknown* pAggregate)
{
   g_pAggregate = pAggregate; 
}
