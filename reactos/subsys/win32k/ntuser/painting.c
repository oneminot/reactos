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
/* $Id: painting.c,v 1.32 2003/09/09 20:06:58 gvg Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Painting
 * FILE:             subsys/win32k/ntuser/painting.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <include/object.h>
#include <include/guicheck.h>
#include <include/window.h>
#include <include/class.h>
#include <include/error.h>
#include <include/winsta.h>
#include <windows.h>
#include <include/painting.h>
#include <user32/wininternal.h>
#include <include/rect.h>
#include <win32k/coord.h>
#include <win32k/region.h>
#include <include/vis.h>

#define NDEBUG
#include <debug.h>


/* GLOBALS *******************************************************************/

/* client rect in window coordinates */
#define GETCLIENTRECTW(wnd, r)	(r).left = (wnd)->ClientRect.left - (wnd)->WindowRect.left; \
				(r).top = (wnd)->ClientRect.top - (wnd)->WindowRect.top; \
				(r).right = (wnd)->ClientRect.right - (wnd)->WindowRect.left; \
				(r).bottom = (wnd)->ClientRect.bottom - (wnd)->WindowRect.top

/* FUNCTIONS *****************************************************************/

HRGN STATIC STDCALL
PaintDoPaint(PWINDOW_OBJECT Window, HRGN hRgn, ULONG Flags, ULONG ExFlags)
{
  HDC hDC;
  HWND hWnd = Window->Self;
  BOOL bIcon = (0 != (Window->Style & WS_MINIMIZE)) &&
               (0 != IntGetClassLong(Window, GCL_HICON, FALSE));

  if (0 != (ExFlags & RDW_EX_DELAY_NCPAINT) ||
      PaintHaveToDelayNCPaint(Window, 0))
    {
      ExFlags |= RDW_EX_DELAY_NCPAINT;
    }

  if (Flags & RDW_UPDATENOW)
    {
      if (NULL != Window->UpdateRegion)
	{
	  if (IntIsDesktopWindow(Window))
	    {
	      VIS_RepaintDesktop(Window->Self, Window->UpdateRegion);
	    }
	  else
	    {
	      NtUserSendMessage(hWnd, bIcon ? WM_PAINTICON : WM_PAINT, bIcon, 0);
	    }
	}
    }
  else if (Flags & RDW_ERASENOW || ExFlags & RDW_EX_TOPFRAME)
    {
      UINT Dcx = DCX_INTERSECTRGN | DCX_USESTYLE | DCX_KEEPCLIPRGN |
	DCX_WINDOWPAINT | DCX_CACHE;
      HRGN hRgnRet;

      hRgnRet =
	PaintUpdateNCRegion(Window,
			 hRgn,
			 UNC_REGION | UNC_CHECK |
			 ((ExFlags & RDW_EX_TOPFRAME) ? UNC_ENTIRE : 0) |
			 ((ExFlags & RDW_EX_DELAY_NCPAINT) ?
			  UNC_DELAY_NCPAINT : 0));
      if (NULL != hRgnRet)
	{
	  if ((HRGN) 1 < hRgnRet)
	    {
	      hRgn = hRgnRet;
	    }
	  else
	    {
	      hRgnRet = NULL;
	    }
	  if (0 != (Window->Flags & WINDOWOBJECT_NEED_ERASEBACKGRD))
	    {
	      if (bIcon)
		{
		  Dcx |= DCX_WINDOW;
		}
	      if (NULL != hRgnRet)
		{
		  NtGdiOffsetRgn(hRgnRet,
				Window->WindowRect.left -
				Window->ClientRect.left,
				Window->WindowRect.top -
				Window->ClientRect.top);
		}
	      else
		{
		  Dcx &= ~DCX_INTERSECTRGN;
		}
	      if (NULL != (hDC = NtUserGetDCEx(hWnd, hRgnRet, Dcx)))
		{
		  if (IntIsDesktopWindow(Window))
		    {
		      VIS_RepaintDesktop(Window->Self, Window->UpdateRegion);
		      NtGdiDeleteObject(Window->UpdateRegion);
		      Window->UpdateRegion = 0;
		    }
		  else
		    {
		      if (0 != NtUserSendMessage(hWnd, bIcon ? WM_ICONERASEBKGND :
						 WM_ERASEBKGND, (WPARAM)hDC, 0))
			{
			  Window->Flags &= ~WINDOWOBJECT_NEED_ERASEBACKGRD;
			}		      
		    }
		  NtUserReleaseDC(hWnd, hDC);
		}
	    }
	}
    }

  /* FIXME: Check that the window is still valid at this point. */

  ExFlags &= ~RDW_EX_TOPFRAME;

  /* FIXME: Paint child windows. */

  return(hRgn);
}

