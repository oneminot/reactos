/*** Autogenerated by WIDL 0.1 from dcom.idl - Do not edit ***/
#include <rpc.h>
#include <rpcndr.h>

#ifndef __WIDL_DCOM_H
#define __WIDL_DCOM_H
#ifdef __cplusplus
extern "C" {
#endif
#include <unknwn.h>
typedef MIDL_uhyper ID;

typedef ID MID;

typedef ID OXID;

typedef ID OID;

typedef ID SETID;

typedef GUID IPID;

typedef GUID CID;

typedef REFGUID REFIPID;

#define COM_MINOR_VERSION_1 (1)

#define COM_MINOR_VERSION_2 (2)

#define COM_MAJOR_VERSION (5)

#define COM_MINOR_VERSION (3)

typedef struct tagCOMVERSION {
    unsigned short MajorVersion;
    unsigned short MinorVersion;
} COMVERSION;

#define ORPCF_NULL (0)

#define ORPCF_LOCAL (1)

#define ORPCF_RESERVED1 (2)

#define ORPCF_RESERVED2 (4)

#define ORPCF_RESERVED3 (8)

#define ORPCF_RESERVED4 (16)

typedef struct tagORPC_EXTENT {
    GUID id;
    unsigned long size;
    byte data[1];
} ORPC_EXTENT;

typedef struct tagORPC_EXTENT_ARRAY {
    unsigned long size;
    unsigned long reserved;
    ORPC_EXTENT **extent;
} ORPC_EXTENT_ARRAY;

typedef struct tagORPCTHIS {
    COMVERSION version;
    unsigned long flags;
    unsigned long reserved1;
    CID cid;
    ORPC_EXTENT_ARRAY *extensions;
} ORPCTHIS;

typedef struct tagORPCTHAT {
    unsigned long flags;
    ORPC_EXTENT_ARRAY *extensions;
} ORPCTHAT;

#define NCADG_IP_UDP (0x8)

#define NCACN_IP_TCP (0x7)

#define NCADG_IPX (0xe)

#define NCACN_SPX (0xc)

#define NCACN_NB_NB (0x12)

#define NCACN_NB_IPX (0xd)

#define NCACN_DNET_NSP (0x4)

#define NCACN_HTTP (0x1f)

typedef struct tagSTRINGBINDING {
    unsigned short wTowerId;
    unsigned short aNetworkAddr[1];
} STRINGBINDING;

#define COM_C_AUTHZ_NONE (0xffff)

typedef struct tagSECURITYBINDING {
    unsigned short wAuthnSvc;
    unsigned short wAuthzSvc;
    unsigned short aPrincName[1];
} SECURITYBINDING;

typedef struct tagDUALSTRINGARRAY {
    unsigned short wNumEntries;
    unsigned short wSecurityOffset;
    unsigned short aStringArray[1];
} DUALSTRINGARRAY;

#define OBJREF_SIGNATURE (0x574f454d)

#define OBJREF_STANDARD (0x1)

#define OBJREF_HANDLER (0x2)

#define OBJREF_CUSTOM (0x4)

#define SORF_OXRES1 (0x1)

#define SORF_OXRES2 (0x20)

#define SORF_OXRES3 (0x40)

#define SORF_OXRES4 (0x80)

#define SORF_OXRES5 (0x100)

#define SORF_OXRES6 (0x200)

#define SORF_OXRES7 (0x400)

#define SORF_OXRES8 (0x800)

#define SORF_NULL (0x0)

#define SORF_NOPING (0x1000)

typedef struct tagSTDOBJREF {
    unsigned long flags;
    unsigned long cPublicRefs;
    OXID oxid;
    OID oid;
    IPID ipid;
} STDOBJREF;

typedef struct tagOBJREF {
    unsigned long signature;
    unsigned long flags;
    GUID iid;
    union {
        struct OR_STANDARD {
            STDOBJREF std;
            DUALSTRINGARRAY saResAddr;
        } u_standard;
        struct OR_HANDLER {
            STDOBJREF std;
            CLSID clsid;
            DUALSTRINGARRAY saResAddr;
        } u_handler;
        struct OR_CUSTOM {
            CLSID clsid;
            unsigned long cbExtension;
            unsigned long size;
            byte *pData;
        } u_custom;
    } u_objref;
} OBJREF;

typedef struct tagMInterfacePointer {
    ULONG ulCntData;
    BYTE abData[1];
} MInterfacePointer;

typedef MInterfacePointer *PMInterfacePointer;

#ifndef __IRemUnknown_FWD_DEFINED__
#define __IRemUnknown_FWD_DEFINED__
typedef struct IRemUnknown IRemUnknown;
#endif

typedef IRemUnknown *LPREMUNKNOWN;

typedef struct tagREMQIRESULT {
    HRESULT hResult;
    STDOBJREF std;
} REMQIRESULT;

typedef struct tagREMINTERFACEREF {
    IPID ipid;
    unsigned long cPublicRefs;
    unsigned long cPrivateRefs;
} REMINTERFACEREF;

/*****************************************************************************
 * IRemUnknown interface
 */
#ifndef __IRemUnknown_INTERFACE_DEFINED__
#define __IRemUnknown_INTERFACE_DEFINED__

DEFINE_GUID(IID_IRemUnknown, 0x00000131, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);
#if defined(__cplusplus) && !defined(CINTERFACE)
struct IRemUnknown : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE RemQueryInterface(
        REFIPID ripid,
        unsigned long cRefs,
        unsigned short cIids,
        IID* iids,
        REMQIRESULT** ppQIResults) = 0;

    virtual HRESULT STDMETHODCALLTYPE RemAddRef(
        unsigned short cInterfaceRefs,
        REMINTERFACEREF* InterfaceRefs,
        HRESULT* pResults) = 0;

    virtual HRESULT STDMETHODCALLTYPE RemRelease(
        unsigned short cInterfaceRefs,
        REMINTERFACEREF* InterfaceRefs) = 0;

};
#else
typedef struct IRemUnknownVtbl IRemUnknownVtbl;
struct IRemUnknown {
    const IRemUnknownVtbl* lpVtbl;
};
struct IRemUnknownVtbl {
    ICOM_MSVTABLE_COMPAT_FIELDS

