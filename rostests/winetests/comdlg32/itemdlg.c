/*
 * Unit tests for the Common Item Dialog
 *
 * Copyright 2010,2011 David Hedberg
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#define COBJMACROS
#define CONST_VTABLE

#include "shlobj.h"
#include "wine/test.h"

static HRESULT (WINAPI *pSHCreateShellItem)(LPCITEMIDLIST,IShellFolder*,LPCITEMIDLIST,IShellItem**);
static HRESULT (WINAPI *pSHGetIDListFromObject)(IUnknown*, PIDLIST_ABSOLUTE*);
static HRESULT (WINAPI *pSHCreateItemFromParsingName)(PCWSTR,IBindCtx*,REFIID,void**);

static void init_function_pointers(void)
{
    HMODULE hmod = GetModuleHandleA("shell32.dll");

#define MAKEFUNC(f) (p##f = (void*)GetProcAddress(hmod, #f))
    MAKEFUNC(SHCreateShellItem);
    MAKEFUNC(SHGetIDListFromObject);
    MAKEFUNC(SHCreateItemFromParsingName);
#undef MAKEFUNC
}

#include <initguid.h>
DEFINE_GUID(IID_IFileDialogCustomizeAlt, 0x8016B7B3, 0x3D49, 0x4504, 0xA0,0xAA, 0x2A,0x37,0x49,0x4E,0x60,0x6F);

/**************************************************************************
 * IFileDialogEvents implementation
 */
typedef struct {
    IFileDialogEvents IFileDialogEvents_iface;
    LONG ref;
    LONG QueryInterface;
    LONG OnFileOk, OnFolderChanging, OnFolderChange;
    LONG OnSelectionChange, OnShareViolation, OnTypeChange;
    LONG OnOverwrite;
    LPCWSTR set_filename;
    BOOL set_filename_tried;
    BOOL cfd_test1;
} IFileDialogEventsImpl;

static inline IFileDialogEventsImpl *impl_from_IFileDialogEvents(IFileDialogEvents *iface)
{
    return CONTAINING_RECORD(iface, IFileDialogEventsImpl, IFileDialogEvents_iface);
}

static HRESULT WINAPI IFileDialogEvents_fnQueryInterface(IFileDialogEvents *iface, REFIID riid, void **ppv)
{
    /* Not called. */
    ok(0, "Unexpectedly called.\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI IFileDialogEvents_fnAddRef(IFileDialogEvents *iface)
{
    IFileDialogEventsImpl *This = impl_from_IFileDialogEvents(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IFileDialogEvents_fnRelease(IFileDialogEvents *iface)
{
    IFileDialogEventsImpl *This = impl_from_IFileDialogEvents(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    if(!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI IFileDialogEvents_fnOnFileOk(IFileDialogEvents *iface, IFileDialog *pfd)
{
    IFileDialogEventsImpl *This = impl_from_IFileDialogEvents(iface);
    This->OnFileOk++;
    return S_OK;
}

static HRESULT WINAPI IFileDialogEvents_fnOnFolderChanging(IFileDialogEvents *iface,
                                                           IFileDialog *pfd,
                                                           IShellItem *psiFolder)
{
    IFileDialogEventsImpl *This = impl_from_IFileDialogEvents(iface);
    This->OnFolderChanging++;
    return S_OK;
}

static void test_customize_onfolderchange(IFileDialog *pfd);

static LRESULT CALLBACK test_customize_dlgproc(HWND hwnd, UINT message, LPARAM lparam, WPARAM wparam)
{
    WNDPROC oldwndproc = GetPropA(hwnd, "WT_OLDWC");

    if(message == WM_USER+0x1234)
    {
        IFileDialog *pfd = (IFileDialog*)lparam;
        test_customize_onfolderchange(pfd);
    }

    return CallWindowProcW(oldwndproc, hwnd, message, lparam, wparam);
}

static HRESULT WINAPI IFileDialogEvents_fnOnFolderChange(IFileDialogEvents *iface, IFileDialog *pfd)
{
    IFileDialogEventsImpl *This = impl_from_IFileDialogEvents(iface);
    IOleWindow *pow;
    HWND dlg_hwnd;
    HRESULT hr;
    BOOL br;
    This->OnFolderChange++;

    if(This->set_filename)
    {
        hr = IFileDialog_QueryInterface(pfd, &IID_IOleWindow, (void**)&pow);
        ok(hr == S_OK, "Got 0x%08x\n", hr);

        hr = IOleWindow_GetWindow(pow, &dlg_hwnd);
        ok(hr == S_OK, "Got 0x%08x\n", hr);
        ok(dlg_hwnd != NULL, "Got NULL.\n");

        IOleWindow_Release(pow);

        hr = IFileDialog_SetFileName(pfd, This->set_filename);
        ok(hr == S_OK, "Got 0x%08x\n", hr);

        if(!This->set_filename_tried)
        {
            br = PostMessageW(dlg_hwnd, WM_COMMAND, IDOK, 0);
            ok(br, "Failed\n");
            This->set_filename_tried = TRUE;
        }
    }

    if(This->cfd_test1)
    {
        WNDPROC oldwndproc;
        hr = IFileDialog_QueryInterface(pfd, &IID_IOleWindow, (void**)&pow);
        ok(hr == S_OK, "Got 0x%08x\n", hr);

        hr = IOleWindow_GetWindow(pow, &dlg_hwnd);
        ok(hr == S_OK, "Got 0x%08x\n", hr);
        ok(dlg_hwnd != NULL, "Got NULL.\n");

        IOleWindow_Release(pow);

        /* On Vista, the custom control area of the dialog is not
         * fully set up when the first OnFolderChange event is
         * issued. */
        oldwndproc = (WNDPROC)GetWindowLongPtrW(dlg_hwnd, GWLP_WNDPROC);
        SetPropA(dlg_hwnd, "WT_OLDWC", (HANDLE)oldwndproc);
        SetWindowLongPtrW(dlg_hwnd, GWLP_WNDPROC, (LPARAM)test_customize_dlgproc);

        br = PostMessageW(dlg_hwnd, WM_USER+0x1234, (LPARAM)pfd, 0);
        ok(br, "Failed\n");
    }

    return S_OK;
}

static HRESULT WINAPI IFileDialogEvents_fnOnSelectionChange(IFileDialogEvents *iface, IFileDialog *pfd)
{
    IFileDialogEventsImpl *This = impl_from_IFileDialogEvents(iface);
    This->OnSelectionChange++;
    return S_OK;
}

static HRESULT WINAPI IFileDialogEvents_fnOnShareViolation(IFileDialogEvents *iface,
                                                           IFileDialog *pfd,
                                                           IShellItem *psi,
                                                           FDE_SHAREVIOLATION_RESPONSE *pResponse)
{
    IFileDialogEventsImpl *This = impl_from_IFileDialogEvents(iface);
    This->OnShareViolation++;
    return S_OK;
}

static HRESULT WINAPI IFileDialogEvents_fnOnTypeChange(IFileDialogEvents *iface, IFileDialog *pfd)
{
    IFileDialogEventsImpl *This = impl_from_IFileDialogEvents(iface);
    This->OnTypeChange++;
    return S_OK;
}

static HRESULT WINAPI IFileDialogEvents_fnOnOverwrite(IFileDialogEvents *iface,
                                                      IFileDialog *pfd,
                                                      IShellItem *psi,
                                                      FDE_OVERWRITE_RESPONSE *pResponse)
{
    IFileDialogEventsImpl *This = impl_from_IFileDialogEvents(iface);
    This->OnOverwrite++;
    return S_OK;
}

static const IFileDialogEventsVtbl vt_IFileDialogEvents = {
    IFileDialogEvents_fnQueryInterface,
    IFileDialogEvents_fnAddRef,
    IFileDialogEvents_fnRelease,
    IFileDialogEvents_fnOnFileOk,
    IFileDialogEvents_fnOnFolderChanging,
    IFileDialogEvents_fnOnFolderChange,
    IFileDialogEvents_fnOnSelectionChange,
    IFileDialogEvents_fnOnShareViolation,
    IFileDialogEvents_fnOnTypeChange,
    IFileDialogEvents_fnOnOverwrite
};

static IFileDialogEvents *IFileDialogEvents_Constructor(void)
{
    IFileDialogEventsImpl *This;

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IFileDialogEventsImpl));
    This->IFileDialogEvents_iface.lpVtbl = &vt_IFileDialogEvents;
    This->ref = 1;

    return &This->IFileDialogEvents_iface;
}

static BOOL test_instantiation(void)
{
    IFileDialog *pfd;
    IFileOpenDialog *pfod;
    IFileSaveDialog *pfsd;
    IServiceProvider *psp;
    IOleWindow *pow;
    IUnknown *punk;
    HRESULT hr;
    LONG ref;

    /* Instantiate FileOpenDialog */
    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IFileOpenDialog, (void**)&pfod);
    if(FAILED(hr))
    {
        skip("Could not instantiate the FileOpenDialog.\n");
        return FALSE;
    }
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    hr = IFileOpenDialog_QueryInterface(pfod, &IID_IFileDialog, (void**)&pfd);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IFileDialog_Release(pfd);

    hr = IFileOpenDialog_QueryInterface(pfod, &IID_IFileDialogCustomize, (void**)&punk);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);

    hr = IFileOpenDialog_QueryInterface(pfod, &IID_IFileDialogCustomizeAlt, (void**)&punk);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);

    hr = IFileOpenDialog_QueryInterface(pfod, &IID_IFileSaveDialog, (void**)&pfsd);
    ok(hr == E_NOINTERFACE, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IFileSaveDialog_Release(pfsd);

    hr = IFileOpenDialog_QueryInterface(pfod, &IID_IServiceProvider, (void**)&psp);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr))
    {
        IExplorerBrowser *peb;
        IShellBrowser *psb;

        hr = IServiceProvider_QueryService(psp, &SID_STopLevelBrowser, &IID_IExplorerBrowser, (void**)&peb);
        ok(hr == E_FAIL, "got 0x%08x.\n", hr);
        if(SUCCEEDED(hr)) IExplorerBrowser_Release(peb);
        hr = IServiceProvider_QueryService(psp, &SID_STopLevelBrowser, &IID_IShellBrowser, (void**)&psb);
        ok(hr == E_FAIL, "got 0x%08x.\n", hr);
        if(SUCCEEDED(hr)) IShellBrowser_Release(psb);
        hr = IServiceProvider_QueryService(psp, &SID_STopLevelBrowser, &IID_ICommDlgBrowser, (void**)&punk);
        ok(hr == E_FAIL, "got 0x%08x.\n", hr);
        if(SUCCEEDED(hr)) IUnknown_Release(punk);
        hr = IServiceProvider_QueryService(psp, &SID_SExplorerBrowserFrame, &IID_ICommDlgBrowser, (void**)&punk);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        if(SUCCEEDED(hr)) IUnknown_Release(punk);

        IServiceProvider_Release(psp);
    }

    hr = IFileOpenDialog_QueryInterface(pfod, &IID_IFileDialogEvents, (void**)&punk);
    ok(hr == E_NOINTERFACE, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);

    hr = IFileOpenDialog_QueryInterface(pfod, &IID_IExplorerBrowser, (void**)&punk);
    ok(hr == E_NOINTERFACE, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);

    hr = IFileOpenDialog_QueryInterface(pfod, &IID_IExplorerBrowserEvents, (void**)&punk);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);

    hr = IFileOpenDialog_QueryInterface(pfod, &IID_ICommDlgBrowser3, (void**)&punk);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);

    hr = IFileOpenDialog_QueryInterface(pfod, &IID_IShellBrowser, (void**)&punk);
    ok(hr == E_NOINTERFACE, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);

    hr = IFileOpenDialog_QueryInterface(pfod, &IID_IOleWindow, (void**)&pow);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr))
    {
        HWND hwnd;

        hr = IOleWindow_ContextSensitiveHelp(pow, TRUE);
        todo_wine ok(hr == S_OK, "Got 0x%08x\n", hr);

        hr = IOleWindow_ContextSensitiveHelp(pow, FALSE);
        todo_wine ok(hr == S_OK, "Got 0x%08x\n", hr);

        if(0)
        {
            /* Crashes on win7 */
            IOleWindow_GetWindow(pow, NULL);
        }

        hr = IOleWindow_GetWindow(pow, &hwnd);
        ok(hr == S_OK, "Got 0x%08x\n", hr);
        ok(hwnd == NULL, "Got %p\n", hwnd);

        IOleWindow_Release(pow);
    }

    ref = IFileOpenDialog_Release(pfod);
    ok(!ref, "Got refcount %d, should have been released.\n", ref);

    /* Instantiate FileSaveDialog */
    hr = CoCreateInstance(&CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IFileSaveDialog, (void**)&pfsd);
    if(FAILED(hr))
    {
        skip("Could not instantiate the FileSaveDialog.\n");
        return FALSE;
    }
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    hr = IFileSaveDialog_QueryInterface(pfsd, &IID_IFileDialog, (void**)&pfd);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IFileDialog_Release(pfd);

    hr = IFileSaveDialog_QueryInterface(pfsd, &IID_IFileDialogCustomize, (void**)&punk);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);

    hr = IFileSaveDialog_QueryInterface(pfsd, &IID_IFileDialogCustomizeAlt, (void**)&punk);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);

    hr = IFileSaveDialog_QueryInterface(pfsd, &IID_IFileOpenDialog, (void**)&pfod);
    ok(hr == E_NOINTERFACE, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IFileOpenDialog_Release(pfod);

    hr = IFileSaveDialog_QueryInterface(pfsd, &IID_IFileDialogEvents, (void**)&punk);
    ok(hr == E_NOINTERFACE, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(pfd);

    hr = IFileSaveDialog_QueryInterface(pfsd, &IID_IExplorerBrowser, (void**)&punk);
    ok(hr == E_NOINTERFACE, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);

    hr = IFileSaveDialog_QueryInterface(pfsd, &IID_IExplorerBrowserEvents, (void**)&punk);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);

    hr = IFileOpenDialog_QueryInterface(pfsd, &IID_ICommDlgBrowser3, (void**)&punk);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);

    hr = IFileSaveDialog_QueryInterface(pfsd, &IID_IShellBrowser, (void**)&punk);
    ok(hr == E_NOINTERFACE, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);

    hr = IFileSaveDialog_QueryInterface(pfsd, &IID_IOleWindow, (void**)&pow);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr))
    {
        HWND hwnd;

        hr = IOleWindow_ContextSensitiveHelp(pow, TRUE);
        todo_wine ok(hr == S_OK, "Got 0x%08x\n", hr);

        hr = IOleWindow_ContextSensitiveHelp(pow, FALSE);
        todo_wine ok(hr == S_OK, "Got 0x%08x\n", hr);

        if(0)
        {
            /* Crashes on win7 */
            IOleWindow_GetWindow(pow, NULL);
        }

        hr = IOleWindow_GetWindow(pow, &hwnd);
        ok(hr == S_OK, "Got 0x%08x\n", hr);
        ok(hwnd == NULL, "Got %p\n", hwnd);

        IOleWindow_Release(pow);
    }


    ref = IFileSaveDialog_Release(pfsd);
    ok(!ref, "Got refcount %d, should have been released.\n", ref);
    return TRUE;
}

