// $Header: r:/prj/cam/src/dark/RCS/drkbook.h 1.1 1998/08/24 16:21:40 mahk Exp $
#pragma once  
#ifndef __DRKBOOK_H
#define __DRKBOOK_H

////////////////////////////////////////////////////////////
// Dark Book API
//

//
// Init/Term
//
EXTERN void DarkBookInit(void); 
EXTERN void DarkBookTerm(void); 

//
// Show a book, given a text resource and an art subdir, both under
// "Books" 
//
EXTERN void SwitchToDarkBookMode(BOOL push, const char *text, const char* art); 

#endif // __DRKBOOK_H
