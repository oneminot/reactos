#pragma once

#include <ndk/kbd.h>

typedef struct _KBL
{
  LIST_ENTRY List;
  DWORD Flags;
  WCHAR Name[KL_NAMELENGTH];    // used w GetKeyboardLayoutName same as wszKLID.
  struct _KBDTABLES* KBTables;  // KBDTABLES in ndk/kbd.h
  HANDLE hModule;
  ULONG RefCount;
  HKL hkl;
  DWORD klid; // Low word - language id. High word - device id.
} KBL, *PKBL;

typedef struct _ATTACHINFO
{
  struct _ATTACHINFO* paiNext;
  PTHREADINFO pti1;
  PTHREADINFO pti2;
} ATTACHINFO, *PATTACHINFO;

extern PATTACHINFO gpai;

#define KBL_UNLOAD 1
#define KBL_PRELOAD 2
#define KBL_RESET 4

/* Key States */
#define KS_DOWN_BIT      0x80
#define KS_LOCK_BIT      0x01
/* Scan Codes */
#define SC_KEY_UP        0x8000
/* lParam bits */
#define LP_EXT_BIT         (1<<24)
#define LP_DO_NOT_CARE_BIT (1<<25) // for GetKeyNameText
#define LP_CONTEXT_BIT     (1<<29)
#define LP_PREV_STATE_BIT  (1<<30)
#define LP_TRANSITION_BIT  (1<<31)


INIT_FUNCTION NTSTATUS NTAPI InitInputImpl(VOID);
INIT_FUNCTION NTSTATUS NTAPI InitKeyboardImpl(VOID);
PKBL W32kGetDefaultKeyLayout(VOID);
VOID NTAPI UserProcessKeyboardInput(PKEYBOARD_INPUT_DATA pKeyInput);
BOOL NTAPI UserSendKeyboardInput(KEYBDINPUT *pKbdInput, BOOL bInjected);
BOOL FASTCALL IntBlockInput(PTHREADINFO W32Thread, BOOL BlockIt);
BOOL FASTCALL IntMouseInput(MOUSEINPUT *mi, BOOL Injected);
BOOL UserInitDefaultKeyboardLayout(VOID);
PKBL UserHklToKbl(HKL hKl);
BOOL FASTCALL UserAttachThreadInput(PTHREADINFO,PTHREADINFO,BOOL);
VOID FASTCALL DoTheScreenSaver(VOID);
#define ThreadHasInputAccess(W32Thread) \
  (TRUE)

extern HANDLE ghKeyboardDevice;
extern PTHREADINFO ptiRawInput;
extern BYTE gKeyStateTable[0x100];