    /*** IUnknown methods ***/
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IRemUnknown* This,
        REFIID riid,
        void** ppvObject);

    ULONG (STDMETHODCALLTYPE *AddRef)(
        IRemUnknown* This);

    ULONG (STDMETHODCALLTYPE *Release)(
        IRemUnknown* This);

    /*** IRemUnknown methods ***/
    HRESULT (STDMETHODCALLTYPE *RemQueryInterface)(
        IRemUnknown* This,
        REFIPID ripid,
        unsigned long cRefs,
        unsigned short cIids,
        IID* iids,
        REMQIRESULT** ppQIResults);

    HRESULT (STDMETHODCALLTYPE *RemAddRef)(
        IRemUnknown* This,
        unsigned short cInterfaceRefs,
        REMINTERFACEREF* InterfaceRefs,
        HRESULT* pResults);

    HRESULT (STDMETHODCALLTYPE *RemRelease)(
        IRemUnknown* This,
        unsigned short cInterfaceRefs,
        REMINTERFACEREF* InterfaceRefs);

};

/*** IUnknown methods ***/
#define IRemUnknown_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IRemUnknown_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IRemUnknown_Release(p) (p)->lpVtbl->Release(p)
/*** IRemUnknown methods ***/
#define IRemUnknown_RemQueryInterface(p,a,b,c,d,e) (p)->lpVtbl->RemQueryInterface(p,a,b,c,d,e)
#define IRemUnknown_RemAddRef(p,a,b,c) (p)->lpVtbl->RemAddRef(p,a,b,c)
#define IRemUnknown_RemRelease(p,a,b) (p)->lpVtbl->RemRelease(p,a,b)

#endif

#define IRemUnknown_METHODS \
    ICOM_MSVTABLE_COMPAT_FIELDS \
    /*** IUnknown methods ***/ \
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE; \
    STDMETHOD_(ULONG,AddRef)(THIS) PURE; \
    STDMETHOD_(ULONG,Release)(THIS) PURE; \
    /*** IRemUnknown methods ***/ \
    STDMETHOD_(HRESULT,RemQueryInterface)(THIS_ REFIPID ripid, unsigned long cRefs, unsigned short cIids, IID* iids, REMQIRESULT** ppQIResults) PURE; \
    STDMETHOD_(HRESULT,RemAddRef)(THIS_ unsigned short cInterfaceRefs, REMINTERFACEREF* InterfaceRefs, HRESULT* pResults) PURE; \
    STDMETHOD_(HRESULT,RemRelease)(THIS_ unsigned short cInterfaceRefs, REMINTERFACEREF* InterfaceRefs) PURE;

HRESULT CALLBACK IRemUnknown_RemQueryInterface_Proxy(
    IRemUnknown* This,
    REFIPID ripid,
    unsigned long cRefs,
    unsigned short cIids,
    IID* iids,
    REMQIRESULT** ppQIResults);
void __RPC_STUB IRemUnknown_RemQueryInterface_Stub(
    struct IRpcStubBuffer* This,
    struct IRpcChannelBuffer* pRpcChannelBuffer,
    PRPC_MESSAGE pRpcMessage,
    DWORD* pdwStubPhase);
