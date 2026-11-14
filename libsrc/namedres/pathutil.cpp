#include <lg.h>
#include "pathutil.h"

#include <cstring>

void GetPathTop(const char* pFullPath, char* pTopLevel, const char** ppPathRest)
{
	if (!pFullPath || !pTopLevel || !ppPathRest)
		return;

	int i = 0;
	char c;
	for (c = *pFullPath; c != '\0' && c != '\\' && c != '/'; c = pFullPath[i])
		++i;

	memmove(pTopLevel, pFullPath, i);
	pTopLevel[i] = '\\';
	pTopLevel[i + 1] = '\0';

	if (c == '\0')
		*ppPathRest = &pFullPath[i];
	else
		*ppPathRest = &pFullPath[i + 1];
}

bool PathAndName(const char* pPathName, char* pPath, char* pName)
{
	if (!pPathName || !pPath || !pName)
		return false;

	int nSize = strlen(pPathName);
	*pPath = '\0';
	*pName = '\0';

	if (nSize > 544)
	{
		*pPath = '\0'; *pName = '\0';
		return false;
	}

	int i;
	for (i = nSize - 1; i >= 0 && pPathName[i] != '\\' && pPathName[i] != '/'; --i)
		;
	if (nSize - i > 32)
	{
		*pPath = '\0'; *pName = '\0';
		return false;
	}

	memmove(pName, &pPathName[i + 1], nSize - i + 1);
	if (i < 0)
		return false;

	memmove(pPath, pPathName, i + 1);
	pPath[i + 1] = '\0';

	return true;
}

void GetNormalizedPath(const char* pOldPath, char** ppNewPath)
{
	if (!pOldPath)
	{
		*ppNewPath = NULL;
		return;
	}

	size_t nameLen = strlen(pOldPath);
	char lastChar = pOldPath[nameLen - 1];
	BOOL needSlash = FALSE;

	if (lastChar != '\\' && lastChar != '/')
	{
		++nameLen;
		needSlash = TRUE;
	}

	*ppNewPath = (char*)Malloc(nameLen + 1);
	char* pNewPath = *ppNewPath;

	for (const char* p = pOldPath; *p != '\0'; ++p, ++pNewPath)
	{
		if (*p == '/')
			*pNewPath = '\\';
		else
			*pNewPath = *p;
	}

	if (needSlash)
		*pNewPath++ = '\\';

	*pNewPath = '\0';
}
