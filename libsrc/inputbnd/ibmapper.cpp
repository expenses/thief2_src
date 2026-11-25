#include <math.h>
#include <string.h>

#include <str.h>

#include <hashset.h>
#include <hshstimp.h>
#include <hshsttem.h>

#include <dbmem.h>

#include "ibmapper.h"
#include <keydefs.h>
#include <ibvarman.h>
#include <lg_file.h>
#include <config.h>

extern cIBVariableManager* g_IB_variable_manager;
static void _RemoveCmdCtrlNode(cAnsiStrList* pList, char* pszCtrl);
#define IsCtrlModifier(s) (!strcmp(s, "alt") || !strcmp(s, "ctrl") || !strcmp(s, "shift"))
static double ShortScaleDouble(short m);

typedef struct _input_code
{
	char* name;
	short code;
} input_code;

EXTERN int kbd_modifier_state;
cIBInputMapper* g_IB_input_mapper;


static aatree<short> g_input_codes;
static aatree<char> g_input_controls[3];
static aatree<unsigned char> g_input_down;
static cControlDownList g_ctrldown_list;


static int g_id;
static short g_prev_mousex;
static short g_prev_mousey;
static unsigned short g_shift_to_scan[KB_CNV_TBLSIZE];

#define MASK_CAPS_LOCK 0x000004
#define MASK_LSHIFT    0x000008
#define MASK_RSHIFT    0x000010
#define MASK_SHIFT     (MASK_LSHIFT | MASK_RSHIFT)
#define MASK_LCTRL     0x000020
#define MASK_RCTRL     0x000040
#define MASK_CTRL      (MASK_LCTRL | MASK_RCTRL)
#define MASK_LALT      0x000080
#define MASK_RALT      0x000100
#define MASK_ALT       (MASK_LALT | MASK_RALT)

#define SCANCODE_LCTRL 29
#define SCANCODE_RCTRL 157
#define SCANCODE_LSHIFT 42
#define SCANCODE_RSHIFT 54
#define SCANCODE_LALT 56
#define SCANCODE_RALT 184
#define SCANCODE_CAPS_LOCK 58

input_code g_valid_input_controls[202] = {
	{"esc", KEY_ESC},
	{"pause", KEY_PAUSE},
	{"print_screen", KEY_PRNTSCRN},
	{"scroll_lock", KEY_SCRLK},
	{"1", 0},
	{"2", 0},
	{"3", 0},
	{"4", 0},
	{"5", 0},
	{"6", 0},
	{"7", 0},
	{"8", 0},
	{"9", 0},
	{"0", 0},
	{"!", 0},
	{"@", 0},
	{"#", 0},
	{"$", 0},
	{"%", 0},
	{"^", 0},
	{"&", 0},
	{"*", 0},
	{"(", 0},
	{")", 0},
	{"a", 0},
	{"b", 0},
	{"c", 0},
	{"d", 0},
	{"e", 0},
	{"f", 0},
	{"g", 0},
	{"h", 0},
	{"i", 0},
	{"j", 0},
	{"k", 0},
	{"l", 0},
	{"m", 0},
	{"n", 0},
	{"o", 0},
	{"p", 0},
	{"q", 0},
	{"r", 0},
	{"s", 0},
	{"t", 0},
	{"u", 0},
	{"v", 0},
	{"w", 0},
	{"x", 0},
	{"y", 0},
	{"z", 0},
	{"`", 0},
	{"-", 0},
	{"=", 0},
	{"[", 0},
	{"]", 0},
	{"\\", 0},
	{";", 0},
	{"'", 0},
	{",", 0},
	{".", 0},
	{"/", 0},
	{"~", 0},
	{"_", 0},
	{"+", 0},
	{"{", 0},
	{"}", 0},
	{"|", 0},
	{":", 0},
	{"quote", '"'},
	{"<", 0},
	{">", 0},
	{"?", 0},
	{"\x81", 0}, // ü
	{"\x82", 0}, // é
	{"\x80", 0}, // Ç
	{"\x83", 0}, // â
	{"\x84", 0}, // ä
	{"\x85", 0}, // à
	{"\x86", 0}, // å
	{"\x87", 0}, // ç
	{"\x88", 0}, // ê
	{"\x89", 0}, // ë
	{"\x8A", 0}, // è
	{"\x8B", 0}, // ï
	{"\x8C", 0}, // î
	{"\x8D", 0}, // ì
	{"\x93", 0}, // ô
	{"\x94", 0}, // ö
	{"\x95", 0}, // ò
	{"\x96", 0}, // û
	{"\x97", 0}, // ù
	{"\x98", 0}, // ÿ
	{"super_2", 253},
	{"\x0E1", 0}, // ß
	{"f1", KEY_F1},
	{"f2", KEY_F2},
	{"f3", KEY_F3},
	{"f4", KEY_F4},
	{"f5", KEY_F5},
	{"f6", KEY_F6},
	{"f7", KEY_F7},
	{"f8", KEY_F8},
	{"f9", KEY_F9},
	{"f10", KEY_F10},
	{"f11", KEY_F11},
	{"f12", KEY_F12},
	{"backspace", KEY_BS},
	{"tab", KEY_TAB},
	{"enter", KEY_ENTER},
	{"space", KEY_SPACE},
	{"ins", KEY_INS},
	{"del", KEY_DEL},
	{"home", KEY_HOME},
	{"end", KEY_END},
	{"pgup", KEY_PGUP},
	{"pgdn", KEY_PGDN},
	{"up", KEY_UP},
	{"down", KEY_DOWN},
	{"left", KEY_LEFT},
	{"right", KEY_RIGHT},
	{"keypad_ins", KEY_PAD_INS},
	{"keypad_del", KEY_PAD_DEL},
	{"keypad_end", KEY_PAD_END},
	{"keypad_down", KEY_PAD_DOWN},
	{"keypad_pgdn", KEY_PAD_PGDN},
	{"keypad_left", KEY_PAD_LEFT},
	{"keypad_center", KEY_PAD_CENTER},
	{"keypad_right", KEY_PAD_RIGHT},
	{"keypad_home", KEY_PAD_HOME},
	{"keypad_up", KEY_PAD_UP},
	{"keypad_pgup", KEY_PAD_PGUP},
	{"keypad_slash", KEY_GREY_SLASH},
	{"keypad_star", KEY_GREY_STAR},
	{"keypad_minus", KEY_GREY_MINUS},
	{"keypad_plus", KEY_GREY_PLUS},
	{"keypad_enter", KEY_GREY_ENTER},
	{"alt", -1},
	{"ctrl", -1},
	{"shift", -1},
	{"lalt", -1},
	{"lctrl", -1},
	{"rshift", -1},
	{"ralt", -1},
	{"rctrl", -1},
	{"rshift", -1},
	{"mouse1", MOUSE_LDOWN},
	{"mouse2", MOUSE_RDOWN},
	{"mouse3", MOUSE_CDOWN},
	{"mouse4", MOUSE_X1DOWN},
	{"mouse5", MOUSE_X2DOWN},
	{"mouse_axisx", -1},
	{"mouse_axisy", -1},
	{"mouse_axisz", -1},
	{"mouse_wheel", -1},
	{"joy1", 0},
	{"joy2", 0},
	{"joy3", 0},
	{"joy4", 0},
	{"joy5", 0},
	{"joy6", 0},
	{"joy7", 0},
	{"joy8", 0},
	{"joy9", 0},
	{"joy10", 0},
	{"joy11", 0},
	{"joy12", 0},
	{"joy13", 0},
	{"joy14", 0},
	{"joy15", 0},
	{"joy16", 0},
	{"joy17", 0},
	{"joy18", 0},
	{"joy19", 0},
	{"joy20", 0},
	{"joy21", 0},
	{"joy22", 0},
	{"joy23", 0},
	{"joy24", 0},
	{"joy25", 0},
	{"joy26", 0},
	{"joy27", 0},
	{"joy28", 0},
	{"joy29", 0},
	{"joy30", 0},
	{"joy_axisx", -1},
	{"joy_axisy", -1},
	{"joy_axisz", -1},
	{"joy_axisr", -1},
	{"joy_hataxisx", -1},
	{"joy_hataxisy", -1},
	{"joy_hatup", -1},
	{"joy_hatdn", -1},
	{"joy_hatrt", -1},
	{"joy_hatlt", -1},
	{NULL, 0} };

