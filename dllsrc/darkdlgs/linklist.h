#ifndef __LINKLIST_H
#define __LINKLIST_H

#include <comtools.h>
#include <linktype.h>
#include <lnkquery.h>

F_DECLARE_INTERFACE(IEditTools);
F_DECLARE_INTERFACE(ILinkManager);

/////////////////////////////////////////////////////////////////////////////
// cLinkList window
//
// A report-view list control of links, one row per LinkID

class cLinkList : public CListCtrl
{
// Construction
public:
	cLinkList();

// Attributes
public:
   // Query the selected links 
   virtual ILinkQuery* Selection(); 

// Operations
public:
   virtual void EditLink(LinkID id); 
   virtual void RemoveLink(LinkID id); 
   // returns the row 
   virtual int AddLink(LinkID id); 

   void SetupColumns(); 

   enum eColumns
   {
      kIDColumn,
      kFlavorColumn,
      kSourceColumn,
      kDestColumn,
      kDataColumn,
   }; 

   struct sColumnDesc
   {
      char name[16];
      int cx; 
      int subid; 
   }; 

protected:
   int Column(int cx); 
   virtual const sColumnDesc* DescribeColumns(); 

// Implementation
public:
	virtual ~cLinkList();

protected:
   void SetItem(int row, int columnid, CString text); 

   IEditTools* m_pTools; 
   int* m_column_map; 
   int m_ncolumns; 
   ILinkManager* m_pLinkMan; 

	// Generated message map functions
protected:
	//{{AFX_MSG(cLinkList)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif // __LINKLIST_H
