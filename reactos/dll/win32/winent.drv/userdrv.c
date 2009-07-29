/*
 * PROJECT:         ReactOS
 * LICENSE:         LGPL
 * FILE:            dll/win32/winent.drv/userdrv.c
 * PURPOSE:         User driver stub for ReactOS/Windows
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <stdarg.h>
#include <stdio.h>
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winuser16.h"
#include "wingdi.h"
#define NTOS_USER_MODE
#include <ndk/ntndk.h>
#include "ntrosgdi.h"
#include "win32k/rosuser.h"
#include "winent.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rosuserdrv);

/* FUNCTIONS **************************************************************/

/***********************************************************************
 *		move_window_bits
 *
 * Move the window bits when a window is moved.
 */
void move_window_bits( HWND hwnd, const RECT *old_rect, const RECT *new_rect,
                              const RECT *old_client_rect )
{
    RECT src_rect = *old_rect;
    RECT dst_rect = *new_rect;
    HDC hdc_src, hdc_dst;
    HRGN rgn = 0;
    HWND parent = 0;

    if (FALSE)
    {
        //OffsetRect( &dst_rect, -data->window_rect.left, -data->window_rect.top );
        parent = GetAncestor( hwnd, GA_PARENT );
        hdc_src = GetDCEx( parent, 0, DCX_CACHE );
        hdc_dst = GetDCEx( hwnd, 0, DCX_CACHE | DCX_WINDOW );
    }
    else
    {
    //OffsetRect( &dst_rect, -data->client_rect.left, -data->client_rect.top );
    /* make src rect relative to the old position of the window */
    OffsetRect( &src_rect, -old_client_rect->left, -old_client_rect->top );
    //if (dst_rect.left == src_rect.left && dst_rect.top == src_rect.top) return;
    hdc_src = hdc_dst = GetDCEx( hwnd, 0, DCX_CACHE );
    }

    //code = X11DRV_START_EXPOSURES;
    //ExtEscape( hdc_dst, X11DRV_ESCAPE, sizeof(code), (LPSTR)&code, 0, NULL );

    ERR( "copying bits for win %p (parent %p)/ %s -> %s\n",
           hwnd, parent,
           wine_dbgstr_rect(&src_rect), wine_dbgstr_rect(&dst_rect) );
    BitBlt( hdc_dst, dst_rect.left, dst_rect.top,
            dst_rect.right - dst_rect.left, dst_rect.bottom - dst_rect.top,
            hdc_src, src_rect.left, src_rect.top, SRCCOPY );

    //code = X11DRV_END_EXPOSURES;
    //ExtEscape( hdc_dst, X11DRV_ESCAPE, sizeof(code), (LPSTR)&code, sizeof(rgn), (LPSTR)&rgn );

    ReleaseDC( hwnd, hdc_dst );
    if (hdc_src != hdc_dst) ReleaseDC( parent, hdc_src );

    if (rgn)
    {
        RedrawWindow( hwnd, NULL, rgn, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN );
        DeleteObject( rgn );
    }
}

HKL CDECL RosDrv_ActivateKeyboardLayout( HKL layout, UINT flags )
{
    UNIMPLEMENTED;
    return 0;
}

void CDECL RosDrv_Beep(void)
{
    UNIMPLEMENTED;
}

SHORT CDECL RosDrv_GetAsyncKeyState( INT key )
{
    UNIMPLEMENTED;
    return 0;
}

INT CDECL RosDrv_GetKeyNameText( LONG lparam, LPWSTR buffer, INT size )
{
    UNIMPLEMENTED;
    return 0;
}

HKL CDECL RosDrv_GetKeyboardLayout( DWORD layout )
{
    UNIMPLEMENTED;
    return 0;
}

BOOL CDECL RosDrv_GetKeyboardLayoutName( LPWSTR name )
{
    UNIMPLEMENTED;
    return FALSE;
}

HKL CDECL RosDrv_LoadKeyboardLayout( LPCWSTR name, UINT flags )
{
    UNIMPLEMENTED;
    return 0;
}

UINT CDECL RosDrv_MapVirtualKeyEx( UINT code, UINT type, HKL layout )
{
    UNIMPLEMENTED;
    return 0;
}

