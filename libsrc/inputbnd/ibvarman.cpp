#include <ibvarman.h>
#include <ibmapper.h>

#include <memory>
#include <math.h>
#include <cstdlib>
#include <lg_file.h>

extern cIBInputMapper* g_IB_input_mapper;
cIBVariableManager* g_IB_variable_manager;

static char* SkipWhiteSpace(char** str);
extern char* g_apszErrors[7];
extern char* g_apszAdjTypes[3];
static void GetChunk(char* dest, char** src, char ch1, char ch2);

BOOL CDECL IBMaxActiveAgg(intrnl_var_channel** chans, long num_chans, char* str)
{
	intrnl_var_channel* max_chan = NULL;
	BOOL ret_val = FALSE;
	double max_val = 0;

	for (int i = 0; i < num_chans; ++i)
	{
		if (chans[i]->active)
		{
			double cur_val = atof(chans[i]->val);
			if (cur_val > max_val || !ret_val)
			{
				ret_val = TRUE;
				max_val = cur_val;
				max_chan = chans[i];
			}
		}
	}

	if (ret_val)
		strcpy(str, max_chan->val);

	return ret_val;
}

BOOL CDECL IBAddActiveAgg(intrnl_var_channel** chans, long num_chans, char* str)
{
	BOOL found = false;
	double sum = 0.0;
	for (int i = 0; i < num_chans; ++i)
	{
		if (chans[i]->active)
		{
			found = true;
			sum = atof(chans[i]->val) + sum;
		}
	}

	if (found)
		sprintf(str, "%8.8f", sum);

	return found;
}

cIBVariableManager::cIBVariableManager()
{
	Init();
	SetMasterAggregation(IBMaxActiveAgg);
}


cIBVariableManager::cIBVariableManager(IB_var* var)
{
	Init();
	VarSet(var, 0);
}


cIBVariableManager::cIBVariableManager(IB_var* var, int num)
{
	Init();
	VarSet(var, num, 1);
}


void cIBVariableManager::Init()
{
	m_glob_cb = 0;
	m_bnd_path = 0;
	SetBndSearchPath(".");
	g_IB_variable_manager = this;
	SetMasterAggregation(IBMaxActiveAgg);
}


cIBVariableManager::~cIBVariableManager()
{
	if (m_bnd_path)
		delete [] m_bnd_path;
}


int cIBVariableManager::VarSet(IB_var* vars, int alias)
{
	if (!vars)
		return 0;

	int ret_val = 1;

	char** cmd_line_chan = new char* [2];
	cmd_line_chan[0] = new char[64];
	cmd_line_chan[1] = new char[64];
	strcpy(cmd_line_chan[0], "cmd_line");

	while (vars->name[0] != '\0')
	{
		IB_var* cur_var = vars++;
		if (strlen(cur_var->name) + 1 > IB_VARSTRMAX || strlen(cur_var->val) + 1 > IB_VARSTRMAX)
		{
			ret_val = 0;
			continue;
		}

		char name[32];
		strcpy(name, cur_var->name);
		strlwr(name);

		intrnl_var* int_var = m_vars.Find(name);
		if (int_var)
		{
			strcpy(int_var->var.val, cur_var->val);
			strlwr(int_var->var.val);
			int_var->var.cb = cur_var->cb;
			int_var->var.agg = cur_var->agg;
			continue;
		}

		int_var = new intrnl_var();
		memcpy(&int_var->var, cur_var, sizeof(IB_var));
		if (alias)
			int_var->var.flags |= 2;
		if (!cur_var->val[0])
			sprintf(int_var->var.val, "%8.8f", 0.0);

		strcpy(int_var->var.last_val, int_var->var.val);
		strlwr(int_var->var.name);
		strlwr(int_var->var.val);
		m_vars.Add(int_var->var.name, int_var, 1);
		strcpy(cmd_line_chan[1], name);
		AddChannel(cmd_line_chan);
	}

	delete[] cmd_line_chan[0];
	delete[] cmd_line_chan[1];
	delete[] cmd_line_chan;

	return ret_val;
}


