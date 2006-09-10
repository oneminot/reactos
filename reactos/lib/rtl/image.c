/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/image.c
 * PURPOSE:         Image handling functions
 *                  Relocate functions were previously located in
 *                  ntoskrnl/ldr/loader.c
 * PROGRAMMER:      Eric Kohl + original authors from loader.c file
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
PIMAGE_NT_HEADERS NTAPI
RtlImageNtHeader (IN PVOID BaseAddress)
{
  PIMAGE_NT_HEADERS NtHeader;
  PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)BaseAddress;

  if (DosHeader && DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
      DPRINT1("DosHeader->e_magic %x\n", DosHeader->e_magic);
      DPRINT1("NtHeader 0x%lx\n", ((ULONG_PTR)BaseAddress + DosHeader->e_lfanew));
    }

  if (DosHeader && DosHeader->e_magic == IMAGE_DOS_SIGNATURE)
    {
      NtHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)BaseAddress + DosHeader->e_lfanew);
      if (NtHeader->Signature == IMAGE_NT_SIGNATURE)
	return NtHeader;
    }

  return NULL;
}


/*
 * @implemented
 */
PVOID
NTAPI
RtlImageDirectoryEntryToData(PVOID BaseAddress,
                             BOOLEAN MappedAsImage,
                             USHORT Directory,
                             PULONG Size)
{
	PIMAGE_NT_HEADERS NtHeader;
	ULONG Va;

	/* Magic flag for non-mapped images. */
	if ((ULONG_PTR)BaseAddress & 1)
	{
		BaseAddress = (PVOID)((ULONG_PTR)BaseAddress & ~1);
		MappedAsImage = FALSE;
        }


	NtHeader = RtlImageNtHeader (BaseAddress);
	if (NtHeader == NULL)
		return NULL;

	if (Directory >= NtHeader->OptionalHeader.NumberOfRvaAndSizes)
		return NULL;

	Va = NtHeader->OptionalHeader.DataDirectory[Directory].VirtualAddress;
	if (Va == 0)
		return NULL;

	*Size = NtHeader->OptionalHeader.DataDirectory[Directory].Size;

	if (MappedAsImage || Va < NtHeader->OptionalHeader.SizeOfHeaders)
		return (PVOID)((ULONG_PTR)BaseAddress + Va);

	/* image mapped as ordinary file, we must find raw pointer */
	return RtlImageRvaToVa (NtHeader, BaseAddress, Va, NULL);
}


/*
 * @implemented
 */
PIMAGE_SECTION_HEADER
NTAPI
RtlImageRvaToSection (
	PIMAGE_NT_HEADERS	NtHeader,
	PVOID			BaseAddress,
	ULONG			Rva
	)
{
	PIMAGE_SECTION_HEADER Section;
	ULONG Va;
	ULONG Count;

	Count = NtHeader->FileHeader.NumberOfSections;
	Section = (PIMAGE_SECTION_HEADER)((ULONG)&NtHeader->OptionalHeader +
	                                  NtHeader->FileHeader.SizeOfOptionalHeader);
	while (Count)
	{
		Va = Section->VirtualAddress;
		if ((Va <= Rva) &&
		    (Rva < Va + Section->SizeOfRawData))
			return Section;
		Section++;
	}
	return NULL;
}


/*
 * @implemented
 */
PVOID
NTAPI
RtlImageRvaToVa (
	PIMAGE_NT_HEADERS	NtHeader,
	PVOID			BaseAddress,
	ULONG			Rva,
	PIMAGE_SECTION_HEADER	*SectionHeader
	)
{
	PIMAGE_SECTION_HEADER Section = NULL;

	if (SectionHeader)
		Section = *SectionHeader;

	if (Section == NULL ||
	    Rva < Section->VirtualAddress ||
	    Rva >= Section->VirtualAddress + Section->SizeOfRawData)
	{
		Section = RtlImageRvaToSection (NtHeader, BaseAddress, Rva);
		if (Section == NULL)
			return 0;

		if (SectionHeader)
			*SectionHeader = Section;
	}

	return (PVOID)((ULONG_PTR)BaseAddress +
	               Rva +
	               Section->PointerToRawData -
	               (ULONG_PTR)Section->VirtualAddress);
}