UINT CDECL RosDrv_SendInput( UINT count, LPINPUT inputs, int size )
{
    UINT i;

    for (i = 0; i < count; i++, inputs++)
    {
        switch(inputs->type)
        {
        case INPUT_MOUSE:
            RosDrv_send_mouse_input( 0, inputs->mi.dwFlags, inputs->mi.dx, inputs->mi.dy,
                                     inputs->mi.mouseData, inputs->mi.time,
                                     inputs->mi.dwExtraInfo, LLMHF_INJECTED );
            break;
        case INPUT_KEYBOARD:
            FIXME( "INPUT_KEYBOARD not supported\n" );
            break;
        case INPUT_HARDWARE:
            FIXME( "INPUT_HARDWARE not supported\n" );
            break;
        }
    }
    return count;
}

INT CDECL RosDrv_ToUnicodeEx( UINT virt, UINT scan, const BYTE *state, LPWSTR str,
                                      int size, UINT flags, HKL layout )
{
    UNIMPLEMENTED;
    return 0;
}

BOOL CDECL RosDrv_UnloadKeyboardLayout( HKL layout )
{
    UNIMPLEMENTED;
    return 0;
}

SHORT CDECL RosDrv_VkKeyScanEx( WCHAR ch, HKL layout )
{
    UNIMPLEMENTED;
    return -1;
}

void CDECL RosDrv_SetCursor( HCURSOR hCursor )
{
    ICONINFO IconInfo;
    BITMAP bitmap;

    if (hCursor == NULL)
    {
        RosUserSetCursor(NULL);
    }
    else
    {
        if( GetIconInfo(hCursor, &IconInfo) )
        {
            /* Create handle mappings for IconInfo.hbmMask and
               IconInfo.hbmColor in kernel mode */
            if( GetObjectW(IconInfo.hbmMask, sizeof(bitmap), &bitmap))
                RosGdiCreateBitmap(NULL, IconInfo.hbmMask, &bitmap, bitmap.bmBits);

            if( GetObjectW(IconInfo.hbmColor, sizeof(bitmap), &bitmap))
                RosGdiCreateBitmap(NULL, IconInfo.hbmColor, &bitmap, bitmap.bmBits);

            RosUserSetCursor( &IconInfo );
        }
    }
}

BOOL CDECL RosDrv_GetCursorPos( LPPOINT pt )
{
    return RosUserGetCursorPos( pt );
}

BOOL CDECL RosDrv_SetCursorPos( INT x, INT y )
{
    return RosUserSetCursorPos( x, y );
}

BOOL CDECL RosDrv_ClipCursor( LPCRECT clip )
{
    return RosUserClipCursor( clip );
}

BOOL CDECL RosDrv_GetScreenSaveActive(void)
{
    UNIMPLEMENTED;
    return FALSE;
}

void CDECL RosDrv_SetScreenSaveActive( BOOL on )
{
    UNIMPLEMENTED;
}

INT CDECL RosDrv_AcquireClipboard( HWND hwnd )
{
    UNIMPLEMENTED;
    return 0;
}

BOOL CDECL RosDrv_CountClipboardFormats(void)
{
    UNIMPLEMENTED;
    return 0;
}

void CDECL RosDrv_EmptyClipboard( BOOL keepunowned )
{
    UNIMPLEMENTED;
}

void CDECL RosDrv_EndClipboardUpdate(void)
{
    UNIMPLEMENTED;
}

UINT CDECL RosDrv_EnumClipboardFormats( UINT format )
{
    UNIMPLEMENTED;
    return 0;
}

BOOL CDECL RosDrv_GetClipboardData( UINT format, HANDLE16 *h16, HANDLE *h32 )
{
    UNIMPLEMENTED;
    return FALSE;
}

INT CDECL RosDrv_GetClipboardFormatName( UINT format, LPWSTR buffer, UINT len )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL CDECL RosDrv_IsClipboardFormatAvailable( UINT format )
{
    UNIMPLEMENTED;
    return FALSE;
}

