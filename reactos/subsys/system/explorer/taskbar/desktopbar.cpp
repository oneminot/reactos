/*
 * Copyright 2003 Martin Fuchs
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


 //
 // Explorer clone
 //
 // desktopbar.cpp
 //
 // Martin Fuchs, 22.08.2003
 //


#include "../utility/utility.h"

#include "../explorer.h"
#include "../globals.h"
#include "../externals.h"
#include "../explorer_intres.h"

#include "desktopbar.h"
#include "taskbar.h"
#include "startmenu.h"
#include "traynotify.h"


HWND InitializeExplorerBar(HINSTANCE hInstance)
{
	RECT rect;

	rect.left = -2;	// hide left border
#ifdef TASKBAR_AT_TOP
	rect.top = -2;	// hide top border
#else
	rect.top = GetSystemMetrics(SM_CYSCREEN) - TASKBAR_HEIGHT;
#endif
	rect.right = GetSystemMetrics(SM_CXSCREEN) + 2;
	rect.bottom = rect.top + TASKBAR_HEIGHT + 2;

	return Window::Create(WINDOW_CREATOR(DesktopBar), WS_EX_PALETTEWINDOW,
							BtnWindowClass(CLASSNAME_EXPLORERBAR), TITLE_EXPLORERBAR,
							WS_POPUP|WS_THICKFRAME|WS_CLIPCHILDREN|WS_VISIBLE,
							rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, 0);
}


DesktopBar::DesktopBar(HWND hwnd)
 :	super(hwnd),
	WM_TASKBARCREATED(RegisterWindowMessage(WINMSG_TASKBARCREATED))
{
}

DesktopBar::~DesktopBar()
{
	 // exit application after destroying desktop window
	PostQuitMessage(0);
}


LRESULT DesktopBar::Init(LPCREATESTRUCT pcs)
{
	if (super::Init(pcs))
		return 1;

	 // create start button
	new PictureButton(Button(_hwnd, ResString(IDS_START), 2, 2, STARTBUTTON_WIDTH, TASKBAR_HEIGHT-8, IDC_START, WS_VISIBLE|WS_CHILD|BS_OWNERDRAW),
						SmallIcon(IDI_STARTMENU));

	ClientRect clnt(_hwnd);

	 // create task bar
	_hwndTaskBar = Window::Create(WINDOW_CREATOR(TaskBar), 0,
							BtnWindowClass(CLASSNAME_TASKBAR), TITLE_TASKBAR, WS_CHILD|WS_VISIBLE,
							TASKBAR_LEFT, 0, clnt.right-TASKBAR_LEFT-(NOTIFYAREA_WIDTH+1), TASKBAR_HEIGHT, _hwnd);

	TaskBar* taskbar = static_cast<TaskBar*>(Window::get_window(_hwndTaskBar));
	taskbar->_desktop_bar = this;

	 // create tray notification area
	_hwndNotify = Window::Create(WINDOW_CREATOR(NotifyArea), WS_EX_STATICEDGE,
							BtnWindowClass(CLASSNAME_TRAYNOTIFY,CS_DBLCLKS), TITLE_TRAYNOTIFY, WS_CHILD|WS_VISIBLE,
							clnt.right-(NOTIFYAREA_WIDTH+1), 1, NOTIFYAREA_WIDTH, clnt.bottom-2, _hwnd);

//	NotifyArea* notify_area = static_cast<NotifyArea*>(Window::get_window(_hwndNotify));
//	notify_area->_desktop_bar = this;

	RegisterHotkeys();

	 // notify all top level windows about the successfully created desktop bar
	PostMessage(HWND_BROADCAST, WM_TASKBARCREATED, 0, 0);

	return 0;
}


void DesktopBar::RegisterHotkeys()
{
	 // register hotkey CTRL+ESC for opening Startmenu
	RegisterHotKey(_hwnd, 0, MOD_CONTROL, VK_ESCAPE);

	 // register hotkey WIN+E opening explorer
	RegisterHotKey(_hwnd, 1, MOD_WIN, 'E');

		//TODO: register all common hotkeys
}

void DesktopBar::ProcessHotKey(int id_hotkey)
{
	switch(id_hotkey) {
	  case 0:	ToggleStartmenu();							break;
	  case 1:	explorer_show_frame(_hwnd, SW_SHOWNORMAL);	break;
		//TODO: implement all common hotkeys
	}
}


LRESULT DesktopBar::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_NCHITTEST: {
		LRESULT res = super::WndProc(nmsg, wparam, lparam);

		if (res>=HTSIZEFIRST && res<=HTSIZELAST) {
#ifdef TASKBAR_AT_TOP
			if (res == HTBOTTOM)	// enable vertical resizing at the lower border
#else
			if (res == HTTOP)		// enable vertical resizing at the upper border
#endif
				return res;
			else
				return HTCLIENT;	// disable any other resizing
		}
		return res;}

	  case WM_SYSCOMMAND:
		if ((wparam&0xFFF0) == SC_SIZE) {
#ifdef TASKBAR_AT_TOP
			if (wparam == SC_SIZE+6)// enable vertical resizing at the lower border
#else
			if (wparam == SC_SIZE+3)// enable vertical resizing at the upper border
#endif
				goto def;
			else
				return 0;			// disable any other resizing
		}
		goto def;

	  case WM_SIZE: {
		ClientRect clnt(_hwnd);
		int cy = HIWORD(lparam);

		if (_hwndTaskBar)
			MoveWindow(_hwndTaskBar, TASKBAR_LEFT, 0, clnt.right-TASKBAR_LEFT-(NOTIFYAREA_WIDTH+1), cy, TRUE);

		if (_hwndNotify)
			MoveWindow(_hwndNotify, clnt.right-(NOTIFYAREA_WIDTH+1), 1, NOTIFYAREA_WIDTH, cy-2, TRUE);
		break;}

	  case WM_CLOSE:
		break;

	  case PM_STARTMENU_CLOSED:
		_startMenuRoot = 0;
		break;

	  case WM_SETFOCUS:
		CloseStartMenu();
		goto def;

	  case WM_HOTKEY:
		ProcessHotKey(wparam);
		break;

	  case WM_COPYDATA:
		return ProcessCopyData((COPYDATASTRUCT*)lparam);

	  default: def:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}


int DesktopBar::Command(int id, int code)
{
	switch(id) {
	  case IDC_START:	//TODO: startmenu should popup for WM_LBUTTONDOWN, not for WM_COMMAND
		ToggleStartmenu();
		break;
	}

	return 0;
}


void DesktopBar::ToggleStartmenu()
{
	if (_startMenuRoot && IsWindow(_startMenuRoot)) {	// IsWindow(): safety first
		 // dispose Startmenu
		DestroyWindow(_startMenuRoot);
		_startMenuRoot = 0;
	} else {
		 // create Startmenu
		_startMenuRoot = StartMenuRoot::Create(_hwnd);
	}
}

void DesktopBar::CloseStartMenu()
{
	if (_startMenuRoot) {
		DestroyWindow(_startMenuRoot);

		_startMenuRoot = 0;
	}
}


 /// copy data structure for tray notifications
struct TrayNotifyCDS {
	DWORD	cookie;
	DWORD	notify_code;
	NOTIFYICONDATA nicon_data;
};

LRESULT DesktopBar::ProcessCopyData(COPYDATASTRUCT* pcd)
{
	 // Is this a tray notification message?
	if (pcd->dwData == 1) {
		TrayNotifyCDS* ptr = (TrayNotifyCDS*) pcd->lpData;

		NotifyArea* notify_area = static_cast<NotifyArea*>(Window::get_window(_hwndNotify));

		if (notify_area)
			return notify_area->ProcessTrayNotification(ptr->notify_code, &ptr->nicon_data);
	}

	return 0;
}
