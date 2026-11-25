/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/quest.cpp,v 1.14 1999/08/31 17:42:29 Justin Exp $

#include <questapi.h>
#include <quesscpt.h>

#include <scrptapi.h>

#include <hashset.h>
#include <hshsttem.h>

#include <hashpp.h>
#include <hshpptem.h>

#include <dlist.h>

#include <appagg.h>
#include <aggmemb.h>

#include <creatext.h>

#include <iobjsys.h>
#include <objtype.h>
#include <objdef.h>
#include <objnotif.h>
#include <listset.h>
#include <lststtem.h>

#include <str.h>

#include <netmsg.h>
#include <ctype.h>
#include <quesbin.h>

#ifndef SHIP
#include <mprintf.h>
#endif

// Include these absolutely last
#include <dbmem.h>
#include <initguid.h>
#include <questiid.h>

//------------------------------------------------------------
// Quest Data
//

class cObjList: public cSimpleListSet<ObjID>
{
}; 

// List for storing pointers to data nodes for each object that subscribes to data change messages
class cQuestDataNode;

typedef cSimpleDList<cQuestDataNode*> cQuestNodeList;

// Hash set stuff
class cQuestDataNode
{
public:
   cStr m_pName;
   int m_value;
   eQuestDataType m_type;
   cObjList m_subscriberList;

   cQuestDataNode(const char *pName, eQuestDataType type): 
      m_pName(pName), m_type(type), m_value(0) {};

   STDMETHOD(SendMsgs)(int oldValue) const;
};

// The actual data hash
class cQuestDataHash: public cStrIHashSet<cQuestDataNode*>
{
   virtual tHashSetKey GetKey(tHashSetNode p) const
   {
      return (tHashSetKey)(const char*)((cQuestDataNode*)p)->m_pName;
   }
};

// Object->data node hash for msg subscriptions
typedef cHashTableFunctions<ObjID> cObjIDHashFunctions;
typedef cHashTable<ObjID, cQuestNodeList*, cObjIDHashFunctions> cQuestSubscribeHash;

class cQuestData;

//------------------------------------------------------------
// Forward declare the iterator class
//
class cQuestDataIter : public cCTUnaggregated<IQuestDataIter,&IID_IQuestDataIter,kCTU_Default>
{
private:
   tHashSetHandle m_handle;
   cQuestDataNode *m_pNode;
   const cQuestDataHash *m_pHash;
   eQuestDataType m_type;

   void Match(void)
   {
      while ((m_pNode != NULL) && (m_pNode->m_type != m_type))
      {
         m_pNode = m_pHash->GetNext(m_handle);
      }
   }

public:

   cQuestDataIter(const cQuestDataHash *pHash, eQuestDataType type): 
      m_pNode(pHash->GetFirst(m_handle)),
      m_pHash(pHash),
      m_type(type)
   {
       Match();
   }

   STDMETHOD_(BOOL,Done)(void) {return (m_pNode == NULL);}
   STDMETHOD(Next)(void) {if (m_pNode != NULL) m_pNode = m_pHash->GetNext(m_handle); Match(); return S_OK;}

   STDMETHOD_(const char*,Name)() {if (m_pNode != NULL) return (const char*)(m_pNode->m_pName); else return NULL;};
   STDMETHOD_(int,Value)() {if (m_pNode != NULL) return m_pNode->m_value; else return 0;};
};



