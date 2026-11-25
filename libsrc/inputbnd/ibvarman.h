#pragma once

#include <inpbnd_i.h>

#include <aatree.h>

class cIBInputMapper;

typedef struct _intrnl_var
{
	IB_var var;
	aatree<intrnl_var_channel> channels;
	short stamp;
} intrnl_var;

class cIBVariableManager
{
	friend class cInputBinder;
	friend class cIBInputMapper;

public:
	cIBVariableManager();
	cIBVariableManager(IB_var* var);
	cIBVariableManager(IB_var* var, int num);
	~cIBVariableManager();

	void Init();
	int VarSet(IB_var* vars, int alias);
	int VarSet(IB_var* vars, long num, int alias);
	char* VarUnset(char** var_names, int user);
	char* VarUnset(char** var_names, long num, int user);
	char* VarUnsetAll();
	char* Cmd(char* cmd, int already_down);
	void SetMasterAggregation(tBindAggCallback func);
	void SetMasterProcessCallback(tBindProcCallback func);
	void SetBndSearchPath(char* p_path);

private:
	char* ProcessCommand(char** tokens, long num_tokens, int already_down);
	char* GetVarVal(char* dest, char* val_name);
	char* AddChannel(char** control);
	char* SubtractChannel(char* var_name, char* control);
	intrnl_var_channel* FindContextChannel(intrnl_var* var, char* control);
	void LoadBndContexted(char* pFName, sBindContext* pContexts, unsigned long iNumContexts, cIBInputMapper** m_ppMappers);
	char* LoadBnd(char* bnd_filename, char* prefix);
	char** Tokenize(char* cmd, long* num_tokens, int expand_alias);
	void GlueTokens(char* dest, char** tokens, long num_tokens);
	void FreeTokens(char** tokens, long num_tokens);
	void TokensToStr(char* str, long str_len, char** tokens, long num_tokens);

	aatree<intrnl_var> m_vars;
	tBindAggCallback m_glob_agg;
	tBindProcCallback m_glob_cb;
	char m_misc_str[128];
	char* m_bnd_path;
};
