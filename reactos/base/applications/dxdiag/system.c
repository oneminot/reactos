/*
 * PROJECT:     ReactX Diagnosis Application
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/dxdiag/system.c
 * PURPOSE:     ReactX diagnosis system page
 * COPYRIGHT:   Copyright 2008 Johannes Anderwald
 *
 */

#include "precomp.h"

static 
BOOL
GetDirectXVersion(WCHAR * szBuffer)
{
    WCHAR szVer[20];
    DWORD dwVer = sizeof(szVer);

    if (RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\DirectX", L"Version", RRF_RT_REG_SZ, NULL, szVer, &dwVer) != ERROR_SUCCESS)
        return FALSE;

    if(!wcscmp(szVer, L"4.02.0095"))
        wcscpy(szBuffer, L"1.0");
    else if (!wcscmp(szVer, L"4.03.00.1096"))
        wcscpy(szBuffer, L"2.0");
    else if (!wcscmp(szVer, L"4.04.0068"))
        wcscpy(szBuffer, L"3.0");
    else if (!wcscmp(szVer, L"4.04.0069"))
        wcscpy(szBuffer, L"3.0");
    else if (!wcscmp(szVer, L"4.05.00.0155"))
        wcscpy(szBuffer, L"5.0");
    else if (!wcscmp(szVer, L"4.05.01.1721"))
        wcscpy(szBuffer, L"5.0");
    else if (!wcscmp(szVer, L"4.05.01.1998"))
        wcscpy(szBuffer, L"5.0");
    else if (!wcscmp(szVer, L"4.06.02.0436"))
        wcscpy(szBuffer, L"6.0");
    else if (!wcscmp(szVer, L"4.07.00.0700"))
        wcscpy(szBuffer, L"7.0");
    else if (!wcscmp(szVer, L"4.07.00.0716"))
        wcscpy(szBuffer, L"7.0a");
    else if (!wcscmp(szVer, L"4.08.00.0400"))
        wcscpy(szBuffer, L"8.0");
    else if (!wcscmp(szVer, L"4.08.01.0881"))
        wcscpy(szBuffer, L"8.1");
    else if (!wcscmp(szVer, L"4.08.01.0810"))
        wcscpy(szBuffer, L"8.1");
    else if (!wcscmp(szVer, L"4.09.0000.0900"))
        wcscpy(szBuffer, L"9.0");
    else if (!wcscmp(szVer, L"4.09.00.0900"))
        wcscpy(szBuffer, L"9.0");
    else if (!wcscmp(szVer, L"4.09.0000.0901"))
        wcscpy(szBuffer, L"9.0a");
    else if (!wcscmp(szVer, L"4.09.00.0901"))
        wcscpy(szBuffer, L"9.0a");
    else if (!wcscmp(szVer, L"4.09.0000.0902"))
        wcscpy(szBuffer, L"9.0b");
    else if (!wcscmp(szVer, L"4.09.00.0902"))
        wcscpy(szBuffer, L"9.0b");
    else if (!wcscmp(szVer, L"4.09.00.0904"))
        wcscpy(szBuffer, L"9.0c");
    else if (!wcscmp(szVer, L"4.09.0000.0904"))
        wcscpy(szBuffer, L"9.0c");
    else
        return FALSE;

    return TRUE;
}

#if 0
static
BOOL
GetVistaVersion(WCHAR * szBuffer)
{
     DWORD Length;

     if (GetProductInfo(6, 0, 0, 0, &Length))
     {
         switch(Length)
         {
             case PRODUCT_ULTIMATE:
                 swprintf(szBuffer, L"Windows Vista Ultimate (6.0, Build %04u)", info.dwBuildNumber);
                 return TRUE;
             case PRODUCT_HOME_BASIC:
                 swprintf(szBuffer, L"Windows Vista Home Basic (6.0, Build %04u)", info.dwBuildNumber);
                 return TRUE;
             case PRODUCT_HOME_PREMIUM:
                 swprintf(szBuffer, L"Windows Vista Home Premimum (6.0, Build %04u)", info.dwBuildNumber);
                 return TRUE;
             case PRODUCT_ENTERPRISE:
                 swprintf(szBuffer, L"Windows Vista Enterprise (6.0, Build %04u)", info.dwBuildNumber);
                 return TRUE;
             case PRODUCT_HOME_BASIC_N:
                 swprintf(szBuffer, L"Windows Vista Home Basic N(6.0, Build %04u)", info.dwBuildNumber);
                 return TRUE;
             case PRODUCT_BUSINESS:
                 swprintf(szBuffer, L"Windows Vista Business(6.0, Build %04u)", info.dwBuildNumber);
                 return TRUE;
            case PRODUCT_STARTER:
                 swprintf(szBuffer, L"Windows Vista Starter(6.0, Build %04u)", info.dwBuildNumber);
                 return TRUE;
            case PRODUCT_BUSINESS_N:
                 swprintf(szBuffer, L"Windows Vista Business N(6.0, Build %04u)", info.dwBuildNumber);
                 return TRUE;
            default:
                 return FALSE;
                }
            }
}

#endif


static
BOOL
GetOSVersion(WCHAR * szBuffer)
{
    OSVERSIONINFOEXW info;

    ZeroMemory(&info, sizeof(info));
    info.dwOSVersionInfoSize = sizeof(info);
    if (GetVersionExW((LPOSVERSIONINFO)&info))
    {
        /* FIXME retrieve ReactOS version*/
        if (info.dwMajorVersion == 4)
        {
            wcscpy(szBuffer, L"Windows NT 4.0");
            if (info.szCSDVersion[0])
            {
                wcscat(szBuffer, L" ");
                wcscat(szBuffer, info.szCSDVersion);
            }
            return TRUE;
        }

        if (info.dwMajorVersion == 5 && info.dwMinorVersion == 0)
        {
            wcscpy(szBuffer, L"Windows 2000");
            if (info.szCSDVersion[0])
            {
                wcscat(szBuffer, L" ");
                wcscat(szBuffer, info.szCSDVersion);
            }
            return TRUE;
        }

        if (info.dwMajorVersion == 5 && info.dwMinorVersion == 1)
        {
            wcscpy(szBuffer, L"Windows XP");
            if (info.szCSDVersion[0])
            {
                wcscat(szBuffer, L" ");
                wcscat(szBuffer, info.szCSDVersion);
            }
            return TRUE;
        }

        if (info.dwMajorVersion == 6 && info.dwMinorVersion == 0)
        {
//            if (GetVistaVersion(szBuffer))
//                return TRUE;

            swprintf(szBuffer, L"Windows Vista (6.0, Build %04u)", info.dwBuildNumber);
            return TRUE;
        }
    }

    return FALSE;
}

static
VOID
InitializeSystemPage(HWND hwndDlg)
{
    WCHAR szTime[200];
    DWORD Length;
    DWORDLONG AvailableBytes, UsedBytes;
    MEMORYSTATUSEX mem;
    WCHAR szFormat[40];
    WCHAR szDesc[50];
    SYSTEM_INFO SysInfo;

    /* set date/time */
    szTime[0] = L'\0';
    Length = GetDateFormat(LOCALE_SYSTEM_DEFAULT, DATE_LONGDATE, NULL, NULL, szTime, sizeof(szTime) / sizeof(WCHAR));
    if (Length)
    {
        szTime[Length-1] = L',';
        szTime[Length++] = L' ';
    }
    Length = GetTimeFormatW(LOCALE_SYSTEM_DEFAULT, TIME_FORCE24HOURFORMAT|LOCALE_NOUSEROVERRIDE, NULL, NULL, &szTime[Length], (sizeof(szTime) / sizeof(WCHAR)));
    szTime[199] = L'\0';
    SendDlgItemMessageW(hwndDlg, IDC_STATIC_TIME, WM_SETTEXT, 0, (LPARAM)szTime);

    /* set computer name */
    szTime[0] = L'\0';
    Length = sizeof(szTime) / sizeof(WCHAR);
    if (GetComputerNameW(szTime, &Length))
        SendDlgItemMessageW(hwndDlg, IDC_STATIC_COMPUTER, WM_SETTEXT, 0, (LPARAM)szTime);

    /* set product name */
    if (GetOSVersion(szTime))
    {
        SendDlgItemMessage(hwndDlg, IDC_STATIC_OS, WM_SETTEXT, 0, (LPARAM)szTime);
    }
    else
    {
        if (LoadStringW(hInst, IDS_VERSION_UNKNOWN, szTime, sizeof(szTime) / sizeof(WCHAR)))
        {
            szTime[(sizeof(szTime) / sizeof(WCHAR))-1] = L'\0';
            SendDlgItemMessage(hwndDlg, IDC_STATIC_VERSION, WM_SETTEXT, 0, (LPARAM)szTime);
        }
    }

    /* FIXME set product language/local language */
    if (GetLocaleInfo(LOCALE_SYSTEM_DEFAULT,LOCALE_SLANGUAGE , szTime, sizeof(szTime) / sizeof(WCHAR)))
        SendDlgItemMessageW(hwndDlg, IDC_STATIC_LANG, WM_SETTEXT, 0, (LPARAM)szTime);

    /* set system manufacturer */
    szTime[0] = L'\0';
    Length = sizeof(szTime) / sizeof(WCHAR);
    if (RegGetValueW(HKEY_LOCAL_MACHINE, L"Hardware\\Description\\System\\BIOS", L"SystemManufacturer", RRF_RT_REG_SZ, NULL, szTime, &Length) == ERROR_SUCCESS)
    {
        szTime[199] = L'\0';
        SendDlgItemMessageW(hwndDlg, IDC_STATIC_MANU, WM_SETTEXT, 0, (LPARAM)szTime);
    }

    /* set motherboard model */
    szTime[0] = L'\0';
    Length = sizeof(szTime) / sizeof(WCHAR);
    if (RegGetValueW(HKEY_LOCAL_MACHINE, L"Hardware\\Description\\System\\BIOS", L"SystemProductName", RRF_RT_REG_SZ, NULL, szTime, &Length) == ERROR_SUCCESS)
    {
        szTime[199] = L'\0';
        SendDlgItemMessageW(hwndDlg, IDC_STATIC_MODEL, WM_SETTEXT, 0, (LPARAM)szTime);
    }

    /* set bios model */
    szTime[0] = L'\0';
    Length = sizeof(szTime) / sizeof(WCHAR);
    if (RegGetValueW(HKEY_LOCAL_MACHINE, L"Hardware\\Description\\System\\BIOS", L"BIOSVendor", RRF_RT_REG_SZ, NULL, szTime, &Length) == ERROR_SUCCESS)
    {
        DWORD Index;
        DWORD StrLength = (sizeof(szTime) / sizeof(WCHAR)) - (Length/sizeof(WCHAR));

        Index = (Length/sizeof(WCHAR));
        szTime[Index-1] = L' ';

        if (RegGetValueW(HKEY_LOCAL_MACHINE, L"Hardware\\Description\\System\\BIOS", L"BIOSReleaseDate", RRF_RT_REG_SZ, NULL, &szTime[Index], &StrLength) == ERROR_SUCCESS)
        {
            StrLength = (StrLength/sizeof(WCHAR));
        }
        szTime[Index+StrLength] = L' ';
        wcscpy(&szTime[Index+StrLength], L"Ver: "); //FIXME NON-NLS
        szTime[199] = L'\0';
        SendDlgItemMessageW(hwndDlg, IDC_STATIC_BIOS, WM_SETTEXT, 0, (LPARAM)szTime);
        //FIXME  retrieve BiosMajorRelease, BiosMinorRelease
    }
    /* set processor string */
    Length = sizeof(szDesc);
    if (RegGetValueW(HKEY_LOCAL_MACHINE, L"Hardware\\Description\\System\\CentralProcessor\\0", L"ProcessorNameString", RRF_RT_REG_SZ, NULL, szDesc, &Length) == ERROR_SUCCESS)
    {
        /* FIXME retrieve current speed */
        szFormat[0] = L'\0';
        GetSystemInfo(&SysInfo);
        if (SysInfo.dwNumberOfProcessors > 1)
            LoadStringW(hInst, IDS_FORMAT_MPPROC, szFormat, sizeof(szFormat) / sizeof(WCHAR));
        else
            LoadStringW(hInst, IDS_FORMAT_UNIPROC, szFormat, sizeof(szFormat) / sizeof(WCHAR));

        szFormat[(sizeof(szFormat)/sizeof(WCHAR))-1] = L'\0';
        swprintf(szTime, szFormat, szDesc, SysInfo.dwNumberOfProcessors);
        SendDlgItemMessageW(hwndDlg, IDC_STATIC_PROC, WM_SETTEXT, 0, (LPARAM)szTime);
    }

    /* retrieve available memory */
    ZeroMemory(&mem, sizeof(mem));
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem))
    {
        if (LoadStringW(hInst, IDS_FORMAT_MB, szFormat, sizeof(szFormat) / sizeof(WCHAR)))
        {
            /* set total mem string */
            szFormat[(sizeof(szFormat) / sizeof(WCHAR))-1] = L'\0';
            swprintf(szTime, szFormat, (mem.ullTotalPhys/1048576));
            SendDlgItemMessageW(hwndDlg, IDC_STATIC_MEM, WM_SETTEXT, 0, (LPARAM)szTime);
        }

        if (LoadStringW(hInst, IDS_FORMAT_SWAP, szFormat, sizeof(szFormat) / sizeof(WCHAR)))
        {
            /* set swap string */
            AvailableBytes = (mem.ullTotalPageFile-mem.ullTotalPhys)/1048576;
            UsedBytes = (mem.ullTotalPageFile-mem.ullAvailPageFile)/1048576;

            szFormat[(sizeof(szFormat) / sizeof(WCHAR))-1] = L'\0';
            swprintf(szTime, szFormat, (UsedBytes), (AvailableBytes));
            SendDlgItemMessageW(hwndDlg, IDC_STATIC_SWAP, WM_SETTEXT, 0, (LPARAM)szTime);
        }
    }
    /* set directx version string */
    wcscpy(szTime, L"ReactX ");
    if (GetDirectXVersion(&szTime[7]))
    {
        SendDlgItemMessage(hwndDlg, IDC_STATIC_VERSION, WM_SETTEXT, 0, (LPARAM)szTime);
    }
    else
    {
        if (LoadStringW(hInst, IDS_VERSION_UNKNOWN, szTime, sizeof(szTime) / sizeof(WCHAR)))
        {
            szTime[(sizeof(szTime) / sizeof(WCHAR))-1] = L'\0';
            SendDlgItemMessage(hwndDlg, IDC_STATIC_VERSION, WM_SETTEXT, 0, (LPARAM)szTime);
        }
    }
}


INT_PTR CALLBACK
SystemPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    switch (message)
    {
        case WM_INITDIALOG:
        {
            SetWindowPos(hDlg, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
            InitializeSystemPage(hDlg);
            return TRUE;
        }
    }

    return FALSE;
}
