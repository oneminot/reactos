/*
 * Copyright 2000 Juergen Schmied
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

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/************************* STRRET functions ****************************/

BOOL WINAPI StrRetToStrNA(LPSTR dest, DWORD len, LPSTRRET src, const ITEMIDLIST *pidl)
{
    TRACE("dest=%p len=0x%x strret=%p(%s) pidl=%p\n",
        dest,len,src,
        (src->uType == STRRET_WSTR) ? "STRRET_WSTR" :
        (src->uType == STRRET_CSTR) ? "STRRET_CSTR" :
        (src->uType == STRRET_OFFSET) ? "STRRET_OFFSET" : "STRRET_???",
        pidl);

    if (!dest)
        return FALSE;

    switch (src->uType)
    {
      case STRRET_WSTR:
        WideCharToMultiByte(CP_ACP, 0, src->pOleStr, -1, dest, len, NULL, NULL);
        CoTaskMemFree(src->pOleStr);
        break;

      case STRRET_CSTR:
        lstrcpynA(dest, src->cStr, len);
        break;

      case STRRET_OFFSET:
        lstrcpynA(dest, ((LPCSTR)&pidl->mkid)+src->uOffset, len);
        break;

      default:
        FIXME("unknown type!\n");
        if (len) *dest = '\0';
        return FALSE;
    }
    TRACE("-- %s\n", debugstr_a(dest) );
    return TRUE;
}

/************************************************************************/

BOOL WINAPI StrRetToStrNW(LPWSTR dest, DWORD len, LPSTRRET src, const ITEMIDLIST *pidl)
{
    TRACE("dest=%p len=0x%x strret=%p(%s) pidl=%p\n",
        dest,len,src,
        (src->uType == STRRET_WSTR) ? "STRRET_WSTR" :
        (src->uType == STRRET_CSTR) ? "STRRET_CSTR" :
        (src->uType == STRRET_OFFSET) ? "STRRET_OFFSET" : "STRRET_???",
        pidl);

    if (!dest)
        return FALSE;

    switch (src->uType)
    {
      case STRRET_WSTR:
        lstrcpynW(dest, src->pOleStr, len);
        CoTaskMemFree(src->pOleStr);
        break;

      case STRRET_CSTR:
            if (!MultiByteToWideChar( CP_ACP, 0, src->cStr, -1, dest, len ) && len)
                  dest[len-1] = 0;
        break;

      case STRRET_OFFSET:
            if (!MultiByteToWideChar( CP_ACP, 0, ((LPCSTR)&pidl->mkid)+src->uOffset, -1, dest, len ) && len)
                  dest[len-1] = 0;
        break;

      default:
        FIXME("unknown type!\n");
        if (len) *dest = '\0';
        return FALSE;
    }
    return TRUE;
}


/*************************************************************************
 * StrRetToStrN                [SHELL32.96]
 *
 * converts a STRRET to a normal string
 *
 * NOTES
 *  the pidl is for STRRET OFFSET
 */
EXTERN_C BOOL WINAPI StrRetToStrNAW(LPVOID dest, DWORD len, LPSTRRET src, const ITEMIDLIST *pidl)
{
    if(SHELL_OsIsUnicode())
        return StrRetToStrNW((LPWSTR)dest, len, src, pidl);
    else
        return StrRetToStrNA((LPSTR)dest, len, src, pidl);
}

/************************* OLESTR functions ****************************/

/************************************************************************
 *    StrToOleStr            [SHELL32.163]
 *
 */
static int StrToOleStrA (LPWSTR lpWideCharStr, LPCSTR lpMultiByteString)
{
    TRACE("(%p, %p %s)\n",
    lpWideCharStr, lpMultiByteString, debugstr_a(lpMultiByteString));

    return MultiByteToWideChar(0, 0, lpMultiByteString, -1, lpWideCharStr, MAX_PATH);

}
static int StrToOleStrW (LPWSTR lpWideCharStr, LPCWSTR lpWString)
{
    TRACE("(%p, %p %s)\n",
    lpWideCharStr, lpWString, debugstr_w(lpWString));

    wcscpy (lpWideCharStr, lpWString );
    return wcslen(lpWideCharStr);
}

EXTERN_C BOOL WINAPI StrToOleStrAW (LPWSTR lpWideCharStr, LPCVOID lpString)
{
    if (SHELL_OsIsUnicode())
      return StrToOleStrW (lpWideCharStr, (LPCWSTR)lpString);
    return StrToOleStrA (lpWideCharStr, (LPCSTR)lpString);
}

