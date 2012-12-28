/*
 * IDirect3DStateBlock9 implementation
 *
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2005 Oliver Stieber
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

#include "config.h"
#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

static inline struct d3d9_stateblock *impl_from_IDirect3DStateBlock9(IDirect3DStateBlock9 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d9_stateblock, IDirect3DStateBlock9_iface);
}

static HRESULT WINAPI d3d9_stateblock_QueryInterface(IDirect3DStateBlock9 *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DStateBlock9)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DStateBlock9_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d9_stateblock_AddRef(IDirect3DStateBlock9 *iface)
{
    struct d3d9_stateblock *stateblock = impl_from_IDirect3DStateBlock9(iface);
    ULONG refcount = InterlockedIncrement(&stateblock->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3d9_stateblock_Release(IDirect3DStateBlock9 *iface)
{
    struct d3d9_stateblock *stateblock = impl_from_IDirect3DStateBlock9(iface);
    ULONG refcount = InterlockedDecrement(&stateblock->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        wined3d_mutex_lock();
        wined3d_stateblock_decref(stateblock->wined3d_stateblock);
        wined3d_mutex_unlock();

        IDirect3DDevice9Ex_Release(stateblock->parent_device);
        HeapFree(GetProcessHeap(), 0, stateblock);
    }

    return refcount;
}

static HRESULT WINAPI d3d9_stateblock_GetDevice(IDirect3DStateBlock9 *iface, IDirect3DDevice9 **device)
{
    struct d3d9_stateblock *stateblock = impl_from_IDirect3DStateBlock9(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)stateblock->parent_device;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI d3d9_stateblock_Capture(IDirect3DStateBlock9 *iface)
{
    struct d3d9_stateblock *stateblock = impl_from_IDirect3DStateBlock9(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_capture(stateblock->wined3d_stateblock);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_stateblock_Apply(IDirect3DStateBlock9 *iface)
{
    struct d3d9_stateblock *stateblock = impl_from_IDirect3DStateBlock9(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_apply(stateblock->wined3d_stateblock);
    wined3d_mutex_unlock();

    return hr;
}


static const struct IDirect3DStateBlock9Vtbl d3d9_stateblock_vtbl =
{
    /* IUnknown */
    d3d9_stateblock_QueryInterface,
    d3d9_stateblock_AddRef,
    d3d9_stateblock_Release,
    /* IDirect3DStateBlock9 */
    d3d9_stateblock_GetDevice,
    d3d9_stateblock_Capture,
    d3d9_stateblock_Apply,
};

HRESULT stateblock_init(struct d3d9_stateblock *stateblock, struct d3d9_device *device,
        D3DSTATEBLOCKTYPE type, struct wined3d_stateblock *wined3d_stateblock)
{
    HRESULT hr;

    stateblock->IDirect3DStateBlock9_iface.lpVtbl = &d3d9_stateblock_vtbl;
    stateblock->refcount = 1;

    if (wined3d_stateblock)
    {
        stateblock->wined3d_stateblock = wined3d_stateblock;
    }
    else
    {
        wined3d_mutex_lock();
        hr = wined3d_stateblock_create(device->wined3d_device,
                (enum wined3d_stateblock_type)type, &stateblock->wined3d_stateblock);
        wined3d_mutex_unlock();
        if (FAILED(hr))
        {
            WARN("Failed to create wined3d stateblock, hr %#x.\n", hr);
            return hr;
        }
    }

    stateblock->parent_device = &device->IDirect3DDevice9Ex_iface;
    IDirect3DDevice9Ex_AddRef(stateblock->parent_device);

    return D3D_OK;
}
