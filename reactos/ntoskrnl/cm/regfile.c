/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             ntoskrnl/cm/regfile.c
 * PURPOSE:          Registry file manipulation routines
 * UPDATE HISTORY:
*/

#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <roscfg.h>
#include <internal/ob.h>
#include <limits.h>
#include <string.h>
#include <internal/pool.h>
#include <internal/registry.h>
#include <reactos/bugcodes.h>

#define NDEBUG
#include <internal/debug.h>

#include "cm.h"


/* uncomment to enable hive checks (incomplete and probably buggy) */
// #define HIVE_CHECK

/* LOCAL MACROS *************************************************************/

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define ROUND_DOWN(N, S) ((N) - ((N) % (S)))

#define ABS_VALUE(V) (((V) < 0) ? -(V) : (V))

BOOLEAN CmiDoVerify = FALSE;

static ULONG
CmiCalcChecksum(PULONG Buffer);

/* FUNCTIONS ****************************************************************/

VOID
CmiCreateDefaultHiveHeader(PHIVE_HEADER Header)
{
  assert(Header);
  RtlZeroMemory(Header, sizeof(HIVE_HEADER));
  Header->BlockId = REG_HIVE_ID;
  Header->UpdateCounter1 = 0;
  Header->UpdateCounter2 = 0;
  Header->DateModified.dwLowDateTime = 0;
  Header->DateModified.dwHighDateTime = 0;
  Header->Unused3 = 1;
  Header->Unused4 = 3;
  Header->Unused5 = 0;
  Header->Unused6 = 1;
  Header->Unused7 = 1;
  Header->RootKeyCell = 0;
  Header->BlockSize = REG_BLOCK_SIZE;
  Header->Unused6 = 1;
  Header->Checksum = 0;
}


VOID
CmiCreateDefaultBinCell(PHBIN BinCell)
{
  assert(BinCell);
  RtlZeroMemory(BinCell, sizeof(HBIN));
  BinCell->BlockId = REG_BIN_ID;
  BinCell->DateModified.dwLowDateTime = 0;
  BinCell->DateModified.dwHighDateTime = 0;
  BinCell->BlockSize = REG_BLOCK_SIZE;
}


VOID
CmiCreateDefaultRootKeyCell(PKEY_CELL RootKeyCell)
{
  assert(RootKeyCell);
  RtlZeroMemory(RootKeyCell, sizeof(KEY_CELL));
  RootKeyCell->CellSize = -sizeof(KEY_CELL);
  RootKeyCell->Id = REG_KEY_CELL_ID;
  RootKeyCell->Flags = REG_KEY_ROOT_CELL | REG_KEY_NAME_PACKED;
  NtQuerySystemTime((PTIME) &RootKeyCell->LastWriteTime);
  RootKeyCell->ParentKeyOffset = 0;
  RootKeyCell->NumberOfSubKeys = 0;
  RootKeyCell->HashTableOffset = -1;
  RootKeyCell->NumberOfValues = 0;
  RootKeyCell->ValuesOffset = -1;
  RootKeyCell->SecurityKeyOffset = 0;
  RootKeyCell->ClassNameOffset = -1;
  RootKeyCell->NameSize = 0;
  RootKeyCell->ClassSize = 0;
}


VOID
CmiVerifyBinCell(PHBIN BinCell)
{
  if (CmiDoVerify)
    {

  assert(BinCell);

  if (BinCell->BlockId != REG_BIN_ID)
    {
      DbgPrint("BlockId is %.08x (should be %.08x)\n",
        BinCell->BlockId, REG_BIN_ID);
      assert(BinCell->BlockId == REG_BIN_ID);
    }

  //BinCell->DateModified.dwLowDateTime

  //BinCell->DateModified.dwHighDateTime

  
  if (BinCell->BlockSize != REG_BLOCK_SIZE)
    {
      DbgPrint("BlockSize is %.08x (should be %.08x)\n",
        BinCell->BlockSize, REG_BLOCK_SIZE);
      assert(BinCell->BlockSize == REG_BLOCK_SIZE);
    }

    }
}


VOID
CmiVerifyKeyCell(PKEY_CELL KeyCell)
{
  if (CmiDoVerify)
    {

  assert(KeyCell);

  if (KeyCell->CellSize == 0)
    {
      DbgPrint("CellSize is %d (must not be 0)\n",
        KeyCell->CellSize);
      assert(KeyCell->CellSize != 0);
    }

  if (KeyCell->Id != REG_KEY_CELL_ID)
    {
      DbgPrint("Id is %.08x (should be %.08x)\n",
        KeyCell->Id, REG_KEY_CELL_ID);
      assert(KeyCell->Id == REG_KEY_CELL_ID);
    }

  //KeyCell->Flags;

  //KeyCell->LastWriteTime;

  if (KeyCell->ParentKeyOffset < 0)
    {
      DbgPrint("ParentKeyOffset is %d (must not be < 0)\n",
        KeyCell->ParentKeyOffset);
      assert(KeyCell->ParentKeyOffset >= 0);
    }

  if (KeyCell->NumberOfSubKeys < 0)
    {
      DbgPrint("NumberOfSubKeys is %d (must not be < 0)\n",
        KeyCell->NumberOfSubKeys);
      assert(KeyCell->NumberOfSubKeys >= 0);
    }

  //KeyCell->HashTableOffset;

  if (KeyCell->NumberOfValues < 0)
    {
      DbgPrint("NumberOfValues is %d (must not be < 0)\n",
        KeyCell->NumberOfValues);
      assert(KeyCell->NumberOfValues >= 0);
    }

  //KeyCell->ValuesOffset = -1;

  if (KeyCell->SecurityKeyOffset < 0)
    {
      DbgPrint("SecurityKeyOffset is %d (must not be < 0)\n",
        KeyCell->SecurityKeyOffset);
      assert(KeyCell->SecurityKeyOffset >= 0);
    }

  //KeyCell->ClassNameOffset = -1;

  //KeyCell->NameSize

  //KeyCell->ClassSize

    }
}


VOID
CmiVerifyRootKeyCell(PKEY_CELL RootKeyCell)
{
  if (CmiDoVerify)
    {

  CmiVerifyKeyCell(RootKeyCell);

  if (!(RootKeyCell->Flags & REG_KEY_ROOT_CELL))
    {
      DbgPrint("Type is %.08x (should be %.08x)\n",
        RootKeyCell->Flags, REG_KEY_ROOT_CELL | REG_KEY_NAME_PACKED);
      assert(!(RootKeyCell->Type & REG_KEY_ROOT_CELL));
    }

    }
}


VOID
CmiVerifyValueCell(PVALUE_CELL ValueCell)
{
  if (CmiDoVerify)
    {

  assert(ValueCell);

  if (ValueCell->CellSize == 0)
    {
      DbgPrint("CellSize is %d (must not be 0)\n",
        ValueCell->CellSize);
      assert(ValueCell->CellSize != 0);
    }

  if (ValueCell->Id != REG_VALUE_CELL_ID)
    {
      DbgPrint("Id is %.08x (should be %.08x)\n",
        ValueCell->Id, REG_VALUE_CELL_ID);
      assert(ValueCell->Id == REG_VALUE_CELL_ID);
    }

  //ValueCell->NameSize;
  //ValueCell->LONG  DataSize;
  //ValueCell->DataOffset;
  //ValueCell->ULONG  DataType;
  //ValueCell->USHORT Flags;
  //ValueCell->USHORT Unused1;
  //ValueCell->UCHAR  Name[0];
    }
}


VOID
CmiVerifyValueListCell(PVALUE_LIST_CELL ValueListCell)
{
  if (CmiDoVerify)
    {

  if (ValueListCell->CellSize == 0)
    {
      DbgPrint("CellSize is %d (must not be 0)\n",
        ValueListCell->CellSize);
      assert(ValueListCell->CellSize != 0);
    }

    }
}


VOID
CmiVerifyKeyObject(PKEY_OBJECT KeyObject)
{
  if (CmiDoVerify)
    {

  if (KeyObject->RegistryHive == NULL)
    {
      DbgPrint("RegistryHive is NULL (must not be NULL)\n",
        KeyObject->RegistryHive);
      assert(KeyObject->RegistryHive != NULL);
    }

  if (KeyObject->KeyCell == NULL)
    {
      DbgPrint("KeyCell is NULL (must not be NULL)\n",
        KeyObject->KeyCell);
      assert(KeyObject->KeyCell != NULL);
    }

  if (KeyObject->ParentKey == NULL)
    {
      DbgPrint("ParentKey is NULL (must not be NULL)\n",
        KeyObject->ParentKey);
      assert(KeyObject->ParentKey != NULL);
    }

    }
}


VOID
CmiVerifyHiveHeader(PHIVE_HEADER Header)
{
  if (CmiDoVerify)
    {

  if (Header->BlockId != REG_HIVE_ID)
    {
      DbgPrint("BlockId is %.08x (must be %.08x)\n",
        Header->BlockId,
        REG_HIVE_ID);
      assert(Header->BlockId == REG_HIVE_ID);
    }

  if (Header->Unused3 != 1)
    {
      DbgPrint("Unused3 is %.08x (must be 1)\n",
        Header->Unused3);
      assert(Header->Unused3 == 1);
    }

  if (Header->Unused4 != 3)
    {
      DbgPrint("Unused4 is %.08x (must be 3)\n",
        Header->Unused4);
      assert(Header->Unused4 == 3);
    }

  if (Header->Unused5 != 0)
    {
      DbgPrint("Unused5 is %.08x (must be 0)\n",
        Header->Unused5);
      assert(Header->Unused5 == 0);
    }

  if (Header->Unused6 != 1)
    {
      DbgPrint("Unused6 is %.08x (must be 1)\n",
        Header->Unused6);
      assert(Header->Unused6 == 1);
    }

  if (Header->Unused7 != 1)
    {
      DbgPrint("Unused7 is %.08x (must be 1)\n",
        Header->Unused7);
      assert(Header->Unused7 == 1);
    }

    }
}


VOID
CmiVerifyRegistryHive(PREGISTRY_HIVE RegistryHive)
{
  if (CmiDoVerify)
    {

      CmiVerifyHiveHeader(RegistryHive->HiveHeader);

    }
}


static NTSTATUS
CmiCreateNewRegFile(HANDLE FileHandle)
{
  IO_STATUS_BLOCK IoStatusBlock;
  PCELL_HEADER FreeCell;
  PHIVE_HEADER HiveHeader;
  PKEY_CELL RootKeyCell;
  NTSTATUS Status;
  PHBIN BinCell;
  PCHAR Buffer;

  Buffer = (PCHAR) ExAllocatePool(NonPagedPool, 2 * REG_BLOCK_SIZE);
  if (Buffer == NULL)
    return STATUS_INSUFFICIENT_RESOURCES;

  HiveHeader = (PHIVE_HEADER)Buffer;
  BinCell = (PHBIN)((ULONG_PTR)Buffer + REG_BLOCK_SIZE);
  RootKeyCell = (PKEY_CELL)((ULONG_PTR)Buffer + REG_BLOCK_SIZE + REG_HBIN_DATA_OFFSET);
  FreeCell = (PCELL_HEADER)((ULONG_PTR)Buffer + REG_BLOCK_SIZE + REG_HBIN_DATA_OFFSET + sizeof(KEY_CELL));

  CmiCreateDefaultHiveHeader(HiveHeader);
  CmiCreateDefaultBinCell(BinCell);
  CmiCreateDefaultRootKeyCell(RootKeyCell);

  /* First block */
  BinCell->BlockOffset = 0;

  /* Offset to root key block */
  HiveHeader->RootKeyCell = REG_HBIN_DATA_OFFSET;

  /* The rest of the block is free */
  FreeCell->CellSize = REG_BLOCK_SIZE - (REG_HBIN_DATA_OFFSET + sizeof(KEY_CELL));

  Status = NtWriteFile(FileHandle,
		       NULL,
		       NULL,
		       NULL,
		       &IoStatusBlock,
		       Buffer,
		       2 * REG_BLOCK_SIZE,
		       0,
		       NULL);

  ExFreePool(Buffer);

  assertmsg(NT_SUCCESS(Status), ("Status: 0x%X\n", Status));

  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  Status = NtFlushBuffersFile(FileHandle,
			      &IoStatusBlock);

  return(Status);
}