VOID STATIC FASTCALL
PaintUpdateInternalPaint(PWINDOW_OBJECT Window, ULONG Flags)
{
  if (Flags & RDW_INTERNALPAINT)
    {
      if (Window->UpdateRegion == NULL &&
	  !(Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT))
	{
	  MsqIncPaintCountQueue(Window->MessageQueue);
	}
      Window->Flags &= ~WINDOWOBJECT_NEED_INTERNALPAINT;
    }
  else if (Flags & RDW_NOINTERNALPAINT)
    {
      if (Window->UpdateRegion == NULL &&
	  (Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT))
	{
	  MsqDecPaintCountQueue(Window->MessageQueue);
	}
      Window->Flags &= ~WINDOWOBJECT_NEED_INTERNALPAINT;
    }
}

VOID STATIC FASTCALL
PaintValidateParent(PWINDOW_OBJECT Child)
{
  HWND DesktopHandle = IntGetDesktopWindow();
  PWINDOW_OBJECT Parent = Child->Parent;
  PWINDOW_OBJECT Desktop = IntGetWindowObject(DesktopHandle);
  HRGN hRgn;

  if ((HRGN) 1 == Child->UpdateRegion)
    {
      RECT Rect;

      Rect.left = Rect.top = 0;
      Rect.right = Child->WindowRect.right - Child->WindowRect.left;
      Rect.bottom = Child->WindowRect.bottom - Child->WindowRect.top;

      hRgn = UnsafeIntCreateRectRgnIndirect(&Rect);
    }
  else
    {
      hRgn = Child->UpdateRegion;
    }

  while (NULL != Parent && Parent != Desktop)
    {
      if (0 == (Parent->Style & WS_CLIPCHILDREN))
	{
	  if (NULL != Parent->UpdateRegion)
	    {
	      POINT Offset;

	      if ((HRGN) 1 == Parent->UpdateRegion)
		{
		  RECT Rect1;

		  Rect1.left = Rect1.top = 0;
		  Rect1.right = Parent->WindowRect.right - 
		    Parent->WindowRect.left;
		  Rect1.bottom = Parent->WindowRect.bottom -
		    Parent->WindowRect.top;

		  Parent->UpdateRegion = 
		    UnsafeIntCreateRectRgnIndirect(&Rect1);
		}
	      Offset.x = Child->WindowRect.left - Parent->WindowRect.left;
	      Offset.y = Child->WindowRect.top - Parent->WindowRect.top;
	      NtGdiOffsetRgn(hRgn, Offset.x, Offset.y);
	      NtGdiCombineRgn(Parent->UpdateRegion, Parent->UpdateRegion, hRgn,
			     RGN_DIFF);
	      NtGdiOffsetRgn(hRgn, -Offset.x, -Offset.y);
	    }
	}
      Parent = Parent->Parent;
    }
  if (hRgn != Child->UpdateRegion)
    {
      NtGdiDeleteObject(Child->UpdateRegion);
    }
  IntReleaseWindowObject(Desktop);
}

