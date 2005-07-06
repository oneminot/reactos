/*
 * SetupAPI device installer
 *
 * Copyright 2000 Andreas Mohr for CodeWeavers
 *           2005 Herv� Poussineau (hpoussin@reactos.com)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winreg.h"
#include "winternl.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "setupapi.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "cfgmgr32.h"
#include "initguid.h"
#include "winioctl.h"
#include "rpc.h"
#include "rpcdce.h"

#include "setupapi_private.h"


WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

/* Unicode constants */
static const WCHAR ClassGUID[]  = {'C','l','a','s','s','G','U','I','D',0};
static const WCHAR Class[]  = {'C','l','a','s','s',0};
static const WCHAR ClassInstall32[]  = {'C','l','a','s','s','I','n','s','t','a','l','l','3','2',0};
static const WCHAR NoDisplayClass[]  = {'N','o','D','i','s','p','l','a','y','C','l','a','s','s',0};
static const WCHAR NoInstallClass[]  = {'N','o','I','s','t','a','l','l','C','l','a','s','s',0};
static const WCHAR NoUseClass[]  = {'N','o','U','s','e','C','l','a','s','s',0};
static const WCHAR NtExtension[]  = {'.','N','T',0};
static const WCHAR NtPlatformExtension[]  = {'.','N','T','x','8','6',0};
static const WCHAR Version[]  = {'V','e','r','s','i','o','n',0};
static const WCHAR WinExtension[]  = {'.','W','i','n',0};

/* Registry key and value names */
static const WCHAR ControlClass[] = {'S','y','s','t','e','m','\\',
                                  'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                  'C','o','n','t','r','o','l','\\',
                                  'C','l','a','s','s',0};

static const WCHAR DeviceClasses[] = {'S','y','s','t','e','m','\\',
                                  'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                  'C','o','n','t','r','o','l','\\',
                                  'D','e','v','i','c','e','C','l','a','s','s','e','s',0};


typedef struct _DeviceInfo
{
    LIST_ENTRY ItemEntry;
#ifdef __WINE__
    PWSTR InterfaceName; /* "COMx:" */
    LPGUID InterfaceGuid; /* GUID_DEVINTERFACE_SERENUM_BUS_ENUMERATOR */
#else
    BOOL IsDevice; /* This entry is a device or an interface */
    union
    {
        struct
        {
            GUID ClassGuid;
            WCHAR RegistryKey[0]; /* "0000", "0001"... */
        } Device;
        struct
        {
            GUID ClassGuid;
            GUID InterfaceGuid;
            WCHAR DeviceInstance[0]; /* "ACPI\PNP0501\4&2658d0a0&0"... */
        } Interface;
    };
#endif
} DeviceInfo;

#define SETUP_DEV_INFO_LIST_MAGIC 0xd00ff056

typedef struct _DeviceInfoList
{
    DWORD magic;
    GUID ClassGuid; /* Only devices related of this class are in the device list */
    HWND hWnd;
    HKEY HKLM; /* Local or distant HKEY_LOCAL_MACHINE registry key */
    DWORD numberOfEntries;
    LIST_ENTRY ListHead;
} DeviceInfoList;