static char* g_err_msg[] = {
	"Invalid input control",
	"Multiple non-modifier controls are not allowed",
	"A non-modifier control is needed in compound bindings",
	"Duplicate modifier",
	"Error opening file",
	"Command control string malformed",
};

char g_mods[3][3][8] = {
	{
		"+shift", "+lshift", "+rshift"
	},
	{
		"+ctrl", "+lctrl", "+rctrl",
	},
	{
		"+alt", "+lalt", "+ralt"
	}
};

static double m = 2.0 / 65535.0;

#define kMaxMouseSmoothFrames 30

struct sMouseSmoothBuffer
{
	int cur;
	double samples[kMaxMouseSmoothFrames];
};

static sMouseSmoothBuffer g_MouseSmoothX;
static sMouseSmoothBuffer g_MouseSmoothY;
static int g_nMouseSmoothFrames;
static double g_MouseSmoothWeight = 0.5;

extern float g_fD8ScreenScaleX;
extern float g_fD8ScreenScaleY;

static double MouseSmooth(sMouseSmoothBuffer* buf, double val)
{
	if (g_nMouseSmoothFrames <= 1)
		return val;

	buf->cur = (buf->cur + 1) % g_nMouseSmoothFrames;
	buf->samples[buf->cur] = val;

	double sum = 0.0;
	double weights = 0.0;
	double weight = 1.0;
	for (int i = 0; i < g_nMouseSmoothFrames; ++i)
	{
		sum += buf->samples[(buf->cur + g_nMouseSmoothFrames - 1 - i) % g_nMouseSmoothFrames] * weight;
		weights += weight;
		weight *= g_MouseSmoothWeight;
	}
	return sum / weights;
}

cIBInputMapper::cIBInputMapper()
	: m_control_binds()
{
	m_mod_states = 0;
	m_trapping = false;
	m_valid_events = UINT_MAX;
	m_pCtrlIterNode = NULL;
	m_joyproc = NULL;

	memset(&g_MouseSmoothX, 0, sizeof(g_MouseSmoothX));
	memset(&g_MouseSmoothY, 0, sizeof(g_MouseSmoothY));
	g_nMouseSmoothFrames = 0;
	if (config_get_int("mouse_smoothing", &g_nMouseSmoothFrames))
	{
		if (g_nMouseSmoothFrames <= 1)
			g_nMouseSmoothFrames = 0;
		else if (g_nMouseSmoothFrames > kMaxMouseSmoothFrames)
			g_nMouseSmoothFrames = kMaxMouseSmoothFrames;

		if (g_nMouseSmoothFrames)
		{
			float weight = 0.5;
			if (config_get_float("mouse_smoothing_weight", &weight))
			{
				if (weight < 0.0)
					weight = 0.0;
				else if (weight > 1.0)
					weight = 1.0;
			}
			g_MouseSmoothWeight = weight;
		}
	}

	uiSetMouseMotionPolling(true);

	if (!g_input_codes.GetNumNodes())
	{
		for (input_code* cur_control = g_valid_input_controls; cur_control->name != NULL; ++cur_control)
		{
			short* code = new short;
			if (cur_control->code)
				*code = cur_control->code;
			else
				*code = (uchar)cur_control->name[0];

			g_input_codes.Add(cur_control->name, code, 1);

			char* control_name = new char[strlen(cur_control->name) + 2];
			strcpy(control_name, cur_control->name);

			char code_str[16];
			sprintf(code_str, "%d", *code);

			if (!strncmp("mouse", control_name, 5))
			{
				g_input_controls[1].Add(code_str, control_name, strlen(control_name) + 1);
			}
			else if (!strncmp("joy", control_name, 3))
			{
				g_input_controls[2].Add(control_name, control_name, strlen(control_name) + 1);
			}
			else
			{
				g_input_controls[0].Add(code_str, control_name, strlen(control_name) + 1);
			}
		}
	}

	for (int i = 0; i < 224; ++i)
	{
		g_shift_to_scan[i] = 0;
		for (int j = 0; j < 224; ++j)
		{
			if ((uchar)i == (uchar)kb_cnv_table[j][1])
			{
				g_shift_to_scan[i] = j;
				break;
			}
		}
	}

	g_IB_input_mapper = this;
}

cIBInputMapper::~cIBInputMapper()
{
}

