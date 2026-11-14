#include <windows.h>
#include <string.h>

#include <lg.h>
#include <exceptio.h>
#include <mprintf.h>

#define ONEK            1024
#define SIXTYFOURK      (64 * ONEK)
#define ONEM            (ONEK * ONEK)
#define ONEG            (ONEK * ONEK * ONEK)
#define NUM_CODE_BYTES  16
#define MAX_STACK_DUMP  2048

BOOL g_fTrapExceptions = 1;
static int BeenHere = 0;

static int __cdecl RecordSystemInformation(void);
static void __cdecl RecordModuleList(void);
static void __cdecl ShowModuleInfo(HMODULE ModuleHandle);
static void __cdecl PrintTime(char *output, FILETIME TimeToPrint);
static char *__cdecl GetExceptionDescription(unsigned int ExceptionCode);
static char *__cdecl GetFilePart(char *source);

int __cdecl RecordExceptionInfo(PEXCEPTION_POINTERS data, const char *Message)
{
    int monoMode;
    PEXCEPTION_RECORD Exception;
    char *CrashModuleFileName;
    PCONTEXT Context;
    char *FilePart;
    char *lastperiod;
    char ModuleName[MAX_PATH];
    MEMORY_BASIC_INFORMATION MemInfo;
    char CrashModulePathName[MAX_PATH];
    unsigned char *code;
    int codebyte;

    if (BeenHere || !g_fTrapExceptions)
        return 0;
    BeenHere = 1;

    {
        mprintf("Fatal exception occured; dumping crash info to log...\n");
        monoMode = mono_getmode();
        mono_setmode(0);
        mprintf("\n------------------------------------------------------------------------------\n");
    }

    char FileName[MAX_PATH] = "Unknown";

    {
        if (GetModuleFileNameA(NULL, ModuleName, sizeof(ModuleName)) <= 0)
            ModuleName[0] = 0;
        FilePart = GetFilePart(ModuleName);
        lstrcpyA(FileName, FilePart);
        lastperiod = strrchr(FileName, '.');
        if (lastperiod)
            *lastperiod = 0;
        lstrcpyA(FilePart, "errorlog.txt");
        mprintf("\n\n\n\n");
        Exception = data->ExceptionRecord;
        Context = data->ContextRecord;
        CrashModuleFileName = "Unknown";
        if (VirtualQuery((LPCVOID)Context->Eip, &MemInfo, sizeof(MemInfo))
            && GetModuleFileNameA((HMODULE)MemInfo.AllocationBase, CrashModulePathName, sizeof(CrashModulePathName)) > 0)
        {
            CrashModuleFileName = GetFilePart(CrashModulePathName);
        }
        mprintf("%s caused %s in module %s at %04x:%08x.\n",
            FileName,
            GetExceptionDescription(Exception->ExceptionCode),
            CrashModuleFileName,
            Context->SegCs,
            Context->Eip);
        mprintf("Exception handler called in %s.\n", Message);
        RecordSystemInformation();
        if (Exception->ExceptionCode == STATUS_ACCESS_VIOLATION && Exception->NumberParameters >= 2)
        {
            char DebugMessage[1000];
            const char *readwrite;

            readwrite = "Read from";
            if (Exception->ExceptionInformation[0])
                readwrite = "Write to";
            wsprintf(DebugMessage,
                "%s location %08x caused an access violation.\n",
                readwrite,
                Exception->ExceptionInformation[1]);
            mprintf("%s", DebugMessage);
        }
        mprintf("\n");
        mprintf("Registers:\n");
        mprintf("EAX=%08x CS=%04x EIP=%08x EFLGS=%08x\n",
            Context->Eax, Context->SegCs, Context->Eip, Context->EFlags);
        mprintf("EBX=%08x SS=%04x ESP=%08x EBP=%08x\n",
            Context->Ebx, Context->SegSs, Context->Esp, Context->Ebp);
        mprintf("ECX=%08x DS=%04x ESI=%08x FS=%04x\n",
            Context->Ecx, Context->SegDs, Context->Esi, Context->SegFs);
        mprintf("EDX=%08x ES=%04x EDI=%08x GS=%04x\n",
            Context->Edx, Context->SegEs, Context->Edi, Context->SegGs);
        mprintf("Bytes at CS:EIP:\n");
        code = (unsigned char *)Context->Eip;
        for (codebyte = 0; codebyte < NUM_CODE_BYTES; codebyte++)
            __try
            {
                mprintf("%02x ", code[codebyte]);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                mprintf("?? ");
            }
        mprintf("\nStack dump:\n");
        __try
        {
            unsigned int *pStack;
            unsigned int *pStackTop;
            int Count;
            char buffer[1000];
            char *nearend;
            char *output;

            pStack = (unsigned int *)Context->Esp;
            __asm mov eax, fs:[4]
            __asm mov pStackTop, eax
            if (pStackTop > pStack + MAX_STACK_DUMP)
                pStackTop = pStack + MAX_STACK_DUMP;
            Count = 0;
            buffer[0] = *"";
            memset(&buffer[1], 0, sizeof(buffer) - 1);
            {
                const int margin = 50;
                nearend = &buffer[sizeof(buffer) - margin];
            }
            output = buffer;
            while (pStack + 1 <= pStackTop)
            {
                char *Suffix;

                if (!(Count % 8))
                    output += wsprintf(output, "%08x: ", pStack);
                Suffix = " ";
                if (!(++Count % 8) || pStack + 2 > pStackTop)
                    Suffix = "\n";
                output += wsprintf(output, "%08x%s", *pStack, Suffix);
                pStack++;
                if (output > nearend)
                {
                    mprintf("%s", buffer);
                    buffer[0] = 0;
                    output = buffer;
                }
            }
            mprintf("%s", buffer);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            mprintf("Exception encountered during stack dump.\n");
        }
        RecordModuleList();
        mprintf("\n------------------------------------------------------------------------------\n");
        mono_setmode(monoMode);
    }
    return 0;
}

