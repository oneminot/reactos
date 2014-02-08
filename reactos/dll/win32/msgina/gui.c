/*
 * PROJECT:         ReactOS msgina.dll
 * FILE:            dll/win32/msgina/gui.c
 * PURPOSE:         ReactOS Logon GINA DLL
 * PROGRAMMER:      Herv� Poussineau (hpoussin@reactos.org)
 */

#include "msgina.h"

#include <wingdi.h>
#include <winnls.h>

typedef struct _DISPLAYSTATUSMSG
{
    PGINA_CONTEXT Context;
    HDESK hDesktop;
    DWORD dwOptions;
    PWSTR pTitle;
    PWSTR pMessage;
    HANDLE StartupEvent;
} DISPLAYSTATUSMSG, *PDISPLAYSTATUSMSG;

static BOOL
GUIInitialize(
    IN OUT PGINA_CONTEXT pgContext)
{
    TRACE("GUIInitialize(%p)\n", pgContext);
    return TRUE;
}

static INT_PTR CALLBACK
StatusMessageWindowProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            PDISPLAYSTATUSMSG msg = (PDISPLAYSTATUSMSG)lParam;
            if (!msg)
                return FALSE;

            msg->Context->hStatusWindow = hwndDlg;

            if (msg->pTitle)
                SetWindowTextW(hwndDlg, msg->pTitle);
            SetDlgItemTextW(hwndDlg, IDC_STATUSLABEL, msg->pMessage);
            SetEvent(msg->StartupEvent);
            return TRUE;
        }
    }
    return FALSE;
}

static DWORD WINAPI
StartupWindowThread(LPVOID lpParam)
{
    HDESK hDesk;
    PDISPLAYSTATUSMSG msg = (PDISPLAYSTATUSMSG)lpParam;

    /* When SetThreadDesktop is called the system closes the desktop handle when needed
       so we have to create a new handle because this handle may still be in use by winlogon  */
    if (!DuplicateHandle (  GetCurrentProcess(), 
                            msg->hDesktop, 
                            GetCurrentProcess(), 
                            (HANDLE*)&hDesk, 
                            0, 
                            FALSE, 
                            DUPLICATE_SAME_ACCESS))
    {
        HeapFree(GetProcessHeap(), 0, lpParam);
        return FALSE;
    }

    if(!SetThreadDesktop(hDesk))
    {
        HeapFree(GetProcessHeap(), 0, lpParam);
        return FALSE;
    }

    DialogBoxParam(
        hDllInstance,
        MAKEINTRESOURCE(IDD_STATUSWINDOW_DLG),
        GetDesktopWindow(),
        StatusMessageWindowProc,
        (LPARAM)lpParam);

    HeapFree(GetProcessHeap(), 0, lpParam);
    return TRUE;
}

static BOOL
GUIDisplayStatusMessage(
    IN PGINA_CONTEXT pgContext,
    IN HDESK hDesktop,
    IN DWORD dwOptions,
    IN PWSTR pTitle,
    IN PWSTR pMessage)
{
    PDISPLAYSTATUSMSG msg;
    HANDLE Thread;
    DWORD ThreadId;

    TRACE("GUIDisplayStatusMessage(%ws)\n", pMessage);

    if (!pgContext->hStatusWindow)
    {
        msg = (PDISPLAYSTATUSMSG)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DISPLAYSTATUSMSG));
        if(!msg)
            return FALSE;

        msg->Context = pgContext;
        msg->dwOptions = dwOptions;
        msg->pTitle = pTitle;
        msg->pMessage = pMessage;
        msg->hDesktop = hDesktop;

        msg->StartupEvent = CreateEventW(
            NULL,
            TRUE,
            FALSE,
            NULL);

        if (!msg->StartupEvent)
            return FALSE;

        Thread = CreateThread(
            NULL,
            0,
            StartupWindowThread,
            (PVOID)msg,
            0,
            &ThreadId);
        if (Thread)
        {
            CloseHandle(Thread);
            WaitForSingleObject(msg->StartupEvent, INFINITE);
            CloseHandle(msg->StartupEvent);
            return TRUE;
        }

        return FALSE;
    }

    if (pTitle)
        SetWindowTextW(pgContext->hStatusWindow, pTitle);

    SetDlgItemTextW(pgContext->hStatusWindow, IDC_STATUSLABEL, pMessage);

    return TRUE;
}

