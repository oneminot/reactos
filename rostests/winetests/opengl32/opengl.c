/*
 * Some tests for OpenGL functions
 *
 * Copyright (C) 2007-2008 Roderick Colenbrander
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <windows.h>
#include <wingdi.h>
#include "wine/test.h"

void WINAPI glClearColor(float red, float green, float blue, float alpha);
void WINAPI glClear(unsigned int mask);
void WINAPI glFinish(void);
#define GL_COLOR_BUFFER_BIT 0x00004000
const unsigned char * WINAPI glGetString(unsigned int);
#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02

#define MAX_FORMATS 256
typedef void* HPBUFFERARB;

/* WGL_ARB_create_context */
HGLRC (WINAPI *pwglCreateContextAttribsARB)(HDC hDC, HGLRC hShareContext, const int *attribList);
/* GetLastError */
#define ERROR_INVALID_VERSION_ARB 0x2095
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB 0x2093
#define WGL_CONTEXT_FLAGS_ARB 0x2094
/* Flags for WGL_CONTEXT_FLAGS_ARB */
#define WGL_CONTEXT_DEBUG_BIT_ARB 0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB	0x0002

/* WGL_ARB_extensions_string */
static const char* (WINAPI *pwglGetExtensionsStringARB)(HDC);
static int (WINAPI *pwglReleasePbufferDCARB)(HPBUFFERARB, HDC);

/* WGL_ARB_make_current_read */
static BOOL (WINAPI *pwglMakeContextCurrentARB)(HDC hdraw, HDC hread, HGLRC hglrc);
static HDC (WINAPI *pwglGetCurrentReadDCARB)(void);

/* WGL_ARB_pixel_format */
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_COLOR_BITS_ARB 0x2014
#define WGL_RED_BITS_ARB   0x2015
#define WGL_GREEN_BITS_ARB 0x2017
#define WGL_BLUE_BITS_ARB  0x2019
#define WGL_ALPHA_BITS_ARB 0x201B
#define WGL_SUPPORT_GDI_ARB   0x200F
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_NO_ACCELERATION_ARB        0x2025
#define WGL_GENERIC_ACCELERATION_ARB   0x2026
#define WGL_FULL_ACCELERATION_ARB      0x2027

static BOOL (WINAPI *pwglChoosePixelFormatARB)(HDC, const int *, const FLOAT *, UINT, int *, UINT *);
static BOOL (WINAPI *pwglGetPixelFormatAttribivARB)(HDC, int, int, UINT, const int *, int *);

/* WGL_ARB_pbuffer */
#define WGL_DRAW_TO_PBUFFER_ARB 0x202D
static HPBUFFERARB* (WINAPI *pwglCreatePbufferARB)(HDC, int, int, int, const int *);
static HDC (WINAPI *pwglGetPbufferDCARB)(HPBUFFERARB);

static const char* wgl_extensions = NULL;

