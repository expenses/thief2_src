#include "stdafx.h"
#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <linkman.h>
#include <relation.h>
#include <linkbase.h>
#include <linkid.h>
#include <edittool.h>
#include <sdestool.h>

#include "darkdlgs.h"
#include "linklist.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// cLinkList




cLinkList::cLinkList()
   : m_pTools(AppGetObj(IEditTools)),
     m_column_map(NULL),
     m_ncolumns(0),
     m_pLinkMan(AppGetObj(ILinkManager))
{
}


cLinkList::~cLinkList()
{
   SafeRelease(m_pLinkMan); 
   SafeRelease(m_pTools); 
}


BEGIN_MESSAGE_MAP(cLinkList, CListCtrl)
	//{{AFX_MSG_MAP(cLinkList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()





int cLinkList::AddLink(LinkID id)
{
   int idx = GetItemCount(); 

   char buf[64]; 
   sprintf(buf,"%08X",id); 
   InsertItem(idx,buf); 

   IRelation* rel = m_pLinkMan->GetRelation(LINKID_RELATION(id)); 
   SetItem(idx,kFlavorColumn,rel->Describe()->name); 

   sLink link = { 0, 0 };  
   rel->Get(id,&link); 
   SetItem(idx,kSourceColumn,m_pTools->ObjName(link.source)); 
   SetItem(idx,kDestColumn,m_pTools->ObjName(link.dest)); 

   void* data = rel->GetData(id); 
   const sRelationDataDesc* desc = rel->DescribeData(); 
   if (data != NULL && desc->size > 0)
   {
      AutoAppIPtr_(StructDescTools,pSdescTools); 
      const sStructDesc* sdesc = pSdescTools->Lookup(desc->type); 
      char buf[80]; 
      if (sdesc != NULL)
         pSdescTools->UnparseFull(sdesc,data,buf,sizeof(buf)); 
      else
         sprintf(buf,"%08X",data); 
      SetItem(idx,kDataColumn,buf); 
   }

   SetItemData(idx,id); 
   SafeRelease(rel); 
   return idx; 
}














void cLinkList::RemoveLink(LinkID id)
{
}


void cLinkList::EditLink(LinkID id)
{
   IRelation* rel = m_pLinkMan->GetRelation(LINKID_RELATION(id)); 
   void* data = rel->GetData(id); 
   const sRelationDataDesc* desc = rel->DescribeData(); 
   if (desc->size <= 0)
      return;

   {
      BOOL deldata = FALSE;
      if (data == NULL)
      {
         data = new char[desc->size]; 
         memset(data,0,desc->size);
         deldata = TRUE; 
      }
      m_pTools->EditTypedData("Link Data",desc->type,data); 
      rel->SetData(id,data); 
      if (deldata)
         delete data; 
   }
}

static cLinkList::sColumnDesc columns[] = 
{
   { "ID",     75,  cLinkList::kIDColumn },
   { "Flavor", 100, cLinkList::kFlavorColumn },
   { "From",   90,  cLinkList::kSourceColumn },
   { "To",     90,  cLinkList::kDestColumn },
   { "Data",   200, cLinkList::kDataColumn },
   { "" }, 
}; 

const cLinkList::sColumnDesc* cLinkList::DescribeColumns()
{
   return columns; 
}




void cLinkList::SetupColumns()
{
   const sColumnDesc* desc; 

   // figure out how big the column map is 
   for (desc = DescribeColumns(), m_ncolumns = 0; desc != NULL && desc->name[0] != '\0'; desc++)
      if (m_ncolumns <= desc->subid)
         m_ncolumns = desc->subid + 1; 

   m_column_map = new int[m_ncolumns]; 

   int i = 0; 
   for (desc = DescribeColumns(); desc != NULL && desc->name[0] != '\0'; desc++, i++)
   {
      m_column_map[desc->subid] = i; 
      InsertColumn(i,desc->name,LVCFMT_LEFT,desc->cx,desc->subid); 
   }
}

////////////////////////////////////////////////////////////
// Link query over the rows of a cLinkList 
//

class cLinkListQuery : public ILinkQuery
{
protected:
   cLinkList* pList; 
   ILinkManager* pLinkMan; 
   int Idx; 
   BOOL Inverted; 




public:
   cLinkListQuery(cLinkList* list, ILinkManager* lm)
      : pList(list), pLinkMan(lm), Idx(0), Inverted(0)
   {
      lm->AddRef(); 
   }

public:
   DECLARE_UNAGGREGATABLE(); 



protected:
   cLinkListQuery(cLinkListQuery* q, BOOL inverted)
      : pList(q->pList), pLinkMan(q->pLinkMan), Idx(q->Idx), Inverted(inverted)
   {
   }

public:
   virtual ~cLinkListQuery()
   {
      SafeRelease(pLinkMan); 
   }

   STDMETHOD_(BOOL,Done)() const { return Idx > pList->GetItemCount(); }

   STDMETHOD_(LinkID,ID)() const
   {
      LinkID id = pList->GetItemData(Idx); 
      return Inverted ? RELID_INVERT(id) : id; 
   }

   STDMETHOD(Link)(sLink* link) const
   {
      return pLinkMan->Get(ID(),link) ? E_FAIL : S_OK; 
   }

   STDMETHOD_(void*,Data)() const { return pLinkMan->GetData(ID()); }
}; 

//
// Iterates over just the selected rows
//

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cLinkListQuery,ILinkQuery); 

class cLinkListSelectionQuery : public cLinkListQuery
{
   cLinkListSelectionQuery(cLinkListQuery* q, BOOL inverted)
      : cLinkListQuery(q,inverted)
   {
   }


public:
   cLinkListSelectionQuery(cLinkList* list, ILinkManager* lm)
      : cLinkListQuery(list,lm)
   {
      if (!pList->GetItemState(Idx,LVIS_SELECTED))
         Next(); 
   }


   STDMETHOD(Next)()
   {
      int n = pList->GetItemCount(); 
      for (Idx++; Idx < n && !pList->GetItemState(Idx,LVIS_SELECTED); Idx++)
         ; 
      return S_OK; 
   }

   STDMETHOD_(ILinkQuery*,Inverse)()
   {
      return new cLinkListSelectionQuery(this,!Inverted); 
   }
}; 



ILinkQuery* cLinkList::Selection()
{
   return new cLinkListSelectionQuery(this,m_pLinkMan); 
}

////////////////////////////////////////////////////////////

void cLinkList::SetItem(int row, int columnid, CString text)
{
   int c = Column(columnid); 
   if (c >= 0)
      CListCtrl::SetItem(row,c,LVIF_TEXT,text,0,0,0,0); 
}

int cLinkList::Column(int cx)
{
   return (cx < m_ncolumns) ? m_column_map[cx] : -1; 
}
