/*
 *  ITfInputProcessorProfiles implementation
 *
 *  Copyright 2009 Aric Stewart, CodeWeavers
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
 */

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <config.h>

//#include <stdarg.h>

#define COBJMACROS

#include <wine/debug.h>
//#include "windef.h"
#include <winbase.h>
#include <winreg.h>
//#include "winuser.h"
//#include "shlwapi.h"
//#include "winerror.h"
#include <objbase.h>
#include <olectl.h>

#include <wine/unicode.h>
#include <wine/list.h>

#include <msctf.h>
#include "msctf_internal.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctf);

static const WCHAR szwLngp[] = {'L','a','n','g','u','a','g','e','P','r','o','f','i','l','e',0};
static const WCHAR szwEnable[] = {'E','n','a','b','l','e',0};
static const WCHAR szwTipfmt[] = {'%','s','\\','%','s',0};
static const WCHAR szwFullLangfmt[] = {'%','s','\\','%','s','\\','%','s','\\','0','x','%','0','8','x','\\','%','s',0};

static const WCHAR szwAssemblies[] = {'A','s','s','e','m','b','l','i','e','s',0};
static const WCHAR szwDefault[] = {'D','e','f','a','u','l','t',0};
static const WCHAR szwProfile[] = {'P','r','o','f','i','l','e',0};
static const WCHAR szwDefaultFmt[] = {'%','s','\\','%','s','\\','0','x','%','0','8','x','\\','%','s',0};

typedef struct tagInputProcessorProfilesSink {
    struct list         entry;
    union {
        /* InputProcessorProfile Sinks */
        IUnknown            *pIUnknown;
        ITfLanguageProfileNotifySink *pITfLanguageProfileNotifySink;
    } interfaces;
} InputProcessorProfilesSink;

typedef struct tagInputProcessorProfiles {
    const ITfInputProcessorProfilesVtbl *InputProcessorProfilesVtbl;
    const ITfSourceVtbl *SourceVtbl;
    /* const ITfInputProcessorProfileMgrVtbl *InputProcessorProfileMgrVtbl; */
    /* const ITfInputProcessorProfilesExVtbl *InputProcessorProfilesExVtbl; */
    /* const ITfInputProcessorProfileSubstituteLayoutVtbl *InputProcessorProfileSubstituteLayoutVtbl; */
    LONG refCount;

    LANGID  currentLanguage;

    struct list     LanguageProfileNotifySink;
} InputProcessorProfiles;

typedef struct tagProfilesEnumGuid {
    const IEnumGUIDVtbl *Vtbl;
    LONG refCount;

    HKEY key;
    DWORD next_index;
} ProfilesEnumGuid;

typedef struct tagEnumTfLanguageProfiles {
    const IEnumTfLanguageProfilesVtbl *Vtbl;
    LONG refCount;

    HKEY    tipkey;
    DWORD   tip_index;
    WCHAR   szwCurrentClsid[39];

    HKEY    langkey;
    DWORD   lang_index;

    LANGID  langid;
    ITfCategoryMgr *catmgr;
} EnumTfLanguageProfiles;

static HRESULT ProfilesEnumGuid_Constructor(IEnumGUID **ppOut);
static HRESULT EnumTfLanguageProfiles_Constructor(LANGID langid, IEnumTfLanguageProfiles **ppOut);

static inline InputProcessorProfiles *impl_from_ITfSourceVtbl(ITfSource *iface)
{
    return (InputProcessorProfiles *)((char *)iface - FIELD_OFFSET(InputProcessorProfiles,SourceVtbl));
}

static void free_sink(InputProcessorProfilesSink *sink)
{
        IUnknown_Release(sink->interfaces.pIUnknown);
        HeapFree(GetProcessHeap(),0,sink);
}

static void InputProcessorProfiles_Destructor(InputProcessorProfiles *This)
{
    struct list *cursor, *cursor2;
    TRACE("destroying %p\n", This);

    /* free sinks */
    LIST_FOR_EACH_SAFE(cursor, cursor2, &This->LanguageProfileNotifySink)
    {
        InputProcessorProfilesSink* sink = LIST_ENTRY(cursor,InputProcessorProfilesSink,entry);
        list_remove(cursor);
        free_sink(sink);
    }

    HeapFree(GetProcessHeap(),0,This);
}

static void add_userkey( REFCLSID rclsid, LANGID langid,
                                REFGUID guidProfile)
{
    HKEY key;
    WCHAR buf[39];
    WCHAR buf2[39];
    WCHAR fullkey[168];
    DWORD disposition = 0;
    ULONG res;

    TRACE("\n");

    StringFromGUID2(rclsid, buf, 39);
    StringFromGUID2(guidProfile, buf2, 39);
    sprintfW(fullkey,szwFullLangfmt,szwSystemTIPKey,buf,szwLngp,langid,buf2);

    res = RegCreateKeyExW(HKEY_CURRENT_USER,fullkey, 0, NULL, 0,
                   KEY_READ | KEY_WRITE, NULL, &key, &disposition);

    if (!res && disposition == REG_CREATED_NEW_KEY)
    {
        DWORD zero = 0x0;
        RegSetValueExW(key, szwEnable, 0, REG_DWORD, (LPBYTE)&zero, sizeof(DWORD));
    }

    if (!res)
        RegCloseKey(key);
}

