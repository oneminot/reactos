/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for _vsnprintf
 */

#define WIN32_NO_STATUS
#include <stdio.h>
#include <wine/test.h>
#include <tchar.h>
#include <pseh/pseh2.h>
#include <ndk/mmfuncs.h>
#include <ndk/rtlfuncs.h>

#define StartSeh()              ExceptionStatus = STATUS_SUCCESS; _SEH2_TRY {
#define EndSeh(ExpectedStatus)  } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) { ExceptionStatus = _SEH2_GetExceptionCode(); } _SEH2_END; ok(ExceptionStatus == ExpectedStatus, "Exception %lx, expected %lx\n", ExceptionStatus, ExpectedStatus)

void call_varargs(char* buf, size_t buf_size, int expected_ret, LPCSTR formatString, ...)
{
    va_list args;
    int ret;
    /* Test the basic functionality */
    va_start(args, formatString);
    ret = _vsnprintf(buf, 255, formatString, args);
    ok(expected_ret == ret, "Test failed: expected %i, got %i.\n", expected_ret, ret);
}

START_TEST(_vsnprintf)
{
    char buffer[255];
    NTSTATUS ExceptionStatus;
    /* Test basic functionality */
    call_varargs(buffer, 255, 12, "%s world!", "hello");
    /* This is how WINE implements _vcsprintf, and they are obviously wrong */
    StartSeh()
        call_varargs(NULL, INT_MAX, -1, "%s it really work?", "does");
#if defined(TEST_CRTDLL) || defined(TEST_USER32)
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh(STATUS_SUCCESS);
#endif
    /* This one is no better */
    StartSeh()    
        call_varargs(NULL, 0, -1, "%s it really work?", "does");
#if defined(TEST_CRTDLL) || defined(TEST_USER32)
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh(STATUS_SUCCESS);
#endif
}