//------------------------------------------------------------
// Now the real quest data class
//
class cQuestData: public cCTDelegating<IQuestData>,
                  public cCTAggregateMemberControl<kCTU_Default>
{
   static sRelativeConstraint gConstraints[]; 

public:
   cQuestData(IUnknown* pOuter)
      : m_filter(NULL)
   {
      MI_INIT_AGGREGATION_1(pOuter, IQuestData, kPriorityNormal, gConstraints);
   }

   STDMETHOD(Init)()
   {
      sObjListenerDesc objListenerDesc = {&cQuestData::ObjListener,};

      AutoAppIPtr(ObjectSystem);
      m_listenerHandle = pObjectSystem->Listen(&objListenerDesc); 
      
      m_pCreateMsg = new cNetMsg(&gm_CreateMsgDesc, this);
      m_pSetMsg = new cNetMsg(&gm_SetMsgDesc, this);

      m_binData.Init();

      return S_OK; 
   }

   STDMETHOD(End)()
   {
      DeleteAll(); 
      delete m_pCreateMsg;
      delete m_pSetMsg;
      m_binData.Term();
      return S_OK; 
   }

   STDMETHOD(Create)(const char *pName, int value, eQuestDataType type);
   STDMETHOD(Set)(const char *pName, int value);
   STDMETHOD_(int, Get)(const char *pName);
   STDMETHOD_(BOOL, Exists)(const char *pName)
   {
      return (m_nameHash.Search(pName)!=NULL);
   }
   STDMETHOD_(BOOL, ExistsType)(const char *pName, eQuestDataType type)
   {
      cQuestDataNode *pNode = m_nameHash.Search(pName);
      return pNode != NULL && pNode->m_type == type;
   }
   STDMETHOD(Delete)(const char *pName);
   STDMETHOD_(IQuestDataIter*, Iter)(eQuestDataType type)
   {
      return (IQuestDataIter*)(new cQuestDataIter(&m_nameHash, type));
   }
   STDMETHOD(DeleteAll)(void);
   STDMETHOD(DeleteAllType)(eQuestDataType type);

   STDMETHOD_(BOOL, Save)(QuestMoveFunc moveFunc, eQuestDataType type);
   STDMETHOD_(BOOL, Load)(QuestMoveFunc moveFunc, eQuestDataType type);

   STDMETHOD(SubscribeMsg)(ObjID objID, const char *pName, eQuestDataType type);
   STDMETHOD(UnsubscribeMsg)(ObjID objID, const char *pName);

   static void ObjListener(ObjID objID, eObjNotifyMsg msg, void *data);
   STDMETHOD(ObjDeleteListener)(ObjID objID);

   STDMETHOD(UnsubscribeAll)(); 

#ifndef SHIP
   STDMETHOD(ObjSpewListen)(ObjID objID);
#endif

   STDMETHOD(Filter)(QuestFilterFunc filter, void *pClientData);

   // Internal methods that need to get called from network callbacks:
   STDMETHOD(doCreate)(const char *pName, int value, eQuestDataType type);
   STDMETHOD(doSet)(const char *pName, int value);
   STDMETHOD(SendAllMsgs)(const char *pName, int oldValue, int newValue);

private:

   tObjListenerHandle m_listenerHandle;
   cQuestDataHash m_nameHash;
   cQuestSubscribeHash m_subscriberHash;
   QuestFilterFunc m_filter;
   void *m_pClientData;

   // Network messages:
   static sNetMsgDesc gm_CreateMsgDesc;
   cNetMsg *m_pCreateMsg;
   static sNetMsgDesc gm_SetMsgDesc;
   cNetMsg *m_pSetMsg;

   cQuestBinData m_binData;

   cObjList m_allSubscriberList;
};

//////////
//
// Binary quest data
//

cQuestBinData *g_pQuestBinData = NULL;

static void dispatchBinCreate(const char *pName, int size, const void *pData, void *pBinData)
{
   char *pCopy = new char[size];
   memcpy(pCopy, pData, size);
   ((cQuestBinData*)pBinData)->doCreate(pName, pCopy, size);
}

static sNetMsgDesc gBinCreateMsgDesc = {
   kNMF_Broadcast,
   "QuesXCreate",
   "Create QuestEx Data",
   NULL,
   dispatchBinCreate,
   {{kNMPT_String},
    {kNMPT_DynBlock},
    {kNMPT_End}}
};