/* FIXME: header mess */
DEFINE_GUID(GUID_NULL,
  0x00000000L, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

/***********************************************************************
 *              SetupDiBuildClassInfoList  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiBuildClassInfoList(
        DWORD Flags,
        LPGUID ClassGuidList,
        DWORD ClassGuidListSize,
        PDWORD RequiredSize)
{
    TRACE("\n");
    return SetupDiBuildClassInfoListExW(Flags, ClassGuidList,
                                        ClassGuidListSize, RequiredSize,
                                        NULL, NULL);
}

/***********************************************************************
 *              SetupDiBuildClassInfoListExA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiBuildClassInfoListExA(
        DWORD Flags,
        LPGUID ClassGuidList,
        DWORD ClassGuidListSize,
        PDWORD RequiredSize,
        LPCSTR MachineName,
        PVOID Reserved)
{
    LPWSTR MachineNameW = NULL;
    BOOL bResult;

    TRACE("\n");

    if (MachineName)
    {
        MachineNameW = MultiByteToUnicode(MachineName, CP_ACP);
        if (MachineNameW == NULL) return FALSE;
    }

    bResult = SetupDiBuildClassInfoListExW(Flags, ClassGuidList,
                                           ClassGuidListSize, RequiredSize,
                                           MachineNameW, Reserved);

    if (MachineNameW)
        MyFree(MachineNameW);

    return bResult;
}

/***********************************************************************
 *		SetupDiBuildClassInfoListExW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiBuildClassInfoListExW(
        DWORD Flags,
        LPGUID ClassGuidList,
        DWORD ClassGuidListSize,
        PDWORD RequiredSize,
        LPCWSTR MachineName,
        PVOID Reserved)
{
    WCHAR szKeyName[MAX_GUID_STRING_LEN + 1];
    HKEY hClassesKey;
    HKEY hClassKey;
    DWORD dwLength;
    DWORD dwIndex;
    LONG lError;
    DWORD dwGuidListIndex = 0;

    TRACE("\n");

    if (RequiredSize != NULL)
	*RequiredSize = 0;

    hClassesKey = SetupDiOpenClassRegKeyExW(NULL,
                                            KEY_ALL_ACCESS,
                                            DIOCR_INSTALLER,
                                            MachineName,
                                            Reserved);
    if (hClassesKey == INVALID_HANDLE_VALUE)
    {
	return FALSE;
    }

    for (dwIndex = 0; ; dwIndex++)
    {
	dwLength = MAX_GUID_STRING_LEN + 1;
	lError = RegEnumKeyExW(hClassesKey,
			       dwIndex,
			       szKeyName,
			       &dwLength,
			       NULL,
			       NULL,
			       NULL,
			       NULL);
	TRACE("RegEnumKeyExW() returns %ld\n", lError);
	if (lError == ERROR_SUCCESS || lError == ERROR_MORE_DATA)
	{
	    TRACE("Key name: %p\n", szKeyName);

	    if (RegOpenKeyExW(hClassesKey,
			      szKeyName,
			      0,
			      KEY_ALL_ACCESS,
			      &hClassKey))
	    {
		RegCloseKey(hClassesKey);
		return FALSE;
	    }

	    if (!RegQueryValueExW(hClassKey,
				  NoUseClass,
				  NULL,
				  NULL,
				  NULL,
				  NULL))
	    {
		TRACE("'NoUseClass' value found!\n");
		RegCloseKey(hClassKey);
		continue;
	    }

	    if ((Flags & DIBCI_NOINSTALLCLASS) &&
		(!RegQueryValueExW(hClassKey,
				   NoInstallClass,
				   NULL,
				   NULL,
				   NULL,
				   NULL)))
	    {
		TRACE("'NoInstallClass' value found!\n");
		RegCloseKey(hClassKey);
		continue;
	    }

	    if ((Flags & DIBCI_NODISPLAYCLASS) &&
		(!RegQueryValueExW(hClassKey,
				   NoDisplayClass,
				   NULL,
				   NULL,
				   NULL,
				   NULL)))
	    {
		TRACE("'NoDisplayClass' value found!\n");
		RegCloseKey(hClassKey);
		continue;
	    }

	    RegCloseKey(hClassKey);

	    TRACE("Guid: %p\n", szKeyName);
	    if (dwGuidListIndex < ClassGuidListSize)
	    {
		if (szKeyName[0] == L'{' && szKeyName[37] == L'}')
		{
		    szKeyName[37] = 0;
		}
		TRACE("Guid: %p\n", &szKeyName[1]);

		UuidFromStringW(&szKeyName[1],
				&ClassGuidList[dwGuidListIndex]);
	    }

	    dwGuidListIndex++;
	}

	if (lError != ERROR_SUCCESS)
	    break;
    }

    RegCloseKey(hClassesKey);

    if (RequiredSize != NULL)
	*RequiredSize = dwGuidListIndex;

    if (ClassGuidListSize < dwGuidListIndex)
    {
	SetLastError(ERROR_INSUFFICIENT_BUFFER);
	return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *		SetupDiClassGuidsFromNameA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassGuidsFromNameA(
        LPCSTR ClassName,
        LPGUID ClassGuidList,
        DWORD ClassGuidListSize,
        PDWORD RequiredSize)
{
  return SetupDiClassGuidsFromNameExA(ClassName, ClassGuidList,
                                      ClassGuidListSize, RequiredSize,
                                      NULL, NULL);
}

/***********************************************************************
 *		SetupDiClassGuidsFromNameW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassGuidsFromNameW(
        LPCWSTR ClassName,
        LPGUID ClassGuidList,
        DWORD ClassGuidListSize,
        PDWORD RequiredSize)
{
  return SetupDiClassGuidsFromNameExW(ClassName, ClassGuidList,
                                      ClassGuidListSize, RequiredSize,
                                      NULL, NULL);
}

/***********************************************************************
 *		SetupDiClassGuidsFromNameExA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassGuidsFromNameExA(
        LPCSTR ClassName,
        LPGUID ClassGuidList,
        DWORD ClassGuidListSize,
        PDWORD RequiredSize,
        LPCSTR MachineName,
        PVOID Reserved)
{
    LPWSTR ClassNameW = NULL;
    LPWSTR MachineNameW = NULL;
    BOOL bResult;

    TRACE("\n");

    ClassNameW = MultiByteToUnicode(ClassName, CP_ACP);
    if (ClassNameW == NULL)
        return FALSE;

    if (MachineNameW)
    {
        MachineNameW = MultiByteToUnicode(MachineName, CP_ACP);
        if (MachineNameW == NULL)
        {
            MyFree(ClassNameW);
            return FALSE;
        }
    }

    bResult = SetupDiClassGuidsFromNameExW(ClassNameW, ClassGuidList,
                                           ClassGuidListSize, RequiredSize,
                                           MachineNameW, Reserved);

    if (MachineNameW)
        MyFree(MachineNameW);

    MyFree(ClassNameW);

    return bResult;
}

/***********************************************************************
 *		SetupDiClassGuidsFromNameExW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassGuidsFromNameExW(
        LPCWSTR ClassName,
        LPGUID ClassGuidList,
        DWORD ClassGuidListSize,
        PDWORD RequiredSize,
        LPCWSTR MachineName,
        PVOID Reserved)
{
    WCHAR szKeyName[MAX_GUID_STRING_LEN + 1];
    WCHAR szClassName[256];
    HKEY hClassesKey;
    HKEY hClassKey;
    DWORD dwLength;
    DWORD dwIndex;
    LONG lError;
    DWORD dwGuidListIndex = 0;

    if (RequiredSize != NULL)
	*RequiredSize = 0;

    hClassesKey = SetupDiOpenClassRegKeyExW(NULL,
                                            KEY_ENUMERATE_SUB_KEYS,
                                            DIOCR_INSTALLER,
                                            MachineName,
                                            Reserved);
    if (hClassesKey == INVALID_HANDLE_VALUE)
    {
	return FALSE;
    }

    for (dwIndex = 0; ; dwIndex++)
    {
	dwLength = MAX_GUID_STRING_LEN + 1;
	lError = RegEnumKeyExW(hClassesKey,
			       dwIndex,
			       szKeyName,
			       &dwLength,
			       NULL,
			       NULL,
			       NULL,
			       NULL);
	TRACE("RegEnumKeyExW() returns %ld\n", lError);
	if (lError == ERROR_SUCCESS || lError == ERROR_MORE_DATA)
	{
	    TRACE("Key name: %p\n", szKeyName);

	    if (RegOpenKeyExW(hClassesKey,
			      szKeyName,
			      0,
			      KEY_QUERY_VALUE,
			      &hClassKey))
	    {
		RegCloseKey(hClassesKey);
		return FALSE;
	    }

	    dwLength = 256 * sizeof(WCHAR);
	    if (!RegQueryValueExW(hClassKey,
				  Class,
				  NULL,
				  NULL,
				  (LPBYTE)szClassName,
				  &dwLength))
	    {
		TRACE("Class name: %p\n", szClassName);

		if (strcmpiW(szClassName, ClassName) == 0)
		{
		    TRACE("Found matching class name\n");

		    TRACE("Guid: %p\n", szKeyName);
		    if (dwGuidListIndex < ClassGuidListSize)
		    {
			if (szKeyName[0] == L'{' && szKeyName[37] == L'}')
			{
			    szKeyName[37] = 0;
			}
			TRACE("Guid: %p\n", &szKeyName[1]);

			UuidFromStringW(&szKeyName[1],
					&ClassGuidList[dwGuidListIndex]);
		    }

		    dwGuidListIndex++;
		}
	    }

	    RegCloseKey(hClassKey);
	}

	if (lError != ERROR_SUCCESS)
	    break;
    }

    RegCloseKey(hClassesKey);

    if (RequiredSize != NULL)
	*RequiredSize = dwGuidListIndex;

    if (ClassGuidListSize < dwGuidListIndex)
    {
	SetLastError(ERROR_INSUFFICIENT_BUFFER);
	return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *              SetupDiClassNameFromGuidA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassNameFromGuidA(
        const GUID* ClassGuid,
        PSTR ClassName,
        DWORD ClassNameSize,
        PDWORD RequiredSize)
{
  return SetupDiClassNameFromGuidExA(ClassGuid, ClassName,
                                     ClassNameSize, RequiredSize,
                                     NULL, NULL);
}

/***********************************************************************
 *              SetupDiClassNameFromGuidW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassNameFromGuidW(
        const GUID* ClassGuid,
        PWSTR ClassName,
        DWORD ClassNameSize,
        PDWORD RequiredSize)
{
  return SetupDiClassNameFromGuidExW(ClassGuid, ClassName,
                                     ClassNameSize, RequiredSize,
                                     NULL, NULL);
}

/***********************************************************************
 *              SetupDiClassNameFromGuidExA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassNameFromGuidExA(
        const GUID* ClassGuid,
        PSTR ClassName,
        DWORD ClassNameSize,
        PDWORD RequiredSize,
        PCSTR MachineName,
        PVOID Reserved)
{
    WCHAR ClassNameW[MAX_CLASS_NAME_LEN];
    LPWSTR MachineNameW = NULL;
    BOOL ret;

    if (MachineName)
        MachineNameW = MultiByteToUnicode(MachineName, CP_ACP);
    ret = SetupDiClassNameFromGuidExW(ClassGuid, ClassNameW, MAX_CLASS_NAME_LEN,
     NULL, MachineNameW, Reserved);
    if (ret)
    {
        int len = WideCharToMultiByte(CP_ACP, 0, ClassNameW, -1, ClassName,
         ClassNameSize, NULL, NULL);

        if (!ClassNameSize && RequiredSize)
            *RequiredSize = len;
    }
    MyFree(MachineNameW);
    return ret;
}

/***********************************************************************
 *		SetupDiClassNameFromGuidExW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassNameFromGuidExW(
        const GUID* ClassGuid,
        PWSTR ClassName,
        DWORD ClassNameSize,
        PDWORD RequiredSize,
        PCWSTR MachineName,
        PVOID Reserved)
{
    HKEY hKey;
    DWORD dwLength;
    LONG rc;

    hKey = SetupDiOpenClassRegKeyExW(ClassGuid,
                                     KEY_QUERY_VALUE,
                                     DIOCR_INSTALLER,
                                     MachineName,
                                     Reserved);
    if (hKey == INVALID_HANDLE_VALUE)
    {
	return FALSE;
    }

    if (RequiredSize != NULL)
    {
	dwLength = 0;
	rc = RegQueryValueExW(hKey,
			     Class,
			     NULL,
			     NULL,
			     NULL,
			     &dwLength);
	if (rc != ERROR_SUCCESS)
	{
	    SetLastError(rc);
	    RegCloseKey(hKey);
	    return FALSE;
	}

	*RequiredSize = dwLength / sizeof(WCHAR);
    }

    dwLength = ClassNameSize * sizeof(WCHAR);
    rc = RegQueryValueExW(hKey,
			 Class,
			 NULL,
			 NULL,
			 (LPBYTE)ClassName,
			 &dwLength);
    if (rc != ERROR_SUCCESS)
    {
	SetLastError(rc);
	RegCloseKey(hKey);
	return FALSE;
    }

    RegCloseKey(hKey);

    return TRUE;
}

/***********************************************************************
 *		SetupDiCreateDeviceInfoList (SETUPAPI.@)
 */
HDEVINFO WINAPI
SetupDiCreateDeviceInfoList(const GUID *ClassGuid,
			    HWND hwndParent)
{
  return SetupDiCreateDeviceInfoListExW(ClassGuid, hwndParent, NULL, NULL);
}

/***********************************************************************
 *		SetupDiCreateDeviceInfoListExA (SETUPAPI.@)
 */
HDEVINFO WINAPI
SetupDiCreateDeviceInfoListExA(const GUID *ClassGuid,
			       HWND hwndParent,
			       PCSTR MachineName,
			       PVOID Reserved)
{
    LPWSTR MachineNameW = NULL;
    HDEVINFO hDevInfo;

    TRACE("\n");

    if (MachineName)
    {
        MachineNameW = MultiByteToUnicode(MachineName, CP_ACP);
        if (MachineNameW == NULL)
            return (HDEVINFO)INVALID_HANDLE_VALUE;
    }

    hDevInfo = SetupDiCreateDeviceInfoListExW(ClassGuid, hwndParent,
                                              MachineNameW, Reserved);

    if (MachineNameW)
        MyFree(MachineNameW);

    return hDevInfo;
}

/***********************************************************************
 *		SetupDiCreateDeviceInfoListExW (SETUPAPI.@)
 */
HDEVINFO WINAPI
SetupDiCreateDeviceInfoListExW(const GUID *ClassGuid,
			       HWND hwndParent,
			       PCWSTR MachineName,
			       PVOID Reserved)
{
  DeviceInfoList* list;
  DWORD rc;

  TRACE("%p %p %p %p\n", ClassGuid, hwndParent, MachineName, Reserved);

  list = HeapAlloc(GetProcessHeap(), 0, sizeof(DeviceInfoList));
  if (!list)
  {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return (HDEVINFO)INVALID_HANDLE_VALUE;
  }

  list->magic = SETUP_DEV_INFO_LIST_MAGIC;
  list->hWnd = hwndParent;
  list->numberOfEntries = 0;
  if (MachineName)
    rc = RegConnectRegistryW(MachineName, HKEY_LOCAL_MACHINE, &list->HKLM);
  else
    rc = RegOpenKey(HKEY_LOCAL_MACHINE, NULL, &list->HKLM);
  if (rc != ERROR_SUCCESS)
  {
    SetLastError(rc);
    HeapFree(GetProcessHeap(), 0, list);
    return (HDEVINFO)INVALID_HANDLE_VALUE;
  }
  memcpy(
    &list->ClassGuid,
    ClassGuid ? ClassGuid : &GUID_NULL,
    sizeof(list->ClassGuid));
  InitializeListHead(&list->ListHead);
  return (HDEVINFO)list;
}

/***********************************************************************
 *		SetupDiEnumDeviceInfo (SETUPAPI.@)
 */
BOOL WINAPI SetupDiEnumDeviceInfo(
        HDEVINFO DeviceInfoSet,
        DWORD MemberIndex,
        PSP_DEVINFO_DATA DeviceInfoData)
{
    BOOL ret = FALSE;

    TRACE("%p, 0x%08lx, %p\n", DeviceInfoSet, MemberIndex, DeviceInfoData);
    if (!DeviceInfoData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoSet && DeviceInfoSet != (HDEVINFO)INVALID_HANDLE_VALUE)
    {
        DeviceInfoList *list = (DeviceInfoList *)DeviceInfoSet;
        
        if (list->magic != SETUP_DEV_INFO_LIST_MAGIC)
            SetLastError(ERROR_INVALID_HANDLE);
        else if (DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
            SetLastError(ERROR_INVALID_USER_BUFFER);
        else if (MemberIndex >= list->numberOfEntries)
            SetLastError(ERROR_NO_MORE_ITEMS);
        else
        {
            PLIST_ENTRY ItemList = list->ListHead.Flink;
            DeviceInfo* DevInfo;
            while (MemberIndex-- > 0)
                ItemList = ItemList->Flink;
            DevInfo = (DeviceInfo *)ItemList;
            if (DevInfo->IsDevice)
            {
#ifdef __WINE__
                memcpy(&DeviceInfoData->ClassGuid,
                    DevInfo->InterfaceGuid,
                    sizeof(GUID));
                DeviceInfoData->DevInst = 0; /* FIXME */
                DeviceInfoData->Reserved = 0;
#else
                memcpy(&DeviceInfoData->ClassGuid,
                    &DevInfo->Device.ClassGuid,
                    sizeof(GUID));
                DeviceInfoData->DevInst = 0; /* FIXME */
                DeviceInfoData->Reserved = 0;
#endif
                ret = TRUE;
            }
            else
            {
                SetLastError(ERROR_INVALID_PARAMETER);
            }
        }
    }
    else
        SetLastError(ERROR_INVALID_HANDLE);
    return ret;
}

/***********************************************************************
 *		SetupDiGetActualSectionToInstallA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetActualSectionToInstallA(
        HINF InfHandle,
        PCSTR InfSectionName,
        PSTR InfSectionWithExt,
        DWORD InfSectionWithExtSize,
        PDWORD RequiredSize,
        PSTR *Extension)
{
    LPWSTR InfSectionNameW = NULL;
    PWSTR InfSectionWithExtW = NULL;
    PWSTR ExtensionW;
    BOOL bResult;

    TRACE("\n");

    if (InfSectionName)
    {
        InfSectionNameW = MultiByteToUnicode(InfSectionName, CP_ACP);
        if (InfSectionNameW == NULL) goto end;
    }
    if (InfSectionWithExt)
    {
        InfSectionWithExtW = HeapAlloc(GetProcessHeap(), 0, InfSectionWithExtSize * sizeof(WCHAR));
        if (InfSectionWithExtW == NULL) goto end;
    }

    bResult = SetupDiGetActualSectionToInstallW(InfHandle, InfSectionNameW,
                                                InfSectionWithExt ? InfSectionNameW : NULL,
                                                InfSectionWithExtSize, RequiredSize,
                                                Extension ? &ExtensionW : NULL);

    if (bResult && InfSectionWithExt)
    {
         bResult = WideCharToMultiByte(CP_ACP, 0, InfSectionWithExtW, -1, InfSectionWithExt,
             InfSectionWithExtSize, NULL, NULL) != 0;
    }
    if (bResult && Extension)
    {
        if (ExtensionW == NULL)
            *Extension = NULL;
         else
            *Extension = &InfSectionWithExt[ExtensionW - InfSectionWithExtW];
    }

end:
    if (InfSectionNameW) MyFree(InfSectionNameW);
    if (InfSectionWithExtW) HeapFree(GetProcessHeap(), 0, InfSectionWithExtW);

    return bResult;
}

/***********************************************************************
 *		SetupDiGetActualSectionToInstallW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetActualSectionToInstallW(
        HINF InfHandle,
        PCWSTR InfSectionName,
        PWSTR InfSectionWithExt,
        DWORD InfSectionWithExtSize,
        PDWORD RequiredSize,
        PWSTR *Extension)
{
    WCHAR szBuffer[MAX_PATH];
    DWORD dwLength;
    DWORD dwFullLength;
    LONG lLineCount = -1;

    lstrcpyW(szBuffer, InfSectionName);
    dwLength = lstrlenW(szBuffer);

    if (OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
	/* Test section name with '.NTx86' extension */
	lstrcpyW(&szBuffer[dwLength], NtPlatformExtension);
	lLineCount = SetupGetLineCountW(InfHandle, szBuffer);

	if (lLineCount == -1)
	{
	    /* Test section name with '.NT' extension */
	    lstrcpyW(&szBuffer[dwLength], NtExtension);
	    lLineCount = SetupGetLineCountW(InfHandle, szBuffer);
	}
    }
    else
    {
	/* Test section name with '.Win' extension */
	lstrcpyW(&szBuffer[dwLength], WinExtension);
	lLineCount = SetupGetLineCountW(InfHandle, szBuffer);
    }

    if (lLineCount == -1)
    {
	/* Test section name without extension */
	szBuffer[dwLength] = 0;
	lLineCount = SetupGetLineCountW(InfHandle, szBuffer);
    }

    if (lLineCount == -1)
    {
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }

    dwFullLength = lstrlenW(szBuffer);

    if (InfSectionWithExt != NULL && InfSectionWithExtSize != 0)
    {
	if (InfSectionWithExtSize < (dwFullLength + 1))
	{
	    SetLastError(ERROR_INSUFFICIENT_BUFFER);
	    return FALSE;
	}

	lstrcpyW(InfSectionWithExt, szBuffer);
	if (Extension != NULL)
	{
	    *Extension = (dwLength == dwFullLength) ? NULL : &InfSectionWithExt[dwLength];
	}
    }

    if (RequiredSize != NULL)
    {
	*RequiredSize = dwFullLength + 1;
    }

    return TRUE;
}

