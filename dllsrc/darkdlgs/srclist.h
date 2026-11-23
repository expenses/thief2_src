#ifndef __SRCLIST_H
#define __SRCLIST_H

#include "linklist.h"

F_DECLARE_INTERFACE(IStimSources);

/////////////////////////////////////////////////////////////////////////////
// cSourceList
//
// Link list of stimulus sources

class cSourceList : public cLinkList
{
public:
   cSourceList(); 
   virtual ~cSourceList(); 

   enum eSourceColumns
   {
      kListColumns = kDataColumn, 
      kLevelColumn, 
      kPropagatorColumn, 
   }; 

   int AddLink(LinkID id); 
   const sColumnDesc* DescribeColumns(); 

protected:
   void EditLink(LinkID sourceid); 

   IStimSources* m_pSources; 

	DECLARE_MESSAGE_MAP()
}; 

#endif // __SRCLIST_H
