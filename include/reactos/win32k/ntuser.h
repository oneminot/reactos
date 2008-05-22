#ifndef __WIN32K_NTUSER_H
#define __WIN32K_NTUSER_H

struct _W32PROCESSINFO;
struct _W32THREADINFO;
struct _WINDOW;

/* FIXME: UserHMGetHandle needs to be updated once the new handle manager is implemented */
#define UserHMGetHandle(obj) ((obj)->hdr.Handle)

typedef struct _REGISTER_SYSCLASS
{
    /* This is a reactos specific class used to initialize the
       system window classes during user32 initialization */
    UNICODE_STRING ClassName;
    UINT Style;
    WNDPROC ProcW;
    WNDPROC ProcA;
    UINT ExtraBytes;
    HICON hCursor;
    HBRUSH hBrush;
    UINT ClassId;
} REGISTER_SYSCLASS, *PREGISTER_SYSCLASS;

typedef struct _USER_OBJHDR
{
    /* This is the common header for all user handle objects */
    HANDLE Handle;
} USER_OBJHDR, PUSER_OBJHDR;

typedef struct _DESKTOP
{
    HANDLE hKernelHeap;
    ULONG_PTR HeapLimit;
    HWND hTaskManWindow;
    HWND hProgmanWindow;
    struct _WINDOW *Wnd;

    union
    {
        UINT Dummy;
        struct
        {
            UINT LastInputWasKbd : 1;
        };
    };

    WCHAR szDesktopName[1];
} DESKTOP, *PDESKTOP;

typedef struct _CALLPROC
{
    USER_OBJHDR hdr; /* FIXME: Move out of the structure once new handle manager is implemented */
    struct _W32PROCESSINFO *pi;
    WNDPROC WndProc;
    struct _CALLPROC *Next;
    UINT Unicode : 1;
} CALLPROC, *PCALLPROC;

typedef struct _WINDOWCLASS
{
    struct _WINDOWCLASS *Next;
    struct _WINDOWCLASS *Clone;
    struct _WINDOWCLASS *Base;
    PDESKTOP Desktop;
    RTL_ATOM Atom;
    ULONG Windows;

    UINT Style;
    WNDPROC WndProc;
    union
    {
        WNDPROC WndProcExtra;
        PCALLPROC CallProc;
    };
    PCALLPROC CallProcList;
    INT ClsExtra;
    INT WndExtra;
    PVOID Dce;
    HINSTANCE hInstance;
    HANDLE hIcon; /* FIXME - Use pointer! */
    HANDLE hIconSm; /* FIXME - Use pointer! */
    HANDLE hCursor; /* FIXME - Use pointer! */
    HBRUSH hbrBackground;
    HANDLE hMenu; /* FIXME - Use pointer! */
    PWSTR MenuName;
    PSTR AnsiMenuName;

    UINT Destroying : 1;
    UINT Unicode : 1;
    UINT System : 1;
    UINT Global : 1;
    UINT MenuNameIsString : 1;

#define CLASS_DEFAULT    0x0
#define CLASS_DESKTOP    0x1
#define CLASS_DIALOG     0x2
#define CLASS_POPUPMENU  0x3
#define CLASS_COMBO      0x4
#define CLASS_COMBOLBOX  0x5
#define CLASS_MDICLIENT  0x6
#define CLASS_MENU       0x7
#define CLASS_SCROLL     0x8
#define CLASS_BUTTON     0x9
#define CLASS_LISTBOX    0xA
#define CLASS_EDIT       0xB
#define CLASS_ICONTITLE  0xC
#define CLASS_STATIC     0xD
    UINT ClassId : 4;

} WINDOWCLASS, *PWINDOWCLASS;

typedef struct _WINDOW
{
    USER_OBJHDR hdr; /* FIXME: Move out of the structure once new handle manager is implemented */

    /* NOTE: This structure is located in the desktop heap and will
             eventually replace WINDOW_OBJECT. Right now WINDOW_OBJECT
             keeps a reference to this structure until all the information
             is moved to this structure */
    struct _W32PROCESSINFO *pi; /* FIXME: Move to object header some day */
    struct _W32THREADINFO *ti;
    RECT WindowRect;
    RECT ClientRect;

    WNDPROC WndProc;
    union
    {
        /* Pointer to a call procedure handle */
        PCALLPROC CallProc;
        /* Extra Wnd proc (windows of system classes) */
        WNDPROC WndProcExtra;
    };

    struct _WINDOW *Parent;
    struct _WINDOW *Owner;

    /* Size of the extra data associated with the window. */
    ULONG ExtraDataSize;
    /* Style. */
    DWORD Style;
    /* Extended style. */
    DWORD ExStyle;
    /* Handle of the module that created the window. */
    HINSTANCE Instance;
    /* Window menu handle or window id */
    UINT IDMenu;
    LONG UserData;
    /* Pointer to the window class. */
    PWINDOWCLASS Class;
    /* Window name. */
    UNICODE_STRING WindowName;
    /* Context help id */
    DWORD ContextHelpId;

    struct
    {
        RECT NormalRect;
        POINT IconPos;
        POINT MaxPos;
    } InternalPos;

    UINT Unicode : 1;
    /* Indicates whether the window is derived from a system class */
    UINT IsSystem : 1;
    UINT InternalPosInitialized : 1;
    UINT HideFocus : 1;
    UINT HideAccel : 1;
} WINDOW, *PWINDOW;

typedef struct _SERVERINFO
{
  DWORD SrvEventActivity;

} SERVERINFO, *PSERVERINFO;

typedef struct _W32PROCESSINFO
{
    PVOID UserHandleTable;
    HANDLE hUserHeap;
    ULONG_PTR UserHeapDelta;
    HINSTANCE hModUser;
    PWINDOWCLASS LocalClassList;
    PWINDOWCLASS GlobalClassList;
    PWINDOWCLASS SystemClassList;

    UINT RegisteredSysClasses : 1;

    PSERVERINFO psi;

} W32PROCESSINFO, *PW32PROCESSINFO;

#define CTI_INSENDMESSAGE 0x0002

typedef struct _CLIENTTHREADINFO
{
    DWORD CTI_flags;
    DWORD dwcPumpHook;
} CLIENTTHREADINFO, *PCLIENTTHREADINFO;

typedef struct _W32THREADINFO
{
    PW32PROCESSINFO pi; /* [USER] */
    PW32PROCESSINFO kpi; /* [KERNEL] */
    PDESKTOP Desktop;
    PVOID DesktopHeapBase;
    ULONG_PTR DesktopHeapLimit;
    ULONG_PTR DesktopHeapDelta;
    /* A mask of what hooks are currently active */
    ULONG Hooks;
    /* Application compatibility flags */
    DWORD AppCompatFlags;
    DWORD AppCompatFlags2;
    CLIENTTHREADINFO ClientThreadInfo;
} W32THREADINFO, *PW32THREADINFO;

/* Window Client Information structure */

typedef struct _CALLBACKWND
{
     HWND hWnd;
     PVOID pvWnd;
} CALLBACKWND, *PCALLBACKWND;

typedef struct _W32CLIENTINFO
{
    ULONG CI_flags;
    ULONG cSpins;
    ULONG ulWindowsVersion;
    ULONG ulAppCompatFlags;
    ULONG ulAppCompatFlags2;
    DWORD dwTIFlags;
    PVOID pDeskInfo;
    ULONG_PTR ulClientDelta;
    PVOID phkCurrent;
    ULONG fsHooks;
    HWND  hWND;  // Well be replaced with CALLBACKWND.
    PVOID pvWND; // " "
    ULONG Win32ClientInfo;
    DWORD dwHookCurrent;
    ULONG Win32ClientInfo1;
    PCLIENTTHREADINFO pClientThreadInfo;
    DWORD dwHookData;
    DWORD dwKeyCache;
    ULONG Win32ClientInfo2[7];
    USHORT CodePage;
    USHORT csCF;
    HANDLE hKL;
    ULONG Win32ClientInfo3[35];
} W32CLIENTINFO, *PW32CLIENTINFO;

#define GetWin32ClientInfo() (PW32CLIENTINFO)(NtCurrentTeb()->Win32ClientInfo)

// Server event activity bits.
#define SRV_EVENT_MENU            0x0001
#define SRV_EVENT_END_APPLICATION 0x0002
#define SRV_EVENT_RUNNING         0x0004
#define SRV_EVENT_NAMECHANGE      0x0008
#define SRV_EVENT_VALUECHANGE     0x0010
#define SRV_EVENT_STATECHANGE     0x0020
#define SRV_EVENT_LOCATIONCHANGE  0x0040
#define SRV_EVENT_CREATE          0x8000