/***********************************************************************
 *		SetupDiGetClassDescriptionA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetClassDescriptionA(
        const GUID* ClassGuid,
        PSTR ClassDescription,
        DWORD ClassDescriptionSize,
        PDWORD RequiredSize)
{
  return SetupDiGetClassDescriptionExA(ClassGuid, ClassDescription,
                                       ClassDescriptionSize,
                                       RequiredSize, NULL, NULL);
}

/***********************************************************************
 *		SetupDiGetClassDescriptionW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetClassDescriptionW(
        const GUID* ClassGuid,
        PWSTR ClassDescription,
        DWORD ClassDescriptionSize,
        PDWORD RequiredSize)
{
  return SetupDiGetClassDescriptionExW(ClassGuid, ClassDescription,
                                       ClassDescriptionSize,
                                       RequiredSize, NULL, NULL);
}

/***********************************************************************
 *		SetupDiGetClassDescriptionExA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetClassDescriptionExA(
        const GUID* ClassGuid,
        PSTR ClassDescription,
        DWORD ClassDescriptionSize,
        PDWORD RequiredSize,
        PCSTR MachineName,
        PVOID Reserved)
{
    PWCHAR ClassDescriptionW;
    LPWSTR MachineNameW = NULL;
    BOOL ret;

    TRACE("\n");
    if (ClassDescriptionSize > 0)
    {
        ClassDescriptionW = HeapAlloc(GetProcessHeap(), 0, ClassDescriptionSize * sizeof(WCHAR));
        if (!ClassDescriptionW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            ret = FALSE;
            goto end;
        }
    }

    if (MachineName)
    {
        MachineNameW = MultiByteToUnicode(MachineName, CP_ACP);
        if (!MachineNameW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            ret = FALSE;
            goto end;
        }
    }

    ret = SetupDiGetClassDescriptionExW(ClassGuid, ClassDescriptionW, ClassDescriptionSize * sizeof(WCHAR),
     NULL, MachineNameW, Reserved);
    if (ret)
    {
        int len = WideCharToMultiByte(CP_ACP, 0, ClassDescriptionW, -1, ClassDescription,
         ClassDescriptionSize, NULL, NULL);

        if (!ClassDescriptionSize && RequiredSize)
            *RequiredSize = len;
    }

end:
    HeapFree(GetProcessHeap(), 0, ClassDescriptionW);
    MyFree(MachineNameW);
    return ret;
}

/***********************************************************************
 *		SetupDiGetClassDescriptionExW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetClassDescriptionExW(
        const GUID* ClassGuid,
        PWSTR ClassDescription,
        DWORD ClassDescriptionSize,
        PDWORD RequiredSize,
        PCWSTR MachineName,
        PVOID Reserved)
{
    HKEY hKey;
    DWORD dwLength;

    hKey = SetupDiOpenClassRegKeyExW(ClassGuid,
                                     KEY_ALL_ACCESS,
                                     DIOCR_INSTALLER,
                                     MachineName,
                                     Reserved);
    if (hKey == INVALID_HANDLE_VALUE)
    {
	WARN("SetupDiOpenClassRegKeyExW() failed (Error %lu)\n", GetLastError());
	return FALSE;
    }

    if (RequiredSize != NULL)
    {
	dwLength = 0;
	if (RegQueryValueExW(hKey,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     &dwLength))
	{
	    RegCloseKey(hKey);
	    return FALSE;
	}

	*RequiredSize = dwLength / sizeof(WCHAR);
    }

    dwLength = ClassDescriptionSize * sizeof(WCHAR);
    if (RegQueryValueExW(hKey,
			 NULL,
			 NULL,
			 NULL,
			 (LPBYTE)ClassDescription,
			 &dwLength))
    {
	RegCloseKey(hKey);
	return FALSE;
    }

    RegCloseKey(hKey);

    return TRUE;
}

/***********************************************************************
 *		SetupDiGetClassDevsA (SETUPAPI.@)
 */
