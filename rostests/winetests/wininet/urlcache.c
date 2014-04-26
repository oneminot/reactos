/*
 * URL Cache Tests
 *
 * Copyright 2008 Robert Shearman for CodeWeavers
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

#include <stdarg.h>
//#include <stdio.h>
//#include <stdlib.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <wininet.h>
#include <winineti.h>

#include <wine/test.h>

static const char test_url[] = "http://urlcachetest.winehq.org/index.html";
static const WCHAR test_urlW[] = {'h','t','t','p',':','/','/','u','r','l','c','a','c','h','e','t','e','s','t','.',
    'w','i','n','e','h','q','.','o','r','g','/','i','n','d','e','x','.','h','t','m','l',0};
static const char test_url1[] = "Visited: user@http://urlcachetest.winehq.org/index.html";
static const char test_hash_collisions1[] = "Visited: http://winehq.org/doc0.html";
static const char test_hash_collisions2[] = "Visited: http://winehq.org/doc75651909.html";

static BOOL (WINAPI *pDeleteUrlCacheEntryA)(LPCSTR);
static BOOL (WINAPI *pUnlockUrlCacheEntryFileA)(LPCSTR,DWORD);

static char filenameA[MAX_PATH + 1];
static char filenameA1[MAX_PATH + 1];
static BOOL old_ie = FALSE;
static BOOL ie10_cache = FALSE;

static void check_cache_entry_infoA(const char *returnedfrom, INTERNET_CACHE_ENTRY_INFOA *lpCacheEntryInfo)
{
    ok(lpCacheEntryInfo->dwStructSize == sizeof(*lpCacheEntryInfo), "%s: dwStructSize was %d\n", returnedfrom, lpCacheEntryInfo->dwStructSize);
    ok(!strcmp(lpCacheEntryInfo->lpszSourceUrlName, test_url), "%s: lpszSourceUrlName should be %s instead of %s\n", returnedfrom, test_url, lpCacheEntryInfo->lpszSourceUrlName);
    ok(!strcmp(lpCacheEntryInfo->lpszLocalFileName, filenameA), "%s: lpszLocalFileName should be %s instead of %s\n", returnedfrom, filenameA, lpCacheEntryInfo->lpszLocalFileName);
    ok(!strcmp(lpCacheEntryInfo->lpszFileExtension, "html"), "%s: lpszFileExtension should be html instead of %s\n", returnedfrom, lpCacheEntryInfo->lpszFileExtension);
}

static void test_find_url_cache_entriesA(void)
{
    BOOL ret;
    HANDLE hEnumHandle;
    BOOL found = FALSE;
    DWORD cbCacheEntryInfo;
    DWORD cbCacheEntryInfoSaved;
    INTERNET_CACHE_ENTRY_INFOA *lpCacheEntryInfo;

    cbCacheEntryInfo = 0;
    SetLastError(0xdeadbeef);
    hEnumHandle = FindFirstUrlCacheEntryA(NULL, NULL, &cbCacheEntryInfo);
    ok(!hEnumHandle, "FindFirstUrlCacheEntry should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "FindFirstUrlCacheEntry should have set last error to ERROR_INSUFFICIENT_BUFFER instead of %d\n", GetLastError());
    lpCacheEntryInfo = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo * sizeof(char));
    cbCacheEntryInfoSaved = cbCacheEntryInfo;
    hEnumHandle = FindFirstUrlCacheEntryA(NULL, lpCacheEntryInfo, &cbCacheEntryInfo);
    ok(hEnumHandle != NULL, "FindFirstUrlCacheEntry failed with error %d\n", GetLastError());
    while (TRUE)
    {
        if (!strcmp(lpCacheEntryInfo->lpszSourceUrlName, test_url))
        {
            found = TRUE;
            ret = TRUE;
            break;
        }
        SetLastError(0xdeadbeef);
        cbCacheEntryInfo = cbCacheEntryInfoSaved;
        ret = FindNextUrlCacheEntryA(hEnumHandle, lpCacheEntryInfo, &cbCacheEntryInfo);
        if (!ret)
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                lpCacheEntryInfo = HeapReAlloc(GetProcessHeap(), 0, lpCacheEntryInfo, cbCacheEntryInfo);
                cbCacheEntryInfoSaved = cbCacheEntryInfo;
                ret = FindNextUrlCacheEntryA(hEnumHandle, lpCacheEntryInfo, &cbCacheEntryInfo);
            }
        }
        if (!ret)
            break;
    }
    ok(ret, "FindNextUrlCacheEntry failed with error %d\n", GetLastError());
    ok(found, "Committed url cache entry not found during enumeration\n");

    ret = FindCloseUrlCache(hEnumHandle);
    ok(ret, "FindCloseUrlCache failed with error %d\n", GetLastError());
}

static void test_GetUrlCacheEntryInfoExA(void)
{
    BOOL ret;
    DWORD cbCacheEntryInfo, cbRedirectUrl;
    INTERNET_CACHE_ENTRY_INFOA *lpCacheEntryInfo;

    SetLastError(0xdeadbeef);
    ret = GetUrlCacheEntryInfoExA(NULL, NULL, NULL, NULL, NULL, NULL, 0);
    ok(!ret, "GetUrlCacheEntryInfoEx with NULL URL and NULL args should have failed\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "GetUrlCacheEntryInfoEx with NULL URL and NULL args should have set last error to ERROR_INVALID_PARAMETER instead of %d\n", GetLastError());

    cbCacheEntryInfo = sizeof(INTERNET_CACHE_ENTRY_INFOA);
    SetLastError(0xdeadbeef);
    ret = GetUrlCacheEntryInfoExA("", NULL, &cbCacheEntryInfo, NULL, NULL, NULL, 0);
    ok(!ret, "GetUrlCacheEntryInfoEx with zero-length buffer should fail\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND,
       "GetUrlCacheEntryInfoEx should have set last error to ERROR_FILE_NOT_FOUND instead of %d\n", GetLastError());

    ret = GetUrlCacheEntryInfoExA(test_url, NULL, NULL, NULL, NULL, NULL, 0);
    ok(ret, "GetUrlCacheEntryInfoEx with NULL args failed with error %d\n", GetLastError());

    cbCacheEntryInfo = 0;
    SetLastError(0xdeadbeef);
    ret = GetUrlCacheEntryInfoExA(test_url, NULL, &cbCacheEntryInfo, NULL, NULL, NULL, 0);
    ok(!ret, "GetUrlCacheEntryInfoEx with zero-length buffer should fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "GetUrlCacheEntryInfoEx should have set last error to ERROR_INSUFFICIENT_BUFFER instead of %d\n", GetLastError());

    lpCacheEntryInfo = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo);

    SetLastError(0xdeadbeef);
    ret = GetUrlCacheEntryInfoExA(test_url, NULL, NULL, NULL, NULL, NULL, 0x200 /*GET_INSTALLED_ENTRY*/);
    ok(ret == ie10_cache, "GetUrlCacheEntryInfoEx returned %x\n", ret);
    if (!ret) ok(GetLastError() == ERROR_FILE_NOT_FOUND,
            "GetUrlCacheEntryInfoEx should have set last error to ERROR_FILE_NOT_FOUND instead of %d\n", GetLastError());

    /* Unicode version of function seems to ignore 0x200 flag */
    ret = GetUrlCacheEntryInfoExW(test_urlW, NULL, NULL, NULL, NULL, NULL, 0x200 /*GET_INSTALLED_ENTRY*/);
    ok(ret || broken(old_ie && !ret), "GetUrlCacheEntryInfoExW failed with error %d\n", GetLastError());

    ret = GetUrlCacheEntryInfoExA(test_url, lpCacheEntryInfo, &cbCacheEntryInfo, NULL, NULL, NULL, 0);
    ok(ret, "GetUrlCacheEntryInfoEx failed with error %d\n", GetLastError());

    if (ret) check_cache_entry_infoA("GetUrlCacheEntryInfoEx", lpCacheEntryInfo);

    lpCacheEntryInfo->CacheEntryType |= 0x10000000; /* INSTALLED_CACHE_ENTRY */
    ret = SetUrlCacheEntryInfoA(test_url, lpCacheEntryInfo, CACHE_ENTRY_ATTRIBUTE_FC);
    ok(ret, "SetUrlCacheEntryInfoA failed with error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetUrlCacheEntryInfoExA(test_url, NULL, NULL, NULL, NULL, NULL, 0x200 /*GET_INSTALLED_ENTRY*/);
    ok(ret, "GetUrlCacheEntryInfoEx failed with error %d\n", GetLastError());

    cbCacheEntryInfo = 100000;
    SetLastError(0xdeadbeef);
    ret = GetUrlCacheEntryInfoExA(test_url, NULL, &cbCacheEntryInfo, NULL, NULL, NULL, 0);
    ok(!ret, "GetUrlCacheEntryInfoEx with zero-length buffer should fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetUrlCacheEntryInfoEx should have set last error to ERROR_INSUFFICIENT_BUFFER instead of %d\n", GetLastError());

    HeapFree(GetProcessHeap(), 0, lpCacheEntryInfo);

    /* Querying the redirect URL fails with ERROR_INVALID_PARAMETER */
    SetLastError(0xdeadbeef);
    ret = GetUrlCacheEntryInfoExA(test_url, NULL, NULL, NULL, &cbRedirectUrl, NULL, 0);
    ok(!ret, "GetUrlCacheEntryInfoEx should have failed\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = GetUrlCacheEntryInfoExA(test_url, NULL, &cbCacheEntryInfo, NULL, &cbRedirectUrl, NULL, 0);
    ok(!ret, "GetUrlCacheEntryInfoEx should have failed\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
}

static void test_RetrieveUrlCacheEntryA(void)
{
    BOOL ret;
    DWORD cbCacheEntryInfo;

    cbCacheEntryInfo = 0;
    SetLastError(0xdeadbeef);
    ret = RetrieveUrlCacheEntryFileA(NULL, NULL, &cbCacheEntryInfo, 0);
    ok(!ret, "RetrieveUrlCacheEntryFile should have failed\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "RetrieveUrlCacheEntryFile should have set last error to ERROR_INVALID_PARAMETER instead of %d\n", GetLastError());

    if (0)
    {
        /* Crashes on Win9x, NT4 and W2K */
        SetLastError(0xdeadbeef);
        ret = RetrieveUrlCacheEntryFileA(test_url, NULL, NULL, 0);
        ok(!ret, "RetrieveUrlCacheEntryFile should have failed\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER, "RetrieveUrlCacheEntryFile should have set last error to ERROR_INVALID_PARAMETER instead of %d\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    cbCacheEntryInfo = 100000;
    ret = RetrieveUrlCacheEntryFileA(NULL, NULL, &cbCacheEntryInfo, 0);
    ok(!ret, "RetrieveUrlCacheEntryFile should have failed\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "RetrieveUrlCacheEntryFile should have set last error to ERROR_INVALID_PARAMETER instead of %d\n", GetLastError());
}

static void test_IsUrlCacheEntryExpiredA(void)
{
    static const char uncached_url[] =
        "What's the airspeed velocity of an unladen swallow?";
    BOOL ret;
    FILETIME ft;
    DWORD size;
    INTERNET_CACHE_ENTRY_INFOA *info;
    ULARGE_INTEGER exp_time;

    /* The function returns TRUE when the output time is NULL or the tested URL
     * is NULL.
     */
    ret = IsUrlCacheEntryExpiredA(NULL, 0, NULL);
    ok(!ret == ie10_cache, "IsUrlCacheEntryExpiredA returned %x\n", ret);
    ft.dwLowDateTime = 0xdeadbeef;
    ft.dwHighDateTime = 0xbaadf00d;
    ret = IsUrlCacheEntryExpiredA(NULL, 0, &ft);
    ok(!ret == ie10_cache, "IsUrlCacheEntryExpiredA returned %x\n", ret);
    ok(ft.dwLowDateTime == 0xdeadbeef && ft.dwHighDateTime == 0xbaadf00d,
       "expected time to be unchanged, got (%u,%u)\n",
       ft.dwLowDateTime, ft.dwHighDateTime);
    ret = IsUrlCacheEntryExpiredA(test_url, 0, NULL);
    ok(!ret == ie10_cache, "IsUrlCacheEntryExpiredA returned %x\n", ret);

    /* The return value should indicate whether the URL is expired,
     * and the filetime indicates the last modified time, but a cache entry
     * with a zero expire time is "not expired".
     */
    ft.dwLowDateTime = 0xdeadbeef;
    ft.dwHighDateTime = 0xbaadf00d;
    ret = IsUrlCacheEntryExpiredA(test_url, 0, &ft);
    ok(!ret, "expected FALSE\n");
    ok(!ft.dwLowDateTime && !ft.dwHighDateTime,
       "expected time (0,0), got (%u,%u)\n",
       ft.dwLowDateTime, ft.dwHighDateTime);

    /* Same behavior with bogus flags. */
    ft.dwLowDateTime = 0xdeadbeef;
    ft.dwHighDateTime = 0xbaadf00d;
    ret = IsUrlCacheEntryExpiredA(test_url, 0xffffffff, &ft);
    ok(!ret, "expected FALSE\n");
    ok(!ft.dwLowDateTime && !ft.dwHighDateTime,
       "expected time (0,0), got (%u,%u)\n",
       ft.dwLowDateTime, ft.dwHighDateTime);

    /* Set the expire time to a point in the past.. */
    ret = GetUrlCacheEntryInfoA(test_url, NULL, &size);
    ok(!ret, "GetUrlCacheEntryInfo should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    info = HeapAlloc(GetProcessHeap(), 0, size);
    ret = GetUrlCacheEntryInfoA(test_url, info, &size);
    ok(ret, "GetUrlCacheEntryInfo failed: %d\n", GetLastError());
    GetSystemTimeAsFileTime(&info->ExpireTime);
    exp_time.u.LowPart = info->ExpireTime.dwLowDateTime;
    exp_time.u.HighPart = info->ExpireTime.dwHighDateTime;
    exp_time.QuadPart -= 10 * 60 * (ULONGLONG)10000000;
    info->ExpireTime.dwLowDateTime = exp_time.u.LowPart;
    info->ExpireTime.dwHighDateTime = exp_time.u.HighPart;
    ret = SetUrlCacheEntryInfoA(test_url, info, CACHE_ENTRY_EXPTIME_FC);
    ok(ret, "SetUrlCacheEntryInfo failed: %d\n", GetLastError());
    ft.dwLowDateTime = 0xdeadbeef;
    ft.dwHighDateTime = 0xbaadf00d;
    /* and the entry should be expired. */
    ret = IsUrlCacheEntryExpiredA(test_url, 0, &ft);
    ok(ret, "expected TRUE\n");
    /* The modified time returned is 0. */
    ok(!ft.dwLowDateTime && !ft.dwHighDateTime,
       "expected time (0,0), got (%u,%u)\n",
       ft.dwLowDateTime, ft.dwHighDateTime);
    /* Set the expire time to a point in the future.. */
    exp_time.QuadPart += 20 * 60 * (ULONGLONG)10000000;
    info->ExpireTime.dwLowDateTime = exp_time.u.LowPart;
    info->ExpireTime.dwHighDateTime = exp_time.u.HighPart;
    ret = SetUrlCacheEntryInfoA(test_url, info, CACHE_ENTRY_EXPTIME_FC);
    ok(ret, "SetUrlCacheEntryInfo failed: %d\n", GetLastError());
    ft.dwLowDateTime = 0xdeadbeef;
    ft.dwHighDateTime = 0xbaadf00d;
    /* and the entry should no longer be expired. */
    ret = IsUrlCacheEntryExpiredA(test_url, 0, &ft);
    ok(!ret, "expected FALSE\n");
    /* The modified time returned is still 0. */
    ok(!ft.dwLowDateTime && !ft.dwHighDateTime,
       "expected time (0,0), got (%u,%u)\n",
       ft.dwLowDateTime, ft.dwHighDateTime);
    /* Set the modified time... */
    GetSystemTimeAsFileTime(&info->LastModifiedTime);
    ret = SetUrlCacheEntryInfoA(test_url, info, CACHE_ENTRY_MODTIME_FC);
    ok(ret, "SetUrlCacheEntryInfo failed: %d\n", GetLastError());
    /* and the entry should still be unexpired.. */
    ret = IsUrlCacheEntryExpiredA(test_url, 0, &ft);
    ok(!ret, "expected FALSE\n");
    /* but the modified time returned is the last modified time just set. */
    ok(ft.dwLowDateTime == info->LastModifiedTime.dwLowDateTime &&
       ft.dwHighDateTime == info->LastModifiedTime.dwHighDateTime,
       "expected time (%u,%u), got (%u,%u)\n",
       info->LastModifiedTime.dwLowDateTime,
       info->LastModifiedTime.dwHighDateTime,
       ft.dwLowDateTime, ft.dwHighDateTime);
    HeapFree(GetProcessHeap(), 0, info);

    /* An uncached URL is implicitly expired, but with unknown time. */
    ft.dwLowDateTime = 0xdeadbeef;
    ft.dwHighDateTime = 0xbaadf00d;
    ret = IsUrlCacheEntryExpiredA(uncached_url, 0, &ft);
    ok(!ret == ie10_cache, "IsUrlCacheEntryExpiredA returned %x\n", ret);
    ok(!ft.dwLowDateTime && !ft.dwHighDateTime,
       "expected time (0,0), got (%u,%u)\n",
       ft.dwLowDateTime, ft.dwHighDateTime);
}

static void _check_file_exists(LONG l, LPCSTR filename)
{
    HANDLE file;

    file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok_(__FILE__,l)(file != INVALID_HANDLE_VALUE,
                    "expected file to exist, CreateFile failed with error %d\n",
                    GetLastError());
    CloseHandle(file);
}

#define check_file_exists(f) _check_file_exists(__LINE__, f)

static void _check_file_not_exists(LONG l, LPCSTR filename)
{
    HANDLE file;

    file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok_(__FILE__,l)(file == INVALID_HANDLE_VALUE,
                    "expected file not to exist\n");
    if (file != INVALID_HANDLE_VALUE)
        CloseHandle(file);
}

#define check_file_not_exists(f) _check_file_not_exists(__LINE__, f)

static void create_and_write_file(LPCSTR filename, void *data, DWORD len)
{
    HANDLE file;
    DWORD written;
    BOOL ret;

    file = CreateFileA(filename, GENERIC_WRITE,
                       FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA failed with error %d\n", GetLastError());

    ret = WriteFile(file, data, len, &written, NULL);
    ok(ret, "WriteFile failed with error %d\n", GetLastError());

    CloseHandle(file);
}

static void test_urlcacheA(void)
{
    static char ok_header[] = "HTTP/1.0 200 OK\r\n\r\n";
    BOOL ret;
    HANDLE hFile;
    BYTE zero_byte = 0;
    INTERNET_CACHE_ENTRY_INFOA *lpCacheEntryInfo;
    INTERNET_CACHE_ENTRY_INFOA *lpCacheEntryInfo2;
    DWORD cbCacheEntryInfo;
    static const FILETIME filetime_zero;
    FILETIME now;

    ret = CreateUrlCacheEntryA(test_url, 0, "html", filenameA, 0);
    ok(ret, "CreateUrlCacheEntry failed with error %d\n", GetLastError());

    ret = CreateUrlCacheEntryA(test_url, 0, "html", filenameA1, 0);
    ok(ret, "CreateUrlCacheEntry failed with error %d\n", GetLastError());
    check_file_exists(filenameA1);
    DeleteFileA(filenameA1);

    ok(lstrcmpiA(filenameA, filenameA1), "expected a different file name\n");

    create_and_write_file(filenameA, &zero_byte, sizeof(zero_byte));

    ret = CommitUrlCacheEntryA(test_url1, NULL, filetime_zero, filetime_zero, NORMAL_CACHE_ENTRY, NULL, 0, "html", NULL);
    ok(ret, "CommitUrlCacheEntry failed with error %d\n", GetLastError());
    cbCacheEntryInfo = 0;
    ret = GetUrlCacheEntryInfoA(test_url1, NULL, &cbCacheEntryInfo);
    ok(!ret, "GetUrlCacheEntryInfo should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "GetUrlCacheEntryInfo should have set last error to ERROR_INSUFFICIENT_BUFFER instead of %d\n", GetLastError());
    lpCacheEntryInfo = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo);
    ret = GetUrlCacheEntryInfoA(test_url1, lpCacheEntryInfo, &cbCacheEntryInfo);
    ok(ret, "GetUrlCacheEntryInfo failed with error %d\n", GetLastError());
    ok(!memcmp(&lpCacheEntryInfo->ExpireTime, &filetime_zero, sizeof(FILETIME)),
       "expected zero ExpireTime\n");
    ok(!memcmp(&lpCacheEntryInfo->LastModifiedTime, &filetime_zero, sizeof(FILETIME)),
       "expected zero LastModifiedTime\n");
    ok(lpCacheEntryInfo->CacheEntryType == (NORMAL_CACHE_ENTRY|URLHISTORY_CACHE_ENTRY) ||
       broken(lpCacheEntryInfo->CacheEntryType == NORMAL_CACHE_ENTRY /* NT4/W2k */),
       "expected type NORMAL_CACHE_ENTRY|URLHISTORY_CACHE_ENTRY, got %08x\n",
       lpCacheEntryInfo->CacheEntryType);
    ok(!U(*lpCacheEntryInfo).dwExemptDelta, "expected dwExemptDelta 0, got %d\n",
       U(*lpCacheEntryInfo).dwExemptDelta);

    /* Make sure there is a notable change in timestamps */
    Sleep(1000);

    /* A subsequent commit with a different time/type doesn't change most of the entry */
    GetSystemTimeAsFileTime(&now);
    ret = CommitUrlCacheEntryA(test_url1, NULL, now, now, NORMAL_CACHE_ENTRY,
            (LPBYTE)ok_header, strlen(ok_header), NULL, NULL);
    ok(ret, "CommitUrlCacheEntry failed with error %d\n", GetLastError());
    cbCacheEntryInfo = 0;
    ret = GetUrlCacheEntryInfoA(test_url1, NULL, &cbCacheEntryInfo);
    ok(!ret, "GetUrlCacheEntryInfo should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    lpCacheEntryInfo2 = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo);
    ret = GetUrlCacheEntryInfoA(test_url1, lpCacheEntryInfo2, &cbCacheEntryInfo);
    ok(ret, "GetUrlCacheEntryInfo failed with error %d\n", GetLastError());
    /* but it does change the time.. */
    ok(memcmp(&lpCacheEntryInfo2->ExpireTime, &filetime_zero, sizeof(FILETIME)),
       "expected positive ExpireTime\n");
    ok(memcmp(&lpCacheEntryInfo2->LastModifiedTime, &filetime_zero, sizeof(FILETIME)),
       "expected positive LastModifiedTime\n");
    ok(lpCacheEntryInfo2->CacheEntryType == (NORMAL_CACHE_ENTRY|URLHISTORY_CACHE_ENTRY) ||
       broken(lpCacheEntryInfo2->CacheEntryType == NORMAL_CACHE_ENTRY /* NT4/W2k */),
       "expected type NORMAL_CACHE_ENTRY|URLHISTORY_CACHE_ENTRY, got %08x\n",
       lpCacheEntryInfo2->CacheEntryType);
    /* and set the headers. */
    ok(lpCacheEntryInfo2->dwHeaderInfoSize == 19,
        "expected headers size 19, got %d\n",
        lpCacheEntryInfo2->dwHeaderInfoSize);
    /* Hit rate gets incremented by 1 */
    ok((lpCacheEntryInfo->dwHitRate + 1) == lpCacheEntryInfo2->dwHitRate,
        "HitRate not incremented by one on commit\n");
    /* Last access time should be updated */
    ok(!(lpCacheEntryInfo->LastAccessTime.dwHighDateTime == lpCacheEntryInfo2->LastAccessTime.dwHighDateTime &&
        lpCacheEntryInfo->LastAccessTime.dwLowDateTime == lpCacheEntryInfo2->LastAccessTime.dwLowDateTime),
        "Last accessed time was not updated by commit\n");
    /* File extension should be unset */
    ok(lpCacheEntryInfo2->lpszFileExtension == NULL,
        "Fileextension isn't unset: %s\n",
        lpCacheEntryInfo2->lpszFileExtension);
    HeapFree(GetProcessHeap(), 0, lpCacheEntryInfo);
    HeapFree(GetProcessHeap(), 0, lpCacheEntryInfo2);

    ret = CommitUrlCacheEntryA(test_url, filenameA, filetime_zero, filetime_zero, NORMAL_CACHE_ENTRY, NULL, 0, "html", NULL);
    ok(ret, "CommitUrlCacheEntry failed with error %d\n", GetLastError());

    cbCacheEntryInfo = 0;
    SetLastError(0xdeadbeef);
    ret = RetrieveUrlCacheEntryFileA(test_url, NULL, &cbCacheEntryInfo, 0);
    ok(!ret, "RetrieveUrlCacheEntryFile should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "RetrieveUrlCacheEntryFile should have set last error to ERROR_INSUFFICIENT_BUFFER instead of %d\n", GetLastError());

    lpCacheEntryInfo = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo);
    ret = RetrieveUrlCacheEntryFileA(test_url, lpCacheEntryInfo, &cbCacheEntryInfo, 0);
    ok(ret, "RetrieveUrlCacheEntryFile failed with error %d\n", GetLastError());

    if (ret) check_cache_entry_infoA("RetrieveUrlCacheEntryFile", lpCacheEntryInfo);

    HeapFree(GetProcessHeap(), 0, lpCacheEntryInfo);

    cbCacheEntryInfo = 0;
    SetLastError(0xdeadbeef);
    ret = RetrieveUrlCacheEntryFileA(test_url1, NULL, &cbCacheEntryInfo, 0);
    ok(!ret, "RetrieveUrlCacheEntryFile should have failed\n");
    ok(GetLastError() == ERROR_INVALID_DATA || GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "RetrieveUrlCacheEntryFile should have set last error to ERROR_INVALID_DATA instead of %d\n", GetLastError());

    if (pUnlockUrlCacheEntryFileA)
    {
        ret = pUnlockUrlCacheEntryFileA(test_url, 0);
        ok(ret, "UnlockUrlCacheEntryFileA failed with error %d\n", GetLastError());
    }

    /* test Find*UrlCacheEntry functions */
    test_find_url_cache_entriesA();

    test_GetUrlCacheEntryInfoExA();
    test_RetrieveUrlCacheEntryA();
    test_IsUrlCacheEntryExpiredA();

    if (pDeleteUrlCacheEntryA)
    {
        ret = pDeleteUrlCacheEntryA(test_url);
        ok(ret, "DeleteUrlCacheEntryA failed with error %d\n", GetLastError());
        ret = pDeleteUrlCacheEntryA(test_url1);
        ok(ret, "DeleteUrlCacheEntryA failed with error %d\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    ret = DeleteFileA(filenameA);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND, "local file should no longer exist\n");

    /* Creating two entries with the same URL */
    ret = CreateUrlCacheEntryA(test_url, 0, "html", filenameA, 0);
    ok(ret, "CreateUrlCacheEntry failed with error %d\n", GetLastError());

    ret = CreateUrlCacheEntryA(test_url, 0, "html", filenameA1, 0);
    ok(ret, "CreateUrlCacheEntry failed with error %d\n", GetLastError());

    ok(lstrcmpiA(filenameA, filenameA1), "expected a different file name\n");

    create_and_write_file(filenameA, &zero_byte, sizeof(zero_byte));
    create_and_write_file(filenameA1, &zero_byte, sizeof(zero_byte));
    check_file_exists(filenameA);
    check_file_exists(filenameA1);

    ret = CommitUrlCacheEntryA(test_url, filenameA, filetime_zero,
            filetime_zero, NORMAL_CACHE_ENTRY, (LPBYTE)ok_header,
            strlen(ok_header), "html", NULL);
    ok(ret, "CommitUrlCacheEntry failed with error %d\n", GetLastError());
    check_file_exists(filenameA);
    check_file_exists(filenameA1);
    ret = CommitUrlCacheEntryA(test_url, filenameA1, filetime_zero,
            filetime_zero, COOKIE_CACHE_ENTRY, NULL, 0, "html", NULL);
    ok(ret, "CommitUrlCacheEntry failed with error %d\n", GetLastError());
    /* By committing the same URL a second time, the prior entry is
     * overwritten...
     */
    cbCacheEntryInfo = 0;
    SetLastError(0xdeadbeef);
    ret = GetUrlCacheEntryInfoA(test_url, NULL, &cbCacheEntryInfo);
    ok(!ret, "GetUrlCacheEntryInfo should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    lpCacheEntryInfo = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo);
    ret = GetUrlCacheEntryInfoA(test_url, lpCacheEntryInfo, &cbCacheEntryInfo);
    ok(ret, "GetUrlCacheEntryInfo failed with error %d\n", GetLastError());
    /* with the previous entry type retained.. */
    ok(lpCacheEntryInfo->CacheEntryType & NORMAL_CACHE_ENTRY,
       "expected cache entry type NORMAL_CACHE_ENTRY, got %d (0x%08x)\n",
       lpCacheEntryInfo->CacheEntryType, lpCacheEntryInfo->CacheEntryType);
    /* and the headers overwritten.. */
    ok(!lpCacheEntryInfo->dwHeaderInfoSize, "expected headers size 0, got %d\n",
       lpCacheEntryInfo->dwHeaderInfoSize);
    HeapFree(GetProcessHeap(), 0, lpCacheEntryInfo);
    /* and the previous filename shouldn't exist. */
    check_file_not_exists(filenameA);
    check_file_exists(filenameA1);

    if (pDeleteUrlCacheEntryA)
    {
        ret = pDeleteUrlCacheEntryA(test_url);
        ok(ret, "DeleteUrlCacheEntryA failed with error %d\n", GetLastError());
        check_file_not_exists(filenameA);
        check_file_not_exists(filenameA1);
        /* Just in case, clean up files */
        DeleteFileA(filenameA1);
        DeleteFileA(filenameA);
    }

    /* Check whether a retrieved cache entry can be deleted before it's
     * unlocked:
     */
    ret = CreateUrlCacheEntryA(test_url, 0, "html", filenameA, 0);
    ok(ret, "CreateUrlCacheEntry failed with error %d\n", GetLastError());
    ret = CommitUrlCacheEntryA(test_url, filenameA, filetime_zero, filetime_zero,
            NORMAL_CACHE_ENTRY, NULL, 0, "html", NULL);
    ok(ret, "CommitUrlCacheEntry failed with error %d\n", GetLastError());

    cbCacheEntryInfo = 0;
    SetLastError(0xdeadbeef);
    ret = RetrieveUrlCacheEntryFileA(test_url, NULL, &cbCacheEntryInfo, 0);
    ok(!ret, "RetrieveUrlCacheEntryFile should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    lpCacheEntryInfo = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo);
    ret = RetrieveUrlCacheEntryFileA(test_url, lpCacheEntryInfo,
            &cbCacheEntryInfo, 0);
    ok(ret, "RetrieveUrlCacheEntryFile failed with error %d\n", GetLastError());

    HeapFree(GetProcessHeap(), 0, lpCacheEntryInfo);

    if (pDeleteUrlCacheEntryA)
    {
        ret = pDeleteUrlCacheEntryA(test_url);
        ok(!ret, "Expected failure\n");
        ok(GetLastError() == ERROR_SHARING_VIOLATION,
           "Expected ERROR_SHARING_VIOLATION, got %d\n", GetLastError());
        check_file_exists(filenameA);
    }

    lpCacheEntryInfo = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo);
    memset(lpCacheEntryInfo, 0, cbCacheEntryInfo);
    ret = GetUrlCacheEntryInfoA(test_url, lpCacheEntryInfo, &cbCacheEntryInfo);
    ok(ret, "GetUrlCacheEntryInfo failed with error %d\n", GetLastError());
    ok(lpCacheEntryInfo->CacheEntryType & 0x400000,
        "CacheEntryType hasn't PENDING_DELETE_CACHE_ENTRY set, (flags %08x)\n",
        lpCacheEntryInfo->CacheEntryType);
    HeapFree(GetProcessHeap(), 0, lpCacheEntryInfo);

    if (pUnlockUrlCacheEntryFileA)
    {
        check_file_exists(filenameA);
        ret = pUnlockUrlCacheEntryFileA(test_url, 0);
        ok(ret, "UnlockUrlCacheEntryFileA failed: %d\n", GetLastError());
        /* By unlocking the already-deleted cache entry, the file associated
         * with it is deleted..
         */
        check_file_not_exists(filenameA);
        /* (just in case, delete file) */
        DeleteFileA(filenameA);
    }
    if (pDeleteUrlCacheEntryA)
    {
        /* and a subsequent deletion should fail. */
        ret = pDeleteUrlCacheEntryA(test_url);
        ok(!ret, "Expected failure\n");
        ok(GetLastError() == ERROR_FILE_NOT_FOUND,
           "expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    }

    /* Test whether preventing a file from being deleted causes
     * DeleteUrlCacheEntryA to fail.
     */
    ret = CreateUrlCacheEntryA(test_url, 0, "html", filenameA, 0);
    ok(ret, "CreateUrlCacheEntry failed with error %d\n", GetLastError());

    create_and_write_file(filenameA, &zero_byte, sizeof(zero_byte));
    check_file_exists(filenameA);

    ret = CommitUrlCacheEntryA(test_url, filenameA, filetime_zero,
            filetime_zero, NORMAL_CACHE_ENTRY, (LPBYTE)ok_header,
            strlen(ok_header), "html", NULL);
    ok(ret, "CommitUrlCacheEntry failed with error %d\n", GetLastError());
    check_file_exists(filenameA);
    hFile = CreateFileA(filenameA, GENERIC_READ, 0, NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA failed: %d\n",
       GetLastError());
    if (pDeleteUrlCacheEntryA)
    {
        /* DeleteUrlCacheEntryA should succeed.. */
        ret = pDeleteUrlCacheEntryA(test_url);
        ok(ret, "DeleteUrlCacheEntryA failed with error %d\n", GetLastError());
    }
    CloseHandle(hFile);
    if (pDeleteUrlCacheEntryA)
    {
        /* and a subsequent deletion should fail.. */
        ret = pDeleteUrlCacheEntryA(test_url);
        ok(!ret, "Expected failure\n");
        ok(GetLastError() == ERROR_FILE_NOT_FOUND,
           "expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    }
    /* and the file should be untouched. */
    check_file_exists(filenameA);
    DeleteFileA(filenameA);

    /* Try creating a sticky entry.  Unlike non-sticky entries, the filename
     * must have been set already.
     */
    SetLastError(0xdeadbeef);
    ret = CommitUrlCacheEntryA(test_url, NULL, filetime_zero, filetime_zero,
            STICKY_CACHE_ENTRY, (LPBYTE)ok_header, strlen(ok_header), "html",
            NULL);
    ok(ret == ie10_cache, "CommitUrlCacheEntryA returned %x\n", ret);
    if (!ret) ok(GetLastError() == ERROR_INVALID_PARAMETER,
            "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = CommitUrlCacheEntryA(test_url, NULL, filetime_zero, filetime_zero,
            NORMAL_CACHE_ENTRY|STICKY_CACHE_ENTRY,
            (LPBYTE)ok_header, strlen(ok_header), "html", NULL);
    ok(ret == ie10_cache, "CommitUrlCacheEntryA returned %x\n", ret);
    if (!ret) ok(GetLastError() == ERROR_INVALID_PARAMETER,
            "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    ret = CreateUrlCacheEntryA(test_url, 0, "html", filenameA, 0);
    ok(ret, "CreateUrlCacheEntry failed with error %d\n", GetLastError());
    create_and_write_file(filenameA, &zero_byte, sizeof(zero_byte));
    ret = CommitUrlCacheEntryA(test_url, filenameA, filetime_zero, filetime_zero,
            NORMAL_CACHE_ENTRY|STICKY_CACHE_ENTRY,
            (LPBYTE)ok_header, strlen(ok_header), "html", NULL);
    ok(ret, "CommitUrlCacheEntry failed with error %d\n", GetLastError());
    cbCacheEntryInfo = 0;
    SetLastError(0xdeadbeef);
    ret = GetUrlCacheEntryInfoA(test_url, NULL, &cbCacheEntryInfo);
    ok(!ret, "GetUrlCacheEntryInfo should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    lpCacheEntryInfo = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo);
    ret = GetUrlCacheEntryInfoA(test_url, lpCacheEntryInfo, &cbCacheEntryInfo);
    ok(ret, "GetUrlCacheEntryInfo failed with error %d\n", GetLastError());
    ok(lpCacheEntryInfo->CacheEntryType & (NORMAL_CACHE_ENTRY|STICKY_CACHE_ENTRY),
       "expected cache entry type NORMAL_CACHE_ENTRY | STICKY_CACHE_ENTRY, got %d (0x%08x)\n",
       lpCacheEntryInfo->CacheEntryType, lpCacheEntryInfo->CacheEntryType);
    ok(U(*lpCacheEntryInfo).dwExemptDelta == 86400,
       "expected dwExemptDelta 86400, got %d\n",
       U(*lpCacheEntryInfo).dwExemptDelta);
    HeapFree(GetProcessHeap(), 0, lpCacheEntryInfo);
    if (pDeleteUrlCacheEntryA)
    {
        ret = pDeleteUrlCacheEntryA(test_url);
        ok(ret, "DeleteUrlCacheEntryA failed with error %d\n", GetLastError());
        /* When explicitly deleting the cache entry, the file is also deleted */
        check_file_not_exists(filenameA);
    }
    /* Test once again, setting the exempt delta via SetUrlCacheEntryInfo */
    ret = CreateUrlCacheEntryA(test_url, 0, "html", filenameA, 0);
    ok(ret, "CreateUrlCacheEntry failed with error %d\n", GetLastError());
    create_and_write_file(filenameA, &zero_byte, sizeof(zero_byte));
    ret = CommitUrlCacheEntryA(test_url, filenameA, filetime_zero, filetime_zero,
            NORMAL_CACHE_ENTRY|STICKY_CACHE_ENTRY,
            (LPBYTE)ok_header, strlen(ok_header), "html", NULL);
    ok(ret, "CommitUrlCacheEntry failed with error %d\n", GetLastError());
    cbCacheEntryInfo = 0;
    SetLastError(0xdeadbeef);
    ret = GetUrlCacheEntryInfoA(test_url, NULL, &cbCacheEntryInfo);
    ok(!ret, "GetUrlCacheEntryInfo should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    lpCacheEntryInfo = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo);
    ret = GetUrlCacheEntryInfoA(test_url, lpCacheEntryInfo, &cbCacheEntryInfo);
    ok(ret, "GetUrlCacheEntryInfo failed with error %d\n", GetLastError());
    ok(lpCacheEntryInfo->CacheEntryType & (NORMAL_CACHE_ENTRY|STICKY_CACHE_ENTRY),
       "expected cache entry type NORMAL_CACHE_ENTRY | STICKY_CACHE_ENTRY, got %d (0x%08x)\n",
       lpCacheEntryInfo->CacheEntryType, lpCacheEntryInfo->CacheEntryType);
    ok(U(*lpCacheEntryInfo).dwExemptDelta == 86400,
       "expected dwExemptDelta 86400, got %d\n",
       U(*lpCacheEntryInfo).dwExemptDelta);
    U(*lpCacheEntryInfo).dwExemptDelta = 0;
    ret = SetUrlCacheEntryInfoA(test_url, lpCacheEntryInfo,
            CACHE_ENTRY_EXEMPT_DELTA_FC);
    ok(ret, "SetUrlCacheEntryInfo failed: %d\n", GetLastError());
    ret = GetUrlCacheEntryInfoA(test_url, lpCacheEntryInfo, &cbCacheEntryInfo);
    ok(ret, "GetUrlCacheEntryInfo failed with error %d\n", GetLastError());
    ok(!U(*lpCacheEntryInfo).dwExemptDelta, "expected dwExemptDelta 0, got %d\n",
       U(*lpCacheEntryInfo).dwExemptDelta);
    /* See whether a sticky cache entry has the flag cleared once the exempt
     * delta is meaningless.
     */
    ok(lpCacheEntryInfo->CacheEntryType & (NORMAL_CACHE_ENTRY|STICKY_CACHE_ENTRY),
       "expected cache entry type NORMAL_CACHE_ENTRY | STICKY_CACHE_ENTRY, got %d (0x%08x)\n",
       lpCacheEntryInfo->CacheEntryType, lpCacheEntryInfo->CacheEntryType);

    /* Recommit of Url entry keeps dwExemptDelta */
    U(*lpCacheEntryInfo).dwExemptDelta = 8600;
    ret = SetUrlCacheEntryInfoA(test_url, lpCacheEntryInfo,
            CACHE_ENTRY_EXEMPT_DELTA_FC);
    ok(ret, "SetUrlCacheEntryInfo failed: %d\n", GetLastError());

    ret = CreateUrlCacheEntryA(test_url, 0, "html", filenameA1, 0);
    ok(ret, "CreateUrlCacheEntry failed with error %d\n", GetLastError());
    create_and_write_file(filenameA1, &zero_byte, sizeof(zero_byte));

    ret = CommitUrlCacheEntryA(test_url, filenameA1, filetime_zero, filetime_zero,
            NORMAL_CACHE_ENTRY|STICKY_CACHE_ENTRY,
            (LPBYTE)ok_header, strlen(ok_header), "html", NULL);
    ok(ret, "CommitUrlCacheEntry failed with error %d\n", GetLastError());

    ret = GetUrlCacheEntryInfoA(test_url, lpCacheEntryInfo, &cbCacheEntryInfo);
    ok(ret, "GetUrlCacheEntryInfo failed with error %d\n", GetLastError());
    ok(U(*lpCacheEntryInfo).dwExemptDelta == 8600 || (ie10_cache && U(*lpCacheEntryInfo).dwExemptDelta == 86400),
       "expected dwExemptDelta 8600, got %d\n", U(*lpCacheEntryInfo).dwExemptDelta);

    HeapFree(GetProcessHeap(), 0, lpCacheEntryInfo);

    if (pDeleteUrlCacheEntryA)
    {
        ret = pDeleteUrlCacheEntryA(test_url);
        ok(ret, "DeleteUrlCacheEntryA failed with error %d\n", GetLastError());
        check_file_not_exists(filenameA);
    }

    /* Test if files with identical hash keys are handled correctly */
    ret = CommitUrlCacheEntryA(test_hash_collisions1, NULL, filetime_zero, filetime_zero, NORMAL_CACHE_ENTRY, NULL, 0, "html", NULL);
    ok(ret, "CommitUrlCacheEntry failed with error %d\n", GetLastError());
    ret = CommitUrlCacheEntryA(test_hash_collisions2, NULL, filetime_zero, filetime_zero, NORMAL_CACHE_ENTRY, NULL, 0, "html", NULL);
    ok(ret, "CommitUrlCacheEntry failed with error %d\n", GetLastError());

    cbCacheEntryInfo = 0;
    ret = GetUrlCacheEntryInfoA(test_hash_collisions1, NULL, &cbCacheEntryInfo);
    ok(!ret, "GetUrlCacheEntryInfo should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
            "expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    lpCacheEntryInfo = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo);
    ret = GetUrlCacheEntryInfoA(test_hash_collisions1, lpCacheEntryInfo, &cbCacheEntryInfo);
    ok(ret, "GetUrlCacheEntryInfo failed with error %d\n", GetLastError());
    ok(!strcmp(lpCacheEntryInfo->lpszSourceUrlName, test_hash_collisions1),
            "got incorrect entry: %s\n", lpCacheEntryInfo->lpszSourceUrlName);
    HeapFree(GetProcessHeap(), 0, lpCacheEntryInfo);

    cbCacheEntryInfo = 0;
    ret = GetUrlCacheEntryInfoA(test_hash_collisions2, NULL, &cbCacheEntryInfo);
    ok(!ret, "GetUrlCacheEntryInfo should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
            "expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    lpCacheEntryInfo = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo);
    ret = GetUrlCacheEntryInfoA(test_hash_collisions2, lpCacheEntryInfo, &cbCacheEntryInfo);
    ok(ret, "GetUrlCacheEntryInfo failed with error %d\n", GetLastError());
    ok(!strcmp(lpCacheEntryInfo->lpszSourceUrlName, test_hash_collisions2),
            "got incorrect entry: %s\n", lpCacheEntryInfo->lpszSourceUrlName);
    HeapFree(GetProcessHeap(), 0, lpCacheEntryInfo);

    if (pDeleteUrlCacheEntryA) {
        ret = pDeleteUrlCacheEntryA(test_hash_collisions1);
        ok(ret, "DeleteUrlCacheEntry failed: %d\n", GetLastError());
        ret = pDeleteUrlCacheEntryA(test_hash_collisions2);
        ok(ret, "DeleteUrlCacheEntry failed: %d\n", GetLastError());
    }
}

static void test_urlcacheW(void)
{
    static struct test_data
    {
        DWORD err;
        WCHAR url[128];
        char encoded_url[128];
        WCHAR extension[32];
        WCHAR header_info[128];
    }urls[] = {
        {
            0, {'h','t','t','p',':','/','/','T','.','p','l','/','t',0},
            "http://T.pl/t", {0}, {0}
        },
        {
            0, {'w','w','w','.','T','.','p','l','/','t',0},
            "www.T.pl/t", {0}, {0}
        },
        {
            0, {'h','t','t','p',':','/','/','w','w','w','.','t','e','s','t',0x15b,0x107,
                '.','o','r','g','/','t','e','s','t','.','h','t','m','l',0},
            "http://www.xn--test-ota71c.org/test.html", {'t','x','t',0}, {0}
        },
        {
            0, {'w','w','w','.','T','e','s','t',0x15b,0x107,'.','o','r','g',
                '/','t','e','s','t','.','h','t','m','l',0},
            "www.Test\xc5\x9b\xc4\x87.org/test.html", {'a',0x106,'a',0}, {'b',0x106,'b',0}
        },
        {
            0, {'H','t','t','p','s',':','/','/',0x15b,0x15b,0x107,'/','t',0x107,'/',
                't','e','s','t','?','a','=','%','2','0',0x106,0},
            "Https://xn--4da1oa/t\xc4\x87/test?a=%20\xc4\x86", {'a',0x15b,'a',0}, {'b',0x15b,'b',0}
        },
        {
            12005, {'h','t','t','p','s',':','/','/','/','/',0x107,'.','o','r','g','/','t','e','s','t',0},
            "", {0}, {0}
        },
        {
            0, {'C','o','o','k','i','e',':',' ','u','s','e','r','@','h','t','t','p',
                ':','/','/','t',0x15b,0x107,'.','o','r','g','/',0},
            "Cookie: user@http://t\xc5\x9b\xc4\x87.org/", {0}, {0}
        }
    };
    static const FILETIME filetime_zero;

    WCHAR bufW[MAX_PATH];
    DWORD i;
    BOOL ret;

    if(old_ie) {
        win_skip("urlcache unicode functions\n");
        return;
    }

    if(ie10_cache) {
        if(!MultiByteToWideChar(CP_ACP, 0, urls[6].encoded_url, -1,
                    urls[6].url, sizeof(urls[6].url)/sizeof(WCHAR)))
            urls[6].url[0] = 0;

        trace("converted url in test 6: %s\n", wine_dbgstr_w(urls[6].url));
    }

    for(i=0; i<sizeof(urls)/sizeof(*urls); i++) {
        INTERNET_CACHE_ENTRY_INFOA *entry_infoA;
        INTERNET_CACHE_ENTRY_INFOW *entry_infoW;
        DWORD size;

        if(!urls[i].url[0]) {
            win_skip("No UTF16 version of url (%d)\n", i);
            continue;
        }

        SetLastError(0xdeadbeef);
        ret = CreateUrlCacheEntryW(urls[i].url, 0, NULL, bufW, 0);
        if(urls[i].err != 0) {
            ok(!ret, "%d) CreateUrlCacheEntryW succeeded\n", i);
            ok(urls[i].err == GetLastError(), "%d) GetLastError() = %d\n", i, GetLastError());
            continue;
        }
        ok(ret, "%d) CreateUrlCacheEntryW failed: %d\n", i, GetLastError());

        /* dwHeaderSize is ignored, pass 0 to prove it */
        ret = CommitUrlCacheEntryW(urls[i].url, bufW, filetime_zero, filetime_zero,
                NORMAL_CACHE_ENTRY, urls[i].header_info, 0, urls[i].extension, NULL);
        ok(ret, "%d) CommitUrlCacheEntryW failed: %d\n", i, GetLastError());

        SetLastError(0xdeadbeef);
        size = 0;
        ret = GetUrlCacheEntryInfoW(urls[i].url, NULL, &size);
        ok(!ret && GetLastError()==ERROR_INSUFFICIENT_BUFFER,
                "%d) GetLastError() = %d\n", i, GetLastError());
        entry_infoW = HeapAlloc(GetProcessHeap(), 0, size);
        ret = GetUrlCacheEntryInfoW(urls[i].url, entry_infoW, &size);
        ok(ret, "%d) GetUrlCacheEntryInfoW failed: %d\n", i, GetLastError());

        ret = GetUrlCacheEntryInfoA(urls[i].encoded_url, NULL, &size);
        ok(!ret && GetLastError()==ERROR_INSUFFICIENT_BUFFER,
                "%d) GetLastError() = %d\n", i, GetLastError());
        if(!ret && GetLastError()!=ERROR_INSUFFICIENT_BUFFER) {
            win_skip("ANSI version of url is incorrect\n");
            continue;
        }
        entry_infoA = HeapAlloc(GetProcessHeap(), 0, size);
        ret = GetUrlCacheEntryInfoA(urls[i].encoded_url, entry_infoA, &size);
        ok(ret, "%d) GetUrlCacheEntryInfoA failed: %d\n", i, GetLastError());

        ok(entry_infoW->dwStructSize == entry_infoA->dwStructSize,
                "%d) entry_infoW->dwStructSize = %d, expected %d\n",
                i, entry_infoW->dwStructSize, entry_infoA->dwStructSize);
        ok(!lstrcmpW(urls[i].url, entry_infoW->lpszSourceUrlName),
                "%d) entry_infoW->lpszSourceUrlName = %s\n",
                i, wine_dbgstr_w(entry_infoW->lpszSourceUrlName));
        ok(!lstrcmpA(urls[i].encoded_url, entry_infoA->lpszSourceUrlName),
                "%d) entry_infoA->lpszSourceUrlName = %s\n",
                i, entry_infoA->lpszSourceUrlName);
        ok(entry_infoW->CacheEntryType == entry_infoA->CacheEntryType,
                "%d) entry_infoW->CacheEntryType = %x, expected %x\n",
                i, entry_infoW->CacheEntryType, entry_infoA->CacheEntryType);
        ok(entry_infoW->dwUseCount == entry_infoA->dwUseCount,
                "%d) entry_infoW->dwUseCount = %d, expected %d\n",
                i, entry_infoW->dwUseCount, entry_infoA->dwUseCount);
        ok(entry_infoW->dwHitRate == entry_infoA->dwHitRate,
                "%d) entry_infoW->dwHitRate = %d, expected %d\n",
                i, entry_infoW->dwHitRate, entry_infoA->dwHitRate);
        ok(entry_infoW->dwSizeLow == entry_infoA->dwSizeLow,
                "%d) entry_infoW->dwSizeLow = %d, expected %d\n",
                i, entry_infoW->dwSizeLow, entry_infoA->dwSizeLow);
        ok(entry_infoW->dwSizeHigh == entry_infoA->dwSizeHigh,
                "%d) entry_infoW->dwSizeHigh = %d, expected %d\n",
                i, entry_infoW->dwSizeHigh, entry_infoA->dwSizeHigh);
        ok(!memcmp(&entry_infoW->LastModifiedTime, &entry_infoA->LastModifiedTime, sizeof(FILETIME)),
                "%d) entry_infoW->LastModifiedTime is incorrect\n", i);
        ok(!memcmp(&entry_infoW->ExpireTime, &entry_infoA->ExpireTime, sizeof(FILETIME)),
                "%d) entry_infoW->ExpireTime is incorrect\n", i);
        ok(!memcmp(&entry_infoW->LastAccessTime, &entry_infoA->LastAccessTime, sizeof(FILETIME)),
                "%d) entry_infoW->LastAccessTime is incorrect\n", i);
        ok(!memcmp(&entry_infoW->LastSyncTime, &entry_infoA->LastSyncTime, sizeof(FILETIME)),
                "%d) entry_infoW->LastSyncTime is incorrect\n", i);

        MultiByteToWideChar(CP_ACP, 0, entry_infoA->lpszLocalFileName, -1, bufW, MAX_PATH);
        ok(!lstrcmpW(entry_infoW->lpszLocalFileName, bufW),
                "%d) entry_infoW->lpszLocalFileName = %s, expected %s\n",
                i, wine_dbgstr_w(entry_infoW->lpszLocalFileName), wine_dbgstr_w(bufW));

        if(!urls[i].header_info[0]) {
            ok(!entry_infoW->lpHeaderInfo, "entry_infoW->lpHeaderInfo != NULL\n");
        }else {
            ok(!lstrcmpW((WCHAR*)entry_infoW->lpHeaderInfo, urls[i].header_info),
                    "%d) entry_infoW->lpHeaderInfo = %s\n",
                    i, wine_dbgstr_w((WCHAR*)entry_infoW->lpHeaderInfo));
        }

        if(!urls[i].extension[0]) {
            ok(!entry_infoW->lpszFileExtension || (ie10_cache && !entry_infoW->lpszFileExtension[0]),
                    "%d) entry_infoW->lpszFileExtension = %s\n",
                    i, wine_dbgstr_w(entry_infoW->lpszFileExtension));
        }else {
            MultiByteToWideChar(CP_ACP, 0, entry_infoA->lpszFileExtension, -1, bufW, MAX_PATH);
            ok(!lstrcmpW(entry_infoW->lpszFileExtension, bufW) ||
                    (ie10_cache && !lstrcmpW(entry_infoW->lpszFileExtension, urls[i].extension)),
                    "%d) entry_infoW->lpszFileExtension = %s, expected %s\n",
                    i, wine_dbgstr_w(entry_infoW->lpszFileExtension), wine_dbgstr_w(bufW));
        }

        HeapFree(GetProcessHeap(), 0, entry_infoW);
        HeapFree(GetProcessHeap(), 0, entry_infoA);

        if(pDeleteUrlCacheEntryA) {
            ret = pDeleteUrlCacheEntryA(urls[i].encoded_url);
            ok(ret, "%d) DeleteUrlCacheEntryW failed: %d\n", i, GetLastError());
        }
    }
}

static void test_FindCloseUrlCache(void)
{
    BOOL r;
    DWORD err;

    SetLastError(0xdeadbeef);
    r = FindCloseUrlCache(NULL);
    err = GetLastError();
    ok(0 == r, "expected 0, got %d\n", r);
    ok(ERROR_INVALID_HANDLE == err, "expected %d, got %d\n", ERROR_INVALID_HANDLE, err);
}

static void test_GetDiskInfoA(void)
{
    BOOL ret;
    DWORD error, cluster_size;
    DWORDLONG free, total;
    char path[MAX_PATH], *p;

    GetSystemDirectoryA(path, MAX_PATH);
    if ((p = strchr(path, '\\'))) *++p = 0;

    ret = GetDiskInfoA(path, &cluster_size, &free, &total);
    ok(ret, "GetDiskInfoA failed %u\n", GetLastError());

    ret = GetDiskInfoA(path, &cluster_size, &free, NULL);
    ok(ret, "GetDiskInfoA failed %u\n", GetLastError());

    ret = GetDiskInfoA(path, &cluster_size, NULL, NULL);
    ok(ret, "GetDiskInfoA failed %u\n", GetLastError());

    ret = GetDiskInfoA(path, NULL, NULL, NULL);
    ok(ret, "GetDiskInfoA failed %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    strcpy(p, "\\non\\existing\\path");
    ret = GetDiskInfoA(path, NULL, NULL, NULL);
    error = GetLastError();
    ok(!ret ||
       broken(old_ie && ret), /* < IE7 */
       "GetDiskInfoA succeeded\n");
    ok(error == ERROR_PATH_NOT_FOUND ||
       broken(old_ie && error == 0xdeadbeef), /* < IE7 */
       "got %u expected ERROR_PATH_NOT_FOUND\n", error);

    SetLastError(0xdeadbeef);
    ret = GetDiskInfoA(NULL, NULL, NULL, NULL);
    error = GetLastError();
    ok(!ret, "GetDiskInfoA succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "got %u expected ERROR_INVALID_PARAMETER\n", error);
}

START_TEST(urlcache)
{
    HMODULE hdll;
    hdll = GetModuleHandleA("wininet.dll");

    if(!GetProcAddress(hdll, "InternetGetCookieExW")) {
        win_skip("Too old IE (older than 6.0)\n");
        return;
    }
    if(!GetProcAddress(hdll, "InternetGetSecurityInfoByURL")) /* < IE7 */
        old_ie = TRUE;

    if(GetProcAddress(hdll, "CreateUrlCacheEntryExW")) {
        trace("Running tests on IE10 or newer\n");
        ie10_cache = TRUE;
    }

    pDeleteUrlCacheEntryA = (void*)GetProcAddress(hdll, "DeleteUrlCacheEntryA");
    pUnlockUrlCacheEntryFileA = (void*)GetProcAddress(hdll, "UnlockUrlCacheEntryFileA");
    test_urlcacheA();
    test_urlcacheW();
    test_FindCloseUrlCache();
    test_GetDiskInfoA();
}
