/* $Id: loader.c,v 1.116 2002/07/17 21:04:56 dwelch Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/loader.c
 * PURPOSE:         Loaders for PE executables
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 *                  Jason Filby (jasonfilby@yahoo.com)
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *   DW   22/05/98   Created
 *   RJJ  10/12/98   Completed image loader function and added hooks for MZ/PE
 *   RJJ  10/12/98   Built driver loader function and added hooks for PE/COFF
 *   RJJ  10/12/98   Rolled in David's code to load COFF drivers
 *   JM   14/12/98   Built initial PE user module loader
 *   RJJ  06/03/99   Moved user PE loader into NTDLL
 *   JF   26/01/2000 Recoded some parts to retrieve export details correctly
 *   DW   27/06/2000 Removed redundant header files
 *   CSH  11/04/2001 Added automatic loading of module symbols if they exist
 */


/* INCLUDES *****************************************************************/

#include <limits.h>
#include <ddk/ntddk.h>
#include <roscfg.h>
#include <internal/module.h>
#include <internal/ntoskrnl.h>
#include <internal/kd.h>
#include <internal/io.h>
#include <internal/mm.h>
#include <internal/ps.h>
#include <internal/ldr.h>
#include <internal/pool.h>

#ifdef HALDBG
#include <internal/ntosdbg.h>
#else
#define ps(args...)
#endif

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#ifdef DBG

typedef struct _SYMBOLFILE_HEADER {
  unsigned long StabsOffset;
  unsigned long StabsLength;
  unsigned long StabstrOffset;
  unsigned long StabstrLength;
} SYMBOLFILE_HEADER, *PSYMBOLFILE_HEADER;

typedef struct _IMAGE_SYMBOL_INFO_CACHE {
  LIST_ENTRY ListEntry;
  UNICODE_STRING FullName;
  PVOID FileBuffer;
  PVOID SymbolsBase;
  ULONG SymbolsLength;
  PVOID SymbolStringsBase;
  ULONG SymbolStringsLength;
} IMAGE_SYMBOL_INFO_CACHE, *PIMAGE_SYMBOL_INFO_CACHE;


typedef struct _STAB_ENTRY {
  unsigned long n_strx;         /* index into string table of name */
  unsigned char n_type;         /* type of symbol */
  unsigned char n_other;        /* misc info (usually empty) */
  unsigned short n_desc;        /* description field */
  unsigned long n_value;        /* value of symbol */
} _STAB_ENTRY, *PSTAB_ENTRY;

/*
 * Desc - Line number
 * Value - Relative virtual address
 */
#define N_FUN 0x24

/*
 * Desc - Line number
 * Value - Relative virtual address
 */
#define N_SLINE 0x44

/*
 * String - First containing a '/' is the compillation directory (CD)
 *          Not containing a '/' is a source file relative to CD
 */
#define N_SO 0x64

LIST_ENTRY SymbolListHead;

#endif /* DBG */

LIST_ENTRY ModuleListHead;
KSPIN_LOCK ModuleListLock;

LIST_ENTRY ModuleTextListHead;
STATIC MODULE_TEXT_SECTION NtoskrnlTextSection;
STATIC MODULE_TEXT_SECTION LdrHalTextSection;
ULONG_PTR LdrHalBase;

#define TAG_DRIVER_MEM  TAG('D', 'R', 'V', 'M')

/* FORWARD DECLARATIONS ******************************************************/

NTSTATUS
LdrProcessModule(PVOID ModuleLoadBase,
		 PUNICODE_STRING ModuleName,
		 PMODULE_OBJECT *ModuleObject);

PVOID
LdrGetExportAddress(PMODULE_OBJECT ModuleObject,
		    char *Name,
		    unsigned short Hint);

static VOID
LdrpBuildModuleBaseName(PUNICODE_STRING BaseName,
			PUNICODE_STRING FullName);

static LONG
LdrpCompareModuleNames(IN PUNICODE_STRING String1,
		       IN PUNICODE_STRING String2);


/*  PE Driver load support  */
static NTSTATUS LdrPEProcessModule(PVOID ModuleLoadBase,
                                   PUNICODE_STRING FileName,
                                   PMODULE_OBJECT *ModuleObject);
static PVOID
LdrPEGetExportAddress(PMODULE_OBJECT ModuleObject,
		      PCHAR Name,
		      USHORT Hint);

static PVOID
LdrSafePEGetExportAddress(PVOID ImportModuleBase,
			  PCHAR Name,
			  USHORT Hint);

static PVOID
LdrPEFixupForward(PCHAR ForwardName);


/* FUNCTIONS *****************************************************************/

VOID
LdrInitDebug(PLOADER_MODULE Module, PWCH Name)
{
  PLIST_ENTRY current_entry;
  MODULE_TEXT_SECTION* current;

  current_entry = ModuleTextListHead.Flink;
  while (current_entry != &ModuleTextListHead)
    {
      current = 
	CONTAINING_RECORD(current_entry, MODULE_TEXT_SECTION, ListEntry);
      if (wcscmp(current->Name, Name) == 0)
	{
	  break;
	}
      current_entry = current_entry->Flink;
    }

  if (current_entry == &ModuleTextListHead)
    {
      return;
    }
}

VOID
LdrInit1(VOID)
{
  PIMAGE_DOS_HEADER DosHeader;
  PIMAGE_FILE_HEADER FileHeader;
  PIMAGE_OPTIONAL_HEADER OptionalHeader;
  PIMAGE_SECTION_HEADER SectionList;

  InitializeListHead(&ModuleTextListHead);

  /* Setup ntoskrnl.exe text section */
  DosHeader = (PIMAGE_DOS_HEADER) KERNEL_BASE;
  FileHeader =
    (PIMAGE_FILE_HEADER) ((DWORD)KERNEL_BASE + 
			  DosHeader->e_lfanew + sizeof(ULONG));
  OptionalHeader = (PIMAGE_OPTIONAL_HEADER)
    ((DWORD)FileHeader + sizeof(IMAGE_FILE_HEADER));
  SectionList = (PIMAGE_SECTION_HEADER)
    ((DWORD)OptionalHeader + sizeof(IMAGE_OPTIONAL_HEADER));
  NtoskrnlTextSection.Base = KERNEL_BASE;
  NtoskrnlTextSection.Length = SectionList[0].Misc.VirtualSize +
    SectionList[0].VirtualAddress;
  NtoskrnlTextSection.Name = KERNEL_MODULE_NAME;
#ifdef DBG
  RtlZeroMemory(&NtoskrnlTextSection.SymbolInfo, sizeof(NtoskrnlTextSection.SymbolInfo));
  NtoskrnlTextSection.SymbolInfo.ImageBase = OptionalHeader->ImageBase;
  NtoskrnlTextSection.SymbolInfo.ImageSize = NtoskrnlTextSection.Length;
#endif
  InsertTailList(&ModuleTextListHead, &NtoskrnlTextSection.ListEntry);

  /* Setup hal.dll text section */
  DosHeader = (PIMAGE_DOS_HEADER)LdrHalBase;
  FileHeader =
    (PIMAGE_FILE_HEADER) ((DWORD)LdrHalBase + 
			  DosHeader->e_lfanew + sizeof(ULONG));
  OptionalHeader = (PIMAGE_OPTIONAL_HEADER)
    ((DWORD)FileHeader + sizeof(IMAGE_FILE_HEADER));
  SectionList = (PIMAGE_SECTION_HEADER)
    ((DWORD)OptionalHeader + sizeof(IMAGE_OPTIONAL_HEADER));
  LdrHalTextSection.Base = LdrHalBase;
  LdrHalTextSection.Length = SectionList[0].Misc.VirtualSize +
    SectionList[0].VirtualAddress;
  LdrHalTextSection.Name = HAL_MODULE_NAME;
#ifdef DBG
  RtlZeroMemory(&LdrHalTextSection.SymbolInfo, sizeof(LdrHalTextSection.SymbolInfo));
  LdrHalTextSection.SymbolInfo.ImageBase = OptionalHeader->ImageBase;
  LdrHalTextSection.SymbolInfo.ImageSize = LdrHalTextSection.Length;
#endif
  InsertTailList(&ModuleTextListHead, &LdrHalTextSection.ListEntry);

#ifdef DBG
  InitializeListHead(&SymbolListHead);
#endif
}


VOID
LdrInitModuleManagement(VOID)
{
  PIMAGE_DOS_HEADER DosHeader;
  PMODULE_OBJECT ModuleObject;

  /* Initialize the module list and spinlock */
  InitializeListHead(&ModuleListHead);
  KeInitializeSpinLock(&ModuleListLock);

  /* Create module object for NTOSKRNL */
  ModuleObject = ExAllocatePool(NonPagedPool, sizeof(MODULE_OBJECT));
  assert(ModuleObject != NULL);
  RtlZeroMemory(ModuleObject, sizeof(MODULE_OBJECT));

  /* Initialize ModuleObject data */
  ModuleObject->Base = (PVOID) KERNEL_BASE;
  ModuleObject->Flags = MODULE_FLAG_PE;
  RtlCreateUnicodeString(&ModuleObject->FullName,
			 KERNEL_MODULE_NAME);
  LdrpBuildModuleBaseName(&ModuleObject->BaseName,
			  &ModuleObject->FullName);

  DosHeader = (PIMAGE_DOS_HEADER) KERNEL_BASE;
  ModuleObject->Image.PE.FileHeader =
    (PIMAGE_FILE_HEADER) ((DWORD) ModuleObject->Base +
    DosHeader->e_lfanew + sizeof(ULONG));
  ModuleObject->Image.PE.OptionalHeader = (PIMAGE_OPTIONAL_HEADER)
    ((DWORD)ModuleObject->Image.PE.FileHeader + sizeof(IMAGE_FILE_HEADER));
  ModuleObject->Image.PE.SectionList = (PIMAGE_SECTION_HEADER)
    ((DWORD)ModuleObject->Image.PE.OptionalHeader + sizeof(IMAGE_OPTIONAL_HEADER));
  ModuleObject->EntryPoint = (PVOID) ((DWORD) ModuleObject->Base +
    ModuleObject->Image.PE.OptionalHeader->AddressOfEntryPoint);
  DPRINT("ModuleObject:%08x  entrypoint at %x\n", ModuleObject, ModuleObject->EntryPoint);
  ModuleObject->Length = ModuleObject->Image.PE.OptionalHeader->SizeOfImage;
  ModuleObject->TextSection = &NtoskrnlTextSection;

  InsertTailList(&ModuleListHead,
		 &ModuleObject->ListEntry);

  /* Create module object for HAL */
  ModuleObject = ExAllocatePool(NonPagedPool, sizeof(MODULE_OBJECT));
  assert(ModuleObject != NULL);
  RtlZeroMemory(ModuleObject, sizeof(MODULE_OBJECT));

  /* Initialize ModuleObject data */
  ModuleObject->Base = (PVOID) LdrHalBase;
  ModuleObject->Flags = MODULE_FLAG_PE;

  RtlCreateUnicodeString(&ModuleObject->FullName,
			 HAL_MODULE_NAME);
  LdrpBuildModuleBaseName(&ModuleObject->BaseName,
			  &ModuleObject->FullName);

  DosHeader = (PIMAGE_DOS_HEADER) LdrHalBase;
  ModuleObject->Image.PE.FileHeader =
    (PIMAGE_FILE_HEADER) ((DWORD) ModuleObject->Base +
    DosHeader->e_lfanew + sizeof(ULONG));
  ModuleObject->Image.PE.OptionalHeader = (PIMAGE_OPTIONAL_HEADER)
    ((DWORD)ModuleObject->Image.PE.FileHeader + sizeof(IMAGE_FILE_HEADER));
  ModuleObject->Image.PE.SectionList = (PIMAGE_SECTION_HEADER)
    ((DWORD)ModuleObject->Image.PE.OptionalHeader + sizeof(IMAGE_OPTIONAL_HEADER));
  ModuleObject->EntryPoint = (PVOID) ((DWORD) ModuleObject->Base +
    ModuleObject->Image.PE.OptionalHeader->AddressOfEntryPoint);
  DPRINT("ModuleObject:%08x  entrypoint at %x\n", ModuleObject, ModuleObject->EntryPoint);
  ModuleObject->Length = ModuleObject->Image.PE.OptionalHeader->SizeOfImage;
  ModuleObject->TextSection = &LdrHalTextSection;

  InsertTailList(&ModuleListHead,
		 &ModuleObject->ListEntry);
}

