#pragma once

#define PCI_ADDRESS_MEMORY_SPACE            0x00000000

//
// Helper Macros
//
#define PASTE2(x,y)                                                     x ## y
#define POINTER_TO_(x)                                                  PASTE2(P,x)
#define READ_FROM(x)                                                    PASTE2(READ_PORT_, x)
#define WRITE_TO(x)                                                     PASTE2(WRITE_PORT_, x)

//
// Declares a PCI Register Read/Write Routine
//
#define TYPE_DEFINE(x, y)                                               \
    ULONG                                                               \
    NTAPI                                                               \
    x(                                                                  \
        IN PPCIPBUSDATA BusData,                                        \
        IN y PciCfg,                                                    \
        IN PUCHAR Buffer,                                               \
        IN ULONG Offset                                                 \
    )
#define TYPE1_DEFINE(x) TYPE_DEFINE(x, PPCI_TYPE1_CFG_BITS);
#define TYPE2_DEFINE(x) TYPE_DEFINE(x, PPCI_TYPE2_ADDRESS_BITS);

//
// Defines a PCI Register Read/Write Type 1 Routine Prologue and Epilogue
//
#define TYPE1_START(x, y)                                               \
    TYPE_DEFINE(x, PPCI_TYPE1_CFG_BITS)                                 \
{                                                                       \
    ULONG i = Offset % sizeof(ULONG);                                   \
    PciCfg->u.bits.RegisterNumber = Offset / sizeof(ULONG);             \
    WRITE_PORT_ULONG(BusData->Config.Type1.Address, PciCfg->u.AsULONG);
#define TYPE1_END(y)                                                    \
    return sizeof(y); }
#define TYPE2_END       TYPE1_END

//
// PCI Register Read Type 1 Routine
//
#define TYPE1_READ(x, y)                                                \
    TYPE1_START(x, y)                                                   \
    *((POINTER_TO_(y))Buffer) =                                         \
    READ_FROM(y)((POINTER_TO_(y))(ULONG_PTR)(BusData->Config.Type1.Data + i));     \
    TYPE1_END(y)

//
// PCI Register Write Type 1 Routine
//
#define TYPE1_WRITE(x, y)                                               \
    TYPE1_START(x, y)                                                   \
    WRITE_TO(y)((POINTER_TO_(y))(ULONG_PTR)(BusData->Config.Type1.Data + i),       \
                *((POINTER_TO_(y))Buffer));                             \
    TYPE1_END(y)

