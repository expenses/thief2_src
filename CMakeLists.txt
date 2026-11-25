cmake_minimum_required(VERSION 3.10)

set(CMAKE_SYSTEM_NAME Windows)

# VC6 (cl 12.00.8168, the build stamped in the targets) with no link-time
# codegen, so each .obj really corresponds to a run of code in the image.
#   retail thief2.exe : cmake -S . -B build/vc6 -G Ninja -DCMAKE_BUILD_TYPE=Release
#   OEM dromed.exe    : cmake -S . -B build/vc6_dromed -G Ninja -DCMAKE_BUILD_TYPE=Release -DEDITOR=ON
#   NewDark (vs2008)  : cmake -S . -B build/vs2008 -G Ninja -DCMAKE_BUILD_TYPE=Release -DTOOLCHAIN=vs2008
#   NewDark DromEd    : cmake -S . -B build/vs2008_dromed -G Ninja -DCMAKE_BUILD_TYPE=Release -DTOOLCHAIN=vs2008 -DEDITOR=ON
#     VS2008 SP1 build of the same sources for the NewDark labelling work; it
#     skips the VC6 header overrides. NewDark changes are written unconditionally
#     (no NEWDARK define); the OldDark text lives on the oldark bookmark.
option(EDITOR "Build the OEM dromed.exe (dromed.dsp 'Win32 Opt') instead of the retail game" OFF)
set(TOOLCHAIN "vc6" CACHE STRING "vc6 (OldDark targets) or vs2008 (NewDark)")

if(TOOLCHAIN STREQUAL "vc6")
  set(CMAKE_C_COMPILER   ${CMAKE_CURRENT_LIST_DIR}/vc6/cl.sh)
  set(CMAKE_CXX_COMPILER ${CMAKE_CURRENT_LIST_DIR}/vc6/cl.sh)
  set(CMAKE_LINKER       ${CMAKE_CURRENT_LIST_DIR}/vc6/link.sh)
  set(CMAKE_AR           ${CMAKE_CURRENT_LIST_DIR}/vc6/lib.sh CACHE FILEPATH "" FORCE)
else()
  set(CMAKE_C_COMPILER   ${CMAKE_CURRENT_LIST_DIR}/vs2008/cl.sh)
  set(CMAKE_CXX_COMPILER ${CMAKE_CURRENT_LIST_DIR}/vs2008/cl.sh)
  set(CMAKE_LINKER       ${CMAKE_CURRENT_LIST_DIR}/vs2008/link.sh)
  set(CMAKE_AR           ${CMAKE_CURRENT_LIST_DIR}/vs2008/lib.sh CACHE FILEPATH "" FORCE)
  # The pruned VS2008 tree keeps only the release CRTs, so CMake's /MDd /RTC1
  # compiler probe cannot link; probe with a static library instead.
  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
endif()
set(CMAKE_RANLIB       "" CACHE FILEPATH "" FORCE)
set(CMAKE_C_ARCHIVE_CREATE   "<CMAKE_AR> /nologo /out:<TARGET> <LINK_FLAGS> <OBJECTS>" CACHE STRING "" FORCE)
set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> /nologo /out:<TARGET> <LINK_FLAGS> <OBJECTS>" CACHE STRING "" FORCE)
set(CMAKE_C_ARCHIVE_FINISH   "" CACHE STRING "" FORCE)
set(CMAKE_CXX_ARCHIVE_FINISH "" CACHE STRING "" FORCE)
set(CMAKE_STATIC_LIBRARY_PREFIX "" CACHE STRING "" FORCE)
set(CMAKE_STATIC_LIBRARY_SUFFIX ".lib" CACHE STRING "" FORCE)
if(TOOLCHAIN STREQUAL "vc6")
  set(CMAKE_FIND_ROOT_PATH ${CMAKE_CURRENT_LIST_DIR}/vc6)
else()
  set(CMAKE_FIND_ROOT_PATH ${CMAKE_CURRENT_LIST_DIR}/vs2008)
endif()

# The asm objects were built with MASM 6.1x; ML 9 pads differently.
if(TOOLCHAIN STREQUAL "vc6")
  set(CMAKE_ASM_MASM_COMPILER ${CMAKE_CURRENT_LIST_DIR}/vc6/ml.sh)
else()
  set(CMAKE_ASM_MASM_COMPILER ${CMAKE_CURRENT_LIST_DIR}/vs2008/ml.sh)
endif()
set(CMAKE_ASM_MASM_FLAGS "/c /nologo /coff" CACHE STRING "" FORCE)
set(CMAKE_ASM_MASM_FLAGS_RELEASE "/c /nologo /coff" CACHE STRING "" FORCE)

set(CMAKE_RC_COMPILER ${CMAKE_CURRENT_LIST_DIR}/vc6/rc.sh CACHE FILEPATH "" FORCE)

# Wine reports some include paths as z:\...; CMake's compiler-based dependency
# extraction writes those into depfiles where the drive colon breaks the build.
set(CMAKE_C_DEPENDS_USE_COMPILER FALSE CACHE BOOL "" FORCE)
set(CMAKE_CXX_DEPENDS_USE_COMPILER FALSE CACHE BOOL "" FORCE)

project(darkengine3 C CXX ASM_MASM RC)

# Must override regular variable (CMakeCCompiler.cmake overrides cache with system ar)
if(TOOLCHAIN STREQUAL "vc6")
  set(CMAKE_AR ${CMAKE_CURRENT_LIST_DIR}/vc6/lib.sh)
else()
  set(CMAKE_AR ${CMAKE_CURRENT_LIST_DIR}/vs2008/lib.sh)
endif()
set(CMAKE_RANLIB "")

# vs2008: /Zc:forScope- keeps VC6 for-loop scoping, /wd4430 VC6's default int and an empty
# _CONST_RETURN the C signatures of strchr and friends, which the sources rely on; NewDark was
# built with C++ EH (/EHsc), OldDark without.
if(TOOLCHAIN STREQUAL "vc6")
  set(CMAKE_C_FLAGS "/nologo /MT /W3 /Zp1 /D WIN32 /D _WINDOWS /D _MBCS /D STRICT=1 " CACHE STRING "" FORCE)
  set(CMAKE_CXX_FLAGS "/nologo /MT /W3 /Zp1 /D WIN32 /D _WINDOWS /D _MBCS /D STRICT=1 " CACHE STRING "" FORCE)
else()
  # THIEF.EXE imports MSVCR90.dll and MSVCP90.dll (manifest Microsoft.VC90.CRT 9.0.30729.1): /MD, VS2008 SP1.
  set(CMAKE_C_FLAGS "/nologo /MD /W3 /Zp1 /D WIN32 /D _WINDOWS /D _MBCS /D STRICT=1 /D _CONST_RETURN= " CACHE STRING "" FORCE)
  set(CMAKE_CXX_FLAGS "/nologo /MD /W3 /Zp1 /D WIN32 /D _WINDOWS /D _MBCS /D STRICT=1 /D _CONST_RETURN= /Zc:forScope- /wd4430 /EHsc " CACHE STRING "" FORCE)
endif()

# /Ox /Gy is /O2 without /Gf: the release did not pool strings. NewDark (vs2008) used /O2.
# /Zd as in src/dromed.plg Opt: /Z7 adds typedef records to C1's symbol stream, which
# changes C2 decisions such as the 32-byte cache pad between large adjacent arrays.
if(EDITOR AND TOOLCHAIN STREQUAL "vs2008")
  # NewDark DromEd 1.27 (reference/dromed_newdark.exe): the editor defines with the NewDark optimisation flags.
  set(CMAKE_C_FLAGS_RELEASE "/O2 /Z7 /D EDITOR /D PLAYTEST /D PROPERTY_BLAME /D REPORT /D THIEF /D RGB_LIGHTING /D BIG_LIGHTMAPS /D SOFTWARE_RGB /D SCHEMA_LOADING /D NEW_NETWORK_ENABLED" CACHE STRING "" FORCE)
elseif(EDITOR)
  set(CMAKE_C_FLAGS_RELEASE "/Ox /Gy /Zd /D EDITOR /D PLAYTEST /D PROPERTY_BLAME /D REPORT /D THIEF /D RGB_LIGHTING /D BIG_LIGHTMAPS /D SOFTWARE_RGB /D SCHEMA_LOADING /D NEW_NETWORK_ENABLED" CACHE STRING "" FORCE)
elseif(TOOLCHAIN STREQUAL "vs2008")
  set(CMAKE_C_FLAGS_RELEASE "/O2 /Z7 /D NDEBUG /D THIEF /D RGB_LIGHTING /D BIG_LIGHTMAPS /D SOFTWARE_RGB /D NEW_NETWORK_ENABLED" CACHE STRING "" FORCE)
else()
  set(CMAKE_C_FLAGS_RELEASE "/Ox /Gy /Zd /D NDEBUG /D THIEF /D RGB_LIGHTING /D BIG_LIGHTMAPS /D SOFTWARE_RGB /D NEW_NETWORK_ENABLED" CACHE STRING "" FORCE)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}" CACHE STRING "" FORCE)

# Drive-letter trees (mkdrives.py --hybrid drives/dromed, --retail --hybrid drives/retail):
# cl opens every file under the path the original build used (r:\prj\thief2\..., x:\prj\tech\...,
# s:\ for retail). C1XX's allocation history, and with it vtable/thunk order and tie-breaks, depends on
# those path strings; a #line or __FILE__ redefinition instead costs allocations of its own.
# vc6/cl.sh reads decomp_drives from the build dir when DECOMP_DRIVES is not set in the environment.
if(TOOLCHAIN STREQUAL "vc6")
  if(EDITOR)
    file(WRITE ${CMAKE_BINARY_DIR}/decomp_drives "drives/dromed\n")
  else()
    file(WRITE ${CMAKE_BINARY_DIR}/decomp_drives "drives/retail\n")
  endif()
endif()

# No mt.exe here, and the originals carry their own manifest.
set(CMAKE_EXE_LINKER_FLAGS "/nologo /MAP /MANIFEST:NO" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "" CACHE STRING "" FORCE)
# THIEF.EXE was built with /GL and /LTCG: -DLTCG=ON (vs2008) builds that whole-program image,
# whose chunks scripts/nd_wp_bases.py uses as objdiff bases.
option(LTCG "vs2008: whole-program /GL + /LTCG build" OFF)
if(LTCG)
  add_compile_options(/GL)
  # THIEF.EXE carries a .reloc table; without one the split of this image has no data relocations to compare.
  set(CMAKE_EXE_LINKER_FLAGS "/nologo /MAP /MANIFEST:NO /LTCG /FIXED:NO" CACHE STRING "" FORCE)
  set(CMAKE_STATIC_LINKER_FLAGS "/LTCG" CACHE STRING "" FORCE)
endif()

# Import libraries for the QSound and Miles DLLs, made from their export lists.
# ia3d.lib is the A3D 1.2 SDK's prebuilt helper library, mss32.lib the Miles SDK's.
add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/qmixer.lib
  COMMAND ${CMAKE_AR} /nologo /machine:ix86 /def:${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/qsound/qmixer.def /out:${CMAKE_BINARY_DIR}/qmixer.lib
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/qsound/qmixer.def
)
add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/wail32.lib
  COMMAND ${CMAKE_AR} /nologo /machine:ix86 /def:${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/miles/wail32.def /out:${CMAKE_BINARY_DIR}/wail32.lib
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/miles/wail32.def
)
add_custom_target(import_libs DEPENDS ${CMAKE_BINARY_DIR}/qmixer.lib ${CMAKE_BINARY_DIR}/wail32.lib)

# ── Include directories ──
# vc6/override supplies the STL headers the VC6 CD stores under 8.3 names. The original's
# rpcndr.h declared MIDL_INTERFACE without novtable: vc6/override/RPCNDR.H wraps the pristine copy in
# vc6/install/INCLUDE for VC6 (the original compile opened three distinct rpcndr.h files, one compiler
# id each, which this chain reproduces). vs2008/include/rpcndr.h is now a plain pass-through to the
# Windows SDK 6.0A RpcNdr.h: keeping or stripping novtable made no difference to either NewDark exe.
# h/ is an old tech/h snapshot: searched last, it only supplies what nothing else has.
if(TOOLCHAIN STREQUAL "vc6")
  set(COMMON_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/vc6/override
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
  )
else()
  # The Platform SDK headers come from cl.sh (VS2008 ships none).
  set(COMMON_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/vs2008/include
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
  )
endif()
set(LIBSRC_INCLUDES
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/3d
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/3dsconv
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/actmovie
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/adjud
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/afile
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/anim
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/appcore
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/arg
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/arq
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/babl2
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/biped
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/cdrom
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/compapis
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/comtools
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/config
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/contrast
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/cpptools
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/darkloop
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/date
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/dev2d
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/dispdev
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/div0
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/drum
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/dstruct
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/dump
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/entrydll
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/eos
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/fastmat
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/fix
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/fixpp
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/fltloop
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/font
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/g2
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/gadget
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/gameshel
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/gfile
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/gifd
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/h
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/inp6d
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/inputbnd
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/ipw
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/jpeglg
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/labtab
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/lg
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/lgalloc
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/lgd3d
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/lgstream
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/loco
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/loopsys
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/r3d
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/matrix
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/md
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/mm
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/mp
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/mprintf
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/namedres
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/net
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/network
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/newskel
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/object
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/out2d
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/palmnch
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/portold
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/prof
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/quat
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/r3d3
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/random
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/range
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/recorder
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/ref
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/res
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/res2
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/rg
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/rnd
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/scaleup
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/script
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/sdesc
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/ser
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/skel
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/snd
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/sndcfg
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/sndsrc
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/sndutil
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/sound
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/star
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/tagfile
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/templgen
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/timer
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/tn_edms
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/ui
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/vec2
)
set(WINSRC_INCLUDES
  ${CMAKE_CURRENT_SOURCE_DIR}/winsrc/entrywin
  ${CMAKE_CURRENT_SOURCE_DIR}/winsrc/input
  ${CMAKE_CURRENT_SOURCE_DIR}/winsrc/wintools
)
if(TOOLCHAIN STREQUAL "vc6")
  set(LAST_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/h
    ${CMAKE_CURRENT_SOURCE_DIR}/h/win32
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dx6sdk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vc6/INCLUDE
  )
else()
  set(LAST_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/h
    ${CMAKE_CURRENT_SOURCE_DIR}/h/win32
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dx6sdk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vs2008/VC/include
  )
