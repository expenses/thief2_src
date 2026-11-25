#include <resstat.h>
#include <mprintf.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cNamedStatsData
//

cNamedStatsData::cNamedStatsData(const char* pName)
{
	if (pName)
	{
#line 25
		m_pObjName = static_cast<char*>(operator new(strlen(pName) + 1));
		strcpy(m_pObjName, pName);
	}
	else
		m_pObjName = NULL;
	memset(m_Stats, 0, sizeof(m_Stats));
}

///////////////////////////////////////

cNamedStatsData::~cNamedStatsData()
{
	if (m_pObjName)
	{
		operator delete(m_pObjName);
		m_pObjName = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cNamedStatsHash
//

cNamedStatsHash::cNamedStatsHash()
	: cStrIHashSet<cNamedStatsData*>(500)
{
}

///////////////////////////////////////

cNamedStatsHash::~cNamedStatsHash()
{
	DestroyAll();
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cResStats
//
cResStats::cResStats()
	: m_pResStats(NULL),
	m_pTypeStats(NULL),
	m_pPathStats(NULL)
{
	memset(m_Stats, 0, sizeof(m_Stats));
	for (int i = 0; i < kResStatMode_NumModes; ++i)
		m_StatMode[i] = 0;
}
///////////////////////////////////////
cResStats::~cResStats()
{
	if (m_pResStats)
	{



		delete m_pResStats;
		m_pResStats = NULL;
	}
	if (m_pTypeStats)
	{

		delete m_pTypeStats;
		m_pTypeStats = NULL;
	}
	if (m_pPathStats)
	{

		delete m_pPathStats;
		m_pPathStats = NULL;
	}
}
///////////////////////////////////////
void cResStats::SetMode(eResStatMode ResStatMode, int bTurnOn)
{

#line 97
	AssertMsg(ResStatMode < kResStatMode_NumModes, "Unknown Stat Mode!");

	if (ResStatMode == kResStatMode_All)
	{
		for (int i = 0; i < kResStatMode_NumModes; ++i)
			m_StatMode[i] = bTurnOn;
	}
	else
		m_StatMode[ResStatMode] = bTurnOn;

	if (m_StatMode[kResStatMode_Types] && !m_pTypeStats)




		m_pTypeStats = new cNamedStatsHash;

	if (m_StatMode[kResStatMode_CanonPath] && !m_pPathStats)
		m_pPathStats = new cNamedStatsHash;

	if ((m_StatMode[kResStatMode_Res] || m_StatMode[kResStatMode_TopHits]) && !m_pResStats)


		m_pResStats = new cNamedStatsHash;
}

///////////////////////////////////////





void cResStats::DumpItem(FILE* fp, ulong* pStats)
{
	static const char* pStatNames[NumStats] =
	{
		"Lock Requests",
		"Already Loaded",
		"Lock Failed",
		"Load Failed",
		"New Load no LRU",
		"New Load from LRU",
		"Resource Bindings",
		"Proxied Loads",
		"Total Memory Allocated",
		"Total Memory Freed",
		"Current Memory at Dump",
		"Memory Highwater",
	};

	for (int i = 0; i < NumStats; ++i)
	{
		if (fp)
			fprintf(fp, "    %s: %d\n", pStatNames[i], pStats[i]);
		else
			mprintf("    %s: %d\n", pStatNames[i], pStats[i]);
	}
}

///////////////////////////////////////

void cResStats::DumpTable(FILE* fp, cNamedStatsHash* pTable)
{
	if (!pTable)
		return;

	tHashSetHandle hs;
	for (cNamedStatsData* pNode = pTable->GetFirst(hs); pNode != NULL; pNode = pTable->GetNext(hs))
	{
		if (fp)
			fprintf(fp, "  %s:\n", pNode->m_pObjName);
		else
			mprintf("  %s:\n", pNode->m_pObjName);
		DumpItem(fp, pNode->m_Stats);
	}
}

void cResStats::Dump(const char* pFile)
{
	FILE* fp = NULL;
	if (pFile)
		fp = fopen(pFile, "a+");

	if (m_StatMode[kResStatMode_Summary])
	{
		if (fp)
			fprintf(fp, "\nResource Manager Stats:\n");
		else
			mprintf("\nResource Manager Stats:\n");

		DumpItem(fp, m_Stats);
	}

	if (m_StatMode[kResStatMode_Types])
	{
		if (fp)
			fprintf(fp, "\nStats Broken down by Type:\n");
		else
			mprintf("\nStats Broken down by Type:\n");

		DumpTable(fp, m_pTypeStats);
	}

	if (m_StatMode[kResStatMode_CanonPath])
	{
		if (fp)
			fprintf(fp, "\nStats Broken down by Canonical Path:\n");
		else
			mprintf("\nStats Broken down by Canonical Path:\n");

		DumpTable(fp, m_pPathStats);
	}

	if (m_StatMode[kResStatMode_Res])
	{
		if (fp)
			fprintf(fp, "\nStats Broken down by Resource:\n");
		else
			mprintf("\nStats Broken down by Resource:\n");

		DumpTable(fp, m_pResStats);
	}

	if (fp)
		fclose(fp);
}

///////////////////////////////////////

void cResStats::LogStat(IRes* pRes, ulong* pStats, eResourceStats statType)
{
	if (statType == MemAlloced)
	{
		long size = pRes->GetSize();
		pStats[MemAlloced] += size;
		pStats[MemCurrent] += size;
		if (pStats[MemCurrent] > pStats[MemHighwater])
			pStats[MemHighwater] = pStats[MemCurrent];
	}
	else if (statType == MemFreed)
	{
		long size = pRes->GetSize();
		pStats[MemFreed] += size;
		pStats[MemCurrent] -= size;
	}
	else
	{
		++pStats[statType];
	}
}

///////////////////////////////////////

void cResStats::LogStatTable(IRes* pRes, cNamedStatsHash* pTable, const char* pName, eResourceStats StatType)
{
	if (!pTable)

		CriticalMsg("cResStat: LogStatTable called without a table!");

	if (!pName)
		return;

	cNamedStatsData* pData = pTable->Search(pName);
	if (!pData)
	{
		pData = new cNamedStatsData(pName);
		pTable->Insert(pData);
	}

	LogStat(pRes, pData->m_Stats, StatType);
}

void cResStats::LogStatRes(IRes* pRes, eResourceStats StatType)
{
	if (!pRes)
		return;

	if (StatType >= NumStats)
		CriticalMsg("Unknown Stat type!");
	if (m_StatMode[kResStatMode_Summary])
		LogStat(pRes, m_Stats, StatType);
	if (m_StatMode[kResStatMode_Types])
	{
		IResType* pType = pRes->GetType();
		const char* pTypename = pType->GetName();
		LogStatTable(pRes, m_pTypeStats, pTypename, StatType);
		pType->Release();
	}
	if (m_StatMode[kResStatMode_CanonPath])
	{
		char* pCanonPath;
		pRes->GetCanonPath(&pCanonPath);
		LogStatTable(pRes, m_pPathStats, pCanonPath, StatType);

		operator delete(pCanonPath);
	}

	if (m_StatMode[kResStatMode_Res] || m_StatMode[kResStatMode_TopHits])
	{
		char* pFullPathName;
		pRes->GetStreamName(TRUE, &pFullPathName);
		LogStatTable(pRes, m_pResStats, pFullPathName, StatType);
		operator delete(pFullPathName);
	}
}