#ifdef HIVE_CHECK
static NTSTATUS
CmiCheckAndFixHive(PREGISTRY_HIVE RegistryHive)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  FILE_STANDARD_INFORMATION fsi;
  IO_STATUS_BLOCK IoStatusBlock;
  HANDLE HiveHandle = INVALID_HANDLE_VALUE;
  HANDLE LogHandle = INVALID_HANDLE_VALUE;
  PHIVE_HEADER HiveHeader = NULL;
  PHIVE_HEADER LogHeader = NULL;
  LARGE_INTEGER FileOffset;
  ULONG FileSize;
  ULONG BufferSize;
  ULONG BitmapSize;
  RTL_BITMAP BlockBitMap;
  NTSTATUS Status;

  DPRINT("CmiCheckAndFixHive() called\n");

  /* Try to open the hive file */
  InitializeObjectAttributes(&ObjectAttributes,
			     &RegistryHive->HiveFileName,
			     0,
			     NULL,
			     NULL);

  Status = NtCreateFile(&HiveHandle,
			FILE_READ_DATA | FILE_READ_ATTRIBUTES,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			FILE_OPEN,
			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0);
  if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
      return(STATUS_SUCCESS);
    }
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtCreateFile() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Try to open the log file */
  InitializeObjectAttributes(&ObjectAttributes,
			     &RegistryHive->LogFileName,
			     0,
			     NULL,
			     NULL);

  Status = NtCreateFile(&LogHandle,
			FILE_READ_DATA | FILE_READ_ATTRIBUTES,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			FILE_OPEN,
			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0);
  if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
      LogHandle = INVALID_HANDLE_VALUE;
    }
  else if (!NT_SUCCESS(Status))
    {
      DPRINT("NtCreateFile() failed (Status %lx)\n", Status);
      NtClose(HiveHandle);
      return(Status);
    }

  /* Allocate hive header */
  HiveHeader = ExAllocatePool(PagedPool,
			      sizeof(HIVE_HEADER));
  if (HiveHeader == NULL)
    {
      DPRINT("ExAllocatePool() failed\n");
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto ByeBye;
    }

  /* Read hive base block */
  FileOffset.QuadPart = 0ULL;
  Status = NtReadFile(HiveHandle,
		      0,
		      0,
		      0,
		      &IoStatusBlock,
		      HiveHeader,
		      sizeof(HIVE_HEADER),
		      &FileOffset,
		      0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtReadFile() failed (Status %lx)\n", Status);
      goto ByeBye;
    }

  if (LogHandle == INVALID_HANDLE_VALUE)
    {
      if (HiveHeader->Checksum != CmiCalcChecksum((PULONG)HiveHeader) ||
	  HiveHeader->UpdateCounter1 != HiveHeader->UpdateCounter2)
	{
	  /* There is no way to fix the hive without log file - BSOD! */
	  DPRINT("Hive header inconsistent and no log file available!\n");
	  KeBugCheck(CONFIG_LIST_FAILED);
	}

      Status = STATUS_SUCCESS;
      goto ByeBye;
    }
  else
    {
      /* Allocate hive header */
      LogHeader = ExAllocatePool(PagedPool,
				 sizeof(HIVE_HEADER));
      if (LogHeader == NULL)
	{
	  DPRINT("ExAllocatePool() failed\n");
	  Status = STATUS_INSUFFICIENT_RESOURCES;
	  goto ByeBye;
	}

      /* Read log file header */
      FileOffset.QuadPart = 0ULL;
      Status = NtReadFile(LogHandle,
			  0,
			  0,
			  0,
			  &IoStatusBlock,
			  LogHeader,
			  sizeof(HIVE_HEADER),
			  &FileOffset,
			  0);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT("NtReadFile() failed (Status %lx)\n", Status);
	  goto ByeBye;
	}

      /* Check log file header integrity */
      if (LogHeader->Checksum != CmiCalcChecksum((PULONG)LogHeader) ||
          LogHeader->UpdateCounter1 != LogHeader->UpdateCounter2)
	{
	  if (HiveHeader->Checksum != CmiCalcChecksum((PULONG)HiveHeader) ||
	      HiveHeader->UpdateCounter1 != HiveHeader->UpdateCounter2)
	    {
	      DPRINT("Hive file and log file are inconsistent!\n");
	      KeBugCheck(CONFIG_LIST_FAILED);
	    }

	  /* Log file damaged but hive is okay */
	  Status = STATUS_SUCCESS;
	  goto ByeBye;
	}

      if (HiveHeader->UpdateCounter1 == HiveHeader->UpdateCounter2 &&
	  HiveHeader->UpdateCounter1 == LogHeader->UpdateCounter1)
	{
	  /* Hive and log file are up-to-date */
	  Status = STATUS_SUCCESS;
	  goto ByeBye;
	}

      /*
       * Hive needs an update!
       */

      /* Get file size */
      Status = NtQueryInformationFile(LogHandle,
				      &IoStatusBlock,
				      &fsi,
				      sizeof(fsi),
				      FileStandardInformation);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT("NtQueryInformationFile() failed (Status %lx)\n", Status);
	  goto ByeBye;
	}
      FileSize = fsi.EndOfFile.u.LowPart;

      /* Calculate bitmap and block size */
      BitmapSize = ROUND_UP((FileSize / 4096) - 1, sizeof(ULONG) * 8) / 8;
      BufferSize = sizeof(HIVE_HEADER) +
			  sizeof(ULONG) +
			  BitmapSize;
      BufferSize = ROUND_UP(BufferSize, 4096);

      /* Reallocate log header block */
      ExFreePool(LogHeader);
      LogHeader = ExAllocatePool(PagedPool,
				 BufferSize);
      if (LogHeader == NULL)
	{
	  DPRINT("ExAllocatePool() failed\n");
	  Status = STATUS_INSUFFICIENT_RESOURCES;
	  goto ByeBye;
	}

      /* Read log file header */
      FileOffset.QuadPart = 0ULL;
      Status = NtReadFile(LogHandle,
			  0,
			  0,
			  0,
			  &IoStatusBlock,
			  LogHeader,
			  BufferSize,
			  &FileOffset,
			  0);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT("NtReadFile() failed (Status %lx)\n", Status);
	  goto ByeBye;
	}

      /* Initialize bitmap */
      RtlInitializeBitMap(&BlockBitMap,
			  (PVOID)((ULONG)LogHeader + 4096 + sizeof(ULONG)),
			  BitmapSize * 8);

      /* FIXME: Update dirty blocks */


      /* FIXME: Update hive header */


      Status = STATUS_SUCCESS;
    }


  /* Clean up the mess */
ByeBye:
  if (HiveHeader != NULL)
    ExFreePool(HiveHeader);

  if (LogHeader != NULL)
    ExFreePool(LogHeader);

  if (LogHandle != INVALID_HANDLE_VALUE)
    NtClose(LogHandle);

  NtClose(HiveHandle);

  return(Status);
}
#endif


NTSTATUS
CmiImportHiveBins(PREGISTRY_HIVE Hive,
		  PUCHAR ChunkPtr)
{
  BLOCK_OFFSET BlockOffset;
  ULONG BlockIndex;
  PHBIN Bin;
  ULONG j;

  BlockIndex = 0;
  BlockOffset = 0;
  while (BlockIndex < Hive->BlockListSize)
    {
      Bin = (PHBIN)((ULONG_PTR)ChunkPtr + BlockOffset);

      if (Bin->BlockId != REG_BIN_ID)
	{
	  DPRINT1 ("Bad BlockId %x, offset %x\n", Bin->BlockId, BlockOffset);
	  return STATUS_REGISTRY_CORRUPT;
	}

      assertmsg((Bin->BlockSize % 4096) == 0,
		("BlockSize (0x%.08x) must be multiple of 4K\n",
		Bin->BlockSize));

      /* Allocate the hive block */
      Hive->BlockList[BlockIndex] = ExAllocatePool (PagedPool,
						    Bin->BlockSize);
      if (Hive->BlockList[BlockIndex] == NULL)
	{
	  DPRINT1 ("ExAllocatePool() failed\n");
	  return STATUS_INSUFFICIENT_RESOURCES;
	}

      /* Import the Bin */
      RtlCopyMemory (Hive->BlockList[BlockIndex],
		     Bin,
		     Bin->BlockSize);

      if (Bin->BlockSize > 4096)
	{
	  for (j = 1; j < Bin->BlockSize / 4096; j++)
	    {
	      Hive->BlockList[BlockIndex + j] = Hive->BlockList[BlockIndex];
	    }
	}

      BlockIndex += Bin->BlockSize / 4096;
      BlockOffset += Bin->BlockSize;
    }

  return STATUS_SUCCESS;
}


VOID
CmiFreeHiveBins (PREGISTRY_HIVE Hive)
{
  ULONG i;
  PHBIN Bin;

  Bin = NULL;
  for (i = 0; i < Hive->BlockListSize; i++)
    {
      if (Hive->BlockList[i] == NULL)
	continue;

      if (Hive->BlockList[i] != Bin)
	{
	  Bin = Hive->BlockList[i];
	  ExFreePool (Hive->BlockList[i]);
	}
      Hive->BlockList[i] = NULL;
    }
}


NTSTATUS
CmiCreateHiveFreeCellList(PREGISTRY_HIVE Hive)
{
  BLOCK_OFFSET BlockOffset;
  PCELL_HEADER FreeBlock;
  ULONG BlockIndex;
  ULONG FreeOffset;
  PHBIN Bin;
  NTSTATUS Status;

  /* Initialize the free cell list */
  Hive->FreeListSize = 0;
  Hive->FreeListMax = 0;
  Hive->FreeList = NULL;
  Hive->FreeListOffset = NULL;

  BlockOffset = 0;
  BlockIndex = 0;
  while (BlockIndex < Hive->BlockListSize)
    {
      Bin = Hive->BlockList[BlockIndex];

      /* Search free blocks and add to list */
      FreeOffset = REG_HBIN_DATA_OFFSET;
      while (FreeOffset < Bin->BlockSize)
	{
	  FreeBlock = (PCELL_HEADER) ((ULONG_PTR) Bin + FreeOffset);
	  if (FreeBlock->CellSize > 0)
	    {
	      Status = CmiAddFree(Hive,
				  FreeBlock,
				  Bin->BlockOffset + FreeOffset,
				  FALSE);

	      if (!NT_SUCCESS(Status))
		{
		  return Status;
		}

	      FreeOffset += FreeBlock->CellSize;
	    }
	  else
	    {
	      FreeOffset -= FreeBlock->CellSize;
	    }
	}

      BlockIndex += Bin->BlockSize / 4096;
      BlockOffset += Bin->BlockSize;
    }

  return STATUS_SUCCESS;
}


VOID
CmiFreeHiveFreeCellList(PREGISTRY_HIVE Hive)
{
  ExFreePool (Hive->FreeList);
  ExFreePool (Hive->FreeListOffset);

  Hive->FreeListSize = 0;
  Hive->FreeListMax = 0;
  Hive->FreeList = NULL;
  Hive->FreeListOffset = NULL;
}


NTSTATUS
CmiCreateHiveBitmap(PREGISTRY_HIVE Hive)
{
  ULONG BitmapSize;

  /* Calculate bitmap size in bytes (always a multiple of 32 bits) */
  BitmapSize = ROUND_UP(Hive->BlockListSize, sizeof(ULONG) * 8) / 8;
  DPRINT("Hive->BlockListSize: %lu\n", Hive->BlockListSize);
  DPRINT("BitmapSize:  %lu Bytes  %lu Bits\n", BitmapSize, BitmapSize * 8);

  /* Allocate bitmap */
  Hive->BitmapBuffer = (PULONG)ExAllocatePool(PagedPool,
					      BitmapSize);
  if (Hive->BitmapBuffer == NULL)
    {
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  RtlInitializeBitMap(&Hive->DirtyBitMap,
		      Hive->BitmapBuffer,
		      BitmapSize * 8);

  /* Initialize bitmap */
  RtlClearAllBits(&Hive->DirtyBitMap);
  Hive->HiveDirty = FALSE;

  return STATUS_SUCCESS;
}


static NTSTATUS
CmiInitNonVolatileRegistryHive(PREGISTRY_HIVE RegistryHive,
			       PWSTR Filename,
			       BOOLEAN CreateNew)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  ULONG CreateDisposition;
  IO_STATUS_BLOCK IoSB;
  HANDLE FileHandle;
  HANDLE SectionHandle;
  PUCHAR ViewBase;
  ULONG ViewSize;
  NTSTATUS Status;

  DPRINT("CmiInitNonVolatileRegistryHive(%p, %S, %d) called\n",
	 RegistryHive, Filename, CreateNew);

  /* Duplicate Filename */
  Status = RtlCreateUnicodeString(&RegistryHive->HiveFileName,
				  Filename);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("RtlCreateUnicodeString() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Create log file name */
  RegistryHive->LogFileName.Length = (wcslen(Filename) + 4) * sizeof(WCHAR);
  RegistryHive->LogFileName.MaximumLength = RegistryHive->LogFileName.Length + sizeof(WCHAR);
  RegistryHive->LogFileName.Buffer = ExAllocatePool(NonPagedPool,
						    RegistryHive->LogFileName.MaximumLength);
  if (RegistryHive->LogFileName.Buffer == NULL)
    {
      RtlFreeUnicodeString(&RegistryHive->HiveFileName);
      DPRINT("ExAllocatePool() failed\n");
      return(STATUS_INSUFFICIENT_RESOURCES);
    }
  wcscpy(RegistryHive->LogFileName.Buffer,
	 Filename);
  wcscat(RegistryHive->LogFileName.Buffer,
	 L".log");

#ifdef HIVE_CHECK
  /* Check and eventually fix a hive */
  Status = CmiCheckAndFixHive(RegistryHive);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&RegistryHive->HiveFileName);
      RtlFreeUnicodeString(&RegistryHive->LogFileName);
      DPRINT1("CmiCheckAndFixHive() failed (Status %lx)\n", Status);
      return(Status);
    }
#endif

  InitializeObjectAttributes(&ObjectAttributes,
			     &RegistryHive->HiveFileName,
			     0,
			     NULL,
			     NULL);

  /*
   * Note:
   * This is a workaround to prevent a BSOD because of missing registry hives.
   * The workaround is only useful for developers. An implementation for the
   * ordinary user must bail out on missing registry hives because they are
   * essential to booting and configuring the OS.
   */