char* cIBInputMapper::Bind(char** tokens)
{
	char controls[4][32];
	long num_controls;
	char* ret_val = DecomposeControl(*tokens, controls, &num_controls);
	if (ret_val)
		return ret_val;

	RecomposeControl(m_misc_str, controls, num_controls);

	delete[] tokens[0];
#line 309
	tokens[0] = new char[strlen(m_misc_str) + 1]();
	strcpy(tokens[0], m_misc_str);

	char* cmd = m_control_binds.Find(m_misc_str);
	if (!cmd)
	{
		m_control_binds.Add(m_misc_str, 0, 1);

		uchar* pDown = new uchar[1];
		*pDown = 0;
		g_input_down.Add(m_misc_str, pDown, 1);
	}
	else
	{
		delete[] cmd;
	}

	cmd = tokens[1];
	sCmdCtrlEntry* pCmdCtrlEntry = m_CmdCtrlHash.Search(cmd);
	if (!pCmdCtrlEntry)
	{
		pCmdCtrlEntry = new sCmdCtrlEntry();
		pCmdCtrlEntry->cmd = cmd;
		m_CmdCtrlHash.Insert(pCmdCtrlEntry);
	}
	_RemoveCmdCtrlNode(&pCmdCtrlEntry->ctrlList, m_misc_str);

	cAnsiStrNode* pNode = new cAnsiStrNode();
	pNode->item = m_misc_str;
	pCmdCtrlEntry->ctrlList.Append(pNode);



	cmd = new char[strlen(" control: ") + strlen(tokens[1]) + strlen(m_misc_str) + 1];
	strcpy(cmd, tokens[1]);
	strcat(cmd, " control: ");
	strcat(cmd, m_misc_str);
	m_control_binds.ChangeInfo(m_misc_str, cmd, strlen(cmd) + 1);
	return 0;
}

static void _RemoveCmdCtrlNode(cAnsiStrList* pList, char* pszCtrl)
{
	cAnsiStrNode* pNode;
	for (pNode = pList->GetFirst(); pNode != NULL; pNode = pNode->GetNext())
	{
		if (pNode->item == pszCtrl)
			break;
	}
	if (pNode)
		delete pList->Remove(pNode);
}

char* cIBInputMapper::Unbind(char* control, char** var_name)
{
	var_name[0][0] = '\0';

	char controls[4][32];
	long num_controls;
	char* ret_val = DecomposeControl(control, controls, &num_controls);
	if (ret_val)
		return ret_val;

	RecomposeControl(m_misc_str, controls, num_controls);
	char* cmd = m_control_binds.Find(m_misc_str);
	if (!cmd)
		return 0;

	char pszCmd[256];
	strcpy(pszCmd, cmd);

	char* pStr = strstr(pszCmd, " control: ");
	if (pStr)
	{
		*pStr = '\0';

		sCmdCtrlEntry* pCmdCtrlEntry = m_CmdCtrlHash.Search(pszCmd);
		if (pCmdCtrlEntry)
			_RemoveCmdCtrlNode(&pCmdCtrlEntry->ctrlList, m_misc_str);

		long num_tokens;
		char** tokens = g_IB_variable_manager->Tokenize(cmd, &num_tokens, 1);
		char first = tokens[0][0];
		int active_toggle = (first == '+' || first == '=');
		strcpy(*var_name, &(*tokens)[active_toggle]);

		m_control_binds.Delete(m_misc_str, 1, NULL);
		g_input_down.Delete(m_misc_str, 1, NULL);
		g_IB_variable_manager->FreeTokens(tokens, num_tokens);
		return 0;
	}
	return g_err_msg[5];
}

char* cIBInputMapper::PeekBind(char* control, int strip_control)
{
	char controls[4][32];
	long num_controls;
	char* ret_val = DecomposeControl(control, controls, &num_controls);
	if (ret_val)
		return ret_val;

	RecomposeControl(m_misc_str, controls, num_controls);

	char* cmd = m_control_binds.Find(m_misc_str);
	if (!cmd)
		return 0;
	if (strip_control)
		StripControl(m_misc_str, cmd);

	return m_misc_str;
}

void cIBInputMapper::TrapBind(char* p_cmd, tTrapBindFilter filter_cb, tTrapBindPostFunc post_cb, void* data)
{
	if (m_trapping)
		return;

	m_trapping = true;

	Rect rect;
	rect.ul.x = 0;
	rect.ul.y = 0;
	rect.lr.x = grd_canvas->bm.w;
	rect.lr.y = grd_canvas->bm.h;

	mouse_get_xy(&g_prev_mousex, &g_prev_mousey);
	uiHideMouse(&rect);

	uiSlab* slab;
	uiGetCurrentSlab(&slab);
	uiInstallRegionHandler(slab->creg, SCANCODE_LCTRL, cIBInputMapper::StaticTrapHandler, NULL, &g_id);

	m_bind_cmd = p_cmd;
	m_filter_cb = filter_cb;
	m_post_cb = post_cb;
	m_filter_data = data;
	m_mod_states = 0;
}

bool CDECL cIBInputMapper::StaticTrapHandler(uiEvent* p_event, Region* r, void* state)
{
	return g_IB_input_mapper->TrapHandler(p_event);
}