static BOOL
GUIRemoveStatusMessage(
    IN PGINA_CONTEXT pgContext)
{
    if (pgContext->hStatusWindow)
    {
        EndDialog(pgContext->hStatusWindow, 0);
        pgContext->hStatusWindow = NULL;
    }

    return TRUE;
}

static INT_PTR CALLBACK
EmptyWindowProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PGINA_CONTEXT pgContext;
    
    pgContext = (PGINA_CONTEXT)GetWindowLongPtr(hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pgContext->hBitmap = LoadImage(hDllInstance, MAKEINTRESOURCE(IDI_ROSLOGO), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            if (pgContext->hBitmap)
            {
                hdc = BeginPaint(hwndDlg, &ps);
                DrawStateW(hdc, NULL, NULL, (LPARAM)pgContext->hBitmap, (WPARAM)0, 0, 0, 0, 0, DST_BITMAP);
                EndPaint(hwndDlg, &ps);
            }
            return TRUE;
        }
        case WM_DESTROY:
        {
            DeleteObject(pgContext->hBitmap);
            return TRUE;
        }
    }
    return FALSE;
}

static VOID
GUIDisplaySASNotice(
    IN OUT PGINA_CONTEXT pgContext)
{
    TRACE("GUIDisplaySASNotice()\n");

    /* Display the notice window */
    pgContext->pWlxFuncs->WlxDialogBoxParam(pgContext->hWlx,
                                            pgContext->hDllInstance,
                                            MAKEINTRESOURCEW(IDD_NOTICE_DLG),
                                            GetDesktopWindow(),
                                            EmptyWindowProc,
                                            (LPARAM)NULL);
}

/* Get the text contained in a textbox. Allocates memory in pText
 * to contain the text. Returns TRUE in case of success */
static BOOL
GetTextboxText(
    IN HWND hwndDlg,
    IN INT TextboxId,
    OUT LPWSTR *pText)
{
    LPWSTR Text;
    int Count;

    Count = GetWindowTextLength(GetDlgItem(hwndDlg, TextboxId));
    Text = HeapAlloc(GetProcessHeap(), 0, (Count + 1) * sizeof(WCHAR));
    if (!Text)
        return FALSE;
    if (Count != GetWindowTextW(GetDlgItem(hwndDlg, TextboxId), Text, Count + 1))
    {
        HeapFree(GetProcessHeap(), 0, Text);
        return FALSE;
    }
    *pText = Text;
    return TRUE;
}


static
INT
ResourceMessageBox(
    IN PGINA_CONTEXT pgContext,
    IN HWND hwnd,
    IN UINT uType,
    IN UINT uCaption,
    IN UINT uText)
{
    WCHAR szCaption[256];
    WCHAR szText[256];

    LoadStringW(pgContext->hDllInstance, uCaption, szCaption, 256);
    LoadStringW(pgContext->hDllInstance, uText, szText, 256);

    return pgContext->pWlxFuncs->WlxMessageBox(pgContext->hWlx,
                                               hwnd,
                                               szText,
                                               szCaption,
                                               uType);
}