static void dispatchBinSet(const char *pName, int size, const void *pData, void *pBinData)
{
   char *pCopy = new char[size];
   memcpy(pCopy, pData, size);
   ((cQuestBinData*)pBinData)->doSet(pName, pCopy, size);
}

static sNetMsgDesc gBinSetMsgDesc = {
   kNMF_Broadcast,
   "QuesXSet",
   "Set QuestEx Data",
   NULL,
   dispatchBinSet,
   {{kNMPT_String},
    {kNMPT_DynBlock},
    {kNMPT_End}}
};

static void dispatchBinDelete(const char *pName, void *pBinData)
{
   ((cQuestBinData*)pBinData)->doDelete(pName);
}

static sNetMsgDesc gBinDeleteMsgDesc = {
   kNMF_Broadcast,
   "QuesXDel",
   "Delete QuestEx Data",
   NULL,
   dispatchBinDelete,
   {{kNMPT_String},
    {kNMPT_End}}
};

STDMETHODIMP cQuestBinData::Init()
{
   g_pQuestBinData = this;
   m_pCreateMsg = new cNetMsg(&gBinCreateMsgDesc, this);
   m_pSetMsg = new cNetMsg(&gBinSetMsgDesc, this);
   m_pDeleteMsg = new cNetMsg(&gBinDeleteMsgDesc, this);
   return S_OK;
}

STDMETHODIMP cQuestBinData::Term()
{
   DeleteAll();
   delete m_pCreateMsg;
   delete m_pSetMsg;
   delete m_pDeleteMsg;
   g_pQuestBinData = NULL;
   return S_OK;
}

STDMETHODIMP cQuestBinData::Create(const char *pName, const void *pData, int size)
{
   if (size <= 0 || size > kQuestBinMaxSize || !pData)
      return E_INVALIDARG;

   m_pCreateMsg->Send(OBJ_NULL, pName, size, pData);

   char *pCopy = new char[size];
   memcpy(pCopy, pData, size);
   return doCreate(pName, pCopy, size);
}

STDMETHODIMP cQuestBinData::Set(const char *pName, const void *pData, int size)
{
   if (size <= 0 || size > kQuestBinMaxSize || !pData)
      return E_INVALIDARG;

   m_pSetMsg->Send(OBJ_NULL, pName, size, pData);

   char *pCopy = new char[size];
   memcpy(pCopy, pData, size);
   return doSet(pName, pCopy, size);
}

STDMETHODIMP_(const void*) cQuestBinData::Get(const char *pName, int *pSize)
{
   cQuestBinDataNode *pNode = m_hash.Search(pName);
   if (pNode)
   {
      *pSize = pNode->m_size;
      return pNode->m_pData;
   }
   return NULL;
}

STDMETHODIMP_(BOOL) cQuestBinData::Exists(const char *pName)
{
   return m_hash.Search(pName) != NULL;
}

STDMETHODIMP cQuestBinData::Delete(const char *pName)
{
   cQuestBinDataNode *pNode = m_hash.Search(pName);
   if (!pNode)
      return S_FALSE;

   m_pDeleteMsg->Send(OBJ_NULL, pName);

   m_hash.Remove(pNode);
   delete pNode;
   return S_OK;
}

STDMETHODIMP cQuestBinData::DeleteAll()
{
   m_hash.DestroyAll();
   return S_OK;
}

STDMETHODIMP cQuestBinData::Save(QuestMoveFunc moveFunc)
{
   tHashSetHandle hnd;
   cQuestBinDataNode *pNode;

   for (pNode = m_hash.GetFirst(hnd); pNode != NULL; pNode = m_hash.GetNext(hnd))
   {
      int len = strlen(pNode->m_name) + 1;
      moveFunc(&len, sizeof(int), 1);
      moveFunc((void*)(const char*)pNode->m_name, 1, len);
      moveFunc(&pNode->m_size, sizeof(int), 1);
      moveFunc(pNode->m_pData, 1, pNode->m_size);
   }
   return S_OK;
}

