#ifndef __RESARQ_H
#define __RESARQ_H

#include <pool.h>
#include <hashset.h>
#include <comtools.h>

#include <resapi.h>
#include <arqguid.h>
#include <resmanhs.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cResARQ
//

class cResMan;
class cResManARQ
{
public:
	void Init(cResMan* pManager);
	void Term();
	void ClearPreload(cResourceTypeData* id);
	int Lock(IRes* pRes, int priority);
	int Extract(IRes* pRes, int priority, void* buf, long bufSize);
	int Preload(IRes* pRes);
	int IsFulfilled(IRes* pRes);
	long Kill(IRes* pRes);
	long GetResult(IRes* pRes, void** ppResult);
};
#endif