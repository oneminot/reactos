#pragma once

#define _HUBBUSIF_

#include "usbdi.h"

#if (NTDDI_VERSION >= NTDDI_WINXP)

#if !defined(_USBBUSIF_)
typedef PVOID PUSB_DEVICE_HANDLE;
#endif

typedef struct _ROOTHUB_PDO_EXTENSION {
  ULONG Signature;
} ROOTHUB_PDO_EXTENSION, *PROOTHUB_PDO_EXTENSION;

#define USBD_DEVHACK_SLOW_ENUMERATION   0x00000001
#define USBD_DEVHACK_DISABLE_SN         0x00000002
#define USBD_DEVHACK_SET_DIAG_ID        0x00000004

#ifndef USB_BUSIFFN
#define USB_BUSIFFN __stdcall
#endif

#define CD_ERR_V1               0x00000001

#define ID_ERR_V1               0x00000001

#define USBD_KEEP_DEVICE_DATA   0x00000001
#define USBD_MARK_DEVICE_BUSY   0x00000002

#define USB_IDLE_NOT_READY      0
#define USB_IDLE_READY          1

typedef
NTSTATUS
USB_BUSIFFN
USB_BUSIFFN_CREATE_USB_DEVICE (
  IN PVOID BusContext,
  OUT PUSB_DEVICE_HANDLE *NewDeviceHandle,
  IN PUSB_DEVICE_HANDLE HubDeviceHandle,
  IN USHORT PortStatus,
  IN USHORT PortNumber);
typedef USB_BUSIFFN_CREATE_USB_DEVICE *PUSB_BUSIFFN_CREATE_USB_DEVICE;

typedef enum _USBPORT_CREATEDEV_ERROR {
  CreateDevErrNotSet = 0,
  CreateDevBadHubDevHandle,
  CreateDevFailedAllocDevHandle,
  CreateDevFailedOpenEndpoint,
  CreateDevFailedAllocDsBuff,
  CreateDevFailedGetDs,
  CreateDevTtNotFound,
  CreateDevBadDevHandlePtr
} USBPORT_CREATEDEV_ERROR;

typedef struct _USB_CD_ERROR_INFORMATION {
  ULONG Version;
  USBPORT_CREATEDEV_ERROR PathError;
  ULONG UlongArg1;
  ULONG UlongArg2;
  NTSTATUS NtStatus;
  UCHAR XtraInfo[64];
} USB_CD_ERROR_INFORMATION, *PUSB_CD_ERROR_INFORMATION;

typedef
NTSTATUS
USB_BUSIFFN
USB_BUSIFFN_CREATE_USB_DEVICE_EX (
  IN PVOID BusContext,
  OUT PUSB_DEVICE_HANDLE *NewDeviceHandle,
  IN PUSB_DEVICE_HANDLE HsHubDeviceHandle,
  IN USHORT PortStatus,
  IN USHORT PortNumber,
  OUT PUSB_CD_ERROR_INFORMATION CdErrorInfo,
  IN USHORT TtPortNumber);
typedef USB_BUSIFFN_CREATE_USB_DEVICE_EX *PUSB_BUSIFFN_CREATE_USB_DEVICE_EX;

typedef struct _USB_PORT_PATH {
  ULONG PortPathDepth;
  ULONG PortPath[6];
} USB_PORT_PATH, *PUSB_PORT_PATH;

typedef
NTSTATUS
USB_BUSIFFN
USB_BUSIFFN_CREATE_USB_DEVICE_V7 (
  IN PVOID BusContext,
  OUT PUSB_DEVICE_HANDLE *NewDeviceHandle,
  IN PUSB_DEVICE_HANDLE HsHubDeviceHandle,
  IN USHORT PortStatus,
  IN PUSB_PORT_PATH PortPath,
  OUT PUSB_CD_ERROR_INFORMATION CdErrorInfo,
  IN USHORT TtPortNumber,
  IN PDEVICE_OBJECT PdoDeviceObject,
  IN PUNICODE_STRING PhysicalDeviceObjectName);
typedef USB_BUSIFFN_CREATE_USB_DEVICE_V7 *PUSB_BUSIFFN_CREATE_USB_DEVICE_V7;

typedef enum _USBPORT_INITDEV_ERROR {
  InitDevErrNotSet = 0,
  InitDevFailedSetAddress,
  InitDevFailedPokeEndpoint,
  InitDevBadDeviceDescriptor
} USBPORT_INITDEV_ERROR;

typedef struct _USB_ID_ERROR_INFORMATION {
  ULONG Version;
  USBPORT_INITDEV_ERROR PathError;
  ULONG Arg1;
  ULONG UsbAddress;
  NTSTATUS NtStatus;
  USBD_STATUS UsbdStatus;
  UCHAR XtraInfo[64];
} USB_ID_ERROR_INFORMATION, *PUSB_ID_ERROR_INFORMATION;

typedef
NTSTATUS
USB_BUSIFFN
USB_BUSIFFN_INITIALIZE_USB_DEVICE (
  IN PVOID BusContext,
  IN OUT PUSB_DEVICE_HANDLE DeviceHandle);
