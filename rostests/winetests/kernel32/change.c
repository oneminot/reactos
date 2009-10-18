/*
 * Tests for file change notification functions
 *
 * Copyright (c) 2004 Hans Leidekker
 * Copyright 2006 Mike McCormack for CodeWeavers
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

/* TODO: - security attribute changes
 *       - compound filter and multiple notifications
 *       - subtree notifications
 *       - non-documented flags FILE_NOTIFY_CHANGE_LAST_ACCESS and
 *         FILE_NOTIFY_CHANGE_CREATION
 */

#include <stdarg.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/test.h"
#include <windef.h>
#include <winbase.h>
#include <winternl.h>

static DWORD CALLBACK NotificationThread(LPVOID arg)
{
    HANDLE change = (HANDLE) arg;
    BOOL notified = FALSE;
    BOOL ret = FALSE;
    DWORD status;

    status = WaitForSingleObject(change, 100);

    if (status == WAIT_OBJECT_0 ) {
        notified = TRUE;
        ret = FindNextChangeNotification(change);
    }

    ret = FindCloseChangeNotification(change);
    ok( ret, "FindCloseChangeNotification error: %d\n",
       GetLastError());

    ExitThread((DWORD)notified);
}

static HANDLE StartNotificationThread(LPCSTR path, BOOL subtree, DWORD flags)
{
    HANDLE change, thread;
    DWORD threadId;

    change = FindFirstChangeNotificationA(path, subtree, flags);
    ok(change != INVALID_HANDLE_VALUE, "FindFirstChangeNotification error: %d\n", GetLastError());

    thread = CreateThread(NULL, 0, NotificationThread, (LPVOID)change,
                          0, &threadId);
    ok(thread != NULL, "CreateThread error: %d\n", GetLastError());

    return thread;
}

static DWORD FinishNotificationThread(HANDLE thread)
{
    DWORD status, exitcode;

    status = WaitForSingleObject(thread, 5000);
    ok(status == WAIT_OBJECT_0, "WaitForSingleObject status %d error %d\n", status, GetLastError());

    ok(GetExitCodeThread(thread, &exitcode), "Could not retrieve thread exit code\n");

    return exitcode;
}