int cIBVariableManager::VarSet(IB_var* vars, long num, int alias)
{
	IB_var* new_vars = new IB_var[num + 1];
	memcpy(new_vars, vars, 140 * num);
	memset(&new_vars[num], 0, sizeof(IB_var));
	int ret_val = VarSet(new_vars, alias);
	delete[] new_vars;

	return ret_val;
}


char* cIBVariableManager::VarUnset(char** var_names, int user)
{
	char** p = var_names;
	char* ret_val = NULL;

	for (; *p; ++p)
	{
		char name[IB_VARSTRMAX];
		strcpy(name, *p);
		strlwr(name);

		intrnl_var* var = m_vars.Find(name);
		if (var)
		{
			if (user && !(var->var.flags & 1))
				ret_val = g_apszErrors[4];
			else
			{
				var->channels.DeleteAll(1);
				m_vars.Delete(name, 1, NULL);
			}
		}
		else
			ret_val = g_apszErrors[3];
	}

	return ret_val;
}


char* cIBVariableManager::VarUnset(char** var_names, long num, int user)
{
	char* ret_val = NULL;

	char** new_names = new char* [num + 1];
	int i;
	for (i = 0; i < num; ++i)
	{
		new_names[i] = new char[strlen(var_names[i]) + 1];
		strcpy(new_names[i], var_names[i]);
	}

	new_names[num] = NULL;

	ret_val = VarUnset(new_names, user);
	for (i = 0; i < num; ++i)
		if (new_names[i])
			delete[] new_names[i];
	delete[] new_names;

	return ret_val;
}

void cIBVariableManager::SetMasterAggregation(tBindAggCallback func)
{
	m_glob_agg = func;
}

void cIBVariableManager::SetMasterProcessCallback(tBindProcCallback func)
{
	m_glob_cb = func;
}


char* cIBVariableManager::VarUnsetAll()
{
    do {
		m_vars.ResetVisited(NULL);
		intrnl_var* int_var = m_vars.GetNextInOrder(NULL);
		if (int_var) {
    		int_var->channels.DeleteAll(1);
    		m_vars.Delete(int_var->var.name, 1, 0);
		} else {
			break;
		}
	} while(TRUE);
	return NULL;
}


