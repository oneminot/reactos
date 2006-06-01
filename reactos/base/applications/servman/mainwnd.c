/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/servman/mainwnd.c
 * PURPOSE:     Main window message handler
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

static const TCHAR szMainWndClass[] = TEXT("ServManWndClass");

BOOL bSortAscending = TRUE;

extern HWND hwndGenDlg;

/* Toolbar buttons */
TBBUTTON tbb [NUM_BUTTONS] =
{   /* iBitmap, idCommand, fsState, fsStyle, bReserved[2], dwData, iString */
    {TBICON_PROP,    ID_PROP,    TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0},    /* properties */
    {TBICON_REFRESH, ID_REFRESH, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},          /* refresh */
    {TBICON_EXPORT,  ID_EXPORT,  TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},          /* export */

    /* Note: First item for a seperator is its width in pixels */
    {15, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                                  /* separator */

    {TBICON_CREATE,  ID_CREATE,  TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },         /* create */
    {TBICON_DELETE,  ID_DELETE,  TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },         /* delete */

    {15, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                                  /* separator */

    {TBICON_START,   ID_START,   TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0 },   /* start */
    {TBICON_STOP,    ID_STOP,    TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0 },   /* stop */
    {TBICON_PAUSE,   ID_PAUSE,   TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0 },   /* pause */
    {TBICON_RESTART, ID_RESTART, TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0 },   /* restart */

    {15, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                                  /* separator */

    {TBICON_HELP,    ID_HELP,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },         /* help */
    {TBICON_EXIT,    ID_EXIT,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },          /* exit */

};


VOID SetView(HWND hListView, DWORD View)
{
    DWORD Style = GetWindowLong(hListView, GWL_STYLE);

    if ((Style & LVS_TYPEMASK) != View)
        SetWindowLong(hListView, GWL_STYLE, (Style & ~LVS_TYPEMASK) | View);
}


VOID SetMenuAndButtonStates(PMAIN_WND_INFO Info)
{
    HMENU hMainMenu;
    ENUM_SERVICE_STATUS_PROCESS *Service = NULL;
    DWORD Flags, State;

    /* get handle to menu */
    hMainMenu = GetMenu(Info->hMainWnd);

    /* set all to greyed */
    EnableMenuItem(hMainMenu, ID_START, MF_GRAYED);
    EnableMenuItem(hMainMenu, ID_STOP, MF_GRAYED);
    EnableMenuItem(hMainMenu, ID_PAUSE, MF_GRAYED);
    EnableMenuItem(hMainMenu, ID_RESUME, MF_GRAYED);
    EnableMenuItem(hMainMenu, ID_RESTART, MF_GRAYED);

    EnableMenuItem(Info->hShortcutMenu, ID_START, MF_GRAYED);
    EnableMenuItem(Info->hShortcutMenu, ID_STOP, MF_GRAYED);
    EnableMenuItem(Info->hShortcutMenu, ID_PAUSE, MF_GRAYED);
    EnableMenuItem(Info->hShortcutMenu, ID_RESUME, MF_GRAYED);
    EnableMenuItem(Info->hShortcutMenu, ID_RESTART, MF_GRAYED);

    SendMessage(Info->hTool, TB_SETSTATE, ID_START,
                   (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
    SendMessage(Info->hTool, TB_SETSTATE, ID_STOP,
                   (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
    SendMessage(Info->hTool, TB_SETSTATE, ID_PAUSE,
                   (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
    SendMessage(Info->hTool, TB_SETSTATE, ID_RESTART,
                   (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));

    if (Info->SelectedItem != NO_ITEM_SELECTED)
    {
        /* get pointer to selected service */
        Service = GetSelectedService(Info);

        Flags = Service->ServiceStatusProcess.dwControlsAccepted;
        State = Service->ServiceStatusProcess.dwCurrentState;

        if (State == SERVICE_STOPPED)
        {
            EnableMenuItem(hMainMenu, ID_START, MF_ENABLED);
            EnableMenuItem(Info->hShortcutMenu, ID_START, MF_ENABLED);
            SendMessage(Info->hTool, TB_SETSTATE, ID_START,
                   (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
        }

        if ( (Flags & SERVICE_ACCEPT_STOP) && (State == SERVICE_RUNNING) )
        {
            EnableMenuItem(hMainMenu, ID_STOP, MF_ENABLED);
            EnableMenuItem(Info->hShortcutMenu, ID_STOP, MF_ENABLED);
            SendMessage(Info->hTool, TB_SETSTATE, ID_STOP,
                   (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
        }

        if ( (Flags & SERVICE_ACCEPT_PAUSE_CONTINUE) && (State == SERVICE_RUNNING) )
        {
            EnableMenuItem(hMainMenu, ID_PAUSE, MF_ENABLED);
            EnableMenuItem(Info->hShortcutMenu, ID_PAUSE, MF_ENABLED);
            SendMessage(Info->hTool, TB_SETSTATE, ID_PAUSE,
                   (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
        }

        if ( (Flags & SERVICE_ACCEPT_STOP) && (State == SERVICE_RUNNING) )
        {
            EnableMenuItem(hMainMenu, ID_RESTART, MF_ENABLED);
            EnableMenuItem(Info->hShortcutMenu, ID_RESTART, MF_ENABLED);
            SendMessage(Info->hTool, TB_SETSTATE, ID_RESTART,
                   (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
        }
    }
    else
    {
        EnableMenuItem(hMainMenu, ID_PROP, MF_GRAYED);
        EnableMenuItem(hMainMenu, ID_DELETE, MF_GRAYED);
        EnableMenuItem(Info->hShortcutMenu, ID_DELETE, MF_GRAYED);
        SendMessage(Info->hTool, TB_SETSTATE, ID_PROP,
                   (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
    }

}


INT CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    ENUM_SERVICE_STATUS_PROCESS *Param1;
    ENUM_SERVICE_STATUS_PROCESS *Param2;
//    INT iSubItem = (LPARAM)lParamSort;

    if (bSortAscending) {
        Param1 = (ENUM_SERVICE_STATUS_PROCESS *)lParam1;
        Param2 = (ENUM_SERVICE_STATUS_PROCESS *)lParam2;
    }
    else
    {
        Param1 = (ENUM_SERVICE_STATUS_PROCESS *)lParam2;
        Param2 = (ENUM_SERVICE_STATUS_PROCESS *)lParam1;
    }
    return _tcsicmp(Param1->lpDisplayName, Param2->lpDisplayName);
}


/*FIXME: needs rewriting / optimising */
static VOID CALLBACK
MainWndResize(PVOID Context,
              WORD cx,
              WORD cy)
{
    PMAIN_WND_INFO Info = (PMAIN_WND_INFO)Context;

    RECT rcClient, rcTool, rcStatus;
    int lvHeight, iToolHeight, iStatusHeight;

    /* Size toolbar and get height */
    Info->hTool = GetDlgItem(Info->hMainWnd, IDC_TOOLBAR);
    SendMessage(Info->hTool, TB_AUTOSIZE, 0, 0);

    GetWindowRect(Info->hTool, &rcTool);
    iToolHeight = rcTool.bottom - rcTool.top;

    /* Size status bar and get height */
    Info->hStatus = GetDlgItem(Info->hMainWnd, IDC_STATUSBAR);
    SendMessage(Info->hStatus, WM_SIZE, 0, 0);

    GetWindowRect(Info->hStatus, &rcStatus);
    iStatusHeight = rcStatus.bottom - rcStatus.top;

    /* Calculate remaining height and size list view */
    GetClientRect(Info->hMainWnd, &rcClient);

    lvHeight = rcClient.bottom - iToolHeight - iStatusHeight;

    Info->hListView = GetDlgItem(Info->hMainWnd, IDC_SERVLIST);
    SetWindowPos(Info->hListView, NULL, 0, iToolHeight, rcClient.right, lvHeight, SWP_NOZORDER);
}


static VOID
CreateToolbar(PMAIN_WND_INFO Info)
{
    TBADDBITMAP tbab;
    INT iImageOffset;
    INT NumButtons;

    Info->hTool = CreateWindowEx(0, //WS_EX_TOOLWINDOW
                                 TOOLBARCLASSNAME,
                                 NULL,
                                 WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
                                 0, 0, 0, 0,
                                 Info->hMainWnd,
                                 (HMENU)IDC_TOOLBAR,
                                 hInstance,
                                 NULL);
    if(Info->hTool == NULL)
    {
        MessageBox(Info->hMainWnd,
                   _T("Could not create tool bar."),
                   _T("Error"),
                   MB_OK | MB_ICONERROR);
    }

    /* Send the TB_BUTTONSTRUCTSIZE message, which is required for backward compatibility */
    SendMessage(Info->hTool,
                TB_BUTTONSTRUCTSIZE,
                sizeof(TBBUTTON),
                0);

    NumButtons = sizeof(tbb) / sizeof(tbb[0]);

    /* Add custom images */
    tbab.hInst = hInstance;
    tbab.nID = IDB_BUTTONS;
    iImageOffset = (INT)SendMessage(Info->hTool,
                                    TB_ADDBITMAP,
                                    NumButtons,
                                    (LPARAM)&tbab);
    tbb[0].iBitmap += iImageOffset; /* properties */
    tbb[1].iBitmap += iImageOffset; /* refresh */
    tbb[2].iBitmap += iImageOffset; /* export */
    tbb[4].iBitmap += iImageOffset; /* create */
    tbb[5].iBitmap += iImageOffset; /* delete */
    tbb[7].iBitmap += iImageOffset; /* start */
    tbb[8].iBitmap += iImageOffset; /* stop */
    tbb[9].iBitmap += iImageOffset; /* pause */
    tbb[10].iBitmap += iImageOffset; /* restart */
    tbb[12].iBitmap += iImageOffset; /* help */
    tbb[13].iBitmap += iImageOffset; /* exit */

    /* Add buttons to toolbar */
    SendMessage(Info->hTool,
                TB_ADDBUTTONS,
                NumButtons,
                (LPARAM) &tbb);

    /* Show toolbar */
    ShowWindow(Info->hTool,
               SW_SHOWNORMAL);
}


static BOOL
CreateListView(PMAIN_WND_INFO Info)
{
    LVCOLUMN lvc = { 0 };
    TCHAR szTemp[256];

    Info->hListView = CreateWindowEx(0,
                                     WC_LISTVIEW,
                                     NULL,
                                     WS_CHILD | WS_VISIBLE | LVS_REPORT | WS_BORDER |
                                        LBS_NOTIFY | LVS_SORTASCENDING | LBS_NOREDRAW,
                                     0, 0, 0, 0,
                                     Info->hMainWnd,
                                     (HMENU) IDC_SERVLIST,
                                     hInstance,
                                     NULL);
    if (Info->hListView == NULL)
    {
        MessageBox(Info->hMainWnd,
                   _T("Could not create List View."),
                   _T("Error"),
                   MB_OK | MB_ICONERROR);
        return FALSE;
    }

    ListView_SetExtendedListViewStyle(Info->hListView,
                                      LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);/*LVS_EX_GRIDLINES |*/

    lvc.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH  | LVCF_FMT;
    lvc.fmt  = LVCFMT_LEFT;

    /* Add columns to the list-view */

    /* name */
    lvc.iSubItem = 0;
    lvc.cx       = 150;
    LoadString(hInstance,
               IDS_FIRSTCOLUMN,
               szTemp,
               sizeof(szTemp) / sizeof(TCHAR));
    lvc.pszText  = szTemp;
    ListView_InsertColumn(Info->hListView,
                          0,
                          &lvc);

    /* description */
    lvc.iSubItem = 1;
    lvc.cx       = 240;
    LoadString(hInstance,
               IDS_SECONDCOLUMN,
               szTemp,
               sizeof(szTemp) / sizeof(TCHAR));
    lvc.pszText  = szTemp;
    ListView_InsertColumn(Info->hListView,
                          1,
                          &lvc);

    /* status */
    lvc.iSubItem = 2;
    lvc.cx       = 55;
    LoadString(hInstance,
               IDS_THIRDCOLUMN,
               szTemp,
               sizeof(szTemp) / sizeof(TCHAR));
    lvc.pszText  = szTemp;
    ListView_InsertColumn(Info->hListView,
                          2,
                          &lvc);

    /* startup type */
    lvc.iSubItem = 3;
    lvc.cx       = 80;
    LoadString(hInstance,
               IDS_FOURTHCOLUMN,
               szTemp,
               sizeof(szTemp) / sizeof(TCHAR));
    lvc.pszText  = szTemp;
    ListView_InsertColumn(Info->hListView,
                          3,
                          &lvc);

    /* logon as */
    lvc.iSubItem = 4;
    lvc.cx       = 100;
    LoadString(hInstance,
               IDS_FITHCOLUMN,
               szTemp,
               sizeof(szTemp) / sizeof(TCHAR));
    lvc.pszText  = szTemp;
    ListView_InsertColumn(Info->hListView,
                          4,
                          &lvc);

    return TRUE;
}

static VOID
CreateStatusBar(PMAIN_WND_INFO Info)
{
    INT StatWidths[] = {110, -1}; /* widths of status bar */

    Info->hStatus = CreateWindowEx(0,
                                   STATUSCLASSNAME,
                                   NULL,
                                   WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                                   0, 0, 0, 0,
                                   Info->hMainWnd,
                                   (HMENU)IDC_STATUSBAR,
                                   hInstance,
                                   NULL);
    if(Info->hStatus == NULL)
    {
        MessageBox(Info->hMainWnd,
                   _T("Could not create status bar."),
                   _T("Error!"),
                   MB_OK | MB_ICONERROR);
        return;
    }

    SendMessage(Info->hStatus,
                SB_SETPARTS,
                sizeof(StatWidths) / sizeof(INT),
                (LPARAM)StatWidths);
}


static VOID
InitMainWnd(PMAIN_WND_INFO Info)
{
    CreateToolbar(Info);

    if (!CreateListView(Info))
        return;

    CreateStatusBar(Info);

    /* Create Popup Menu */
    Info->hShortcutMenu = LoadMenu(hInstance,
                                   MAKEINTRESOURCE(IDR_POPUP));
    Info->hShortcutMenu = GetSubMenu(Info->hShortcutMenu,
                                     0);
}


static VOID
MainWndCommand(PMAIN_WND_INFO Info,
               WORD CmdId,
               HWND hControl)
{
    UNREFERENCED_PARAMETER(hControl);

    switch (CmdId)
    {
        case ID_PROP:
        {
            if (Info->SelectedItem != NO_ITEM_SELECTED)
            {
                PPROP_DLG_INFO PropSheet;

                PropSheet = HeapAlloc(ProcessHeap,
                                      HEAP_ZERO_MEMORY,
                                      sizeof(PROP_DLG_INFO));
                if (PropSheet != NULL)
                {
                    Info->PropSheet = PropSheet;
                    OpenPropSheet(Info);
                }

                HeapFree(ProcessHeap,
                         0,
                         PropSheet);
            }
        }
        break;

        case ID_REFRESH:
        {
            RefreshServiceList(Info);
            Info->SelectedItem = NO_ITEM_SELECTED;

            /* disable menus and buttons */
            SetMenuAndButtonStates(Info);

            /* clear the service in the status bar */
            SendMessage(Info->hStatus,
                        SB_SETTEXT,
                        1,
                        _T('\0'));
        }
        break;

        case ID_EXPORT:
        {
            ExportFile(Info);
            SetFocus(Info->hListView);
        }
        break;

        case ID_CREATE:
        {
            DialogBoxParam(hInstance,
                           MAKEINTRESOURCE(IDD_DLG_CREATE),
                           Info->hMainWnd,
                           (DLGPROC)CreateDialogProc,
                           (LPARAM)Info);

            SetFocus(Info->hListView);
        }
        break;

        case ID_DELETE:
        {
            ENUM_SERVICE_STATUS_PROCESS *Service = NULL;

            Service = GetSelectedService(Info);

            if (Service->ServiceStatusProcess.dwCurrentState != SERVICE_RUNNING)
            {
                DialogBoxParam(hInstance,
                               MAKEINTRESOURCE(IDD_DLG_DELETE),
                               Info->hMainWnd,
                               (DLGPROC)DeleteDialogProc,
                               (LPARAM)Info);
            }
            else
            {
                TCHAR Buf[60];
                LoadString(hInstance,
                           IDS_DELETE_STOP,
                           Buf,
                           sizeof(Buf) / sizeof(TCHAR));
                DisplayString(Buf);
            }

            SetFocus(Info->hListView);

        }
        break;

        case ID_START:
        {
            DoStart(Info);
        }
        break;

        case ID_STOP:
        {
            DoStop(Info);
        }
        break;

        case ID_PAUSE:
        {
            Control(Info,
                    SERVICE_CONTROL_PAUSE);
        }
        break;

        case ID_RESUME:
        {
            Control(Info,
                    SERVICE_CONTROL_CONTINUE );
        }
        break;

        case ID_RESTART:
        {
            /* FIXME: remove this hack */
            SendMessage(Info->hMainWnd,
                        WM_COMMAND,
                        0,
                        ID_STOP);
            SendMessage(Info->hMainWnd,
                        WM_COMMAND,
                        0,
                        ID_START);
        }
        break;

        case ID_HELP:
            MessageBox(NULL,
                       _T("Help is not yet implemented\n"),
                       _T("Note!"),
                       MB_OK | MB_ICONINFORMATION);
            SetFocus(Info->hListView);
        break;

        case ID_EXIT:
            PostMessage(Info->hMainWnd,
                        WM_CLOSE,
                        0,
                        0);
        break;

        case ID_VIEW_LARGE:
            SetView(Info->hListView,
                    LVS_ICON);
        break;

        case ID_VIEW_SMALL:
            SetView(Info->hListView,
                    LVS_SMALLICON);
        break;

        case ID_VIEW_LIST:
            SetView(Info->hListView,
                    LVS_LIST);
        break;

        case ID_VIEW_DETAILS:
            SetView(Info->hListView,
                    LVS_REPORT);
        break;

        case ID_VIEW_CUSTOMIZE:
        break;

        case ID_ABOUT:
            DialogBox(hInstance,
                      MAKEINTRESOURCE(IDD_ABOUTBOX),
                      Info->hMainWnd,
                      (DLGPROC)AboutDialogProc);
            SetFocus(Info->hListView);
        break;

    }
}


static LRESULT CALLBACK
MainWndProc(HWND hwnd,
            UINT msg,
            WPARAM wParam,
            LPARAM lParam)
{
    PMAIN_WND_INFO Info;
    LRESULT Ret = 0;

    /* Get the window context */
    Info = (PMAIN_WND_INFO)GetWindowLongPtr(hwnd,
                                            GWLP_USERDATA);
    if (Info == NULL && msg != WM_CREATE)
    {
        goto HandleDefaultMessage;
    }

    switch(msg)
    {
        case WM_CREATE:
        {
            Info = (PMAIN_WND_INFO)(((LPCREATESTRUCT)lParam)->lpCreateParams);

            /* Initialize the main window context */
            Info->hMainWnd = hwnd;

            SetWindowLongPtr(hwnd,
                             GWLP_USERDATA,
                             (LONG_PTR)Info);

            InitMainWnd(Info);

            /* Show the window */
            ShowWindow(hwnd,
                       Info->nCmdShow);

            RefreshServiceList(Info);
        }
        break;

	    case WM_SIZE:
	    {
            MainWndResize(Info,
                          LOWORD(lParam),
                          HIWORD(lParam));
	    }
	    break;

	    case WM_NOTIFY:
        {
            LPNMHDR pnmhdr = (LPNMHDR)lParam;

            switch (pnmhdr->code)
            {
	            case NM_DBLCLK:
	            {
	                POINT pt;
	                RECT rect;

	                GetCursorPos(&pt);
	                GetWindowRect(Info->hListView, &rect);

	                if (PtInRect(&rect, pt))
	                {
                        SendMessage(hwnd,
                                    WM_COMMAND,
                                    //ID_PROP,
                                    MAKEWPARAM((WORD)ID_PROP, (WORD)0),
                                    0);
	                }

                    //OpenPropSheet(Info);
	            }
			    break;

			    case LVN_COLUMNCLICK:
			    {
                    LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;

                    ListView_SortItems(Info->hListView,
                                       CompareFunc,
                                       pnmv->iSubItem);
                    bSortAscending = !bSortAscending;
			    }
                break;

			    case LVN_ITEMCHANGED:
			    {
			        LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;
			        ENUM_SERVICE_STATUS_PROCESS *Service = NULL;
			        HMENU hMainMenu;

                    /* get handle to menu */
                    hMainMenu = GetMenu(Info->hMainWnd);

                    /* activate properties menu item, if not already */
                    if (GetMenuState(hMainMenu,
                                     ID_PROP,
                                     MF_BYCOMMAND) != MF_ENABLED)
                        EnableMenuItem(hMainMenu,
                                       ID_PROP,
                                       MF_ENABLED);

                    /* activate delete menu item, if not already */
                    if (GetMenuState(hMainMenu,
                                     ID_DELETE,
                                     MF_BYCOMMAND) != MF_ENABLED)
                    {
                        EnableMenuItem(hMainMenu,
                                       ID_DELETE,
                                       MF_ENABLED);
                        EnableMenuItem(Info->hShortcutMenu,
                                       ID_DELETE,
                                       MF_ENABLED);
                    }


                    /* globally set selected service */
			        Info->SelectedItem = pnmv->iItem;

                    /* alter options for the service */
			        SetMenuAndButtonStates(Info);

			        /* get pointer to selected service */
                    Service = GetSelectedService(Info);

			        /* set current selected service in the status bar */
                    SendMessage(Info->hStatus,
                                SB_SETTEXT,
                                1,
                                (LPARAM)Service->lpDisplayName);

                    /* show the properties button */
                    SendMessage(Info->hTool,
                                TB_SETSTATE,
                                ID_PROP,
                                (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));

			    }
			    break;

                case TTN_GETDISPINFO:
                {
                    LPTOOLTIPTEXT lpttt;
                    UINT idButton;

                    lpttt = (LPTOOLTIPTEXT)lParam;

                    /* Specify the resource identifier of the descriptive
                     * text for the given button. */
                    idButton = (UINT)lpttt->hdr.idFrom;
                    switch (idButton)
                    {
                        case ID_PROP:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_PROP);
                        break;

                        case ID_REFRESH:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_REFRESH);
                        break;

                        case ID_EXPORT:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_EXPORT);
                        break;

                        case ID_CREATE:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_CREATE);
                        break;

                        case ID_DELETE:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_DELETE);
                        break;

                        case ID_START:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_START);
                        break;

                        case ID_STOP:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_STOP);
                        break;

                        case ID_PAUSE:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_PAUSE);
                        break;

                        case ID_RESTART:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_RESTART);
                        break;

                        case ID_HELP:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_HELP);
                        break;

                        case ID_EXIT:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_EXIT);
                        break;

                    }
                }
                break;

                default:
                break;
            }
        }
        break;

        case WM_CONTEXTMENU:
            {
                POINT pt;
                RECT lvRect;

                INT xPos = GET_X_LPARAM(lParam);
                INT yPos = GET_Y_LPARAM(lParam);

                GetCursorPos(&pt);

                /* display popup when cursor is in the list view */
                GetWindowRect(Info->hListView, &lvRect);
                if (PtInRect(&lvRect, pt))
                {
                    TrackPopupMenuEx(Info->hShortcutMenu,
                                     TPM_RIGHTBUTTON,
                                     xPos,
                                     yPos,
                                     Info->hMainWnd,
                                     NULL);
                }
            }
        break;

        case WM_COMMAND:
        {
            MainWndCommand(Info,
                           LOWORD(wParam),
                           (HWND)lParam);
            goto HandleDefaultMessage;
        }

	    case WM_CLOSE:
	    {
            /* Free service array */
            HeapFree(ProcessHeap,
                     0,
                     Info->pServiceStatus);

            DestroyMenu(Info->hShortcutMenu);
		    DestroyWindow(hwnd);
	    }
	    break;

	    case WM_DESTROY:
        {
            //DestroyMainWnd(Info);

            HeapFree(ProcessHeap,
                     0,
                     Info);
            SetWindowLongPtr(hwnd,
                             GWLP_USERDATA,
                             0);

            /* Break the message queue loop */
            PostQuitMessage(0);
        }
	    break;

	    default:
	    {
HandleDefaultMessage:

            Ret = DefWindowProc(hwnd,
                                msg,
                                wParam,
                                lParam);
	    }
	    break;
    }
    return Ret;
}



HWND
CreateMainWindow(LPCTSTR lpCaption,
                 int nCmdShow)
{
    PMAIN_WND_INFO Info;
    HWND hMainWnd = NULL;

    Info = HeapAlloc(ProcessHeap,
                     HEAP_ZERO_MEMORY,
                     sizeof(MAIN_WND_INFO));

    if (Info != NULL)
    {
        Info->nCmdShow = nCmdShow;

        hMainWnd = CreateWindowEx(WS_EX_WINDOWEDGE,
                                  szMainWndClass,
                                  lpCaption,
                                  WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  650,
                                  450,
                                  NULL,
                                  NULL,
                                  hInstance,
                                  Info);
        if (hMainWnd == NULL)
        {
            int ret;
            ret = GetLastError();
            GetError();
            HeapFree(ProcessHeap,
                     0,
                     Info);
        }
    }

    return hMainWnd;
}

BOOL
InitMainWindowImpl(VOID)
{
    WNDCLASSEX wc = {0};

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance,
                        MAKEINTRESOURCE(IDI_SM_ICON));
    wc.hCursor = LoadCursor(NULL,
                            IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MAINMENU);
    wc.lpszClassName = szMainWndClass;
    wc.hIconSm = (HICON)LoadImage(hInstance,
                                  MAKEINTRESOURCE(IDI_SM_ICON),
                                  IMAGE_ICON,
                                  16,
                                  16,
                                  LR_SHARED);

    return RegisterClassEx(&wc) != (ATOM)0;
}


VOID
UninitMainWindowImpl(VOID)
{
    UnregisterClass(szMainWndClass,
                    hInstance);
}