VOID STATIC STDCALL
PaintUpdateRgns(PWINDOW_OBJECT Window, HRGN hRgn, ULONG Flags,
		BOOL First)
{
  /*
   * Called only when one of the following is set:
   * (RDW_INVALIDATE | RDW_VALIDATE | RDW_INTERNALPAINT | RDW_NOINTERNALPAINT)
   */

  BOOL HadOne = NULL != Window->UpdateRegion && NULL != hRgn;
  BOOL HasChildren = Window->FirstChild &&
    !(Flags & RDW_NOCHILDREN) && !(Window->Style & WS_MINIMIZE) &&
    ((Flags & RDW_ALLCHILDREN) || !(Window->Style & WS_CLIPCHILDREN));
  RECT Rect;

  Rect.left = Rect.top = 0;
  Rect.right = Window->WindowRect.right - Window->WindowRect.left;
  Rect.bottom = Window->WindowRect.bottom - Window->WindowRect.top;

  if (Flags & RDW_INVALIDATE)
    {
      if ((HRGN) 1 < hRgn)
	{
	  if ((HRGN) 1 != Window->UpdateRegion)
	    {
	      if ((HRGN) 1 < Window->UpdateRegion)
		{
		  NtGdiCombineRgn(Window->UpdateRegion, Window->UpdateRegion,
		                 hRgn, RGN_OR);
		}
	      Window->UpdateRegion =
		REGION_CropRgn(Window->UpdateRegion,
			       Window->UpdateRegion ? Window->UpdateRegion : hRgn,
	                       &Rect, NULL);
	      if (! HadOne)
		{
		  UnsafeIntGetRgnBox(Window->UpdateRegion, &Rect);
		  if (NtGdiIsEmptyRect(&Rect))
		    {
		      NtGdiDeleteObject(Window->UpdateRegion);
		      Window->UpdateRegion = NULL;
		      PaintUpdateInternalPaint(Window, Flags);
		      return;
		    }
		}
	    }
	}
      else if ((HRGN) 1 == hRgn)
	{
	  if ((HRGN) 1 < Window->UpdateRegion)
	    {
	      NtGdiDeleteObject(Window->UpdateRegion);
	    }
	  Window->UpdateRegion = (HRGN) 1;
	}
      else
	{
	  hRgn = Window->UpdateRegion; /* this is a trick that depends on code in PaintRedrawWindow() */
	}

      if (! HadOne && 0 == (Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT) &&
	  !IntIsDesktopWindow(Window))
	{
	  MsqIncPaintCountQueue(Window->MessageQueue);
	}

      if (Flags & RDW_FRAME)
	{
	  Window->Flags |= WINDOWOBJECT_NEED_NCPAINT;
	}
      if (Flags & RDW_ERASE)
	{
	  Window->Flags |= WINDOWOBJECT_NEED_ERASEBACKGRD;
	}
      Flags |= RDW_FRAME;
    }
  else if (Flags & RDW_VALIDATE)
    {
      if (NULL != Window->UpdateRegion)
	{
	  if ((HRGN) 1 < hRgn)
	    {
	      if ((HRGN) 1 == Window->UpdateRegion)
		{
		  /* If no NCPAINT needed or if we're going to turn it off
		     the special value 1 means the whole client rect */
		  if (0 == (Window->Flags & WINDOWOBJECT_NEED_NCPAINT) ||
		      0 != (Flags & RDW_NOFRAME))
		    {
		      Rect.left = Window->ClientRect.left - Window->WindowRect.left;
		      Rect.top = Window->ClientRect.top - Window->WindowRect.top;
		      Rect.right = Window->ClientRect.right - Window->WindowRect.left;
		      Rect.bottom = Window->ClientRect.bottom - Window->WindowRect.top;
		    }
		  Window->UpdateRegion = 
		    UnsafeIntCreateRectRgnIndirect(&Rect);
		}
	      if (NtGdiCombineRgn(Window->UpdateRegion, 
				 Window->UpdateRegion, hRgn, 
				 RGN_DIFF) == NULLREGION)
		{
		  NtGdiDeleteObject(Window->UpdateRegion);
		  Window->UpdateRegion = NULL;
		}
	    }
	  else /* validate everything */
	    {
	      if ((HRGN) 1 < Window->UpdateRegion)
		{
		  NtGdiDeleteObject(Window->UpdateRegion);
		}
	      Window->UpdateRegion = NULL;
	    }

	  if (NULL != Window->UpdateRegion)
	    {
	      Window->Flags &= ~WINDOWOBJECT_NEED_ERASEBACKGRD;
	      if (0 != (Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT))
		{
		  MsqDecPaintCountQueue(Window->MessageQueue);
		}
	    }
	}

      if (Flags & RDW_NOFRAME)
	{
	  Window->Flags &= ~WINDOWOBJECT_NEED_NCPAINT;
	}
      if (Flags & RDW_NOERASE)
	{
	  Window->Flags &= ~WINDOWOBJECT_NEED_ERASEBACKGRD;
	}
    }

  if (First && NULL != Window->UpdateRegion && 0 != (Flags & RDW_UPDATENOW))
    {
      PaintValidateParent(Window); /* validate parent covered by region */
    }

  /* in/validate child windows that intersect with the region if it
   * is a valid handle. */

  if (0 != (Flags & (RDW_INVALIDATE | RDW_VALIDATE)))
    {
      if ((HRGN) 1 < hRgn && HasChildren)
	{
	  POINT Total = {0, 0};
	  POINT PrevOrign = {0, 0};
	  PWINDOW_OBJECT Child;

	  ExAcquireFastMutexUnsafe(&Window->ChildrenListLock);
	  Child = Window->FirstChild;
	  while (Child)
	    {
	      if (0 != (Child->Style & WS_VISIBLE))
		{
		  POINT Offset;

		  Rect.left = Child->WindowRect.left - Window->WindowRect.left;
		  Rect.top = Child->WindowRect.top - Window->WindowRect.top;
		  Rect.right = Child->WindowRect.right - Window->WindowRect.left;
		  Rect.bottom = Child->WindowRect.bottom - Window->WindowRect.top;

		  Offset.x = Rect.left - PrevOrign.x;
		  Offset.y = Rect.top - PrevOrign.y;
		  NtGdiOffsetRect(&Rect, -Total.x, -Total.y);

		  if (UnsafeIntRectInRegion(hRgn, &Rect))
		    {
		      NtGdiOffsetRgn(hRgn, -Offset.x, -Offset.y);
		      PaintUpdateRgns(Child, hRgn, Flags, FALSE);
		      PrevOrign.x = Rect.left + Total.x;
		      PrevOrign.y = Rect.right + Total.y;
		      Total.x += Offset.x;
		      Total.y += Offset.y;
		    }
		}
	      Child = Child->NextSibling;
	    }
	  ExReleaseFastMutexUnsafe(&Window->ChildrenListLock);

	  NtGdiOffsetRgn(hRgn, Total.x, Total.y);
	  HasChildren = FALSE;
	}
    }

  if (HasChildren)
  {
    PWINDOW_OBJECT Child;

    ExAcquireFastMutexUnsafe(&Window->ChildrenListLock);
    Child = Window->FirstChild;
    while (Child)
    {
      if (Child->Style & WS_VISIBLE)
      {
        PaintUpdateRgns(Child, hRgn, Flags, FALSE);
      }
      Child = Child->NextSibling;
    }
    ExReleaseFastMutexUnsafe(&Window->ChildrenListLock);
  }

  PaintUpdateInternalPaint(Window, Flags);
}

