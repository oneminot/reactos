/*
 * PROJECT:     ReactOS USB miniport driver (Cromwell type)
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/miniport/common/main.c
 * PURPOSE:     Driver entry
 * PROGRAMMERS: Copyright Aleksey Bragin (aleksey@reactos.org)
 *              Copyright 2005-2006 Herv� Poussineau (hpoussin@reactos.org)
 *              Copyright James Tabor (jimtabor@adsl-64-217-116-74.dsl.hstntx.swbell.net)
 *
 * Some parts of code are inspired (or even just copied) from
 * ReactOS Videoport driver (drivers/video/videoprt)
 */

#define NDEBUG
#include <debug.h>

#define INITGUID
#include "usbcommon.h"

/* Data for embedded drivers */
CONNECT_DATA KbdClassInformation;
CONNECT_DATA MouseClassInformation;
PDEVICE_OBJECT KeyboardFdo = NULL;
PDEVICE_OBJECT MouseFdo = NULL;

static NTSTATUS
CreateRootHubPdo(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Fdo,
	OUT PDEVICE_OBJECT* pPdo)
{
	PDEVICE_OBJECT Pdo;
	PUSBMP_DEVICE_EXTENSION DeviceExtension;
	NTSTATUS Status;

	DPRINT("CreateRootHubPdo()\n");

	Status = IoCreateDevice(
		DriverObject,
		sizeof(USBMP_DEVICE_EXTENSION),
		NULL, /* DeviceName */
		FILE_DEVICE_BUS_EXTENDER,
		FILE_DEVICE_SECURE_OPEN | FILE_AUTOGENERATED_DEVICE_NAME,
		FALSE,
		&Pdo);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("IoCreateDevice() call failed with status 0x%08x\n", Status);
		return Status;
	}

	Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;
	Pdo->Flags |= DO_POWER_PAGABLE;

	/* Zerofill device extension */
	DeviceExtension = (PUSBMP_DEVICE_EXTENSION)Pdo->DeviceExtension;
	RtlZeroMemory(DeviceExtension, sizeof(USBMP_DEVICE_EXTENSION));

	DeviceExtension->IsFDO = FALSE;
	DeviceExtension->FunctionalDeviceObject = Fdo;

	Pdo->Flags &= ~DO_DEVICE_INITIALIZING;

	*pPdo = Pdo;
	return STATUS_SUCCESS;
}

