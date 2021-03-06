/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            ntvdm.c
 * PURPOSE:         Virtual DOS Machine
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"

#include "bios/bios.h"
#include "cpu/cpu.h"

#include "resource.h"

/* VARIABLES ******************************************************************/

static HANDLE ConsoleInput  = INVALID_HANDLE_VALUE;
static HANDLE ConsoleOutput = INVALID_HANDLE_VALUE;
static DWORD  OrgConsoleInputMode, OrgConsoleOutputMode;

HANDLE VdmTaskEvent = NULL;

// Command line of NTVDM
INT     NtVdmArgc;
WCHAR** NtVdmArgv;


static HMENU hConsoleMenu  = NULL;
static INT   VdmMenuPos    = -1;
static BOOLEAN ShowPointer = FALSE;

/*
 * Those menu helpers were taken from the GUI frontend in winsrv.dll
 */
typedef struct _VDM_MENUITEM
{
    UINT uID;
    const struct _VDM_MENUITEM *SubMenu;
    WORD wCmdID;
} VDM_MENUITEM, *PVDM_MENUITEM;

static const VDM_MENUITEM VdmMenuItems[] =
{
    { IDS_VDM_DUMPMEM_TXT, NULL, ID_VDM_DUMPMEM_TXT },
    { IDS_VDM_DUMPMEM_BIN, NULL, ID_VDM_DUMPMEM_BIN },
    { IDS_VDM_QUIT       , NULL, ID_VDM_QUIT        },

    { 0, NULL, 0 }      /* End of list */
};

static const VDM_MENUITEM VdmMainMenuItems[] =
{
    { -1, NULL, 0 },    /* Separator */
    { IDS_HIDE_MOUSE,   NULL, ID_SHOWHIDE_MOUSE },  /* Hide mouse; can be renamed to Show mouse */
    { IDS_VDM_MENU  ,   VdmMenuItems,         0 },  /* ReactOS VDM Menu */

    { 0, NULL, 0 }      /* End of list */
};

static VOID
AppendMenuItems(HMENU hMenu,
                const VDM_MENUITEM *Items)
{
    UINT i = 0;
    WCHAR szMenuString[255];
    HMENU hSubMenu;

    do
    {
        if (Items[i].uID != (UINT)-1)
        {
            if (LoadStringW(GetModuleHandle(NULL),
                            Items[i].uID,
                            szMenuString,
                            ARRAYSIZE(szMenuString)) > 0)
            {
                if (Items[i].SubMenu != NULL)
                {
                    hSubMenu = CreatePopupMenu();
                    if (hSubMenu != NULL)
                    {
                        AppendMenuItems(hSubMenu, Items[i].SubMenu);

                        if (!AppendMenuW(hMenu,
                                         MF_STRING | MF_POPUP,
                                         (UINT_PTR)hSubMenu,
                                         szMenuString))
                        {
                            DestroyMenu(hSubMenu);
                        }
                    }
                }
                else
                {
                    AppendMenuW(hMenu,
                                MF_STRING,
                                Items[i].wCmdID,
                                szMenuString);
                }
            }
        }
        else
        {
            AppendMenuW(hMenu,
                        MF_SEPARATOR,
                        0,
                        NULL);
        }
        i++;
    } while (!(Items[i].uID == 0 && Items[i].SubMenu == NULL && Items[i].wCmdID == 0));
}

/*static*/ VOID
CreateVdmMenu(HANDLE ConOutHandle)
{
    hConsoleMenu = ConsoleMenuControl(ConOutHandle,
                                      ID_SHOWHIDE_MOUSE,
                                      ID_VDM_QUIT);
    if (hConsoleMenu == NULL) return;

    VdmMenuPos = GetMenuItemCount(hConsoleMenu);
    AppendMenuItems(hConsoleMenu, VdmMainMenuItems);
    DrawMenuBar(GetConsoleWindow());
}

