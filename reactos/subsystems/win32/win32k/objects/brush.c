/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS win32 subsystem
 * PURPOSE:           Functions for brushes
 * FILE:              subsystem/win32/win32k/objects/brush.c
 * PROGRAMER:
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

#define GDIOBJATTRFREE 170

typedef struct _GDI_OBJ_ATTR_FREELIST
{
  LIST_ENTRY Entry;
  DWORD nEntries;
  PVOID AttrList[GDIOBJATTRFREE];
} GDI_OBJ_ATTR_FREELIST, *PGDI_OBJ_ATTR_FREELIST;

typedef struct _GDI_OBJ_ATTR_ENTRY
{
  RGN_ATTR Attr[GDIOBJATTRFREE];
} GDI_OBJ_ATTR_ENTRY, *PGDI_OBJ_ATTR_ENTRY;

static const ULONG HatchBrushes[NB_HATCH_STYLES][8] =
{
    {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF}, /* HS_HORIZONTAL */
    {0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7}, /* HS_VERTICAL   */
    {0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F}, /* HS_FDIAGONAL  */
    {0x7F, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE}, /* HS_BDIAGONAL  */
    {0xF7, 0xF7, 0xF7, 0xF7, 0x00, 0xF7, 0xF7, 0xF7}, /* HS_CROSS      */
    {0x7E, 0xBD, 0xDB, 0xE7, 0xE7, 0xDB, 0xBD, 0x7E}  /* HS_DIAGCROSS  */
};


PVOID
FASTCALL
AllocateObjectAttr(VOID)
{
  PTHREADINFO pti;
  PPROCESSINFO ppi;
  PVOID pAttr;
  PGDI_OBJ_ATTR_FREELIST pGdiObjAttrFreeList;
  PGDI_OBJ_ATTR_ENTRY pGdiObjAttrEntry;
  int i;

  pti = PsGetCurrentThreadWin32Thread();
  if (pti->pgdiBrushAttr)
  {
     pAttr = pti->pgdiBrushAttr; // Get the free one.
     pti->pgdiBrushAttr = NULL;
     return pAttr;
  }

  ppi = PsGetCurrentProcessWin32Process();

  if (!ppi->pBrushAttrList) // If set point is null, allocate new group.
  {
     pGdiObjAttrEntry = EngAllocUserMem(sizeof(GDI_OBJ_ATTR_ENTRY), 0);

     if (!pGdiObjAttrEntry)
     {
        DPRINT1("Attr Failed User Allocation!\n");
        return NULL;
     }

     DPRINT("AllocObjectAttr User 0x%x\n",pGdiObjAttrEntry);

     pGdiObjAttrFreeList = ExAllocatePoolWithTag( PagedPool,
                                                  sizeof(GDI_OBJ_ATTR_FREELIST),
                                                  GDITAG_BRUSH_FREELIST);
     if ( !pGdiObjAttrFreeList )
     {
        EngFreeUserMem(pGdiObjAttrEntry);
        return NULL;
     }

     RtlZeroMemory(pGdiObjAttrFreeList, sizeof(GDI_OBJ_ATTR_FREELIST));

     DPRINT("AllocObjectAttr Ex 0x%x\n",pGdiObjAttrFreeList);

     InsertHeadList( &ppi->GDIBrushAttrFreeList, &pGdiObjAttrFreeList->Entry);

     pGdiObjAttrFreeList->nEntries = GDIOBJATTRFREE;
     // Start at the bottom up and set end of free list point.
     ppi->pBrushAttrList = &pGdiObjAttrEntry->Attr[GDIOBJATTRFREE-1];
     // Build the free attr list.
     for ( i = 0; i < GDIOBJATTRFREE; i++)
     {
         pGdiObjAttrFreeList->AttrList[i] = &pGdiObjAttrEntry->Attr[i];
     }
  }

  pAttr = ppi->pBrushAttrList;
  pGdiObjAttrFreeList = (PGDI_OBJ_ATTR_FREELIST)ppi->GDIBrushAttrFreeList.Flink;

  // Free the list when it is full!
  if ( pGdiObjAttrFreeList->nEntries-- == 1)
  {  // No more free entries, so yank the list.
     RemoveEntryList( &pGdiObjAttrFreeList->Entry );

     ExFreePoolWithTag( pGdiObjAttrFreeList, GDITAG_BRUSH_FREELIST );

     if ( IsListEmpty( &ppi->GDIBrushAttrFreeList ) )
     {
        ppi->pBrushAttrList = NULL;
        return pAttr;
     }

     pGdiObjAttrFreeList = (PGDI_OBJ_ATTR_FREELIST)ppi->GDIBrushAttrFreeList.Flink;
  }

  ppi->pBrushAttrList = pGdiObjAttrFreeList->AttrList[pGdiObjAttrFreeList->nEntries-1];

  return pAttr;
}

