#pragma once
#ifndef __QUESBIN_H
#define __QUESBIN_H

#include <questapi.h>
#include <hashset.h>
#include <str.h>

class cNetMsg;

#define kQuestBinMaxSize 0xfde8

class cQuestBinDataNode
{
public:
   cStr m_name;
   int m_size;
   void *m_pData;

   cQuestBinDataNode(const char *pName) :
      m_name(pName), m_size(0), m_pData(NULL) {};
   ~cQuestBinDataNode() { delete [] (char*)m_pData; };
};

class cQuestBinDataHash : public cStrIHashSet<cQuestBinDataNode*>
{
   virtual tHashSetKey GetKey(tHashSetNode p) const
   {
      return (tHashSetKey)(const char*)((cQuestBinDataNode*)p)->m_name;
   }
};

class cQuestBinData
{
public:
   STDMETHOD(Init)();
   STDMETHOD(Term)();
   STDMETHOD_(BOOL, IsEmpty)() { return m_hash.GetCount() == 0; }

   STDMETHOD(Create)(const char *pName, const void *pData, int size);
   STDMETHOD(Set)(const char *pName, const void *pData, int size);
   STDMETHOD_(const void*, Get)(const char *pName, int *pSize);
   STDMETHOD_(BOOL, Exists)(const char *pName);
   STDMETHOD(Delete)(const char *pName);
   STDMETHOD(DeleteAll)();

   STDMETHOD(Save)(QuestMoveFunc moveFunc);
   STDMETHOD(Load)(QuestMoveFunc moveFunc);

   STDMETHOD(doCreate)(const char *pName, void *pData, int size);
   STDMETHOD(doSet)(const char *pName, void *pData, int size);
   STDMETHOD(doDelete)(const char *pName);

#ifndef SHIP
   void Dump();
#endif

private:
   cQuestBinDataHash m_hash;
   cNetMsg *m_pCreateMsg;
   cNetMsg *m_pSetMsg;
   cNetMsg *m_pDeleteMsg;
};

EXTERN cQuestBinData *g_pQuestBinData;

#ifndef SHIP
EXTERN void QuestBinDataDump(void);
#endif

#endif // __QUESBIN_H