/*static*/ VOID
DestroyVdmMenu(VOID)
{
    UINT i = 0;
    const VDM_MENUITEM *Items = VdmMainMenuItems;

    do
    {
        DeleteMenu(hConsoleMenu, VdmMenuPos, MF_BYPOSITION);
        i++;
    } while (!(Items[i].uID == 0 && Items[i].SubMenu == NULL && Items[i].wCmdID == 0));

    DrawMenuBar(GetConsoleWindow());
}

static VOID ShowHideMousePointer(HANDLE ConOutHandle, BOOLEAN ShowPtr)
{
    WCHAR szMenuString[255] = L"";

    if (ShowPtr)
    {
        /* Be sure the cursor will be shown */
        while (ShowConsoleCursor(ConOutHandle, TRUE) < 0) ;
    }
    else
    {
        /* Be sure the cursor will be hidden */
        while (ShowConsoleCursor(ConOutHandle, FALSE) >= 0) ;
    }

    if (LoadStringW(GetModuleHandle(NULL),
                    (!ShowPtr ? IDS_SHOW_MOUSE : IDS_HIDE_MOUSE),
                    szMenuString,
                    ARRAYSIZE(szMenuString)) > 0)
    {
        ModifyMenu(hConsoleMenu, ID_SHOWHIDE_MOUSE,
                   MF_BYCOMMAND, ID_SHOWHIDE_MOUSE, szMenuString);
    }
}

