/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/arch/arm/stubs.c
 * PURPOSE:         Non-completed ARM hardware-specific routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#include <internal/arm/ke.h>
#include <internal/arm/mm.h>
#include <internal/arm/intrin_i.h>

/* GLOBALS ********************************************************************/

ULONG PageDirectoryStart, PageDirectoryEnd;
LOADER_PARAMETER_BLOCK ArmLoaderBlock;
LOADER_PARAMETER_EXTENSION ArmExtension;
extern ARM_TRANSLATION_TABLE ArmTranslationTable;
extern ROS_KERNEL_ENTRY_POINT KernelEntryPoint;

/* FUNCTIONS ******************************************************************/

BOOLEAN
ArmDiskGetDriveGeometry(IN ULONG DriveNumber,
                        OUT PGEOMETRY Geometry)
{
    ASSERT(gRamDiskBase == NULL);
    return FALSE;
}

BOOLEAN
ArmDiskReadLogicalSectors(IN ULONG DriveNumber,
                          IN ULONGLONG SectorNumber,
                          IN ULONG SectorCount,
                          IN PVOID Buffer)
{
    ASSERT(gRamDiskBase == NULL);
    return FALSE;
}

ULONG
ArmDiskGetCacheableBlockCount(IN ULONG DriveNumber)
{
    ASSERT(gRamDiskBase == NULL);
    return FALSE;
}

VOID
ArmSetupPageDirectory(VOID)
{   
    ARM_TTB_REGISTER TtbRegister;
    ARM_DOMAIN_REGISTER DomainRegister;
    ARM_PTE Pte;
    ULONG i;
    PARM_TRANSLATION_TABLE TranslationTable;
    
    //
    // Allocate translation table buffer.
    // During bootstrap, this will be a simple L1 (Master) Page Table with
    // Section entries for KSEG0 and the first MB of RAM.
    //
    TranslationTable = &ArmTranslationTable;
    if (!TranslationTable) return;
    
    //
    // Set it as the TTB
    //
    TtbRegister.AsUlong = (ULONG)TranslationTable;
    ASSERT(TtbRegister.Reserved == 0);
    KeArmTranslationTableRegisterSet(TtbRegister);
    
    //
    // Use Domain 0, enforce AP bits (client)
    //
    DomainRegister.AsUlong = 0;
    DomainRegister.Domain0 = ClientDomain;
    KeArmDomainRegisterSet(DomainRegister);
    
    //
    // Set Fault PTEs everywhere
    //
    RtlZeroMemory(TranslationTable, 4096 * sizeof(ARM_PTE));
    
    //
    // Build the template PTE
    //
    Pte.L1.Section.Type = SectionPte;
    Pte.L1.Section.Buffered = FALSE;
    Pte.L1.Section.Cached = FALSE;
    Pte.L1.Section.Reserved = 1; // ARM926EJ-S manual recommends setting to 1
    Pte.L1.Section.Domain = Domain0;
    Pte.L1.Section.Access = SupervisorAccess;
    Pte.L1.Section.BaseAddress = 0;
    Pte.L1.Section.Ignored = Pte.L1.Section.Ignored1 = 0;
    
    //
    // Map KSEG0 (0x80000000 - 0xA0000000) to 0x00000000 - 0x80000000
    // In this way, the KERNEL_PHYS_ADDR (0x800000) becomes 0x80800000
    // which is the entrypoint, just like on x86.
    //
    for (i = (KSEG0_BASE >> TTB_SHIFT); i < ((KSEG0_BASE + 0x20000000) >> TTB_SHIFT); i++)
    {
        //
        // Write PTE and update the base address (next MB) for the next one
        //
        TranslationTable->Pte[i] = Pte;
        Pte.L1.Section.BaseAddress++;
    }
    
    //
    // Identity map the first MB of memory as well
    //
    Pte.L1.Section.BaseAddress = 0;
    TranslationTable->Pte[0] = Pte;
}

VOID
ArmSetupPagingAndJump(IN ULONG Magic)
{
    ARM_CONTROL_REGISTER ControlRegister;
    
    //
    // Enable MMU, DCache and ICache
    //
    ControlRegister = KeArmControlRegisterGet();
    ControlRegister.MmuEnabled = TRUE;
    ControlRegister.ICacheEnabled = TRUE;
    ControlRegister.DCacheEnabled = TRUE;
    KeArmControlRegisterSet(ControlRegister);
    
    //
    // Jump to Kernel
    //
    (*KernelEntryPoint)(Magic, (PVOID)&ArmLoaderBlock);
}

