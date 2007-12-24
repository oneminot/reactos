/*
 * PROJECT:     RosBE Options Dialog
 * LICENSE:     GPL - See LICENSE.txt in the top level directory.
 * FILE:        Tools/config/options.c
 * PURPOSE:     Configuring RosBE
 * COPYRIGHT:   Copyright 2007 Maarten Bosma
 *              Copyright 2007 Pierre Schweitzer
 *
 */

#include "options.h"

HINSTANCE hInstance;

INT
WriteSettings(HWND hwnd)
{
    INT foreground, background;
    BOOL showtime, writelog, useccache, strip, objstate, outstate;
    WCHAR logdir[MAX_PATH], objdir[MAX_PATH], outdir[MAX_PATH], mingwpath[MAX_PATH], checkmgw[MAX_PATH], optionsfile[MAX_PATH];
    WCHAR msgerror[256];
    HANDLE hFile;
    FILE *pFile;

    showtime = (SendDlgItemMessage(hwnd, ID_SHOWBUILDTIME, BM_GETCHECK, 0, 0) == BST_CHECKED);
    writelog = (SendDlgItemMessage(hwnd, ID_SAVELOGS, BM_GETCHECK, 0, 0) == BST_CHECKED);
    useccache = (SendDlgItemMessage(hwnd, ID_USECCACHE, BM_GETCHECK, 0, 0) == BST_CHECKED);
    strip = (SendDlgItemMessageW(hwnd, ID_STRIP, BM_GETCHECK, 0, 0) == BST_CHECKED);
    objstate = (SendDlgItemMessage(hwnd, ID_OTHEROBJ, BM_GETCHECK, 0, 0) == BST_CHECKED);
    outstate = (SendDlgItemMessageW(hwnd, ID_OTHEROUT, BM_GETCHECK, 0, 0) == BST_CHECKED);
    foreground = SendDlgItemMessageW(hwnd, IDC_FONT, CB_GETCURSEL, 0, 0);
    background = SendDlgItemMessageW(hwnd, IDC_BACK, CB_GETCURSEL, 0, 0);
    GetDlgItemTextW(hwnd, ID_LOGDIR, logdir, MAX_PATH);
    GetDlgItemTextW(hwnd, ID_MGWDIR, mingwpath, MAX_PATH);
    GetDlgItemTextW(hwnd, ID_OBJDIR, objdir, MAX_PATH);
    GetDlgItemTextW(hwnd, ID_OUTDIR, outdir, MAX_PATH);

    if (writelog && (logdir[0] != 0))
        if (0 > (LONG)GetFileAttributes(logdir))
            if (CreateDirectory(logdir, NULL) == 0)
            {
                LoadString(hInstance, MSG_DIREFAILED, msgerror, 256);
                MessageBox(hwnd, msgerror, NULL, MB_ICONERROR);
                return FALSE;
            }

    wcscpy(checkmgw, mingwpath);
    if ((wcslen(checkmgw) + wcslen(L"\\bin\\gcc.exe")) < MAX_PATH)
        wcscat(checkmgw, L"\\bin\\gcc.exe");
    hFile = CreateFile(checkmgw, 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        LoadString(hInstance, MSG_NOGCCFOUND, msgerror, 256);
        MessageBox(hwnd, msgerror, NULL, MB_ICONERROR);
        return FALSE;
    }
    CloseHandle(hFile);

    wcscpy(optionsfile, _wgetenv(L"APPDATA"));
    if ((wcslen(optionsfile) + wcslen(L"\\RosBE\\rosbe-options.cmd")) < MAX_PATH)
        wcscat(optionsfile, L"\\RosBE\\rosbe-options.cmd");
    pFile = _wfopen(optionsfile, L"w");
    if (pFile)
    {
        fwprintf(pFile, L"::\n");
        fwprintf(pFile, L":: This file has been automatically generated by the ReactOS\n");
        fwprintf(pFile, L":: Build Environment options utility.\n");
        fwprintf(pFile, L"::\n\n");
        fwprintf(pFile, L"color %X%X\n", background, foreground);
        fwprintf(pFile, L"set _ROSBE_SHOWTIME=%d\n", showtime);
        fwprintf(pFile, L"set _ROSBE_USECCACHE=%d\n", useccache);
        fwprintf(pFile, L"set _ROSBE_STRIP=%d\n", strip);
        fwprintf(pFile, L"set _ROSBE_WRITELOG=%d\n", writelog);
        if (logdir[0] != 0) fwprintf(pFile, L"set _ROSBE_LOGDIR=%s\n", logdir);
        if (mingwpath[0] != 0) fwprintf(pFile, L"set _ROSBE_MINGWPATH=%s\n", mingwpath);
        if ((objdir[0] != 0) && objstate) fwprintf(pFile, L"set _ROSBE_OBJPATH=%s\n", objdir);
        if ((outdir[0] != 0) && outstate) fwprintf(pFile, L"set _ROSBE_OUTPATH=%s\n", outdir);
        fclose(pFile);
        return TRUE;
    }
    LoadString(hInstance, MSG_FILEFAILED, msgerror, 256);
    MessageBox(hwnd, msgerror, NULL, MB_ICONERROR);
    return FALSE;
}