VOID
FASTCALL
FreeObjectAttr(PVOID pAttr)
{
  PTHREADINFO pti;
  PPROCESSINFO ppi;
  PGDI_OBJ_ATTR_FREELIST pGdiObjAttrFreeList;

  pti = PsGetCurrentThreadWin32Thread();

  if (!pti) return;

  if (!pti->pgdiBrushAttr)
  {  // If it is null, just cache it for the next time.
     pti->pgdiBrushAttr = pAttr;
     return;
  }

  ppi = PsGetCurrentProcessWin32Process();

  pGdiObjAttrFreeList = (PGDI_OBJ_ATTR_FREELIST)ppi->GDIBrushAttrFreeList.Flink;

  // We add to the list of free entries, so this will grows!
  if ( IsListEmpty(&ppi->GDIBrushAttrFreeList) ||
       pGdiObjAttrFreeList->nEntries == GDIOBJATTRFREE )
  {
     pGdiObjAttrFreeList = ExAllocatePoolWithTag( PagedPool,
                                                  sizeof(GDI_OBJ_ATTR_FREELIST),
                                                  GDITAG_BRUSH_FREELIST);
     if ( !pGdiObjAttrFreeList )
     {
        return;
     }
     InsertHeadList( &ppi->GDIBrushAttrFreeList, &pGdiObjAttrFreeList->Entry);
     pGdiObjAttrFreeList->nEntries = 0;
  }
  // Up count, save the entry and set end of free list point.
  ++pGdiObjAttrFreeList->nEntries; // Top Down...
  pGdiObjAttrFreeList->AttrList[pGdiObjAttrFreeList->nEntries-1] = pAttr;
  ppi->pBrushAttrList = pAttr;

  return;
}


BOOL
INTERNAL_CALL
BRUSH_Cleanup(PVOID ObjectBody)
{
    PBRUSH pbrush = (PBRUSH)ObjectBody;
    if (pbrush->flAttrs & (GDIBRUSH_IS_HATCH | GDIBRUSH_IS_BITMAP))
    {
        ASSERT(pbrush->hbmPattern);
        GDIOBJ_SetOwnership(pbrush->hbmPattern, PsGetCurrentProcess());
        GreDeleteObject(pbrush->hbmPattern);
    }

    /* Free the kmode styles array of EXTPENS */
    if (pbrush->pStyle)
    {
        ExFreePool(pbrush->pStyle);
    }

    return TRUE;
}

