#ifndef _WIN32K_CURSORICON_H
#define _WIN32K_CURSORICON_H

#define MAXCURICONHANDLES 4096

typedef struct _CURICON_OBJECT
{
  HICON Handle;
  PW32PROCESS Process;
  HMODULE hModule;
  HRSRC hRsrc;
  HRSRC hGroupRsrc;
  SIZE Size;
  BYTE Shadow;
  ICONINFO IconInfo;
} CURICON_OBJECT, *PCURICON_OBJECT;

HCURSOR FASTCALL IntSetCursor(PWINSTATION_OBJECT WinStaObject, PCURICON_OBJECT NewCursor, BOOL ForceChange);
BOOL FASTCALL IntSetupCurIconHandles(PWINSTATION_OBJECT WinStaObject);
PCURICON_OBJECT FASTCALL IntGetCurIconObject(PWINSTATION_OBJECT WinStaObject, HICON hIcon);
VOID FASTCALL IntReleaseCurIconObject(PWINSTATION_OBJECT WinStaObject);
PCURICON_OBJECT FASTCALL IntCreateCurIconHandle(PWINSTATION_OBJECT WinStaObject);

#endif /* _WIN32K_CURSORICON_H */

/* EOF */