static
BOOL
DoChangePassword(
    IN PGINA_CONTEXT pgContext,
    IN HWND hwndDlg)
{
    WCHAR UserName[256];
    WCHAR DomainName[256];
    WCHAR OldPassword[256];
    WCHAR NewPassword1[256];
    WCHAR NewPassword2[256];
    PMSV1_0_CHANGEPASSWORD_REQUEST RequestBuffer = NULL;
    PMSV1_0_CHANGEPASSWORD_RESPONSE ResponseBuffer = NULL;
    ULONG RequestBufferSize;
    ULONG ResponseBufferSize = 0;
    LPWSTR Ptr;
    BOOL res = FALSE;
    NTSTATUS ProtocolStatus;
    NTSTATUS Status;

    GetDlgItemTextW(hwndDlg, IDC_CHANGEPWD_USERNAME, UserName, 256);
    GetDlgItemTextW(hwndDlg, IDC_CHANGEPWD_DOMAIN, DomainName, 256);
    GetDlgItemTextW(hwndDlg, IDC_CHANGEPWD_OLDPWD, OldPassword, 256);
    GetDlgItemTextW(hwndDlg, IDC_CHANGEPWD_NEWPWD1, NewPassword1, 256);
    GetDlgItemTextW(hwndDlg, IDC_CHANGEPWD_NEWPWD2, NewPassword2, 256);

    /* Compare the two passwords and fail if they do not match */
    if (wcscmp(NewPassword1, NewPassword2) != 0)
    {
        ResourceMessageBox(pgContext,
                           hwndDlg,
                           MB_OK | MB_ICONEXCLAMATION,
                           IDS_CHANGEPWDTITLE,
                           IDS_NONMATCHINGPASSWORDS);
        return FALSE;
    }

    /* Calculate the request buffer size */
    RequestBufferSize = sizeof(MSV1_0_CHANGEPASSWORD_REQUEST) +
                        ((wcslen(DomainName) + 1) * sizeof(WCHAR)) +
                        ((wcslen(UserName) + 1) * sizeof(WCHAR)) +
                        ((wcslen(OldPassword) + 1) * sizeof(WCHAR)) +
                        ((wcslen(NewPassword1) + 1) * sizeof(WCHAR));

    /* Allocate the request buffer */
    RequestBuffer = HeapAlloc(GetProcessHeap(),
                              HEAP_ZERO_MEMORY,
                              RequestBufferSize);
    if (RequestBuffer == NULL)
    {
        ERR("HeapAlloc failed\n");
        return FALSE;
    }

    /* Initialize the request buffer */
    RequestBuffer->MessageType = MsV1_0ChangePassword;
    RequestBuffer->Impersonating = TRUE;

    Ptr = (LPWSTR)((ULONG_PTR)RequestBuffer + sizeof(MSV1_0_CHANGEPASSWORD_REQUEST));

    /* Pack the domain name */
    RequestBuffer->DomainName.Length = wcslen(DomainName) * sizeof(WCHAR);
    RequestBuffer->DomainName.MaximumLength = RequestBuffer->DomainName.Length + sizeof(WCHAR);
    RequestBuffer->DomainName.Buffer = Ptr;

    RtlCopyMemory(RequestBuffer->DomainName.Buffer,
                  DomainName,
                  RequestBuffer->DomainName.MaximumLength);

    Ptr = (LPWSTR)((ULONG_PTR)Ptr + RequestBuffer->DomainName.MaximumLength);

    /* Pack the user name */
    RequestBuffer->AccountName.Length = wcslen(UserName) * sizeof(WCHAR);
    RequestBuffer->AccountName.MaximumLength = RequestBuffer->AccountName.Length + sizeof(WCHAR);
    RequestBuffer->AccountName.Buffer = Ptr;

    RtlCopyMemory(RequestBuffer->AccountName.Buffer,
                  UserName,
                  RequestBuffer->AccountName.MaximumLength);

    Ptr = (LPWSTR)((ULONG_PTR)Ptr + RequestBuffer->AccountName.MaximumLength);

    /* Pack the old password */
    RequestBuffer->OldPassword.Length = wcslen(OldPassword) * sizeof(WCHAR);
    RequestBuffer->OldPassword.MaximumLength = RequestBuffer->OldPassword.Length + sizeof(WCHAR);
    RequestBuffer->OldPassword.Buffer = Ptr;

    RtlCopyMemory(RequestBuffer->OldPassword.Buffer,
                  OldPassword,
                  RequestBuffer->OldPassword.MaximumLength);

    Ptr = (LPWSTR)((ULONG_PTR)Ptr + RequestBuffer->OldPassword.MaximumLength);

    /* Pack the new password */
    RequestBuffer->NewPassword.Length = wcslen(NewPassword1) * sizeof(WCHAR);
    RequestBuffer->NewPassword.MaximumLength = RequestBuffer->NewPassword.Length + sizeof(WCHAR);
    RequestBuffer->NewPassword.Buffer = Ptr;

    RtlCopyMemory(RequestBuffer->NewPassword.Buffer,
                  NewPassword1,
                  RequestBuffer->NewPassword.MaximumLength);

    /* Connect to the LSA server */
    if (!ConnectToLsa(pgContext))
    {
        ERR("ConnectToLsa() failed\n");
        return FALSE;
    }

    /* Call the authentication package */
    Status = LsaCallAuthenticationPackage(pgContext->LsaHandle,
                                          pgContext->AuthenticationPackage,
                                          RequestBuffer,
                                          RequestBufferSize,
                                          (PVOID*)&ResponseBuffer,
                                          &ResponseBufferSize,
                                          &ProtocolStatus);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsaCallAuthenticationPackage failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    if (!NT_SUCCESS(ProtocolStatus))
    {
        TRACE("LsaCallAuthenticationPackage failed (ProtocolStatus 0x%08lx)\n", ProtocolStatus);
        goto done;
    }

    res = TRUE;

    ResourceMessageBox(pgContext,
                       hwndDlg,
                       MB_OK | MB_ICONINFORMATION,
                       IDS_CHANGEPWDTITLE,
                       IDS_PASSWORDCHANGED);

done:
    if (RequestBuffer != NULL)
        HeapFree(GetProcessHeap(), 0, RequestBuffer);

    if (ResponseBuffer != NULL)
        LsaFreeReturnBuffer(ResponseBuffer);

    return res;
}