VOID
ArmPrepareForReactOS(IN BOOLEAN Setup)
{   
    //
    // Initialize the loader block
    //
    InitializeListHead(&ArmLoaderBlock.BootDriverListHead);
    InitializeListHead(&ArmLoaderBlock.LoadOrderListHead);
    InitializeListHead(&ArmLoaderBlock.MemoryDescriptorListHead);
    
    //
    // Setup the extension and setup block
    //
    ArmLoaderBlock.Extension = &ArmExtension;
    ArmLoaderBlock.SetupLdrBlock = NULL;
    
    //
    // TODO: Setup memory descriptors
    //
    
    //
    // TODO: Setup registry data
    //
    
    //
    // TODO: Setup ARC Hardware tree data
    //
    
    //
    // TODO: Setup NLS data
    //
    
    //
    // TODO: Setup boot-driver data
    //
    
    //
    // TODO: Setup extension parameters
    //
    
    //
    // TODO: Setup ARM-specific block
    //
}

PCONFIGURATION_COMPONENT_DATA
ArmHwDetect(VOID)
{
    PCONFIGURATION_COMPONENT_DATA RootNode;
    
    //
    // Create the root node
    //
    FldrCreateSystemKey(&RootNode);
    
    //
    // Write null component information
    //
    FldrSetComponentInformation(RootNode,
                                0x0,
                                0x0,
                                0xFFFFFFFF);
    
    //
    // TODO:
    // There's no such thing as "PnP" on embedded hardware.
    // The boot loader will send us a device tree, similar to ACPI
    // or OpenFirmware device trees, and we will convert it to ARC.
    //

    //
    // Return the root node
    //
    return RootNode;
}

ULONG
ArmMemGetMemoryMap(OUT PBIOS_MEMORY_MAP BiosMemoryMap,
                   IN ULONG MaxMemoryMapSize)
{
    //
    // Return whatever the board returned to us (CS0 Base + Size and FLASH0)
    //
    RtlCopyMemory(BiosMemoryMap,
                  ArmBoardBlock->MemoryMap,
                  ArmBoardBlock->MemoryMapEntryCount * sizeof(BIOS_MEMORY_MAP));
    return ArmBoardBlock->MemoryMapEntryCount;
}

VOID
MachInit(IN PCCH CommandLine)
{
    //
    // Setup board-specific ARM routines
    //
    switch (ArmBoardBlock->BoardType)
    {
        //
        // Check for Feroceon-base boards
        //
        case MACH_TYPE_FEROCEON:
            
            //
            // These boards use a UART16550. Set us up for 115200 bps
            //
            ArmFeroSerialInit(115200);
            MachVtbl.ConsPutChar = ArmFeroPutChar;
            MachVtbl.ConsKbHit = ArmFeroKbHit;
            MachVtbl.ConsGetCh = ArmFeroGetCh;
            break;
            
        //
        // Check for ARM Versatile PB boards
        //
        case MACH_TYPE_VERSATILE_PB:
            
            //
            // These boards use a PrimeCell UART (PL011)
            //
            ArmVersaSerialInit(115200);
            MachVtbl.ConsPutChar = ArmVersaPutChar;
            MachVtbl.ConsKbHit = ArmVersaKbHit;
            MachVtbl.ConsGetCh = ArmVersaGetCh;
            break;
            
        default:
            ASSERT(FALSE);
    }
    
    //
    // Setup generic ARM routines for all boards
    //
    MachVtbl.PrepareForReactOS = ArmPrepareForReactOS;
    MachVtbl.GetMemoryMap = ArmMemGetMemoryMap;
    MachVtbl.HwDetect = ArmHwDetect;

    //
    // Setup disk I/O routines, switch to ramdisk ones for non-NAND boot
    //
    MachVtbl.DiskReadLogicalSectors = ArmDiskReadLogicalSectors;
    MachVtbl.DiskGetDriveGeometry = ArmDiskGetDriveGeometry;
    MachVtbl.DiskGetCacheableBlockCount = ArmDiskGetCacheableBlockCount;
    RamDiskSwitchFromBios();

    //
    // Now set default disk handling routines -- we don't need to override
    //
    MachVtbl.DiskGetBootVolume = DiskGetBootVolume;
    MachVtbl.DiskGetSystemVolume = DiskGetSystemVolume;
    MachVtbl.DiskGetBootPath = DiskGetBootPath;
    MachVtbl.DiskGetBootDevice = DiskGetBootDevice;
    MachVtbl.DiskBootingFromFloppy = DiskBootingFromFloppy;
    MachVtbl.DiskNormalizeSystemPath = DiskNormalizeSystemPath;
    MachVtbl.DiskGetPartitionEntry = DiskGetPartitionEntry;

    //
    // We can now print to the console
    //
    TuiPrintf("%s for ARM\n", GetFreeLoaderVersionString());
    TuiPrintf("Bootargs: %s\n", CommandLine);
}

VOID
FrLdrStartup(IN ULONG Magic)
{
    //
    // Disable interrupts (aleady done)
    //

    //
    // Set proper CPSR (already done)
    //

    //
    // Initialize the page directory
    //
    ArmSetupPageDirectory();

    //
    // Initialize paging and load NTOSKRNL
    //
    ArmSetupPagingAndJump(Magic);
}
