/*** Autogenerated by WIDL 0.3.0 from include\psdk\mimeinfo.idl - Do not edit ***/
#include <rpc.h>
#include <rpcndr.h>

#ifndef __WIDL_INCLUDE_PSDK_MIMEINFO_H
#define __WIDL_INCLUDE_PSDK_MIMEINFO_H
#ifdef __cplusplus
extern "C" {
#endif
#include <objidl.h>
#ifndef __IMimeInfo_FWD_DEFINED__
#define __IMimeInfo_FWD_DEFINED__
typedef interface IMimeInfo IMimeInfo;
#endif

typedef IMimeInfo *LPMIMEINFO;
/*****************************************************************************
 * IMimeInfo interface
 */
#ifndef __IMimeInfo_INTERFACE_DEFINED__
#define __IMimeInfo_INTERFACE_DEFINED__

DEFINE_GUID(IID_IMimeInfo, 0xf77459a0, 0xbf9a, 0x11cf, 0xba,0x4e, 0x00,0xc0,0x4f,0xd7,0x08,0x16);
#if defined(__cplusplus) && !defined(CINTERFACE)
interface IMimeInfo : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetMimeCLSIDMapping(
        UINT* pcTypes,
        LPCSTR** ppszTypes,
        CLSID** ppclsID) = 0;

};
#else
typedef struct IMimeInfoVtbl {
    BEGIN_INTERFACE

    /*** IUnknown methods ***/
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IMimeInfo* This,
        REFIID riid,
        void** ppvObject);

    ULONG (STDMETHODCALLTYPE *AddRef)(
        IMimeInfo* This);

    ULONG (STDMETHODCALLTYPE *Release)(
        IMimeInfo* This);

    /*** IMimeInfo methods ***/
    HRESULT (STDMETHODCALLTYPE *GetMimeCLSIDMapping)(
        IMimeInfo* This,
        UINT* pcTypes,
        LPCSTR** ppszTypes,
        CLSID** ppclsID);

    END_INTERFACE
} IMimeInfoVtbl;
interface IMimeInfo {
    const IMimeInfoVtbl* lpVtbl;
};

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IMimeInfo_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IMimeInfo_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IMimeInfo_Release(p) (p)->lpVtbl->Release(p)
/*** IMimeInfo methods ***/
#define IMimeInfo_GetMimeCLSIDMapping(p,a,b,c) (p)->lpVtbl->GetMimeCLSIDMapping(p,a,b,c)
#endif

#endif

HRESULT CALLBACK IMimeInfo_GetMimeCLSIDMapping_Proxy(
    IMimeInfo* This,
    UINT* pcTypes,
    LPCSTR** ppszTypes,
    CLSID** ppclsID);
void __RPC_STUB IMimeInfo_GetMimeCLSIDMapping_Stub(
    IRpcStubBuffer* This,
    IRpcChannelBuffer* pRpcChannelBuffer,
    PRPC_MESSAGE pRpcMessage,
    DWORD* pdwStubPhase);

#endif  /* __IMimeInfo_INTERFACE_DEFINED__ */

#define SID_IMimeInfo IID_IMimeInfo
/* Begin additional prototypes for all interfaces */


/* End additional prototypes */

#ifdef __cplusplus
}
#endif
#endif /* __WIDL_INCLUDE_PSDK_MIMEINFO_H */