BOOL STDCALL
PaintRedrawWindow( PWINDOW_OBJECT Window, 
		   const RECT* UpdateRect, 
		   HRGN UpdateRgn,
		   ULONG Flags, 
		   ULONG ExFlags)
{
  RECT Rect, Rect2;
  POINT Pt;
  HRGN hRgn = NULL;

  DPRINT("[win32k.sys:painting] In PaintRedrawWindow()\n");

  if ((RDW_INVALIDATE | RDW_FRAME) == (Flags & (RDW_INVALIDATE | RDW_FRAME)) ||
      (RDW_VALIDATE | RDW_NOFRAME) == (Flags & (RDW_VALIDATE | RDW_NOFRAME)))
    {
      Rect = Window->WindowRect;
    }
  else
    {
      Rect = Window->ClientRect;
    }

  if (ExFlags & RDW_EX_XYWINDOW)
    {
      Pt.x = Pt.y = 0;
      NtGdiOffsetRect(&Rect, -Window->WindowRect.left, -Window->WindowRect.top);
    }
  else
    {
      Pt.x = Window->ClientRect.left - Window->WindowRect.left;
      Pt.y = Window->ClientRect.top - Window->WindowRect.top;
      NtGdiOffsetRect(&Rect, -Window->ClientRect.left, -Window->ClientRect.top);
    }

  if (0 != (Flags & RDW_INVALIDATE))  /* ------------------------- Invalidate */
    {
      if (NULL != UpdateRgn)
	{
	  if (NULL != Window->UpdateRegion)
	    {
	      hRgn = REGION_CropRgn(NULL, UpdateRgn, NULL, &Pt);
	    }
	  else
	    {
	      Window->UpdateRegion = REGION_CropRgn(NULL, UpdateRgn, &Rect, &Pt);
	    }
	}
      else if (NULL != UpdateRect)
	{
	  if (! NtGdiIntersectRect(&Rect2, &Rect, UpdateRect))
	    {
	      
	      if ((HRGN) 1 < hRgn && hRgn != UpdateRgn)
		{
		  NtGdiDeleteObject(hRgn);
		}
	      return TRUE;
	    }
	  NtGdiOffsetRect(&Rect2, Pt.x, Pt.y);
	  if (NULL == Window->UpdateRegion)
	    {
	      Window->UpdateRegion =
		UnsafeIntCreateRectRgnIndirect(&Rect2);
	    }
	  else
	    {
	      hRgn = UnsafeIntCreateRectRgnIndirect(&Rect2);
	    }
	}
      else /* entire window or client depending on RDW_FRAME */
	{
	  if (Flags & RDW_FRAME)
	    {
	      if (NULL != Window->UpdateRegion)
		{
		  hRgn = (HRGN) 1;
		}
	      else
		{
	        Window->UpdateRegion = (HRGN) 1;
		}
	    }
	  else
	    {
	      GETCLIENTRECTW(Window, Rect2);
	      if (NULL == Window->UpdateRegion)
		{
		  Window->UpdateRegion = UnsafeIntCreateRectRgnIndirect(&Rect2);
		}
	      else
		{
		  hRgn = UnsafeIntCreateRectRgnIndirect(&Rect2);
		}
	    }
	}
    }
  else if (Flags & RDW_VALIDATE)
    {
      /* In this we cannot leave with zero hRgn */
      if (NULL != UpdateRgn)
	{
	  hRgn = REGION_CropRgn(hRgn, UpdateRgn,  &Rect, &Pt);
	  UnsafeIntGetRgnBox(hRgn, &Rect2);
	  if (NtGdiIsEmptyRect(&Rect2))
	    {
	      
	      if ((HRGN) 1 < hRgn && hRgn != UpdateRgn)
		{
		  NtGdiDeleteObject(hRgn);
		}
	      return TRUE;
	    }
	}
      else if (NULL != UpdateRect)
	{
	  if (! NtGdiIntersectRect(&Rect2, &Rect, UpdateRect))
	    {
	      
	      if ((HRGN) 1 < hRgn && hRgn != UpdateRgn)
		{
		  NtGdiDeleteObject(hRgn);
		}
	      return TRUE;
	    }
	  NtGdiOffsetRect(&Rect2, Pt.x, Pt.y);
	  hRgn = UnsafeIntCreateRectRgnIndirect(&Rect2);
	}
      else /* entire window or client depending on RDW_NOFRAME */
        {
	  if (0 != (Flags & RDW_NOFRAME))
	    {
	      hRgn = (HRGN) 1;
	    }
	  else
	    {
	      GETCLIENTRECTW(Window, Rect2);
	      hRgn = UnsafeIntCreateRectRgnIndirect(&Rect2);
            }
        }
    }

  /* At this point hRgn is either an update region in window coordinates or 1 or 0 */

  PaintUpdateRgns(Window, hRgn, Flags, TRUE);

  /* Erase/update windows, from now on hRgn is a scratch region */

  hRgn = PaintDoPaint(Window, (HRGN) 1 == hRgn ? NULL : hRgn, Flags, ExFlags);

  if ((HRGN) 1 < hRgn && hRgn != UpdateRgn)
    {
      NtGdiDeleteObject(hRgn);
    }

  return TRUE;
}