typedef USB_BUSIFFN_INITIALIZE_USB_DEVICE *PUSB_BUSIFFN_INITIALIZE_USB_DEVICE;

typedef
NTSTATUS
USB_BUSIFFN
USB_BUSIFFN_INITIALIZE_USB_DEVICE_EX (
  IN PVOID BusContext,
  IN OUT PUSB_DEVICE_HANDLE DeviceHandle,
  OUT PUSB_ID_ERROR_INFORMATION IdErrInfo);
typedef USB_BUSIFFN_INITIALIZE_USB_DEVICE_EX *PUSB_BUSIFFN_INITIALIZE_USB_DEVICE_EX;

typedef
NTSTATUS
USB_BUSIFFN
USB_BUSIFFN_REMOVE_USB_DEVICE (
  IN PVOID BusContext,
  IN OUT PUSB_DEVICE_HANDLE DeviceHandle,
  IN ULONG Flags);
typedef USB_BUSIFFN_REMOVE_USB_DEVICE *PUSB_BUSIFFN_REMOVE_USB_DEVICE;

typedef
NTSTATUS
USB_BUSIFFN
USB_BUSIFFN_GET_USB_DESCRIPTORS (
  IN PVOID BusContext,
  IN OUT PUSB_DEVICE_HANDLE DeviceHandle,
  OUT PUCHAR DeviceDescriptorBuffer,
  IN OUT PULONG DeviceDescriptorBufferLength,
  OUT PUCHAR ConfigDescriptorBuffer,
  IN OUT PULONG ConfigDescriptorBufferLength);
typedef USB_BUSIFFN_GET_USB_DESCRIPTORS *PUSB_BUSIFFN_GET_USB_DESCRIPTORS;

typedef
NTSTATUS
USB_BUSIFFN
USB_BUSIFFN_RESTORE_DEVICE (
  IN PVOID BusContext,
  IN OUT PUSB_DEVICE_HANDLE OldDeviceHandle,
  IN OUT PUSB_DEVICE_HANDLE NewDeviceHandle);
typedef USB_BUSIFFN_RESTORE_DEVICE *PUSB_BUSIFFN_RESTORE_DEVICE;

typedef
NTSTATUS
USB_BUSIFFN
USB_BUSIFFN_GET_POTRTHACK_FLAGS (
  IN PVOID BusContext,
  IN OUT PULONG Flags);
typedef USB_BUSIFFN_GET_POTRTHACK_FLAGS *PUSB_BUSIFFN_GET_POTRTHACK_FLAGS;

typedef
NTSTATUS
USB_BUSIFFN
USB_BUSIFFN_GET_DEVICE_INFORMATION (
  IN PVOID BusContext,
  IN PUSB_DEVICE_HANDLE DeviceHandle,
  OUT PVOID DeviceInformationBuffer,
  IN ULONG DeviceInformationBufferLength,
  IN OUT PULONG LengthOfDataCopied);
typedef USB_BUSIFFN_GET_DEVICE_INFORMATION *PUSB_BUSIFFN_GET_DEVICE_INFORMATION;

typedef
NTSTATUS
USB_BUSIFFN
USB_BUSIFFN_GET_CONTROLLER_INFORMATION (
  IN PVOID BusContext,
  IN OUT PVOID ControllerInformationBuffer,
  IN ULONG ControllerInformationBufferLength,
  IN OUT PULONG LengthOfDataCopied);
typedef USB_BUSIFFN_GET_CONTROLLER_INFORMATION *PUSB_BUSIFFN_GET_CONTROLLER_INFORMATION;

typedef
NTSTATUS
USB_BUSIFFN
USB_BUSIFFN_CONTROLLER_SELECTIVE_SUSPEND (
  IN PVOID BusContext,
  IN BOOLEAN Enable);
typedef USB_BUSIFFN_CONTROLLER_SELECTIVE_SUSPEND *PUSB_BUSIFFN_CONTROLLER_SELECTIVE_SUSPEND;

typedef
NTSTATUS
USB_BUSIFFN
USB_BUSIFFN_GET_EXTENDED_HUB_INFO (
  IN PVOID BusContext,
  IN PDEVICE_OBJECT HubPhysicalDeviceObject,
  IN PVOID HubInformationBuffer,
  IN ULONG HubInformationBufferLength,
  OUT PULONG LengthOfDataCopied);
typedef USB_BUSIFFN_GET_EXTENDED_HUB_INFO *PUSB_BUSIFFN_GET_EXTENDED_HUB_INFO;

typedef
NTSTATUS
USB_BUSIFFN
USB_BUSIFFN_GET_ROOTHUB_SYM_NAME (
  IN PVOID BusContext,
  IN PVOID HubSymNameBuffer,
  IN ULONG HubSymNameBufferLength,
  OUT PULONG HubSymNameActualLength);
typedef USB_BUSIFFN_GET_ROOTHUB_SYM_NAME *PUSB_BUSIFFN_GET_ROOTHUB_SYM_NAME;

typedef
PVOID
USB_BUSIFFN
USB_BUSIFFN_GET_DEVICE_BUSCONTEXT (
  IN PVOID HubBusContext,
  IN PVOID DeviceHandle);