static HRESULT WINAPI InputProcessorProfiles_QueryInterface(ITfInputProcessorProfiles *iface, REFIID iid, LPVOID *ppvOut)
{
    InputProcessorProfiles *This = (InputProcessorProfiles *)iface;
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITfInputProcessorProfiles))
    {
        *ppvOut = This;
    }
    else if (IsEqualIID(iid, &IID_ITfSource))
    {
        *ppvOut = &This->SourceVtbl;
    }

    if (*ppvOut)
    {
        IUnknown_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI InputProcessorProfiles_AddRef(ITfInputProcessorProfiles *iface)
{
    InputProcessorProfiles *This = (InputProcessorProfiles *)iface;
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI InputProcessorProfiles_Release(ITfInputProcessorProfiles *iface)
{
    InputProcessorProfiles *This = (InputProcessorProfiles *)iface;
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        InputProcessorProfiles_Destructor(This);
    return ret;
}

/*****************************************************
 * ITfInputProcessorProfiles functions
 *****************************************************/
static HRESULT WINAPI InputProcessorProfiles_Register(
        ITfInputProcessorProfiles *iface, REFCLSID rclsid)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    HKEY tipkey;
    WCHAR buf[39];
    WCHAR fullkey[68];

    TRACE("(%p) %s\n",This,debugstr_guid(rclsid));

    StringFromGUID2(rclsid, buf, 39);
    sprintfW(fullkey,szwTipfmt,szwSystemTIPKey,buf);

    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE,fullkey, 0, NULL, 0,
                    KEY_READ | KEY_WRITE, NULL, &tipkey, NULL) != ERROR_SUCCESS)
        return E_FAIL;

    RegCloseKey(tipkey);

    return S_OK;
}

static HRESULT WINAPI InputProcessorProfiles_Unregister(
        ITfInputProcessorProfiles *iface, REFCLSID rclsid)
{
    WCHAR buf[39];
    WCHAR fullkey[68];
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;

    TRACE("(%p) %s\n",This,debugstr_guid(rclsid));

    StringFromGUID2(rclsid, buf, 39);
    sprintfW(fullkey,szwTipfmt,szwSystemTIPKey,buf);

    RegDeleteTreeW(HKEY_LOCAL_MACHINE, fullkey);
    RegDeleteTreeW(HKEY_CURRENT_USER, fullkey);

    return S_OK;
}

static HRESULT WINAPI InputProcessorProfiles_AddLanguageProfile(
        ITfInputProcessorProfiles *iface, REFCLSID rclsid,
        LANGID langid, REFGUID guidProfile, const WCHAR *pchDesc,
        ULONG cchDesc, const WCHAR *pchIconFile, ULONG cchFile,
        ULONG uIconIndex)
{
    HKEY tipkey,fmtkey;
    WCHAR buf[39];
    WCHAR fullkey[100];
    ULONG res;
    DWORD disposition = 0;

    static const WCHAR fmt2[] = {'%','s','\\','0','x','%','0','8','x','\\','%','s',0};
    static const WCHAR desc[] = {'D','e','s','c','r','i','p','t','i','o','n',0};
    static const WCHAR icnf[] = {'I','c','o','n','F','i','l','e',0};
    static const WCHAR icni[] = {'I','c','o','n','I','n','d','e','x',0};

    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;

    TRACE("(%p) %s %x %s %s %s %i\n",This,debugstr_guid(rclsid), langid,
            debugstr_guid(guidProfile), debugstr_wn(pchDesc,cchDesc),
            debugstr_wn(pchIconFile,cchFile),uIconIndex);

    StringFromGUID2(rclsid, buf, 39);
    sprintfW(fullkey,szwTipfmt,szwSystemTIPKey,buf);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,fullkey, 0, KEY_READ | KEY_WRITE,
                &tipkey ) != ERROR_SUCCESS)
        return E_FAIL;

    StringFromGUID2(guidProfile, buf, 39);
    sprintfW(fullkey,fmt2,szwLngp,langid,buf);

    res = RegCreateKeyExW(tipkey,fullkey, 0, NULL, 0, KEY_READ | KEY_WRITE,
            NULL, &fmtkey, &disposition);

    if (!res)
    {
        DWORD zero = 0x0;
        RegSetValueExW(fmtkey, desc, 0, REG_SZ, (const BYTE*)pchDesc, cchDesc * sizeof(WCHAR));
        RegSetValueExW(fmtkey, icnf, 0, REG_SZ, (const BYTE*)pchIconFile, cchFile * sizeof(WCHAR));
        RegSetValueExW(fmtkey, icni, 0, REG_DWORD, (LPBYTE)&uIconIndex, sizeof(DWORD));
        if (disposition == REG_CREATED_NEW_KEY)
            RegSetValueExW(fmtkey, szwEnable, 0, REG_DWORD, (LPBYTE)&zero, sizeof(DWORD));
        RegCloseKey(fmtkey);

        add_userkey(rclsid, langid, guidProfile);
    }
    RegCloseKey(tipkey);

    if (!res)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI InputProcessorProfiles_RemoveLanguageProfile(
        ITfInputProcessorProfiles *iface, REFCLSID rclsid, LANGID langid,
        REFGUID guidProfile)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InputProcessorProfiles_EnumInputProcessorInfo(
        ITfInputProcessorProfiles *iface, IEnumGUID **ppEnum)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    TRACE("(%p) %p\n",This,ppEnum);
    return ProfilesEnumGuid_Constructor(ppEnum);
}