static void init_functions(void)
{
#define GET_PROC(func) \
    p ## func = (void*)wglGetProcAddress(#func); \
    if(!p ## func) \
      trace("wglGetProcAddress(%s) failed\n", #func);

    /* WGL_ARB_create_context */
    GET_PROC(wglCreateContextAttribsARB);

    /* WGL_ARB_extensions_string */
    GET_PROC(wglGetExtensionsStringARB)

    /* WGL_ARB_make_current_read */
    GET_PROC(wglMakeContextCurrentARB);
    GET_PROC(wglGetCurrentReadDCARB);

    /* WGL_ARB_pixel_format */
    GET_PROC(wglChoosePixelFormatARB)
    GET_PROC(wglGetPixelFormatAttribivARB)

    /* WGL_ARB_pbuffer */
    GET_PROC(wglCreatePbufferARB)
    GET_PROC(wglGetPbufferDCARB)
    GET_PROC(wglReleasePbufferDCARB)

#undef GET_PROC
}

static void test_pbuffers(HDC hdc)
{
    const int iAttribList[] = { WGL_DRAW_TO_PBUFFER_ARB, 1, /* Request pbuffer support */
                                0 };
    int iFormats[MAX_FORMATS];
    unsigned int nOnscreenFormats;
    unsigned int nFormats;
    int i, res;
    int iPixelFormat = 0;

    nOnscreenFormats = DescribePixelFormat(hdc, 0, 0, NULL);

    /* When you want to render to a pbuffer you need to call wglGetPbufferDCARB which
     * returns a 'magic' HDC which you can then pass to wglMakeCurrent to switch rendering
     * to the pbuffer. Below some tests are performed on what happens if you use standard WGL calls
     * on this 'magic' HDC for both a pixelformat that support onscreen and offscreen rendering
     * and a pixelformat that's only available for offscreen rendering (this means that only
     * wglChoosePixelFormatARB and friends know about the format.
     *
     * The first thing we need are pixelformats with pbuffer capabilities.
     */
    res = pwglChoosePixelFormatARB(hdc, iAttribList, NULL, MAX_FORMATS, iFormats, &nFormats);
    if(res <= 0)
    {
        skip("No pbuffer compatible formats found while WGL_ARB_pbuffer is supported\n");
        return;
    }
    trace("nOnscreenFormats: %d\n", nOnscreenFormats);
    trace("Total number of pbuffer capable pixelformats: %d\n", nFormats);

    /* Try to select an onscreen pixelformat out of the list */
    for(i=0; i < nFormats; i++)
    {
        /* Check if the format is onscreen, if it is choose it */
        if(iFormats[i] <= nOnscreenFormats)
        {
            iPixelFormat = iFormats[i];
            trace("Selected iPixelFormat=%d\n", iPixelFormat);
            break;
        }
    }

    /* A video driver supports a large number of onscreen and offscreen pixelformats.
     * The traditional WGL calls only see a subset of the whole pixelformat list. First
     * of all they only see the onscreen formats (the offscreen formats are at the end of the
     * pixelformat list) and second extended pixelformat capabilities are hidden from the
     * standard WGL calls. Only functions that depend on WGL_ARB_pixel_format can see them.
     *
     * Below we check if the pixelformat is also supported onscreen.
     */
    if(iPixelFormat != 0)
    {
        HDC pbuffer_hdc;
        int attrib = 0;
        HPBUFFERARB pbuffer = pwglCreatePbufferARB(hdc, iPixelFormat, 640 /* width */, 480 /* height */, &attrib);
        if(!pbuffer)
            skip("Pbuffer creation failed!\n");

        /* Test the pixelformat returned by GetPixelFormat on a pbuffer as the behavior is not clear */
        pbuffer_hdc = pwglGetPbufferDCARB(pbuffer);
        res = GetPixelFormat(pbuffer_hdc);
        ok(res == iPixelFormat, "Unexpected iPixelFormat=%d returned by GetPixelFormat for format %d\n", res, iPixelFormat);
        trace("iPixelFormat returned by GetPixelFormat: %d\n", res);
        trace("PixelFormat from wglChoosePixelFormatARB: %d\n", iPixelFormat);

        pwglReleasePbufferDCARB(pbuffer, hdc);
    }
    else skip("Pbuffer test for onscreen pixelformat skipped as no onscreen format with pbuffer capabilities have been found\n");

    /* Search for a real offscreen format */
    for(i=0, iPixelFormat=0; i<nFormats; i++)
    {
        if(iFormats[i] > nOnscreenFormats)
        {
            iPixelFormat = iFormats[i];
            trace("Selected iPixelFormat: %d\n", iPixelFormat);
            break;
        }
    }

    if(iPixelFormat != 0)
    {
        HDC pbuffer_hdc;
        HPBUFFERARB pbuffer = pwglCreatePbufferARB(hdc, iPixelFormat, 640 /* width */, 480 /* height */, NULL);
        if(!pbuffer)
            skip("Pbuffer creation failed!\n");

        /* Test the pixelformat returned by GetPixelFormat on a pbuffer as the behavior is not clear */
        pbuffer_hdc = pwglGetPbufferDCARB(pbuffer);
        res = GetPixelFormat(pbuffer_hdc);

        ok(res == 1, "Unexpected iPixelFormat=%d (1 expected) returned by GetPixelFormat for offscreen format %d\n", res, iPixelFormat);
        trace("iPixelFormat returned by GetPixelFormat: %d\n", res);
        trace("PixelFormat from wglChoosePixelFormatARB: %d\n", iPixelFormat);
        pwglReleasePbufferDCARB(pbuffer, hdc);
    }
    else skip("Pbuffer test for offscreen pixelformat skipped as no offscreen-only format with pbuffer capabilities has been found\n");
}

static void test_setpixelformat(HDC winhdc)
{
    int res = 0;
    int nCfgs;
    int pf;
    int i;
    HWND hwnd;
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,                     /* version */
        PFD_DRAW_TO_WINDOW |
        PFD_SUPPORT_OPENGL |
        PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        24,                    /* 24-bit color depth */
        0, 0, 0, 0, 0, 0,      /* color bits */
        0,                     /* alpha buffer */
        0,                     /* shift bit */
        0,                     /* accumulation buffer */
        0, 0, 0, 0,            /* accum bits */
        32,                    /* z-buffer */
        0,                     /* stencil buffer */
        0,                     /* auxiliary buffer */
        PFD_MAIN_PLANE,        /* main layer */
        0,                     /* reserved */
        0, 0, 0                /* layer masks */
    };

    HDC hdc = GetDC(0);
    ok(hdc != 0, "GetDC(0) failed!\n");

    /* This should pass even on the main device context */
    pf = ChoosePixelFormat(hdc, &pfd);
    ok(pf != 0, "ChoosePixelFormat failed on main device context\n");

    /* SetPixelFormat on the main device context 'X root window' should fail,
     * but some broken drivers allow it
     */
    res = SetPixelFormat(hdc, pf, &pfd);
    trace("SetPixelFormat on main device context %s\n", res ? "succeeded" : "failed");

    /* Setting the same format that was set on the HDC is allowed; other
       formats fail */
    nCfgs = DescribePixelFormat(winhdc, 0, 0, NULL);
    pf = GetPixelFormat(winhdc);
    for(i = 1;i <= nCfgs;i++)
    {
        int res = SetPixelFormat(winhdc, i, NULL);
        if(i == pf) ok(res, "Failed to set the same pixel format\n");
        else ok(!res, "Unexpectedly set an alternate pixel format\n");
    }

    hwnd = CreateWindow("static", "Title", WS_OVERLAPPEDWINDOW,
                        10, 10, 200, 200, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "err: %d\n", GetLastError());
    if (hwnd)
    {
        HDC hdc = GetDC( hwnd );
        pf = ChoosePixelFormat( hdc, &pfd );
        ok( pf != 0, "ChoosePixelFormat failed\n" );
        res = SetPixelFormat( hdc, pf, &pfd );
        ok( res != 0, "SetPixelFormat failed\n" );
        i = GetPixelFormat( hdc );
        ok( i == pf, "GetPixelFormat returned wrong format %d/%d\n", i, pf );
        ReleaseDC( hwnd, hdc );
        hdc = GetWindowDC( hwnd );
        i = GetPixelFormat( hdc );
        ok( i == pf, "GetPixelFormat returned wrong format %d/%d\n", i, pf );
        ReleaseDC( hwnd, hdc );
        DestroyWindow( hwnd );
    }

    hwnd = CreateWindow("static", "Title", WS_OVERLAPPEDWINDOW,
                        10, 10, 200, 200, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "err: %d\n", GetLastError());
    if (hwnd)
    {
        HDC hdc = GetWindowDC( hwnd );
        pf = ChoosePixelFormat( hdc, &pfd );
        ok( pf != 0, "ChoosePixelFormat failed\n" );
        res = SetPixelFormat( hdc, pf, &pfd );
        ok( res != 0, "SetPixelFormat failed\n" );
        i = GetPixelFormat( hdc );
        ok( i == pf, "GetPixelFormat returned wrong format %d/%d\n", i, pf );
        ReleaseDC( hwnd, hdc );
        DestroyWindow( hwnd );
    }
}

static void test_sharelists(HDC winhdc)
{
    HGLRC hglrc1, hglrc2, hglrc3;
    int res;

    hglrc1 = wglCreateContext(winhdc);
    res = wglShareLists(0, 0);
    ok(res == FALSE, "Sharing display lists for no contexts passed!\n");

    /* Test 1: Create a context and just share lists without doing anything special */
    hglrc2 = wglCreateContext(winhdc);
    if(hglrc2)
    {
        res = wglShareLists(hglrc1, hglrc2);
        ok(res, "Sharing of display lists failed\n");
        wglDeleteContext(hglrc2);
    }

    /* Test 2: Share display lists with a 'destination' context which has been made current */
    hglrc2 = wglCreateContext(winhdc);
    if(hglrc2)
    {
        res = wglMakeCurrent(winhdc, hglrc2);
        ok(res, "Make current failed\n");
        res = wglShareLists(hglrc1, hglrc2);
        todo_wine ok(res, "Sharing display lists with a destination context which has been made current failed\n");
        wglMakeCurrent(0, 0);
        wglDeleteContext(hglrc2);
    }

    /* Test 3: Share display lists with a context which already shares display lists with another context.
     * According to MSDN the second parameter cannot share any display lists but some buggy drivers might allow it */
    hglrc3 = wglCreateContext(winhdc);
    if(hglrc3)
    {
        res = wglShareLists(hglrc3, hglrc1);
        ok(res == FALSE, "Sharing of display lists passed for a context which already shared lists before\n");
        wglDeleteContext(hglrc3);
    }

    /* Test 4: Share display lists with a 'source' context which has been made current */
    hglrc2 = wglCreateContext(winhdc);
    if(hglrc2)
    {
        res = wglMakeCurrent(winhdc, hglrc1);
        ok(res, "Make current failed\n");
        res = wglShareLists(hglrc1, hglrc2);
        ok(res, "Sharing display lists with a source context which has been made current failed\n");
        wglMakeCurrent(0, 0);
        wglDeleteContext(hglrc2);
    }
}

static void test_makecurrent(HDC winhdc)
{
    BOOL ret;
    HGLRC hglrc;
    DWORD error;

    hglrc = wglCreateContext(winhdc);
    ok( hglrc != 0, "wglCreateContext failed\n" );

    ret = wglMakeCurrent( winhdc, hglrc );
    ok( ret, "wglMakeCurrent failed\n" );

    ok( wglGetCurrentContext() == hglrc, "wrong context\n" );

    /* set the same context again */
    ret = wglMakeCurrent( winhdc, hglrc );
    ok( ret, "wglMakeCurrent failed\n" );

    /* check wglMakeCurrent(x, y) after another call to wglMakeCurrent(x, y) */
    ret = wglMakeCurrent( winhdc, NULL );
    ok( ret, "wglMakeCurrent failed\n" );

    ret = wglMakeCurrent( winhdc, NULL );
    ok( ret, "wglMakeCurrent failed\n" );

    SetLastError( 0xdeadbeef );
    ret = wglMakeCurrent( NULL, NULL );
    ok( !ret, "wglMakeCurrent succeeded\n" );
    error = GetLastError();
    ok( error == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got error=%x\n", error);

    ret = wglMakeCurrent( winhdc, NULL );
    ok( ret, "wglMakeCurrent failed\n" );

    ret = wglMakeCurrent( winhdc, hglrc );
    ok( ret, "wglMakeCurrent failed\n" );

    ret = wglMakeCurrent( NULL, NULL );
    ok( ret, "wglMakeCurrent failed\n" );

    SetLastError( 0xdeadbeef );
    ret = wglMakeCurrent( NULL, NULL );
    ok( !ret, "wglMakeCurrent succeeded\n" );
    error = GetLastError();
    ok( error == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got error=%x\n", error);

    ret = wglMakeCurrent( winhdc, hglrc );
    ok( ret, "wglMakeCurrent failed\n" );
}

static void test_colorbits(HDC hdc)
{
    const int iAttribList[] = { WGL_COLOR_BITS_ARB, WGL_RED_BITS_ARB, WGL_GREEN_BITS_ARB,
                                WGL_BLUE_BITS_ARB, WGL_ALPHA_BITS_ARB };
    int iAttribRet[sizeof(iAttribList)/sizeof(iAttribList[0])];
    const int iAttribs[] = { WGL_ALPHA_BITS_ARB, 1, 0 };
    unsigned int nFormats;
    int res;
    int iPixelFormat = 0;

    if (!pwglChoosePixelFormatARB)
    {
        win_skip("wglChoosePixelFormatARB is not available\n");
        return;
    }

    /* We need a pixel format with at least one bit of alpha */
    res = pwglChoosePixelFormatARB(hdc, iAttribs, NULL, 1, &iPixelFormat, &nFormats);
    if(res == FALSE || nFormats == 0)
    {
        skip("No suitable pixel formats found\n");
        return;
    }

    res = pwglGetPixelFormatAttribivARB(hdc, iPixelFormat, 0,
              sizeof(iAttribList)/sizeof(iAttribList[0]), iAttribList, iAttribRet);
    if(res == FALSE)
    {
        skip("wglGetPixelFormatAttribivARB failed\n");
        return;
    }
    iAttribRet[1] += iAttribRet[2]+iAttribRet[3]+iAttribRet[4];
    ok(iAttribRet[0] == iAttribRet[1], "WGL_COLOR_BITS_ARB (%d) does not equal R+G+B+A (%d)!\n",
                                       iAttribRet[0], iAttribRet[1]);
}

static void test_gdi_dbuf(HDC hdc)
{
    const int iAttribList[] = { WGL_SUPPORT_GDI_ARB, WGL_DOUBLE_BUFFER_ARB };
    int iAttribRet[sizeof(iAttribList)/sizeof(iAttribList[0])];
    unsigned int nFormats;
    int iPixelFormat;
    int res;

    if (!pwglGetPixelFormatAttribivARB)
    {
        win_skip("wglGetPixelFormatAttribivARB is not available\n");
        return;
    }

    nFormats = DescribePixelFormat(hdc, 0, 0, NULL);
    for(iPixelFormat = 1;iPixelFormat <= nFormats;iPixelFormat++)
    {
        res = pwglGetPixelFormatAttribivARB(hdc, iPixelFormat, 0,
                  sizeof(iAttribList)/sizeof(iAttribList[0]), iAttribList,
                  iAttribRet);
        ok(res!=FALSE, "wglGetPixelFormatAttribivARB failed for pixel format %d\n", iPixelFormat);
        if(res == FALSE)
            continue;

        ok(!(iAttribRet[0] && iAttribRet[1]), "GDI support and double buffering on pixel format %d\n", iPixelFormat);
    }
}

static void test_acceleration(HDC hdc)
{
    const int iAttribList[] = { WGL_ACCELERATION_ARB };
    int iAttribRet[sizeof(iAttribList)/sizeof(iAttribList[0])];
    unsigned int nFormats;
    int iPixelFormat;
    int res;
    PIXELFORMATDESCRIPTOR pfd;

    if (!pwglGetPixelFormatAttribivARB)
    {
        win_skip("wglGetPixelFormatAttribivARB is not available\n");
        return;
    }

    nFormats = DescribePixelFormat(hdc, 0, 0, NULL);
    for(iPixelFormat = 1; iPixelFormat <= nFormats; iPixelFormat++)
    {
        res = pwglGetPixelFormatAttribivARB(hdc, iPixelFormat, 0,
                  sizeof(iAttribList)/sizeof(iAttribList[0]), iAttribList,
                  iAttribRet);
        ok(res!=FALSE, "wglGetPixelFormatAttribivARB failed for pixel format %d\n", iPixelFormat);
        if(res == FALSE)
            continue;

        memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
        DescribePixelFormat(hdc, iPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

        switch(iAttribRet[0])
        {
            case WGL_NO_ACCELERATION_ARB:
                ok( (pfd.dwFlags & (PFD_GENERIC_FORMAT | PFD_GENERIC_ACCELERATED)) == PFD_GENERIC_FORMAT , "Expected only PFD_GENERIC_FORMAT to be set for WGL_NO_ACCELERATION_ARB!: iPixelFormat=%d, dwFlags=%x!\n", iPixelFormat, pfd.dwFlags);
                break;
            case WGL_GENERIC_ACCELERATION_ARB:
                ok( (pfd.dwFlags & (PFD_GENERIC_FORMAT | PFD_GENERIC_ACCELERATED)) == (PFD_GENERIC_FORMAT | PFD_GENERIC_ACCELERATED), "Expected both PFD_GENERIC_FORMAT and PFD_GENERIC_ACCELERATION to be set for WGL_GENERIC_ACCELERATION_ARB: iPixelFormat=%d, dwFlags=%x!\n", iPixelFormat, pfd.dwFlags);
                break;
            case WGL_FULL_ACCELERATION_ARB:
                ok( (pfd.dwFlags & (PFD_GENERIC_FORMAT | PFD_GENERIC_ACCELERATED)) == 0, "Expected no PFD_GENERIC_FORMAT/_ACCELERATION to be set for WGL_FULL_ACCELERATION_ARB: iPixelFormat=%d, dwFlags=%x!\n", iPixelFormat, pfd.dwFlags);
                break;
        }
    }
}

static void test_bitmap_rendering(void)
{
    PIXELFORMATDESCRIPTOR pfd;
    int i, iPixelFormat=0;
    unsigned int nFormats;
    HGLRC hglrc;
    BITMAPINFO biDst;
    HBITMAP bmpDst, oldDst;
    HDC hdcDst, hdcScreen;
    UINT32 *dstBuffer;

    memset(&biDst, 0, sizeof(BITMAPINFO));
    biDst.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    biDst.bmiHeader.biWidth = 2;
    biDst.bmiHeader.biHeight = -2;
    biDst.bmiHeader.biPlanes = 1;
    biDst.bmiHeader.biBitCount = 32;
    biDst.bmiHeader.biCompression = BI_RGB;

    hdcScreen = CreateCompatibleDC(0);
    if(GetDeviceCaps(hdcScreen, BITSPIXEL) != 32)
    {
        DeleteDC(hdcScreen);
        trace("Skipping bitmap rendering test\n");
        return;
    }

    hdcDst = CreateCompatibleDC(hdcScreen);
    bmpDst = CreateDIBSection(hdcDst, &biDst, DIB_RGB_COLORS, (void**)&dstBuffer, NULL, 0);
    oldDst = SelectObject(hdcDst, bmpDst);

    /* Pick a pixel format by hand because ChoosePixelFormat is unreliable */
    nFormats = DescribePixelFormat(hdcDst, 0, 0, NULL);
    for(i=1; i<=nFormats; i++)
    {
        memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
        DescribePixelFormat(hdcDst, i, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

        if((pfd.dwFlags & PFD_DRAW_TO_BITMAP) &&
           (pfd.dwFlags & PFD_SUPPORT_OPENGL) &&
           (pfd.cColorBits == 32) &&
           (pfd.cAlphaBits == 8) )
        {
            iPixelFormat = i;
            break;
        }
    }

    if(!iPixelFormat)
    {
        skip("Unable to find a suitable pixel format\n");
    }
    else
    {
        SetPixelFormat(hdcDst, iPixelFormat, &pfd);
        hglrc = wglCreateContext(hdcDst);
        todo_wine ok(hglrc != NULL, "Unable to create a context\n");

        if(hglrc)
        {
            wglMakeCurrent(hdcDst, hglrc);

            /* Note this is RGBA but we read ARGB back */
            glClearColor((float)0x22/0xff, (float)0x33/0xff, (float)0x44/0xff, (float)0x11/0xff);
            glClear(GL_COLOR_BUFFER_BIT);
            glFinish();

            /* Note apparently the alpha channel is not supported by the software renderer (bitmap only works using software) */
            ok(dstBuffer[0] == 0x223344, "Expected color=0x223344, received color=%x\n", dstBuffer[0]);

            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(hglrc);
        }
    }

    SelectObject(hdcDst, oldDst);
    DeleteObject(bmpDst);
    DeleteDC(hdcDst);

    DeleteDC(hdcScreen);
}

struct wgl_thread_param
{
    HANDLE test_finished;
    HGLRC hglrc;
    BOOL hglrc_deleted;
};

static DWORD WINAPI wgl_thread(void *param)
{
    struct wgl_thread_param *p = param;

    p->hglrc_deleted = wglDeleteContext(p->hglrc);
    SetEvent(p->test_finished);

    return 0;
}

static void test_deletecontext(HDC hdc)
{
    struct wgl_thread_param thread_params;
    HGLRC hglrc = wglCreateContext(hdc);
    HANDLE thread_handle;
    DWORD res, tid;

    if(!hglrc)
    {
        skip("wglCreateContext failed!\n");
        return;
    }

    res = wglMakeCurrent(hdc, hglrc);
    if(!res)
    {
        skip("wglMakeCurrent failed!\n");
        return;
    }

    /* WGL doesn't allow you to delete a context from a different thread than the one in which it is current.
     * This differs from GLX which does allow it but it delays actual deletion until the context becomes not current.
     */
    thread_params.hglrc = hglrc;
    thread_params.test_finished = CreateEvent(NULL, FALSE, FALSE, NULL);
    thread_handle = CreateThread(NULL, 0, wgl_thread, &thread_params, 0, &tid);
    ok(!!thread_handle, "Failed to create thread, last error %#x.\n", GetLastError());
    if(thread_handle)
    {
        WaitForSingleObject(thread_handle, INFINITE);
        ok(thread_params.hglrc_deleted == FALSE, "Attempt to delete WGL context from another thread passed but should fail!\n");
    }
    CloseHandle(thread_params.test_finished);

    res = wglDeleteContext(hglrc);
    ok(res == TRUE, "wglDeleteContext failed\n");

    /* WGL makes a context not current when deleting it. This differs from GLX behavior where
     * deletion takes place when the thread becomes not current. */
    hglrc = wglGetCurrentContext();
    ok(hglrc == NULL, "A WGL context is active while none was expected\n");
}

static void test_make_current_read(HDC hdc)
{
    int res;
    HDC hread;
    HGLRC hglrc = wglCreateContext(hdc);

    if(!hglrc)
    {
        skip("wglCreateContext failed!\n");
        return;
    }

    res = wglMakeCurrent(hdc, hglrc);
    if(!res)
    {
        skip("wglMakeCurrent failed!\n");
        return;
    }

    /* Test what wglGetCurrentReadDCARB does for wglMakeCurrent as the spec doesn't mention it */
    hread = pwglGetCurrentReadDCARB();
    trace("hread %p, hdc %p\n", hread, hdc);
    ok(hread == hdc, "wglGetCurrentReadDCARB failed for standard wglMakeCurrent\n");

    pwglMakeContextCurrentARB(hdc, hdc, hglrc);
    hread = pwglGetCurrentReadDCARB();
    ok(hread == hdc, "wglGetCurrentReadDCARB failed for wglMakeContextCurrent\n");
}

static void test_dc(HWND hwnd, HDC hdc)
{
    int pf1, pf2;
    HDC hdc2;

    /* Get another DC and make sure it has the same pixel format */
    hdc2 = GetDC(hwnd);
    if(hdc != hdc2)
    {
        pf1 = GetPixelFormat(hdc);
        pf2 = GetPixelFormat(hdc2);
        ok(pf1 == pf2, "Second DC does not have the same format (%d != %d)\n", pf1, pf2);
    }
    else
        skip("Could not get a different DC for the window\n");

    if(hdc2)
    {
        ReleaseDC(hwnd, hdc2);
        hdc2 = NULL;
    }
}

/* Nvidia converts win32 error codes to (0xc007 << 16) | win32_error_code */
#define NVIDIA_HRESULT_FROM_WIN32(x) (HRESULT_FROM_WIN32(x) | 0x40000000)
static void test_opengl3(HDC hdc)
{
    /* Try to create a context compatible with OpenGL 1.x; 1.0-2.1 is allowed */
    {
        HGLRC gl3Ctx;
        int attribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 1, 0};

        gl3Ctx = pwglCreateContextAttribsARB(hdc, 0, attribs);
        ok(gl3Ctx != 0, "pwglCreateContextAttribsARB for a 1.x context failed!\n");
        wglDeleteContext(gl3Ctx);
    }

    /* Try to pass an invalid HDC */
    {
        HGLRC gl3Ctx;
        DWORD error;
        gl3Ctx = pwglCreateContextAttribsARB((HDC)0xdeadbeef, 0, 0);
        ok(gl3Ctx == 0, "pwglCreateContextAttribsARB using an invalid HDC passed\n");
        error = GetLastError();
        todo_wine ok(error == ERROR_DC_NOT_FOUND ||
                     broken(error == NVIDIA_HRESULT_FROM_WIN32(ERROR_INVALID_DATA)), /* Nvidia Vista + Win7 */
                     "Expected ERROR_DC_NOT_FOUND, got error=%x\n", error);
        wglDeleteContext(gl3Ctx);
    }

    /* Try to pass an invalid shareList */
    {
        HGLRC gl3Ctx;
        DWORD error;
        gl3Ctx = pwglCreateContextAttribsARB(hdc, (HGLRC)0xdeadbeef, 0);
        todo_wine ok(gl3Ctx == 0, "pwglCreateContextAttribsARB using an invalid shareList passed\n");
        error = GetLastError();
        /* The Nvidia implementation seems to return hresults instead of win32 error codes */
        todo_wine ok(error == ERROR_INVALID_OPERATION ||
                     error == NVIDIA_HRESULT_FROM_WIN32(ERROR_INVALID_OPERATION), "Expected ERROR_INVALID_OPERATION, got error=%x\n", error);
        wglDeleteContext(gl3Ctx);
    }

    /* Try to create an OpenGL 3.0 context */
    {
        int attribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 3, WGL_CONTEXT_MINOR_VERSION_ARB, 0, 0};
        HGLRC gl3Ctx = pwglCreateContextAttribsARB(hdc, 0, attribs);

        if(gl3Ctx == NULL)
        {
            skip("Skipping the rest of the WGL_ARB_create_context test due to lack of OpenGL 3.0\n");
            return;
        }

        wglDeleteContext(gl3Ctx);
    }

    /* Test matching an OpenGL 3.0 context with an older one, OpenGL 3.0 should allow it until the new object model is introduced in a future revision */
    {
        HGLRC glCtx = wglCreateContext(hdc);

        int attribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 3, WGL_CONTEXT_MINOR_VERSION_ARB, 0, 0};
        int attribs_future[] = {WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB, WGL_CONTEXT_MAJOR_VERSION_ARB, 3, WGL_CONTEXT_MINOR_VERSION_ARB, 0, 0};

        HGLRC gl3Ctx = pwglCreateContextAttribsARB(hdc, glCtx, attribs);
        ok(gl3Ctx != NULL, "Sharing of a display list between OpenGL 3.0 and OpenGL 1.x/2.x failed!\n");
        if(gl3Ctx)
            wglDeleteContext(gl3Ctx);

        gl3Ctx = pwglCreateContextAttribsARB(hdc, glCtx, attribs_future);
        ok(gl3Ctx != NULL, "Sharing of a display list between a forward compatible OpenGL 3.0 context and OpenGL 1.x/2.x failed!\n");
        if(gl3Ctx)
            wglDeleteContext(gl3Ctx);

        if(glCtx)
            wglDeleteContext(glCtx);
    }

    /* Try to create an OpenGL 3.0 context and test windowless rendering */
    {
        HGLRC gl3Ctx;
        int attribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 3, WGL_CONTEXT_MINOR_VERSION_ARB, 0, 0};
        BOOL res;

        gl3Ctx = pwglCreateContextAttribsARB(hdc, 0, attribs);
        ok(gl3Ctx != 0, "pwglCreateContextAttribsARB for a 3.0 context failed!\n");

        /* OpenGL 3.0 allows offscreen rendering WITHOUT a drawable
         * Neither AMD or Nvidia support it at this point. The WGL_ARB_create_context specs also say that
         * it is hard because drivers use the HDC to enter the display driver and it sounds like they don't
         * expect drivers to ever offer it.
         */
        res = wglMakeCurrent(0, gl3Ctx);
        ok(res == FALSE, "Wow, OpenGL 3.0 windowless rendering passed while it was expected not to!\n");
        if(res)
            wglMakeCurrent(0, 0);

        if(gl3Ctx)
            wglDeleteContext(gl3Ctx);
    }
}

static void test_minimized(void)
{
    PIXELFORMATDESCRIPTOR pf_desc =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,                     /* version */
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        24,                    /* 24-bit color depth */
        0, 0, 0, 0, 0, 0,      /* color bits */
        0,                     /* alpha buffer */
        0,                     /* shift bit */
        0,                     /* accumulation buffer */
        0, 0, 0, 0,            /* accum bits */
        32,                    /* z-buffer */
        0,                     /* stencil buffer */
        0,                     /* auxiliary buffer */
        PFD_MAIN_PLANE,        /* main layer */
        0,                     /* reserved */
        0, 0, 0                /* layer masks */
    };
    int pixel_format;
    HWND window;
    LONG style;
    HGLRC ctx;
    BOOL ret;
    HDC dc;

    window = CreateWindowA("static", "opengl32_test",
            WS_POPUP | WS_MINIMIZE, 0, 0, 640, 480, 0, 0, 0, 0);
    ok(!!window, "Failed to create window, last error %#x.\n", GetLastError());

    dc = GetDC(window);
    ok(!!dc, "Failed to get DC.\n");

    pixel_format = ChoosePixelFormat(dc, &pf_desc);
    if (!pixel_format)
    {
        win_skip("Failed to find pixel format.\n");
        ReleaseDC(window, dc);
        DestroyWindow(window);
        return;
    }

    ret = SetPixelFormat(dc, pixel_format, &pf_desc);
    ok(ret, "Failed to set pixel format, last error %#x.\n", GetLastError());

    style = GetWindowLongA(window, GWL_STYLE);
    ok(style & WS_MINIMIZE, "Window should be minimized, got style %#x.\n", style);

    ctx = wglCreateContext(dc);
    ok(!!ctx, "Failed to create GL context, last error %#x.\n", GetLastError());

    ret = wglMakeCurrent(dc, ctx);
    ok(ret, "Failed to make context current, last error %#x.\n", GetLastError());

    style = GetWindowLongA(window, GWL_STYLE);
    ok(style & WS_MINIMIZE, "window should be minimized, got style %#x.\n", style);

    ret = wglMakeCurrent(NULL, NULL);
    ok(ret, "Failed to clear current context, last error %#x.\n", GetLastError());

    ret = wglDeleteContext(ctx);
    ok(ret, "Failed to delete GL context, last error %#x.\n", GetLastError());

    ReleaseDC(window, dc);
    DestroyWindow(window);
}

