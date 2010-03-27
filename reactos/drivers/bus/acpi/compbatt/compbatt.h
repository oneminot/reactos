/*
 * PROJECT:         ReactOS Composite Battery Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/drivers/bus/acpi/compbatt/compbatt.h
 * PURPOSE:         Main Header File
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include <ntddk.h>
#include <initguid.h>
#include <batclass.h>
#include <debug.h>

typedef struct _COMPBATT_BATTERY_ENTRY
{
    LIST_ENTRY BatteryLink;
    IO_REMOVE_LOCK RemoveLock;
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    WORK_QUEUE_ITEM WorkItem;
    BOOLEAN WaitFlag;
    BATTERY_WAIT_STATUS WaitStatus;
    union
    {
        BATTERY_WAIT_STATUS WorkerWaitStatus;
        BATTERY_STATUS WorkerStatus;
    };
    ULONG Tag;
    ULONG Flags;
    BATTERY_INFORMATION BatteryInformation;
    BATTERY_STATUS BatteryStatus;
    ULONGLONG InterruptTime;
    UNICODE_STRING BatteryName;
} COMPBATT_BATTERY_ENTRY, *PCOMPBATT_BATTERY_ENTRY;

typedef struct _COMPBATT_DEVICE_EXTENSION
{
    PVOID ClassData;
    ULONG NextTag;
    LIST_ENTRY BatteryList;
    FAST_MUTEX Lock;
    ULONG Tag;
    ULONG Flags;
    BATTERY_INFORMATION BatteryInformation;
    BATTERY_STATUS BatteryStatus;
    ULONGLONG InterruptTime;
    POWER_STATE PowerState;
    ULONG LowCapacity;
    ULONG HighCapacity;
    PDEVICE_OBJECT AttachedDevice;
    PDEVICE_OBJECT DeviceObject;
    PVOID NotificationEntry;
} COMPBATT_DEVICE_EXTENSION, *PCOMPBATT_DEVICE_EXTENSION;

NTSTATUS
NTAPI
CompBattAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PdoDeviceObject
);

NTSTATUS
NTAPI
CompBattPowerDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
NTAPI
CompBattPnpDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
NTAPI
CompBattQueryInformation(
    IN PCOMPBATT_DEVICE_EXTENSION FdoExtension,
    IN ULONG Tag,
    IN BATTERY_QUERY_INFORMATION_LEVEL InfoLevel,
    IN OPTIONAL LONG AtRate,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG ReturnedLength
);
                       
NTSTATUS
NTAPI
CompBattQueryStatus(
    IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
    IN ULONG Tag,
    IN PBATTERY_STATUS BatteryStatus
);

NTSTATUS
NTAPI
CompBattSetStatusNotify(
    IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
    IN ULONG BatteryTag,
    IN PBATTERY_NOTIFY BatteryNotify
);

NTSTATUS
NTAPI
CompBattDisableStatusNotify(
    IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
CompBattQueryTag(
    IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
    OUT PULONG Tag
);

extern ULONG CompBattDebug;

/* EOF */
