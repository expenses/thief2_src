// aredit_.h
//
// Implementation of IActReactEditors
//

#ifndef __AREDIT__H
#define __AREDIT__H

#include <comtools.h>
#include <aredit.h>
#include <stimguid.h>

class cAREditors : public IActReactEditors
{
   DECLARE_UNAGGREGATABLE(); 

public:
   cAREditors(); 
   virtual ~cAREditors(); 

   STDMETHOD(SourceDialog)(eARDlgFlags flags, sObjStimPair* elems, sStimSourceDesc* desc); 
   STDMETHOD(ReceptronDialog)(eARDlgFlags flags, sObjStimPair* elems, sReceptron* desc); 
   STDMETHOD(ReceptronList)(eARDlgFlags flags, const sObjStimPair* pair); 
   STDMETHOD(SourceList)(eARDlgFlags flags, const sObjStimPair* pair); 
}; 

extern "C" __declspec(dllexport) IActReactEditors* LGAPI ExportActReactEditors(); 

#endif // __AREDIT__H
