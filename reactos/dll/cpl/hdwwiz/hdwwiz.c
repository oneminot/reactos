/*
 * ReactOS New devices installation
 * Copyright (C) 2005, 2008 ReactOS Team
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
/*
 * PROJECT:         ReactOS Add hardware control panel
 * FILE:            dll/cpl/hdwwiz/hdwwiz.c
 * PURPOSE:         ReactOS Add hardware control panel
 * PROGRAMMER:      Herv� Poussineau (hpoussin@reactos.org)
 *                  Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "resource.h"
#include "hdwwiz.h"

/* GLOBALS ******************************************************************/

HINSTANCE hApplet = NULL;
HFONT hTitleFont;
SP_CLASSIMAGELIST_DATA ImageListData;

typedef BOOL (WINAPI *PINSTALL_NEW_DEVICE)(HWND, LPGUID, PDWORD);


/* STATIC FUNCTIONS *********************************************************/

static HFONT
CreateTitleFont(VOID)
{
    NONCLIENTMETRICS ncm;
    LOGFONT LogFont;
    HDC hdc;
    INT FontSize;
    HFONT hFont;

    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

    LogFont = ncm.lfMessageFont;
    LogFont.lfWeight = FW_BOLD;
    _tcscpy(LogFont.lfFaceName, _T("MS Shell Dlg"));

    hdc = GetDC(NULL);
    FontSize = 12;
    LogFont.lfHeight = 0 - GetDeviceCaps (hdc, LOGPIXELSY) * FontSize / 72;
    hFont = CreateFontIndirect(&LogFont);
    ReleaseDC(NULL, hdc);

    return hFont;
}

static INT_PTR CALLBACK
StartPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* Set title font */
            SendDlgItemMessage(hwndDlg, IDC_FINISHTITLE, WM_SETFONT, (WPARAM)hTitleFont, (LPARAM)TRUE);
        }
        break;
    }

    return FALSE;
}

static INT_PTR CALLBACK
SearchPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}

static INT_PTR CALLBACK
IsConnctedPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_COMMAND:
        {
            if(HIWORD(wParam) == BN_CLICKED)
            {
                if ((SendDlgItemMessage(hwndDlg, IDC_CONNECTED, BM_GETCHECK, 0, 0) == BST_CHECKED) ||
                    (SendDlgItemMessage(hwndDlg, IDC_NOTCONNECTED, BM_GETCHECK, 0, 0) == BST_CHECKED))
                {
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
                }
                else
                {
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
                }
            }
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                {
                    if ((SendDlgItemMessage(hwndDlg, IDC_CONNECTED, BM_GETCHECK, 0, 0) == BST_CHECKED) ||
                        (SendDlgItemMessage(hwndDlg, IDC_NOTCONNECTED, BM_GETCHECK, 0, 0) == BST_CHECKED))
                    {
                        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
                    }
                    else
                    {
                        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
                    }
                }
                break;

                case PSN_WIZNEXT:
                {
                    if (SendDlgItemMessage(hwndDlg, IDC_NOTCONNECTED, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_NOTCONNECTEDPAGE);
                    else
                        SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_PROBELISTPAGE);

                    return TRUE;
                }
            }
        }
        break;
    }

    return FALSE;
}

static INT_PTR CALLBACK
FinishPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* Set title font */
            SendDlgItemMessage(hwndDlg, IDC_FINISHTITLE, WM_SETFONT, (WPARAM)hTitleFont, (LPARAM)TRUE);
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                {
                    /* Only "Finish" button */
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_FINISH);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

static INT_PTR CALLBACK
NotConnectedPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* Set title font */
            SendDlgItemMessage(hwndDlg, IDC_FINISHTITLE, WM_SETFONT, (WPARAM)hTitleFont, (LPARAM)TRUE);
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                {
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_FINISH | PSWIZB_BACK);
                }
                break;

                case PSN_WIZBACK:
                {
                    SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_ISCONNECTEDPAGE);
                    return TRUE;
                }
            }
        }
        break;
    }

    return FALSE;
}

