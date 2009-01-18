/*
 * GUID definitions
 *
 * Copyright 2000 Alexandre Julliard
 * Copyright 2000 Francois Gouget
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

#include <stdarg.h>

#define COM_NO_WINDOWS_H
#include "windef.h"
#include "initguid.h"

/* GUIDs defined in uuids.lib */

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "objbase.h"
#include "servprov.h"

#include "oleauto.h"
#include "oleidl.h"
#include "objidl.h"
#include "olectl.h"

#include "ocidl.h"
#include "ctxtcall.h"

#include "docobj.h"
#include "exdisp.h"

#include "shlguid.h"
#include "shlobj.h"
#include "shldisp.h"
#include "comcat.h"
#include "urlmon.h"
#define _NO_AUTHOR_GUIDS
#include "activaut.h"
#include "activdbg.h"
#define _NO_SCRIPT_GUIDS
#include "activscp.h"
#include "dispex.h"
#include "mlang.h"
#include "mshtml.h"
#include "mshtmhst.h"
#include "richole.h"
#include "xmldom.h"
#include "xmldso.h"
#include "downloadmgr.h"
#include "objsel.h"
#include "hlink.h"
#include "optary.h"
#include "indexsrv.h"
#include "htiframe.h"
#include "urlhist.h"
#include "hlguids.h"
#include "dimm.h"
#include "isguids.h"
#include "objsafe.h"
#include "mstask.h"

/* FIXME: cguids declares GUIDs but does not define their values */

/* other GUIDs */

#if 0 /* FIXME */
#include "uuids.h"
#endif

/* the GUID for these interfaces are already defined by dxguid.c */
#define __IReferenceClock_INTERFACE_DEFINED__
#define __IKsPropertySet_INTERFACE_DEFINED__
#if 0 /* FIXME */
#include "strmif.h"
#endif
#if 0 /* FIXME */
#include "control.h"
#endif
	
/* GUIDs not declared in an exported header file */
DEFINE_GUID(IID_IDirectPlaySP,            0x0c9f6360,0xcc61,0x11cf,0xac,0xec,0x00,0xaa,0x00,0x68,0x86,0xe3);
DEFINE_GUID(IID_ISFHelper,                0x1fe68efb,0x1874,0x9812,0x56,0xdc,0x00,0x00,0x00,0x00,0x00,0x00);
DEFINE_GUID(IID_IDPLobbySP,               0x5a4e5a20,0x2ced,0x11d0,0xa8,0x89,0x00,0xa0,0xc9,0x05,0x43,0x3c);
DEFINE_GUID(IID_IProxyManager,            0x00000008,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(IID_IBindStatusCallbackHolder,0x79eac9cc,0xbaf9,0x11ce,0x8c,0x82,0x00,0xaa,0x00,0x4b,0xa9,0x0b);
DEFINE_GUID(IID_IEnumNetConnection,       0xC08956A0,0x1CD3,0x11D1,0xB1,0xC5,0x00,0x80,0x5F,0xC1,0x27,0x0E);
DEFINE_GUID(IID_INetConnection,           0xC08956A1,0x1CD3,0x11D1,0xB1,0xC5,0x00,0x80,0x5F,0xC1,0x27,0x0E);
DEFINE_GUID(IID_INetConnectionManager,    0xC08956A2,0x1CD3,0x11D1,0xB1,0xC5,0x00,0x80,0x5F,0xC1,0x27,0x0E);
DEFINE_GUID(IID_INetConnectionConnectUi,  0xC08956A3,0x1CD3,0x11D1,0xB1,0xC5,0x00,0x80,0x5F,0xC1,0x27,0x0E);
DEFINE_GUID(IID_INetConnectionPropertyUi, 0xC08956A4,0x1CD3,0x11D1,0xB1,0xC5,0x00,0x80,0x5F,0xC1,0x27,0x0E);
DEFINE_GUID(IID_INetLanConnectionUiInfo,  0xC08956A6,0x1CD3,0x11D1,0xB1,0xC5,0x00,0x80,0x5F,0xC1,0x27,0x0E);
DEFINE_GUID(IID_IEnumNetCfgComponent,     0xC0E8AE92,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);
DEFINE_GUID(IID_INetCfg,                  0xC0E8AE93,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);
DEFINE_GUID(IID_INetCfgComponent,         0xC0E8AE99,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);
DEFINE_GUID(IID_INetCfgComponentBindings, 0xC0E8AE9E,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);
DEFINE_GUID(IID_INetCfgLock,              0xC0E8AE9F,0x306E,0x11D1,0xAA,0xCF,0x00,0x80,0x5F,0xC1,0x27,0x0E);
DEFINE_GUID(IID_INetConnectionPropertyUi2, 0xC08956B9,0x1CD3,0x11D1,0xB1,0xC5,0x00,0x80,0x5F,0xC1,0x27,0x0E);
DEFINE_GUID(IID_INetCfgPnpReconfigCallback,0x8D84BD35,0xE227,0x11D2,0xB7,0x00,0x00,0xA0,0xC9,0x8A,0x6A,0x85);
DEFINE_GUID(IID_INetCfgComponentPropertyUi,0x932238E0,0xBEA1,0x11D0,0x92,0x98,0x00,0xC0,0x4f,0xC9,0x9D,0xCF);
DEFINE_GUID(IID_INetCfgComponentControl,   0x932238DF,0xBEA1,0x11D0,0x92,0x98,0x00,0xC0,0x4f,0xC9,0x9D,0xCF);

DEFINE_GUID(FMTID_SummaryInformation,0xF29F85E0,0x4FF9,0x1068,0xAB,0x91,0x08,0x00,0x2B,0x27,0xB3,0xD9);
DEFINE_GUID(FMTID_DocSummaryInformation,0xD5CDD502,0x2E9C,0x101B,0x93,0x97,0x08,0x00,0x2B,0x2C,0xF9,0xAE);
DEFINE_GUID(FMTID_UserDefinedProperties,0xD5CDD505,0x2E9C,0x101B,0x93,0x97,0x08,0x00,0x2B,0x2C,0xF9,0xAE);

/* COM CLSIDs not declared in an exported header file */
DEFINE_GUID(CLSID_StdMarshal,             0x00000017,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_IdentityUnmarshal,      0x0000001b,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_PSGenObject,            0x0000030c,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_PSClientSite,           0x0000030d,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_PSClassObject,          0x0000030e,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_PSInPlaceActive,        0x0000030f,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_PSInPlaceFrame,         0x00000310,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_PSDragDrop,             0x00000311,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_PSBindCtx,              0x00000312,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_PSEnumerators,          0x00000313,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_Picture_Metafile,       0x00000315,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_StaticMetafile,         0x00000315,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_Picture_Dib,            0x00000316,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_StaticDib,              0x00000316,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_Picture_EnhMetafile,    0x00000316,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_DCOMAccessControl,      0x0000031d,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_StdGlobalInterfaceTable,0x00000323,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_ComBinding,             0x00000328,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_StdEvent,               0x0000032b,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_ManualResetEvent,       0x0000032c,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_SynchronizeContainer,   0x0000032d,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_InProcFreeMarshaler,    0x0000033a,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_ConnectionManager,      0xBA126AD1,0x2166,0x11D1,0xB1,0xD0,0x0,0x80,0x5F,0xC1,0x27,0x0E);
DEFINE_GUID(CLSID_CNetCfg,                0x5B035261,0x40F9,0x11D1,0xAA,0xEC,0x00,0x80,0x5F,0xC1,0x27,0x0E);