bool cIBInputMapper::TrapHandler(uiEvent* p_event)
{
	char bind_control[128];

	switch (p_event->type)
	{
	case UI_EVENT_KBD_RAW: {
		uiRawKeyEvent* key_event = reinterpret_cast<uiRawKeyEvent*>(p_event);
		uint mask = 0;
		int shifted = (m_mod_states & MASK_LSHIFT || m_mod_states & MASK_RSHIFT) ^ ((m_mod_states & MASK_CAPS_LOCK) >> 2);
		int strip_shift;
		char* control = GetRawControl(key_event->scancode, shifted, &strip_shift);
		switch (key_event->scancode)
		{
		case SCANCODE_LSHIFT:
		case SCANCODE_RSHIFT:
			mask = MASK_SHIFT;
			break;
		case SCANCODE_LALT:
		case SCANCODE_RALT:
			mask = MASK_ALT;
			break;
		case SCANCODE_LCTRL:
		case SCANCODE_RCTRL:
			mask = MASK_CTRL;
			break;
		default:
			break;
		}

		if (control || mask)
		{
			if (key_event->action)
			{
				m_mod_states |= mask;
				return true;
			}
			{
				if (mask)
				{
					if (m_mod_states & MASK_LALT)
					{
						strcpy(bind_control, "alt");
					}
					else if (m_mod_states & MASK_LCTRL)
					{
						strcpy(bind_control, "ctrl");
					}
					else if (m_mod_states & MASK_LSHIFT)
					{
						strcpy(bind_control, "shift");
					}
				}
				else
				{
					strcpy(bind_control, control);
					if ((m_mod_states & MASK_LALT) != 0)
						strcat(bind_control, "+alt");
					if ((m_mod_states & MASK_LCTRL) != 0)
						strcat(bind_control, "+ctrl");
					if ((m_mod_states & MASK_LSHIFT) != 0 && !strip_shift)
						strcat(bind_control, "+shift");
				}
				break;
			}
		}

		return true;
	}
	case UI_EVENT_MOUSE: {
		uiMouseEvent* mouse_event = reinterpret_cast<uiMouseEvent*>(p_event);
		short action = p_event->subtype & 0xFF;
		if (action || mouse_event->wheel)
		{
			if (mouse_event->wheel && !action)
			{
				if (config_is_defined("analog_mwheel_bind"))
				{
					strcpy(bind_control, "mouse_wheel");
					break;
				}
				strcpy(bind_control, (mouse_event->wheel > 0) ? "mouse_wheel_up" : "mouse_wheel_down");
				if ((m_mod_states & MASK_LALT) != 0)
					strcat(bind_control, "+alt");
				if ((m_mod_states & MASK_LCTRL) != 0)
					strcat(bind_control, "+ctrl");
				if ((m_mod_states & MASK_LSHIFT) != 0)
					strcat(bind_control, "+shift");
				break;
			}
			char butt_str[32];
			sprintf(butt_str, "%d", action);
			char* control = g_input_controls[1].Find(butt_str);
			if (control)
			{
				strcpy(bind_control, control);
				break;
			}
		}
		return true;
	}
	case UI_EVENT_MOUSE_MOVE: {
		return true;
	}
	case UI_EVENT_JOY: {
		uiJoyEvent* event = reinterpret_cast<uiJoyEvent*>(p_event);
		if (p_event->subtype == 3)
		{
			char str[32];
			sprintf(str, "joy%d", event->joynum + 1);
			char* control = g_input_controls[2].Find(str);
			if (control)
			{
				strcpy(bind_control, control);
				break;
			}
		}
		else
		{
			if (event->joynum == 2)
			{
				if (event->joypos.y == 1)
				{
					strcpy(bind_control, "joy_hatup");
				}
				else if (event->joypos.y == -1)
				{
					strcpy(bind_control, "joy_hatdn");
				}
				else if (event->joypos.x == -1)
				{
					strcpy(bind_control, "joy_hatlt");
				}
				else if (event->joypos.x == 1)
				{
					strcpy(bind_control, "joy_hatrt");
				}
				break;
			}
		}
		return true;
	}
	}


	BOOL bound = FALSE;
	if (m_filter_cb == NULL || m_filter_cb(bind_control, (char*)m_bind_cmd, m_filter_data))
	{
		bound = TRUE;
		char cmd[128];
		strcpy(cmd, "bind ");
		strcat(cmd, bind_control);
		strcat(cmd, " ");

		long num_tokens;
		char** tokens = g_IB_variable_manager->Tokenize(m_bind_cmd, &num_tokens, false);
		g_IB_variable_manager->FreeTokens(tokens, num_tokens);
		if (num_tokens > 1)
		{
			strcat(cmd, "\"");
			strcat(cmd, m_bind_cmd);
			strcat(cmd, "\"");
		}
		else
		{
			strcat(cmd, m_bind_cmd);
		}

		g_IB_variable_manager->Cmd(cmd, false);
	}

	m_trapping = false;

	Rect rect;
	rect.ul.x = 0;
	rect.ul.y = 0;
	rect.lr.x = grd_canvas->bm.w;
	rect.lr.y = grd_canvas->bm.h;

	uiShowMouse(&rect);
	mouse_put_xy(g_prev_mousex, g_prev_mousey);

	uiSlab* slab;
	uiGetCurrentSlab(&slab);
	uiRemoveRegionHandler(slab->creg, g_id);
	m_mod_states = 0;
	if (m_post_cb)
		m_post_cb(bound);

	return true;
}

char* cIBInputMapper::GetControlFromCmdStart(char* pszCmd, char* pszCtrlBuf)
{
	pszCtrlBuf[0] = '\0';

	sCmdCtrlEntry* pCmdCtrlEntry = m_CmdCtrlHash.Search(pszCmd);
	if (pCmdCtrlEntry)
	{
		m_pCtrlIterNode = pCmdCtrlEntry->ctrlList.GetFirst();
		if (m_pCtrlIterNode)
		{
			strcpy(pszCtrlBuf, m_pCtrlIterNode->item);
			m_pCtrlIterNode = m_pCtrlIterNode->GetNext();
		}
	}

	return pszCtrlBuf;
}

char* cIBInputMapper::GetControlFromCmdNext(char* pszCtrlBuf)
{
	pszCtrlBuf[0] = '\0';

	if (m_pCtrlIterNode)
	{
		strcpy(pszCtrlBuf, m_pCtrlIterNode->item);
		m_pCtrlIterNode = m_pCtrlIterNode->GetNext();
	}

	return pszCtrlBuf;
}

char* cIBInputMapper::SaveBnd(char* filename, char* header)
{
	LGFILE* fp;
	char** tokens;
	char control_name[64];
	long num_tokens;

	fp = lg_fopen(filename, "wt");
	if (!fp)
		return g_err_msg[4];

	if (header)
		lg_fprintf(fp, "%s\n\n", header);

	m_control_binds.ResetVisited(NULL);
	while (true)
	{
		char* cmd = m_control_binds.GetNextInOrder(control_name);
		if (!cmd)
			break;

		if (*cmd != '\0')
		{
			StripControl(m_misc_str, cmd);
			tokens = g_IB_variable_manager->Tokenize(m_misc_str, &num_tokens, 0);
			g_IB_variable_manager->FreeTokens(tokens, num_tokens);
			if (num_tokens > 1)
				lg_fprintf(fp, "bind %s \"%s\"\r\n", control_name, m_misc_str);
			else
				lg_fprintf(fp, "bind %s %s\r\n", control_name, m_misc_str);
		}
	}

	lg_fclose(fp);

	return NULL;
}

void cIBInputMapper::PollAllKeys()
{
	ushort cooked;
	bool results;

	FlushKeys();
	kbs_event keycode;
	keycode.state = 0;
	for (int i = 0; i < 224; ++i)
	{
		keycode.code = i;
		kb_cook_real(keycode, &cooked, &results, 0); // fixme: assert on failure
		ProcessRawKey(i, kb_state(cooked), 1);
	}
	kbd_modifier_state = 0;
}