static INT_PTR CALLBACK
ChangePasswordDialogProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PGINA_CONTEXT pgContext;

    pgContext = (PGINA_CONTEXT)GetWindowLongPtr(hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pgContext = (PGINA_CONTEXT)lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)pgContext);

            SetDlgItemTextW(hwndDlg, IDC_CHANGEPWD_USERNAME, pgContext->UserName);
            SendDlgItemMessageW(hwndDlg, IDC_CHANGEPWD_DOMAIN, CB_ADDSTRING, 0, (LPARAM)pgContext->Domain);
            SendDlgItemMessageW(hwndDlg, IDC_CHANGEPWD_DOMAIN, CB_SETCURSEL, 0, 0);
            SetFocus(GetDlgItem(hwndDlg, IDC_CHANGEPWD_OLDPWD));
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    if (DoChangePassword(pgContext, hwndDlg))
                    {
                        EndDialog(hwndDlg, TRUE);
                    }
                    else
                    {
                        SetDlgItemTextW(hwndDlg, IDC_CHANGEPWD_NEWPWD1, NULL);
                        SetDlgItemTextW(hwndDlg, IDC_CHANGEPWD_NEWPWD2, NULL);
                        SetFocus(GetDlgItem(hwndDlg, IDC_CHANGEPWD_OLDPWD));
                    }
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hwndDlg, FALSE);
                    return TRUE;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwndDlg, FALSE);
            return TRUE;
    }

    return FALSE;
}