typedef USB_BUSIFFN_GET_DEVICE_BUSCONTEXT *PUSB_BUSIFFN_GET_DEVICE_BUSCONTEXT;

typedef
NTSTATUS
USB_BUSIFFN
USB_BUSIFFN_INITIALIZE_20HUB (
  IN PVOID BusContext,
  IN PUSB_DEVICE_HANDLE HubDeviceHandle,
  IN ULONG TtCount);
typedef USB_BUSIFFN_INITIALIZE_20HUB *PUSB_BUSIFFN_INITIALIZE_20HUB;

typedef
BOOLEAN
USB_BUSIFFN
USB_BUSIFFN_IS_ROOT (
  IN PVOID BusContext,
  IN PVOID DeviceObject);
typedef USB_BUSIFFN_IS_ROOT *PUSB_BUSIFFN_IS_ROOT;

typedef
VOID
USB_BUSIFFN
USB_BUSIFFN_ACQUIRE_SEMAPHORE (
  IN PVOID BusContext);
typedef USB_BUSIFFN_ACQUIRE_SEMAPHORE *PUSB_BUSIFFN_ACQUIRE_SEMAPHORE;

typedef
VOID
USB_BUSIFFN
USB_BUSIFFN_RELEASE_SEMAPHORE (
  IN PVOID BusContext);
typedef USB_BUSIFFN_RELEASE_SEMAPHORE *PUSB_BUSIFFN_RELEASE_SEMAPHORE;

typedef
VOID
__stdcall
RH_INIT_CALLBACK (
  IN PVOID CallBackContext);
typedef RH_INIT_CALLBACK *PRH_INIT_CALLBACK;

typedef
NTSTATUS
USB_BUSIFFN
USB_BUSIFFN_ROOTHUB_INIT_NOTIFY (
  IN PVOID BusContext,
  IN PVOID CallbackContext,
  IN PRH_INIT_CALLBACK CallbackRoutine);
typedef USB_BUSIFFN_ROOTHUB_INIT_NOTIFY *PUSB_BUSIFFN_ROOTHUB_INIT_NOTIFY;

typedef
VOID
USB_BUSIFFN
USB_BUSIFFN_FLUSH_TRANSFERS (
  IN PVOID BusContext,
  IN PVOID DeviceHandle);
typedef USB_BUSIFFN_FLUSH_TRANSFERS *PUSB_BUSIFFN_FLUSH_TRANSFERS;

typedef
ULONG
USB_BUSIFFN
USB_BUSIFFN_CALC_PIPE_BANDWIDTH (
  IN PVOID BusContext,
  IN PUSBD_PIPE_INFORMATION PipeInfo,
  IN USB_DEVICE_SPEED DeviceSpeed);
typedef USB_BUSIFFN_CALC_PIPE_BANDWIDTH *PUSB_BUSIFFN_CALC_PIPE_BANDWIDTH;

typedef
VOID
USB_BUSIFFN
USB_BUSIFFN_SET_BUS_WAKE_MODE (
  IN PVOID BusContext,
  IN ULONG Mode);
typedef USB_BUSIFFN_SET_BUS_WAKE_MODE *PUSB_BUSIFFN_SET_BUS_WAKE_MODE;

typedef
VOID
USB_BUSIFFN
USB_BUSIFFN_SET_DEVICE_FLAG (
  IN PVOID BusContext,
  IN GUID *DeviceFlagGuid,
  IN PVOID ValueData,
  IN ULONG ValueLength);
typedef USB_BUSIFFN_SET_DEVICE_FLAG *PUSB_BUSIFFN_SET_DEVICE_FLAG;

typedef
VOID
USB_BUSIFFN
USB_BUSIFFN_SET_DEVHANDLE_DATA (
  IN PVOID BusContext,
  IN PVOID DeviceHandle,
  IN PDEVICE_OBJECT UsbDevicePdo);
typedef USB_BUSIFFN_SET_DEVHANDLE_DATA *PUSB_BUSIFFN_SET_DEVHANDLE_DATA;

typedef
NTSTATUS
USB_BUSIFFN
USB_BUSIFFN_TEST_POINT (
  IN PVOID BusContext,
  IN PVOID DeviceHandle,
  IN ULONG Opcode,
  IN PVOID TestData);
typedef USB_BUSIFFN_TEST_POINT *PUSB_BUSIFFN_TEST_POINT;

typedef
NTSTATUS
USB_BUSIFFN
USB_BUSIFFN_GET_DEVICE_PERFORMANCE_INFO (
  IN PVOID BusContext,
  IN PUSB_DEVICE_HANDLE DeviceHandle,
  OUT PVOID DeviceInformationBuffer,
  IN ULONG DeviceInformationBufferLength,
  IN OUT PULONG LengthOfDataCopied);
typedef USB_BUSIFFN_GET_DEVICE_PERFORMANCE_INFO *PUSB_BUSIFFN_GET_DEVICE_PERFORMANCE_INFO;

typedef
NTSTATUS
USB_BUSIFFN
USB_BUSIFFN_WAIT_ASYNC_POWERUP (
  IN PVOID BusContext);
typedef USB_BUSIFFN_WAIT_ASYNC_POWERUP *PUSB_BUSIFFN_WAIT_ASYNC_POWERUP;

