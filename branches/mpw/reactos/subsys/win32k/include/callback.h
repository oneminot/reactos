#ifndef __SUBSYS_WIN32K_INCLUDE_CALLBACK_H
#define __SUBSYS_WIN32K_INCLUDE_CALLBACK_H

LRESULT STDCALL
W32kCallWindowProc(WNDPROC Proc,
		   HWND Wnd,
		   UINT Message,
		   WPARAM wParam,
		   LPARAM lParam);
LRESULT STDCALL
W32kSendNCCREATEMessage(HWND Wnd, CREATESTRUCT* CreateStruct);
LRESULT STDCALL
W32kSendCREATEMessage(HWND Wnd, CREATESTRUCT* CreateStruct);

#endif /* __SUBSYS_WIN32K_INCLUDE_CALLBACK_H */