HDEVINFO WINAPI SetupDiGetClassDevsA(
       CONST GUID *class,
       LPCSTR enumstr,
       HWND parent,
       DWORD flags)
{
    return SetupDiGetClassDevsExA(class, enumstr, parent,
                                  flags, NULL, NULL, NULL);
}

/***********************************************************************
 *		SetupDiGetClassDevsW (SETUPAPI.@)
 */
HDEVINFO WINAPI SetupDiGetClassDevsW(
       CONST GUID *class,
       LPCWSTR enumstr,
       HWND parent,
       DWORD flags)
{
    return SetupDiGetClassDevsExW(class, enumstr, parent,
                                  flags, NULL, NULL, NULL);
}

/***********************************************************************
 *		SetupDiGetClassDevsExA (SETUPAPI.@)
 */
HDEVINFO WINAPI SetupDiGetClassDevsExA(
       CONST GUID *class,
       LPCSTR enumstr,
       HWND parent,
       DWORD flags,
       HDEVINFO deviceset,
       LPCSTR machine,
       PVOID reserved)
{
    HDEVINFO ret;
    LPWSTR enumstrW = NULL;
    LPWSTR machineW = NULL;

    if (enumstr)
    {
        int len = MultiByteToWideChar(CP_ACP, 0, enumstr, -1, NULL, 0);
        enumstrW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!enumstrW)
        {
            ret = (HDEVINFO)INVALID_HANDLE_VALUE;
            goto end;
        }
        MultiByteToWideChar(CP_ACP, 0, enumstr, -1, enumstrW, len);
    }
    if (machine)
    {
        int len = MultiByteToWideChar(CP_ACP, 0, machine, -1, NULL, 0);
        machineW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!machineW)
        {
            ret = (HDEVINFO)INVALID_HANDLE_VALUE;
            goto end;
        }
        MultiByteToWideChar(CP_ACP, 0, machine, -1, machineW, len);
    }
    ret = SetupDiGetClassDevsExW(class, enumstrW, parent, flags, deviceset, machineW, reserved);

end:
    HeapFree(GetProcessHeap(), 0, enumstrW);
    HeapFree(GetProcessHeap(), 0, machineW);
    return ret;
}

#ifdef __WINE__
static HDEVINFO SETUP_CreateSerialDeviceList(void)
{
    static const size_t initialSize = 100;
    size_t size;
    WCHAR buf[initialSize];
    LPWSTR devices;
    HDEVINFO ret;
    BOOL failed = FALSE;

    devices = buf;
    size = initialSize;
    do {
        if (QueryDosDeviceW(NULL, devices, size) == 0)
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                size *= 2;
                if (devices != buf)
                    HeapFree(GetProcessHeap(), 0, devices);
                devices = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
                if (!devices)
                    failed = TRUE;
                else
                    *devices = 0;
            }
            else
                failed = TRUE;
        }
    } while (!*devices && !failed);
    if (!failed)
    {
        static const WCHAR comW[] = { 'C','O','M',0 };
        LPWSTR ptr;
        UINT numSerialPorts = 0;
        SerialPortList *list;

        for (ptr = devices; *ptr; ptr += strlenW(ptr) + 1)
        {
            if (!strncmpW(comW, ptr, sizeof(comW) / sizeof(comW[0]) - 1))
                numSerialPorts++;
        }
        list = HeapAlloc(GetProcessHeap(), 0, sizeof(SerialPortList) +
         numSerialPorts ? (numSerialPorts - 1) * sizeof(SerialPortName) : 0);
        if (list)
        {
            list->magic = SETUP_SERIAL_PORT_MAGIC;
            list->numPorts = 0;
            for (ptr = devices; *ptr; ptr += strlenW(ptr) + 1)
            {
                if (!strncmpW(comW, ptr, sizeof(comW) / sizeof(comW[0]) - 1))
                {
                    lstrcpynW(list->names[list->numPorts].name, ptr,
                     sizeof(list->names[list->numPorts].name) /
                     sizeof(list->names[list->numPorts].name[0]));
                    TRACE("Adding %s to list\n",
                     debugstr_w(list->names[list->numPorts].name));
                    list->numPorts++;
                }
            }
            TRACE("list->numPorts is %d\n", list->numPorts);
        }
        ret = (HDEVINFO)list;
    }
    else
        ret = (HDEVINFO)INVALID_HANDLE_VALUE;
    if (devices != buf)
        HeapFree(GetProcessHeap(), 0, devices);
    TRACE("returning %p\n", ret);
    return ret;
}
#endif /* __WINE__ */

static LONG SETUP_CreateDevListFromClass(
       DeviceInfoList *list,
       PCWSTR MachineName,
       LPGUID class,
       PCWSTR Enumerator)
{
    HKEY KeyClass;
    LONG rc;
    DWORD subKeys = 0, maxSubKey;
    DeviceInfo* deviceInfo;
    DWORD i;

    KeyClass = SetupDiOpenClassRegKeyExW(class, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, DIOCR_INSTALLER, MachineName, NULL);
    if (KeyClass == INVALID_HANDLE_VALUE)
        return GetLastError();

    if (Enumerator)
        FIXME("Enumerator parameter ignored\n");

    rc = RegQueryInfoKeyW(
        KeyClass,
        NULL, NULL,
        NULL,
        &subKeys,
        &maxSubKey,
        NULL, NULL, NULL, NULL, NULL, NULL);
    if (rc != ERROR_SUCCESS)
    {
        RegCloseKey(KeyClass);
        return rc;
    }

    for (i = 0; i < subKeys; i++)
    {
        deviceInfo = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(DeviceInfo, Device.RegistryKey) + maxSubKey * sizeof(WCHAR));
        if (!deviceInfo)
        {
           RegCloseKey(KeyClass);
           return ERROR_NO_SYSTEM_RESOURCES;
        }
        rc = RegEnumKeyW(KeyClass, i, &deviceInfo->Device.RegistryKey[0], maxSubKey);
        if (rc == ERROR_NO_MORE_ITEMS)
            break;
        if (rc != ERROR_SUCCESS)
        {
           RegCloseKey(KeyClass);
           return rc;
        }
        deviceInfo->IsDevice = TRUE;
        memcpy(&deviceInfo->Device.ClassGuid, class, sizeof(GUID));
        InsertTailList(&list->ListHead, &deviceInfo->ItemEntry);
        list->numberOfEntries++;
    }

    RegCloseKey(KeyClass);
    return ERROR_SUCCESS;
}

