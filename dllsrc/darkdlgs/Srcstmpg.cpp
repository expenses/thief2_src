#include "stdafx.h"
#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <stimuli.h>
#include <objquery.h>
#include <propag8n.h>
#include <propag8r.h>
#include <isdesced.h>
#include <isdescst.h>

#include "darkdlgs.h"
#include "Srcstmpg.h"

extern "C" IStructEditor* LGAPI ConstructStructEditor(sStructEditorDesc* eddesc, sStructDesc* desc, void* editme); 

/////////////////////////////////////////////////////////////////////////////
// cSourceStimPage property page

IMPLEMENT_DYNCREATE(cSourceStimPage, CPropertyPage)

cSourceStimPage::cSourceStimPage() : CPropertyPage(cSourceStimPage::IDD)
{
	//{{AFX_DATA_INIT(cSourceStimPage)
	m_level = 0.0f;
	//}}AFX_DATA_INIT
}

cSourceStimPage::~cSourceStimPage()
{
}

void cSourceStimPage::DoDataExchange(CDataExchange* pDX)
{
   BOOL init = !pDX->m_bSaveAndValidate; 

	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(cSourceStimPage)
	DDX_Control(pDX, IDC_SHAPEBUTT, m_shape);
	DDX_Control(pDX, IDC_LIFEBUTT, m_life);
	DDX_Control(pDX, IDC_OBJ, m_obj);
	DDX_Control(pDX, IDC_STIMULUS, m_stim);
	DDX_Control(pDX, IDC_PGATOR, m_gator);
	DDX_Text(pDX, IDC_LEVEL, m_level);
	//}}AFX_DATA_MAP

   if (init)
   {
      AutoAppIPtr(Stimuli); 
      ObjID root = pStimuli->GetRootStimulus(); 
      IObjectQuery* query = pStimuli->QueryHeirs(root); 
      m_stim.Fill(query); 
      SafeRelease(query); 

      m_stim.SelectItem(m_elems.stim); 
      m_obj.SetObj(m_elems.obj); 
      m_gator.Fill(m_elems.stim); 
      m_gator.SelectItem(m_desc.propagator); 

      m_gator.UpdateData(FALSE); 
      m_stim.UpdateData(FALSE); 
      m_obj.UpdateData(FALSE); 

      m_life.EnableWindow(m_desc.valid_fields & kStimSrcLifeValid); 
      m_shape.EnableWindow(m_desc.valid_fields & kStimSrcShapeValid); 
   }

   if (!init)
   {
      m_elems.stim = m_stim.Selection(); 
      m_elems.obj = m_obj.GetObj(); 
      m_desc.propagator = m_gator.Selection(); 
   }
}


BEGIN_MESSAGE_MAP(cSourceStimPage, CPropertyPage)
	//{{AFX_MSG_MAP(cSourceStimPage)
	ON_CBN_SELCHANGE(IDC_PGATOR, OnSelchangePgator)
	ON_CBN_SELCHANGE(IDC_STIMULUS, OnSelchangeStimulus)
	ON_BN_CLICKED(IDC_SHAPEBUTT, OnShapebutt)
	ON_BN_CLICKED(IDC_LIFEBUTT, OnLifebutt)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cSourceStimPage message handlers

void cSourceStimPage::OnSelchangePgator() 
{
   UpdateData(TRUE); 

   // let the new propagator set up the defaults
   AutoAppIPtr_(Propagation,pGation); 
   IPropagator* pGator = pGation->GetPropagator(m_gator.Selection()); 
   pGator->InitSource(&m_desc); 
   m_level = m_desc.level; 
   UpdateData(FALSE); 
   SafeRelease(pGator); 
}

void cSourceStimPage::OnSelchangeStimulus() 
{
   m_elems.stim = m_stim.Selection(); 
   UpdateData(FALSE); 
}

BOOL cSourceStimPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL cSourceStimPage::FromSource(sObjStimPair* elems, sStimSourceDesc* desc)
{
   m_elems = *elems; 
   m_desc = *desc; 
   m_level = desc->level; 
   return TRUE; 
}

BOOL cSourceStimPage::ToSource(sObjStimPair* elems, sStimSourceDesc* desc)
{
   *elems = m_elems; 
   m_desc.level = m_level; 
   *desc = m_desc; 
   return TRUE; 
}

void cSourceStimPage::OnShapebutt() 
{
   AutoAppIPtr_(Propagation,pGation); 
   IPropagator* pGator = pGation->GetPropagator(m_gator.Selection()); 
   const sStructDesc* desc = pGator->DescribeShapes(); 
   sStructEditorDesc eddesc = { "Shape Data" }; 
   IStructEditor* sed = ConstructStructEditor(&eddesc,(sStructDesc*)desc,&m_desc.shape); 
   sed->Go(kStructEdModal); 
   SafeRelease(sed); 
}

void cSourceStimPage::OnLifebutt() 
{
   AutoAppIPtr_(Propagation,pGation); 
   IPropagator* pGator = pGation->GetPropagator(m_gator.Selection()); 
   const sStructDesc* desc = pGator->DescribeLifeCycles(); 
   sStructEditorDesc eddesc = { "Life Cycle Data" }; 
   IStructEditor* sed = ConstructStructEditor(&eddesc,(sStructDesc*)desc,&m_desc.life); 
   sed->Go(kStructEdModal); 
   SafeRelease(sed); 
}