char* cIBVariableManager::Cmd(char* pCmd, int already_down)
{
	char* ret_str = NULL;

	char* cmd = (char*)pCmd;
	strlwr(cmd);

	long num_tokens;
	char** tokens = Tokenize(cmd, &num_tokens, 1);
	if (num_tokens == 0 || **tokens == ';')
	{
		FreeTokens(tokens, num_tokens);
		return NULL;
	}

	if (strcmp("bind", tokens[0]) == 0)
	{
		if (num_tokens < 2)
		{
			ret_str = g_apszErrors[0];
			goto cleanup;
		}

		if (strlen(tokens[1]) + 1 > 32 || num_tokens == 3 && strlen(tokens[2]) + 1 > 64)
		{
			ret_str = g_apszErrors[5];
			goto cleanup;
		}

		if (num_tokens == 2)
		{
			ret_str = g_IB_input_mapper->PeekBind(tokens[1], 1);
			goto cleanup;
		}

		long loc_num_tokens;
		char** loc_tokens = Tokenize(cmd, &loc_num_tokens, 0);
		if (loc_num_tokens != 2 && loc_num_tokens != 3)
			ret_str = g_apszErrors[0];
		else
		{
		if (strcmp(tokens[1], "shift") == 0 || strcmp(tokens[1], "alt") == 0 || strcmp(tokens[1], "ctrl") == 0)
		{
			char* dest = new char[strlen("bind ") + strlen(loc_tokens[1]) + strlen(loc_tokens[2]) + 3];
			strcpy(dest, "bind ");
			strcat(dest, "l");
			strcat(dest, loc_tokens[1]);
			if (loc_num_tokens == 3)
			{
				strcat(dest, " ");
				strcat(dest, loc_tokens[2]);
			}

			Cmd(dest, 0);
			dest[5] = 'r';
			ret_str = Cmd(dest, 0);
			delete[] dest;
		}

		char* tmp_cmd = new char[strlen("unbind ") + strlen(loc_tokens[1]) + 1];
		strcpy(tmp_cmd, "unbind ");
		strcat(tmp_cmd, loc_tokens[1]);
		Cmd(tmp_cmd, 0);

		if (strstr(loc_tokens[1], "mouse_wheel_"))
		{
			strcpy(tmp_cmd, "unbind ");
			strcat(tmp_cmd, "mouse_wheel");
			Cmd(tmp_cmd, 0);
		}
		else if (!strcmp(loc_tokens[1], "mouse_wheel"))
		{
			delete[] tmp_cmd;
			tmp_cmd = new char[strlen(loc_tokens[1]) + 40];
			for (int i = 0; i < 2; i++)
			{
				strcpy(tmp_cmd, "unbind ");
				strcat(tmp_cmd, (i == 0) ? "mouse_wheel_up" : "mouse_wheel_down");
				char* mods = tmp_cmd + strlen(tmp_cmd);
				for (int j = 0; j < 8; j++)
				{
					*mods = '\0';
					if (j & 1)
						strcat(tmp_cmd, "+alt");
					if (j & 2)
						strcat(tmp_cmd, "+ctrl");
					if (j & 4)
						strcat(tmp_cmd, "+shift");
					Cmd(tmp_cmd, 0);
				}
			}
		}
		delete[] tmp_cmd;

		ret_str = g_IB_input_mapper->Bind(loc_tokens + 1);
		if (!ret_str)
			AddChannel(tokens + 1);
		}

		FreeTokens(loc_tokens, loc_num_tokens);

		goto cleanup;
	}

	if (!strcmp("unbind", tokens[0]))
	{
		if (num_tokens != 2)
		{
			ret_str = g_apszErrors[0];
			goto cleanup;
		}

		if (strlen(tokens[1]) + 1 > 32)
		{
			ret_str = g_apszErrors[5];
			goto cleanup;
		}

		if (!strcmp(tokens[1], "shift") || !strcmp(tokens[1], "alt") || !strcmp(tokens[1], "ctrl"))
		{
			char* dest = new char[strlen("unbind ") + strlen(tokens[1]) + 2];
			strcpy(dest, "unbind ");
			strcat(dest, "l");
			strcat(dest, tokens[1]);
			Cmd(dest, 0);
			dest[7] = 'r';
			ret_str = Cmd(dest, 0);
			delete[] dest;
		}

		char* dest = new char[32];
		ret_str = g_IB_input_mapper->Unbind(tokens[1], &dest);
		if (!ret_str && *dest)
			SubtractChannel(dest, tokens[1]);

		delete[] dest;
		goto cleanup;
	}

	if (!strcmp("ibset", tokens[0]) || !strcmp("alias", tokens[0]))
	{
		if (num_tokens != 2 && num_tokens != 3)
		{
			ret_str = g_apszErrors[0];
			goto cleanup;
		}

		if (strlen(tokens[1]) + 1 > 32 || num_tokens == 3 && strlen(tokens[2]) + 1 > 32)
		{
			ret_str = g_apszErrors[5];
			goto cleanup;
		}

		char name[32];
		strcpy(name, tokens[1]);
		strlwr(name);
		intrnl_var* int_var = m_vars.Find(name);
		if (int_var)
		{
			if (num_tokens == 3)
			{
				strcpy(int_var->var.val, tokens[2]);
				strlwr(int_var->var.val);
			}
		}
		else
		{
			IB_var var;

			BOOL alias = strcmp("alias", tokens[0]) == 0;
			strcpy(var.name, name);
			if (num_tokens == 3)
				strcpy(var.val, tokens[2]);
			else
				strcpy(var.val, "0");

			var.flags = 1;
			var.cb = 0;
			var.agg = 0;
			VarSet(&var, 1, alias);
		}

		goto cleanup;
	}

	if (!strcmp("ibunset", tokens[0]) || !strcmp("alias", tokens[0]))
	{
		if (num_tokens != 2)
		{
			ret_str = g_apszErrors[0];
			goto cleanup;
		}

		if (strlen(tokens[1]) + 1 > 32)
		{
			ret_str = g_apszErrors[5];
			goto cleanup;
		}

		ret_str = VarUnset(tokens + 1, 1, 1);
		goto cleanup;
	}

	if (!strcmp("echo", tokens[0]))
	{
		TokensToStr(m_misc_str, sizeof(m_misc_str) / sizeof(m_misc_str[0]), const_cast<char**>(tokens) + 1, num_tokens - 1);
		ret_str = m_misc_str;
		goto cleanup;
	}

	if (!strcmp("loadbnd", tokens[0]))
	{
		if (num_tokens != 2)
		{
			ret_str = g_apszErrors[0];
			goto cleanup;
		}

		ret_str = LoadBnd(tokens[1], 0);
		goto cleanup;
	}

	if (!strcmp("savebnd", *tokens))
	{
		if (num_tokens != 2)
		{
			ret_str = g_apszErrors[0];
			goto cleanup;
		}

		g_IB_input_mapper->SaveBnd(tokens[1], 0);
		goto cleanup;
	}

	if (num_tokens > 0 && **tokens)
	{
		ret_str = ProcessCommand(const_cast<char**>(tokens), num_tokens, already_down);
		goto cleanup;
	}

cleanup:

	FreeTokens(tokens, num_tokens);

	return ret_str;
}