static HRESULT WINAPI InputProcessorProfiles_GetDefaultLanguageProfile(
        ITfInputProcessorProfiles *iface, LANGID langid, REFGUID catid,
        CLSID *pclsid, GUID *pguidProfile)
{
    WCHAR fullkey[168];
    WCHAR buf[39];
    HKEY hkey;
    DWORD count;
    ULONG res;
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;

    TRACE("%p) %x %s %p %p\n",This, langid, debugstr_guid(catid),pclsid,pguidProfile);

    if (!catid || !pclsid || !pguidProfile)
        return E_INVALIDARG;

    StringFromGUID2(catid, buf, 39);
    sprintfW(fullkey, szwDefaultFmt, szwSystemCTFKey, szwAssemblies, langid, buf);

    if (RegOpenKeyExW(HKEY_CURRENT_USER, fullkey, 0, KEY_READ | KEY_WRITE,
                &hkey ) != ERROR_SUCCESS)
        return S_FALSE;

    count = sizeof(buf);
    res = RegQueryValueExW(hkey, szwDefault, 0, NULL, (LPBYTE)buf, &count);
    if (res != ERROR_SUCCESS)
    {
        RegCloseKey(hkey);
        return S_FALSE;
    }
    CLSIDFromString(buf,pclsid);

    res = RegQueryValueExW(hkey, szwProfile, 0, NULL, (LPBYTE)buf, &count);
    if (res == ERROR_SUCCESS)
        CLSIDFromString(buf,pguidProfile);

    RegCloseKey(hkey);

    return S_OK;
}

static HRESULT WINAPI InputProcessorProfiles_SetDefaultLanguageProfile(
        ITfInputProcessorProfiles *iface, LANGID langid, REFCLSID rclsid,
        REFGUID guidProfiles)
{
    WCHAR fullkey[168];
    WCHAR buf[39];
    HKEY hkey;
    GUID catid;
    HRESULT hr;
    ITfCategoryMgr *catmgr;
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    static const GUID * tipcats[3] = { &GUID_TFCAT_TIP_KEYBOARD,
                                       &GUID_TFCAT_TIP_SPEECH,
                                       &GUID_TFCAT_TIP_HANDWRITING };

    TRACE("%p) %x %s %s\n",This, langid, debugstr_guid(rclsid),debugstr_guid(guidProfiles));

    if (!rclsid || !guidProfiles)
        return E_INVALIDARG;

    hr = CategoryMgr_Constructor(NULL,(IUnknown**)&catmgr);

    if (FAILED(hr))
        return hr;

    if (ITfCategoryMgr_FindClosestCategory(catmgr, rclsid,
            &catid, tipcats, 3) != S_OK)
        hr = ITfCategoryMgr_FindClosestCategory(catmgr, rclsid,
                &catid, NULL, 0);
    ITfCategoryMgr_Release(catmgr);

    if (FAILED(hr))
        return E_FAIL;

    StringFromGUID2(&catid, buf, 39);
    sprintfW(fullkey, szwDefaultFmt, szwSystemCTFKey, szwAssemblies, langid, buf);

    if (RegCreateKeyExW(HKEY_CURRENT_USER, fullkey, 0, NULL, 0, KEY_READ | KEY_WRITE,
                NULL, &hkey, NULL ) != ERROR_SUCCESS)
        return E_FAIL;

    StringFromGUID2(rclsid, buf, 39);
    RegSetValueExW(hkey, szwDefault, 0, REG_SZ, (LPBYTE)buf, sizeof(buf));
    StringFromGUID2(guidProfiles, buf, 39);
    RegSetValueExW(hkey, szwProfile, 0, REG_SZ, (LPBYTE)buf, sizeof(buf));
    RegCloseKey(hkey);

    return S_OK;
}

