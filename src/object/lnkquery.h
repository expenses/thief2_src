// $Header: x:/prj/tech/libsrc/object/link/RCS/lnkquery.h 1.1 1999/04/07 14:38:23 TOML Exp $

#ifndef __LNKQUERY_H
#define __LNKQUERY_H
#include <comtools.h>
#include <linktype.h>

#undef INTERFACE 
#define INTERFACE ILinkQuery

DECLARE_INTERFACE_(ILinkQuery,IUnknown)
{
   DECLARE_UNKNOWN_PURE();
   
   //
   // Return whether or not the query is finished
   //
   STDMETHOD_(BOOL,Done)(THIS) CONSTFUNC PURE;

#define ILinkQuery_Done(p)    COMCall0(p, Done)

   //
   // Get the current link, ID and data 
   //
   STDMETHOD(Link)(THIS_ sLink* link) CONSTFUNC PURE;
   STDMETHOD_(LinkID,ID)(THIS) CONSTFUNC PURE;
   STDMETHOD_(void*,Data)(THIS) CONSTFUNC PURE;

#define ILinkQuery_Link(p, a)       COMCall1(p, Link, a)
#define ILinkQuery_ID(p)            COMCall0(p, Link)
#define ILinkQuery_Data(p)          COMCall0(p, Link)

   //
   // Go to the next link
   //
   STDMETHOD(Next)(THIS) PURE;

#define ILinkQuery_Next(p)    COMCall0(p, Next)

   //
   // Return a query in the same state as this one, 
   // but that returns the inverse links
   //

   STDMETHOD_(ILinkQuery*,Inverse)(THIS) PURE;

#define ILinkQuery_Inverse(p) COMCall0(p, Inverse)

};

//------------------------------------------------------------
// Link Query Factories

EXTERN ILinkQuery* CreateEmptyLinkQuery(void);

//
// Creates a proxied link query that inverts the links on the way out
// Useful for most link query implementations
//

EXTERN ILinkQuery* CreateInverseLinkQuery(ILinkQuery* );

#endif // __LNKQUERY_H