void cIBVariableManager::SetBndSearchPath(char* p_path)
{
	if (m_bnd_path)
		delete[] m_bnd_path;
	m_bnd_path = new char[strlen(p_path) + 1];
	strcpy(m_bnd_path, p_path);
}


char* cIBVariableManager::ProcessCommand(
	char** tokens,
	long num_tokens,
	int already_down)
{
	char* ret_val = NULL;

	char control[32];
	strcpy(control, "cmd_line");

	char first = tokens[0][0];
	int active_toggle = (first == '+' || first == '-') ? 1 : 0;
	intrnl_var* var = m_vars.Find(&tokens[0][active_toggle]);
	if (var == NULL)
	{
		if (m_glob_cb)
		{
			static char str1[128], str2[128];

			GlueTokens(str1, tokens, num_tokens);
			g_IB_input_mapper->StripControl(str2, str1);
			return m_glob_cb(str2, NULL, already_down);
		}

		return NULL;
	}

	if (num_tokens > 2 && !strcmp(tokens[num_tokens - 2], "control:"))
	{
		strcpy(control, tokens[num_tokens - 1]);
		num_tokens -= 2;
	}

	char* val;
	if (active_toggle)
	{
		intrnl_var_channel* channel = var->channels.Find(control);
		if (channel)
		{
			channel->active = first == '+';
			channel->stamp = ++var->stamp;
			val = channel->val;
		}
		else
			return g_apszErrors[2];
	}
	else
	{
		val = var->var.val;
	}

	if (num_tokens > 1)
	{
		if (!strcmp(tokens[1], g_apszAdjTypes[1]))
		{
			if (num_tokens > 2)
				sprintf(val, "%8.8f", atof(tokens[2]) + atof(val));
		}
		else if (!strcmp(tokens[1], g_apszAdjTypes[2]))
		{
			if (num_tokens == 3)
			{
				if (atof(val) != 0.0)
					strcpy(val, "0");
				else
					strcpy(val, tokens[2]);
			}
			else
				sprintf(val, "%8.8f", 1.0 - floor(atof(val) + 0.5));
		}
		else
		{
			int cur_token = 1;
			if (strcmp(tokens[1], g_apszAdjTypes[0]) == 0)
				cur_token = 2;

			strcpy(val, tokens[cur_token]);
		}
	}
	else
	{
		if (active_toggle)
		{
			if (first == '+')
				strcpy(val, "1.0");
			else
				strcpy(val, "0.0");
		}
		else
			sprintf(val, "%8.8f", 1.0 - floor(atof(val) + 0.5));
	}

	tBindAggCallback agg = NULL;
	if (var->var.agg)
		agg = var->var.agg;
	else
		agg = m_glob_agg;

	int chan_ret = 0;
	char final_val[32];

	if (agg)
	{
		int num_chans = var->channels.GetNumNodes();
		intrnl_var_channel** chan_array = new intrnl_var_channel * [num_chans];

		var->channels.ResetVisited(NULL);
		for (int i = 0; i < num_chans; ++i)
			chan_array[i] = var->channels.GetNextInOrder(NULL);

		chan_ret = agg(chan_array, num_chans, final_val);
		delete[] chan_array;
	}

	if (!chan_ret)
		strcpy(final_val, var->var.val);

	if (strcmp(var->var.last_val, final_val))
	{
		if (var->var.cb)
			ret_val = var->var.cb(var->var.name, final_val, already_down);
		else if (m_glob_cb)
			ret_val = m_glob_cb(var->var.name, final_val, already_down);

		strcpy(var->var.last_val, final_val);
	}

	return ret_val;
}