STDMETHODIMP cQuestBinData::Load(QuestMoveFunc moveFunc)
{
   int len;

   while (moveFunc(&len, sizeof(int), 1) == sizeof(int))
   {
      cStr name("", len);
      if (moveFunc((void*)(const char*)name, 1, len) != len)
         return E_FAIL;

      int size;
      if (moveFunc(&size, sizeof(int), 1) != sizeof(int))
         return E_FAIL;
      if (size <= 0 || size > kQuestBinMaxSize)
         return E_FAIL;

      char *pData = new char[size];
      if (moveFunc(pData, 1, size) != size)
         return E_FAIL;

      m_pCreateMsg->Send(OBJ_NULL, (const char*)name, size, pData);
      doCreate(name, pData, size);
   }
   return S_OK;
}

STDMETHODIMP cQuestBinData::doCreate(const char *pName, void *pData, int size)
{
   cQuestBinDataNode *pNode = m_hash.Search(pName);
   if (pNode)
   {
      delete [] (char*)pNode->m_pData;
      pNode->m_pData = pData;
      pNode->m_size = size;
      return S_OK;
   }

   pNode = new cQuestBinDataNode(pName);
   pNode->m_pData = pData;
   pNode->m_size = size;
   m_hash.Insert(pNode);
   return S_OK;
}

STDMETHODIMP cQuestBinData::doSet(const char *pName, void *pData, int size)
{
   cQuestBinDataNode *pNode = m_hash.Search(pName);
   if (!pNode)
      return Create(pName, pData, size);

   delete [] (char*)pNode->m_pData;
   pNode->m_pData = pData;
   pNode->m_size = size;
   return S_OK;
}

STDMETHODIMP cQuestBinData::doDelete(const char *pName)
{
   cQuestBinDataNode *pNode = m_hash.Search(pName);
   if (!pNode)
      return S_FALSE;

   m_hash.Remove(pNode);
   delete pNode;
   return S_OK;
}

#ifndef SHIP
void cQuestBinData::Dump()
{
   if (IsEmpty())
   {
      mprintf("no bin quest data\n");
      return;
   }

   mprintf("\nQuest Bin Store Dump:\n--------------------\n");

   tHashSetHandle hnd;
   cQuestBinDataNode *pNode;
   for (pNode = m_hash.GetFirst(hnd); pNode != NULL; pNode = m_hash.GetNext(hnd))
   {
      mprintf("\"%s\" (%d bytes):\n", (const char*)pNode->m_name, pNode->m_size);

      const uchar *p = (const uchar*)pNode->m_pData;
      const uchar *pEnd = p + pNode->m_size;
      while (p < pEnd)
      {
         int i;
         for (i = 0; i < 16; i++)
         {
            if (p + i < pEnd)
               mprintf(" %02X", p[i]);
            else
               mprintf("   ");
         }
         for (i = 0; i < 16 && p < pEnd; i++, p++)
            mprintf(" %c", isprint(*p) ? *p : '.');
         mprintf("\n");
      }
      mprintf("\n");
   }
}

void QuestBinDataDump(void)
{
   g_pQuestBinData->Dump();
}
#endif

//////////
//
// Network dispatching stuff
//

static void dispatchCreate(const char *pName, 
                           int value, 
                           eQuestDataType type,
                           void *pQuestData)
{
   ((cQuestData *) pQuestData)->doCreate(pName, value, type);
}

sNetMsgDesc cQuestData::gm_CreateMsgDesc = {
   kNMF_Broadcast,
   "QuesCreate",
   "Create Quest Data",
   NULL,
   dispatchCreate,
   {{kNMPT_String},
    {kNMPT_Int},
    {kNMPT_Int},
    {kNMPT_End}}
};