static VOID EnableExtraHardware(HANDLE ConsoleInput)
{
    DWORD ConInMode;

    if (GetConsoleMode(ConsoleInput, &ConInMode))
    {
#if 0
        // GetNumberOfConsoleMouseButtons();
        // GetSystemMetrics(SM_CMOUSEBUTTONS);
        // GetSystemMetrics(SM_MOUSEPRESENT);
        if (MousePresent)
        {
#endif
            /* Support mouse input events if there is a mouse on the system */
            ConInMode |= ENABLE_MOUSE_INPUT;
#if 0
        }
        else
        {
            /* Do not support mouse input events if there is no mouse on the system */
            ConInMode &= ~ENABLE_MOUSE_INPUT;
        }
#endif

        SetConsoleMode(ConsoleInput, ConInMode);
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
DisplayMessage(LPCWSTR Format, ...)
{
#ifndef WIN2K_COMPLIANT
    WCHAR  StaticBuffer[256];
    LPWSTR Buffer = StaticBuffer; // Use the static buffer by default.
#else
    WCHAR  Buffer[2048]; // Large enough. If not, increase it by hand.
#endif
    size_t MsgLen;
    va_list Parameters;

    va_start(Parameters, Format);

#ifndef WIN2K_COMPLIANT
    /*
     * Retrieve the message length and if it is too long, allocate
     * an auxiliary buffer; otherwise use the static buffer.
     */
    MsgLen = _vscwprintf(Format, Parameters) + 1; // NULL-terminated
    if (MsgLen > ARRAYSIZE(StaticBuffer))
    {
        Buffer = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, MsgLen * sizeof(WCHAR));
        if (Buffer == NULL)
        {
            /* Allocation failed, use the static buffer and display a suitable error message */
            Buffer = StaticBuffer;
            Format = L"DisplayMessage()\nOriginal message is too long and allocating an auxiliary buffer failed.";
            MsgLen = wcslen(Format);
        }
    }
#else
    MsgLen = ARRAYSIZE(Buffer);
#endif

    /* Display the message */
    _vsnwprintf(Buffer, MsgLen, Format, Parameters);
    DPRINT1("\n\nNTVDM Subsystem\n%S\n\n", Buffer);
    MessageBoxW(NULL, Buffer, L"NTVDM Subsystem", MB_OK);

#ifndef WIN2K_COMPLIANT
    /* Free the buffer if needed */
    if (Buffer != StaticBuffer) RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
#endif

    va_end(Parameters);
}

static BOOL
WINAPI
ConsoleCtrlHandler(DWORD ControlType)
{
// HACK: Should be removed!
#ifndef STANDALONE
extern BOOLEAN AcceptCommands;
extern HANDLE CommandThread;
#endif

    switch (ControlType)
    {
        case CTRL_LAST_CLOSE_EVENT:
        {
            if (WaitForSingleObject(VdmTaskEvent, 0) == WAIT_TIMEOUT)
            {
                /* Nothing runs, so exit immediately */
#ifndef STANDALONE
                if (CommandThread) TerminateThread(CommandThread, 0);
#endif
                EmulatorTerminate();
            }
#ifndef STANDALONE
            else
            {
                /* A command is running, let it run, but stop accepting new commands */
                AcceptCommands = FALSE;
            }
#endif

            break;
        }

        default:
        {
            /* Stop the VDM if the user logs out or closes the console */
            EmulatorTerminate();
        }
    }
    return TRUE;
}

static VOID
ConsoleInitUI(VOID)
{
    CreateVdmMenu(ConsoleOutput);
}

static VOID
ConsoleCleanupUI(VOID)
{
    /* Display again properly the mouse pointer */
    if (ShowPointer) ShowHideMousePointer(ConsoleOutput, ShowPointer);

    DestroyVdmMenu();
}

BOOL
ConsoleAttach(VOID)
{
    /* Save the original input and output console modes */
    if (!GetConsoleMode(ConsoleInput , &OrgConsoleInputMode ) ||
        !GetConsoleMode(ConsoleOutput, &OrgConsoleOutputMode))
    {
        CloseHandle(ConsoleOutput);
        CloseHandle(ConsoleInput);
        wprintf(L"FATAL: Cannot save console in/out modes\n");
        // return FALSE;
    }

    /* Set the console input mode */
    // FIXME: Activate ENABLE_WINDOW_INPUT when we will want to perform actions
    // upon console window events (screen buffer resize, ...).
    SetConsoleMode(ConsoleInput, 0 /* | ENABLE_WINDOW_INPUT */);
    EnableExtraHardware(ConsoleInput);

    /* Set the console output mode */
    // SetConsoleMode(ConsoleOutput, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);

    /* Initialize the UI */
    ConsoleInitUI();

    return TRUE;
}

VOID
ConsoleDetach(VOID)
{
    /* Restore the original input and output console modes */
    SetConsoleMode(ConsoleOutput, OrgConsoleOutputMode);
    SetConsoleMode(ConsoleInput , OrgConsoleInputMode );

    /* Cleanup the UI */
    ConsoleCleanupUI();
}

static BOOL
ConsoleInit(VOID)
{
    /* Set the handler routine */
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    /* Enable the CTRL_LAST_CLOSE_EVENT */
    SetLastConsoleEventActive();

    /*
     * NOTE: The CONIN$ and CONOUT$ "virtual" files
     * always point to non-redirected console handles.
     */

    /* Get the input handle to the real console, and check for success */
    ConsoleInput = CreateFileW(L"CONIN$",
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL);
    if (ConsoleInput == INVALID_HANDLE_VALUE)
    {
        wprintf(L"FATAL: Cannot retrieve a handle to the console input\n");
        return FALSE;
    }

    /* Get the output handle to the real console, and check for success */
    ConsoleOutput = CreateFileW(L"CONOUT$",
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                NULL);
    if (ConsoleOutput == INVALID_HANDLE_VALUE)
    {
        CloseHandle(ConsoleInput);
        wprintf(L"FATAL: Cannot retrieve a handle to the console output\n");
        return FALSE;
    }

    /* Effectively attach to the console */
    return ConsoleAttach();
}

static VOID
ConsoleCleanup(VOID)
{
    /* Detach from the console */
    ConsoleDetach();

    /* Close the console handles */
    if (ConsoleOutput != INVALID_HANDLE_VALUE) CloseHandle(ConsoleOutput);
    if (ConsoleInput  != INVALID_HANDLE_VALUE) CloseHandle(ConsoleInput);
}

VOID MenuEventHandler(PMENU_EVENT_RECORD MenuEvent)
{
    switch (MenuEvent->dwCommandId)
    {
        case ID_SHOWHIDE_MOUSE:
            ShowHideMousePointer(ConsoleOutput, ShowPointer);
            ShowPointer = !ShowPointer;
            break;

        case ID_VDM_DUMPMEM_TXT:
            DumpMemory(TRUE);
            break;

        case ID_VDM_DUMPMEM_BIN:
            DumpMemory(FALSE);
            break;

        case ID_VDM_QUIT:
            /* Stop the VDM */
            EmulatorTerminate();
            break;

        default:
            break;
    }
}

VOID FocusEventHandler(PFOCUS_EVENT_RECORD FocusEvent)
{
    DPRINT1("Focus events not handled\n");
}

static BOOL
LoadGlobalSettings(VOID)
{
// FIXME: These strings should be localized.
#define ERROR_MEMORYVDD L"Insufficient memory to load installable Virtual Device Drivers."
#define ERROR_REGVDD    L"Virtual Device Driver format in the registry is invalid."
#define ERROR_LOADVDD   L"An installable Virtual Device Driver failed Dll initialization."

    BOOL  Success = TRUE;
    LONG  Error   = 0;

    HKEY    hNTVDMKey;
    LPCWSTR NTVDMKeyName   = L"SYSTEM\\CurrentControlSet\\Control\\NTVDM";

    /* Try to open the NTVDM registry key */
    Error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          NTVDMKeyName,
                          0,
                          KEY_QUERY_VALUE,
                          &hNTVDMKey);
    if (Error == ERROR_FILE_NOT_FOUND)
    {
        /* If the key just doesn't exist, don't do anything else */
        return TRUE;
    }
    else if (Error != ERROR_SUCCESS)
    {
        /* The key exists but there was an access error: display an error and quit */
        DisplayMessage(ERROR_REGVDD);
        return FALSE;
    }

    /*
     * Now we can do:
     * - CPU core choice
     * - Video choice
     * - Sound choice
     * - Mem?
     * - ...
     * - Standalone mode?
     * - Debug settings
     */

// Quit:
    RegCloseKey(hNTVDMKey);
    return Success;
}