char* cIBVariableManager::GetVarVal(char* dest, char* val_name)
{
	intrnl_var* var = m_vars.Find(val_name);
	if (!var)
	{
    	*dest = '\0';
    	return g_apszErrors[3];
	}
	else
	{
    	strcpy(dest, var->var.val);
    	return NULL;
	}
}



char* cIBVariableManager::AddChannel(char** control)
{
	int cur_token = 1;

	int active_toggle = control[1][0] == '+' || control[1][0] == '-';

	long num_tokens;
	char** tokens = Tokenize(&control[1][active_toggle], &num_tokens, 0);
	if (num_tokens == 0)
	{
		FreeTokens(tokens, num_tokens);
		return g_apszErrors[0];
	}

	intrnl_var* var = m_vars.Find(tokens[0]);
	if (var == NULL)
	{
		FreeTokens(tokens, num_tokens);
		return g_apszErrors[3];
	}

	intrnl_var_channel* channel = new intrnl_var_channel();
	channel->active = FALSE;
	if (num_tokens > 1)
	{
		channel->adj_type = 0;
		if (!strcmp(tokens[1], g_apszAdjTypes[1]))
		{
			channel->adj_type = 1;
			cur_token = 2;
		}
		else if (!strcmp(tokens[1], g_apszAdjTypes[2]))
		{
			channel->adj_type = 2;
			cur_token = 2;
		}
		if (num_tokens > cur_token)
			strcpy(channel->val, tokens[cur_token]);
		else
			sprintf(channel->val, "%8.8f", 0.0);
	}
	else
	{
		channel->adj_type = 2;
		sprintf(channel->val, "%8.8f", 0.0);
	}

	channel->context = g_IB_input_mapper->m_context;
	var->channels.Add(*control, channel, 1);
	FreeTokens(tokens, num_tokens);

	return NULL;
}

char* g_apszBindCmds[] =
{
  "bind",
  "unbind",
  "ibset",
  "ibunset",
  "alias",
  "unalias",
  "echo",
  "loadbnd",
  "savebnd",
  NULL
};

char* g_apszErrors[7] =
{
  "Syntax error",
  "Unrecognized command",
  "Channel not found",
  "Variable not found",
  "Variable is reserved",
  "Field too long",
  "File not found"
};

char* g_apszAdjTypes[3] =
{
  "abs",
  "delta",
  "toggle"
};


char* cIBVariableManager::SubtractChannel(char* var_name, char* control)
{
	intrnl_var* var = m_vars.Find(var_name);
	if (!var)
		return g_apszErrors[3];

	intrnl_var_channel* channel = FindContextChannel(var, control);
	if (!channel)
		return g_apszErrors[2];

	var->channels.Delete(control, 1, channel);

	return NULL;
}


intrnl_var_channel* cIBVariableManager::FindContextChannel(intrnl_var* var, char* control)
{
	intrnl_var_channel* channel = var->channels.Find(control);
	if (!channel)
		return NULL;

	var->channels.ResetVisited(NULL);
	var->channels.VisitBefore(channel, NULL);

	char str[128];
	do
	{
		if (channel->context == g_IB_input_mapper->m_context)
			return channel;

		channel = var->channels.GetNextInOrder(str);
	} while (channel && !strcmp(control, str));

	return NULL;
}