static void __cdecl RecordModuleList(void)
{
    void *LastAllocationBase;
    unsigned int pageNum;
    struct {
        unsigned int PageSize;
        unsigned int NumPages;
    } locals;
    SYSTEM_INFO SystemInfo;
    MEMORY_BASIC_INFORMATION MemInfo;

    mprintf("\nModule list: names, addresses, sizes, time stamps and file times:\n");
    GetSystemInfo(&SystemInfo);
    locals.PageSize = SystemInfo.dwPageSize;
    locals.NumPages = 4 * (ONEG / locals.PageSize);
    pageNum = 0;
    LastAllocationBase = 0;
    while (pageNum < locals.NumPages)
    {
        if (VirtualQuery((LPCVOID)(pageNum * locals.PageSize), &MemInfo, sizeof(MemInfo)))
        {
            if (MemInfo.RegionSize > 0)
            {
                pageNum += MemInfo.RegionSize / locals.PageSize;
                if (MemInfo.State == MEM_COMMIT)
                {
                    if (MemInfo.AllocationBase > LastAllocationBase)
                    {
                        LastAllocationBase = MemInfo.AllocationBase;
                        ShowModuleInfo((HMODULE)LastAllocationBase);
                    }
                }
            }
            else
            {
                pageNum = pageNum + (SIXTYFOURK / locals.PageSize);
            }
        }
        else
        {
            pageNum = pageNum + (SIXTYFOURK / locals.PageSize);
        }
    }
}