void cIBInputMapper::FlushKeys()
{
	kb_flush();

	g_input_down.ResetVisited(NULL);
	uchar* down;
	while (down = g_input_down.GetNextInOrder(NULL))
	{
		*down = 0;
	}
}

bool CDECL cIBInputMapper::InputBindingHandler(uiEvent* p_event, Region* r, void* state)
{
	uint valid_events = g_IB_input_mapper->m_valid_events;
	uchar ret = 0;

	switch (p_event->type)
	{
	case UI_EVENT_KBD_COOKED:
		if (valid_events & UI_EVENT_KBD_COOKED)
			ret = g_IB_input_mapper->ProcessCookedKey(p_event->subtype);
		break;
	case UI_EVENT_KBD_RAW:
		if (valid_events & UI_EVENT_KBD_RAW)
			ret = g_IB_input_mapper->ProcessRawKey(p_event->subtype, p_event->data[0], 0);
		break;
	case UI_EVENT_MOUSE:
		if (valid_events & UI_EVENT_MOUSE)
			ret = g_IB_input_mapper->ProcessMouseButton(reinterpret_cast<uiMouseEvent*>(p_event));
		break;
	case UI_EVENT_MOUSE_MOVE:
		if (valid_events & UI_EVENT_MOUSE_MOVE)
			ret = g_IB_input_mapper->ProcessMouseMove(reinterpret_cast<uiMouseEvent*>(p_event));
		break;
	case UI_EVENT_JOY:
		if (valid_events & UI_EVENT_JOY)
			ret = g_IB_input_mapper->ProcessJoystick(reinterpret_cast<uiJoyEvent*>(p_event));
		break;
	}

	return ret;
}

cControlDownNode* GetControlDownNode(char* pControl)
{
	cControlDownNode* pNode = g_ctrldown_list.GetFirst();
	while (pNode)
	{
		if (pNode->item.control == pControl)
			break;
		pNode = pNode->GetNext();
	}

	return pNode;
}

int cIBInputMapper::SendButtonCmd(char* control, int down, int mod)
{
	int ret_val = 1;
	uchar* already_down = g_input_down.Find(control);
	if (down)
	{
		char* cmd = m_control_binds.Find(control);
		if (cmd)
		{
			if (!GetControlDownNode(control))
			{
				cControlDownNode* node = new cControlDownNode;
				node->item.control = control;
				node->item.pCmd = cmd;
				g_ctrldown_list.Prepend(node);
			}

			g_IB_variable_manager->Cmd(cmd, *already_down);
		}
		else
		{
			ret_val = 0;
		}

		if (already_down)
			*already_down = true;
	}
	else
	{
		if (already_down)
			*already_down = false;
		cControlDownNode* node = GetControlDownNode(control);
		char* pCmd;
		if (node && (pCmd = (char*)node->item.pCmd, delete g_ctrldown_list.Remove(node), pCmd))
		{
			if (pCmd[0] == '+')
			{
				pCmd[0] = '-';
				g_IB_variable_manager->Cmd(pCmd, *already_down);
				pCmd[0] = '+';
			}
			else if (pCmd[0] == '-')
			{
				pCmd[0] = '+';
				g_IB_variable_manager->Cmd(pCmd, *already_down);
				pCmd[0] = '-';
			}
		}
		else
		{
			ret_val = 0;
		}
	}

	if (mod)
	{
		uint mask = 0;
		if (!strcmp("shift", control))
		{
			mask = MASK_SHIFT;
		}
		else if (!strcmp("lshift", control))
		{
			mask = MASK_LSHIFT;
		}
		else if (!strcmp("rshift", control))
		{
			mask = MASK_RSHIFT;
		}
		else if (!strcmp("alt", control))
		{
			mask = MASK_ALT;
		}
		else if (!strcmp("lalt", control))
		{
			mask = MASK_LALT;
		}
		else if (!strcmp("ralt", control))
		{
			mask = MASK_RALT;
		}
		else if (!strcmp("ctrl", control))
		{
			mask = MASK_CTRL;
		}
		else if (!strcmp("lctrl", control))
		{
			mask = MASK_LCTRL;
		}
		else if (!strcmp("caps_lock", control))
		{
			m_mod_states ^= MASK_CAPS_LOCK;
			// fixme: control instead of shift??
			SendButtonCmd("shift", (m_mod_states & MASK_CAPS_LOCK) != 0, 0);
		}

		if (down)
			m_mod_states |= mask;
		else
			m_mod_states &= ~mask;
	}

	return ret_val;
}

int cIBInputMapper::ProcessRawKey(short scancode, uchar action, int polling)
{
	switch (scancode)
	{
	case SCANCODE_LSHIFT:
		if (m_mod_states & MASK_CAPS_LOCK)
			return 0;
		if (m_mod_states & MASK_LSHIFT && action)
			return 0;
		return ProcessRawMod("lshift", action, FALSE);
		break;
	case SCANCODE_RSHIFT:
		if (m_mod_states & MASK_CAPS_LOCK)
			return 0;
		if (m_mod_states & MASK_RSHIFT && action)
			return 0;
		return ProcessRawMod("rshift", action, FALSE);
		break;
	case SCANCODE_LCTRL:
		if (m_mod_states & MASK_LCTRL && action)
			return 0;
		return ProcessRawMod("lctrl", action, FALSE);
		break;
	case SCANCODE_RCTRL:
		if (m_mod_states & MASK_RCTRL && action)
			return 0;
		return ProcessRawMod("rctrl", action, FALSE);
		break;
	case SCANCODE_LALT:
		if (m_mod_states & MASK_LALT && action)
			return 0;
		return ProcessRawMod("lalt", action, FALSE);
		break;
	case SCANCODE_RALT:
		if (m_mod_states & MASK_RALT && action)
			return 0;
		return ProcessRawMod("ralt", action, FALSE);
		break;
	case SCANCODE_CAPS_LOCK:
		if (m_mod_states & MASK_SHIFT || action || polling)
			return 0;
		return ProcessRawMod("caps_lock", 0, TRUE);
		break;
	default:
		int strip_shift;
		char* control = GetRawControl(scancode, (m_mod_states & MASK_LSHIFT || m_mod_states & MASK_RSHIFT) ^ ((m_mod_states & MASK_CAPS_LOCK) >> 2), &strip_shift);
		if (control == NULL)
			break;

		uchar* down = g_input_down.Find(control);
		if (down)
		{
			if (*down != FALSE)
				*down = FALSE;
			else
				*down = action;
		}
		return AttachMods(control, action, strip_shift);
		break;
	}
	return 0;
}


