/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/querynul.cpp,v 1.3 1997/10/14 11:20:38 TOML Exp $
#include <linktype.h>
#include <linkbase.h>
#include <querynul.h>

// Must be last header
#include <dbmem.h>

static void * gpNullLinkQueryFreeList = NULL;

void * cNullLinkQuery::operator new(size_t n)
{
   void ** p;
   if (gpNullLinkQueryFreeList)
   {
      p = (void **) gpNullLinkQueryFreeList;
      gpNullLinkQueryFreeList = *p;
   }
   else
      p = (void **) Malloc(n + sizeof(void *));
   return p + 1;
}

void cNullLinkQuery::operator delete(void * p)
{
   if (p)
   {
      void ** pNode = (void **) p - 1;
      *pNode = gpNullLinkQueryFreeList;
      gpNullLinkQueryFreeList = pNode;
   }
}

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cNullLinkQuery,ILinkQuery);

STDMETHODIMP_(BOOL) cNullLinkQuery::Done() const
{
   return TRUE;  // we awways dun
}

STDMETHODIMP cNullLinkQuery::Link(sLink*) const 
{
   Warning(("cNullLinkQuery::Link() has been called\n"));
   return E_FAIL;
}

STDMETHODIMP cNullLinkQuery::Next()
{
   Warning(("cNullLinkQuery::Next() has been called\n"));
   return E_FAIL;   
}

ILinkQuery* CreateEmptyLinkQuery(void)
{
   return new cNullLinkQuery;
}
