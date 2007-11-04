#include "precomp.h"

#define NDEBUG
#include <debug.h>



/*
 * @implemented
 */
HPEN
APIENTRY
ExtCreatePen(DWORD dwPenStyle,
             DWORD dwWidth,
             CONST LOGBRUSH *lplb,
             DWORD dwStyleCount,
             CONST DWORD *lpStyle)
{
    /* Call NTGDI (hack... like most of gdi32..sigh) */
    return NtGdiExtCreatePen(dwPenStyle,
                             dwWidth,
                             lplb->lbStyle,
                             lplb->lbColor,
                             lplb->lbHatch,
                             0,
                             dwStyleCount,
                             (PULONG)lpStyle,
                             0,
                             FALSE,
                             NULL);
}

/*
 * @implemented
 */
HBRUSH STDCALL
CreateDIBPatternBrush(
   HGLOBAL hglbDIBPacked,
   UINT fuColorSpec)
{
   PVOID lpPackedDIB;
   HBRUSH hBrush = NULL;
   PBITMAPINFO pConvertedInfo;
   UINT ConvertedInfoSize;

   lpPackedDIB = GlobalLock(hglbDIBPacked);
   if (lpPackedDIB == NULL)
      return 0;

   pConvertedInfo = ConvertBitmapInfo((PBITMAPINFO)lpPackedDIB, fuColorSpec,
                                      &ConvertedInfoSize, TRUE);
   if (pConvertedInfo)
   {
      hBrush = NtGdiCreateDIBBrush(pConvertedInfo, fuColorSpec,
                                   ConvertedInfoSize, FALSE, FALSE, lpPackedDIB);
      if ((PBITMAPINFO)lpPackedDIB != pConvertedInfo)
         RtlFreeHeap(RtlGetProcessHeap(), 0, pConvertedInfo);
   }

   GlobalUnlock(hglbDIBPacked);

   return hBrush;
}

/*
 * @implemented
 */
HBRUSH STDCALL
CreateDIBPatternBrushPt(
   CONST VOID *lpPackedDIB,
   UINT fuColorSpec)
{
   HBRUSH hBrush = NULL;
   PBITMAPINFO pConvertedInfo;
   UINT ConvertedInfoSize;

   if (lpPackedDIB == NULL)
      return 0;

   pConvertedInfo = ConvertBitmapInfo((PBITMAPINFO)lpPackedDIB, fuColorSpec,
                                      &ConvertedInfoSize, TRUE);
   if (pConvertedInfo)
   {
      hBrush = NtGdiCreateDIBBrush(pConvertedInfo, fuColorSpec,
                                   ConvertedInfoSize, FALSE, FALSE, (PVOID)lpPackedDIB);
      if ((PBITMAPINFO)lpPackedDIB != pConvertedInfo)
         RtlFreeHeap(RtlGetProcessHeap(), 0, pConvertedInfo);
   }

   return hBrush;
}

/*
 * @implemented
 */
HBRUSH
STDCALL
CreateHatchBrush(INT fnStyle,
                 COLORREF clrref)
{
    return NtGdiCreateHatchBrushInternal(fnStyle, clrref, FALSE);
}

/*
 * @implemented
 */
HBRUSH
STDCALL
CreatePatternBrush(HBITMAP hbmp)
{
    return NtGdiCreatePatternBrushInternal(hbmp, FALSE, FALSE);
}

/*
 * @implemented
 */
HBRUSH
STDCALL
CreateSolidBrush(IN COLORREF crColor)
{
    /* Call Server-Side API */
    return NtGdiCreateSolidBrush(crColor, NULL);
}

/*
 * @implemented
 */
HBRUSH STDCALL
CreateBrushIndirect(
   CONST LOGBRUSH *LogBrush)
{
   HBRUSH hBrush;

   switch (LogBrush->lbStyle)
   {
      case BS_DIBPATTERN8X8:
      case BS_DIBPATTERN:
         hBrush = CreateDIBPatternBrush((HGLOBAL)LogBrush->lbHatch,
                                        LogBrush->lbColor);
         break;

      case BS_DIBPATTERNPT:
         hBrush = CreateDIBPatternBrushPt((PVOID)LogBrush->lbHatch,
                                          LogBrush->lbColor);
         break;

      case BS_PATTERN:
         hBrush = NtGdiCreatePatternBrushInternal((HBITMAP)LogBrush->lbHatch,
                                                  FALSE,
                                                  FALSE);
         break;

      case BS_PATTERN8X8:
         hBrush = NtGdiCreatePatternBrushInternal((HBITMAP)LogBrush->lbHatch,
                                                  FALSE,
                                                  TRUE);
         break;

      case BS_SOLID:
         hBrush = NtGdiCreateSolidBrush(LogBrush->lbColor, 0);
         break;

      case BS_HATCHED:
         hBrush = NtGdiCreateHatchBrushInternal(LogBrush->lbHatch,
                                                LogBrush->lbColor,
                                                FALSE);
         break;

      case BS_NULL:
         hBrush = NtGdiGetStockObject(NULL_BRUSH);
         break;

      default:
         SetLastError(ERROR_INVALID_PARAMETER);
         hBrush = NULL;
         break;
   }

   return hBrush;
}