char* cIBInputMapper::GetRawControl(short scancode, int shifted, int* strip_shift)
{
	ushort unshifted_key = kb_cnv_table[scancode][KB_CNV_NOSHIFT];
	ushort cnv = shifted ? kb_cnv_table[scancode][KB_CNV_SHIFT] : unshifted_key;

	*strip_shift = 0;

	if (shifted)
	{
		if ((cnv & KB_FLAG_SPECIAL) == 0 && (cnv & 0xff) >= 'A' && (cnv & 0xff) <= 'Z')
		{
		    cnv += 'a' - 'A';
		}
		else
		{
    		if (cnv != unshifted_key && (cnv & CNV_NUM) == 0 && (uchar)(cnv) != '\t')
    			*strip_shift = 1;
		}
	}

	if ((cnv & CNV_NUM) != 0)
	{
		cnv = scancode | KB_FLAG_SPECIAL;
	}
	else
	{
		cnv &= ~(KB_FLAG_DOWN | CNV_CTRL | CNV_ALT | CNV_SHIFT | CNV_NUM | CNV_CAPS);
	}

	char key_str[32];
	sprintf(key_str, "%d", cnv);
	return g_input_controls[0].Find(key_str);
}

int cIBInputMapper::AttachMods(char* control, uchar action, int strip_shift)
{
	static int valid_mod_fields[3][3];
	valid_mod_fields[0][0] = ((m_mod_states & MASK_LSHIFT || m_mod_states & MASK_RSHIFT) ^ ((m_mod_states & MASK_CAPS_LOCK) >> 2)) && !strip_shift;
	valid_mod_fields[0][1] = ((m_mod_states & MASK_CAPS_LOCK) ^ ((m_mod_states & MASK_LSHIFT) >> 1)) && !strip_shift;
	valid_mod_fields[0][2] = ((m_mod_states & MASK_CAPS_LOCK) ^ ((m_mod_states & MASK_RSHIFT) >> 2)) && !strip_shift;
	if ((m_mod_states & MASK_LCTRL) || (m_mod_states & MASK_RCTRL))
		valid_mod_fields[1][0] = 1;
	else
		valid_mod_fields[1][0] = 0;
	valid_mod_fields[1][1] = (m_mod_states & MASK_LCTRL) >> 5;
	valid_mod_fields[1][2] = (m_mod_states & MASK_RCTRL) >> 6;
	if ((m_mod_states & MASK_LALT) || (m_mod_states & MASK_RALT))
		valid_mod_fields[2][0] = 1;
	else
		valid_mod_fields[2][0] = 0;
	valid_mod_fields[2][1] = (m_mod_states & MASK_LALT) >> 7;
	valid_mod_fields[2][2] = (m_mod_states & MASK_RALT) >> 8;

	int i;
	uint8 mod_mask = 0;
	for (i = 0; i < 3; ++i)
		mod_mask |= valid_mod_fields[i][0] << i;

	int mod_loop_num[3];
	for (i = 0; i < 3; ++i)
		mod_loop_num[i] = valid_mod_fields[i][0] != 0 ? 3 : 1;

	i = 0;
	do
	{
		if (i == 0)
		{
			if ((m_mod_states & MASK_LSHIFT || m_mod_states & MASK_RSHIFT) ^ ((m_mod_states & MASK_CAPS_LOCK) >> 2))
				break;
		}
		else if (valid_mod_fields[i][0])
			break;
		++i;
	} while (i < 3);

	while (i < 3)
	{
		for (int s = 0; s < mod_loop_num[0]; ++s)
		{
			for (int c = 0; c < mod_loop_num[1]; ++c)
			{
				for (int a = 0; a < mod_loop_num[2]; ++a)
				{
					uint8 cur_mask = valid_mod_fields[0][s];
					cur_mask |= valid_mod_fields[1][c] << 1;
					cur_mask |= valid_mod_fields[2][a] << 2;
					if (cur_mask == mod_mask)
					{
						static char final_control[64];
						strcpy(final_control, control);
						if (valid_mod_fields[2][a])
							strcat(final_control, g_mods[2][a]);
						if (valid_mod_fields[1][c])
							strcat(final_control, g_mods[1][c]);
						if (valid_mod_fields[0][s])
							strcat(final_control, g_mods[0][s]);

						uchar* down = g_input_down.Find(final_control);
						if (!action || down)
						{
							int ret_val = SendButtonCmd(final_control, action, 0);
							if (action)
								return ret_val;
						}
					}
				}
			}
		}

		for (int j = 0; j < 3; ++j)
			valid_mod_fields[i][j] = false;

		if (strip_shift && !i)
		{
			{
				short* key = g_input_codes.Find(control);
				int dummy;
				char* ret = GetRawControl(g_shift_to_scan[*key], 0, &dummy);
				if (ret)
					control = ret;
			}
		}

		++i;
		while (i < 3 && !valid_mod_fields[i][0])
			++i;
		++i;
	}

	return SendButtonCmd(control, action, 0);
}

uint8 cIBInputMapper::GetControlMask(char(*controls)[32], int num_controls)
{
	uint8 mask = 0;

	for (int i = 1; i < num_controls; ++i)
	{
		if (!strcmp(&(*controls)[32 * i], "shift") || !strcmp(&(*controls)[32 * i], "lshift") || !strcmp(&(*controls)[32 * i], "rshift"))
		{
			mask |= 1u;
		}
		else if (!strcmp(&(*controls)[32 * i], "ctrl") || !strcmp(&(*controls)[32 * i], "lctrl") || !strcmp(&(*controls)[32 * i], "rctrl"))
		{
			mask |= 2u;
		}
		else if (!strcmp(&(*controls)[32 * i], "alt") || !strcmp(&(*controls)[32 * i], "lalt") || !strcmp(&(*controls)[32 * i], "ralt"))
		{
			mask |= 4u;
		}
	}

	return mask;
}

uint8 cIBInputMapper::GetStateMask()
{
	uint8 mask = 0;
	if ((m_mod_states & MASK_LSHIFT) || (m_mod_states & MASK_RSHIFT))
		mask |= 1u;

	if ((m_mod_states & MASK_LALT) || (m_mod_states & MASK_RALT))
		mask |= 2u;

	if ((m_mod_states & MASK_LCTRL) || (m_mod_states & MASK_RCTRL))
		mask |= 4u;

	return mask;
}