HRESULT CALLBACK IRemUnknown_RemAddRef_Proxy(
    IRemUnknown* This,
    unsigned short cInterfaceRefs,
    REMINTERFACEREF* InterfaceRefs,
    HRESULT* pResults);
void __RPC_STUB IRemUnknown_RemAddRef_Stub(
    struct IRpcStubBuffer* This,
    struct IRpcChannelBuffer* pRpcChannelBuffer,
    PRPC_MESSAGE pRpcMessage,
    DWORD* pdwStubPhase);
HRESULT CALLBACK IRemUnknown_RemRelease_Proxy(
    IRemUnknown* This,
    unsigned short cInterfaceRefs,
    REMINTERFACEREF* InterfaceRefs);
void __RPC_STUB IRemUnknown_RemRelease_Stub(
    struct IRpcStubBuffer* This,
    struct IRpcChannelBuffer* pRpcChannelBuffer,
    PRPC_MESSAGE pRpcMessage,
    DWORD* pdwStubPhase);

#endif  /* __IRemUnknown_INTERFACE_DEFINED__ */

#ifndef __IRemUnknown2_FWD_DEFINED__
#define __IRemUnknown2_FWD_DEFINED__
typedef struct IRemUnknown2 IRemUnknown2;
#endif

typedef IRemUnknown2 *LPREMUNKNOWN2;

/*****************************************************************************
 * IRemUnknown2 interface
 */
#ifndef __IRemUnknown2_INTERFACE_DEFINED__
#define __IRemUnknown2_INTERFACE_DEFINED__

DEFINE_GUID(IID_IRemUnknown2, 0x00000142, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);
#if defined(__cplusplus) && !defined(CINTERFACE)
struct IRemUnknown2 : public IRemUnknown
{
    virtual HRESULT STDMETHODCALLTYPE RemQueryInterface2(
        REFIPID ripid,
        unsigned short cIids,
        IID* iids,
        HRESULT* phr,
        MInterfacePointer** ppMIF) = 0;

};
#else
typedef struct IRemUnknown2Vtbl IRemUnknown2Vtbl;
struct IRemUnknown2 {
    const IRemUnknown2Vtbl* lpVtbl;
};
struct IRemUnknown2Vtbl {
    ICOM_MSVTABLE_COMPAT_FIELDS

    /*** IUnknown methods ***/
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IRemUnknown2* This,
        REFIID riid,
        void** ppvObject);

    ULONG (STDMETHODCALLTYPE *AddRef)(
        IRemUnknown2* This);

    ULONG (STDMETHODCALLTYPE *Release)(
        IRemUnknown2* This);

    /*** IRemUnknown methods ***/
    HRESULT (STDMETHODCALLTYPE *RemQueryInterface)(
        IRemUnknown2* This,
        REFIPID ripid,
        unsigned long cRefs,
        unsigned short cIids,
        IID* iids,
        REMQIRESULT** ppQIResults);

    HRESULT (STDMETHODCALLTYPE *RemAddRef)(
        IRemUnknown2* This,
        unsigned short cInterfaceRefs,
        REMINTERFACEREF* InterfaceRefs,
        HRESULT* pResults);

    HRESULT (STDMETHODCALLTYPE *RemRelease)(
        IRemUnknown2* This,
        unsigned short cInterfaceRefs,
        REMINTERFACEREF* InterfaceRefs);

    /*** IRemUnknown2 methods ***/
    HRESULT (STDMETHODCALLTYPE *RemQueryInterface2)(
        IRemUnknown2* This,
        REFIPID ripid,
        unsigned short cIids,
        IID* iids,
        HRESULT* phr,
        MInterfacePointer** ppMIF);

};

/*** IUnknown methods ***/
#define IRemUnknown2_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IRemUnknown2_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IRemUnknown2_Release(p) (p)->lpVtbl->Release(p)
/*** IRemUnknown methods ***/
#define IRemUnknown2_RemQueryInterface(p,a,b,c,d,e) (p)->lpVtbl->RemQueryInterface(p,a,b,c,d,e)
#define IRemUnknown2_RemAddRef(p,a,b,c) (p)->lpVtbl->RemAddRef(p,a,b,c)
#define IRemUnknown2_RemRelease(p,a,b) (p)->lpVtbl->RemRelease(p,a,b)
/*** IRemUnknown2 methods ***/
#define IRemUnknown2_RemQueryInterface2(p,a,b,c,d,e) (p)->lpVtbl->RemQueryInterface2(p,a,b,c,d,e)

#endif

