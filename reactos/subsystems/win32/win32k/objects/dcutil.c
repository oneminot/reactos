
#include <w32k.h>

#define NDEBUG
#include <debug.h>

static
VOID
CopytoUserDcAttr(PDC dc, PDC_ATTR Dc_Attr, FLONG Dirty)
{
      Dc_Attr->hpen              = dc->Dc_Attr.hpen;
      Dc_Attr->hbrush            = dc->Dc_Attr.hbrush;
      Dc_Attr->hColorSpace       = dc->Dc_Attr.hColorSpace;
      Dc_Attr->hlfntNew          = dc->Dc_Attr.hlfntNew;

      Dc_Attr->jROP2             = dc->Dc_Attr.jROP2;
      Dc_Attr->jFillMode         = dc->Dc_Attr.jFillMode;
      Dc_Attr->jStretchBltMode   = dc->Dc_Attr.jStretchBltMode;
      Dc_Attr->lRelAbs           = dc->Dc_Attr.lRelAbs;
      Dc_Attr->jBkMode           = dc->Dc_Attr.jBkMode;

      Dc_Attr->crBackgroundClr   = dc->Dc_Attr.crBackgroundClr;
      Dc_Attr->ulBackgroundClr   = dc->Dc_Attr.ulBackgroundClr;
      Dc_Attr->crForegroundClr   = dc->Dc_Attr.crForegroundClr;
      Dc_Attr->ulForegroundClr   = dc->Dc_Attr.ulForegroundClr;

      Dc_Attr->ulBrushClr        = dc->Dc_Attr.ulBrushClr;
      Dc_Attr->crBrushClr        = dc->Dc_Attr.crBrushClr;

      Dc_Attr->ulPenClr          = dc->Dc_Attr.ulPenClr;
      Dc_Attr->crPenClr          = dc->Dc_Attr.crPenClr;

      Dc_Attr->ptlBrushOrigin.x  = dc->Dc_Attr.ptlBrushOrigin.x;
      Dc_Attr->ptlBrushOrigin.y  = dc->Dc_Attr.ptlBrushOrigin.y;

      Dc_Attr->lTextAlign        = dc->Dc_Attr.lTextAlign;
      Dc_Attr->lTextExtra        = dc->Dc_Attr.lTextExtra;
      Dc_Attr->cBreak            = dc->Dc_Attr.cBreak;
      Dc_Attr->lBreakExtra       = dc->Dc_Attr.lBreakExtra;
      Dc_Attr->iMapMode          = dc->Dc_Attr.iMapMode;
      Dc_Attr->iGraphicsMode     = dc->Dc_Attr.iGraphicsMode;

      Dc_Attr->ptlCurrent.x      = dc->Dc_Attr.ptlCurrent.x;
      Dc_Attr->ptlCurrent.y      = dc->Dc_Attr.ptlCurrent.y;
      Dc_Attr->ptlWindowOrg.x    = dc->Dc_Attr.ptlWindowOrg.x;
      Dc_Attr->ptlWindowOrg.y    = dc->Dc_Attr.ptlWindowOrg.y;
      Dc_Attr->szlWindowExt.cx   = dc->Dc_Attr.szlWindowExt.cx;
      Dc_Attr->szlWindowExt.cy   = dc->Dc_Attr.szlWindowExt.cy;
      Dc_Attr->ptlViewportOrg.x  = dc->Dc_Attr.ptlViewportOrg.x;
      Dc_Attr->ptlViewportOrg.y  = dc->Dc_Attr.ptlViewportOrg.y;
      Dc_Attr->szlViewportExt.cx = dc->Dc_Attr.szlViewportExt.cx;
      Dc_Attr->szlViewportExt.cy = dc->Dc_Attr.szlViewportExt.cy;

      Dc_Attr->ulDirty_          = dc->Dc_Attr.ulDirty_; //Copy flags! We may have set them.
      
      XForm2MatrixS( &Dc_Attr->mxWorldToDevice, &dc->w.xformWorld2Vport);
      XForm2MatrixS( &Dc_Attr->mxDevicetoWorld, &dc->w.xformVport2World);
      XForm2MatrixS( &Dc_Attr->mxWorldToPage, &dc->w.xformWorld2Wnd);
}