static void test_FindFirstChangeNotification(void)
{
    HANDLE change, file, thread;
    DWORD attributes, count;
    BOOL ret;

    char workdir[MAX_PATH], dirname1[MAX_PATH], dirname2[MAX_PATH];
    char filename1[MAX_PATH], filename2[MAX_PATH];
    static const char prefix[] = "FCN";
    char buffer[2048];

    /* pathetic checks */

    change = FindFirstChangeNotificationA("not-a-file", FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
    ok(change == INVALID_HANDLE_VALUE, "Expected INVALID_HANDLE_VALUE, got %p\n", change);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND ||
       GetLastError() == ERROR_NO_MORE_FILES, /* win95 */
       "FindFirstChangeNotification error: %d\n", GetLastError());

    if (0) /* This documents win2k behavior. It crashes on win98. */
    { 
        change = FindFirstChangeNotificationA(NULL, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
        ok(change == NULL && GetLastError() == ERROR_PATH_NOT_FOUND,
        "FindFirstChangeNotification error: %d\n", GetLastError());
    }

    ret = FindNextChangeNotification(NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE, "FindNextChangeNotification error: %d\n",
       GetLastError());

    ret = FindCloseChangeNotification(NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE, "FindCloseChangeNotification error: %d\n",
       GetLastError());

    ret = GetTempPathA(MAX_PATH, workdir);
    ok(ret, "GetTempPathA error: %d\n", GetLastError());

    lstrcatA(workdir, "testFileChangeNotification");

    ret = CreateDirectoryA(workdir, NULL);
    ok(ret, "CreateDirectoryA error: %d\n", GetLastError());

    ret = GetTempFileNameA(workdir, prefix, 0, filename1);
    ok(ret, "GetTempFileNameA error: %d\n", GetLastError());

    file = CreateFileA(filename1, GENERIC_WRITE|GENERIC_READ, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA error: %d\n", GetLastError());
    ret = CloseHandle(file);
    ok( ret, "CloseHandle error: %d\n", GetLastError());

    /* Try to register notification for a file. win98 and win2k behave differently here */
    change = FindFirstChangeNotificationA(filename1, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
    ok(change == INVALID_HANDLE_VALUE && (GetLastError() == ERROR_DIRECTORY ||
                                          GetLastError() == ERROR_FILE_NOT_FOUND),
       "FindFirstChangeNotification error: %d\n", GetLastError());

    lstrcpyA(dirname1, filename1);
    lstrcatA(dirname1, "dir");

    lstrcpyA(dirname2, dirname1);
    lstrcatA(dirname2, "new");

    ret = CreateDirectoryA(dirname1, NULL);
    ok(ret, "CreateDirectoryA error: %d\n", GetLastError());

    /* What if we move the directory we registered notification for? */
    thread = StartNotificationThread(dirname1, FALSE, FILE_NOTIFY_CHANGE_DIR_NAME);
    ret = MoveFileA(dirname1, dirname2);
    ok(ret, "MoveFileA error: %d\n", GetLastError());
    /* win9x and win2k behave differently here, don't check result */
    FinishNotificationThread(thread);

    /* What if we remove the directory we registered notification for? */
    thread = StartNotificationThread(dirname2, FALSE, FILE_NOTIFY_CHANGE_DIR_NAME);
    ret = RemoveDirectoryA(dirname2);
    ok(ret, "RemoveDirectoryA error: %d\n", GetLastError());
    /* win9x and win2k behave differently here, don't check result */
    FinishNotificationThread(thread);

    /* functional checks */

    /* Create a directory */
    thread = StartNotificationThread(workdir, FALSE, FILE_NOTIFY_CHANGE_DIR_NAME);
    ret = CreateDirectoryA(dirname1, NULL);
    ok(ret, "CreateDirectoryA error: %d\n", GetLastError());
    ok(FinishNotificationThread(thread), "Missed notification\n");

    /* Rename a directory */
    thread = StartNotificationThread(workdir, FALSE, FILE_NOTIFY_CHANGE_DIR_NAME);
    ret = MoveFileA(dirname1, dirname2);
    ok(ret, "MoveFileA error: %d\n", GetLastError());
    ok(FinishNotificationThread(thread), "Missed notification\n");

    /* Delete a directory */
    thread = StartNotificationThread(workdir, FALSE, FILE_NOTIFY_CHANGE_DIR_NAME);
    ret = RemoveDirectoryA(dirname2);
    ok(ret, "RemoveDirectoryA error: %d\n", GetLastError());
    ok(FinishNotificationThread(thread), "Missed notification\n");

    lstrcpyA(filename2, filename1);
    lstrcatA(filename2, "new");

    /* Rename a file */
    thread = StartNotificationThread(workdir, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
    ret = MoveFileA(filename1, filename2);
    ok(ret, "MoveFileA error: %d\n", GetLastError());
    ok(FinishNotificationThread(thread), "Missed notification\n");

    /* Delete a file */
    thread = StartNotificationThread(workdir, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
    ret = DeleteFileA(filename2);
    ok(ret, "DeleteFileA error: %d\n", GetLastError());
    ok(FinishNotificationThread(thread), "Missed notification\n");

    /* Create a file */
    thread = StartNotificationThread(workdir, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
    file = CreateFileA(filename2, GENERIC_WRITE|GENERIC_READ, 0, NULL, CREATE_ALWAYS, 
                       FILE_ATTRIBUTE_NORMAL, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA error: %d\n", GetLastError());
    ret = CloseHandle(file);
    ok( ret, "CloseHandle error: %d\n", GetLastError());
    ok(FinishNotificationThread(thread), "Missed notification\n");

    attributes = GetFileAttributesA(filename2);
    ok(attributes != INVALID_FILE_ATTRIBUTES, "GetFileAttributesA error: %d\n", GetLastError());
    attributes &= FILE_ATTRIBUTE_READONLY;

    /* Change file attributes */
    thread = StartNotificationThread(workdir, FALSE, FILE_NOTIFY_CHANGE_ATTRIBUTES);
    ret = SetFileAttributesA(filename2, attributes);
    ok(ret, "SetFileAttributesA error: %d\n", GetLastError());
    ok(FinishNotificationThread(thread), "Missed notification\n");

    /* Change last write time by writing to a file */
    thread = StartNotificationThread(workdir, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);
    file = CreateFileA(filename2, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
                       FILE_ATTRIBUTE_NORMAL, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA error: %d\n", GetLastError());
    ret = WriteFile(file, buffer, sizeof(buffer), &count, NULL);
    ok(ret && count == sizeof(buffer), "WriteFile error: %d\n", GetLastError());
    ret = CloseHandle(file);
    ok( ret, "CloseHandle error: %d\n", GetLastError());
    ok(FinishNotificationThread(thread), "Missed notification\n");

    /* Change file size by truncating a file */
    thread = StartNotificationThread(workdir, FALSE, FILE_NOTIFY_CHANGE_SIZE);
    file = CreateFileA(filename2, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
                       FILE_ATTRIBUTE_NORMAL, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA error: %d\n", GetLastError());
    ret = WriteFile(file, buffer, sizeof(buffer) / 2, &count, NULL);
    ok(ret && count == sizeof(buffer) / 2, "WriteFileA error: %d\n", GetLastError());
    ret = CloseHandle(file);
    ok( ret, "CloseHandle error: %d\n", GetLastError());
    ok(FinishNotificationThread(thread), "Missed notification\n");

    /* clean up */
    
    ret = DeleteFileA(filename2);
    ok(ret, "DeleteFileA error: %d\n", GetLastError());

    ret = RemoveDirectoryA(workdir);
    ok(ret, "RemoveDirectoryA error: %d\n", GetLastError());
}

/* this test concentrates more on the wait behaviour of the handle */
static void test_ffcn(void)
{
    DWORD filter;
    HANDLE handle;
    LONG r;
    WCHAR path[MAX_PATH], subdir[MAX_PATH];
    static const WCHAR szBoo[] = { '\\','b','o','o',0 };
    static const WCHAR szHoo[] = { '\\','h','o','o',0 };

    SetLastError(0xdeadbeef);
    r = GetTempPathW( MAX_PATH, path );
    if (!r && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        skip("GetTempPathW is not implemented\n");
        return;
    }
    ok( r != 0, "temp path failed\n");
    if (!r)
        return;

    lstrcatW( path, szBoo );
    lstrcpyW( subdir, path );
    lstrcatW( subdir, szHoo );

    RemoveDirectoryW( subdir );
    RemoveDirectoryW( path );
    
    r = CreateDirectoryW(path, NULL);
    ok( r == TRUE, "failed to create directory\n");

    filter = FILE_NOTIFY_CHANGE_FILE_NAME;
    filter |= FILE_NOTIFY_CHANGE_DIR_NAME;

    handle = FindFirstChangeNotificationW( path, 1, filter);
    ok( handle != INVALID_HANDLE_VALUE, "invalid handle\n");

    r = WaitForSingleObject( handle, 0 );
    ok( r == STATUS_TIMEOUT, "should time out\n");

    r = CreateDirectoryW( subdir, NULL );
    ok( r == TRUE, "failed to create subdir\n");

    r = WaitForSingleObject( handle, 0 );
    ok( r == WAIT_OBJECT_0, "should be ready\n");

    r = WaitForSingleObject( handle, 0 );
    ok( r == WAIT_OBJECT_0, "should be ready\n");

    r = FindNextChangeNotification(handle);
    ok( r == TRUE, "find next failed\n");

    r = WaitForSingleObject( handle, 0 );
    ok( r == STATUS_TIMEOUT, "should time out\n");

    r = RemoveDirectoryW( subdir );
    ok( r == TRUE, "failed to remove subdir\n");

    r = WaitForSingleObject( handle, 0 );
    ok( r == WAIT_OBJECT_0, "should be ready\n");

    r = WaitForSingleObject( handle, 0 );
    ok( r == WAIT_OBJECT_0, "should be ready\n");

    r = FindNextChangeNotification(handle);
    ok( r == TRUE, "find next failed\n");

    r = FindNextChangeNotification(handle);
    ok( r == TRUE, "find next failed\n");

    r = FindCloseChangeNotification(handle);
    ok( r == TRUE, "should succeed\n");

    r = RemoveDirectoryW( path );
    ok( r == TRUE, "failed to remove dir\n");
}

/* this test concentrates on the wait behavior when multiple threads are
 * waiting on a change notification handle. */
static void test_ffcnMultipleThreads(void)
{
    LONG r;
    DWORD filter, threadId, status, exitcode;
    HANDLE handles[2];
    char path[MAX_PATH];

    r = GetTempPathA(MAX_PATH, path);
    ok(r, "GetTempPathA error: %d\n", GetLastError());

    lstrcatA(path, "ffcnTestMultipleThreads");

    RemoveDirectoryA(path);

    r = CreateDirectoryA(path, NULL);
    ok(r, "CreateDirectoryA error: %d\n", GetLastError());

    filter = FILE_NOTIFY_CHANGE_FILE_NAME;
    filter |= FILE_NOTIFY_CHANGE_DIR_NAME;

    handles[0] = FindFirstChangeNotificationA(path, FALSE, filter);
    ok(handles[0] != INVALID_HANDLE_VALUE, "FindFirstChangeNotification error: %d\n", GetLastError());

    /* Test behavior if a waiting thread holds the last reference to a change
     * directory object with an empty wine user APC queue for this thread (bug #7286) */

    /* Create our notification thread */
    handles[1] = CreateThread(NULL, 0, NotificationThread, (LPVOID)handles[0],
                              0, &threadId);
    ok(handles[1] != NULL, "CreateThread error: %d\n", GetLastError());

    status = WaitForMultipleObjects(2, handles, FALSE, 5000);
    ok(status == WAIT_OBJECT_0 || status == WAIT_OBJECT_0+1, "WaitForMultipleObjects status %d error %d\n", status, GetLastError());
    ok(GetExitCodeThread(handles[1], &exitcode), "Could not retrieve thread exit code\n");

    /* Clean up */
    r = RemoveDirectoryA( path );
    ok( r == TRUE, "failed to remove dir\n");
}

typedef BOOL (WINAPI *fnReadDirectoryChangesW)(HANDLE,LPVOID,DWORD,BOOL,DWORD,
                         LPDWORD,LPOVERLAPPED,LPOVERLAPPED_COMPLETION_ROUTINE);
fnReadDirectoryChangesW pReadDirectoryChangesW;

static void test_readdirectorychanges(void)
{
    HANDLE hdir;
    char buffer[0x1000];
    DWORD fflags, filter = 0, r, dwCount;
    OVERLAPPED ov;
    WCHAR path[MAX_PATH], subdir[MAX_PATH], subsubdir[MAX_PATH];
    static const WCHAR szBoo[] = { '\\','b','o','o',0 };
    static const WCHAR szHoo[] = { '\\','h','o','o',0 };
    static const WCHAR szGa[] = { '\\','h','o','o','\\','g','a',0 };
    PFILE_NOTIFY_INFORMATION pfni;

    if (!pReadDirectoryChangesW)
    {
        skip("ReadDirectoryChangesW is not available\n");
        return;
    }

    SetLastError(0xdeadbeef);
    r = GetTempPathW( MAX_PATH, path );
    if (!r && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        skip("GetTempPathW is not implemented\n");
        return;
    }
    ok( r != 0, "temp path failed\n");
    if (!r)
        return;

    lstrcatW( path, szBoo );
    lstrcpyW( subdir, path );
    lstrcatW( subdir, szHoo );

    lstrcpyW( subsubdir, path );
    lstrcatW( subsubdir, szGa );

    RemoveDirectoryW( subsubdir );
    RemoveDirectoryW( subdir );
    RemoveDirectoryW( path );
    
    r = CreateDirectoryW(path, NULL);
    ok( r == TRUE, "failed to create directory\n");

    SetLastError(0xd0b00b00);
    r = pReadDirectoryChangesW(NULL,NULL,0,FALSE,0,NULL,NULL,NULL);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,"last error wrong\n");
    ok(r==FALSE, "should return false\n");

    fflags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;
    hdir = CreateFileW(path, GENERIC_READ|SYNCHRONIZE|FILE_LIST_DIRECTORY, 
                        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, 
                        OPEN_EXISTING, fflags, NULL);
    ok( hdir != INVALID_HANDLE_VALUE, "failed to open directory\n");

    ov.hEvent = CreateEvent( NULL, 1, 0, NULL );

    SetLastError(0xd0b00b00);
    r = pReadDirectoryChangesW(hdir,NULL,0,FALSE,0,NULL,NULL,NULL);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,"last error wrong\n");
    ok(r==FALSE, "should return false\n");

    SetLastError(0xd0b00b00);
    r = pReadDirectoryChangesW(hdir,NULL,0,FALSE,0,NULL,&ov,NULL);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,"last error wrong\n");
    ok(r==FALSE, "should return false\n");

    filter = FILE_NOTIFY_CHANGE_FILE_NAME;
    filter |= FILE_NOTIFY_CHANGE_DIR_NAME;
    filter |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
    filter |= FILE_NOTIFY_CHANGE_SIZE;
    filter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
    filter |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
    filter |= FILE_NOTIFY_CHANGE_CREATION;
    filter |= FILE_NOTIFY_CHANGE_SECURITY;

    SetLastError(0xd0b00b00);
    ov.Internal = 0;
    ov.InternalHigh = 0;
    memset( buffer, 0, sizeof buffer );

    r = pReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,-1,NULL,&ov,NULL);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,"last error wrong\n");
    ok(r==FALSE, "should return false\n");

    r = pReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,0,NULL,&ov,NULL);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,"last error wrong\n");
    ok(r==FALSE, "should return false\n");

    r = pReadDirectoryChangesW(hdir,buffer,sizeof buffer,TRUE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    r = WaitForSingleObject( ov.hEvent, 10 );
    ok( r == STATUS_TIMEOUT, "should timeout\n" );

    r = CreateDirectoryW( subdir, NULL );
    ok( r == TRUE, "failed to create directory\n");

    r = WaitForSingleObject( ov.hEvent, 1000 );
    ok( r == WAIT_OBJECT_0, "event should be ready\n" );

    ok( ov.Internal == STATUS_SUCCESS, "ov.Internal wrong\n");
    ok( ov.InternalHigh == 0x12, "ov.InternalHigh wrong\n");

    pfni = (PFILE_NOTIFY_INFORMATION) buffer;
    ok( pfni->NextEntryOffset == 0, "offset wrong\n" );
    ok( pfni->Action == FILE_ACTION_ADDED, "action wrong\n" );
    ok( pfni->FileNameLength == 6, "len wrong\n" );
    ok( !memcmp(pfni->FileName,&szHoo[1],6), "name wrong\n" );

    ResetEvent(ov.hEvent);
    SetLastError(0xd0b00b00);
    r = pReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,0,NULL,NULL,NULL);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,"last error wrong\n");
    ok(r==FALSE, "should return false\n");

    r = pReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,0,NULL,&ov,NULL);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,"last error wrong\n");
    ok(r==FALSE, "should return false\n");

    filter = FILE_NOTIFY_CHANGE_SIZE;

    SetEvent(ov.hEvent);
    ov.Internal = 1;
    ov.InternalHigh = 1;
    S(U(ov)).Offset = 0;
    S(U(ov)).OffsetHigh = 0;
    memset( buffer, 0, sizeof buffer );
    r = pReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    ok( ov.Internal == STATUS_PENDING, "ov.Internal wrong\n");
    ok( ov.InternalHigh == 1, "ov.InternalHigh wrong\n");

    r = WaitForSingleObject( ov.hEvent, 0 );
    ok( r == STATUS_TIMEOUT, "should timeout\n" );

    r = RemoveDirectoryW( subdir );
    ok( r == TRUE, "failed to remove directory\n");

    r = WaitForSingleObject( ov.hEvent, 1000 );
    ok( r == WAIT_OBJECT_0, "should be ready\n" );

    ok( ov.Internal == STATUS_SUCCESS, "ov.Internal wrong\n");
    ok( ov.InternalHigh == 0x12, "ov.InternalHigh wrong\n");

    if (ov.Internal == STATUS_SUCCESS)
    {
        r = GetOverlappedResult( hdir, &ov, &dwCount, TRUE );
        ok( r == TRUE, "getoverlappedresult failed\n");
        ok( dwCount == 0x12, "count wrong\n");
    }

    pfni = (PFILE_NOTIFY_INFORMATION) buffer;
    ok( pfni->NextEntryOffset == 0, "offset wrong\n" );
    ok( pfni->Action == FILE_ACTION_REMOVED, "action wrong\n" );
    ok( pfni->FileNameLength == 6, "len wrong\n" );
    ok( !memcmp(pfni->FileName,&szHoo[1],6), "name wrong\n" );

    /* what happens if the buffer is too small? */
    r = pReadDirectoryChangesW(hdir,buffer,0x10,FALSE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    r = CreateDirectoryW( subdir, NULL );
    ok( r == TRUE, "failed to create directory\n");

    r = WaitForSingleObject( ov.hEvent, 1000 );
    ok( r == WAIT_OBJECT_0, "should be ready\n" );

    ok( ov.Internal == STATUS_NOTIFY_ENUM_DIR, "ov.Internal wrong\n");
    ok( ov.InternalHigh == 0, "ov.InternalHigh wrong\n");

    /* test the recursive watch */
    r = pReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    r = CreateDirectoryW( subsubdir, NULL );
    ok( r == TRUE, "failed to create directory\n");

    r = WaitForSingleObject( ov.hEvent, 1000 );
    ok( r == WAIT_OBJECT_0, "should be ready\n" );

    ok( ov.Internal == STATUS_SUCCESS, "ov.Internal wrong\n");
    ok( ov.InternalHigh == 0x18, "ov.InternalHigh wrong\n");

    pfni = (PFILE_NOTIFY_INFORMATION) buffer;
    ok( pfni->NextEntryOffset == 0, "offset wrong\n" );
    ok( pfni->Action == FILE_ACTION_ADDED, "action wrong\n" );
    ok( pfni->FileNameLength == 0x0c, "len wrong\n" );
    ok( !memcmp(pfni->FileName,&szGa[1],6), "name wrong\n" );

    r = RemoveDirectoryW( subsubdir );
    ok( r == TRUE, "failed to remove directory\n");

    ov.Internal = 1;
    ov.InternalHigh = 1;
    r = pReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    r = RemoveDirectoryW( subdir );
    ok( r == TRUE, "failed to remove directory\n");

    r = WaitForSingleObject( ov.hEvent, 1000 );
    ok( r == WAIT_OBJECT_0, "should be ready\n" );

    pfni = (PFILE_NOTIFY_INFORMATION) buffer;
    ok( pfni->NextEntryOffset == 0, "offset wrong\n" );
    ok( pfni->Action == FILE_ACTION_REMOVED, "action wrong\n" );
    ok( pfni->FileNameLength == 0x0c, "len wrong\n" );
    ok( !memcmp(pfni->FileName,&szGa[1],6), "name wrong\n" );

    ok( ov.Internal == STATUS_SUCCESS, "ov.Internal wrong\n");
    ok( ov.InternalHigh == 0x18, "ov.InternalHigh wrong\n");

    CloseHandle(hdir);

    r = RemoveDirectoryW( path );
    ok( r == TRUE, "failed to remove directory\n");
}