BOOL STDCALL
PaintHaveToDelayNCPaint(PWINDOW_OBJECT Window, ULONG Flags)
{
  if (Flags & UNC_DELAY_NCPAINT)
    {
      return(TRUE);
    }

  if (Flags & UNC_IN_BEGINPAINT)
    {
      return(FALSE);
    }

  Window = Window->Parent;
  while (Window != NULL)
    {
      if (Window->Style & WS_CLIPCHILDREN && Window->UpdateRegion != NULL)
	{
	  return TRUE;
	}
      Window = Window->Parent;
    }

  return FALSE;
}

HWND STDCALL
PaintingFindWinToRepaint(HWND hWnd, PW32THREAD Thread)
{
  PWINDOW_OBJECT Window;
  PWINDOW_OBJECT BaseWindow;
  PLIST_ENTRY current_entry;
  HWND hFoundWnd = NULL;

  if (hWnd == NULL)
    {
      ExAcquireFastMutex(&Thread->WindowListLock);
      current_entry = Thread->WindowListHead.Flink;
      while (current_entry != &Thread->WindowListHead)
	{
	  Window = CONTAINING_RECORD(current_entry, WINDOW_OBJECT,
				     ThreadListEntry);
	  if (Window->Style & WS_VISIBLE)
	    {
	      hFoundWnd = 
		PaintingFindWinToRepaint(Window->Self, Thread);
	      if (hFoundWnd != NULL)
		{
		  ExReleaseFastMutex(&Thread->WindowListLock);
		  return(hFoundWnd);
		}
	    }
	  current_entry = current_entry->Flink;
	}
      ExReleaseFastMutex(&Thread->WindowListLock);
      return(NULL);
    }

  BaseWindow = IntGetWindowObject(hWnd);
  if (BaseWindow == NULL)
    {
      return(NULL);
    }
  if (BaseWindow->UpdateRegion != NULL ||
      BaseWindow->Flags & WINDOWOBJECT_NEED_INTERNALPAINT)
    {
      IntReleaseWindowObject(BaseWindow);
      return(hWnd);
    }

  ExAcquireFastMutex(&BaseWindow->ChildrenListLock);
  Window = BaseWindow->FirstChild;
  while (Window)
  {
    if (Window->Style & WS_VISIBLE)
    {
      hFoundWnd = PaintingFindWinToRepaint(Window->Self, Thread);
      if (hFoundWnd != NULL)
      {
        break;
      }
    }
    Window = Window->NextSibling;
  }
  ExReleaseFastMutex(&BaseWindow->ChildrenListLock);

  IntReleaseWindowObject(BaseWindow);
  return(hFoundWnd);
}

