///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/ai/RCS/aistdmrg.h 1.4 1998/10/20 23:15:06 TOML Exp $
//
// The standard movement regulators
//
// @Note (toml 03-31-98):
// Most of the ones from the previous AI have been removed or reexpressed. Do
// do: possibly reintegrate cliff prediction, object avoidance, maybe holy/esp
// related game specific forces
//

#ifndef __AISTDMRG_H
#define __AISTDMRG_H

#include <aibasmrg.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIWallsCliffsMovReg
//
// Responsible for making sure the AI doesn't run into walls
//

class cAIWallsCliffsMovReg : public cAIMoveRegulator
{
public:
   STDMETHOD_(const char *, GetName)();

   //
   STDMETHOD_(BOOL, NewRegulations)();

   //
   STDMETHOD (SuggestRegulations)(cAIMoveSuggestions &);

};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIObjectsMovReg
//
// Responsible for making sure the AI doesn't run into objects
//

class cAIObjectsMovReg : public cAIMoveRegulator
{
public:
   STDMETHOD_(const char *, GetName)();

   //
   STDMETHOD_(BOOL, NewRegulations)();

   //
   STDMETHOD (SuggestRegulations)(cAIMoveSuggestions &);
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AISTDMRG_H */