PW32THREADINFO GetW32ThreadInfo(VOID);
PW32PROCESSINFO GetW32ProcessInfo(VOID);

DWORD
NTAPI
NtUserAssociateInputContext(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
NTAPI
NtUserBuildHimcList(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4);

DWORD
NTAPI
NtUserCalcMenuBar(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5);

DWORD
NTAPI
NtUserCheckMenuItem(
  HMENU hmenu,
  UINT uIDCheckItem,
  UINT uCheck);

DWORD
NTAPI
NtUserCtxDisplayIOCtl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

BOOL
NTAPI
NtUserDeleteMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags);

BOOL
NTAPI
NtUserDestroyMenu(
  HMENU hMenu);

DWORD
NTAPI
NtUserDrawMenuBarTemp(
  HWND hWnd,
  HDC hDC,
  PRECT hRect,
  HMENU hMenu,
  HFONT hFont);

UINT
NTAPI
NtUserEnableMenuItem(
  HMENU hMenu,
  UINT uIDEnableItem,
  UINT uEnable);

BOOL
NTAPI
NtUserEndMenu(VOID);

BOOL
NTAPI
NtUserGetMenuBarInfo(
  HWND hwnd,
  LONG idObject,
  LONG idItem,
  PMENUBARINFO pmbi);

UINT
NTAPI
NtUserGetMenuIndex(
  HMENU hMenu,
  UINT wID);

BOOL
NTAPI
NtUserGetMenuItemRect(
  HWND hWnd,
  HMENU hMenu,
  UINT uItem,
  LPRECT lprcItem);

HMENU
NTAPI
NtUserGetSystemMenu(
  HWND hWnd,
  BOOL bRevert);

BOOL
NTAPI
NtUserHiliteMenuItem(
  HWND hwnd,
  HMENU hmenu,
  UINT uItemHilite,
  UINT uHilite);

int
NTAPI
NtUserMenuItemFromPoint(
  HWND hWnd,
  HMENU hMenu,
  DWORD X,
  DWORD Y);

BOOL
NTAPI
NtUserRemoveMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags);

BOOL
NTAPI
NtUserSetMenu(
  HWND hWnd,
  HMENU hMenu,
  BOOL bRepaint);

BOOL
NTAPI
NtUserSetMenuContextHelpId(
  HMENU hmenu,
  DWORD dwContextHelpId);

BOOL
NTAPI
NtUserSetMenuDefaultItem(
  HMENU hMenu,
  UINT uItem,
  UINT fByPos);

BOOL
NTAPI
NtUserSetMenuFlagRtoL(
  HMENU hMenu);

BOOL
NTAPI
NtUserSetSystemMenu(
  HWND hWnd,
  HMENU hMenu);

DWORD
NTAPI
NtUserThunkedMenuInfo(
  HMENU hMenu,
  LPCMENUINFO lpcmi);

DWORD
NTAPI
NtUserThunkedMenuItemInfo(
  HMENU hMenu,
  UINT uItem,
  BOOL fByPosition,
  BOOL bInsert,
  LPMENUITEMINFOW lpmii,
  PUNICODE_STRING lpszCaption);

BOOL
NTAPI
NtUserTrackPopupMenuEx(
  HMENU hmenu,
  UINT fuFlags,
  int x,
  int y,
  HWND hwnd,
  LPTPMPARAMS lptpm);

HKL
NTAPI
NtUserActivateKeyboardLayout(
  HKL hKl,
  ULONG Flags);

