/*
 *  GDI object common header definition
 *
 */

#ifndef __WIN32K_GDIOBJ_H
#define __WIN32K_GDIOBJ_H

#include <ddk/ntddk.h>

/* base address where the handle table is mapped to */
#define GDI_HANDLE_TABLE_BASE_ADDRESS (0x400000)

/* gdi handle table can hold 0x4000 handles */
#define GDI_HANDLE_COUNT 0x4000

#define GDI_GLOBAL_PROCESS (0x0)

#define GDI_HANDLE_INDEX_MASK (GDI_HANDLE_COUNT - 1)
#define GDI_HANDLE_TYPE_MASK  0x007f0000
#define GDI_HANDLE_STOCK_MASK 0x00800000

#define GDI_HANDLE_CREATE(i, t)    ((HANDLE)(((i) & GDI_HANDLE_INDEX_MASK) | ((t) & GDI_HANDLE_TYPE_MASK)))
#define GDI_HANDLE_GET_INDEX(h)    (((DWORD)(h)) & GDI_HANDLE_INDEX_MASK)
#define GDI_HANDLE_GET_TYPE(h)     (((DWORD)(h)) & GDI_HANDLE_TYPE_MASK)
#define GDI_HANDLE_IS_TYPE(h, t)   ((t) == (((DWORD)(h)) & GDI_HANDLE_TYPE_MASK))
#define GDI_HANDLE_IS_STOCKOBJ(h)  (0 != (((DWORD)(h)) & GDI_HANDLE_STOCK_MASK))
#define GDI_HANDLE_SET_STOCKOBJ(h) ((h) = (HANDLE)(((DWORD)(h)) | GDI_HANDLE_STOCK_MASK))

/*! \defgroup GDI object types
 *
 *  GDI object types
 *
 */
/*@{*/
#define GDI_OBJECT_TYPE_DC          0x00010000
#define GDI_OBJECT_TYPE_REGION      0x00040000
#define GDI_OBJECT_TYPE_BITMAP      0x00050000
#define GDI_OBJECT_TYPE_PALETTE     0x00080000
#define GDI_OBJECT_TYPE_FONT        0x000a0000
#define GDI_OBJECT_TYPE_BRUSH       0x00100000
#define GDI_OBJECT_TYPE_EMF         0x00210000
#define GDI_OBJECT_TYPE_PEN         0x00300000
#define GDI_OBJECT_TYPE_EXTPEN      0x00500000
/* Following object types made up for ROS */
#define GDI_OBJECT_TYPE_METADC      0x00710000
#define GDI_OBJECT_TYPE_METAFILE    0x00720000
#define GDI_OBJECT_TYPE_ENHMETAFILE 0x00730000
#define GDI_OBJECT_TYPE_ENHMETADC   0x00740000
#define GDI_OBJECT_TYPE_MEMDC       0x00750000
#define GDI_OBJECT_TYPE_DCE         0x00770000
#define GDI_OBJECT_TYPE_DONTCARE    0x007f0000
/*@}*/

typedef PVOID PGDIOBJ;

typedef BOOL (FASTCALL *GDICLEANUPPROC)(PGDIOBJ Obj);

/*!
 * GDI object header. This is a part of any GDI object
*/
typedef struct _GDIOBJHDR
{
  LONG RefCount; /* reference count for the object */
  struct _W32THREAD *LockingThread; /* only assigned if a thread is holding the lock! */
  ULONG Type;
  PPAGED_LOOKASIDE_LIST LookasideList; /* FIXME - remove this */
#ifdef GDI_DEBUG
  const char* lockfile;
  int lockline;
#endif
} GDIOBJHDR, *PGDIOBJHDR;

typedef struct _GDIMULTILOCK
{
  HGDIOBJ hObj;
  PGDIOBJ pObj;
  DWORD	ObjectType;
} GDIMULTILOCK, *PGDIMULTILOCK;

HGDIOBJ FASTCALL GDIOBJ_AllocObj(ULONG ObjectType);
BOOL    FASTCALL GDIOBJ_FreeObj (HGDIOBJ Obj, DWORD ObjectType);
BOOL    FASTCALL GDIOBJ_LockMultipleObj(PGDIMULTILOCK pList, INT nObj);
BOOL    FASTCALL GDIOBJ_UnlockMultipleObj(PGDIMULTILOCK pList, INT nObj);
BOOL    FASTCALL GDIOBJ_OwnedByCurrentProcess(HGDIOBJ ObjectHandle);
void    FASTCALL GDIOBJ_SetOwnership(HGDIOBJ ObjectHandle, PEPROCESS Owner);
void    FASTCALL GDIOBJ_CopyOwnership(HGDIOBJ CopyFrom, HGDIOBJ CopyTo);
BOOL    FASTCALL GDIOBJ_ConvertToStockObj(HGDIOBJ *hObj);
BOOL    FASTCALL GDIOBJ_LockMultipleObj(PGDIMULTILOCK pList, INT nObj);

#define GDIOBJ_GetObjectType(Handle) \
  GDI_HANDLE_GET_TYPE(Handle)

#ifdef GDI_DEBUG

/* a couple macros for debugging GDIOBJ locking */
#define GDIOBJ_LockObj(obj,ty) GDIOBJ_LockObjDbg(__FILE__,__LINE__,obj,ty)
#define GDIOBJ_UnlockObj(obj) GDIOBJ_UnlockObjDbg(__FILE__,__LINE__,obj)

PGDIOBJ FASTCALL GDIOBJ_LockObjDbg (const char* file, int line, HGDIOBJ Obj, DWORD ObjectType);
BOOL FASTCALL GDIOBJ_UnlockObjDbg (const char* file, int line, PGDIOBJ Object);

#else /* !GDI_DEBUG */

PGDIOBJ FASTCALL GDIOBJ_LockObj (HGDIOBJ Obj, DWORD ObjectType);
BOOL    FASTCALL GDIOBJ_UnlockObj (PGDIOBJ Object);

#endif /* GDI_DEBUG */

#define GDIOBJFLAG_DEFAULT	(0x0)
#define GDIOBJFLAG_IGNOREPID 	(0x1)
#define GDIOBJFLAG_IGNORELOCK 	(0x2)

PVOID   FASTCALL GDI_MapHandleTable(HANDLE hProcess);

#endif
