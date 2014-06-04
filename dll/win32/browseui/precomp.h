#ifndef _BROWSEUI_PCH_
#define _BROWSEUI_PCH_

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <wincon.h>
#include <shlobj.h>
#include <tlogstg.h>
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>
#include <shdeprecated.h>
#include <tchar.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <perhist.h>
#include <exdispid.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <wine/debug.h>

#include "resource.h"

#include "aclmulti.h"
#include "addressband.h"
#include "addresseditbox.h"
#include "bandproxy.h"
#include "bandsite.h"
#include "bandsitemenu.h"
#include "brandband.h"
#include "internettoolbar.h"
#include "commonbrowser.h"
#include "globalfoldersettings.h"
#include "regtreeoptions.h"
#include <stdio.h>

static __inline ULONG
Win32DbgPrint(const char *filename, int line, const char *lpFormat, ...)
{
    char szMsg[512];
    char *szMsgStart;
    const char *fname;
    va_list vl;
    ULONG uRet;

    fname = strrchr(filename, '\\');
    if (fname == NULL)
    {
        fname = strrchr(filename, '/');
        if (fname != NULL)
            fname++;
    }
    else
        fname++;

    if (fname == NULL)
        fname = filename;

    szMsgStart = szMsg + sprintf(szMsg, "%s:%d: ", fname, line);

    va_start(vl, lpFormat);
    uRet = (ULONG) vsprintf(szMsgStart, lpFormat, vl);
    va_end(vl);

    OutputDebugStringA(szMsg);

    return uRet;
}

#define DbgPrint(fmt, ...) \
    Win32DbgPrint(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

#if 1
#define FAILED_UNEXPECTEDLY(hr) (FAILED(hr) && (DbgPrint("Unexpected failure %08x.\n", hr), TRUE))
#else
#define FAILED_UNEXPECTEDLY(hr) FAILED(hr)
#endif


template <class Base>
class CComDebugObject : public Base
{
public:
    CComDebugObject(void * = NULL)
    {
        _pAtlModule->Lock();
    }

    virtual ~CComDebugObject()
    {
        this->FinalRelease();
        _pAtlModule->Unlock();
    }

    STDMETHOD_(ULONG, AddRef)()
    {
        int rc = this->InternalAddRef();
        DbgPrint("RefCount is now %d(++)!\n", rc);
        return rc;
    }

    STDMETHOD_(ULONG, Release)()
    {
        ULONG								newRefCount;

        newRefCount = this->InternalRelease();
        DbgPrint("RefCount is now %d(--)!\n", newRefCount);
        if (newRefCount == 0)
            delete this;
        return newRefCount;
    }

    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject)
    {
        return this->_InternalQueryInterface(iid, ppvObject);
    }

    static HRESULT WINAPI CreateInstance(CComDebugObject<Base> **pp)
    {
        CComDebugObject<Base>				*newInstance;
        HRESULT								hResult;

        ATLASSERT(pp != NULL);
        if (pp == NULL)
            return E_POINTER;

        hResult = E_OUTOFMEMORY;
        newInstance = NULL;
        ATLTRY(newInstance = new CComDebugObject<Base>())
            if (newInstance != NULL)
            {
            newInstance->SetVoid(NULL);
            newInstance->InternalFinalConstructAddRef();
            hResult = newInstance->_AtlInitialConstruct();
            if (SUCCEEDED(hResult))
                hResult = newInstance->FinalConstruct();
            if (SUCCEEDED(hResult))
                hResult = newInstance->_AtlFinalConstruct();
            newInstance->InternalFinalConstructRelease();
            if (hResult != S_OK)
            {
                delete newInstance;
                newInstance = NULL;
            }
            }
        *pp = newInstance;
        return hResult;
    }
};

WINE_DEFAULT_DEBUG_CHANNEL(browseui);

#endif /* _BROWSEUI_PCH_ */