#if 0
  if (CreateNew == TRUE)
    CreateDisposition = FILE_OPEN_IF;
  else
    CreateDisposition = FILE_OPEN;
#endif
  CreateDisposition = FILE_OPEN_IF;

  Status = NtCreateFile(&FileHandle,
			FILE_ALL_ACCESS,
			&ObjectAttributes,
			&IoSB,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			CreateDisposition,
			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&RegistryHive->HiveFileName);
      RtlFreeUnicodeString(&RegistryHive->LogFileName);
      DPRINT("NtCreateFile() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Note: Another workaround! See the note above! */
#if 0
  if ((CreateNew) && (IoSB.Information == FILE_CREATED))
#endif
  if (IoSB.Information != FILE_OPENED)
    {
      Status = CmiCreateNewRegFile(FileHandle);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT("CmiCreateNewRegFile() failed (Status %lx)\n", Status);
	  NtClose(FileHandle);
	  RtlFreeUnicodeString(&RegistryHive->HiveFileName);
	  RtlFreeUnicodeString(&RegistryHive->LogFileName);
	  return(Status);
	}
    }

  /* Create the hive section */
  Status = NtCreateSection(&SectionHandle,
			   SECTION_ALL_ACCESS,
			   NULL,
			   NULL,
			   PAGE_READWRITE,
			   SEC_COMMIT,
			   FileHandle);
  NtClose(FileHandle);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtCreateSection() failed (Status %lx)\n", Status);
      RtlFreeUnicodeString(&RegistryHive->HiveFileName);
      RtlFreeUnicodeString(&RegistryHive->LogFileName);
      return(Status);
    }

  /* Map the hive file */
  ViewBase = NULL;
  ViewSize = 0;
  Status = NtMapViewOfSection(SectionHandle,
			      NtCurrentProcess(),
			      (PVOID*)&ViewBase,
			      0,
			      ViewSize,
			      NULL,
			      &ViewSize,
			      0,
			      MEM_COMMIT,
			      PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("MmMapViewInSystemSpace() failed (Status %lx)\n", Status);
      NtClose(SectionHandle);
      RtlFreeUnicodeString(&RegistryHive->HiveFileName);
      RtlFreeUnicodeString(&RegistryHive->LogFileName);
      return(Status);
    }
  DPRINT("ViewBase %p  ViewSize %lx\n", ViewBase, ViewSize);

  /* Copy hive header and initalize hive */
  RtlCopyMemory (RegistryHive->HiveHeader,
		 ViewBase,
		 sizeof(HIVE_HEADER));
  RegistryHive->FileSize = ViewSize;
  RegistryHive->BlockListSize = (RegistryHive->FileSize / 4096) - 1;
  RegistryHive->UpdateCounter = RegistryHive->HiveHeader->UpdateCounter1;

  /* Allocate hive block list */
  RegistryHive->BlockList = ExAllocatePool(NonPagedPool,
	  sizeof(PHBIN *) * RegistryHive->BlockListSize);
  if (RegistryHive->BlockList == NULL)
    {
      DPRINT1("Failed to allocate the hive block list\n");
      NtUnmapViewOfSection(NtCurrentProcess(),
			   ViewBase);
      NtClose(SectionHandle);
      RtlFreeUnicodeString(&RegistryHive->HiveFileName);
      RtlFreeUnicodeString(&RegistryHive->LogFileName);
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  /* Import the hive bins */
  Status = CmiImportHiveBins (RegistryHive,
			      ViewBase + 4096);
  if (!NT_SUCCESS(Status))
    {
      ExFreePool(RegistryHive->BlockList);
      NtUnmapViewOfSection(NtCurrentProcess(),
			   ViewBase);
      NtClose(SectionHandle);
      RtlFreeUnicodeString(&RegistryHive->HiveFileName);
      RtlFreeUnicodeString(&RegistryHive->LogFileName);
      return Status;
    }

  /* Unmap and dereference the hive section */
  NtUnmapViewOfSection(NtCurrentProcess(),
		       ViewBase);
  NtClose(SectionHandle);

  /* Initialize the free cell list */
  Status = CmiCreateHiveFreeCellList (RegistryHive);
  if (!NT_SUCCESS(Status))
    {
      CmiFreeHiveBins(RegistryHive);
      ExFreePool(RegistryHive->BlockList);
      RtlFreeUnicodeString(&RegistryHive->HiveFileName);
      RtlFreeUnicodeString(&RegistryHive->LogFileName);
      return Status;
    }

  /* Create the block bitmap */
  Status = CmiCreateHiveBitmap (RegistryHive);
  if (!NT_SUCCESS(Status))
    {
      CmiFreeHiveFreeCellList(RegistryHive);
      CmiFreeHiveBins(RegistryHive);
      ExFreePool(RegistryHive->BlockList);
      RtlFreeUnicodeString(&RegistryHive->HiveFileName);
      RtlFreeUnicodeString(&RegistryHive->LogFileName);
      return Status;
    }

  DPRINT("CmiInitNonVolatileRegistryHive(%p, %S, %d) - Finished.\n",
	 RegistryHive, Filename, CreateNew);

  return(STATUS_SUCCESS);
}


static NTSTATUS
CmiInitVolatileRegistryHive(PREGISTRY_HIVE RegistryHive)
{
  PKEY_CELL RootKeyCell;

  RegistryHive->Flags |= (HIVE_VOLATILE | HIVE_POINTER);

  CmiCreateDefaultHiveHeader(RegistryHive->HiveHeader);

  RootKeyCell = (PKEY_CELL) ExAllocatePool(NonPagedPool, sizeof(KEY_CELL));

  if (RootKeyCell == NULL)
    return STATUS_INSUFFICIENT_RESOURCES;

  CmiCreateDefaultRootKeyCell(RootKeyCell);

  RegistryHive->HiveHeader->RootKeyCell = (BLOCK_OFFSET) RootKeyCell;

  return STATUS_SUCCESS;
}


NTSTATUS
CmiCreateRegistryHive(PWSTR Filename,
		      PREGISTRY_HIVE *RegistryHive,
		      BOOLEAN CreateNew)
{
  PREGISTRY_HIVE Hive;
  NTSTATUS Status;

  DPRINT("CmiCreateRegistryHive(Filename %S)\n", Filename);

  *RegistryHive = NULL;

  Hive = ExAllocatePool(NonPagedPool, sizeof(REGISTRY_HIVE));
  if (Hive == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  DPRINT("Hive %x\n", Hive);

  RtlZeroMemory(Hive, sizeof(REGISTRY_HIVE));

  Hive->HiveHeader = (PHIVE_HEADER)
    ExAllocatePool(NonPagedPool, sizeof(HIVE_HEADER));

  if (Hive->HiveHeader == NULL)
    {
      ExFreePool(Hive);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  if (Filename != NULL)
    {
      Status = CmiInitNonVolatileRegistryHive(Hive, Filename, CreateNew);
    }
  else
    {
      Status = CmiInitVolatileRegistryHive(Hive);
    }

  if (!NT_SUCCESS(Status))
    {
      ExFreePool(Hive->HiveHeader);
      ExFreePool(Hive);
      return(Status);
    }

  ExInitializeResourceLite(&Hive->HiveResource);

  /* Acquire hive list lock exclusively */
  ExAcquireResourceExclusiveLite(&CmiHiveListLock, TRUE);

  /* Add the new hive to the hive list */
  InsertTailList(&CmiHiveListHead, &Hive->HiveList);

  /* Release hive list lock */
  ExReleaseResourceLite(&CmiHiveListLock);

  VERIFY_REGISTRY_HIVE(Hive);

  *RegistryHive = Hive;

  DPRINT("CmiCreateRegistryHive(Filename %S) - Finished.\n", Filename);

  return(STATUS_SUCCESS);
}


NTSTATUS
CmiRemoveRegistryHive(PREGISTRY_HIVE RegistryHive)
{
  /* Acquire hive list lock exclusively */
  ExAcquireResourceExclusiveLite(&CmiHiveListLock, TRUE);

  /* Remove hive from hive list */
  RemoveEntryList(&RegistryHive->HiveList);

  /* Release hive list lock */
  ExReleaseResourceLite(&CmiHiveListLock);


  /* FIXME: Remove attached keys and values */


  /* Release hive header */
  ExFreePool(RegistryHive->HiveHeader);

  /* Release hive */
  ExFreePool(RegistryHive);

  return(STATUS_SUCCESS);
}


static ULONG
CmiCalcChecksum(PULONG Buffer)
{
  ULONG Sum = 0;
  ULONG i;

  for (i = 0; i < 127; i++)
    Sum += Buffer[i];

  return(Sum);
}


static NTSTATUS
CmiStartLogUpdate(PREGISTRY_HIVE RegistryHive)
{
  FILE_END_OF_FILE_INFORMATION EndOfFileInfo;
  FILE_ALLOCATION_INFORMATION FileAllocationInfo;
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  HANDLE FileHandle;
  LARGE_INTEGER FileOffset;
  ULONG BufferSize;
  ULONG BitmapSize;
  PUCHAR Buffer;
  PUCHAR Ptr;
  ULONG BlockIndex;
  PVOID BlockPtr;
  NTSTATUS Status;

  DPRINT("CmiStartLogUpdate() called\n");

  BitmapSize = ROUND_UP(RegistryHive->BlockListSize, sizeof(ULONG) * 8) / 8;
  BufferSize = sizeof(HIVE_HEADER) +
	       sizeof(ULONG) +
	       BitmapSize;
  BufferSize = ROUND_UP(BufferSize, 4096);

  DPRINT("Bitmap size %lu  buffer size: %lu\n", BitmapSize, BufferSize);

  Buffer = (PUCHAR)ExAllocatePool(NonPagedPool, BufferSize);
  if (Buffer == NULL)
    {
      DPRINT("ExAllocatePool() failed\n");
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  /* Open log file for writing */
  InitializeObjectAttributes(&ObjectAttributes,
			     &RegistryHive->LogFileName,
			     0,
			     NULL,
			     NULL);

  Status = NtCreateFile(&FileHandle,
			FILE_ALL_ACCESS,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			FILE_SUPERSEDE,
			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtCreateFile() failed (Status %lx)\n", Status);
      ExFreePool(Buffer);
      return(Status);
    }

  /* Update firt update counter and checksum */
  RegistryHive->HiveHeader->UpdateCounter1 = RegistryHive->UpdateCounter + 1;
  RegistryHive->HiveHeader->Checksum = CmiCalcChecksum((PULONG)RegistryHive->HiveHeader);

  /* Copy hive header */
  RtlCopyMemory(Buffer,
		RegistryHive->HiveHeader,
		sizeof(HIVE_HEADER));
  Ptr = Buffer + sizeof(HIVE_HEADER);

  RtlCopyMemory(Ptr,
		"DIRT",
		4);
  Ptr += 4;
  RtlCopyMemory(Ptr,
		RegistryHive->DirtyBitMap.Buffer,
		BitmapSize);

  /* Write hive block and block bitmap */
  FileOffset.QuadPart = 0ULL;
  Status = NtWriteFile(FileHandle,
		       NULL,
		       NULL,
		       NULL,
		       &IoStatusBlock,
		       Buffer,
		       BufferSize,
		       &FileOffset,
		       NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
      NtClose(FileHandle);
      ExFreePool(Buffer);
      return(Status);
    }
  ExFreePool(Buffer);

  /* Write dirty blocks */
  FileOffset.QuadPart = (ULONGLONG)BufferSize;
  BlockIndex = 0;
  while (TRUE)
    {
      BlockIndex = RtlFindSetBits(&RegistryHive->DirtyBitMap,
				  1,
				  BlockIndex);
      if ((BlockIndex == (ULONG)-1) ||
	  (BlockIndex >= RegistryHive->BlockListSize))
	{
	  DPRINT("No more set bits\n");
	  Status = STATUS_SUCCESS;
	  break;
	}

      DPRINT("Block %lu is dirty\n", BlockIndex);

      BlockPtr = RegistryHive->BlockList[BlockIndex];
      DPRINT("BlockPtr %p\n", BlockPtr);
      DPRINT("File offset %I64x\n", FileOffset.QuadPart);

      /* Write hive block */
      Status = NtWriteFile(FileHandle,
			   NULL,
			   NULL,
			   NULL,
			   &IoStatusBlock,
			   BlockPtr,
			   REG_BLOCK_SIZE,
			   &FileOffset,
			   NULL);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1("NtWriteFile() failed (Status %lx)\n", Status);
	  NtClose(FileHandle);
	  return(Status);
	}

      BlockIndex++;
      FileOffset.QuadPart += 4096ULL;
    }

  /* Truncate log file */
  EndOfFileInfo.EndOfFile.QuadPart = FileOffset.QuadPart;
  Status = NtSetInformationFile(FileHandle,
				&IoStatusBlock,
				&EndOfFileInfo,
				sizeof(FILE_END_OF_FILE_INFORMATION),
				FileEndOfFileInformation);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtSetInformationFile() failed (Status %lx)\n", Status);
      NtClose(FileHandle);
      return(Status);
    }

  FileAllocationInfo.AllocationSize.QuadPart = FileOffset.QuadPart;
  Status = NtSetInformationFile(FileHandle,
				&IoStatusBlock,
				&FileAllocationInfo,
				sizeof(FILE_ALLOCATION_INFORMATION),
				FileAllocationInformation);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtSetInformationFile() failed (Status %lx)\n", Status);
      NtClose(FileHandle);
      return(Status);
    }

  /* Flush the log file */
  Status = NtFlushBuffersFile(FileHandle,
			      &IoStatusBlock);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtFlushBuffersFile() failed (Status %lx)\n", Status);
    }

  NtClose(FileHandle);

  return(Status);
}