//
// Defines a PCI Register Read/Write Type 2 Routine Prologue and Epilogue
//
#define TYPE2_START(x, y)                                               \
    TYPE_DEFINE(x, PPCI_TYPE2_ADDRESS_BITS)                             \
{                                                                       \
    PciCfg->u.bits.RegisterNumber = (USHORT)Offset;

//
// PCI Register Read Type 2 Routine
//
#define TYPE2_READ(x, y)                                                \
    TYPE2_START(x, y)                                                   \
    *((POINTER_TO_(y))Buffer) =                                         \
        READ_FROM(y)((POINTER_TO_(y))(ULONG_PTR)PciCfg->u.AsUSHORT);        \
    TYPE2_END(y)

//
// PCI Register Write Type 2 Routine
//
#define TYPE2_WRITE(x, y)                                               \
    TYPE2_START(x, y)                                                   \
    WRITE_TO(y)((POINTER_TO_(y))(ULONG_PTR)PciCfg->u.AsUSHORT,              \
                *((POINTER_TO_(y))Buffer));                             \
    TYPE2_END(y)

typedef NTSTATUS
(NTAPI *PciIrqRange)(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PCI_SLOT_NUMBER PciSlot,
    OUT PSUPPORTED_RANGE *Interrupt
);

typedef struct _PCIPBUSDATA
{
    PCIBUSDATA CommonData;
    union
    {
        struct
        {
            PULONG Address;
            ULONG Data;
        } Type1;
        struct
        {
            PUCHAR CSE;
            PUCHAR Forward;
            ULONG Base;
        } Type2;
    } Config;
    ULONG MaxDevice;
    PciIrqRange GetIrqRange;
    BOOLEAN BridgeConfigRead;
    UCHAR ParentBus;
    UCHAR Subtractive;
    UCHAR reserved[1];
    UCHAR SwizzleIn[4];
    RTL_BITMAP DeviceConfigured;
    ULONG ConfiguredBits[PCI_MAX_DEVICES * PCI_MAX_FUNCTION / 32];
} PCIPBUSDATA, *PPCIPBUSDATA;

typedef ULONG
(NTAPI *FncConfigIO)(
    IN PPCIPBUSDATA BusData,
    IN PVOID State,
    IN PUCHAR Buffer,
    IN ULONG Offset
);

typedef VOID
(NTAPI *FncSync)(
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PKIRQL Irql,
    IN PVOID State
);

typedef VOID
(NTAPI *FncReleaseSync)(
    IN PBUS_HANDLER BusHandler,
    IN KIRQL Irql
);

typedef struct _PCI_CONFIG_HANDLER
{
    FncSync Synchronize;
    FncReleaseSync ReleaseSynchronzation;
    FncConfigIO ConfigRead[3];
    FncConfigIO ConfigWrite[3];
} PCI_CONFIG_HANDLER, *PPCI_CONFIG_HANDLER;

typedef struct _PCI_REGISTRY_INFO_INTERNAL
{
    UCHAR MajorRevision;
    UCHAR MinorRevision;
    UCHAR NoBuses; // Number Of Buses
    UCHAR HardwareMechanism;
    ULONG ElementCount;
    PCI_CARD_DESCRIPTOR CardList[ANYSIZE_ARRAY];
} PCI_REGISTRY_INFO_INTERNAL, *PPCI_REGISTRY_INFO_INTERNAL;

typedef struct _ARRAY
{
    ULONG ArraySize;
    PVOID Element[ANYSIZE_ARRAY];
} ARRAY, *PARRAY;

typedef struct _HAL_BUS_HANDLER
{
    LIST_ENTRY AllHandlers;
    ULONG ReferenceCount;
    BUS_HANDLER Handler;
} HAL_BUS_HANDLER, *PHAL_BUS_HANDLER;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
HalpPCISynchronizeType1(
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PKIRQL Irql,
    IN PPCI_TYPE1_CFG_BITS PciCfg
);

VOID
NTAPI
HalpPCIReleaseSynchronzationType1(
    IN PBUS_HANDLER BusHandler,
    IN KIRQL Irql
);

VOID
NTAPI
HalpPCISynchronizeType2(
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PKIRQL Irql,
    IN PPCI_TYPE2_ADDRESS_BITS PciCfg
);

VOID
NTAPI
HalpPCIReleaseSynchronizationType2(
    IN PBUS_HANDLER BusHandler,
    IN KIRQL Irql
);

TYPE1_DEFINE(HalpPCIReadUcharType1);
TYPE1_DEFINE(HalpPCIReadUshortType1);
TYPE1_DEFINE(HalpPCIReadUlongType1);
TYPE2_DEFINE(HalpPCIReadUcharType2);
TYPE2_DEFINE(HalpPCIReadUshortType2);
TYPE2_DEFINE(HalpPCIReadUlongType2);
TYPE1_DEFINE(HalpPCIWriteUcharType1);
TYPE1_DEFINE(HalpPCIWriteUshortType1);
TYPE1_DEFINE(HalpPCIWriteUlongType1);
TYPE2_DEFINE(HalpPCIWriteUcharType2);
TYPE2_DEFINE(HalpPCIWriteUshortType2);
TYPE2_DEFINE(HalpPCIWriteUlongType2);

BOOLEAN
NTAPI
HalpValidPCISlot(
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot
);

VOID
NTAPI
HalpReadPCIConfig(
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
);

VOID
NTAPI
HalpWritePCIConfig(
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
);

ULONG
NTAPI
HalpGetSystemInterruptVector(
    ULONG BusNumber,
    ULONG BusInterruptLevel,
    ULONG BusInterruptVector,
    PKIRQL Irql,
    PKAFFINITY Affinity
);

ULONG
NTAPI
HalpGetCmosData(
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
);

ULONG
NTAPI
HalpSetCmosData(
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
);

ULONG
NTAPI
HalpGetPCIData(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootBusHandler,
    IN PCI_SLOT_NUMBER SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
);

ULONG
NTAPI
HalpSetPCIData(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootBusHandler,
    IN PCI_SLOT_NUMBER SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
);

NTSTATUS
NTAPI
HalpAssignPCISlotResources(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING DriverClassName OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN ULONG Slot,
    IN OUT PCM_RESOURCE_LIST *pAllocatedResources
);

VOID
NTAPI
HalpInitializePciBus(
    VOID
);

VOID
NTAPI
HalpInitializePciStubs(
    VOID
);

VOID
NTAPI
HalpInitBusHandler(
    VOID
);

BOOLEAN
NTAPI
HalpTranslateBusAddress(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
);

NTSTATUS
NTAPI
HalpAssignSlotResources(
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING DriverClassName,
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN OUT PCM_RESOURCE_LIST *AllocatedResources
);

BOOLEAN
NTAPI
HalpFindBusAddressTranslation(
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress,
    IN OUT PULONG_PTR Context,
    IN BOOLEAN NextBus
);

extern ULONG HalpBusType;
extern BOOLEAN HalpPCIConfigInitialized;
extern BUS_HANDLER HalpFakePciBusHandler;
extern ULONG HalpMinPciBus, HalpMaxPciBus;

/* EOF */
