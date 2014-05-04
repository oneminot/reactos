/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            consrv/include/conio_winsrv.h
 * PURPOSE:         Public Console I/O Interface
 * PROGRAMMERS:     G� van Geldorp
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

#define CSR_DEFAULT_CURSOR_SIZE 25

typedef struct _FRONTEND FRONTEND, *PFRONTEND;
/* HACK: */ typedef struct _CONSOLE_INFO *PCONSOLE_INFO;
typedef struct _FRONTEND_VTBL
{
    // NTSTATUS (NTAPI *UnloadFrontEnd)(IN OUT PFRONTEND This);

    /*
     * Internal interface (functions called by the console server only)
     */
    NTSTATUS (NTAPI *InitFrontEnd)(IN OUT PFRONTEND This,
                                   IN struct _CONSOLE* Console);
    VOID (NTAPI *DeinitFrontEnd)(IN OUT PFRONTEND This);

    /* Interface used for both text-mode and graphics screen buffers */
    VOID (NTAPI *DrawRegion)(IN OUT PFRONTEND This,
                             SMALL_RECT* Region);
    /* Interface used only for text-mode screen buffers */
    VOID (NTAPI *WriteStream)(IN OUT PFRONTEND This,
                              SMALL_RECT* Region,
                              SHORT CursorStartX,
                              SHORT CursorStartY,
                              UINT ScrolledLines,
                              PWCHAR Buffer,
                              UINT Length);
    BOOL (NTAPI *SetCursorInfo)(IN OUT PFRONTEND This,
                                PCONSOLE_SCREEN_BUFFER ScreenBuffer);
    BOOL (NTAPI *SetScreenInfo)(IN OUT PFRONTEND This,
                                PCONSOLE_SCREEN_BUFFER ScreenBuffer,
                                SHORT OldCursorX,
                                SHORT OldCursorY);
    VOID (NTAPI *ResizeTerminal)(IN OUT PFRONTEND This);
    VOID (NTAPI *SetActiveScreenBuffer)(IN OUT PFRONTEND This);
    VOID (NTAPI *ReleaseScreenBuffer)(IN OUT PFRONTEND This,
                                      IN PCONSOLE_SCREEN_BUFFER ScreenBuffer);
    BOOL (NTAPI *ProcessKeyCallback)(IN OUT PFRONTEND This,
                                     MSG* msg,
                                     BYTE KeyStateMenu,
                                     DWORD ShiftState,
                                     UINT VirtualKeyCode,
                                     BOOL Down);
    VOID (NTAPI *RefreshInternalInfo)(IN OUT PFRONTEND This);

    /*
     * External interface (functions corresponding to the Console API)
     */
    VOID (NTAPI *ChangeTitle)(IN OUT PFRONTEND This);
    BOOL (NTAPI *ChangeIcon)(IN OUT PFRONTEND This,
                             HICON IconHandle);
    HWND (NTAPI *GetConsoleWindowHandle)(IN OUT PFRONTEND This);
    VOID (NTAPI *GetLargestConsoleWindowSize)(IN OUT PFRONTEND This,
                                              PCOORD pSize);
    BOOL (NTAPI *GetSelectionInfo)(IN OUT PFRONTEND This,
                                   PCONSOLE_SELECTION_INFO pSelectionInfo);
    BOOL (NTAPI *SetPalette)(IN OUT PFRONTEND This,
                             HPALETTE PaletteHandle,
                             UINT PaletteUsage);
    ULONG (NTAPI *GetDisplayMode)(IN OUT PFRONTEND This);
    BOOL  (NTAPI *SetDisplayMode)(IN OUT PFRONTEND This,
                                  ULONG NewMode);
    INT   (NTAPI *ShowMouseCursor)(IN OUT PFRONTEND This,
                                   BOOL Show);
    BOOL  (NTAPI *SetMouseCursor)(IN OUT PFRONTEND This,
                                  HCURSOR CursorHandle);
    HMENU (NTAPI *MenuControl)(IN OUT PFRONTEND This,
                               UINT CmdIdLow,
                               UINT CmdIdHigh);
    BOOL  (NTAPI *SetMenuClose)(IN OUT PFRONTEND This,
                                BOOL Enable);
} FRONTEND_VTBL, *PFRONTEND_VTBL;

struct _FRONTEND
{
    PFRONTEND_VTBL Vtbl;        /* Virtual table */
    NTSTATUS (NTAPI *UnloadFrontEnd)(IN OUT PFRONTEND This);

    struct _CONSOLE* Console;   /* Console to which the frontend is attached to */
    PVOID Data;                 /* Private data  */
    PVOID OldData;              /* Reserved      */
};

/* PauseFlags values (internal only) */
#define PAUSED_FROM_KEYBOARD  0x1
#define PAUSED_FROM_SCROLLBAR 0x2
#define PAUSED_FROM_SELECTION 0x4

