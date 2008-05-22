/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Coordinate systems
 * FILE:             subsys/win32k/objects/coord.c
 * PROGRAMER:        Unknown
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

void FASTCALL
IntFixIsotropicMapping(PDC dc)
{
  ULONG xdim;
  ULONG ydim;
  PDC_ATTR Dc_Attr = dc->pDc_Attr;
  if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;

  xdim = EngMulDiv(Dc_Attr->szlViewportExt.cx,
               ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulHorzSize,
               ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulHorzRes) /
                                Dc_Attr->szlWindowExt.cx;
  ydim = EngMulDiv(Dc_Attr->szlViewportExt.cy,
               ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulVertSize,
               ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulVertRes) /
                                Dc_Attr->szlWindowExt.cy;

  if (xdim > ydim)
  {
    Dc_Attr->szlViewportExt.cx = Dc_Attr->szlViewportExt.cx * abs(ydim / xdim);
    if (!Dc_Attr->szlViewportExt.cx) Dc_Attr->szlViewportExt.cx = 1;
  }
  else
  {
    Dc_Attr->szlViewportExt.cy = Dc_Attr->szlViewportExt.cy * abs(xdim / ydim);
    if (!Dc_Attr->szlViewportExt.cy) Dc_Attr->szlViewportExt.cy = 1;
  }
}

BOOL FASTCALL
IntGdiCombineTransform(LPXFORM XFormResult,
                       LPXFORM xform1,
                       LPXFORM xform2)
{
  /* Check for illegal parameters */
  if (!XFormResult || !xform1 || !xform2)
  {
    return  FALSE;
  }

  /* Create the result in a temporary XFORM, since xformResult may be
   * equal to xform1 or xform2 */
  XFormResult->eM11 = xform1->eM11 * xform2->eM11 + xform1->eM12 * xform2->eM21;
  XFormResult->eM12 = xform1->eM11 * xform2->eM12 + xform1->eM12 * xform2->eM22;
  XFormResult->eM21 = xform1->eM21 * xform2->eM11 + xform1->eM22 * xform2->eM21;
  XFormResult->eM22 = xform1->eM21 * xform2->eM12 + xform1->eM22 * xform2->eM22;
  XFormResult->eDx  = xform1->eDx  * xform2->eM11 + xform1->eDy  * xform2->eM21 + xform2->eDx;
  XFormResult->eDy  = xform1->eDx  * xform2->eM12 + xform1->eDy  * xform2->eM22 + xform2->eDy;

  return TRUE;
}

BOOL STDCALL NtGdiCombineTransform(LPXFORM  UnsafeXFormResult,
                                   LPXFORM  Unsafexform1,
                                   LPXFORM  Unsafexform2)
{
  XFORM  xformTemp;
  XFORM  xform1 = {0}, xform2 = {0};
  NTSTATUS Status = STATUS_SUCCESS;
  BOOL Ret;

  _SEH_TRY
  {
    ProbeForWrite(UnsafeXFormResult,
                  sizeof(XFORM),
                  1);
    ProbeForRead(Unsafexform1,
                 sizeof(XFORM),
                 1);
    ProbeForRead(Unsafexform2,
                 sizeof(XFORM),
                 1);
    xform1 = *Unsafexform1;
    xform2 = *Unsafexform2;
  }
  _SEH_HANDLE
  {
    Status = _SEH_GetExceptionCode();
  }
  _SEH_END;

  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }

  Ret = IntGdiCombineTransform(&xformTemp, &xform1, &xform2);

  /* Copy the result to xformResult */
  _SEH_TRY
  {
    /* pointer was already probed! */
    *UnsafeXFormResult = xformTemp;
  }
  _SEH_HANDLE
  {
    Status = _SEH_GetExceptionCode();
  }
  _SEH_END;

  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }

  return Ret;
}

VOID FASTCALL
CoordDPtoLP(PDC Dc, LPPOINT Point)
{
FLOAT x, y;
  x = (FLOAT)Point->x;
  y = (FLOAT)Point->y;
  Point->x = x * Dc->w.xformVport2World.eM11 +
    y * Dc->w.xformVport2World.eM21 + Dc->w.xformVport2World.eDx;
  Point->y = x * Dc->w.xformVport2World.eM12 +
    y * Dc->w.xformVport2World.eM22 + Dc->w.xformVport2World.eDy;
}