static void dispatchSet(const char *pName, 
                        int value, 
                        void *pQuestData)
{
   ((cQuestData *) pQuestData)->doSet(pName, value);
}

sNetMsgDesc cQuestData::gm_SetMsgDesc = {
   kNMF_Broadcast,
   "QuesSet",
   "Set Quest Data",
   NULL,
   dispatchSet,
   {{kNMPT_String},
    {kNMPT_Int},
    {kNMPT_End}}
};


sRelativeConstraint cQuestData::gConstraints[] = 
{
   { kConstrainAfter, &IID_IObjectSystem }, 
   { kNullConstraint }, 
}; 

STDMETHODIMP cQuestDataNode::SendMsgs(int oldValue) const
{

   AutoAppIPtr(ScriptMan);

   cObjList::cIter iter; 
   for (iter = m_subscriberList.Iter(); !iter.Done(); iter.Next())
   {
      sQuestMsg msg(iter.Value(), m_pName, oldValue, m_value);
      pScriptMan->SendMessage(&msg);
   }
   return S_OK;
}

STDMETHODIMP cQuestData::Create(const char *pName, int value, eQuestDataType type)
{
   m_pCreateMsg->Send(OBJ_NULL, pName, value, type);
   return doCreate(pName, value, type);
}

STDMETHODIMP cQuestData::doCreate(const char *pName, int value, eQuestDataType type)
{
   cQuestDataNode *pNode;
   int oldValue;

   if ((pNode = m_nameHash.Search((char*)pName)) != NULL)
   {
      oldValue = pNode->m_value;
      pNode->m_value = value;
      if (pNode->m_type != type)
      {
         if (pNode->m_type != kQuestDataUnknown)
            Warning(("Changing type of quest data item %s\n", pName));
         pNode->m_type = type;
      }
   }
   else
   {
      pNode = new cQuestDataNode(pName, type);
      pNode->m_value = value;
      m_nameHash.Insert(pNode);
      oldValue = 0;
   }
   pNode->SendMsgs(oldValue);
   return S_OK;
}

STDMETHODIMP cQuestData::Set(const char *pName, int value)
{
   m_pSetMsg->Send(OBJ_NULL, pName, value);
   return doSet(pName, value);
}

STDMETHODIMP cQuestData::doSet(const char *pName, int value)
{
   cQuestDataNode *pNode;

   if ((pNode = m_nameHash.Search((char*)pName)) == NULL)
      return Create(pName, value, kQuestDataCampaign);

   int oldValue = pNode->m_value;
   if (m_filter)
   {
      if (!m_filter(pName, oldValue, value))
         return S_FALSE;
   }

   pNode->m_value = value;
   pNode->SendMsgs(oldValue);
   SendAllMsgs(pNode->m_pName, oldValue, value);
   return S_OK;
}

STDMETHODIMP cQuestData::SendAllMsgs(const char *pName, int oldValue, int newValue)
{
   if (!m_allSubscriberList.Size())
      return S_OK;

   AutoAppIPtr(ScriptMan);

   ObjID objs[512];
   int n = 0;
   cObjList::cIter iter;
   for (iter = m_allSubscriberList.Iter(); !iter.Done() && n < 512; iter.Next())
      objs[n++] = iter.Value();

   for (int i = 0; i < n; i++)
   {
      sQuestMsg msg(objs[i], pName, oldValue, newValue);
      pScriptMan->SendMessage(&msg);
   }
   return S_OK;
}

STDMETHODIMP_(BOOL) cQuestData::Get(const char *pName)
{
   cQuestDataNode *pNode;

   if ((pNode = m_nameHash.Search(pName)) != NULL)
      return pNode->m_value;
   return 0;
}