static void __cdecl ShowModuleInfo(HMODULE ModuleHandle)
{
    static char g_TimeBufferInitByte;
    char filename[MAX_PATH];
    unsigned int FileSize;
    char TimeBuffer[100];
    IMAGE_DOS_HEADER *DosHeader;
    HANDLE ModuleFile;
    IMAGE_NT_HEADERS *NTHeader;
    FILETIME LastWrite;

    __try
    {
        if (GetModuleFileNameA(ModuleHandle, filename, sizeof(filename)) > 0)
        {
            DosHeader = (IMAGE_DOS_HEADER *)ModuleHandle;
        if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
            return;
        NTHeader = (IMAGE_NT_HEADERS *)((char *)DosHeader + DosHeader->e_lfanew);
        if (NTHeader->Signature != IMAGE_NT_SIGNATURE)
            return;
        ModuleFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        TimeBuffer[0] = g_TimeBufferInitByte;
        memset(&TimeBuffer[1], 0, sizeof(TimeBuffer) - 1);
        FileSize = 0;
        if (ModuleFile != INVALID_HANDLE_VALUE)
        {
            FileSize = GetFileSize(ModuleFile, NULL);
            if (GetFileTime(ModuleFile, NULL, NULL, &LastWrite))
            {
                wsprintf(TimeBuffer, " - file date is ");
                PrintTime(TimeBuffer + lstrlenA(TimeBuffer), LastWrite);
            }
            CloseHandle(ModuleFile);
        }
        mprintf("%s, loaded at 0x%08x - %d bytes - %08x%s\n",
            filename,
            ModuleHandle,
            FileSize,
            NTHeader->FileHeader.TimeDateStamp,
            TimeBuffer);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

static void __cdecl PrintTime(char *output, FILETIME TimeToPrint)
{
    WORD Time;
    WORD Date;

    if (FileTimeToLocalFileTime(&TimeToPrint, &TimeToPrint) && FileTimeToDosDateTime(&TimeToPrint, &Date, &Time))
        wsprintf(output,
            "%d/%d/%d %02d:%02d:%02d",
            (Date / 32) & 15,
            Date & 31,
            Date / 512 + 1980,
            Time / 2048,
            (Time / 32) & 63,
            2 * (Time & 31));
    else
        *output = 0;
}

static int __cdecl RecordSystemInformation()
{
    char UserName[200];
    MEMORYSTATUS MemInfo;
    char ModuleName[MAX_PATH];
    char TimeBuffer[100];
    SYSTEM_INFO SystemInfo;
    FILETIME CurrentTime;
    DWORD UserNameSize;

    GetSystemTimeAsFileTime(&CurrentTime);
    PrintTime(TimeBuffer, CurrentTime);
    mprintf("Error occurred at %s.\n", TimeBuffer);
    if (GetModuleFileNameA(NULL, ModuleName, sizeof(ModuleName)) <= 0)
    {
        lstrcpyA(ModuleName, "Unknown");
    }
    UserNameSize = sizeof(UserName);
    if (!GetUserNameA(UserName, &UserNameSize))
        lstrcpyA(UserName, "Unknown");
    mprintf("%s, run by %s.\n", ModuleName, UserName);
    GetSystemInfo(&SystemInfo);
    mprintf("%d processor(s), type %d.\n", SystemInfo.dwNumberOfProcessors, SystemInfo.dwProcessorType);
    MemInfo.dwLength = sizeof(MemInfo);
    GlobalMemoryStatus(&MemInfo);
    return mprintf("%d MBytes physical memory.\n", (MemInfo.dwTotalPhys + ONEM - 1) / ONEM);
}

static char *__cdecl GetExceptionDescription(unsigned int ExceptionCode)
{
    int i;

    struct {
        unsigned int Code;
        char *Name;
    } ExceptionNames[] = {
        { DBG_CONTROL_C, "a Control-C" },
        { DBG_CONTROL_BREAK, "a Control-Break" },
        { STATUS_DATATYPE_MISALIGNMENT, "a Datatype Misalignment" },
        { STATUS_BREAKPOINT, "a Breakpoint" },
        { STATUS_ACCESS_VIOLATION, "an Access Violation" },
        { STATUS_IN_PAGE_ERROR, "an In Page Error" },
        { STATUS_NO_MEMORY, "a No Memory" },
        { STATUS_ILLEGAL_INSTRUCTION, "an Illegal Instruction" },
        { STATUS_NONCONTINUABLE_EXCEPTION, "a Noncontinuable Exception" },
        { STATUS_INVALID_DISPOSITION, "an Invalid Disposition" },
        { STATUS_ARRAY_BOUNDS_EXCEEDED, "a Array Bounds Exceeded" },
        { STATUS_FLOAT_DENORMAL_OPERAND, "a Float Denormal Operand" },
        { STATUS_FLOAT_DIVIDE_BY_ZERO, "a Float Divide by Zero" },
        { STATUS_FLOAT_INEXACT_RESULT, "a Float Inexact Result" },
        { STATUS_FLOAT_INVALID_OPERATION, "a Float Invalid Operation" },
        { STATUS_FLOAT_OVERFLOW, "a Float Overflow" },
        { STATUS_FLOAT_STACK_CHECK, "a Float Stack Check" },
        { STATUS_FLOAT_UNDERFLOW, "a Float Underflow" },
        { STATUS_INTEGER_DIVIDE_BY_ZERO, "an Integer Divide by Zero" },
        { STATUS_INTEGER_OVERFLOW, "an Integer Overflow" },
        { STATUS_PRIVILEGED_INSTRUCTION, "a Privileged Instruction" },
        { STATUS_STACK_OVERFLOW, "a Stack Overflow" },
        { 0xC0000142, "a DLL Initialization Failed" },
        { 0xE06D7363, "a Microsoft C++ Exception" },
    };

    for (i = 0; i < sizeof(ExceptionNames) / sizeof(ExceptionNames[0]); i++)
    {
        if (ExceptionCode == ExceptionNames[i].Code)
            return ExceptionNames[i].Name;
    }
    return "Unknown exception type";
}

static char *__cdecl GetFilePart(char *source)
{
    char *result;

    result = strrchr(source, '\\');
    if (result)
        result = result + 1;
    else
        result = source;
    return result;
}
