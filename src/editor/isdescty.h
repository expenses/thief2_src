// $Header: x:/prj/tech/libsrc/sdesc/RCS/isdescty.h 1.5 1999/01/27 17:33:59 mahk Exp $
#pragma once  
#ifndef __ISDESCTY_H
#define __ISDESCTY_H

#include <sdestype.h>
#include <comtools.h>

F_DECLARE_INTERFACE(IStructEditor);

typedef struct sStructEditorDesc sStructEditorDesc;
typedef struct sStructEditEvent sStructEditEvent;
typedef void* StructEditCBData;

typedef void (LGAPI *StructEditCB)(sStructEditEvent* event, StructEditCBData data);

#endif // __ISDESCTY_H

