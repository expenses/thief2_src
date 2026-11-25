/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/prompts.h,v 1.2 2000/01/29 13:13:01 adurant Exp $
// basic prompts for asking for data from the user
#pragma once

#ifndef __PROMPTS_H
#define __PROMPTS_H

// basic "get this value for me" prompts - all modal
// title may be NULL for a default; ok, if non-NULL, gets whether the user accepted
EXTERN BOOL   prompt_bool(char *def, char *title, BOOL *ok);
EXTERN int    prompt_int(char *def, char *title, BOOL *ok);
EXTERN double prompt_double(char *def, char *title, BOOL *ok);
EXTERN char  *prompt_string(char *def, char *buf, char *title);

// for prompt strings
#define PROMPT_STR_LEN 128

#endif  // __PROMPTS_H