UINT CDECL RosDrv_RegisterClipboardFormat( LPCWSTR name )
{
    UNIMPLEMENTED;
    return 0;
}

BOOL CDECL RosDrv_SetClipboardData( UINT format, HANDLE16 h16, HANDLE h32, BOOL owner )
{
    UNIMPLEMENTED;
    return FALSE;
}

LONG CDECL RosDrv_ChangeDisplaySettingsEx( LPCWSTR name, LPDEVMODEW mode, HWND hwnd,
                                             DWORD flags, LPVOID lparam )
{
    UNIMPLEMENTED;
    return DISP_CHANGE_FAILED;
}

BOOL CDECL RosDrv_EnumDisplayMonitors( HDC hdc, LPRECT rect, MONITORENUMPROC proc, LPARAM lp )
{
    RECT monrect = {0, 0, 640, 480};

    FIXME("RosDrv_EnumDisplayMonitors is a hack\n");

    proc((HMONITOR)1, hdc, &monrect, lp);

    return TRUE;
}

BOOL CDECL RosDrv_EnumDisplaySettingsEx( LPCWSTR name, DWORD num, LPDEVMODEW mode, DWORD flags )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL CDECL RosDrv_GetMonitorInfo( HMONITOR handle, LPMONITORINFO info )
{
    RECT monrect = {0, 0, 640, 480};

    FIXME("RosDrv_GetMonitorInfo(%x %p) is a hack\n", handle, info);

    info->rcMonitor = monrect;
    info->rcWork = monrect;

    return TRUE;
}

BOOL CDECL RosDrv_CreateDesktopWindow( HWND hwnd )
{
    unsigned int width, height;

    /* retrieve the real size of the desktop */
    SERVER_START_REQ( get_window_rectangles )
    {
        req->handle = wine_server_user_handle( hwnd );
        wine_server_call( req );
        width  = reply->window.right - reply->window.left;
        height = reply->window.bottom - reply->window.top;
    }
    SERVER_END_REQ;

    TRACE("RosDrv_CreateDesktopWindow(%x), w %d h %d\n", hwnd, width, height);

    if (!width && !height)  /* not initialized yet */
    {
        SERVER_START_REQ( set_window_pos )
        {
            req->handle        = wine_server_user_handle( hwnd );
            req->previous      = 0;
            req->flags         = SWP_NOZORDER;
            req->window.left   = 0;
            req->window.top    = 0;
            req->window.right  = 800; // FIXME: Use primary surface's dimensions!
            req->window.bottom = 600;
            req->client        = req->window;
            wine_server_call( req );
        }
        SERVER_END_REQ;
    }

    return TRUE;
}

BOOL CDECL RosDrv_CreateWindow( HWND hwnd )
{
    WARN("RosDrv_CreateWindow(%x)\n", hwnd);
    return TRUE;
}

void CDECL RosDrv_DestroyWindow( HWND hwnd )
{
    UNIMPLEMENTED;
}