/*************************************************************************
 * StrToOleStrN                    [SHELL32.79]
 *  lpMulti, nMulti, nWide [IN]
 *  lpWide [OUT]
 */
static BOOL StrToOleStrNA (LPWSTR lpWide, INT nWide, LPCSTR lpStrA, INT nStr)
{
    TRACE("(%p, %x, %s, %x)\n", lpWide, nWide, debugstr_an(lpStrA,nStr), nStr);
    return MultiByteToWideChar (0, 0, lpStrA, nStr, lpWide, nWide);
}
static BOOL StrToOleStrNW (LPWSTR lpWide, INT nWide, LPCWSTR lpStrW, INT nStr)
{
    TRACE("(%p, %x, %s, %x)\n", lpWide, nWide, debugstr_wn(lpStrW, nStr), nStr);

    if (lstrcpynW (lpWide, lpStrW, nWide))
    { return wcslen (lpWide);
    }
    return 0;
}

EXTERN_C BOOL WINAPI StrToOleStrNAW (LPWSTR lpWide, INT nWide, LPCVOID lpStr, INT nStr)
{
    if (SHELL_OsIsUnicode())
      return StrToOleStrNW (lpWide, nWide, (LPCWSTR)lpStr, nStr);
    return StrToOleStrNA (lpWide, nWide, (LPCSTR)lpStr, nStr);
}

/*************************************************************************
 * OleStrToStrN                    [SHELL32.78]
 */
static BOOL OleStrToStrNA (LPSTR lpStr, INT nStr, LPCWSTR lpOle, INT nOle)
{
    TRACE("(%p, %x, %s, %x)\n", lpStr, nStr, debugstr_wn(lpOle,nOle), nOle);
    return WideCharToMultiByte (0, 0, lpOle, nOle, lpStr, nStr, NULL, NULL);
}

static BOOL OleStrToStrNW (LPWSTR lpwStr, INT nwStr, LPCWSTR lpOle, INT nOle)
{
    TRACE("(%p, %x, %s, %x)\n", lpwStr, nwStr, debugstr_wn(lpOle,nOle), nOle);

    if (lstrcpynW ( lpwStr, lpOle, nwStr))
    { return wcslen (lpwStr);
    }
    return 0;
}

EXTERN_C BOOL WINAPI OleStrToStrNAW (LPVOID lpOut, INT nOut, LPCVOID lpIn, INT nIn)
{
    if (SHELL_OsIsUnicode())
      return OleStrToStrNW ((LPWSTR)lpOut, nOut, (LPCWSTR)lpIn, nIn);
    return OleStrToStrNA ((LPSTR)lpOut, nOut, (LPCWSTR)lpIn, nIn);
}


/*************************************************************************
 * CheckEscapesA             [SHELL32.@]
 *
 * Checks a string for special characters which are not allowed in a path
 * and encloses it in quotes if that is the case.
 *
 * PARAMS
 *  string     [I/O] string to check and on return eventually quoted
 *  len        [I]   length of string
 *
 * RETURNS
 *  length of actual string
 *
 * NOTES
 *  Not really sure if this function returns actually a value at all.
 */
DWORD WINAPI CheckEscapesA(
    LPSTR    string,         /* [I/O]   string to check ??*/
    DWORD    len)            /* [I]      is 0 */
{
    LPWSTR wString;
    DWORD ret = 0;

    TRACE("(%s %d)\n", debugstr_a(string), len);
    wString = (LPWSTR)LocalAlloc(LPTR, len * sizeof(WCHAR));
    if (wString)
    {
      MultiByteToWideChar(CP_ACP, 0, string, len, wString, len);
      ret = CheckEscapesW(wString, len);
      WideCharToMultiByte(CP_ACP, 0, wString, len, string, len, NULL, NULL);
      LocalFree(wString);
    }
    return ret;
}

static const WCHAR strEscapedChars[] = {' ','"',',',';','^',0};

/*************************************************************************
 * CheckEscapesW             [SHELL32.@]
 *
 * See CheckEscapesA.
 */
DWORD WINAPI CheckEscapesW(
    LPWSTR    string,
    DWORD    len)
{
    DWORD size = wcslen(string);
    LPWSTR s, d;

    TRACE("(%s %d) stub\n", debugstr_w(string), len);

    if (StrPBrkW(string, strEscapedChars) && size + 2 <= len)
    {
      s = &string[size - 1];
      d = &string[size + 2];
      *d-- = 0;
      *d-- = '"';
      for (;d > string;)
        *d-- = *s--;
      *d = '"';
      return size + 2;
    }
    return size;
}