void cIBVariableManager::LoadBndContexted(
	char* pFName,
	sBindContext* pContexts,
	unsigned long iNumContexts,
	cIBInputMapper** m_ppMappers)
{
	LGFILE* pFile = lg_fopen(pFName, "r");
	if (pFile == NULL)
		return;

	do
	{
		char aszBuf[256];
		fgets(aszBuf, 255, pFile->fp);

		long iNumTokens;
		char** ppszTokens = Tokenize(aszBuf, &iNumTokens, 1);
		char* pszPrefix = ppszTokens[0];
		if (iNumTokens)
		{
			for (int k = 0; g_apszBindCmds[k]; ++k)
			{
				if (!stricmp(pszPrefix, g_apszBindCmds[k]))
				{
					pszPrefix = "";
					break;
				}
			}

			unsigned int i;
			for (i = 0; i < iNumContexts; ++i)
				if (!stricmp(pszPrefix, pContexts[i].aszStr))
					break;

			if (i < iNumContexts)
			{
				unsigned int iContext = pContexts[i].iContext;
				cIBInputMapper** ppMapper = m_ppMappers;
				for (int j = 0; j < 32; ++j, ++ppMapper)
				{
					if ((iContext >> j) & 1)
					{
						if (!*ppMapper)
						{
							*ppMapper = new cIBInputMapper();
							(*ppMapper)->m_context = 1 << j;
						}
						g_IB_input_mapper = *ppMapper;
						Cmd(&aszBuf[strlen(pszPrefix)], 0);
					}
				}
			}
			FreeTokens(ppszTokens, iNumTokens);
		}
	} while (!feof(pFile->fp));

	lg_fclose(pFile);
}


char* cIBVariableManager::LoadBnd(char* bnd_filename, char* prefix)
{
	LGFILE* fp = lg_fopen(bnd_filename, "r");
	if (!fp)
		return g_apszErrors[6];

	fseek(fp->fp, 0, 0);

	do
	{
		char str[128];
		fgets(str, sizeof(str) / sizeof(str[0]) - 1, fp->fp);
		if (prefix)
		{
			long num_tokens;
			char** tokens = Tokenize(str, &num_tokens, 1);
			if (num_tokens)
			{
				if (!strcmp(tokens[0], prefix))
					Cmd(&str[strlen(tokens[0])], 0);
			}
			FreeTokens(tokens, num_tokens);
		}
		else
		{
			Cmd(str, 0);
		}
	} while (!feof(fp->fp));

	lg_fclose(fp);

	return NULL;
}