BOOL
STDCALL
PatBlt(HDC hdc,
       int nXLeft,
       int nYLeft,
       int nWidth,
       int nHeight,
       DWORD dwRop)
{
    /* FIXME some part need be done in user mode */
    return NtGdiPatBlt( hdc,  nXLeft,  nYLeft,  nWidth,  nHeight,  dwRop);
}

BOOL
STDCALL
PolyPatBlt(IN HDC hdc,
           IN DWORD rop4,
           IN PPOLYPATBLT pPoly,
           IN DWORD Count,
           IN DWORD Mode)
{
    /* FIXME some part need be done in user mode */
    return NtGdiPolyPatBlt(hdc, rop4, pPoly,Count,Mode);
}


/*
 * @implemented
 */
int
STDCALL
SetROP2(HDC hdc,
        int fnDrawMode)
{
  PDC_ATTR Dc_Attr;
  INT Old_ROP2;
  
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_SetROP2( hdc, fnDrawMode);
    else
    {
      PLDC pLDC = GdiGetLDC(hdc);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_SetROP2(( hdc, fnDrawMode);
      }
      return FALSE;
    }
 }
#endif
 if (!GdiGetHandleUserData((HGDIOBJ) hdc, (PVOID) &Dc_Attr)) return FALSE;

 if (NtCurrentTeb()->GdiTebBatch.HDC == (ULONG) hdc)
 {
    if (Dc_Attr->ulDirty_ & DC_MODE_DIRTY)
    {
       NtGdiFlush();
       Dc_Attr->ulDirty_ &= ~DC_MODE_DIRTY;
    }
 }

 Old_ROP2 = Dc_Attr->jROP2;
 Dc_Attr->jROP2 = fnDrawMode;

 return Old_ROP2;
}

/*
 * @implemented
 */
BOOL
STDCALL
SetBrushOrgEx(HDC hdc,
              int nXOrg,
              int nYOrg,
              LPPOINT lppt)
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
 {
    PLDC pLDC = GdiGetLDC(hdc);
    if ( (pLDC == NULL) || (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }
    if (pLDC->iType == LDC_EMFLDC)
    {
      return EMFDRV_SetBrushOrg(hdc, nXOrg, nYOrg); // ReactOS only.
    }
    return FALSE;
 }
#endif
#if 0
  PDC_ATTR Dc_Attr;

 if (GdiGetHandleUserData((HGDIOBJ) hdc, (PVOID) &Dc_Attr))
 {
    PTEB pTeb = NtCurrentTeb();
    if (lppt)
    {
       lppt->x = Dc_Attr->ptlBrushOrigin.x;
       lppt->y = Dc_Attr->ptlBrushOrigin.y;
    }
    if ((nXOrg == Dc_Attr->ptlBrushOrigin.x) && (nYOrg == Dc_Attr->ptlBrushOrigin.y))
       return TRUE;

    if(((pTeb->GdiTebBatch.HDC == 0) || (pTeb->GdiTebBatch.HDC == (ULONG)hdc)) &&
       ((pTeb->GdiTebBatch.Offset + sizeof(GDIBSSETBRHORG)) <= GDIBATCHBUFSIZE) &&
       (!(Dc_Attr->ulDirty_ & DC_DIBSECTION)) )
    {
       PGDIBSSETBRHORG pgSBO = (PGDIBSSETBRHORG)(&pTeb->GdiTebBatch.Buffer[0] +
                                                      pTeb->GdiTebBatch.Offset);

       Dc_Attr->ptlBrushOrigin.x = nXOrg;
       Dc_Attr->ptlBrushOrigin.y = nYOrg;

       pgSBO->gbHdr.Cmd = GdiBCSetBrushOrg;
       pgSBO->gbHdr.Size = sizeof(GDIBSSETBRHORG);
       pgSBO->ptlBrushOrigin = Dc_Attr->ptlBrushOrigin;
       
       pTeb->GdiTebBatch.Offset += sizeof(GDIBSSETBRHORG);
       pTeb->GdiTebBatch.HDC = (ULONG)hdc;
       pTeb->GdiBatchCount++;
       if (pTeb->GdiBatchCount >= GDI_BatchLimit) NtGdiFlush();

       return TRUE;
    }
 }
#endif
 return NtGdiSetBrushOrg(hdc,nXOrg,nYOrg,lppt);
}