int cIBInputMapper::ProcessRawMod(char* cmod, uchar action, int general)
{
	char* mod = (char*)cmod;

	if (!general)
	{
		mod[0] = mod[0] == 'r' ? 'l' : 'r';
		const uchar* down = g_input_down.Find(mod);
		if (down == NULL || down[0] == '\0')
			ProcessRawMod(&mod[1], action, TRUE);

		mod[0] = mod[0] == 'r' ? 'l' : 'r';
	}

	SendButtonCmd(mod, action, 1);
	if (!strcmp(mod, "caps_lock"))
		return 0;

	m_control_binds.ResetVisited(NULL);
	if (!action)
	{
		char control[64];
		while (m_control_binds.GetNextInOrder(control))
		{
			const uchar* down = g_input_down.Find(control);
			if (down == NULL || *down == '\0')
				continue;

			char controls[4][32];
			long num_controls;
			DecomposeControl(control, controls, &num_controls);
			for (int i = 0; i < num_controls; ++i)
			{
				if (!strcmp(controls[i], mod))
				{
					SendButtonCmd(control, 0, 0);
					break;
				}
			}
		}
	}

	return 0;
}

int cIBInputMapper::ProcessCookedKey(short code)
{
	ushort key = code & ~(KB_FLAG_DOWN | KB_FLAG_CTRL | KB_FLAG_ALT | KB_FLAG_SHIFT);
	int letter_upp = 0;
	if (key >= 'A' && key <= 'Z')
	{
		key += 'a' - 'A';
		letter_upp = 1;
	}

	char key_str[33];
	sprintf(key_str, "%d", key);
	char* control = g_input_controls[0].Find(key_str);
	if (!control)
		return 0;

	strcpy(key_str, control);
	if ((code & 0x400) == 0x400)
		strcat(key_str, "+alt");

	if ((code & 0x200) == 0x200)
		strcat(key_str, "+ctrl");

	if ((code & 0x1000) == 0x1000 || letter_upp)
		strcat(key_str, "+shift");

	return SendButtonCmd(key_str, (code & 0x100) != 0, 0);
}

int cIBInputMapper::ProcessMouseButton(uiMouseEvent* event)
{
	short action = event->action;
	if (!action && !event->wheel)
		return 0;

	char butt_str[32];
	char final[128];
	int isMouseWheel = FALSE;
	if (event->wheel && !action)
		isMouseWheel = TRUE;

	for (int i = 0; i < 16; ++i)
	{
		int action_bit = 1 << i;
		if (action & action_bit)
		{
			int ret;
			sprintf(butt_str, "%d", action_bit);
			char* control = g_input_controls[1].Find(butt_str);
			if (control)
				ret = SendButtonCmd(control, TRUE, 0);
			else
			{
				sprintf(butt_str, "%d", action_bit >> 1);
				control = g_input_controls[1].Find(butt_str);
				if (!control)
					continue;
				ret = SendButtonCmd(control, FALSE, 0);
			}
			if (ret)
				return ret;
		}
	}

	if (!isMouseWheel)
		return 0;

	char* cmd = m_control_binds.Find("mouse_wheel");
	if (cmd)
	{
		final[0] = '\0';
		StripControl(final, cmd);
		sprintf(butt_str, " %d", event->wheel);
		strcat(final, butt_str);
		g_IB_variable_manager->Cmd(final, 0);
		return 1;
	}

	char* wheel_control = (event->wheel > 0) ? "mouse_wheel_up" : "mouse_wheel_down";
	if (!m_control_binds.Find(wheel_control))
		return 0;

	int deadzone = 0;
	if (!g_IB_variable_manager->GetVarVal(final, "mwheel_deadzone"))
		deadzone = abs(atoi(final));
	if (abs(event->wheel) < deadzone)
		return 0;

	strcpy(final, wheel_control);
	if (m_mod_states & (MASK_LALT | MASK_LCTRL | MASK_LSHIFT))
	{
		if (m_mod_states & MASK_LALT)
			strcat(final, "+alt");
		if (m_mod_states & MASK_LCTRL)
			strcat(final, "+ctrl");
		if (m_mod_states & MASK_LSHIFT)
			strcat(final, "+shift");
		if (!m_control_binds.Find(final))
			strcpy(final, wheel_control);
	}
	SendButtonCmd(final, TRUE, 0);
	SendButtonCmd(final, FALSE, 0);
	return 1;
}

int cIBInputMapper::ProcessMouseMove(uiMouseEvent* event)
{
	static char str[64];
	char final[64];
	char* cmd;
	float rel;

	mouseMask &= 254;
	mouse_put_xy(grd_canvas->bm.w >> 1, grd_canvas->bm.h >> 1);

	if (event->pos.x < 0)
		event->pos.x = 0;
	else if (event->pos.x > grd_canvas->bm.w)
		event->pos.x = grd_canvas->bm.w;
	if (event->pos.y < 0)
		event->pos.y = 0;
	else if (event->pos.y > grd_canvas->bm.h)
		event->pos.y = grd_canvas->bm.h;

	cmd = g_IB_input_mapper->m_control_binds.Find("mouse_axisx");
	if (cmd)
	{
		final[0] = '\0';
		StripControl(final, cmd);
		rel = event->pos.x - (grd_canvas->bm.w >> 1);
		rel *= g_fD8ScreenScaleX;
		rel = MouseSmooth(&g_MouseSmoothX, rel);
		sprintf(str, " %8.8f", rel);
		strcat(final, str);
		g_IB_variable_manager->Cmd(final, 0);
	}

	cmd = g_IB_input_mapper->m_control_binds.Find("mouse_axisy");
	if (cmd)
	{
		final[0] = '\0';
		StripControl(final, cmd);
		rel = event->pos.y - (grd_canvas->bm.h >> 1);
		rel *= g_fD8ScreenScaleY;
		rel = MouseSmooth(&g_MouseSmoothY, rel);
		sprintf(str, " %8.8f", -rel);
		strcat(final, str);
		g_IB_variable_manager->Cmd(final, 0);
	}

	return 0;
}

void cIBInputMapper::StripSendDoubleCmd(char* control, double a)
{
	static char str[64];
	static char final[64];

	char* cmd = m_control_binds.Find(control);
	if (cmd == NULL)
		return;

	sprintf(str, " %8.8f", a);
	StripControl(final, cmd);
	strcat(final, str);
	g_IB_variable_manager->Cmd(final, 0);
}