INT
FASTCALL
BRUSH_GetObject(PBRUSH pbrush, INT Count, LPLOGBRUSH Buffer)
{
    if (Buffer == NULL) return sizeof(LOGBRUSH);
    if (Count == 0) return 0;

    /* Set colour */
    Buffer->lbColor = pbrush->BrushAttr.lbColor;

    /* Set Hatch */
    if ((pbrush->flAttrs & GDIBRUSH_IS_HATCH)!=0)
    {
        /* FIXME : this is not the right value */
        Buffer->lbHatch = (LONG)pbrush->hbmPattern;
    }
    else
    {
        Buffer->lbHatch = 0;
    }

    Buffer->lbStyle = 0;

    /* Get the type of style */
    if ((pbrush->flAttrs & GDIBRUSH_IS_SOLID)!=0)
    {
        Buffer->lbStyle = BS_SOLID;
    }
    else if ((pbrush->flAttrs & GDIBRUSH_IS_NULL)!=0)
    {
        Buffer->lbStyle = BS_NULL; // BS_HOLLOW
    }
    else if ((pbrush->flAttrs & GDIBRUSH_IS_HATCH)!=0)
    {
        Buffer->lbStyle = BS_HATCHED;
    }
    else if ((pbrush->flAttrs & GDIBRUSH_IS_BITMAP)!=0)
    {
        Buffer->lbStyle = BS_PATTERN;
    }
    else if ((pbrush->flAttrs & GDIBRUSH_IS_DIB)!=0)
    {
        Buffer->lbStyle = BS_DIBPATTERN;
    }

    /* FIXME
    else if ((pbrush->flAttrs & )!=0)
    {
        Buffer->lbStyle = BS_INDEXED;
    }
    else if ((pbrush->flAttrs & )!=0)
    {
        Buffer->lbStyle = BS_DIBPATTERNPT;
    }
    */

    /* FIXME */
    return sizeof(LOGBRUSH);
}