/* show the behaviour when a null buffer is passed */
static void test_readdirectorychanges_null(void)
{
    NTSTATUS r;
    HANDLE hdir;
    char buffer[0x1000];
    DWORD fflags, filter = 0;
    OVERLAPPED ov;
    WCHAR path[MAX_PATH], subdir[MAX_PATH];
    static const WCHAR szBoo[] = { '\\','b','o','o',0 };
    static const WCHAR szHoo[] = { '\\','h','o','o',0 };
    PFILE_NOTIFY_INFORMATION pfni;

    if (!pReadDirectoryChangesW)
    {
        skip("ReadDirectoryChangesW is not available\n");
        return;
    }
    SetLastError(0xdeadbeef);
    r = GetTempPathW( MAX_PATH, path );
    if (!r && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        skip("GetTempPathW is not implemented\n");
        return;
    }
    ok( r != 0, "temp path failed\n");
    if (!r)
        return;

    lstrcatW( path, szBoo );
    lstrcpyW( subdir, path );
    lstrcatW( subdir, szHoo );

    RemoveDirectoryW( subdir );
    RemoveDirectoryW( path );
    
    r = CreateDirectoryW(path, NULL);
    ok( r == TRUE, "failed to create directory\n");

    fflags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;
    hdir = CreateFileW(path, GENERIC_READ|SYNCHRONIZE|FILE_LIST_DIRECTORY, 
                        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, 
                        OPEN_EXISTING, fflags, NULL);
    ok( hdir != INVALID_HANDLE_VALUE, "failed to open directory\n");

    ov.hEvent = CreateEvent( NULL, 1, 0, NULL );

    filter = FILE_NOTIFY_CHANGE_FILE_NAME;
    filter |= FILE_NOTIFY_CHANGE_DIR_NAME;

    SetLastError(0xd0b00b00);
    ov.Internal = 0;
    ov.InternalHigh = 0;
    memset( buffer, 0, sizeof buffer );

    r = pReadDirectoryChangesW(hdir,NULL,0,FALSE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    r = WaitForSingleObject( ov.hEvent, 0 );
    ok( r == STATUS_TIMEOUT, "should timeout\n" );

    r = CreateDirectoryW( subdir, NULL );
    ok( r == TRUE, "failed to create directory\n");

    r = WaitForSingleObject( ov.hEvent, 0 );
    ok( r == WAIT_OBJECT_0, "event should be ready\n" );

    ok( ov.Internal == STATUS_NOTIFY_ENUM_DIR, "ov.Internal wrong\n");
    ok( ov.InternalHigh == 0, "ov.InternalHigh wrong\n");

    ov.Internal = 0;
    ov.InternalHigh = 0;
    S(U(ov)).Offset = 0;
    S(U(ov)).OffsetHigh = 0;
    memset( buffer, 0, sizeof buffer );

    r = pReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    r = WaitForSingleObject( ov.hEvent, 0 );
    ok( r == STATUS_TIMEOUT, "should timeout\n" );

    r = RemoveDirectoryW( subdir );
    ok( r == TRUE, "failed to remove directory\n");

    r = WaitForSingleObject( ov.hEvent, 1000 );
    ok( r == WAIT_OBJECT_0, "should be ready\n" );

    ok( ov.Internal == STATUS_NOTIFY_ENUM_DIR, "ov.Internal wrong\n");
    ok( ov.InternalHigh == 0, "ov.InternalHigh wrong\n");

    pfni = (PFILE_NOTIFY_INFORMATION) buffer;
    ok( pfni->NextEntryOffset == 0, "offset wrong\n" );

    CloseHandle(hdir);

    r = RemoveDirectoryW( path );
    ok( r == TRUE, "failed to remove directory\n");
}

static void test_readdirectorychanges_filedir(void)
{
    NTSTATUS r;
    HANDLE hdir, hfile;
    char buffer[0x1000];
    DWORD fflags, filter = 0;
    OVERLAPPED ov;
    WCHAR path[MAX_PATH], subdir[MAX_PATH], file[MAX_PATH];
    static const WCHAR szBoo[] = { '\\','b','o','o',0 };
    static const WCHAR szHoo[] = { '\\','h','o','o',0 };
    static const WCHAR szFoo[] = { '\\','f','o','o',0 };
    PFILE_NOTIFY_INFORMATION pfni;

    SetLastError(0xdeadbeef);
    r = GetTempPathW( MAX_PATH, path );
    if (!r && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        skip("GetTempPathW is not implemented\n");
        return;
    }
    ok( r != 0, "temp path failed\n");
    if (!r)
        return;

    lstrcatW( path, szBoo );
    lstrcpyW( subdir, path );
    lstrcatW( subdir, szHoo );

    lstrcpyW( file, path );
    lstrcatW( file, szFoo );

    DeleteFileW( file );
    RemoveDirectoryW( subdir );
    RemoveDirectoryW( path );
    
    r = CreateDirectoryW(path, NULL);
    ok( r == TRUE, "failed to create directory\n");

    fflags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;
    hdir = CreateFileW(path, GENERIC_READ|SYNCHRONIZE|FILE_LIST_DIRECTORY, 
                        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, 
                        OPEN_EXISTING, fflags, NULL);
    ok( hdir != INVALID_HANDLE_VALUE, "failed to open directory\n");

    ov.hEvent = CreateEvent( NULL, 0, 0, NULL );

    filter = FILE_NOTIFY_CHANGE_FILE_NAME;

    r = pReadDirectoryChangesW(hdir,buffer,sizeof buffer,TRUE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    r = WaitForSingleObject( ov.hEvent, 10 );
    ok( r == WAIT_TIMEOUT, "should timeout\n" );

    r = CreateDirectoryW( subdir, NULL );
    ok( r == TRUE, "failed to create directory\n");

    hfile = CreateFileW( file, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL );
    ok( hfile != INVALID_HANDLE_VALUE, "failed to create file\n");
    ok( CloseHandle(hfile), "failed toc lose file\n");

    r = WaitForSingleObject( ov.hEvent, 1000 );
    ok( r == WAIT_OBJECT_0, "event should be ready\n" );

    ok( ov.Internal == STATUS_SUCCESS, "ov.Internal wrong\n");
    ok( ov.InternalHigh == 0x12, "ov.InternalHigh wrong\n");

    pfni = (PFILE_NOTIFY_INFORMATION) buffer;
    ok( pfni->NextEntryOffset == 0, "offset wrong\n" );
    ok( pfni->Action == FILE_ACTION_ADDED, "action wrong\n" );
    ok( pfni->FileNameLength == 6, "len wrong\n" );
    ok( !memcmp(pfni->FileName,&szFoo[1],6), "name wrong\n" );

    r = DeleteFileW( file );
    ok( r == TRUE, "failed to delete file\n");

    r = RemoveDirectoryW( subdir );
    ok( r == TRUE, "failed to remove directory\n");

    CloseHandle(hdir);

    r = RemoveDirectoryW( path );
    ok( r == TRUE, "failed to remove directory\n");
}

static void test_ffcn_directory_overlap(void)
{
    HANDLE parent_watch, child_watch, parent_thread, child_thread;
    char workdir[MAX_PATH], parentdir[MAX_PATH], childdir[MAX_PATH];
    char tempfile[MAX_PATH];
    DWORD threadId;
    BOOL ret;

    /* Setup directory hierarchy */
    ret = GetTempPathA(MAX_PATH, workdir);
    ok((ret > 0) && (ret <= MAX_PATH),
       "GetTempPathA error: %d\n", GetLastError());

    ret = GetTempFileNameA(workdir, "fcn", 0, tempfile);
    ok(ret, "GetTempFileNameA error: %d\n", GetLastError());
    ret = DeleteFileA(tempfile);
    ok(ret, "DeleteFileA error: %d\n", GetLastError());

    lstrcpyA(parentdir, tempfile);
    ret = CreateDirectoryA(parentdir, NULL);
    ok(ret, "CreateDirectoryA error: %d\n", GetLastError());

    lstrcpyA(childdir, parentdir);
    lstrcatA(childdir, "\\c");
    ret = CreateDirectoryA(childdir, NULL);
    ok(ret, "CreateDirectoryA error: %d\n", GetLastError());


    /* When recursively watching overlapping directories, changes in child
     * should trigger notifications for both child and parent */
    parent_thread = StartNotificationThread(parentdir, TRUE,
                                            FILE_NOTIFY_CHANGE_FILE_NAME);
    child_thread = StartNotificationThread(childdir, TRUE,
                                            FILE_NOTIFY_CHANGE_FILE_NAME);

    /* Create a file in child */
    ret = GetTempFileNameA(childdir, "fcn", 0, tempfile);
    ok(ret, "GetTempFileNameA error: %d\n", GetLastError());

    /* Both watches should trigger */
    ret = FinishNotificationThread(parent_thread);
    ok(ret, "Missed parent notification\n");
    ret = FinishNotificationThread(child_thread);
    ok(ret, "Missed child notification\n");

    ret = DeleteFileA(tempfile);
    ok(ret, "DeleteFileA error: %d\n", GetLastError());


    /* Removing a recursive parent watch should not affect child watches. Doing
     * so used to crash wineserver. */
    parent_watch = FindFirstChangeNotificationA(parentdir, TRUE,
                                                FILE_NOTIFY_CHANGE_FILE_NAME);
    ok(parent_watch != INVALID_HANDLE_VALUE,
       "FindFirstChangeNotification error: %d\n", GetLastError());
    child_watch = FindFirstChangeNotificationA(childdir, TRUE,
                                               FILE_NOTIFY_CHANGE_FILE_NAME);
    ok(child_watch != INVALID_HANDLE_VALUE,
       "FindFirstChangeNotification error: %d\n", GetLastError());

    ret = FindCloseChangeNotification(parent_watch);
    ok(ret, "FindCloseChangeNotification error: %d\n", GetLastError());

    child_thread = CreateThread(NULL, 0, NotificationThread,
                                (LPVOID)child_watch, 0, &threadId);
    ok(child_thread != NULL, "CreateThread error: %d\n", GetLastError());

    /* Create a file in child */
    ret = GetTempFileNameA(childdir, "fcn", 0, tempfile);
    ok(ret, "GetTempFileNameA error: %d\n", GetLastError());

    /* Child watch should trigger */
    ret = FinishNotificationThread(child_thread);
    ok(ret, "Missed child notification\n");

    /* clean up */
    ret = DeleteFileA(tempfile);
    ok(ret, "DeleteFileA error: %d\n", GetLastError());

    ret = RemoveDirectoryA(childdir);
    ok(ret, "RemoveDirectoryA error: %d\n", GetLastError());

    ret = RemoveDirectoryA(parentdir);
    ok(ret, "RemoveDirectoryA error: %d\n", GetLastError());
}

START_TEST(change)
{
    HMODULE hkernel32 = GetModuleHandle("kernel32");
    pReadDirectoryChangesW = (fnReadDirectoryChangesW)
        GetProcAddress(hkernel32, "ReadDirectoryChangesW");

    test_ffcnMultipleThreads();
    /* The above function runs a test that must occur before FindCloseChangeNotification is run in the
       current thread to preserve the emptiness of the wine user APC queue. To ensure this it should be
       placed first. */
    test_FindFirstChangeNotification();
    test_ffcn();
    test_readdirectorychanges();
    test_readdirectorychanges_null();
    test_readdirectorychanges_filedir();
    test_ffcn_directory_overlap();
}