int cIBInputMapper::ProcessJoystick(uiJoyEvent* event)
{
	if (event->action == 1 || event->action == 3)
	{
		char str[32];
		sprintf(str, "joy%d", event->joynum + 1);
		char* control = g_input_controls[2].Find(str);
		if (control == NULL)
			return 0;

		return SendButtonCmd(control, event->action == 3, 0);
	}
	else if (event->action == 0)
	{
		if (event->joynum == 0)
		{
			double x = ShortScaleDouble(event->joypos.x);
			double y = ShortScaleDouble(event->joypos.y);
			if (m_joyproc)
				m_joyproc->ProcessXY(&x, &y);

			StripSendDoubleCmd("joy_axisx", x);
			StripSendDoubleCmd("joy_axisy", y);
		}
		else if (event->joynum == 1)
		{
			double r = ShortScaleDouble(event->joypos.x);
			double z = ShortScaleDouble(event->joypos.y);
			if (m_joyproc)
				m_joyproc->ProcessZR(&z, &r);

			StripSendDoubleCmd("joy_axisz", z);
			StripSendDoubleCmd("joy_axisr", r);
		}
		else if (event->joynum == 2)
		{
			if (event->joypos.y == 1)
			{
				SendButtonCmd("joy_hatup", 1, 0);
			}
			else if (event->joypos.y == -1)
			{
				SendButtonCmd("joy_hatdn", 1, 0);
			}
			else
			{
				SendButtonCmd("joy_hatup", 0, 0);
				SendButtonCmd("joy_hatdn", 0, 0);
			}
			if (event->joypos.x == -1)
			{
				SendButtonCmd("joy_hatlt", 1, 0);
			}
			else if (event->joypos.x == 1)
			{
				SendButtonCmd("joy_hatrt", 1, 0);
			}
			else
			{
				SendButtonCmd("joy_hatlt", 0, 0);
				SendButtonCmd("joy_hatrt", 0, 0);
			}
		}
	}

	return 1;
}
static double ShortScaleDouble(short a)
{
	return (double)(a - 0x7FFF) * m + 1.0;
}

char* cIBInputMapper::DecomposeControl(char* control_str, char controls[][32], long* num_controls)
{
	char* err = NULL;
	int i, j;
	char** tmp_controls = new char*[4];
	for (i = 0; i < 4; ++i)
		tmp_controls[i] = new char[32];

	int num = 0;
	char c;
	i = 0;
	while (1)
	{
		if (*control_str == '\0')
			break;
		if ((c = *control_str) == '+' && i)
		{
			tmp_controls[num++][i] = '\0';
			if (num == 4)
				goto split_done;
			i = 0;
			c = *++control_str;
		}
		tmp_controls[num][i++] = c;
		++control_str;
	}
	tmp_controls[num++][i] = '\0';
split_done:

	*num_controls = num;
	for (i = 0; i < num; ++i)
	{
		if (!IsCtrlModifier(tmp_controls[i]))
		{
			if (i != 0)
			{
				char* tmp = tmp_controls[0];
				tmp_controls[0] = tmp_controls[i];
				tmp_controls[i] = tmp;
			}
			break;
		}
		if (num != 1 && i == num - 1)
			err = g_err_msg[2];
	}

	for (int k = num; k > 2 && !err; )
	{
		--k;
		for (j = 1; j < k; ++j)
		{
			if (!IsCtrlModifier(tmp_controls[j]) || !IsCtrlModifier(tmp_controls[j + 1]))
			{
				err = g_err_msg[1];
				break;
			}



			int cmp = strcmp(tmp_controls[j], tmp_controls[j + 1]);
			if (cmp > 0)
			{
				char* tmp = tmp_controls[j];
				tmp_controls[j] = tmp_controls[j + 1];
				tmp_controls[j + 1] = tmp;
			}
			else if (cmp == 0)
			{
				err = g_err_msg[3];
				break;
			}
		}
	}

	if (num == 2 && !IsCtrlModifier(tmp_controls[1]))
		err = g_err_msg[1];

	for (i = 0; i < num && !err; ++i)
		strcpy(controls[i], tmp_controls[i]);

	for (i = 0; i < 4; ++i)
		delete[] tmp_controls[i];
	delete[] tmp_controls;

	for (i = num - 1; i >= 0; --i)
	{
		if (err)
			return err;
		if (!g_input_codes.Find(controls[i]))
			return g_err_msg[0];
	}
	return err;
}

void cIBInputMapper::RecomposeControl(char* control_str, char controls[][32], long num_controls)
{
	control_str[0] = '\0';
	for (int i = 0; i < num_controls; i++)
	{
		if (i > 0)
			strcat(control_str, "+");
		strcat(control_str, controls[i]);
	}
}

void cIBInputMapper::StripControl(char* dest, char* src)
{
	dest[0] = '\0';
	if (src == NULL)
		return;

	long num_tokens;
	char** tokens = g_IB_variable_manager->Tokenize(src, &num_tokens, 0);
	if (num_tokens >= 3 && strcmp("control:", tokens[num_tokens - 2]) == 0)
	{
		g_IB_variable_manager->TokensToStr(dest, 128, const_cast<char**>(tokens), num_tokens - 2);
		g_IB_variable_manager->FreeTokens(tokens, num_tokens);
		return;
	}

	strcpy(dest, src);
	g_IB_variable_manager->FreeTokens(tokens, num_tokens);
}
// cIBJoyAxisProcess methods

cIBJoyAxisProcess::cIBJoyAxisProcess()
{
	m_deadzone_x = 0.0;
	m_deadzone_y = 0.0;
	m_deadzone_z = 0.0;
	m_deadzone_r = 0.0;
}

void cIBJoyAxisProcess::ProcessXY(double* x, double* y)
{
	ApplyDeadzone(x, m_deadzone_x);
	ApplyDeadzone(y, m_deadzone_y);
}

void cIBJoyAxisProcess::ProcessZR(double* z, double* r)
{
	ApplyDeadzone(z, m_deadzone_z);
	ApplyDeadzone(r, m_deadzone_r);
}

void cIBJoyAxisProcess::ApplyDeadzone(double* axis, double deadzone)
{
	double aAxis = fabs(*axis);
	double result;
	double inv;

	if (aAxis < deadzone)
	{
		*axis = 0;
	}
	else
	{
		inv = 1.0 / (1.0 - deadzone);
		result = (aAxis - deadzone) * inv;
		*axis = (*axis < 0) ? -result : result;
	}
}
