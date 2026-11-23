#include "stdafx.h"
#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <reaction.h>
#include <stimuli.h>
#include <objquery.h>
#include <edittool.h>
#include <sdestool.h>
#include <sdesbase.h>

#include "darkdlgs.h"
#include "trondlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// cReceptronDialog dialog


cReceptronDialog::cReceptronDialog(sObjStimPair* pair, sReceptron* tron, CWnd* pParent /*=NULL*/)
	: CDialog(cReceptronDialog::IDD, pParent),
     m_agent(NULL),
     m_target(NULL)
{
	//{{AFX_DATA_INIT(cReceptronDialog)
	//}}AFX_DATA_INIT
   FromTron(pair,tron); 
}


void cReceptronDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(cReceptronDialog)
	DDX_Control(pDX, IDC_EFFECT_TEXT, m_effect_text);
	DDX_Control(pDX, IDC_REACTION, m_effect);
	DDX_Control(pDX, IDC_STIMULUS, m_stim);
	DDX_Control(pDX, IDC_TRON_OBJ, m_obj);
	DDX_Text(pDX, IDC_MAX, m_max);
	DDX_Text(pDX, IDC_MIN, m_min);
	DDX_Check(pDX, IDC_NOMAX, m_nomax);
	DDX_Check(pDX, IDC_NOMIN, m_nomin);
	//}}AFX_DATA_MAP

   if (pDX->m_bSaveAndValidate)
   {
      m_target.UpdateData(TRUE);
      m_agent.UpdateData(TRUE);
      m_param.obj[kReactDirectObj] = m_target.GetObj();
      m_param.obj[kReactIndirectObj] = m_agent.GetObj();
      m_tron.effect.kind = m_effect.Selection();
      m_elems.stim = m_stim.Selection();
      m_elems.obj = m_obj.GetObj();
   }
   else
   {
      m_target.SetObj(m_param.obj[kReactDirectObj]);
      m_agent.SetObj(m_param.obj[kReactIndirectObj]);
      m_effect.SelectItem(m_tron.effect.kind);
      m_stim.SelectItem(m_elems.stim);
      m_obj.SetObj(m_elems.obj);
      OnNomax();
      OnNomin();
      UpdateReaction();
      m_target.UpdateData(FALSE);
      m_agent.UpdateData(FALSE);
      m_effect.UpdateData(FALSE);
      m_stim.UpdateData(FALSE);
      m_obj.UpdateData(FALSE);
   }
}


BEGIN_MESSAGE_MAP(cReceptronDialog, CDialog)
	//{{AFX_MSG_MAP(cReceptronDialog)
	ON_BN_CLICKED(IDC_NOMAX, OnNomax)
	ON_BN_CLICKED(IDC_NOMIN, OnNomin)
	ON_BN_CLICKED(IDC_TEMPBUTT, OnTempbutt)
	ON_CBN_SELCHANGE(IDC_REACTION, OnSelchangeReaction)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cReceptronDialog message handlers

void cReceptronDialog::OnNomax() 
{
   m_nomax = IsDlgButtonChecked(IDC_NOMAX); 
   CWnd* wnd = GetDlgItem(IDC_MAX); 
   wnd->EnableWindow(!m_nomax); 
}

void cReceptronDialog::OnNomin() 
{
   m_nomin = IsDlgButtonChecked(IDC_NOMIN); 
   CWnd* wnd = GetDlgItem(IDC_MIN); 
   wnd->EnableWindow(!m_nomin); 
}

void cReceptronDialog::OnTempbutt() 
{
   m_effect.UpdateData(TRUE); 
   ulong id = m_effect.Selection(); 
   AutoAppIPtr(Reactions); 
   const sReactionDesc* desc = pReactions->DescribeReaction(id); 
   if (desc != NULL)
   {
      AutoAppIPtr_(EditTools,pEdit); 
      pEdit->EditTypedData("Effect Data",desc->param_type.name,&m_param.data); 
      FillEffectText(desc->param_type.name); 
   }
}

void cReceptronDialog::OnSelchangeReaction() 
{
   UpdateData(TRUE); 
   // new reaction, new parameters 
   memset(&m_param.data,0,sizeof(m_param.data)); 
   UpdateData(FALSE); 
}

