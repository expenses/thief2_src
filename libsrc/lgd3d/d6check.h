#pragma once

#include <lgassert.h>

#ifdef SHIP
static char* pcDXef = "%s: error %x";
#else
static char* pcDXef = "%s: error %d\n%s ";
static char* pcLGD3Def = "LGD3D error no %d : %s : message: %d\n%s ";
#endif

#define CheckHResult(hr, msg)          AssertMsg3((hr) == 0, pcDXef, msg, HRESULT_CODE(hr), GetDDErrorMsg(hr))
#define CriticalHResult(hr, msg)       CriticalMsg3(pcDXef, msg, HRESULT_CODE(hr), GetDDErrorMsg(hr))
#define CriticalLGD3DError(code, hr)   CriticalMsg4(pcLGD3Def, code, GetLgd3dErrorCode(code), HRESULT_CODE(hr), GetDDErrorMsg(hr))
#ifdef SHIP
#define D6Msg(s) NULL
#else
#define D6Msg(s) s
#endif

#define WarnLGD3DError(code, hr)       Warning((pcLGD3Def, code, GetLgd3dErrorCode(code), HRESULT_CODE(hr), GetDDErrorMsg(hr)))