VOID LoadSettings(HWND hwnd, PSETTINGS LoadedSettings)
{
    FILE *pFile;
    WCHAR optionsfile[MAX_PATH];
    WCHAR *ptr, *ptr2;
    WCHAR WTempLine[25+MAX_PATH];
    WCHAR SBTitle[256];

    wcscpy(optionsfile, _wgetenv(L"APPDATA"));
    if ((wcslen(optionsfile) + wcslen(L"\\RosBE\\rosbe-options.cmd")) < MAX_PATH)
        wcscat(optionsfile, L"\\RosBE\\rosbe-options.cmd");
    pFile = _wfopen(optionsfile, L"r");
    if (pFile)
    {
        while (fgetws(WTempLine, 24+MAX_PATH, pFile))
        {
            ptr = wcstok(WTempLine, L" ");
            if (wcscmp(ptr, L"color") == 0)
            {
                ptr = wcstok(NULL, L" ");
                ptr[2] = ptr[1];
                ptr[1] = 0;
                LoadedSettings->background = wcstol(&ptr[0], NULL, 16);
                LoadedSettings->foreground = wcstol(&ptr[2], NULL, 16);
            }
            else if (wcscmp(ptr, L"set") == 0)
            {
                ptr = wcstok(NULL, L" ");
                ptr = wcstok(ptr, L"=");
                ptr2 = wcstok(NULL, L"=");
                if (wcscmp(ptr, L"_ROSBE_SHOWTIME") == 0)
                    LoadedSettings->showtime = wcstol(ptr2, NULL, 2);
                else if (wcscmp(ptr, L"_ROSBE_USECCACHE") == 0)
                    LoadedSettings->useccache = wcstol(ptr2, NULL, 2);
                else if (wcscmp(ptr, L"_ROSBE_STRIP") == 0)
                    LoadedSettings->strip = wcstol(ptr2, NULL, 2);
                else if (wcscmp(ptr, L"_ROSBE_WRITELOG") == 0)
                    LoadedSettings->writelog = wcstol(ptr2, NULL, 2);
                else if (wcscmp(ptr, L"_ROSBE_LOGDIR") == 0)
                    wcsncpy(LoadedSettings->logdir, ptr2, wcslen(ptr2)-1);
                else if (wcscmp(ptr, L"_ROSBE_MINGWPATH") == 0)
                    wcsncpy(LoadedSettings->mingwpath, ptr2, wcslen(ptr2)-1);
                else if (wcscmp(ptr, L"_ROSBE_OBJPATH") == 0)
                    wcsncpy(LoadedSettings->objdir, ptr2, wcslen(ptr2)-1);
                else if (wcscmp(ptr, L"_ROSBE_OUTPATH") == 0)
                    wcsncpy(LoadedSettings->outdir, ptr2, wcslen(ptr2)-1);
            }
            free(WTempLine);
        }
        fclose(pFile);
        free(WTempLine);
        LoadString(hInstance, MSG_SETLOADSUC, SBTitle, 256);
        SetDlgItemText(hwnd, ID_STATUSBAR, SBTitle);
    }
    else
    {
        LoadedSettings->foreground = 0xa;
        LoadedSettings->background = 0;
        GetCurrentDirectory(MAX_PATH, LoadedSettings->mingwpath);
        if ((wcslen(LoadedSettings->mingwpath) + wcslen(MINGWVERSION)) < MAX_PATH)
            wcscat(LoadedSettings->mingwpath, MINGWVERSION);
    }
    SendDlgItemMessageW(hwnd, IDC_FONT, CB_SETCURSEL, LoadedSettings->foreground, 0);
    SendDlgItemMessageW(hwnd, IDC_BACK, CB_SETCURSEL, LoadedSettings->background, 0);
    SendDlgItemMessage(hwnd, ID_SHOWBUILDTIME, BM_SETCHECK, LoadedSettings->showtime, 0);
    SendDlgItemMessage(hwnd, ID_SAVELOGS, BM_SETCHECK, LoadedSettings->writelog, 0);
    if (LoadedSettings->writelog)
    {
        EnableWindow(GetDlgItem(hwnd, ID_BROWSE), TRUE);
        EnableWindow(GetDlgItem(hwnd, ID_LOGDIR), TRUE);
    }
    SendDlgItemMessage(hwnd, ID_USECCACHE, BM_SETCHECK, LoadedSettings->useccache, 0);
    SendDlgItemMessageW(hwnd, ID_STRIP, BM_SETCHECK, LoadedSettings->strip, 0);
    SetDlgItemText(hwnd, ID_MGWDIR, LoadedSettings->mingwpath);
    SetDlgItemText(hwnd, ID_LOGDIR, LoadedSettings->logdir);
    if (LoadedSettings->objdir[0] != 0)
    {
        LoadedSettings->objstate = 1;
        SetDlgItemText(hwnd, ID_OBJDIR, LoadedSettings->objdir);
        SendDlgItemMessage(hwnd, ID_OTHEROBJ, BM_SETCHECK, BST_CHECKED, 0);
        EnableWindow(GetDlgItem(hwnd, ID_BROWSEOBJ), TRUE);
        EnableWindow(GetDlgItem(hwnd, ID_OBJDIR), TRUE);
    }
    if (LoadedSettings->outdir[0] != 0)
    {
        LoadedSettings->outstate = 1;
        SetDlgItemText(hwnd, ID_OUTDIR, LoadedSettings->outdir);
        SendDlgItemMessage(hwnd, ID_OTHEROUT, BM_SETCHECK, BST_CHECKED, 0);
        EnableWindow(GetDlgItem(hwnd, ID_BROWSEOUT), TRUE);
        EnableWindow(GetDlgItem(hwnd, ID_OUTDIR), TRUE);
    }
}