endif()
# dromed.dsp searches src\dark last, so motion's crjoint.h wins over dark's stale copy.
set(SRC_INCLUDES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/3dsconv
  ${CMAKE_CURRENT_SOURCE_DIR}/src/Debug
  ${CMAKE_CURRENT_SOURCE_DIR}/src/Dromed___Win32_Opt_No_Precompiled_Header
  ${CMAKE_CURRENT_SOURCE_DIR}/src/Opt
  ${CMAKE_CURRENT_SOURCE_DIR}/src/actreact
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ai
  ${CMAKE_CURRENT_SOURCE_DIR}/src/csg
  ${CMAKE_CURRENT_SOURCE_DIR}/src/cvs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/data
  ${CMAKE_CURRENT_SOURCE_DIR}/src/deepc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/editor
  ${CMAKE_CURRENT_SOURCE_DIR}/src/engfeat
  ${CMAKE_CURRENT_SOURCE_DIR}/src/framewrk
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mktmp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/motion
  ${CMAKE_CURRENT_SOURCE_DIR}/src/object
  ${CMAKE_CURRENT_SOURCE_DIR}/src/physics
  ${CMAKE_CURRENT_SOURCE_DIR}/src/portal
  ${CMAKE_CURRENT_SOURCE_DIR}/src/render
  ${CMAKE_CURRENT_SOURCE_DIR}/src/script
  ${CMAKE_CURRENT_SOURCE_DIR}/src/scripts
  ${CMAKE_CURRENT_SOURCE_DIR}/src/shock
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sim
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sound
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
  ${CMAKE_CURRENT_SOURCE_DIR}/src/win32
  ${CMAKE_CURRENT_SOURCE_DIR}/src/wrend
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dark
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
set(GAME_INCLUDES
  ${SRC_INCLUDES}
  ${COMMON_INCLUDES}
  ${LIBSRC_INCLUDES}
  ${WINSRC_INCLUDES}
  ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/object/property
  ${CMAKE_CURRENT_SOURCE_DIR}/src/deepc/motion
  ${LAST_INCLUDES}
)

# ── Per-source settings ──
# Each exe lists its sources directly (below), so settings are by source tree.
# The globs only attach settings; what gets built is what the exe lists.
file(GLOB_RECURSE GAME_C_SOURCES src/*.c src/*.cpp)
file(GLOB_RECURSE WINSRC_C_SOURCES winsrc/*.c winsrc/*.cpp)
file(GLOB_RECURSE TECH_C_SOURCES libsrc/*.c libsrc/*.cpp)
file(GLOB_RECURSE TECH_ASM_SOURCES libsrc/*.asm)
file(GLOB_RECURSE GAME_ASM_SOURCES src/*.asm)
file(GLOB_RECURSE TECH_ASM_SOURCES libsrc/*.asm)

# dromed.dsp compiles every TU with /FI"types.h" /D "__MSVC".
set_source_files_properties(${GAME_C_SOURCES} PROPERTIES
  INCLUDE_DIRECTORIES "${GAME_INCLUDES}"
  COMPILE_OPTIONS /FItypes.h
  COMPILE_DEFINITIONS __MSVC
)
# dromed.plg 'Win32 Opt' forces src/pch.h, but the OEM dromed came from the
# makefile build (module paths obj\win32\v60\a611\src\opt). Without pch.h every
# dromed object's function order equals the original (ambprop's inline
# emission order was the tell) and dromed has 20 non-100 instead of 22.
# h/win32 holds the comp_h exports (dev2d, g2, font, r3d) regenerated from
# libsrc, searched first as in LG's game include order (src, h/win32, h).
# g2s_point and the dev2d enums follow dbgdrom's PDB (g2s_point unions, no
# GDC_*_SLOT enumerators); with those the exports serve retail as well.
set_source_files_properties(${GAME_C_SOURCES} PROPERTIES
  INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/h/win32;${GAME_INCLUDES}"
  COMPILE_OPTIONS "/FItypes.h"
)
set_source_files_properties(${WINSRC_C_SOURCES} PROPERTIES
  INCLUDE_DIRECTORIES "${COMMON_INCLUDES};${WINSRC_INCLUDES};${LIBSRC_INCLUDES};${LAST_INCLUDES}"
  COMPILE_OPTIONS /FItypes.h
  COMPILE_DEFINITIONS __MSVC
)
# Tech libraries see windows.h only where a file includes it: the `$L` label ids in
# dbgdrom.pdb (scripts/tools/labelcmp.py) are ~40-52k lower than a build on h/lgpch.h
# for lg, mprintf, gadget, dev2d, md, ui, cpptools and script, and match within a few
# ids with /FItypes.h.
set_source_files_properties(${TECH_C_SOURCES} PROPERTIES
  INCLUDE_DIRECTORIES "${COMMON_INCLUDES};${LIBSRC_INCLUDES};${LAST_INCLUDES}"
  COMPILE_OPTIONS /FItypes.h
  COMPILE_DEFINITIONS __MSVC
)
set_source_files_properties(${GAME_ASM_SOURCES} PROPERTIES
  INCLUDE_DIRECTORIES "${COMMON_INCLUDES};${SRC_INCLUDES};${LIBSRC_INCLUDES};${CMAKE_CURRENT_SOURCE_DIR}/h;${CMAKE_CURRENT_SOURCE_DIR}/h/win32"
)
set_source_files_properties(${TECH_ASM_SOURCES} PROPERTIES
  INCLUDE_DIRECTORIES "${COMMON_INCLUDES};${LIBSRC_INCLUDES};${CMAKE_CURRENT_SOURCE_DIR}/h;${CMAKE_CURRENT_SOURCE_DIR}/h/win32"
)

# These need windows.h types (CRITICAL_SECTION in thrdtool.h, MEMORYSTATUS, max) and have
# no include of their own for it; arqreq's label ids show the original saw windows.h.
set_source_files_properties(
  libsrc/arq/arq.cpp
  libsrc/arq/arqqueue.cpp
  libsrc/arq/arqreq.cpp
  libsrc/arq/arqstrrd.cpp
  libsrc/arq/arqthred.cpp
  libsrc/lgd3d/d6render.cpp
  libsrc/net/msgdelay.cpp
  libsrc/res/rescache.cpp
  libsrc/sdesc/sdestool.cpp
  PROPERTIES COMPILE_OPTIONS /FIlgpch.h
)

# zlib is third-party and not built on lgpch.h.
set_source_files_properties(
  libsrc/namedres/adler32.c
  libsrc/namedres/infblock.c
  libsrc/namedres/infcodes.c
  libsrc/namedres/inffast.c
  libsrc/namedres/inflate.c
  libsrc/namedres/inftrees.c
  libsrc/namedres/infutil.c
  libsrc/namedres/zutil.c
  PROPERTIES COMPILE_OPTIONS /FItypes.h
)

# darkloop and namedres were not built on lgpch.h either: dbgdrom.pdb names their
# methods SendMessage, PostMessage, GetMessage, FindResource and GetFullPathName,
# so those translation units never saw the windows.h A/W macros.
set_source_files_properties(
  libsrc/darkloop/gloopcli.cpp
  libsrc/darkloop/loopdisp.cpp
  libsrc/darkloop/loopfact.cpp
  libsrc/darkloop/loopman.cpp
  libsrc/darkloop/loopmode.cpp
  libsrc/darkloop/loopque.cpp
  libsrc/namedres/binrstyp.cpp
  libsrc/namedres/bmp.cpp
  libsrc/namedres/cel.cpp
  libsrc/namedres/defresm.cpp
  libsrc/namedres/defstfct.cpp
  libsrc/namedres/filestrm.cpp
  libsrc/namedres/fonrstyp.cpp
  libsrc/namedres/gif.cpp
  libsrc/namedres/imghack.cpp
  libsrc/namedres/imgrstyp.cpp
  libsrc/namedres/mdlrstyp.cpp
  libsrc/namedres/palrstyp.cpp
  libsrc/namedres/pathutil.cpp
  libsrc/namedres/pcx.cpp
  libsrc/namedres/resarq.cpp
  libsrc/namedres/resimage.cpp
  libsrc/namedres/resman.cpp
  libsrc/namedres/resmanhs.cpp
  libsrc/namedres/resmanx.cpp
  libsrc/namedres/respall.cpp
  libsrc/namedres/resstat.cpp
  libsrc/namedres/resstr.cpp
  libsrc/namedres/restmpl.cpp
  libsrc/namedres/resutil.cpp
  libsrc/namedres/search.cpp
  libsrc/namedres/sndrstyp.cpp
  libsrc/namedres/stdres.cpp
  libsrc/namedres/storbase.cpp
  libsrc/namedres/storedir.cpp
  libsrc/namedres/storeman.cpp
  libsrc/namedres/storezip.cpp
  libsrc/namedres/strrstyp.cpp
  libsrc/namedres/tga.cpp
  libsrc/namedres/zipstfct.cpp
  libsrc/namedres/zipstrm.cpp
  libsrc/namedres/resids.c
  PROPERTIES COMPILE_OPTIONS /FItypes.h
)

# The DirectX Media SDK's static quartz.lib held the Quartz interface IDs in one
# object: strmif.h's IIDs in declaration order, then control.h's GUIDs.
set_source_files_properties(3rdparty/dxmedia/quartz.c PROPERTIES
  INCLUDE_DIRECTORIES "${COMMON_INCLUDES};${LAST_INCLUDES}"
  COMPILE_OPTIONS ""
  COMPILE_DEFINITIONS ""
)

# The target exports RecordExceptionInfo with a C++ mangled name.
set_source_files_properties(libsrc/appcore/exceptio.c PROPERTIES COMPILE_OPTIONS /TP)
# NewDark compiled these as C++: they call C++-linkage NewDark functions (CameraSendAttachMessages,
# MtlConfigInit).
if(TOOLCHAIN STREQUAL "vs2008")
  set_source_files_properties(src/render/camera.c src/framewrk/resapp.c PROPERTIES COMPILE_OPTIONS /TP)
endif()

# Retail links dark's crjoint.cpp, which wants dark's crjoint.h.
set_source_files_properties(src/dark/crjoint.cpp PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/src/dark;${GAME_INCLUDES}")

# libsrc/r3d is searched first for its _test.h; these want libsrc/matrix's.
set_source_files_properties(libsrc/matrix/vecang.c libsrc/matrix/vecangd.c PROPERTIES INCLUDE_DIRECTORIES
  "${CMAKE_CURRENT_SOURCE_DIR}/libsrc/matrix;${COMMON_INCLUDES};${LIBSRC_INCLUDES};${LAST_INCLUDES}"
)

# dev2d/clip.h and r3d/clip.h share the __CLIP_H guard (dev2d/init.h and g2/init.h
# share __INIT_H), so a TU that includes both libraries' headers has to see the
# flattened tech/h exports, as LG's own build did.
set_source_files_properties(
  libsrc/lgd3d/d6prim_s.cpp
  libsrc/lgd3d/d8Frame.cpp
  libsrc/lgd3d/d8Prim_s.cpp
  libsrc/lgd3d/lgd3d_g.cpp
  libsrc/md/mipmap.c
  libsrc/r3d/clipproj.c
  libsrc/r3d/ctxtguts.c
  libsrc/r3d/prim.c
  libsrc/r3d/primd3d.c
  libsrc/r3d/primg2.c
  libsrc/r3d/primix.c
  libsrc/r3d/primmisc.c
  libsrc/r3d/primnull.c
  libsrc/r3d/primpoly.c
  libsrc/r3d/space.c
  libsrc/r3d/view.c
  libsrc/star/star.c
  PROPERTIES INCLUDE_DIRECTORIES
  "${CMAKE_CURRENT_SOURCE_DIR}/h/win32;${COMMON_INCLUDES};${LIBSRC_INCLUDES};${LAST_INCLUDES}"
)

# These were built against libsrc's working copies of str.h and dlist.h rather
# than the published tech/h ones: different __FILE__ and __LINE__ in their asserts.
file(GLOB CPPTOOLS_SOURCES libsrc/cpptools/*.cpp)
set_property(SOURCE
  ${CPPTOOLS_SOURCES}
  libsrc/arq/arqqueue.cpp
  libsrc/net/msgdelay.cpp
  libsrc/res/rescache.cpp
  winsrc/input/inpman.cpp
  APPEND PROPERTY COMPILE_DEFINITIONS LIBSRC_WORKING_HEADERS
)

# resacc.cpp's <resarq.h> is the res library's own header, not namedres's: the
# original resacc.obj carries none of the namedres hash-set instantiations.
set_source_files_properties(libsrc/res/resacc.cpp PROPERTIES INCLUDE_DIRECTORIES
  "${CMAKE_CURRENT_SOURCE_DIR}/libsrc/res;${COMMON_INCLUDES};${LIBSRC_INCLUDES};${LAST_INCLUDES}"
)

# resdata.c includes game headers.
set_source_files_properties(libsrc/res/resdata.c PROPERTIES INCLUDE_DIRECTORIES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/render;${CMAKE_CURRENT_SOURCE_DIR}/src/framewrk;${CMAKE_CURRENT_SOURCE_DIR}/src;${COMMON_INCLUDES};${LIBSRC_INCLUDES};${LAST_INCLUDES}"
)

# YY_STATIC keeps the lexers' input()/unput() file-scoped, as in the original DSP.
set_property(SOURCE
  src/win32/inclex.c
  src/win32/mschlex.c
  src/win32/schlex.c
  src/win32/songlex.c
  APPEND PROPERTY COMPILE_DEFINITIONS YY_STATIC
)

# The retail game has no EDITOR, but these TUs need it to compile at all.
set_property(SOURCE
  src/csg/csgbuild.c
  src/csg/csgclip.c
  src/csg/csg.c
  src/sound/songedtr.cpp
  src/editor/strprped.cpp
  src/editor/ged_csg.cpp
  src/editor/famedit.cpp
  src/editor/gedit.c
  APPEND PROPERTY COMPILE_OPTIONS /DEDITOR
)

# Retail: SHIP everywhere, and TERSE_ASSERT/NO_ASSERTS in the tech libraries.
if(NOT EDITOR)
  set_property(SOURCE ${GAME_C_SOURCES} APPEND PROPERTY COMPILE_DEFINITIONS SHIP)
  set_property(SOURCE ${WINSRC_C_SOURCES} ${TECH_C_SOURCES} APPEND PROPERTY COMPILE_DEFINITIONS SHIP TERSE_ASSERT NO_ASSERTS)
  # g2/tmaps.inc follows tmaps.h, whose field order depends on SHIP.
  set_property(SOURCE ${TECH_ASM_SOURCES} APPEND PROPERTY COMPILE_DEFINITIONS SHIP)
  # With SHIP, memall.h hides the f_*_db externs and rsdcvt's dbm spill moves.
  set_source_files_properties(
    libsrc/cpptools/filespec.cpp
    libsrc/dev2d/rsdcvt.c
    PROPERTIES COMPILE_OPTIONS /USHIP
  )
endif()

# net.cpp alone pools its strings in dromed (??_C@ publics, one copy of __FILE__); /O2 = /Ox /Gy /Gf.
set_source_files_properties(libsrc/net/net.cpp PROPERTIES COMPILE_OPTIONS /Gf)

# gameshel's DirectX GUID instances need the SDK's dplay.h, which libsrc/net shadows.
set_source_files_properties(libsrc/gameshel/dxguid.c PROPERTIES
  INCLUDE_DIRECTORIES "${COMMON_INCLUDES};${CMAKE_CURRENT_SOURCE_DIR}/libsrc/sound;${LAST_INCLUDES}"
  COMPILE_OPTIONS ""
  COMPILE_DEFINITIONS ""
)

# Retail pairs some template code to this copy of resarq, but only namedres's is linked.
if(NOT EDITOR)
  add_library(objdiff_only STATIC libsrc/res/resarq.cpp)
endif()

# ── Executables ──
# Sources are in the original link order: the linker lays objects out as listed.
if(EDITOR)
  # Tech libraries, one per makefile library and in its link order. The linker
  # pulls members on demand, so their order in the image follows the references.
  add_library(dromed_mprintf STATIC
    libsrc/mprintf/mprintf.c
  )
  add_library(dromed_lg STATIC
    libsrc/lg/hashfns.cpp
    libsrc/lg/appagg.cpp
    libsrc/lg/lgassert.cpp
    libsrc/lg/lglog.cpp
    libsrc/lg/lgsprntf.c
    libsrc/lg/lg_file.c
    libsrc/lg/datapath.cpp
    libsrc/lg/dbg.c
    libsrc/lg/exit.c
    libsrc/lg/breakkey.cpp
    libsrc/lg/dpathdir.c
    libsrc/lg/tmpalloc.c
    libsrc/lg/coremutx.cpp
    libsrc/lg/stack.c
    libsrc/lg/timings.cpp
    libsrc/lg/codewrit.cpp
    libsrc/lg/thrdtool.cpp
  )
  add_library(dromed_lgalloc STATIC
    libsrc/lgalloc/memall.c
    libsrc/lgalloc/memcore.cpp
    libsrc/lgalloc/multpool.cpp
    libsrc/lgalloc/stdalloc.cpp
    libsrc/lgalloc/allcguid.cpp
    libsrc/lgalloc/primallc.cpp
    libsrc/lgalloc/dbgalloc.cpp
    libsrc/lgalloc/memtimer.cpp
    libsrc/lgalloc/heap.cpp
    libsrc/lgalloc/membase.cpp
    libsrc/lgalloc/pool.cpp
    libsrc/lgalloc/nullallc.cpp
    libsrc/lgalloc/poolimp.cpp
  )
  # NewDark imports malloc/free/new/delete from MSVCR90: no allocator override.
  if(TOOLCHAIN STREQUAL "vs2008")
    target_sources(dromed_lgalloc PRIVATE libsrc/lgalloc/nonover.cpp)
  endif()
  add_library(dromed_matrix STATIC
    libsrc/matrix/vecvec.c
    libsrc/matrix/matang.c
    libsrc/matrix/matvec.c
    libsrc/matrix/matmat.c
    libsrc/matrix/vecang.c
    libsrc/matrix/vecvecd.c
    libsrc/matrix/vec2.c
    libsrc/matrix/trans.c
    libsrc/matrix/matvecd.c
    libsrc/matrix/matangd.c
    libsrc/matrix/matmatd.c
    libsrc/matrix/transasm.asm
  )
  add_library(dromed_fix STATIC
    libsrc/fix/fixdiv.asm
    libsrc/fix/fix.c
    libsrc/fix/fix_asm.asm
    libsrc/fix/trigtab.c
    libsrc/fix/fix_pow.c
  )
  add_library(dromed_dstruct STATIC
    libsrc/dstruct/hheap.c
    libsrc/dstruct/lllist.c
    libsrc/dstruct/hash.c
    libsrc/dstruct/rect.c
    libsrc/dstruct/array.c
  )
  add_library(dromed_cpptools STATIC
    libsrc/cpptools/dynarray.cpp
    libsrc/cpptools/str.cpp
    libsrc/cpptools/hashset.cpp
    libsrc/cpptools/dlist.cpp
    libsrc/cpptools/packflag.cpp
    libsrc/cpptools/filespec.cpp
    libsrc/cpptools/hashpp.cpp
    libsrc/cpptools/dynfunc.cpp
    libsrc/cpptools/delmlist.cpp
    libsrc/cpptools/filepath.cpp
    libsrc/cpptools/fnamutil.cpp
    libsrc/cpptools/splitpat.cpp
    libsrc/cpptools/pdynarr.cpp
    libsrc/cpptools/relocptr.cpp
  )
  add_library(dromed_comtools STATIC
    libsrc/comtools/genagg.cpp
    libsrc/comtools/interset.cpp
    libsrc/comtools/pguidset.cpp
    libsrc/comtools/comconn.cpp
  )
  add_library(dromed_compapis STATIC
    libsrc/compapis/genguid1.c
  )
  add_library(dromed_appcore STATIC
    libsrc/appcore/wappcore.cpp
    libsrc/appcore/appcore.cpp
    libsrc/appcore/exceptio.c
    libsrc/appcore/appflags.cpp
  )
  add_library(dromed_dev2d STATIC
    libsrc/dev2d/grmalloc.c
    libsrc/dev2d/grd.c
    libsrc/dev2d/bitmap.c
    libsrc/dev2d/canvas.c
    libsrc/dev2d/mode.c
    libsrc/dev2d/ipal.c
    libsrc/dev2d/pal.c
    libsrc/dev2d/pal16.c
    libsrc/dev2d/ltab.c
    libsrc/dev2d/gdbm.c
    libsrc/dev2d/gdrect.c
    libsrc/dev2d/clut16.c
    libsrc/dev2d/tluctab.c
    libsrc/dev2d/smode.c
    libsrc/dev2d/screen.c
    libsrc/dev2d/genbmf.c
    libsrc/dev2d/cpal.c
    libsrc/dev2d/gdpix.c
    libsrc/dev2d/gpal.c
    libsrc/dev2d/gdgbm.c
    libsrc/dev2d/fcolor.c
    libsrc/dev2d/valloc.c
    libsrc/dev2d/gdlin.c
    libsrc/dev2d/comdfcn.cpp
    libsrc/dev2d/genrsd8.c
    libsrc/dev2d/genf16.c
    libsrc/dev2d/genfl8.c
    libsrc/dev2d/genmono.c
    libsrc/dev2d/rsdcvt.c
    libsrc/dev2d/rsdunpck.asm
    libsrc/dev2d/init.c
    libsrc/dev2d/hflip.c
    libsrc/dev2d/astring.c
    libsrc/dev2d/stastk.c
    libsrc/dev2d/cominit.c
    libsrc/dev2d/ddevblt.c
    libsrc/dev2d/f16lin.c
    libsrc/dev2d/f16bm.c
    libsrc/dev2d/f16pix.c
    libsrc/dev2d/lpix.c
    libsrc/dev2d/fl8lin.c
    libsrc/dev2d/fl8bm.c
    libsrc/dev2d/fl8pix.c
    libsrc/dev2d/f16hlin.c
    libsrc/dev2d/f16vlin.c
    libsrc/dev2d/f16rsd8.c
    libsrc/dev2d/genf16u.c
    libsrc/dev2d/f16f16.c
    libsrc/dev2d/f16fl8.c
    libsrc/dev2d/genfl8u.c
    libsrc/dev2d/f16mono.c
    libsrc/dev2d/genbm.c
    libsrc/dev2d/genmonou.c
    libsrc/dev2d/f16pix8.c
    libsrc/dev2d/f16pix16.c
    libsrc/dev2d/fl8hlin.c
    libsrc/dev2d/fl8vlin.c
    libsrc/dev2d/fl8rsd8.c
    libsrc/dev2d/fl8fl8.c
    libsrc/dev2d/fl8mono.c
    libsrc/dev2d/fl8pix8.c
    libsrc/dev2d/f16set.asm
    libsrc/dev2d/f16f16c.asm
    libsrc/dev2d/rsdblt16.asm
    libsrc/dev2d/f16fl8il.asm
    libsrc/dev2d/cstring.asm
    libsrc/dev2d/f16il.asm
    libsrc/dev2d/rsdblt8.asm
    libsrc/dev2d/bank.c
    libsrc/dev2d/comdtab.c
    libsrc/dev2d/ddevcnv.c
    libsrc/dev2d/f16cnv.c
    libsrc/dev2d/fl8cnv.c
    libsrc/dev2d/f16hlinf.c
    libsrc/dev2d/f16vlinf.c
    libsrc/dev2d/f16ubmf.c
    libsrc/dev2d/f16p8f.c
    libsrc/dev2d/f16p16f.c
    libsrc/dev2d/fl8hlinf.c
    libsrc/dev2d/fl8vlinf.c
    libsrc/dev2d/fl8ubmf.c
    libsrc/dev2d/fl8p8f.c
    libsrc/dev2d/fl8p16f.c
    libsrc/dev2d/grnull.c
    libsrc/dev2d/mxlatch.c
  )
  add_library(dromed_g2 STATIC
    libsrc/g2/g2d.c
    libsrc/g2/init.c
    libsrc/g2/g2circ.c
    libsrc/g2/g2ov.c
    libsrc/g2/genrast.c
    libsrc/g2/pt_main.c
    libsrc/g2/pt_map.c
    libsrc/g2/clplin.c
    libsrc/g2/pt_duv.c
    libsrc/g2/tmsetup.c
    libsrc/g2/point.c
    libsrc/g2/setdrv.c
    libsrc/g2/g2makewr.c
    libsrc/g2/g2el.c
    libsrc/g2/rastel.c
    libsrc/g2/uvgrad.asm
    libsrc/g2/uvedge.asm
    libsrc/g2/tmapd.c
    libsrc/g2/trigrad.c
    libsrc/g2/ptfloat.asm
    libsrc/g2/ptmap_8.asm
    libsrc/g2/ptpersp.asm
    libsrc/g2/ptmap_n.asm
    libsrc/g2/ptmapper.asm
    libsrc/g2/pt_grad.c
    libsrc/g2/ptlinear.asm
    libsrc/g2/f16cnv.c
    libsrc/g2/fl8cnv.c
    libsrc/g2/gencnv.c
    libsrc/g2/permap.c
    libsrc/g2/tf.c
    libsrc/g2/n88il.asm
    libsrc/g2/lo816il.asm
    libsrc/g2/lo88il.asm
    libsrc/g2/co88il.asm
    libsrc/g2/o88il.asm
    libsrc/g2/o88bil.asm
    libsrc/g2/ptmap.asm
    libsrc/g2/clip.c
    libsrc/g2/f16pm.c
    libsrc/g2/f16lm.c
    libsrc/g2/gensply.c
    libsrc/g2/genply.c
    libsrc/g2/gens.c
    libsrc/g2/genline.c
    libsrc/g2/fl8pm.c
    libsrc/g2/fl8lm.c
    libsrc/g2/fl8sply.c
    libsrc/g2/fl8s.c
    libsrc/g2/fl8l.c
    libsrc/g2/genlm.c
    libsrc/g2/f16lmf.c
    libsrc/g2/f16llmf.c
    libsrc/g2/genusf.c
    libsrc/g2/gensf.c
    libsrc/g2/fl8lmf.c
    libsrc/g2/fl8llmf.c
    libsrc/g2/fl8usf.c
    libsrc/g2/fl8sf.c
    libsrc/g2/fl8ulf.c
    libsrc/g2/genlmf.c
    libsrc/g2/genllmf.c
    libsrc/g2/genrsdtm.c
    libsrc/g2/genf16l.c
    libsrc/g2/genfl8l.c
    libsrc/g2/o816.c
    libsrc/g2/lo816.c
    libsrc/g2/genrsds.c
    libsrc/g2/genf16s.c
    libsrc/g2/genfl8s.c
    libsrc/g2/scshell.c
    libsrc/g2/r88.c
    libsrc/g2/co88.c
    libsrc/g2/o88.c
    libsrc/g2/lo88.c
    libsrc/g2/fl8fl8s.c
    libsrc/g2/fl8lin.c
    libsrc/g2/gen16il.c
    libsrc/g2/gen8il.c
    libsrc/g2/o816il.asm
    libsrc/g2/clo816il.asm
  )
  add_library(dromed_font STATIC
    libsrc/font/str.c
    libsrc/font/strsiz.c
    libsrc/font/chrsiz.c
    libsrc/font/strwrap.c
    libsrc/font/setup.c
    libsrc/font/f16fl8t.c
  )
  add_library(dromed_dispdev STATIC
    libsrc/dispdev/dddynf.cpp
    libsrc/dispdev/wdcreate.cpp
    libsrc/dispdev/wdisp.cpp
    libsrc/dispdev/wdispgdi.cpp
    libsrc/dispdev/wdispdd.cpp
    libsrc/dispdev/dispbase.cpp
    libsrc/dispdev/wdspprov.cpp
    libsrc/dispdev/wdspmode.cpp
    libsrc/dispdev/wdspgshl.cpp
    libsrc/dispdev/wdisptls.cpp
    libsrc/dispdev/wddmode.cpp
    libsrc/dispdev/cbchain.cpp
  )
  if(TOOLCHAIN STREQUAL "vs2008")
    target_sources(dromed_dispdev PRIVATE
      libsrc/dispdev/wdispd3d.cpp
      libsrc/dispdev/wd3dmode.cpp
      libsrc/dispdev/wd3dhw2d.cpp
    )
    target_include_directories(dromed_dispdev PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dx9sdk/include)
    # NewDark was built against a PSDK with WINVER 0x0500: it imports GetMonitorInfoA, and
    # below 0x0500 the dx6 ddraw.h #defines HMONITOR, which breaks d3d9.h.
    set_source_files_properties(
      libsrc/dispdev/dddynf.cpp
      libsrc/dispdev/wdcreate.cpp
      libsrc/dispdev/wdispd3d.cpp
      libsrc/dispdev/wd3dmode.cpp
      libsrc/dispdev/wd3dhw2d.cpp
      PROPERTIES COMPILE_DEFINITIONS "__MSVC;WINVER=0x0500"
    )
  endif()
  add_library(dromed_config STATIC
    libsrc/config/config.c
    libsrc/config/cfg.c
  )
  add_library(dromed_res STATIC
    libsrc/res/resguid.c
    libsrc/res/rescache.cpp
    libsrc/res/resagg.c
    libsrc/res/refacc.cpp
    libsrc/res/resacc.cpp
    libsrc/res/resmem.cpp
    libsrc/res/resfile.cpp
    libsrc/res/resthred.cpp
    libsrc/res/res.cpp
    libsrc/res/resload.cpp
    libsrc/res/pkzip.cpp
    libsrc/res/lzw.cpp
    libsrc/res/resbuild.cpp
    libsrc/res/rescback.cpp
  )
  add_library(dromed_input STATIC
    winsrc/input/kbcntry.c
    winsrc/input/mousesys.cpp
    winsrc/input/inpinit.c
    winsrc/input/inpman.cpp
    winsrc/input/inpguid.c
    winsrc/input/kbasctab.cpp
    winsrc/input/kbcook.cpp
    winsrc/input/recinput.cpp
    winsrc/input/inpcompo.cpp
    winsrc/input/defehand.cpp
    winsrc/input/kbstart.cpp
    winsrc/input/kbshut.cpp
    winsrc/input/inpdynf.cpp
    winsrc/input/joystick.cpp
    winsrc/input/kbsys.cpp
    winsrc/input/inperr.cpp
    winsrc/input/joyold.cpp
    winsrc/input/joywin.cpp
  )
  add_library(dromed_gameshel STATIC
    libsrc/gameshel/wgscreat.cpp
    libsrc/gameshel/dxguid.c
    libsrc/gameshel/wgshell.cpp
    libsrc/gameshel/wgshelmt.cpp
    libsrc/gameshel/winput.cpp
  )
  add_library(dromed_timer STATIC
    libsrc/timer/tmwin32.c
    libsrc/timer/tmrt.c
    libsrc/timer/tmftab.c
  )
  add_library(dromed_rnd STATIC
    libsrc/rnd/rnd.c
  )
  add_library(dromed_recorder STATIC
    libsrc/recorder/recorder.cpp
    libsrc/recorder/recprmpt.cpp
    libsrc/recorder/rectempl.cpp
  )
  add_library(dromed_ui STATIC
    libsrc/ui/region.c
    libsrc/ui/hotkey.c
    libsrc/ui/cursors.c
    libsrc/ui/event.c
    libsrc/ui/slab.c
    libsrc/ui/curdrw.c
    libsrc/ui/vmouse.c
  )
  add_library(dromed_gadget STATIC
    libsrc/gadget/uitexted.c
    libsrc/gadget/gadget.c
    libsrc/gadget/gadtext.c
    libsrc/gadget/gadblist.c
    libsrc/gadget/gcompose.c
    libsrc/gadget/guistyle.c
    libsrc/gadget/drawelem.c
    libsrc/gadget/uibutton.c
    libsrc/gadget/util2d.c
  )
  add_library(dromed_sound STATIC
    libsrc/sound/dmixer.cpp
    libsrc/sound/sndfmt.cpp
    libsrc/sound/dsnddynf.cpp
    libsrc/sound/volconv.cpp
    libsrc/sound/lgsndid.cpp
    libsrc/sound/mixer.cpp
    libsrc/sound/sample.cpp
    libsrc/sound/dsample.cpp
    libsrc/sound/fade.cpp
  )
  if(TOOLCHAIN STREQUAL "vs2008")
    target_sources(dromed_sound PRIVATE
      libsrc/sound/almixer.cpp
    )
    target_include_directories(dromed_sound PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/openal/include)
  endif()
  add_library(dromed_actmovie STATIC
    libsrc/actmovie/actmovie.cpp
    libsrc/actmovie/getlgvid.cpp
    libsrc/actmovie/amguids.cpp
    libsrc/actmovie/imutil.cpp
    libsrc/actmovie/amalloc.cpp
  )
  add_library(dromed_quartz STATIC
    3rdparty/dxmedia/quartz.c
  )
  add_library(dromed_r3d STATIC
    libsrc/r3d/block.c
    libsrc/r3d/space.c
    libsrc/r3d/xfrmglob.c
    libsrc/r3d/prim.c
    libsrc/r3d/clip.c
    libsrc/r3d/init.c
    libsrc/r3d/frame.c
    libsrc/r3d/object.c
    libsrc/r3d/ctxtmang.c
    libsrc/r3d/view.c
    libsrc/r3d/primmisc.c
    libsrc/r3d/clipglob.c
    libsrc/r3d/primg2.c
    libsrc/r3d/primd3d.c
    libsrc/r3d/clipplan.c
    libsrc/r3d/primnull.c
    libsrc/r3d/xfrmlin.c
    libsrc/r3d/xfrmunsc.c
    libsrc/r3d/xfrmproj.c
    libsrc/r3d/xfrmclip.c
    libsrc/r3d/primfunc.c
    libsrc/r3d/ixmgr.c
    libsrc/r3d/ctxtguts.c
    libsrc/r3d/primconv.c
    libsrc/r3d/clipunsc.c
    libsrc/r3d/clipproj.c
    libsrc/r3d/projfast.asm
    libsrc/r3d/clipclip.c
    libsrc/r3d/primix.c
    libsrc/r3d/primpoly.c
    libsrc/r3d/primface.c
    libsrc/r3d/cliptest.c
  )
  add_library(dromed_star STATIC
    libsrc/star/star.c
  )
  add_library(dromed_mp STATIC
    libsrc/mp/mplist.c
    libsrc/mp/mpinit.c
    libsrc/mp/qt.c
    libsrc/mp/mpmot.c
    libsrc/mp/mpupdate.c
    libsrc/mp/mpstack.c
    libsrc/mp/mpvm.c
    libsrc/mp/mploco.c
  )
  add_library(dromed_md STATIC
    libsrc/md/md_lgd3d.cpp
    libsrc/md/render.c
    libsrc/md/light.c
    libsrc/md/hitdet.c
    libsrc/md/scale.c
    libsrc/md/fancy.c
    libsrc/md/utils.c
    libsrc/md/cbacks.c
    libsrc/md/mipmap.c
    libsrc/md/mxstack.c
  )
  add_library(dromed_dump STATIC
    libsrc/dump/filefind.c
    libsrc/dump/pcxdump.c
    libsrc/dump/bmpdump.c
  )
  add_library(dromed_vec2 STATIC
    libsrc/vec2/vec2.c
  )
  add_library(dromed_prof STATIC
    libsrc/prof/prof.c
  )
  add_library(dromed_darkloop STATIC
    libsrc/darkloop/gloopcli.cpp
    libsrc/darkloop/loopfact.cpp
    libsrc/darkloop/loopman.cpp
    libsrc/darkloop/loopmode.cpp
    libsrc/darkloop/loopdisp.cpp
    libsrc/darkloop/loopque.cpp
  )
  add_library(dromed_lgd3d STATIC
    libsrc/lgd3d/d6intfc.cpp
    libsrc/lgd3d/d6enum.cpp
    libsrc/lgd3d/texmgr.cpp
    libsrc/lgd3d/d6sts_s.cpp
    libsrc/lgd3d/lgd3d_g.cpp
    libsrc/lgd3d/d6frame.cpp
    libsrc/lgd3d/d6render.cpp
    libsrc/lgd3d/d6prim_s.cpp
    libsrc/lgd3d/d6base.cpp
    libsrc/lgd3d/lgsurf.cpp
    libsrc/lgd3d/d6over.cpp
  )
  # NewDark adds the Direct3D 9 renderer beside the d6 one (docs/notes/project_newdark_d8_renderer.md).
  if(TOOLCHAIN STREQUAL "vs2008")
    target_sources(dromed_lgd3d PRIVATE
      libsrc/lgd3d/d8Frame.cpp
      libsrc/lgd3d/d8Prim_s.cpp
      libsrc/lgd3d/d8Render.cpp
      libsrc/lgd3d/d8States.cpp
    )
    target_include_directories(dromed_lgd3d PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dx9sdk/include)
  endif()
  add_library(dromed_mm STATIC
    libsrc/mm/render.c
    libsrc/mm/debug.c
    libsrc/mm/sortpoly.c
    libsrc/mm/xformseg.c
  )
  add_library(dromed_script STATIC
    libsrc/script/scrptman.cpp
    libsrc/script/scrptmsg.cpp
    libsrc/script/persist.cpp
    libsrc/script/multparm.cpp
    libsrc/script/scrobj.cpp
    libsrc/script/smodinfo.cpp
    libsrc/script/scrptmod.cpp
    libsrc/script/nullsrv.cpp
  )
  add_library(dromed_sndsrc STATIC
    libsrc/sndsrc/sndsrc.cpp
    libsrc/sndsrc/imaadpcm.cpp
    libsrc/sndsrc/sndsrcid.cpp
    libsrc/sndsrc/playlist.cpp
    libsrc/sndsrc/refill.cpp
    libsrc/sndsrc/nrezseg.cpp
    libsrc/sndsrc/memseg.cpp
    libsrc/sndsrc/fileseg.cpp
    libsrc/sndsrc/rezseg.cpp
    libsrc/sndsrc/sndseg.cpp
  )
  add_library(dromed_ref STATIC
    libsrc/ref/refsys.c
  )
  add_library(dromed_sdesc STATIC
    libsrc/sdesc/sdestool.cpp
    libsrc/sdesc/sdesc.cpp
  )
  add_library(dromed_arq STATIC
    libsrc/arq/arq.cpp
    libsrc/arq/arqthred.cpp
    libsrc/arq/arqqueue.cpp
    libsrc/arq/arqreq.cpp
    libsrc/arq/arqstrrd.cpp
  )
  add_library(dromed_inputbnd STATIC
    libsrc/inputbnd/ibvarman.cpp
    libsrc/inputbnd/inputbnd.cpp
    libsrc/inputbnd/inpguid.cpp
    libsrc/inputbnd/ibmapper.cpp
  )
  add_library(dromed_namedres STATIC
    libsrc/namedres/resids.c
    libsrc/namedres/resman.cpp
    libsrc/namedres/mdlrstyp.cpp
    libsrc/namedres/fonrstyp.cpp
    libsrc/namedres/sndrstyp.cpp
    libsrc/namedres/strrstyp.cpp
    libsrc/namedres/imgrstyp.cpp
    libsrc/namedres/palrstyp.cpp
    libsrc/namedres/imghack.cpp
    libsrc/namedres/defresm.cpp
    libsrc/namedres/resmanhs.cpp
    libsrc/namedres/resmanx.cpp
    libsrc/namedres/resarq.cpp
    libsrc/namedres/resstat.cpp
    libsrc/namedres/zipstfct.cpp
    libsrc/namedres/storeman.cpp
    libsrc/namedres/binrstyp.cpp
    libsrc/namedres/pathutil.cpp
    libsrc/namedres/search.cpp
    libsrc/namedres/stdres.cpp
    libsrc/namedres/resstr.cpp
    libsrc/namedres/restmpl.cpp
    libsrc/namedres/resimage.cpp
    libsrc/namedres/respall.cpp
    libsrc/namedres/storezip.cpp
    libsrc/namedres/defstfct.cpp
    libsrc/namedres/storedir.cpp
    libsrc/namedres/storbase.cpp
    libsrc/namedres/resutil.cpp
    libsrc/namedres/tga.cpp
    libsrc/namedres/pcx.cpp
    libsrc/namedres/gif.cpp
    libsrc/namedres/cel.cpp
    libsrc/namedres/bmp.cpp
    libsrc/namedres/zipstrm.cpp
    libsrc/namedres/filestrm.cpp
  )
  if(NOT TOOLCHAIN STREQUAL "vs2008")
    target_sources(dromed_namedres PRIVATE
      libsrc/namedres/inflate.c
      libsrc/namedres/infblock.c
      libsrc/namedres/adler32.c
      libsrc/namedres/zutil.c
      libsrc/namedres/infcodes.c
      libsrc/namedres/inftrees.c
      libsrc/namedres/infutil.c
      libsrc/namedres/inffast.c
    )
  endif()
  add_library(dromed_net STATIC
    libsrc/net/net.cpp
    libsrc/net/msgdelay.cpp
  )
  add_library(dromed_random STATIC
    libsrc/random/xrandom.cpp
    libsrc/random/rng.cpp
    libsrc/random/rngcong.cpp
    libsrc/random/rngfib.cpp
  )
  add_library(dromed_tagfile STATIC
    libsrc/tagfile/tagfile.cpp
    libsrc/tagfile/memtfile.cpp
    libsrc/tagfile/vernum.cpp
  )
  add_library(dromed_entrywin STATIC
    winsrc/entrywin/entrywin.cpp
  )

  if(TOOLCHAIN STREQUAL "vc6")
    add_executable(dromed WIN32
      src/framewrk/rand.cpp
      src/framewrk/2dapp.c
      src/render/animlgt.c
      src/render/anim_txt.c
      src/sound/ambient.c
      src/sound/ambprop.cpp
      src/framewrk/appname.c
      src/sound/appsfx.cpp
      src/sound/auxsnd.cpp
      src/framewrk/backup.cpp
      src/framewrk/basemode.c
      src/motion/biploop.c
      src/framewrk/buftagf.cpp
      src/ui/bugterm.c
      src/ui/buttpanl.cpp
      src/render/camera.c
      src/camguids.c
      src/engfeat/campaign.cpp
      src/framewrk/chainevt.cpp
      src/ui/cmdterm.c
      src/framewrk/cfgtool.c
      src/framewrk/command.c
      src/render/corona.cpp
      src/framewrk/ctag.cpp
      src/framewrk/ctagset.cpp
      src/render/ctimer.c
      src/framewrk/dappsink.cpp
      src/framewrk/dbfile.cpp
      src/framewrk/diskfree.c
      src/framewrk/dispatch.cpp
      src/engfeat/dmgmodel.cpp
      src/engfeat/dmgprop.cpp
      src/engfeat/dmgreact.cpp
      src/framewrk/dxload.cpp
      src/framewrk/dxwrndlg.cpp
      src/sim/eosapp.c
      src/object/edittool.cpp
      src/render/elitprop.cpp
      src/sound/esnd.cpp
      src/sound/esndprop.cpp
      src/render/face.cpp
      src/render/family.c
      src/render/fas.c
      src/render/fasprop.cpp
      src/framewrk/fcytagdb.cpp
      src/framewrk/filetool.c
      src/editor/filemenu.cpp
      src/framewrk/filevar.cpp
      src/engfeat/flinder.cpp
      src/sim/flowarch.cpp
      src/framewrk/fltang.cpp
      src/render/fxdisk.c
      src/render/fxspan.c
      src/framewrk/gameapp.c
      src/framewrk/gameinfo.c
      src/framewrk/gamemode.c
      src/framewrk/gamescrn.cpp
      src/framewrk/gamestr.cpp
      src/framewrk/gametool.cpp
      src/framewrk/gen_bind.cpp
      src/render/gfxprop.cpp
      src/goof.c
      src/render/h2ocolor.cpp
      src/framewrk/hchkthrd.cpp
      src/render/hdskprop.cpp
      src/sim/headmove.c
      src/engfeat/hpprop.cpp
      src/engfeat/trapprop.cpp
      src/render/hud.c
      src/inc.c
      src/win32/inclex.c
      src/inctab.cpp
      src/framewrk/init.cpp
      src/framewrk/imgsrc.cpp
      src/editor/isdesced.cpp
      src/ui/joypoll.cpp
      src/killme.cpp
      src/render/litprop.cpp
      src/framewrk/lresdisk.c
      src/framewrk/loopapp.c
      src/render/medmotn.c
      src/render/meshray.c
      src/render/meshtex.cpp
      src/sound/metasnd.cpp
      src/render/mipmap.c
      src/render/missrend.cpp
      src/render/mlight.c
      src/render/mnamprop.cpp
      src/render/mnumprop.cpp
      src/editor/menus.cpp
      src/render/meshatt.cpp
      src/ui/movie.cpp
      src/motion/mputil.cpp
      src/framewrk/namemap.cpp
      src/render/objcast.cpp
      src/render/objlight.c
      src/sim/objloop.cpp
      src/render/objmodel.c
      src/sim/objpos.cpp
      src/object/objremap.cpp
      src/object/objlpars.cpp
      src/object/objproxy.cpp
      src/render/objshape.cpp
      src/object/objsys.cpp
      src/sim/objtool.cpp
      src/render/objtxtrp.cpp
      src/engfeat/ownprop.cpp
      src/render/palette.c
      src/render/palmgr.c
      src/ui/panlmode.cpp
      src/render/particle.cpp
      src/render/partprop.cpp
      src/render/pgroup.c
      src/render/pgrpprop.cpp
      src/script/pgrpscpt.cpp
      src/framewrk/pickgame.cpp
      src/sim/playtest.cpp
      src/sim/plyablty.cpp
      src/sim/plycbllm.cpp
      src/sim/plyrhdlr.cpp
      src/sim/plyrloop.cpp
      src/sim/plyrmode.cpp
      src/sim/plyrmov.cpp
      src/sim/plyrvmot.cpp
      src/render/portapp.c
      src/render/porthlpr.c
      src/editor/prompts.c
      src/motion/puppet.cpp
      src/motion/pupprop.cpp
      src/engfeat/quest.cpp
      src/engfeat/quesloop.cpp
      src/engfeat/questprp.cpp
      src/framewrk/r3app.c
      src/render/render.cpp
      src/render/rendloop.c
      src/render/skyrend.cpp
      src/render/skyobj.cpp
      src/render/starobj.cpp
      src/render/celstobj.cpp
      src/render/distobj.cpp
      src/render/cloudobj.cpp
      src/render/rendobj.cpp
      src/render/rendphys.cpp
      src/render/rendprop.cpp
      src/render/rendptst.c
      src/framewrk/reputil.cpp
      src/framewrk/resapp.c
      src/framewrk/resdata.c
      src/framewrk/resloop.c
      src/render/rndflash.cpp
      src/render/scalprop.cpp
      src/sound/schdb.cpp
      src/sound/schema.cpp
      src/sound/schloop.c
      src/sound/schmsg.cpp
      src/sound/schprop.cpp
      src/sound/schsamps.cpp
      src/render/scrnloop.c
      src/framewrk/scrndump.cpp
      src/render/scrnman.c
      src/render/scrnmode.cpp
      src/render/scrnovls.cpp
      src/script/scrptcmd.cpp
      src/script/scrptprp.cpp
      src/script/scrptst.cpp
      src/sim/simloop.c
      src/sim/simman.cpp
      src/sim/simtime.cpp
      src/engfeat/simpdmg.cpp
      src/sim/simstate.c
      src/render/slitprop.cpp
      src/sound/sndgen.cpp
      src/sound/sndloop.cpp
      src/sound/sndprop.cpp
      src/sound/songi.cpp
      src/sound/songid.cpp
      src/sound/songplri.cpp
      src/sound/songutil.cpp
      src/sound/songfile.cpp
      src/sound/songmiss.cpp
      src/sound/spchdom.cpp
      src/sound/spchprop.cpp
      src/sound/spchvoc.cpp
      src/sound/speech.cpp
      src/render/sphrcst.cpp
      src/render/sprkprop.cpp
      src/render/starhack.c
      src/editor/status.c
      src/framewrk/subscrip.cpp
      src/engfeat/susprop.cpp
      src/framewrk/tagdbdat.cpp
      src/framewrk/tagdbin.cpp
      src/framewrk/tagdbkey.cpp
      src/framewrk/tagdbkst.cpp
      src/framewrk/tagdbout.cpp
      src/framewrk/tagpatch.cpp
      src/framewrk/tagremap.cpp
      src/render/tanimprp.cpp
      src/testloop.c
      src/testmode.c
      src/render/texmem.c
      src/render/texprop.c
      src/render/texsave.cpp
      src/render/textarch.cpp
      src/ui/uiamov.cpp
      src/ui/uianim.cpp
      src/ui/uiapp.cpp
      src/ui/uigame.cpp
      src/ui/uiloop.cpp
      src/version.c
      src/render/weather.cpp
      src/ui/winui.c
      src/sim/wrbsp.cpp
      src/sim/wrcell.c
      src/sim/wrloop.cpp
      src/ai/aitest.cpp
      src/ai/aimplyr.cpp
      src/ai/aimttest.cpp
      src/ai/aie3bhv.cpp
      src/ai/ai.cpp
      src/ai/aiavoid.cpp
      src/ai/aibasabl.cpp
      src/ai/aibasact.cpp
      src/ai/aibasatm.cpp
      src/ai/aibasbhv.cpp
      src/ai/aibasctm.cpp
      src/ai/aibasmrg.cpp
      src/ai/aicmds.cpp
      src/ai/aicnvman.cpp
      src/ai/aicnvrse.cpp
      src/ai/aicpinit.cpp
      src/ai/aicretsz.cpp
      src/ai/aidetect.cpp
      src/ai/aidoormr.cpp
      src/ai/aigoal.cpp
      src/ai/aiground.cpp
      src/ai/aiinform.cpp
      src/ai/aiman.cpp
      src/ai/aimove.cpp
      src/ai/aimovmot.cpp
      src/ai/aimovsug.cpp
      src/ai/ainet.cpp
      src/ai/ainetman.cpp
      src/ai/aiphycst.cpp
      src/ai/aiproxy.cpp
      src/ai/aisound.cpp
      src/ai/aisubabl.cpp
      src/ai/aisuslnk.cpp
      src/ai/aistdmrg.cpp
      src/ai/aistun.cpp
      src/ai/aitagtyp.cpp
      src/ai/aiteams.cpp
      src/ai/aitrig.cpp
      src/ai/aiutils.cpp
      src/ai/aiwr.cpp
      src/ai/aicbhtoh.cpp
      src/ai/aicbrbkp.cpp
      src/ai/aicbrclo.cpp
      src/ai/aicbrfle.cpp
      src/ai/aicbridl.cpp
      src/ai/aicbrloc.cpp
      src/ai/aicbrmod.cpp
      src/ai/aicbrmpr.cpp
      src/ai/aicbrnew.cpp
      src/ai/aicbrngd.cpp
      src/ai/aicbrsht.cpp
      src/ai/aicbrstl.cpp
      src/ai/aicbrstr.cpp
      src/ai/aicbrwnd.cpp
      src/ai/aicdummy.cpp
      src/ai/aicombat.cpp
      src/ai/aidev.cpp
      src/ai/aimultcb.cpp
      src/ai/aisubcb.cpp
      src/ai/aiturret.cpp
      src/ai/aicamera.cpp
      src/ai/aiturbs.cpp
      src/ai/aiqdeath.cpp
      src/ai/aiactfol.cpp
      src/ai/aiactfrb.cpp
      src/ai/aiactinv.cpp
      src/ai/aiactjr.cpp
      src/ai/aiactjs.cpp
      src/ai/aiactjsc.cpp
      src/ai/aiactlch.cpp
      src/ai/aiactloc.cpp
      src/ai/aiactmot.cpp
      src/ai/aiactmov.cpp
      src/ai/aiactmrk.cpp
      src/ai/aiactprx.cpp
      src/ai/aiactps.cpp
      src/ai/aiactori.cpp
      src/ai/aiactrng.cpp
      src/ai/aiactseq.cpp
      src/ai/aiactsnd.cpp
      src/ai/aiactwnd.cpp
      src/ai/aiactwt.cpp
      src/ai/aiactnet.cpp
      src/ai/aigunact.cpp
      src/ai/aialtrsp.cpp
      src/ai/aibdyrsp.cpp
      src/ai/aicnvabl.cpp
      src/ai/aicbtrsp.cpp
      src/ai/aideath.cpp
      src/ai/aidefend.cpp
      src/ai/aiflee.cpp
      src/ai/aifollow.cpp
      src/ai/aifreeze.cpp
      src/ai/aiidle.cpp
      src/ai/aiinvest.cpp
      src/ai/ailisten.cpp
      src/ai/aincbdmg.cpp
      src/ai/ainoncbt.cpp
      src/ai/aipatrol.cpp
      src/ai/aisusrsp.cpp
      src/ai/aithreat.cpp
      src/ai/aiwatch.cpp
      src/ai/aipath.cpp
      src/ai/aipathdb.cpp
      src/ai/aipthcst.cpp
      src/ai/aipthdbg.cpp
      src/ai/aipthedg.cpp
      src/ai/aipthfnd.cpp
      src/ai/aipthloc.cpp
      src/ai/aipthobb.cpp
      src/ai/aipthzon.cpp
      src/sim/rpathfnd.cpp
      src/sim/rpaths.cpp
      src/ai/aiprabil.cpp
      src/ai/aiprconv.cpp
      src/ai/aiprcore.cpp
      src/ai/aiprdev.cpp
      src/ai/aiprattr.cpp
      src/ai/aiprops.cpp
      src/ai/aiprrngd.cpp
      src/ai/aiprutil.cpp
      src/ai/aiaware.cpp
      src/ai/aibassns.cpp
      src/ai/aisgtcst.cpp
      src/ai/aisnsdev.cpp
      src/ai/aidbdraw.cpp
      src/ai/aidbgcmp.cpp
      src/ai/aidebug.cpp
      src/ai/aireport.cpp
      src/ai/aiscrabl.cpp
      src/ai/aiscrpt.cpp
      src/render/antxloop.cpp
      src/script/atkscrpt.cpp
      src/framewrk/cdplayer.cpp
      src/engfeat/contain.cpp
      src/engfeat/contprop.cpp
      src/script/contscrs.cpp
      src/render/ctrast.c
      src/sim/doorphys.cpp
      src/sim/doorprop.cpp
      src/sim/doorrep.cpp
      src/actreact/dumbprox.cpp
      src/render/dyntex.cpp
      src/engfeat/engfeat.cpp
      src/engfeat/frobprop.cpp
      src/engfeat/frobctrl.cpp
      src/engfeat/invlimb.cpp
      src/engfeat/inv_rend.c
      src/engfeat/invrndpr.cpp
      src/script/keyscpt.cpp
      src/engfeat/keysys.cpp
      src/render/litnet.cpp
      src/engfeat/lockprop.cpp
      src/script/lockscpt.cpp
      src/ui/panltool.cpp
      src/engfeat/pick.cpp
      src/engfeat/preldprp.cpp
      src/sim/prjctile.cpp
      src/engfeat/quesreac.cpp
      src/script/scrptnet.cpp
      src/sound/sndnet.cpp
      src/engfeat/tweqctrl.cpp
      src/engfeat/tweqprop.cpp
      src/engfeat/tweqrep.cpp
      src/script/twqscrpt.cpp
      src/sound/vocore.cpp
      src/engfeat/weapcb.cpp
      src/engfeat/weaphalo.cpp
      src/engfeat/weaphit.cpp
      src/engfeat/weapon.cpp
      src/engfeat/weapprop.cpp
      src/engfeat/weapreac.cpp
      src/engfeat/gunflash.cpp
      src/engfeat/gunprop.cpp
      src/engfeat/projprop.cpp
      src/engfeat/projlink.cpp
      src/render/antxscpt.cpp
      src/script/arscript.cpp
      src/script/bodscrpt.cpp
      src/script/camscrpt.cpp
      src/script/datascrs.cpp
      src/script/dmgscrpt.cpp
      src/script/drscrpt.cpp
      src/script/engscrpt.cpp
      src/script/linkscpt.cpp
      src/script/litscrpt.cpp
      src/script/netscrpt.cpp
      src/script/objscrpt.cpp
      src/script/propscpt.cpp
      src/script/pupscrpt.cpp
      src/script/quesscpt.cpp
      src/editor/sdesparm.cpp
      src/script/sndscrpt.cpp
      src/script/mtscrpt.cpp
      src/sim/room.cpp
      src/sim/roomcore.cpp
      src/sim/roomeax.cpp
      src/sim/roomloop.cpp
      src/sim/roompa.cpp
      src/sim/roomprop.cpp
      src/sim/rooms.cpp
      src/sim/roomutil.cpp
      src/script/rooscrpt.cpp
      src/sim/rportal.cpp
      src/sound/psndapi.cpp
      src/sound/psnd.cpp
      src/sound/psndinst.cpp
      src/sound/sndframe.cpp
      src/actreact/arloop.cpp
      src/actreact/contag8r.cpp
      src/sound/esndreac.cpp
      src/frezreac.cpp
      src/actreact/mediag8r.cpp
      src/actreact/periog8r.cpp
      src/actreact/pg8rnull.cpp
      src/actreact/propag8n.cpp
      src/actreact/propag8r.cpp
      src/engfeat/propreac.cpp
      src/actreact/radiag8r.cpp
      src/actreact/reaction.cpp
      src/actreact/ssrclife.cpp
      src/actreact/stimbase.cpp
      src/actreact/stimguid.c
      src/actreact/stimsens.cpp
      src/actreact/stimsrc.cpp
      src/actreact/stimul8r.cpp
      src/actreact/stimuli.cpp
      src/object/bintrait.cpp
      src/object/dataops.cpp
      src/object/link.cpp
      src/object/linkarry.cpp
      src/object/linkboth.cpp
      src/object/lnkbyobj.cpp
      src/object/linkdb.cpp
      src/object/linkdata.cpp
      src/object/linkguid.c
      src/object/linkman.cpp
      src/object/linkset.cpp
      src/object/linkstor.cpp
      src/object/linkqdb.cpp
      src/object/linkprop.cpp
      src/object/osetlnkq.cpp
      src/object/proplink.cpp
      src/object/lnkquery.cpp
      src/object/querynul.cpp
      src/object/relation.cpp
      src/object/relinver.cpp
      src/object/relnull.cpp
      src/object/prcniter.cpp
      src/object/propdb.cpp
      src/object/property.cpp
      src/object/propface.cpp
      src/object/propguid.c
      src/object/propstor.cpp
      src/object/propman.cpp
      src/object/propmix.cpp
      src/object/propnull.cpp
      src/object/propobsv.cpp
      src/object/propsave.cpp
      src/object/strprop.cpp
      src/object/donorq.cpp
      src/object/nameprop.cpp
      src/object/objquery.cpp
      src/object/traisrch.cpp
      src/object/trait.cpp
      src/object/traitq.cpp
      src/object/traitiid.c
      src/object/traitlnk.cpp
      src/object/traitman.cpp
      src/object/traitnul.cpp
      src/object/traitprp.cpp
      src/object/traitqdb.cpp
      src/object/trcache.cpp
      src/object/trcachui.cpp
      src/physics/collide.cpp
      src/physics/collprop.cpp
      src/physics/objmedia.cpp
      src/physics/phclimb.cpp
      src/physics/phclsn.cpp
      src/physics/phcontct.cpp
      src/physics/phcore.cpp
      src/physics/phctrl.cpp
      src/physics/phdyn.cpp
      src/physics/phflist.cpp
      src/physics/phlisten.cpp
      src/physics/phmedtr.cpp
      src/physics/phmod.cpp
      src/physics/phmodbsp.cpp
      src/physics/phmodobb.cpp
      src/physics/phmods.cpp
      src/physics/phmodsph.cpp
      src/physics/phmodutl.cpp
      src/physics/phmsg.cpp
      src/physics/phnet.cpp
      src/physics/phoprop.cpp
      src/physics/phprop.cpp
      src/physics/phprops.cpp
      src/physics/phmtprop.cpp
      src/physics/phmterr.cpp
      src/physics/phmtscpt.cpp
      src/physics/phppscpt.cpp
      src/physics/phref.cpp
      src/physics/phreport.cpp
      src/physics/phsubmod.cpp
      src/physics/phscrpt.cpp
      src/physics/phterr.cpp
      src/physics/phtxtprp.cpp
      src/physics/phutils.cpp
      src/physics/physapi.cpp
      src/physics/physcast.cpp
      src/physics/physloop.cpp
      src/physics/phystest.cpp
      src/physics/phmotion.cpp
      src/physics/phmodata.c
      src/physics/phmoapi.cpp
      src/physics/gunproj.cpp
      src/physics/gunvhot.cpp
      src/physics/spherchk.cpp
      src/motion/cerebllm.cpp
      src/motion/mcoord.cpp
      src/motion/mvrcmbat.cpp
      src/motion/mvrflags.cpp
      src/motion/mvrflbow.cpp
      src/motion/mvrgact.cpp
      src/motion/mvrgloco.cpp
      src/motion/mvrplyr.cpp
      src/motion/mvrsngle.cpp
      src/motion/mvrutils.cpp
      src/motion/motdb.cpp
      src/motion/motdesc.cpp
      src/motion/motdmnge.c
      src/motion/motmngr.cpp
      src/motion/motschem.cpp
      src/motion/motset.cpp
      src/motion/motprop.cpp
      src/motion/mschutil.cpp
      src/motion/mskilset.cpp
      src/motion/crattach.cpp
      src/motion/crwpnlst.cpp
      src/motion/crwpnapi.cpp
      src/motion/crwpnlup.cpp
      src/motion/creature.cpp
      src/motion/cretprop.cpp
      src/motion/crexp.cpp
      src/motion/crjoint.cpp
      src/dark/ccrappa.cpp
      src/dark/ccrbug.cpp
      src/dark/ccrcray.cpp
      src/dark/ccrhuman.cpp
      src/dark/crappa.c
      src/dark/crbowarm.c
      src/dark/crbugbst.c
      src/dark/crburrik.c
      src/dark/crconsta.c
      src/dark/crcraymn.c
      src/dark/crcutty.c
      src/dark/crfrog.c
      src/dark/crhumand.c
      src/dark/crplyarm.c
      src/dark/crrobot.cpp
      src/dark/crrope.c
      src/dark/crsmrbt.cpp
      src/dark/crspider.c
      src/dark/crspidsm.c
      src/dark/crspirob.c
      src/dark/crsweel.c
      src/dark/crzombie.c
      src/dark/drkdiff.cpp
      src/dark/drkpanl.cpp
      src/dark/drkuires.cpp
      src/dark/filepanl.cpp
      src/dark/gadslide.cpp
      src/framewrk/initedit.cpp
      src/sim/avatar.cpp
      src/framewrk/ctagnet.cpp
      src/sim/ghost.cpp
      src/sim/ghosthst.cpp
      src/sim/ghostlst.cpp
      src/sim/ghostlup.cpp
      src/sim/ghostmsg.cpp
      src/sim/ghostphy.cpp
      src/sim/ghostrcv.cpp
      src/sim/ghostshk.cpp
      src/sim/ghostsnd.cpp
      src/sim/ghostwep.cpp
      src/framewrk/netman.cpp
      src/framewrk/netmsg.cpp
      src/framewrk/netprops.cpp
      src/framewrk/netsim.cpp
      src/framewrk/netsynch.cpp
      src/framewrk/netvoice.cpp
      src/sim/objnet.cpp
      src/portal/wrfunc.c
      src/portal/wrcast.c
      src/portal/bspsphr.c
      src/portal/wrdbg.c
      src/portal/animlit.c
      src/portal/portsurf.c
      src/portal/surfaces.c
      src/portal/ptsurf.asm
      src/portal/ptsurf16.asm
      src/portal/pt_mem.c
      src/portal/pt_clut.c
      src/portal/portal.c
      src/portal/portdraw.c
      src/portal/ptdrawhw.c
      src/portal/portlit.c
      src/portal/portclip.c
      src/portal/portbg.c
      src/portal/portpick.c
      src/portal/portwatr.c
      src/portal/porthw.c
      src/portal/portsky.c
      src/csg/bspdata.c
      src/csg/csg.c
      src/csg/csgalloc.c
      src/csg/csgbbox.c
      src/csg/csgbuild.c
      src/csg/csgcheck.c
      src/csg/csgclip.c
      src/csg/csgcube.c
      src/csg/csgemit.c
      src/csg/csgfind.c
      src/csg/csgmedia.c
      src/csg/csgmerge.c
      src/csg/csgutil.c
      src/csg/hep.c
      src/editor/alignpnp.c
      src/editor/arcmd.cpp
      src/editor/areapnp.c
      src/editor/autovbr.cpp
      src/editor/bldstair.c
      src/editor/brinfo.c
      src/editor/brlist.c
      src/editor/brloop.c
      src/editor/brobjpro.cpp
      src/editor/brquery.c
      src/editor/brrend.c
      src/editor/brundo.c
      src/editor/brushgfh.c
      src/editor/cmdbutts.c
      src/editor/convpred.cpp
      src/editor/cvtbrush.c
      src/editor/cyclegad.c
      src/editor/doorblok.cpp
      src/editor/dragbox.c
      src/editor/editapp.c
      src/editor/editbr.c
      src/editor/editgeom.c
      src/editor/editmode.c
      src/editor/editobj.c
      src/editor/editsave.c
      src/editor/edmedmo.c
      src/editor/famedit.cpp
      src/editor/flowpnp.cpp
      src/editor/fvaredit.cpp
      src/editor/ged_csg.cpp
      src/editor/ged_line.c
      src/editor/ged_rend.c
      src/editor/ged_rmap.c
      src/editor/ged_room.cpp
      src/editor/ged_undo.c
      src/editor/gedit.c
      src/editor/gedmedmo.c
      src/editor/gfhfrobs.c
      src/editor/gridpnp.c
      src/editor/gridsnap.c
      src/editor/hilight.c
      src/editor/lightpnp.c
      src/editor/linkdraw.cpp
      src/editor/lnktrait.cpp
      src/script/miscrprt.cpp
      src/editor/modalui.c
      src/editor/namenum.c
      src/editor/objedit.cpp
      src/editor/objhistp.cpp
      src/editor/objpnp.c
      src/editor/phproped.cpp
      src/editor/pnpslide.cpp
      src/editor/pnptools.c
      src/editor/primal.c
      src/editor/primshap.c
      src/editor/preload.c
      src/editor/psdialog.cpp
      src/editor/quesedit.cpp
      src/editor/rendtest.c
      src/framewrk/report.cpp
      src/framewrk/reprthlp.cpp
      src/editor/roompnp.c
      src/editor/roomtowr.cpp
      src/editor/saveload.c
      src/sound/schfile.cpp
      src/win32/schlex.c
      src/sound/schyacc.c
      src/editor/scrnedit.cpp
      src/editor/scrpted.cpp
      src/editor/simpwrap.cpp
      src/win32/songlex.c
      src/songyacc.c
      src/sound/songedtr.cpp
      src/editor/stupidui.c
      src/editor/strprped.cpp
      src/editor/swappnp.c
      src/editor/swapregs.c
      src/editor/swaptool.c
      src/editor/txtpnp.c
      src/editor/txtrpal.cpp
      src/editor/uiedit.c
      src/editor/undoredo.c
      src/editor/userpnp.c
      src/editor/vbrush.c
      src/editor/vbr_math.c
      src/editor/viewmgr.c
      src/editor/vmwincfg.c
      src/editor/vslider.c
      src/editor/vumanui.c
      src/editor/lgslider.c
      src/editor/medflbox.c
      src/editor/mediface.c
      src/editor/medmenu.cpp
      src/editor/medplay.cpp
      src/win32/mschlex.c
      src/motion/mschyacc.c
      src/editor/motedit.cpp
      src/editor/brprop.cpp
      src/editor/linkedit.cpp
      src/editor/objed.cpp
      src/editor/otreeed.cpp
      src/editor/proped.cpp
      src/editor/proptest.cpp
      src/dark/bowscrpt.cpp
      src/render/camovl.cpp
      src/dark/drk_bind.cpp
      src/dark/drkamap.cpp
      src/dark/drkbook.cpp
      src/dark/drkbreth.cpp
      src/dark/drkchkcd.cpp
      src/dark/drkcmbat.cpp
      src/dark/drkcolor.cpp
      src/dark/drkcret.cpp
      src/dark/drkdebrf.cpp
      src/dark/drkfixpr.cpp
      src/dark/drkgame.cpp
      src/dark/drkgoalr.cpp
      src/dark/drkldout.cpp
      src/dark/drklinks.cpp
      src/dark/drkloop.c
      src/dark/drkloot.cpp
      src/dark/drkmenu.cpp
      src/dark/drkmislp.cpp
      src/dark/drkmiss.cpp
      src/dark/drkmsg.cpp
      src/dark/drkinit.c
      src/dark/drkinv.c
      src/dark/drkinvsc.cpp
      src/dark/drkinvpr.cpp
      src/dark/drkinvui.cpp
      src/dark/drknetap.cpp
      src/dark/drkoptmn.cpp
      src/dark/drkplinv.cpp
      src/dark/drkpwscr.cpp
      src/dark/drkpwups.cpp
      src/dark/drkreact.cpp
      src/dark/drkrend.c
      src/dark/drkreprt.cpp
      src/dark/drkrloop.c
      src/dark/drkrutil.cpp
      src/dark/drksave.cpp
      src/dark/drksavui.cpp
      src/dark/drksloop.cpp
      src/dark/drkscrpt.cpp
      src/dark/drkscrs.cpp
      src/dark/drksound.cpp
      src/dark/drkstats.cpp
      src/dark/drkuiscr.cpp
      src/dark/drkvo.cpp
      src/dark/drkwbdy.cpp
      src/dark/drkwbow.cpp
      src/dark/drkwbow_.c
      src/dark/drkwswd.cpp
      src/dark/drkwswd_.c
      src/dark/eidosafp.c
      src/dark/lootprop.cpp
      src/dark/picklock.cpp
      src/dark/pickscrs.cpp
      src/dark/plyscrpt.cpp
      src/dark/metagame.cpp
      src/dark/statbar.cpp
      src/dark/vismeter.cpp
      src/dark/weapscpt.cpp
      src/dark/meshprop.cpp
      src/dark/darkai.cpp
      src/dark/daielem.cpp
      src/dark/daifrog.cpp
      src/dark/daicam.cpp
      src/darktabl.c
      src/framewrk/main.cpp
    )
  else()
    # NewDark link order, from the target DromEd function layout (scripts/tools/nd_order_cmp.py).
    add_executable(dromed WIN32
      src/script/arscript.cpp
      src/script/atkscrpt.cpp
      src/script/bodscrpt.cpp
      src/script/camscrpt.cpp
      src/script/contscrs.cpp
      src/script/datascrs.cpp
      src/script/dmgscrpt.cpp
      src/script/drscrpt.cpp
      src/script/engscrpt.cpp
      src/script/keyscpt.cpp
      src/script/linkscpt.cpp
      src/script/litscrpt.cpp
      src/script/lockscpt.cpp
      src/script/mtscrpt.cpp
      src/script/netscrpt.cpp
      src/script/objscrpt.cpp
      src/script/pgrpscpt.cpp
      src/script/propscpt.cpp
      src/script/pupscrpt.cpp
      src/script/quesscpt.cpp
      src/script/rooscrpt.cpp
      src/script/scrptcmd.cpp
      src/script/scrptnet.cpp
      src/script/scrptprp.cpp
      src/script/scrptst.cpp
      src/sim/simloop.c
      src/script/sndscrpt.cpp
      src/script/twqscrpt.cpp
      src/ui/bugterm.c
      src/ui/buttpanl.cpp
      src/ui/cmdterm.c
      src/ui/joypoll.cpp
      src/killme.cpp
      src/ui/movie.cpp
      src/motion/mputil.cpp
      src/ui/panlmode.cpp
      src/ui/panltool.cpp
      src/ui/uianim.cpp
      src/ui/uiapp.cpp
      src/ui/uigame.cpp
      src/ui/uiloop.cpp
      src/version.c
      src/ui/winui.c
      src/framewrk/backup.cpp
      src/framewrk/basemode.c
      src/framewrk/buftagf.cpp
      src/framewrk/cdplayer.cpp
      src/framewrk/cfgtool.c
      src/framewrk/command.c
      src/framewrk/ctag.cpp
      src/framewrk/ctagset.cpp
      src/framewrk/dappsink.cpp
      src/framewrk/dbfile.cpp
      src/framewrk/diskfree.c
      src/framewrk/dispatch.cpp
      src/framewrk/dxload.cpp
      src/framewrk/dxwrndlg.cpp
      src/framewrk/fcytagdb.cpp
      src/framewrk/filetool.c
      src/framewrk/filevar.cpp
      src/framewrk/fltang.cpp
      src/framewrk/gameapp.c
      src/framewrk/gameinfo.c
      src/framewrk/gamemode.c
      src/framewrk/gamescrn.cpp
      src/framewrk/gamestr.cpp
      src/framewrk/gametool.cpp
      src/framewrk/gen_bind.cpp
      src/render/gfxprop.cpp
      src/goof.c
      src/framewrk/hchkthrd.cpp
      src/framewrk/imgsrc.cpp
      src/framewrk/init.cpp
      src/framewrk/initedit.cpp
      src/framewrk/loopapp.c
      src/render/medmotn.c
      src/render/meshray.c
      src/framewrk/lresdisk.c
      src/framewrk/main.cpp
      src/script/cdscript.cpp
      src/framewrk/namemap.cpp
      src/framewrk/netman.cpp
      src/framewrk/netmsg.cpp
      src/framewrk/netprops.cpp
      src/framewrk/netsim.cpp
      src/framewrk/netsynch.cpp
      src/framewrk/netvoice.cpp
      src/framewrk/pickgame.cpp
      src/framewrk/r3app.c
      src/framewrk/rand.cpp
      src/framewrk/2dapp.c
      src/framewrk/report.cpp
      src/framewrk/reprthlp.cpp
      src/framewrk/reputil.cpp
      src/framewrk/resapp.c
      src/framewrk/resdata.c
      src/framewrk/resloop.c
      src/framewrk/subscrip.cpp
      src/framewrk/tagdbdat.cpp
      src/framewrk/tagdbin.cpp
      src/framewrk/tagdbkey.cpp
      src/framewrk/tagdbkst.cpp
      src/framewrk/tagdbout.cpp
      src/framewrk/tagpatch.cpp
      src/framewrk/tagremap.cpp
      src/dark/bowscrpt.cpp
      src/dark/ccrappa.cpp
      src/dark/ccrbug.cpp
      src/dark/ccrcray.cpp
      src/dark/ccrhuman.cpp
      src/dark/crappa.c
      src/dark/crbowarm.c
      src/dark/crbugbst.c
      src/dark/crburrik.c
      src/dark/crconsta.c
      src/dark/crcraymn.c
      src/dark/crcutty.c
      src/dark/crfrog.c
      src/dark/crhumand.c
      src/dark/crplyarm.c
      src/dark/crrobot.cpp
      src/dark/crrope.c
      src/dark/crsmrbt.cpp
      src/dark/crspider.c
      src/dark/crspidsm.c
      src/dark/crspirob.c
      src/dark/crsweel.c
      src/dark/crzombie.c
      src/dark/daicam.cpp
      src/darktabl.c
      src/dark/daielem.cpp
      src/dark/daifrog.cpp
      src/dark/darkai.cpp
      src/dark/drk_bind.cpp
      src/dark/drkamap.cpp
      src/dark/drkbook.cpp
      src/dark/drkchkcd.cpp
      src/dark/drkcmbat.cpp
      src/dark/drkcolor.cpp
      src/dark/drkcret.cpp
      src/dark/drkdebrf.cpp
      src/dark/drkfixpr.cpp
      src/dark/drkgame.cpp
      src/dark/drkgoalr.cpp
      src/dark/drkinit.c
      src/dark/drkinv.c
      src/dark/drkinvpr.cpp
      src/dark/drkinvsc.cpp
      src/dark/drkinvui.cpp
      src/dark/drkldout.cpp
      src/dark/drklinks.cpp
      src/dark/drkloop.c
      src/dark/drkloot.cpp
      src/dark/drkmenu.cpp
      src/dark/drkmislp.cpp
      src/dark/drkmiss.cpp
      src/dark/drkmsg.cpp
      src/dark/drknetap.cpp
      src/dark/drkoptmn.cpp
      src/dark/drkplinv.cpp
      src/dark/drkpwscr.cpp
      src/dark/drkpwups.cpp
      src/dark/drkreact.cpp
      src/dark/drkrend.c
      src/dark/drkreprt.cpp
      src/dark/drkrloop.c
      src/dark/drkrutil.cpp
      src/dark/drksave.cpp
      src/dark/drksavui.cpp
      src/dark/drksloop.cpp
      src/dark/drkscrpt.cpp
      src/dark/drkscrs.cpp
      src/dark/drksound.cpp
      src/dark/drkstats.cpp
      src/dark/drkvo.cpp
      src/dark/drkuiscr.cpp
      src/dark/drkwbdy.cpp
      src/dark/drkwbow.cpp
      src/dark/drkwbow_.c
      src/dark/drkwswd.cpp
      src/dark/drkwswd_.c
      src/dark/eidosafp.c
      src/dark/lootprop.cpp
      src/dark/meshprop.cpp
      src/dark/metagame.cpp
      src/dark/picklock.cpp
      src/dark/pickscrs.cpp
      src/dark/plyscrpt.cpp
      src/dark/statbar.cpp
      src/dark/vismeter.cpp
      src/dark/weapscpt.cpp
      src/dark/drkbreth.cpp
      src/dark/drkdiff.cpp
      src/dark/drkpanl.cpp
      src/dark/drkuires.cpp
      src/dark/filepanl.cpp
      src/dark/gadslide.cpp
      src/csg/bspdata.c
      src/csg/csg.c
      src/csg/csgalloc.c
      src/csg/csgbbox.c
      src/csg/csgbuild.c
      src/csg/csgcheck.c
      src/csg/csgclip.c
      src/csg/csgcube.c
      src/csg/csgemit.c
      src/csg/csgfind.c
      src/csg/csgmedia.c
      src/csg/csgmerge.c
      src/csg/csgutil.c
      src/csg/hep.c
      src/editor/alignpnp.c
      src/editor/arcmd.cpp
      src/editor/areapnp.c
      src/editor/autovbr.cpp
      src/editor/bldstair.c
      src/editor/brinfo.c
      src/editor/brlist.c
      src/editor/brloop.c
      src/editor/brobjpro.cpp
      src/editor/brprop.cpp
      src/editor/brquery.c
      src/editor/brrend.c
      src/editor/brundo.c
      src/editor/brushgfh.c
      src/editor/cmdbutts.c
      src/editor/convpred.cpp
      src/editor/cvtbrush.c
      src/editor/ged_rmap.c
      src/editor/cyclegad.c
      src/editor/doorblok.cpp
      src/editor/dragbox.c
      src/editor/editapp.c
      src/editor/editbr.c
      src/editor/editgeom.c
      src/editor/editmode.c
      src/editor/editobj.c
      src/editor/editsave.c
      src/editor/edmedmo.c
      src/editor/famedit.cpp
      src/editor/filemenu.cpp
      src/editor/flowpnp.cpp
      src/editor/fvaredit.cpp
      src/editor/ged_csg.cpp
      src/editor/ged_line.c
      src/editor/ged_rend.c
      src/editor/ged_room.cpp
      src/editor/ged_undo.c
      src/editor/gedit.c
      src/editor/gedmedmo.c
      src/editor/gfhfrobs.c
      src/editor/gridpnp.c
      src/editor/gridsnap.c
      src/editor/hilight.c
      src/editor/lgslider.c
      src/editor/lightpnp.c
      src/editor/linkdraw.cpp
      src/editor/lnktrait.cpp
      src/script/miscrprt.cpp
      src/editor/modalui.c
      src/editor/medflbox.c
      src/editor/mediface.c
      src/editor/medmenu.cpp
      src/editor/medplay.cpp
      src/win32/mschlex.c
      src/motion/mschyacc.c
      src/editor/motedit.cpp
      src/editor/namenum.c
      src/editor/objed.cpp
      src/editor/objedit.cpp
      src/editor/phproped.cpp
      src/editor/objhistp.cpp
      src/editor/objpnp.c
      src/editor/otreeed.cpp
      src/editor/pnpslide.cpp
      src/editor/pnptools.c
      src/editor/preload.c
      src/editor/primal.c
      src/editor/primshap.c
      src/editor/proped.cpp
      src/editor/proptest.cpp
      src/editor/psdialog.cpp
      src/editor/quesedit.cpp
      src/editor/roompnp.c
      src/editor/roomtowr.cpp
      src/editor/saveload.c
      src/editor/scrnedit.cpp
      src/editor/scrpted.cpp
      src/editor/simpwrap.cpp
      src/win32/songlex.c
      src/editor/isdesced.cpp
      src/editor/stupidui.c
      src/editor/swappnp.c
      src/editor/swapregs.c
      src/editor/swaptool.c
      src/editor/txtpnp.c
      src/editor/strprped.cpp
      src/editor/txtrpal.cpp
      src/editor/uiedit.c
      src/editor/undoredo.c
      src/editor/userpnp.c
      src/editor/vbr_math.c
      src/editor/vbrush.c
      src/editor/viewmgr.c
      src/editor/vmwincfg.c
      src/editor/vslider.c
      src/editor/vumanui.c
      src/editor/menus.cpp
      src/editor/prompts.c
      src/editor/sdesparm.cpp
      src/editor/status.c
      src/actreact/arloop.cpp
      src/actreact/contag8r.cpp
      src/sound/esndreac.cpp
      src/actreact/dumbprox.cpp
      src/actreact/mediag8r.cpp
      src/actreact/periog8r.cpp
      src/actreact/pg8rnull.cpp
      src/actreact/propag8n.cpp
      src/actreact/propag8r.cpp
      src/actreact/radiag8r.cpp
      src/actreact/reaction.cpp
      src/actreact/ssrclife.cpp
      src/actreact/stimbase.cpp
      src/actreact/stimguid.c
      src/actreact/stimsens.cpp
      src/actreact/stimsrc.cpp
      src/actreact/stimul8r.cpp
      src/actreact/stimuli.cpp
      src/ai/ai.cpp
      src/ai/aiactfol.cpp
      src/ai/aiactfrb.cpp
      src/ai/aiactinv.cpp
      src/object/objquery.cpp
      src/ai/aiactjr.cpp
      src/ai/aiactjs.cpp
      src/ai/aiactjsc.cpp
      src/ai/aiactlch.cpp
      src/ai/aiactloc.cpp
      src/ai/aiactmot.cpp
      src/ai/aiactmov.cpp
      src/ai/aiactmrk.cpp
      src/ai/aiactnet.cpp
      src/ai/aiactori.cpp
      src/ai/aiactprx.cpp
      src/ai/aiactps.cpp
      src/ai/aiactrng.cpp
      src/ai/aiactseq.cpp
      src/ai/aiactsnd.cpp
      src/ai/aiactwnd.cpp
      src/ai/aiactwt.cpp
      src/ai/aialtrsp.cpp
      src/ai/aiavoid.cpp
      src/ai/aiaware.cpp
      src/ai/aibasabl.cpp
      src/ai/aibasact.cpp
      src/ai/aibasatm.cpp
      src/ai/aibasbhv.cpp
      src/ai/aibasctm.cpp
      src/ai/aibassns.cpp
      src/ai/aibdyrsp.cpp
      src/ai/aicamera.cpp
      src/ai/aicbhtoh.cpp
      src/ai/aicbrbkp.cpp
      src/ai/aicbrclo.cpp
      src/ai/aicbrfle.cpp
      src/ai/aicbridl.cpp
      src/ai/aicbrloc.cpp
      src/ai/aicbrmod.cpp
      src/ai/aicbrmpr.cpp
      src/ai/aicbrnew.cpp
      src/ai/aicbrngd.cpp
      src/ai/aicbrsht.cpp
      src/ai/aicbrstl.cpp
      src/ai/aicbrstr.cpp
      src/ai/aicbrwnd.cpp
      src/ai/aicdummy.cpp
      src/ai/aicbtrsp.cpp
      src/ai/aicmds.cpp
      src/ai/aicnvabl.cpp
      src/ai/aicnvman.cpp
      src/ai/aicnvrse.cpp
      src/ai/aicombat.cpp
      src/ai/aicpinit.cpp
      src/ai/aicretsz.cpp
      src/ai/aidbdraw.cpp
      src/ai/aidbgcmp.cpp
      src/ai/aideath.cpp
      src/ai/aidebug.cpp
      src/ai/aidefend.cpp
      src/ai/aidetect.cpp
      src/ai/aidev.cpp
      src/ai/aidoormr.cpp
      src/ai/aiflee.cpp
      src/ai/aifollow.cpp
      src/ai/aifreeze.cpp
      src/ai/aitest.cpp
      src/ai/aigoal.cpp
      src/ai/aiground.cpp
      src/ai/aigunact.cpp
      src/ai/aiidle.cpp
      src/ai/aiinform.cpp
      src/ai/aiinvest.cpp
      src/ai/ailisten.cpp
      src/ai/aiman.cpp
      src/ai/aimove.cpp
      src/ai/aimovmot.cpp
      src/ai/aimovsug.cpp
      src/ai/aimplyr.cpp
      src/ai/aimttest.cpp
      src/ai/aie3bhv.cpp
      src/ai/aimultcb.cpp
      src/ai/aisubcb.cpp
      src/ai/aincbdmg.cpp
      src/ai/ainet.cpp
      src/ai/ainetman.cpp
      src/ai/ainoncbt.cpp
      src/ai/aipath.cpp
      src/ai/aipathdb.cpp
      src/ai/aipatrol.cpp
      src/ai/aiphycst.cpp
      src/ai/aiprabil.cpp
      src/ai/aiprattr.cpp
      src/ai/aiprops.cpp
      src/ai/aiprconv.cpp
      src/ai/aiprcore.cpp
      src/ai/aiprdev.cpp
      src/ai/aiproxy.cpp
      src/ai/aiprrngd.cpp
      src/ai/aiprutil.cpp
      src/ai/aipthcst.cpp
      src/ai/aipthdbg.cpp
      src/ai/aipthedg.cpp
      src/ai/aipthfnd.cpp
      src/ai/aipthloc.cpp
      src/ai/aipthobb.cpp
      src/ai/aipthzon.cpp
      src/sim/rpathfnd.cpp
      src/ai/aiqdeath.cpp
      src/ai/aireport.cpp
      src/ai/aiscrabl.cpp
      src/ai/aiscrpt.cpp
      src/ai/aisgtcst.cpp
      src/ai/aisnsdev.cpp
      src/ai/aisound.cpp
      src/ai/aisubabl.cpp
      src/ai/aistdmrg.cpp
      src/ai/aistun.cpp
      src/ai/aisuslnk.cpp
      src/ai/aisusrsp.cpp
      src/ai/aitagtyp.cpp
      src/ai/aiteams.cpp
      src/ai/aithreat.cpp
      src/ai/aitrig.cpp
      src/ai/aiturbs.cpp
      src/ai/aiturret.cpp
      src/ai/aiutils.cpp
      src/ai/aiwatch.cpp
      src/ai/aiwr.cpp
      src/portal/bspsphr.c
      src/portal/merge.c
      src/portal/portal.c
      src/portal/portbg.c
      src/portal/portclip.c
      src/portal/portdraw.c
      src/portal/porthw.c
      src/portal/portlit.c
      src/portal/portpick.c
      src/portal/portsky.c
      src/portal/portsurf.c
      src/portal/portwatr.c
      src/portal/pt_mem.c
      src/portal/pt_clut.c
      src/portal/ptdrawhw.c
      src/portal/surfaces.c
      src/portal/ptsurf.asm
      src/portal/ptsurf16.asm
      src/portal/wrcast.c
      src/portal/wrdbg.c
      src/portal/animlit.c
      src/portal/wrfunc.c
      src/object/bintrait.cpp
      src/object/dataops.cpp
      src/object/donorq.cpp
      src/object/edittool.cpp
      src/object/link.cpp
      src/object/linkarry.cpp
      src/object/linkboth.cpp
      src/object/linkdata.cpp
      src/object/linkguid.c
      src/object/linkdb.cpp
      src/object/linkman.cpp
      src/object/linkprop.cpp
      src/object/linkset.cpp
      src/object/linkstor.cpp
      src/object/lnkbyobj.cpp
      src/object/lnkquery.cpp
      src/object/nameprop.cpp
      src/object/objlpars.cpp
      src/object/objproxy.cpp
      src/object/objremap.cpp
      src/object/objsys.cpp
      src/object/osetlnkq.cpp
      src/object/prcniter.cpp
      src/object/propdb.cpp
      src/object/property.cpp
      src/object/propface.cpp
      src/object/propguid.c
      src/object/proplink.cpp
      src/object/linkqdb.cpp
      src/object/propman.cpp
      src/object/propmix.cpp
      src/object/propnull.cpp
      src/object/propobsv.cpp
      src/object/propsave.cpp
      src/object/propstor.cpp
      src/object/querynul.cpp
      src/object/relation.cpp
      src/object/relinver.cpp
      src/object/relnull.cpp
      src/object/strprop.cpp
      src/object/traisrch.cpp
      src/object/trait.cpp
      src/object/traitlnk.cpp
      src/object/traitman.cpp
      src/object/traitnul.cpp
      src/object/traitprp.cpp
      src/object/traitq.cpp
      src/object/traitiid.c
      src/object/traitqdb.cpp
      src/object/trcache.cpp
      src/object/trcachui.cpp
      src/physics/collide.cpp
      src/physics/collprop.cpp
      src/physics/gunproj.cpp
      src/physics/gunvhot.cpp
      src/physics/objmedia.cpp
      src/physics/phclimb.cpp
      src/physics/phclsn.cpp
      src/physics/phcontct.cpp
      src/physics/phcore.cpp
      src/ai/aibasmrg.cpp
      src/physics/phctrl.cpp
      src/physics/phdyn.cpp
      src/physics/phflist.cpp
      src/physics/phlisten.cpp
      src/physics/phmedtr.cpp
      src/physics/phmoapi.cpp
      src/physics/phmod.cpp
      src/physics/phmodbsp.cpp
      src/physics/phmodobb.cpp
      src/physics/phmods.cpp
      src/physics/phmodsph.cpp
      src/physics/phmodutl.cpp
      src/physics/phmotion.cpp
      src/physics/phmodata.c
      src/physics/phmsg.cpp
      src/physics/phmterr.cpp
      src/physics/phmtprop.cpp
      src/physics/phmtscpt.cpp
      src/physics/phnet.cpp
      src/physics/phoprop.cpp
      src/physics/phppscpt.cpp
      src/physics/phprop.cpp
      src/physics/phref.cpp
      src/physics/phreport.cpp
      src/physics/phscrpt.cpp
      src/physics/phsubmod.cpp
      src/physics/phterr.cpp
      src/physics/phtxtprp.cpp
      src/physics/phutils.cpp
      src/physics/physapi.cpp
      src/physics/physcast.cpp
      src/physics/physloop.cpp
      src/physics/phystest.cpp
      src/physics/spherchk.cpp
      src/motion/cerebllm.cpp
      src/sim/avatar.cpp
      src/framewrk/ctagnet.cpp
      src/sim/doorphys.cpp
      src/sim/doorprop.cpp
      src/sim/doorrep.cpp
      src/sim/eosapp.c
      src/sim/flowarch.cpp
      src/sim/ghost.cpp
      src/sim/ghosthst.cpp
      src/sim/ghostlst.cpp
      src/sim/ghostlup.cpp
      src/sim/ghostmsg.cpp
      src/sim/ghostphy.cpp
      src/sim/ghostrcv.cpp
      src/sim/ghostshk.cpp
      src/sim/ghostsnd.cpp
      src/sim/ghostwep.cpp
      src/sim/headmove.c
      src/sim/objloop.cpp
      src/sim/objnet.cpp
      src/sim/objpos.cpp
      src/sim/objtool.cpp
      src/sim/playtest.cpp
      src/sim/plyablty.cpp
      src/sim/plycbllm.cpp
      src/sim/plyrhdlr.cpp
      src/sim/plyrloop.cpp
      src/sim/plyrmode.cpp
      src/sim/plyrmov.cpp
      src/sim/plyrvmot.cpp
      src/sim/prjctile.cpp
      src/sim/room.cpp
      src/sim/roomcore.cpp
      src/sim/roomeax.cpp
      src/sim/roomloop.cpp
      src/sim/roompa.cpp
      src/sim/roomprop.cpp
      src/sim/rooms.cpp
      src/sim/roomutil.cpp
      src/sim/rpaths.cpp
      src/editor/linkedit.cpp
      src/sim/rportal.cpp
      src/sim/simman.cpp
      src/sim/simstate.c
      src/sim/simtime.cpp
      src/sim/wrbsp.cpp
      src/sim/wrcell.c
      src/sim/wrloop.cpp
      src/motion/biploop.c
      src/motion/crattach.cpp
      src/motion/creature.cpp
      src/motion/cretprop.cpp
      src/motion/crexp.cpp
      src/motion/crjoint.cpp
      src/motion/crwpnapi.cpp
      src/motion/crwpnlup.cpp
      src/motion/crwpnlst.cpp
      src/motion/mcoord.cpp
      src/motion/motdb.cpp
      src/motion/motdesc.cpp
      src/motion/motdmnge.c
      src/motion/motmngr.cpp
      src/motion/motprop.cpp
      src/motion/motschem.cpp
      src/motion/motset.cpp
      src/motion/mschutil.cpp
      src/motion/mskilset.cpp
      src/motion/mvrcmbat.cpp
      src/motion/mvrflags.cpp
      src/motion/mvrflbow.cpp
      src/motion/mvrgact.cpp
      src/motion/mvrgloco.cpp
      src/motion/mvrplyr.cpp
      src/motion/mvrsngle.cpp
      src/motion/mvrutils.cpp
      src/motion/puppet.cpp
      src/motion/pupprop.cpp
      src/sound/ambient.c
      src/sound/ambprop.cpp
      src/framewrk/appname.c
      src/sound/appsfx.cpp
      src/sound/auxsnd.cpp
      src/sound/esnd.cpp
      src/sound/esndprop.cpp
      src/sound/metasnd.cpp
      src/sound/psnd.cpp
      src/sound/psndapi.cpp
      src/sound/psndinst.cpp
      src/sound/schdb.cpp
      src/sound/schema.cpp
      src/sound/schloop.c
      src/sound/schmsg.cpp
      src/sound/schfile.cpp
      src/win32/schlex.c
      src/sound/schprop.cpp
      src/sound/schsamps.cpp
      src/sound/schyacc.c
      src/sound/sndframe.cpp
      src/sound/sndgen.cpp
      src/sound/sndloop.cpp
      src/sound/sndnet.cpp
      src/sound/sndprop.cpp
      src/sound/songedtr.cpp
      src/sound/songfile.cpp
      src/sound/songmiss.cpp
      src/sound/songi.cpp
      src/sound/songid.cpp
      src/sound/songplri.cpp
      src/sound/songutil.cpp
      src/sound/spchdom.cpp
      src/sound/spchprop.cpp
      src/sound/spchvoc.cpp
      src/sound/speech.cpp
      src/sound/vocore.cpp
      src/engfeat/campaign.cpp
      src/framewrk/chainevt.cpp
      src/engfeat/contain.cpp
      src/engfeat/dmgmodel.cpp
      src/engfeat/dmgprop.cpp
      src/engfeat/dmgreact.cpp
      src/engfeat/engfeat.cpp
      src/engfeat/flinder.cpp
      src/engfeat/frobctrl.cpp
      src/engfeat/invlimb.cpp
      src/engfeat/frobprop.cpp
      src/engfeat/gunflash.cpp
      src/engfeat/gunprop.cpp
      src/engfeat/hpprop.cpp
      src/engfeat/trapprop.cpp
      src/render/hud.c
      src/engfeat/inv_rend.c
      src/engfeat/invrndpr.cpp
      src/engfeat/keysys.cpp
      src/engfeat/lockprop.cpp
      src/engfeat/pick.cpp
      src/engfeat/projlink.cpp
      src/engfeat/projprop.cpp
      src/engfeat/propreac.cpp
      src/engfeat/quesloop.cpp
      src/engfeat/quesreac.cpp
      src/engfeat/quest.cpp
      src/engfeat/preldprp.cpp
      src/engfeat/questprp.cpp
      src/engfeat/simpdmg.cpp
      src/engfeat/susprop.cpp
      src/editor/rendtest.c
      src/engfeat/tweqctrl.cpp
      src/engfeat/tweqprop.cpp
      src/engfeat/tweqrep.cpp
      src/engfeat/weapcb.cpp
      src/engfeat/weaphalo.cpp
      src/engfeat/weaphit.cpp
      src/engfeat/weapon.cpp
      src/engfeat/weapprop.cpp
      src/engfeat/weapreac.cpp
      src/engfeat/contprop.cpp
      src/render/anim_txt.c
      src/render/animlgt.c
      src/render/antxloop.cpp
      src/render/antxscpt.cpp
      src/render/camera.c
      src/camguids.c
      src/render/camovl.cpp
      src/render/celstobj.cpp
      src/render/cloudobj.cpp
      src/render/corona.cpp
      src/render/ctimer.c
      src/render/ctrast.c
      src/render/distobj.cpp
      src/render/dyntex.cpp
      src/render/elitprop.cpp
      src/render/face.cpp
      src/render/family.c
      src/render/fas.c
      src/render/fasprop.cpp
      src/render/fxdisk.c
      src/render/fxspan.c
      src/render/h2ocolor.cpp
      src/render/hdskprop.cpp
      src/render/litnet.cpp
      src/render/litprop.cpp
      src/render/meshatt.cpp
      src/render/meshtex.cpp
      src/render/mipmap.c
      src/render/missrend.cpp
      src/render/mlight.c
      src/render/mnamprop.cpp
      src/render/mnumprop.cpp
      src/render/objcast.cpp
      src/render/objlight.c
      src/render/objmodel.c
      src/render/objshape.cpp
      src/render/objtxtrp.cpp
      src/engfeat/ownprop.cpp
      src/render/palmgr.c
      src/render/particle.cpp
      src/render/partprop.cpp
      src/render/pgroup.c
      src/render/pgrpprop.cpp
      src/render/portapp.c
      src/render/porthlpr.c
      src/render/render.cpp
      src/render/rendloop.c
      src/render/rendobj.cpp
      src/render/rendphys.cpp
      src/render/rendprop.cpp
      src/render/rendptst.c
      src/render/rndflash.cpp
      src/render/scalprop.cpp
      src/render/scrnloop.c
      src/framewrk/scrndump.cpp
      src/render/scrnman.c
      src/render/scrnmode.cpp
      src/render/scrnovls.cpp
      src/render/skyobj.cpp
      src/render/skyrend.cpp
      src/render/slitprop.cpp
      src/render/sphrcst.cpp
      src/render/sprkprop.cpp
      src/render/starhack.c
      src/render/starobj.cpp
      src/render/tanimprp.cpp
      src/testloop.c
      src/testmode.c
      src/render/texmem.c
      src/render/texprop.c
      src/render/texsave.cpp
      src/render/textarch.cpp
      src/ui/uiamov.cpp
      src/render/weather.cpp
      src/win32/inclex.c
      src/frezreac.cpp
      src/inc.c
      src/inctab.cpp
      src/songyacc.c
      src/render/mtlparse.cpp
      src/physics/phprops.cpp
      src/render/palette.c
    )
  endif()
  # dromed.exe carries embedded CodeView (Misc, FPO and NB11 debug directory entries in .rdata):
  # /debug with /pdb:none. /debug turns /opt:ref and /opt:icf off, so they are given explicitly.
  if(TOOLCHAIN STREQUAL "vc6")
    set_target_properties(dromed PROPERTIES LINK_FLAGS "/debug /debugtype:cv /pdb:none /opt:ref /opt:icf")
  endif()
  target_link_libraries(dromed
    dromed_mprintf
    dromed_lg
    dromed_lgalloc
    dromed_matrix
    dromed_fix
    dromed_dstruct
    dromed_cpptools
    dromed_comtools
    dromed_compapis
    dromed_appcore
    dromed_dev2d
    dromed_g2
    dromed_font
    dromed_dispdev
    dromed_config
    dromed_res
    dromed_input
    dromed_gameshel
    dromed_timer
    dromed_rnd
    dromed_recorder
    dromed_ui
    dromed_gadget
    dromed_sound
    dromed_actmovie
    dromed_quartz
    dromed_r3d
    dromed_star
    dromed_mp
    dromed_md
    dromed_dump
    dromed_vec2
    dromed_prof
    dromed_darkloop
    dromed_lgd3d
    dromed_mm
    dromed_script
    dromed_sndsrc
    dromed_ref
    dromed_sdesc
    dromed_arq
    dromed_inputbnd
    dromed_namedres
    dromed_net
    dromed_random
    dromed_tagfile
    dromed_entrywin
    # Import descriptors follow library order: the makefile's compiler libs
    # (comdlg32 comctl32) came first, then kernel32 user32 gdi32 winmm ole32 advapi32.
    # The CRT is not named: libcpmt, LIBCMT and OLDNAMES come from the objects'
    # default-library directives, in that order, after every named library
    # (operator delete is libcpmt's delop.obj).
    comdlg32.lib comctl32.lib uuid.lib
    kernel32.lib user32.lib gdi32.lib winmm.lib ole32.lib advapi32.lib
    winspool.lib shell32.lib oleaut32.lib odbc32.lib odbccp32.lib
    ${CMAKE_BINARY_DIR}/wail32.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/a3d/ia3d.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/miles/mss32.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dx6sdk/lib/ddraw.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dx6sdk/lib/dxguid.lib
    ${CMAKE_BINARY_DIR}/qmixer.lib
  )
  add_dependencies(dromed import_libs)
  if(TOOLCHAIN STREQUAL "vs2008")
    # The NewDark texture palette reads thumbnails back through the D3D9 HW2D renderer.
    set_property(SOURCE src/editor/txtrpal.cpp APPEND PROPERTY
      INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dx9sdk/include
    )
    set_property(SOURCE src/editor/txtrpal.cpp APPEND PROPERTY
      COMPILE_DEFINITIONS WINVER=0x0500
    )
  endif()
else()
  # Tech libraries, one per makefile library and in its link order. The linker
  # pulls members on demand, so their order in the image follows the references.
  add_library(thief2_mprintf STATIC
    libsrc/mprintf/mprintf.c
  )
  add_library(thief2_lg STATIC
    libsrc/lg/hashfns.cpp
    libsrc/lg/appagg.cpp
    libsrc/lg/lgsprntf.c
    libsrc/lg/datapath.cpp
    libsrc/lg/dbg.c
    libsrc/lg/lgassert.cpp
    libsrc/lg/exit.c
    libsrc/lg/breakkey.cpp
    libsrc/lg/dpathdir.c
    libsrc/lg/tmpalloc.c
    libsrc/lg/coremutx.cpp
    libsrc/lg/stack.c
    libsrc/lg/codewrit.cpp
    libsrc/lg/thrdtool.cpp
    libsrc/lg/lglog.cpp
  )
  if(TOOLCHAIN STREQUAL "vs2008")
    target_sources(thief2_lg PRIVATE libsrc/lg/lg_file.c)
  endif()
  add_library(thief2_lgalloc STATIC
    libsrc/lgalloc/memall.c
    libsrc/lgalloc/memcore.cpp
    libsrc/lgalloc/multpool.cpp
    libsrc/lgalloc/stdalloc.cpp
    libsrc/lgalloc/primallc.cpp
    libsrc/lgalloc/heap.cpp
    libsrc/lgalloc/poolimp.cpp
    libsrc/lgalloc/membase.cpp
    libsrc/lgalloc/pool.cpp
    libsrc/lgalloc/nullallc.cpp
  )
  # THIEF.EXE imports malloc/free/new/delete from MSVCR90: no allocator override.
  if(TOOLCHAIN STREQUAL "vs2008")
    target_sources(thief2_lgalloc PRIVATE libsrc/lgalloc/nonover.cpp)
  endif()
  add_library(thief2_matrix STATIC
    libsrc/matrix/vecvec.c
    libsrc/matrix/matang.c
    libsrc/matrix/matvec.c
    libsrc/matrix/matmat.c
    libsrc/matrix/vecang.c
    libsrc/matrix/vecvecd.c
    libsrc/matrix/vec2.c
    libsrc/matrix/trans.c
    libsrc/matrix/transasm.asm
  )
  add_library(thief2_fix STATIC
    libsrc/fix/fixdiv.asm
    libsrc/fix/fix.c
    libsrc/fix/fix_asm.asm
    libsrc/fix/trigtab.c
    libsrc/fix/fix_pow.c
  )
  add_library(thief2_dstruct STATIC
    libsrc/dstruct/hash.c
    libsrc/dstruct/rect.c
    libsrc/dstruct/array.c
  )
  add_library(thief2_cpptools STATIC
    libsrc/cpptools/dynarray.cpp
    libsrc/cpptools/hashset.cpp
    libsrc/cpptools/str.cpp
    libsrc/cpptools/packflag.cpp
    libsrc/cpptools/filespec.cpp
    libsrc/cpptools/hashpp.cpp
    libsrc/cpptools/dynfunc.cpp
    libsrc/cpptools/delmlist.cpp
    libsrc/cpptools/filepath.cpp
    libsrc/cpptools/fnamutil.cpp
    libsrc/cpptools/splitpat.cpp
    libsrc/cpptools/pdynarr.cpp
    libsrc/cpptools/relocptr.cpp
  )
  add_library(thief2_comtools STATIC
    libsrc/comtools/genagg.cpp
    libsrc/comtools/interset.cpp
    libsrc/comtools/pguidset.cpp
    libsrc/comtools/geniset.cpp
    libsrc/comtools/comconn.cpp
  )
  add_library(thief2_compapis STATIC
    libsrc/compapis/genguid1.c
  )
  add_library(thief2_appcore STATIC
    libsrc/appcore/wappcore.cpp
    libsrc/appcore/appcore.cpp
    libsrc/appcore/appflags.cpp
  )
  add_library(thief2_dev2d STATIC
    libsrc/dev2d/grd.c
    libsrc/dev2d/bitmap.c
    libsrc/dev2d/canvas.c
    libsrc/dev2d/mode.c
    libsrc/dev2d/ipal.c
    libsrc/dev2d/pal.c
    libsrc/dev2d/pal16.c
    libsrc/dev2d/ltab.c
    libsrc/dev2d/gdbm.c
    libsrc/dev2d/gdrect.c
    libsrc/dev2d/clut16.c
    libsrc/dev2d/tluctab.c
    libsrc/dev2d/smode.c
    libsrc/dev2d/screen.c
    libsrc/dev2d/cpal.c
    libsrc/dev2d/gdpix.c
    libsrc/dev2d/gpal.c
    libsrc/dev2d/gdgbm.c
    libsrc/dev2d/fcolor.c
    libsrc/dev2d/valloc.c
    libsrc/dev2d/gdlin.c
    libsrc/dev2d/grmalloc.c
    libsrc/dev2d/genbmf.c
    libsrc/dev2d/comdfcn.cpp
    libsrc/dev2d/genrsd8.c
    libsrc/dev2d/genf16.c
    libsrc/dev2d/genfl8.c
    libsrc/dev2d/genmono.c
    libsrc/dev2d/rsdcvt.c
    libsrc/dev2d/rsdunpck.asm
    libsrc/dev2d/comdtab.c
    libsrc/dev2d/ddevcnv.c
    libsrc/dev2d/f16cnv.c
    libsrc/dev2d/fl8cnv.c
    libsrc/dev2d/f16hlinf.c
    libsrc/dev2d/f16vlinf.c
    libsrc/dev2d/f16ubmf.c
    libsrc/dev2d/f16p8f.c
    libsrc/dev2d/f16p16f.c
    libsrc/dev2d/fl8hlinf.c
    libsrc/dev2d/fl8vlinf.c
    libsrc/dev2d/fl8ubmf.c
    libsrc/dev2d/fl8p16f.c
    libsrc/dev2d/fl8p8f.c
    libsrc/dev2d/bank.c
    libsrc/dev2d/grnull.c
    libsrc/dev2d/mxlatch.c
    libsrc/dev2d/init.c
    libsrc/dev2d/hflip.c
    libsrc/dev2d/astring.c
    libsrc/dev2d/stastk.c
    libsrc/dev2d/cominit.c
    libsrc/dev2d/ddevblt.c
    libsrc/dev2d/f16lin.c
    libsrc/dev2d/f16bm.c
    libsrc/dev2d/f16pix.c
    libsrc/dev2d/lpix.c
    libsrc/dev2d/fl8lin.c
    libsrc/dev2d/fl8bm.c
    libsrc/dev2d/fl8pix.c
    libsrc/dev2d/f16hlin.c
    libsrc/dev2d/f16vlin.c
    libsrc/dev2d/f16rsd8.c
    libsrc/dev2d/genf16u.c
    libsrc/dev2d/f16f16.c
    libsrc/dev2d/f16fl8.c
    libsrc/dev2d/genfl8u.c
    libsrc/dev2d/f16mono.c
    libsrc/dev2d/genbm.c
    libsrc/dev2d/genmonou.c
    libsrc/dev2d/f16pix8.c
    libsrc/dev2d/f16pix16.c
    libsrc/dev2d/fl8hlin.c
    libsrc/dev2d/fl8vlin.c
    libsrc/dev2d/fl8rsd8.c
    libsrc/dev2d/fl8fl8.c
    libsrc/dev2d/fl8mono.c
    libsrc/dev2d/fl8pix8.c
    libsrc/dev2d/f16set.asm
    libsrc/dev2d/f16f16c.asm
    libsrc/dev2d/rsdblt16.asm
    libsrc/dev2d/f16fl8il.asm
    libsrc/dev2d/cstring.asm
    libsrc/dev2d/f16il.asm
    libsrc/dev2d/rsdblt8.asm
  )
  add_library(thief2_g2 STATIC
    libsrc/g2/g2circ.c
    libsrc/g2/init.c
    libsrc/g2/g2ov.c
    libsrc/g2/g2d.c
    libsrc/g2/pt_main.c
    libsrc/g2/pt_map.c
    libsrc/g2/clplin.c
    libsrc/g2/pt_duv.c
    libsrc/g2/tmsetup.c
    libsrc/g2/setdrv.c
    libsrc/g2/g2makewr.c
    libsrc/g2/g2el.c
    libsrc/g2/rastel.c
    libsrc/g2/ptfloat.asm
    libsrc/g2/ptmap_8.asm
    libsrc/g2/ptpersp.asm
    libsrc/g2/ptmap_n.asm
    libsrc/g2/tmapd.c
    libsrc/g2/ptmapper.asm
    libsrc/g2/uvedge.asm
    libsrc/g2/genrast.c
    libsrc/g2/pt_grad.c
    libsrc/g2/ptlinear.asm
    libsrc/g2/permap.c
    libsrc/g2/tf.c
    libsrc/g2/f16cnv.c
    libsrc/g2/fl8cnv.c
    libsrc/g2/gencnv.c
    libsrc/g2/n88il.asm
    libsrc/g2/lo816il.asm
    libsrc/g2/lo88il.asm
    libsrc/g2/co88il.asm
    libsrc/g2/o88il.asm
    libsrc/g2/o88bil.asm
    libsrc/g2/ptmap.asm
    libsrc/g2/uvgrad.asm
    libsrc/g2/trigrad.c
    libsrc/g2/clip.c
    libsrc/g2/f16pm.c
    libsrc/g2/f16lm.c
    libsrc/g2/gensply.c
    libsrc/g2/genply.c
    libsrc/g2/gens.c
    libsrc/g2/genline.c
    libsrc/g2/fl8pm.c
    libsrc/g2/fl8lm.c
    libsrc/g2/fl8sply.c
    libsrc/g2/fl8s.c
    libsrc/g2/fl8l.c
    libsrc/g2/genlm.c
    libsrc/g2/genrsdtm.c
    libsrc/g2/genf16l.c
    libsrc/g2/genfl8l.c
    libsrc/g2/o816.c
    libsrc/g2/lo816.c
    libsrc/g2/genrsds.c
    libsrc/g2/genf16s.c
    libsrc/g2/genfl8s.c
    libsrc/g2/scshell.c
    libsrc/g2/r88.c
    libsrc/g2/co88.c
    libsrc/g2/o88.c
    libsrc/g2/lo88.c
    libsrc/g2/fl8fl8s.c
    libsrc/g2/fl8lin.c
    libsrc/g2/gen16il.c
    libsrc/g2/gen8il.c
    libsrc/g2/f16lmf.c
    libsrc/g2/f16llmf.c
    libsrc/g2/genusf.c
    libsrc/g2/gensf.c
    libsrc/g2/fl8lmf.c
    libsrc/g2/fl8llmf.c
    libsrc/g2/fl8usf.c
    libsrc/g2/fl8sf.c
    libsrc/g2/fl8ulf.c
    libsrc/g2/genllmf.c
    libsrc/g2/genlmf.c
    libsrc/g2/o816il.asm
    libsrc/g2/clo816il.asm
    libsrc/g2/point.c
  )
  add_library(thief2_font STATIC
    libsrc/font/str.c
    libsrc/font/strsiz.c
    libsrc/font/chrsiz.c
    libsrc/font/strwrap.c
    libsrc/font/setup.c
    libsrc/font/f16fl8t.c
  )
  add_library(thief2_dispdev STATIC
    libsrc/dispdev/dddynf.cpp
    libsrc/dispdev/wdcreate.cpp
    libsrc/dispdev/wdisp.cpp
    libsrc/dispdev/wdispgdi.cpp
    libsrc/dispdev/wdispdd.cpp
    libsrc/dispdev/dispbase.cpp
    libsrc/dispdev/wdspprov.cpp
    libsrc/dispdev/wdspmode.cpp
    libsrc/dispdev/wdspgshl.cpp
    libsrc/dispdev/wdisptls.cpp
    libsrc/dispdev/wddmode.cpp
    libsrc/dispdev/cbchain.cpp
  )
  if(TOOLCHAIN STREQUAL "vs2008")
    target_sources(thief2_dispdev PRIVATE
      libsrc/dispdev/wdispd3d.cpp
      libsrc/dispdev/wd3dmode.cpp
      libsrc/dispdev/wd3dhw2d.cpp
    )
    target_include_directories(thief2_dispdev PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dx9sdk/include)
    # Same PSDK WINVER as the DromEd dispdev block.
    set_source_files_properties(
      libsrc/dispdev/dddynf.cpp
      libsrc/dispdev/wdcreate.cpp
      libsrc/dispdev/wdispd3d.cpp
      libsrc/dispdev/wd3dmode.cpp
      libsrc/dispdev/wd3dhw2d.cpp
      PROPERTIES COMPILE_DEFINITIONS "__MSVC;WINVER=0x0500"
    )
  endif()
  add_library(thief2_config STATIC
    libsrc/config/config.c
    libsrc/config/cfg.c
  )
  add_library(thief2_res STATIC
    libsrc/res/resguid.c
    libsrc/res/rescache.cpp
    libsrc/res/resmem.cpp
    libsrc/res/resagg.c
    libsrc/res/refacc.cpp
    libsrc/res/resacc.cpp
    libsrc/res/resfile.cpp
    libsrc/res/resthred.cpp
    libsrc/res/res.cpp
    libsrc/res/resload.cpp
    libsrc/res/pkzip.cpp
    libsrc/res/lzw.cpp
    libsrc/res/resbuild.cpp
    libsrc/res/rescback.cpp
  )
  add_library(thief2_input STATIC
    winsrc/input/kbcntry.c
    winsrc/input/mousesys.cpp
    winsrc/input/inpinit.c
    winsrc/input/inpman.cpp
    winsrc/input/kbasctab.cpp
    winsrc/input/inpguid.c
    winsrc/input/kbcook.cpp
    winsrc/input/inpcompo.cpp
    winsrc/input/defehand.cpp
    winsrc/input/kbstart.cpp
    winsrc/input/kbshut.cpp
    winsrc/input/inpdynf.cpp
    winsrc/input/joystick.cpp
    winsrc/input/kbsys.cpp
    winsrc/input/inperr.cpp
    winsrc/input/joywin.cpp
    winsrc/input/recinput.cpp
    winsrc/input/joyold.cpp
  )
  add_library(thief2_gameshel STATIC
    libsrc/gameshel/wgscreat.cpp
    libsrc/gameshel/dxguid.c
    libsrc/gameshel/wgshell.cpp
    libsrc/gameshel/wgshelmt.cpp
    libsrc/gameshel/winput.cpp
  )
  add_library(thief2_timer STATIC
    libsrc/timer/tmwin32.c
    libsrc/timer/tmrt.c
    libsrc/timer/tmftab.c
  )
  add_library(thief2_rnd STATIC
    libsrc/rnd/rnd.c
  )
  add_library(thief2_recorder STATIC
    libsrc/recorder/recorder.cpp
    libsrc/recorder/recprmpt.cpp
    libsrc/recorder/rectempl.cpp
  )
  add_library(thief2_ui STATIC
    libsrc/ui/cursors.c
    libsrc/ui/region.c
    libsrc/ui/hotkey.c
    libsrc/ui/event.c
    libsrc/ui/slab.c
    libsrc/ui/curdrw.c
    libsrc/ui/vmouse.c
  )
  add_library(thief2_gadget STATIC
    libsrc/gadget/gadget.c
    libsrc/gadget/gadblist.c
    libsrc/gadget/uitexted.c
    libsrc/gadget/gadtext.c
    libsrc/gadget/gcompose.c
    libsrc/gadget/guistyle.c
    libsrc/gadget/drawelem.c
    libsrc/gadget/uibutton.c
    libsrc/gadget/util2d.c
  )
  add_library(thief2_sound STATIC
    libsrc/sound/dmixer.cpp
    libsrc/sound/sndfmt.cpp
    libsrc/sound/dsnddynf.cpp
    libsrc/sound/volconv.cpp
    libsrc/sound/lgsndid.cpp
    libsrc/sound/mixer.cpp
    libsrc/sound/sample.cpp
    libsrc/sound/dsample.cpp
    libsrc/sound/fade.cpp
  )
  if(TOOLCHAIN STREQUAL "vs2008")
    target_sources(thief2_sound PRIVATE
      libsrc/sound/almixer.cpp
    )
    target_include_directories(thief2_sound PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/openal/include)
  endif()
  add_library(thief2_actmovie STATIC
    libsrc/actmovie/actmovie.cpp
    libsrc/actmovie/getlgvid.cpp
    libsrc/actmovie/amguids.cpp
    libsrc/actmovie/imutil.cpp
    libsrc/actmovie/amalloc.cpp
  )
  add_library(thief2_quartz STATIC
    3rdparty/dxmedia/quartz.c
  )
  add_library(thief2_r3d STATIC
    libsrc/r3d/block.c
    libsrc/r3d/prim.c
    libsrc/r3d/xfrmglob.c
    libsrc/r3d/clip.c
    libsrc/r3d/init.c
    libsrc/r3d/frame.c
    libsrc/r3d/object.c
    libsrc/r3d/ctxtmang.c
    libsrc/r3d/space.c
    libsrc/r3d/view.c
    libsrc/r3d/primmisc.c
    libsrc/r3d/clipglob.c
    libsrc/r3d/primg2.c
    libsrc/r3d/primd3d.c
    libsrc/r3d/clipplan.c
    libsrc/r3d/primfunc.c
    libsrc/r3d/xfrmlin.c
    libsrc/r3d/xfrmunsc.c
    libsrc/r3d/xfrmproj.c
    libsrc/r3d/xfrmclip.c
    libsrc/r3d/ixmgr.c
    libsrc/r3d/ctxtguts.c
    libsrc/r3d/primconv.c
    libsrc/r3d/primix.c
    libsrc/r3d/primpoly.c
    libsrc/r3d/clipunsc.c
    libsrc/r3d/clipproj.c
    libsrc/r3d/projfast.asm
    libsrc/r3d/clipclip.c
    libsrc/r3d/primface.c
    libsrc/r3d/cliptest.c
  )
  add_library(thief2_star STATIC
    libsrc/star/star.c
  )
  add_library(thief2_mp STATIC
    libsrc/mp/mplist.c
    libsrc/mp/mpinit.c
    libsrc/mp/qt.c
    libsrc/mp/mpmot.c
    libsrc/mp/mpupdate.c
    libsrc/mp/mpstack.c
    libsrc/mp/mpvm.c
    libsrc/mp/mploco.c
  )
  add_library(thief2_md STATIC
    libsrc/md/md_lgd3d.cpp
    libsrc/md/render.c
    libsrc/md/light.c
    libsrc/md/hitdet.c
    libsrc/md/scale.c
    libsrc/md/fancy.c
    libsrc/md/utils.c
    libsrc/md/cbacks.c
    libsrc/md/mipmap.c
    libsrc/md/mxstack.c
  )
  add_library(thief2_dump STATIC
    libsrc/dump/filefind.c
    libsrc/dump/pcxdump.c
  )
  add_library(thief2_vec2 STATIC
    libsrc/vec2/vec2.c
  )
  add_library(thief2_darkloop STATIC
    libsrc/darkloop/gloopcli.cpp
    libsrc/darkloop/loopfact.cpp
    libsrc/darkloop/loopman.cpp
    libsrc/darkloop/loopmode.cpp
    libsrc/darkloop/loopdisp.cpp
    libsrc/darkloop/loopque.cpp
  )
  add_library(thief2_lgd3d STATIC
    libsrc/lgd3d/d6intfc.cpp
    libsrc/lgd3d/d6enum.cpp
    libsrc/lgd3d/texmgr.cpp
    libsrc/lgd3d/d6sts_s.cpp
    libsrc/lgd3d/lgd3d_g.cpp
    libsrc/lgd3d/d6frame.cpp
    libsrc/lgd3d/d6render.cpp
    libsrc/lgd3d/d6prim_s.cpp
    libsrc/lgd3d/d6base.cpp
    libsrc/lgd3d/lgsurf.cpp
    libsrc/lgd3d/d6over.cpp
  )
  # NewDark adds the Direct3D 9 renderer beside the d6 one (docs/notes/project_newdark_d8_renderer.md).
  if(TOOLCHAIN STREQUAL "vs2008")
    target_sources(thief2_lgd3d PRIVATE
      libsrc/lgd3d/d8Frame.cpp
      libsrc/lgd3d/d8Prim_s.cpp
      libsrc/lgd3d/d8Render.cpp
      libsrc/lgd3d/d8States.cpp
    )
    target_include_directories(thief2_lgd3d PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dx9sdk/include)
  endif()
  add_library(thief2_mm STATIC
    libsrc/mm/render.c
    libsrc/mm/sortpoly.c
    libsrc/mm/xformseg.c
    libsrc/mm/debug.c
  )
  add_library(thief2_script STATIC
    libsrc/script/scrptman.cpp
    libsrc/script/scrptmsg.cpp
    libsrc/script/persist.cpp
    libsrc/script/multparm.cpp
    libsrc/script/scrobj.cpp
    libsrc/script/smodinfo.cpp
    libsrc/script/scrptmod.cpp
    libsrc/script/nullsrv.cpp
  )
  add_library(thief2_sndsrc STATIC
    libsrc/sndsrc/sndsrc.cpp
    libsrc/sndsrc/sndsrcid.cpp
    libsrc/sndsrc/playlist.cpp
    libsrc/sndsrc/refill.cpp
    libsrc/sndsrc/nrezseg.cpp
    libsrc/sndsrc/memseg.cpp
    libsrc/sndsrc/fileseg.cpp
    libsrc/sndsrc/rezseg.cpp
    libsrc/sndsrc/sndseg.cpp
    libsrc/sndsrc/imaadpcm.cpp
  )
  add_library(thief2_ref STATIC
    libsrc/ref/refsys.c
  )
  add_library(thief2_sdesc STATIC
    libsrc/sdesc/sdestool.cpp
    libsrc/sdesc/sdesc.cpp
  )
  add_library(thief2_arq STATIC
    libsrc/arq/arq.cpp
    libsrc/arq/arqthred.cpp
    libsrc/arq/arqqueue.cpp
    libsrc/arq/arqreq.cpp
    libsrc/arq/arqstrrd.cpp
  )
  add_library(thief2_inputbnd STATIC
    libsrc/inputbnd/ibvarman.cpp
    libsrc/inputbnd/inputbnd.cpp
    libsrc/inputbnd/inpguid.cpp
    libsrc/inputbnd/ibmapper.cpp
  )
  add_library(thief2_namedres STATIC
    libsrc/namedres/resids.c
    libsrc/namedres/resman.cpp
    libsrc/namedres/mdlrstyp.cpp
    libsrc/namedres/fonrstyp.cpp
    libsrc/namedres/sndrstyp.cpp
    libsrc/namedres/strrstyp.cpp
    libsrc/namedres/imgrstyp.cpp
    libsrc/namedres/palrstyp.cpp
    libsrc/namedres/imghack.cpp
    libsrc/namedres/defresm.cpp
    libsrc/namedres/resmanhs.cpp
    libsrc/namedres/resmanx.cpp
    libsrc/namedres/resarq.cpp
    libsrc/namedres/zipstfct.cpp
    libsrc/namedres/storeman.cpp
    libsrc/namedres/binrstyp.cpp
    libsrc/namedres/pathutil.cpp
    libsrc/namedres/search.cpp
    libsrc/namedres/stdres.cpp
    libsrc/namedres/resstat.cpp
    libsrc/namedres/resstr.cpp
    libsrc/namedres/restmpl.cpp
    libsrc/namedres/resimage.cpp
    libsrc/namedres/respall.cpp
    libsrc/namedres/storezip.cpp
    libsrc/namedres/defstfct.cpp
    libsrc/namedres/storedir.cpp
    libsrc/namedres/storbase.cpp
    libsrc/namedres/resutil.cpp
    libsrc/namedres/tga.cpp
    libsrc/namedres/pcx.cpp
    libsrc/namedres/gif.cpp
    libsrc/namedres/cel.cpp
    libsrc/namedres/bmp.cpp
    libsrc/namedres/zipstrm.cpp
    libsrc/namedres/filestrm.cpp
  )
  if(NOT TOOLCHAIN STREQUAL "vs2008")
    target_sources(thief2_namedres PRIVATE
      libsrc/namedres/inflate.c
      libsrc/namedres/infblock.c
      libsrc/namedres/adler32.c
      libsrc/namedres/zutil.c
      libsrc/namedres/infcodes.c
      libsrc/namedres/inftrees.c
      libsrc/namedres/infutil.c
      libsrc/namedres/inffast.c
    )
  endif()
  add_library(thief2_net STATIC
    libsrc/net/net.cpp
  )
  add_library(thief2_random STATIC
    libsrc/random/xrandom.cpp
    libsrc/random/rng.cpp
    libsrc/random/rngcong.cpp
    libsrc/random/rngfib.cpp
  )
  add_library(thief2_tagfile STATIC
    libsrc/tagfile/tagfile.cpp
    libsrc/tagfile/memtfile.cpp
    libsrc/tagfile/vernum.cpp
  )
  add_library(thief2_entrywin STATIC
    winsrc/entrywin/entrywin.cpp
  )

  if(TOOLCHAIN STREQUAL "vc6")
    add_executable(thief2 WIN32
      src/dark.rc
      src/framewrk/rand.cpp
      src/framewrk/2dapp.c
      src/render/animlgt.c
      src/render/anim_txt.c
      src/sound/ambient.c
      src/sound/ambprop.cpp
      src/framewrk/appname.c
      src/sound/appsfx.cpp
      src/sound/auxsnd.cpp
      src/framewrk/backup.cpp
      src/framewrk/basemode.c
      src/motion/biploop.c
      src/framewrk/buftagf.cpp
      src/ui/bugterm.c
      src/ui/buttpanl.cpp
      src/render/camera.c
      src/camguids.c
      src/engfeat/campaign.cpp
      src/framewrk/chainevt.cpp
      src/ui/cmdterm.c
      src/framewrk/cfgtool.c
      src/framewrk/command.c
      src/render/corona.cpp
      src/framewrk/ctag.cpp
      src/framewrk/ctagset.cpp
      src/render/ctimer.c
      src/framewrk/dappsink.cpp
      src/framewrk/dbfile.cpp
      src/framewrk/diskfree.c
      src/framewrk/dispatch.cpp
      src/engfeat/dmgmodel.cpp
      src/engfeat/dmgprop.cpp
      src/engfeat/dmgreact.cpp
      src/framewrk/dxload.cpp
      src/framewrk/dxwrndlg.cpp
      src/sim/eosapp.c
      src/object/edittool.cpp
      src/render/elitprop.cpp
      src/sound/esnd.cpp
      src/sound/esndprop.cpp
      src/render/face.cpp
      src/render/family.c
      src/render/fas.c
      src/render/fasprop.cpp
      src/framewrk/fcytagdb.cpp
      src/framewrk/filetool.c
      src/editor/filemenu.cpp
      src/framewrk/filevar.cpp
      src/engfeat/flinder.cpp
      src/sim/flowarch.cpp
      src/framewrk/fltang.cpp
      src/render/fxdisk.c
      src/render/fxspan.c
      src/framewrk/gameapp.c
      src/framewrk/gameinfo.c
      src/framewrk/gamemode.c
      src/framewrk/gamescrn.cpp
      src/framewrk/gamestr.cpp
      src/framewrk/gametool.cpp
      src/framewrk/gen_bind.cpp
      src/render/gfxprop.cpp
      src/goof.c
      src/render/h2ocolor.cpp
      src/framewrk/hchkthrd.cpp
      src/render/hdskprop.cpp
      src/sim/headmove.c
      src/engfeat/hpprop.cpp
      src/engfeat/trapprop.cpp
      src/render/hud.c
      src/inc.c
      src/win32/inclex.c
      src/inctab.cpp
      src/framewrk/init.cpp
      src/framewrk/imgsrc.cpp
      src/editor/isdesced.cpp
      src/ui/joypoll.cpp
      src/killme.cpp
      src/render/litprop.cpp
      src/framewrk/lresdisk.c
      src/framewrk/loopapp.c
      src/render/medmotn.c
      src/render/meshray.c
      src/render/meshtex.cpp
      src/sound/metasnd.cpp
      src/render/mipmap.c
      src/render/missrend.cpp
      src/render/mlight.c
      src/render/mnamprop.cpp
      src/render/mnumprop.cpp
      src/editor/menus.cpp
      src/render/meshatt.cpp
      src/ui/movie.cpp
      src/motion/mputil.cpp
      src/framewrk/namemap.cpp
      src/render/objcast.cpp
      src/render/objlight.c
      src/sim/objloop.cpp
      src/render/objmodel.c
      src/sim/objpos.cpp
      src/object/objremap.cpp
      src/object/objlpars.cpp
      src/object/objproxy.cpp
      src/render/objshape.cpp
      src/object/objsys.cpp
      src/sim/objtool.cpp
      src/render/objtxtrp.cpp
      src/engfeat/ownprop.cpp
      src/render/palette.c
      src/render/palmgr.c
      src/ui/panlmode.cpp
      src/render/particle.cpp
      src/render/partprop.cpp
      src/render/pgroup.c
      src/render/pgrpprop.cpp
      src/script/pgrpscpt.cpp
      src/framewrk/pickgame.cpp
      src/sim/playtest.cpp
      src/sim/plyablty.cpp
      src/sim/plycbllm.cpp
      src/sim/plyrhdlr.cpp
      src/sim/plyrloop.cpp
      src/sim/plyrmode.cpp
      src/sim/plyrmov.cpp
      src/sim/plyrvmot.cpp
      src/render/portapp.c
      src/render/porthlpr.c
      src/editor/prompts.c
      src/motion/puppet.cpp
      src/motion/pupprop.cpp
      src/engfeat/quest.cpp
      src/engfeat/quesloop.cpp
      src/engfeat/questprp.cpp
      src/framewrk/r3app.c
      src/render/render.cpp
      src/render/rendloop.c
      src/render/skyrend.cpp
      src/render/skyobj.cpp
      src/render/starobj.cpp
      src/render/celstobj.cpp
      src/render/distobj.cpp
      src/render/cloudobj.cpp
      src/render/rendobj.cpp
      src/render/rendphys.cpp
      src/render/rendprop.cpp
      src/render/rendptst.c
      src/framewrk/reputil.cpp
      src/framewrk/resapp.c
      src/framewrk/resdata.c
      src/framewrk/resloop.c
      src/render/rndflash.cpp
      src/render/scalprop.cpp
      src/sound/schdb.cpp
      src/sound/schema.cpp
      src/sound/schloop.c
      src/sound/schmsg.cpp
      src/sound/schprop.cpp
      src/sound/schsamps.cpp
      src/render/scrnloop.c
      src/framewrk/scrndump.cpp
      src/render/scrnman.c
      src/render/scrnmode.cpp
      src/render/scrnovls.cpp
      src/script/scrptcmd.cpp
      src/script/scrptprp.cpp
      src/script/scrptst.cpp
      src/sim/simloop.c
      src/sim/simman.cpp
      src/sim/simtime.cpp
      src/engfeat/simpdmg.cpp
      src/sim/simstate.c
      src/render/slitprop.cpp
      src/sound/sndgen.cpp
      src/sound/sndloop.cpp
      src/sound/sndprop.cpp
      src/sound/songi.cpp
      src/sound/songid.cpp
      src/sound/songplri.cpp
      src/sound/songutil.cpp
      src/sound/songfile.cpp
      src/sound/songmiss.cpp
      src/sound/spchdom.cpp
      src/sound/spchprop.cpp
      src/sound/spchvoc.cpp
      src/sound/speech.cpp
      src/render/sphrcst.cpp
      src/render/sprkprop.cpp
      src/render/starhack.c
      src/editor/status.c
      src/framewrk/subscrip.cpp
      src/engfeat/susprop.cpp
      src/framewrk/tagdbdat.cpp
      src/framewrk/tagdbin.cpp
      src/framewrk/tagdbkey.cpp
      src/framewrk/tagdbkst.cpp
      src/framewrk/tagdbout.cpp
      src/framewrk/tagpatch.cpp
      src/framewrk/tagremap.cpp
      src/render/tanimprp.cpp
      src/testloop.c
      src/testmode.c
      src/render/texmem.c
      src/render/texprop.c
      src/render/texsave.cpp
      src/render/textarch.cpp
      src/ui/uiamov.cpp
      src/ui/uianim.cpp
      src/ui/uiapp.cpp
      src/ui/uigame.cpp
      src/ui/uiloop.cpp
      src/version.c
      src/render/weather.cpp
      src/ui/winui.c
      src/sim/wrbsp.cpp
      src/sim/wrcell.c
      src/sim/wrloop.cpp
      src/ai/aimplyr.cpp
      src/ai/aimttest.cpp
      src/ai/aitest.cpp
      src/ai/aie3bhv.cpp
      src/ai/ai.cpp
      src/ai/aiavoid.cpp
      src/ai/aibasabl.cpp
      src/ai/aibasact.cpp
      src/ai/aibasatm.cpp
      src/ai/aibasbhv.cpp
      src/ai/aibasctm.cpp
      src/ai/aibasmrg.cpp
      src/ai/aicmds.cpp
      src/ai/aicnvman.cpp
      src/ai/aicnvrse.cpp
      src/ai/aicpinit.cpp
      src/ai/aicretsz.cpp
      src/ai/aidetect.cpp
      src/ai/aidoormr.cpp
      src/ai/aigoal.cpp
      src/ai/aiground.cpp
      src/ai/aiinform.cpp
      src/ai/aiman.cpp
      src/ai/aimove.cpp
      src/ai/aimovmot.cpp
      src/ai/aimovsug.cpp
      src/ai/ainet.cpp
      src/ai/ainetman.cpp
      src/ai/aiphycst.cpp
      src/ai/aiproxy.cpp
      src/ai/aisound.cpp
      src/ai/aisubabl.cpp
      src/ai/aisuslnk.cpp
      src/ai/aistdmrg.cpp
      src/ai/aistun.cpp
      src/ai/aitagtyp.cpp
      src/ai/aiteams.cpp
      src/ai/aitrig.cpp
      src/ai/aiutils.cpp
      src/ai/aiwr.cpp
      src/ai/aicbhtoh.cpp
      src/ai/aicbrbkp.cpp
      src/ai/aicbrclo.cpp
      src/ai/aicbrfle.cpp
      src/ai/aicbridl.cpp
      src/ai/aicbrloc.cpp
      src/ai/aicbrmod.cpp
      src/ai/aicbrmpr.cpp
      src/ai/aicbrnew.cpp
      src/ai/aicbrngd.cpp
      src/ai/aicbrsht.cpp
      src/ai/aicbrstl.cpp
      src/ai/aicbrstr.cpp
      src/ai/aicbrwnd.cpp
      src/ai/aicdummy.cpp
      src/ai/aicombat.cpp
      src/ai/aidev.cpp
      src/ai/aimultcb.cpp
      src/ai/aiturret.cpp
      src/ai/aicamera.cpp
      src/ai/aiturbs.cpp
      src/ai/aiqdeath.cpp
      src/ai/aiactfol.cpp
      src/ai/aiactfrb.cpp
      src/ai/aiactinv.cpp
      src/ai/aiactjr.cpp
      src/ai/aiactjs.cpp
      src/ai/aiactjsc.cpp
      src/ai/aiactlch.cpp
      src/ai/aiactloc.cpp
      src/ai/aiactmot.cpp
      src/ai/aiactmov.cpp
      src/ai/aiactmrk.cpp
      src/ai/aiactprx.cpp
      src/ai/aiactps.cpp
      src/ai/aiactori.cpp
      src/ai/aiactrng.cpp
      src/ai/aiactseq.cpp
      src/ai/aiactsnd.cpp
      src/ai/aiactwnd.cpp
      src/ai/aiactwt.cpp
      src/ai/aiactnet.cpp
      src/ai/aigunact.cpp
      src/ai/aialtrsp.cpp
      src/ai/aibdyrsp.cpp
      src/ai/aicnvabl.cpp
      src/ai/aicbtrsp.cpp
      src/ai/aideath.cpp
      src/ai/aidefend.cpp
      src/ai/aiflee.cpp
      src/ai/aifollow.cpp
      src/ai/aifreeze.cpp
      src/ai/aiidle.cpp
      src/ai/aiinvest.cpp
      src/ai/ailisten.cpp
      src/ai/aincbdmg.cpp
      src/ai/ainoncbt.cpp
      src/ai/aipatrol.cpp
      src/ai/aisusrsp.cpp
      src/ai/aithreat.cpp
      src/ai/aiwatch.cpp
      src/ai/aipath.cpp
      src/ai/aipathdb.cpp
      src/ai/aipthcst.cpp
      src/ai/aipthdbg.cpp
      src/ai/aipthedg.cpp
      src/ai/aipthfnd.cpp
      src/ai/aipthloc.cpp
      src/ai/aipthobb.cpp
      src/ai/aipthzon.cpp
      src/sim/rpathfnd.cpp
      src/sim/rpaths.cpp
      src/ai/aiprabil.cpp
      src/ai/aiprconv.cpp
      src/ai/aiprcore.cpp
      src/ai/aiprdev.cpp
      src/ai/aiprattr.cpp
      src/ai/aiprops.cpp
      src/ai/aiprrngd.cpp
      src/ai/aiprutil.cpp
      src/ai/aiaware.cpp
      src/ai/aibassns.cpp
      src/ai/aisgtcst.cpp
      src/ai/aisnsdev.cpp
      src/ai/aidbgcmp.cpp
      src/ai/aidebug.cpp
      src/ai/aiscrabl.cpp
      src/ai/aiscrpt.cpp
      src/render/antxloop.cpp
      src/script/atkscrpt.cpp
      src/framewrk/cdplayer.cpp
      src/engfeat/contain.cpp
      src/engfeat/contprop.cpp
      src/script/contscrs.cpp
      src/render/ctrast.c
      src/sim/doorphys.cpp
      src/sim/doorprop.cpp
      src/actreact/dumbprox.cpp
      src/render/dyntex.cpp
      src/engfeat/engfeat.cpp
      src/engfeat/frobprop.cpp
      src/engfeat/frobctrl.cpp
      src/engfeat/invlimb.cpp
      src/engfeat/inv_rend.c
      src/engfeat/invrndpr.cpp
      src/script/keyscpt.cpp
      src/engfeat/keysys.cpp
      src/render/litnet.cpp
      src/engfeat/lockprop.cpp
      src/script/lockscpt.cpp
      src/ui/panltool.cpp
      src/engfeat/pick.cpp
      src/engfeat/preldprp.cpp
      src/sim/prjctile.cpp
      src/engfeat/quesreac.cpp
      src/script/scrptnet.cpp
      src/sound/sndnet.cpp
      src/engfeat/tweqctrl.cpp
      src/engfeat/tweqprop.cpp
      src/script/twqscrpt.cpp
      src/sound/vocore.cpp
      src/engfeat/weapcb.cpp
      src/engfeat/weaphalo.cpp
      src/engfeat/weaphit.cpp
      src/engfeat/weapon.cpp
      src/engfeat/weapprop.cpp
      src/engfeat/weapreac.cpp
      src/engfeat/gunflash.cpp
      src/engfeat/gunprop.cpp
      src/engfeat/projprop.cpp
      src/engfeat/projlink.cpp
      src/render/antxscpt.cpp
      src/script/arscript.cpp
      src/script/bodscrpt.cpp
      src/script/camscrpt.cpp
      src/script/datascrs.cpp
      src/script/dmgscrpt.cpp
      src/script/drscrpt.cpp
      src/script/engscrpt.cpp
      src/script/linkscpt.cpp
      src/script/litscrpt.cpp
      src/script/netscrpt.cpp
      src/script/objscrpt.cpp
      src/script/propscpt.cpp
      src/script/pupscrpt.cpp
      src/script/quesscpt.cpp
      src/editor/sdesparm.cpp
      src/script/sndscrpt.cpp
      src/script/mtscrpt.cpp
      src/sim/room.cpp
      src/sim/roomcore.cpp
      src/sim/roomeax.cpp
      src/sim/roomloop.cpp
      src/sim/roompa.cpp
      src/sim/roomprop.cpp
      src/sim/rooms.cpp
      src/sim/roomutil.cpp
      src/script/rooscrpt.cpp
      src/sim/rportal.cpp
      src/sound/psndapi.cpp
      src/sound/psnd.cpp
      src/sound/psndinst.cpp
      src/sound/sndframe.cpp
      src/actreact/arloop.cpp
      src/actreact/contag8r.cpp
      src/sound/esndreac.cpp
      src/frezreac.cpp
      src/actreact/mediag8r.cpp
      src/actreact/periog8r.cpp
      src/actreact/pg8rnull.cpp
      src/actreact/propag8n.cpp
      src/actreact/propag8r.cpp
      src/engfeat/propreac.cpp
      src/actreact/radiag8r.cpp
      src/actreact/reaction.cpp
      src/actreact/ssrclife.cpp
      src/actreact/stimbase.cpp
      src/actreact/stimguid.c
      src/actreact/stimsens.cpp
      src/actreact/stimsrc.cpp
      src/actreact/stimul8r.cpp
      src/actreact/stimuli.cpp
      src/object/bintrait.cpp
      src/object/dataops.cpp
      src/object/link.cpp
      src/object/linkarry.cpp
      src/object/linkboth.cpp
      src/object/lnkbyobj.cpp
      src/object/linkdb.cpp
      src/object/linkdata.cpp
      src/object/linkguid.c
      src/object/linkman.cpp
      src/object/linkset.cpp
      src/object/linkstor.cpp
      src/object/linkqdb.cpp
      src/object/linkprop.cpp
      src/object/osetlnkq.cpp
      src/object/proplink.cpp
      src/object/lnkquery.cpp
      src/object/querynul.cpp
      src/object/relation.cpp
      src/object/relinver.cpp
      src/object/relnull.cpp
      src/object/prcniter.cpp
      src/object/propdb.cpp
      src/object/property.cpp
      src/object/propface.cpp
      src/object/propguid.c
      src/object/propstor.cpp
      src/object/propman.cpp
      src/object/propmix.cpp
      src/object/propnull.cpp
      src/object/propobsv.cpp
      src/object/propsave.cpp
      src/object/strprop.cpp
      src/object/donorq.cpp
      src/object/nameprop.cpp
      src/object/objquery.cpp
      src/object/traisrch.cpp
      src/object/trait.cpp
      src/object/traitq.cpp
      src/object/traitiid.c
      src/object/traitlnk.cpp
      src/object/traitman.cpp
      src/object/traitnul.cpp
      src/object/traitprp.cpp
      src/object/traitqdb.cpp
      src/object/trcache.cpp
      src/object/trcachui.cpp
      src/physics/collide.cpp
      src/physics/collprop.cpp
      src/physics/objmedia.cpp
      src/physics/phclimb.cpp
      src/physics/phclsn.cpp
      src/physics/phcontct.cpp
      src/physics/phcore.cpp
      src/physics/phctrl.cpp
      src/physics/phdyn.cpp
      src/physics/phflist.cpp
      src/physics/phlisten.cpp
      src/physics/phmedtr.cpp
      src/physics/phmod.cpp
      src/physics/phmodbsp.cpp
      src/physics/phmodobb.cpp
      src/physics/phmods.cpp
      src/physics/phmodsph.cpp
      src/physics/phmodutl.cpp
      src/physics/phmsg.cpp
      src/physics/phnet.cpp
      src/physics/phoprop.cpp
      src/physics/phprop.cpp
      src/physics/phprops.cpp
      src/physics/phmtprop.cpp
      src/physics/phmterr.cpp
      src/physics/phmtscpt.cpp
      src/physics/phppscpt.cpp
      src/physics/phref.cpp
      src/physics/phsubmod.cpp
      src/physics/phscrpt.cpp
      src/physics/phterr.cpp
      src/physics/phtxtprp.cpp
      src/physics/phutils.cpp
      src/physics/physapi.cpp
      src/physics/physcast.cpp
      src/physics/physloop.cpp
      src/physics/phmotion.cpp
      src/physics/phmodata.c
      src/physics/phmoapi.cpp
      src/physics/gunproj.cpp
      src/physics/gunvhot.cpp
      src/physics/spherchk.cpp
      src/motion/cerebllm.cpp
      src/motion/mcoord.cpp
      src/motion/mvrcmbat.cpp
      src/motion/mvrflags.cpp
      src/motion/mvrflbow.cpp
      src/motion/mvrgact.cpp
      src/motion/mvrgloco.cpp
      src/motion/mvrplyr.cpp
      src/motion/mvrsngle.cpp
      src/motion/mvrutils.cpp
      src/motion/motdb.cpp
      src/motion/motdesc.cpp
      src/motion/motdmnge.c
      src/motion/motmngr.cpp
      src/motion/motschem.cpp
      src/motion/motset.cpp
      src/motion/motprop.cpp
      src/motion/mschutil.cpp
      src/motion/mskilset.cpp
      src/motion/crattach.cpp
      src/motion/crwpnlst.cpp
      src/motion/crwpnapi.cpp
      src/motion/crwpnlup.cpp
      src/motion/creature.cpp
      src/motion/cretprop.cpp
      src/motion/crexp.cpp
      src/dark/crjoint.cpp
      src/dark/ccrappa.cpp
      src/dark/ccrbug.cpp
      src/dark/ccrcray.cpp
      src/dark/ccrhuman.cpp
      src/dark/crappa.c
      src/dark/crbowarm.c
      src/dark/crbugbst.c
      src/dark/crburrik.c
      src/dark/crconsta.c
      src/dark/crcraymn.c
      src/dark/crcutty.c
      src/dark/crfrog.c
      src/dark/crhumand.c
      src/dark/crplyarm.c
      src/dark/crrobot.cpp
      src/dark/crrope.c
      src/dark/crsmrbt.cpp
      src/dark/crspider.c
      src/dark/crspidsm.c
      src/dark/crspirob.c
      src/dark/crsweel.c
      src/dark/crzombie.c
      src/dark/drkdiff.cpp
      src/dark/drkpanl.cpp
      src/dark/drkuires.cpp
      src/dark/filepanl.cpp
      src/dark/gadslide.cpp
      src/framewrk/initgame.cpp
      src/framewrk/gameiid.c
      src/sim/avatar.cpp
      src/framewrk/ctagnet.cpp
      src/sim/ghost.cpp
      src/sim/ghostlst.cpp
      src/sim/ghostlup.cpp
      src/sim/ghostmsg.cpp
      src/sim/ghostphy.cpp
      src/sim/ghostrcv.cpp
      src/sim/ghostsnd.cpp
      src/sim/ghostwep.cpp
      src/framewrk/netman.cpp
      src/framewrk/netmsg.cpp
      src/framewrk/netprops.cpp
      src/framewrk/netsim.cpp
      src/framewrk/netsynch.cpp
      src/framewrk/netvoice.cpp
      src/sim/objnet.cpp
      src/portal/wrfunc.c
      src/portal/wrcast.c
      src/portal/bspsphr.c
      src/portal/animlit.c
      src/portal/portsurf.c
      src/portal/surfaces.c
      src/portal/ptsurf.asm
      src/portal/ptsurf16.asm
      src/portal/pt_mem.c
      src/portal/pt_clut.c
      src/portal/portal.c
      src/portal/portdraw.c
      src/portal/ptdrawhw.c
      src/portal/portlit.c
      src/portal/portclip.c
      src/portal/portbg.c
      src/portal/portpick.c
      src/portal/portwatr.c
      src/portal/porthw.c
      src/portal/portsky.c
      src/dark/bowscrpt.cpp
      src/render/camovl.cpp
      src/dark/drk_bind.cpp
      src/dark/drkamap.cpp
      src/dark/drkbook.cpp
      src/dark/drkbreth.cpp
      src/dark/drkchkcd.cpp
      src/dark/drkcmbat.cpp
      src/dark/drkcolor.cpp
      src/dark/drkcret.cpp
      src/dark/drkdebrf.cpp
      src/dark/drkfixpr.cpp
      src/dark/drkgame.cpp
      src/dark/drkgoalr.cpp
      src/dark/drkldout.cpp
      src/dark/drklinks.cpp
      src/dark/drkloop.c
      src/dark/drkloot.cpp
      src/dark/drkmenu.cpp
      src/dark/drkmislp.cpp
      src/dark/drkmiss.cpp
      src/dark/drkmsg.cpp
      src/dark/drkinit.c
      src/dark/drkinv.c
      src/dark/drkinvsc.cpp
      src/dark/drkinvpr.cpp
      src/dark/drkinvui.cpp
      src/dark/drknetap.cpp
      src/dark/drkoptmn.cpp
      src/dark/drkplinv.cpp
      src/dark/drkpwscr.cpp
      src/dark/drkpwups.cpp
      src/dark/drkreact.cpp
      src/dark/drkrend.c
      src/dark/drkrloop.c
      src/dark/drkrutil.cpp
      src/dark/drksave.cpp
      src/dark/drksavui.cpp
      src/dark/drksloop.cpp
      src/dark/drkscrpt.cpp
      src/dark/drkscrs.cpp
      src/dark/drksound.cpp
      src/dark/drkstats.cpp
      src/dark/drkuiscr.cpp
      src/dark/drkvo.cpp
      src/dark/drkwbdy.cpp
      src/dark/drkwbow.cpp
      src/dark/drkwbow_.c
      src/dark/drkwswd.cpp
      src/dark/drkwswd_.c
      src/dark/eidosafp.c
      src/dark/lootprop.cpp
      src/dark/picklock.cpp
      src/dark/pickscrs.cpp
      src/dark/plyscrpt.cpp
      src/dark/metagame.cpp
      src/dark/statbar.cpp
      src/dark/vismeter.cpp
      src/dark/weapscpt.cpp
      src/dark/meshprop.cpp
      src/dark/darkai.cpp
      src/dark/daielem.cpp
      src/dark/daifrog.cpp
      src/dark/daicam.cpp
      src/darktabl.c
      src/framewrk/main.cpp
    )
  else()
    # NewDark link order, from the target function layout (scripts/tools/nd_link_order.py).
    add_executable(thief2 WIN32
      src/dark.rc
      src/script/arscript.cpp
      src/script/bodscrpt.cpp
      src/script/camscrpt.cpp
      src/script/contscrs.cpp
      src/script/datascrs.cpp
      src/script/dmgscrpt.cpp
      src/script/drscrpt.cpp
      src/script/engscrpt.cpp
      src/script/keyscpt.cpp
      src/script/linkscpt.cpp
      src/script/litscrpt.cpp
      src/script/lockscpt.cpp
      src/script/netscrpt.cpp
      src/script/objscrpt.cpp
      src/script/pgrpscpt.cpp
      src/script/propscpt.cpp
      src/script/pupscrpt.cpp
      src/script/rooscrpt.cpp
      src/script/scrptnet.cpp
      src/script/scrptprp.cpp
      src/script/scrptst.cpp
      src/sim/simloop.c
      src/script/sndscrpt.cpp
      src/script/twqscrpt.cpp
      src/ui/buttpanl.cpp
      src/ui/cmdterm.c
      src/ui/joypoll.cpp
      src/killme.cpp
      src/ui/movie.cpp
      src/motion/mputil.cpp
      src/ui/panlmode.cpp
      src/ui/panltool.cpp
      src/engfeat/pick.cpp
      src/ui/uianim.cpp
      src/ui/uiapp.cpp
      src/ui/uigame.cpp
      src/ui/uiloop.cpp
      src/version.c
      src/framewrk/buftagf.cpp
      src/ui/bugterm.c
      src/framewrk/cdplayer.cpp
      src/framewrk/cfgtool.c
      src/framewrk/command.c
      src/framewrk/ctag.cpp
      src/framewrk/ctagset.cpp
      src/render/ctimer.c
      src/framewrk/dappsink.cpp
      src/framewrk/dbfile.cpp
      src/framewrk/diskfree.c
      src/framewrk/dispatch.cpp
      src/framewrk/dxload.cpp
      src/framewrk/dxwrndlg.cpp
      src/framewrk/fcytagdb.cpp
      src/framewrk/filetool.c
      src/editor/filemenu.cpp
      src/framewrk/filevar.cpp
      src/framewrk/fltang.cpp
      src/framewrk/gameapp.c
      src/framewrk/gameinfo.c
      src/framewrk/gamemode.c
      src/framewrk/gamescrn.cpp
      src/framewrk/gamestr.cpp
      src/framewrk/gametool.cpp
      src/framewrk/gen_bind.cpp
      src/render/gfxprop.cpp
      src/goof.c
      src/framewrk/hchkthrd.cpp
      src/framewrk/imgsrc.cpp
      src/editor/isdesced.cpp
      src/framewrk/init.cpp
      src/framewrk/initgame.cpp
      src/framewrk/gameiid.c
      src/framewrk/loopapp.c
      src/framewrk/lresdisk.c
      src/framewrk/namemap.cpp
      src/framewrk/netman.cpp
      src/object/objremap.cpp
      src/framewrk/netmsg.cpp
      src/framewrk/netprops.cpp
      src/framewrk/netsim.cpp
      src/framewrk/netsynch.cpp
      src/framewrk/netvoice.cpp
      src/framewrk/pickgame.cpp
      src/framewrk/r3app.c
      src/framewrk/rand.cpp
      src/framewrk/2dapp.c
      src/framewrk/resapp.c
      src/framewrk/resdata.c
      src/framewrk/resloop.c
      src/framewrk/scrndump.cpp
      src/framewrk/tagdbdat.cpp
      src/framewrk/tagdbin.cpp
      src/framewrk/tagdbkey.cpp
      src/framewrk/tagdbkst.cpp
      src/framewrk/tagdbout.cpp
      src/framewrk/tagpatch.cpp
      src/framewrk/tagremap.cpp
      src/dark/bowscrpt.cpp
      src/dark/ccrappa.cpp
      src/dark/ccrbug.cpp
      src/dark/ccrcray.cpp
      src/dark/ccrhuman.cpp
      src/dark/crappa.c
      src/dark/crbowarm.c
      src/dark/crbugbst.c
      src/dark/crburrik.c
      src/dark/crconsta.c
      src/dark/crcraymn.c
      src/dark/crcutty.c
      src/dark/crfrog.c
      src/dark/crhumand.c
      src/dark/crplyarm.c
      src/dark/crrobot.cpp
      src/dark/crrope.c
      src/dark/crsmrbt.cpp
      src/dark/crspider.c
      src/dark/crspidsm.c
      src/dark/crspirob.c
      src/dark/crsweel.c
      src/dark/crzombie.c
      src/dark/daielem.cpp
      src/dark/daifrog.cpp
      src/dark/darkai.cpp
      src/dark/drk_bind.cpp
      src/dark/drkamap.cpp
      src/dark/drkbook.cpp
      src/dark/drkchkcd.cpp
      src/dark/drkcmbat.cpp
      src/dark/drkcolor.cpp
      src/dark/drkcret.cpp
      src/dark/drkdebrf.cpp
      src/dark/drkfixpr.cpp
      src/dark/drkgame.cpp
      src/dark/drkgoalr.cpp
      src/dark/drkinv.c
      src/dark/drkinvpr.cpp
      src/dark/drkinvsc.cpp
      src/dark/drkinvui.cpp
      src/dark/drkldout.cpp
      src/dark/drklinks.cpp
      src/dark/drkloop.c
      src/dark/drkloot.cpp
      src/dark/drkmenu.cpp
      src/dark/drkmiss.cpp
      src/dark/drkmsg.cpp
      src/dark/drkinit.c
      src/dark/drknetap.cpp
      src/dark/drkoptmn.cpp
      src/dark/drkplinv.cpp
      src/dark/drkpwscr.cpp
      src/dark/drkpwups.cpp
      src/dark/drkreact.cpp
      src/dark/drkrend.c
      src/dark/drkrloop.c
      src/dark/drkrutil.cpp
      src/dark/drksave.cpp
      src/dark/drksavui.cpp
      src/dark/drksloop.cpp
      src/dark/drkscrpt.cpp
      src/dark/drkscrs.cpp
      src/dark/drksound.cpp
      src/dark/drkstats.cpp
      src/dark/drkvo.cpp
      src/dark/drkwbdy.cpp
      src/dark/drkwbow.cpp
      src/dark/drkwbow_.c
      src/dark/drkwswd.cpp
      src/dark/drkwswd_.c
      src/dark/eidosafp.c
      src/ai/aibasmrg.cpp
      src/ai/aicmds.cpp
      src/dark/meshprop.cpp
      src/dark/metagame.cpp
      src/dark/picklock.cpp
      src/dark/pickscrs.cpp
      src/dark/plyscrpt.cpp
      src/dark/statbar.cpp
      src/dark/vismeter.cpp
      src/dark/weapscpt.cpp
      src/dark/drkbreth.cpp
      src/dark/drkpanl.cpp
      src/dark/filepanl.cpp
      src/dark/gadslide.cpp
      src/editor/prompts.c
      src/editor/sdesparm.cpp
      src/editor/status.c
      src/actreact/contag8r.cpp
      src/actreact/dumbprox.cpp
      src/actreact/mediag8r.cpp
      src/actreact/periog8r.cpp
      src/actreact/pg8rnull.cpp
      src/actreact/propag8n.cpp
      src/actreact/propag8r.cpp
      src/actreact/radiag8r.cpp
      src/actreact/reaction.cpp
      src/actreact/ssrclife.cpp
      src/actreact/stimbase.cpp
      src/actreact/stimguid.c
      src/actreact/stimsens.cpp
      src/actreact/stimsrc.cpp
      src/actreact/stimul8r.cpp
      src/actreact/stimuli.cpp
      src/ai/ai.cpp
      src/ai/aiactfol.cpp
      src/ai/aiactfrb.cpp
      src/ai/aiactinv.cpp
      src/ai/aiactjr.cpp
      src/ai/aiactjs.cpp
      src/ai/aiactjsc.cpp
      src/ai/aiactloc.cpp
      src/ai/aiactmot.cpp
      src/ai/aiactmov.cpp
      src/ai/aiactmrk.cpp
      src/ai/aiactnet.cpp
      src/ai/aiactori.cpp
      src/ai/aiactprx.cpp
      src/ai/aiactrng.cpp
      src/ai/aiactps.cpp
      src/ai/aiactseq.cpp
      src/ai/aiactwnd.cpp
      src/ai/aiactwt.cpp
      src/ai/aialtrsp.cpp
      src/ai/aiavoid.cpp
      src/ai/aiaware.cpp
      src/ai/aibasabl.cpp
      src/ai/aiactsnd.cpp
      src/ai/aibasact.cpp
      src/ai/aibasatm.cpp
      src/ai/aibasbhv.cpp
      src/ai/aibasctm.cpp
      src/ai/aibassns.cpp
      src/ai/aibdyrsp.cpp
      src/ai/aicamera.cpp
      src/ai/aicbhtoh.cpp
      src/ai/aicbrbkp.cpp
      src/ai/aicbrfle.cpp
      src/ai/aicbridl.cpp
      src/ai/aicbrloc.cpp
      src/ai/aicbrmod.cpp
      src/ai/aicbrmpr.cpp
      src/ai/aicbrnew.cpp
      src/ai/aicbrngd.cpp
      src/ai/aicbrsht.cpp
      src/ai/aicbrstl.cpp
      src/ai/aicbrstr.cpp
      src/ai/aicbrwnd.cpp
      src/ai/aicdummy.cpp
      src/ai/aicbtrsp.cpp
      src/ai/aicnvabl.cpp
      src/ai/aicnvman.cpp
      src/ai/aicnvrse.cpp
      src/ai/aicombat.cpp
      src/ai/aicpinit.cpp
      src/ai/aicretsz.cpp
      src/ai/aidbgcmp.cpp
      src/ai/aidebug.cpp
      src/ai/aideath.cpp
      src/ai/aidefend.cpp
      src/ai/aidetect.cpp
      src/ai/aidev.cpp
      src/ai/aidoormr.cpp
      src/ai/aiflee.cpp
      src/ai/aifollow.cpp
      src/ai/aifreeze.cpp
      src/ai/aitest.cpp
      src/ai/aie3bhv.cpp
      src/ai/aigoal.cpp
      src/ai/aiground.cpp
      src/ai/aigunact.cpp
      src/ai/aiidle.cpp
      src/ai/aiinform.cpp
      src/ai/aiinvest.cpp
      src/framewrk/subscrip.cpp
      src/ai/ailisten.cpp
      src/ai/aiman.cpp
      src/ai/aimove.cpp
      src/ai/aimovmot.cpp
      src/ai/aimovsug.cpp
      src/ai/aimultcb.cpp
      src/ai/aincbdmg.cpp
      src/ai/ainet.cpp
      src/ai/ainetman.cpp
      src/ai/ainoncbt.cpp
      src/ai/aipath.cpp
      src/ai/aipatrol.cpp
      src/ai/aiphycst.cpp
      src/ai/aiprabil.cpp
      src/ai/aiprattr.cpp
      src/ai/aiprops.cpp
      src/ai/aiprconv.cpp
      src/ai/aiprcore.cpp
      src/ai/aipathdb.cpp
      src/ai/aiprdev.cpp
      src/ai/aiproxy.cpp
      src/dark/lootprop.cpp
      src/ai/aiprrngd.cpp
      src/ai/aiprutil.cpp
      src/ai/aipthcst.cpp
      src/ai/aipthdbg.cpp
      src/ai/aipthedg.cpp
      src/ai/aipthfnd.cpp
      src/ai/aipthloc.cpp
      src/ai/aipthobb.cpp
      src/ai/aipthzon.cpp
      src/sim/rpathfnd.cpp
      src/ai/aiqdeath.cpp
      src/ai/aiscrabl.cpp
      src/script/atkscrpt.cpp
      src/script/mtscrpt.cpp
      src/ai/aiscrpt.cpp
      src/ai/aisgtcst.cpp
      src/ai/aisnsdev.cpp
      src/ai/aisound.cpp
      src/ai/aisubabl.cpp
      src/ai/aistdmrg.cpp
      src/ai/aistun.cpp
      src/ai/aisuslnk.cpp
      src/ai/aisusrsp.cpp
      src/ai/aitagtyp.cpp
      src/ai/aiteams.cpp
      src/ai/aithreat.cpp
      src/ai/aitrig.cpp
      src/ai/aiturbs.cpp
      src/ai/aiturret.cpp
      src/ai/aiutils.cpp
      src/ai/aiwatch.cpp
      src/ai/aiwr.cpp
      src/portal/bspsphr.c
      src/portal/animlit.c
      src/portal/portal.c
      src/portal/portbg.c
      src/portal/portpick.c
      src/portal/portclip.c
      src/portal/portdraw.c
      src/portal/porthw.c
      src/portal/portlit.c
      src/portal/portsky.c
      src/portal/portsurf.c
      src/portal/surfaces.c
      src/portal/ptsurf.asm
      src/portal/ptsurf16.asm
      src/portal/portwatr.c
      src/portal/pt_mem.c
      src/portal/pt_clut.c
      src/portal/ptdrawhw.c
      src/portal/wrcast.c
      src/portal/wrfunc.c
      src/object/bintrait.cpp
      src/object/dataops.cpp
      src/object/donorq.cpp
      src/object/link.cpp
      src/object/linkarry.cpp
      src/object/linkboth.cpp
      src/object/linkdata.cpp
      src/object/linkguid.c
      src/object/linkdb.cpp
      src/object/linkman.cpp
      src/object/linkprop.cpp
      src/object/linkset.cpp
      src/object/linkstor.cpp
      src/object/lnkbyobj.cpp
      src/object/lnkquery.cpp
      src/object/nameprop.cpp
      src/object/objlpars.cpp
      src/object/objproxy.cpp
      src/object/objquery.cpp
      src/object/objsys.cpp
      src/object/osetlnkq.cpp
      src/object/prcniter.cpp
      src/object/propdb.cpp
      src/object/property.cpp
      src/object/propface.cpp
      src/object/propguid.c
      src/script/quesscpt.cpp
      src/object/proplink.cpp
      src/object/propman.cpp
      src/object/propmix.cpp
      src/object/propnull.cpp
      src/object/propobsv.cpp
      src/object/propsave.cpp
      src/object/propstor.cpp
      src/editor/menus.cpp
      src/object/relation.cpp
      src/object/querynul.cpp
      src/object/relinver.cpp
      src/object/relnull.cpp
      src/object/strprop.cpp
      src/object/traisrch.cpp
      src/object/trait.cpp
      src/object/traitman.cpp
      src/object/traitnul.cpp
      src/object/traitprp.cpp
      src/object/traitq.cpp
      src/object/traitiid.c
      src/object/traitqdb.cpp
      src/object/linkqdb.cpp
      src/object/trcache.cpp
      src/object/trcachui.cpp
      src/physics/collide.cpp
      src/physics/collprop.cpp
      src/physics/gunproj.cpp
      src/physics/gunvhot.cpp
      src/physics/objmedia.cpp
      src/physics/phclimb.cpp
      src/physics/phclsn.cpp
      src/physics/phcontct.cpp
      src/physics/phcore.cpp
      src/physics/phctrl.cpp
      src/physics/phdyn.cpp
      src/physics/phflist.cpp
      src/physics/phlisten.cpp
      src/physics/phmedtr.cpp
      src/physics/phmoapi.cpp
      src/physics/phmod.cpp
      src/physics/phmodbsp.cpp
      src/physics/phmodobb.cpp
      src/physics/phmods.cpp
      src/physics/phmodsph.cpp
      src/physics/phmodutl.cpp
      src/physics/phmsg.cpp
      src/physics/phmterr.cpp
      src/physics/phmtprop.cpp
      src/physics/phmtscpt.cpp
      src/physics/phnet.cpp
      src/physics/phoprop.cpp
      src/physics/phppscpt.cpp
      src/physics/phprop.cpp
      src/physics/phref.cpp
      src/physics/phscrpt.cpp
      src/physics/phmotion.cpp
      src/physics/phmodata.c
      src/physics/phsubmod.cpp
      src/physics/phterr.cpp
      src/physics/phtxtprp.cpp
      src/physics/phutils.cpp
      src/physics/physapi.cpp
      src/physics/physcast.cpp
      src/physics/physloop.cpp
      src/physics/spherchk.cpp
      src/motion/cerebllm.cpp
      src/sim/avatar.cpp
      src/framewrk/ctagnet.cpp
      src/sim/doorphys.cpp
      src/sim/doorprop.cpp
      src/sim/eosapp.c
      src/object/edittool.cpp
      src/sim/flowarch.cpp
      src/sim/ghost.cpp
      src/sim/ghostlst.cpp
      src/sim/ghostlup.cpp
      src/sim/ghostmsg.cpp
      src/sim/ghostphy.cpp
      src/sim/ghostrcv.cpp
      src/sim/ghostsnd.cpp
      src/sim/ghostwep.cpp
      src/sim/headmove.c
      src/sim/objnet.cpp
      src/sim/objpos.cpp
      src/sim/objtool.cpp
      src/sim/playtest.cpp
      src/sim/plyablty.cpp
      src/sim/plycbllm.cpp
      src/sim/plyrhdlr.cpp
      src/sim/plyrloop.cpp
      src/sim/plyrmode.cpp
      src/sim/plyrmov.cpp
      src/sim/plyrvmot.cpp
      src/ai/aicbrclo.cpp
      src/sim/prjctile.cpp
      src/sim/room.cpp
      src/sim/roomcore.cpp
      src/sim/roomeax.cpp
      src/engfeat/quesloop.cpp
      src/sim/roomloop.cpp
      src/sim/roompa.cpp
      src/sim/roomprop.cpp
      src/sim/rooms.cpp
      src/sim/roomutil.cpp
      src/sim/rpaths.cpp
      src/sim/rportal.cpp
      src/sim/simman.cpp
      src/sim/simstate.c
      src/sim/simtime.cpp
      src/sim/wrbsp.cpp
      src/sim/wrcell.c
      src/sim/wrloop.cpp
      src/ai/aimplyr.cpp
      src/ai/aimttest.cpp
      src/motion/biploop.c
      src/motion/crattach.cpp
      src/motion/creature.cpp
      src/motion/cretprop.cpp
      src/motion/crexp.cpp
      src/dark/crjoint.cpp
      src/motion/crwpnapi.cpp
      src/motion/crwpnlup.cpp
      src/motion/crwpnlst.cpp
      src/motion/mcoord.cpp
      src/motion/motdb.cpp
      src/motion/motdesc.cpp
      src/motion/motdmnge.c
      src/motion/motmngr.cpp
      src/motion/motprop.cpp
      src/motion/mschutil.cpp
      src/motion/motschem.cpp
      src/sound/sndprop.cpp
      src/motion/motset.cpp
      src/motion/mskilset.cpp
      src/motion/mvrcmbat.cpp
      src/dark/drkmislp.cpp
      src/motion/mvrflags.cpp
      src/motion/mvrflbow.cpp
      src/motion/mvrgact.cpp
      src/motion/mvrgloco.cpp
      src/motion/mvrplyr.cpp
      src/motion/mvrsngle.cpp
      src/motion/mvrutils.cpp
      src/motion/puppet.cpp
      src/motion/pupprop.cpp
      src/sound/ambient.c
      src/sound/ambprop.cpp
      src/framewrk/appname.c
      src/sound/appsfx.cpp
      src/sound/auxsnd.cpp
      src/framewrk/backup.cpp
      src/framewrk/basemode.c
      src/sound/esnd.cpp
      src/sound/esndprop.cpp
      src/sound/esndreac.cpp
      src/sound/metasnd.cpp
      src/sound/psnd.cpp
      src/sound/psndapi.cpp
      src/sound/psndinst.cpp
      src/sound/schdb.cpp
      src/sound/schema.cpp
      src/sound/schloop.c
      src/sound/schmsg.cpp
      src/sound/schprop.cpp
      src/sound/schsamps.cpp
      src/sound/sndframe.cpp
      src/actreact/arloop.cpp
      src/sound/sndgen.cpp
      src/sound/sndloop.cpp
      src/sound/sndnet.cpp
      src/sound/songi.cpp
      src/sound/songid.cpp
      src/sound/songplri.cpp
      src/sound/songutil.cpp
      src/sound/spchdom.cpp
      src/sound/spchprop.cpp
      src/sound/spchvoc.cpp
      src/sound/speech.cpp
      src/sound/vocore.cpp
      src/engfeat/campaign.cpp
      src/framewrk/chainevt.cpp
      src/engfeat/contain.cpp
      src/engfeat/dmgmodel.cpp
      src/engfeat/dmgprop.cpp
      src/engfeat/dmgreact.cpp
      src/engfeat/engfeat.cpp
      src/engfeat/flinder.cpp
      src/engfeat/frobctrl.cpp
      src/engfeat/invlimb.cpp
      src/engfeat/frobprop.cpp
      src/engfeat/gunflash.cpp
      src/engfeat/gunprop.cpp
      src/engfeat/preldprp.cpp
      src/engfeat/hpprop.cpp
      src/engfeat/inv_rend.c
      src/engfeat/invrndpr.cpp
      src/engfeat/keysys.cpp
      src/engfeat/lockprop.cpp
      src/engfeat/projlink.cpp
      src/engfeat/projprop.cpp
      src/engfeat/propreac.cpp
      src/engfeat/quesreac.cpp
      src/engfeat/quest.cpp
      src/engfeat/questprp.cpp
      src/dark/daicam.cpp
      src/darktabl.c
      src/framewrk/main.cpp
      src/engfeat/simpdmg.cpp
      src/engfeat/susprop.cpp
      src/engfeat/trapprop.cpp
      src/render/hud.c
      src/inc.c
      src/win32/inclex.c
      src/inctab.cpp
      src/engfeat/tweqctrl.cpp
      src/dark/drkuiscr.cpp
      src/engfeat/tweqprop.cpp
      src/engfeat/weapcb.cpp
      src/engfeat/weaphalo.cpp
      src/engfeat/weaphit.cpp
      src/engfeat/weapon.cpp
      src/engfeat/weapprop.cpp
      src/engfeat/contprop.cpp
      src/engfeat/weapreac.cpp
      src/render/anim_txt.c
      src/render/animlgt.c
      src/render/antxloop.cpp
      src/render/antxscpt.cpp
      src/render/camera.c
      src/camguids.c
      src/render/camovl.cpp
      src/render/celstobj.cpp
      src/render/cloudobj.cpp
      src/dark/drkdiff.cpp
      src/render/corona.cpp
      src/render/ctrast.c
      src/render/distobj.cpp
      src/render/dyntex.cpp
      src/render/elitprop.cpp
      src/render/face.cpp
      src/render/family.c
      src/render/fas.c
      src/render/fasprop.cpp
      src/render/fxdisk.c
      src/render/fxspan.c
      src/render/h2ocolor.cpp
      src/render/hdskprop.cpp
      src/render/litnet.cpp
      src/render/litprop.cpp
      src/render/medmotn.c
      src/render/meshray.c
      src/render/meshatt.cpp
      src/render/meshtex.cpp
      src/render/mipmap.c
      src/render/missrend.cpp
      src/render/mlight.c
      src/render/mnamprop.cpp
      src/render/mnumprop.cpp
      src/render/objcast.cpp
      src/render/objlight.c
      src/sim/objloop.cpp
      src/render/objmodel.c
      src/render/objshape.cpp
      src/render/objtxtrp.cpp
      src/engfeat/ownprop.cpp
      src/render/palmgr.c
      src/render/particle.cpp
      src/render/partprop.cpp
      src/render/pgroup.c
      src/render/pgrpprop.cpp
      src/render/portapp.c
      src/render/porthlpr.c
      src/render/rendloop.c
      src/render/render.cpp
      src/render/rendobj.cpp
      src/render/rendphys.cpp
      src/render/rendprop.cpp
      src/render/rendptst.c
      src/framewrk/reputil.cpp
      src/render/rndflash.cpp
      src/render/scalprop.cpp
      src/render/scrnloop.c
      src/render/scrnman.c
      src/render/scrnmode.cpp
      src/render/scrnovls.cpp
      src/render/skyobj.cpp
      src/render/skyrend.cpp
      src/render/slitprop.cpp
      src/render/sphrcst.cpp
      src/render/sprkprop.cpp
      src/render/starhack.c
      src/render/starobj.cpp
      src/render/tanimprp.cpp
      src/render/texmem.c
      src/render/texprop.c
      src/render/texsave.cpp
      src/render/textarch.cpp
      src/ui/uiamov.cpp
      src/render/weather.cpp
      src/ui/winui.c
      src/frezreac.cpp
      src/testloop.c
      src/testmode.c
      src/object/traitlnk.cpp
      src/ai/aiactlch.cpp
      src/sound/songfile.cpp
      src/sound/songmiss.cpp
      src/render/palette.c
      src/physics/phprops.cpp
      src/script/scrptcmd.cpp
      src/dark/drkuires.cpp
    )
  endif()
  # NewDark links the CD script service, which 1.18 did not.
  if(TOOLCHAIN STREQUAL "vs2008")
    target_sources(thief2 PRIVATE src/script/cdscript.cpp src/render/mtlparse.cpp)
  endif()
  target_link_libraries(thief2
    thief2_mprintf
    thief2_lg
    thief2_lgalloc
    thief2_matrix
    thief2_fix
    thief2_dstruct
    thief2_cpptools
    thief2_comtools
    thief2_compapis
    thief2_appcore
    thief2_dev2d
    thief2_g2
    thief2_font
    thief2_dispdev
    thief2_config
    thief2_res
    thief2_input
    thief2_gameshel
    thief2_timer
    thief2_rnd
    thief2_recorder
    thief2_ui
    thief2_gadget
    thief2_sound
    thief2_actmovie
    thief2_quartz
    thief2_r3d
    thief2_star
    thief2_mp
    thief2_md
    thief2_dump
    thief2_vec2
    thief2_darkloop
    thief2_lgd3d
    thief2_mm
    thief2_script
    thief2_sndsrc
    thief2_ref
    thief2_sdesc
    thief2_arq
    thief2_inputbnd
    thief2_namedres
    thief2_net
    thief2_random
    thief2_tagfile
    thief2_entrywin
    # Import descriptors follow library order: kernel32 user32 gdi32 winmm ole32 advapi32.
    # The CRT is not named: libcpmt, LIBCMT and OLDNAMES come from the objects'
    # default-library directives, in that order, after every named library.
    uuid.lib
    kernel32.lib user32.lib gdi32.lib winmm.lib ole32.lib advapi32.lib
    winspool.lib comdlg32.lib shell32.lib oleaut32.lib
    odbc32.lib odbccp32.lib comctl32.lib
    ${CMAKE_BINARY_DIR}/wail32.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/a3d/ia3d.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/miles/mss32.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dx6sdk/lib/ddraw.lib
    ${CMAKE_BINARY_DIR}/qmixer.lib
  )
  add_dependencies(thief2 import_libs)
endif()

# ── Third-party libraries NewDark links (objdiff only, not part of either exe) ──
# zlib 1.2.5, libpng 1.6.34, libogg 1.3.2 and libvorbis 1.3.2, unmodified upstream sources; the versions were
# identified by compiling candidates and scoring them against THIEF.EXE
# (results/newdark_thirdparty.json). zlib and libpng were built /Zp1 like the game (THIEF.EXE
# reads deflate_state.bi_buf at 0x16b2, packed; exact functions 24 -> 47), libogg with default packing. pnglibconf.h is libpng's scripts/pnglibconf.h.prebuilt
# without STDIO, CONSOLE_IO and SIMPLIFIED_WRITE_STDIO (the target lacks the default handlers' strings
# "libpng error: %s", "Read Error"; +96 retail, +98 dromed);
# ogg's config_types.h is config_types.h.in with the MSVC sizes filled in.
if(NOT TOOLCHAIN STREQUAL "vc6")
  add_library(thirdparty_zlib STATIC
    3rdparty/zlib-1.2.5/adler32.c
    3rdparty/zlib-1.2.5/compress.c
    3rdparty/zlib-1.2.5/crc32.c
    3rdparty/zlib-1.2.5/deflate.c
    3rdparty/zlib-1.2.5/gzclose.c
    3rdparty/zlib-1.2.5/gzlib.c
    3rdparty/zlib-1.2.5/gzread.c
    3rdparty/zlib-1.2.5/gzwrite.c
    3rdparty/zlib-1.2.5/infback.c
    3rdparty/zlib-1.2.5/inffast.c
    3rdparty/zlib-1.2.5/inflate.c
    3rdparty/zlib-1.2.5/inftrees.c
    3rdparty/zlib-1.2.5/trees.c
    3rdparty/zlib-1.2.5/uncompr.c
    3rdparty/zlib-1.2.5/zutil.c
  )
  target_include_directories(thirdparty_zlib BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zlib-1.2.5
    ${CMAKE_CURRENT_SOURCE_DIR}/vs2008/include
  )
  target_compile_options(thirdparty_zlib PRIVATE /W0)

  add_library(thirdparty_png STATIC
    3rdparty/libpng-1.6.34/png.c
    3rdparty/libpng-1.6.34/pngerror.c
    3rdparty/libpng-1.6.34/pngget.c
    3rdparty/libpng-1.6.34/pngmem.c
    3rdparty/libpng-1.6.34/pngpread.c
    3rdparty/libpng-1.6.34/pngread.c
    3rdparty/libpng-1.6.34/pngrio.c
    3rdparty/libpng-1.6.34/pngrtran.c
    3rdparty/libpng-1.6.34/pngrutil.c
    3rdparty/libpng-1.6.34/pngset.c
    3rdparty/libpng-1.6.34/pngtrans.c
    3rdparty/libpng-1.6.34/pngwio.c
    3rdparty/libpng-1.6.34/pngwrite.c
    3rdparty/libpng-1.6.34/pngwtran.c
    3rdparty/libpng-1.6.34/pngwutil.c
  )
  target_include_directories(thirdparty_png BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libpng-1.6.34
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zlib-1.2.5
    ${CMAKE_CURRENT_SOURCE_DIR}/vs2008/include
  )
  target_compile_options(thirdparty_png PRIVATE /W0)

  add_library(thirdparty_ogg STATIC
    3rdparty/libogg-1.3.2/src/bitwise.c
    3rdparty/libogg-1.3.2/src/framing.c
  )
  target_include_directories(thirdparty_ogg BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libogg-1.3.2/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vs2008/include
  )
  target_compile_options(thirdparty_ogg PRIVATE /Zp8 /W0)

  # libvorbis 1.3.2: compiled 1.2.3 and 1.3.1-1.3.6 and scored them against DromEd's 0x796dc0-0x7a6720;
  # 1.3.2 wins (floor1_unpack 83 vs 23 for 1.3.3, best sum and mutual count). Built like libogg.
  add_library(thirdparty_vorbis STATIC
    3rdparty/libvorbis-1.3.2/lib/analysis.c
    3rdparty/libvorbis-1.3.2/lib/bitrate.c
    3rdparty/libvorbis-1.3.2/lib/block.c
    3rdparty/libvorbis-1.3.2/lib/codebook.c
    3rdparty/libvorbis-1.3.2/lib/envelope.c
    3rdparty/libvorbis-1.3.2/lib/floor0.c
    3rdparty/libvorbis-1.3.2/lib/floor1.c
    3rdparty/libvorbis-1.3.2/lib/info.c
    3rdparty/libvorbis-1.3.2/lib/lookup.c
    3rdparty/libvorbis-1.3.2/lib/lpc.c
    3rdparty/libvorbis-1.3.2/lib/lsp.c
    3rdparty/libvorbis-1.3.2/lib/mapping0.c
    3rdparty/libvorbis-1.3.2/lib/mdct.c
    3rdparty/libvorbis-1.3.2/lib/psy.c
    3rdparty/libvorbis-1.3.2/lib/registry.c
    3rdparty/libvorbis-1.3.2/lib/res0.c
    3rdparty/libvorbis-1.3.2/lib/sharedbook.c
    3rdparty/libvorbis-1.3.2/lib/smallft.c
    3rdparty/libvorbis-1.3.2/lib/synthesis.c
    3rdparty/libvorbis-1.3.2/lib/vorbisenc.c
    3rdparty/libvorbis-1.3.2/lib/vorbisfile.c
    3rdparty/libvorbis-1.3.2/lib/window.c
  )
  target_include_directories(thirdparty_vorbis BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libvorbis-1.3.2/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libvorbis-1.3.2/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libogg-1.3.2/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vs2008/include
  )
  # /GS-: THIEF.EXE's vorbis functions with alloca/arrays have no security cookie (+11 KB strict); zlib/libpng do.
  target_compile_options(thirdparty_vorbis PRIVATE /Zp8 /W0 /GS-)
  # vorbisfile.c without /GL: THIEF.EXE keeps vorbis_info_init/vorbis_synthesis_read etc. out of line at its call
  # sites (+4 KB strict); /GL- or /Ob1 on the whole library loses ~45 functions.
  set_source_files_properties(3rdparty/libvorbis-1.3.2/lib/vorbisfile.c PROPERTIES COMPILE_OPTIONS "/GL-;/Od;/GS")
endif()

# ── NewDark-only modules (objdiff only for now) ──
# Code in THIEF.EXE with no counterpart in the 1.18 source, reconstructed from the binary:
# subtitles (0x698340-0x69e440). Names are ours; the exe has no RTTI or symbols for it.
# Files with a name in NewDark DromEd's assert paths go under that name
# (docs/notes/project_newdark_file_layout.md).
if(NOT TOOLCHAIN STREQUAL "vc6")
  add_library(newdark OBJECT
    src/sound/subparse.cpp
  )
  target_include_directories(newdark BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sound
  )
endif()
if(TOOLCHAIN STREQUAL "vs2008")
  if(EDITOR)
    target_sources(dromed PRIVATE $<TARGET_OBJECTS:newdark>)
  else()
    target_sources(thief2 PRIVATE $<TARGET_OBJECTS:newdark>)
  endif()
endif()

if(TOOLCHAIN STREQUAL "vs2008")
  set_property(SOURCE src/script/objscrpt.cpp APPEND PROPERTY COMPILE_OPTIONS /EHs-c-)
endif()

if(TOOLCHAIN STREQUAL "vs2008")
  set_property(SOURCE src/dark/drkuiscr.cpp APPEND PROPERTY COMPILE_OPTIONS /EHs-c-)
endif()

if(TOOLCHAIN STREQUAL "vs2008")
  set_property(SOURCE src/script/datascrs.cpp APPEND PROPERTY COMPILE_OPTIONS /EHs-c-)
endif()

# cFileSpec/cFilePath/fnamutil/cDelimList keep cleanup locals across calls yet have no frames in either target.
if(TOOLCHAIN STREQUAL "vs2008")
  set_property(SOURCE libsrc/cpptools/filespec.cpp APPEND PROPERTY COMPILE_OPTIONS /EHs-c-)
  set_property(SOURCE libsrc/cpptools/filepath.cpp APPEND PROPERTY COMPILE_OPTIONS /EHs-c-)
  set_property(SOURCE libsrc/cpptools/fnamutil.cpp APPEND PROPERTY COMPILE_OPTIONS /EHs-c-)
  set_property(SOURCE libsrc/cpptools/delmlist.cpp APPEND PROPERTY COMPILE_OPTIONS /EHs-c-)
endif()

# NewDark DromEd built these tech libraries with asserts off: their AssertMsg/Assert_ sites are absent in the
# target (namedres 7 of our 90 assert calls remain, comtools 3/33, dispdev 11/39; the survivors are CriticalMsg).
# cpptools, lgd3d, arq, inputbnd and the game sources keep them. Retail is SHIP, which drops them anyway.
if(TOOLCHAIN STREQUAL "vs2008" AND EDITOR)
  target_compile_definitions(dromed_namedres PRIVATE NO_ASSERTS)
  target_compile_definitions(dromed_comtools PRIVATE NO_ASSERTS)
  target_compile_definitions(dromed_dispdev PRIVATE NO_ASSERTS)
  target_compile_definitions(dromed_res PRIVATE NO_ASSERTS)
  target_compile_definitions(dromed_gadget PRIVATE NO_ASSERTS)
  target_compile_definitions(dromed_lg PRIVATE NO_ASSERTS)
  target_compile_definitions(dromed_script PRIVATE NO_ASSERTS)
endif()

# NewDark built the tech libraries without C++ exception handling: their functions have no EH
# frames in either target exe, while arq, inputbnd and sdesc (and the game sources) do.
# cpptools was built with /EHsc: under LTCG a caller of a /EHs-c- callee keeps its unwind frame, and the
# cAnsiStr callers have none in either target (per-library sweep: only cpptools gains, lg loses ~450).
if(TOOLCHAIN STREQUAL "vs2008")
  if(EDITOR)
  target_compile_options(dromed_actmovie PRIVATE /EHs-c-)
  target_compile_options(dromed_appcore PRIVATE /EHs-c-)
  target_compile_options(dromed_compapis PRIVATE /EHs-c-)
  target_compile_options(dromed_comtools PRIVATE /EHs-c-)
  target_compile_options(dromed_config PRIVATE /EHs-c-)
  target_compile_options(dromed_darkloop PRIVATE /EHs-c-)
  target_compile_options(dromed_dev2d PRIVATE /EHs-c-)
  target_compile_options(dromed_dispdev PRIVATE /EHs-c-)
  target_compile_options(dromed_dstruct PRIVATE /EHs-c-)
  target_compile_options(dromed_dump PRIVATE /EHs-c-)
  target_compile_options(dromed_entrywin PRIVATE /EHs-c-)
  target_compile_options(dromed_fix PRIVATE /EHs-c-)
  target_compile_options(dromed_font PRIVATE /EHs-c-)
  target_compile_options(dromed_g2 PRIVATE /EHs-c-)
  target_compile_options(dromed_gadget PRIVATE /EHs-c-)
  target_compile_options(dromed_gameshel PRIVATE /EHs-c-)
  target_compile_options(dromed_input PRIVATE /EHs-c-)
  target_compile_options(dromed_lg PRIVATE /EHs-c-)
  target_compile_options(dromed_lgalloc PRIVATE /EHs-c-)
  target_compile_options(dromed_lgd3d PRIVATE /EHs-c-)
  target_compile_options(dromed_matrix PRIVATE /EHs-c-)
  target_compile_options(dromed_md PRIVATE /EHs-c-)
  target_compile_options(dromed_mm PRIVATE /EHs-c-)
  target_compile_options(dromed_mp PRIVATE /EHs-c-)
  target_compile_options(dromed_mprintf PRIVATE /EHs-c-)
  target_compile_options(dromed_namedres PRIVATE /EHs-c-)
  target_compile_options(dromed_net PRIVATE /EHs-c-)
  target_compile_options(dromed_prof PRIVATE /EHs-c-)
  target_compile_options(dromed_quartz PRIVATE /EHs-c-)
  target_compile_options(dromed_r3d PRIVATE /EHs-c-)
  target_compile_options(dromed_random PRIVATE /EHs-c-)
  target_compile_options(dromed_recorder PRIVATE /EHs-c-)
  target_compile_options(dromed_ref PRIVATE /EHs-c-)
  target_compile_options(dromed_res PRIVATE /EHs-c-)
  target_compile_options(dromed_rnd PRIVATE /EHs-c-)
  target_compile_options(dromed_script PRIVATE /EHs-c-)
  target_compile_options(dromed_sndsrc PRIVATE /EHs-c-)
  target_compile_options(dromed_sound PRIVATE /EHs-c-)
  target_compile_options(dromed_star PRIVATE /EHs-c-)
  target_compile_options(dromed_tagfile PRIVATE /EHs-c-)
  target_compile_options(dromed_timer PRIVATE /EHs-c-)
  target_compile_options(dromed_ui PRIVATE /EHs-c-)
  target_compile_options(dromed_vec2 PRIVATE /EHs-c-)
  else()
  target_compile_options(thief2_actmovie PRIVATE /EHs-c-)
  target_compile_options(thief2_appcore PRIVATE /EHs-c-)
  target_compile_options(thief2_compapis PRIVATE /EHs-c-)
  target_compile_options(thief2_comtools PRIVATE /EHs-c-)
  target_compile_options(thief2_config PRIVATE /EHs-c-)
  target_compile_options(thief2_darkloop PRIVATE /EHs-c-)
  target_compile_options(thief2_dev2d PRIVATE /EHs-c-)
  target_compile_options(thief2_dispdev PRIVATE /EHs-c-)
  target_compile_options(thief2_dstruct PRIVATE /EHs-c-)
  target_compile_options(thief2_dump PRIVATE /EHs-c-)
  target_compile_options(thief2_entrywin PRIVATE /EHs-c-)
  target_compile_options(thief2_fix PRIVATE /EHs-c-)
  target_compile_options(thief2_font PRIVATE /EHs-c-)
  target_compile_options(thief2_g2 PRIVATE /EHs-c-)
  target_compile_options(thief2_gadget PRIVATE /EHs-c-)
  target_compile_options(thief2_gameshel PRIVATE /EHs-c-)
  target_compile_options(thief2_input PRIVATE /EHs-c-)
  target_compile_options(thief2_lg PRIVATE /EHs-c-)
  target_compile_options(thief2_lgalloc PRIVATE /EHs-c-)
  target_compile_options(thief2_lgd3d PRIVATE /EHs-c-)
  target_compile_options(thief2_matrix PRIVATE /EHs-c-)
  target_compile_options(thief2_md PRIVATE /EHs-c-)
  target_compile_options(thief2_mm PRIVATE /EHs-c-)
  target_compile_options(thief2_mp PRIVATE /EHs-c-)
  target_compile_options(thief2_mprintf PRIVATE /EHs-c-)
  target_compile_options(thief2_namedres PRIVATE /EHs-c-)
  target_compile_options(thief2_net PRIVATE /EHs-c-)
  target_compile_options(thief2_quartz PRIVATE /EHs-c-)
  target_compile_options(thief2_r3d PRIVATE /EHs-c-)
  target_compile_options(thief2_random PRIVATE /EHs-c-)
  target_compile_options(thief2_recorder PRIVATE /EHs-c-)
  target_compile_options(thief2_ref PRIVATE /EHs-c-)
  target_compile_options(thief2_res PRIVATE /EHs-c-)
  target_compile_options(thief2_rnd PRIVATE /EHs-c-)
  target_compile_options(thief2_script PRIVATE /EHs-c-)
  target_compile_options(thief2_sndsrc PRIVATE /EHs-c-)
  target_compile_options(thief2_sound PRIVATE /EHs-c-)
  target_compile_options(thief2_star PRIVATE /EHs-c-)
  target_compile_options(thief2_tagfile PRIVATE /EHs-c-)
  target_compile_options(thief2_timer PRIVATE /EHs-c-)
  target_compile_options(thief2_ui PRIVATE /EHs-c-)
  target_compile_options(thief2_vec2 PRIVATE /EHs-c-)
  endif()
endif()



# NewDark PNG image reader (libsrc/namedres/imgpng.cpp), PNG screenshot writer
# (libsrc/dump/pngdump.c) and the libpng/zlib they pull in.
if(TOOLCHAIN STREQUAL "vs2008")
  # THIEF.EXE's dmp_png_dump_screen calls the scalar operator new/delete: NewDark compiled pngdump as C++.
  set_property(SOURCE libsrc/dump/pngdump.c APPEND PROPERTY COMPILE_OPTIONS /TP)
  set_property(SOURCE libsrc/namedres/imgpng.cpp libsrc/dump/pngdump.c APPEND PROPERTY INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libpng-1.6.34
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zlib-1.2.5)
  # NewDark dropped namedres' own zlib 1.1.2 copy: zipstrm.cpp builds against zlib 1.2.5 (THIEF.EXE has no "1.1.2"
  # string and LTCG folds the single ZLIB_VERSION argument out of inflateInit2_).
  set_source_files_properties(libsrc/namedres/zipstrm.cpp PROPERTIES INCLUDE_DIRECTORIES
    "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zlib-1.2.5;${COMMON_INCLUDES};${LIBSRC_INCLUDES};${LAST_INCLUDES}")
  if(EDITOR)
    target_sources(dromed_namedres PRIVATE libsrc/namedres/imgpng.cpp libsrc/namedres/imgdds.cpp)
    target_sources(dromed_dump PRIVATE libsrc/dump/pngdump.c)
    target_link_libraries(dromed thirdparty_png thirdparty_zlib)
  else()
    target_sources(thief2_namedres PRIVATE libsrc/namedres/imgpng.cpp libsrc/namedres/imgdds.cpp)
    target_sources(thief2_dump PRIVATE libsrc/dump/bmpdump.c libsrc/dump/pngdump.c)
    target_link_libraries(thief2 thirdparty_png thirdparty_zlib)
  endif()
endif()

# NewDark Ogg Vorbis sound resources (libsrc/sound/sndfmt.cpp) and the vorbis/ogg libraries.
if(TOOLCHAIN STREQUAL "vs2008")
  set_property(SOURCE libsrc/sound/sndfmt.cpp libsrc/sndsrc/oggdec.cpp libsrc/sndsrc/sndseg.cpp src/render/face.cpp libsrc/namedres/sndrstyp.cpp APPEND PROPERTY INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libvorbis-1.3.2/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libogg-1.3.2/include)
  if(EDITOR)
    target_sources(dromed_sndsrc PRIVATE libsrc/sndsrc/oggdec.cpp)
    target_link_libraries(dromed thirdparty_vorbis thirdparty_ogg)
  else()
    target_sources(thief2_sndsrc PRIVATE libsrc/sndsrc/oggdec.cpp)
    target_link_libraries(thief2 thirdparty_vorbis thirdparty_ogg)
  endif()
endif()