#ifdef DBG

VOID
LdrpParseImageSymbols(PIMAGE_SYMBOL_INFO SymbolInfo)
/* Note: It is important that the symbol strings buffer not be released after
   this function is called because the strings are still referenced */
{
  PSYMBOL CurrentFileNameSymbol;
  PSYMBOL CurrentFunctionSymbol;
  PSYMBOL CurrentLineNumberSymbol;
  PSYMBOL Symbol;
  PSTAB_ENTRY StabEntry;
  PVOID StabsEnd;
  PCHAR String;
  ULONG_PTR FunRelativeAddress;
  ULONG FunLineNumber;
  ULONG_PTR ImageBase;

  assert(SymbolInfo);

  DPRINT("Parsing symbols.\n");

  SymbolInfo->FileNameSymbols.SymbolCount = 0;
  SymbolInfo->FileNameSymbols.Symbols = NULL;
  SymbolInfo->FunctionSymbols.SymbolCount = 0;
  SymbolInfo->FunctionSymbols.Symbols = NULL;
  SymbolInfo->LineNumberSymbols.SymbolCount = 0;
  SymbolInfo->LineNumberSymbols.Symbols = NULL;
  StabsEnd = SymbolInfo->SymbolsBase + SymbolInfo->SymbolsLength;
  StabEntry = (PSTAB_ENTRY) SymbolInfo->SymbolsBase;
  ImageBase = SymbolInfo->ImageBase;
  FunRelativeAddress = 0;
  FunLineNumber = 0;
  CurrentFileNameSymbol = NULL;
  CurrentFunctionSymbol = NULL;
  CurrentLineNumberSymbol = NULL;
  while ((ULONG_PTR) StabEntry < (ULONG_PTR) StabsEnd)
    {
      Symbol = NULL;

      if (StabEntry->n_type == N_FUN)
        {
          if (StabEntry->n_desc > 0)
            {
              assert(StabEntry->n_value >= ImageBase);

              FunRelativeAddress = StabEntry->n_value - ImageBase;
              FunLineNumber = StabEntry->n_desc;

              Symbol = ExAllocatePool(NonPagedPool, sizeof(SYMBOL));
              assert(Symbol);
              Symbol->Next = NULL;
              Symbol->SymbolType = ST_FUNCTION;
              Symbol->RelativeAddress = FunRelativeAddress;
              Symbol->LineNumber = FunLineNumber;
              String = (PCHAR)SymbolInfo->SymbolStringsBase + StabEntry->n_strx;
              RtlInitAnsiString(&Symbol->Name, String);

              DPRINT("FUN found. '%s' %d @ %x\n",
                Symbol->Name.Buffer, FunLineNumber, FunRelativeAddress);
            }
        }
      else if (StabEntry->n_type == N_SLINE)
        {
          Symbol = ExAllocatePool(NonPagedPool, sizeof(SYMBOL));
          assert(Symbol);
          Symbol->Next = NULL;
          Symbol->SymbolType = ST_LINENUMBER;
          Symbol->RelativeAddress = FunRelativeAddress + StabEntry->n_value;
          Symbol->LineNumber = StabEntry->n_desc;

          DPRINT("SLINE found. %d @ %x\n",
            Symbol->LineNumber, Symbol->RelativeAddress);
        }
      else if (StabEntry->n_type == N_SO)
        {
          Symbol = ExAllocatePool(NonPagedPool, sizeof(SYMBOL));
          assert(Symbol);
          Symbol->Next = NULL;
          Symbol->SymbolType = ST_FILENAME;
          Symbol->RelativeAddress = StabEntry->n_value - ImageBase;
          Symbol->LineNumber = 0;
          String = (PCHAR)SymbolInfo->SymbolStringsBase + StabEntry->n_strx;
          RtlInitAnsiString(&Symbol->Name, String);

          DPRINT("SO found. '%s' @ %x\n",
            Symbol->Name.Buffer, Symbol->RelativeAddress);
        }

      if (Symbol != NULL)
        {
          switch (Symbol->SymbolType)
          {
            case ST_FILENAME:
              if (SymbolInfo->FileNameSymbols.Symbols == NULL)
                SymbolInfo->FileNameSymbols.Symbols = Symbol;
              else
                CurrentFileNameSymbol->Next = Symbol;

              CurrentFileNameSymbol = Symbol;

              SymbolInfo->FileNameSymbols.SymbolCount++;
              break;
            case ST_FUNCTION:
              if (SymbolInfo->FunctionSymbols.Symbols == NULL)
                SymbolInfo->FunctionSymbols.Symbols = Symbol;
              else
                CurrentFunctionSymbol->Next = Symbol;

              CurrentFunctionSymbol = Symbol;

              SymbolInfo->FunctionSymbols.SymbolCount++;
              break;
            case ST_LINENUMBER:
              if (SymbolInfo->LineNumberSymbols.Symbols == NULL)
                SymbolInfo->LineNumberSymbols.Symbols = Symbol;
              else
                CurrentLineNumberSymbol->Next = Symbol;

              CurrentLineNumberSymbol = Symbol;

              SymbolInfo->LineNumberSymbols.SymbolCount++;
              break;
          }
        }

      StabEntry++;
    }
}

static NTSTATUS
LdrpGetFileName(IN PIMAGE_SYMBOL_INFO  SymbolInfo,
  IN ULONG_PTR  RelativeAddress,
  OUT PCH  FileName)
{
  PSYMBOL NextSymbol;
  ULONG_PTR NextAddress;
  PSYMBOL Symbol;

  Symbol = SymbolInfo->FileNameSymbols.Symbols;
  while (Symbol != NULL)
    {
      NextSymbol = Symbol->Next;
      if (NextSymbol != NULL)
        NextAddress = NextSymbol->RelativeAddress;
      else
        NextAddress = SymbolInfo->ImageSize;

      DPRINT("FN SEARCH: Type %d  RelativeAddress %x >= Symbol->RelativeAddress %x  < NextAddress %x\n",
        Symbol->SymbolType, RelativeAddress, Symbol->RelativeAddress, NextAddress);

      if ((Symbol->SymbolType == ST_FILENAME) &&
        (RelativeAddress >= Symbol->RelativeAddress) &&
        (RelativeAddress < NextAddress))
        {
          DPRINT("FN found\n");
          strcpy(FileName, Symbol->Name.Buffer);
          return STATUS_SUCCESS;
        }
      Symbol = NextSymbol;
    }

  DPRINT("FN not found\n");

  return STATUS_UNSUCCESSFUL;
}

static NTSTATUS
LdrpGetFunctionName(IN PIMAGE_SYMBOL_INFO  SymbolInfo,
  IN ULONG_PTR  RelativeAddress,
  OUT PCH  FunctionName)
{
  PSYMBOL NextSymbol;
  ULONG_PTR NextAddress;
  PSYMBOL Symbol;

  Symbol = SymbolInfo->FunctionSymbols.Symbols;
  while (Symbol != NULL)
    {
      NextSymbol = Symbol->Next;
      if (NextSymbol != NULL)
        NextAddress = NextSymbol->RelativeAddress;
      else
        NextAddress = SymbolInfo->ImageSize;

      DPRINT("FUN SEARCH: Type %d  RelativeAddress %x >= Symbol->RelativeAddress %x  < NextAddress %x\n",
        Symbol->SymbolType, RelativeAddress, Symbol->RelativeAddress, NextAddress);

      if ((Symbol->SymbolType == ST_FUNCTION) &&
        (RelativeAddress >= Symbol->RelativeAddress) &&
        (RelativeAddress < NextAddress))
        {
          PCHAR ExtraInfo;
          ULONG Length;

          DPRINT("FUN found\n");

          /* Remove the extra information from the function name */
          ExtraInfo = strchr(Symbol->Name.Buffer, ':');
          if (ExtraInfo != NULL)
            Length = ExtraInfo - Symbol->Name.Buffer;
          else
            Length = strlen(Symbol->Name.Buffer);

          strncpy(FunctionName, Symbol->Name.Buffer, Length);
          return STATUS_SUCCESS;
        }
      Symbol = NextSymbol;
    }

  DPRINT("FUN not found\n");

  return STATUS_UNSUCCESSFUL;
}

