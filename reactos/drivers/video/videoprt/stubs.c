/*
 * VideoPort driver
 *
 * Copyright (C) 2002-2004, 2007 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */


#include "videoprt.h"
#include <wdmguid.h>

VP_STATUS
NTAPI
VideoPortFlushRegistry(
    PVOID HwDeviceExtension)
{
    UNIMPLEMENTED;
    return 0;
}

ULONG
NTAPI
VideoPortGetAssociatedDeviceID(
    IN PVOID DeviceObject)
{
    UNIMPLEMENTED;
    return 0;
}


ULONG
NTAPI
VideoPortGetBytesUsed(
    IN PVOID HwDeviceExtension,
    IN PDMA pDma)
{
    UNIMPLEMENTED;
    return 0;
}

PVOID
NTAPI
VideoPortGetMdl(
    IN PVOID HwDeviceExtension,
    IN PDMA pDma)
{
    UNIMPLEMENTED;
    return 0;
}

BOOLEAN
NTAPI
VideoPortLockPages(
    IN PVOID HwDeviceExtension,
    IN OUT PVIDEO_REQUEST_PACKET pVrp,
    IN PEVENT pUEvent,
    IN PEVENT pDisplayEvent,
    IN DMA_FLAGS DmaFlags)
{
    UNIMPLEMENTED;
    return 0;
}

LONG
NTAPI
VideoPortReadStateEvent(
    IN PVOID HwDeviceExtension,
    IN PEVENT pEvent)
{
    UNIMPLEMENTED;
    return 0;
}

VOID
NTAPI
VideoPortSetBytesUsed(
    IN PVOID HwDeviceExtension,
    IN OUT PDMA pDma,
    IN ULONG BytesUsed)
{
    UNIMPLEMENTED;
}

BOOLEAN
NTAPI
VideoPortUnlockPages(
    IN PVOID hwDeviceExtension,
    IN PDMA pDma)
{
    UNIMPLEMENTED;
    return 0;
}