static LONG SETUP_CreateInterfaceList(
       DeviceInfoList *list,
       PCWSTR MachineName,
       LPGUID InterfaceGuid,
       PCWSTR DeviceInstance /* OPTIONAL */)
{
    HKEY hInterfaceKey, hEnumKey, hKey;
    LONG rc;
    WCHAR KeyBuffer[max(MAX_PATH, MAX_GUID_STRING_LEN) + 1];
    DWORD i;
    DWORD dwLength, dwRegType;
    DeviceInfo *deviceInfo;

    /* Open registry key related to this interface */
    hInterfaceKey = SetupDiOpenClassRegKeyExW(InterfaceGuid, KEY_ENUMERATE_SUB_KEYS, DIOCR_INTERFACE, MachineName, NULL);
    if (hInterfaceKey == INVALID_HANDLE_VALUE)
        return GetLastError();

    /* Enumerate sub keys */
    i = 0;
    while (TRUE)
    {
        dwLength = sizeof(KeyBuffer) / sizeof(KeyBuffer[0]);
        rc = RegEnumKeyExW(hInterfaceKey, i, KeyBuffer, &dwLength, NULL, NULL, NULL, NULL);
        if (rc == ERROR_NO_MORE_ITEMS)
            break;
        if (rc != ERROR_SUCCESS)
        {
            RegCloseKey(hInterfaceKey);
            return rc;
        }
        i++;

        /* Open sub key */
        rc = RegOpenKeyEx(hInterfaceKey, KeyBuffer, 0, KEY_QUERY_VALUE, &hKey);
        if (rc != ERROR_SUCCESS)
        {
            RegCloseKey(hInterfaceKey);
            return rc;
        }

        /* Read DeviceInstance value */
        rc = RegQueryValueExW(hKey, L"DeviceInstance", NULL, &dwRegType, NULL, &dwLength);
        if (rc != ERROR_SUCCESS )
        {
            RegCloseKey(hKey);
            RegCloseKey(hInterfaceKey);
            return rc;
        }
        if (dwRegType != REG_SZ)
        {
            RegCloseKey(hKey);
            RegCloseKey(hInterfaceKey);
            return ERROR_GEN_FAILURE;
        }

        /* Allocate memory for list entry and read DeviceInstance */
        deviceInfo = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(DeviceInfo, Interface.DeviceInstance) + dwLength + sizeof(UNICODE_NULL));
        if (!deviceInfo)
        {
            RegCloseKey(hKey);
            RegCloseKey(hInterfaceKey);
            return ERROR_NO_SYSTEM_RESOURCES;
        }
        rc = RegQueryValueExW(hKey, L"DeviceInstance", NULL, NULL, (LPBYTE)&deviceInfo->Interface.DeviceInstance[0], &dwLength);
        if (rc != ERROR_SUCCESS)
        {
            HeapFree(GetProcessHeap(), 0, deviceInfo);
            RegCloseKey(hKey);
            RegCloseKey(hInterfaceKey);
            return rc;
        }
        deviceInfo->Interface.DeviceInstance[dwLength / sizeof(WCHAR)] = UNICODE_NULL;
        TRACE("DeviceInstance %S\n", deviceInfo->Interface.DeviceInstance);
        RegCloseKey(hKey);

        if (DeviceInstance)
        {
            /* Check if device enumerator is not the right one */
            if (wcscmp(DeviceInstance, deviceInfo->Interface.DeviceInstance) != 0)
            {
                HeapFree(GetProcessHeap(), 0, deviceInfo);;
                continue;
            }
        }

        /* Find class GUID associated to the device instance */
        rc = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Enum",
            0, /* Options */
            KEY_ENUMERATE_SUB_KEYS,
            &hEnumKey);
        if (rc != ERROR_SUCCESS)
        {
            HeapFree(GetProcessHeap(), 0, deviceInfo);
            RegCloseKey(hInterfaceKey);
            return rc;
        }
        rc = RegOpenKeyEx(
            hEnumKey,
            deviceInfo->Interface.DeviceInstance,
            0, /* Options */
            KEY_QUERY_VALUE,
            &hKey);
        RegCloseKey(hEnumKey);
        if (rc != ERROR_SUCCESS)
        {
            HeapFree(GetProcessHeap(), 0, deviceInfo);
            RegCloseKey(hInterfaceKey);
            return rc;
        }
        dwLength = sizeof(KeyBuffer) - sizeof(UNICODE_NULL);
        rc = RegQueryValueExW(hKey, L"ClassGUID", NULL, NULL, (LPBYTE)KeyBuffer, &dwLength);
        if (rc != ERROR_SUCCESS)
        {
            HeapFree(GetProcessHeap(), 0, deviceInfo);
            RegCloseKey(hKey);
            RegCloseKey(hInterfaceKey);
            return rc;
        }
        KeyBuffer[dwLength / sizeof(WCHAR)] = UNICODE_NULL;
        RegCloseKey(hKey);
        KeyBuffer[37] = UNICODE_NULL; /* Replace the } by a NULL character */
        if (UuidFromStringW(&KeyBuffer[1], &deviceInfo->Interface.ClassGuid) != RPC_S_OK)
        {
            HeapFree(GetProcessHeap(), 0, deviceInfo);
            return ERROR_GEN_FAILURE;
        }
        TRACE("ClassGUID %S\n", KeyBuffer);

        /* If current device matches the list GUID (if any), append the entry to the list */
        if (IsEqualIID(&list->ClassGuid, &GUID_NULL) || IsEqualIID(&list->ClassGuid, &deviceInfo->Interface.ClassGuid))
        {
            TRACE("Entry found\n");
            deviceInfo->IsDevice = FALSE;
            memcpy(
                &deviceInfo->Interface.InterfaceGuid,
                InterfaceGuid,
                sizeof(deviceInfo->Interface.InterfaceGuid));
            InsertTailList(&list->ListHead, &deviceInfo->ItemEntry);
            list->numberOfEntries++;
        }
        else
        {
            HeapFree(GetProcessHeap(), 0, deviceInfo);
        }
    }
    RegCloseKey(hInterfaceKey);
    return ERROR_SUCCESS;
}

/***********************************************************************
 *		SetupDiGetClassDevsExW (SETUPAPI.@)
 */
HDEVINFO WINAPI SetupDiGetClassDevsExW(
       CONST GUID *class,
       LPCWSTR enumstr,
       HWND parent,
       DWORD flags,
       HDEVINFO deviceset,
       LPCWSTR machine,
       PVOID reserved)
{
    HDEVINFO hDeviceInfo = INVALID_HANDLE_VALUE;
    DeviceInfoList *list;
    LPGUID pClassGuid;
    LPGUID ClassGuidList;
    DWORD RequiredSize;
    LONG i;
    LONG rc;

    TRACE("%s %s %p 0x%08lx %p %s %p\n", debugstr_guid(class), debugstr_w(enumstr),
     parent, flags, deviceset, debugstr_w(machine), reserved);

    /* Create the deviceset if not set */
    if (deviceset)
    {
        list = (DeviceInfoList *)deviceset;
        if (list->magic != SETUP_DEV_INFO_LIST_MAGIC)
        {
            SetLastError(ERROR_INVALID_HANDLE);
            return INVALID_HANDLE_VALUE;
        }
        hDeviceInfo = deviceset;
    }
    else
    {
         hDeviceInfo = SetupDiCreateDeviceInfoListExW(
             flags & DIGCF_DEVICEINTERFACE ? NULL : class,
             NULL, machine, NULL);
         if (hDeviceInfo == INVALID_HANDLE_VALUE)
             return INVALID_HANDLE_VALUE;
         list = (DeviceInfoList *)hDeviceInfo;
    }

    if (IsEqualIID(&list->ClassGuid, &GUID_NULL))
        pClassGuid = NULL;
    else
        pClassGuid = &list->ClassGuid;

    if (flags & DIGCF_PRESENT)
        FIXME(": flag DIGCF_PRESENT ignored\n");
    if (flags & DIGCF_PROFILE)
        FIXME(": flag DIGCF_PROFILE ignored\n");

    if (flags & DIGCF_ALLCLASSES)
    {
        /* Get list of device classes */
        SetupDiBuildClassInfoList(0, NULL, 0, &RequiredSize);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            if (!deviceset)
                SetupDiDestroyDeviceInfoList(hDeviceInfo);
            return INVALID_HANDLE_VALUE;
        }
        ClassGuidList = HeapAlloc(GetProcessHeap(), 0, RequiredSize * sizeof(GUID));
        if (!ClassGuidList)
        {
            if (!deviceset)
                SetupDiDestroyDeviceInfoList(hDeviceInfo);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return INVALID_HANDLE_VALUE;
        }
        if (!SetupDiBuildClassInfoListExW(0, ClassGuidList, RequiredSize, &RequiredSize, machine, NULL))
        {
            HeapFree(GetProcessHeap(), 0, ClassGuidList);
            if (!deviceset)
                SetupDiDestroyDeviceInfoList(hDeviceInfo);
            return INVALID_HANDLE_VALUE;
        }

        /* Enumerate devices in each device class */
        for (i = 0; i < RequiredSize; i++)
        {
            if (pClassGuid == NULL || IsEqualIID(pClassGuid, &ClassGuidList[i]))
            {
                rc = SETUP_CreateDevListFromClass(list, machine, &ClassGuidList[i], enumstr);
                if (rc != ERROR_SUCCESS)
                {
                    HeapFree(GetProcessHeap(), 0, ClassGuidList);
                    SetLastError(rc);
                    if (!deviceset)
                        SetupDiDestroyDeviceInfoList(hDeviceInfo);
                    return INVALID_HANDLE_VALUE;
                }
            }
        }
        HeapFree(GetProcessHeap(), 0, ClassGuidList);
        return hDeviceInfo;
    }
    else if (flags & DIGCF_DEVICEINTERFACE)
    {
        if (class == NULL)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            if (!deviceset)
                SetupDiDestroyDeviceInfoList(hDeviceInfo);
            return INVALID_HANDLE_VALUE;
        }