typedef
NTSTATUS
USB_BUSIFFN
USB_BUSIFFN_GET_DEVICE_ADDRESS (
  IN PVOID BusContext,
  IN PUSB_DEVICE_HANDLE DeviceHandle,
  OUT PUSHORT DeviceAddress);
typedef USB_BUSIFFN_GET_DEVICE_ADDRESS *PUSB_BUSIFFN_GET_DEVICE_ADDRESS;

typedef
VOID
USB_BUSIFFN
USB_BUSIFFN_DEREF_DEVICE_HANDLE (
  IN PVOID BusContext,
  IN PUSB_DEVICE_HANDLE DeviceHandle,
  IN PVOID Object,
  IN ULONG Tag);
typedef USB_BUSIFFN_DEREF_DEVICE_HANDLE *PUSB_BUSIFFN_DEREF_DEVICE_HANDLE;

typedef
NTSTATUS
USB_BUSIFFN
USB_BUSIFFN_REF_DEVICE_HANDLE (
  IN PVOID BusContext,
  IN PUSB_DEVICE_HANDLE DeviceHandle,
  IN PVOID Object,
  IN ULONG Tag);
typedef USB_BUSIFFN_REF_DEVICE_HANDLE *PUSB_BUSIFFN_REF_DEVICE_HANDLE;

typedef
ULONG
USB_BUSIFFN
USB_BUSIFFN_SET_DEVICE_HANDLE_IDLE_READY_STATE (
  IN PVOID BusContext,
  IN PUSB_DEVICE_HANDLE DeviceHandle,
  IN ULONG NewIdleReadyState);
typedef USB_BUSIFFN_SET_DEVICE_HANDLE_IDLE_READY_STATE *PUSB_BUSIFFN_SET_DEVICE_HANDLE_IDLE_READY_STATE;

typedef
NTSTATUS
USB_BUSIFFN
USB_BUSIFFN_GET_CONTAINER_ID_FOR_PORT (
  IN PVOID BusContext,
  IN USHORT PortNumber,
  OUT LPGUID ContainerId);
typedef USB_BUSIFFN_GET_CONTAINER_ID_FOR_PORT *PUSB_BUSIFFN_GET_CONTAINER_ID_FOR_PORT;

typedef
VOID
USB_BUSIFFN
USB_BUSIFFN_SET_CONTAINER_ID_FOR_PORT (
  IN PVOID BusContext,
  IN USHORT PortNumber,
  IN LPGUID ContainerId);
typedef USB_BUSIFFN_SET_CONTAINER_ID_FOR_PORT *PUSB_BUSIFFN_SET_CONTAINER_ID_FOR_PORT;

typedef
NTSTATUS
USB_BUSIFFN
USB_BUSIFFN_ABORT_ALL_DEVICE_PIPES (
  IN PVOID BusContext,
  IN PUSB_DEVICE_HANDLE DeviceHandle);
typedef USB_BUSIFFN_ABORT_ALL_DEVICE_PIPES *PUSB_BUSIFFN_ABORT_ALL_DEVICE_PIPES;

#define ERRATA_FLAG_RESET_TT_ON_CANCEL              1
#define ERRATA_FLAG_NO_CLEAR_TT_BUFFER_ON_CANCEL    2

#define USB_BUSIF_HUB_VERSION_0         0x0000
#define USB_BUSIF_HUB_VERSION_1         0x0001
#define USB_BUSIF_HUB_VERSION_2         0x0002
#define USB_BUSIF_HUB_VERSION_3         0x0003
#define USB_BUSIF_HUB_VERSION_4         0x0004
#define USB_BUSIF_HUB_VERSION_5         0x0005
#define USB_BUSIF_HUB_VERSION_6         0x0006
#define USB_BUSIF_HUB_VERSION_7         0x0007

#define USB_BUSIF_HUB_MIDUMP_VERSION_0  0x0000

#define USB_BUSIF_HUB_SS_VERSION_0      0x0000

typedef
VOID
USB_BUSIFFN
USB_BUSIFFN_SET_DEVICE_ERRATA_FLAG (
  IN PVOID BusContext,
  IN PUSB_DEVICE_HANDLE DeviceHandle,
  IN ULONG DeviceErrataFlag);
typedef USB_BUSIFFN_SET_DEVICE_ERRATA_FLAG *PUSB_BUSIFFN_SET_DEVICE_ERRATA_FLAG;

DEFINE_GUID(USB_BUS_INTERFACE_HUB_GUID,
  0xb2bb8c0a, 0x5ab4, 0x11d3, 0xa8, 0xcd, 0x0, 0xc0, 0x4f, 0x68, 0x74, 0x7a);

typedef struct _USB_BUS_INTERFACE_HUB_V0 {
  USHORT Size;
  USHORT Version;
  PVOID BusContext;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
} USB_BUS_INTERFACE_HUB_V0, *PUSB_BUS_INTERFACE_HUB_V0;