static VOID
OnInitSecurityDlg(HWND hwnd,
                  PGINA_CONTEXT pgContext)
{
    WCHAR Buffer1[256];
    WCHAR Buffer2[256];
    WCHAR Buffer3[256];
    WCHAR Buffer4[512];

    LoadStringW(pgContext->hDllInstance, IDS_LOGONMSG, Buffer1, 256);

    wsprintfW(Buffer2, L"%s\\%s", pgContext->Domain, pgContext->UserName);
    wsprintfW(Buffer4, Buffer1, Buffer2);

    SetDlgItemTextW(hwnd, IDC_LOGONMSG, Buffer4);

    LoadStringW(pgContext->hDllInstance, IDS_LOGONDATE, Buffer1, 256);

    GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE,
                   (SYSTEMTIME*)&pgContext->LogonTime, NULL, Buffer2, 256);

    GetTimeFormatW(LOCALE_USER_DEFAULT, 0,
                   (SYSTEMTIME*)&pgContext->LogonTime, NULL, Buffer3, 256);

    wsprintfW(Buffer4, Buffer1, Buffer2, Buffer3);

    SetDlgItemTextW(hwnd, IDC_LOGONDATE, Buffer4);

    if (pgContext->bAutoAdminLogon == TRUE)
        EnableWindow(GetDlgItem(hwnd, IDC_LOGOFF), FALSE);
}


static BOOL
OnChangePassword(
    IN HWND hwnd,
    IN PGINA_CONTEXT pgContext)
{
    INT res;

    TRACE("OnChangePassword()\n");

    res = pgContext->pWlxFuncs->WlxDialogBoxParam(
        pgContext->hWlx,
        pgContext->hDllInstance,
        MAKEINTRESOURCEW(IDD_CHANGE_PASSWORD),
        hwnd,
        ChangePasswordDialogProc,
        (LPARAM)pgContext);

    TRACE("Result: %x\n", res);

    return FALSE;
}


static INT_PTR CALLBACK
LogOffDialogProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDYES:
                    EndDialog(hwndDlg, IDYES);
                    return TRUE;

                case IDNO:
                    EndDialog(hwndDlg, IDNO);
                    return TRUE;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwndDlg, IDNO);
            return TRUE;
    }

    return FALSE;
}


static
INT
OnLogOff(
    IN HWND hwndDlg,
    IN PGINA_CONTEXT pgContext)
{
    return pgContext->pWlxFuncs->WlxDialogBoxParam(
        pgContext->hWlx,
        pgContext->hDllInstance,
        MAKEINTRESOURCEW(IDD_LOGOFF_DLG),
        hwndDlg,
        LogOffDialogProc,
        (LPARAM)pgContext);
}


static INT_PTR CALLBACK
LoggedOnWindowProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PGINA_CONTEXT pgContext;

    pgContext = (PGINA_CONTEXT)GetWindowLongPtr(hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pgContext = (PGINA_CONTEXT)lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)pgContext);

            OnInitSecurityDlg(hwndDlg, (PGINA_CONTEXT)lParam);
            SetFocus(GetDlgItem(hwndDlg, IDNO));
            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_LOCK:
                    EndDialog(hwndDlg, WLX_SAS_ACTION_LOCK_WKSTA);
                    return TRUE;
                case IDC_LOGOFF:
                    if (OnLogOff(hwndDlg, pgContext) == IDYES)
                        EndDialog(hwndDlg, WLX_SAS_ACTION_LOGOFF);
                    return TRUE;
                case IDC_SHUTDOWN:
                    EndDialog(hwndDlg, WLX_SAS_ACTION_SHUTDOWN_POWER_OFF);
                    return TRUE;
                case IDC_CHANGEPWD:
                    if (OnChangePassword(hwndDlg, pgContext))
                        EndDialog(hwndDlg, WLX_SAS_ACTION_PWD_CHANGED);
                    return TRUE;
                case IDC_TASKMGR:
                    EndDialog(hwndDlg, WLX_SAS_ACTION_TASKLIST);
                    return TRUE;
                case IDCANCEL:
                    EndDialog(hwndDlg, WLX_SAS_ACTION_NONE);
                    return TRUE;
            }
            break;
        }
        case WM_CLOSE:
        {
            EndDialog(hwndDlg, WLX_SAS_ACTION_NONE);
            return TRUE;
        }
    }

    return FALSE;
}

