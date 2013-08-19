#pragma once

typedef struct _DESKTOP
{
    PDESKTOPINFO pDeskInfo;
    LIST_ENTRY ListEntry;
    /* Pointer to the associated window station. */
    struct _WINSTATION_OBJECT *rpwinstaParent;
    DWORD dwDTFlags;
    PWND spwndForeground;
    PWND spwndTray;
    PWND spwndMessage;
    PWND spwndTooltip;
    PVOID hsectionDesktop;
    PWIN32HEAP pheapDesktop;
    ULONG_PTR ulHeapSize;
    LIST_ENTRY PtiList;
    /* Use for tracking mouse moves. */
    PWND spwndTrack;
    DWORD htEx;
    RECT rcMouseHover;
    DWORD dwMouseHoverTime;

    /* ReactOS */
    /* Pointer to the active queue. */
    struct _USER_MESSAGE_QUEUE *ActiveMessageQueue;
    /* Handle of the desktop window. */
    HWND DesktopWindow;
    /* Thread blocking input */
    PVOID BlockInputThread;
    LIST_ENTRY ShellHookWindows;
} DESKTOP;

// Desktop flags
#define DF_TME_HOVER        0x00000400
#define DF_TME_LEAVE        0x00000800
#define DF_HOTTRACK         0x00004000
#define DF_DESTROYED        0x00008000
#define DF_DESKWNDDESTROYED 0x00010000
#define DF_DYING            0x00020000

// Index offset for Desktop data. Should these be global?
#define DT_GWL_PROCESSID 0
#define DT_GWL_THREADID  4

#define DESKTOP_READ       STANDARD_RIGHTS_READ      | \
                           DESKTOP_ENUMERATE         | \
                           DESKTOP_READOBJECTS

#define DESKTOP_WRITE       STANDARD_RIGHTS_WRITE    | \
                            DESKTOP_CREATEMENU       | \
                            DESKTOP_CREATEWINDOW     | \
                            DESKTOP_HOOKCONTROL      | \
                            DESKTOP_JOURNALPLAYBACK  | \
                            DESKTOP_JOURNALRECORD    | \
                            DESKTOP_WRITEOBJECTS

#define DESKTOP_EXECUTE     STANDARD_RIGHTS_EXECUTE  | \
                            DESKTOP_SWITCHDESKTOP

#define DESKTOP_ALL_ACCESS  STANDARD_RIGHTS_REQUIRED | \
                            DESKTOP_CREATEMENU       | \
                            DESKTOP_CREATEWINDOW     | \
                            DESKTOP_ENUMERATE        | \
                            DESKTOP_HOOKCONTROL      | \
                            DESKTOP_JOURNALPLAYBACK  | \
                            DESKTOP_JOURNALRECORD    | \
                            DESKTOP_READOBJECTS      | \
                            DESKTOP_SWITCHDESKTOP    | \
                            DESKTOP_WRITEOBJECTS

extern PDESKTOP InputDesktop;
extern PCLS DesktopWindowClass;
extern HDC ScreenDeviceContext;
extern PTHREADINFO gptiDesktopThread;

typedef struct _SHELL_HOOK_WINDOW
{
  LIST_ENTRY ListEntry;
  HWND hWnd;
} SHELL_HOOK_WINDOW, *PSHELL_HOOK_WINDOW;

INIT_FUNCTION
NTSTATUS
NTAPI
InitDesktopImpl(VOID);

NTSTATUS
APIENTRY
IntDesktopObjectParse(IN PVOID ParseObject,
                      IN PVOID ObjectType,
                      IN OUT PACCESS_STATE AccessState,
                      IN KPROCESSOR_MODE AccessMode,
                      IN ULONG Attributes,
                      IN OUT PUNICODE_STRING CompleteName,
                      IN OUT PUNICODE_STRING RemainingName,
                      IN OUT PVOID Context OPTIONAL,
                      IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
                      OUT PVOID *Object);

VOID APIENTRY
IntDesktopObjectDelete(PWIN32_DELETEMETHOD_PARAMETERS Parameters);

NTSTATUS NTAPI 
IntDesktopOkToClose(PWIN32_OKAYTOCLOSEMETHOD_PARAMETERS Parameters);

NTSTATUS NTAPI 
IntDesktopObjectOpen(PWIN32_OPENMETHOD_PARAMETERS Parameters);

NTSTATUS NTAPI 
IntDesktopObjectClose(PWIN32_CLOSEMETHOD_PARAMETERS Parameters);

HDC FASTCALL
IntGetScreenDC(VOID);

HWND FASTCALL
IntGetDesktopWindow (VOID);

PWND FASTCALL
UserGetDesktopWindow(VOID);

HWND FASTCALL
IntGetCurrentThreadDesktopWindow(VOID);

PUSER_MESSAGE_QUEUE FASTCALL
IntGetFocusMessageQueue(VOID);

VOID FASTCALL
IntSetFocusMessageQueue(PUSER_MESSAGE_QUEUE NewQueue);

PDESKTOP FASTCALL
IntGetActiveDesktop(VOID);

NTSTATUS FASTCALL
co_IntShowDesktop(PDESKTOP Desktop, ULONG Width, ULONG Height, BOOL Redraw);

NTSTATUS FASTCALL
IntHideDesktop(PDESKTOP Desktop);

BOOL IntSetThreadDesktop(IN HDESK hDesktop,
                         IN BOOL FreeOnFailure);