static void test_basics(void)
{
    IFileOpenDialog *pfod;
    IFileSaveDialog *pfsd;
    IFileDialog2 *pfd2;
    FILEOPENDIALOGOPTIONS fdoptions;
    IShellFolder *psfdesktop;
    IShellItem *psi, *psidesktop, *psi_original;
    IShellItemArray *psia;
    IPropertyStore *pps;
    LPITEMIDLIST pidl;
    WCHAR *filename;
    UINT filetype;
    LONG ref;
    HRESULT hr;
    const WCHAR txt[] = {'t','x','t', 0};
    const WCHAR null[] = {0};
    const WCHAR fname1[] = {'f','n','a','m','e','1', 0};
    const WCHAR fspec1[] = {'*','.','t','x','t',0};
    const WCHAR fname2[] = {'f','n','a','m','e','2', 0};
    const WCHAR fspec2[] = {'*','.','e','x','e',0};
    COMDLG_FILTERSPEC filterspec[2] = {{fname1, fspec1}, {fname2, fspec2}};

    /* This should work on every platform with IFileDialog */
    SHGetDesktopFolder(&psfdesktop);
    hr = pSHGetIDListFromObject((IUnknown*)psfdesktop, &pidl);
    if(SUCCEEDED(hr))
    {
        hr = pSHCreateShellItem(NULL, NULL, pidl, &psidesktop);
        ILFree(pidl);
    }
    IShellFolder_Release(psfdesktop);
    if(FAILED(hr))
    {
        skip("Failed to get ShellItem from DesktopFolder, skipping tests.\n");
        return;
    }


    /* Instantiate FileOpenDialog and FileSaveDialog */
    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IFileOpenDialog, (void**)&pfod);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = CoCreateInstance(&CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IFileSaveDialog, (void**)&pfsd);
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    /* ClearClientData */
    todo_wine
    {
    hr = IFileOpenDialog_ClearClientData(pfod);
    ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_ClearClientData(pfsd);
    ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    }

    /* GetOptions */
    hr = IFileOpenDialog_GetOptions(pfod, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_GetOptions(pfsd, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);

    /* Check default options */
    hr = IFileOpenDialog_GetOptions(pfod, &fdoptions);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    ok(fdoptions == (FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST | FOS_NOCHANGEDIR),
       "Unexpected default options: 0x%08x\n", fdoptions);
    hr = IFileSaveDialog_GetOptions(pfsd, &fdoptions);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    ok(fdoptions == (FOS_OVERWRITEPROMPT | FOS_NOREADONLYRETURN | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR),
       "Unexpected default options: 0x%08x\n", fdoptions);

    /* GetResult */
    hr = IFileOpenDialog_GetResult(pfod, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_GetResult(pfsd, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);

    psi = (void*)0xdeadbeef;
    hr = IFileOpenDialog_GetResult(pfod, &psi);
    ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);
    ok(psi == (void*)0xdeadbeef, "got %p.\n", psi);
    psi = (void*)0xdeadbeef;
    hr = IFileSaveDialog_GetResult(pfsd, &psi);
    ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);
    ok(psi == (void*)0xdeadbeef, "got %p.\n", psi);

    /* GetCurrentSelection */
    if(0) {
        /* Crashes on Vista/W2K8. Tests below passes on Windows 7 */
        hr = IFileOpenDialog_GetCurrentSelection(pfod, NULL);
        ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);
        hr = IFileSaveDialog_GetCurrentSelection(pfsd, NULL);
        ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);
        hr = IFileOpenDialog_GetCurrentSelection(pfod, &psi);
        ok(hr == E_FAIL, "got 0x%08x.\n", hr);
        hr = IFileSaveDialog_GetCurrentSelection(pfsd, &psi);
        ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    }

    /* GetFileName */
    hr = IFileOpenDialog_GetFileName(pfod, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);
    filename = (void*)0xdeadbeef;
    hr = IFileOpenDialog_GetFileName(pfod, &filename);
    ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    ok(filename == NULL, "got %p\n", filename);
    hr = IFileSaveDialog_GetFileName(pfsd, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);
    filename = (void*)0xdeadbeef;
    hr = IFileSaveDialog_GetFileName(pfsd, &filename);
    ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    ok(filename == NULL, "got %p\n", filename);

    /* GetFileTypeIndex */
    hr = IFileOpenDialog_GetFileTypeIndex(pfod, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);
    filetype = 0x12345;
    hr = IFileOpenDialog_GetFileTypeIndex(pfod, &filetype);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    ok(filetype == 0, "got %d.\n", filetype);
    hr = IFileSaveDialog_GetFileTypeIndex(pfsd, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);
    filetype = 0x12345;
    hr = IFileSaveDialog_GetFileTypeIndex(pfsd, &filetype);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    ok(filetype == 0, "got %d.\n", filetype);

    /* SetFileTypes / SetFileTypeIndex */
    hr = IFileOpenDialog_SetFileTypes(pfod, 0, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);
    hr = IFileOpenDialog_SetFileTypes(pfod, 0, filterspec);
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    hr = IFileOpenDialog_SetFileTypeIndex(pfod, -1);
    ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    hr = IFileOpenDialog_SetFileTypeIndex(pfod, 0);
    ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    hr = IFileOpenDialog_SetFileTypeIndex(pfod, 1);
    ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    hr = IFileOpenDialog_SetFileTypes(pfod, 1, filterspec);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileOpenDialog_SetFileTypes(pfod, 0, filterspec);
    ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);
    hr = IFileOpenDialog_SetFileTypeIndex(pfod, 0);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileOpenDialog_SetFileTypeIndex(pfod, 100);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileOpenDialog_SetFileTypes(pfod, 1, filterspec);
    ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);
    hr = IFileOpenDialog_SetFileTypes(pfod, 1, &filterspec[1]);
    ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);

    hr = IFileSaveDialog_SetFileTypeIndex(pfsd, -1);
    ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_SetFileTypeIndex(pfsd, 0);
    ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_SetFileTypeIndex(pfsd, 1);
    ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_SetFileTypes(pfsd, 1, filterspec);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_SetFileTypeIndex(pfsd, 0);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_SetFileTypeIndex(pfsd, 100);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_SetFileTypes(pfsd, 1, filterspec);
    ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_SetFileTypes(pfsd, 1, &filterspec[1]);
    ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);

    /* SetFilter */
    todo_wine
    {
    hr = IFileOpenDialog_SetFilter(pfod, NULL);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_SetFilter(pfsd, NULL);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    }

    /* SetFolder */
    hr = IFileOpenDialog_SetFolder(pfod, NULL);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_SetFolder(pfsd, NULL);
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    /* SetDefaultExtension */
    hr = IFileOpenDialog_SetDefaultExtension(pfod, NULL);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileOpenDialog_SetDefaultExtension(pfod, txt);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileOpenDialog_SetDefaultExtension(pfod, null);
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    hr = IFileSaveDialog_SetDefaultExtension(pfsd, NULL);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_SetDefaultExtension(pfsd, txt);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_SetDefaultExtension(pfsd, null);
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    /* SetDefaultFolder */
    hr = IFileOpenDialog_SetDefaultFolder(pfod, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetDefaultFolder(pfsd, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IFileOpenDialog_SetDefaultFolder(pfod, psidesktop);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetDefaultFolder(pfsd, psidesktop);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    if(0)
    {
        /* Crashes under Windows 7 */
        IFileOpenDialog_SetDefaultFolder(pfod, (void*)0x1234);
        IFileSaveDialog_SetDefaultFolder(pfsd, (void*)0x1234);
    }

    /* GetFolder / SetFolder */
    hr = IFileOpenDialog_GetFolder(pfod, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);

    hr = IFileOpenDialog_GetFolder(pfod, &psi_original);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IFileOpenDialog_SetFolder(pfod, psidesktop);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        hr = IFileOpenDialog_SetFolder(pfod, psi_original);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        IShellItem_Release(psi_original);
    }

    hr = IFileSaveDialog_GetFolder(pfsd, &psi_original);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IFileSaveDialog_SetFolder(pfsd, psidesktop);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        hr = IFileSaveDialog_SetFolder(pfsd, psi_original);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        IShellItem_Release(psi_original);
    }

    /* AddPlace */
    if(0)
    {
        /* Crashes under Windows 7 */
        IFileOpenDialog_AddPlace(pfod, NULL, 0);
        IFileSaveDialog_AddPlace(pfsd, NULL, 0);
    }

    todo_wine
    {
    hr = IFileOpenDialog_AddPlace(pfod, psidesktop, FDAP_TOP + 1);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    hr = IFileOpenDialog_AddPlace(pfod, psidesktop, FDAP_BOTTOM);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileOpenDialog_AddPlace(pfod, psidesktop, FDAP_TOP);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IFileSaveDialog_AddPlace(pfsd, psidesktop, FDAP_TOP + 1);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_AddPlace(pfsd, psidesktop, FDAP_BOTTOM);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_AddPlace(pfsd, psidesktop, FDAP_TOP);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    }

    /* SetFileName */
    hr = IFileOpenDialog_SetFileName(pfod, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileOpenDialog_SetFileName(pfod, null);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileOpenDialog_SetFileName(pfod, txt);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileOpenDialog_GetFileName(pfod, &filename);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    ok(!lstrcmpW(filename, txt), "Strings do not match.\n");
    CoTaskMemFree(filename);

    hr = IFileSaveDialog_SetFileName(pfsd, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetFileName(pfsd, null);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetFileName(pfsd, txt);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_GetFileName(pfsd, &filename);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    ok(!lstrcmpW(filename, txt), "Strings do not match.\n");
    CoTaskMemFree(filename);

    /* SetFileNameLabel */
    hr = IFileOpenDialog_SetFileNameLabel(pfod, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileOpenDialog_SetFileNameLabel(pfod, null);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileOpenDialog_SetFileNameLabel(pfod, txt);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IFileSaveDialog_SetFileNameLabel(pfsd, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetFileNameLabel(pfsd, null);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetFileNameLabel(pfsd, txt);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* Close */
    hr = IFileOpenDialog_Close(pfod, S_FALSE);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_Close(pfsd, S_FALSE);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* SetOkButtonLabel */
    hr = IFileOpenDialog_SetOkButtonLabel(pfod, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileOpenDialog_SetOkButtonLabel(pfod, null);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileOpenDialog_SetOkButtonLabel(pfod, txt);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetOkButtonLabel(pfsd, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetOkButtonLabel(pfsd, null);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetOkButtonLabel(pfsd, txt);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* SetTitle */
    hr = IFileOpenDialog_SetTitle(pfod, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileOpenDialog_SetTitle(pfod, null);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileOpenDialog_SetTitle(pfod, txt);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetTitle(pfsd, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetTitle(pfsd, null);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetTitle(pfsd, txt);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /** IFileOpenDialog specific **/

    /* GetResults */
    if(0)
    {
        /* Crashes under Windows 7 */
        IFileOpenDialog_GetResults(pfod, NULL);
    }
    psia = (void*)0xdeadbeef;
    hr = IFileOpenDialog_GetResults(pfod, &psia);
    ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    ok(psia == NULL, "got %p.\n", psia);

    /* GetSelectedItems */
    if(0)
    {
        /* Crashes under W2K8 */
        hr = IFileOpenDialog_GetSelectedItems(pfod, NULL);
        ok(hr == E_FAIL, "got 0x%08x.\n", hr);
        psia = (void*)0xdeadbeef;
        hr = IFileOpenDialog_GetSelectedItems(pfod, &psia);
        ok(hr == E_FAIL, "got 0x%08x.\n", hr);
        ok(psia == (void*)0xdeadbeef, "got %p.\n", psia);
    }

    /** IFileSaveDialog specific **/

    /* ApplyProperties */
    if(0)
    {
        /* Crashes under windows 7 */
        IFileSaveDialog_ApplyProperties(pfsd, NULL, NULL, NULL, NULL);
        IFileSaveDialog_ApplyProperties(pfsd, psidesktop, NULL, NULL, NULL);
    }

    /* GetProperties */
    hr = IFileSaveDialog_GetProperties(pfsd, NULL);
    todo_wine ok(hr == E_UNEXPECTED, "got 0x%08x\n", hr);
    pps = (void*)0xdeadbeef;
    hr = IFileSaveDialog_GetProperties(pfsd, &pps);
    todo_wine ok(hr == E_UNEXPECTED, "got 0x%08x\n", hr);
    ok(pps == (void*)0xdeadbeef, "got %p\n", pps);

    /* SetProperties */
    if(0)
    {
        /* Crashes under W2K8 */
        hr = IFileSaveDialog_SetProperties(pfsd, NULL);
        ok(hr == S_OK, "got 0x%08x\n", hr);
    }

    /* SetCollectedProperties */
    todo_wine
    {
    hr = IFileSaveDialog_SetCollectedProperties(pfsd, NULL, TRUE);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetCollectedProperties(pfsd, NULL, FALSE);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    }

    /* SetSaveAsItem */
    todo_wine
    {
    hr = IFileSaveDialog_SetSaveAsItem(pfsd, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetSaveAsItem(pfsd, psidesktop);
    ok(hr == MK_E_NOOBJECT, "got 0x%08x\n", hr);
    }

    /** IFileDialog2 **/

    hr = IFileOpenDialog_QueryInterface(pfod, &IID_IFileDialog2, (void**)&pfd2);
    ok((hr == S_OK) || broken(hr == E_NOINTERFACE), "got 0x%08x\n", hr);
    if(SUCCEEDED(hr))
    {
        /* SetCancelButtonLabel */
        hr = IFileDialog2_SetOkButtonLabel(pfd2, NULL);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        hr = IFileDialog2_SetOkButtonLabel(pfd2, null);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        hr = IFileDialog2_SetOkButtonLabel(pfd2, txt);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        /* SetNavigationRoot */
        todo_wine
        {
        hr = IFileDialog2_SetNavigationRoot(pfd2, NULL);
        ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
        hr = IFileDialog2_SetNavigationRoot(pfd2, psidesktop);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        }

        IFileDialog2_Release(pfd2);
    }

    hr = IFileSaveDialog_QueryInterface(pfsd, &IID_IFileDialog2, (void**)&pfd2);
    ok((hr == S_OK) || broken(hr == E_NOINTERFACE), "got 0x%08x\n", hr);
    if(SUCCEEDED(hr))
    {
        /* SetCancelButtonLabel */
        hr = IFileDialog2_SetOkButtonLabel(pfd2, NULL);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        hr = IFileDialog2_SetOkButtonLabel(pfd2, null);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        hr = IFileDialog2_SetOkButtonLabel(pfd2, txt);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        /* SetNavigationRoot */
        todo_wine
        {
        hr = IFileDialog2_SetNavigationRoot(pfd2, NULL);
        ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
        hr = IFileDialog2_SetNavigationRoot(pfd2, psidesktop);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        }

        IFileDialog2_Release(pfd2);
    }

    /* Cleanup */
    IShellItem_Release(psidesktop);
    ref = IFileOpenDialog_Release(pfod);
    ok(!ref, "Got refcount %d, should have been released.\n", ref);
    ref = IFileSaveDialog_Release(pfsd);
    ok(!ref, "Got refcount %d, should have been released.\n", ref);
}

static void ensure_zero_events_(const char *file, int line, IFileDialogEventsImpl *impl)
{
    ok_(file, line)(!impl->OnFileOk, "OnFileOk: %d\n", impl->OnFileOk);
    ok_(file, line)(!impl->OnFolderChanging, "OnFolderChanging: %d\n", impl->OnFolderChanging);
    ok_(file, line)(!impl->OnFolderChange, "OnFolderChange: %d\n", impl->OnFolderChange);
    ok_(file, line)(!impl->OnSelectionChange, "OnSelectionChange: %d\n", impl->OnSelectionChange);
    ok_(file, line)(!impl->OnShareViolation, "OnShareViolation: %d\n", impl->OnShareViolation);
    ok_(file, line)(!impl->OnTypeChange, "OnTypeChange: %d\n", impl->OnTypeChange);
    ok_(file, line)(!impl->OnOverwrite, "OnOverwrite: %d\n", impl->OnOverwrite);
    impl->OnFileOk = impl->OnFolderChanging = impl->OnFolderChange = 0;
    impl->OnSelectionChange = impl->OnShareViolation = impl->OnTypeChange = 0;
    impl->OnOverwrite = 0;
}
#define ensure_zero_events(impl) ensure_zero_events_(__FILE__, __LINE__, impl)

static void test_advise_helper(IFileDialog *pfd)
{
    IFileDialogEventsImpl *pfdeimpl;
    IFileDialogEvents *pfde;
    DWORD cookie[10];
    UINT i;
    HRESULT hr;

    pfde = IFileDialogEvents_Constructor();
    pfdeimpl = impl_from_IFileDialogEvents(pfde);

    hr = IFileDialog_Advise(pfd, NULL, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    hr = IFileDialog_Advise(pfd, pfde, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    hr = IFileDialog_Advise(pfd, NULL, &cookie[0]);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    ok(pfdeimpl->ref == 1, "got ref %d\n", pfdeimpl->ref);
    ensure_zero_events(pfdeimpl);

    hr = IFileDialog_Unadvise(pfd, 0);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    for(i = 0; i < 10; i++) {
        hr = IFileDialog_Advise(pfd, pfde, &cookie[i]);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(cookie[i] == i+1, "Got cookie: %d\n", cookie[i]);
    }
    ok(pfdeimpl->ref == 10+1, "got ref %d\n", pfdeimpl->ref);
    ensure_zero_events(pfdeimpl);

    for(i = 3; i < 7; i++) {
        hr = IFileDialog_Unadvise(pfd, cookie[i]);
        ok(hr == S_OK, "got 0x%08x\n", hr);
    }
    ok(pfdeimpl->ref == 6+1, "got ref %d\n", pfdeimpl->ref);
    ensure_zero_events(pfdeimpl);

    for(i = 0; i < 3; i++) {
        hr = IFileDialog_Unadvise(pfd, cookie[i]);
        ok(hr == S_OK, "got 0x%08x\n", hr);
    }
    ok(pfdeimpl->ref == 3+1, "got ref %d\n", pfdeimpl->ref);
    ensure_zero_events(pfdeimpl);

    for(i = 7; i < 10; i++) {
        hr = IFileDialog_Unadvise(pfd, cookie[i]);
        ok(hr == S_OK, "got 0x%08x\n", hr);
    }
    ok(pfdeimpl->ref == 1, "got ref %d\n", pfdeimpl->ref);
    ensure_zero_events(pfdeimpl);

    hr = IFileDialog_Unadvise(pfd, cookie[9]+1);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    ok(pfdeimpl->ref == 1, "got ref %d\n", pfdeimpl->ref);
    ensure_zero_events(pfdeimpl);

    hr = IFileDialog_Advise(pfd, pfde, &cookie[0]);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    todo_wine ok(cookie[0] == 1, "got cookie: %d\n", cookie[0]);
    ok(pfdeimpl->ref == 1+1, "got ref %d\n", pfdeimpl->ref);
    ensure_zero_events(pfdeimpl);

    hr = IFileDialog_Unadvise(pfd, cookie[0]);

    if(0)
    {
        /* Unadvising already unadvised cookies crashes on
           Windows 7. */
        IFileDialog_Unadvise(pfd, cookie[0]);
    }


    IFileDialogEvents_Release(pfde);
}

static void test_advise(void)
{
    IFileDialog *pfd;
    HRESULT hr;
    LONG ref;

    trace("Testing FileOpenDialog (advise)\n");
    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IFileDialog, (void**)&pfd);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    test_advise_helper(pfd);
    ref = IFileDialog_Release(pfd);
    ok(!ref, "Got refcount %d, should have been released.\n", ref);

    trace("Testing FileSaveDialog (advise)\n");
    hr = CoCreateInstance(&CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IFileDialog, (void**)&pfd);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    test_advise_helper(pfd);
    ref = IFileDialog_Release(pfd);
    ok(!ref, "Got refcount %d, should have been released.\n", ref);
}

static void touch_file(LPCWSTR filename)
{
    HANDLE file;
    file = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Failed to create file.\n");
    CloseHandle(file);
}

static void test_filename_savedlg_(LPCWSTR set_filename, LPCWSTR defext,
                                   const COMDLG_FILTERSPEC *filterspec, UINT fs_count,
                                   LPCWSTR exp_filename, const char *file, int line)
{
    IFileSaveDialog *pfsd;
    IFileDialogEventsImpl *pfdeimpl;
    IFileDialogEvents *pfde;
    DWORD cookie;
    LPWSTR filename;
    IShellItem *psi;
    LONG ref;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IFileSaveDialog, (void**)&pfsd);
    ok_(file,line)(hr == S_OK, "Got 0x%08x\n", hr);

    if(fs_count)
    {
        hr = IFileSaveDialog_SetFileTypes(pfsd, fs_count, filterspec);
        ok_(file,line)(hr == S_OK, "SetFileTypes failed: Got 0x%08x\n", hr);
    }

    if(defext)
    {
        hr = IFileSaveDialog_SetDefaultExtension(pfsd, defext);
        ok_(file,line)(hr == S_OK, "SetDefaultExtensions failed: Got 0x%08x\n", hr);
    }

    pfde = IFileDialogEvents_Constructor();
    pfdeimpl = impl_from_IFileDialogEvents(pfde);
    pfdeimpl->set_filename = set_filename;
    hr = IFileSaveDialog_Advise(pfsd, pfde, &cookie);
    ok_(file,line)(hr == S_OK, "Advise failed: Got 0x%08x\n", hr);

    hr = IFileSaveDialog_Show(pfsd, NULL);
    ok_(file,line)(hr == S_OK, "Show failed: Got 0x%08x\n", hr);

    hr = IFileSaveDialog_GetFileName(pfsd, &filename);
    ok_(file,line)(hr == S_OK, "GetFileName failed: Got 0x%08x\n", hr);
    ok_(file,line)(!lstrcmpW(filename, set_filename), "Got %s\n", wine_dbgstr_w(filename));
    CoTaskMemFree(filename);

    hr = IFileSaveDialog_GetResult(pfsd, &psi);
    ok_(file,line)(hr == S_OK, "GetResult failed: Got 0x%08x\n", hr);

    hr = IShellItem_GetDisplayName(psi, SIGDN_PARENTRELATIVEPARSING, &filename);
    ok_(file,line)(hr == S_OK, "GetDisplayName failed: Got 0x%08x\n", hr);
    ok_(file,line)(!lstrcmpW(filename, exp_filename), "(GetDisplayName) Got %s\n", wine_dbgstr_w(filename));
    CoTaskMemFree(filename);
    IShellItem_Release(psi);

    hr = IFileSaveDialog_Unadvise(pfsd, cookie);
    ok_(file,line)(hr == S_OK, "Unadvise failed: Got 0x%08x\n", hr);

    ref = IFileSaveDialog_Release(pfsd);
    ok_(file,line)(!ref, "Got refcount %d, should have been released.\n", ref);

    IFileDialogEvents_Release(pfde);
}
#define test_filename_savedlg(set_filename, defext, filterspec, fs_count, exp_filename) \
    test_filename_savedlg_(set_filename, defext, filterspec, fs_count, exp_filename, __FILE__, __LINE__)

static void test_filename_opendlg_(LPCWSTR set_filename, IShellItem *psi_current, LPCWSTR defext,
                                   const COMDLG_FILTERSPEC *filterspec, UINT fs_count,
                                   LPCWSTR exp_filename, const char *file, int line)
{
    IFileOpenDialog *pfod;
    IFileDialogEventsImpl *pfdeimpl;
    IFileDialogEvents *pfde;
    DWORD cookie;
    LPWSTR filename;
    IShellItemArray *psia;
    IShellItem *psi;
    LONG ref;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IFileOpenDialog, (void**)&pfod);
    ok_(file,line)(hr == S_OK, "CoCreateInstance failed: Got 0x%08x\n", hr);

    if(defext)
    {
        hr = IFileOpenDialog_SetDefaultExtension(pfod, defext);
        ok_(file,line)(hr == S_OK, "SetDefaultExtensions failed: Got 0x%08x\n", hr);
    }

    if(fs_count)
    {
        hr = IFileOpenDialog_SetFileTypes(pfod, 2, filterspec);
        ok_(file,line)(hr == S_OK, "SetFileTypes failed: Got 0x%08x\n", hr);
    }

    hr = IFileOpenDialog_SetFolder(pfod, psi_current);
    ok_(file,line)(hr == S_OK, "SetFolder failed: Got 0x%08x\n", hr);

    pfde = IFileDialogEvents_Constructor();
    pfdeimpl = impl_from_IFileDialogEvents(pfde);
    pfdeimpl->set_filename = set_filename;
    pfdeimpl->set_filename_tried = FALSE;
    hr = IFileOpenDialog_Advise(pfod, pfde, &cookie);
    ok_(file,line)(hr == S_OK, "Advise failed: Got 0x%08x\n", hr);

    hr = IFileOpenDialog_Show(pfod, NULL);
    ok_(file,line)(hr == S_OK || (!exp_filename && hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)),
                   "Show failed: Got 0x%08x\n", hr);
    if(hr == S_OK)
    {
        hr = IFileOpenDialog_GetResult(pfod, &psi);
        ok_(file,line)(hr == S_OK, "GetResult failed: Got 0x%08x\n", hr);

        hr = IShellItem_GetDisplayName(psi, SIGDN_PARENTRELATIVEPARSING, &filename);
        ok_(file,line)(hr == S_OK, "GetDisplayName(Result) failed: Got 0x%08x\n", hr);
        ok_(file,line)(!lstrcmpW(filename, exp_filename), "(GetResult) Got %s\n", wine_dbgstr_w(filename));
        CoTaskMemFree(filename);
        IShellItem_Release(psi);

        hr = IFileOpenDialog_GetResults(pfod, &psia);
        ok_(file,line)(hr == S_OK, "GetResults failed: Got 0x%08x\n", hr);
        hr = IShellItemArray_GetItemAt(psia, 0, &psi);
        ok_(file,line)(hr == S_OK, "GetItemAt failed: Got 0x%08x\n", hr);

        hr = IShellItem_GetDisplayName(psi, SIGDN_PARENTRELATIVEPARSING, &filename);
        ok_(file,line)(hr == S_OK, "GetDisplayName(Results) failed: Got 0x%08x\n", hr);
        ok_(file,line)(!lstrcmpW(filename, exp_filename), "(GetResults) Got %s\n", wine_dbgstr_w(filename));
        CoTaskMemFree(filename);

        IShellItem_Release(psi);
        IShellItemArray_Release(psia);
    }
    else
    {
        hr = IFileOpenDialog_GetResult(pfod, &psi);
        ok_(file,line)(hr == E_UNEXPECTED, "GetResult: Got 0x%08x\n", hr);

        hr = IFileOpenDialog_GetResults(pfod, &psia);
        ok_(file,line)(hr == E_FAIL, "GetResults: Got 0x%08x\n", hr);
    }

    hr = IFileOpenDialog_GetFileName(pfod, &filename);
    ok_(file,line)(hr == S_OK, "GetFileName failed: Got 0x%08x\n", hr);
    ok_(file,line)(!lstrcmpW(filename, set_filename), "(GetFileName) Got %s\n", wine_dbgstr_w(filename));
    CoTaskMemFree(filename);


    hr = IFileOpenDialog_Unadvise(pfod, cookie);
    ok_(file,line)(hr == S_OK, "Unadvise failed: Got 0x%08x\n", hr);

    ref = IFileOpenDialog_Release(pfod);
    ok_(file,line)(!ref, "Got refcount %d, should have been released.\n", ref);

    IFileDialogEvents_Release(pfde);
}
#define test_filename_opendlg(set_filename, psi, defext, filterspec, fs_count, exp_filename) \
    test_filename_opendlg_(set_filename, psi, defext, filterspec, fs_count, exp_filename, __FILE__, __LINE__)

static void test_filename(void)
{
    IShellItem *psi_current;
    HRESULT hr;
    WCHAR buf[MAX_PATH];

    static const WCHAR filename_noextW[] = {'w','i','n','e','t','e','s','t',0};
    static const WCHAR filename_dotextW[] = {'w','i','n','e','t','e','s','t','.',0};
    static const WCHAR filename_dotanddefW[] = {'w','i','n','e','t','e','s','t','.','.','w','t','e',0};
    static const WCHAR filename_defextW[] = {'w','i','n','e','t','e','s','t','.','w','t','e',0};
    static const WCHAR filename_ext1W[] = {'w','i','n','e','t','e','s','t','.','w','t','1',0};
    static const WCHAR filename_ext2W[] = {'w','i','n','e','t','e','s','t','.','w','t','2',0};
    static const WCHAR filename_ext1anddefW[] =
        {'w','i','n','e','t','e','s','t','.','w','t','1','.','w','t','e',0};
    static const WCHAR defextW[] = {'w','t','e',0};
    static const WCHAR desc1[] = {'d','e','s','c','r','i','p','t','i','o','n','1',0};
    static const WCHAR desc2[] = {'d','e','s','c','r','i','p','t','i','o','n','2',0};
    static const WCHAR descdef[] = {'d','e','f','a','u','l','t',' ','d','e','s','c',0};
    static const WCHAR ext1[] = {'*','.','w','t','1',0};
    static const WCHAR ext2[] = {'*','.','w','t','2',0};
    static const WCHAR extdef[] = {'*','.','w','t','e',0};
    static const WCHAR complexext[] = {'*','.','w','t','2',';','*','.','w','t','1',0};

    static const COMDLG_FILTERSPEC filterspec[] = {
        { desc1, ext1 }, { desc2, ext2 }, { descdef, extdef }
    };
    static const COMDLG_FILTERSPEC filterspec2[] = {
        { desc1, complexext }
    };

    /* No extension */
    test_filename_savedlg(filename_noextW, NULL, NULL, 0, filename_noextW);
    /* Default extension */
    test_filename_savedlg(filename_noextW, defextW, NULL, 0, filename_defextW);
    /* Default extension on filename ending with a . */
    test_filename_savedlg(filename_dotextW, defextW, NULL, 0, filename_dotanddefW);
    /* Default extension on filename with default extension */
    test_filename_savedlg(filename_defextW, defextW, NULL, 0, filename_defextW);
    /* Default extension on filename with another extension */
    test_filename_savedlg(filename_ext1W, defextW, NULL, 0, filename_ext1anddefW);
    /* Default extension, filterspec without default extension */
    test_filename_savedlg(filename_noextW, defextW, filterspec, 2, filename_ext1W);
    /* Default extension, filterspec with default extension */
    test_filename_savedlg(filename_noextW, defextW, filterspec, 3, filename_ext1W);
    /* Default extension, filterspec with "complex" extension */
    test_filename_savedlg(filename_noextW, defextW, filterspec2, 1, filename_ext2W);

    GetCurrentDirectoryW(MAX_PATH, buf);
    ok(!!pSHCreateItemFromParsingName, "SHCreateItemFromParsingName is missing.\n");
    hr = pSHCreateItemFromParsingName(buf, NULL, &IID_IShellItem, (void**)&psi_current);
    ok(hr == S_OK, "Got 0x%08x\n", hr);

    touch_file(filename_noextW);
    touch_file(filename_defextW);
    touch_file(filename_ext2W);

    /* IFileOpenDialog, default extension */
    test_filename_opendlg(filename_noextW, psi_current, defextW, NULL, 0, filename_noextW);
    /* IFileOpenDialog, default extension and filterspec */
    test_filename_opendlg(filename_noextW, psi_current, defextW, filterspec, 2, filename_noextW);

    DeleteFileW(filename_noextW);
    /* IFileOpenDialog, default extension, noextW deleted */
    test_filename_opendlg(filename_noextW, psi_current, defextW, NULL, 0, filename_defextW);
    if(0) /* Interactive */
    {
    /* IFileOpenDialog, filterspec, no default extension, noextW deleted */
    test_filename_opendlg(filename_noextW, psi_current, NULL, filterspec, 2, NULL);
    }

    IShellItem_Release(psi_current);
    DeleteFileW(filename_defextW);
    DeleteFileW(filename_ext2W);
}

static const WCHAR label[] = {'l','a','b','e','l',0};
static const WCHAR label2[] = {'t','e','s','t',0};
static const WCHAR menuW[] = {'m','e','n','u','_','i','t','e','m',0};
static const WCHAR pushbutton1W[] = {'p','u','s','h','b','u','t','t','o','n','_','i','t','e','m',0};
static const WCHAR pushbutton2W[] = {'p','u','s','h','b','u','t','t','o','n','2','_','i','t','e','m',0};
static const WCHAR comboboxitem1W[] = {'c','o','m','b','o','b','o','x','1','_','i','t','e','m',0};
static const WCHAR comboboxitem2W[] = {'c','o','m','b','o','b','o','x','2','_','i','t','e','m',0};
static const WCHAR radiobutton1W[] = {'r','a','d','i','o','b','u','t','t','o','n','1','_','i','t','e','m',0};
static const WCHAR radiobutton2W[] = {'r','a','d','i','o','b','u','t','t','o','n','2','_','i','t','e','m',0};
static const WCHAR checkbutton1W[] = {'c','h','e','c','k','b','u','t','t','o','n','1','_','i','t','e','m',0};
static const WCHAR checkbutton2W[] = {'c','h','e','c','k','b','u','t','t','o','n','2','_','i','t','e','m',0};
static const WCHAR editbox1W[] = {'e','d','i','t','b','o','x','W','1','_','i','t','e','m',0};
static const WCHAR editbox2W[] = {'e','d','i','t','b','o','x','W','2','_','i','t','e','m',0};
static const WCHAR textW[] = {'t','e','x','t','_','i','t','e','m',0};
static const WCHAR text2W[] = {'t','e','x','t','2','_','i','t','e','m',0};
static const WCHAR separatorW[] = {'s','e','p','a','r','a','t','o','r','_','i','t','e','m',0};
static const WCHAR visualgroup1W[] = {'v','i','s','u','a','l','g','r','o','u','p','1',0};
static const WCHAR visualgroup2W[] = {'v','i','s','u','a','l','g','r','o','u','p','2',0};

static const WCHAR floatnotifysinkW[] = {'F','l','o','a','t','N','o','t','i','f','y','S','i','n','k',0};
static const WCHAR RadioButtonListW[] = {'R','a','d','i','o','B','u','t','t','o','n','L','i','s','t',0};

struct fw_arg {
    LPCWSTR class, text;
    HWND hwnd_res;
};
static BOOL CALLBACK find_window_callback(HWND hwnd, LPARAM lparam)
{
    struct fw_arg *arg = (struct fw_arg*)lparam;
    WCHAR buf[1024];

    if(arg->class)
    {
        GetClassNameW(hwnd, buf, 1024);
        if(lstrcmpW(buf, arg->class))
            return TRUE;
    }

    if(arg->text)
    {
        GetWindowTextW(hwnd, buf, 1024);
        if(lstrcmpW(buf, arg->text))
            return TRUE;
    }

    arg->hwnd_res = hwnd;
    return FALSE;
}

static HWND find_window(HWND parent, LPCWSTR class, LPCWSTR text)
{
    struct fw_arg arg = {class, text, NULL};

    EnumChildWindows(parent, find_window_callback, (LPARAM)&arg);
    return arg.hwnd_res;
}

static void test_customize_onfolderchange(IFileDialog *pfd)
{
    IOleWindow *pow;
    HWND dlg_hwnd, item, item_parent;
    HRESULT hr;
    BOOL br;
    WCHAR buf[1024];

    buf[0] = '\0';

    hr = IFileDialog_QueryInterface(pfd, &IID_IOleWindow, (void**)&pow);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    hr = IOleWindow_GetWindow(pow, &dlg_hwnd);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    ok(dlg_hwnd != NULL, "Got NULL.\n");
    IOleWindow_Release(pow);

    item = find_window(dlg_hwnd, NULL, checkbutton2W);
    ok(item != NULL, "Failed to find item.\n");
    item_parent = GetParent(item);
    GetClassNameW(item_parent, buf, 1024);
    ok(!lstrcmpW(buf, floatnotifysinkW), "Got %s\n", wine_dbgstr_w(buf));
    item = find_window(dlg_hwnd, NULL, text2W);
    ok(item != NULL, "Failed to find item.\n");
    item_parent = GetParent(item);
    GetClassNameW(item_parent, buf, 1024);
    ok(!lstrcmpW(buf, floatnotifysinkW), "Got %s\n", wine_dbgstr_w(buf));
    item = find_window(dlg_hwnd, NULL, radiobutton1W);
    todo_wine ok(item != NULL, "Failed to find item.\n");
    item_parent = GetParent(item);
    GetClassNameW(item_parent, buf, 1024);
    todo_wine ok(!lstrcmpW(buf, RadioButtonListW), "Got %s\n", wine_dbgstr_w(buf));
    item_parent = GetParent(item_parent);
    GetClassNameW(item_parent, buf, 1024);
    ok(!lstrcmpW(buf, floatnotifysinkW), "Got %s\n", wine_dbgstr_w(buf));

    item = find_window(dlg_hwnd, NULL, pushbutton1W);
    ok(item == NULL, "Found item: %p\n", item);
    item = find_window(dlg_hwnd, NULL, pushbutton2W);
    ok(item == NULL, "Found item: %p\n", item);
    item = find_window(dlg_hwnd, NULL, comboboxitem1W);
    ok(item == NULL, "Found item: %p\n", item);
    item = find_window(dlg_hwnd, NULL, comboboxitem2W);
    ok(item == NULL, "Found item: %p\n", item);
    item = find_window(dlg_hwnd, NULL, radiobutton2W);
    ok(item == NULL, "Found item: %p\n", item);
    item = find_window(dlg_hwnd, NULL, checkbutton1W);
    ok(item == NULL, "Found item: %p\n", item);
    item = find_window(dlg_hwnd, NULL, editbox1W);
    ok(item == NULL, "Found item: %p\n", item);
    item = find_window(dlg_hwnd, NULL, editbox2W);
    ok(item == NULL, "Found item: %p\n", item);
    item = find_window(dlg_hwnd, NULL, textW);
    ok(item == NULL, "Found item: %p\n", item);
    item = find_window(dlg_hwnd, NULL, separatorW);
    ok(item == NULL, "Found item: %p\n", item);
    item = find_window(dlg_hwnd, NULL, visualgroup1W);
    ok(item == NULL, "Found item: %p\n", item);
    item = find_window(dlg_hwnd, NULL, visualgroup2W);
    ok(item == NULL, "Found item: %p\n", item);

    br = PostMessageW(dlg_hwnd, WM_COMMAND, IDCANCEL, 0);
    ok(br, "Failed\n");
}

static void test_customize(void)
{
    IFileDialog *pfod;
    IFileDialogCustomize *pfdc;
    IFileDialogEventsImpl *pfdeimpl;
    IFileDialogEvents *pfde;
    IOleWindow *pow;
    CDCONTROLSTATEF cdstate;
    DWORD cookie;
    LPWSTR tmpstr;
    UINT i;
    LONG ref;
    HWND dlg_hwnd;
    HRESULT hr;
    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IFileDialog, (void**)&pfod);
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    hr = IFileOpenDialog_QueryInterface(pfod, &IID_IFileDialogCustomize, (void**)&pfdc);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(FAILED(hr))
    {
        skip("Skipping IFileDialogCustomize tests.\n");
        return;
    }

    i = 0;
    hr = IFileDialogCustomize_AddPushButton(pfdc, i, pushbutton1W);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileDialogCustomize_AddPushButton(pfdc, i, pushbutton1W);
    ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);

    hr = IFileDialog_QueryInterface(pfod, &IID_IOleWindow, (void**)&pow);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    hr = IOleWindow_GetWindow(pow, &dlg_hwnd);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    ok(dlg_hwnd == NULL, "NULL\n");
    IOleWindow_Release(pow);

    cdstate = 0xdeadbeef;
    hr = IFileDialogCustomize_GetControlState(pfdc, i, &cdstate);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    ok(cdstate == CDCS_ENABLEDVISIBLE, "got 0x%08x.\n", cdstate);

    hr = IFileDialogCustomize_AddControlItem(pfdc, i, 0, label);
    ok(hr == E_NOINTERFACE, "got 0x%08x.\n", hr);

    hr = IFileDialogCustomize_SetControlLabel(pfdc, i, label2);
    ok(hr == S_OK, "got 0x%08x (control: %d).\n", hr, i);

    hr = IFileDialogCustomize_EnableOpenDropDown(pfdc, i);
    todo_wine ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);
    hr = IFileDialogCustomize_EnableOpenDropDown(pfdc, ++i);
    todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);

    cdstate = 0xdeadbeef;
    hr = IFileDialogCustomize_GetControlState(pfdc, i, &cdstate);
    ok(hr == E_NOTIMPL, "got 0x%08x.\n", hr);
    ok(cdstate == 0xdeadbeef, "got 0x%08x.\n", cdstate);

    hr = IFileDialogCustomize_AddControlItem(pfdc, i, 0, label);
    todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileDialogCustomize_AddControlItem(pfdc, i, 0, label);
    todo_wine ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);

    cdstate = 0xdeadbeef;
    hr = IFileDialogCustomize_GetControlItemState(pfdc, i, 0, &cdstate);
    todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);
    todo_wine ok(cdstate == CDCS_ENABLEDVISIBLE, "got 0x%08x.\n", cdstate);
    hr = IFileDialogCustomize_SetControlItemState(pfdc, i, 0, 0);
    todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);
    cdstate = 0xdeadbeef;
    hr = IFileDialogCustomize_GetControlItemState(pfdc, i, 0, &cdstate);
    todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);
    todo_wine ok(!cdstate, "got 0x%08x.\n", cdstate);
    hr = IFileDialogCustomize_SetControlItemState(pfdc, i, 0, CDCS_ENABLEDVISIBLE);
    todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);
    cdstate = 0xdeadbeef;
    hr = IFileDialogCustomize_GetControlItemState(pfdc, i, 0, &cdstate);
    todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);
    todo_wine ok(cdstate == CDCS_ENABLEDVISIBLE, "got 0x%08x.\n", cdstate);

    hr = IFileDialogCustomize_SetControlLabel(pfdc, i, label2);
    todo_wine ok(hr == E_NOTIMPL, "got 0x%08x (control: %d).\n", hr, i);

    hr = IFileDialogCustomize_AddMenu(pfdc, i, menuW);
    todo_wine ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);
    hr = IFileDialogCustomize_AddMenu(pfdc, ++i, label);
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    cdstate = 0xdeadbeef;
    hr = IFileDialogCustomize_GetControlState(pfdc, i, &cdstate);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    ok(cdstate == CDCS_ENABLEDVISIBLE, "got 0x%08x.\n", cdstate);

    hr = IFileDialogCustomize_AddControlItem(pfdc, i, 0, label);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileDialogCustomize_AddControlItem(pfdc, i, 0, label);
    ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);

    hr = IFileDialogCustomize_SetControlLabel(pfdc, i, label2);
    ok(hr == S_OK, "got 0x%08x (control: %d).\n", hr, i);

    hr = IFileDialogCustomize_AddPushButton(pfdc, i, pushbutton2W);
    ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);
    hr = IFileDialogCustomize_AddPushButton(pfdc, ++i, pushbutton2W);
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    cdstate = 0xdeadbeef;
    hr = IFileDialogCustomize_GetControlState(pfdc, i, &cdstate);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    ok(cdstate == CDCS_ENABLEDVISIBLE, "got 0x%08x.\n", cdstate);

    hr = IFileDialogCustomize_AddControlItem(pfdc, i, 0, label);
    ok(hr == E_NOINTERFACE, "got 0x%08x.\n", hr);

    hr = IFileDialogCustomize_SetControlLabel(pfdc, i, label2);
    ok(hr == S_OK, "got 0x%08x (control: %d).\n", hr, i);

    hr = IFileDialogCustomize_AddComboBox(pfdc, i);
    ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);
    hr = IFileDialogCustomize_AddComboBox(pfdc, ++i);
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    cdstate = 0xdeadbeef;
    hr = IFileDialogCustomize_GetControlState(pfdc, i, &cdstate);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    ok(cdstate == CDCS_ENABLEDVISIBLE, "got 0x%08x.\n", cdstate);

    hr = IFileDialogCustomize_AddControlItem(pfdc, i, 0, label);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileDialogCustomize_AddControlItem(pfdc, i, 0, label);
    ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);

    hr = IFileDialogCustomize_SetControlLabel(pfdc, i, label2);
    ok(hr == S_OK, "got 0x%08x (control: %d).\n", hr, i);

    hr = IFileDialogCustomize_AddRadioButtonList(pfdc, i);
    todo_wine ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);
    hr = IFileDialogCustomize_AddRadioButtonList(pfdc, ++i);
    todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);

    cdstate = 0xdeadbeef;
    hr = IFileDialogCustomize_GetControlState(pfdc, i, &cdstate);
    todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);
    todo_wine ok(cdstate == CDCS_ENABLEDVISIBLE, "got 0x%08x.\n", cdstate);

    hr = IFileDialogCustomize_AddControlItem(pfdc, i, 0, radiobutton1W);
    todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileDialogCustomize_AddControlItem(pfdc, i, 0, radiobutton1W);
    todo_wine ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);

    hr = IFileDialogCustomize_SetControlLabel(pfdc, i, radiobutton2W);
    todo_wine ok(hr == S_OK, "got 0x%08x (control: %d).\n", hr, i);

    hr = IFileDialogCustomize_AddCheckButton(pfdc, i, label, TRUE);
    todo_wine ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);
    hr = IFileDialogCustomize_AddCheckButton(pfdc, ++i, checkbutton1W, TRUE);
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    cdstate = 0xdeadbeef;
    hr = IFileDialogCustomize_GetControlState(pfdc, i, &cdstate);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    ok(cdstate == CDCS_ENABLEDVISIBLE, "got 0x%08x.\n", cdstate);

    hr = IFileDialogCustomize_AddControlItem(pfdc, i, 0, label);
    ok(hr == E_NOINTERFACE, "got 0x%08x.\n", hr);

    hr = IFileDialogCustomize_SetControlLabel(pfdc, i, checkbutton2W);
    ok(hr == S_OK, "got 0x%08x (control: %d).\n", hr, i);

    if(SUCCEEDED(hr))
    {
        BOOL checked;
        hr = IFileDialogCustomize_GetCheckButtonState(pfdc, i, &checked);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        ok(checked, "checkbox not checked.\n");

        hr = IFileDialogCustomize_SetCheckButtonState(pfdc, i, FALSE);
        ok(hr == S_OK, "got 0x%08x.\n", hr);

        hr = IFileDialogCustomize_GetCheckButtonState(pfdc, i, &checked);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        ok(!checked, "checkbox checked.\n");

        hr = IFileDialogCustomize_SetCheckButtonState(pfdc, i, TRUE);
        ok(hr == S_OK, "got 0x%08x.\n", hr);

        hr = IFileDialogCustomize_GetCheckButtonState(pfdc, i, &checked);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        ok(checked, "checkbox not checked.\n");
    }

    hr = IFileDialogCustomize_AddEditBox(pfdc, i, label);
    ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);
    hr = IFileDialogCustomize_AddEditBox(pfdc, ++i, editbox1W);
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    cdstate = 0xdeadbeef;
    hr = IFileDialogCustomize_GetControlState(pfdc, i, &cdstate);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    ok(cdstate == CDCS_ENABLEDVISIBLE, "got 0x%08x.\n", cdstate);

    hr = IFileDialogCustomize_AddControlItem(pfdc, i, 0, label);
    ok(hr == E_NOINTERFACE, "got 0x%08x.\n", hr);

    /* Does not affect the text in the editbox */
    hr = IFileDialogCustomize_SetControlLabel(pfdc, i, editbox2W);
    ok(hr == S_OK, "got 0x%08x (control: %d).\n", hr, i);

    hr = IFileDialogCustomize_GetEditBoxText(pfdc, i, &tmpstr);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr))
    {
        ok(!lstrcmpW(tmpstr, editbox1W), "got %s.\n", wine_dbgstr_w(tmpstr));
        CoTaskMemFree(tmpstr);
    }

    hr = IFileDialogCustomize_SetEditBoxText(pfdc, i, label2);
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    hr = IFileDialogCustomize_GetEditBoxText(pfdc, i, &tmpstr);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr))
    {
        ok(!lstrcmpW(tmpstr, label2), "got %s.\n", wine_dbgstr_w(tmpstr));
        CoTaskMemFree(tmpstr);
    }

    hr = IFileDialogCustomize_AddSeparator(pfdc, i);
    ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);
    hr = IFileDialogCustomize_AddSeparator(pfdc, ++i);
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    cdstate = 0xdeadbeef;
    hr = IFileDialogCustomize_GetControlState(pfdc, i, &cdstate);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    ok(cdstate == CDCS_ENABLEDVISIBLE, "got 0x%08x.\n", cdstate);

    hr = IFileDialogCustomize_AddControlItem(pfdc, i, 0, label);
    ok(hr == E_NOINTERFACE, "got 0x%08x.\n", hr);

    hr = IFileDialogCustomize_SetControlLabel(pfdc, i, separatorW);
    ok(hr == S_OK, "got 0x%08x (control: %d).\n", hr, i);

    hr = IFileDialogCustomize_AddText(pfdc, i, label);
    ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);
    hr = IFileDialogCustomize_AddText(pfdc, ++i, textW);
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    cdstate = 0xdeadbeef;
    hr = IFileDialogCustomize_GetControlState(pfdc, i, &cdstate);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    ok(cdstate == CDCS_ENABLEDVISIBLE, "got 0x%08x.\n", cdstate);

    hr = IFileDialogCustomize_AddControlItem(pfdc, i, 0, label);
    ok(hr == E_NOINTERFACE, "got 0x%08x.\n", hr);

    hr = IFileDialogCustomize_SetControlLabel(pfdc, i, text2W);
    ok(hr == S_OK, "got 0x%08x (control: %d).\n", hr, i);

    hr = IFileDialogCustomize_StartVisualGroup(pfdc, i, label);
    todo_wine ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);
    hr = IFileDialogCustomize_StartVisualGroup(pfdc, ++i, visualgroup1W);
    todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);

    hr = IFileDialogCustomize_AddControlItem(pfdc, i, 0, label);
    todo_wine ok(hr == E_NOINTERFACE, "got 0x%08x.\n", hr);

    hr = IFileDialogCustomize_SetControlLabel(pfdc, i, visualgroup2W);
    todo_wine ok(hr == S_OK, "got 0x%08x (control: %d).\n", hr, i);

    cdstate = 0xdeadbeef;
    hr = IFileDialogCustomize_GetControlState(pfdc, i, &cdstate);
    todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);
    todo_wine ok(cdstate == CDCS_ENABLEDVISIBLE, "got 0x%08x.\n", cdstate);

    hr = IFileDialogCustomize_StartVisualGroup(pfdc, ++i, label);
    todo_wine ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);
    hr = IFileDialogCustomize_EndVisualGroup(pfdc);
    todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);

    i++; /* Nonexisting control */
    hr = IFileDialogCustomize_AddControlItem(pfdc, i, 0, label);
    todo_wine ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);
    hr = IFileDialogCustomize_SetControlLabel(pfdc, i, label2);
    ok(hr == E_INVALIDARG, "got 0x%08x (control: %d).\n", hr, i);
    cdstate = 0xdeadbeef;
    hr = IFileDialogCustomize_GetControlState(pfdc, i, &cdstate);
    todo_wine ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);
    ok(cdstate == 0xdeadbeef, "got 0x%08x.\n", cdstate);

    pfde = IFileDialogEvents_Constructor();
    pfdeimpl = impl_from_IFileDialogEvents(pfde);
    pfdeimpl->cfd_test1 = TRUE;
    hr = IFileDialog_Advise(pfod, pfde, &cookie);
    ok(hr == S_OK, "Got 0x%08x\n", hr);

    hr = IFileDialog_Show(pfod, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_CANCELLED), "Got 0x%08x\n", hr);

    hr = IFileDialog_Unadvise(pfod, cookie);
    ok(hr == S_OK, "Got 0x%08x\n", hr);

    IFileDialogEvents_Release(pfde);
    IFileDialogCustomize_Release(pfdc);
    ref = IFileOpenDialog_Release(pfod);
    ok(!ref, "Refcount not zero (%d).\n", ref);


    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IFileDialog, (void**)&pfod);
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    hr = IFileDialogCustomize_QueryInterface(pfod, &IID_IFileDialogCustomize, (void**)&pfdc);
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    i = 0;
    hr = IFileDialogCustomize_AddMenu(pfdc, ++i, label);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr))
    {
        DWORD selected;
        UINT j = 0;

        for(j = 0; j < 10; j++)
        {
            hr = IFileDialogCustomize_AddControlItem(pfdc, i, j, label);
            ok(hr == S_OK, "got 0x%08x.\n", hr);
        }

        hr = IFileDialogCustomize_GetSelectedControlItem(pfdc, i, &selected);
        ok(hr == E_NOTIMPL, "got 0x%08x.\n", hr);

        cdstate = 0xdeadbeef;
        hr = IFileDialogCustomize_GetControlItemState(pfdc, i, 0, &cdstate);
        todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);
        todo_wine ok(cdstate == CDCS_ENABLEDVISIBLE, "got 0x%08x.\n", cdstate);
        hr = IFileDialogCustomize_SetControlItemState(pfdc, i, 0, 0);
        todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);
        cdstate = 0xdeadbeef;
        hr = IFileDialogCustomize_GetControlItemState(pfdc, i, 0, &cdstate);
        todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);
        todo_wine ok(cdstate == 0, "got 0x%08x.\n", cdstate);
        hr = IFileDialogCustomize_SetControlItemState(pfdc, i, 0, CDCS_ENABLEDVISIBLE);
        todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);
        cdstate = 0xdeadbeef;
        hr = IFileDialogCustomize_GetControlItemState(pfdc, i, 0, &cdstate);
        todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);
        todo_wine ok(cdstate == CDCS_ENABLEDVISIBLE, "got 0x%08x.\n", cdstate);

        hr = IFileDialogCustomize_RemoveAllControlItems(pfdc, i);
        ok(hr == E_NOTIMPL, "got 0x%08x.\n", hr);

        for(j = 0; j < 10; j++)
        {
            hr = IFileDialogCustomize_RemoveControlItem(pfdc, i, j);
            ok(hr == S_OK, "got 0x%08x.\n", hr);
        }
    }
    hr = IFileDialogCustomize_AddPushButton(pfdc, ++i, label);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileDialogCustomize_AddComboBox(pfdc, ++i);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr))
    {
        DWORD selected = -1;
        UINT j = 0;

        for(j = 0; j < 10; j++)
        {
            hr = IFileDialogCustomize_AddControlItem(pfdc, i, j, label);
            ok(hr == S_OK, "got 0x%08x.\n", hr);
        }

        hr = IFileDialogCustomize_GetSelectedControlItem(pfdc, i, &selected);
        ok(hr == E_FAIL, "got 0x%08x.\n", hr);
        ok(selected == -1, "got %d.\n", selected);

        todo_wine {
        cdstate = 0xdeadbeef;
        hr = IFileDialogCustomize_GetControlItemState(pfdc, i, 0, &cdstate);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        ok(cdstate == CDCS_ENABLEDVISIBLE, "got 0x%08x.\n", cdstate);
        hr = IFileDialogCustomize_SetControlItemState(pfdc, i, 0, 0);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        cdstate = 0xdeadbeef;
        hr = IFileDialogCustomize_GetControlItemState(pfdc, i, 0, &cdstate);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        ok(cdstate == 0, "got 0x%08x.\n", cdstate);
        hr = IFileDialogCustomize_SetControlItemState(pfdc, i, 0, CDCS_ENABLEDVISIBLE);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        cdstate = 0xdeadbeef;
        hr = IFileDialogCustomize_GetControlItemState(pfdc, i, 0, &cdstate);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        ok(cdstate == CDCS_ENABLEDVISIBLE, "got 0x%08x.\n", cdstate);
        }

        for(j = 0; j < 10; j++)
        {
            hr = IFileDialogCustomize_SetSelectedControlItem(pfdc, i, j);
            ok(hr == S_OK, "got 0x%08x.\n", hr);
            hr = IFileDialogCustomize_GetSelectedControlItem(pfdc, i, &selected);
            ok(hr == S_OK, "got 0x%08x.\n", hr);
            ok(selected == j, "got %d.\n", selected);
        }
        j++;
        hr = IFileDialogCustomize_SetSelectedControlItem(pfdc, i, j);
        ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);

        hr = IFileDialogCustomize_RemoveAllControlItems(pfdc, i);
        ok(hr == E_NOTIMPL, "got 0x%08x.\n", hr);

        for(j = 0; j < 10; j++)
        {
            hr = IFileDialogCustomize_RemoveControlItem(pfdc, i, j);
            ok(hr == S_OK, "got 0x%08x.\n", hr);
        }
    }

    hr = IFileDialogCustomize_AddRadioButtonList(pfdc, ++i);
    todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr))
    {
        DWORD selected = -1;
        UINT j = 0;

        for(j = 0; j < 10; j++)
        {
            hr = IFileDialogCustomize_AddControlItem(pfdc, i, j, label);
            ok(hr == S_OK, "got 0x%08x.\n", hr);
        }

        hr = IFileDialogCustomize_GetSelectedControlItem(pfdc, i, &selected);
        ok(hr == E_FAIL, "got 0x%08x.\n", hr);
        ok(selected == -1, "got %d.\n", selected);

        todo_wine {
        cdstate = 0xdeadbeef;
        hr = IFileDialogCustomize_GetControlItemState(pfdc, i, 0, &cdstate);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        ok(cdstate == CDCS_ENABLEDVISIBLE, "got 0x%08x.\n", cdstate);
        hr = IFileDialogCustomize_SetControlItemState(pfdc, i, 0, 0);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        cdstate = 0xdeadbeef;
        hr = IFileDialogCustomize_GetControlItemState(pfdc, i, 0, &cdstate);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        ok(cdstate == 0, "got 0x%08x.\n", cdstate);
        hr = IFileDialogCustomize_SetControlItemState(pfdc, i, 0, CDCS_ENABLEDVISIBLE);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        cdstate = 0xdeadbeef;
        hr = IFileDialogCustomize_GetControlItemState(pfdc, i, 0, &cdstate);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        ok(cdstate == CDCS_ENABLEDVISIBLE, "got 0x%08x.\n", cdstate);
        }

        for(j = 0; j < 10; j++)
        {
            hr = IFileDialogCustomize_SetSelectedControlItem(pfdc, i, j);
            ok(hr == S_OK, "got 0x%08x.\n", hr);
            hr = IFileDialogCustomize_GetSelectedControlItem(pfdc, i, &selected);
            ok(hr == S_OK, "got 0x%08x.\n", hr);
            ok(selected == j, "got %d.\n", selected);
        }
        j++;
        hr = IFileDialogCustomize_SetSelectedControlItem(pfdc, i, j);
        ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);

        hr = IFileDialogCustomize_RemoveAllControlItems(pfdc, i);
        ok(hr == E_NOTIMPL, "got 0x%08x.\n", hr);

        for(j = 0; j < 10; j++)
        {
            hr = IFileDialogCustomize_RemoveControlItem(pfdc, i, j);
            ok(hr == S_OK, "got 0x%08x.\n", hr);
        }
    }
    hr = IFileDialogCustomize_EnableOpenDropDown(pfdc, ++i);
    todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr))
    {
        DWORD selected = -1;
        UINT j = 0;

        for(j = 0; j < 10; j++)
        {
            hr = IFileDialogCustomize_AddControlItem(pfdc, i, j, label);
            ok(hr == S_OK, "got 0x%08x.\n", hr);
        }

        hr = IFileDialogCustomize_GetSelectedControlItem(pfdc, i, &selected);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        ok(selected == 0, "got %d.\n", selected);

        cdstate = 0xdeadbeef;
        hr = IFileDialogCustomize_GetControlItemState(pfdc, i, 0, &cdstate);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        ok(cdstate == CDCS_ENABLEDVISIBLE, "got 0x%08x.\n", cdstate);
        hr = IFileDialogCustomize_SetControlItemState(pfdc, i, 0, 0);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        cdstate = 0xdeadbeef;
        hr = IFileDialogCustomize_GetControlItemState(pfdc, i, 0, &cdstate);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        ok(cdstate == 0, "got 0x%08x.\n", cdstate);
        hr = IFileDialogCustomize_SetControlItemState(pfdc, i, 0, CDCS_ENABLEDVISIBLE);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        cdstate = 0xdeadbeef;
        hr = IFileDialogCustomize_GetControlItemState(pfdc, i, 0, &cdstate);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        ok(cdstate == CDCS_ENABLEDVISIBLE, "got 0x%08x.\n", cdstate);
        hr = IFileDialogCustomize_SetSelectedControlItem(pfdc, i, 0);
        ok(hr == E_NOTIMPL, "got 0x%08x.\n", hr);

        hr = IFileDialogCustomize_RemoveAllControlItems(pfdc, i);
        ok(hr == E_NOTIMPL, "got 0x%08x.\n", hr);

        for(j = 0; j < 10; j++)
        {
            hr = IFileDialogCustomize_RemoveControlItem(pfdc, i, j);
            ok(hr == S_OK, "got 0x%08x.\n", hr);
        }
    }

    IFileDialogCustomize_Release(pfdc);
    ref = IFileOpenDialog_Release(pfod);
    ok(!ref, "Refcount not zero (%d).\n", ref);
}

START_TEST(itemdlg)
{
    OleInitialize(NULL);
    init_function_pointers();

    if(test_instantiation())
    {
        test_basics();
        test_advise();
        test_filename();
        test_customize();
    }
    else
        skip("Skipping all Item Dialog tests.\n");

    OleUninitialize();
}
