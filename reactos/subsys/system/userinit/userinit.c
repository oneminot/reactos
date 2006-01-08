/*
 *  ReactOS applications
 *  Copyright (C) 2001, 2002 ReactOS Team
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
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Userinit Logon Application
 * FILE:        subsys/system/userinit/userinit.c
 * PROGRAMMERS: Thomas Weidenmueller (w3seek@users.sourceforge.net)
 */
#include <windows.h>
#include <cfgmgr32.h>
#include "resource.h"

#define CMP_MAGIC  0x01234567

/* GLOBALS ******************************************************************/

/* FUNCTIONS ****************************************************************/

static
BOOL GetShell(WCHAR *CommandLine)
{
  HKEY hKey;
  DWORD Type, Size;
  WCHAR Shell[MAX_PATH];
  BOOL Ret = FALSE;

  if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                  L"SOFTWARE\\ReactOS\\Windows NT\\CurrentVersion\\Winlogon",
                  0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
  {
    Size = MAX_PATH * sizeof(WCHAR);
    if(RegQueryValueEx(hKey,
                       L"Shell",
                       NULL,
                       &Type,
                       (LPBYTE)Shell,
                       &Size) == ERROR_SUCCESS)
    {
      if((Type == REG_SZ) || (Type == REG_EXPAND_SZ))
      {
        wcscpy(CommandLine, Shell);
        Ret = TRUE;
      }
    }
    RegCloseKey(hKey);
  }

  if(!Ret)
  {
    if(GetWindowsDirectory(CommandLine, MAX_PATH - 13))
      wcscat(CommandLine, L"\\explorer.exe");
    else
      wcscpy(CommandLine, L"explorer.exe");
  }

  return Ret;
}

static
void StartShell(void)
{
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  WCHAR Shell[MAX_PATH];
  WCHAR ExpandedShell[MAX_PATH];
  TCHAR szMsg[RC_STRING_MAX_SIZE];

  GetShell(Shell);

  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

  ExpandEnvironmentStrings(Shell, ExpandedShell, MAX_PATH);

  if(CreateProcess(NULL,
                   ExpandedShell,
                   NULL,
                   NULL,
                   FALSE,
                   NORMAL_PRIORITY_CLASS,
                   NULL,
                   NULL,
                   &si,
                   &pi))
  {
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
  }
  else
  {
    LoadString( GetModuleHandle(NULL), STRING_USERINIT_FAIL, szMsg, sizeof(szMsg) / sizeof(szMsg[0]));
    MessageBox(0, szMsg, NULL, 0);
  }
}

static
void SetUserSettings(void)
{
  HKEY hKey;
  DWORD Type, Size;
  WCHAR szWallpaper[MAX_PATH + 1];

  if(RegOpenKeyEx(HKEY_CURRENT_USER,
                  L"Control Panel\\Desktop",
                  0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
  {
    Size = sizeof(szWallpaper);
    if(RegQueryValueEx(hKey,
                       L"Wallpaper",
                       NULL,
                       &Type,
                       (LPBYTE)szWallpaper,
                       &Size) == ERROR_SUCCESS
       && Type == REG_SZ)
    {
      /* Load and change the wallpaper */
      SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, szWallpaper, SPIF_SENDCHANGE);
    }
    else
    {
      /* remove the wallpaper */
      SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, NULL, SPIF_SENDCHANGE);
    }
    RegCloseKey(hKey);
  }
}


typedef DWORD (WINAPI *PCMP_REPORT_LOGON)(DWORD, DWORD);

static VOID
NotifyLogon(VOID)
{
    HINSTANCE hModule;
    PCMP_REPORT_LOGON CMP_Report_LogOn;

    hModule = LoadLibrary(L"setupapi.dll");
    if (hModule)
    {
        CMP_Report_LogOn = (PCMP_REPORT_LOGON)GetProcAddress(hModule, "CMP_Report_LogOn");
        if (CMP_Report_LogOn)
            CMP_Report_LogOn(CMP_MAGIC, GetCurrentProcessId());

        FreeLibrary(hModule);
    }
}


#ifdef _MSC_VER
#pragma warning(disable : 4100)
#endif /* _MSC_VER */

int WINAPI
WinMain(HINSTANCE hInst,
        HINSTANCE hPrevInstance,
        LPSTR lpszCmdLine,
        int nCmdShow)
{
  NotifyLogon();
  SetUserSettings();
  StartShell();
  return 0;
}

/* EOF */
