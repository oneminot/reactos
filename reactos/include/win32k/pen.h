#ifndef __WIN32K_PEN_H
#define __WIN32K_PEN_H

#include <win32k/gdiobj.h>
#include <win32k/brush.h>

/* Internal interface */

#define PENOBJ_AllocPen() ((HPEN)GDIOBJ_AllocObj(GDI_OBJECT_TYPE_PEN))
#define PENOBJ_FreePen(hBMObj) GDIOBJ_FreeObj((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_PEN)
#define PENOBJ_LockPen(hBMObj) ((PGDIBRUSHOBJ)GDIOBJ_LockObj((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_PEN))
#define PENOBJ_UnlockPen(hBMObj) GDIOBJ_UnlockObj((HGDIOBJ) hBMObj)

HPEN STDCALL
NtGdiCreatePen(
   INT PenStyle,
   INT Width,
   COLORREF Color);

HPEN STDCALL
NtGdiCreatePenIndirect(
   CONST PLOGPEN LogBrush);

HPEN STDCALL
NtGdiExtCreatePen(
   DWORD PenStyle,
   DWORD Width,
   CONST LOGBRUSH *LogBrush,
   DWORD StyleCount,
   CONST DWORD *Style);

#endif