HRGN STDCALL
PaintUpdateNCRegion(PWINDOW_OBJECT Window, HRGN hRgn, ULONG Flags)
{
  HRGN hRgnRet;
  RECT ClientRect;
  HRGN hClip = NULL;

  /* Desktop has no parent. */
  if (Window->Parent == NULL)
    {
      Window->Flags &= ~WINDOWOBJECT_NEED_NCPAINT;
      if ((HRGN) 1 < Window->UpdateRegion)
	{
	  hRgnRet = REGION_CropRgn(hRgn, Window->UpdateRegion, NULL, NULL);
	}
      else
	{
	  hRgnRet = Window->UpdateRegion;
	}
      return(hRgnRet);
    }

#if 0 /* NtUserGetFOregroundWindow() not implemented yet */
  if ((Window->Self == NtUserGetForegroundWindow()) &&
      0 == (Window->Flags & WIN_NCACTIVATED) )
    {
      Window->Flags |= WIN_NCACTIVATED;
      Flags |= UNC_ENTIRE;
    }
#endif

    /*
     * If the window's non-client area needs to be painted,
     */
  if (0 != (Window->Flags & WINDOWOBJECT_NEED_NCPAINT) &&
      ! PaintHaveToDelayNCPaint(Window, Flags))
    {
      RECT UpdateRegionBox;
      RECT Rect;

      Window->Flags &= ~WINDOWOBJECT_NEED_NCPAINT;
      GETCLIENTRECTW(Window, ClientRect);

      if ((HRGN) 1 < Window->UpdateRegion)
	{
	  UnsafeIntGetRgnBox(Window->UpdateRegion, &UpdateRegionBox);
	  NtGdiUnionRect(&Rect, &ClientRect, &UpdateRegionBox);
	  if (Rect.left != ClientRect.left || Rect.top != ClientRect.top ||
	      Rect.right != ClientRect.right || Rect.bottom != ClientRect.bottom)
	    {
	      hClip = Window->UpdateRegion;
	      Window->UpdateRegion = REGION_CropRgn(hRgn, hClip,
						    &ClientRect, NULL);
	      if (Flags & UNC_REGION)
		{
		  hRgnRet = hClip;
		}
	    }

	  if (Flags & UNC_CHECK)
	    {
	      UnsafeIntGetRgnBox(Window->UpdateRegion, &UpdateRegionBox);
	      if (NtGdiIsEmptyRect(&UpdateRegionBox))
		{
		  NtGdiDeleteObject(Window->UpdateRegion);
		  Window->UpdateRegion = NULL;
		  if (0 == (Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT))
		    {
		      MsqDecPaintCountQueue(Window->MessageQueue);
		    }
		  Window->Flags &= ~WINDOWOBJECT_NEED_ERASEBACKGRD;
		}
	    }

	  if (0 == hClip && 0 != Window->UpdateRegion)
	    {
	    goto copyrgn;
	    }
	}
      else if ((HRGN) 1 == Window->UpdateRegion)
	{
	  if (0 != (Flags & UNC_UPDATE))
	    {
	      Window->UpdateRegion =
		UnsafeIntCreateRectRgnIndirect(&ClientRect);
	    }
	  if (Flags & UNC_REGION)
	    {
	      hRgnRet = (HRGN) 1;
	    }
	  Flags |= UNC_ENTIRE;
	}
    }
  else /* no WM_NCPAINT unless forced */
    {
      if ((HRGN) 1 < Window->UpdateRegion)
	{
copyrgn:
	  if (0 != (Flags & UNC_REGION))
	    {
	      hRgnRet = REGION_CropRgn(hRgn, Window->UpdateRegion, NULL, NULL);
	    }
	}
      else if ((HRGN) 1 == Window->UpdateRegion && 0 != (Flags & UNC_UPDATE))
	{
	  GETCLIENTRECTW(Window, ClientRect);
	  Window->UpdateRegion =
	    UnsafeIntCreateRectRgnIndirect(&ClientRect);
	  if (Flags & UNC_REGION)
	    {
	      hRgnRet = (HRGN) 1;
	    }
	}
    }

  if (NULL == hClip && 0 != (Flags & UNC_ENTIRE))
    {
      if (RtlCompareMemory(&Window->WindowRect, &Window->ClientRect,
			   sizeof(RECT)) != sizeof(RECT))
	{
	  hClip = (HRGN) 1;
	}
      else
	{
	  hClip = NULL;
	}
    }

  if (NULL != hClip) /* NOTE: WM_NCPAINT allows wParam to be 1 */
    {
      if (hClip == hRgnRet && (HRGN) 1 < hRgnRet)
	{
	  hClip = NtGdiCreateRectRgn(0, 0, 0, 0);
	  NtGdiCombineRgn(hClip, hRgnRet, 0, RGN_COPY);
	}

      NtUserSendMessage(Window->Self, WM_NCPAINT, (WPARAM) hClip, 0);

      if ((HRGN) 1 < hClip && hClip != hRgn && hClip != hRgnRet)
	{
	  NtGdiDeleteObject(hClip);
	}

      /* FIXME: Need to check the window is still valid. */
    }
  return(hRgnRet);
}