static
VOID
CopyFromUserDcAttr(PDC dc, PDC_ATTR Dc_Attr, FLONG Dirty)
{
      if ( (Dirty & DIRTY_FILL) || (Dc_Attr->ulDirty_ & DIRTY_FILL))
      {
        dc->Dc_Attr.ulBrushClr     = Dc_Attr->ulBrushClr;
        dc->Dc_Attr.crBrushClr     = Dc_Attr->crBrushClr;
        Dc_Attr->ulDirty_ &= ~DIRTY_FILL;
      }
      if ( Dirty & DIRTY_LINE || (Dc_Attr->ulDirty_ & DIRTY_LINE))
      {
        dc->Dc_Attr.crBackgroundClr = Dc_Attr->crBackgroundClr;
        dc->Dc_Attr.ulBackgroundClr = Dc_Attr->ulBackgroundClr;
        dc->Dc_Attr.ulPenClr        = Dc_Attr->ulPenClr;
        dc->Dc_Attr.crPenClr        = Dc_Attr->crPenClr;
        Dc_Attr->ulDirty_ &= ~DIRTY_LINE;
      }
      if ( Dirty & DIRTY_TEXT || (Dc_Attr->ulDirty_ & DIRTY_TEXT))
      {
        dc->Dc_Attr.crForegroundClr = Dc_Attr->crForegroundClr;
        dc->Dc_Attr.ulForegroundClr = Dc_Attr->ulForegroundClr;
        Dc_Attr->ulDirty_ &= ~DIRTY_TEXT;
      }

      if ( Dirty & (DC_MODE_DIRTY|DC_FONTTEXT_DIRTY) ||
          (Dc_Attr->ulDirty_ & (DC_MODE_DIRTY|DC_FONTTEXT_DIRTY)))
      {
        dc->Dc_Attr.jROP2         = Dc_Attr->jROP2;
        dc->Dc_Attr.iGraphicsMode = Dc_Attr->iGraphicsMode;
        dc->Dc_Attr.lFillMode     = Dc_Attr->lFillMode;
        dc->Dc_Attr.flFontMapper  = Dc_Attr->flFontMapper;
        dc->Dc_Attr.lBreakExtra   = Dc_Attr->lBreakExtra;
        dc->Dc_Attr.cBreak        = Dc_Attr->cBreak;
      }
}

static
BOOL
ReadWriteVMDcAttr(PDC dc, FLONG Dirty, BOOL Write)
{
  BOOL Ret = FALSE;
  KeEnterCriticalRegion();
    {
      INT Index = GDI_HANDLE_GET_INDEX(dc->hHmgr);
      PGDI_TABLE_ENTRY Entry = &GdiHandleTable->Entries[Index];
      HANDLE ProcessId = (HANDLE)(((ULONG_PTR)(Entry->ProcessId)) & ~1);
      DC_ATTR lDc_AttrData;

      if(Entry->UserData)
      {
         NTSTATUS Status = ZwReadVirtualMemory ( ProcessId,
                                         &(Entry->UserData),
                                             &lDc_AttrData,
                                            sizeof(DC_ATTR),
                                                      NULL );
         if (Write)
         {
            if (NT_SUCCESS(Status)) CopytoUserDcAttr(dc, &lDc_AttrData, Dirty);
            Ret = TRUE;
         }
         else
         {
            if (NT_SUCCESS(Status)) CopyFromUserDcAttr(dc, &lDc_AttrData, Dirty);
            Ret = TRUE;
         }
         if (Write)
             Status = ZwWriteVirtualMemory ( ProcessId,
                                     &(Entry->UserData),
                                         &lDc_AttrData,
                                        sizeof(DC_ATTR),
                                                  NULL );
         if(!NT_SUCCESS(Status))
         {
           SetLastNtError(Status);
           Ret = FALSE;
         }
      }
    }
  KeLeaveCriticalRegion();
  return Ret;
}


