/* $Id: stubs.c,v 1.37 2002/12/06 13:14:14 robd Exp $
 *
 * KERNEL32.DLL stubs (unimplemented functions)
 * Remove from this file, if you implement them.
 */
#include <windows.h>


BOOL
STDCALL
IsProcessorFeaturePresent( DWORD ProcessorFeature )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOL
STDCALL
BaseAttachCompleteThunk (VOID)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOL
STDCALL
CmdBatNotification (
    DWORD   Unknown
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


int
STDCALL
CompareStringA (
    LCID    Locale,
    DWORD   dwCmpFlags,
    LPCSTR  lpString1,
    int cchCount1,
    LPCSTR  lpString2,
    int cchCount2
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


int
STDCALL
CompareStringW (
    LCID    Locale,
    DWORD   dwCmpFlags,
    LPCWSTR lpString1,
    int cchCount1,
    LPCWSTR lpString2,
    int cchCount2
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


LCID
STDCALL
ConvertDefaultLocale (
    LCID    Locale
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


DWORD
STDCALL
CreateVirtualBuffer (
    DWORD   Unknown0,
    DWORD   Unknown1,
    DWORD   Unknown2
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


WINBOOL
STDCALL
EnumCalendarInfoW (
    CALINFO_ENUMPROC lpCalInfoEnumProc,
    LCID              Locale,
    CALID             Calendar,
    CALTYPE           CalType
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
EnumCalendarInfoA (
    CALINFO_ENUMPROC    lpCalInfoEnumProc,
    LCID            Locale,
    CALID           Calendar,
    CALTYPE         CalType
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
EnumDateFormatsW (
    DATEFMT_ENUMPROC    lpDateFmtEnumProc,
    LCID            Locale,
    DWORD           dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
EnumDateFormatsA (
    DATEFMT_ENUMPROC    lpDateFmtEnumProc,
    LCID            Locale,
    DWORD           dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
EnumSystemCodePagesW (
    CODEPAGE_ENUMPROC   lpCodePageEnumProc,
    DWORD           dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
EnumSystemCodePagesA (
    CODEPAGE_ENUMPROC   lpCodePageEnumProc,
    DWORD           dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
EnumSystemLocalesW (
    LOCALE_ENUMPROC lpLocaleEnumProc,
    DWORD       dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
EnumSystemLocalesA (
    LOCALE_ENUMPROC lpLocaleEnumProc,
    DWORD       dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
EnumTimeFormatsW (
    TIMEFMT_ENUMPROC    lpTimeFmtEnumProc,
    LCID            Locale,
    DWORD           dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
EnumTimeFormatsA (
    TIMEFMT_ENUMPROC    lpTimeFmtEnumProc,
    LCID            Locale,
    DWORD           dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


DWORD
STDCALL
ExitVDM (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


BOOL
STDCALL
ExtendVirtualBuffer (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


int
STDCALL
FoldStringW (
    DWORD   dwMapFlags,
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPWSTR  lpDestStr,
    int cchDest
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


int
STDCALL
FoldStringA (
    DWORD   dwMapFlags,
    LPCSTR  lpSrcStr,
    int cchSrc,
    LPSTR   lpDestStr,
    int cchDest
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


BOOL
STDCALL
FreeVirtualBuffer (
    HANDLE  hVirtualBuffer
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


UINT
STDCALL
GetACP (VOID)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


WINBOOL
STDCALL
GetBinaryTypeW (
    LPCWSTR lpApplicationName,
    LPDWORD lpBinaryType
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
GetBinaryTypeA (
    LPCSTR  lpApplicationName,
    LPDWORD lpBinaryType
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
GetCPInfo (
    UINT        a0,
    LPCPINFO    a1
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


int
STDCALL
GetCurrencyFormatW (
    LCID            Locale,
    DWORD           dwFlags,
    LPCWSTR         lpValue,
    CONST CURRENCYFMT   * lpFormat,
    LPWSTR          lpCurrencyStr,
    int         cchCurrency
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


int
STDCALL
GetCurrencyFormatA (
    LCID            Locale,
    DWORD           dwFlags,
    LPCSTR          lpValue,
    CONST CURRENCYFMT   * lpFormat,
    LPSTR           lpCurrencyStr,
    int         cchCurrency
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


int
STDCALL
GetDateFormatW (
    LCID            Locale,
    DWORD           dwFlags,
    CONST SYSTEMTIME    * lpDate,
    LPCWSTR         lpFormat,
    LPWSTR          lpDateStr,
    int         cchDate
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


int
STDCALL
GetDateFormatA (
    LCID            Locale,
    DWORD           dwFlags,
    CONST SYSTEMTIME    * lpDate,
    LPCSTR          lpFormat,
    LPSTR           lpDateStr,
    int         cchDate
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


int
STDCALL
GetLocaleInfoW (
    LCID    Locale,
    LCTYPE  LCType,
    LPWSTR  lpLCData,
    int cchData
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


int
STDCALL
GetLocaleInfoA (
    LCID    Locale,
    LCTYPE  LCType,
    LPSTR   lpLCData,
    int cchData
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


DWORD
STDCALL
GetNextVDMCommand (
    DWORD   Unknown0
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


int
STDCALL
GetNumberFormatW (
    LCID        Locale,
    DWORD       dwFlags,
    LPCWSTR     lpValue,
    CONST NUMBERFMT * lpFormat,
    LPWSTR      lpNumberStr,
    int     cchNumber
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


int
STDCALL
GetNumberFormatA (
    LCID        Locale,
    DWORD       dwFlags,
    LPCSTR      lpValue,
    CONST NUMBERFMT * lpFormat,
    LPSTR       lpNumberStr,
    int     cchNumber
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


UINT
STDCALL
GetOEMCP (VOID)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 437; /* FIXME: call csrss.exe */
}


WINBOOL
STDCALL
GetStringTypeExW (
    LCID    Locale,
    DWORD   dwInfoType,
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPWORD  lpCharType
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
GetStringTypeExA (
    LCID    Locale,
    DWORD   dwInfoType,
    LPCSTR  lpSrcStr,
    int cchSrc,
    LPWORD  lpCharType
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
GetStringTypeW (
    DWORD   dwInfoType,
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPWORD  lpCharType
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
GetStringTypeA (
    LCID    Locale,
    DWORD   dwInfoType,
    LPCSTR  lpSrcStr,
    int cchSrc,
    LPWORD  lpCharType
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


LCID
STDCALL
GetSystemDefaultLCID (VOID)
{
    /* FIXME: ??? */
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return MAKELCID(
        LANG_ENGLISH,
        SORT_DEFAULT
        );
}


LANGID
STDCALL
GetSystemDefaultLangID (VOID)
{
     /* FIXME: ??? */
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return MAKELANGID(
        LANG_ENGLISH,
        SUBLANG_ENGLISH_US
        );
}


DWORD
STDCALL
GetSystemPowerStatus (
    DWORD   Unknown0
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


LCID
STDCALL
GetThreadLocale (VOID)
{
    /* FIXME: ??? */
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return MAKELCID(
        LANG_ENGLISH,
        SORT_DEFAULT
        );
}


WINBOOL
STDCALL
GetThreadPriorityBoost (
    HANDLE      hThread,
    DWORD       dwSelector,
    LPLDT_ENTRY lpSelectorEntry
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
GetThreadSelectorEntry (
    HANDLE      hThread,
    DWORD       dwSelector,
    LPLDT_ENTRY lpSelectorEntry
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


int
STDCALL
GetTimeFormatW (
    LCID            Locale,
    DWORD           dwFlags,
    CONST SYSTEMTIME    * lpTime,
    LPCWSTR         lpFormat,
    LPWSTR          lpTimeStr,
    int         cchTime
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


int
STDCALL
GetTimeFormatA (
    LCID            Locale,
    DWORD           dwFlags,
    CONST SYSTEMTIME    * lpTime,
    LPCSTR          lpFormat,
    LPSTR           lpTimeStr,
    int         cchTime
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


LCID
STDCALL
GetUserDefaultLCID (VOID)
{
    /* FIXME: ??? */
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return MAKELCID(
        LANG_ENGLISH,
        SORT_DEFAULT
        );
}


LANGID
STDCALL
GetUserDefaultLangID (VOID)
{
     /* FIXME: ??? */
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return MAKELANGID(
        LANG_ENGLISH,
        SUBLANG_ENGLISH_US
        );
}


DWORD
STDCALL
GetVDMCurrentDirectories (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


WINBOOL
STDCALL
IsDBCSLeadByte (
    BYTE    TestChar
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
IsDBCSLeadByteEx (
    UINT    CodePage,
    BYTE    TestChar
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
IsValidCodePage (
    UINT    CodePage
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
IsValidLocale (
    LCID    Locale,
    DWORD   dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


int
STDCALL
LCMapStringA (
    LCID    Locale,
    DWORD   dwMapFlags,
    LPCSTR  lpSrcStr,
    int cchSrc,
    LPSTR   lpDestStr,
    int cchDest
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


int
STDCALL
LCMapStringW (
    LCID    Locale,
    DWORD   dwMapFlags,
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPWSTR  lpDestStr,
    int cchDest
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


DWORD
STDCALL
LoadModule (
    LPCSTR  lpModuleName,
    LPVOID  lpParameterBlock
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


WINBOOL
STDCALL
QueryPerformanceCounter (
    LARGE_INTEGER   * lpPerformanceCount
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
QueryPerformanceFrequency (
    LARGE_INTEGER   * lpFrequency
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
RegisterConsoleVDM (
    DWORD   Unknown0,
    DWORD   Unknown1,
    DWORD   Unknown2,
    DWORD   Unknown3,
    DWORD   Unknown4,
    DWORD   Unknown5,
    DWORD   Unknown6,
    DWORD   Unknown7,
    DWORD   Unknown8,
    DWORD   Unknown9,
    DWORD   Unknown10
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
RegisterWowBaseHandlers (
    DWORD   Unknown0
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
RegisterWowExec (
    DWORD   Unknown0
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
SetComputerNameA (
    LPCSTR  lpComputerName
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
SetComputerNameW (
    LPCWSTR lpComputerName
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
SetLocaleInfoA (
    LCID    Locale,
    LCTYPE  LCType,
    LPCSTR  lpLCData
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
SetLocaleInfoW (
    LCID    Locale,
    LCTYPE  LCType,
    LPCWSTR lpLCData
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
SetSystemPowerState (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
SetThreadIdealProcessor(VOID)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

WINBOOL
STDCALL
SetThreadLocale (
    LCID    Locale
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


WINBOOL
STDCALL
SetThreadPriorityBoost(VOID)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


WINBOOL
STDCALL
SetVDMCurrentDirectories (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


DWORD
STDCALL
TrimVirtualBuffer (
    DWORD   Unknown0
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


DWORD
STDCALL
VDMConsoleOperation (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


DWORD
STDCALL
VDMOperationStarted (
    DWORD   Unknown0
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


DWORD
STDCALL
VerLanguageNameA (
    DWORD   wLang,
    LPSTR   szLang,
    DWORD   nSize
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


DWORD
STDCALL
VerLanguageNameW (
    DWORD   wLang,
    LPWSTR  szLang,
    DWORD   nSize
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


DWORD
STDCALL
VirtualBufferExceptionHandler (
    DWORD   Unknown0,
    DWORD   Unknown1,
    DWORD   Unknown2
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/* EOF */
