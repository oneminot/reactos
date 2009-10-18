

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 7.00.0493 */
/* at Sun Aug 13 16:46:06 2006
 */
/* Compiler settings for .\mstsclib.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, LIBID_MSTSCLib,0x8C11EFA1,0x92C3,0x11D1,0xBC,0x1E,0x00,0xC0,0x4F,0xA3,0x14,0x89);


MIDL_DEFINE_GUID(IID, DIID_IMsTscAxEvents,0x336D5562,0xEFA8,0x482E,0x8C,0xB3,0xC5,0xC0,0xFC,0x7A,0x7D,0xB6);


MIDL_DEFINE_GUID(IID, IID_IMsTscAx,0x8C11EFAE,0x92C3,0x11D1,0xBC,0x1E,0x00,0xC0,0x4F,0xA3,0x14,0x89);


MIDL_DEFINE_GUID(IID, IID_IMsRdpClient,0x92B4A539,0x7115,0x4B7C,0xA5,0xA9,0xE5,0xD9,0xEF,0xC2,0x78,0x0A);


MIDL_DEFINE_GUID(IID, IID_IMsRdpClient2,0xE7E17DC4,0x3B71,0x4BA7,0xA8,0xE6,0x28,0x1F,0xFA,0xDC,0xA2,0x8F);


MIDL_DEFINE_GUID(IID, IID_IMsRdpClient3,0x91B7CBC5,0xA72E,0x4FA0,0x93,0x00,0xD6,0x47,0xD7,0xE8,0x97,0xFF);


MIDL_DEFINE_GUID(IID, IID_IMsRdpClient4,0x095E0738,0xD97D,0x488B,0xB9,0xF6,0xDD,0x0E,0x8D,0x66,0xC0,0xDE);


MIDL_DEFINE_GUID(IID, IID_IMsTscNonScriptable,0xC1E6743A,0x41C1,0x4A74,0x83,0x2A,0x0D,0xD0,0x6C,0x1C,0x7A,0x0E);


MIDL_DEFINE_GUID(IID, IID_IMsRdpClientNonScriptable,0x2F079C4C,0x87B2,0x4AFD,0x97,0xAB,0x20,0xCD,0xB4,0x30,0x38,0xAE);


MIDL_DEFINE_GUID(IID, IID_IMsRdpClientNonScriptable2,0x17A5E535,0x4072,0x4FA4,0xAF,0x32,0xC8,0xD0,0xD4,0x73,0x45,0xE9);


MIDL_DEFINE_GUID(IID, IID_IMsTscAdvancedSettings,0x809945CC,0x4B3B,0x4A92,0xA6,0xB0,0xDB,0xF9,0xB5,0xF2,0xEF,0x2D);


MIDL_DEFINE_GUID(IID, IID_IMsRdpClientAdvancedSettings,0x3C65B4AB,0x12B3,0x465B,0xAC,0xD4,0xB8,0xDA,0xD3,0xBF,0xF9,0xE2);


MIDL_DEFINE_GUID(IID, IID_IMsRdpClientAdvancedSettings2,0x9AC42117,0x2B76,0x4320,0xAA,0x44,0x0E,0x61,0x6A,0xB8,0x43,0x7B);


MIDL_DEFINE_GUID(IID, IID_IMsRdpClientAdvancedSettings3,0x19CD856B,0xC542,0x4C53,0xAC,0xEE,0xF1,0x27,0xE3,0xBE,0x1A,0x59);


MIDL_DEFINE_GUID(IID, IID_IMsRdpClientAdvancedSettings4,0xFBA7F64E,0x7345,0x4405,0xAE,0x50,0xFA,0x4A,0x76,0x3D,0xC0,0xDE);


MIDL_DEFINE_GUID(IID, IID_IMsTscSecuredSettings,0xC9D65442,0xA0F9,0x45B2,0x8F,0x73,0xD6,0x1D,0x2D,0xB8,0xCB,0xB6);


MIDL_DEFINE_GUID(IID, IID_IMsRdpClientSecuredSettings,0x605BEFCF,0x39C1,0x45CC,0xA8,0x11,0x06,0x8F,0xB7,0xBE,0x34,0x6D);


MIDL_DEFINE_GUID(IID, IID_IMsTscDebug,0x209D0EB9,0x6254,0x47B1,0x90,0x33,0xA9,0x8D,0xAE,0x55,0xBB,0x27);


MIDL_DEFINE_GUID(CLSID, CLSID_MsTscAx,0xA41A4187,0x5A86,0x4E26,0xB4,0x0A,0x85,0x6F,0x90,0x35,0xD9,0xCB);


MIDL_DEFINE_GUID(CLSID, CLSID_MsRdpClient,0x7CACBD7B,0x0D99,0x468F,0xAC,0x33,0x22,0xE4,0x95,0xC0,0xAF,0xE5);


MIDL_DEFINE_GUID(CLSID, CLSID_MsRdpClient2,0x3523C2FB,0x4031,0x44E4,0x9A,0x3B,0xF1,0xE9,0x49,0x86,0xEE,0x7F);


MIDL_DEFINE_GUID(CLSID, CLSID_MsRdpClient3,0xACE575FD,0x1FCF,0x4074,0x94,0x01,0xEB,0xAB,0x99,0x0F,0xA9,0xDE);


MIDL_DEFINE_GUID(CLSID, CLSID_MsRdpClient4,0x6AE29350,0x321B,0x42BE,0xBB,0xE5,0x12,0xFB,0x52,0x70,0xC0,0xDE);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