VOID
FASTCALL
IntDPtoLP ( PDC dc, LPPOINT Points, INT Count )
{
  INT i;

  ASSERT ( Points );

  for ( i = 0; i < Count; i++ )
    CoordDPtoLP ( dc, &Points[i] );
}

int
FASTCALL
IntGetGraphicsMode ( PDC dc )
{
  PDC_ATTR Dc_Attr;
  ASSERT ( dc );
  Dc_Attr = dc->pDc_Attr;
  if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;
  return Dc_Attr->iGraphicsMode;
}

BOOL
FASTCALL
IntGdiModifyWorldTransform(PDC pDc,
                           CONST LPXFORM lpXForm,
                           DWORD Mode)
{
   ASSERT(pDc && lpXForm);

   switch(Mode)
   {
     case MWT_IDENTITY:
       pDc->w.xformWorld2Wnd.eM11 = 1.0f;
       pDc->w.xformWorld2Wnd.eM12 = 0.0f;
       pDc->w.xformWorld2Wnd.eM21 = 0.0f;
       pDc->w.xformWorld2Wnd.eM22 = 1.0f;
       pDc->w.xformWorld2Wnd.eDx  = 0.0f;
       pDc->w.xformWorld2Wnd.eDy  = 0.0f;
       break;

     case MWT_LEFTMULTIPLY:
       IntGdiCombineTransform(&pDc->w.xformWorld2Wnd, lpXForm, &pDc->w.xformWorld2Wnd );
       break;

     case MWT_RIGHTMULTIPLY:
       IntGdiCombineTransform(&pDc->w.xformWorld2Wnd, &pDc->w.xformWorld2Wnd, lpXForm);
       break;

     case MWT_MAX+1: // Must be MWT_SET????
       pDc->w.xformWorld2Wnd = *lpXForm; // Do it like Wine.
       break;

     default:
       SetLastWin32Error(ERROR_INVALID_PARAMETER);
       return FALSE;
  }
  DC_UpdateXforms(pDc);
  return TRUE;
}