START_TEST(opengl)
{
    HWND hwnd;
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,                     /* version */
        PFD_DRAW_TO_WINDOW |
        PFD_SUPPORT_OPENGL |
        PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        24,                    /* 24-bit color depth */
        0, 0, 0, 0, 0, 0,      /* color bits */
        0,                     /* alpha buffer */
        0,                     /* shift bit */
        0,                     /* accumulation buffer */
        0, 0, 0, 0,            /* accum bits */
        32,                    /* z-buffer */
        0,                     /* stencil buffer */
        0,                     /* auxiliary buffer */
        PFD_MAIN_PLANE,        /* main layer */
        0,                     /* reserved */
        0, 0, 0                /* layer masks */
    };

    hwnd = CreateWindow("static", "Title", WS_OVERLAPPEDWINDOW,
                        10, 10, 200, 200, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "err: %d\n", GetLastError());
    if (hwnd)
    {
        HDC hdc;
        int iPixelFormat, res;
        HGLRC hglrc;
        DWORD error;
        ShowWindow(hwnd, SW_SHOW);

        hdc = GetDC(hwnd);

        iPixelFormat = ChoosePixelFormat(hdc, &pfd);
        if(iPixelFormat == 0)
        {
            /* This should never happen as ChoosePixelFormat always returns a closest match, but currently this fails in Wine if we don't have glX */
            win_skip("Unable to find pixel format.\n");
            goto cleanup;
        }

        /* We shouldn't be able to create a context from a hdc which doesn't have a pixel format set */
        hglrc = wglCreateContext(hdc);
        ok(hglrc == NULL, "wglCreateContext should fail when no pixel format has been set, but it passed\n");
        error = GetLastError();
        ok(error == ERROR_INVALID_PIXEL_FORMAT, "expected ERROR_INVALID_PIXEL_FORMAT for wglCreateContext without a pixelformat set, but received %#x\n", error);

        res = SetPixelFormat(hdc, iPixelFormat, &pfd);
        ok(res, "SetPixelformat failed: %x\n", GetLastError());

        test_bitmap_rendering();
        test_minimized();
        test_dc(hwnd, hdc);

        hglrc = wglCreateContext(hdc);
        res = wglMakeCurrent(hdc, hglrc);
        ok(res, "wglMakeCurrent failed!\n");
        if(res)
        {
            trace("OpenGL renderer: %s\n", glGetString(GL_RENDERER));
            trace("OpenGL driver version: %s\n", glGetString(GL_VERSION));
            trace("OpenGL vendor: %s\n", glGetString(GL_VENDOR));
        }
        else
        {
            skip("Skipping OpenGL tests without a current context\n");
            return;
        }

        /* Initialisation of WGL functions depends on an implicit WGL context. For this reason we can't load them before making
         * any WGL call :( On Wine this would work but not on real Windows because there can be different implementations (software, ICD, MCD).
         */
        init_functions();
        /* The lack of wglGetExtensionsStringARB in general means broken software rendering or the lack of decent OpenGL support, skip tests in such cases */
        if (!pwglGetExtensionsStringARB)
        {
            win_skip("wglGetExtensionsStringARB is not available\n");
            return;
        }

        test_deletecontext(hdc);
        test_makecurrent(hdc);
        test_setpixelformat(hdc);
        test_sharelists(hdc);
        test_colorbits(hdc);
        test_gdi_dbuf(hdc);
        test_acceleration(hdc);

        wgl_extensions = pwglGetExtensionsStringARB(hdc);
        if(wgl_extensions == NULL) skip("Skipping opengl32 tests because this OpenGL implementation doesn't support WGL extensions!\n");

        if(strstr(wgl_extensions, "WGL_ARB_create_context"))
            test_opengl3(hdc);

        if(strstr(wgl_extensions, "WGL_ARB_make_current_read"))
            test_make_current_read(hdc);
        else
            skip("WGL_ARB_make_current_read not supported, skipping test\n");

        if(strstr(wgl_extensions, "WGL_ARB_pbuffer"))
            test_pbuffers(hdc);
        else
            skip("WGL_ARB_pbuffer not supported, skipping pbuffer test\n");

cleanup:
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
    }
}