VOID SetSaveState(HWND hwnd, PSETTINGS DefaultSettings)
{
    INT foreground, background;
    BOOL showtime, writelog, useccache, strip, objstate, outstate;
    WCHAR logdir[MAX_PATH], objdir[MAX_PATH], outdir[MAX_PATH], mingwpath[MAX_PATH];
    BOOL StateObj = TRUE, StateOut = TRUE, StateLog = TRUE, State = TRUE;

    showtime = (SendDlgItemMessage(hwnd, ID_SHOWBUILDTIME, BM_GETCHECK, 0, 0) == BST_CHECKED);
    writelog = (SendDlgItemMessage(hwnd, ID_SAVELOGS, BM_GETCHECK, 0, 0) == BST_CHECKED);
    useccache = (SendDlgItemMessage(hwnd, ID_USECCACHE, BM_GETCHECK, 0, 0) == BST_CHECKED);
    strip = (SendDlgItemMessageW(hwnd, ID_STRIP, BM_GETCHECK, 0, 0) == BST_CHECKED);
    objstate = (SendDlgItemMessageW(hwnd, ID_OTHEROBJ, BM_GETCHECK, 0, 0) == BST_CHECKED);
    outstate = (SendDlgItemMessageW(hwnd, ID_OTHEROUT, BM_GETCHECK, 0, 0) == BST_CHECKED);
    foreground = SendDlgItemMessageW(hwnd, IDC_FONT, CB_GETCURSEL, 0, 0);
    background = SendDlgItemMessageW(hwnd, IDC_BACK, CB_GETCURSEL, 0, 0);
    GetDlgItemTextW(hwnd, ID_LOGDIR, logdir, MAX_PATH);
    GetDlgItemTextW(hwnd, ID_MGWDIR, mingwpath, MAX_PATH);
    GetDlgItemTextW(hwnd, ID_OBJDIR, objdir, MAX_PATH);
    GetDlgItemTextW(hwnd, ID_OUTDIR, outdir, MAX_PATH);

    if (objstate)
        if ((wcscmp(objdir, DefaultSettings->objdir) != 0) && (wcslen(objdir) > 0))
            StateObj = FALSE;
    if (outstate)
        if ((wcscmp(outdir, DefaultSettings->outdir) != 0) && (wcslen(outdir) > 0))
            StateOut = FALSE;
    if (writelog)
        if ((wcscmp(logdir, DefaultSettings->logdir) != 0) && (wcslen(logdir) > 0))
            StateLog = FALSE;

    State ^= ((foreground == DefaultSettings->foreground) && (background == DefaultSettings->background) &&
            (showtime == DefaultSettings->showtime) && (writelog == DefaultSettings->writelog) &&
            (useccache == DefaultSettings->useccache) && (strip == DefaultSettings->strip) &&
            (objstate == DefaultSettings->objstate) && (outstate == DefaultSettings->outstate) &&
            (StateLog) && (wcscmp(mingwpath, DefaultSettings->mingwpath) == 0) &&
            (StateObj) && (StateOut));

    EnableWindow(GetDlgItem(hwnd, ID_OK), State);
}