BOOL
STDCALL
NtGdiGetTransform(HDC  hDC,
               DWORD iXform,
              LPXFORM  XForm)
{
  PDC  dc;
  NTSTATUS Status = STATUS_SUCCESS;

  dc = DC_LockDc ( hDC );
  if (!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  if (!XForm)
  {
    DC_UnlockDc(dc);
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  _SEH_TRY
  {
    ProbeForWrite(XForm,
                  sizeof(XFORM),
                  1);
   switch(iXform)
   {
     case GdiWorldSpaceToPageSpace:
        *XForm = dc->w.xformWorld2Wnd;
     break;
     default:
     break;
   }
  }
  _SEH_HANDLE
  {
    Status = _SEH_GetExceptionCode();
  }
  _SEH_END;

  DC_UnlockDc(dc);
  return NT_SUCCESS(Status);
}

VOID
FASTCALL
CoordLPtoDP ( PDC Dc, LPPOINT Point )
{
  FLOAT x, y;

  ASSERT ( Dc );
  ASSERT ( Point );

  x = (FLOAT)Point->x;
  y = (FLOAT)Point->y;
  Point->x = x * Dc->w.xformWorld2Vport.eM11 +
    y * Dc->w.xformWorld2Vport.eM21 + Dc->w.xformWorld2Vport.eDx;
  Point->y = x * Dc->w.xformWorld2Vport.eM12 +
    y * Dc->w.xformWorld2Vport.eM22 + Dc->w.xformWorld2Vport.eDy;
}

VOID
FASTCALL
IntLPtoDP ( PDC dc, LPPOINT Points, INT Count )
{
  INT i;

  ASSERT ( Points );

  for ( i = 0; i < Count; i++ )
    CoordLPtoDP ( dc, &Points[i] );
}

/*!
 * Converts points from logical coordinates into device coordinates. Conversion depends on the mapping mode,
 * world transfrom, viewport origin settings for the given device context.
 * \param	hDC		device context.
 * \param	Points	an array of POINT structures (in/out).
 * \param	Count	number of elements in the array of POINT structures.
 * \return  TRUE 	if success.
*/
BOOL
APIENTRY
NtGdiTransformPoints( HDC hDC,
                      PPOINT UnsafePtsIn,
                      PPOINT UnsafePtOut,
                      INT Count,
                      INT iMode )
{
   PDC dc;
   NTSTATUS Status = STATUS_SUCCESS;
   LPPOINT Points;
   ULONG Size;

   dc = DC_LockDc(hDC);
   if (!dc)
   {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return FALSE;
   }

   if (!UnsafePtsIn || !UnsafePtOut || Count <= 0)
   {
     DC_UnlockDc(dc);
     SetLastWin32Error(ERROR_INVALID_PARAMETER);
     return FALSE;
   }

   Size = Count * sizeof(POINT);

   Points = (LPPOINT)ExAllocatePoolWithTag(PagedPool, Size, TAG_COORD);
   if(!Points)
   {
     DC_UnlockDc(dc);
     SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
     return FALSE;
   }

   _SEH_TRY
   {
      ProbeForWrite(UnsafePtOut,
                    Size,
                    1);
      ProbeForRead(UnsafePtsIn,
                    Size,
                    1);
      RtlCopyMemory(Points,
                    UnsafePtsIn,
                    Size);
   }
   _SEH_HANDLE
   {
      Status = _SEH_GetExceptionCode();
   }
   _SEH_END;

   if(!NT_SUCCESS(Status))
   {
     DC_UnlockDc(dc);
     ExFreePool(Points);
     SetLastNtError(Status);
     return FALSE;
   }

   switch (iMode)
   {
      case GdiDpToLp:
        IntDPtoLP(dc, Points, Count);
        break;
      case GdiLpToDp:
        IntLPtoDP(dc, Points, Count);
        break;
      case 2: // Not supported yet. Need testing.
      default:
      {
        DC_UnlockDc(dc);
        ExFreePool(Points);
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return FALSE;
      }
   }

   _SEH_TRY
   {
      /* pointer was already probed! */
      RtlCopyMemory(UnsafePtOut,
                    Points,
                    Size);
   }
   _SEH_HANDLE
   {
      Status = _SEH_GetExceptionCode();
   }
   _SEH_END;

   if(!NT_SUCCESS(Status))
   {
     DC_UnlockDc(dc);
     ExFreePool(Points);
     SetLastNtError(Status);
     return FALSE;
   }
//
// If we are getting called that means User XForms is a mess!
//
   DC_UnlockDc(dc);
   ExFreePool(Points);
   return TRUE;
}

BOOL
STDCALL
NtGdiModifyWorldTransform(HDC hDC,
                          LPXFORM  UnsafeXForm,
                          DWORD Mode)
{
   PDC dc;
   XFORM SafeXForm;
   BOOL Ret = FALSE;

   if (!UnsafeXForm)
   {
     SetLastWin32Error(ERROR_INVALID_PARAMETER);
     return FALSE;
   }

   dc = DC_LockDc(hDC);
   if (!dc)
   {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return FALSE;
   }

   _SEH_TRY
   {
      ProbeForRead(UnsafeXForm, sizeof(XFORM), 1);
      RtlCopyMemory(&SafeXForm, UnsafeXForm, sizeof(XFORM));
   }
   _SEH_HANDLE
   {
      SetLastNtError(_SEH_GetExceptionCode());
   }
   _SEH_END;

   // Safe to handle kernel mode data.
   Ret = IntGdiModifyWorldTransform(dc, &SafeXForm, Mode);
   DC_UnlockDc(dc);
   return Ret;
}

BOOL
STDCALL
NtGdiOffsetViewportOrgEx(HDC hDC,
                        int XOffset,
                        int YOffset,
                        LPPOINT UnsafePoint)
{
  PDC      dc;
  PDC_ATTR Dc_Attr;
  NTSTATUS Status = STATUS_SUCCESS;

  dc = DC_LockDc ( hDC );
  if(!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  Dc_Attr = dc->pDc_Attr;
  if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;
  
  if (UnsafePoint)
    {
        _SEH_TRY
        {
            ProbeForWrite(UnsafePoint,
                          sizeof(POINT),
                          1);
            UnsafePoint->x = Dc_Attr->ptlViewportOrg.x;
            UnsafePoint->y = Dc_Attr->ptlViewportOrg.y;
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

	if ( !NT_SUCCESS(Status) )
	  {
	    SetLastNtError(Status);
	    DC_UnlockDc(dc);
	    return FALSE;
	  }
    }

  Dc_Attr->ptlViewportOrg.x += XOffset;
  Dc_Attr->ptlViewportOrg.y += YOffset;
  DC_UpdateXforms(dc);
  DC_UnlockDc(dc);
  return TRUE;
}

BOOL
STDCALL
NtGdiOffsetWindowOrgEx(HDC  hDC,
                      int  XOffset,
                      int  YOffset,
                      LPPOINT  Point)
{
  PDC dc;
  PDC_ATTR Dc_Attr;
  
  dc = DC_LockDc(hDC);
  if (!dc)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }
  Dc_Attr = dc->pDc_Attr;
  if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;

  if (Point)
    {
      NTSTATUS Status = STATUS_SUCCESS;

      _SEH_TRY
      {
         ProbeForWrite(Point,
                       sizeof(POINT),
                       1);
         Point->x = Dc_Attr->ptlWindowOrg.x;
         Point->y = Dc_Attr->ptlWindowOrg.y;
      }
      _SEH_HANDLE
      {
         Status = _SEH_GetExceptionCode();
      }
      _SEH_END;

      if(!NT_SUCCESS(Status))
      {
        SetLastNtError(Status);
        DC_UnlockDc(dc);
        return FALSE;
      }
    }

  Dc_Attr->ptlWindowOrg.x += XOffset;
  Dc_Attr->ptlWindowOrg.y += YOffset;

  DC_UpdateXforms(dc);
  DC_UnlockDc(dc);

  return TRUE;
}

BOOL
STDCALL
NtGdiScaleViewportExtEx(HDC  hDC,
                       int  Xnum,
                     int  Xdenom,
                       int  Ynum,
                     int  Ydenom,
                    LPSIZE pSize)
{
  PDC pDC;
  PDC_ATTR pDc_Attr;
  BOOL Ret = FALSE;
  LONG X, Y;
  
  pDC = DC_LockDc(hDC);
  if (!pDC)
  {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
  }
  pDc_Attr = pDC->pDc_Attr;
  if(!pDc_Attr) pDc_Attr = &pDC->Dc_Attr;

  if ( pSize )
  {
     NTSTATUS Status = STATUS_SUCCESS;

     _SEH_TRY
     {
       ProbeForWrite(pSize,
            sizeof(LPSIZE),
                         1);

       pSize->cx = pDc_Attr->szlViewportExt.cx;
       pSize->cy = pDc_Attr->szlViewportExt.cy;
     }
     _SEH_HANDLE
     {
         Status = _SEH_GetExceptionCode();
     }
     _SEH_END;

     if(!NT_SUCCESS(Status))
     {
        SetLastNtError(Status);
        DC_UnlockDc(pDC);
        return FALSE;
     }
  }

  if (pDc_Attr->iMapMode > MM_TWIPS)
  { 
     if ( ( Xdenom ) && ( Ydenom ) )
     {
        X = Xnum * pDc_Attr->szlViewportExt.cx / Xdenom;
        if ( X )
        {
           Y = Ynum * pDc_Attr->szlViewportExt.cy / Ydenom;
           if ( Y )
           {
              pDc_Attr->szlViewportExt.cx = X; 
              pDc_Attr->szlViewportExt.cy = Y;

              IntMirrorWindowOrg(pDC);
              
              pDc_Attr->flXform |= (PAGE_EXTENTS_CHANGED|INVALIDATE_ATTRIBUTES|DEVICE_TO_WORLD_INVALID);

              if (pDc_Attr->iMapMode == MM_ISOTROPIC) IntFixIsotropicMapping(pDC);
              DC_UpdateXforms(pDC);

              Ret = TRUE;
           }
        }
     }
  }
  else
    Ret = TRUE;

  DC_UnlockDc(pDC);
  return Ret;
}

BOOL
STDCALL
NtGdiScaleWindowExtEx(HDC  hDC,
                     int  Xnum,
                   int  Xdenom,
                     int  Ynum,
                   int  Ydenom,
                  LPSIZE pSize)
{
  PDC pDC;
  PDC_ATTR pDc_Attr;
  BOOL Ret = FALSE;
  LONG X, Y;
  
  pDC = DC_LockDc(hDC);
  if (!pDC)
  {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
  }
  pDc_Attr = pDC->pDc_Attr;
  if(!pDc_Attr) pDc_Attr = &pDC->Dc_Attr;

  if ( pSize )
  {
     NTSTATUS Status = STATUS_SUCCESS;

     _SEH_TRY
     {
       ProbeForWrite(pSize,
            sizeof(LPSIZE),
                         1);

       X = pDc_Attr->szlWindowExt.cx;
       if (pDc_Attr->dwLayout & LAYOUT_RTL) X = -X;
       pSize->cx = X;
       pSize->cy = pDc_Attr->szlWindowExt.cy;
     }
     _SEH_HANDLE
     {
         Status = _SEH_GetExceptionCode();
     }
     _SEH_END;

     if(!NT_SUCCESS(Status))
     {
        SetLastNtError(Status);
        DC_UnlockDc(pDC);
        return FALSE;
     }
  }

  if (pDc_Attr->iMapMode > MM_TWIPS)
  { 
     if (( Xdenom ) && ( Ydenom ))
     {
        X = Xnum * pDc_Attr->szlWindowExt.cx / Xdenom;
        if ( X )
        {
           Y = Ynum * pDc_Attr->szlWindowExt.cy / Ydenom;
           if ( Y )
           {
              pDc_Attr->szlWindowExt.cx = X;
              pDc_Attr->szlWindowExt.cy = Y;

              IntMirrorWindowOrg(pDC);
              
              pDc_Attr->flXform |= (PAGE_EXTENTS_CHANGED|INVALIDATE_ATTRIBUTES|DEVICE_TO_WORLD_INVALID);

              if (pDc_Attr->iMapMode == MM_ISOTROPIC) IntFixIsotropicMapping(pDC);
              DC_UpdateXforms(pDC);

              Ret = TRUE;
           }
        }
     }
  }
  else
    Ret = TRUE;

  DC_UnlockDc(pDC);
  return Ret;
}

int
STDCALL
IntGdiSetMapMode(PDC  dc,
                int  MapMode)
{
  int PrevMapMode;
  PDC_ATTR Dc_Attr = dc->pDc_Attr;
  if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;

  PrevMapMode = Dc_Attr->iMapMode;

  if (MapMode != Dc_Attr->iMapMode || (MapMode != MM_ISOTROPIC && MapMode != MM_ANISOTROPIC))
  {
    Dc_Attr->iMapMode = MapMode;

    switch (MapMode)
    {
      case MM_TEXT:
        Dc_Attr->szlWindowExt.cx = 1;
        Dc_Attr->szlWindowExt.cy = 1;
        Dc_Attr->szlViewportExt.cx = 1;
        Dc_Attr->szlViewportExt.cy = 1;
        break;

      case MM_LOMETRIC:
      case MM_ISOTROPIC:
        Dc_Attr->szlWindowExt.cx = ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulHorzSize * 10;
        Dc_Attr->szlWindowExt.cy = ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulVertSize * 10;
        Dc_Attr->szlViewportExt.cx = ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulHorzRes;
        Dc_Attr->szlViewportExt.cy = -((PGDIDEVICE)dc->pPDev)->GDIInfo.ulVertRes;
        break;

      case MM_HIMETRIC:
        Dc_Attr->szlWindowExt.cx = ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulHorzSize * 100;
        Dc_Attr->szlWindowExt.cy = ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulVertSize * 100;
        Dc_Attr->szlViewportExt.cx = ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulHorzRes;
        Dc_Attr->szlViewportExt.cy = -((PGDIDEVICE)dc->pPDev)->GDIInfo.ulVertRes;
        break;

      case MM_LOENGLISH:
        Dc_Attr->szlWindowExt.cx = EngMulDiv(1000, ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulHorzSize, 254);
        Dc_Attr->szlWindowExt.cy = EngMulDiv(1000, ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulVertSize, 254);
        Dc_Attr->szlViewportExt.cx = ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulHorzRes;
        Dc_Attr->szlViewportExt.cy = -((PGDIDEVICE)dc->pPDev)->GDIInfo.ulVertRes;
        break;

      case MM_HIENGLISH:
        Dc_Attr->szlWindowExt.cx = EngMulDiv(10000, ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulHorzSize, 254);
        Dc_Attr->szlWindowExt.cy = EngMulDiv(10000, ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulVertSize, 254);
        Dc_Attr->szlViewportExt.cx = ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulHorzRes;
        Dc_Attr->szlViewportExt.cy = -((PGDIDEVICE)dc->pPDev)->GDIInfo.ulVertRes;
        break;

      case MM_TWIPS:
        Dc_Attr->szlWindowExt.cx = EngMulDiv(14400, ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulHorzSize, 254);
        Dc_Attr->szlWindowExt.cy = EngMulDiv(14400, ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulVertSize, 254);
        Dc_Attr->szlViewportExt.cx = ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulHorzRes;
        Dc_Attr->szlViewportExt.cy = -((PGDIDEVICE)dc->pPDev)->GDIInfo.ulVertRes;
        break;

      case MM_ANISOTROPIC:
        break;
    }

    DC_UpdateXforms(dc);
  }

  return PrevMapMode;
}

BOOL
STDCALL
NtGdiSetViewportExtEx(HDC  hDC,
                      int  XExtent,
                      int  YExtent,
                      LPSIZE  Size)
{
  PDC dc;
  PDC_ATTR Dc_Attr;

  dc = DC_LockDc(hDC);
  if ( !dc )
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  Dc_Attr = dc->pDc_Attr;
  if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;

  switch (Dc_Attr->iMapMode)
    {
      case MM_HIENGLISH:
      case MM_HIMETRIC:
      case MM_LOENGLISH:
      case MM_LOMETRIC:
      case MM_TEXT:
      case MM_TWIPS:
	DC_UnlockDc(dc);
	return FALSE;

      case MM_ISOTROPIC:
	// Here we should (probably) check that SetWindowExtEx *really* has
	// been called
	break;
    }

  if (Size)
    {
      NTSTATUS Status = STATUS_SUCCESS;

      _SEH_TRY
      {
         ProbeForWrite(Size,
                       sizeof(SIZE),
                       1);
         Size->cx = Dc_Attr->szlViewportExt.cx;
         Size->cy = Dc_Attr->szlViewportExt.cy;

         Dc_Attr->szlViewportExt.cx = XExtent;
         Dc_Attr->szlViewportExt.cy = YExtent;

         if (Dc_Attr->iMapMode == MM_ISOTROPIC)
             IntFixIsotropicMapping(dc);
      }
      _SEH_HANDLE
      {
         Status = _SEH_GetExceptionCode();
      }
      _SEH_END;

      if(!NT_SUCCESS(Status))
      {
        SetLastNtError(Status);
        DC_UnlockDc(dc);
        return FALSE;
      }
    }


  DC_UpdateXforms(dc);
  DC_UnlockDc(dc);

  return TRUE;
}

BOOL
STDCALL
NtGdiSetViewportOrgEx(HDC  hDC,
                     int  X,
                     int  Y,
                     LPPOINT  Point)
{
  PDC dc;
  PDC_ATTR Dc_Attr;

  dc = DC_LockDc(hDC);
  if (!dc)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }
  Dc_Attr = dc->pDc_Attr;
  if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;

  if (Point)
    {
      NTSTATUS Status = STATUS_SUCCESS;

      _SEH_TRY
      {
         ProbeForWrite(Point,
                       sizeof(POINT),
                       1);
         Point->x = Dc_Attr->ptlViewportOrg.x;
         Point->y = Dc_Attr->ptlViewportOrg.y;
      }
      _SEH_HANDLE
      {
         Status = _SEH_GetExceptionCode();
      }
      _SEH_END;

      if(!NT_SUCCESS(Status))
      {
        SetLastNtError(Status);
        DC_UnlockDc(dc);
        return FALSE;
      }
    }

  Dc_Attr->ptlViewportOrg.x = X;
  Dc_Attr->ptlViewportOrg.y = Y;

  DC_UpdateXforms(dc);
  DC_UnlockDc(dc);

  return TRUE;
}

BOOL
STDCALL
NtGdiSetWindowExtEx(HDC  hDC,
                   int  XExtent,
                   int  YExtent,
                   LPSIZE  Size)
{
  PDC dc;
  PDC_ATTR Dc_Attr;

  dc = DC_LockDc(hDC);
  if (!dc)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }
  Dc_Attr = dc->pDc_Attr;
  if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;

  switch (Dc_Attr->iMapMode)
    {
      case MM_HIENGLISH:
      case MM_HIMETRIC:
      case MM_LOENGLISH:
      case MM_LOMETRIC:
      case MM_TEXT:
      case MM_TWIPS:
	DC_UnlockDc(dc);
	return FALSE;
    }

  if (Size)
    {
      NTSTATUS Status = STATUS_SUCCESS;

      _SEH_TRY
      {
         ProbeForWrite(Size,
                       sizeof(SIZE),
                       1);
         Size->cx = Dc_Attr->szlWindowExt.cx;
         Size->cy = Dc_Attr->szlWindowExt.cy;
      }
      _SEH_HANDLE
      {
         Status = _SEH_GetExceptionCode();
      }
      _SEH_END;

      if(!NT_SUCCESS(Status))
      {
        SetLastNtError(Status);
        DC_UnlockDc(dc);
        return FALSE;
      }
    }

  Dc_Attr->szlWindowExt.cx = XExtent;
  Dc_Attr->szlWindowExt.cy = YExtent;

  DC_UpdateXforms(dc);
  DC_UnlockDc(dc);

  return TRUE;
}

BOOL
STDCALL
NtGdiSetWindowOrgEx(HDC  hDC,
                   int  X,
                   int  Y,
                   LPPOINT  Point)
{
  PDC dc;
  PDC_ATTR Dc_Attr;

  dc = DC_LockDc(hDC);
  if (!dc)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }
  Dc_Attr = dc->pDc_Attr;
  if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;

  if (Point)
    {
      NTSTATUS Status = STATUS_SUCCESS;

      _SEH_TRY
      {
         ProbeForWrite(Point,
                       sizeof(POINT),
                       1);
         Point->x = Dc_Attr->ptlWindowOrg.x;
         Point->y = Dc_Attr->ptlWindowOrg.y;
      }
      _SEH_HANDLE
      {
         Status = _SEH_GetExceptionCode();
      }
      _SEH_END;

      if(!NT_SUCCESS(Status))
      {
        SetLastNtError(Status);
        DC_UnlockDc(dc);
        return FALSE;
      }
    }

  Dc_Attr->ptlWindowOrg.x = X;
  Dc_Attr->ptlWindowOrg.y = Y;

  DC_UpdateXforms(dc);
  DC_UnlockDc(dc);

  return TRUE;
}