static HRESULT WINAPI InputProcessorProfiles_ActivateLanguageProfile(
        ITfInputProcessorProfiles *iface, REFCLSID rclsid, LANGID langid,
        REFGUID guidProfiles)
{
    HRESULT hr;
    BOOL enabled;
    TF_LANGUAGEPROFILE LanguageProfile;
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;

    TRACE("(%p) %s %x %s\n",This,debugstr_guid(rclsid),langid,debugstr_guid(guidProfiles));

    if (langid != This->currentLanguage) return E_INVALIDARG;

    if (get_active_textservice(rclsid,NULL))
    {
        TRACE("Already Active\n");
        return E_FAIL;
    }

    hr = ITfInputProcessorProfiles_IsEnabledLanguageProfile(iface, rclsid,
            langid, guidProfiles, &enabled);
    if (FAILED(hr) || !enabled)
    {
        TRACE("Not Enabled\n");
        return E_FAIL;
    }

    LanguageProfile.clsid = *rclsid;
    LanguageProfile.langid = langid;
    LanguageProfile.guidProfile = *guidProfiles;

    hr = add_active_textservice(&LanguageProfile);

    return hr;
}

static HRESULT WINAPI InputProcessorProfiles_GetActiveLanguageProfile(
        ITfInputProcessorProfiles *iface, REFCLSID rclsid, LANGID *plangid,
        GUID *pguidProfile)
{
    TF_LANGUAGEPROFILE profile;
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;

    TRACE("(%p) %s %p %p\n",This,debugstr_guid(rclsid),plangid,pguidProfile);

    if (!rclsid || !plangid || !pguidProfile)
        return E_INVALIDARG;

    if (get_active_textservice(rclsid, &profile))
    {
        *plangid = profile.langid;
        *pguidProfile = profile.guidProfile;
        return S_OK;
    }
    else
    {
        *pguidProfile = GUID_NULL;
        return S_FALSE;
    }
}

static HRESULT WINAPI InputProcessorProfiles_GetLanguageProfileDescription(
        ITfInputProcessorProfiles *iface, REFCLSID rclsid, LANGID langid,
        REFGUID guidProfile, BSTR *pbstrProfile)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InputProcessorProfiles_GetCurrentLanguage(
        ITfInputProcessorProfiles *iface, LANGID *plangid)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    TRACE("(%p) 0x%x\n",This,This->currentLanguage);

    if (!plangid)
        return E_INVALIDARG;

    *plangid = This->currentLanguage;

    return S_OK;
}

static HRESULT WINAPI InputProcessorProfiles_ChangeCurrentLanguage(
        ITfInputProcessorProfiles *iface, LANGID langid)
{
    struct list *cursor;
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    BOOL accept;

    FIXME("STUB:(%p)\n",This);

    LIST_FOR_EACH(cursor, &This->LanguageProfileNotifySink)
    {
        InputProcessorProfilesSink* sink = LIST_ENTRY(cursor,InputProcessorProfilesSink,entry);
        accept = TRUE;
        ITfLanguageProfileNotifySink_OnLanguageChange(sink->interfaces.pITfLanguageProfileNotifySink, langid, &accept);
        if (!accept)
            return  E_FAIL;
    }

    /* TODO:  On successful language change call OnLanguageChanged sink */
    return E_NOTIMPL;
}

static HRESULT WINAPI InputProcessorProfiles_GetLanguageList(
        ITfInputProcessorProfiles *iface, LANGID **ppLangId, ULONG *pulCount)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    FIXME("Semi-STUB:(%p)\n",This);
    *ppLangId = CoTaskMemAlloc(sizeof(LANGID));
    **ppLangId = This->currentLanguage;
    *pulCount = 1;
    return S_OK;
}

static HRESULT WINAPI InputProcessorProfiles_EnumLanguageProfiles(
        ITfInputProcessorProfiles *iface, LANGID langid,
        IEnumTfLanguageProfiles **ppEnum)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    TRACE("(%p) %x %p\n",This,langid,ppEnum);
    return EnumTfLanguageProfiles_Constructor(langid, ppEnum);
}