typedef struct _WINSRV_CONSOLE
{
/******************************* Console Set-up *******************************/
    /* This **MUST** be FIRST!! */
    // CONSOLE;

    // LONG ReferenceCount;                    /* Is incremented each time a handle to something in the console (a screen-buffer or the input buffer of this console) gets referenced */
    // CRITICAL_SECTION Lock;
    // CONSOLE_STATE State;                    /* State of the console */

    FRONTEND FrontEndIFace;                     /* Frontend-specific interface */

/******************************* Process support ******************************/
    LIST_ENTRY ProcessList;         /* List of processes owning the console. The first one is the so-called "Console Leader Process" */
    PCONSOLE_PROCESS_DATA NotifiedLastCloseProcess; /* Pointer to the unique process that needs to be notified when the console leader process is killed */
    BOOLEAN NotifyLastClose;        /* TRUE if the console should send a control event when the console leader process is killed */

    BOOLEAN QuickEdit;

/******************************* Pausing support ******************************/
    BYTE PauseFlags;
    LIST_ENTRY  ReadWaitQueue;      /* List head for the queue of unique input buffer read wait blocks */
    LIST_ENTRY WriteWaitQueue;      /* List head for the queue of current screen-buffer write wait blocks */

/**************************** Aliases and Histories ***************************/
    struct _ALIAS_HEADER *Aliases;
    LIST_ENTRY HistoryBuffers;
    ULONG HistoryBufferSize;                /* Size for newly created history buffers */
    ULONG NumberOfHistoryBuffers;           /* Maximum number of history buffers allowed */
    BOOLEAN HistoryNoDup;                   /* Remove old duplicate history entries */

} WINSRV_CONSOLE, *PWINSRV_CONSOLE;

/* console.c */
VOID ConioPause(PCONSOLE Console, UINT Flags);
VOID ConioUnpause(PCONSOLE Console, UINT Flags);

PCONSOLE_PROCESS_DATA NTAPI
ConSrvGetConsoleLeaderProcess(IN PCONSOLE Console);
NTSTATUS
ConSrvConsoleCtrlEvent(IN ULONG CtrlEvent,
                       IN PCONSOLE_PROCESS_DATA ProcessData);
NTSTATUS NTAPI
ConSrvConsoleProcessCtrlEvent(IN PCONSOLE Console,
                              IN ULONG ProcessGroupId,
                              IN ULONG CtrlEvent);

/* coninput.c */
VOID NTAPI ConioProcessKey(PCONSOLE Console, MSG* msg);
NTSTATUS ConioAddInputEvent(PCONSOLE Console,
                                     PINPUT_RECORD InputEvent,
                                     BOOLEAN AppendToEnd);
NTSTATUS ConioProcessInputEvent(PCONSOLE Console,
                                         PINPUT_RECORD InputEvent);

/* conoutput.c */
#define ConioInitRect(Rect, top, left, bottom, right) \
do {    \
    ((Rect)->Top) = top;    \
    ((Rect)->Left) = left;  \
    ((Rect)->Bottom) = bottom;  \
    ((Rect)->Right) = right;    \
} while (0)
#define ConioIsRectEmpty(Rect) \
    (((Rect)->Left > (Rect)->Right) || ((Rect)->Top > (Rect)->Bottom))
#define ConioRectHeight(Rect) \
    (((Rect)->Top) > ((Rect)->Bottom) ? 0 : ((Rect)->Bottom) - ((Rect)->Top) + 1)
#define ConioRectWidth(Rect) \
    (((Rect)->Left) > ((Rect)->Right) ? 0 : ((Rect)->Right) - ((Rect)->Left) + 1)

#define ConsoleUnicodeCharToAnsiChar(Console, dChar, sWChar) \
    WideCharToMultiByte((Console)->OutputCodePage, 0, (sWChar), 1, (dChar), 1, NULL, NULL)

#define ConsoleAnsiCharToUnicodeChar(Console, dWChar, sChar) \
    MultiByteToWideChar((Console)->OutputCodePage, 0, (sChar), 1, (dWChar), 1)

PCHAR_INFO ConioCoordToPointer(PTEXTMODE_SCREEN_BUFFER Buff, ULONG X, ULONG Y);
VOID ConioDrawConsole(PCONSOLE Console);
NTSTATUS ConioResizeBuffer(PCONSOLE Console,
                           PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                           COORD Size);
NTSTATUS ConioWriteConsole(PCONSOLE Console,
                           PTEXTMODE_SCREEN_BUFFER Buff,
                           PWCHAR Buffer,
                           DWORD Length,
                           BOOL Attrib);
DWORD ConioEffectiveCursorSize(PCONSOLE Console,
                                        DWORD Scale);

/* EOF */