STDMETHODIMP cQuestData::SubscribeMsg(ObjID objID, const char *pName, eQuestDataType type)
{
   cQuestDataNode *pQuestNode;

   if (pName && pName[0] == '*' && pName[1] == '\0')
      return m_allSubscriberList.AddElem(objID) ? S_OK : S_FALSE;

   if ((pQuestNode = m_nameHash.Search(pName)) == NULL)
   {
      // if this quest item doesn't exist, create and set to zero
      Create(pName, 0, type);
      pQuestNode = m_nameHash.Search(pName);
   }

   // add the data to the list
   if (!pQuestNode->m_subscriberList.AddElem(objID))
      return S_FALSE;     // already subscribed

   cQuestNodeList *pSubscribedList;
   // add to the subscriber hash
   if (!m_subscriberHash.Lookup(objID, &pSubscribedList))
   {
      pSubscribedList = new cQuestNodeList;
      m_subscriberHash.Set(objID, pSubscribedList);
   }
   pSubscribedList->Append(pQuestNode);

   return S_OK;
}

STDMETHODIMP cQuestData::UnsubscribeMsg(ObjID objID, const char *pName)
{
   if (pName && pName[0] == '*' && pName[1] == '\0')
      return m_allSubscriberList.RemoveElem(objID) ? S_OK : S_FALSE;

   cQuestDataNode* pQuestNode = m_nameHash.Search(pName); 
   if (pQuestNode  == NULL)
      return S_FALSE;

   // remove from subscriber list 
   if (!pQuestNode->m_subscriberList.RemoveElem(objID))
      return S_FALSE;   

   BOOL found = FALSE;
   cQuestNodeList *pSubscribedList;

   // remove from subscriber hash
   if (m_subscriberHash.Lookup(objID, &pSubscribedList))
   {
      cQuestNodeList::cIter iter;
      for (iter = pSubscribedList->Iter(); !iter.Done(); iter.Next())
      {
         cQuestDataNode* pQuestNode = iter.Value(); 
         if (strcmp(pQuestNode->m_pName,pName) == 0)
         {
            pSubscribedList->Remove(&iter.Node()); 
            break; 
         }
      }

      // if the list is now empty... get rid of it
      if (pSubscribedList->GetFirst() == NULL)
      {
         m_subscriberHash.Delete(objID); 
         delete pSubscribedList; 
      }
   }
   return S_OK;
}

STDMETHODIMP cQuestData::UnsubscribeAll()
{
   tHashSetHandle hnd; 
   cQuestDataNode* node; 

   // Clear all subscriber lists 
   for (node = m_nameHash.GetFirst(hnd); node != NULL; node = m_nameHash.GetNext(hnd))
      node->m_subscriberList.RemoveAll(); 

   cQuestSubscribeHash::cIter iter; 
   for (iter = m_subscriberHash.Iter(); !iter.Done(); iter.Next())
   {
      cQuestNodeList* pSubList = iter.Value(); 
      delete pSubList; 
   }

   m_subscriberHash.Clear(); 

   m_allSubscriberList.RemoveAll(); 

   return S_OK; 
   
}

STDMETHODIMP cQuestData::Delete(THIS_ const char *pName)
{
   cQuestDataNode *pNode = m_nameHash.Search(pName); 

   if (!pNode)
      return S_FALSE; 

   // deal with listeners
   cObjList::cIter iter = pNode->m_subscriberList.Iter(); 
   for (; !iter.Done(); iter.Next())
      UnsubscribeMsg(iter.Value(),pName); 

   m_nameHash.Remove(pNode);
   delete pNode;
   return S_OK;
}

STDMETHODIMP cQuestData::DeleteAll(void)
{
   tHashSetHandle handle;
   cQuestDataNode *pNode;

   pNode = m_nameHash.GetFirst(handle);
   for (; pNode != NULL; pNode = m_nameHash.GetNext(handle))
      Delete(pNode->m_pName); 
   return S_OK;
}