static VOID
TrimGuidString(LPWSTR szString, LPWSTR szNewString)
{
    WCHAR szBuffer[39];
    INT Index;

    if (wcslen(szString) == 38)
    {
        if ((szString[0] == L'{') && (szString[37] == L'}'))
        {
            for (Index = 0; Index < wcslen(szString); Index++)
                szBuffer[Index] = szString[Index + 1];

            szBuffer[36] = L'\0';
            wcscpy(szNewString, szBuffer);
            return;
        }
    }
    wcscpy(szNewString, L"\0");
}

static VOID
InitProbeListPage(HWND hwndDlg)
{
    LV_COLUMN Column;
    LV_ITEM Item;
    WCHAR szBuffer[MAX_STR_SIZE], szGuid[MAX_STR_SIZE], szTrimGuid[MAX_STR_SIZE];
    HWND hList = GetDlgItem(hwndDlg, IDC_PROBELIST);
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA DevInfoData;
    ULONG ulStatus, ulProblemNumber;
    GUID ClassGuid;
    RECT Rect;
    DWORD Index;

    if (!hList) return;

    ZeroMemory(&Column, sizeof(LV_COLUMN));

    GetClientRect(hList, &Rect);

    Column.mask         = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    Column.fmt          = LVCFMT_LEFT;
    Column.iSubItem     = 0;
    Column.pszText      = NULL;
    Column.cx           = Rect.right - GetSystemMetrics(SM_CXVSCROLL);
    (VOID) ListView_InsertColumn(hList, 0, &Column);

    ZeroMemory(&Item, sizeof(LV_ITEM));

    LoadString(hApplet, IDS_ADDNEWDEVICE, szBuffer, sizeof(szBuffer) / sizeof(WCHAR));

    Item.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
    Item.pszText    = (LPTSTR) szBuffer;
    Item.iItem      = 0;
    Item.iImage     = -1;
    (VOID) ListView_InsertItem(hList, &Item);

    (VOID) ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT);

    hDevInfo = SetupDiGetClassDevsEx(NULL, NULL, NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT, NULL, NULL, 0);

    if (hDevInfo == INVALID_HANDLE_VALUE) return;

    /* get the device image List */
    ImageListData.cbSize = sizeof(ImageListData);
    SetupDiGetClassImageList(&ImageListData);

    DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (Index = 0; TRUE; Index++)
    {
        if (!SetupDiEnumDeviceInfo(hDevInfo, Index, &DevInfoData)) break;

        if (CM_Get_DevNode_Status_Ex(&ulStatus, &ulProblemNumber, DevInfoData.DevInst, 0, NULL) == CR_SUCCESS)
        {
            if (ulStatus & DN_NO_SHOW_IN_DM) continue;
        }

        /* get the device's friendly name */
        if (!SetupDiGetDeviceRegistryProperty(hDevInfo,
                                              &DevInfoData,
                                              SPDRP_FRIENDLYNAME,
                                              0,
                                              (BYTE*)szBuffer,
                                              MAX_STR_SIZE,
                                              NULL))
        {
            /* if the friendly name fails, try the description instead */
            SetupDiGetDeviceRegistryProperty(hDevInfo,
                                             &DevInfoData,
                                             SPDRP_DEVICEDESC,
                                             0,
                                             (BYTE*)szBuffer,
                                             MAX_STR_SIZE,
                                             NULL);
        }

        SetupDiGetDeviceRegistryProperty(hDevInfo,
                                         &DevInfoData,
                                         SPDRP_CLASSGUID,
                                         0,
                                         (BYTE*)szGuid,
                                         MAX_STR_SIZE,
                                         NULL);

        TrimGuidString(szGuid, szTrimGuid);
        UuidFromStringW(szTrimGuid, &ClassGuid);

        SetupDiGetClassImageIndex(&ImageListData,
                                  &ClassGuid,
                                  &Item.iImage);

        /* Set device name */
        Item.pszText = (LPTSTR) szBuffer;
        Item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
        Item.iItem = (INT) ListView_GetItemCount(hList);
        (VOID) ListView_InsertItem(hList, &Item);

        DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    }

    (VOID) ListView_SetImageList(hList, ImageListData.ImageList, LVSIL_SMALL);
    SetupDiDestroyDeviceInfoList(hDevInfo);
}