static INT
GUILoggedOnSAS(
    IN OUT PGINA_CONTEXT pgContext,
    IN DWORD dwSasType)
{
    INT result;

    TRACE("GUILoggedOnSAS()\n");

    if (dwSasType != WLX_SAS_TYPE_CTRL_ALT_DEL)
    {
        /* Nothing to do for WLX_SAS_TYPE_TIMEOUT ; the dialog will
         * close itself thanks to the use of WlxDialogBoxParam */
        return WLX_SAS_ACTION_NONE;
    }

    pgContext->pWlxFuncs->WlxSwitchDesktopToWinlogon(
        pgContext->hWlx);

    result = pgContext->pWlxFuncs->WlxDialogBoxParam(
        pgContext->hWlx,
        pgContext->hDllInstance,
        MAKEINTRESOURCEW(IDD_LOGGEDON_DLG),
        GetDesktopWindow(),
        LoggedOnWindowProc,
        (LPARAM)pgContext);

    if (result < WLX_SAS_ACTION_LOGON ||
        result > WLX_SAS_ACTION_SWITCH_CONSOLE)
    {
        result = WLX_SAS_ACTION_NONE;
    }

    if (result == WLX_SAS_ACTION_NONE)
    {
        pgContext->pWlxFuncs->WlxSwitchDesktopToUser(
            pgContext->hWlx);
    }

    return result;
}

static INT_PTR CALLBACK
LoggedOutWindowProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PGINA_CONTEXT pgContext;

    pgContext = (PGINA_CONTEXT)GetWindowLongPtr(hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* FIXME: take care of NoDomainUI */
            pgContext = (PGINA_CONTEXT)lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)pgContext);

            if (pgContext->bDontDisplayLastUserName == FALSE)
                SetDlgItemTextW(hwndDlg, IDC_USERNAME, pgContext->UserName);

            if (pgContext->bDisableCAD == TRUE)
                EnableWindow(GetDlgItem(hwndDlg, IDCANCEL), FALSE);

            if (pgContext->bShutdownWithoutLogon == FALSE)
                EnableWindow(GetDlgItem(hwndDlg, IDC_SHUTDOWN), FALSE);

            SetFocus(GetDlgItem(hwndDlg, pgContext->bDontDisplayLastUserName ? IDC_USERNAME : IDC_PASSWORD));

            pgContext->hBitmap = LoadImage(hDllInstance, MAKEINTRESOURCE(IDI_ROSLOGO), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
            return TRUE;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            if (pgContext->hBitmap)
            {
                hdc = BeginPaint(hwndDlg, &ps);
                DrawStateW(hdc, NULL, NULL, (LPARAM)pgContext->hBitmap, (WPARAM)0, 0, 0, 0, 0, DST_BITMAP);
                EndPaint(hwndDlg, &ps);
            }
            return TRUE;
        }
        case WM_DESTROY:
        {
            DeleteObject(pgContext->hBitmap);
            return TRUE;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    LPWSTR UserName = NULL, Password = NULL;
                    INT result = WLX_SAS_ACTION_NONE;

                    if (GetTextboxText(hwndDlg, IDC_USERNAME, &UserName) && *UserName == '\0')
                        break;
                    if (GetTextboxText(hwndDlg, IDC_PASSWORD, &Password) &&
                        DoLoginTasks(pgContext, UserName, NULL, Password))
                    {
                        pgContext->Password = HeapAlloc(GetProcessHeap(),
                                                        HEAP_ZERO_MEMORY,
                                                        (wcslen(Password) + 1) * sizeof(WCHAR));
                        if (pgContext->Password != NULL)
                            wcscpy(pgContext->Password, Password);

                        result = WLX_SAS_ACTION_LOGON;
                    }
                    HeapFree(GetProcessHeap(), 0, UserName);
                    HeapFree(GetProcessHeap(), 0, Password);
                    EndDialog(hwndDlg, result);
                    return TRUE;
                }
                case IDCANCEL:
                {
                    EndDialog(hwndDlg, WLX_SAS_ACTION_NONE);
                    return TRUE;
                }
                case IDC_SHUTDOWN:
                {
                    EndDialog(hwndDlg, WLX_SAS_ACTION_SHUTDOWN);
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}