BOOL
FASTCALL
DCU_UpdateUserXForms(PDC pDC, ULONG uMask)
{
  PDC_ATTR DC_Attr = pDC->pDc_Attr;

  if (!uMask) return FALSE;

  if (!DC_Attr) return FALSE;
  else
  {
    NTSTATUS Status = STATUS_SUCCESS;
    KeEnterCriticalRegion();
    _SEH_TRY
    {
      ProbeForWrite(DC_Attr,
            sizeof(DC_ATTR),
                          1);
    if (uMask & WORLD_XFORM_CHANGED)
      XForm2MatrixS( &DC_Attr->mxWorldToDevice, &pDC->w.xformWorld2Vport);

    if (uMask & DEVICE_TO_WORLD_INVALID)
      XForm2MatrixS( &DC_Attr->mxDevicetoWorld, &pDC->w.xformVport2World);

    if (uMask & WORLD_TO_PAGE_IDENTITY)
      XForm2MatrixS( &DC_Attr->mxWorldToPage, &pDC->w.xformWorld2Wnd);
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    KeLeaveCriticalRegion();
    if(!NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return FALSE;
    }
  }
  return TRUE;
}

BOOL
FASTCALL
DCU_SyncDcAttrtoUser(PDC dc, FLONG Dirty)
{
  BOOL TryHarder = FALSE;
  PDC_ATTR Dc_Attr = dc->pDc_Attr;
  if (!Dirty) return FALSE;
  if (!Dc_Attr) return FALSE;
  else
  {
    NTSTATUS Status = STATUS_SUCCESS;
    KeEnterCriticalRegion();
    _SEH_TRY
    {
      ProbeForWrite(Dc_Attr,
            sizeof(DC_ATTR),
                          1);
      CopytoUserDcAttr( dc, Dc_Attr, Dirty);
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    KeLeaveCriticalRegion();
    if(!NT_SUCCESS(Status)) TryHarder = TRUE;
    if (TryHarder) return ReadWriteVMDcAttr( dc, Dirty, TRUE);
  }
  return TRUE;
}

BOOL
FASTCALL
DCU_SynchDcAttrtoUser(HDC hDC, FLONG Dirty)
{
  PDC pDC = DC_LockDc ( hDC );
  if (!pDC) return FALSE;
  BOOL Ret = DCU_SyncDcAttrtoUser(pDC, Dirty);
  DC_UnlockDc( pDC );
  return Ret;
}

BOOL
FASTCALL
DCU_SyncDcAttrtoW32k(PDC dc, FLONG Dirty)
{
  BOOL TryHarder = FALSE;
  PDC_ATTR Dc_Attr = dc->pDc_Attr;
  if (!Dirty) return FALSE;
  if (!Dc_Attr) return FALSE;
  else
  {
    NTSTATUS Status = STATUS_SUCCESS;
    KeEnterCriticalRegion();
    _SEH_TRY
    {
      ProbeForRead(Dc_Attr,
            sizeof(DC_ATTR),
                         1);
      CopyFromUserDcAttr( dc, Dc_Attr, Dirty);
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    KeLeaveCriticalRegion();
    if(!NT_SUCCESS(Status)) TryHarder = TRUE;
    if (TryHarder) return ReadWriteVMDcAttr( dc, Dirty, FALSE);
  }
  return TRUE;
}

BOOL
FASTCALL
DCU_SynchDcAttrtoW32k(HDC hDC, FLONG Dirty)
{
  PDC pDC = DC_LockDc ( hDC );
  if (!pDC) return FALSE;
  BOOL Ret = DCU_SyncDcAttrtoW32k(pDC, Dirty);
  DC_UnlockDc( pDC );
  return Ret;
}

//
//
// Gdi Batch Flush support functions.
//


//
// Process the batch.
//
ULONG
FASTCALL
GdiFlushUserBatch(HDC hDC, PGDIBATCHHDR pHdr)
{
  switch(pHdr->Cmd)
  {
     case GdiBCPatBlt: // Highest pri first!
     case GdiBCPolyPatBlt:
     case GdiBCTextOut:
     case GdiBCExtTextOut:
     case GdiBCSetBrushOrg:
     case GdiBCExtSelClipRgn:
     case GdiBCSelObj:
     case GdiBCDelObj:
     case GdiBCDelRgn:
     default:
       return 0;
  }
  return pHdr->Size; // Return the full size of the structure.
}

/*
 * NtGdiFlush
 *
 * Flushes the calling thread's current batch.
 */
VOID
APIENTRY
NtGdiFlush(VOID)
{
  UNIMPLEMENTED;
}

/*
 * NtGdiFlushUserBatch
 *
 * Callback for thread batch flush routine.
 *
 * Think small & fast!
 */
NTSTATUS
APIENTRY
NtGdiFlushUserBatch(VOID)
{
  PTEB pTeb = NtCurrentTeb();
  ULONG GdiBatchCount = pTeb->GdiBatchCount;
  
  if( (GdiBatchCount > 0) && (GdiBatchCount <= GDIBATCHBUFSIZE))
  {
    HDC hDC = (HDC) pTeb->GdiTebBatch.HDC;
    if (hDC)
    {
       PULONG pHdr = &pTeb->GdiTebBatch.Buffer[0];
       // No need to init anything, just go!
       for (; GdiBatchCount > 0; GdiBatchCount--)
       {
           // Process Gdi Batch!
           pHdr += GdiFlushUserBatch( hDC, (PGDIBATCHHDR) pHdr );
       }
       // Exit and clear out for the next round.
       pTeb->GdiTebBatch.Offset = 0;
       pTeb->GdiBatchCount = 0;
       pTeb->GdiTebBatch.HDC = 0;
    }
  }
  return STATUS_SUCCESS;
}

