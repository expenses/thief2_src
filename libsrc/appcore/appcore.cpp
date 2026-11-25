
///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/appcore/RCS/appcore.cpp $
// $Author: TOML $
// $Date: 1997/02/15 17:44:44 $
// $Revision: 1.28 $
//

#ifdef _WIN32
#include <windows.h>
#endif

#include <lg.h>
#include <codewrit.h>
#include <appagg.h>
#include <objcguid.h>
#include <objcoll.h>
#include <exceptio.h>
#include <stdlib.h>
#include <stdio.h>
#include <lglog.h>
#include <lgassert.h>

///////////////////////////////////////////////////////////////////////////////

static IAggregate * pAppAggregate;

///////////////////////////////////////////////////////////////////////////////

tResult _AppAggregateCreate(REFIID)
{
    pAppAggregate = CreateGenericAggregate("Application Aggregate", kISetKeyLGGUIDs);
    AssertMsg(pAppAggregate, "Failed to create application aggregate!");
    _AppSetAggregate(pAppAggregate);
    return (pAppAggregate) ? NOERROR : E_FAIL;
}

///////////////////////////////////////

static BOOL g_fCallAppExit;
static BOOL g_fReturnedFromMain;

void AppOnExit()
{
    ClipCursor(NULL);

    // Tell the app the components are about to go away
    if (g_fCallAppExit)
        AppExit();
    if (pAppAggregate)
    {
        pAppAggregate->End();

        //
        // This module always retains 1 reference on the aggregate. However, this
        // function must handle the (not too unusual) case of closing without cleaning
        // up all reference counts.
        //
        ulong ulRefsRemaining = pAppAggregate->ReleaseAll();
        //AssertMsg1(!g_fReturnedFromMain || ulRefsRemaining == 1, "Reference of application object is incorrect on exit (was %d, expected 1)", ulRefsRemaining);
        pAppAggregate = NULL;
    }
}

///////////////////////////////////////

#ifndef PF_XMMI64_INSTRUCTIONS_AVAILABLE
#define PF_XMMI64_INSTRUCTIONS_AVAILABLE 10
#endif
#ifndef PF_SSE3_INSTRUCTIONS_AVAILABLE
#define PF_SSE3_INSTRUCTIONS_AVAILABLE 13
#endif
#ifndef PF_3DNOW_INSTRUCTIONS_AVAILABLE
#define PF_3DNOW_INSTRUCTIONS_AVAILABLE 7
#endif
#ifndef PROCESSOR_ARCHITECTURE_AMD64
#define PROCESSOR_ARCHITECTURE_AMD64 9
#endif
#ifndef PROCESSOR_ARCHITECTURE_IA32_ON_WIN64
#define PROCESSOR_ARCHITECTURE_IA32_ON_WIN64 10
#endif
#ifndef SM_CXVIRTUALSCREEN
#define SM_CXVIRTUALSCREEN 78
#define SM_CYVIRTUALSCREEN 79
#endif

typedef BOOL (WINAPI * tIsProcessorFeaturePresentFunc)(DWORD);

extern "C" {
DWORD g_CPUSSELevel;
BOOL g_bCPUHasMMX;
BOOL g_bCPUHas3DNow;
}

void DetectCPUFeatures()
{
    g_CPUSSELevel = 0;
    g_bCPUHasMMX = FALSE;
    g_bCPUHas3DNow = FALSE;

    HMODULE hKernel = LoadLibrary("kernel32.dll");
    if (hKernel)
    {
        tIsProcessorFeaturePresentFunc pfnIsProcessorFeaturePresent =
            (tIsProcessorFeaturePresentFunc)GetProcAddress(hKernel, "IsProcessorFeaturePresent");
        if (pfnIsProcessorFeaturePresent)
        {
            if (pfnIsProcessorFeaturePresent(PF_SSE3_INSTRUCTIONS_AVAILABLE))
                g_CPUSSELevel = 3;
            else if (pfnIsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE))
                g_CPUSSELevel = 2;
            else if (pfnIsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE))
                g_CPUSSELevel = 1;

            if (pfnIsProcessorFeaturePresent(PF_MMX_INSTRUCTIONS_AVAILABLE))
                g_bCPUHasMMX = TRUE;
            if (pfnIsProcessorFeaturePresent(PF_3DNOW_INSTRUCTIONS_AVAILABLE))
                g_bCPUHas3DNow = TRUE;
        }
        FreeLibrary(hKernel);
    }
}

///////////////////////////////////////////////////////////////////////////////

extern "C" {

int _g_referenceExtendedStartup = 0;
extern int _g_referenceEntryPoint;

};

