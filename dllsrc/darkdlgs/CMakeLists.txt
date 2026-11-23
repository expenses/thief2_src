# darkdlgs.dll -- DromEd's MFC dialog library.
#
# Flags mirror the original Developer Studio "Win32 Debug" build recorded in
# darkdlgs.pdb (C:\prj\cam\dllsrc\darkdlgs, /MTd /ZI /Od, static MFC).
#
# Differences from the original:
#  * no precompiled header (-Yc/-Yu): loading a VC6 .pch crashes under wibo
#    on macOS (MapViewOfFileEx at a fixed base).  Codegen is unaffected.
#  * resources compiled by llvm-rc instead of RC.EXE.

set(DARKDLGS_SOURCES
  addlink.cpp
  aredit.cpp
  Cobjided.cpp
  COMobjed.cpp
  darkdlgs.cpp
  ddlgagg.cpp
  ddlgguid.cpp
  hierarch.cpp
  intsetbx.cpp
  linklist.cpp
  linkview.cpp
  objdlg.cpp
  objparam.cpp
  ObjTree.cpp
  pg8rlist.cpp
  proptree.cpp
  sdbits.cpp
  sdcheck.cpp
  sdenum.cpp
  sdescdlg.cpp
  sdesface.cpp
  sdtxtbox.cpp
  sdvecdlg.cpp
  simpmenu.cpp
  srclist.CPP
  srcsheet.cpp
  Srcstmpg.cpp
  srcview.CPP
  StdAfx.cpp
  stimlist.cpp
  trondlg.cpp
  tronlist.cpp
  tronview.cpp
)

set(ROOT "${PROJECT_SOURCE_DIR}")

vc6_add_dll(darkdlgs
  SOURCES ${DARKDLGS_SOURCES}
  RC darkdlgs.rc
  CFLAGS -nologo -MTd -W3 -GX -ZI -Od
         -D_DEBUG -DWIN32 -D_WINDOWS -D_USRDLL -D_WINDLL -D_MBCS -FD -TP
  INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}" "${ROOT}/src" "${ROOT}/src/object" "${ROOT}/h"
           "${ROOT}/src/actreact" "${ROOT}/src/framewrk" "${ROOT}/src/editor"
  LINKFLAGS -subsystem:windows -incremental:yes -debug -machine:I386 -pdbtype:sept
)
