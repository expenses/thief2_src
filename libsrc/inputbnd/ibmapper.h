#pragma once

#include <aatree.h>
#include <hashset.h>
#include <inpbnd_i.h>
#include <str.h>
#include <dlist.h>

struct sControlDown
{
	cAnsiStr control;
	const char* pCmd;
};

typedef cContainerDList<sControlDown, 0> cControlDownList;
typedef cContDListNode<sControlDown, 0> cControlDownNode;

typedef cContainerDList<cAnsiStr, 0> cAnsiStrList;
typedef cContDListNode<cAnsiStr, 0> cAnsiStrNode;

struct sCmdCtrlEntry
{
	cAnsiStr cmd;
	cAnsiStrList ctrlList;
};

class cIBInputMapper
{
	friend class cInputBinder;
	friend class cIBVariableManager;

public:
	cIBInputMapper();
	~cIBInputMapper();

	char* Bind(char** tokens);
	char* Unbind(char* control, char** var_name);
	char* PeekBind(char* control, int strip_control);
	void TrapBind(char* p_cmd, tTrapBindFilter filter_cb, tTrapBindPostFunc post_cb, void* data);
	bool TrapHandler(uiEvent* p_event);
	char* GetControlFromCmdStart(char* pszCmd, char* pszCtrlBuf);
	char* GetControlFromCmdNext(char* pszCtrlBuf);
	char* SaveBnd(char* filename, char* header);
	void PollAllKeys();

	static bool CDECL InputBindingHandler(uiEvent* p_event, Region* r, void* state);
	static bool CDECL StaticTrapHandler(uiEvent* p_event, Region* r, void* state);

private:
	void FlushKeys();

	int SendButtonCmd(char* control, int down, int mod);
	int AttachMods(char* control, uchar action, int strip_shift);

	char* GetRawControl(short scancode, int shifted, int* strip_shift);
	uint8 GetControlMask(char(*controls)[32], int num_controls);
	uint8 GetStateMask();

	char* DecomposeControl(char* control_str, char controls[][32], long* num_controls);
	void StripControl(char* dest, char* src);
	void RecomposeControl(char* control_str, char controls[][32], long num_controls);

	int ProcessRawKey(short scancode, uchar action, int polling);
	int ProcessRawMod(char* mod, uchar action, int general);
	int ProcessCookedKey(short code);
	int ProcessMouseButton(uiMouseEvent* event);
	int ProcessMouseMove(uiMouseEvent* event);
	int ProcessJoystick(uiJoyEvent* event);

	void StripSendDoubleCmd(char* control, double a);

public:
	aatree<char> m_control_binds;
	char m_misc_str[128];
	unsigned int m_context;
	unsigned int m_mod_states;
	cIBJoyAxisProcess* m_joyproc;
	bool m_trapping;
	char* m_bind_cmd;
	tTrapBindFilter m_filter_cb;
	tTrapBindPostFunc m_post_cb;
	void* m_filter_data;
	cAnsiStrNode* m_pCtrlIterNode;
	unsigned int m_valid_events;
};
