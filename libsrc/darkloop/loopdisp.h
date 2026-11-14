#pragma once

#include <loopapi.h>
#include <loopque.h>
#include <pintarr.h>
#include <timings.h>

struct sClientInfo
{
	sPriIntInfo<ILoopClient> priIntInfo;
	ulong interests;
#ifndef SHIP
	cAverageTimer timer;
#endif
};

struct cClientInfoList
{
	~cClientInfoList()
	{
		for (int i = 0; i < Size(); ++i)
			delete list[i];
	}

	int Append(const struct sClientInfo& info) { return list.Append(new sClientInfo(info)); }
	sClientInfo& operator[](int i) { return *list[i]; }
	int Size() { return list.Size(); }

private:
	cDynArray<sClientInfo*> list;
};

struct ConstraintTable
{
	enum { kConstraintTableSize = 32 };

	cDynArray<sAbsoluteConstraint>& operator[](int idx) { return table[idx]; }

private:
	cDynArray<sAbsoluteConstraint> table[kConstraintTableSize];
};

class cLoopDispatch : public ILoopDispatch
{
	DECLARE_UNAGGREGATABLE();
public:
	cLoopDispatch(ILoopMode* mode, sLoopModeInitParmList paramList, tLoopMessageSet messageSet);
	virtual ~cLoopDispatch();

private:
	// Send a message down/up the dispatch chain
	STDMETHOD(SendMessage)(eLoopMessage message, tLoopMessageData hData, int flags);

	// Send a message forward down the dispatch chain with no data
	STDMETHOD(SendSimpleMessage)(eLoopMessage message);

	// Post a message to be sent down/up the dispatch chain
	// the next time ProcessQueue() is called
	STDMETHOD(PostMessage)(eLoopMessage message, tLoopMessageData hData, int flags);

	// Post a message to be sent forward down the dispatch chain with no data
	// the next time ProcessQueue() is called
	STDMETHOD(PostSimpleMessage)(eLoopMessage message);

	// Send all posted messages in queue
	STDMETHOD(ProcessQueue)();

	// Describe my instantiaion
	STDMETHOD_(const sLoopModeName*, Describe)(sLoopModeInitParmList* list);

#ifndef SHIP
	// Set/Get the diagnostic mode
	STDMETHOD_(void, SetDiagnostics)(unsigned fDiagnostics, tLoopMessageSet messages);
	STDMETHOD_(void, GetDiagnostics)(unsigned* pfDiagnostics, tLoopMessageSet* pMessages);

	// Set messages and optional client to use the profileable dispatcher
	STDMETHOD_(void, SetProfile)(tLoopMessageSet messages, tLoopClientID*);
	STDMETHOD_(void, GetProfile)(tLoopMessageSet* pMessages, tLoopClientID**);

	STDMETHOD_(void, ClearTimers)();
	STDMETHOD_(void, DumpTimerInfo)();
#endif

private:
	bool DispatchNormalFrame(ILoopClient*, tLoopMessageData hData);
	void AddClientsFromMode(ILoopMode*, ConstraintTable& table);
	void SortClients(ConstraintTable& table);

private:
	ulong m_msgs;
	cLoopQueue m_Queue;

	cPriIntArray<ILoopClient> m_DispatchLists[ConstraintTable::kConstraintTableSize];
	ILoopMode* m_pLoopMode;
	cClientInfoList m_aClientInfo;

#ifndef SHIP
	uint m_fDiagnostics;
	tLoopMessageSet m_diagnosticSet;
	tLoopMessageSet m_ProfileSet;
	tLoopClientID* m_pProfileClientId;
	cAverageTimer m_AverageFrameTimer;

	cSimpleTimer m_TotalModeTime;
#endif

	class cInitParmTable
	{
		sLoopModeInitParmList copy_parm(sLoopModeInitParmList list)
		{
			if (!list)
				return NULL;

			int n;
			for (n = 0; list[n].pID; n++)
				;

			sLoopModeInitParm* result = new sLoopModeInitParm[n + 1];
			memcpy(result, list, sizeof(sLoopModeInitParm) * (n + 1));

			return result;
		}

	public:
		cInitParmTable(sLoopModeInitParmList list)
			: m_list(copy_parm(list)) {}

		~cInitParmTable() { delete[] m_list; }

		sLoopModeInitParm* Search(const GUID* pID)
		{
			sLoopModeInitParm* p = m_list;
			if (!p)
				return NULL;

			while (p->pID && *p->pID != *pID)
				p++;

			if (!p->pID)
				return NULL;
			return p;
		}

		sLoopModeInitParmList List() { return m_list; }

	private:
		sLoopModeInitParmList m_list;
	};

	cInitParmTable m_Parms;
};