//
// Mirror Window function.
//
VOID
FASTCALL
IntMirrorWindowOrg(PDC dc)
{
  PDC_ATTR Dc_Attr;
  LONG X;
  
  Dc_Attr = dc->pDc_Attr;
  if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;

  if (!(Dc_Attr->dwLayout & LAYOUT_RTL))
  {
     Dc_Attr->ptlWindowOrg.x = Dc_Attr->lWindowOrgx; // Flip it back.
     return;
  }
  if (!Dc_Attr->szlViewportExt.cx) return;
  // 
  // WOrgx = wox - (Width - 1) * WExtx / VExtx
  //
  X = (dc->erclWindow.right - dc->erclWindow.left) - 1; // Get device width - 1

  X = ( X * Dc_Attr->szlWindowExt.cx) / Dc_Attr->szlViewportExt.cx;

  Dc_Attr->ptlWindowOrg.x = Dc_Attr->lWindowOrgx - X; // Now set the inverted win origion.

  return;
}

// NtGdiSetLayout
// 
// The default is left to right. This function changes it to right to left, which
// is the standard in Arabic and Hebrew cultures.
//
/*
 * @implemented
 */
DWORD
APIENTRY
NtGdiSetLayout(
    IN HDC hdc,
    IN LONG wox,
    IN DWORD dwLayout)
{
  PDC dc;
  PDC_ATTR Dc_Attr;
  DWORD oLayout;

  dc = DC_LockDc(hdc);
  if (!dc)
  {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return GDI_ERROR;
  }
  Dc_Attr = dc->pDc_Attr;
  if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;

  Dc_Attr->dwLayout = dwLayout;
  oLayout = Dc_Attr->dwLayout;

  if (!(dwLayout & LAYOUT_ORIENTATIONMASK))
  {
     DC_UnlockDc(dc);
     return oLayout;
  }

  if (dwLayout & LAYOUT_RTL) Dc_Attr->iMapMode = MM_ANISOTROPIC;

  Dc_Attr->szlWindowExt.cy = -Dc_Attr->szlWindowExt.cy;
  Dc_Attr->ptlWindowOrg.x  = -Dc_Attr->ptlWindowOrg.x;

  if (wox == -1)
     IntMirrorWindowOrg(dc);
  else
     Dc_Attr->ptlWindowOrg.x = wox - Dc_Attr->ptlWindowOrg.x;

  if (!(Dc_Attr->flTextAlign & TA_CENTER)) Dc_Attr->flTextAlign |= TA_RIGHT;

  if (dc->DcLevel.flPath & DCPATH_CLOCKWISE)
      dc->DcLevel.flPath &= ~DCPATH_CLOCKWISE;
  else
      dc->DcLevel.flPath |= DCPATH_CLOCKWISE;

  Dc_Attr->flXform |= (PAGE_EXTENTS_CHANGED|INVALIDATE_ATTRIBUTES|DEVICE_TO_WORLD_INVALID);

//  DC_UpdateXforms(dc);
  DC_UnlockDc(dc);
  return oLayout;
}

/*
 * @implemented
 */
LONG
APIENTRY
NtGdiGetDeviceWidth(
    IN HDC hdc)
{
  PDC dc;
  LONG Ret;
  dc = DC_LockDc(hdc);
  if (!dc)
  {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return 0;
  }
  Ret = dc->erclWindow.right - dc->erclWindow.left;
  DC_UnlockDc(dc);
  return Ret;
}

/*
 * @implemented
 */
BOOL
APIENTRY
NtGdiMirrorWindowOrg(
    IN HDC hdc)
{
  PDC dc;
  dc = DC_LockDc(hdc);
  if (!dc)
  {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return FALSE;
  }
  IntMirrorWindowOrg(dc);
  DC_UnlockDc(dc);
  return TRUE;
}

/*
 * @implemented
 */
BOOL
APIENTRY
NtGdiSetSizeDevice(
    IN HDC hdc,
    IN INT cxVirtualDevice,
    IN INT cyVirtualDevice)
{
    PDC dc;
    PDC_ATTR pDc_Attr;

    if (!cxVirtualDevice ||
        !cyVirtualDevice  ) return FALSE;

    dc = DC_LockDc(hdc);
    if (!dc) return FALSE;
    
    pDc_Attr = dc->pDc_Attr;
    if(!pDc_Attr) pDc_Attr = &dc->Dc_Attr;

    pDc_Attr->szlVirtualDeviceSize.cx = cxVirtualDevice;
    pDc_Attr->szlVirtualDeviceSize.cy = cyVirtualDevice;

//    DC_UpdateXforms(dc);    
    DC_UnlockDc(dc);

    return TRUE;
}

/*
 * @implemented
 */
BOOL
APIENTRY
NtGdiSetVirtualResolution(
    IN HDC hdc,
    IN INT cxVirtualDevicePixel,
    IN INT cyVirtualDevicePixel,
    IN INT cxVirtualDeviceMm,
    IN INT cyVirtualDeviceMm)
{
    PDC dc;
    PDC_ATTR pDc_Attr;

    // Need test types for zeros and non zeros

    dc = DC_LockDc(hdc);
    if (!dc) return FALSE;
    
    pDc_Attr = dc->pDc_Attr;
    if(!pDc_Attr) pDc_Attr = &dc->Dc_Attr;

    pDc_Attr->szlVirtualDevicePixel.cx = cxVirtualDevicePixel;
    pDc_Attr->szlVirtualDevicePixel.cy = cyVirtualDevicePixel;
    pDc_Attr->szlVirtualDeviceMm.cx = cxVirtualDeviceMm;
    pDc_Attr->szlVirtualDeviceMm.cy = cyVirtualDeviceMm;

//    DC_UpdateXforms(dc);    
    DC_UnlockDc(dc);
    return TRUE;
}

/* EOF */