DWORD
NTAPI
NtUserAlterWindowStyle(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
NTAPI
NtUserAttachThreadInput(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

HDC NTAPI
NtUserBeginPaint(HWND hWnd, PAINTSTRUCT* lPs);

DWORD
NTAPI
NtUserBitBltSysBmp(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6,
  DWORD Unknown7);

BOOL
NTAPI
NtUserBlockInput(
  BOOL BlockIt);

NTSTATUS
NTAPI
NtUserBuildHwndList(
  HDESK hDesktop,
  HWND hwndParent,
  BOOLEAN bChildren,
  ULONG dwThreadId,
  ULONG lParam,
  HWND* pWnd,
  ULONG* pBufSize);

NTSTATUS NTAPI
NtUserBuildNameList(
   HWINSTA hWinSta,
   ULONG dwSize,
   PVOID lpBuffer,
   PULONG pRequiredSize);

NTSTATUS
NTAPI
NtUserBuildPropList(
  HWND hWnd,
  LPVOID Buffer,
  DWORD BufferSize,
  DWORD *Count);

/* apfnSimpleCall indices from Windows XP SP 2 */
/* TODO: Check for differences in Windows 2000, 2003 and 2008 */
#define WIN32K_VERSION NTDDI_WINXPSP2 // FIXME: this should go somewhere else

enum SimpleCallRoutines
{
	NOPARAM_ROUTINE_CREATEMENU,
	NOPARAM_ROUTINE_CREATEMENUPOPUP,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	NOPARAM_ROUTINE_ALLOWFOREGNDACTIVATION,
	NOPARAM_ROUTINE_MSQCLEARWAKEMASK,
	NOPARAM_ROUTINE_CREATESYSTEMTHREADS,
	NOPARAM_ROUTINE_DESTROY_CARET,
#endif
	NOPARAM_ROUTINE_ENABLEPROCWNDGHSTING,
#if (WIN32K_VERSION < NTDDI_VISTA)
	NOPARAM_ROUTINE_MSQCLEARWAKEMASK,
	NOPARAM_ROUTINE_ALLOWFOREGNDACTIVATION,
	NOPARAM_ROUTINE_DESTROY_CARET,
#endif
	NOPARAM_ROUTINE_GETDEVICECHANGEINFO,
	NOPARAM_ROUTINE_GETIMESHOWSTATUS,
	NOPARAM_ROUTINE_GETINPUTDESKTOP,
	NOPARAM_ROUTINE_GETMSESSAGEPOS,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	NOPARAM_ROUTINE_HANDLESYSTHRDCREATFAIL,
#else
	NOPARAM_ROUTINE_GETREMOTEPROCID,
#endif
	NOPARAM_ROUTINE_HIDECURSORNOCAPTURE,
	NOPARAM_ROUTINE_LOADCURSANDICOS,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	NOPARAM_ROUTINE_LOADUSERAPIHOOK,
	NOPARAM_ROUTINE_PREPAREFORLOGOFF, /* 0x0f */
#endif
	NOPARAM_ROUTINE_RELEASECAPTURE,
	NOPARAM_ROUTINE_RESETDBLCLICK,
	NOPARAM_ROUTINE_ZAPACTIVEANDFOUS,
	NOPARAM_ROUTINE_REMOTECONSHDWSTOP,
	NOPARAM_ROUTINE_REMOTEDISCONNECT,
	NOPARAM_ROUTINE_REMOTELOGOFF,
	NOPARAM_ROUTINE_REMOTENTSECURITY,
	NOPARAM_ROUTINE_REMOTESHDWSETUP,
	NOPARAM_ROUTINE_REMOTESHDWSTOP,
	NOPARAM_ROUTINE_REMOTEPASSTHRUENABLE,
	NOPARAM_ROUTINE_REMOTEPASSTHRUDISABLE,
	NOPARAM_ROUTINE_REMOTECONNECTSTATE,
	NOPARAM_ROUTINE_UPDATEPERUSERIMMENABLING,
	NOPARAM_ROUTINE_USERPWRCALLOUTWORKER,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	NOPARAM_ROUTINE_WAKERITFORSHTDWN,
#endif
	NOPARAM_ROUTINE_INIT_MESSAGE_PUMP,
	NOPARAM_ROUTINE_UNINIT_MESSAGE_PUMP,
#if (WIN32K_VERSION < NTDDI_VISTA)
	NOPARAM_ROUTINE_LOADUSERAPIHOOK,
#endif
	ONEPARAM_ROUTINE_BEGINDEFERWNDPOS,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	ONEPARAM_ROUTINE_GETSENDMSGRECVR,
#endif
	ONEPARAM_ROUTINE_WINDOWFROMDC,
	ONEPARAM_ROUTINE_ALLOWSETFOREGND,
	ONEPARAM_ROUTINE_CREATEEMPTYCUROBJECT,
#if (WIN32K_VERSION < NTDDI_VISTA)
	ONEPARAM_ROUTINE_CREATESYSTEMTHREADS,
#endif
	ONEPARAM_ROUTINE_CSDDEUNINITIALIZE,
	ONEPARAM_ROUTINE_DIRECTEDYIELD,
	ONEPARAM_ROUTINE_ENUMCLIPBOARDFORMATS,
#if (WIN32K_VERSION < NTDDI_VISTA)
	ONEPARAM_ROUTINE_GETCURSORPOS,
#endif
	ONEPARAM_ROUTINE_GETINPUTEVENT,
	ONEPARAM_ROUTINE_GETKEYBOARDLAYOUT,
	ONEPARAM_ROUTINE_GETKEYBOARDTYPE,
	ONEPARAM_ROUTINE_GETPROCDEFLAYOUT,
	ONEPARAM_ROUTINE_GETQUEUESTATUS,
	ONEPARAM_ROUTINE_GETWINSTAINFO,
#if (WIN32K_VERSION < NTDDI_VISTA)
	ONEPARAM_ROUTINE_HANDLESYSTHRDCREATFAIL,
#endif
	ONEPARAM_ROUTINE_LOCKFOREGNDWINDOW,
	ONEPARAM_ROUTINE_LOADFONTS,
	ONEPARAM_ROUTINE_MAPDEKTOPOBJECT,
	ONEPARAM_ROUTINE_MESSAGEBEEP,
	ONEPARAM_ROUTINE_PLAYEVENTSOUND,
	ONEPARAM_ROUTINE_POSTQUITMESSAGE,
#if (WIN32K_VERSION < NTDDI_VISTA)
	ONEPARAM_ROUTINE_PREPAREFORLOGOFF,
#endif
	ONEPARAM_ROUTINE_REALIZEPALETTE,
	ONEPARAM_ROUTINE_REGISTERLPK,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	ONEPARAM_ROUTINE_REGISTERSYSTEMTHREAD,
#endif
	ONEPARAM_ROUTINE_REMOTERECONNECT,
	ONEPARAM_ROUTINE_REMOTETHINWIRESTATUS,
	ONEPARAM_ROUTINE_RELEASEDC,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	ONEPARAM_ROUTINE_REMOTENOTIFY,
#endif
	ONEPARAM_ROUTINE_REPLYMESSAGE,
	ONEPARAM_ROUTINE_SETCARETBLINKTIME,
	ONEPARAM_ROUTINE_SETDBLCLICKTIME,
#if (WIN32K_VERSION < NTDDI_VISTA)
	ONEPARAM_ROUTINE_SETIMESHOWSTATUS,
#endif
	ONEPARAM_ROUTINE_SETMESSAGEEXTRAINFO,
	ONEPARAM_ROUTINE_SETPROCDEFLAYOUT,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	ONEPARAM_ROUTINE_SETWATERMARKSTRINGS,
#endif
	ONEPARAM_ROUTINE_SHOWCURSOR,
	ONEPARAM_ROUTINE_SHOWSTARTGLASS,
	ONEPARAM_ROUTINE_SWAPMOUSEBUTTON,
	X_ROUTINE_WOWMODULEUNLOAD,
#if (WIN32K_VERSION < NTDDI_VISTA)
	X_ROUTINE_REMOTENOTIFY,
#endif
	HWND_ROUTINE_DEREGISTERSHELLHOOKWINDOW,
	HWND_ROUTINE_DWP_GETENABLEDPOPUP,
	HWND_ROUTINE_GETWNDCONTEXTHLPID,
	HWND_ROUTINE_REGISTERSHELLHOOKWINDOW,
	HWND_ROUTINE_SETMSGBOX,
	HWNDOPT_ROUTINE_SETPROGMANWINDOW,
	HWNDOPT_ROUTINE_SETTASKMANWINDOW,
	HWNDPARAM_ROUTINE_GETCLASSICOCUR,
	HWNDPARAM_ROUTINE_CLEARWINDOWSTATE,
	HWNDPARAM_ROUTINE_KILLSYSTEMTIMER,
	HWNDPARAM_ROUTINE_SETDIALOGPOINTER,
	HWNDPARAM_ROUTINE_SETVISIBLE,
	HWNDPARAM_ROUTINE_SETWNDCONTEXTHLPID,
	HWNDPARAM_ROUTINE_SETWINDOWSTATE,
	HWNDLOCK_ROUTINE_WINDOWHASSHADOW, /* correct prefix ? */
	HWNDLOCK_ROUTINE_ARRANGEICONICWINDOWS,
	HWNDLOCK_ROUTINE_DRAWMENUBAR,
	HWNDLOCK_ROUTINE_CHECKIMESHOWSTATUSINTHRD,
	HWNDLOCK_ROUTINE_GETSYSMENUHANDLE,
	HWNDLOCK_ROUTINE_REDRAWFRAME,
	HWNDLOCK_ROUTINE_REDRAWFRAMEANDHOOK,
	HWNDLOCK_ROUTINE_SETDLGSYSMENU,
	HWNDLOCK_ROUTINE_SETFOREGROUNDWINDOW,
	HWNDLOCK_ROUTINE_SETSYSMENU,
	HWNDLOCK_ROUTINE_UPDATECKIENTRECT,
	HWNDLOCK_ROUTINE_UPDATEWINDOW,
	X_ROUTINE_IMESHOWSTATUSCHANGE,
	TWOPARAM_ROUTINE_ENABLEWINDOW,
	TWOPARAM_ROUTINE_REDRAWTITLE,
	TWOPARAM_ROUTINE_SHOWOWNEDPOPUPS,
	TWOPARAM_ROUTINE_SWITCHTOTHISWINDOW,
	TWOPARAM_ROUTINE_UPDATEWINDOWS,
	TWOPARAM_ROUTINE_VALIDATERGN,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	TWOPARAM_ROUTINE_CHANGEWNDMSGFILTER,
	TWOPARAM_ROUTINE_GETCURSORPOS,
#endif
	TWOPARAM_ROUTINE_GETHDEVNAME,
	TWOPARAM_ROUTINE_INITANSIOEM,
	TWOPARAM_ROUTINE_NLSSENDIMENOTIFY,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	TWOPARAM_ROUTINE_REGISTERGHSTWND,
#endif
	TWOPARAM_ROUTINE_REGISTERLOGONPROCESS,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	TWOPARAM_ROUTINE_REGISTERSBLFROSTWND,
#else
	TWOPARAM_ROUTINE_REGISTERSYSTEMTHREAD,
#endif
	TWOPARAM_ROUTINE_REGISTERUSERHUNGAPPHANDLERS,
	TWOPARAM_ROUTINE_SHADOWCLEANUP,
	TWOPARAM_ROUTINE_REMOTESHADOWSTART,
	TWOPARAM_ROUTINE_SETCARETPOS,
	TWOPARAM_ROUTINE_SETCURSORPOS,
#if (WIN32K_VERSION >= NTDDI_VISTA)
	TWOPARAM_ROUTINE_SETPHYSCURSORPOS,
#endif
	TWOPARAM_ROUTINE_UNHOOKWINDOWSHOOK,
	TWOPARAM_ROUTINE_WOWCLEANUP
};

DWORD
NTAPI
NtUserCallHwnd(
  HWND hWnd,
  DWORD Routine);

BOOL
NTAPI
NtUserCallHwndLock(
  HWND hWnd,
  DWORD Routine);

HWND
NTAPI
NtUserCallHwndOpt(
  HWND hWnd,
  DWORD Routine);

DWORD
NTAPI
NtUserCallHwndParam(
  HWND hWnd,
  DWORD Param,
  DWORD Routine);

DWORD
NTAPI
NtUserCallHwndParamLock(
  HWND hWnd,
  DWORD Param,
  DWORD Routine);

BOOL
NTAPI
NtUserCallMsgFilter(
  LPMSG msg,
  INT code);

LRESULT
NTAPI
NtUserCallNextHookEx(
  HHOOK Hook,
  int Code,
  WPARAM wParam,
  LPARAM lParam);

DWORD
NTAPI
NtUserCallNoParam(
  DWORD Routine);

DWORD
NTAPI
NtUserCallOneParam(
  DWORD Param,
  DWORD Routine);

DWORD
NTAPI
NtUserCallTwoParam(
  DWORD Param1,
  DWORD Param2,
  DWORD Routine);

BOOL
NTAPI
NtUserChangeClipboardChain(
  HWND hWndRemove,
  HWND hWndNewNext);

LONG
NTAPI
NtUserChangeDisplaySettings(
  PUNICODE_STRING lpszDeviceName,
  LPDEVMODEW lpDevMode,
  HWND hwnd,
  DWORD dwflags,
  LPVOID lParam);

DWORD
STDCALL
NtUserCheckImeHotKey(
  DWORD dwUnknown1,
  DWORD dwUnknown2);

HWND NTAPI
NtUserChildWindowFromPointEx(
  HWND Parent,
  LONG x,
  LONG y,
  UINT Flags);

BOOL
NTAPI
NtUserClipCursor(
    RECT *lpRect);

BOOL
NTAPI
NtUserCloseClipboard(VOID);

BOOL
NTAPI
NtUserCloseDesktop(
  HDESK hDesktop);

BOOL
NTAPI
NtUserCloseWindowStation(
  HWINSTA hWinSta);

DWORD
NTAPI
NtUserConsoleControl(
  DWORD dwUnknown1,
  DWORD dwUnknown2,
  DWORD dwUnknown3);

DWORD
NTAPI
NtUserConvertMemHandle(
  DWORD Unknown0,
  DWORD Unknown1);

int
NTAPI
NtUserCopyAcceleratorTable(
  HACCEL Table,
  LPACCEL Entries,
  int EntriesCount);

DWORD
NTAPI
NtUserCountClipboardFormats(VOID);

HACCEL
NTAPI
NtUserCreateAcceleratorTable(
  LPACCEL Entries,
  SIZE_T EntriesCount);

BOOL
NTAPI
NtUserCreateCaret(
  HWND hWnd,
  HBITMAP hBitmap,
  int nWidth,
  int nHeight);

HDESK
NTAPI
NtUserCreateDesktop(
  PUNICODE_STRING lpszDesktopName,
  DWORD dwFlags,
  ACCESS_MASK dwDesiredAccess,
  LPSECURITY_ATTRIBUTES lpSecurity,
  HWINSTA hWindowStation);

DWORD
NTAPI
NtUserCreateInputContext(
    DWORD dwUnknown1);

DWORD
NTAPI
NtUserCreateLocalMemHandle(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

HWND
NTAPI
NtUserCreateWindowEx(
  DWORD dwExStyle,
  PUNICODE_STRING lpClassName,
  PUNICODE_STRING lpWindowName,
  DWORD dwStyle,
  LONG x,
  LONG y,
  LONG nWidth,
  LONG nHeight,
  HWND hWndParent,
  HMENU hMenu,
  HINSTANCE hInstance,
  LPVOID lpParam,
  DWORD dwShowMode,
  BOOL bUnicodeWindow,
  DWORD dwUnknown);

HWINSTA
NTAPI
NtUserCreateWindowStation(
  PUNICODE_STRING lpszWindowStationName,
  ACCESS_MASK dwDesiredAccess,
  LPSECURITY_ATTRIBUTES lpSecurity,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6);

DWORD
NTAPI
NtUserDdeGetQualityOfService(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
NTAPI
NtUserDdeInitialize(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4);

DWORD
NTAPI
NtUserDdeSetQualityOfService(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

HDWP NTAPI
NtUserDeferWindowPos(HDWP WinPosInfo,
		     HWND Wnd,
		     HWND WndInsertAfter,
		     int x,
         int y,
         int cx,
         int cy,
		     UINT Flags);
BOOL NTAPI
NtUserDefSetText(HWND WindowHandle, PUNICODE_STRING WindowText);

BOOLEAN
NTAPI
NtUserDestroyAcceleratorTable(
  HACCEL Table);

BOOL
NTAPI
NtUserDestroyCursor(
  HANDLE Handle,
  DWORD Unknown);

DWORD
NTAPI
NtUserDestroyInputContext(
    DWORD dwUnknown1);

BOOLEAN NTAPI
NtUserDestroyWindow(HWND Wnd);

DWORD
NTAPI
NtUserDisableThreadIme(
    DWORD dwUnknown1);

typedef struct tagNTUSERDISPATCHMESSAGEINFO
{
  BOOL HandledByKernel;
  BOOL Ansi;
  WNDPROC Proc;
  MSG Msg;
} NTUSERDISPATCHMESSAGEINFO, *PNTUSERDISPATCHMESSAGEINFO;

LRESULT
NTAPI
NtUserDispatchMessage(PNTUSERDISPATCHMESSAGEINFO MsgInfo);

BOOL
NTAPI
NtUserDragDetect(
  HWND hWnd,
  LONG x,
  LONG y);

DWORD
NTAPI
NtUserDragObject(
   HWND    hwnd1,
   HWND    hwnd2,
   UINT    u1,
   DWORD   dw1,
   HCURSOR hc1);

DWORD
NTAPI
NtUserDrawAnimatedRects(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

BOOL
NTAPI
NtUserDrawCaption(
   HWND hWnd,
   HDC hDc,
   LPCRECT lpRc,
   UINT uFlags);

BOOL
STDCALL
NtUserDrawCaptionTemp(
  HWND hWnd,
  HDC hDC,
  LPCRECT lpRc,
  HFONT hFont,
  HICON hIcon,
  const PUNICODE_STRING str,
  UINT uFlags);

BOOL
NTAPI
NtUserDrawIconEx(
  HDC hdc,
  int xLeft,
  int yTop,
  HICON hIcon,
  int cxWidth,
  int cyWidth,
  UINT istepIfAniCur,
  HBRUSH hbrFlickerFreeDraw,
  UINT diFlags,
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
NTAPI
NtUserEmptyClipboard(VOID);

BOOL
NTAPI
NtUserEnableScrollBar(
  HWND hWnd,
  UINT wSBflags,
  UINT wArrows);

DWORD
NTAPI
NtUserEndDeferWindowPosEx(
  DWORD Unknown0,
  DWORD Unknown1);

BOOL NTAPI
NtUserEndPaint(HWND hWnd, CONST PAINTSTRUCT* lPs);

BOOL
NTAPI
NtUserEnumDisplayDevices (
  PUNICODE_STRING lpDevice, /* device name */
  DWORD iDevNum, /* display device */
  PDISPLAY_DEVICEW lpDisplayDevice, /* device information */
  DWORD dwFlags ); /* reserved */

/*BOOL
NTAPI
NtUserEnumDisplayMonitors (
  HDC hdc,
  LPCRECT lprcClip,
  MONITORENUMPROC lpfnEnum,
  LPARAM dwData );*/
/* FIXME:  The call below is ros-specific and should be rewritten to use the same params as the correct call above.  */
INT
NTAPI
NtUserEnumDisplayMonitors(
  OPTIONAL IN HDC hDC,
  OPTIONAL IN LPCRECT pRect,
  OPTIONAL OUT HMONITOR *hMonitorList,
  OPTIONAL OUT LPRECT monitorRectList,
  OPTIONAL IN DWORD listSize );


NTSTATUS
NTAPI
NtUserEnumDisplaySettings(
  PUNICODE_STRING lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODEW lpDevMode, /* FIXME is this correct? */
  DWORD dwFlags );

DWORD
NTAPI
NtUserEvent(
  DWORD Unknown0);

DWORD
NTAPI
NtUserExcludeUpdateRgn(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
NTAPI
NtUserFillWindow(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

HICON
NTAPI
NtUserFindExistingCursorIcon(
  HMODULE hModule,
  HRSRC hRsrc,
  LONG cx,
  LONG cy);

HWND
NTAPI
NtUserFindWindowEx(
  HWND  hwndParent,
  HWND  hwndChildAfter,
  PUNICODE_STRING  ucClassName,
  PUNICODE_STRING  ucWindowName,
  DWORD dwUnknown
  );

DWORD
NTAPI
NtUserFlashWindowEx(
  DWORD Unknown0);

DWORD
NTAPI
NtUserGetAltTabInfo(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5);

HWND NTAPI
NtUserGetAncestor(HWND hWnd, UINT Flags);

DWORD
NTAPI
NtUserGetAppImeLevel(
    DWORD dwUnknown1);

DWORD
NTAPI
NtUserGetAsyncKeyState(
  DWORD Unknown0);

DWORD
NTAPI
NtUserGetAtomName(
    DWORD dwUnknown1,
    DWORD dwUnknown2);

UINT
NTAPI
NtUserGetCaretBlinkTime(VOID);

BOOL
NTAPI
NtUserGetCaretPos(
  LPPOINT lpPoint);

BOOL NTAPI
NtUserGetClassInfo(HINSTANCE hInstance,
		   PUNICODE_STRING ClassName,
		   LPWNDCLASSEXW wcex,
		   LPWSTR *ppszMenuName,
		   BOOL Ansi);

INT
NTAPI
NtUserGetClassName(HWND hWnd,
		   PUNICODE_STRING ClassName,
                   BOOL Ansi);
#if 0 // Real NtUserGetClassName
INT
NTAPI
NtUserGetClassName(HWND hWnd,
                   BOOL Unknown, // 0 GetClassNameW, 1 RealGetWindowClassA/W
                   PUNICODE_STRING ClassName);
#endif

HANDLE
NTAPI
NtUserGetClipboardData(
  UINT uFormat,
  PVOID pBuffer);

INT
NTAPI
NtUserGetClipboardFormatName(
  UINT format,
  PUNICODE_STRING FormatName,
  INT cchMaxCount);

HWND
NTAPI
NtUserGetClipboardOwner(VOID);

DWORD
NTAPI
NtUserGetClipboardSequenceNumber(VOID);

HWND
NTAPI
NtUserGetClipboardViewer(VOID);

BOOL
NTAPI
NtUserGetClipCursor(
  RECT *lpRect);

BOOL
NTAPI
NtUserGetComboBoxInfo(
  HWND hWnd,
  PCOMBOBOXINFO pcbi);

DWORD
NTAPI
NtUserGetControlBrush(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
NTAPI
NtUserGetControlColor(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
NTAPI
NtUserGetCPD(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
NTAPI
NtUserGetCursorFrameInfo(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

BOOL
NTAPI
NtUserGetCursorInfo(
  PCURSORINFO pci);

HDC
NTAPI
NtUserGetDC(
  HWND hWnd);

HDC
NTAPI
NtUserGetDCEx(
  HWND hWnd,
  HANDLE hRegion,
  ULONG Flags);

UINT
NTAPI
NtUserGetDoubleClickTime(VOID);

HWND
NTAPI
NtUserGetForegroundWindow(VOID);

DWORD
NTAPI
NtUserGetGuiResources(
  HANDLE hProcess,
  DWORD uiFlags);

BOOL
NTAPI
NtUserGetGUIThreadInfo(
  DWORD idThread,
  LPGUITHREADINFO lpgui);

BOOL
NTAPI
NtUserGetIconInfo(
   HANDLE hCurIcon,
   PICONINFO IconInfo,
   PUNICODE_STRING lpInstName,
   PUNICODE_STRING lpResName,
   LPDWORD pbpp,
   BOOL bInternal);

BOOL
NTAPI
NtUserGetIconSize(
    HANDLE Handle,
    UINT istepIfAniCur,
    LONG  *plcx,
    LONG  *plcy);

DWORD
NTAPI
NtUserGetImeHotKey(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
NTAPI
NtUserGetImeInfoEx(
    DWORD dwUnknown1,
    DWORD dwUnknown2);

DWORD
NTAPI
NtUserGetInternalWindowPos(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

HKL
NTAPI
NtUserGetKeyboardLayout(
  DWORD dwThreadid);

UINT
NTAPI
NtUserGetKeyboardLayoutList(
  INT nItems,
  HKL *pHklBuff);

BOOL
NTAPI
NtUserGetKeyboardLayoutName(
  LPWSTR lpszName);

DWORD
NTAPI
NtUserGetKeyboardState(
  LPBYTE Unknown0);

DWORD
NTAPI
NtUserGetKeyboardType(
  DWORD TypeFlag);

DWORD
NTAPI
NtUserGetKeyNameText( LONG lParam, LPWSTR lpString, int nSize );

DWORD
NTAPI
NtUserGetKeyState(
  DWORD Unknown0);

DWORD
NTAPI
NtUserGetLayeredWindowAttributes(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4);

DWORD
NTAPI
NtUserGetListBoxInfo(
  HWND hWnd);

typedef struct tagNTUSERGETMESSAGEINFO
{
  MSG Msg;
  ULONG LParamSize;
} NTUSERGETMESSAGEINFO, *PNTUSERGETMESSAGEINFO;

BOOL
NTAPI
NtUserGetMessage(
  PNTUSERGETMESSAGEINFO MsgInfo,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax);

DWORD
NTAPI
NtUserGetMouseMovePointsEx(
  UINT cbSize,
  LPMOUSEMOVEPOINT lppt,
  LPMOUSEMOVEPOINT lpptBuf,
  int nBufPoints,
  DWORD resolution);

BOOL
NTAPI
NtUserGetObjectInformation(
  HANDLE hObject,
  DWORD nIndex,
  PVOID pvInformation,
  DWORD nLength,
  PDWORD nLengthNeeded);

HWND
NTAPI
NtUserGetOpenClipboardWindow(VOID);

INT
NTAPI
NtUserGetPriorityClipboardFormat(
  UINT *paFormatPriorityList,
  INT cFormats);

HWINSTA
NTAPI
NtUserGetProcessWindowStation(VOID);

DWORD
NTAPI
NtUserGetRawInputBuffer(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
NTAPI
NtUserGetRawInputData(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5);

DWORD
NTAPI
NtUserGetRawInputDeviceInfo(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4);

DWORD
NTAPI
NtUserGetRawInputDeviceList(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
NTAPI
NtUserGetRegisteredRawInputDevices(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

BOOL
NTAPI
NtUserGetScrollBarInfo(
  HWND hWnd,
  LONG idObject,
  PSCROLLBARINFO psbi);

HDESK
NTAPI
NtUserGetThreadDesktop(
  DWORD dwThreadId,
  DWORD Unknown1);


enum ThreadStateRoutines
{
    THREADSTATE_GETTHREADINFO,
    THREADSTATE_INSENDMESSAGE,
    THREADSTATE_FOCUSWINDOW,
    THREADSTATE_ACTIVEWINDOW,
    THREADSTATE_CAPTUREWINDOW,
    THREADSTATE_PROGMANWINDOW,
    THREADSTATE_TASKMANWINDOW
};

DWORD
NTAPI
NtUserGetThreadState(
  DWORD Routine);

DWORD
NTAPI
NtUserGetTitleBarInfo(
  HWND hwnd,
  PTITLEBARINFO pti);

BOOL NTAPI
NtUserGetUpdateRect(HWND hWnd, LPRECT lpRect, BOOL fErase);

int
NTAPI
NtUserGetUpdateRgn(
  HWND hWnd,
  HRGN hRgn,
  BOOL bErase);

HDC
NTAPI
NtUserGetWindowDC(
  HWND hWnd);

BOOL
NTAPI
NtUserGetWindowPlacement(
  HWND hWnd,
  WINDOWPLACEMENT *lpwndpl);

DWORD
NTAPI
NtUserGetWOWClass(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
NTAPI
NtUserHardErrorControl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
NTAPI
NtUserImpersonateDdeClientWindow(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
NTAPI
NtUserInitialize(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
NTAPI
NtUserInitializeClientPfnArrays(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
NTAPI
NtUserInitTask(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6,
  DWORD Unknown7,
  DWORD Unknown8,
  DWORD Unknown9,
  DWORD Unknown10,
  DWORD Unknown11);

INT
NTAPI
NtUserInternalGetWindowText(
  HWND hWnd,
  LPWSTR lpString,
  INT nMaxCount);

BOOL
NTAPI
NtUserInvalidateRect(
    HWND hWnd,
    CONST RECT *lpRect,
    BOOL bErase);

BOOL
NTAPI
NtUserInvalidateRgn(
    HWND hWnd,
    HRGN hRgn,
    BOOL bErase);

BOOL
NTAPI
NtUserIsClipboardFormatAvailable(
  UINT format);

BOOL
NTAPI
NtUserKillTimer
(
 HWND hWnd,
 UINT_PTR uIDEvent
);

HKL
STDCALL
NtUserLoadKeyboardLayoutEx(
   IN HANDLE Handle,
   IN DWORD offTable,
   IN PUNICODE_STRING puszKeyboardName,
   IN HKL hKL,
   IN PUNICODE_STRING puszKLID,
   IN DWORD dwKLID,
   IN UINT Flags);

BOOL
NTAPI
NtUserLockWindowStation(
  HWINSTA hWindowStation);

DWORD
NTAPI
NtUserLockWindowUpdate(
  DWORD Unknown0);

DWORD
NTAPI
NtUserLockWorkStation(VOID);

UINT
NTAPI
NtUserMapVirtualKeyEx( UINT keyCode,
		       UINT transType,
		       DWORD keyboardId,
		       HKL dwhkl );

#define NUMC_SENDMESSAGE            0x02B0
// Kernel has option to use TO or normal msg send, based on type of msg.
#define NUMC_SENDMESSAGEWTOOPTION   0x02B1
#define NUMC_SENDMESSAGETIMEOUT     0x02B2
#define NUMC_BROADCASTSYSTEMMESSAGE 0x02B4
#define NUMC_SENDNOTIFYMESSAGE      0x02B7
#define NUMC_SENDMESSAGECALLBACK    0x02B8
LRESULT
NTAPI
NtUserMessageCall(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  ULONG_PTR ResultInfo,
  DWORD dwType, // NUMC_XX types
  BOOL Ansi);

DWORD
NTAPI
NtUserMinMaximize(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
NTAPI
NtUserMNDragLeave(VOID);

DWORD
NTAPI
NtUserMNDragOver(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
NTAPI
NtUserModifyUserStartupInfoFlags(
  DWORD Unknown0,
  DWORD Unknown1);

BOOL
NTAPI
NtUserMoveWindow(
    HWND hWnd,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    BOOL bRepaint
);

DWORD
NTAPI
NtUserNotifyIMEStatus(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
NTAPI
NtUserNotifyProcessCreate(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4);

VOID
NTAPI
NtUserNotifyWinEvent(
  DWORD Event,
  HWND  hWnd,
  LONG  idObject,
  LONG  idChild);

BOOL
NTAPI
NtUserOpenClipboard(
  HWND hWnd,
  DWORD Unknown1);

HDESK
NTAPI
NtUserOpenDesktop(
  PUNICODE_STRING lpszDesktopName,
  DWORD dwFlags,
  ACCESS_MASK dwDesiredAccess);

HDESK
NTAPI
NtUserOpenInputDesktop(
  DWORD dwFlags,
  BOOL fInherit,
  ACCESS_MASK dwDesiredAccess);

HWINSTA
NTAPI
NtUserOpenWindowStation(
  PUNICODE_STRING lpszWindowStationName,
  ACCESS_MASK dwDesiredAccess);

BOOL
NTAPI
NtUserPaintDesktop(
  HDC hDC);

DWORD
NTAPI
NtUserPaintMenuBar(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5,
    DWORD dwUnknown6);

BOOL
NTAPI
NtUserPeekMessage(
  PNTUSERGETMESSAGEINFO MsgInfo,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax,
  UINT wRemoveMsg);

BOOL
NTAPI
NtUserPostMessage(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam);

BOOL
NTAPI
NtUserPostThreadMessage(
  DWORD idThread,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam);

DWORD
NTAPI
NtUserPrintWindow(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
NTAPI
NtUserProcessConnect(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
NTAPI
NtUserQueryInformationThread(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5);

DWORD
NTAPI
NtUserQueryInputContext(
    DWORD dwUnknown1,
    DWORD dwUnknown2);

DWORD
NTAPI
NtUserQuerySendMessage(
  DWORD Unknown0);

DWORD
NTAPI
NtUserQueryUserCounters(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4);

#define QUERY_WINDOW_UNIQUE_PROCESS_ID	0x00
#define QUERY_WINDOW_UNIQUE_THREAD_ID	0x01
#define QUERY_WINDOW_ISHUNG	0x04
DWORD
NTAPI
NtUserQueryWindow(
  HWND hWnd,
  DWORD Index);

DWORD
NTAPI
NtUserRealInternalGetMessage(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5,
    DWORD dwUnknown6);

DWORD
NTAPI
NtUserRealChildWindowFromPoint(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
NTAPI
NtUserRealWaitMessageEx(
    DWORD dwUnknown1,
    DWORD dwUnknown2);

BOOL
NTAPI
NtUserRedrawWindow
(
 HWND hWnd,
 CONST RECT *lprcUpdate,
 HRGN hrgnUpdate,
 UINT flags
);

HWINSTA
NTAPI
NtUserRegisterClassExWOW(
    CONST WNDCLASSEXW* lpwcx,
    BOOL bUnicodeClass,
    WNDPROC wpExtra,
    DWORD dwUnknown4,
    DWORD dwUnknown5,
    DWORD dwUnknown6,
    DWORD dwUnknown7);

DWORD
NTAPI
NtUserRegisterRawInputDevices(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
NTAPI
NtUserRegisterUserApiHook(
    DWORD dwUnknown1,
    DWORD dwUnknown2);

BOOL
NTAPI
NtUserRegisterHotKey(HWND hWnd,
		     int id,
		     UINT fsModifiers,
		     UINT vk);

DWORD
NTAPI
NtUserRegisterTasklist(
  DWORD Unknown0);

UINT NTAPI
NtUserRegisterWindowMessage(PUNICODE_STRING MessageName);

DWORD
NTAPI
NtUserRemoteConnect(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
NTAPI
NtUserRemoteRedrawRectangle(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4);

DWORD
NTAPI
NtUserRemoteRedrawScreen(VOID);

DWORD
NTAPI
NtUserRemoteStopScreenUpdates(VOID);

HANDLE NTAPI
NtUserRemoveProp(HWND hWnd, ATOM Atom);

DWORD
NTAPI
NtUserResolveDesktop(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4);

DWORD
NTAPI
NtUserResolveDesktopForWOW(
  DWORD Unknown0);

DWORD
NTAPI
NtUserSBGetParms(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

BOOL
NTAPI
NtUserScrollDC(
  HDC hDC,
  int dx,
  int dy,
  CONST RECT *lprcScroll,
  CONST RECT *lprcClip ,
  HRGN hrgnUpdate,
  LPRECT lprcUpdate);

DWORD NTAPI
NtUserScrollWindowEx(HWND hWnd, INT dx, INT dy, const RECT *rect,
   const RECT *clipRect, HRGN hrgnUpdate, LPRECT rcUpdate, UINT flags);

UINT
NTAPI
NtUserSendInput(
  UINT nInputs,
  LPINPUT pInput,
  INT cbSize);

HWND NTAPI
NtUserSetActiveWindow(HWND Wnd);

DWORD
NTAPI
NtUserSetAppImeLevel(
    DWORD dwUnknown1,
    DWORD dwUnknown2);

HWND NTAPI
NtUserSetCapture(HWND Wnd);

ULONG_PTR NTAPI
NtUserSetClassLong(
  HWND  hWnd,
  INT Offset,
  ULONG_PTR  dwNewLong,
  BOOL  Ansi );

DWORD
NTAPI
NtUserSetClassWord(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

HANDLE
NTAPI
NtUserSetClipboardData(
  UINT uFormat,
  HANDLE hMem,
  DWORD Unknown2);

HWND
NTAPI
NtUserSetClipboardViewer(
  HWND hWndNewViewer);

HPALETTE
STDCALL
NtUserSelectPalette(
    HDC hDC,
    HPALETTE  hpal,
    BOOL  ForceBackground
);

DWORD
NTAPI
NtUserSetConsoleReserveKeys(
  DWORD Unknown0,
  DWORD Unknown1);

HCURSOR
NTAPI
NtUserSetCursor(
  HCURSOR hCursor);

BOOL
NTAPI
NtUserSetCursorContents(
  HANDLE Handle,
  PICONINFO IconInfo);

BOOL
NTAPI
NtUserSetCursorIconData(
  HANDLE Handle,
  PBOOL fIcon,
  POINT *Hotspot,
  HMODULE hModule,
  HRSRC hRsrc,
  HRSRC hGroupRsrc);

DWORD
NTAPI
NtUserSetDbgTag(
  DWORD Unknown0,
  DWORD Unknown1);

HWND
NTAPI
NtUserSetFocus(
  HWND hWnd);

DWORD
NTAPI
NtUserSetImeHotKey(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4);

DWORD
NTAPI
NtUserSetImeInfoEx(
    DWORD dwUnknown1);

DWORD
NTAPI
NtUserSetImeOwnerWindow(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
NTAPI
NtUserSetInformationProcess(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4);

DWORD
NTAPI
NtUserSetInformationThread(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4);

DWORD
NTAPI
NtUserSetInternalWindowPos(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
NTAPI
NtUserSetKeyboardState(
  LPBYTE Unknown0);

DWORD
NTAPI
NtUserSetLayeredWindowAttributes(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

DWORD
NTAPI
NtUserSetLogonNotifyWindow(
  DWORD Unknown0);

BOOL
NTAPI
NtUserSetObjectInformation(
  HANDLE hObject,
  DWORD nIndex,
  PVOID pvInformation,
  DWORD nLength);

HWND
NTAPI
NtUserSetParent(
  HWND hWndChild,
  HWND hWndNewParent);

BOOL
NTAPI
NtUserSetProcessWindowStation(
  HWINSTA hWindowStation);

BOOL NTAPI
NtUserSetProp(HWND hWnd, ATOM Atom, HANDLE Data);

DWORD
NTAPI
NtUserSetRipFlags(
  DWORD Unknown0,
  DWORD Unknown1);

DWORD
NTAPI
NtUserSetScrollInfo(
  HWND hwnd,
  int fnBar,
  LPCSCROLLINFO lpsi,
  BOOL bRedraw);

BOOL
NTAPI
NtUserSetShellWindowEx(
  HWND hwndShell,
  HWND hwndShellListView);

DWORD
NTAPI
NtUserSetSysColors(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

BOOL
NTAPI
NtUserSetSystemCursor(
  HCURSOR hcur,
  DWORD id);

BOOL
NTAPI
NtUserSetThreadDesktop(
  HDESK hDesktop);

DWORD
NTAPI
NtUserSetThreadState(
  DWORD Unknown0,
  DWORD Unknown1);

UINT_PTR
NTAPI
NtUserSetSystemTimer
(
 HWND hWnd,
 UINT_PTR nIDEvent,
 UINT uElapse,
 TIMERPROC lpTimerFunc
);

DWORD
NTAPI
NtUserSetThreadLayoutHandles(
    DWORD dwUnknown1,
    DWORD dwUnknown2);

UINT_PTR
NTAPI
NtUserSetTimer
(
 HWND hWnd,
 UINT_PTR nIDEvent,
 UINT uElapse,
 TIMERPROC lpTimerFunc
);

DWORD
NTAPI
NtUserSetWindowFNID(
  DWORD Unknown0,
  DWORD Unknown1);

LONG
NTAPI
NtUserSetWindowLong(
  HWND hWnd,
  DWORD Index,
  LONG NewValue,
  BOOL Ansi);

BOOL
NTAPI
NtUserSetWindowPlacement(
  HWND hWnd,
  WINDOWPLACEMENT *lpwndpl);

BOOL
NTAPI NtUserSetWindowPos(
    HWND hWnd,
    HWND hWndInsertAfter,
    int X,
    int Y,
    int cx,
    int cy,
    UINT uFlags
);

INT
NTAPI
NtUserSetWindowRgn(
  HWND hWnd,
  HRGN hRgn,
  BOOL bRedraw);

HHOOK
NTAPI
NtUserSetWindowsHookAW(
  int idHook,
  HOOKPROC lpfn,
  BOOL Ansi);

HHOOK
NTAPI
NtUserSetWindowsHookEx(
  HINSTANCE Mod,
  PUNICODE_STRING ModuleName,
  DWORD ThreadId,
  int HookId,
  HOOKPROC HookProc,
  BOOL Ansi);

DWORD
NTAPI
NtUserSetWindowStationUser(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3);

WORD NTAPI
NtUserSetWindowWord(HWND hWnd, INT Index, WORD NewVal);

HWINEVENTHOOK
NTAPI
NtUserSetWinEventHook(
  UINT eventMin,
  UINT eventMax,
  HMODULE hmodWinEventProc,
  PUNICODE_STRING puString,
  WINEVENTPROC lpfnWinEventProc,
  DWORD idProcess,
  DWORD idThread,
  UINT dwflags);

BOOL
NTAPI
NtUserShowCaret(
  HWND hWnd);

BOOL
NTAPI
NtUserHideCaret(
  HWND hWnd);

DWORD
NTAPI
NtUserShowScrollBar(HWND hWnd, int wBar, DWORD bShow);

BOOL
NTAPI
NtUserShowWindow(
  HWND hWnd,
  LONG nCmdShow);

BOOL
NTAPI
NtUserShowWindowAsync(
  HWND hWnd,
  LONG nCmdShow);

DWORD
NTAPI
NtUserSoundSentry(VOID);

BOOL
NTAPI
NtUserSwitchDesktop(
  HDESK hDesktop);

BOOL
NTAPI
NtUserSystemParametersInfo(
  UINT uiAction,
  UINT uiParam,
  PVOID pvParam,
  UINT fWinIni);

DWORD
NTAPI
NtUserTestForInteractiveUser(
    DWORD dwUnknown1);

INT
NTAPI
NtUserToUnicodeEx(
		  UINT wVirtKey,
		  UINT wScanCode,
		  PBYTE lpKeyState,
		  LPWSTR pwszBuff,
		  int cchBuff,
		  UINT wFlags,
		  HKL dwhkl );

DWORD
NTAPI
NtUserTrackMouseEvent(
  DWORD Unknown0);

int
NTAPI
NtUserTranslateAccelerator(
  HWND Window,
  HACCEL Table,
  LPMSG Message);

BOOL
NTAPI
NtUserTranslateMessage(
  LPMSG lpMsg,
  HKL dwhkl );

BOOL
NTAPI
NtUserUnhookWindowsHookEx(
  HHOOK Hook);

BOOL
NTAPI
NtUserUnhookWinEvent(
  HWINEVENTHOOK hWinEventHook);

BOOL
NTAPI
NtUserUnloadKeyboardLayout(
  HKL hKl);

BOOL
NTAPI
NtUserUnlockWindowStation(
  HWINSTA hWindowStation);

BOOL
NTAPI
NtUserUnregisterClass(
  PUNICODE_STRING ClassNameOrAtom,
  HINSTANCE hInstance,
  DWORD Unknown);

BOOL
NTAPI
NtUserUnregisterHotKey(HWND hWnd,
		       int id);

DWORD
NTAPI
NtUserUnregisterUserApiHook(VOID);

DWORD
NTAPI
NtUserUpdateInputContext(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
NTAPI
NtUserUpdateInstance(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
NTAPI
NtUserUpdateLayeredWindow(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6,
  DWORD Unknown7,
  DWORD Unknown8);

BOOL
NTAPI
NtUserUpdatePerUserSystemParameters(
  DWORD dwReserved,
  BOOL bEnable);

DWORD
NTAPI
NtUserUserHandleGrantAccess(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

BOOL
NTAPI
NtUserValidateHandleSecure(
  HANDLE hHdl,
  BOOL Restricted);

BOOL
NTAPI
NtUserValidateRect(
    HWND hWnd,
    CONST RECT *lpRect);

DWORD
NTAPI
NtUserValidateTimerCallback(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
NTAPI
NtUserVkKeyScanEx(
  WCHAR wChar,
  HKL KeyboardLayout,
  DWORD Unknown2);

DWORD
NTAPI
NtUserWaitForInputIdle(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2);

DWORD
NTAPI
NtUserWaitForMsgAndEvent(
  DWORD Unknown0);

BOOL
NTAPI
NtUserWaitMessage(VOID);

DWORD
NTAPI
NtUserWin32PoolAllocationStats(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5);

HWND
NTAPI
NtUserWindowFromPoint(
  LONG X,
  LONG Y);

DWORD
NTAPI
NtUserYieldTask(VOID);

/* lParam of DDE messages */
typedef struct tagKMDDEEXECUTEDATA
{
  HWND Sender;
  HGLOBAL ClientMem;
  /* BYTE Data[DataSize] */
} KMDDEEXECUTEDATA, *PKMDDEEXECUTEDATA;

typedef struct tagKMDDELPARAM
{
  BOOL Packed;
  union
    {
      struct
        {
          UINT uiLo;
          UINT uiHi;
        } Packed;
      LPARAM Unpacked;
    } Value;
} KMDDELPARAM, *PKMDDELPARAM;





/* NtUserBad
 * ReactOS-specific NtUser calls and their related structures, both which shouldn't exist.
 */

#define NOPARAM_ROUTINE_GETMESSAGEEXTRAINFO   0xffff0005
#define NOPARAM_ROUTINE_ANYPOPUP              0xffff0006
#define NOPARAM_ROUTINE_CSRSS_INITIALIZED     0xffff0007
#define ONEPARAM_ROUTINE_CSRSS_GUICHECK       0xffff0008
#define ONEPARAM_ROUTINE_GETMENU              0xfffe0001 // usermode
#define ONEPARAM_ROUTINE_ISWINDOWUNICODE      0xfffe0002
#define ONEPARAM_ROUTINE_GETCARETINFO         0xfffe0007
#define ONEPARAM_ROUTINE_SWITCHCARETSHOWING   0xfffe0008
#define ONEPARAM_ROUTINE_ISWINDOWINDESTROY    0xfffe000c
#define ONEPARAM_ROUTINE_ENABLEPROCWNDGHSTING 0xfffe000d
#define ONEPARAM_ROUTINE_GETDESKTOPMAPPING    0xfffe000e
#define ONEPARAM_ROUTINE_GETWINDOWINSTANCE    0xfffe0010
#define ONEPARAM_ROUTINE_CREATECURICONHANDLE  0xfffe0025 // CREATE_EMPTY_CURSOR_OBJECT ?
#define ONEPARAM_ROUTINE_MSQSETWAKEMASK       0xfffe0027
#define ONEPARAM_ROUTINE_REGISTERUSERMODULE   0xfffe0031
#define ONEPARAM_ROUTINE_GETWNDCONTEXTHLPID   0xfffe0047 // use HWND_ROUTINE_GETWNDCONTEXTHLPID
#define ONEPARAM_ROUTINE_GETCURSORPOSITION    0xfffe0048 // use ONEPARAM_ or TWOPARAM routine ?
#define TWOPARAM_ROUTINE_GETWINDOWRGNBOX    0xfffd0048 // user mode
#define TWOPARAM_ROUTINE_GETWINDOWRGN       0xfffd0049 // user mode
#define TWOPARAM_ROUTINE_SETMENUBARHEIGHT   0xfffd0050
#define TWOPARAM_ROUTINE_SETMENUITEMRECT    0xfffd0051
#define TWOPARAM_ROUTINE_SETGUITHRDHANDLE   0xfffd0052
  #define MSQ_STATE_CAPTURE	0x1
  #define MSQ_STATE_ACTIVE	0x2
  #define MSQ_STATE_FOCUS	0x3
  #define MSQ_STATE_MENUOWNER	0x4
  #define MSQ_STATE_MOVESIZE	0x5
  #define MSQ_STATE_CARET	0x6
#define TWOPARAM_ROUTINE_SETWNDCONTEXTHLPID 0xfffd0058 // use HWNDPARAM_ROUTINE_SETWNDCONTEXTHLPID
#define TWOPARAM_ROUTINE_SETCARETPOS        0xfffd0060
#define TWOPARAM_ROUTINE_GETWINDOWINFO      0xfffd0061
#define TWOPARAM_ROUTINE_REGISTERLOGONPROC  0xfffd0062
#define TWOPARAM_ROUTINE_GETSYSCOLORBRUSHES 0xfffd0063
#define TWOPARAM_ROUTINE_GETSYSCOLORPENS    0xfffd0064
#define TWOPARAM_ROUTINE_GETSYSCOLORS       0xfffd0065
#define TWOPARAM_ROUTINE_SETSYSCOLORS       0xfffd0066 // use NtUserSetSysColors
#define TWOPARAM_ROUTINE_ROS_SHOWWINDOW     0x1000
#define TWOPARAM_ROUTINE_ROS_ISACTIVEICON   0x1001
#define TWOPARAM_ROUTINE_ROS_NCDESTROY      0x1002
#define TWOPARAM_ROUTINE_ROS_REGSYSCLASSES  0x1003
#define TWOPARAM_ROUTINE_ROS_UPDATEUISTATE  0x1004

DWORD
NTAPI
NtUserBuildMenuItemList(
 HMENU hMenu,
 PVOID Buffer,
 ULONG nBufSize,
 DWORD Reserved);

/* Use ONEPARAM_ROUTINE_CREATEEMPTYCURSOROBJECT (0x21) ? */
HANDLE
NTAPI
NtUserCreateCursorIconHandle(
  PICONINFO IconInfo,
  BOOL Indirect);


/* Should be done in usermode */
ULONG_PTR
NTAPI
NtUserGetClassLong(HWND hWnd, INT Offset, BOOL Ansi);

UINT
NTAPI
NtUserGetMenuDefaultItem(
  HMENU hMenu,
  UINT fByPos,
  UINT gmdiFlags);

BOOL
NTAPI
NtUserGetLastInputInfo(
    PLASTINPUTINFO plii);

BOOL
NTAPI
NtUserGetMinMaxInfo(
  HWND hwnd,
  MINMAXINFO *MinMaxInfo,
  BOOL SendMessage);

BOOL
NTAPI
NtUserGetMonitorInfo(
  IN HMONITOR hMonitor,
  OUT LPMONITORINFO pMonitorInfo);

/* Should be done in usermode */
HANDLE
NTAPI
NtUserGetProp(HWND hWnd, ATOM Atom);

BOOL
NTAPI
NtUserGetScrollInfo(
  HWND hwnd,
  int fnBar,
  LPSCROLLINFO lpsi);

HWND
NTAPI
NtUserGetShellWindow();

ULONG
NTAPI
NtUserGetSystemMetrics(ULONG Index);

HWND
NTAPI
NtUserGetWindow(HWND hWnd, UINT Relationship);

/* Should be done in usermode */
LONG
NTAPI
NtUserGetWindowLong(HWND hWnd, DWORD Index, BOOL Ansi);

DWORD
NTAPI
NtUserGetWindowThreadProcessId(HWND hWnd, LPDWORD UnsafePid);



/* (other FocusedItem values give the position of the focused item) */
#define NO_SELECTED_ITEM  0xffff

typedef struct tagROSMENUINFO
{
    /* ----------- MENUINFO ----------- */
    DWORD cbSize;
    DWORD fMask;
    DWORD dwStyle;
    UINT cyMax;
    HBRUSH  hbrBack;
    DWORD dwContextHelpID;
    ULONG_PTR dwMenuData;
    /* ----------- Extra ----------- */
    HMENU Self;         /* Handle of this menu */
    WORD Flags;         /* Menu flags (MF_POPUP, MF_SYSMENU) */
    UINT FocusedItem;   /* Currently focused item */
    UINT MenuItemCount; /* Number of items in the menu */
    HWND Wnd;           /* Window containing the menu */
    WORD Width;         /* Width of the whole menu */
    WORD Height;        /* Height of the whole menu */
    HWND WndOwner;     /* window receiving the messages for ownerdraw */
    BOOL TimeToHide;   /* Request hiding when receiving a second click in the top-level menu item */
    SIZE maxBmpSize;   /* Maximum size of the bitmap items in MIIM_BITMAP state */
} ROSMENUINFO, *PROSMENUINFO;

BOOL
NTAPI
NtUserMenuInfo(
 HMENU hmenu,
 PROSMENUINFO lpmi,
 BOOL fsog
);



typedef struct tagROSMENUITEMINFO
{
    /* ----------- MENUITEMINFOW ----------- */
    UINT cbSize;
    UINT fMask;
    UINT fType;
    UINT fState;
    UINT wID;
    HMENU hSubMenu;
    HBITMAP hbmpChecked;
    HBITMAP hbmpUnchecked;
    DWORD dwItemData;
    LPWSTR dwTypeData;
    UINT cch;
    HBITMAP hbmpItem;
    /* ----------- Extra ----------- */
    RECT Rect;      /* Item area (relative to menu window) */
    UINT XTab;      /* X position of text after Tab */
    LPWSTR Text;    /* Copy of the text pointer in MenuItem->Text */
} ROSMENUITEMINFO, *PROSMENUITEMINFO;

BOOL
NTAPI
NtUserMenuItemInfo(
 HMENU hMenu,
 UINT uItem,
 BOOL fByPosition,
 PROSMENUITEMINFO lpmii,
 BOOL fsog
);

HMONITOR
NTAPI
NtUserMonitorFromPoint(
  IN POINT point,
  IN DWORD dwFlags);

HMONITOR
NTAPI
NtUserMonitorFromRect(
  IN LPCRECT pRect,
  IN DWORD dwFlags);

HMONITOR
NTAPI
NtUserMonitorFromWindow(
  IN HWND hWnd,
  IN DWORD dwFlags);


/* FIXME: These flag constans aren't what Windows uses. */
#define REGISTERCLASS_ANSI	2
#define REGISTERCLASS_ALL	(REGISTERCLASS_ANSI)

RTL_ATOM
NTAPI
NtUserRegisterClassEx(
   CONST WNDCLASSEXW* lpwcx,
   PUNICODE_STRING ClassName,
   PUNICODE_STRING MenuName,
   WNDPROC wpExtra,
   DWORD Flags,
   HMENU hMenu);



typedef struct tagNTUSERSENDMESSAGEINFO
{
  BOOL HandledByKernel;
  BOOL Ansi;
  WNDPROC Proc;
} NTUSERSENDMESSAGEINFO, *PNTUSERSENDMESSAGEINFO;

/* use NtUserMessageCall */
LRESULT NTAPI
NtUserSendMessage(HWND hWnd,
		  UINT Msg,
		  WPARAM wParam,
		  LPARAM lParam,
          PNTUSERSENDMESSAGEINFO Info);

/* use NtUserMessageCall */
LRESULT NTAPI
NtUserSendMessageTimeout(HWND hWnd,
			 UINT Msg,
			 WPARAM wParam,
			 LPARAM lParam,
			 UINT uFlags,
			 UINT uTimeout,
			 ULONG_PTR *uResult,
             PNTUSERSENDMESSAGEINFO Info);

/* use NtUserMessageCall */
BOOL
NTAPI
NtUserSendNotifyMessage(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam);


typedef struct _SETSCROLLBARINFO
{
  int nTrackPos;
  int reserved;
  DWORD rgstate[CCHILDREN_SCROLLBAR+1];
} SETSCROLLBARINFO, *PSETSCROLLBARINFO;

BOOL
NTAPI
NtUserSetScrollBarInfo(
  HWND hwnd,
  LONG idObject,
  SETSCROLLBARINFO *info);



#endif /* __WIN32K_NTUSER_H */

/* EOF */