static NTSTATUS
AddRegistryEntry(
	IN PCWSTR PortTypeName,
	IN PUNICODE_STRING DeviceName,
	IN PCWSTR RegistryPath)
{
	UNICODE_STRING PathU = RTL_CONSTANT_STRING(L"\\REGISTRY\\MACHINE\\HARDWARE\\DEVICEMAP");
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE hDeviceMapKey = (HANDLE)-1;
	HANDLE hPortKey = (HANDLE)-1;
	UNICODE_STRING PortTypeNameU;
	NTSTATUS Status;

	InitializeObjectAttributes(&ObjectAttributes, &PathU, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
	Status = ZwOpenKey(&hDeviceMapKey, 0, &ObjectAttributes);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("ZwOpenKey() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}

	RtlInitUnicodeString(&PortTypeNameU, PortTypeName);
	InitializeObjectAttributes(&ObjectAttributes, &PortTypeNameU, OBJ_KERNEL_HANDLE, hDeviceMapKey, NULL);
	Status = ZwCreateKey(&hPortKey, KEY_SET_VALUE, &ObjectAttributes, 0, NULL, REG_OPTION_VOLATILE, NULL);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("ZwCreateKey() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}

	Status = ZwSetValueKey(hPortKey, DeviceName, 0, REG_SZ, (PVOID)RegistryPath, wcslen(RegistryPath) * sizeof(WCHAR) + sizeof(UNICODE_NULL));
	if (!NT_SUCCESS(Status))
	{
		DPRINT("ZwSetValueKey() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}

	Status = STATUS_SUCCESS;

cleanup:
	if (hDeviceMapKey != (HANDLE)-1)
		ZwClose(hDeviceMapKey);
	if (hPortKey != (HANDLE)-1)
		ZwClose(hPortKey);
	return Status;
}

static NTSTATUS
AddDevice_Keyboard(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo)
{
	UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\KeyboardPortUSB");
	PUSBMP_DEVICE_EXTENSION DeviceExtension;
	PDEVICE_OBJECT Fdo;
	NTSTATUS Status;

	Status = AddRegistryEntry(L"KeyboardPort", &DeviceName, L"REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\usbport");
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("AddRegistryEntry() for usb keyboard driver failed with status 0x%08lx\n", Status);
		return Status;
	}

	Status = IoCreateDevice(DriverObject,
		sizeof(USBMP_DEVICE_EXTENSION),
		&DeviceName,
		FILE_DEVICE_KEYBOARD,
		FILE_DEVICE_SECURE_OPEN,
		TRUE,
		&Fdo);

	if (!NT_SUCCESS(Status))
	{
		DPRINT1("IoCreateDevice() for usb keyboard driver failed with status 0x%08lx\n", Status);
		return Status;
	}
	DeviceExtension = (PUSBMP_DEVICE_EXTENSION)Fdo->DeviceExtension;
	RtlZeroMemory(DeviceExtension, sizeof(USBMP_DEVICE_EXTENSION));
	DeviceExtension->IsFDO = FALSE;
	KeyboardFdo = Fdo;
	Fdo->Flags &= ~DO_DEVICE_INITIALIZING;
	DPRINT("Created keyboard Fdo: %p\n", Fdo);

	return STATUS_SUCCESS;
}

static NTSTATUS
AddDevice_Mouse(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo)
{
	UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\PointerPortUSB");
	PUSBMP_DEVICE_EXTENSION DeviceExtension;
	PDEVICE_OBJECT Fdo;
	NTSTATUS Status;

	Status = AddRegistryEntry(L"PointerPort", &DeviceName, L"REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\usbport");
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("AddRegistryEntry() for usb mouse driver failed with status 0x%08lx\n", Status);
		return Status;
	}

	Status = IoCreateDevice(DriverObject,
		sizeof(USBMP_DEVICE_EXTENSION),
		&DeviceName,
		FILE_DEVICE_MOUSE,
		FILE_DEVICE_SECURE_OPEN,
		TRUE,
		&Fdo);

	if (!NT_SUCCESS(Status))
	{
		DPRINT1("IoCreateDevice() for usb mouse driver failed with status 0x%08lx\n", Status);
		return Status;
	}
	DeviceExtension = (PUSBMP_DEVICE_EXTENSION)Fdo->DeviceExtension;
	RtlZeroMemory(DeviceExtension, sizeof(USBMP_DEVICE_EXTENSION));
	DeviceExtension->IsFDO = FALSE;
	MouseFdo = Fdo;
	Fdo->Flags &= ~DO_DEVICE_INITIALIZING;
	DPRINT("Created mouse Fdo: %p\n", Fdo);

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI
AddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT pdo)
{
	PDEVICE_OBJECT fdo = NULL;
	NTSTATUS Status;
	WCHAR DeviceBuffer[20];
	WCHAR LinkDeviceBuffer[20];
	UNICODE_STRING DeviceName;
	UNICODE_STRING LinkDeviceName;
	PUSBMP_DRIVER_EXTENSION DriverExtension;
	PUSBMP_DEVICE_EXTENSION DeviceExtension = NULL;
	static ULONG DeviceNumber = 0;
	BOOLEAN AlreadyRestarted = FALSE;

	/* Allocate driver extension now */
	DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);
	if (DriverExtension == NULL)
	{
		Status = IoAllocateDriverObjectExtension(
					DriverObject,
					DriverObject,
					sizeof(USBMP_DRIVER_EXTENSION),
					(PVOID *)&DriverExtension);

		if (!NT_SUCCESS(Status))
		{
			DPRINT1("Allocating DriverObjectExtension failed.\n");
			goto cleanup;
		}
	}

	/* Create a unicode device name. Allocate a new device number every time */
	while (TRUE)
	{
		swprintf(DeviceBuffer, L"\\Device\\USBFDO-%lu", DeviceNumber);
		RtlInitUnicodeString(&DeviceName, DeviceBuffer);

		Status = IoCreateDevice(DriverObject,
					sizeof(USBMP_DEVICE_EXTENSION),
					&DeviceName,
					FILE_DEVICE_BUS_EXTENDER,
					0,
					FALSE,
					&fdo);
		if (Status != STATUS_OBJECT_NAME_COLLISION)
			break;

		if (DeviceNumber == 9999)
		{
			/* Hmm. We don't have a free number. */ 
			if (AlreadyRestarted)
			{
				Status = STATUS_TOO_MANY_NAMES;
				break;
			}
			/* Start again at DeviceNumber = 0 to find a free number */
			DeviceNumber = 0;
			AlreadyRestarted = TRUE;
		}
		else
			DeviceNumber++;
	}

	if (!NT_SUCCESS(Status))
	{
		DPRINT1("IoCreateDevice call failed with status 0x%08lx\n", Status);
		goto cleanup;
	}

	/* Zerofill device extension */
	DeviceExtension = (PUSBMP_DEVICE_EXTENSION)fdo->DeviceExtension;
	RtlZeroMemory(DeviceExtension, sizeof(USBMP_DEVICE_EXTENSION));

	/* Create root hub Pdo */
	Status = CreateRootHubPdo(DriverObject, fdo, &DeviceExtension->RootHubPdo);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("CreateRootHubPdo() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}

	/* Register device interface for controller */
	Status = IoRegisterDeviceInterface(
		pdo,
		&GUID_DEVINTERFACE_USB_HOST_CONTROLLER,
		NULL,
		&DeviceExtension->HcdInterfaceName);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("IoRegisterDeviceInterface() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}

	DeviceExtension->NextDeviceObject = IoAttachDeviceToDeviceStack(fdo, pdo);

	/* Initialize device extension */
	DeviceExtension->IsFDO = TRUE;
	DeviceExtension->DeviceNumber = DeviceNumber;
	DeviceExtension->PhysicalDeviceObject = pdo;
	DeviceExtension->FunctionalDeviceObject = fdo;
	DeviceExtension->DriverExtension = DriverExtension;

	fdo->Flags &= ~DO_DEVICE_INITIALIZING;

	/* Use the same number as the FDO */
	swprintf(LinkDeviceBuffer, L"\\??\\HCD%lu", DeviceNumber);

	RtlInitUnicodeString(&LinkDeviceName, LinkDeviceBuffer);
	Status = IoCreateSymbolicLink(&LinkDeviceName, &DeviceName);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("IoCreateSymbolicLink() call failed with status 0x%08x\n", Status);
		goto cleanup;
	}

	return STATUS_SUCCESS;

cleanup:
	if (DeviceExtension && DeviceExtension->RootHubPdo)
		IoDeleteDevice(DeviceExtension->RootHubPdo);
	if (fdo)
		IoDeleteDevice(fdo);
	return Status;
}

NTSTATUS NTAPI
IrpStub(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	NTSTATUS Status = STATUS_NOT_SUPPORTED;

	if (((PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFDO)
	{
		DPRINT1("FDO stub for major function 0x%lx\n",
			IoGetCurrentIrpStackLocation(Irp)->MajorFunction);
		ASSERT(FALSE);
		return ForwardIrpAndForget(DeviceObject, Irp);
	}
	else
	{
		/* We can't forward request to the lower driver, because
		 * we are a Pdo, so we don't have lower driver...
		 */
		DPRINT1("PDO stub for major function 0x%lx\n",
			IoGetCurrentIrpStackLocation(Irp)->MajorFunction);
		ASSERT(FALSE);
	}

	Status = Irp->IoStatus.Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

static NTSTATUS NTAPI
DispatchCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	if (((PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFDO)
		return UsbMpFdoCreate(DeviceObject, Irp);
	else
		return UsbMpPdoCreate(DeviceObject, Irp);
}

static NTSTATUS NTAPI
DispatchClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	if (((PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFDO)
		return UsbMpFdoClose(DeviceObject, Irp);
	else
		return UsbMpPdoClose(DeviceObject, Irp);
}

static NTSTATUS NTAPI
DispatchCleanup(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	if (((PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFDO)
		return UsbMpFdoCleanup(DeviceObject, Irp);
	else
		return UsbMpPdoCleanup(DeviceObject, Irp);
}

static NTSTATUS NTAPI
DispatchDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	if (((PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFDO)
		return UsbMpFdoDeviceControl(DeviceObject, Irp);
	else
		return IrpStub(DeviceObject, Irp);
}

static NTSTATUS NTAPI
DispatchInternalDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	if (((PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFDO)
		return IrpStub(DeviceObject, Irp);
	else
		return UsbMpPdoInternalDeviceControl(DeviceObject, Irp);
}

static NTSTATUS NTAPI
DispatchPnp(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	if (((PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFDO)
		return UsbMpFdoPnp(DeviceObject, Irp);
	else
		return UsbMpPdoPnp(DeviceObject, Irp);
}

static NTSTATUS NTAPI 
DispatchPower(PDEVICE_OBJECT fido, PIRP Irp)
{
	DPRINT1("IRP_MJ_POWER unimplemented\n");
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_NOT_SUPPORTED;
}

/*
 * Standard DriverEntry method.
 */
NTSTATUS NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegPath)
{
	USBPORT_INTERFACE UsbPortInterface;
	ULONG i;

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->DriverExtension->AddDevice = AddDevice;

	for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
		DriverObject->MajorFunction[i] = IrpStub;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = DispatchCleanup;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceControl;
	DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = DispatchInternalDeviceControl;
	DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;
	DriverObject->MajorFunction[IRP_MJ_POWER] = DispatchPower;

	/* Register in usbcore.sys */
	UsbPortInterface.KbdConnectData = &KbdClassInformation;
	UsbPortInterface.MouseConnectData = &MouseClassInformation;

	KbdClassInformation.ClassService = NULL;
	KbdClassInformation.ClassDeviceObject = NULL;
	MouseClassInformation.ClassService = NULL;
	MouseClassInformation.ClassDeviceObject = NULL;

	RegisterPortDriver(DriverObject, &UsbPortInterface);

	AddDevice_Keyboard(DriverObject, NULL);
	AddDevice_Mouse(DriverObject, NULL);

	return STATUS_SUCCESS;
}
