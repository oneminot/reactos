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
 // desktop.h
 //
 // Martin Fuchs, 09.08.2003
 //


 /// subclassed Background window behind the visible desktop window
struct BackgroundWindow : public SubclassedWindow
{
	typedef SubclassedWindow super;

	BackgroundWindow(HWND hwnd) : super(hwnd) {}

protected:
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
};


 /// Implementation of the Explorer desktop window
struct DesktopWindow : public Window, public IShellBrowserImpl
{
	typedef Window super;

	DesktopWindow(HWND hwnd);
	~DesktopWindow();

	static HWND Create();

	virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND* lphwnd)
	{
		*lphwnd = _hwnd;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE QueryActiveShellView(IShellView** ppshv)
	{
		_pShellView->AddRef();
		*ppshv = _pShellView;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetControlWindow(UINT id, HWND* lphwnd)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE SendControlMsg(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pret)
	{
		return E_NOTIMPL;
	}

protected:
	LRESULT	Init(LPCREATESTRUCT pcs);
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);

	IShellView*	_pShellView;
	WindowHandle _desktopBar;

	virtual HRESULT OnDefaultCommand(LPIDA pida);
};


 /// subclassed ShellView window
struct DesktopShellView : public SubclassedWindow
{
	typedef SubclassedWindow super;

	DesktopShellView(HWND hwnd, IShellView* pShellView);

protected:
	IShellView* _pShellView;

	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int		Command(int id, int code);
	int		Notify(int id, NMHDR* pnmh);

	bool	DoContextMenu(int x, int y);
};
