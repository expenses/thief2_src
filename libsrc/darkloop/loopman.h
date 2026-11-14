#pragma once

#include <loopapi.h>
#include <loopstk.h>
#include <loopfact.h>
#include <aggmemb.h>
#include <interset.h>

#define kLoopRunning        0x01
#define kLoopPaused         0x02
#define kLoopPauseRequest   0x04
#define kLoopUnpauseRequest 0x08
#define kLoopNewMinorMode   0x10
#define kLoopNewMode        0x20
#define kLoopSuspend        0x40
#define kLoopResume         0x80

class cLoopManager : public ILoopManager
{
public:
	cLoopManager(IUnknown* pOuterUnknown, unsigned nMaxModes);

	virtual ~cLoopManager();

private:

	// Add a client
	STDMETHOD(AddClient)(THIS_ ILoopClient*, ulong* pCookie);

	// Remove a client
	STDMETHOD(RemoveClient)(THIS_ ulong cookie);

	// Add a client factory
	STDMETHOD(AddClientFactory)(THIS_ ILoopClientFactory*, ulong* pCookie);

	// Remove a client factory
	STDMETHOD(RemoveClientFactory)(THIS_ ulong cookie);

	// Find/create a client
	STDMETHOD(GetClient)(THIS_ tLoopClientID*, tLoopClientData, ILoopClient**);

	// Add a mode
	STDMETHOD(AddMode)(THIS_ const sLoopModeDesc*);

	// Get a mode
	STDMETHOD_(ILoopMode*, GetMode)(THIS_ tLoopModeID*);

	// Remove a mode
	STDMETHOD(RemoveMode)(THIS_ tLoopModeID*);

	// Set/Get the elements shared by all modes
	STDMETHOD(SetBaseMode)(THIS_ tLoopModeID*);
	STDMETHOD_(ILoopMode*, GetBaseMode)(THIS);

	DECLARE_AGGREGATION(cLoopManager);

	HRESULT Init() { return S_OK; }
	HRESULT End();

private:
	class cLoop : public ILoop
	{
	public:
		static cLoop* gm_pLoop;

	public:
		// Constructor
		cLoop(IUnknown* pOuterUnknown, cLoopManager* pLoopManager);
		virtual ~cLoop();

		DECLARE_DELEGATION(cLoop);

	private:


		// Run the loop
		STDMETHOD_(int, Go)(THIS_ sLoopInstantiator* loop);


		// Unwind stack and exit loop at end of current loop iteration
		STDMETHOD(EndAllModes)(THIS_ int goRetVal);


		// Clean up all outstanding loops and prepare to shut down the app.
		// Only intended to be used for cleaning up state right before calling exit();
		STDMETHOD(Terminate)(THIS);


		// Query for current frame info. Values undefined if not actually looping
		STDMETHOD_(const sLoopFrameInfo*, GetFrameInfo)(THIS);


		// Change mode at end of current loop iteration
		STDMETHOD(ChangeMode)(THIS_ eLoopModeChangeKind, sLoopInstantiator* loop);


		// End current mode at end of current loop iteration,
		// exit loop if current mode is bottom of stack
		STDMETHOD(EndMode)(THIS_ int goRetVal);


		// Get the currently running mode
		STDMETHOD_(ILoopMode*, GetCurrentMode)(THIS);


		// Get the current mode's dispatcher
		STDMETHOD_(ILoopDispatch*, GetCurrentDispatch)(THIS);


		// Pause/unpause the game
		STDMETHOD_(void, Pause)(THIS_ BOOL);
		STDMETHOD_(BOOL, IsPaused)(THIS);

		// Change the minor mode
		STDMETHOD(ChangeMinorMode)(THIS_ int minorMode);
		STDMETHOD_(int, GetMinorMode)(THIS);

		// Convenience functions to dispatch messsages to current loop mode dispatcher
		STDMETHOD(SendMessage)(THIS_ eLoopMessage, tLoopMessageData hData, int flags);
		STDMETHOD(SendSimpleMessage)(THIS_ eLoopMessage);
		STDMETHOD(PostMessage)(THIS_ eLoopMessage, tLoopMessageData hData, int flags);
		STDMETHOD(PostSimpleMessage)(THIS_ eLoopMessage);
		STDMETHOD(ProcessQueue)(THIS);

		// Debugging features
	#ifndef SHIP
		STDMETHOD_(void, SetDiagnostics)(THIS_ uint fDiagnostics, tLoopMessageSet messages);
		STDMETHOD_(void, GetDiagnostics)(THIS_ uint* pfDiagnostics, tLoopMessageSet* pMessages);

		// Set messages and optional client to use the profileable dispatcher
		STDMETHOD_(void, SetProfile)(THIS_ tLoopMessageSet messages, tLoopClientID*);
		STDMETHOD_(void, GetProfile)(THIS_ tLoopMessageSet* pMessages, tLoopClientID**);
	#endif

	public:
		static CDECL void OnExit();

	private:
		ILoopDispatch* m_pCurrentDispatch;
		unsigned int m_fState;
		sLoopFrameInfo m_FrameInfo;

		int m_fNewMinorMode;
		int m_fGoReturn;

		ILoopManager* m_pLoopManager;
		cLoopModeStack* m_pLoopStack;
		ILoopDispatch* m_pNextDispatch;

	#ifndef SHIP
		uint m_fTempDiagnostics;
		ulong m_tempDiagnosticSet;

		ulong m_TempProfileSet;
		tLoopClientID* m_pTempProfileClientId;
	#endif
	};

	cLoop m_Loop;
	uint m_nMaxModes;
	cLoopClientFactory m_Factory;
	cInterfaceTable m_nLoopModes;
	tLoopModeID* m_pBaseMode;
};

inline cLoopManager::cLoop::cLoop(IUnknown* pOuterUnknown, cLoopManager* pLoopManager)
	: m_pCurrentDispatch(NULL),
	m_fState(0),
	m_fGoReturn(0),
	m_pLoopManager(pLoopManager),
	m_pLoopStack(new cLoopModeStack()),
	m_pNextDispatch(NULL)
#ifndef SHIP
	,
	m_fTempDiagnostics(0),
	m_tempDiagnosticSet(0),
	m_TempProfileSet(0),
	m_pTempProfileClientId(NULL)
#endif
{
	INIT_DELEGATION(pOuterUnknown);

	m_FrameInfo.fMinorMode = m_FrameInfo.nCount = 0;
	gm_pLoop = this;
}
