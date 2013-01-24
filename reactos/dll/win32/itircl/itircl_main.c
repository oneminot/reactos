/*
 * itircl main
 *
 * Copyright 2008 James Hawkins
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

#include <config.h>

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(itircl);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            break;
        default:
            break;
    }

    return TRUE;
}

/***********************************************************************
 *              DllRegisterServer (ITIRCL.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    FIXME("stub\n");
    return S_FALSE;
}

/***********************************************************************
 *              DllUnregisterServer (ITIRCL.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    FIXME("stub\n");
    return S_FALSE;
}

/***********************************************************************
 *              DllCanUnloadNow (ITIRCL.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}