#ifdef __WINE__
        /* Special case: find serial ports by calling QueryDosDevice */
        if (IsEqualIID(class, &GUID_DEVINTERFACE_SERENUM_BUS_ENUMERATOR)
        {
            rc = SETUP_CreateSerialDeviceList();
            if (rc != ERROR_SUCCESS)
            {
                SetLastError(rc);
                if (!deviceset)
                    SetupDiDestroyDeviceInfoList(hDeviceInfo);
                return INVALID_HANDLE_VALUE;
            }
            return hDeviceInfo;
        }
#endif

        rc = SETUP_CreateInterfaceList(list, machine, (LPGUID)class, enumstr);
        if (rc != ERROR_SUCCESS)
        {
            SetLastError(rc);
            if (!deviceset)
                SetupDiDestroyDeviceInfoList(hDeviceInfo);
            return INVALID_HANDLE_VALUE;
        }
        return hDeviceInfo;
    }
    else
    {
        rc = SETUP_CreateDevListFromClass(list, machine, (LPGUID)class, enumstr);
        if (rc != ERROR_SUCCESS)
        {
            SetLastError(rc);
            if (!deviceset)
                SetupDiDestroyDeviceInfoList(hDeviceInfo);
            return INVALID_HANDLE_VALUE;
        }
        return hDeviceInfo;
    }
}

/***********************************************************************
 *		SetupDiEnumDeviceInterfaces (SETUPAPI.@)
 */
BOOL WINAPI SetupDiEnumDeviceInterfaces(
       HDEVINFO DeviceInfoSet,
       PSP_DEVINFO_DATA DeviceInfoData,
       CONST GUID * InterfaceClassGuid,
       DWORD MemberIndex,
       PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData)
{
    BOOL ret = FALSE;

    TRACE("%p, %p, %s, 0x%08lx, %p\n", DeviceInfoSet, DeviceInfoData,
     debugstr_guid(InterfaceClassGuid), MemberIndex, DeviceInterfaceData);
    if (!DeviceInterfaceData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoData)
        FIXME(": unimplemented with PSP_DEVINFO_DATA set\n");
    else if (DeviceInfoSet && DeviceInfoSet != (HDEVINFO)INVALID_HANDLE_VALUE)
    {
        DeviceInfoList *list = (DeviceInfoList *)DeviceInfoSet;

        if (list->magic == SETUP_DEV_INFO_LIST_MAGIC)
        {
            if (MemberIndex >= list->numberOfEntries)
                SetLastError(ERROR_NO_MORE_ITEMS);
            else
            {
                PLIST_ENTRY ItemList = list->ListHead.Flink;
                DeviceInfo* DevInfo;
                while (MemberIndex-- > 0)
                    ItemList = ItemList->Flink;
                DevInfo = (DeviceInfo *)ItemList;

                if (!DevInfo->IsDevice)
                {
                    DeviceInterfaceData->cbSize = sizeof(SP_DEVICE_INTERFACE_DATA); /* FIXME: verify instead of setting */
#ifdef __WINE__
                    /* FIXME: this assumes the only possible enumeration is of serial
                     * ports.
                     */
                    memcpy(&DeviceInterfaceData->InterfaceClassGuid,
                        DevInfo->InterfaceGuid,
                        sizeof(DeviceInterfaceData->InterfaceClassGuid));
                    DeviceInterfaceData->Flags = 0;
                    /* Note: this appears to be dangerous, passing a private
                     * pointer a heap-allocated datum to the caller.  However, the
                     * expected lifetime of the device data is the same as the
                     * HDEVINFO; once that is closed, the data are no longer valid.
                     */
                    DeviceInterfaceData->Reserved = (ULONG_PTR)DevInfo->InterfaceName;
#else
                    FIXME("unimplemented\n");
#endif
                    ret = TRUE;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PARAMETER);
                }
            }
        }
        else
            SetLastError(ERROR_INVALID_HANDLE);
    }
    else
        SetLastError(ERROR_INVALID_HANDLE);
    return ret;
}

/***********************************************************************
 *		SetupDiDestroyDeviceInfoList (SETUPAPI.@)
 */
BOOL WINAPI SetupDiDestroyDeviceInfoList(HDEVINFO devinfo)
{
    BOOL ret = FALSE;

    TRACE("%p\n", devinfo);
    if (devinfo && devinfo != (HDEVINFO)INVALID_HANDLE_VALUE)
    {
        DeviceInfoList *list = (DeviceInfoList *)devinfo;

        if (list->magic == SETUP_DEV_INFO_LIST_MAGIC)
        {
            PLIST_ENTRY ListEntry;
            while (!IsListEmpty(&list->ListHead))
            {
                ListEntry = RemoveHeadList(&list->ListHead);
                HeapFree(GetProcessHeap(), 0, ListEntry);
            }
            RegCloseKey(list->HKLM);
            HeapFree(GetProcessHeap(), 0, list);
            ret = TRUE;
        }
        else
            SetLastError(ERROR_INVALID_HANDLE);
    }
    else
        SetLastError(ERROR_INVALID_HANDLE);
    return ret;
}

/***********************************************************************
 *		SetupDiGetDeviceInterfaceDetailA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInterfaceDetailA(
      HDEVINFO DeviceInfoSet,
      PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
      PSP_DEVICE_INTERFACE_DETAIL_DATA_A DeviceInterfaceDetailData,
      DWORD DeviceInterfaceDetailDataSize,
      PDWORD RequiredSize,
      PSP_DEVINFO_DATA DeviceInfoData)
{
    BOOL ret = FALSE;

    TRACE("(%p, %p, %p, %ld, %p, %p)\n", DeviceInfoSet,
     DeviceInterfaceData, DeviceInterfaceDetailData,
     DeviceInterfaceDetailDataSize, RequiredSize, DeviceInfoData);
    if (!DeviceInterfaceData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if ((DeviceInterfaceDetailDataSize && !DeviceInterfaceDetailData) ||
     (DeviceInterfaceDetailData && !DeviceInterfaceDetailDataSize))
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoSet && DeviceInfoSet != (HDEVINFO)INVALID_HANDLE_VALUE)
    {
        DeviceInfoList *list = (DeviceInfoList *)DeviceInfoSet;

        if (list->magic == SETUP_DEV_INFO_LIST_MAGIC)
        {
#ifdef __WINE__
            /* FIXME: this assumes the only possible enumeration is of serial
             * ports.
             */
            LPCWSTR devName = (LPCWSTR)DeviceInterfaceData->Reserved;
            DWORD sizeRequired = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A) +
             lstrlenW(devName);

            if (sizeRequired > DeviceInterfaceDetailDataSize)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                if (RequiredSize)
                    *RequiredSize = sizeRequired;
            }
            else
            {
                LPSTR dst = DeviceInterfaceDetailData->DevicePath;
                LPCWSTR src = devName;

                /* MSDN claims cbSize must be set by the caller, but it lies */
                DeviceInterfaceDetailData->cbSize =
                 sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
                for ( ; *src; src++, dst++)
                    *dst = *src;
                *dst = '\0';
                TRACE("DevicePath is %s\n",
                 debugstr_a(DeviceInterfaceDetailData->DevicePath));
                if (DeviceInfoData)
                {
                    DeviceInfoData->cbSize = sizeof(SP_DEVINFO_DATA);
                    memcpy(&DeviceInfoData->ClassGuid,
                     &GUID_DEVINTERFACE_SERENUM_BUS_ENUMERATOR,
                     sizeof(DeviceInfoData->ClassGuid));
                    DeviceInfoData->DevInst = 0;
                    DeviceInfoData->Reserved = (ULONG_PTR)devName;
                }
                ret = TRUE;
            }
#else /* __WINE__ */
            FIXME("unimplemented\n");
            ret = FALSE;