#define IRemUnknown2_METHODS \
    ICOM_MSVTABLE_COMPAT_FIELDS \
    /*** IUnknown methods ***/ \
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE; \
    STDMETHOD_(ULONG,AddRef)(THIS) PURE; \
    STDMETHOD_(ULONG,Release)(THIS) PURE; \
    /*** IRemUnknown methods ***/ \
    STDMETHOD_(HRESULT,RemQueryInterface)(THIS_ REFIPID ripid, unsigned long cRefs, unsigned short cIids, IID* iids, REMQIRESULT** ppQIResults) PURE; \
    STDMETHOD_(HRESULT,RemAddRef)(THIS_ unsigned short cInterfaceRefs, REMINTERFACEREF* InterfaceRefs, HRESULT* pResults) PURE; \
    STDMETHOD_(HRESULT,RemRelease)(THIS_ unsigned short cInterfaceRefs, REMINTERFACEREF* InterfaceRefs) PURE; \
    /*** IRemUnknown2 methods ***/ \
    STDMETHOD_(HRESULT,RemQueryInterface2)(THIS_ REFIPID ripid, unsigned short cIids, IID* iids, HRESULT* phr, MInterfacePointer** ppMIF) PURE;

HRESULT CALLBACK IRemUnknown2_RemQueryInterface2_Proxy(
    IRemUnknown2* This,
    REFIPID ripid,
    unsigned short cIids,
    IID* iids,
    HRESULT* phr,
    MInterfacePointer** ppMIF);
void __RPC_STUB IRemUnknown2_RemQueryInterface2_Stub(
    struct IRpcStubBuffer* This,
    struct IRpcChannelBuffer* pRpcChannelBuffer,
    PRPC_MESSAGE pRpcMessage,
    DWORD* pdwStubPhase);

#endif  /* __IRemUnknown2_INTERFACE_DEFINED__ */

#if 0
/*****************************************************************************
 * IOXIDResolver interface (v0.0)
 */
DEFINE_GUID(IID_IOXIDResolver, 0x99fcfec4, 0x5260, 0x101b, 0xbb,0xcb, 0x00,0xaa,0x00,0x21,0x34,0x7a);
extern RPC_IF_HANDLE IOXIDResolver_v0_0_c_ifspec;
extern RPC_IF_HANDLE IOXIDResolver_v0_0_s_ifspec;
error_status_t ResolveOxid(
    handle_t hRpc,
    OXID* pOxid,
    unsigned short cRequestedProtseqs,
    unsigned short arRequestedProtseqs[],
    DUALSTRINGARRAY** ppdsaOxidBindings,
    IPID* pipidRemUnknown,
    DWORD* pAuthnHint);
error_status_t SimplePing(
    handle_t hRpc,
    SETID* pSetId);
error_status_t ComplexPing(
    handle_t hRpc,
    SETID* pSetId,
    unsigned short SequenceNum,
    unsigned short cAddToSet,
    unsigned short cDelFromSet,
    OID AddToSet[],
    OID DelFromSet[],
    unsigned short* pPingBackoffFactor);
error_status_t ServerAlive(
    handle_t hRpc);
error_status_t ResolveOxid2(
    handle_t hRpc,
    OXID* pOxid,
    unsigned short cRequestedProtseqs,
    unsigned short arRequestedProtseqs[],
    DUALSTRINGARRAY** ppdsaOxidBindings,
    IPID* pipidRemUnknown,
    DWORD* pAuthnHint,
    COMVERSION* pComVersion);

#define MODE_GET_CLASS_OBJECT (0xffffffff)

/*****************************************************************************
 * IRemoteActivation interface (v0.0)
 */
DEFINE_GUID(IID_IRemoteActivation, 0x4d9f4ab8, 0x7d1c, 0x11cf, 0x86,0x1e, 0x00,0x20,0xaf,0x6e,0x7c,0x57);
extern RPC_IF_HANDLE IRemoteActivation_v0_0_c_ifspec;
extern RPC_IF_HANDLE IRemoteActivation_v0_0_s_ifspec;
HRESULT RemoteActivation(
    handle_t hRpc,
    ORPCTHIS* ORPCthis,
    ORPCTHAT* ORPCthat,
    GUID* Clsid,
    WCHAR* pwszObjectName,
    MInterfacePointer* pObjectStorage,
    DWORD ClientImpLevel,
    DWORD Mode,
    DWORD Interfaces,
    IID* pIIDs,
    unsigned short cRequestedProtseqs,
    unsigned short RequestedProtseqs[],
    OXID* pOxid,
    DUALSTRINGARRAY** ppdsaOxidBindings,
    IPID* pipidRemUnknown,
    DWORD* pAuthnHint,
    COMVERSION* pServerVersion,
    HRESULT* phr,
    MInterfacePointer** ppInterfaceData,
    HRESULT* pResults);

#endif
#ifdef __cplusplus
}
#endif
#endif /* __WIDL_DCOM_H */
