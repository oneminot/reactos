/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI WNDOBJ Functions
 * FILE:              subsystems/win32/win32k/eng/engwindow.c
 * PROGRAMER:         Gregor Anich
 */

#include <win32k.h>
#include <debug.h>
DBG_DEFAULT_CHANNEL(EngWnd);

INT gcountPWO = 0;

/*
 * Calls the WNDOBJCHANGEPROC of the given WNDOBJ
 */
VOID
FASTCALL
IntEngWndCallChangeProc(
    IN WNDOBJ *pwo,
    IN FLONG   flChanged)
{
    XCLIPOBJ* Clip = CONTAINING_RECORD(pwo, XCLIPOBJ, WndObj);

    if (Clip->ChangeProc == NULL)
    {
        return;
    }

    /* check flags of the WNDOBJ */
    flChanged &= Clip->Flags;
    if (flChanged == 0)
    {
        return;
    }

    /* Call the WNDOBJCHANGEPROC */
    if (flChanged == WOC_CHANGED)
    {
        pwo = NULL;
    }

    TRACE("Calling WNDOBJCHANGEPROC (0x%p), Changed = 0x%x\n",
           Clip->ChangeProc, flChanged);
    Clip->ChangeProc(pwo, flChanged);
}

/*
 * Fills the CLIPOBJ and client rect of the WNDOBJ with the data from the given WND
 */
BOOLEAN
FASTCALL
IntEngWndUpdateClipObj(
    XCLIPOBJ* Clip,
    PWND Window)
{
    HRGN hVisRgn;
    PROSRGNDATA visRgn;

    TRACE("IntEngWndUpdateClipObj\n");

    hVisRgn = VIS_ComputeVisibleRegion(Window, TRUE, TRUE, TRUE);
    if (hVisRgn != NULL)
    {
        visRgn = RGNOBJAPI_Lock(hVisRgn, NULL);
        if (visRgn != NULL)
        {
            if (visRgn->rdh.nCount > 0)
            {
                IntEngUpdateClipRegion(Clip, visRgn->rdh.nCount, visRgn->Buffer, &visRgn->rdh.rcBound);
                TRACE("Created visible region with %lu rects\n", visRgn->rdh.nCount);
                TRACE("  BoundingRect: %d, %d  %d, %d\n",
                       visRgn->rdh.rcBound.left, visRgn->rdh.rcBound.top,
                       visRgn->rdh.rcBound.right, visRgn->rdh.rcBound.bottom);
                {
                    ULONG i;
                    for (i = 0; i < visRgn->rdh.nCount; i++)
                    {
                        TRACE("  Rect #%lu: %ld,%ld  %ld,%ld\n", i+1,
                               visRgn->Buffer[i].left, visRgn->Buffer[i].top,
                               visRgn->Buffer[i].right, visRgn->Buffer[i].bottom);
                    }
                }
            }
            RGNOBJAPI_Unlock(visRgn);
        }
        else
        {
            WARN("Couldn't lock visible region of window DC\n");
        }
        GreDeleteObject(hVisRgn);
    }
    else
    {
        /* Fall back to client rect */
        IntEngUpdateClipRegion(Clip, 1, &Window->rcClient, &Window->rcClient);
    }

    /* Update the WNDOBJ */
    Clip->WndObj.rclClient = Window->rcClient;

    return TRUE;
}

/*
 * Updates all WNDOBJs of the given WND and calls the change-procs.
 */
VOID
FASTCALL
IntEngWindowChanged(
    _In_    PWND  Window,
    _In_    FLONG flChanged)
{
    PPROPERTY pprop;
    XCLIPOBJ *Current;
    HWND hWnd;

    ASSERT_IRQL_LESS_OR_EQUAL(PASSIVE_LEVEL);

    hWnd = Window->head.h;
    pprop = IntGetProp(Window, AtomWndObj);
    if (!pprop)
    {
        return;
    }
    Current = (XCLIPOBJ *)pprop->Data;
    if ( gcountPWO &&
            Current &&
            Current->Hwnd == hWnd &&
            Current->WndObj.pvConsumer != NULL )
    {
        /* Update the WNDOBJ */
        switch (flChanged)
        {
        case WOC_RGN_CLIENT:
            /* Update the clipobj and client rect of the WNDOBJ */
            IntEngWndUpdateClipObj(Current, Window);
            break;

        case WOC_DELETE:
            /* FIXME: Should the WNDOBJs be deleted by win32k or by the driver? */
            break;
        }

        /* Call the change proc */
        IntEngWndCallChangeProc(&Current->WndObj, flChanged);

        /* HACK: Send WOC_CHANGED after WOC_RGN_CLIENT */
        if (flChanged == WOC_RGN_CLIENT)
        {
            IntEngWndCallChangeProc(&Current->WndObj, WOC_CHANGED);
        }
    }
}

/*
 * @implemented
 */