BOOL CALLBACK EnumChildProc(HWND hwndChild, LPARAM lParam)
{
    if (hwndChild == (HWND)lParam)
    {
        HWND hwndParent;
        LONG WindowID;
        INT TextID = HLP_DEFAULTMSG;
        WCHAR HelpTxt[256];
        hwndParent = GetParent(hwndChild);
        WindowID = GetWindowLong(hwndChild, GWL_ID);
        switch(WindowID)
        {
            case IDC_BACK: TextID = HLP_BACKCOLORC; break;
            case IDC_FONT: TextID = HLP_FONTCOLORC; break;
            case ID_SHOWBUILDTIME: TextID = HLP_SBUILDTIME; break;
            case ID_USECCACHE: TextID = HLP_CCACHEUSED; break;
            case ID_STRIP: TextID = HLP_STRIPEDEXE; break;
            case ID_MGWDIR:
            case ID_BROWSEMGW: TextID = HLP_FINDMGWDIR; break;
            case ID_LOGDIR:
            case ID_BROWSE:
            case ID_SAVELOGS: TextID = HLP_FINDLOGDIR; break;
            case ID_OBJDIR:
            case ID_BROWSEOBJ:
            case ID_OTHEROBJ: TextID = HLP_FINDOBJDIR; break;
            case ID_OUTDIR:
            case ID_BROWSEOUT:
            case ID_OTHEROUT: TextID = HLP_FINDOUTDIR; break;
            case ID_OK: TextID = HLP_SAVEBUTTON; break;
            case ID_CANCEL: TextID = HLP_QUITBUTTON; break;
        }
        LoadString(hInstance, TextID, HelpTxt, 256);
        SetDlgItemText(hwndParent, ID_STATUSBAR, HelpTxt);
        return FALSE;
    }
    return TRUE;
}

INT CALLBACK
BrowseProc(HWND Dlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if ((Msg == BFFM_VALIDATEFAILED) && (lParam != ID_MGWDIR))
    {
        WCHAR BoxMsg[256], BoxTitle[128];
        HWND hwndParent;
        hwndParent = GetWindow(Dlg, GW_OWNER);
        LoadString(hInstance, MSG_WARNINGBOX, BoxTitle, 128);
        LoadString(hInstance, MSG_INVALIDDIR, BoxMsg, 256);
        if (MessageBox(Dlg, BoxMsg, BoxTitle, MB_ICONWARNING | MB_YESNO) == IDYES)
        {
            if (CreateDirectory((LPWSTR)wParam, NULL) == 0)
            {
                LoadString(hInstance, MSG_DIREFAILED, BoxMsg, 256);
                MessageBox(Dlg, BoxMsg, NULL, MB_ICONERROR);
            }
            else
            {
                SetDlgItemText(hwndParent, lParam, (LPWSTR)wParam);
            }
        }
    }
    return FALSE;
}