BOOL STDCALL
NtUserEndPaint(HWND hWnd, CONST PAINTSTRUCT* lPs)
{
  NtUserReleaseDC(hWnd, lPs->hdc);
  /* FIXME: Show claret. */
  return(TRUE);
}

static
HRGN FASTCALL
GetClientUpdateRegion(PWINDOW_OBJECT Window)
{
  POINT Offset;
  RECT Rect;

  if ((DWORD) Window->UpdateRegion <= 1)
    {
      return Window->UpdateRegion;
    }

  Offset.x = Window->WindowRect.left - Window->ClientRect.left;
  Offset.y = Window->WindowRect.top - Window->ClientRect.top;
  Rect.left = - Offset.x;
  Rect.top = - Offset.y;
  Rect.right = Rect.left + (Window->ClientRect.right - Window->ClientRect.left);
  Rect.bottom = Rect.top + (Window->ClientRect.bottom - Window->ClientRect.top);

  return REGION_CropRgn(NULL, Window->UpdateRegion, &Rect, &Offset);
}

HDC STDCALL
NtUserBeginPaint(HWND hWnd, PAINTSTRUCT* lPs)
{
  BOOL IsIcon;
  PWINDOW_OBJECT Window;
  HRGN UpdateRegion;
  RECT ClientRect;
  RECT ClipRect;
  //NTSTATUS Status;
  INT DcxFlags;

  if (!(Window = IntGetWindowObject(hWnd)))
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return NULL;
  }
  
  /* Send WM_NCPAINT */
  PaintUpdateNCRegion(Window, 0, UNC_UPDATE | UNC_IN_BEGINPAINT);

  /* Check ifthe window is still valid. */
  if (!IntGetWindowObject(hWnd))
  {
    return 0;
  }

  /* retrieve update region */
  UpdateRegion = GetClientUpdateRegion(Window);
  if (1 < (DWORD) Window->UpdateRegion)
    {
      NtGdiDeleteObject(Window->UpdateRegion);
    }
  Window->UpdateRegion = 0;
  if (UpdateRegion != NULL || (Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT))
    {
      MsqDecPaintCountQueue(Window->MessageQueue);
    }
  Window->Flags &= ~WINDOWOBJECT_NEED_INTERNALPAINT;

  /* FIXME: Hide caret. */

  IsIcon = (Window->Style & WS_MINIMIZE) && IntGetClassLong(Window, GCL_HICON, FALSE);

  DcxFlags = DCX_INTERSECTRGN | DCX_WINDOWPAINT | DCX_USESTYLE;
  if (IsIcon)
    {
    DcxFlags |= DCX_WINDOW;
    }
  if (IntGetClassLong(Window, GCL_STYLE, FALSE) & CS_PARENTDC)
    {
      /* Don't clip the output to the update region for CS_PARENTDC window */
      if ((HRGN) 1 < UpdateRegion)
	{
	  NtGdiDeleteObject(UpdateRegion);
	}
      UpdateRegion = NULL;
      DcxFlags &= ~DCX_INTERSECTRGN;
    }
  else
    {
      if (NULL == UpdateRegion)  /* empty region, clip everything */
	{
          UpdateRegion = NtGdiCreateRectRgn(0, 0, 0, 0);
	}
      else if ((HRGN) 1 == UpdateRegion)  /* whole client area, don't clip */
	{
	  UpdateRegion = NULL;
	  DcxFlags &= ~DCX_INTERSECTRGN;
	}
    }
  lPs->hdc = NtUserGetDCEx(hWnd, UpdateRegion, DcxFlags);

  /* FIXME: Check for DC creation failure. */

  IntGetClientRect(Window, &ClientRect);
  NtGdiGetClipBox(lPs->hdc, &ClipRect);
  NtGdiLPtoDP(lPs->hdc, (LPPOINT)&ClipRect, 2);
  NtGdiIntersectRect(&lPs->rcPaint, &ClientRect, &ClipRect);
  NtGdiDPtoLP(lPs->hdc, (LPPOINT)&lPs->rcPaint, 2);

  if (Window->Flags & WINDOWOBJECT_NEED_ERASEBACKGRD)
    {
      BOOLEAN Result;
      Window->Flags &= ~WINDOWOBJECT_NEED_ERASEBACKGRD;
      Result = NtUserSendMessage(hWnd,
				 IsIcon ? WM_ICONERASEBKGND : WM_ERASEBKGND,
				 (WPARAM)lPs->hdc,
				 0);
      lPs->fErase = !Result;
    }
  else
    {
      lPs->fErase = FALSE;
    }

  ObmDereferenceObject(Window);
  return(lPs->hdc);
}