NTSTATUS FASTCALL
IntValidateDesktopHandle(
   HDESK Desktop,
   KPROCESSOR_MODE AccessMode,
   ACCESS_MASK DesiredAccess,
   PDESKTOP *Object);

NTSTATUS FASTCALL
IntParseDesktopPath(PEPROCESS Process,
                    PUNICODE_STRING DesktopPath,
                    HWINSTA *hWinSta,
                    HDESK *hDesktop);

VOID APIENTRY UserRedrawDesktop(VOID);
BOOL IntRegisterShellHookWindow(HWND hWnd);
BOOL IntDeRegisterShellHookWindow(HWND hWnd);
VOID co_IntShellHookNotify(WPARAM Message, WPARAM wParam, LPARAM lParam);
HDC FASTCALL UserGetDesktopDC(ULONG,BOOL,BOOL);

#define IntIsActiveDesktop(Desktop) \
  ((Desktop)->rpwinstaParent->ActiveDesktop == (Desktop))

#define GET_DESKTOP_NAME(d)                                             \
    OBJECT_HEADER_TO_NAME_INFO(OBJECT_TO_OBJECT_HEADER(d)) ?            \
    &(OBJECT_HEADER_TO_NAME_INFO(OBJECT_TO_OBJECT_HEADER(d))->Name) :   \
    NULL

HWND FASTCALL IntGetMessageWindow(VOID);
PWND FASTCALL UserGetMessageWindow(VOID);

static __inline PVOID
DesktopHeapAlloc(IN PDESKTOP Desktop,
                 IN SIZE_T Bytes)
{
    return RtlAllocateHeap(Desktop->pheapDesktop,
                           HEAP_NO_SERIALIZE,
                           Bytes);
}

static __inline BOOL
DesktopHeapFree(IN PDESKTOP Desktop,
                IN PVOID lpMem)
{
    return RtlFreeHeap(Desktop->pheapDesktop,
                       HEAP_NO_SERIALIZE,
                       lpMem);
}

static __inline PVOID
DesktopHeapReAlloc(IN PDESKTOP Desktop,
                   IN PVOID lpMem,
                   IN SIZE_T Bytes)
{
#if 0
    /* NOTE: ntoskrnl doesn't export RtlReAllocateHeap... */
    return RtlReAllocateHeap(Desktop->pheapDesktop,
                             HEAP_NO_SERIALIZE,
                             lpMem,
                             Bytes);
#else
    SIZE_T PrevSize;
    PVOID pNew;

    PrevSize = RtlSizeHeap(Desktop->pheapDesktop,
                           HEAP_NO_SERIALIZE,
                           lpMem);

    if (PrevSize == Bytes)
        return lpMem;

    pNew = RtlAllocateHeap(Desktop->pheapDesktop,
                           HEAP_NO_SERIALIZE,
                           Bytes);
    if (pNew != NULL)
    {
        if (PrevSize < Bytes)
            Bytes = PrevSize;

        RtlCopyMemory(pNew,
                      lpMem,
                      Bytes);

        RtlFreeHeap(Desktop->pheapDesktop,
                    HEAP_NO_SERIALIZE,
                    lpMem);
    }

    return pNew;
#endif
}

static __inline ULONG_PTR
DesktopHeapGetUserDelta(VOID)
{
    PW32HEAP_USER_MAPPING Mapping;
    PTHREADINFO pti;
    PPROCESSINFO W32Process;
    PWIN32HEAP pheapDesktop;
    ULONG_PTR Delta = 0;

    pti = PsGetCurrentThreadWin32Thread();
    if (!pti->rpdesk)
        return 0;

    pheapDesktop = pti->rpdesk->pheapDesktop;

    W32Process = PsGetCurrentProcessWin32Process();
    Mapping = W32Process->HeapMappings.Next;
    while (Mapping != NULL)
    {
        if (Mapping->KernelMapping == (PVOID)pheapDesktop)
        {
            Delta = (ULONG_PTR)Mapping->KernelMapping - (ULONG_PTR)Mapping->UserMapping;
            break;
        }

        Mapping = Mapping->Next;
    }

    return Delta;
}

static __inline PVOID
DesktopHeapAddressToUser(PVOID lpMem)
{
    PW32HEAP_USER_MAPPING Mapping;
    PPROCESSINFO W32Process;

    W32Process = PsGetCurrentProcessWin32Process();
    Mapping = W32Process->HeapMappings.Next;
    while (Mapping != NULL)
    {
        if ((ULONG_PTR)lpMem >= (ULONG_PTR)Mapping->KernelMapping &&
            (ULONG_PTR)lpMem < (ULONG_PTR)Mapping->KernelMapping + Mapping->Limit)
        {
            return (PVOID)(((ULONG_PTR)lpMem - (ULONG_PTR)Mapping->KernelMapping) +
                           (ULONG_PTR)Mapping->UserMapping);
        }

        Mapping = Mapping->Next;
    }

    return NULL;
}

PWND FASTCALL IntGetThreadDesktopWindow(PTHREADINFO);
PWND FASTCALL co_GetDesktopWindow(PWND);
BOOL FASTCALL IntPaintDesktop(HDC);
BOOL FASTCALL DesktopWindowProc(PWND, UINT, WPARAM, LPARAM, LRESULT *);
BOOL FASTCALL UserMessageWindowProc(PWND pwnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT *lResult);
VOID NTAPI DesktopThreadMain();
/* EOF */