void cReceptronDialog::FromTron(sObjStimPair* elems, sReceptron* tron)
{
   m_tron = *tron; 
   m_min = tron->trigger.min; 
   m_max = tron->trigger.max; 
   m_nomax = tron->trigger.flags & kStimTrigNoMax; 
   m_nomin = tron->trigger.flags & kStimTrigNoMin; 
   m_param = tron->effect.param; 
   m_elems = *elems; 
}

void cReceptronDialog::ToTron(sObjStimPair* elems, sReceptron* tron)
{
   tron->trigger.min = m_min; 
   tron->trigger.max = m_max; 
   tron->trigger.flags = 0; 
   if (m_nomax)
      tron->trigger.flags |= kStimTrigNoMax; 
   if (m_nomin)
      tron->trigger.flags |= kStimTrigNoMin; 
   tron->effect.kind = m_effect.Selection(); 
   tron->effect.param = m_param; 
   m_tron = *tron; 
   *elems = m_elems; 
}

BOOL cReceptronDialog::OnInitDialog() 
{
   m_target.DoCreate(this,IDC_TARGET); 
   m_target.SetCaption("Target Object"); 
   m_target.SelectSpecial(kObjParamSensor); 
   m_agent.DoCreate(this,IDC_AGENT); 
   m_agent.SetCaption("Agent Object"); 
   m_agent.SetObj(OBJ_NULL); 



   VERIFY(m_effect.SubclassDlgItem(IDC_REACTION,this)); 
   AutoAppIPtr(Reactions); 
   m_effect.AddItemNamed(REACTION_NULL,"None"); 
   IReactionQuery* query = pReactions->QueryAll(); 
   for (query->Start(); !query->Done(); query->Next())
   {
      const sReactionDesc* desc = query->Reaction(); 
      m_effect.AddItemNamed(query->ID(),desc->friendly_name); 
   }
   SafeRelease(query);




   VERIFY(m_stim.SubclassDlgItem(IDC_STIMULUS,this)); 
   AutoAppIPtr(Stimuli); 
   ObjID root = pStimuli->GetRootStimulus(); 
   IObjectQuery* oquery = pStimuli->QueryHeirs(root); 
   m_stim.Fill(oquery); 
   SafeRelease(oquery); 

	CDialog::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void cReceptronDialog::FillEffectText(const char* type)
{
   AutoAppIPtr_(StructDescTools,pTools); 
   const sStructDesc* desc = pTools->Lookup(type); 
   if (desc == NULL)
   {
      CString text = ""; 
      m_effect_text.SetWindowText(text); 
      return; 
   }

   int n = desc->nfields; 
   CString text = ""; 
   for (int i = 0; i < n; i++)
   {
      const sFieldDesc& field = desc->fields[i]; 
      text += field.name; 
      text += ": "; 
      char buf[64]; 
      pTools->UnparseField(&field,&m_param.data,buf,sizeof(buf)); 
      text += buf; 
      text += "\n"; 
   }
   m_effect_text.SetWindowText(text); 
}

static sReactionTypeDesc no_param = NO_REACTION_PARAM; 

void cReceptronDialog::UpdateReaction()
{
   AutoAppIPtr_(Reactions,pReact); 
   ulong id = m_effect.Selection(); 
   const sReactionDesc* desc = pReact->DescribeReaction(id); 
   ulong flags = (desc) ? desc->flags : 0; 

   CWnd* wnd = GetDlgItem(IDC_TARGET); 
   if (flags & kReactionHasDirectObj)
      wnd->ShowWindow(SW_SHOW);
   else
      wnd->ShowWindow(SW_HIDE);
   wnd = GetDlgItem(IDC_AGENT);
   if (flags & kReactionHasIndirectObj)
      wnd->ShowWindow(SW_SHOW);
   else
      wnd->ShowWindow(SW_HIDE); 

   // only edit parameters if there are any 
   const char* name = desc->param_type.name; 
   BOOL none = strcmp(no_param.name,name) == 0; 
   wnd = GetDlgItem(IDC_TEMPBUTT);
   wnd->EnableWindow(!none); 
   FillEffectText(name); 
}