HBRUSH
APIENTRY
IntGdiCreateDIBBrush(
    CONST BITMAPINFO *BitmapInfo,
    UINT ColorSpec,
    UINT BitmapInfoSize,
    CONST VOID *PackedDIB)
{
    HBRUSH hBrush;
    PBRUSH pbrush;
    HBITMAP hPattern;
    ULONG_PTR DataPtr;
    PVOID pvDIBits;

    if (BitmapInfo->bmiHeader.biSize < sizeof(BITMAPINFOHEADER))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    DataPtr = (ULONG_PTR)BitmapInfo + DIB_BitmapInfoSize(BitmapInfo, ColorSpec);

    hPattern = DIB_CreateDIBSection(NULL, BitmapInfo, ColorSpec, &pvDIBits, NULL, 0, 0);
    if (hPattern == NULL)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
	RtlCopyMemory(pvDIBits,
		          (PVOID)DataPtr,
				  DIB_GetDIBImageBytes(BitmapInfo->bmiHeader.biWidth,
                                       BitmapInfo->bmiHeader.biHeight,
                                       BitmapInfo->bmiHeader.biBitCount * BitmapInfo->bmiHeader.biPlanes));

    pbrush = BRUSH_AllocBrushWithHandle();
    if (pbrush == NULL)
    {
        GreDeleteObject(hPattern);
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    hBrush = pbrush->BaseObject.hHmgr;

    pbrush->flAttrs |= GDIBRUSH_IS_BITMAP | GDIBRUSH_IS_DIB;
    pbrush->hbmPattern = hPattern;
    /* FIXME: Fill in the rest of fields!!! */

    GDIOBJ_SetOwnership(hPattern, NULL);

    BRUSH_UnlockBrush(pbrush);

    return hBrush;
}

HBRUSH
APIENTRY
IntGdiCreateHatchBrush(
    INT Style,
    COLORREF Color)
{
    HBRUSH hBrush;
    PBRUSH pbrush;
    HBITMAP hPattern;

    if (Style < 0 || Style >= NB_HATCH_STYLES)
    {
        return 0;
    }

    hPattern = GreCreateBitmap(8, 8, 1, 1, (LPBYTE)HatchBrushes[Style]);
    if (hPattern == NULL)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    pbrush = BRUSH_AllocBrushWithHandle();
    if (pbrush == NULL)
    {
        GreDeleteObject(hPattern);
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    hBrush = pbrush->BaseObject.hHmgr;

    pbrush->flAttrs |= GDIBRUSH_IS_HATCH;
    pbrush->hbmPattern = hPattern;
    pbrush->BrushAttr.lbColor = Color & 0xFFFFFF;

    GDIOBJ_SetOwnership(hPattern, NULL);

    BRUSH_UnlockBrush(pbrush);

    return hBrush;
}

HBRUSH
APIENTRY
IntGdiCreatePatternBrush(
    HBITMAP hBitmap)
{
    HBRUSH hBrush;
    PBRUSH pbrush;
    HBITMAP hPattern;

    hPattern = BITMAP_CopyBitmap(hBitmap);
    if (hPattern == NULL)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    pbrush = BRUSH_AllocBrushWithHandle();
    if (pbrush == NULL)
    {
        GreDeleteObject(hPattern);
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    hBrush = pbrush->BaseObject.hHmgr;

    pbrush->flAttrs |= GDIBRUSH_IS_BITMAP;
    pbrush->hbmPattern = hPattern;
    /* FIXME: Fill in the rest of fields!!! */

    GDIOBJ_SetOwnership(hPattern, NULL);

    BRUSH_UnlockBrush(pbrush);

    return hBrush;
}

HBRUSH
APIENTRY
IntGdiCreateSolidBrush(
    COLORREF Color)
{
    HBRUSH hBrush;
    PBRUSH pbrush;

    pbrush = BRUSH_AllocBrushWithHandle();
    if (pbrush == NULL)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    hBrush = pbrush->BaseObject.hHmgr;

    pbrush->flAttrs |= GDIBRUSH_IS_SOLID;

    pbrush->BrushAttr.lbColor = Color & 0x00FFFFFF;
    /* FIXME: Fill in the rest of fields!!! */

    BRUSH_UnlockBrush(pbrush);

    return hBrush;
}

HBRUSH
APIENTRY
IntGdiCreateNullBrush(VOID)
{
    HBRUSH hBrush;
    PBRUSH pbrush;

    pbrush = BRUSH_AllocBrushWithHandle();
    if (pbrush == NULL)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    hBrush = pbrush->BaseObject.hHmgr;

    pbrush->flAttrs |= GDIBRUSH_IS_NULL;
    BRUSH_UnlockBrush(pbrush);

    return hBrush;
}

VOID
FASTCALL
IntGdiSetSolidBrushColor(HBRUSH hBrush, COLORREF Color)
{
    PBRUSH pbrush;

    pbrush = BRUSH_LockBrush(hBrush);
    if (pbrush->flAttrs & GDIBRUSH_IS_SOLID)
    {
        pbrush->BrushAttr.lbColor = Color & 0xFFFFFF;
    }
    BRUSH_UnlockBrush(pbrush);
}


/* PUBLIC FUNCTIONS ***********************************************************/

HBRUSH
APIENTRY
NtGdiCreateDIBBrush(
    IN PVOID BitmapInfoAndData,
    IN FLONG ColorSpec,
    IN UINT BitmapInfoSize,
    IN BOOL  b8X8,
    IN BOOL bPen,
    IN PVOID PackedDIB)
{
    BITMAPINFO *SafeBitmapInfoAndData;
    NTSTATUS Status = STATUS_SUCCESS;
    HBRUSH hBrush;

    SafeBitmapInfoAndData = EngAllocMem(FL_ZERO_MEMORY, BitmapInfoSize, TAG_DIB);
    if (SafeBitmapInfoAndData == NULL)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    _SEH2_TRY
    {
        ProbeForRead(BitmapInfoAndData, BitmapInfoSize, 1);
        RtlCopyMemory(SafeBitmapInfoAndData, BitmapInfoAndData, BitmapInfoSize);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        EngFreeMem(SafeBitmapInfoAndData);
        SetLastNtError(Status);
        return 0;
    }

    hBrush = IntGdiCreateDIBBrush(SafeBitmapInfoAndData,
                                  ColorSpec,
                                  BitmapInfoSize,
                                  PackedDIB);

    EngFreeMem(SafeBitmapInfoAndData);

    return hBrush;
}

HBRUSH
APIENTRY
NtGdiCreateHatchBrushInternal(
    ULONG Style,
    COLORREF Color,
    BOOL bPen)
{
    return IntGdiCreateHatchBrush(Style, Color);
}

HBRUSH
APIENTRY
NtGdiCreatePatternBrushInternal(
    HBITMAP hBitmap,
    BOOL bPen,
    BOOL b8x8)
{
    return IntGdiCreatePatternBrush(hBitmap);
}

HBRUSH
APIENTRY
NtGdiCreateSolidBrush(COLORREF Color,
                      IN OPTIONAL HBRUSH hbr)
{
    return IntGdiCreateSolidBrush(Color);
}

/**
 * \name NtGdiSetBrushOrg
 *
 * \brief Sets the brush origin that GDI assigns to
 * the next brush an application selects into the specified device context.
 *
 * @implemented
 */
BOOL
APIENTRY
NtGdiSetBrushOrg(HDC hDC, INT XOrg, INT YOrg, LPPOINT Point)
{
    PDC dc;
    PDC_ATTR pdcattr;

    dc = DC_LockDc(hDC);
    if (dc == NULL)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = dc->pdcattr;

    if (Point != NULL)
    {
        NTSTATUS Status = STATUS_SUCCESS;
        POINT SafePoint;
        SafePoint.x = pdcattr->ptlBrushOrigin.x;
        SafePoint.y = pdcattr->ptlBrushOrigin.y;
        _SEH2_TRY
        {
            ProbeForWrite(Point, sizeof(POINT), 1);
            *Point = SafePoint;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            DC_UnlockDc(dc);
            SetLastNtError(Status);
            return FALSE;
        }
    }

    pdcattr->ptlBrushOrigin.x = XOrg;
    pdcattr->ptlBrushOrigin.y = YOrg;
    IntptlBrushOrigin(dc, XOrg, YOrg );
    DC_UnlockDc(dc);

    return TRUE;
}

BOOL
FASTCALL
IntGdiSetBrushOwner(PBRUSH pbr, DWORD OwnerMask)
{
  HBRUSH hBR;
  PEPROCESS Owner = NULL;
  PGDI_TABLE_ENTRY pEntry = NULL;

  if (!pbr) return FALSE;

  hBR = pbr->BaseObject.hHmgr;

  if (!hBR || (GDI_HANDLE_GET_TYPE(hBR) != GDI_OBJECT_TYPE_BRUSH))
     return FALSE;
  else
  {
     INT Index = GDI_HANDLE_GET_INDEX((HGDIOBJ)hBR);
     pEntry = &GdiHandleTable->Entries[Index];
  }

  if (pbr->flAttrs & GDIBRUSH_IS_GLOBAL)
  {
     GDIOBJ_ShareUnlockObjByPtr((POBJ)pbr);
     return TRUE;
  }

  if ((OwnerMask == GDI_OBJ_HMGR_PUBLIC) || OwnerMask == GDI_OBJ_HMGR_NONE)
  {
     // Set this Brush to inaccessible mode and to an Owner of NONE.
//     if (OwnerMask == GDI_OBJ_HMGR_NONE) Owner = OwnerMask;

     if (!GDIOBJ_SetOwnership((HGDIOBJ) hBR, Owner))
        return FALSE;

     // Deny user access to User Data.
     pEntry->UserData = NULL; // This hBR is inaccessible!
  }

  if (OwnerMask == GDI_OBJ_HMGR_POWNED)
  {
     if (!GDIOBJ_SetOwnership((HGDIOBJ) hBR, PsGetCurrentProcess() ))
        return FALSE;

     // Allow user access to User Data.
     pEntry->UserData = pbr->pBrushAttr;
  }
  return TRUE;
}

/* EOF */