INT_PTR CALLBACK
DlgProc(HWND Dlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    static HICON hIcon;
    static HFONT hFont;
    static SETTINGS Settings;

    switch (Msg)
    {
        case WM_INITDIALOG:
        {
            LOGFONT lf;
            WCHAR TempColor[256];

            hIcon = LoadImage( hInstance,
                               MAKEINTRESOURCE(ID_OPTICON),
                               IMAGE_ICON,
                               GetSystemMetrics(SM_CXSMICON),
                               GetSystemMetrics(SM_CYSMICON),
                               0);
            if(hIcon)
                SendMessage(Dlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

            GetObject(GetStockObject(ANSI_FIXED_FONT), sizeof(LOGFONT),  &lf);
            lf.lfWeight = FW_BOLD;
            hFont = CreateFont(lf.lfHeight, lf.lfWidth,  lf.lfEscapement, lf.lfOrientation, lf.lfWeight,
            lf.lfItalic, lf.lfUnderline, lf.lfStrikeOut, lf.lfCharSet,  lf.lfOutPrecision,
            lf.lfClipPrecision, lf.lfQuality,  lf.lfPitchAndFamily, lf.lfFaceName);

            for(UINT i = 0; i < 16; i++)
            {
                LoadString(hInstance, i, TempColor, 256);
                SendDlgItemMessageW(Dlg, IDC_BACK, CB_ADDSTRING, 0, (LPARAM) TempColor);
                SendDlgItemMessageW(Dlg, IDC_FONT, CB_ADDSTRING, 0, (LPARAM) TempColor);
            }
            LoadSettings(Dlg, &Settings);
            return TRUE;
        }

        case WM_COMMAND:
        {
            if ((HIWORD(wParam) == CBN_SELCHANGE) && ((LOWORD(wParam) == IDC_FONT) || (LOWORD(wParam) == IDC_BACK)))
            {
                RECT rcWnd;
                GetClientRect(GetDlgItem(Dlg, ID_EXAMPLE), &rcWnd);
                InvalidateRect(GetDlgItem(Dlg, ID_EXAMPLE), &rcWnd, FALSE);
            }
            else
            {
                switch (wParam)
                {
                    case ID_OK:
                    {
                       if (!WriteSettings(Dlg))
                            break;
                    }
                    case ID_CANCEL:
                    {
                        WCHAR BoxMsg[256], BoxTitle[128];
                        if ((IsWindowEnabled(GetDlgItem(Dlg, ID_OK))) && (wParam == ID_CANCEL))
                        {
                            LoadString(hInstance, MSG_WARNINGBOX, BoxTitle, 128);
                            LoadString(hInstance, MSG_EXITCHANGE, BoxMsg, 256);
                            if (MessageBox(Dlg, BoxMsg, BoxTitle, MB_ICONWARNING | MB_YESNO) == IDNO)
                                break;
                        }
                        PostMessage(Dlg, WM_CLOSE, 0, 0);
                        break;
                    }
                    case ID_BROWSE:
                    case ID_BROWSEMGW:
                    case ID_BROWSEOBJ:
                    case ID_BROWSEOUT:
                    {
                        BROWSEINFO PathInfo;
                        LPITEMIDLIST pidl;
                        LPMALLOC pMalloc;
                        INT Control = ID_LOGDIR;
                        INT IDText = MSG_FINDLOGDIR;
                        WCHAR path[MAX_PATH];
                        WCHAR Text[512];

                        if (SHGetMalloc(&pMalloc) == NOERROR)
                        {
                            ZeroMemory(&PathInfo, sizeof(BROWSEINFO));
                            PathInfo.hwndOwner = Dlg;
                            PathInfo.ulFlags = BIF_EDITBOX | BIF_VALIDATE;
                            PathInfo.lpfn = (BFFCALLBACK)BrowseProc;
                            PathInfo.lParam = ID_LOGDIR;
#if 0
                            SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAM_FILES, &pidl);
                            PathInfo.pidlRoot = pidl;
#endif
                            if ((wParam == ID_BROWSEMGW) || (wParam == ID_BROWSEOBJ) || (wParam == ID_BROWSEOUT))
                            {
#if 0
                                HINSTANCE hDLL;
                                ILCREATEFROMPATHW ILCreateFromPathW;
#endif
                                Control = ID_MGWDIR;
                                IDText = MSG_FINDMGWDIR;
                                if (wParam == ID_BROWSEOBJ)
                                {
                                    Control = ID_OBJDIR;
                                    IDText = MSG_FINDOBJDIR;
                                }
                                else if (wParam == ID_BROWSEOUT)
                                {
                                    Control = ID_OUTDIR;
                                    IDText = MSG_FINDOUTDIR;
                                }
                                PathInfo.lParam = Control;
#if 0
                                hDLL = LoadLibrary(L"shell32.dll");
                                if (hDLL)
                                {
                                    ILCreateFromPathW = (ILCREATEFROMPATHW)GetProcAddress(hDLL, "ILCreateFromPathW");
                                    if (ILCreateFromPathW)
                                    {
                                        GetDlgItemTextW(Dlg, Control, path, MAX_PATH);
                                        PathInfo.pidlRoot = ILCreateFromPathW(path);
                                    }
                                    FreeLibrary(hDLL);
                                }
#endif
                            }
                            LoadString(hInstance, IDText, Text, 512);
                            PathInfo.lpszTitle = Text;
                            if ((pidl = SHBrowseForFolder(&PathInfo)) != NULL)
                            {
                                if (SHGetPathFromIDList(pidl, path))
                                    SetDlgItemText(Dlg, Control, path);
                                pMalloc->lpVtbl->Free(pMalloc, pidl);
                            }
                            pMalloc->lpVtbl->Release(pMalloc);
                        }
                        break;
                    }
                    case ID_OTHEROBJ:
                    case ID_OTHEROUT:
                    case ID_SAVELOGS:
                    {
                        BOOL WriteLogSet;
                        INT Dialog1 = ID_BROWSE;
                        INT Dialog2 = ID_LOGDIR;
                        WriteLogSet = (SendDlgItemMessageW(Dlg, wParam, BM_GETCHECK, 0, 0) == BST_CHECKED);
                        if (wParam == ID_OTHEROBJ)
                        {
                            Dialog1 = ID_BROWSEOBJ;
                            Dialog2 = ID_OBJDIR;
                        }
                        else if (wParam == ID_OTHEROUT)
                        {
                            Dialog1 = ID_BROWSEOUT;
                            Dialog2 = ID_OUTDIR;
                        }
                        EnableWindow(GetDlgItem(Dlg, Dialog1), WriteLogSet);
                        EnableWindow(GetDlgItem(Dlg, Dialog2), WriteLogSet);
                        break;
                    }
                }

            }
            SetSaveState(Dlg, &Settings);
            return FALSE;
        }

        case WM_CTLCOLORSTATIC:
        {
            HFONT hFontOld;
            // note: do not change the order - it matches to previous order
            COLORREF ColorsRGB[] = { 0x00000000, 0x00800000, 0x00008000, 0x00808000, 0x00000080, 0x00800080,
                0x00008080, 0x00c0c0c0, 0x00808080, 0x00ff0000, 0x0000ff00,
                0x00ffff00, 0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff
            };

            if((HWND)lParam == GetDlgItem(Dlg, ID_EXAMPLE))
            {
                hFontOld = SelectObject((HDC)wParam, hFont);
                SetTextColor((HDC)wParam, ColorsRGB[SendDlgItemMessageW(Dlg, IDC_FONT, CB_GETCURSEL, 0, 0)]);
                SetBkColor((HDC)wParam, ColorsRGB[SendDlgItemMessageW(Dlg, IDC_BACK, CB_GETCURSEL, 0, 0)]);
                return (LONG)CreateSolidBrush(ColorsRGB[SendDlgItemMessageW(Dlg, IDC_BACK, CB_GETCURSEL, 0, 0)]);
            }
            break;
        }

        case WM_SETCURSOR:
        {
            EnumChildWindows(Dlg, EnumChildProc, (LPARAM)wParam);
            return FALSE;
        }

        case WM_DESTROY:
        {
            if (hIcon)
                DestroyIcon(hIcon);
        }

        case WM_CLOSE:
        {
            EndDialog(Dlg, 0);
            return TRUE;
        }
    }
    return FALSE;
}

int WINAPI
WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdline, int cmdshow)
{
    hInstance = hInst;

    DialogBox(hInst, MAKEINTRESOURCE(ID_DIALOG), 0, DlgProc);
    return 0;
}