static NTSTATUS
CmiFinishLogUpdate(PREGISTRY_HIVE RegistryHive)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  HANDLE FileHandle;
  LARGE_INTEGER FileOffset;
  ULONG BufferSize;
  ULONG BitmapSize;
  PUCHAR Buffer;
  PUCHAR Ptr;
  NTSTATUS Status;

  DPRINT("CmiFinishLogUpdate() called\n");

  BitmapSize = ROUND_UP(RegistryHive->BlockListSize, sizeof(ULONG) * 8) / 8;
  BufferSize = sizeof(HIVE_HEADER) +
	       sizeof(ULONG) +
	       BitmapSize;
  BufferSize = ROUND_UP(BufferSize, 4096);

  DPRINT("Bitmap size %lu  buffer size: %lu\n", BitmapSize, BufferSize);

  Buffer = (PUCHAR)ExAllocatePool(NonPagedPool, BufferSize);
  if (Buffer == NULL)
    {
      DPRINT("ExAllocatePool() failed\n");
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  /* Open log file for writing */
  InitializeObjectAttributes(&ObjectAttributes,
			     &RegistryHive->LogFileName,
			     0,
			     NULL,
			     NULL);

  Status = NtCreateFile(&FileHandle,
			FILE_ALL_ACCESS,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			FILE_OPEN,
			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtCreateFile() failed (Status %lx)\n", Status);
      ExFreePool(Buffer);
      return(Status);
    }

  /* Update first and second update counter and checksum */
  RegistryHive->HiveHeader->UpdateCounter1 = RegistryHive->UpdateCounter + 1;
  RegistryHive->HiveHeader->UpdateCounter2 = RegistryHive->UpdateCounter + 1;
  RegistryHive->HiveHeader->Checksum = CmiCalcChecksum((PULONG)RegistryHive->HiveHeader);

  /* Copy hive header */
  RtlCopyMemory(Buffer,
		RegistryHive->HiveHeader,
		sizeof(HIVE_HEADER));
  Ptr = Buffer + sizeof(HIVE_HEADER);

  /* Write empty block bitmap */
  RtlCopyMemory(Ptr,
		"DIRT",
		4);
  Ptr += 4;
  RtlZeroMemory(Ptr,
		BitmapSize);

  /* Write hive block and block bitmap */
  FileOffset.QuadPart = 0ULL;
  Status = NtWriteFile(FileHandle,
		       NULL,
		       NULL,
		       NULL,
		       &IoStatusBlock,
		       Buffer,
		       BufferSize,
		       &FileOffset,
		       NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
      NtClose(FileHandle);
      ExFreePool(Buffer);
      return(Status);
    }

  ExFreePool(Buffer);

  /* Flush the log file */
  Status = NtFlushBuffersFile(FileHandle,
			      &IoStatusBlock);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtFlushBuffersFile() failed (Status %lx)\n", Status);
    }

  NtClose(FileHandle);

  return(Status);
}


static NTSTATUS
CmiCleanupLogUpdate(PREGISTRY_HIVE RegistryHive)
{
  FILE_END_OF_FILE_INFORMATION EndOfFileInfo;
  FILE_ALLOCATION_INFORMATION FileAllocationInfo;
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  HANDLE FileHandle;
  ULONG BufferSize;
  ULONG BitmapSize;
  NTSTATUS Status;

  DPRINT("CmiCleanupLogUpdate() called\n");

  BitmapSize = ROUND_UP(RegistryHive->BlockListSize, sizeof(ULONG) * 8) / 8;
  BufferSize = sizeof(HIVE_HEADER) +
	       sizeof(ULONG) +
	       BitmapSize;
  BufferSize = ROUND_UP(BufferSize, 4096);

  DPRINT("Bitmap size %lu  buffer size: %lu\n", BitmapSize, BufferSize);

  /* Open log file for writing */
  InitializeObjectAttributes(&ObjectAttributes,
			     &RegistryHive->LogFileName,
			     0,
			     NULL,
			     NULL);

  Status = NtCreateFile(&FileHandle,
			FILE_ALL_ACCESS,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			FILE_OPEN,
			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtCreateFile() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Truncate log file */
  EndOfFileInfo.EndOfFile.QuadPart = (ULONGLONG)BufferSize;
  Status = NtSetInformationFile(FileHandle,
				&IoStatusBlock,
				&EndOfFileInfo,
				sizeof(FILE_END_OF_FILE_INFORMATION),
				FileEndOfFileInformation);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtSetInformationFile() failed (Status %lx)\n", Status);
      NtClose(FileHandle);
      return(Status);
    }

  FileAllocationInfo.AllocationSize.QuadPart = (ULONGLONG)BufferSize;
  Status = NtSetInformationFile(FileHandle,
				&IoStatusBlock,
				&FileAllocationInfo,
				sizeof(FILE_ALLOCATION_INFORMATION),
				FileAllocationInformation);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtSetInformationFile() failed (Status %lx)\n", Status);
      NtClose(FileHandle);
      return(Status);
    }

  /* Flush the log file */
  Status = NtFlushBuffersFile(FileHandle,
			      &IoStatusBlock);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtFlushBuffersFile() failed (Status %lx)\n", Status);
    }

  NtClose(FileHandle);

  return(Status);
}


static NTSTATUS
CmiStartHiveUpdate(PREGISTRY_HIVE RegistryHive)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  HANDLE FileHandle;
  LARGE_INTEGER FileOffset;
  ULONG BlockIndex;
  PVOID BlockPtr;
  NTSTATUS Status;

  DPRINT("CmiStartHiveUpdate() called\n");

  /* Open hive for writing */
  InitializeObjectAttributes(&ObjectAttributes,
			     &RegistryHive->HiveFileName,
			     0,
			     NULL,
			     NULL);

  Status = NtCreateFile(&FileHandle,
			FILE_ALL_ACCESS,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			FILE_OPEN,
			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtCreateFile() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Update firt update counter and checksum */
  RegistryHive->HiveHeader->UpdateCounter1 = RegistryHive->UpdateCounter + 1;
  RegistryHive->HiveHeader->Checksum = CmiCalcChecksum((PULONG)RegistryHive->HiveHeader);

  /* Write hive block */
  FileOffset.QuadPart = 0ULL;
  Status = NtWriteFile(FileHandle,
		       NULL,
		       NULL,
		       NULL,
		       &IoStatusBlock,
		       RegistryHive->HiveHeader,
		       sizeof(HIVE_HEADER),
		       &FileOffset,
		       NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
      NtClose(FileHandle);
      return(Status);
    }

  BlockIndex = 0;
  while (TRUE)
    {
      BlockIndex = RtlFindSetBits(&RegistryHive->DirtyBitMap,
				  1,
				  BlockIndex);
      if ((BlockIndex == (ULONG)-1) ||
	  (BlockIndex >= RegistryHive->BlockListSize))
	{
	  DPRINT("No more set bits\n");
	  Status = STATUS_SUCCESS;
	  break;
	}

      DPRINT("Block %lu is dirty\n", BlockIndex);

      BlockPtr = RegistryHive->BlockList[BlockIndex];
      DPRINT("BlockPtr %p\n", BlockPtr);

      FileOffset.QuadPart = (ULONGLONG)(BlockIndex + 1) * 4096ULL;
      DPRINT("File offset %I64x\n", FileOffset.QuadPart);

      /* Write hive block */
      Status = NtWriteFile(FileHandle,
			   NULL,
			   NULL,
			   NULL,
			   &IoStatusBlock,
			   BlockPtr,
			   REG_BLOCK_SIZE,
			   &FileOffset,
			   NULL);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
	  NtClose(FileHandle);
	  return(Status);
	}

      BlockIndex++;
    }

  Status = NtFlushBuffersFile(FileHandle,
			      &IoStatusBlock);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtFlushBuffersFile() failed (Status %lx)\n", Status);
    }

  NtClose(FileHandle);

  return(Status);
}


static NTSTATUS
CmiFinishHiveUpdate(PREGISTRY_HIVE RegistryHive)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  LARGE_INTEGER FileOffset;
  HANDLE FileHandle;
  NTSTATUS Status;

  DPRINT("CmiFinishHiveUpdate() called\n");

  InitializeObjectAttributes(&ObjectAttributes,
			     &RegistryHive->HiveFileName,
			     0,
			     NULL,
			     NULL);

  Status = NtCreateFile(&FileHandle,
			FILE_ALL_ACCESS,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			FILE_OPEN,
			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtCreateFile() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Update second update counter and checksum */
  RegistryHive->HiveHeader->UpdateCounter1 = RegistryHive->UpdateCounter + 1;
  RegistryHive->HiveHeader->UpdateCounter2 = RegistryHive->UpdateCounter + 1;
  RegistryHive->HiveHeader->Checksum = CmiCalcChecksum((PULONG)RegistryHive->HiveHeader);

  /* Write hive block */
  FileOffset.QuadPart = 0ULL;
  Status = NtWriteFile(FileHandle,
		       NULL,
		       NULL,
		       NULL,
		       &IoStatusBlock,
		       RegistryHive->HiveHeader,
		       sizeof(HIVE_HEADER),
		       &FileOffset,
		       NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
      NtClose(FileHandle);
      return(Status);
    }

  Status = NtFlushBuffersFile(FileHandle,
			      &IoStatusBlock);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtFlushBuffersFile() failed (Status %lx)\n", Status);
    }

  NtClose(FileHandle);

  return(Status);
}