static INT_PTR CALLBACK
ProbeListPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            InitProbeListPage(hwndDlg);
        }
        break;

        case WM_COMMAND:
        {

        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                {
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
                }
                break;

                case PSN_WIZNEXT:
                {
                    SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_FINISHPAGE);
                    return TRUE;
                }
            }
        }
        break;
    }

    return FALSE;
}

static VOID
HardwareWizardInit(HWND hwnd)
{
    HPROPSHEETPAGE ahpsp[5];
    PROPSHEETPAGE psp = {0};
    PROPSHEETHEADER psh;
    UINT nPages = 0;

    /* Create the Start page, until setup is working */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
    psp.hInstance = hApplet;
    psp.lParam = 0;
    psp.pfnDlgProc = StartPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_STARTPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create search page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_SEARCHTITLE);
    psp.pszHeaderSubTitle = NULL;
    psp.hInstance = hApplet;
    psp.lParam = 0;
    psp.pfnDlgProc = SearchPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_SEARCHPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create is connected page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_ISCONNECTED);
    psp.pszHeaderSubTitle = NULL;
    psp.hInstance = hApplet;
    psp.lParam = 0;
    psp.pfnDlgProc = IsConnctedPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_ISCONNECTEDPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create probe list page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_PROBELISTTITLE);
    psp.pszHeaderSubTitle = NULL;
    psp.hInstance = hApplet;
    psp.lParam = 0;
    psp.pfnDlgProc = ProbeListPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROBELISTPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create finish page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
    psp.hInstance = hApplet;
    psp.lParam = 0;
    psp.pfnDlgProc = FinishPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_FINISHPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create not connected page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
    psp.hInstance = hApplet;
    psp.lParam = 0;
    psp.pfnDlgProc = NotConnectedPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_NOTCONNECTEDPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create the property sheet */
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
    psh.hInstance = hApplet;
    psh.hwndParent = hwnd;
    psh.nPages = nPages;
    psh.nStartPage = 0;
    psh.phpage = ahpsp;
    psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
    psh.pszbmHeader = MAKEINTRESOURCE(IDB_HEADER);

    /* Create title font */
    hTitleFont = CreateTitleFont();

    /* Display the wizard */
    PropertySheet(&psh);

    DeleteObject(hTitleFont);
}

/* FUNCTIONS ****************************************************************/

BOOL WINAPI
InstallNewDevice(HWND hwndParent, LPGUID ClassGuid, PDWORD pReboot)
{
    return FALSE;
}

VOID WINAPI
AddHardwareWizard(HWND hwnd, LPWSTR lpName)
{
    if (lpName != NULL)
    {
        DPRINT1("No support of remote installation yet!\n");
        return;
    }

    HardwareWizardInit(hwnd);
}

/* Control Panel Callback */
LONG CALLBACK
CPlApplet(HWND hwndCpl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    switch (uMsg)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_GETCOUNT:
            return 1;

        case CPL_INQUIRE:
            {
                CPLINFO *CPlInfo = (CPLINFO*)lParam2;
                CPlInfo->lData = 0;
                CPlInfo->idIcon = IDI_CPLICON;
                CPlInfo->idName = IDS_CPLNAME;
                CPlInfo->idInfo = IDS_CPLDESCRIPTION;
            }
            break;

        case CPL_DBLCLK:
            AddHardwareWizard(hwndCpl, NULL);
            break;
    }

    return FALSE;
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
    UNREFERENCED_PARAMETER(lpvReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hApplet = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

    return TRUE;
}