static INT
GUILoggedOutSAS(
    IN OUT PGINA_CONTEXT pgContext)
{
    int result;

    TRACE("GUILoggedOutSAS()\n");

    result = pgContext->pWlxFuncs->WlxDialogBoxParam(
        pgContext->hWlx,
        pgContext->hDllInstance,
        MAKEINTRESOURCEW(IDD_LOGGEDOUT_DLG),
        GetDesktopWindow(),
        LoggedOutWindowProc,
        (LPARAM)pgContext);
    if (result >= WLX_SAS_ACTION_LOGON &&
        result <= WLX_SAS_ACTION_SWITCH_CONSOLE)
    {
        WARN("WlxLoggedOutSAS() returns 0x%x\n", result);
        return result;
    }

    WARN("WlxDialogBoxParam() failed (0x%x)\n", result);
    return WLX_SAS_ACTION_NONE;
}


static VOID
SetLockMessage(HWND hwnd,
               INT nDlgItem,
               PGINA_CONTEXT pgContext)
{
    WCHAR Buffer1[256];
    WCHAR Buffer2[256];
    WCHAR Buffer3[512];

    LoadStringW(pgContext->hDllInstance, IDS_LOCKMSG, Buffer1, 256);

    wsprintfW(Buffer2, L"%s\\%s", pgContext->Domain, pgContext->UserName);
    wsprintfW(Buffer3, Buffer1, Buffer2);

    SetDlgItemTextW(hwnd, nDlgItem, Buffer3);
}


static
BOOL
DoUnlock(
    IN HWND hwndDlg,
    IN PGINA_CONTEXT pgContext,
    OUT LPINT Action)
{
    WCHAR Buffer1[256];
    WCHAR Buffer2[256];
    LPWSTR UserName = NULL;
    LPWSTR Password = NULL;
    BOOL res = FALSE;

    if (GetTextboxText(hwndDlg, IDC_USERNAME, &UserName) && *UserName == '\0')
        return FALSE;

    if (GetTextboxText(hwndDlg, IDC_PASSWORD, &Password))
    {
        if (UserName != NULL && Password != NULL &&
            wcscmp(UserName, pgContext->UserName) == 0 &&
            wcscmp(Password, pgContext->Password) == 0)
        {
            *Action = WLX_SAS_ACTION_UNLOCK_WKSTA;
            res = TRUE;
        }
        else if (wcscmp(UserName, pgContext->UserName) == 0 &&
                 wcscmp(Password, pgContext->Password) != 0)
        {
            /* Wrong Password */
            LoadStringW(pgContext->hDllInstance, IDS_LOCKEDWRONGPASSWORD, Buffer2, 256);
            LoadStringW(pgContext->hDllInstance, IDS_COMPUTERLOCKED, Buffer1, 256);
            MessageBoxW(hwndDlg, Buffer2, Buffer1, MB_OK | MB_ICONERROR);
        }
        else
        {
            /* Wrong user name */
            if (DoAdminUnlock(pgContext, UserName, NULL, Password))
            {
                *Action = WLX_SAS_ACTION_UNLOCK_WKSTA;
                res = TRUE;
            }
            else
            {
                LoadStringW(pgContext->hDllInstance, IDS_LOCKEDWRONGUSER, Buffer1, 256);
                wsprintfW(Buffer2, Buffer1, pgContext->Domain, pgContext->UserName);
                LoadStringW(pgContext->hDllInstance, IDS_COMPUTERLOCKED, Buffer1, 256);
                MessageBoxW(hwndDlg, Buffer2, Buffer1, MB_OK | MB_ICONERROR);
            }
        }
    }

    if (UserName != NULL)
        HeapFree(GetProcessHeap(), 0, UserName);

    if (Password != NULL)
        HeapFree(GetProcessHeap(), 0, Password);

    return res;
}