typedef struct _USB_BUS_INTERFACE_HUB_V1 {
  USHORT Size;
  USHORT Version;
  PVOID BusContext;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PUSB_BUSIFFN_CREATE_USB_DEVICE CreateUsbDevice;
  PUSB_BUSIFFN_INITIALIZE_USB_DEVICE InitializeUsbDevice;
  PUSB_BUSIFFN_GET_USB_DESCRIPTORS GetUsbDescriptors;
  PUSB_BUSIFFN_REMOVE_USB_DEVICE RemoveUsbDevice;
  PUSB_BUSIFFN_RESTORE_DEVICE RestoreUsbDevice;
  PUSB_BUSIFFN_GET_POTRTHACK_FLAGS GetPortHackFlags;
  PUSB_BUSIFFN_GET_DEVICE_INFORMATION QueryDeviceInformation;
} USB_BUS_INTERFACE_HUB_V1, *PUSB_BUS_INTERFACE_HUB_V1;

typedef struct _USB_BUS_INTERFACE_HUB_V2 {
  USHORT Size;
  USHORT Version;
  PVOID BusContext;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PUSB_BUSIFFN_CREATE_USB_DEVICE CreateUsbDevice;
  PUSB_BUSIFFN_INITIALIZE_USB_DEVICE InitializeUsbDevice;
  PUSB_BUSIFFN_GET_USB_DESCRIPTORS GetUsbDescriptors;
  PUSB_BUSIFFN_REMOVE_USB_DEVICE RemoveUsbDevice;
  PUSB_BUSIFFN_RESTORE_DEVICE RestoreUsbDevice;
  PUSB_BUSIFFN_GET_POTRTHACK_FLAGS GetPortHackFlags;
  PUSB_BUSIFFN_GET_DEVICE_INFORMATION QueryDeviceInformation;
  PUSB_BUSIFFN_GET_CONTROLLER_INFORMATION GetControllerInformation;
  PUSB_BUSIFFN_CONTROLLER_SELECTIVE_SUSPEND ControllerSelectiveSuspend;
  PUSB_BUSIFFN_GET_EXTENDED_HUB_INFO GetExtendedHubInformation;
  PUSB_BUSIFFN_GET_ROOTHUB_SYM_NAME GetRootHubSymbolicName;
  PUSB_BUSIFFN_GET_DEVICE_BUSCONTEXT GetDeviceBusContext;    
  PUSB_BUSIFFN_INITIALIZE_20HUB Initialize20Hub;
} USB_BUS_INTERFACE_HUB_V2, *PUSB_BUS_INTERFACE_HUB_V2;

typedef struct _USB_BUS_INTERFACE_HUB_V3 {
  USHORT Size;
  USHORT Version;
  PVOID BusContext;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PUSB_BUSIFFN_CREATE_USB_DEVICE CreateUsbDevice;
  PUSB_BUSIFFN_INITIALIZE_USB_DEVICE InitializeUsbDevice;
  PUSB_BUSIFFN_GET_USB_DESCRIPTORS GetUsbDescriptors;
  PUSB_BUSIFFN_REMOVE_USB_DEVICE RemoveUsbDevice;
  PUSB_BUSIFFN_RESTORE_DEVICE RestoreUsbDevice;
  PUSB_BUSIFFN_GET_POTRTHACK_FLAGS GetPortHackFlags;
  PUSB_BUSIFFN_GET_DEVICE_INFORMATION QueryDeviceInformation;
  PUSB_BUSIFFN_GET_CONTROLLER_INFORMATION GetControllerInformation;
  PUSB_BUSIFFN_CONTROLLER_SELECTIVE_SUSPEND ControllerSelectiveSuspend;
  PUSB_BUSIFFN_GET_EXTENDED_HUB_INFO GetExtendedHubInformation;
  PUSB_BUSIFFN_GET_ROOTHUB_SYM_NAME GetRootHubSymbolicName;
  PUSB_BUSIFFN_GET_DEVICE_BUSCONTEXT GetDeviceBusContext;
  PUSB_BUSIFFN_INITIALIZE_20HUB Initialize20Hub;
  PUSB_BUSIFFN_ROOTHUB_INIT_NOTIFY RootHubInitNotification;
} USB_BUS_INTERFACE_HUB_V3, *PUSB_BUS_INTERFACE_HUB_V3;

typedef struct _USB_BUS_INTERFACE_HUB_V4 {
  USHORT Size;
  USHORT Version;
  PVOID BusContext;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PUSB_BUSIFFN_CREATE_USB_DEVICE CreateUsbDevice;
  PUSB_BUSIFFN_INITIALIZE_USB_DEVICE InitializeUsbDevice;
  PUSB_BUSIFFN_GET_USB_DESCRIPTORS GetUsbDescriptors;
  PUSB_BUSIFFN_REMOVE_USB_DEVICE RemoveUsbDevice;
  PUSB_BUSIFFN_RESTORE_DEVICE RestoreUsbDevice;
  PUSB_BUSIFFN_GET_POTRTHACK_FLAGS GetPortHackFlags;
  PUSB_BUSIFFN_GET_DEVICE_INFORMATION QueryDeviceInformation;
  PUSB_BUSIFFN_GET_CONTROLLER_INFORMATION GetControllerInformation;
  PUSB_BUSIFFN_CONTROLLER_SELECTIVE_SUSPEND ControllerSelectiveSuspend;
  PUSB_BUSIFFN_GET_EXTENDED_HUB_INFO GetExtendedHubInformation;
  PUSB_BUSIFFN_GET_ROOTHUB_SYM_NAME GetRootHubSymbolicName;
  PUSB_BUSIFFN_GET_DEVICE_BUSCONTEXT GetDeviceBusContext;
  PUSB_BUSIFFN_INITIALIZE_20HUB Initialize20Hub;
  PUSB_BUSIFFN_ROOTHUB_INIT_NOTIFY RootHubInitNotification;
  PUSB_BUSIFFN_FLUSH_TRANSFERS FlushTransfers;
} USB_BUS_INTERFACE_HUB_V4, *PUSB_BUS_INTERFACE_HUB_V4;

