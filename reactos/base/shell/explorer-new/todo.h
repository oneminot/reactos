#ifndef __TODO_H
#define __TODO_H

/*
 * Stuff missing in our headers
 */

#define SM_REMOTECONTROL 0x2001

/* FIXME: Ugly hack!!! FIX ASAP! Move to uuid! */
static const GUID IID_HACK_IShellView2 = {0x88E39E80,0x3578,0x11CF,{0xAE,0x69,0x08,0x00,0x2B,0x2E,0x12,0x62}};
#define IID_IShellView2 IID_HACK_IShellView2
static const GUID IID_HACK_IShellView3 = {0xEC39FA88,0xF8AF,0x41CF,{0x84,0x21,0x38,0xBE,0xD2,0x8F,0x46,0x73}};
#define IID_IShellView3 IID_HACK_IShellView2
static const GUID VID_HACK_LargeIcons = {0x0057D0E0, 0x3573, 0x11CF, {0xAE, 0x69, 0x08, 0x00, 0x2B, 0x2E, 0x12, 0x62}};
#define VID_LargeIcons VID_HACK_LargeIcons

static const GUID IID_HACK_IDeskBarClient = {0xEB0FE175, 0x1A3A, 0x11D0, {0x89, 0xB3, 0x00, 0xA0, 0xC9, 0x0A, 0x90, 0xAC}};
#define IID_IDeskBarClient IID_HACK_IDeskBarClient
static const GUID IID_HACK_IDeskBar = {0xEB0FE173, 0x1A3A, 0x11D0, {0x89, 0xB3, 0x00, 0xA0, 0xC9, 0x0A, 0x90, 0xAC}};
#define IID_IDeskBar IID_HACK_IDeskBar

static const GUID IID_HACK_IMenuPopup = {0xD1E7AFEB,0x6A2E,0x11D0,{0x8C,0x78,0x00,0xC0,0x4F,0xD9,0x18,0xB4}};
#define IID_IMenuPopup IID_HACK_IMenuPopup
static const GUID IID_HACK_IBanneredBar = {0x596A9A94,0x013E,0x11D1,{0x8D,0x34,0x00,0xA0,0xC9,0x0F,0x27,0x19}};
#define IID_IBanneredBar IID_HACK_IBanneredBar

static const GUID IID_HACK_IInitializeObject = {0x4622AD16,0xFF23,0x11D0,{0x8D,0x34,0x00,0xA0,0xC9,0x0F,0x27,0x19}};
#define IID_IInitializeObject IID_HACK_IInitializeObject

static const GUID SID_HACK_SMenuPopup = {0xD1E7AFEB,0x6A2E,0x11D0,{0x8C,0x78,0x00,0xC0,0x4F,0xD9,0x18,0xB4}};
#define SID_SMenuPopup SID_HACK_SMenuPopup




#ifdef COBJMACROS
#define IDeskBarClient_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IDeskBarClient_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IDeskBarClient_Release(T) (T)->lpVtbl->Release(T)
#define IDeskBarClient_GetWindow(T,a) (T)->lpVtbl->GetWindow(T,a)
#define IDeskBarClient_ContextSensitiveHelp(T,a) (T)->lpVtbl->ContextSensitiveHelp(T,a)
#define IDeskBarClient_SetDeskBarSite(T,a) (T)->lpVtbl->SetDeskBarSite(T,a)
#define IDeskBarClient_SetModeDBC(T,a) (T)->lpVtbl->SetModeDBC(T,a)
#define IDeskBarClient_UIActivateDBC(T,a) (T)->lpVtbl->UIActivateDBC(T,a)
#define IDeskBarClient_GetSize(T,a,b) (T)->lpVtbl->GetSize(T,a,b)
#endif


#define SHGVSPB_PERUSER 0x1
#define SHGVSPB_PERFOLDER   0x4
#define SHGVSPB_ROAM    0x00000020
#define SHGVSPB_NOAUTODEFAULTS  0x80000000
#define SHGVSPB_FOLDER  (SHGVSPB_PERUSER | SHGVSPB_PERFOLDER)
#define SHGVSPB_FOLDERNODEFAULTS    (SHGVSPB_PERUSER | SHGVSPB_PERFOLDER | SHGVSPB_NOAUTODEFAULTS)


#define DBC_SHOW    1
#define DBC_HIDE    0

static const GUID IID_HACK_IShellService = {0x5836FB00,0x8187,0x11CF,{0xA1,0x2B,0x00,0xAA,0x00,0x4A,0xE8,0x37}};
#define IID_IShellService IID_HACK_IShellService