static HRESULT WINAPI InputProcessorProfiles_EnableLanguageProfile(
        ITfInputProcessorProfiles *iface, REFCLSID rclsid, LANGID langid,
        REFGUID guidProfile, BOOL fEnable)
{
    HKEY key;
    WCHAR buf[39];
    WCHAR buf2[39];
    WCHAR fullkey[168];
    ULONG res;

    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    TRACE("(%p) %s %x %s %i\n",This, debugstr_guid(rclsid), langid, debugstr_guid(guidProfile), fEnable);

    StringFromGUID2(rclsid, buf, 39);
    StringFromGUID2(guidProfile, buf2, 39);
    sprintfW(fullkey,szwFullLangfmt,szwSystemTIPKey,buf,szwLngp,langid,buf2);

    res = RegOpenKeyExW(HKEY_CURRENT_USER, fullkey, 0, KEY_READ | KEY_WRITE, &key);

    if (!res)
    {
        RegSetValueExW(key, szwEnable, 0, REG_DWORD, (LPBYTE)&fEnable, sizeof(DWORD));
        RegCloseKey(key);
    }
    else
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI InputProcessorProfiles_IsEnabledLanguageProfile(
        ITfInputProcessorProfiles *iface, REFCLSID rclsid, LANGID langid,
        REFGUID guidProfile, BOOL *pfEnable)
{
    HKEY key;
    WCHAR buf[39];
    WCHAR buf2[39];
    WCHAR fullkey[168];
    ULONG res;

    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    TRACE("(%p) %s, %i, %s, %p\n",This,debugstr_guid(rclsid),langid,debugstr_guid(guidProfile),pfEnable);

    if (!pfEnable)
        return E_INVALIDARG;

    StringFromGUID2(rclsid, buf, 39);
    StringFromGUID2(guidProfile, buf2, 39);
    sprintfW(fullkey,szwFullLangfmt,szwSystemTIPKey,buf,szwLngp,langid,buf2);

    res = RegOpenKeyExW(HKEY_CURRENT_USER, fullkey, 0, KEY_READ | KEY_WRITE, &key);

    if (!res)
    {
        DWORD count = sizeof(DWORD);
        res = RegQueryValueExW(key, szwEnable, 0, NULL, (LPBYTE)pfEnable, &count);
        RegCloseKey(key);
    }

    if (res)  /* Try Default */
    {
        res = RegOpenKeyExW(HKEY_LOCAL_MACHINE, fullkey, 0, KEY_READ | KEY_WRITE, &key);

        if (!res)
        {
            DWORD count = sizeof(DWORD);
            res = RegQueryValueExW(key, szwEnable, 0, NULL, (LPBYTE)pfEnable, &count);
            RegCloseKey(key);
        }
    }

    if (!res)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI InputProcessorProfiles_EnableLanguageProfileByDefault(
        ITfInputProcessorProfiles *iface, REFCLSID rclsid, LANGID langid,
        REFGUID guidProfile, BOOL fEnable)
{
    HKEY key;
    WCHAR buf[39];
    WCHAR buf2[39];
    WCHAR fullkey[168];
    ULONG res;

    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    TRACE("(%p) %s %x %s %i\n",This,debugstr_guid(rclsid),langid,debugstr_guid(guidProfile),fEnable);

    StringFromGUID2(rclsid, buf, 39);
    StringFromGUID2(guidProfile, buf2, 39);
    sprintfW(fullkey,szwFullLangfmt,szwSystemTIPKey,buf,szwLngp,langid,buf2);

    res = RegOpenKeyExW(HKEY_LOCAL_MACHINE, fullkey, 0, KEY_READ | KEY_WRITE, &key);

    if (!res)
    {
        RegSetValueExW(key, szwEnable, 0, REG_DWORD, (LPBYTE)&fEnable, sizeof(DWORD));
        RegCloseKey(key);
    }
    else
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI InputProcessorProfiles_SubstituteKeyboardLayout(
        ITfInputProcessorProfiles *iface, REFCLSID rclsid, LANGID langid,
        REFGUID guidProfile, HKL hKL)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}


static const ITfInputProcessorProfilesVtbl InputProcessorProfiles_InputProcessorProfilesVtbl =
{
    InputProcessorProfiles_QueryInterface,
    InputProcessorProfiles_AddRef,
    InputProcessorProfiles_Release,

    InputProcessorProfiles_Register,
    InputProcessorProfiles_Unregister,
    InputProcessorProfiles_AddLanguageProfile,
    InputProcessorProfiles_RemoveLanguageProfile,
    InputProcessorProfiles_EnumInputProcessorInfo,
    InputProcessorProfiles_GetDefaultLanguageProfile,
    InputProcessorProfiles_SetDefaultLanguageProfile,
    InputProcessorProfiles_ActivateLanguageProfile,
    InputProcessorProfiles_GetActiveLanguageProfile,
    InputProcessorProfiles_GetLanguageProfileDescription,
    InputProcessorProfiles_GetCurrentLanguage,
    InputProcessorProfiles_ChangeCurrentLanguage,
    InputProcessorProfiles_GetLanguageList,
    InputProcessorProfiles_EnumLanguageProfiles,
    InputProcessorProfiles_EnableLanguageProfile,
    InputProcessorProfiles_IsEnabledLanguageProfile,
    InputProcessorProfiles_EnableLanguageProfileByDefault,
    InputProcessorProfiles_SubstituteKeyboardLayout
};

/*****************************************************
 * ITfSource functions
 *****************************************************/
static HRESULT WINAPI IPPSource_QueryInterface(ITfSource *iface, REFIID iid, LPVOID *ppvOut)
{
    InputProcessorProfiles *This = impl_from_ITfSourceVtbl(iface);
    return InputProcessorProfiles_QueryInterface((ITfInputProcessorProfiles *)This, iid, *ppvOut);
}

static ULONG WINAPI IPPSource_AddRef(ITfSource *iface)
{
    InputProcessorProfiles *This = impl_from_ITfSourceVtbl(iface);
    return InputProcessorProfiles_AddRef((ITfInputProcessorProfiles*)This);
}

static ULONG WINAPI IPPSource_Release(ITfSource *iface)
{
    InputProcessorProfiles *This = impl_from_ITfSourceVtbl(iface);
    return InputProcessorProfiles_Release((ITfInputProcessorProfiles *)This);
}

static HRESULT WINAPI IPPSource_AdviseSink(ITfSource *iface,
        REFIID riid, IUnknown *punk, DWORD *pdwCookie)
{
    InputProcessorProfilesSink *ipps;
    InputProcessorProfiles *This = impl_from_ITfSourceVtbl(iface);

    TRACE("(%p) %s %p %p\n",This,debugstr_guid(riid),punk,pdwCookie);

    if (!riid || !punk || !pdwCookie)
        return E_INVALIDARG;

    if (IsEqualIID(riid, &IID_ITfLanguageProfileNotifySink))
    {
        ipps = HeapAlloc(GetProcessHeap(),0,sizeof(InputProcessorProfilesSink));
        if (!ipps)
            return E_OUTOFMEMORY;
        if (FAILED(IUnknown_QueryInterface(punk, riid, (LPVOID *)&ipps->interfaces.pITfLanguageProfileNotifySink)))
        {
            HeapFree(GetProcessHeap(),0,ipps);
            return CONNECT_E_CANNOTCONNECT;
        }
        list_add_head(&This->LanguageProfileNotifySink,&ipps->entry);
        *pdwCookie = generate_Cookie(COOKIE_MAGIC_IPPSINK, ipps);
    }
    else
    {
        FIXME("(%p) Unhandled Sink: %s\n",This,debugstr_guid(riid));
        return E_NOTIMPL;
    }

    TRACE("cookie %x\n",*pdwCookie);

    return S_OK;
}

static HRESULT WINAPI IPPSource_UnadviseSink(ITfSource *iface, DWORD pdwCookie)
{
    InputProcessorProfilesSink *sink;
    InputProcessorProfiles *This = impl_from_ITfSourceVtbl(iface);

    TRACE("(%p) %x\n",This,pdwCookie);

    if (get_Cookie_magic(pdwCookie)!=COOKIE_MAGIC_IPPSINK)
        return E_INVALIDARG;

    sink = (InputProcessorProfilesSink*)remove_Cookie(pdwCookie);
    if (!sink)
        return CONNECT_E_NOCONNECTION;

    list_remove(&sink->entry);
    free_sink(sink);

    return S_OK;
}

static const ITfSourceVtbl InputProcessorProfiles_SourceVtbl =
{
    IPPSource_QueryInterface,
    IPPSource_AddRef,
    IPPSource_Release,

    IPPSource_AdviseSink,
    IPPSource_UnadviseSink,
};

HRESULT InputProcessorProfiles_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    InputProcessorProfiles *This;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(InputProcessorProfiles));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->InputProcessorProfilesVtbl= &InputProcessorProfiles_InputProcessorProfilesVtbl;
    This->SourceVtbl = &InputProcessorProfiles_SourceVtbl;
    This->refCount = 1;
    This->currentLanguage = GetUserDefaultLCID();

    list_init(&This->LanguageProfileNotifySink);

    TRACE("returning %p\n", This);
    *ppOut = (IUnknown *)This;
    return S_OK;
}