void CDECL RosDrv_GetDC( HDC hdc, HWND hwnd, HWND top_win, const RECT *win_rect,
                                 const RECT *top_rect, DWORD flags )
{
    struct ntdrv_escape_set_drawable escape;
    //struct ntdrv_win_data *data = X11DRV_get_win_data( hwnd );

    escape.code        = NTDRV_SET_DRAWABLE;
    //escape.mode        = IncludeInferiors;
    //escape.fbconfig_id = 0;
    //escape.gl_drawable = 0;
    //escape.pixmap      = 0;
    escape.gl_copy     = FALSE;

#if 0
    if (top == hwnd && data && IsIconic( hwnd ) && data->icon_window)
    {
        //escape.drawable = data->icon_window;
    }
    else if (top == hwnd)
    {
        escape.fbconfig_id = data ? data->fbconfig_id : (XID)GetPropA( hwnd, fbconfig_id_prop );
        /* GL draws to the client area even for window DCs */
        /*escape.gl_drawable = data ? data->client_window : X11DRV_get_client_window( hwnd );
        if (flags & DCX_WINDOW)
            escape.drawable = data ? data->whole_window : X11DRV_get_whole_window( hwnd );
        else
            escape.drawable = escape.gl_drawable;*/
    }
    else
    {
        //escape.drawable    = X11DRV_get_client_window( top );
        //escape.fbconfig_id = data ? data->fbconfig_id : (XID)GetPropA( hwnd, fbconfig_id_prop );
        //escape.gl_drawable = data ? data->gl_drawable : (Drawable)GetPropA( hwnd, gl_drawable_prop );
        //escape.pixmap      = data ? data->pixmap : (Pixmap)GetPropA( hwnd, pixmap_prop );
        //escape.gl_copy     = (escape.gl_drawable != 0);
        if (flags & DCX_CLIPCHILDREN) escape.mode = ClipByChildren;
    }
#endif

    escape.dc_rect.left         = win_rect->left - top_rect->left;
    escape.dc_rect.top          = win_rect->top - top_rect->top;
    escape.dc_rect.right        = win_rect->right - top_rect->left;
    escape.dc_rect.bottom       = win_rect->bottom - top_rect->top;
    escape.drawable_rect.left   = top_rect->left;
    escape.drawable_rect.top    = top_rect->top;
    escape.drawable_rect.right  = top_rect->right;
    escape.drawable_rect.bottom = top_rect->bottom;

    ExtEscape( hdc, NTDRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );
}

DWORD CDECL RosDrv_MsgWaitForMultipleObjectsEx( DWORD count, const HANDLE *handles, DWORD timeout,
                                                        DWORD mask, DWORD flags )
{
    TRACE("WaitForMultipleObjectsEx(%d %p %d %x %x %x\n", count, handles, timeout, mask, flags);

    if (!count && !timeout) return WAIT_TIMEOUT;
    return WaitForMultipleObjectsEx( count, handles, flags & MWMO_WAITALL,
                                     timeout, flags & MWMO_ALERTABLE );
}

void CDECL RosDrv_ReleaseDC( HWND hwnd, HDC hdc )
{
    struct ntdrv_escape_set_drawable escape;

    escape.code        = NTDRV_SET_DRAWABLE;
    escape.gl_copy     = FALSE;

    escape.dc_rect.left         = 0;
    escape.dc_rect.top          = 0;
    escape.dc_rect.right        = 0;
    escape.dc_rect.bottom       = 0;
    escape.drawable_rect.left   = 0;
    escape.drawable_rect.top    = 0;
    escape.drawable_rect.right  = 0;
    escape.drawable_rect.bottom = 0;

    ExtEscape( hdc, NTDRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );
}

BOOL CDECL RosDrv_ScrollDC( HDC hdc, INT dx, INT dy, const RECT *scroll, const RECT *clip,
                                    HRGN hrgn, LPRECT update )
{
    UNIMPLEMENTED;
    return FALSE;
}

void CDECL RosDrv_SetCapture( HWND hwnd, UINT flags )
{
    UNIMPLEMENTED;
}

void CDECL RosDrv_SetFocus( HWND hwnd )
{
    UNIMPLEMENTED;
}

void CDECL RosDrv_SetLayeredWindowAttributes( HWND hwnd, COLORREF key, BYTE alpha, DWORD flags )
{
    UNIMPLEMENTED;
}

void CDECL RosDrv_SetParent( HWND hwnd, HWND parent, HWND old_parent )
{
    UNIMPLEMENTED;
}

int CDECL RosDrv_SetWindowRgn( HWND hwnd, HRGN hrgn, BOOL redraw )
{
    UNIMPLEMENTED;
    return 1;
}

void CDECL RosDrv_SetWindowIcon( HWND hwnd, UINT type, HICON icon )
{
    UNIMPLEMENTED;
}

void CDECL RosDrv_SetWindowStyle( HWND hwnd, INT offset, STYLESTRUCT *style )
{
    DWORD changed;
    RECT rcWnd, rcClient;
    INT x,y,cx,cy;

    if (hwnd == GetDesktopWindow()) return;
    changed = style->styleNew ^ style->styleOld;

    if (offset == GWL_STYLE && (changed & WS_VISIBLE) && (style->styleNew & WS_VISIBLE))
    {
        /* Do some magic... */
        TRACE("Window %x is being made visible\n", hwnd);
    }

    if (offset == GWL_STYLE && (changed & WS_DISABLED))
    {
        UNIMPLEMENTED;
    }

    if (offset == GWL_EXSTYLE && (changed & WS_EX_LAYERED))
    {
        /* changing WS_EX_LAYERED resets attributes */
        UNIMPLEMENTED;
    }
}