static
INT_PTR
CALLBACK
UnlockWindowProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PGINA_CONTEXT pgContext;
    INT result = WLX_SAS_ACTION_NONE;

    pgContext = (PGINA_CONTEXT)GetWindowLongPtr(hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pgContext = (PGINA_CONTEXT)lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)pgContext);

            SetLockMessage(hwndDlg, IDC_LOCKMSG, pgContext);

            SetDlgItemTextW(hwndDlg, IDC_USERNAME, pgContext->UserName);
            SetFocus(GetDlgItem(hwndDlg, IDC_PASSWORD));

            if (pgContext->bDisableCAD == TRUE)
                EnableWindow(GetDlgItem(hwndDlg, IDCANCEL), FALSE);

            pgContext->hBitmap = LoadImage(hDllInstance, MAKEINTRESOURCE(IDI_ROSLOGO), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
            return TRUE;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            if (pgContext->hBitmap)
            {
                hdc = BeginPaint(hwndDlg, &ps);
                DrawStateW(hdc, NULL, NULL, (LPARAM)pgContext->hBitmap, (WPARAM)0, 0, 0, 0, 0, DST_BITMAP);
                EndPaint(hwndDlg, &ps);
            }
            return TRUE;
        }
        case WM_DESTROY:
            DeleteObject(pgContext->hBitmap);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    if (DoUnlock(hwndDlg, pgContext, &result))
                        EndDialog(hwndDlg, result);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hwndDlg, WLX_SAS_ACTION_NONE);
                    return TRUE;
            }
            break;
    }

    return FALSE;
}


static INT
GUILockedSAS(
    IN OUT PGINA_CONTEXT pgContext)
{
    int result;

    TRACE("GUILockedSAS()\n");

    result = pgContext->pWlxFuncs->WlxDialogBoxParam(
        pgContext->hWlx,
        pgContext->hDllInstance,
        MAKEINTRESOURCEW(IDD_UNLOCK_DLG),
        GetDesktopWindow(),
        UnlockWindowProc,
        (LPARAM)pgContext);
    if (result >= WLX_SAS_ACTION_LOGON &&
        result <= WLX_SAS_ACTION_SWITCH_CONSOLE)
    {
        WARN("GUILockedSAS() returns 0x%x\n", result);
        return result;
    }

    WARN("GUILockedSAS() failed (0x%x)\n", result);
    return WLX_SAS_ACTION_NONE;
}


static INT_PTR CALLBACK
LockedWindowProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PGINA_CONTEXT pgContext;

    pgContext = (PGINA_CONTEXT)GetWindowLongPtr(hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pgContext = (PGINA_CONTEXT)lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)pgContext);

            pgContext->hBitmap = LoadImage(hDllInstance, MAKEINTRESOURCE(IDI_ROSLOGO), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
            SetLockMessage(hwndDlg, IDC_LOCKMSG, pgContext);
            return TRUE;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            if (pgContext->hBitmap)
            {
                hdc = BeginPaint(hwndDlg, &ps);
                DrawStateW(hdc, NULL, NULL, (LPARAM)pgContext->hBitmap, (WPARAM)0, 0, 0, 0, 0, DST_BITMAP);
                EndPaint(hwndDlg, &ps);
            }
            return TRUE;
        }
        case WM_DESTROY:
        {
            DeleteObject(pgContext->hBitmap);
            return TRUE;
        }
    }

    return FALSE;
}


static VOID
GUIDisplayLockedNotice(
    IN OUT PGINA_CONTEXT pgContext)
{
    TRACE("GUIdisplayLockedNotice()\n");

    pgContext->pWlxFuncs->WlxDialogBoxParam(
        pgContext->hWlx,
        pgContext->hDllInstance,
        MAKEINTRESOURCEW(IDD_LOCKED_DLG),
        GetDesktopWindow(),
        LockedWindowProc,
        (LPARAM)pgContext);
}

GINA_UI GinaGraphicalUI = {
    GUIInitialize,
    GUIDisplayStatusMessage,
    GUIRemoveStatusMessage,
    GUIDisplaySASNotice,
    GUILoggedOnSAS,
    GUILoggedOutSAS,
    GUILockedSAS,
    GUIDisplayLockedNotice,
};
