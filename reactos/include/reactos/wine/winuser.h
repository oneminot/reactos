#ifndef __WINE_WINUSER_H
#define __WINE_WINUSER_H

/*
 * Compatibility header
 */

#if !defined(_MSC_VER)
#include_next "winuser.h"
#endif

#define DCX_USESTYLE     0x00010000
#define LB_CARETOFF      0x01a4

WINUSERAPI UINT WINAPI PrivateExtractIconsA(LPCSTR,int,int,int,HICON*,UINT*,UINT,UINT);
WINUSERAPI UINT WINAPI PrivateExtractIconsW(LPCWSTR,int,int,int,HICON*,UINT*,UINT,UINT);

#endif /* __WINE_WINUSER_H */