#endif /* __WINE__ */
        }
        else
            SetLastError(ERROR_INVALID_HANDLE);
    }
    else
        SetLastError(ERROR_INVALID_HANDLE);
    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiGetDeviceInterfaceDetailW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInterfaceDetailW(
      HDEVINFO DeviceInfoSet,
      PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
      PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData,
      DWORD DeviceInterfaceDetailDataSize,
      PDWORD RequiredSize,
      PSP_DEVINFO_DATA DeviceInfoData)
{
    FIXME("(%p, %p, %p, %ld, %p, %p): stub\n", DeviceInfoSet,
     DeviceInterfaceData, DeviceInterfaceDetailData,
     DeviceInterfaceDetailDataSize, RequiredSize, DeviceInfoData);
    return FALSE;
}

/***********************************************************************
 *		SetupDiGetDeviceRegistryPropertyA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceRegistryPropertyA(
        HDEVINFO  devinfo,
        PSP_DEVINFO_DATA  DeviceInfoData,
        DWORD   Property,
        PDWORD  PropertyRegDataType,
        PBYTE   PropertyBuffer,
        DWORD   PropertyBufferSize,
        PDWORD  RequiredSize)
{
    BOOL bResult;
    BOOL bIsStringProperty;
    DWORD RegType;
    DWORD RequiredSizeA, RequiredSizeW;
    DWORD PropertyBufferSizeW;
    PBYTE PropertyBufferW;

    TRACE("%04lx %p %ld %p %p %ld %p\n", (DWORD)devinfo, DeviceInfoData,
        Property, PropertyRegDataType, PropertyBuffer, PropertyBufferSize,
        RequiredSize);

    PropertyBufferSizeW = PropertyBufferSize * 2;
    PropertyBufferW = HeapAlloc(GetProcessHeap(), 0, PropertyBufferSizeW);

    bResult = SetupDiGetDeviceRegistryPropertyW(
        devinfo,
        DeviceInfoData,
        Property,
        &RegType,
        PropertyBufferW,
        PropertyBufferSizeW,
        &RequiredSizeW);

    HeapFree(GetProcessHeap(), 0, PropertyBufferW);

    if (!bResult)
        return bResult;

    bIsStringProperty = (RegType == REG_SZ || RegType == REG_MULTI_SZ);

    if (bIsStringProperty)
        RequiredSizeA = RequiredSizeW / sizeof(WCHAR);
    else
        RequiredSizeA = RequiredSizeW;

    if (RequiredSizeA <= PropertyBufferSize)
    {
        if (bIsStringProperty && PropertyBufferSize > 0)
        {
            if (WideCharToMultiByte(CP_ACP, 0, (LPWSTR)PropertyBufferW, RequiredSizeW / sizeof(WCHAR), PropertyBuffer, PropertyBufferSize, NULL, NULL) == 0)
            {
                /* Last error is already set by WideCharToMultiByte */
                bResult = FALSE;
            }
        }
        else
            memcpy(PropertyBuffer, PropertyBufferW, RequiredSizeA);
    }
    else
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        bResult = FALSE;
    }

    if (PropertyRegDataType)
        *PropertyRegDataType = RegType;
    if (RequiredSize)
        *RequiredSize = RequiredSizeA;
    return bResult;
}

/***********************************************************************
 *		SetupDiGetDeviceRegistryPropertyW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceRegistryPropertyW(
        HDEVINFO  devinfo,
        PSP_DEVINFO_DATA  DeviceInfoData,
        DWORD   Property,
        PDWORD  PropertyRegDataType,
        PBYTE   PropertyBuffer,
        DWORD   PropertyBufferSize,
        PDWORD  RequiredSize)
{
    FIXME("%04lx %p %ld %p %p %ld %p\n", (DWORD)devinfo, DeviceInfoData,
        Property, PropertyRegDataType, PropertyBuffer, PropertyBufferSize,
        RequiredSize);
    SetLastError(ERROR_GEN_FAILURE);
    return FALSE;
}


/***********************************************************************
 *		SetupDiInstallClassA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiInstallClassA(
        HWND hwndParent,
        PCSTR InfFileName,
        DWORD Flags,
        HSPFILEQ FileQueue)
{
    UNICODE_STRING FileNameW;
    BOOL Result;

    if (!RtlCreateUnicodeStringFromAsciiz(&FileNameW, InfFileName))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    Result = SetupDiInstallClassW(hwndParent, FileNameW.Buffer, Flags, FileQueue);

    RtlFreeUnicodeString(&FileNameW);

    return Result;
}

static HKEY CreateClassKey(HINF hInf)
{
    WCHAR FullBuffer[MAX_PATH];
    WCHAR Buffer[MAX_PATH];
    DWORD RequiredSize;
    HKEY hClassKey;

    Buffer[0] = '\\';
    if (!SetupGetLineTextW(NULL,
			   hInf,
			   Version,
			   ClassGUID,
			   &Buffer[1],
			   MAX_PATH - 1,
			   &RequiredSize))
    {
        return INVALID_HANDLE_VALUE;
    }

    lstrcpyW(FullBuffer, ControlClass);
    lstrcatW(FullBuffer, Buffer);


    if (!SetupGetLineTextW(NULL,
			       hInf,
			       Version,
			       Class,
			       Buffer,
			       MAX_PATH,
			       &RequiredSize))
    {
        RegDeleteKeyW(HKEY_LOCAL_MACHINE, FullBuffer);
        return INVALID_HANDLE_VALUE;
    }

    if (ERROR_SUCCESS != RegCreateKeyExW(HKEY_LOCAL_MACHINE,
			    FullBuffer,
			    0,
			    NULL,
			    REG_OPTION_NON_VOLATILE,
			    KEY_ALL_ACCESS,
			    NULL,
			    &hClassKey,
             NULL))
    {
        RegDeleteKeyW(HKEY_LOCAL_MACHINE, FullBuffer);
        return INVALID_HANDLE_VALUE;
    }

    if (ERROR_SUCCESS != RegSetValueExW(hClassKey,
		       Class,
		       0,
		       REG_SZ,
		       (LPBYTE)Buffer,
             RequiredSize * sizeof(WCHAR)))
    {
        RegCloseKey(hClassKey);
        RegDeleteKeyW(HKEY_LOCAL_MACHINE, FullBuffer);
        return INVALID_HANDLE_VALUE;
    }

    return hClassKey;
}

/***********************************************************************
 *		SetupDiInstallClassW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiInstallClassW(
        HWND hwndParent,
        PCWSTR InfFileName,
        DWORD Flags,
        HSPFILEQ FileQueue)
{
    WCHAR SectionName[MAX_PATH];
    DWORD SectionNameLength = 0;
    HINF hInf;
    BOOL bFileQueueCreated = FALSE;
    HKEY hClassKey;

    FIXME("not fully implemented\n");

    if ((Flags & DI_NOVCP) && (FileQueue == NULL || FileQueue == INVALID_HANDLE_VALUE))
    {
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }

    /* Open the .inf file */
    hInf = SetupOpenInfFileW(InfFileName,
			     NULL,
			     INF_STYLE_WIN4,
			     NULL);
    if (hInf == INVALID_HANDLE_VALUE)
    {

	return FALSE;
    }

    /* Create or open the class registry key 'HKLM\\CurrentControlSet\\Class\\{GUID}' */
    hClassKey = CreateClassKey(hInf);
    if (hClassKey == INVALID_HANDLE_VALUE)
    {
	SetupCloseInfFile(hInf);
	return FALSE;
    }



    /* Try to append a layout file */
#if 0
    SetupOpenAppendInfFileW(NULL, hInf, NULL);
#endif

    /* Retrieve the actual section name */
    SetupDiGetActualSectionToInstallW(hInf,
				      ClassInstall32,
				      SectionName,
				      MAX_PATH,
				      &SectionNameLength,
				      NULL);

#if 0
    if (!(Flags & DI_NOVCP))
    {
	FileQueue = SetupOpenFileQueue();
	if (FileQueue == INVALID_HANDLE_VALUE)
	{
	    SetupCloseInfFile(hInf);
       RegCloseKey(hClassKey);
	    return FALSE;
	}

	bFileQueueCreated = TRUE;

    }
#endif

    SetupInstallFromInfSectionW(NULL,
				hInf,
				SectionName,
				SPINST_REGISTRY,
				hClassKey,
				NULL,
				0,
				NULL,
				NULL,
				INVALID_HANDLE_VALUE,
				NULL);

    /* FIXME: More code! */

    if (bFileQueueCreated)
	SetupCloseFileQueue(FileQueue);

    SetupCloseInfFile(hInf);

    RegCloseKey(hClassKey);
    return TRUE;
}


/***********************************************************************
 *		SetupDiOpenClassRegKey  (SETUPAPI.@)
 */
HKEY WINAPI SetupDiOpenClassRegKey(
        const GUID* ClassGuid,
        REGSAM samDesired)
{
    return SetupDiOpenClassRegKeyExW(ClassGuid, samDesired,
                                     DIOCR_INSTALLER, NULL, NULL);
}