DWORD
STDCALL
NtUserInvalidateRect(
  HWND hWnd,
  CONST RECT *Rect,
  WINBOOL Erase)
{
  return NtUserRedrawWindow(hWnd, Rect, 0, RDW_INVALIDATE | (Erase ? RDW_ERASE : 0));
}

DWORD
STDCALL
NtUserInvalidateRgn(
  HWND hWnd,
  HRGN Rgn,
  WINBOOL Erase)
{
  return NtUserRedrawWindow(hWnd, NULL, Rgn, RDW_INVALIDATE | (Erase ? RDW_ERASE : 0));
}

BOOL
STDCALL
NtUserValidateRgn(
  HWND hWnd,
  HRGN hRgn)
{
  return NtUserRedrawWindow(hWnd, NULL, hRgn, RDW_VALIDATE | RDW_NOCHILDREN);
}

int
STDCALL
NtUserGetUpdateRgn(
  HWND hWnd,
  HRGN hRgn,
  WINBOOL bErase)
{
  PWINDOW_OBJECT Window;
  int RegionType;

  if (!(Window = IntGetWindowObject(hWnd)))
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return ERROR;
  }
    
  if (NULL == Window->UpdateRegion)
    {
      RegionType = (NtGdiSetRectRgn(hRgn, 0, 0, 0, 0) ? NULLREGION : ERROR);
    }
  else if ((HRGN) 1 == Window->UpdateRegion)
    {
      RegionType = (NtGdiSetRectRgn(hRgn,
                                   0, 0,
                                   Window->ClientRect.right - Window->ClientRect.left,
                                   Window->ClientRect.bottom - Window->ClientRect.top) ?
                    SIMPLEREGION : ERROR);
    }
  else
    {
      RegionType = NtGdiCombineRgn(hRgn, Window->UpdateRegion, hRgn, RGN_COPY);
      NtGdiOffsetRgn(hRgn, Window->WindowRect.left - Window->ClientRect.left,
			  Window->WindowRect.top - Window->ClientRect.top );
    }

  if (bErase &&
      (SIMPLEREGION == RegionType || COMPLEXREGION == RegionType))
    {
      PaintRedrawWindow(Window, NULL, NULL, RDW_ERASENOW | RDW_NOCHILDREN, 0);
    }
  
  IntReleaseWindowObject(Window);

  return RegionType;
}

/* EOF */