char** cIBVariableManager::Tokenize(char* cmd, long* num_tokens, int expand_alias)
{
	int expand_var = 0;
	if (!cmd)
		return NULL;

	if (!cmd[0] || cmd[0] == '\r' || cmd[0] == '\n')
	{

		char** tokens = new char* [1];
		tokens[0] = new char[1];
		tokens[0][0] = '\0';
		*num_tokens = 1;

		return tokens;
	}

	int tmp_tokens_num = 32;
	char** tmp_tokens = new char* [tmp_tokens_num];
	int i;
	for (i = 0; i < tmp_tokens_num; ++i)
		tmp_tokens[i] = new char[128];

	SkipWhiteSpace(&cmd);
	int num = 0;
	char separator;

	for (i = 0; i < 32; ++i)
	{
		if (!cmd[0] || cmd[0] == '\r' || cmd[0] == '\n')
			break;
		if (*cmd == '"')
		{
			++cmd;
			separator = '"';
		}
		else if (*cmd == ';')
		{
			if (num < 1)
				break;
			if (num == 1)
			{
				if (strcmp(*tmp_tokens, "bind") && strcmp(*tmp_tokens, "unbind"))
					break;
			}
			else if (strcmp(tmp_tokens[num - 1], "control:"))
			{
				break;
			}
			separator = ' ';
		}
		else
		{
			separator = ' ';
		}

		if (*cmd == '=')
		{
			strcpy(tmp_tokens[num], "=");
			++i;
			++num;
			++cmd;
		}
		else
		{
			if (*cmd == '$')
			{
				if (num - 1 >= 0)
				{
					if (strcmp(tmp_tokens[num - 1], "bind") && strcmp(tmp_tokens[num - 1], "unbind"))
						expand_var = 1;
				}
				else
				{
					expand_var = 1;
				}
			}
			GetChunk(tmp_tokens[num], &cmd, separator, '\0');
			intrnl_var* var = m_vars.Find(tmp_tokens[num]);
			if (expand_var || expand_alias && var && (var->var.flags & 2) != 0)
			{
				GetVarVal(tmp_tokens[num], &tmp_tokens[num][expand_var]);
				long num_expanded_tokens;
				char** expanded_tokens = Tokenize(tmp_tokens[num], &num_expanded_tokens, 1);
				for (int j = 0; j < num_expanded_tokens; ++j)
				{
					if (j + num >= 32)
					{
						delete[] expanded_tokens[j];
					}
					else
					{
						delete[](&tmp_tokens[j])[num];
						(&tmp_tokens[j])[num] = expanded_tokens[j];
					}
				}
				if (num_expanded_tokens)
					delete[] expanded_tokens;
				expand_var = 0;
				num = num + num_expanded_tokens - 1;
			}
			++num;
		}
		SkipWhiteSpace(&cmd);
	}

	*num_tokens = num;
	char** tokens = new char* [num];
	for (i = 0; i < num; ++i)
	{
		tokens[i] = new char[strlen(tmp_tokens[i]) + 1];
		strcpy(tokens[i], tmp_tokens[i]);
	}

	for (i = 0; i < tmp_tokens_num; ++i)
		delete[] tmp_tokens[i];
	delete[] tmp_tokens;

	return tokens;
}

char* SkipWhiteSpace(char** str)
{
	char c = **str;
	while (c == ' ' && c != '\0' && c != '\r' && c != '\n')
	{
		++*str;
		c = **str;
	}

	return *str;
}

void GetChunk(char* dest, char** src, char ch1, char ch2)
{
	char sch = **src;
	while (sch != ch1 && sch != ch2 && sch != '=' && sch != '\0' && sch != '\r' && sch != '\n')
	{
		*dest++ = **src;
		++*src;
		sch = **src;
	}

	*dest = '\0';

	if (sch != '=' && sch != '\0' && sch != '\r' && sch != '\n')
		++*src;
}

void cIBVariableManager::GlueTokens(char* dest, char** tokens, long num_tokens)
{
	*dest = '\0';
	for (int i = 0; i < num_tokens; i++)
	{
		if (i != 0)
			strcat(dest, " ");
		strcat(dest, tokens[i]);
	}
}

void cIBVariableManager::FreeTokens(char** tokens, long num_tokens)
{
	for (int i = 0; i < num_tokens; ++i)
		delete[] tokens[i];

	delete[] tokens;
}

void cIBVariableManager::TokensToStr(char* str, long str_len, char** tokens, long num_tokens)
{
	m_misc_str[0] = '\0';
	for (int i = 0; i < num_tokens; ++i)
	{
		if (strlen(str) + strlen(tokens[i]) >= str_len)
			break;

		strcat(str, tokens[i]);
		if (i < num_tokens - 1)
			strcat(str, " ");
	}
}

BOOL CDECL IBAveActiveAgg(intrnl_var_channel** chans, long num_chans, char* str)
{
	double avg = 0.0;
	int count = 0;
	double val = 0.0;
	for (int i = 0; i < num_chans; ++i)
	{
		if (chans[i]->active)
		{
		    val = atof(chans[i]->val);
			avg += val;
			count++;
		}
	}
	if (count)
	{
		avg /= count;
		sprintf(str, "%8.8f", avg);
		return TRUE;
	}
	return FALSE;
}

BOOL CDECL IBMRUActiveAgg(intrnl_var_channel** chans, long num_chans, char* str)
{
	if (num_chans > 0 && chans[0] && chans[0]->active)
	{
		strcpy(str, chans[0]->val);
		return TRUE;
	}
	return FALSE;
}