///////////////////////////////////////////////////////////////////////////////
//
// This is the primary client entry-point
//

extern "C"
int LGAPI _AppMain(int argc, const char *argv[])
{
    DetectCPUFeatures();

#ifndef SHIP
    int nAssertMode = GetPrivateProfileInt("DebugOptions", "AssertMode", 1, ".\\lg.ini");
    if (nAssertMode > 0)
    {
        g_fQuietAssert = FALSE;
        g_fAssertAlwaysAsk = (nAssertMode > 1);
    }
    else if (nAssertMode == 0)
        g_fQuietAssert = TRUE;
#endif

    if (LogGetOptions() & kLogToAll)
    {
        SYSTEM_INFO si = { 0 };
        GetSystemInfo(&si);

        const char *pszArch;
        char szArch[32];
        switch (si.wProcessorArchitecture)
        {
            case PROCESSOR_ARCHITECTURE_INTEL:
            case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
                pszArch = "x86-64";
                break;
            case PROCESSOR_ARCHITECTURE_AMD64:
                pszArch = "x64";
                break;
            case PROCESSOR_ARCHITECTURE_IA64:
                pszArch = "ia64";
                break;
            default:
                sprintf(szArch, "%d", si.wProcessorArchitecture);
                pszArch = szArch;
                break;
        }

        LogMsg1("PID: %d", GetCurrentProcessId());
        LogMsg3("CPU: arch=%s , fam=%d , cpus=%d", pszArch, si.dwProcessorType, si.dwNumberOfProcessors);
        LogMsg3("SIMD: SSE=%d , 3DNow=%d , MMX=%d", g_CPUSSELevel, g_bCPUHas3DNow, g_bCPUHasMMX);

        BYTE *pImage = (BYTE *)GetModuleHandle(NULL);
        if (((IMAGE_DOS_HEADER *)pImage)->e_magic == IMAGE_DOS_SIGNATURE)
        {
            IMAGE_NT_HEADERS *pNT = (IMAGE_NT_HEADERS *)(pImage + ((IMAGE_DOS_HEADER *)pImage)->e_lfanew);
            if (pNT->Signature == IMAGE_NT_SIGNATURE && (pNT->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE))
                LogMsg("Large Address Aware is enabled.");
        }

        OSVERSIONINFO osvi = { sizeof(OSVERSIONINFO) };
        if (GetVersionEx(&osvi))
            LogMsg(_LogFmt("OS Version: %d.%d.%d %s", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber, osvi.szCSDVersion));

        LogMsg2("Desktop resolution: %d x %d", GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
        LogMsg2("Virtual desktop resolution: %d x %d", GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN));
        LogMsg("-----------------------------------------------------------");

        const char *pszCmdLine = GetCommandLine();
        if (pszCmdLine)
        {
            LogMsg1("Command line: %s", pszCmdLine);
            LogMsg("-----------------------------------------------------------");
        }
    }

    // Force link of main/WinMain/LibMain
    _g_referenceEntryPoint = 1;

#ifdef _WIN32
    // Allow our code to be self-modifying
    if (GetPrivateProfileInt("AppCore", "MakeAllCodeWritable", !!(g_fAppStartupFlags & kASF_MakeCodeWritable), ".\\lg.ini"))
        MakeAllCodeWritable();
#endif

    // Increase the number of available handles in the standard buffered i/o
    #ifdef __WATCOMC__
    _grow_handles(60);
    #else
    #pragma message("Need a way to increase available file handles")
    #endif

    int mainRetVal = 1;

    BEGIN_EXCEPT_TRY
    {
    // Create the core aggregate entity
    _AppAggregateCreate(IID_IAggregate);

    // Tell the app to create the application-level COM objects
    if (AppCreateObjects(argc, argv) == NOERROR)
    {
        // Clean-up is done through exit()
        atexit(AppOnExit);

        // Initialize the components
        if (pAppAggregate)
            pAppAggregate->Init();

        // Tell the app the components are initialized
        if (AppInit() == NOERROR)
        {
            // Set flag indicating successful initialization, so should call AppExit()
            g_fCallAppExit = TRUE;

            // Start the application
            mainRetVal = AppMain(argc, argv);
        }
    }
    g_fReturnedFromMain = TRUE;
    }
    BEGIN_EXCEPT_CATCH("_AppMain()")
    {
    }

    return mainRetVal;
}

///////////////////////////////////////////////////////////////////////////////
//
// Symbol to Force inclusion of this extended startup code
//

int __ExtendedStartup(void)
{
    return 0;
}
