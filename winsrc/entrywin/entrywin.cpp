///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/entrywin/rcs/entrywin.cpp $
// $Author: TOML $
// $Date: 1996/10/21 09:35:22 $
// $Revision: 1.3 $
//
//
// Main entry point for windowed Windows applications
//

#ifdef _WIN32

#include <windows.h>
#include <lg.h>

EXTERN int LGAPI _AppMain(int argc, const char *argv[]);

extern "C"
{
int _g_referenceEntryPoint = 0;
}

int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR pszCmdLine, int fShow)
    {
    // Turn Windows command line into classic arguments    @TBD (toml 04-24-96):  won't do quoted arguments correctly
    int argc = 1;
    const char *argv[32];

    argv[0] = "";
    argv[1] = strtok(pszCmdLine, " ");
    while (argv[argc] != NULL && argc < 31 )
        argv[++argc] = strtok(NULL, " ");

    return _AppMain(argc, argv);

    }

#endif