typedef struct _USB_BUS_INTERFACE_HUB_V5 {
  USHORT Size;
  USHORT Version;
  PVOID BusContext;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PUSB_BUSIFFN_CREATE_USB_DEVICE CreateUsbDevice;
  PUSB_BUSIFFN_INITIALIZE_USB_DEVICE InitializeUsbDevice;
  PUSB_BUSIFFN_GET_USB_DESCRIPTORS GetUsbDescriptors;
  PUSB_BUSIFFN_REMOVE_USB_DEVICE RemoveUsbDevice;
  PUSB_BUSIFFN_RESTORE_DEVICE RestoreUsbDevice;
  PUSB_BUSIFFN_GET_POTRTHACK_FLAGS GetPortHackFlags;
  PUSB_BUSIFFN_GET_DEVICE_INFORMATION QueryDeviceInformation;
  PUSB_BUSIFFN_GET_CONTROLLER_INFORMATION GetControllerInformation;
  PUSB_BUSIFFN_CONTROLLER_SELECTIVE_SUSPEND ControllerSelectiveSuspend;
  PUSB_BUSIFFN_GET_EXTENDED_HUB_INFO GetExtendedHubInformation;
  PUSB_BUSIFFN_GET_ROOTHUB_SYM_NAME GetRootHubSymbolicName;
  PUSB_BUSIFFN_GET_DEVICE_BUSCONTEXT GetDeviceBusContext;
  PUSB_BUSIFFN_INITIALIZE_20HUB Initialize20Hub;
  PUSB_BUSIFFN_ROOTHUB_INIT_NOTIFY RootHubInitNotification;
  PUSB_BUSIFFN_FLUSH_TRANSFERS FlushTransfers;
  PUSB_BUSIFFN_SET_DEVHANDLE_DATA SetDeviceHandleData;
} USB_BUS_INTERFACE_HUB_V5, *PUSB_BUS_INTERFACE_HUB_V5;

typedef struct _USB_BUS_INTERFACE_HUB_V6 {
  USHORT Size;
  USHORT Version;
  PVOID BusContext;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PUSB_BUSIFFN_CREATE_USB_DEVICE_EX CreateUsbDevice;
  PUSB_BUSIFFN_INITIALIZE_USB_DEVICE_EX InitializeUsbDevice;
  PUSB_BUSIFFN_GET_USB_DESCRIPTORS GetUsbDescriptors;
  PUSB_BUSIFFN_REMOVE_USB_DEVICE RemoveUsbDevice;
  PUSB_BUSIFFN_RESTORE_DEVICE RestoreUsbDevice;
  PUSB_BUSIFFN_GET_POTRTHACK_FLAGS GetPortHackFlags;
  PUSB_BUSIFFN_GET_DEVICE_INFORMATION QueryDeviceInformation;
  PUSB_BUSIFFN_GET_CONTROLLER_INFORMATION GetControllerInformation;
  PUSB_BUSIFFN_CONTROLLER_SELECTIVE_SUSPEND ControllerSelectiveSuspend;
  PUSB_BUSIFFN_GET_EXTENDED_HUB_INFO GetExtendedHubInformation;
  PUSB_BUSIFFN_GET_ROOTHUB_SYM_NAME GetRootHubSymbolicName;
  PUSB_BUSIFFN_GET_DEVICE_BUSCONTEXT GetDeviceBusContext;
  PUSB_BUSIFFN_INITIALIZE_20HUB Initialize20Hub;
  PUSB_BUSIFFN_ROOTHUB_INIT_NOTIFY RootHubInitNotification;
  PUSB_BUSIFFN_FLUSH_TRANSFERS FlushTransfers;
  PUSB_BUSIFFN_SET_DEVHANDLE_DATA SetDeviceHandleData;
  PUSB_BUSIFFN_IS_ROOT HubIsRoot;
  PUSB_BUSIFFN_ACQUIRE_SEMAPHORE AcquireBusSemaphore;
  PUSB_BUSIFFN_RELEASE_SEMAPHORE ReleaseBusSemaphore;
  PUSB_BUSIFFN_CALC_PIPE_BANDWIDTH CaculatePipeBandwidth;
  PUSB_BUSIFFN_SET_BUS_WAKE_MODE SetBusSystemWakeMode;
  PUSB_BUSIFFN_SET_DEVICE_FLAG SetDeviceFlag;
  PUSB_BUSIFFN_TEST_POINT HubTestPoint;
  PUSB_BUSIFFN_GET_DEVICE_PERFORMANCE_INFO GetDevicePerformanceInfo;
  PUSB_BUSIFFN_WAIT_ASYNC_POWERUP WaitAsyncPowerUp;
  PUSB_BUSIFFN_GET_DEVICE_ADDRESS GetDeviceAddress;
  PUSB_BUSIFFN_REF_DEVICE_HANDLE RefDeviceHandle;
  PUSB_BUSIFFN_DEREF_DEVICE_HANDLE DerefDeviceHandle;
  PUSB_BUSIFFN_SET_DEVICE_HANDLE_IDLE_READY_STATE SetDeviceHandleIdleReadyState;
} USB_BUS_INTERFACE_HUB_V6, *PUSB_BUS_INTERFACE_HUB_V6;