STDMETHODIMP cQuestData::DeleteAllType(eQuestDataType type)
{
   tHashSetHandle handle;
   cQuestDataNode *pNode;

   pNode = m_nameHash.GetFirst(handle);
   for (; pNode != NULL; pNode = m_nameHash.GetNext(handle))
      if (pNode->m_type == type)
         Delete(pNode->m_pName); 
   return S_OK;
}

STDMETHODIMP_(BOOL) cQuestData::Save(QuestMoveFunc moveFunc, eQuestDataType type)
{
   tHashSetHandle handle;
   int len;
   cQuestDataNode *pNode;

   pNode = m_nameHash.GetFirst(handle);
   while (pNode != NULL)
   {
      if ((pNode->m_type == type) || ((pNode->m_type == kQuestDataUnknown) && (type == kQuestDataMission)))
      {
         len = strlen(pNode->m_pName)+1;
         moveFunc((void*)&len, sizeof(int), 1);
         moveFunc((void*)(const char*)pNode->m_pName, sizeof(char), len);
         moveFunc((void*)&(pNode->m_value), sizeof(int), 1);
      }
      pNode = m_nameHash.GetNext(handle);
   }
   return S_OK;
}

STDMETHODIMP_(BOOL) cQuestData::Load(QuestMoveFunc moveFunc, eQuestDataType type)
{
   int len;
   int num;
   int value;

   while ((num = moveFunc((void*)&len, sizeof(int), 1)) == sizeof(int))
   {
      // Alloc a string of size len, bound within the scope of this block
      cStr name("",len); 
      char* pName = (char*)(const char*)name; 

      if ((num = moveFunc((void*)pName, sizeof(char), len)) != len*sizeof(char))
      {
         Warning(("cQuestData::Load - bad save format\n"));
         return E_FAIL;
      };
      if ((num = moveFunc((void*)&value, sizeof(int), 1)) != sizeof(int))
      {
         Warning(("cQuestData::Load - bad save format\n"));
         return E_FAIL;
      };
      Create(pName, value, type);
   }
   return S_OK;
}

void cQuestData::ObjListener(ObjID objID, eObjNotifyMsg msg, void *data)
{
   AutoAppIPtr(QuestData);
   if (msg == kObjNotifyDelete)
      pQuestData->ObjDeleteListener(objID);
}

STDMETHODIMP cQuestData::ObjDeleteListener(ObjID objID)
{
   cQuestNodeList *pSubscribedList;
   if (m_subscriberHash.Lookup(objID, &pSubscribedList))
   {
      cQuestNodeList::cIter iter = pSubscribedList->Iter(); 

      for (; !iter.Done(); iter.Next())
      {
         cQuestDataNode* pNode = iter.Value(); 
         pNode->m_subscriberList.RemoveElem(objID); 
      }
      
      delete pSubscribedList; 
      m_subscriberHash.Delete(objID); 
   }
   if (m_allSubscriberList.Size())
      m_allSubscriberList.RemoveElem(objID);
   return S_OK;
}

#ifndef SHIP
STDMETHODIMP cQuestData::ObjSpewListen(ObjID objID)
{
   cQuestNodeList *pSubscribedList;
   if (m_subscriberHash.Lookup(objID, &pSubscribedList))
   {
      cQuestNodeList::cIter iter = pSubscribedList->Iter(); 
      for (; !iter.Done(); iter.Next())
      {
         cQuestDataNode* pQuestNode = iter.Value(); 
         mprintf("%s ", pQuestNode->m_pName);
      }
   }
   return S_OK;
}
#endif

// @NOTE: the implementation currently only allows for a single filter.
// We could enhance this to allow any number of filters, but do bear in
// mind that they will all be run on every Set(), so this can get
// expensive...
STDMETHODIMP cQuestData::Filter(QuestFilterFunc filter, void *pClientData)
{
   m_filter = filter;
   m_pClientData = pClientData;
   return S_OK;
}

void QuestDataCreate()
{
   AutoAppIPtr(Unknown); 
   new cQuestData(pUnknown); 
}