NTSTATUS
CmiFlushRegistryHive(PREGISTRY_HIVE RegistryHive)
{
  NTSTATUS Status;

  DPRINT("CmiFlushRegistryHive() called\n");

  if (RegistryHive->HiveDirty == FALSE)
    {
      return(STATUS_SUCCESS);
    }

  DPRINT("Hive '%wZ' is dirty\n",
	 &RegistryHive->HiveFileName);
  DPRINT("Log file: '%wZ'\n",
	 &RegistryHive->LogFileName);

  /* Update hive header modification time */
  NtQuerySystemTime((PTIME)&RegistryHive->HiveHeader->DateModified);

  /* Start log update */
  Status = CmiStartLogUpdate(RegistryHive);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("CmiStartLogUpdate() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Finish log update */
  Status = CmiFinishLogUpdate(RegistryHive);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("CmiFinishLogUpdate() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Start hive update */
  Status = CmiStartHiveUpdate(RegistryHive);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("CmiStartHiveUpdate() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Finish the hive update */
  Status = CmiFinishHiveUpdate(RegistryHive);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("CmiFinishHiveUpdate() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Cleanup log update */
  Status = CmiCleanupLogUpdate(RegistryHive);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("CmiFinishLogUpdate() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Increment hive update counter */
  RegistryHive->UpdateCounter++;

  /* Clear dirty bitmap and dirty flag */
  RtlClearAllBits(&RegistryHive->DirtyBitMap);
  RegistryHive->HiveDirty = FALSE;

  DPRINT("CmiFlushRegistryHive() done\n");

  return(STATUS_SUCCESS);
}


ULONG
CmiGetMaxNameLength(PREGISTRY_HIVE  RegistryHive,
		    PKEY_CELL  KeyCell)
{
  PHASH_TABLE_CELL HashBlock;
  PKEY_CELL CurSubKeyCell;
  ULONG MaxName;
  ULONG i;

  VERIFY_KEY_CELL(KeyCell);

  MaxName = 0;
  HashBlock = CmiGetBlock(RegistryHive, KeyCell->HashTableOffset, NULL);
  if (HashBlock == NULL)
    {
      DPRINT("CmiGetBlock() failed\n");
      return 0;
    }

  for (i = 0; i < HashBlock->HashTableSize; i++)
    {
      if (HashBlock->Table[i].KeyOffset != 0)
	{
	  CurSubKeyCell = CmiGetBlock(RegistryHive,
				      HashBlock->Table[i].KeyOffset,
				      NULL);
	  if (CurSubKeyCell == NULL)
	    {
	      DPRINT("CmiGetBlock() failed\n");
	      return 0;
	    }

	  if (MaxName < CurSubKeyCell->NameSize)
	    {
	      MaxName = CurSubKeyCell->NameSize;
	    }
	}
    }

  return MaxName;
}


ULONG
CmiGetMaxClassLength(PREGISTRY_HIVE  RegistryHive,
		     PKEY_CELL  KeyCell)
{
  PHASH_TABLE_CELL HashBlock;
  PKEY_CELL CurSubKeyCell;
  ULONG MaxClass;
  ULONG i;

  VERIFY_KEY_CELL(KeyCell);

  MaxClass = 0;
  HashBlock = CmiGetBlock(RegistryHive, KeyCell->HashTableOffset, NULL);
  if (HashBlock == NULL)
    {
      DPRINT("CmiGetBlock() failed\n");
      return 0;
    }

  for (i = 0; i < HashBlock->HashTableSize; i++)
    {
      if (HashBlock->Table[i].KeyOffset != 0)
	{
	  CurSubKeyCell = CmiGetBlock(RegistryHive,
				      HashBlock->Table[i].KeyOffset,
				      NULL);
	  if (CurSubKeyCell == NULL)
	    {
	      DPRINT("CmiGetBlock() failed\n");
	      return 0;
	    }

	  if (MaxClass < CurSubKeyCell->ClassSize)
	    {
	      MaxClass = CurSubKeyCell->ClassSize;
	    }
	}
    }

  return MaxClass;
}


ULONG
CmiGetMaxValueNameLength(PREGISTRY_HIVE RegistryHive,
			 PKEY_CELL KeyCell)
{
  PVALUE_LIST_CELL ValueListCell;
  PVALUE_CELL CurValueCell;
  ULONG MaxValueName;
  ULONG i;

  VERIFY_KEY_CELL(KeyCell);

  MaxValueName = 0;
  ValueListCell = CmiGetBlock(RegistryHive,
			      KeyCell->ValuesOffset,
			      NULL);
  if (ValueListCell == NULL)
    {
      DPRINT("CmiGetBlock() failed\n");
      return 0;
    }

  for (i = 0; i < KeyCell->NumberOfValues; i++)
    {
      CurValueCell = CmiGetBlock (RegistryHive,
				  ValueListCell->Values[i],
				  NULL);
      if (CurValueCell == NULL)
	{
	  DPRINT("CmiGetBlock() failed\n");
	}

      if (CurValueCell != NULL &&
          MaxValueName < CurValueCell->NameSize)
        {
          MaxValueName = CurValueCell->NameSize;
        }
    }

  return MaxValueName;
}


ULONG
CmiGetMaxValueDataLength(PREGISTRY_HIVE RegistryHive,
			 PKEY_CELL KeyCell)
{
  PVALUE_LIST_CELL ValueListCell;
  PVALUE_CELL CurValueCell;
  LONG MaxValueData;
  ULONG i;

  VERIFY_KEY_CELL(KeyCell);

  MaxValueData = 0;
  ValueListCell = CmiGetBlock(RegistryHive, KeyCell->ValuesOffset, NULL);
  if (ValueListCell == NULL)
    {
      return 0;
    }

  for (i = 0; i < KeyCell->NumberOfValues; i++)
    {
      CurValueCell = CmiGetBlock(RegistryHive,
                                  ValueListCell->Values[i],NULL);
      if ((CurValueCell != NULL) &&
          (MaxValueData < (CurValueCell->DataSize & LONG_MAX)))
        {
          MaxValueData = CurValueCell->DataSize & LONG_MAX;
        }
    }

  return MaxValueData;
}


NTSTATUS
CmiScanForSubKey(IN PREGISTRY_HIVE RegistryHive,
		 IN PKEY_CELL KeyCell,
		 OUT PKEY_CELL *SubKeyCell,
		 OUT BLOCK_OFFSET *BlockOffset,
		 IN PCHAR KeyName,
		 IN ACCESS_MASK DesiredAccess,
		 IN ULONG Attributes)
{
  PHASH_TABLE_CELL HashBlock;
  PKEY_CELL CurSubKeyCell;
  USHORT KeyLength;
  ULONG i;

  VERIFY_KEY_CELL(KeyCell);

  //DPRINT("Scanning for sub key %s\n", KeyName);

  assert(RegistryHive);

  *SubKeyCell = NULL;

  /* The key does not have any subkeys */
  if (KeyCell->HashTableOffset == (BLOCK_OFFSET)-1)
    {
      return STATUS_SUCCESS;
    }

  /* Get hash table */
  HashBlock = CmiGetBlock(RegistryHive, KeyCell->HashTableOffset, NULL);
  if (HashBlock == NULL)
    {
      DPRINT("CmiGetBlock() failed\n");
      return STATUS_UNSUCCESSFUL;
    }

  KeyLength = strlen(KeyName);
  for (i = 0; (i < KeyCell->NumberOfSubKeys) && (i < HashBlock->HashTableSize); i++)
    {
      if (Attributes & OBJ_CASE_INSENSITIVE)
	{
	  if ((HashBlock->Table[i].KeyOffset != 0) &&
	      (HashBlock->Table[i].KeyOffset != (ULONG_PTR)-1) &&
	      (_strnicmp(KeyName, (PCHAR) &HashBlock->Table[i].HashValue, 4) == 0))
	    {
	      CurSubKeyCell = CmiGetBlock(RegistryHive,
					  HashBlock->Table[i].KeyOffset,
					  NULL);
	      if (CurSubKeyCell == NULL)
		{
		  DPRINT("CmiGetBlock() failed\n");
		  return STATUS_UNSUCCESSFUL;
		}

              if ((CurSubKeyCell->NameSize == KeyLength)
                  && (_strnicmp(KeyName, CurSubKeyCell->Name, KeyLength) == 0))
                {
                  *SubKeyCell = CurSubKeyCell;
                  *BlockOffset = HashBlock->Table[i].KeyOffset;
                  break;
                }
            }
	}
      else
	{
	  if (HashBlock->Table[i].KeyOffset != 0 &&
	      HashBlock->Table[i].KeyOffset != (ULONG_PTR) -1 &&
	      !strncmp(KeyName, (PCHAR) &HashBlock->Table[i].HashValue, 4))
	    {
	      CurSubKeyCell = CmiGetBlock(RegistryHive,
					  HashBlock->Table[i].KeyOffset,
					  NULL);
	      if (CurSubKeyCell == NULL)
		{
		  DPRINT("CmiGetBlock() failed\n");
		  return STATUS_UNSUCCESSFUL;
		}

              if (CurSubKeyCell->NameSize == KeyLength
                  && !_strnicmp(KeyName, CurSubKeyCell->Name, KeyLength))
                {
                  *SubKeyCell = CurSubKeyCell;
                  *BlockOffset = HashBlock->Table[i].KeyOffset;
                  break;
                }
            }
	}
    }

  return STATUS_SUCCESS;
}


NTSTATUS
CmiAddSubKey(PREGISTRY_HIVE RegistryHive,
	     PKEY_OBJECT Parent,
	     PKEY_OBJECT SubKey,
	     PWSTR NewSubKeyName,
	     USHORT NewSubKeyNameSize,
	     ULONG TitleIndex,
	     PUNICODE_STRING Class,
	     ULONG CreateOptions)
{
  PHASH_TABLE_CELL NewHashBlock;
  PHASH_TABLE_CELL HashBlock;
  BLOCK_OFFSET NKBOffset;
  PKEY_CELL NewKeyCell; 
  ULONG NewBlockSize;
  PKEY_CELL KeyCell;
  NTSTATUS Status;
  USHORT NameSize;

  KeyCell = Parent->KeyCell;

  VERIFY_KEY_CELL(KeyCell);

  if (NewSubKeyName[0] == L'\\')
    {
      NewSubKeyName++;
      NameSize = NewSubKeyNameSize / 2 - 1;
    }
  else
    {
      NameSize = NewSubKeyNameSize / 2;
    }
  Status = STATUS_SUCCESS;

  NewBlockSize = sizeof(KEY_CELL) + NameSize;
  Status = CmiAllocateBlock(RegistryHive,
			    (PVOID) &NewKeyCell,
			    NewBlockSize,
			    &NKBOffset);
  if (NewKeyCell == NULL)
    {
      Status = STATUS_INSUFFICIENT_RESOURCES;
    }
  else
    {
      NewKeyCell->Id = REG_KEY_CELL_ID;
      NewKeyCell->Flags = REG_KEY_NAME_PACKED;
      ZwQuerySystemTime((PTIME) &NewKeyCell->LastWriteTime);
      NewKeyCell->ParentKeyOffset = -1;
      NewKeyCell->NumberOfSubKeys = 0;
      NewKeyCell->HashTableOffset = -1;
      NewKeyCell->NumberOfValues = 0;
      NewKeyCell->ValuesOffset = -1;
      NewKeyCell->SecurityKeyOffset = -1;
      NewKeyCell->NameSize = NameSize;
      wcstombs(NewKeyCell->Name, NewSubKeyName, NameSize);
      NewKeyCell->ClassNameOffset = -1;

      VERIFY_KEY_CELL(NewKeyCell);

      if (Class)
	{
	  PDATA_CELL pClass;

	  NewKeyCell->ClassSize = Class->Length + sizeof(WCHAR);
	  Status = CmiAllocateBlock(RegistryHive,
				    (PVOID) &pClass,
				    NewKeyCell->ClassSize,
				    &NewKeyCell->ClassNameOffset);
	  wcsncpy((PWSTR) pClass->Data, Class->Buffer, Class->Length);
	  ((PWSTR) (pClass->Data))[Class->Length] = 0;
	}
    }

  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  SubKey->KeyCell = NewKeyCell;
  SubKey->BlockOffset = NKBOffset;

  /* Don't modify hash table if key is located in a pointer-based hive and parent key is not */
  if (IsPointerHive(RegistryHive) && (!IsPointerHive(Parent->RegistryHive)))
    {
      return(Status);
    }

  if (KeyCell->HashTableOffset == (ULONG_PTR) -1)
    {
      Status = CmiAllocateHashTableBlock(RegistryHive,
					 &HashBlock,
					 &KeyCell->HashTableOffset,
					 REG_INIT_HASH_TABLE_SIZE);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}
    }
  else
    {
      HashBlock = CmiGetBlock(RegistryHive, KeyCell->HashTableOffset, NULL);
      if (HashBlock == NULL)
	{
	  DPRINT("CmiGetBlock() failed\n");
	  return STATUS_UNSUCCESSFUL;
	}

      if (((KeyCell->NumberOfSubKeys + 1) >= HashBlock->HashTableSize))
	{
	  BLOCK_OFFSET HTOffset;

	  /* Reallocate the hash table block */
	  Status = CmiAllocateHashTableBlock(RegistryHive,
					     &NewHashBlock,
					     &HTOffset,
					     HashBlock->HashTableSize +
					       REG_EXTEND_HASH_TABLE_SIZE);
	  if (!NT_SUCCESS(Status))
	    {
	      return Status;
	    }

	  RtlZeroMemory(&NewHashBlock->Table[0],
			sizeof(NewHashBlock->Table[0]) * NewHashBlock->HashTableSize);
	  RtlCopyMemory(&NewHashBlock->Table[0],
			&HashBlock->Table[0],
			sizeof(NewHashBlock->Table[0]) * HashBlock->HashTableSize);
	  CmiDestroyBlock(RegistryHive,
			  HashBlock,
			  KeyCell->HashTableOffset);
	  KeyCell->HashTableOffset = HTOffset;
	  HashBlock = NewHashBlock;
	}
    }

  Status = CmiAddKeyToHashTable(RegistryHive,
				HashBlock,
				NewKeyCell,
				NKBOffset);
  if (NT_SUCCESS(Status))
    {
      KeyCell->NumberOfSubKeys++;
    }

  return(Status);
}


NTSTATUS
CmiRemoveSubKey(PREGISTRY_HIVE RegistryHive,
		PKEY_OBJECT ParentKey,
		PKEY_OBJECT SubKey)
{
  PHASH_TABLE_CELL HashBlock;
  PVALUE_LIST_CELL ValueList;
  PVALUE_CELL ValueCell;
  PDATA_CELL DataCell;
  ULONG i;

  DPRINT("CmiRemoveSubKey() called\n");

  /* Remove all values */
  if (SubKey->KeyCell->NumberOfValues != 0)
    {
      /* Get pointer to the value list cell */
      ValueList = CmiGetBlock(RegistryHive,
			      SubKey->KeyCell->ValuesOffset,
			      NULL);
      if (ValueList == NULL)
	{
	  DPRINT("CmiGetBlock() failed\n");
	  return STATUS_UNSUCCESSFUL;
	}

      if (ValueList != NULL)
	{
	  /* Enumerate all values */
	  for (i = 0; i < SubKey->KeyCell->NumberOfValues; i++)
	    {
	      /* Get pointer to value cell */
	      ValueCell = CmiGetBlock(RegistryHive,
				      ValueList->Values[i],
				      NULL);
	      if (ValueCell != NULL)
		{
		  if (ValueCell->DataSize > 4)
		    {
		      DataCell = CmiGetBlock(RegistryHive,
					     ValueCell->DataOffset,
					     NULL);
		      if (DataCell == NULL)
			{
			  DPRINT("CmiGetBlock() failed\n");
			  return STATUS_UNSUCCESSFUL;
			}

		      if (DataCell != NULL)
			{
			  /* Destroy data cell */
			  CmiDestroyBlock(RegistryHive,
					  DataCell,
					  ValueCell->DataOffset);
			}
		    }

		  /* Destroy value cell */
		  CmiDestroyBlock(RegistryHive,
				  ValueCell,
				  ValueList->Values[i]);
		}
	    }
	}

      /* Destroy value list cell */
      CmiDestroyBlock(RegistryHive,
		      ValueList,
		      SubKey->KeyCell->ValuesOffset);

      SubKey->KeyCell->NumberOfValues = 0;
      SubKey->KeyCell->ValuesOffset = -1;
    }

  /* Remove the key from the parent key's hash block */
  if (ParentKey->KeyCell->HashTableOffset != (BLOCK_OFFSET) -1)
    {
      DPRINT("ParentKey HashTableOffset %lx\n", ParentKey->KeyCell->HashTableOffset)
      HashBlock = CmiGetBlock(RegistryHive,
			      ParentKey->KeyCell->HashTableOffset,
			      NULL);
      if (HashBlock == NULL)
	{
	  DPRINT("CmiGetBlock() failed\n");
	  return STATUS_UNSUCCESSFUL;
	}
      DPRINT("ParentKey HashBlock %p\n", HashBlock)
      if (HashBlock != NULL)
	{
	  CmiRemoveKeyFromHashTable(RegistryHive,
				    HashBlock,
				    SubKey->BlockOffset);
	  CmiMarkBlockDirty(RegistryHive,
			    ParentKey->KeyCell->HashTableOffset);
	}
    }

  /* Remove the key's hash block */
  if (SubKey->KeyCell->HashTableOffset != (BLOCK_OFFSET) -1)
    {
      DPRINT("SubKey HashTableOffset %lx\n", SubKey->KeyCell->HashTableOffset)
      HashBlock = CmiGetBlock(RegistryHive,
			      SubKey->KeyCell->HashTableOffset,
			      NULL);
      if (HashBlock == NULL)
	{
	  DPRINT("CmiGetBlock() failed\n");
	  return STATUS_UNSUCCESSFUL;
	}
      DPRINT("SubKey HashBlock %p\n", HashBlock)
      if (HashBlock != NULL)
	{
	  CmiDestroyBlock(RegistryHive,
			  HashBlock,
			  SubKey->KeyCell->HashTableOffset);
	  SubKey->KeyCell->HashTableOffset = -1;
	}
    }

  /* Decrement the number of the parent key's sub keys */
  if (ParentKey != NULL)
    {
      DPRINT("ParentKey %p\n", ParentKey)
      ParentKey->KeyCell->NumberOfSubKeys--;

      /* Remove the parent key's hash table */
      if (ParentKey->KeyCell->NumberOfSubKeys == 0)
	{
	  DPRINT("ParentKey HashTableOffset %lx\n", ParentKey->KeyCell->HashTableOffset)
	  HashBlock = CmiGetBlock(RegistryHive,
				  ParentKey->KeyCell->HashTableOffset,
				  NULL);
	  if (HashBlock == NULL)
	    {
	      DPRINT("CmiGetBlock() failed\n");
	      return STATUS_UNSUCCESSFUL;
	    }
	  DPRINT("ParentKey HashBlock %p\n", HashBlock)
	  if (HashBlock != NULL)
	    {
	      CmiDestroyBlock(RegistryHive,
			      HashBlock,
			      ParentKey->KeyCell->HashTableOffset);
	      ParentKey->KeyCell->HashTableOffset = -1;
	    }
	}

      NtQuerySystemTime((PTIME)&ParentKey->KeyCell->LastWriteTime);
      CmiMarkBlockDirty(RegistryHive,
			ParentKey->BlockOffset);
    }

  /* Destroy key cell */
  CmiDestroyBlock(RegistryHive,
		  SubKey->KeyCell,
		  SubKey->BlockOffset);
  SubKey->BlockOffset = -1;
  SubKey->KeyCell = NULL;

  return(STATUS_SUCCESS);
}


NTSTATUS
CmiScanKeyForValue(IN PREGISTRY_HIVE RegistryHive,
		   IN PKEY_CELL KeyCell,
		   IN PUNICODE_STRING ValueName,
		   OUT PVALUE_CELL *ValueCell,
		   OUT BLOCK_OFFSET *VBOffset)
{
  PVALUE_LIST_CELL ValueListCell;
  PVALUE_CELL CurValueCell;
  ULONG i;

  *ValueCell = NULL;

  /* The key does not have any values */
  if (KeyCell->ValuesOffset == (BLOCK_OFFSET)-1)
    {
      return STATUS_SUCCESS;
    }

  ValueListCell = CmiGetBlock(RegistryHive, KeyCell->ValuesOffset, NULL);
  if (ValueListCell == NULL)
    {
      DPRINT("ValueListCell is NULL\n");
      return STATUS_UNSUCCESSFUL;
    }

  VERIFY_VALUE_LIST_CELL(ValueListCell);

  for (i = 0; i < KeyCell->NumberOfValues; i++)
    {
      CurValueCell = CmiGetBlock(RegistryHive,
				 ValueListCell->Values[i],
				 NULL);
      if (CurValueCell == NULL)
	{
	  DPRINT("CmiGetBlock() failed\n");
	  return STATUS_UNSUCCESSFUL;
	}

      if ((CurValueCell != NULL) &&
	  CmiComparePackedNames(ValueName,
				CurValueCell->Name,
				CurValueCell->NameSize,
				CurValueCell->Flags & REG_VALUE_NAME_PACKED))
	{
	  *ValueCell = CurValueCell;
	  if (VBOffset)
	    *VBOffset = ValueListCell->Values[i];
	  //DPRINT("Found value %s\n", ValueName);
	  break;
	}
    }

  return STATUS_SUCCESS;
}


NTSTATUS
CmiGetValueFromKeyByIndex(IN PREGISTRY_HIVE RegistryHive,
			  IN PKEY_CELL KeyCell,
			  IN ULONG Index,
			  OUT PVALUE_CELL *ValueCell)
{
  PVALUE_LIST_CELL ValueListCell;
  PVALUE_CELL CurValueCell;

  *ValueCell = NULL;

  if (KeyCell->ValuesOffset == (BLOCK_OFFSET)-1)
    {
      return STATUS_NO_MORE_ENTRIES;
    }

  if (Index >= KeyCell->NumberOfValues)
    {
      return STATUS_NO_MORE_ENTRIES;
    }


  ValueListCell = CmiGetBlock(RegistryHive, KeyCell->ValuesOffset, NULL);
  if (ValueListCell == NULL)
    {
      DPRINT("CmiGetBlock() failed\n");
      return STATUS_UNSUCCESSFUL;
    }

  VERIFY_VALUE_LIST_CELL(ValueListCell);


  CurValueCell = CmiGetBlock(RegistryHive,
			     ValueListCell->Values[Index],
			     NULL);
  if (CurValueCell == NULL)
    {
      DPRINT("CmiGetBlock() failed\n");
      return STATUS_UNSUCCESSFUL;
    }

  *ValueCell = CurValueCell;

  return STATUS_SUCCESS;
}


NTSTATUS
CmiAddValueToKey(IN PREGISTRY_HIVE RegistryHive,
		 IN PKEY_CELL KeyCell,
		 IN PUNICODE_STRING ValueName,
		 OUT PVALUE_CELL *pValueCell,
		 OUT BLOCK_OFFSET *pVBOffset)
{
  PVALUE_LIST_CELL NewValueListCell;
  PVALUE_LIST_CELL ValueListCell;
  PVALUE_CELL NewValueCell;
  BLOCK_OFFSET VLBOffset;
  BLOCK_OFFSET VBOffset;
  NTSTATUS Status;

  Status = CmiAllocateValueCell(RegistryHive,
				&NewValueCell,
				&VBOffset,
				ValueName);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  DPRINT("KeyCell->ValuesOffset %lu\n", (ULONG)KeyCell->ValuesOffset);

  ValueListCell = CmiGetBlock(RegistryHive, KeyCell->ValuesOffset, NULL);

  if (ValueListCell == NULL)
    {
      Status = CmiAllocateBlock(RegistryHive,
	      (PVOID) &ValueListCell,
	      sizeof(BLOCK_OFFSET) * 3,
	      &VLBOffset);

      if (!NT_SUCCESS(Status))
	{
	  CmiDestroyValueCell(RegistryHive, NewValueCell, VBOffset);
	  return Status;
	}
      KeyCell->ValuesOffset = VLBOffset;
    }
  else if (KeyCell->NumberOfValues >= 
	   (((ULONG)ABS_VALUE(ValueListCell->CellSize) - 4) / sizeof(BLOCK_OFFSET)))
    {
      Status = CmiAllocateBlock(RegistryHive,
				(PVOID) &NewValueListCell,
				(KeyCell->NumberOfValues + REG_VALUE_LIST_CELL_MULTIPLE) *
				  sizeof(BLOCK_OFFSET),
				&VLBOffset);
      if (!NT_SUCCESS(Status))
	{
	  CmiDestroyValueCell(RegistryHive, NewValueCell, VBOffset);
	  return Status;
	}

      RtlCopyMemory(&NewValueListCell->Values[0],
		    &ValueListCell->Values[0],
		    sizeof(BLOCK_OFFSET) * KeyCell->NumberOfValues);
      CmiDestroyBlock(RegistryHive, ValueListCell, KeyCell->ValuesOffset);
      KeyCell->ValuesOffset = VLBOffset;
      ValueListCell = NewValueListCell;
    }

  DPRINT("KeyCell->NumberOfValues %lu, ValueListCell->CellSize %lu (%lu %lx)\n",
	 KeyCell->NumberOfValues,
	 (ULONG)ABS_VALUE(ValueListCell->CellSize),
	 ((ULONG)ABS_VALUE(ValueListCell->CellSize) - 4) / sizeof(BLOCK_OFFSET),
	 ((ULONG)ABS_VALUE(ValueListCell->CellSize) - 4) / sizeof(BLOCK_OFFSET));

  ValueListCell->Values[KeyCell->NumberOfValues] = VBOffset;
  KeyCell->NumberOfValues++;

  *pValueCell = NewValueCell;
  *pVBOffset = VBOffset;

  return STATUS_SUCCESS;
}


NTSTATUS
CmiDeleteValueFromKey(IN PREGISTRY_HIVE RegistryHive,
		      IN PKEY_CELL KeyCell,
		      IN BLOCK_OFFSET KeyCellOffset,
		      IN PUNICODE_STRING ValueName)
{
  PVALUE_LIST_CELL ValueListCell;
  PVALUE_CELL CurValueCell;
  ULONG  i;

  ValueListCell = CmiGetBlock(RegistryHive, KeyCell->ValuesOffset, NULL);

  if (ValueListCell == NULL)
    {
      DPRINT("CmiGetBlock() failed\n");
      return STATUS_SUCCESS;
    }

  VERIFY_VALUE_LIST_CELL(ValueListCell);

  for (i = 0; i < KeyCell->NumberOfValues; i++)
    {
      CurValueCell = CmiGetBlock(RegistryHive, ValueListCell->Values[i], NULL);
      if (CurValueCell == NULL)
	{
	  DPRINT("CmiGetBlock() failed\n");
	  return STATUS_UNSUCCESSFUL;
	}

      if ((CurValueCell != NULL) &&
	  CmiComparePackedNames(ValueName,
				CurValueCell->Name,
				CurValueCell->NameSize,
				CurValueCell->Flags & REG_VALUE_NAME_PACKED))
	{
	  CmiDestroyValueCell(RegistryHive, CurValueCell, ValueListCell->Values[i]);

	  if ((KeyCell->NumberOfValues - 1) < i)
	    {
	      RtlCopyMemory(&ValueListCell->Values[i],
			    &ValueListCell->Values[i + 1],
			    sizeof(BLOCK_OFFSET) * (KeyCell->NumberOfValues - 1 - i));
	    }
	  else
	    {
	      RtlZeroMemory(&ValueListCell->Values[i], sizeof(BLOCK_OFFSET));
	    }

	  KeyCell->NumberOfValues -= 1;
	  break;
	}
    }

  if (KeyCell->NumberOfValues == 0)
    {
      CmiDestroyBlock(RegistryHive,
		      ValueListCell,
		      KeyCell->ValuesOffset);
    }
  else
    {
      CmiMarkBlockDirty(RegistryHive,
			KeyCell->ValuesOffset);
    }

  CmiMarkBlockDirty(RegistryHive,
		    KeyCellOffset);

  return STATUS_SUCCESS;
}


NTSTATUS
CmiAllocateHashTableBlock(IN PREGISTRY_HIVE RegistryHive,
	OUT PHASH_TABLE_CELL *HashBlock,
	OUT BLOCK_OFFSET *HBOffset,
	IN ULONG HashTableSize)
{
  PHASH_TABLE_CELL NewHashBlock;
  ULONG NewHashSize;
  NTSTATUS Status;

  Status = STATUS_SUCCESS;
  *HashBlock = NULL;
  NewHashSize = sizeof(HASH_TABLE_CELL) + 
    (HashTableSize - 1) * sizeof(HASH_RECORD);
  Status = CmiAllocateBlock(RegistryHive,
			    (PVOID*) &NewHashBlock,
			    NewHashSize,
			    HBOffset);

  if ((NewHashBlock == NULL) || (!NT_SUCCESS(Status)))
    {
      Status = STATUS_INSUFFICIENT_RESOURCES;
    }
  else
    {
      NewHashBlock->Id = REG_HASH_TABLE_BLOCK_ID;
      NewHashBlock->HashTableSize = HashTableSize;
      *HashBlock = NewHashBlock;
    }

  return Status;
}


PKEY_CELL
CmiGetKeyFromHashByIndex(PREGISTRY_HIVE RegistryHive,
			 PHASH_TABLE_CELL HashBlock,
			 ULONG Index)
{
  BLOCK_OFFSET KeyOffset;
  PKEY_CELL KeyCell;

  if (HashBlock == NULL)
    return NULL;

  if (IsPointerHive(RegistryHive))
    {
      KeyCell = (PKEY_CELL) HashBlock->Table[Index].KeyOffset;
    }
  else
    {
      KeyOffset =  HashBlock->Table[Index].KeyOffset;
      KeyCell = CmiGetBlock(RegistryHive, KeyOffset, NULL);
    }

  return KeyCell;
}


NTSTATUS
CmiAddKeyToHashTable(PREGISTRY_HIVE RegistryHive,
		     PHASH_TABLE_CELL HashBlock,
		     PKEY_CELL NewKeyCell,
		     BLOCK_OFFSET NKBOffset)
{
  ULONG i;

  for (i = 0; i < HashBlock->HashTableSize; i++)
    {
      if (HashBlock->Table[i].KeyOffset == 0)
	{
	  HashBlock->Table[i].KeyOffset = NKBOffset;
	  RtlCopyMemory(&HashBlock->Table[i].HashValue,
			NewKeyCell->Name,
			4);
	  return STATUS_SUCCESS;
	}
    }

  return STATUS_UNSUCCESSFUL;
}


NTSTATUS
CmiRemoveKeyFromHashTable(PREGISTRY_HIVE RegistryHive,
			  PHASH_TABLE_CELL HashBlock,
			  BLOCK_OFFSET NKBOffset)
{
  ULONG i;

  for (i = 0; i < HashBlock->HashTableSize; i++)
    {
      if (HashBlock->Table[i].KeyOffset == NKBOffset)
	{
	  HashBlock->Table[i].KeyOffset = 0;
	  RtlZeroMemory(&HashBlock->Table[i].HashValue,
			4);
	  return STATUS_SUCCESS;
	}
    }

  return STATUS_UNSUCCESSFUL;
}


NTSTATUS
CmiAllocateValueCell(PREGISTRY_HIVE RegistryHive,
		     PVALUE_CELL *ValueCell,
		     BLOCK_OFFSET *VBOffset,
		     IN PUNICODE_STRING ValueName)
{
  PVALUE_CELL NewValueCell;
  NTSTATUS Status;
  BOOLEAN Packable;
  ULONG NameSize;
  ULONG i;

  Status = STATUS_SUCCESS;

  NameSize = CmiGetPackedNameLength(ValueName,
				    &Packable);

  DPRINT("ValueName->Length %lu  NameSize %lu\n", ValueName->Length, NameSize);

  Status = CmiAllocateBlock(RegistryHive,
			    (PVOID*) &NewValueCell,
			    sizeof(VALUE_CELL) + NameSize,
			    VBOffset);
  if ((NewValueCell == NULL) || (!NT_SUCCESS(Status)))
    {
      Status = STATUS_INSUFFICIENT_RESOURCES;
    }
  else
    {
      NewValueCell->Id = REG_VALUE_CELL_ID;
      NewValueCell->NameSize = NameSize;
      if (Packable)
	{
	  /* Pack the value name */
	  for (i = 0; i < NameSize; i++)
	    NewValueCell->Name[i] = (CHAR)ValueName->Buffer[i];
	  NewValueCell->Flags |= REG_VALUE_NAME_PACKED;
	}
      else
	{
	  /* Copy the value name */
	  RtlCopyMemory(NewValueCell->Name,
			ValueName->Buffer,
			NameSize);
	  NewValueCell->Flags = 0;
	}
      NewValueCell->DataType = 0;
      NewValueCell->DataSize = 0;
      NewValueCell->DataOffset = 0xffffffff;
      *ValueCell = NewValueCell;
    }

  return Status;
}


NTSTATUS
CmiDestroyValueCell(PREGISTRY_HIVE RegistryHive,
		    PVALUE_CELL ValueCell,
		    BLOCK_OFFSET VBOffset)
{
  NTSTATUS Status;
  PVOID pBlock;
  PHBIN pBin;

  DPRINT("CmiDestroyValueCell(Cell %p  Offset %lx)\n", ValueCell, VBOffset);

  VERIFY_VALUE_CELL(ValueCell);

  /* Destroy the data cell */
  if (ValueCell->DataSize > 4)
    {
      pBlock = CmiGetBlock(RegistryHive, ValueCell->DataOffset, &pBin);
      if (pBlock == NULL)
	{
	  DPRINT("CmiGetBlock() failed\n");
	  return STATUS_UNSUCCESSFUL;
	}

      Status = CmiDestroyBlock(RegistryHive, pBlock, ValueCell->DataOffset);
      if (!NT_SUCCESS(Status))
	{
	  return  Status;
	}

      /* Update time of heap */
      if (!IsVolatileHive(RegistryHive))
	NtQuerySystemTime((PTIME) &pBin->DateModified);
    }

  /* Destroy the value cell */
  Status = CmiDestroyBlock(RegistryHive, ValueCell, VBOffset);

  /* Update time of heap */
  if (!IsVolatileHive(RegistryHive) && CmiGetBlock(RegistryHive, VBOffset, &pBin))
    {
      NtQuerySystemTime((PTIME) &pBin->DateModified);
    }

  return Status;
}


NTSTATUS
CmiAddBin(PREGISTRY_HIVE RegistryHive,
	  PVOID *NewBlock,
	  BLOCK_OFFSET *NewBlockOffset)
{
  PCELL_HEADER tmpBlock;
  PHBIN * tmpBlockList;
  PHBIN tmpBin;

  tmpBin = ExAllocatePool(PagedPool, REG_BLOCK_SIZE);
  if (tmpBin == NULL)
    {
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  tmpBin->BlockId = REG_BIN_ID;
  tmpBin->BlockOffset = RegistryHive->FileSize - REG_BLOCK_SIZE;
  RegistryHive->FileSize += REG_BLOCK_SIZE;
  tmpBin->BlockSize = REG_BLOCK_SIZE;
  tmpBin->Unused1 = 0;
  ZwQuerySystemTime((PTIME) &tmpBin->DateModified);
  tmpBin->Unused2 = 0;

  /* Increase size of list of blocks */
  tmpBlockList = ExAllocatePool(NonPagedPool,
	  sizeof(PHBIN *) * (RegistryHive->BlockListSize + 1));
  if (tmpBlockList == NULL)
    {
      ExFreePool(tmpBin);
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  if (RegistryHive->BlockListSize > 0)
    {
      RtlCopyMemory (tmpBlockList,
		     RegistryHive->BlockList,
		     sizeof(PHBIN *)*(RegistryHive->BlockListSize));
      ExFreePool(RegistryHive->BlockList);
    }

  RegistryHive->BlockList = tmpBlockList;
  RegistryHive->BlockList[RegistryHive->BlockListSize] = tmpBin;
  RegistryHive->BlockListSize++;

  /* Initialize a free block in this heap : */
  tmpBlock = (PCELL_HEADER)((ULONG_PTR) tmpBin + REG_HBIN_DATA_OFFSET);
  tmpBlock->CellSize = (REG_BLOCK_SIZE - REG_HBIN_DATA_OFFSET);

  /* Grow bitmap if necessary */
  if (IsVolatileHive(RegistryHive) &&
      (RegistryHive->BlockListSize % (sizeof(ULONG) * 8) == 0))
    {
      PULONG BitmapBuffer;
      ULONG BitmapSize;

      DPRINT("Grow hive bitmap\n");

      /* Calculate bitmap size in bytes (always a multiple of 32 bits) */
      BitmapSize = ROUND_UP(RegistryHive->BlockListSize, sizeof(ULONG) * 8) / 8;
      DPRINT("RegistryHive->BlockListSize: %lu\n", RegistryHive->BlockListSize);
      DPRINT("BitmapSize:  %lu Bytes  %lu Bits\n", BitmapSize, BitmapSize * 8);
      BitmapBuffer = (PULONG)ExAllocatePool(PagedPool,
					    BitmapSize);
      RtlZeroMemory(BitmapBuffer, BitmapSize);
      RtlCopyMemory(BitmapBuffer,
		    RegistryHive->DirtyBitMap.Buffer,
		    RegistryHive->DirtyBitMap.SizeOfBitMap);
      ExFreePool(RegistryHive->BitmapBuffer);
      RegistryHive->BitmapBuffer = BitmapBuffer;
      RtlInitializeBitMap(&RegistryHive->DirtyBitMap,
			  RegistryHive->BitmapBuffer,
			  BitmapSize * 8);
    }

  *NewBlock = (PVOID) tmpBlock;

  if (NewBlockOffset)
    *NewBlockOffset = tmpBin->BlockOffset + REG_HBIN_DATA_OFFSET;

  /* Mark new bin dirty */
  CmiMarkBinDirty(RegistryHive,
		  tmpBin->BlockOffset);

  return STATUS_SUCCESS;
}


NTSTATUS
CmiAllocateBlock(PREGISTRY_HIVE RegistryHive,
		 PVOID *Block,
		 LONG BlockSize,
		 BLOCK_OFFSET * pBlockOffset)
{
  PCELL_HEADER NewBlock;
  NTSTATUS Status;
  PHBIN pBin;
  ULONG i;
  PVOID Temp;

  Status = STATUS_SUCCESS;

  /* Round to 16 bytes multiple */
  BlockSize = (BlockSize + sizeof(DWORD) + 15) & 0xfffffff0;

  /* Handle volatile hives first */
  if (IsPointerHive(RegistryHive))
    {
      NewBlock = ExAllocatePool(NonPagedPool, BlockSize);

      if (NewBlock == NULL)
	{
	  Status = STATUS_INSUFFICIENT_RESOURCES;
	}
      else
	{
	  RtlZeroMemory(NewBlock, BlockSize);
	  NewBlock->CellSize = BlockSize;
	  *Block = NewBlock;
	  if (pBlockOffset)
	    *pBlockOffset = (BLOCK_OFFSET) NewBlock;
	}
    }
  else
    {
      /* first search in free blocks */
      NewBlock = NULL;
      for (i = 0; i < RegistryHive->FreeListSize; i++)
	{
	  if (RegistryHive->FreeList[i]->CellSize >= BlockSize)
	    {
	      NewBlock = RegistryHive->FreeList[i];
	      if (pBlockOffset)
		*pBlockOffset = RegistryHive->FreeListOffset[i];

	      /* Update time of heap */
	      Temp = CmiGetBlock(RegistryHive, RegistryHive->FreeListOffset[i], &pBin);
	      if (Temp == NULL)
		{
		  DPRINT("CmiGetBlock() failed\n");
		  return STATUS_UNSUCCESSFUL;
		}

	      if (Temp)
		{
		  NtQuerySystemTime((PTIME) &pBin->DateModified);
		  CmiMarkBlockDirty(RegistryHive, RegistryHive->FreeListOffset[i]);
		}

	      if ((i + 1) < RegistryHive->FreeListSize)
		{
		  RtlMoveMemory(&RegistryHive->FreeList[i],
				&RegistryHive->FreeList[i + 1],
				sizeof(RegistryHive->FreeList[0])
				  * (RegistryHive->FreeListSize - i - 1));
		  RtlMoveMemory(&RegistryHive->FreeListOffset[i],
				&RegistryHive->FreeListOffset[i + 1],
				sizeof(RegistryHive->FreeListOffset[0])
				  * (RegistryHive->FreeListSize - i - 1));
		}
	      RegistryHive->FreeListSize--;
	      break;
	    }
	}

      /* Need to extend hive file : */
      if (NewBlock == NULL)
	{
	  /* Add a new block */
	  Status = CmiAddBin(RegistryHive, (PVOID *) &NewBlock , pBlockOffset);
	}

      if (NT_SUCCESS(Status))
	{
	  *Block = NewBlock;

	  /* Split the block in two parts */
	  if (NewBlock->CellSize > BlockSize)
	    {
	      NewBlock = (PCELL_HEADER) ((ULONG_PTR) NewBlock+BlockSize);
	      NewBlock->CellSize = ((PCELL_HEADER) (*Block))->CellSize - BlockSize;
	      CmiAddFree(RegistryHive,
			 NewBlock,
			 *pBlockOffset + BlockSize,
			 TRUE);
	      CmiMarkBlockDirty(RegistryHive,
				*pBlockOffset + BlockSize);
	    }
	  else if (NewBlock->CellSize < BlockSize)
	    {
	      return(STATUS_UNSUCCESSFUL);
	    }

	  RtlZeroMemory(*Block, BlockSize);
	  ((PCELL_HEADER) (*Block))->CellSize = -BlockSize;
	}
    }

  return(Status);
}


NTSTATUS
CmiDestroyBlock(PREGISTRY_HIVE RegistryHive,
		PVOID Block,
		BLOCK_OFFSET Offset)
{
  NTSTATUS Status;
  PHBIN pBin;

  Status = STATUS_SUCCESS;

  if (IsPointerHive(RegistryHive))
    {
      ExFreePool(Block);
    }
  else
    {
      PCELL_HEADER pFree = Block;

      if (pFree->CellSize < 0)
        pFree->CellSize = -pFree->CellSize;

      /* Clear block (except the block size) */
      RtlZeroMemory(((PVOID)pFree) + sizeof(ULONG),
		    pFree->CellSize - sizeof(ULONG));

      /* Add block to the list of free blocks */
      CmiAddFree(RegistryHive, Block, Offset, TRUE);

      /* Update time of heap */
      if (!IsVolatileHive(RegistryHive) && CmiGetBlock(RegistryHive, Offset,&pBin))
	NtQuerySystemTime((PTIME) &pBin->DateModified);

      CmiMarkBlockDirty(RegistryHive, Offset);
    }

  return Status;
}


static BOOLEAN
CmiMergeFree(PREGISTRY_HIVE RegistryHive,
	     PCELL_HEADER FreeBlock,
	     BLOCK_OFFSET FreeOffset)
{
  BLOCK_OFFSET BlockOffset;
  BLOCK_OFFSET BinOffset;
  ULONG BlockSize;
  ULONG BinSize;
  PHBIN Bin;
  ULONG i;

  DPRINT("CmiMergeFree(Block %lx  Offset %lx  Size %lx) called\n",
	 FreeBlock, FreeOffset, FreeBlock->CellSize);

  CmiGetBlock(RegistryHive,
	      FreeOffset,
	      &Bin);
  DPRINT("Bin %p\n", Bin);
  if (Bin == NULL)
    return(FALSE);

  BinOffset = Bin->BlockOffset;
  BinSize = Bin->BlockSize;
  DPRINT("Bin %p  Offset %lx  Size %lx\n", Bin, BinOffset, BinSize);

  for (i = 0; i < RegistryHive->FreeListSize; i++)
    {
      BlockOffset = RegistryHive->FreeListOffset[i];
      BlockSize = RegistryHive->FreeList[i]->CellSize;
      if (BlockOffset > BinOffset &&
	  BlockOffset < BinOffset + BinSize)
	{
	  DPRINT("Free block: Offset %lx  Size %lx\n",
		  BlockOffset, BlockSize);

	  if ((i < (RegistryHive->FreeListSize - 1)) &&
	      (BlockOffset + BlockSize == FreeOffset) &&
	      (FreeOffset + FreeBlock->CellSize == RegistryHive->FreeListOffset[i + 1]))
	    {
	      DPRINT("Merge current block with previous and next block\n");

	      RegistryHive->FreeList[i]->CellSize +=
		(FreeBlock->CellSize + RegistryHive->FreeList[i + 1]->CellSize);

	      FreeBlock->CellSize = 0;
	      RegistryHive->FreeList[i + 1]->CellSize = 0;


	      if ((i + 2) < RegistryHive->FreeListSize)
		{
		  RtlMoveMemory(&RegistryHive->FreeList[i + 1],
				&RegistryHive->FreeList[i + 2],
				sizeof(RegistryHive->FreeList[0])
				  * (RegistryHive->FreeListSize - i - 2));
		  RtlMoveMemory(&RegistryHive->FreeListOffset[i + 1],
				&RegistryHive->FreeListOffset[i + 2],
				sizeof(RegistryHive->FreeListOffset[0])
				  * (RegistryHive->FreeListSize - i - 2));
		}
	      RegistryHive->FreeListSize--;

	      CmiMarkBlockDirty(RegistryHive, BlockOffset);

	      return(TRUE);
	    }
	  else if (BlockOffset + BlockSize == FreeOffset)
	    {
	      DPRINT("Merge current block with previous block\n");

	      RegistryHive->FreeList[i]->CellSize += FreeBlock->CellSize;
	      FreeBlock->CellSize = 0;

	      CmiMarkBlockDirty(RegistryHive, BlockOffset);

	      return(TRUE);
	    }
	  else if (FreeOffset + FreeBlock->CellSize == BlockOffset)
	    {
	      DPRINT("Merge current block with next block\n");

	      FreeBlock->CellSize += RegistryHive->FreeList[i]->CellSize;
	      RegistryHive->FreeList[i]->CellSize = 0;
	      RegistryHive->FreeList[i] = FreeBlock;
	      RegistryHive->FreeListOffset[i] = FreeOffset;

	      CmiMarkBlockDirty(RegistryHive, FreeOffset);

	      return(TRUE);
	    }
	}
    }

  return(FALSE);
}


NTSTATUS
CmiAddFree(PREGISTRY_HIVE RegistryHive,
	   PCELL_HEADER FreeBlock,
	   BLOCK_OFFSET FreeOffset,
	   BOOLEAN MergeFreeBlocks)
{
  PCELL_HEADER *tmpList;
  BLOCK_OFFSET *tmpListOffset;
  LONG minInd;
  LONG maxInd;
  LONG medInd;

  assert(RegistryHive);
  assert(FreeBlock);

  DPRINT("FreeBlock %.08lx  FreeOffset %.08lx\n",
	 FreeBlock, FreeOffset);

  /* Merge free blocks */
  if (MergeFreeBlocks == TRUE)
    {
      if (CmiMergeFree(RegistryHive, FreeBlock, FreeOffset))
	return(STATUS_SUCCESS);
    }

  if ((RegistryHive->FreeListSize + 1) > RegistryHive->FreeListMax)
    {
      tmpList = ExAllocatePool(PagedPool,
			  sizeof(PCELL_HEADER) * (RegistryHive->FreeListMax + 32));
      if (tmpList == NULL)
	return STATUS_INSUFFICIENT_RESOURCES;

      tmpListOffset = ExAllocatePool(PagedPool,
			  sizeof(BLOCK_OFFSET) * (RegistryHive->FreeListMax + 32));

      if (tmpListOffset == NULL)
	{
	  ExFreePool(tmpList);
	  return STATUS_INSUFFICIENT_RESOURCES;
	}

      if (RegistryHive->FreeListMax)
	{
	  RtlMoveMemory(tmpList,
			RegistryHive->FreeList,
			sizeof(PCELL_HEADER) * (RegistryHive->FreeListMax));
	  RtlMoveMemory(tmpListOffset,
			RegistryHive->FreeListOffset,
			sizeof(BLOCK_OFFSET) * (RegistryHive->FreeListMax));
	  ExFreePool(RegistryHive->FreeList);
	  ExFreePool(RegistryHive->FreeListOffset);
	}
      RegistryHive->FreeList = tmpList;
      RegistryHive->FreeListOffset = tmpListOffset;
      RegistryHive->FreeListMax += 32;
    }

  /* Add new offset to free list, maintaining list in ascending order */
  if ((RegistryHive->FreeListSize == 0)
     || (RegistryHive->FreeListOffset[RegistryHive->FreeListSize-1] < FreeOffset))
    {
      /* Add to end of list */
      RegistryHive->FreeList[RegistryHive->FreeListSize] = FreeBlock;
      RegistryHive->FreeListOffset[RegistryHive->FreeListSize++] = FreeOffset;
    }
  else if (RegistryHive->FreeListOffset[0] > FreeOffset)
    {
      /* Add to begin of list */
      RtlMoveMemory(&RegistryHive->FreeList[1],
		    &RegistryHive->FreeList[0],
		    sizeof(RegistryHive->FreeList[0]) * RegistryHive->FreeListSize);
      RtlMoveMemory(&RegistryHive->FreeListOffset[1],
		    &RegistryHive->FreeListOffset[0],
		    sizeof(RegistryHive->FreeListOffset[0]) * RegistryHive->FreeListSize);
      RegistryHive->FreeList[0] = FreeBlock;
      RegistryHive->FreeListOffset[0] = FreeOffset;
      RegistryHive->FreeListSize++;
    }
  else
    {
      /* Search where to insert */
      minInd = 0;
      maxInd = RegistryHive->FreeListSize - 1;
      while ((maxInd - minInd) > 1)
	{
	  medInd = (minInd + maxInd) / 2;
	  if (RegistryHive->FreeListOffset[medInd] > FreeOffset)
	    maxInd = medInd;
	  else
	    minInd = medInd;
	}

      /* Insert before maxInd */
      RtlMoveMemory(&RegistryHive->FreeList[maxInd+1],
		    &RegistryHive->FreeList[maxInd],
		    sizeof(RegistryHive->FreeList[0]) * (RegistryHive->FreeListSize - minInd));
      RtlMoveMemory(&RegistryHive->FreeListOffset[maxInd + 1],
		    &RegistryHive->FreeListOffset[maxInd],
		    sizeof(RegistryHive->FreeListOffset[0]) * (RegistryHive->FreeListSize-minInd));
      RegistryHive->FreeList[maxInd] = FreeBlock;
      RegistryHive->FreeListOffset[maxInd] = FreeOffset;
      RegistryHive->FreeListSize++;
    }

  return STATUS_SUCCESS;
}


PVOID
CmiGetBlock(PREGISTRY_HIVE RegistryHive,
	    BLOCK_OFFSET BlockOffset,
	    PHBIN * ppBin)
{
  PHBIN pBin;

  if (ppBin)
    *ppBin = NULL;

  if (BlockOffset == (BLOCK_OFFSET)-1)
    {
      return NULL;
    }

  if (IsPointerHive (RegistryHive))
    {
      return (PVOID)BlockOffset;
    }
  else
    {
      if (BlockOffset > RegistryHive->BlockListSize * 4096)
	{
	  DPRINT1("BlockOffset exceeds valid range (%lu > %lu)\n",
		  BlockOffset, RegistryHive->BlockListSize * 4096);
	  return NULL;
	}
      pBin = RegistryHive->BlockList[BlockOffset / 4096];
      if (ppBin)
	*ppBin = pBin;
      return((PVOID)((ULONG_PTR)pBin + (BlockOffset - pBin->BlockOffset)));
    }
}


VOID
CmiMarkBlockDirty(PREGISTRY_HIVE RegistryHive,
		  BLOCK_OFFSET BlockOffset)
{
  PDATA_CELL Cell;
  LONG CellSize;
  ULONG BlockNumber;
  ULONG BlockCount;

  if (IsVolatileHive(RegistryHive))
    return;

  DPRINT("CmiMarkBlockDirty(Offset 0x%lx)\n", (ULONG)BlockOffset);

  BlockNumber = (ULONG)BlockOffset / 4096;

  Cell = CmiGetBlock(RegistryHive,
		     BlockOffset,
		     NULL);

  CellSize = Cell->CellSize;
  if (CellSize < 0)
    CellSize = -CellSize;

  BlockCount = (ROUND_UP(BlockOffset + CellSize, 4096) - ROUND_DOWN(BlockOffset, 4096)) / 4096;

  DPRINT("  BlockNumber %lu  Size %lu (%s)  BlockCount %lu\n",
	 BlockNumber,
	 CellSize,
	 (Cell->CellSize < 0) ? "used" : "free",
	 BlockCount);

  RegistryHive->HiveDirty = TRUE;
  RtlSetBits(&RegistryHive->DirtyBitMap,
	     BlockNumber,
	     BlockCount);
}


VOID
CmiMarkBinDirty(PREGISTRY_HIVE RegistryHive,
		BLOCK_OFFSET BinOffset)
{
  ULONG BlockNumber;
  ULONG BlockCount;
  PHBIN Bin;

  if (IsVolatileHive(RegistryHive))
    return;

  DPRINT("CmiMarkBinDirty(Offset 0x%lx)\n", (ULONG)BinOffset);

  BlockNumber = (ULONG)BinOffset / 4096;

  Bin = RegistryHive->BlockList[BlockNumber];

  BlockCount = Bin->BlockSize / 4096;

  DPRINT("  BlockNumber %lu  Size %lu  BlockCount %lu\n",
	 BlockNumber,
	 Bin->BlockSize,
	 BlockCount);

  RegistryHive->HiveDirty = TRUE;
  RtlSetBits(&RegistryHive->DirtyBitMap,
	     BlockNumber,
	     BlockCount);
}


ULONG
CmiGetPackedNameLength(IN PUNICODE_STRING Name,
		       OUT PBOOLEAN Packable)
{
  ULONG i;

  if (Packable != NULL)
    *Packable = TRUE;

  for (i = 0; i < Name->Length; i++)
    {
      if (Name->Buffer[i] > 0xFF)
	{
	  if (Packable != NULL)
	    *Packable = FALSE;
	  return(Name->Length);
	}
    }

  return(Name->Length / sizeof(WCHAR));
}


BOOLEAN
CmiComparePackedNames(IN PUNICODE_STRING Name,
		      IN PCHAR NameBuffer,
		      IN USHORT NameBufferSize,
		      IN BOOLEAN NamePacked)
{
  PWCHAR UNameBuffer;
  ULONG i;

  if (NamePacked == TRUE)
    {
      if (Name->Length != NameBufferSize * sizeof(WCHAR))
	return(FALSE);

      for (i = 0; i < Name->Length / sizeof(WCHAR); i++)
	{
	  if (RtlUpcaseUnicodeChar(Name->Buffer[i]) != RtlUpcaseUnicodeChar((WCHAR)NameBuffer[i]))
	    return(FALSE);
	}
    }
  else
    {
      if (Name->Length != NameBufferSize)
	return(FALSE);

      UNameBuffer = (PWCHAR)NameBuffer;

      for (i = 0; i < Name->Length / sizeof(WCHAR); i++)
	{
	  if (RtlUpcaseUnicodeChar(Name->Buffer[i]) != RtlUpcaseUnicodeChar(UNameBuffer[i]))
	    return(FALSE);
	}
    }

  return(TRUE);
}


VOID
CmiCopyPackedName(PWCHAR NameBuffer,
		  PCHAR PackedNameBuffer,
		  ULONG PackedNameSize)
{
  ULONG i;

  for (i = 0; i < PackedNameSize; i++)
    NameBuffer[i] = (WCHAR)PackedNameBuffer[i];
}

/* EOF */