/**************************************************
 * IEnumGUID implementation for ITfInputProcessorProfiles::EnumInputProcessorInfo
 **************************************************/
static void ProfilesEnumGuid_Destructor(ProfilesEnumGuid *This)
{
    TRACE("destroying %p\n", This);
    RegCloseKey(This->key);
    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI ProfilesEnumGuid_QueryInterface(IEnumGUID *iface, REFIID iid, LPVOID *ppvOut)
{
    ProfilesEnumGuid *This = (ProfilesEnumGuid *)iface;
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IEnumGUID))
    {
        *ppvOut = This;
    }

    if (*ppvOut)
    {
        IUnknown_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ProfilesEnumGuid_AddRef(IEnumGUID *iface)
{
    ProfilesEnumGuid *This = (ProfilesEnumGuid*)iface;
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI ProfilesEnumGuid_Release(IEnumGUID *iface)
{
    ProfilesEnumGuid *This = (ProfilesEnumGuid *)iface;
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        ProfilesEnumGuid_Destructor(This);
    return ret;
}

/*****************************************************
 * IEnumGuid functions
 *****************************************************/
static HRESULT WINAPI ProfilesEnumGuid_Next( LPENUMGUID iface,
    ULONG celt, GUID *rgelt, ULONG *pceltFetched)
{
    ProfilesEnumGuid *This = (ProfilesEnumGuid *)iface;
    ULONG fetched = 0;

    TRACE("(%p)\n",This);

    if (rgelt == NULL) return E_POINTER;

    if (This->key) while (fetched < celt)
    {
        LSTATUS res;
        HRESULT hr;
        WCHAR catid[39];
        DWORD cName = 39;

        res = RegEnumKeyExW(This->key, This->next_index, catid, &cName,
                    NULL, NULL, NULL, NULL);
        if (res != ERROR_SUCCESS && res != ERROR_MORE_DATA) break;
        ++(This->next_index);

        hr = CLSIDFromString(catid, rgelt);
        if (FAILED(hr)) continue;

        ++fetched;
        ++rgelt;
    }

    if (pceltFetched) *pceltFetched = fetched;
    return fetched == celt ? S_OK : S_FALSE;
}

static HRESULT WINAPI ProfilesEnumGuid_Skip( LPENUMGUID iface, ULONG celt)
{
    ProfilesEnumGuid *This = (ProfilesEnumGuid *)iface;
    TRACE("(%p)\n",This);

    This->next_index += celt;
    return S_OK;
}

static HRESULT WINAPI ProfilesEnumGuid_Reset( LPENUMGUID iface)
{
    ProfilesEnumGuid *This = (ProfilesEnumGuid *)iface;
    TRACE("(%p)\n",This);
    This->next_index = 0;
    return S_OK;
}

static HRESULT WINAPI ProfilesEnumGuid_Clone( LPENUMGUID iface,
    IEnumGUID **ppenum)
{
    ProfilesEnumGuid *This = (ProfilesEnumGuid *)iface;
    HRESULT res;

    TRACE("(%p)\n",This);

    if (ppenum == NULL) return E_POINTER;

    res = ProfilesEnumGuid_Constructor(ppenum);
    if (SUCCEEDED(res))
    {
        ProfilesEnumGuid *new_This = (ProfilesEnumGuid *)*ppenum;
        new_This->next_index = This->next_index;
    }
    return res;
}

static const IEnumGUIDVtbl IEnumGUID_Vtbl ={
    ProfilesEnumGuid_QueryInterface,
    ProfilesEnumGuid_AddRef,
    ProfilesEnumGuid_Release,

    ProfilesEnumGuid_Next,
    ProfilesEnumGuid_Skip,
    ProfilesEnumGuid_Reset,
    ProfilesEnumGuid_Clone
};

static HRESULT ProfilesEnumGuid_Constructor(IEnumGUID **ppOut)
{
    ProfilesEnumGuid *This;

    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ProfilesEnumGuid));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->Vtbl= &IEnumGUID_Vtbl;
    This->refCount = 1;

    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, szwSystemTIPKey, 0, NULL, 0,
                    KEY_READ | KEY_WRITE, NULL, &This->key, NULL) != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, This);
        return E_FAIL;
    }

    TRACE("returning %p\n", This);
    *ppOut = (IEnumGUID*)This;
    return S_OK;
}