typedef struct _USB_BUS_INTERFACE_HUB_V7 {
  USHORT Size;
  USHORT Version;
  PVOID BusContext;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PUSB_BUSIFFN_CREATE_USB_DEVICE_EX CreateUsbDevice;
  PUSB_BUSIFFN_INITIALIZE_USB_DEVICE_EX InitializeUsbDevice;
  PUSB_BUSIFFN_GET_USB_DESCRIPTORS GetUsbDescriptors;
  PUSB_BUSIFFN_REMOVE_USB_DEVICE RemoveUsbDevice;
  PUSB_BUSIFFN_RESTORE_DEVICE RestoreUsbDevice;
  PUSB_BUSIFFN_GET_POTRTHACK_FLAGS GetPortHackFlags;
  PUSB_BUSIFFN_GET_DEVICE_INFORMATION QueryDeviceInformation;
  PUSB_BUSIFFN_GET_CONTROLLER_INFORMATION GetControllerInformation;
  PUSB_BUSIFFN_CONTROLLER_SELECTIVE_SUSPEND ControllerSelectiveSuspend;
  PUSB_BUSIFFN_GET_EXTENDED_HUB_INFO GetExtendedHubInformation;
  PUSB_BUSIFFN_GET_ROOTHUB_SYM_NAME GetRootHubSymbolicName;
  PUSB_BUSIFFN_GET_DEVICE_BUSCONTEXT GetDeviceBusContext;
  PUSB_BUSIFFN_INITIALIZE_20HUB Initialize20Hub;
  PUSB_BUSIFFN_ROOTHUB_INIT_NOTIFY RootHubInitNotification;
  PUSB_BUSIFFN_FLUSH_TRANSFERS FlushTransfers;
  PUSB_BUSIFFN_SET_DEVHANDLE_DATA SetDeviceHandleData;
  PUSB_BUSIFFN_IS_ROOT HubIsRoot;
  PUSB_BUSIFFN_ACQUIRE_SEMAPHORE AcquireBusSemaphore;
  PUSB_BUSIFFN_RELEASE_SEMAPHORE ReleaseBusSemaphore;
  PUSB_BUSIFFN_CALC_PIPE_BANDWIDTH CaculatePipeBandwidth;
  PUSB_BUSIFFN_SET_BUS_WAKE_MODE SetBusSystemWakeMode;
  PUSB_BUSIFFN_SET_DEVICE_FLAG SetDeviceFlag;
  PUSB_BUSIFFN_TEST_POINT HubTestPoint;
  PUSB_BUSIFFN_GET_DEVICE_PERFORMANCE_INFO GetDevicePerformanceInfo;
  PUSB_BUSIFFN_WAIT_ASYNC_POWERUP WaitAsyncPowerUp;
  PUSB_BUSIFFN_GET_DEVICE_ADDRESS GetDeviceAddress;
  PUSB_BUSIFFN_REF_DEVICE_HANDLE RefDeviceHandle;
  PUSB_BUSIFFN_DEREF_DEVICE_HANDLE DerefDeviceHandle;
  PUSB_BUSIFFN_SET_DEVICE_HANDLE_IDLE_READY_STATE SetDeviceHandleIdleReadyState;
  PUSB_BUSIFFN_CREATE_USB_DEVICE_V7 CreateUsbDeviceV7;
  PUSB_BUSIFFN_GET_CONTAINER_ID_FOR_PORT GetContainerIdForPort;
  PUSB_BUSIFFN_SET_CONTAINER_ID_FOR_PORT SetContainerIdForPort;
  PUSB_BUSIFFN_ABORT_ALL_DEVICE_PIPES AbortAllDevicePipes;
  PUSB_BUSIFFN_SET_DEVICE_ERRATA_FLAG SetDeviceErrataFlag;
} USB_BUS_INTERFACE_HUB_V7, *PUSB_BUS_INTERFACE_HUB_V7;

DEFINE_GUID(USB_BUS_INTERFACE_HUB_MINIDUMP_GUID,
  0xc5485f21, 0x4e81, 0x4a23, 0xa8, 0xf9, 0xd8, 0x51, 0x8a, 0xf4, 0x5c, 0x38);

typedef VOID
(USB_BUSIFFN *PUSB_BUSIFFN_SET_MINIDUMP_FLAGS) (
  IN PVOID);