static NTSTATUS
LdrpGetLineNumber(IN PIMAGE_SYMBOL_INFO  SymbolInfo,
  IN ULONG_PTR  RelativeAddress,
  OUT PULONG  LineNumber)
{
  PSYMBOL NextSymbol;
  ULONG_PTR NextAddress;
  PSYMBOL Symbol;

  Symbol = SymbolInfo->LineNumberSymbols.Symbols;
  while (Symbol != NULL)
    {
      NextSymbol = Symbol->Next;
      if (NextSymbol != NULL)
        NextAddress = NextSymbol->RelativeAddress;
      else
        NextAddress = SymbolInfo->ImageSize;

      DPRINT("LN SEARCH: Type %d  RelativeAddress %x >= Symbol->RelativeAddress %x  < NextAddress %x\n",
        Symbol->SymbolType, RelativeAddress, Symbol->RelativeAddress, NextAddress);

      if ((Symbol->SymbolType == ST_LINENUMBER) &&
        (RelativeAddress >= Symbol->RelativeAddress) &&
        (RelativeAddress < NextAddress))
        {
          DPRINT("LN found\n");
          *LineNumber = Symbol->LineNumber;
          return STATUS_SUCCESS;
        }
      Symbol = NextSymbol;
    }

  DPRINT("LN not found\n");

  return STATUS_UNSUCCESSFUL;
}

NTSTATUS
LdrGetAddressInformation(IN PIMAGE_SYMBOL_INFO  SymbolInfo,
  IN ULONG_PTR  RelativeAddress,
  OUT PULONG LineNumber,
  OUT PCH FileName  OPTIONAL,
  OUT PCH FunctionName  OPTIONAL)
{
  NTSTATUS Status;

  *LineNumber = 0;

  DPRINT("RelativeAddress %p\n", RelativeAddress);

  if (RelativeAddress >= SymbolInfo->ImageSize)
    {
      DPRINT("Address is not within .text section. RelativeAddress %p  Length 0x%x\n",
        RelativeAddress, SymbolInfo->ImageSize);
      return STATUS_UNSUCCESSFUL;
    }

  if (!AreSymbolsParsed(SymbolInfo))
    {
      LdrpParseImageSymbols(SymbolInfo);
    }

  Status = LdrpGetLineNumber(SymbolInfo, RelativeAddress, LineNumber);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  if (FileName)
   {
     Status = LdrpGetFileName(SymbolInfo, RelativeAddress, FileName);
     if (!NT_SUCCESS(Status))
       {
         strcpy(FileName, "");
       }
   }

  if (FunctionName)
   {
     Status = LdrpGetFunctionName(SymbolInfo, RelativeAddress, FunctionName);
     if (!NT_SUCCESS(Status))
       {
         strcpy(FunctionName, "");
       }
   }

  return STATUS_SUCCESS;
}

VOID
LdrpLoadModuleSymbols(PUNICODE_STRING FileName,
  PIMAGE_SYMBOL_INFO SymbolInfo)
{
  FILE_STANDARD_INFORMATION FileStdInfo;
  OBJECT_ATTRIBUTES ObjectAttributes;
  WCHAR TmpFileName[MAX_PATH];
  UNICODE_STRING SymFileName;
  LPWSTR Start, Ext;
  HANDLE FileHandle;
  PVOID FileBuffer;
  NTSTATUS Status;
  ULONG Length;
  IO_STATUS_BLOCK IoStatusBlock;
  PSYMBOLFILE_HEADER SymbolFileHeader;

  /*  Get the path to the symbol store  */
  wcscpy(TmpFileName, L"\\SystemRoot\\symbols\\");

  /*  Get the symbol filename from the module name  */
  Start = wcsrchr(FileName->Buffer, L'\\');
  if (Start == NULL)
    Start = FileName->Buffer;
  else
    Start++;

  Ext = wcsrchr(FileName->Buffer, L'.');
  if (Ext != NULL)
    Length = Ext - Start;
  else
    Length = wcslen(Start);

  wcsncat(TmpFileName, Start, Length);
  wcscat(TmpFileName, L".sym");
  RtlInitUnicodeString(&SymFileName, TmpFileName);

  /*  Open the file  */
  InitializeObjectAttributes(&ObjectAttributes,
                             &SymFileName,
                             0,
                             NULL,
                             NULL);

  Status = ZwOpenFile(&FileHandle,
                      FILE_ALL_ACCESS,
                      &ObjectAttributes,
                      &IoStatusBlock,
                      0,
                      0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Could not open symbol file: %wZ\n", &SymFileName);
      return;
    }

  CPRINT("Loading symbols from %wZ...\n", &SymFileName);

  /*  Get the size of the file  */
  Status = ZwQueryInformationFile(FileHandle,
                                  &IoStatusBlock,
                                  &FileStdInfo,
                                  sizeof(FileStdInfo),
                                  FileStandardInformation);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Could not get file size\n");
      ZwClose(FileHandle);
      return;
    }

  /*  Allocate nonpageable memory for symbol file  */
  FileBuffer = ExAllocatePool(NonPagedPool,
                              FileStdInfo.EndOfFile.u.LowPart);

  if (FileBuffer == NULL)
    {
      DPRINT("Could not allocate memory for symbol file\n");
      ZwClose(FileHandle);
      return;
    }
   
  /*  Load file into memory chunk  */
  Status = ZwReadFile(FileHandle,
                      0, 0, 0,
                      &IoStatusBlock,
                      FileBuffer,
                      FileStdInfo.EndOfFile.u.LowPart,
                      0, 0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Could not read symbol file into memory (Status 0x%x)\n", Status);
      ExFreePool(FileBuffer);
      ZwClose(FileHandle);
      return;
    }

  ZwClose(FileHandle);

  SymbolFileHeader = (PSYMBOLFILE_HEADER) FileBuffer;
  SymbolInfo->FileBuffer = FileBuffer;
  SymbolInfo->SymbolsBase = FileBuffer + SymbolFileHeader->StabsOffset;
  SymbolInfo->SymbolsLength = SymbolFileHeader->StabsLength;
  SymbolInfo->SymbolStringsBase = FileBuffer + SymbolFileHeader->StabstrOffset;
  SymbolInfo->SymbolStringsLength = SymbolFileHeader->StabstrLength;
}


VOID
LdrUnloadModuleSymbols(PIMAGE_SYMBOL_INFO SymbolInfo)
{
  PSYMBOL NextSymbol;
  PSYMBOL Symbol;

  DPRINT("Unloading symbols\n");

  if (SymbolInfo != NULL)
    {
      Symbol = SymbolInfo->FileNameSymbols.Symbols;
      while (Symbol != NULL)
	{
	  NextSymbol = Symbol->Next;
	  RtlFreeAnsiString(&Symbol->Name);
	  ExFreePool(Symbol);
	  Symbol = NextSymbol;
	}

      SymbolInfo->FileNameSymbols.SymbolCount = 0;
      SymbolInfo->FileNameSymbols.Symbols = NULL;

      Symbol = SymbolInfo->FunctionSymbols.Symbols;
      while (Symbol != NULL)
	{
	  NextSymbol = Symbol->Next;
	  RtlFreeAnsiString(&Symbol->Name);
	  ExFreePool(Symbol);
	  Symbol = NextSymbol;
	}

      SymbolInfo->FunctionSymbols.SymbolCount = 0;
      SymbolInfo->FunctionSymbols.Symbols = NULL;

      Symbol = SymbolInfo->LineNumberSymbols.Symbols;
      while (Symbol != NULL)
	{
	  NextSymbol = Symbol->Next;
	  RtlFreeAnsiString(&Symbol->Name);
	  ExFreePool(Symbol);
	  Symbol = NextSymbol;
	}

      SymbolInfo->LineNumberSymbols.SymbolCount = 0;
      SymbolInfo->LineNumberSymbols.Symbols = NULL;
#if 0
      /* Don't free buffers because we cache symbol buffers
         (eg. they are shared across processes) */
      /* FIXME: We can free them if we do reference counting */
      if (SymbolInfo->FileBuffer != NULL)
        {
          ExFreePool(SymbolInfo->FileBuffer);
          SymbolInfo->FileBuffer = NULL;
          SymbolInfo->SymbolsBase = NULL;
          SymbolInfo->SymbolsLength = 0;
        }
#endif
    }
}


PIMAGE_SYMBOL_INFO_CACHE
LdrpLookupUserSymbolInfo(PLDR_MODULE LdrModule)
{
  PIMAGE_SYMBOL_INFO_CACHE Current;
  PLIST_ENTRY CurrentEntry;
  KIRQL Irql;

  DPRINT("Searching symbols for %S\n", LdrModule->FullDllName.Buffer);

  KeAcquireSpinLock(&ModuleListLock,&Irql);

  CurrentEntry = SymbolListHead.Flink;
  while (CurrentEntry != (&SymbolListHead))
    {
      Current = CONTAINING_RECORD(CurrentEntry, IMAGE_SYMBOL_INFO_CACHE, ListEntry);

      if (RtlEqualUnicodeString(&Current->FullName, &LdrModule->FullDllName, TRUE))
        {
          KeReleaseSpinLock(&ModuleListLock, Irql);
          return Current;
        }

      CurrentEntry = CurrentEntry->Flink;
    }

  KeReleaseSpinLock(&ModuleListLock, Irql);

  return(NULL);
}


VOID
LdrLoadUserModuleSymbols(PLDR_MODULE LdrModule)
{
  PIMAGE_SYMBOL_INFO_CACHE CacheEntry;

  DPRINT("LdrModule %p\n", LdrModule);

  RtlZeroMemory(&LdrModule->SymbolInfo, sizeof(LdrModule->SymbolInfo));
  LdrModule->SymbolInfo.ImageBase = (ULONG_PTR) LdrModule->BaseAddress;
  LdrModule->SymbolInfo.ImageSize = LdrModule->SizeOfImage;

  CacheEntry = LdrpLookupUserSymbolInfo(LdrModule);
  if (CacheEntry != NULL)
    {
      DPRINT("Symbol cache hit for %S\n", CacheEntry->FullName.Buffer);
   
      LdrModule->SymbolInfo.FileBuffer = CacheEntry->FileBuffer;
      LdrModule->SymbolInfo.SymbolsBase = CacheEntry->SymbolsBase;
      LdrModule->SymbolInfo.SymbolsLength = CacheEntry->SymbolsLength;
      LdrModule->SymbolInfo.SymbolStringsBase = CacheEntry->SymbolStringsBase;
      LdrModule->SymbolInfo.SymbolStringsLength = CacheEntry->SymbolStringsLength;
    }
  else
    {
      CacheEntry = ExAllocatePool(NonPagedPool, sizeof(IMAGE_SYMBOL_INFO_CACHE));
      assert(CacheEntry);
      RtlZeroMemory(CacheEntry, sizeof(IMAGE_SYMBOL_INFO_CACHE));
      RtlCreateUnicodeString(&CacheEntry->FullName, LdrModule->FullDllName.Buffer);
      assert(CacheEntry->FullName.Buffer);
      LdrpLoadModuleSymbols(&LdrModule->FullDllName, &LdrModule->SymbolInfo);
      InsertTailList(&SymbolListHead, &CacheEntry->ListEntry);
    }
}

#endif /* DBG */


NTSTATUS
LdrpLoadImage(PUNICODE_STRING DriverName,
	      PVOID *ModuleBase,
	      PVOID *SectionPointer,
	      PVOID *EntryPoint,
	      PVOID *ExportSectionPointer)
{
  PMODULE_OBJECT ModuleObject;
  NTSTATUS Status;

  ModuleObject = LdrGetModuleObject(DriverName);
  if (ModuleObject == NULL)
    {
      Status = LdrLoadModule(DriverName, &ModuleObject);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}
    }

  if (ModuleBase)
    *ModuleBase = ModuleObject->Base;

//  if (SectionPointer)
//    *SectionPointer = ModuleObject->

  if (EntryPoint)
    *EntryPoint = ModuleObject->EntryPoint;

//  if (ExportSectionPointer)
//    *ExportSectionPointer = ModuleObject->

  return(STATUS_SUCCESS);
}


NTSTATUS
LdrpUnloadImage(PVOID ModuleBase)
{
  return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS
LdrpLoadAndCallImage(PUNICODE_STRING ModuleName)
{
  PDRIVER_INITIALIZE DriverEntry;
  PMODULE_OBJECT ModuleObject;
  NTSTATUS Status;

  ModuleObject = LdrGetModuleObject(ModuleName);
  if (ModuleObject != NULL)
    {
      return(STATUS_IMAGE_ALREADY_LOADED);
    }

  Status = LdrLoadModule(ModuleName, &ModuleObject);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  DriverEntry = (PDRIVER_INITIALIZE)ModuleObject->EntryPoint;

  Status = DriverEntry(NULL, NULL);
  if (!NT_SUCCESS(Status))
    {
      LdrUnloadModule(ModuleObject);
    }

  return(Status);
}


NTSTATUS
LdrLoadModule(PUNICODE_STRING Filename,
	      PMODULE_OBJECT *ModuleObject)
{
  PVOID ModuleLoadBase;
  NTSTATUS Status;
  HANDLE FileHandle;
  OBJECT_ATTRIBUTES ObjectAttributes;
  PMODULE_OBJECT Module;
  FILE_STANDARD_INFORMATION FileStdInfo;
  IO_STATUS_BLOCK IoStatusBlock;

  *ModuleObject = NULL;

  DPRINT("Loading Module %wZ...\n", Filename);

  /*  Open the Module  */
  InitializeObjectAttributes(&ObjectAttributes,
                             Filename,
                             0,
                             NULL,
                             NULL);
  CHECKPOINT;
  Status = NtOpenFile(&FileHandle,
                      FILE_ALL_ACCESS,
                      &ObjectAttributes,
                      &IoStatusBlock,
                      0,
                      0);
  CHECKPOINT;
  if (!NT_SUCCESS(Status))
    {
      CPRINT("Could not open module file: %wZ\n", Filename);
      return(Status);
    }
  CHECKPOINT;

  /*  Get the size of the file  */
  Status = NtQueryInformationFile(FileHandle,
                                  &IoStatusBlock,
                                  &FileStdInfo,
                                  sizeof(FileStdInfo),
                                  FileStandardInformation);
  if (!NT_SUCCESS(Status))
    {
      CPRINT("Could not get file size\n");
      NtClose(FileHandle);
      return(Status);
    }
  CHECKPOINT;

  /*  Allocate nonpageable memory for driver  */
  ModuleLoadBase = ExAllocatePoolWithTag(NonPagedPool,
					 FileStdInfo.EndOfFile.u.LowPart,
					 TAG_DRIVER_MEM);
  if (ModuleLoadBase == NULL)
    {
      CPRINT("Could not allocate memory for module");
      NtClose(FileHandle);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }
  CHECKPOINT;

  /*  Load driver into memory chunk  */
  Status = NtReadFile(FileHandle,
                      0, 0, 0,
                      &IoStatusBlock,
                      ModuleLoadBase,
                      FileStdInfo.EndOfFile.u.LowPart,
                      0, 0);
  if (!NT_SUCCESS(Status))
    {
      CPRINT("Could not read module file into memory");
      ExFreePool(ModuleLoadBase);
      NtClose(FileHandle);
      return(Status);
    }
  CHECKPOINT;

  NtClose(FileHandle);

  Status = LdrProcessModule(ModuleLoadBase,
                            Filename,
                            &Module);
  if (!NT_SUCCESS(Status))
    {
      CPRINT("Could not process module");
      ExFreePool(ModuleLoadBase);
      return(Status);
    }

  /*  Cleanup  */
  ExFreePool(ModuleLoadBase);

#ifdef DBG
  /* Load symbols for the image if available */
  LdrpLoadModuleSymbols(Filename, &Module->TextSection->SymbolInfo);
#endif /* DBG */

  *ModuleObject = Module;

  return(STATUS_SUCCESS);
}


NTSTATUS
LdrUnloadModule(PMODULE_OBJECT ModuleObject)
{
  KIRQL Irql;

  /* Remove the module from the module list */
  KeAcquireSpinLock(&ModuleListLock,&Irql);
  RemoveEntryList(&ModuleObject->ListEntry);
  KeReleaseSpinLock(&ModuleListLock, Irql);

#ifdef DBG
  /* Unload symbols for module if available */
  LdrUnloadModuleSymbols(&ModuleObject->TextSection->SymbolInfo);
#endif /* DBG */

  /* Free text section */
  if (ModuleObject->TextSection != NULL)
    {
      ExFreePool(ModuleObject->TextSection->Name);
      RemoveEntryList(&ModuleObject->TextSection->ListEntry);
      ExFreePool(ModuleObject->TextSection);
      ModuleObject->TextSection = NULL;
    }

  /* Free module section */
//  MmFreeSection(ModuleObject->Base);

  ExFreePool(ModuleObject);

  return(STATUS_SUCCESS);
}


NTSTATUS
LdrInitializeBootStartDriver(PVOID ModuleLoadBase,
			     PCHAR FileName,
			     ULONG ModuleLength)
{
#ifdef DBG
  PSYMBOLFILE_HEADER SymbolFileHeader;
  PIMAGE_SYMBOL_INFO SymbolInfo;
#endif /* DBG */
  PMODULE_OBJECT ModuleObject;
  UNICODE_STRING ModuleName;
  PDEVICE_NODE DeviceNode;
  NTSTATUS Status;

  WCHAR Buffer[MAX_PATH];
  ULONG Length;
  LPWSTR Start;
  LPWSTR Ext;
  PCHAR FileExt;

  CHAR TextBuffer [256];
  ULONG x, y, cx, cy;

#ifdef DBG
  CHAR TmpBaseName[MAX_PATH];
  CHAR TmpFileName[MAX_PATH];
  ANSI_STRING AnsiString;
#endif /* DBG */

  HalQueryDisplayParameters(&x, &y, &cx, &cy);
  RtlFillMemory(TextBuffer, x, ' ');
  TextBuffer[x] = '\0';
  HalSetDisplayParameters(0, y-1);
  HalDisplayString(TextBuffer);

  sprintf(TextBuffer, "Initializing %s...\n", FileName);
  HalSetDisplayParameters(0, y-1);
  HalDisplayString(TextBuffer);
  HalSetDisplayParameters(cx, cy);

  /*  Split the filename into base name and extension  */
  FileExt = strrchr(FileName, '.');
  if (FileExt != NULL)
    Length = FileExt - FileName;
  else
    Length = strlen(FileName);

#ifdef DBG

  if ((FileExt != NULL) && (strcmp(FileExt, ".sym") == 0))
    {
      DPRINT("Module %s is a symbol file\n", FileName);

      strncpy(TmpBaseName, FileName, Length);
      TmpBaseName[Length] = '\0';

      DPRINT("base: %s (Length %d)\n", TmpBaseName, Length);

      strcpy(TmpFileName, TmpBaseName);
      strcat(TmpFileName, ".sys");
      RtlInitAnsiString(&AnsiString, TmpFileName);

      RtlAnsiStringToUnicodeString(&ModuleName, &AnsiString, TRUE);
      ModuleObject = LdrGetModuleObject(&ModuleName);
      RtlFreeUnicodeString(&ModuleName);
      if (ModuleObject == NULL)
	{
	  strcpy(TmpFileName, TmpBaseName);
	  strcat(TmpFileName, ".exe");
	  RtlInitAnsiString(&AnsiString, TmpFileName);
	  RtlAnsiStringToUnicodeString(&ModuleName, &AnsiString, TRUE);
	  ModuleObject = LdrGetModuleObject(&ModuleName);
	  RtlFreeUnicodeString(&ModuleName);
	}
      if (ModuleObject != NULL)
	{
          SymbolInfo = (PIMAGE_SYMBOL_INFO) &ModuleObject->TextSection->SymbolInfo;
          SymbolFileHeader = (PSYMBOLFILE_HEADER) ModuleLoadBase;
          SymbolInfo->FileBuffer = ModuleLoadBase;
          SymbolInfo->SymbolsBase = ModuleLoadBase + SymbolFileHeader->StabsOffset;
          SymbolInfo->SymbolsLength = SymbolFileHeader->StabsLength;
          SymbolInfo->SymbolStringsBase = ModuleLoadBase + SymbolFileHeader->StabstrOffset;
          SymbolInfo->SymbolStringsLength = SymbolFileHeader->StabstrLength;
	}

      return(STATUS_SUCCESS);
    }
  else
    {
      DPRINT("Module %s is non-symbol file\n", FileName);
    }

#endif /* !DBG */

  if ((FileExt != NULL) && !(strcmp(FileExt, ".sys") == 0))
    {
      CPRINT("Ignoring non-driver file %s\n", FileName);
      return STATUS_SUCCESS;
    }

  /* Use IopRootDeviceNode for now */
  Status = IopCreateDeviceNode(IopRootDeviceNode, NULL, &DeviceNode);
  if (!NT_SUCCESS(Status))
    {
      CPRINT("Driver load failed, status (%x)\n", Status);
      return(Status);
    }

  RtlCreateUnicodeStringFromAsciiz(&ModuleName,
				   FileName);
  Status = LdrProcessModule(ModuleLoadBase,
			    &ModuleName,
			    &ModuleObject);
  RtlFreeUnicodeString(&ModuleName);
  if (ModuleObject == NULL)
    {
      IopFreeDeviceNode(DeviceNode);
      CPRINT("Driver load failed, status (%x)\n", Status);
      return(STATUS_UNSUCCESSFUL);
    }


  /* Get the service name from the module name */
  Start = wcsrchr(ModuleObject->BaseName.Buffer, L'\\');
  if (Start == NULL)
    Start = ModuleObject->BaseName.Buffer;
  else
    Start++;

  Ext = wcsrchr(ModuleObject->BaseName.Buffer, L'.');
  if (Ext != NULL)
    Length = Ext - Start;
  else
    Length = wcslen(Start);

  wcsncpy(Buffer, Start, Length);
  RtlCreateUnicodeString(&DeviceNode->ServiceName, Buffer);

  Status = IopInitializeDriver(ModuleObject->EntryPoint,
			       DeviceNode, FALSE);
  if (!NT_SUCCESS(Status))
    {
      IopFreeDeviceNode(DeviceNode);
      CPRINT("Driver load failed, status (%x)\n", Status);
    }

  return(Status);
}


NTSTATUS
LdrProcessModule(PVOID ModuleLoadBase,
		 PUNICODE_STRING ModuleName,
		 PMODULE_OBJECT *ModuleObject)
{
  PIMAGE_DOS_HEADER PEDosHeader;

  /*  If MZ header exists  */
  PEDosHeader = (PIMAGE_DOS_HEADER) ModuleLoadBase;
  if (PEDosHeader->e_magic == IMAGE_DOS_MAGIC && PEDosHeader->e_lfanew != 0L)
    {
      return LdrPEProcessModule(ModuleLoadBase,
				ModuleName,
				ModuleObject);
    }

  CPRINT("Module wasn't PE\n");
  return STATUS_UNSUCCESSFUL;
}


PVOID
LdrGetExportAddress(PMODULE_OBJECT ModuleObject,
                    char *Name,
                    unsigned short Hint)
{
  if (ModuleObject->Flags & MODULE_FLAG_PE)
    {
      return LdrPEGetExportAddress(ModuleObject, Name, Hint);
    }
  else
    {
      return 0;
    }
}


NTSTATUS
LdrpQueryModuleInformation(PVOID Buffer,
			   ULONG Size,
			   PULONG ReqSize)
{
  PLIST_ENTRY current_entry;
  PMODULE_OBJECT current;
  ULONG ModuleCount = 0;
  PSYSTEM_MODULE_INFORMATION Smi;
  ANSI_STRING AnsiName;
  PCHAR p;
  KIRQL Irql;

  KeAcquireSpinLock(&ModuleListLock,&Irql);

  /* calculate required size */
  current_entry = ModuleListHead.Flink;
  while (current_entry != (&ModuleListHead))
    {
      ModuleCount++;
      current_entry = current_entry->Flink;
    }

  *ReqSize = sizeof(SYSTEM_MODULE_INFORMATION)+
    (ModuleCount - 1) * sizeof(SYSTEM_MODULE_ENTRY);

  if (Size < *ReqSize)
    {
      KeReleaseSpinLock(&ModuleListLock, Irql);
      return(STATUS_INFO_LENGTH_MISMATCH);
    }

  /* fill the buffer */
  memset(Buffer, '=', Size);

  Smi = (PSYSTEM_MODULE_INFORMATION)Buffer;
  Smi->Count = ModuleCount;

  ModuleCount = 0;
  current_entry = ModuleListHead.Flink;
  while (current_entry != (&ModuleListHead))
    {
      current = CONTAINING_RECORD(current_entry,MODULE_OBJECT,ListEntry);

      Smi->Module[ModuleCount].Unknown2 = 0;		/* Always 0 */
      Smi->Module[ModuleCount].BaseAddress = current->Base;
      Smi->Module[ModuleCount].Size = current->Length;
      Smi->Module[ModuleCount].Flags = 0;		/* Flags ??? (GN) */
      Smi->Module[ModuleCount].EntryIndex = ModuleCount;

      AnsiName.Length = 0;
      AnsiName.MaximumLength = 256;
      AnsiName.Buffer = Smi->Module[ModuleCount].Name;
      RtlUnicodeStringToAnsiString(&AnsiName,
				   &current->FullName,
				   FALSE);

      p = strrchr(AnsiName.Buffer, '\\');
      if (p == NULL)
	{
	  Smi->Module[ModuleCount].PathLength = 0;
	  Smi->Module[ModuleCount].NameLength = strlen(AnsiName.Buffer);
	}
      else
	{
	  p++;
	  Smi->Module[ModuleCount].PathLength = p - AnsiName.Buffer;
	  Smi->Module[ModuleCount].NameLength = strlen(p);
	}

      ModuleCount++;
      current_entry = current_entry->Flink;
    }

  KeReleaseSpinLock(&ModuleListLock, Irql);

  return(STATUS_SUCCESS);
}


static VOID
LdrpBuildModuleBaseName(PUNICODE_STRING BaseName,
			PUNICODE_STRING FullName)
{
   UNICODE_STRING Name;
   PWCHAR p;
   PWCHAR q;

   DPRINT("LdrpBuildModuleBaseName()\n");
   DPRINT("FullName %wZ\n", FullName);

   p = wcsrchr(FullName->Buffer, L'\\');
   if (p == NULL)
     {
	p = FullName->Buffer;
     }
   else
     {
	p++;
     }

   DPRINT("p %S\n", p);

   RtlCreateUnicodeString(&Name, p);

   q = wcschr(Name.Buffer, L'.');
   if (q != NULL)
     {
	*q = (WCHAR)0;
     }

   DPRINT("p %S\n", p);

   RtlCreateUnicodeString(BaseName, Name.Buffer);
   RtlFreeUnicodeString(&Name);
}


static LONG
LdrpCompareModuleNames(IN PUNICODE_STRING String1,
		       IN PUNICODE_STRING String2)
{
  ULONG len1, len2, i;
  PWCHAR s1, s2, p;
  WCHAR  c1, c2;

  if (String1 && String2)
    {
      /* Search String1 for last path component */
      len1 = String1->Length / sizeof(WCHAR);
      s1 = String1->Buffer;
      for (i = 0, p = String1->Buffer; i < String1->Length; i = i + sizeof(WCHAR), p++)
	{
	  if (*p == L'\\')
	    {
	      if (i == String1->Length - sizeof(WCHAR))
		{
		  s1 = NULL;
		  len1 = 0;
		}
	      else
		{
		  s1 = p + 1;
		  len1 = (String1->Length - i) / sizeof(WCHAR);
		}
	    }
	}

      /* Search String2 for last path component */
      len2 = String2->Length / sizeof(WCHAR);
      s2 = String2->Buffer;
      for (i = 0, p = String2->Buffer; i < String2->Length; i = i + sizeof(WCHAR), p++)
	{
	  if (*p == L'\\')
	    {
	      if (i == String2->Length - sizeof(WCHAR))
		{
		  s2 = NULL;
		  len2 = 0;
		}
	      else
		{
		  s2 = p + 1;
		  len2 = (String2->Length - i) / sizeof(WCHAR);
		}
	    }
	}

      /* Compare last path components */
      if (s1 && s2)
	{
	  while (1)
	    {
	      c1 = len1-- ? RtlUpcaseUnicodeChar (*s1++) : 0;
	      c2 = len2-- ? RtlUpcaseUnicodeChar (*s2++) : 0;
	      if ((c1 == 0 && c2 == L'.') || (c1 == L'.' && c2 == 0))
		return(0);
	      if (!c1 || !c2 || c1 != c2)
		return(c1 - c2);
	    }
	}
    }

  return(0);
}


PMODULE_OBJECT
LdrGetModuleObject(PUNICODE_STRING ModuleName)
{
  PMODULE_OBJECT Module;
  PLIST_ENTRY Entry;
  KIRQL Irql;

  DPRINT("LdrpGetModuleObject(%wZ) called\n", ModuleName);

  KeAcquireSpinLock(&ModuleListLock,&Irql);

  Entry = ModuleListHead.Flink;
  while (Entry != &ModuleListHead)
    {
      Module = CONTAINING_RECORD(Entry, MODULE_OBJECT, ListEntry);

      DPRINT("Comparing %wZ and %wZ\n",
	     &Module->BaseName,
	     ModuleName);

      if (!LdrpCompareModuleNames(&Module->BaseName, ModuleName))
	{
	  DPRINT("Module %wZ\n", &Module->BaseName);
	  KeReleaseSpinLock(&ModuleListLock, Irql);
	  return(Module);
	}

      Entry = Entry->Flink;
    }

  KeReleaseSpinLock(&ModuleListLock, Irql);

  DPRINT("Could not find module '%wZ'\n", ModuleName);

  return(NULL);
}


/*  ----------------------------------------------  PE Module support */

static NTSTATUS
LdrPEProcessModule(PVOID ModuleLoadBase,
		   PUNICODE_STRING FileName,
		   PMODULE_OBJECT *ModuleObject)
{
  unsigned int DriverSize, Idx;
  ULONG RelocDelta, NumRelocs;
  DWORD CurrentSize, TotalRelocs;
  PVOID DriverBase;
  PULONG PEMagic;
  PIMAGE_DOS_HEADER PEDosHeader;
  PIMAGE_FILE_HEADER PEFileHeader;
  PIMAGE_OPTIONAL_HEADER PEOptionalHeader;
  PIMAGE_SECTION_HEADER PESectionHeaders;
  PRELOCATION_DIRECTORY RelocDir;
  PRELOCATION_ENTRY RelocEntry;
  PMODULE_OBJECT  LibraryModuleObject;
  PMODULE_OBJECT CreatedModuleObject;
  PVOID *ImportAddressList;
  PULONG FunctionNameList;
  PCHAR pName;
  WORD Hint;
  UNICODE_STRING ModuleName;
  UNICODE_STRING NameString;
  WCHAR  NameBuffer[60];
  MODULE_TEXT_SECTION* ModuleTextSection;
  NTSTATUS Status;
  KIRQL Irql;

  DPRINT("Processing PE Module at module base:%08lx\n", ModuleLoadBase);

  /*  Get header pointers  */
  PEDosHeader = (PIMAGE_DOS_HEADER) ModuleLoadBase;
  PEMagic = (PULONG) ((unsigned int) ModuleLoadBase + 
    PEDosHeader->e_lfanew);
  PEFileHeader = (PIMAGE_FILE_HEADER) ((unsigned int) ModuleLoadBase + 
    PEDosHeader->e_lfanew + sizeof(ULONG));
  PEOptionalHeader = (PIMAGE_OPTIONAL_HEADER) ((unsigned int) ModuleLoadBase + 
    PEDosHeader->e_lfanew + sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER));
  PESectionHeaders = (PIMAGE_SECTION_HEADER) ((unsigned int) ModuleLoadBase + 
    PEDosHeader->e_lfanew + sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER) +
    sizeof(IMAGE_OPTIONAL_HEADER));
  CHECKPOINT;

  /*  Check file magic numbers  */
  if (PEDosHeader->e_magic != IMAGE_DOS_MAGIC)
    {
      CPRINT("Incorrect MZ magic: %04x\n", PEDosHeader->e_magic);
      return STATUS_UNSUCCESSFUL;
    }
  if (PEDosHeader->e_lfanew == 0)
    {
      CPRINT("Invalid lfanew offset: %08x\n", PEDosHeader->e_lfanew);
      return STATUS_UNSUCCESSFUL;
    }
  if (*PEMagic != IMAGE_PE_MAGIC)
    {
      CPRINT("Incorrect PE magic: %08x\n", *PEMagic);
      return STATUS_UNSUCCESSFUL;
    }
  if (PEFileHeader->Machine != IMAGE_FILE_MACHINE_I386)
    {
      CPRINT("Incorrect Architechture: %04x\n", PEFileHeader->Machine);
      return STATUS_UNSUCCESSFUL;
    }
  CHECKPOINT;

  /* FIXME: if image is fixed-address load, then fail  */

  /* FIXME: check/verify OS version number  */

  DPRINT("OptionalHdrMagic:%04x LinkVersion:%d.%d\n", 
         PEOptionalHeader->Magic,
         PEOptionalHeader->MajorLinkerVersion,
         PEOptionalHeader->MinorLinkerVersion);
  DPRINT("Entry Point:%08lx\n", PEOptionalHeader->AddressOfEntryPoint);
  CHECKPOINT;

  /*  Determine the size of the module  */
  DriverSize = PEOptionalHeader->SizeOfImage;
  DPRINT("DriverSize %x\n",DriverSize);

  /*  Allocate a virtual section for the module  */
  DriverBase = MmAllocateSection(DriverSize);
  if (DriverBase == 0)
    {
      CPRINT("Failed to allocate a virtual section for driver\n");
      return STATUS_UNSUCCESSFUL;
    }
  DbgPrint("DriverBase for %wZ: %x\n", FileName, DriverBase);
  CHECKPOINT;
  /*  Copy headers over */
  memcpy(DriverBase, ModuleLoadBase, PEOptionalHeader->SizeOfHeaders);
   CurrentSize = 0;
  /*  Copy image sections into virtual section  */
  for (Idx = 0; Idx < PEFileHeader->NumberOfSections; Idx++)
    {
      //  Copy current section into current offset of virtual section
      if (PESectionHeaders[Idx].Characteristics & 
          (IMAGE_SECTION_CHAR_CODE | IMAGE_SECTION_CHAR_DATA))
        {
	   DPRINT("PESectionHeaders[Idx].VirtualAddress + DriverBase %x\n",
		  PESectionHeaders[Idx].VirtualAddress + DriverBase);
           memcpy(PESectionHeaders[Idx].VirtualAddress + DriverBase,
                  (PVOID)(ModuleLoadBase + PESectionHeaders[Idx].PointerToRawData),
                  PESectionHeaders[Idx].Misc.VirtualSize > PESectionHeaders[Idx].SizeOfRawData
                  ? PESectionHeaders[Idx].SizeOfRawData : PESectionHeaders[Idx].Misc.VirtualSize );
        }
      else
        {
	   DPRINT("PESectionHeaders[Idx].VirtualAddress + DriverBase %x\n",
		  PESectionHeaders[Idx].VirtualAddress + DriverBase);
	   memset(PESectionHeaders[Idx].VirtualAddress + DriverBase, 
		  '\0', PESectionHeaders[Idx].Misc.VirtualSize);

        }
      CurrentSize += ROUND_UP(PESectionHeaders[Idx].Misc.VirtualSize,
                              PEOptionalHeader->SectionAlignment);


//      CurrentBase = (PVOID)((DWORD)CurrentBase + 
  //      ROUND_UP(PESectionHeaders[Idx].SizeOfRawData.Misc.VirtualSize,
    //             PEOptionalHeader->SectionAlignment));
    }

  /*  Perform relocation fixups  */
  RelocDelta = (DWORD) DriverBase - PEOptionalHeader->ImageBase;
  RelocDir = (PRELOCATION_DIRECTORY)(PEOptionalHeader->DataDirectory[
    IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
  DPRINT("DrvrBase:%08lx ImgBase:%08lx RelocDelta:%08lx\n", 
         DriverBase,
         PEOptionalHeader->ImageBase,
         RelocDelta);   
  DPRINT("RelocDir %x\n",RelocDir);
#if 1
  for (Idx = 0; Idx < PEFileHeader->NumberOfSections; Idx++)
    {
       if (PESectionHeaders[Idx].VirtualAddress == (DWORD)RelocDir)
	 {
	    DPRINT("Name %.8s PESectionHeader[Idx].PointerToRawData %x\n",
		   PESectionHeaders[Idx].Name,
		   PESectionHeaders[Idx].PointerToRawData);
	    RelocDir = PESectionHeaders[Idx].PointerToRawData +
	      ModuleLoadBase;
            CurrentSize = PESectionHeaders[Idx].Misc.VirtualSize;
	    break;
	 }
    }
#else
   RelocDir = RelocDir + (ULONG)DriverBase;
   CurrentSize = PEOptionalHeader->DataDirectory
		  [IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
#endif
  DPRINT("RelocDir %08lx CurrentSize %08lx\n", RelocDir, CurrentSize);
  TotalRelocs = 0;
  while (TotalRelocs < CurrentSize && RelocDir->SizeOfBlock != 0)
    {
      NumRelocs = (RelocDir->SizeOfBlock - sizeof(RELOCATION_DIRECTORY)) / 
        sizeof(WORD);
/*      DPRINT("RelocDir at %08lx for VA %08lx with %08lx relocs\n",
             RelocDir, 
             RelocDir->VirtualAddress,
             NumRelocs);*/
      RelocEntry = (PRELOCATION_ENTRY) ((DWORD)RelocDir + 
        sizeof(RELOCATION_DIRECTORY));
      for (Idx = 0; Idx < NumRelocs; Idx++)
        {
	   ULONG Offset;
	   ULONG Type;
	   PDWORD RelocItem;
	   
	   Offset = RelocEntry[Idx].TypeOffset & 0xfff;
	   Type = (RelocEntry[Idx].TypeOffset >> 12) & 0xf;
	   RelocItem = (PDWORD)(DriverBase + RelocDir->VirtualAddress + 
				Offset);
/*	   DPRINT("  reloc at %08lx %x %s old:%08lx new:%08lx\n", 
		  RelocItem,
		  Type,
		  Type ? "HIGHLOW" : "ABS",
		  *RelocItem,
		  (*RelocItem) + RelocDelta); */
          if (Type == 3)
            {
              (*RelocItem) += RelocDelta;
            }
          else if (Type != 0)
            {
              CPRINT("Unknown relocation type %x at %x\n",Type, &Type);
              return STATUS_UNSUCCESSFUL;
            }
        }
      TotalRelocs += RelocDir->SizeOfBlock;
      RelocDir = (PRELOCATION_DIRECTORY)((DWORD)RelocDir + 
        RelocDir->SizeOfBlock);
//      DPRINT("TotalRelocs: %08lx  CurrentSize: %08lx\n", TotalRelocs, CurrentSize);
    }
   
  DPRINT("PEOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT] %x\n",
         PEOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
         .VirtualAddress);
  /*  Perform import fixups  */
  if (PEOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress)
    {
      PIMAGE_IMPORT_MODULE_DIRECTORY ImportModuleDirectory;

      /*  Process each import module  */
      ImportModuleDirectory = (PIMAGE_IMPORT_MODULE_DIRECTORY)
        ((DWORD)DriverBase + PEOptionalHeader->
          DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
      DPRINT("Processeing import directory at %p\n", ImportModuleDirectory);
      while (ImportModuleDirectory->dwRVAModuleName)
        {
          /*  Check to make sure that import lib is kernel  */
          pName = (PCHAR) DriverBase + 
            ImportModuleDirectory->dwRVAModuleName;

          RtlCreateUnicodeStringFromAsciiz(&ModuleName, pName);
          DPRINT("Import module: %wZ\n", &ModuleName);

          LibraryModuleObject = LdrGetModuleObject(&ModuleName);
          if (LibraryModuleObject == NULL)
            {
              CPRINT("Module '%wZ' not loaded yet\n", &ModuleName);
              wcscpy(NameBuffer, L"\\SystemRoot\\system32\\drivers\\");
              wcscat(NameBuffer, ModuleName.Buffer);
              RtlInitUnicodeString(&NameString, NameBuffer);
              Status = LdrLoadModule(&NameString, &LibraryModuleObject);
              if (!NT_SUCCESS(Status))
                {
                  CPRINT("Unknown import module: %wZ (Status %lx)\n", &ModuleName, Status);
                  return(Status);
                }
            }
          /*  Get the import address list  */
          ImportAddressList = (PVOID *) ((DWORD)DriverBase + 
            ImportModuleDirectory->dwRVAFunctionAddressList);

          /*  Get the list of functions to import  */
          if (ImportModuleDirectory->dwRVAFunctionNameList != 0)
            {
              FunctionNameList = (PULONG) ((DWORD)DriverBase + 
                ImportModuleDirectory->dwRVAFunctionNameList);
            }
          else
            {
              FunctionNameList = (PULONG) ((DWORD)DriverBase + 
                ImportModuleDirectory->dwRVAFunctionAddressList);
            }
          /*  Walk through function list and fixup addresses  */
          while (*FunctionNameList != 0L)
            {
              if ((*FunctionNameList) & 0x80000000) // hint
                {
                  pName = NULL;


                  Hint = (*FunctionNameList) & 0xffff;
                }
              else // hint-name
                {
                  pName = (PCHAR)((DWORD)DriverBase + 
                                  *FunctionNameList + 2);
                  Hint = *(PWORD)((DWORD)DriverBase + *FunctionNameList);
                }
              DPRINT("  Hint:%04x  Name:%s\n", Hint, pName);

              /*  Fixup the current import symbol  */
              if (LibraryModuleObject != NULL)
                {
                  *ImportAddressList = LdrGetExportAddress(LibraryModuleObject, 
                                                           pName, 
                                                           Hint);
                }
              else
                {
                  CPRINT("Unresolved kernel symbol: %s\n", pName);
                  return STATUS_UNSUCCESSFUL;
                }
              ImportAddressList++;
              FunctionNameList++;
            }

          RtlFreeUnicodeString(&ModuleName);

          ImportModuleDirectory++;
        }
    }

  /* Create the module */
  CreatedModuleObject = ExAllocatePool(NonPagedPool, sizeof(MODULE_OBJECT));
  if (CreatedModuleObject == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  RtlZeroMemory(CreatedModuleObject, sizeof(MODULE_OBJECT));

   /*  Initialize ModuleObject data  */
   CreatedModuleObject->Base = DriverBase;
   CreatedModuleObject->Flags = MODULE_FLAG_PE;

   RtlCreateUnicodeString(&CreatedModuleObject->FullName,
			  FileName->Buffer);
   LdrpBuildModuleBaseName(&CreatedModuleObject->BaseName,
			   &CreatedModuleObject->FullName);

  CreatedModuleObject->EntryPoint = (PVOID)((DWORD)DriverBase + 
    PEOptionalHeader->AddressOfEntryPoint);
  CreatedModuleObject->Length = DriverSize;
  DPRINT("EntryPoint at %x\n", CreatedModuleObject->EntryPoint);

  CreatedModuleObject->Image.PE.FileHeader =
    (PIMAGE_FILE_HEADER) ((unsigned int) DriverBase + PEDosHeader->e_lfanew + sizeof(ULONG));

  DPRINT("FileHeader at %x\n", CreatedModuleObject->Image.PE.FileHeader);
  CreatedModuleObject->Image.PE.OptionalHeader = 
    (PIMAGE_OPTIONAL_HEADER) ((unsigned int) DriverBase + PEDosHeader->e_lfanew + sizeof(ULONG) +
    sizeof(IMAGE_FILE_HEADER));
  DPRINT("OptionalHeader at %x\n", CreatedModuleObject->Image.PE.OptionalHeader);
  CreatedModuleObject->Image.PE.SectionList = 
    (PIMAGE_SECTION_HEADER) ((unsigned int) DriverBase + PEDosHeader->e_lfanew + sizeof(ULONG) +
    sizeof(IMAGE_FILE_HEADER) + sizeof(IMAGE_OPTIONAL_HEADER));
  DPRINT("SectionList at %x\n", CreatedModuleObject->Image.PE.SectionList);

  /* Insert module */
  KeAcquireSpinLock(&ModuleListLock, &Irql);
  InsertTailList(&ModuleListHead,
		 &CreatedModuleObject->ListEntry);
  KeReleaseSpinLock(&ModuleListLock, Irql);


  ModuleTextSection = ExAllocatePool(NonPagedPool, 
				     sizeof(MODULE_TEXT_SECTION));
  assert(ModuleTextSection);
  RtlZeroMemory(ModuleTextSection, sizeof(MODULE_TEXT_SECTION));
  ModuleTextSection->Base = (ULONG)DriverBase;
  ModuleTextSection->Length = DriverSize;
#ifdef DBG
  RtlZeroMemory(&ModuleTextSection->SymbolInfo, sizeof(ModuleTextSection->SymbolInfo));
  ModuleTextSection->SymbolInfo.ImageBase = PEOptionalHeader->ImageBase;
  ModuleTextSection->SymbolInfo.ImageSize = ModuleTextSection->Length;
#endif /* DBG */
  ModuleTextSection->Name = 
    ExAllocatePool(NonPagedPool, 
		   (wcslen(NameBuffer) + 1) * sizeof(WCHAR));
  wcscpy(ModuleTextSection->Name, NameBuffer);
  InsertTailList(&ModuleTextListHead, &ModuleTextSection->ListEntry);

  CreatedModuleObject->TextSection = ModuleTextSection;

  *ModuleObject = CreatedModuleObject;

  DPRINT("Loading Module %wZ...\n", FileName);

  if ((KdDebuggerEnabled == TRUE) && (KdDebugState & KD_DEBUG_GDB))
    {
      DPRINT("Module %wZ loaded at 0x%.08x.\n",
	      FileName, CreatedModuleObject->Base);
    }

  return STATUS_SUCCESS;
}


PVOID
LdrSafePEProcessModule(PVOID ModuleLoadBase,
		       PVOID DriverBase,
		       PVOID ImportModuleBase,
		       PULONG DriverSize)
{
  unsigned int Idx;
  ULONG RelocDelta, NumRelocs;
  ULONG CurrentSize, TotalRelocs;
  PULONG PEMagic;
  PIMAGE_DOS_HEADER PEDosHeader;
  PIMAGE_FILE_HEADER PEFileHeader;
  PIMAGE_OPTIONAL_HEADER PEOptionalHeader;
  PIMAGE_SECTION_HEADER PESectionHeaders;
  PRELOCATION_DIRECTORY RelocDir;
  PRELOCATION_ENTRY RelocEntry;
  PVOID *ImportAddressList;
  PULONG FunctionNameList;
  PCHAR pName;
  USHORT Hint;

  ps("Processing PE Module at module base:%08lx\n", ModuleLoadBase);

  /*  Get header pointers  */
  PEDosHeader = (PIMAGE_DOS_HEADER) ModuleLoadBase;
  PEMagic = (PULONG) ((unsigned int) ModuleLoadBase + 
    PEDosHeader->e_lfanew);
  PEFileHeader = (PIMAGE_FILE_HEADER) ((unsigned int) ModuleLoadBase + 
    PEDosHeader->e_lfanew + sizeof(ULONG));
  PEOptionalHeader = (PIMAGE_OPTIONAL_HEADER) ((unsigned int) ModuleLoadBase + 
    PEDosHeader->e_lfanew + sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER));
  PESectionHeaders = (PIMAGE_SECTION_HEADER) ((unsigned int) ModuleLoadBase + 
    PEDosHeader->e_lfanew + sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER) +
    sizeof(IMAGE_OPTIONAL_HEADER));
  CHECKPOINT;

  /*  Check file magic numbers  */
  if (PEDosHeader->e_magic != IMAGE_DOS_MAGIC)
    {
      return 0;
    }
  if (PEDosHeader->e_lfanew == 0)
    {
      return 0;
    }
  if (*PEMagic != IMAGE_PE_MAGIC)
    {
      return 0;
    }
  if (PEFileHeader->Machine != IMAGE_FILE_MACHINE_I386)
    {
      return 0;
    }

  ps("OptionalHdrMagic:%04x LinkVersion:%d.%d\n", 
         PEOptionalHeader->Magic,
         PEOptionalHeader->MajorLinkerVersion,
         PEOptionalHeader->MinorLinkerVersion);
  ps("Entry Point:%08lx\n", PEOptionalHeader->AddressOfEntryPoint);

  /*  Determine the size of the module  */
  *DriverSize = PEOptionalHeader->SizeOfImage;
  ps("DriverSize %x\n",*DriverSize);

  /*  Copy headers over */
  if (DriverBase != ModuleLoadBase)
    {
      memcpy(DriverBase, ModuleLoadBase, PEOptionalHeader->SizeOfHeaders);
    }

  ps("Hdr: 0x%X\n", (ULONG)PEOptionalHeader);
  ps("Hdr->SizeOfHeaders: 0x%X\n", (ULONG)PEOptionalHeader->SizeOfHeaders);
  ps("FileHdr->NumberOfSections: 0x%X\n", (ULONG)PEFileHeader->NumberOfSections);

  /* Ntoskrnl.exe need no relocation fixups since it is linked to run at the same
     address as it is mapped */
  if (DriverBase != ModuleLoadBase)
    {
      CurrentSize = 0;

  /*  Copy image sections into virtual section  */
  for (Idx = 0; Idx < PEFileHeader->NumberOfSections; Idx++)
    {
      //  Copy current section into current offset of virtual section
      if (PESectionHeaders[Idx].Characteristics & 
          (IMAGE_SECTION_CHAR_CODE | IMAGE_SECTION_CHAR_DATA))
	{
	  //ps("PESectionHeaders[Idx].VirtualAddress (%X) + DriverBase %x\n",
	  //PESectionHeaders[Idx].VirtualAddress, PESectionHeaders[Idx].VirtualAddress + DriverBase);
	  memcpy(PESectionHeaders[Idx].VirtualAddress + DriverBase,
		 (PVOID)(ModuleLoadBase + PESectionHeaders[Idx].PointerToRawData),
		 PESectionHeaders[Idx].Misc.VirtualSize > PESectionHeaders[Idx].SizeOfRawData ?
		   PESectionHeaders[Idx].SizeOfRawData : PESectionHeaders[Idx].Misc.VirtualSize );
	}
      else
	{
	  ps("PESectionHeaders[Idx].VirtualAddress (%X) + DriverBase %x\n",
	     PESectionHeaders[Idx].VirtualAddress, PESectionHeaders[Idx].VirtualAddress + DriverBase);
	  memset(PESectionHeaders[Idx].VirtualAddress + DriverBase, 
		 '\0',
		 PESectionHeaders[Idx].Misc.VirtualSize);
	}
      CurrentSize += ROUND_UP(PESectionHeaders[Idx].Misc.VirtualSize,
                              PEOptionalHeader->SectionAlignment);
    }

  /*  Perform relocation fixups  */
  RelocDelta = (ULONG) DriverBase - PEOptionalHeader->ImageBase;
  RelocDir = (PRELOCATION_DIRECTORY)(PEOptionalHeader->DataDirectory[
    IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
  ps("DrvrBase:%08lx ImgBase:%08lx RelocDelta:%08lx\n", 
         DriverBase,
         PEOptionalHeader->ImageBase,
         RelocDelta);   
  ps("RelocDir %x\n",RelocDir);

  for (Idx = 0; Idx < PEFileHeader->NumberOfSections; Idx++)
    {
      if (PESectionHeaders[Idx].VirtualAddress == (ULONG)RelocDir)
	{
	  DPRINT("Name %.8s PESectionHeader[Idx].PointerToRawData %x\n",
		 PESectionHeaders[Idx].Name,
		 PESectionHeaders[Idx].PointerToRawData);
	  RelocDir = PESectionHeaders[Idx].PointerToRawData + ModuleLoadBase;
	  CurrentSize = PESectionHeaders[Idx].Misc.VirtualSize;
	  break;
	}
    }

  ps("RelocDir %08lx CurrentSize %08lx\n", RelocDir, CurrentSize);

  TotalRelocs = 0;
  while (TotalRelocs < CurrentSize && RelocDir->SizeOfBlock != 0)
    {
      NumRelocs = (RelocDir->SizeOfBlock - sizeof(RELOCATION_DIRECTORY)) / 
        sizeof(USHORT);
      RelocEntry = (PRELOCATION_ENTRY)((ULONG)RelocDir + 
        sizeof(RELOCATION_DIRECTORY));
      for (Idx = 0; Idx < NumRelocs; Idx++)
        {
	  ULONG Offset;
	  ULONG Type;
	  PDWORD RelocItem;

	  Offset = RelocEntry[Idx].TypeOffset & 0xfff;
	  Type = (RelocEntry[Idx].TypeOffset >> 12) & 0xf;
	  RelocItem = (PULONG)(DriverBase + RelocDir->VirtualAddress + Offset);
	  if (Type == 3)
	    {
	      (*RelocItem) += RelocDelta;
	    }
	  else if (Type != 0)
	    {
	      CPRINT("Unknown relocation type %x at %x\n",Type, &Type);
	      return(0);
	    }
	}
      TotalRelocs += RelocDir->SizeOfBlock;
      RelocDir = (PRELOCATION_DIRECTORY)((ULONG)RelocDir + 
        RelocDir->SizeOfBlock);
    }

    ps("PEOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT] %x\n",
         PEOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
         .VirtualAddress);
  }

  /*  Perform import fixups  */
  if (PEOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress)
    {
      PIMAGE_IMPORT_MODULE_DIRECTORY ImportModuleDirectory;

      /*  Process each import module  */
      ImportModuleDirectory = (PIMAGE_IMPORT_MODULE_DIRECTORY)
        ((ULONG)DriverBase + PEOptionalHeader->
          DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

      ps("Processeing import directory at %p\n", ImportModuleDirectory);

      /*  Check to make sure that import lib is kernel  */
      pName = (PCHAR)DriverBase + ImportModuleDirectory->dwRVAModuleName;

      ps("Import module: %s\n", pName);

      /*  Get the import address list  */
      ImportAddressList = (PVOID *)((ULONG)DriverBase + 
	ImportModuleDirectory->dwRVAFunctionAddressList);

      ps("  ImportModuleDirectory->dwRVAFunctionAddressList: 0x%X\n",
	 ImportModuleDirectory->dwRVAFunctionAddressList);
      ps("  ImportAddressList: 0x%X\n", ImportAddressList);

      /*  Get the list of functions to import  */
      if (ImportModuleDirectory->dwRVAFunctionNameList != 0)
	{
	  ps("Using function name list.\n");

	  FunctionNameList = (PULONG)((ULONG)DriverBase + 
	    ImportModuleDirectory->dwRVAFunctionNameList);
	}
      else
	{
	  ps("Using function address list.\n");

	  FunctionNameList = (PULONG)((ULONG)DriverBase + 
	    ImportModuleDirectory->dwRVAFunctionAddressList);
	}

      /* Walk through function list and fixup addresses */
      while (*FunctionNameList != 0L)
	{
	  if ((*FunctionNameList) & 0x80000000)
	    {
	       /* Hint */
	      pName = NULL;
	      Hint = (*FunctionNameList) & 0xffff;
	    }
	  else
	    {
	      /* Hint name */
	      pName = (PCHAR)((ULONG)DriverBase + *FunctionNameList + 2);
	      Hint = *(PWORD)((ULONG)DriverBase + *FunctionNameList);
	    }
	  //ps("  Hint:%04x  Name:%s(0x%X)(%x)\n", Hint, pName, pName, ImportAddressList);

	  *ImportAddressList = LdrSafePEGetExportAddress(ImportModuleBase,
							 pName,
							 Hint);

	  ImportAddressList++;
	  FunctionNameList++;
	}
    }

  ps("Finished importing.\n");

  return(0);
}


static PVOID
LdrPEGetExportAddress(PMODULE_OBJECT ModuleObject,
		      PCHAR Name,
		      USHORT Hint)
{
  PIMAGE_EXPORT_DIRECTORY ExportDir;
  ULONG ExportDirSize;
  USHORT Idx;
  PVOID  ExportAddress;
  PWORD  OrdinalList;
  PDWORD FunctionList, NameList;

   ExportDir = (PIMAGE_EXPORT_DIRECTORY)
     RtlImageDirectoryEntryToData(ModuleObject->Base,
				  TRUE,
				  IMAGE_DIRECTORY_ENTRY_EXPORT,
				  &ExportDirSize);
   DPRINT("ExportDir %p ExportDirSize %lx\n", ExportDir, ExportDirSize);
   if (ExportDir == NULL)
     {
	return NULL;
     }

   FunctionList = (PDWORD)((DWORD)ExportDir->AddressOfFunctions + ModuleObject->Base);
   NameList = (PDWORD)((DWORD)ExportDir->AddressOfNames + ModuleObject->Base);
   OrdinalList = (PWORD)((DWORD)ExportDir->AddressOfNameOrdinals + ModuleObject->Base);

  ExportAddress = 0;

  if (Name != NULL)
    {
      for (Idx = 0; Idx < ExportDir->NumberOfNames; Idx++)
        {
#if 0
          DPRINT("  Name:%s  NameList[%d]:%s\n", 
                 Name, 
                 Idx, 
                 (DWORD) ModuleObject->Base + NameList[Idx]);

#endif
          if (!strcmp(Name, (PCHAR) ((DWORD)ModuleObject->Base + NameList[Idx])))
            {
              ExportAddress = (PVOID) ((DWORD)ModuleObject->Base +
                FunctionList[OrdinalList[Idx]]);
		  if (((ULONG)ExportAddress >= (ULONG)ExportDir) &&
		      ((ULONG)ExportAddress < (ULONG)ExportDir + ExportDirSize))
		    {
		       DPRINT("Forward: %s\n", (PCHAR)ExportAddress);
		       ExportAddress = LdrPEFixupForward((PCHAR)ExportAddress);
		       DPRINT("ExportAddress: %p\n", ExportAddress);
		    }

              break;
            }
        }
    }
  else  /*  use hint  */
    {
      ExportAddress = (PVOID) ((DWORD)ModuleObject->Base +
        FunctionList[Hint - ExportDir->Base]);
    }

  if (ExportAddress == NULL)
    {
      CPRINT("Export not found for %d:%s\n",
	     Hint,
	     Name != NULL ? Name : "(Ordinal)");
      KeBugCheck(0);
    }

  return(ExportAddress);
}


static PVOID
LdrSafePEGetExportAddress(PVOID ImportModuleBase,
			  PCHAR Name,
			  USHORT Hint)
{
  USHORT Idx;
  PVOID  ExportAddress;
  PWORD  OrdinalList;
  PDWORD FunctionList, NameList;
  PIMAGE_EXPORT_DIRECTORY  ExportDir;
  ULONG ExportDirSize;

  static BOOLEAN EP = FALSE;

  ExportDir = (PIMAGE_EXPORT_DIRECTORY)
    RtlImageDirectoryEntryToData(ImportModuleBase,
	  TRUE,
		IMAGE_DIRECTORY_ENTRY_EXPORT,
		&ExportDirSize);

  if (!EP) {
    EP = TRUE;
    ps("ExportDir %x\n", ExportDir);
  }

  FunctionList = (PDWORD)((DWORD)ExportDir->AddressOfFunctions + ImportModuleBase);
  NameList = (PDWORD)((DWORD)ExportDir->AddressOfNames + ImportModuleBase);
  OrdinalList = (PWORD)((DWORD)ExportDir->AddressOfNameOrdinals + ImportModuleBase);

  ExportAddress = 0;

  if (Name != NULL)
    {
      for (Idx = 0; Idx < ExportDir->NumberOfNames; Idx++)
        {
          if (!strcmp(Name, (PCHAR) ((DWORD)ImportModuleBase + NameList[Idx])))
      			{
              ExportAddress = (PVOID) ((DWORD)ImportModuleBase +
                FunctionList[OrdinalList[Idx]]);
              break;
            }
        }
    }
  else  /*  use hint  */
    {
      ExportAddress = (PVOID) ((DWORD)ImportModuleBase +

        FunctionList[Hint - ExportDir->Base]);
    }

  if (ExportAddress == 0)
    {
      ps("Export not found for %d:%s\n",
	 Hint,
	 Name != NULL ? Name : "(Ordinal)");
      KeBugCheck(0);
    }
  return ExportAddress;
}


static PVOID
LdrPEFixupForward(PCHAR ForwardName)
{
   CHAR NameBuffer[128];
   UNICODE_STRING ModuleName;
   PCHAR p;
   PMODULE_OBJECT ModuleObject;

   DPRINT("LdrPEFixupForward (%s)\n", ForwardName);

   strcpy(NameBuffer, ForwardName);
   p = strchr(NameBuffer, '.');
   if (p == NULL)
     {
	return NULL;
     }

   *p = 0;

   DPRINT("Driver: %s  Function: %s\n", NameBuffer, p+1);

   RtlCreateUnicodeStringFromAsciiz(&ModuleName,
				    NameBuffer);
   ModuleObject = LdrGetModuleObject(&ModuleName);
   RtlFreeUnicodeString(&ModuleName);

   DPRINT("ModuleObject: %p\n", ModuleObject);

   if (ModuleObject == NULL)
     {
	CPRINT("LdrPEFixupForward: failed to find module %s\n", NameBuffer);
	return NULL;
     }

  return(LdrPEGetExportAddress(ModuleObject, p+1, 0));
}

/* EOF */