ULONG
NTAPI
LdrRelocateImageWithBias(
    IN PVOID BaseAddress,
    IN LONGLONG AdditionalBias,
    IN PUCHAR LoaderName,
    IN ULONG Success,
    IN ULONG Conflict,
    IN ULONG Invalid
    )
{
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_DATA_DIRECTORY RelocationDDir;
    PIMAGE_BASE_RELOCATION RelocationDir, RelocationEnd;
    ULONG Count, i;
    PVOID Address;//, MaxAddress;
    PUSHORT TypeOffset;
    ULONG_PTR Delta;
    SHORT Offset;
    USHORT Type;
    PUSHORT ShortPtr;
    PULONG LongPtr;

    NtHeaders = RtlImageNtHeader(BaseAddress);

    if (NtHeaders == NULL)
        return Invalid;

    if (NtHeaders->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED)
    {
        return Conflict;
    }

    RelocationDDir = &NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

    if (RelocationDDir->VirtualAddress == 0 || RelocationDDir->Size == 0)
    {
        return Success;
    }

    Delta = (ULONG_PTR)BaseAddress - NtHeaders->OptionalHeader.ImageBase + AdditionalBias;
    RelocationDir = (PIMAGE_BASE_RELOCATION)((ULONG_PTR)BaseAddress + RelocationDDir->VirtualAddress);
    RelocationEnd = (PIMAGE_BASE_RELOCATION)((ULONG_PTR)RelocationDir + RelocationDDir->Size);
    //MaxAddress = RVA(BaseAddress, DriverSize);

    while (RelocationDir < RelocationEnd &&
        RelocationDir->SizeOfBlock > 0)
    {
        Count = (RelocationDir->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(USHORT);
        Address = RVA(BaseAddress, RelocationDir->VirtualAddress);
        TypeOffset = (PUSHORT)(RelocationDir + 1);

        for (i = 0; i < Count; i++)
        {
            Offset = *TypeOffset & 0xFFF;
            Type = *TypeOffset >> 12;
            ShortPtr = (PUSHORT)(RVA(Address, Offset));

            /* Don't relocate after the end of the loaded driver */
            /*if ((PVOID)ShortPtr >= MaxAddress)
            {
                break;
            }*/

            /*
            * Don't relocate within the relocation section itself.
            * GCC/LD generates sometimes relocation records for the relocation section.
            * This is a bug in GCC/LD.
            */
            if ((ULONG_PTR)ShortPtr < (ULONG_PTR)RelocationDir ||
                (ULONG_PTR)ShortPtr >= (ULONG_PTR)RelocationEnd)
            {
                switch (Type)
                {
                case IMAGE_REL_BASED_ABSOLUTE:
                    break;

                case IMAGE_REL_BASED_HIGH:
                    *ShortPtr += HIWORD(Delta);
                    break;

                case IMAGE_REL_BASED_LOW:
                    *ShortPtr += LOWORD(Delta);
                    break;

                case IMAGE_REL_BASED_HIGHLOW:
                    LongPtr = (PULONG)ShortPtr;
                    *LongPtr += Delta;
                    break;

                case IMAGE_REL_BASED_HIGHADJ:
                case IMAGE_REL_BASED_MIPS_JMPADDR:
                default:
                    DPRINT1("Unknown/unsupported fixup type %hu.\n", Type);
                    DPRINT1("Address %x, Current %d, Count %d, *TypeOffset %x\n", Address, i, Count, *TypeOffset);
                    return Invalid;
                }
            }
            TypeOffset++;
        }
        RelocationDir = (PIMAGE_BASE_RELOCATION)((ULONG_PTR)RelocationDir + RelocationDir->SizeOfBlock);
    }

    return Success;
}
/* EOF */