typedef struct _USB_BUS_INTERFACE_HUB_MINIDUMP {
  USHORT Size;
  USHORT Version;
  PVOID BusContext;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PUSB_BUSIFFN_SET_MINIDUMP_FLAGS SetUsbPortMiniDumpFlags;
} USB_BUS_INTERFACE_HUB_MINIDUMP, *PUSB_BUS_INTERFACE_HUB_MINIDUMP;

DEFINE_GUID(USB_BUS_INTERFACE_HUB_SS_GUID, 
  0xbfc3f363, 0x8ba1, 0x4c7b, 0x97, 0xba, 0x9b, 0x12, 0xb1, 0xca, 0x13, 0x2f);

typedef NTSTATUS
(USB_BUSIFFN *PUSB_BUSIFFN_SUSPEND_HUB) (
  PDEVICE_OBJECT Pdo);

typedef NTSTATUS
(USB_BUSIFFN *PUSB_BUSIFFN_RESUME_HUB) (
  PDEVICE_OBJECT Pdo);

typedef struct _USB_BUS_INTERFACE_HUB_SELECTIVE_SUSPEND {
  USHORT Size;
  USHORT Version;
  PVOID BusContext;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PUSB_BUSIFFN_SUSPEND_HUB SuspendHub;
  PUSB_BUSIFFN_RESUME_HUB ResumeHub;
} USB_BUS_INTERFACE_HUB_SELECTIVE_SUSPEND, *PUSB_BUS_INTERFACE_HUB_SELECTIVE_SUSPEND;

#include <pshpack1.h>

typedef struct _USB_PIPE_INFORMATION_0 {
  USB_ENDPOINT_DESCRIPTOR EndpointDescriptor;
  UCHAR ED_Pad[1];
  ULONG ScheduleOffset;
} USB_PIPE_INFORMATION_0, *PUSB_PIPE_INFORMATION_0;

typedef struct _USB_LEVEL_INFORMATION {
  ULONG InformationLevel;
  ULONG ActualLength;
} USB_LEVEL_INFORMATION, *PUSB_LEVEL_INFORMATION;

typedef struct _USB_DEVICE_INFORMATION_0 {
  ULONG InformationLevel;
  ULONG ActualLength;
  ULONG PortNumber;
  USB_DEVICE_DESCRIPTOR DeviceDescriptor;
  UCHAR DD_pad[2];
  UCHAR CurrentConfigurationValue;
  UCHAR ReservedMBZ;
  USHORT DeviceAddress;
  ULONG HubAddress;
  USB_DEVICE_SPEED DeviceSpeed;
  USB_DEVICE_TYPE DeviceType;
  ULONG NumberOfOpenPipes;
  USB_PIPE_INFORMATION_0 PipeList[1];
} USB_DEVICE_INFORMATION_0, *PUSB_DEVICE_INFORMATION_0;

typedef struct _USB_CONTROLLER_INFORMATION_0 {
  ULONG InformationLevel;
  ULONG ActualLength;
  BOOLEAN SelectiveSuspendEnabled;
  BOOLEAN IsHighSpeedController;
} USB_CONTROLLER_INFORMATION_0, *PUSB_CONTROLLER_INFORMATION_0;

typedef struct _USB_CONTROLLER_INFORMATION_1 {
  ULONG InformationLevel;
  ULONG ActualLength;
  BOOLEAN SelectiveSuspendEnabled;
  BOOLEAN IsHighSpeedController;
  ULONG HcBusNumber;
  ULONG HcBusDevice;
  ULONG HcBusFunction;
} USB_CONTROLLER_INFORMATION_1, *PUSB_CONTROLLER_INFORMATION_1;

typedef struct _USB_EXTPORT_INFORMATION_0 {
  ULONG PhysicalPortNumber;
  ULONG PortLabelNumber;
  USHORT VidOverride;
  USHORT PidOverride;
  ULONG PortAttributes;
} USB_EXTPORT_INFORMATION_0, *PUSB_EXTPORT_INFORMATION;

typedef struct _USB_EXTHUB_INFORMATION_0 {
  ULONG InformationLevel;
  ULONG NumberOfPorts;
  USB_EXTPORT_INFORMATION_0 Port[255];
} USB_EXTHUB_INFORMATION_0, *PUSB_EXTHUB_INFORMATION_0;

typedef struct _USB_DEVICE_PERFORMANCE_INFO_0 {
  ULONG InformationLevel;
  ULONG ActualLength;
  ULONG BulkBytes;
  ULONG BulkUrbCount;
  ULONG ControlDataBytes;
  ULONG ControlUrbCount;
  ULONG IsoBytes;
  ULONG IsoUrbCount;
  ULONG InterruptBytes;
  ULONG InterruptUrbCount;
  ULONG AllocedInterrupt[6];
  ULONG AllocedIso;
  ULONG Total32secBandwidth;
  ULONG TotalTtBandwidth;
  ULONG TotalIsoLatency;
  ULONG DroppedIsoPackets;
  ULONG TransferErrors;
} USB_DEVICE_PERFORMANCE_INFO_0, *PUSB_DEVICE_PERFORMANCE_INFO_0;

#include <poppack.h>

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */
