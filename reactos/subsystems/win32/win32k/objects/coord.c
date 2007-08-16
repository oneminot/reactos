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
/* $Id$
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
  ULONG xdim = EngMulDiv(dc->Dc_Attr.szlViewportExt.cx, dc->GDIInfo->ulHorzSize, dc->GDIInfo->ulHorzRes) / dc->Dc_Attr.szlWindowExt.cx;
  ULONG ydim = EngMulDiv(dc->Dc_Attr.szlViewportExt.cy, dc->GDIInfo->ulVertSize, dc->GDIInfo->ulVertRes) / dc->Dc_Attr.szlWindowExt.cy;

  if (xdim > ydim)
  {
    dc->Dc_Attr.szlViewportExt.cx = dc->Dc_Attr.szlViewportExt.cx * abs(ydim / xdim);
    if (!dc->Dc_Attr.szlViewportExt.cx) dc->Dc_Attr.szlViewportExt.cx = 1;
  }
  else
  {
    dc->Dc_Attr.szlViewportExt.cy = dc->Dc_Attr.szlViewportExt.cy * abs(xdim / ydim);
    if (!dc->Dc_Attr.szlViewportExt.cy) dc->Dc_Attr.szlViewportExt.cy = 1;
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
                           CONST LPXFORM  Unsafexform1,
                           CONST LPXFORM  Unsafexform2)
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
  ASSERT ( dc );
  return dc->Dc_Attr.iGraphicsMode;
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
       DC_UpdateXforms(pDc);             // Good wine port here too.
       break;

     default:
       SetLastWin32Error(ERROR_INVALID_PARAMETER);
       return FALSE;
  }

  DC_UpdateXforms(pDc);
  DC_UnlockDc(pDc);
  return TRUE;
}

int
STDCALL
NtGdiGetGraphicsMode ( HDC hDC )
{
  PDC dc;
  int GraphicsMode; // default to failure

  dc = DC_LockDc ( hDC );
  if (!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return 0;
  }

  GraphicsMode = dc->Dc_Attr.iGraphicsMode;

  DC_UnlockDc(dc);
  return GraphicsMode;
}

BOOL
STDCALL
NtGdiGetWorldTransform(HDC  hDC,
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
    *XForm = dc->w.xformWorld2Wnd;
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

   DC_UnlockDc(dc);
   ExFreePool(Points);
   return TRUE;
}

BOOL
STDCALL
NtGdiModifyWorldTransform(HDC hDC,
                          CONST LPXFORM  UnsafeXForm,
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
      
      Ret = IntGdiModifyWorldTransform(dc, &SafeXForm, Mode);
   }
   _SEH_HANDLE
   {
      SetLastNtError(_SEH_GetExceptionCode());
   }
   _SEH_END;

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
  NTSTATUS Status = STATUS_SUCCESS;

  dc = DC_LockDc ( hDC );
  if(!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  if (UnsafePoint)
    {
        _SEH_TRY
        {
            ProbeForWrite(UnsafePoint,
                          sizeof(POINT),
                          1);
            UnsafePoint->x = dc->Dc_Attr.ptlViewportOrg.x;
            UnsafePoint->y = dc->Dc_Attr.ptlViewportOrg.y;
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

  dc->Dc_Attr.ptlViewportOrg.x += XOffset;
  dc->Dc_Attr.ptlViewportOrg.y += YOffset;
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

  dc = DC_LockDc(hDC);
  if (!dc)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }

  if (Point)
    {
      NTSTATUS Status = STATUS_SUCCESS;
      
      _SEH_TRY
      {
         ProbeForWrite(Point,
                       sizeof(POINT),
                       1);
         Point->x = dc->Dc_Attr.ptlWindowOrg.x;
         Point->y = dc->Dc_Attr.ptlWindowOrg.y;
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

  dc->Dc_Attr.ptlWindowOrg.x += XOffset;
  dc->Dc_Attr.ptlWindowOrg.y += YOffset;

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
                             LPSIZE  Size)
{
   UNIMPLEMENTED;
   return FALSE;
}

BOOL
STDCALL
NtGdiScaleWindowExtEx(HDC  hDC,
                           int  Xnum,
                           int  Xdenom,
                           int  Ynum,
                           int  Ydenom,
                           LPSIZE  Size)
{
   UNIMPLEMENTED;
   return FALSE;
}

int
STDCALL
NtGdiSetGraphicsMode(HDC  hDC,
                    int  Mode)
{
  INT ret;
  PDC dc;

  dc = DC_LockDc (hDC);
  if (!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return 0;
  }

  /* One would think that setting the graphics mode to GM_COMPATIBLE
   * would also reset the world transformation matrix to the unity
   * matrix. However, in Windows, this is not the case. This doesn't
   * make a lot of sense to me, but that's the way it is.
   */

  if ((Mode != GM_COMPATIBLE) && (Mode != GM_ADVANCED))
    {
      DC_UnlockDc(dc);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
    }

  ret = dc->Dc_Attr.iGraphicsMode;
  dc->Dc_Attr.iGraphicsMode = Mode;
  DC_UnlockDc(dc);
  return  ret;
}

int
STDCALL
NtGdiSetMapMode(HDC  hDC,
                int  MapMode)
{
  int PrevMapMode;
  PDC dc;

  dc = DC_LockDc(hDC);
  if (!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return 0;
  }

  PrevMapMode = dc->Dc_Attr.iMapMode;

  if (MapMode != dc->Dc_Attr.iMapMode || (MapMode != MM_ISOTROPIC && MapMode != MM_ANISOTROPIC))
  {
    dc->Dc_Attr.iMapMode = MapMode;

    switch (MapMode)
    {
      case MM_TEXT:
        dc->Dc_Attr.szlWindowExt.cx = 1;
        dc->Dc_Attr.szlWindowExt.cy = 1;
        dc->Dc_Attr.szlViewportExt.cx = 1;
        dc->Dc_Attr.szlViewportExt.cy = 1;
        break;

      case MM_LOMETRIC:
      case MM_ISOTROPIC:
        dc->Dc_Attr.szlWindowExt.cx = dc->GDIInfo->ulHorzSize * 10;
        dc->Dc_Attr.szlWindowExt.cy = dc->GDIInfo->ulVertSize * 10;
        dc->Dc_Attr.szlViewportExt.cx = dc->GDIInfo->ulHorzRes;
        dc->Dc_Attr.szlViewportExt.cy = -dc->GDIInfo->ulVertRes;
        break;

      case MM_HIMETRIC:
        dc->Dc_Attr.szlWindowExt.cx = dc->GDIInfo->ulHorzSize * 100;
        dc->Dc_Attr.szlWindowExt.cy = dc->GDIInfo->ulVertSize * 100;
        dc->Dc_Attr.szlViewportExt.cx = dc->GDIInfo->ulHorzRes;
        dc->Dc_Attr.szlViewportExt.cy = -dc->GDIInfo->ulVertRes;
        break;

      case MM_LOENGLISH:
        dc->Dc_Attr.szlWindowExt.cx = EngMulDiv(1000, dc->GDIInfo->ulHorzSize, 254);
        dc->Dc_Attr.szlWindowExt.cy = EngMulDiv(1000, dc->GDIInfo->ulVertSize, 254);
        dc->Dc_Attr.szlViewportExt.cx = dc->GDIInfo->ulHorzRes;
        dc->Dc_Attr.szlViewportExt.cy = -dc->GDIInfo->ulVertRes;
        break;

      case MM_HIENGLISH:
        dc->Dc_Attr.szlWindowExt.cx = EngMulDiv(10000, dc->GDIInfo->ulHorzSize, 254);
        dc->Dc_Attr.szlWindowExt.cy = EngMulDiv(10000, dc->GDIInfo->ulVertSize, 254);
        dc->Dc_Attr.szlViewportExt.cx = dc->GDIInfo->ulHorzRes;
        dc->Dc_Attr.szlViewportExt.cy = -dc->GDIInfo->ulVertRes;
        break;

      case MM_TWIPS:
        dc->Dc_Attr.szlWindowExt.cx = EngMulDiv(14400, dc->GDIInfo->ulHorzSize, 254);
        dc->Dc_Attr.szlWindowExt.cy = EngMulDiv(14400, dc->GDIInfo->ulVertSize, 254);
        dc->Dc_Attr.szlViewportExt.cx = dc->GDIInfo->ulHorzRes;
        dc->Dc_Attr.szlViewportExt.cy = -dc->GDIInfo->ulVertRes;
        break;

      case MM_ANISOTROPIC:
        break;
    }

    DC_UpdateXforms(dc);
  }

  DC_UnlockDc(dc);

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

  dc = DC_LockDc(hDC);
  if ( !dc )
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  switch (dc->Dc_Attr.iMapMode)
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
         Size->cx = dc->Dc_Attr.szlViewportExt.cx;
         Size->cy = dc->Dc_Attr.szlViewportExt.cy;

		 dc->Dc_Attr.szlViewportExt.cx = XExtent;
         dc->Dc_Attr.szlViewportExt.cy = YExtent;

         if (dc->Dc_Attr.iMapMode == MM_ISOTROPIC)
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

  dc = DC_LockDc(hDC);
  if (!dc)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }

  if (Point)
    {
      NTSTATUS Status = STATUS_SUCCESS;
      
      _SEH_TRY
      {
         ProbeForWrite(Point,
                       sizeof(POINT),
                       1);
         Point->x = dc->Dc_Attr.ptlViewportOrg.x;
         Point->y = dc->Dc_Attr.ptlViewportOrg.y;
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

  dc->Dc_Attr.ptlViewportOrg.x = X;
  dc->Dc_Attr.ptlViewportOrg.y = Y;

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

  dc = DC_LockDc(hDC);
  if (!dc)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }

  switch (dc->Dc_Attr.iMapMode)
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
         Size->cx = dc->Dc_Attr.szlWindowExt.cx;
         Size->cy = dc->Dc_Attr.szlWindowExt.cy;
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

  dc->Dc_Attr.szlWindowExt.cx = XExtent;
  dc->Dc_Attr.szlWindowExt.cy = YExtent;

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

  dc = DC_LockDc(hDC);
  if (!dc)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }

  if (Point)
    {
      NTSTATUS Status = STATUS_SUCCESS;
      
      _SEH_TRY
      {
         ProbeForWrite(Point,
                       sizeof(POINT),
                       1);
         Point->x = dc->Dc_Attr.ptlWindowOrg.x;
         Point->y = dc->Dc_Attr.ptlWindowOrg.y;
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

  dc->Dc_Attr.ptlWindowOrg.x = X;
  dc->Dc_Attr.ptlWindowOrg.y = Y;

  DC_UpdateXforms(dc);
  DC_UnlockDc(dc);

  return TRUE;
}


/* EOF */