/**************************************************
 * IEnumTfLanguageProfiles implementation
 **************************************************/
static void EnumTfLanguageProfiles_Destructor(EnumTfLanguageProfiles *This)
{
    TRACE("destroying %p\n", This);
    RegCloseKey(This->tipkey);
    if (This->langkey)
        RegCloseKey(This->langkey);
    ITfCategoryMgr_Release(This->catmgr);
    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI EnumTfLanguageProfiles_QueryInterface(IEnumTfLanguageProfiles *iface, REFIID iid, LPVOID *ppvOut)
{
    EnumTfLanguageProfiles *This = (EnumTfLanguageProfiles *)iface;
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IEnumTfLanguageProfiles))
    {
        *ppvOut = This;
    }

    if (*ppvOut)
    {
        IUnknown_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI EnumTfLanguageProfiles_AddRef(IEnumTfLanguageProfiles *iface)
{
    EnumTfLanguageProfiles *This = (EnumTfLanguageProfiles*)iface;
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI EnumTfLanguageProfiles_Release(IEnumTfLanguageProfiles *iface)
{
    EnumTfLanguageProfiles *This = (EnumTfLanguageProfiles *)iface;
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        EnumTfLanguageProfiles_Destructor(This);
    return ret;
}

/*****************************************************
 * IEnumGuid functions
 *****************************************************/
static INT next_LanguageProfile(EnumTfLanguageProfiles *This, CLSID clsid, TF_LANGUAGEPROFILE *tflp)
{
    WCHAR fullkey[168];
    ULONG res;
    WCHAR profileid[39];
    DWORD cName = 39;
    GUID  profile;

    static const WCHAR fmt[] = {'%','s','\\','%','s','\\','0','x','%','0','8','x',0};

    if (This->langkey == NULL)
    {
        sprintfW(fullkey,fmt,This->szwCurrentClsid,szwLngp,This->langid);
        res = RegOpenKeyExW(This->tipkey, fullkey, 0, KEY_READ | KEY_WRITE, &This->langkey);
        if (res)
        {
            This->langkey = NULL;
            return -1;
        }
        This->lang_index = 0;
    }
    res = RegEnumKeyExW(This->langkey, This->lang_index, profileid, &cName,
                NULL, NULL, NULL, NULL);
    if (res != ERROR_SUCCESS && res != ERROR_MORE_DATA)
    {
        RegCloseKey(This->langkey);
        This->langkey = NULL;
        return -1;
    }
    ++(This->lang_index);

    if (tflp)
    {
        static const GUID * tipcats[3] = { &GUID_TFCAT_TIP_KEYBOARD,
                                           &GUID_TFCAT_TIP_SPEECH,
                                           &GUID_TFCAT_TIP_HANDWRITING };
        res = CLSIDFromString(profileid, &profile);
        if (FAILED(res)) return 0;

        tflp->clsid = clsid;
        tflp->langid = This->langid;
        tflp->fActive = get_active_textservice(&clsid, NULL);
        tflp->guidProfile = profile;
        if (ITfCategoryMgr_FindClosestCategory(This->catmgr, &clsid,
                &tflp->catid, tipcats, 3) != S_OK)
            ITfCategoryMgr_FindClosestCategory(This->catmgr, &clsid,
                    &tflp->catid, NULL, 0);
    }

    return 1;
}

static HRESULT WINAPI EnumTfLanguageProfiles_Next(IEnumTfLanguageProfiles *iface,
    ULONG ulCount, TF_LANGUAGEPROFILE *pProfile, ULONG *pcFetch)
{
    EnumTfLanguageProfiles *This = (EnumTfLanguageProfiles *)iface;
    ULONG fetched = 0;

    TRACE("(%p)\n",This);

    if (pProfile == NULL) return E_POINTER;

    if (This->tipkey) while (fetched < ulCount)
    {
        LSTATUS res;
        HRESULT hr;
        DWORD cName = 39;
        GUID clsid;

        res = RegEnumKeyExW(This->tipkey, This->tip_index,
                    This->szwCurrentClsid, &cName, NULL, NULL, NULL, NULL);
        if (res != ERROR_SUCCESS && res != ERROR_MORE_DATA) break;
        ++(This->tip_index);
        hr = CLSIDFromString(This->szwCurrentClsid, &clsid);
        if (FAILED(hr)) continue;

        while ( fetched < ulCount)
        {
            INT res = next_LanguageProfile(This, clsid, pProfile);
            if (res == 1)
            {
                ++fetched;
                ++pProfile;
            }
            else if (res == -1)
                break;
            else
                continue;
        }
    }

    if (pcFetch) *pcFetch = fetched;
    return fetched == ulCount ? S_OK : S_FALSE;
}

static HRESULT WINAPI EnumTfLanguageProfiles_Skip( IEnumTfLanguageProfiles* iface, ULONG celt)
{
    EnumTfLanguageProfiles *This = (EnumTfLanguageProfiles *)iface;
    FIXME("STUB (%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumTfLanguageProfiles_Reset( IEnumTfLanguageProfiles* iface)
{
    EnumTfLanguageProfiles *This = (EnumTfLanguageProfiles *)iface;
    TRACE("(%p)\n",This);
    This->tip_index = 0;
    if (This->langkey)
        RegCloseKey(This->langkey);
    This->langkey = NULL;
    This->lang_index = 0;
    return S_OK;
}

static HRESULT WINAPI EnumTfLanguageProfiles_Clone( IEnumTfLanguageProfiles *iface,
    IEnumTfLanguageProfiles **ppenum)
{
    EnumTfLanguageProfiles *This = (EnumTfLanguageProfiles *)iface;
    HRESULT res;

    TRACE("(%p)\n",This);

    if (ppenum == NULL) return E_POINTER;

    res = EnumTfLanguageProfiles_Constructor(This->langid, ppenum);
    if (SUCCEEDED(res))
    {
        EnumTfLanguageProfiles *new_This = (EnumTfLanguageProfiles *)*ppenum;
        new_This->tip_index = This->tip_index;
        lstrcpynW(new_This->szwCurrentClsid,This->szwCurrentClsid,39);

        if (This->langkey)
        {
            WCHAR fullkey[168];
            static const WCHAR fmt[] = {'%','s','\\','%','s','\\','0','x','%','0','8','x',0};

            sprintfW(fullkey,fmt,This->szwCurrentClsid,szwLngp,This->langid);
            res = RegOpenKeyExW(new_This->tipkey, fullkey, 0, KEY_READ | KEY_WRITE, &This->langkey);
            new_This->lang_index = This->lang_index;
        }
    }
    return res;
}

static const IEnumTfLanguageProfilesVtbl IEnumTfLanguageProfiles_Vtbl ={
    EnumTfLanguageProfiles_QueryInterface,
    EnumTfLanguageProfiles_AddRef,
    EnumTfLanguageProfiles_Release,

    EnumTfLanguageProfiles_Clone,
    EnumTfLanguageProfiles_Next,
    EnumTfLanguageProfiles_Reset,
    EnumTfLanguageProfiles_Skip
};

static HRESULT EnumTfLanguageProfiles_Constructor(LANGID langid, IEnumTfLanguageProfiles **ppOut)
{
    HRESULT hr;
    EnumTfLanguageProfiles *This;

    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(EnumTfLanguageProfiles));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->Vtbl= &IEnumTfLanguageProfiles_Vtbl;
    This->refCount = 1;
    This->langid = langid;

    hr = CategoryMgr_Constructor(NULL,(IUnknown**)&This->catmgr);
    if (FAILED(hr))
    {
        HeapFree(GetProcessHeap(),0,This);
        return hr;
    }

    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, szwSystemTIPKey, 0, NULL, 0,
                    KEY_READ | KEY_WRITE, NULL, &This->tipkey, NULL) != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, This);
        return E_FAIL;
    }

    TRACE("returning %p\n", This);
    *ppOut = (IEnumTfLanguageProfiles*)This;
    return S_OK;
}