INT
wmain(INT argc, WCHAR *argv[])
{
    NtVdmArgc = argc;
    NtVdmArgv = argv;

#ifdef STANDALONE

    if (argc < 2)
    {
        wprintf(L"\nReactOS Virtual DOS Machine\n\n"
                L"Usage: NTVDM <executable> [<parameters>]\n");
        return 0;
    }

#endif

    /* Load global VDM settings */
    LoadGlobalSettings();

    DPRINT1("\n\n\nNTVDM - Starting...\n\n\n");

    /* Create the task event */
    VdmTaskEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    ASSERT(VdmTaskEvent != NULL);

    /* Initialize the console */
    if (!ConsoleInit())
    {
        wprintf(L"FATAL: A problem occurred when trying to initialize the console\n");
        goto Cleanup;
    }

    /* Initialize the emulator */
    if (!EmulatorInitialize(ConsoleInput, ConsoleOutput))
    {
        wprintf(L"FATAL: Failed to initialize the emulator\n");
        goto Cleanup;
    }

    /* Initialize the system BIOS */
    if (!BiosInitialize(NULL))
    {
        wprintf(L"FATAL: Failed to initialize the VDM BIOS.\n");
        goto Cleanup;
    }

    /* Let's go! Start simulation */
    CpuSimulate();

Cleanup:
    BiosCleanup();
    EmulatorCleanup();
    ConsoleCleanup();

#ifndef STANDALONE
    ExitVDM(FALSE, 0);
#endif

    /* Quit the VDM */
    DPRINT1("\n\n\nNTVDM - Exiting...\n\n\n");

    return 0;
}

/* EOF */