void CDECL RosDrv_SetWindowText( HWND hwnd, LPCWSTR text )
{
    //UNIMPLEMENTED;
}

UINT CDECL RosDrv_ShowWindow( HWND hwnd, INT cmd, RECT *rect, UINT swp )
{
    /*int x, y;
    unsigned int width, height;*/
    DWORD style = GetWindowLongW( hwnd, GWL_STYLE );

    if (style & WS_MINIMIZE) return swp;
    if (IsRectEmpty( rect )) return swp;

    /* only fetch the new rectangle if the ShowWindow was a result of a window manager event */

    TRACE( "win %p cmd %d at %s flags %08x\n",
           hwnd, cmd, wine_dbgstr_rect(rect), swp );

#if 0
    /* HACK */
    x = 1;
    y = 1;
    width = 50;
    height = 50;

    rect->left   = x;
    rect->top    = y;
    rect->right  = x + width;
    rect->bottom = y + height;
    //OffsetRect( rect, virtual_screen_rect.left, virtual_screen_rect.top );
    //X11DRV_X_to_window_rect( data, rect );
#endif
    return swp & ~(SWP_NOMOVE | SWP_NOCLIENTMOVE | SWP_NOSIZE | SWP_NOCLIENTSIZE);
}

LRESULT CDECL RosDrv_SysCommand( HWND hwnd, WPARAM wparam, LPARAM lparam )
{
    UNIMPLEMENTED;
    return -1;
}

LRESULT CDECL RosDrv_WindowMessage( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    UNIMPLEMENTED;
    return 0;
}

void CDECL RosDrv_WindowPosChanging( HWND hwnd, HWND insert_after, UINT swp_flags,
                                             const RECT *window_rect, const RECT *client_rect,
                                             RECT *visible_rect )
{
    //UNIMPLEMENTED;
    *visible_rect = *window_rect;
}

void CDECL RosDrv_WindowPosChanged( HWND hwnd, HWND insert_after, UINT swp_flags,
                                    const RECT *window_rect, const RECT *rectClient,
                                    const RECT *visible_rect, const RECT *valid_rects )
{
#if 0
    RECT old_whole_rect, old_client_rect;
    RECT whole_rect = *visible_rect;
    RECT client_rect = *rectClient;

    old_whole_rect = whole_rect;
    old_client_rect = client_rect;
#endif

    if (valid_rects)
    {
        TRACE("valid_rects[0] (%d, %d)-(%d,%d)\n",
            valid_rects[0].top, valid_rects[0].left, valid_rects[0].bottom, valid_rects[0].right);
    }
#if 0
    if (!IsRectEmpty( &valid_rects[0] ))
    {
        int x_offset = old_whole_rect.left - whole_rect.left;
        int y_offset = old_whole_rect.top - whole_rect.top;

        /* if all that happened is that the whole window moved, copy everything */
        if (!(swp_flags & SWP_FRAMECHANGED) &&
            old_whole_rect.right   - whole_rect.right   == x_offset &&
            old_whole_rect.bottom  - whole_rect.bottom  == y_offset &&
            old_client_rect.left   - client_rect.left   == x_offset &&
            old_client_rect.right  - client_rect.right  == x_offset &&
            old_client_rect.top    - client_rect.top    == y_offset &&
            old_client_rect.bottom - client_rect.bottom == y_offset &&
            !memcmp( &valid_rects[0], &client_rect, sizeof(RECT) ))
        {
            /* if we have an X window the bits will be moved by the X server */
            //if (!data->whole_window)
                move_window_bits( hwnd, &old_whole_rect, &whole_rect, &old_client_rect );
        }
        else
            move_window_bits( hwnd, &valid_rects[1], &valid_rects[0], &old_client_rect );
    }
#endif
}

/* EOF */