/***********************************************************************
 *		SetupDiOpenClassRegKeyExA  (SETUPAPI.@)
 */
HKEY WINAPI SetupDiOpenClassRegKeyExA(
        const GUID* ClassGuid,
        REGSAM samDesired,
        DWORD Flags,
        PCSTR MachineName,
        PVOID Reserved)
{
    PWSTR MachineNameW = NULL;
    HKEY hKey;

    TRACE("\n");

    if (MachineName)
    {
        MachineNameW = MultiByteToUnicode(MachineName, CP_ACP);
        if (MachineNameW == NULL)
            return INVALID_HANDLE_VALUE;
    }

    hKey = SetupDiOpenClassRegKeyExW(ClassGuid, samDesired,
                                     Flags, MachineNameW, Reserved);

    if (MachineNameW)
        MyFree(MachineNameW);

    return hKey;
}


/***********************************************************************
 *		SetupDiOpenClassRegKeyExW  (SETUPAPI.@)
 */
HKEY WINAPI SetupDiOpenClassRegKeyExW(
        const GUID* ClassGuid,
        REGSAM samDesired,
        DWORD Flags,
        PCWSTR MachineName,
        PVOID Reserved)
{
    LPWSTR lpGuidString;
    LPWSTR lpFullGuidString;
    DWORD dwLength;
    HKEY HKLM;
    HKEY hClassesKey;
    HKEY hClassKey;
    DWORD rc;
    LPCWSTR lpKeyName;

    if (Flags == DIOCR_INSTALLER)
    {
        lpKeyName = ControlClass;
    }
    else if (Flags == DIOCR_INTERFACE)
    {
        lpKeyName = DeviceClasses;
    }
    else
    {
        ERR("Invalid Flags parameter!\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    if (MachineName != NULL)
    {
        rc = RegConnectRegistryW(MachineName, HKEY_LOCAL_MACHINE, &HKLM);
        if (rc != ERROR_SUCCESS)
        {
            SetLastError(rc);
            return INVALID_HANDLE_VALUE;
        }
    }
    else
        HKLM = HKEY_LOCAL_MACHINE;

    rc = RegOpenKeyExW(HKLM,
		      lpKeyName,
		      0,
		      KEY_ALL_ACCESS,
		      &hClassesKey);
    if (MachineName != NULL) RegCloseKey(HKLM);
    if (rc != ERROR_SUCCESS)
    {
	SetLastError(rc);
	return INVALID_HANDLE_VALUE;
    }

    if (ClassGuid == NULL)
        return hClassesKey;

    if (UuidToStringW((UUID*)ClassGuid, &lpGuidString) != RPC_S_OK)
    {
	SetLastError(ERROR_GEN_FAILURE);
	RegCloseKey(hClassesKey);
	return INVALID_HANDLE_VALUE;
    }

    dwLength = lstrlenW(lpGuidString);
    lpFullGuidString = HeapAlloc(GetProcessHeap(), 0, (dwLength + 3) * sizeof(WCHAR));
    if (!lpFullGuidString)
    {
        SetLastError(ERROR_NO_SYSTEM_RESOURCES);
        RpcStringFreeW(&lpGuidString);
        return INVALID_HANDLE_VALUE;
    }
    lpFullGuidString[0] = '{';
    memcpy(&lpFullGuidString[1], lpGuidString, dwLength * sizeof(WCHAR));
    lpFullGuidString[dwLength + 1] = '}';
    lpFullGuidString[dwLength + 2] = UNICODE_NULL;
    RpcStringFreeW(&lpGuidString);

    rc = RegOpenKeyExW(hClassesKey,
		      lpFullGuidString,
		      0,
		      KEY_ALL_ACCESS,
		      &hClassKey);
    if (rc != ERROR_SUCCESS)
    {
	SetLastError(rc);
	HeapFree(GetProcessHeap(), 0, lpFullGuidString);
	RegCloseKey(hClassesKey);
	return INVALID_HANDLE_VALUE;
    }

    HeapFree(GetProcessHeap(), 0, lpFullGuidString);
    RegCloseKey(hClassesKey);

    return hClassKey;
}

/***********************************************************************
 *		SetupDiOpenDeviceInterfaceW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiOpenDeviceInterfaceW(
       HDEVINFO DeviceInfoSet,
       PCWSTR DevicePath,
       DWORD OpenFlags,
       PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData)
{
    FIXME("%p %s %08lx %p\n",
        DeviceInfoSet, debugstr_w(DevicePath), OpenFlags, DeviceInterfaceData);
    return FALSE;
}

/***********************************************************************
 *		SetupDiOpenDeviceInterfaceA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiOpenDeviceInterfaceA(
       HDEVINFO DeviceInfoSet,
       PCSTR DevicePath,
       DWORD OpenFlags,
       PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData)
{
    FIXME("%p %s %08lx %p\n", DeviceInfoSet,
        debugstr_a(DevicePath), OpenFlags, DeviceInterfaceData);
    return FALSE;
}

/***********************************************************************
 *		SetupDiSetClassInstallParamsA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiSetClassInstallParamsA(
       HDEVINFO  DeviceInfoSet,
       PSP_DEVINFO_DATA DeviceInfoData,
       PSP_CLASSINSTALL_HEADER ClassInstallParams,
       DWORD ClassInstallParamsSize)
{
    FIXME("%p %p %x %lu\n",DeviceInfoSet, DeviceInfoData,
          ClassInstallParams->InstallFunction, ClassInstallParamsSize);
    return FALSE;
}

/***********************************************************************
 *		SetupDiCallClassInstaller (SETUPAPI.@)
 */
BOOL WINAPI SetupDiCallClassInstaller(
       DWORD InstallFunction,
       HDEVINFO DeviceInfoSet,
       PSP_DEVINFO_DATA DeviceInfoData)
{
    FIXME("%ld %p %p\n", InstallFunction, DeviceInfoSet, DeviceInfoData);
    return FALSE;
}

/***********************************************************************
 *		SetupDiGetDeviceInstallParamsA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInstallParamsA(
       HDEVINFO DeviceInfoSet,
       PSP_DEVINFO_DATA DeviceInfoData,
       PSP_DEVINSTALL_PARAMS_A DeviceInstallParams)
{
    FIXME("%p %p %p\n", DeviceInfoSet, DeviceInfoData, DeviceInstallParams);
    return FALSE;
}

/***********************************************************************
 *		SetupDiOpenDevRegKey (SETUPAPI.@)
 */
HKEY WINAPI SetupDiOpenDevRegKey(
       HDEVINFO DeviceInfoSet,
       PSP_DEVINFO_DATA DeviceInfoData,
       DWORD Scope,
       DWORD HwProfile,
       DWORD KeyType,
       REGSAM samDesired)
{
    FIXME("%p %p %ld %ld %ld %lx\n", DeviceInfoSet, DeviceInfoData,
          Scope, HwProfile, KeyType, samDesired);
    return INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *		SetupDiCreateDeviceInfoA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiCreateDeviceInfoA(
       HDEVINFO DeviceInfoSet,
       PCSTR DeviceName,
       LPGUID ClassGuid,
       PCSTR DeviceDescription,
       HWND hwndParent,
       DWORD CreationFlags,
       PSP_DEVINFO_DATA DeviceInfoData)
{
    LPWSTR DeviceNameW = NULL;
    LPWSTR DeviceDescriptionW = NULL;
    BOOL bResult;

    TRACE("\n");

    if (DeviceName)
    {
        DeviceNameW = MultiByteToUnicode(DeviceName, CP_ACP);
        if (DeviceNameW == NULL) return FALSE;
    }
    if (DeviceDescription)
    {
        DeviceDescriptionW = MultiByteToUnicode(DeviceDescription, CP_ACP);
        if (DeviceDescriptionW == NULL)
        {
            if (DeviceNameW) MyFree(DeviceNameW);
            return FALSE;
        }
    }

    bResult = SetupDiCreateDeviceInfoW(DeviceInfoSet, DeviceNameW,
                                       ClassGuid, DeviceDescriptionW,
                                       hwndParent, CreationFlags,
                                       DeviceInfoData);

    if (DeviceNameW) MyFree(DeviceNameW);
    if (DeviceDescriptionW) MyFree(DeviceDescriptionW);

    return bResult;
}

/***********************************************************************
 *		SetupDiCreateDeviceInfoW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiCreateDeviceInfoW(
       HDEVINFO DeviceInfoSet,
       PCWSTR DeviceName,
       LPGUID ClassGuid,
       PCWSTR DeviceDescription,
       HWND hwndParent,
       DWORD CreationFlags,
       PSP_DEVINFO_DATA DeviceInfoData)
{
    FIXME("%p %S %p %S %p %lx %p\n", DeviceInfoSet, debugstr_w(DeviceName),
          ClassGuid, debugstr_w(DeviceDescription), hwndParent,
          CreationFlags, DeviceInfoData);
    return FALSE;
}