#define INTERFACE IShellService
DECLARE_INTERFACE_(IShellService,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IShellService methods ***/
    STDMETHOD_(HRESULT,SetOwner)(THIS_ IUnknown*) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IShellService_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IShellService_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IShellService_Release(T) (T)->lpVtbl->Release(T)
#define IShellService_SetOwner(T,a) (T)->lpVtbl->SetOwner(T,a)
#endif

#if 0
HRESULT WINAPI SHGetViewStatePropertyBag(LPCITEMIDLIST,LPCWSTR,DWORD,REFIID,PVOID*);/* FIXME: Parameter should be PCIDLIST_ABSOLUTE */
#else
typedef HRESULT (WINAPI *PSHGetViewStatePropertyBag)(LPCITEMIDLIST,LPCWSTR,DWORD,REFIID,PVOID*);
static HRESULT __inline
SHGetViewStatePropertyBag(IN LPCITEMIDLIST pidl,
                          IN LPCWSTR pszBagName,
                          IN DWORD dwFlags,
                          IN REFIID riid,
                          OUT PVOID* ppv)
{
    static PSHGetViewStatePropertyBag Func = NULL;

    if (Func == NULL)
    {
        HMODULE hShlwapi;
        hShlwapi = LoadLibrary(TEXT("SHLWAPI.DLL"));
        if (hShlwapi != NULL)
        {
            Func = (PSHGetViewStatePropertyBag)GetProcAddress(hShlwapi, "SHGetViewStatePropertyBag");
        }
    }

    if (Func != NULL)
    {
        return Func(pidl, pszBagName, dwFlags, riid, ppv);
    }

    MessageBox(NULL, TEXT("SHGetViewStatePropertyBag not available"), NULL, 0);
    return E_NOTIMPL;
}
#endif

#define PIDLIST_ABSOLUTE LPITEMIDLIST
PIDLIST_ABSOLUTE WINAPI SHCloneSpecialIDList(HWND hwnd, int csidl, BOOL fCreate);

enum
{
    BMICON_LARGE = 0,
    BMICON_SMALL
};
#define INTERFACE IBanneredBar
DECLARE_INTERFACE_(IBanneredBar,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IBanneredBar methods ***/
    STDMETHOD_(HRESULT,SetIconSize)(THIS_ DWORD) PURE;
    STDMETHOD_(HRESULT,GetIconSize)(THIS_ DWORD*) PURE;
    STDMETHOD_(HRESULT,SetBitmap)(THIS_ HBITMAP) PURE;
    STDMETHOD_(HRESULT,GetBitmap)(THIS_ HBITMAP*) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IBanneredBar_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IBanneredBar_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IBanneredBar_Release(T) (T)->lpVtbl->Release(T)
#define IBanneredBar_SetIconSize(T,a) (T)->lpVtbl->SetIconSize(T,a)
#define IBanneredBar_GetIconSize(T,a) (T)->lpVtbl->GetIconSize(T,a)
#define IBanneredBar_SetBitmap(T,a) (T)->lpVtbl->SetBitmap(T,a)
#define IBanneredBar_GetBitmap(T,a) (T)->lpVtbl->GetBitmap(T,a)
#endif




#define INTERFACE IMenuBand
DECLARE_INTERFACE_(IMenuBand,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IMenuBand methods ***/
    STDMETHOD_(HRESULT,IsMenuMessage)(THIS_ MSG*) PURE;
    STDMETHOD_(HRESULT,TranslateMenuMessage)(THIS_ MSG*,LRESULT*) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IMenuBand_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IMenuBand_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IMenuBand_Release(T) (T)->lpVtbl->Release(T)
#define IMenuBand_IsMenuMessage(T,a) (T)->lpVtbl->IsMenuMessage(T,a)
#define IMenuBand_TranslateMenuMessage(T,a,b) (T)->lpVtbl->TranslateMenuMessage(T,a,b)
#endif

#define INTERFACE IInitializeObject
DECLARE_INTERFACE_(IInitializeObject,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IInitializeObject methods ***/
    STDMETHOD_(HRESULT,Initialize)(THIS) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IInitializeObject_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IInitializeObject_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IInitializeObject_Release(T) (T)->lpVtbl->Release(T)
#define IInitializeObject_Initialize(T) (T)->lpVtbl->Initialize(T)
#endif

#endif /* __TODO_H */
