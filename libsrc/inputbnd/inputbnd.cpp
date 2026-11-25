#include <stdio.h>

#include <comtools.h>
#include <aggmemb.h>
#include <dynarray.h>
#include <types.h>

#include <inpbnd_i.h>
#include <ibvarman.h>
#include <ibmapper.h>

#define snprintf _snprintf

extern cIBVariableManager* g_IB_variable_manager;
extern cIBInputMapper* g_IB_input_mapper;

class cInputBinder : public cCTDelegating<IInputBinder>,
	public cCTAggregateMemberControl<kCTU_Default>
{
public:
	cInputBinder(IUnknown* pOuter);
	//
	// Can initialize binding vars and a .bnd file to load upon Init.
	// pVars and pBndFname can both be NULL.
	//
	STDMETHOD_(char*, Init)(THIS_ IB_var* pVars, char* pBndFname)
	{
		m_IB_variable_manager = new cIBVariableManager();

		memset(m_IB_input_mappers, 0, sizeof(m_IB_input_mappers));
		m_IB_input_mappers[0] = new cIBInputMapper();
		m_IB_input_mappers[0]->m_context = 1;
		m_cur_context_idx = 0;

		if (pVars)
			VarSet(pVars);

		if (pBndFname)
			LoadBndFile(pBndFname, 0, NULL);

		return 0;
	}
	STDMETHOD_(char*, Term)(THIS)
	{
		for (int i = 0; i < 32; ++i)
		{
			if (m_IB_input_mappers[i])
				delete m_IB_input_mappers[i];
		}

		if (m_IB_variable_manager)
			delete m_IB_variable_manager;

		m_BndCtxtArray.SetSize(0);

		return NULL;
	}

	//
	// Stuffs ppHandler with the input binder's ui handler.
	//
	STDMETHOD_(void, GetHandler)(THIS_ tBindHandler* ppHandler)
	{
		SetGlobObjs();
		*ppHandler = (tBindHandler)cIBInputMapper::InputBindingHandler;
	}

	//
	// Used to mask which events the binder will trap.
	// These are defined in event.h
	//
	STDMETHOD_(void, SetValidEvents)(THIS_ ulong iEventMask)
	{
		SetGlobObjs();
		g_IB_input_mapper->m_valid_events = iEventMask;
	}
	STDMETHOD_(ulong, GetValidEvents)(THIS)
	{
		SetGlobObjs();
		return g_IB_input_mapper->m_valid_events;
	}

	//
	// Binds pCmd to pControl. Multiple worded commands should
	// be wrapped in quotes. Returns TRUE if all went well.
	//
	STDMETHOD_(BOOL, Bind)(THIS_ char* pControl, char* pCmd)
	{
		char pBuf[256];

		sprintf(pBuf, "bind %s %s", pControl, pCmd);
		return ProcessCmd(pBuf) == 0;
	}

	//
	// Uninds pControl. Returns FALSE if pControl is invalid.
	//
	STDMETHOD_(BOOL, Unbind)(THIS_ char* pControl)
	{
		char pBuf[256];

		sprintf(pBuf, "unbind %s", pControl);
		return ProcessCmd(pBuf) == 0;
	}

	//
	// Stuffs pCmdBuf with whatever command pControl is bound to.
	//
	STDMETHOD_(void, QueryBind)(THIS_ char* pControl, char* pCmdBuf, long iBufLen)
	{
		char pBuf[256];

		sprintf(pBuf, "bind %s", pControl);
		const char* pRes = ProcessCmd(pBuf);
		if (!pRes)
			pRes = "";
		strncpy(pCmdBuf, pRes, iBufLen);
	}

	//
	// Stuffs pValBuf with the value of pVarStr. Currently does not take
	// channels into account.
	//
	STDMETHOD_(void, GetVarValue)(THIS_ char* pVarStr, char* pValBuf, long iBufLen)
	{
		char pBuf[256];

		sprintf(pBuf, "echo $%s", pVarStr);
		const char* pRes = ProcessCmd(pBuf);
		if (!pRes)
			pRes = "";
		strncpy(pValBuf, pRes, iBufLen);
	}

	//
	// Accepts a string which contains commands
	// to interact with the binder. Most users
	// should not have to call this.
	//
	STDMETHOD_(char*, ProcessCmd)(THIS_ char* pCmdStr)
	{
		SetGlobObjs();
		return (char*)m_IB_variable_manager->Cmd(pCmdStr, 0);
	}

	//
	// Trap a control, and bind it to pCmdStr, if the filter callback allows it.
	// Both callbacks and pUserData may be NULL, however.
	//
	STDMETHOD_(void, TrapBind)(THIS_ char* pCmdStr, tTrapBindFilter filter_cb, tTrapBindPostFunc post_cb, void* pUserData)
	{
		SetGlobObjs();
		g_IB_input_mapper->TrapBind(pCmdStr, filter_cb, post_cb, pUserData);
	}

	//
	// Merely sets the internal mapper to the correct context.
	// Usually not needed.
	//
	STDMETHOD_(char*, Update)(THIS)
	{
		SetGlobObjs();
		return NULL;
	}

	//
	// Polls keys and sends out appropriate commands upon change of state.
	//
	STDMETHOD_(void, PollAllKeys)(THIS)
	{
		SetGlobObjs();
		g_IB_input_mapper->PollAllKeys();
	}

	//
	// Registers the application's joystick processor, inherited from above.
	//
	STDMETHOD_(void, RegisterJoyProcObj)(THIS_ void* pJoyProc)
	{
		SetGlobObjs();
		g_IB_input_mapper->m_joyproc = (cIBJoyAxisProcess*)pJoyProc;
	}


	//
	// This is the preferred method of bind loading. You associate context bitmasks
	// and context strings with the ContextAssociate method, then LoadBndContexted
	// will parse the file only once, and map the contexted commands accordingly.
	//
	STDMETHOD_(void, LoadBndContexted)(THIS_ char* pBndFname)
	{
		m_IB_variable_manager->LoadBndContexted(pBndFname, m_BndCtxtArray.AsPointer(), m_BndCtxtArray.Size(), m_IB_input_mappers);
		SetGlobObjs();
	}

	//
	// Accepts a pointer to a kBindContextNull-terminated array of sBindContext's.
	// A call to this method wipes out any previoius context associations. These
	// associations are then used when mapping contexted binds during a call to
	// LoadBndContexted.
	//
	STDMETHOD_(void, ContextAssociate)(THIS_ sBindContext* pBindContext)
	{
		sBindContext ctxtNull = kBindContextNull;

		m_BndCtxtArray.SetSize(0);
		for (int i = 0; memcmp(&pBindContext[i], &ctxtNull, sizeof(sBindContext)); ++i)
			m_BndCtxtArray.Append(pBindContext[i]);
	}


	//
	// Loads a .bnd file into the specified iContext. If pPrefix is not NULL,
	// we bind only the matching prefixed binds.
	//
	STDMETHOD_(char*, LoadBndFile)(THIS_ char* pBndFname, unsigned long iContext, char* pPrefix)
	{
		char str[128];

		strcpy(str, "loadbnd ");
		strcat(str, pBndFname);

		SetGlobObjs();
		if (iContext)
		{
			int old_context = 1 << m_cur_context_idx;
			int bit = 0;
			while (iContext)
			{
				if ((iContext & 1) != 0)
				{
					if (m_IB_input_mappers[bit])
					{
						SetContext(1 << bit, TRUE);
						m_IB_variable_manager->LoadBnd(pBndFname, pPrefix);
					}
				}
				++bit;
				iContext >>= 1;
			}

			SetContext(old_context, TRUE);
			return NULL;
		}
		return (char*)m_IB_variable_manager->Cmd(str, 0);
	}

	//
	// Saves a bind file under the current context only. Should undoubtedly
	// change this in the future. If pHeader is non-NULL, the string will
	// be placed at the top of the file.
	//
	STDMETHOD_(char*, SaveBndFile)(THIS_ char* pBndFname, char* pHeader)
	{
		SetGlobObjs();
		g_IB_input_mapper->SaveBnd(pBndFname, pHeader);
		return NULL;
	}

	//
	// Methods for setting and unsetting bind command variables.
	//
	STDMETHOD_(BOOL, VarSet)(THIS_ IB_var* pVars) //must be NULL-terminated
	{
		SetGlobObjs();
		return m_IB_variable_manager->VarSet(pVars, 0);
	}
	STDMETHOD_(BOOL, VarSetn)(THIS_ IB_var* pVars, long iNum)
	{
		SetGlobObjs();
		return m_IB_variable_manager->VarSet(pVars, iNum, 1);
	}
	// if bUser is set, then the variable(s) can be unset by the player.
	STDMETHOD_(char*, VarUnset)(THIS_ char** ppVarNames, BOOL bUser) //must be NULL-terminated
	{
		SetGlobObjs();
		return (char*)m_IB_variable_manager->VarUnset(ppVarNames, bUser);
	}
	STDMETHOD_(char*, VarUnsetn)(THIS_ char** ppVarNames, long iNum, BOOL bUser)
	{
		SetGlobObjs();
		return (char*)m_IB_variable_manager->VarUnset(ppVarNames, iNum, bUser);
	}
	STDMETHOD_(char*, VarUnsetAll)(THIS)
	{
		return (char*)m_IB_variable_manager->VarUnsetAll();
	}


	//
	// For setting and getting out current context. This should be a bitmask.
	// Also, do we want to poll when changing to a different context?
	//
	STDMETHOD_(BOOL, SetContext)(THIS_ unsigned long iContext, BOOL bPoll)
	{
		if (m_IB_input_mappers[m_cur_context_idx]->m_context == iContext)
			return TRUE;

		if (!iContext || (iContext & -iContext) != iContext)
			return FALSE;

		int bit = 0;
		while ((iContext & 1) == 0)
		{
			++bit;
			iContext >>= 1;
		}

		m_cur_context_idx = bit;
		if (m_IB_input_mappers[m_cur_context_idx])
		{
			m_IB_input_mappers[m_cur_context_idx]->m_mod_states = 0;
			SetGlobObjs();
			if (bPoll)
				PollAllKeys();
			return TRUE;
		}

		m_IB_input_mappers[m_cur_context_idx] = new cIBInputMapper();
		m_IB_input_mappers[m_cur_context_idx]->m_context = 1 << bit;
		SetGlobObjs();
		return TRUE;
	}
	// Stuffs it
	STDMETHOD_(void, GetContext)(THIS_ unsigned long* pContext)
	{
		*pContext = m_IB_input_mappers[m_cur_context_idx]->m_context;
	}

	//
	// Set globally master agg callback.
	// This is also set interally to IBMaxActiveAgg, so most don't need
	// to use this.
	//
	STDMETHOD_(void, SetMasterAggregation)(THIS_ tBindAggCallback func)
	{
		SetGlobObjs();
		m_IB_variable_manager->SetMasterAggregation(func);
	}

	//
	// Set the default variable processing callback. When a command variable
	// doesn't have its own callback, or a bound command isn't an input command
	// this callback will be used. Useful for sending most commands along to
	// a game's command system.
	//
	STDMETHOD_(void, SetMasterProcessCallback)(THIS_ tBindProcCallback func)
	{
		SetGlobObjs();
		m_IB_variable_manager->SetMasterProcessCallback(func);
	}

	//
	// Iterators for getting which controls are bound to a certain command.
	// pControlBuf really should be at least 32 chars long.
	//
	STDMETHOD_(char*, GetControlFromCmdStart)(THIS_ char* pCmd, char* pControlBuf)
	{
		SetGlobObjs();
		return m_IB_input_mappers[m_cur_context_idx]->GetControlFromCmdStart(pCmd, pControlBuf);
	}
	STDMETHOD_(char*, GetControlFromCmdNext)(THIS_ char* pControlBuf)
	{
		SetGlobObjs();
		return m_IB_input_mappers[m_cur_context_idx]->GetControlFromCmdNext(pControlBuf);
	}

	//
	// Not currently used.
	//
	STDMETHOD_(void, SetResPath)(THIS_ char* pPath)
	{
		SetGlobObjs();
		m_IB_variable_manager->SetBndSearchPath(pPath);
	}

	//
	// Useful for taking a composed control (ie. "p+shift+alt") and separating
	// then into ppControls. Maximum controls is 4 (some control+alt+crtl+shift).
	// pNumControls will be stuffed with the number of controls separated.
	//
	STDMETHOD_(char*, DecomposeControl)(THIS_ char* pControlStr, char ppControls[4][32], long* pNumControls)
	{
		SetGlobObjs();

		assert(m_IB_input_mappers[m_cur_context_idx] != NULL);
		return (char*)m_IB_input_mappers[m_cur_context_idx]->DecomposeControl(pControlStr, ppControls, pNumControls);
	}

	//
	// Clears all binds in the current context.
	//
	STDMETHOD_(void, Reset)(THIS)
	{
		cIBInputMapper* pMapper = m_IB_input_mappers[m_cur_context_idx];
		if (pMapper)
		{
			delete pMapper;
			m_IB_input_mappers[m_cur_context_idx] = new cIBInputMapper();
			m_IB_input_mappers[m_cur_context_idx]->m_context = 1 << m_cur_context_idx;
			SetGlobObjs();
		}
	}
private:
	void SetGlobObjs();

private:
	cIBVariableManager* m_IB_variable_manager;
	cIBInputMapper* m_IB_input_mappers[32];
	unsigned int m_cur_context_idx;
	cDynArray<_sBindContext> m_BndCtxtArray;
};

void cInputBinder::SetGlobObjs()
{
	g_IB_variable_manager = m_IB_variable_manager;
	g_IB_input_mapper = m_IB_input_mappers[m_cur_context_idx];
}

tResult LGAPI _CreateInputBinder(REFIID, IInputBinder** ppInputBinder, IUnknown* pOuter)
{
	assert(ppInputBinder != NULL);

	*ppInputBinder = new cInputBinder(pOuter);

	return ppInputBinder ? NOERROR : E_FAIL;
}

inline cInputBinder::cInputBinder(IUnknown* pOuter)
{
	MI_INIT_AGGREGATION_1(pOuter, IInputBinder, kPriorityNormal, NULL);
}
