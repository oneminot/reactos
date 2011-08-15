/* Some typedefs for appearance */

/* This is the global structure used to store the current values.
   A pointer of this get's passed to the functions either directly
   or by passing hwnd and getting the pointer by GetWindowLongPtr */
typedef struct tagGLOBALS
{
	INT SchemeId;	/* Theme is customized if SchemeId == -1 */
	COLOR_SCHEME Scheme;
	COLOR_SCHEME SchemeAdv;
	BOOL bHasChanged;
	HBITMAP hbmpColor[3];
	INT CurrentElement;
	HFONT hBoldFont;
	HFONT hItalicFont;
    BOOL bInitializing;
} GLOBALS;

/* prototypes for appearance.c */
INT_PTR CALLBACK AppearancePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* prototypes for advappdlg.c */
INT_PTR CALLBACK AdvAppearanceDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* prototypes for effappdlg.c */
INT_PTR CALLBACK EffAppearanceDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