WNDOBJ*
APIENTRY
EngCreateWnd(
    SURFOBJ          *pso,
    HWND              hWnd,
    WNDOBJCHANGEPROC  pfn,
    FLONG             fl,
    int               iPixelFormat)
{
    XCLIPOBJ *Clip = NULL;
    WNDOBJ *WndObjUser = NULL;
    PWND Window;
    BOOL calledFromUser;
    DECLARE_RETURN(WNDOBJ*);

    TRACE("EngCreateWnd: pso = 0x%p, hwnd = 0x%p, pfn = 0x%p, fl = 0x%lx, pixfmt = %d\n",
            pso, hWnd, pfn, fl, iPixelFormat);

    calledFromUser = UserIsEntered();
    if (!calledFromUser) {
        UserEnterShared();
    }

    /* Get window object */
    Window = UserGetWindowObject(hWnd);
    if (Window == NULL)
    {
        RETURN( NULL);
    }

    /* Create WNDOBJ */
    Clip = EngAllocMem(FL_ZERO_MEMORY, sizeof (XCLIPOBJ), GDITAG_WNDOBJ);
    if (Clip == NULL)
    {
        ERR("Failed to allocate memory for a WND structure!\n");
        RETURN( NULL);
    }
    IntEngInitClipObj(Clip);

    /* Fill the clipobj */
    if (!IntEngWndUpdateClipObj(Clip, Window))
    {
        EngFreeMem(Clip);
        RETURN( NULL);
    }

    /* Fill user object */
    WndObjUser = &Clip->WndObj;
    WndObjUser->psoOwner = pso;
    WndObjUser->pvConsumer = NULL;

    /* Fill internal object */
    Clip->Hwnd = hWnd;
    Clip->ChangeProc = pfn;
    Clip->Flags = fl;
    Clip->PixelFormat = iPixelFormat;

    /* associate object with window */
    IntSetProp(Window, AtomWndObj, Clip);
    ++gcountPWO;

    TRACE("EngCreateWnd: SUCCESS!\n");

    RETURN( WndObjUser);

CLEANUP:

    if (!calledFromUser) {
        UserLeave();
    }

    END_CLEANUP;
}


/*
 * @implemented
 */
VOID
APIENTRY
EngDeleteWnd(
    IN WNDOBJ *pwo)
{
    XCLIPOBJ* Clip = CONTAINING_RECORD(pwo, XCLIPOBJ, WndObj);
    PWND Window;
    BOOL calledFromUser;

    TRACE("EngDeleteWnd: pwo = 0x%p\n", pwo);

    calledFromUser = UserIsEntered();
    if (!calledFromUser) {
        UserEnterExclusive();
    }

    /* Get window object */
    Window = UserGetWindowObject(Clip->Hwnd);
    if (Window == NULL)
    {
        ERR("Couldnt get window object for WndObjInt->Hwnd!!!\n");
    }
    else
    {
        /* Remove object from window */
        IntRemoveProp(Window, AtomWndObj);
        --gcountPWO;
    }

    if (!calledFromUser) {
        UserLeave();
    }

    /* Free resources */
    IntEngFreeClipResources(Clip);
    EngFreeMem(Clip);
}


/*
 * @implemented
 */
BOOL
APIENTRY
WNDOBJ_bEnum(
    IN WNDOBJ  *pwo,
    IN ULONG  cj,
    OUT ULONG  *pul)
{
    /* Relay */
    return CLIPOBJ_bEnum(&pwo->coClient, cj, pul);
}


/*
 * @implemented
 */
ULONG
APIENTRY
WNDOBJ_cEnumStart(
    IN WNDOBJ  *pwo,
    IN ULONG  iType,
    IN ULONG  iDirection,
    IN ULONG  cLimit)
{
    /* Relay */
    // FIXME: Should we enumerate all rectangles or not?
    return CLIPOBJ_cEnumStart(&pwo->coClient, FALSE, iType, iDirection, cLimit);
}


/*
 * @implemented
 */
VOID
APIENTRY
WNDOBJ_vSetConsumer(
    IN WNDOBJ  *pwo,
    IN PVOID  pvConsumer)
{
    BOOL Hack;

    TRACE("WNDOBJ_vSetConsumer: pwo = 0x%p, pvConsumer = 0x%p\n", pwo, pvConsumer);

    Hack = (pwo->pvConsumer == NULL);
    pwo->pvConsumer = pvConsumer;

    /* HACKHACKHACK
     *
     * MSDN says that the WNDOBJCHANGEPROC will be called with the most recent state
     * when a WNDOBJ is created - we do it here because most drivers will need pvConsumer
     * in the callback to identify the WNDOBJ I think.
     *
     *  - blight
     */
    if (Hack)
    {
        FIXME("Is this hack really needed?\n");
        IntEngWndCallChangeProc(pwo, WOC_RGN_CLIENT);
        IntEngWndCallChangeProc(pwo, WOC_CHANGED);
        IntEngWndCallChangeProc(pwo, WOC_DRAWN);
    }
}

/* EOF */

