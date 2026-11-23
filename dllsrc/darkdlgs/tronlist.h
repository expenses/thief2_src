#ifndef __TRONLIST_H
#define __TRONLIST_H

#include "linklist.h"

F_DECLARE_INTERFACE(IStimSensors);

/////////////////////////////////////////////////////////////////////////////
// cTronList
//
// Link list of receptrons

class cTronList : public cLinkList
{
public:
   cTronList(); 
   virtual ~cTronList(); 

   enum eTronColumns
   {
      kListColumns = kDataColumn, 
      kMinColumn, 
      kMaxColumn, 
      kReactionColumn, 
   }; 

   int AddLink(LinkID id); 
   const sColumnDesc* DescribeColumns(); 

protected:
   void EditLink(LinkID tronid); 

   IStimSensors* m_pSensors; 

	DECLARE_MESSAGE_MAP()
}; 

#endif // __TRONLIST_H
