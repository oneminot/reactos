/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * $Id: section.c,v 1.81.2.1 2002/05/13 20:37:00 chorns Exp $
 * PROJECT: ReactOS kernel
 * FILE: ntoskrnl/mm/section.c
 * PURPOSE: Implements section objects
 * PROGRAMMER: David Welch (welch@mcmail.com)
 *             Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY: Created 22/05/98 
 */

/*
 * INCLUDES
 * ****************************************************************
 */

#include <limits.h>
#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/io.h>
#include <internal/ps.h>
#include <internal/pool.h>
#include <internal/cc.h>
#include <ddk/ntifs.h>

#define NDEBUG
#include <internal/debug.h>

/*
 * GLOBALS
 * ******************************************************************
 */

POBJECT_TYPE EXPORTED MmSectionObjectType = NULL;

static GENERIC_MAPPING MmpSectionMapping = {
  STANDARD_RIGHTS_READ | SECTION_MAP_READ | SECTION_QUERY,
  STANDARD_RIGHTS_WRITE | SECTION_MAP_WRITE,
  STANDARD_RIGHTS_EXECUTE | SECTION_MAP_EXECUTE,
  SECTION_ALL_ACCESS
};

#define TAG_MM_SECTION_SEGMENT   TAG('M', 'M', 'S', 'S')
#define TAG_SECTION_PAGE_TABLE   TAG('M', 'S', 'P', 'T')

#define PAGE_FROM_SSE(E)         ((E) & 0xFFFFF000)
#define SHARE_COUNT_FROM_SSE(E)  (((E) & 0x00000FFE) >> 1)
#define IS_SWAP_FROM_SSE(E)      ((E) & 0x00000001)
#define MAX_SHARE_COUNT          0x7FF
#define MAKE_SSE(P, C)           ((P) | ((C) << 1))
#define SWAPENTRY_FROM_SSE(E)    ((E) >> 1)
#define MAKE_SWAP_SSE(S)         (((S) << 1) | 0x1)

/*
 * FUNCTIONS
 * ****************************************************************
 */

VOID
MmFreePageTablesSectionSegment (PMM_SECTION_SEGMENT Segment)
{
  ULONG i;

  for (i = 0; i < NR_SECTION_PAGE_TABLES; i++)
    {
      if (Segment->PageDirectory.PageTables[i] != NULL)
	{
	  ExFreePool (Segment->PageDirectory.PageTables[i]);
	}
    }
}

VOID
MmFreeSectionSegments (PFILE_OBJECT FileObject)
{
  if (FileObject->SectionObjectPointers->ImageSectionObject != NULL)
    {
      PMM_IMAGE_SECTION_OBJECT ImageSectionObject;

      ULONG i;

      ImageSectionObject =
	(PMM_IMAGE_SECTION_OBJECT) FileObject->
	SectionObjectPointers->ImageSectionObject;

      for (i = 0; i < ImageSectionObject->NrSegments; i++)
	{
	  if (ImageSectionObject->Segments[i].ReferenceCount != 0)
	    {
	      DPRINT1 ("Image segment %d still referenced (was %d)\n", i,
		       ImageSectionObject->Segments[i].ReferenceCount);
	      KeBugCheck (0);
	    }
	  MmFreePageTablesSectionSegment (&ImageSectionObject->Segments[i]);
	}
      ExFreePool (ImageSectionObject);
      FileObject->SectionObjectPointers->ImageSectionObject = NULL;
    }
  if (FileObject->SectionObjectPointers->DataSectionObject != NULL)
    {
      PMM_SECTION_SEGMENT Segment;

      Segment =
	(PMM_SECTION_SEGMENT) FileObject->
	SectionObjectPointers->DataSectionObject;

      assertmsg (Segment->ReferenceCount == 0,
		 ("Data segment still referenced\n"));

      MmFreePageTablesSectionSegment (Segment);
      ExFreePool (Segment);
      FileObject->SectionObjectPointers->DataSectionObject = NULL;
    }
}

NTSTATUS
MmWritePageSectionView (PMADDRESS_SPACE AddressSpace,
			PMEMORY_AREA MArea, PVOID Address)
{
  return (STATUS_UNSUCCESSFUL);
}

VOID
MmLockSection (PSECTION_OBJECT Section)
{
  KeWaitForSingleObject (&Section->Lock,
			 UserRequest, KernelMode, FALSE, NULL);
}

VOID
MmUnlockSection (PSECTION_OBJECT Section)
{
  KeReleaseMutex (&Section->Lock, FALSE);
}

VOID
MmLockSectionSegment (PMM_SECTION_SEGMENT Segment)
{
  KeWaitForSingleObject (&Segment->Lock,
			 UserRequest, KernelMode, FALSE, NULL);
}

VOID
MmUnlockSectionSegment (PMM_SECTION_SEGMENT Segment)
{
  KeReleaseMutex (&Segment->Lock, FALSE);
}

VOID
MmSetPageEntrySectionSegment (IN PMM_SECTION_SEGMENT Segment,
			      IN ULONG Offset, IN ULONG_PTR Entry)
{
  PSECTION_PAGE_TABLE Table;
  ULONG DirectoryOffset;
  ULONG TableOffset;

  DirectoryOffset = PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET (Offset);
  Table = Segment->PageDirectory.PageTables[DirectoryOffset];
  if (Table == NULL)
    {
      Table = Segment->PageDirectory.PageTables[DirectoryOffset] =
	ExAllocatePoolWithTag (NonPagedPool,
			       sizeof (SECTION_PAGE_TABLE),
			       TAG_SECTION_PAGE_TABLE);
      memset (Table, 0, sizeof (SECTION_PAGE_TABLE));
      DPRINT ("Table %x\n", Table);
    }
  TableOffset = PAGE_TO_SECTION_PAGE_TABLE_OFFSET (Offset);
  Table->Entry[TableOffset] = Entry;
}


ULONG_PTR
MmGetPageEntrySectionSegment (IN PMM_SECTION_SEGMENT Segment, IN ULONG Offset)
{
  PSECTION_PAGE_TABLE Table;
  ULONG_PTR Entry;
  ULONG DirectoryOffset;
  ULONG TableOffset;

  DPRINT ("MmGetPageEntrySection(Offset %x)\n", Offset);

  DirectoryOffset = PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET (Offset);
  Table = Segment->PageDirectory.PageTables[DirectoryOffset];
  DPRINT ("Table %x\n", Table);
  if (Table == NULL)
    {
      return (0);
    }
  TableOffset = PAGE_TO_SECTION_PAGE_TABLE_OFFSET (Offset);
  Entry = Table->Entry[TableOffset];
  return (Entry);
}

VOID
MmSharePageEntrySectionSegment (IN PMM_SECTION_SEGMENT Segment,
				IN ULONG Offset)
{
  ULONG_PTR Entry;

  Entry = MmGetPageEntrySectionSegment (Segment, Offset);

  assertmsg (Entry != 0, ("Entry == 0 for MmSharePageEntrySectionSegment\n"));

  assertmsg (SHARE_COUNT_FROM_SSE (Entry) < MAX_SHARE_COUNT,
	     ("Maximum share count reached\n"));

  assert (!IS_SWAP_FROM_SSE (Entry));

  Entry = MAKE_SSE (PAGE_FROM_SSE (Entry), SHARE_COUNT_FROM_SSE (Entry) + 1);
  MmSetPageEntrySectionSegment (Segment, Offset, Entry);
}

BOOLEAN
MmUnsharePageEntrySectionSegment (PSECTION_OBJECT Section,
				  PMM_SECTION_SEGMENT Segment,
				  ULONG Offset, BOOLEAN Dirty)
{
  ULONG Entry;

  Entry = MmGetPageEntrySectionSegment (Segment, Offset);

  assertmsg (Entry != 0, ("Entry == 0 for MmSharePageEntrySectionSegment\n"));

  assertmsg (SHARE_COUNT_FROM_SSE (Entry) != 0,
	     ("Zero share count for unshare\n"));

  assert (!IS_SWAP_FROM_SSE (Entry));

  Entry = MAKE_SSE (PAGE_FROM_SSE (Entry), SHARE_COUNT_FROM_SSE (Entry) - 1);
  /*
   * If we reducing the share count of this entry to zero then set the entry 
   * to zero and tell the cache the page is no longer mapped.
   */
  if (SHARE_COUNT_FROM_SSE (Entry) == 0)
    {
      PREACTOS_COMMON_FCB_HEADER Fcb;
      PFILE_OBJECT FileObject;
      NTSTATUS Status;

      FileObject = Section->FileObject;

      MmSetPageEntrySectionSegment (Segment, Offset, 0);

      if (FileObject != NULL)
	{
	  Fcb = (PREACTOS_COMMON_FCB_HEADER) FileObject->FsContext;

	  if (FileObject->Flags & FO_DIRECT_CACHE_PAGING_READ &&
	      (Offset % PAGESIZE) == 0)
	    {
	      Status = CcRosUnmapCacheSegment (Fcb->Bcb, Offset, Dirty);
	      if (!NT_SUCCESS (Status))
		{
		  KeBugCheck (0);
		}
	    }
	}
    }
  else
    {
      MmSetPageEntrySectionSegment (Segment, Offset, Entry);
    }
  return (SHARE_COUNT_FROM_SSE (Entry) > 1);
}

NTSTATUS
MiReadPage (PMEMORY_AREA MemoryArea, PLARGE_INTEGER Offset, PULONG_PTR Page)
     /*
      * FUNCTION: Read a page for a section backed memory area.
      * PARAMETERS:
      *       MemoryArea - Memory area to read the page for.
      *       Offset - Offset of the page to read.
      *       Page - Variable that receives a page contains the read data.
      */
{
  IO_STATUS_BLOCK IoStatus;
  PFILE_OBJECT FileObject;
  PMDL Mdl;
  NTSTATUS Status;
  PREACTOS_COMMON_FCB_HEADER Fcb;

  FileObject = MemoryArea->Data.SectionData.Section->FileObject;
  Fcb = (PREACTOS_COMMON_FCB_HEADER) FileObject->FsContext;

  /*
   * If the file system is letting us go directly to the cache and the
   * memory area was mapped at an offset in the file which is page aligned
   * then get the related cache segment.
   */
  if (FileObject->Flags & FO_DIRECT_CACHE_PAGING_READ &&
      (Offset->QuadPart % PAGESIZE) == 0)
    {
      ULONG BaseOffset;
      PVOID BaseAddress;
      BOOLEAN UptoDate;
      PCACHE_SEGMENT CacheSeg;
      LARGE_INTEGER SegOffset;
      PHYSICAL_ADDRESS Addr;

      /*
       * Get the related cache segment; we use a lower level interface than
       * filesystems do because it is safe for us to use an offset with a
       * alignment less than the file system block size.
       */
      Status = CcRosGetCacheSegment (Fcb->Bcb,
				     (ULONG) Offset->QuadPart,
				     &BaseOffset,
				     &BaseAddress, &UptoDate, &CacheSeg);
      if (!NT_SUCCESS (Status))
	{
	  return (Status);
	}
      if (!UptoDate)
	{
	  /*
	   * If the cache segment isn't up to date then call the file
	   * system to read in the data.
	   */

	  Mdl = MmCreateMdl (NULL, BaseAddress, Fcb->Bcb->CacheSegmentSize);
	  MmBuildMdlForNonPagedPool (Mdl);
	  SegOffset.QuadPart = BaseOffset;
	  Status = IoPageRead (FileObject, Mdl, &SegOffset, &IoStatus, TRUE);
	  if (!NT_SUCCESS (Status) && Status != STATUS_END_OF_FILE)
	    {
	      CcRosReleaseCacheSegment (Fcb->Bcb, CacheSeg,
					FALSE, FALSE, FALSE);
	      return (Status);
	    }
	}
      /*
       * Retrieve the page from the cache segment that we actually want.
       */
      Addr = MmGetPhysicalAddress (BaseAddress +
				   Offset->QuadPart - BaseOffset);
      (*Page) = (ULONG_PTR) Addr.QuadPart;
      MmReferencePage ((*Page));

      CcRosReleaseCacheSegment (Fcb->Bcb, CacheSeg, TRUE, FALSE, TRUE);
      return (STATUS_SUCCESS);
    }
  else
    {
      /*
       * Allocate a page, this is rather complicated by the possibility
       * we might have to move other things out of memory
       */
      Status = MmRequestPageMemoryConsumer (MC_USER, TRUE, Page);
      if (!NT_SUCCESS (Status))
	{
	  return (Status);
	}

      /*
       * Create an mdl to hold the page we are going to read data into.
       */
      Mdl = MmCreateMdl (NULL, NULL, PAGESIZE);
      MmBuildMdlFromPages (Mdl, Page);
      /*
       * Call the FSD to read the page
       */
      Status = IoPageRead (FileObject, Mdl, Offset, &IoStatus, TRUE);
      return (Status);
    }
}

VOID
MmSectionDeleteMapping (IN PVOID Context,
			IN PEPROCESS Process, IN PVOID Address)
{
  PMADDRESS_SPACE AddressSpace;
  PMM_SECTION_SEGMENT Segment;
  ULONG_PTR PhysicalAddress;
  PSECTION_OBJECT Section;
  PMEMORY_AREA MemoryArea;
  LARGE_INTEGER Offset;
  BOOLEAN WasDirty;
  BOOLEAN Private;
  ULONG_PTR Entry;

  DPRINT ("MmSectionDeleteMapping(Address 0x%.08x)\n", Address);

  assert (Context == NULL);

  assertmsg (((ULONG_PTR) Address & (PAGESIZE - 1)) == 0,
	     ("Address 0x%.08x is not page aligned.\n", Address));

  if ((ULONG_PTR) Address >= KERNEL_BASE)
    {
      AddressSpace = MmGetKernelAddressSpace ();
    }
  else
    {
      AddressSpace = &PsGetCurrentProcess ()->AddressSpace;
    }

  MemoryArea = MmOpenMemoryAreaByAddress (AddressSpace, Address);

  assertmsg (MemoryArea != NULL,
	     ("MemoryArea for address 0x%.08x does not exist.\n", Address));

  Offset.QuadPart =
    (ULONG_PTR) ((ULONG_PTR) Address -
		 (ULONG_PTR) MemoryArea->BaseAddress) +
    MemoryArea->Data.SectionData.ViewOffset;

  Segment = MemoryArea->Data.SectionData.Segment;
  Section = MemoryArea->Data.SectionData.Section;

  Entry = MmGetPageEntrySectionSegment (Segment, Offset.QuadPart);
  PhysicalAddress =
    MmGetPhysicalAddressForProcess (AddressSpace->Process, Address);

  if (Segment->Characteristics & IMAGE_SECTION_CHAR_BSS ||
      IS_SWAP_FROM_SSE (Entry) ||
      (ULONG_PTR) (PAGE_FROM_SSE (Entry)) != PhysicalAddress)
    {
      Private = TRUE;
    }
  else
    {
      Private = FALSE;
    }

  DPRINT ("MmSectionDeleteMapping(Address 0x%.08x) Private %d\n",
	  Address, Private);

  MmDeleteVirtualMapping (Process,
			  Address, FALSE, &WasDirty, &PhysicalAddress);

  if (!Private)
    {
      MmUnsharePageEntrySectionSegment (Section,
					Segment, Offset.u.LowPart, WasDirty);
    }

  MmCloseMemoryArea (MemoryArea);

  MmDereferencePage (PhysicalAddress);
}


NTSTATUS
MmNotPresentFaultSectionView (PMADDRESS_SPACE AddressSpace,
			      MEMORY_AREA * MemoryArea,
			      PVOID Address, BOOLEAN Locked)
{
  ULONG Attributes;
  LARGE_INTEGER Offset;
  ULONG_PTR Page;
  NTSTATUS Status;
  PVOID PAddress;
  PSECTION_OBJECT Section;
  PMM_SECTION_SEGMENT Segment;
  ULONG Entry;
  ULONG Entry1;
  PMM_PAGEOP PageOp;

  DPRINT ("MmNotPresentFaultSectionView(Address 0x%.08x,  Locked %d)\n",
	  Address, Locked);

  assert_irql (PASSIVE_LEVEL);

  /*
   * There is a window between taking the page fault and locking the
   * address space when another thread could load the page so we check
   * that.
   */
  if (MmIsPagePresent (NULL, Address))
    {
      if (Locked)
	{
	  MmLockPage (MmGetPhysicalAddressForProcess (NULL, Address));
	}
      DPRINT ("\n");
      return (STATUS_SUCCESS);
    }

  PAddress = (PVOID) PAGE_ROUND_DOWN (((ULONG_PTR) Address));
  Offset.QuadPart =
    ((ULONG_PTR) PAddress - (ULONG_PTR) MemoryArea->BaseAddress) +
    MemoryArea->Data.SectionData.ViewOffset;

  /*
   * Lock the segment
   */
  Section = MemoryArea->Data.SectionData.Section;
  Segment = MemoryArea->Data.SectionData.Segment;
  MmLockSection (Section);
  MmLockSectionSegment (Segment);

  /*
   * Get or create a page operation descriptor
   */
  PageOp = MmGetPageOp (MemoryArea, 0, 0, Segment, Offset.u.LowPart,
			MM_PAGEOP_PAGEIN);
  assertmsg (PageOp != NULL, ("MmGetPageOp() failed\n"));

  /*
   * Check if someone else is already handling this fault, if so wait
   * for them
   */
  if (PageOp->Thread != PsGetCurrentThread ())
    {
      MmUnlockSectionSegment (Segment);
      MmUnlockSection (Section);
      MmUnlockAddressSpace (AddressSpace);
      Status = KeWaitForSingleObject (&PageOp->CompletionEvent,
				      0, KernelMode, FALSE, NULL);
      /*
       * * Check for various strange conditions
       */
      assertmsg (Status == STATUS_SUCCESS, ("Failed to wait for page op\n"));

      assertmsg (PageOp->Status != STATUS_PENDING,
		 ("Woke for page op before completion\n"));

      /*
       * If this wasn't a pagein then restart the operation
       */
      if (PageOp->OpType != MM_PAGEOP_PAGEIN)
	{
	  MmReleasePageOp (PageOp);
	  MmLockAddressSpace (AddressSpace);
	  DPRINT ("Address 0x%.8X\n", Address);
	  return (STATUS_MM_RESTART_OPERATION);
	}
      /*
       * If the thread handling this fault has failed then we don't retry
       */
      if (!NT_SUCCESS (PageOp->Status))
	{
	  assertmsg (FALSE,
		     ("Page operation completed with status 0x%.08x\n",
		      PageOp->Status));
	  Status = PageOp->Status;
	  MmReleasePageOp (PageOp);
	  MmLockAddressSpace (AddressSpace);
	  DPRINT ("Address 0x%.8X\n", Address);
	  return (Status);
	}
      MmLockAddressSpace (AddressSpace);
      MmLockSection (Section);
      MmLockSectionSegment (Segment);
      /*
       * * If the completed fault was for another address space then set the 
       * * page in this one.
       */
      if (!MmIsPagePresent (NULL, Address))
	{
	  Entry = MmGetPageEntrySectionSegment (Segment, Offset.u.LowPart);
	  if (Entry == 0)
	    {
	      MmReleasePageOp (PageOp);
	      MmUnlockSectionSegment (Segment);
	      MmUnlockSection (Section);
	      return (STATUS_MM_RESTART_OPERATION);
	    }

	  Page = (ULONG_PTR) PAGE_FROM_SSE (Entry);
	  MmReferencePage (Page);
	  MmSharePageEntrySectionSegment (Segment, Offset.u.LowPart);

	  Status = MmCreateVirtualMapping (PsGetCurrentProcess
					   (), Address,
					   MemoryArea->
					   Attributes, Page, FALSE);
	  if (!NT_SUCCESS (Status))
	    {
	      DbgPrint ("Unable to create virtual mapping\n");
	      KeBugCheck (0);
	    }
	  MmInsertRmap (Page, PsGetCurrentProcess (),
			(PVOID) PAGE_ROUND_DOWN (Address),
			MmSectionDeleteMapping, NULL);
	}
      if (Locked)
	{
	  MmLockPage (MmGetPhysicalAddressForProcess (NULL, Address));
	}
      MmReleasePageOp (PageOp);
      MmUnlockSectionSegment (Segment);
      MmUnlockSection (Section);
      DPRINT ("Address 0x%.8X\n", Address);
      return (STATUS_SUCCESS);
    }

  /*
   * Must be private page we have swapped out.
   */
  if (MmIsPageSwapEntry (NULL, PAddress))
    {
      SWAPENTRY SwapEntry;
      PMDL Mdl;

      MmUnlockSectionSegment (Segment);
      MmUnlockSection (Section);

      MmDeletePageFileMapping (NULL, PAddress, &SwapEntry);

      Status = MmRequestPageMemoryConsumer (MC_USER, FALSE, &Page);
      assertmsg (NT_SUCCESS (Status),
		 ("MmRequestPageMemoryConsumer() failed with status 0x%.08x\n",
		  Status));
      while (Status == STATUS_NO_MEMORY)
	{
	  MmUnlockAddressSpace (AddressSpace);
	  Status = MmRequestPageMemoryConsumer (MC_USER, TRUE, &Page);
	  MmLockAddressSpace (AddressSpace);
	}

      MmUnlockAddressSpace (AddressSpace);

      Mdl = MmCreateMdl (NULL, NULL, PAGESIZE);
      MmBuildMdlFromPages (Mdl, (PULONG_PTR) & Page);
      Status = MmReadFromSwapPage (SwapEntry, Mdl);
      MmLockAddressSpace (AddressSpace);
      assertmsg (NT_SUCCESS (Status),
		 ("MmReadFromSwapPage() failed with status 0x%.08x\n",
		  Status));
      if (!NT_SUCCESS (Status))
	{
	  PageOp->Status = Status;
	  KeSetEvent (&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
	  MmReleasePageOp (PageOp);
	  return (Status);
	}

      Status = MmCreateVirtualMapping (PsGetCurrentProcess (),
				       Address,
				       MemoryArea->Attributes, Page, FALSE);
      while (Status == STATUS_NO_MEMORY)
	{
	  MmUnlockAddressSpace (AddressSpace);
	  Status = MmCreateVirtualMapping (PsGetCurrentProcess
					   (), Address,
					   MemoryArea->
					   Attributes, Page, TRUE);
	  MmLockAddressSpace (AddressSpace);
	}
      assertmsg (NT_SUCCESS (Status),
		 ("MmCreateVirtualMapping() failed with status 0x%.08x\n",
		  Status));

      /*
       * * Store the swap entry for later use.
       */
      MmSetSavedSwapEntryPage (Page, SwapEntry);

      /*
       * * Add the page to the process's working set
       */
      MmInsertRmap (Page, PsGetCurrentProcess (),
		    (PVOID) PAGE_ROUND_DOWN (Address),
		    MmSectionDeleteMapping, NULL);

      /*
       * * Finish the operation
       */
      if (Locked)
	{
	  MmLockPage (MmGetPhysicalAddressForProcess (NULL, Address));
	}
      PageOp->Status = STATUS_SUCCESS;
      KeSetEvent (&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
      MmReleasePageOp (PageOp);
      DPRINT ("Address 0x%.8X\n", Address);
      return (STATUS_SUCCESS);
    }

  /*
   * Satisfying a page fault on a map of /Device/PhysicalMemory is easy
   */
  if (Section->Flags & SO_PHYSICAL_MEMORY)
    {
      /*
       * * Just map the desired physical page 
       */

      MmUnlockSectionSegment (Segment);
      MmUnlockSection (Section);

      Status = MmCreateVirtualMapping (PsGetCurrentProcess (),
				       Address,
				       MemoryArea->Attributes, Page, FALSE);
      assertmsg (NT_SUCCESS (Status),
		 ("MmCreateVirtualMapping() failed with status 0x%.08x\n",
		  Status));
      while (!NT_SUCCESS (Status))
	{
	  MmUnlockAddressSpace (AddressSpace);
	  Status = MmCreateVirtualMapping (PsGetCurrentProcess
					   (), Address,
					   MemoryArea->
					   Attributes, Page, TRUE);
	  MmLockAddressSpace (AddressSpace);
	}

      /*
       * Don't add an rmap entry since the page mapped could be for 
       * anything. 
       */
      if (Locked)
	{
	  MmLockPage (MmGetPhysicalAddressForProcess (NULL, Address));
	}

      /*
       * * Cleanup and release locks
       */
      PageOp->Status = STATUS_SUCCESS;
      KeSetEvent (&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
      MmReleasePageOp (PageOp);
      DPRINT ("Address 0x%.8X\n", Address);
      return (STATUS_SUCCESS);
    }

  /*
   * Map anonymous memory for BSS sections
   */
  if (Segment->Characteristics & IMAGE_SECTION_CHAR_BSS)
    {
      Status = MmRequestPageMemoryConsumer (MC_USER, FALSE, &Page);
      assertmsg (NT_SUCCESS (Status),
		 ("MmRequestPageMemoryConsumer() failed with status 0x%.08x\n",
		  Status));
      if (!NT_SUCCESS (Status))
	{
	  MmUnlockSectionSegment (Segment);
	  MmUnlockSection (Section);
	  MmUnlockAddressSpace (AddressSpace);
	  while (!NT_SUCCESS (Status))
	    {
	      Status = MmRequestPageMemoryConsumer (MC_USER, TRUE, &Page);
	    }
	  MmLockAddressSpace (AddressSpace);
	  MmLockSection (Section);
	  MmLockSectionSegment (Segment);
	}

      Status = MmCreateVirtualMapping (PsGetCurrentProcess (),
				       Address,
				       MemoryArea->Attributes, Page, FALSE);
      assertmsg (NT_SUCCESS (Status),
		 ("MmCreateVirtualMapping() failed with status 0x%.08x\n",
		  Status));
      if (!NT_SUCCESS (Status))
	{
	  MmUnlockSectionSegment (Segment);
	  MmUnlockSection (Section);
	  MmUnlockAddressSpace (AddressSpace);
	  while (!NT_SUCCESS (Status))
	    {
	      Status = MmCreateVirtualMapping
		(PsGetCurrentProcess (), Address,
		 MemoryArea->Attributes, Page, TRUE);
	    }
	  MmLockAddressSpace (AddressSpace);
	  MmLockSection (Section);
	  MmLockSectionSegment (Segment);
	}

      MmInsertRmap (Page, PsGetCurrentProcess (),
		    (PVOID) PAGE_ROUND_DOWN (Address),
		    MmSectionDeleteMapping, NULL);

      if (Locked)
	{
	  MmLockPage (MmGetPhysicalAddressForProcess (NULL, Address));
	}

      /*
       * * Cleanup and release locks
       */
      PageOp->Status = STATUS_SUCCESS;
      KeSetEvent (&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
      MmReleasePageOp (PageOp);
      MmUnlockSectionSegment (Segment);
      MmUnlockSection (Section);
      DPRINT ("Address 0x%.8X\n", Address);
      return (STATUS_SUCCESS);
    }

  /*
   * Check if this page needs to be mapped COW
   */

  if (MmIsCopyOnWriteMemoryArea (MemoryArea))
    {
      if (MemoryArea->Attributes == PAGE_READWRITE)
	{
	  Attributes = PAGE_READONLY;
	}
      else if (MemoryArea->Attributes == PAGE_EXECUTE_READWRITE)
	{
	  Attributes = PAGE_EXECUTE_READ;
	}
      else
	{
	  assert (FALSE);
	}
    }
  else
    {
      Attributes = MemoryArea->Attributes;
    }

  /*
   * Get the entry corresponding to the offset within the section
   */
  Entry = MmGetPageEntrySectionSegment (Segment, Offset.u.LowPart);

  if (Entry == 0)
    {
      /*
       * * If the entry is zero (and it can't change because we have
       * * locked the segment) then we need to load the page.
       */

      /*
       * * Release all our locks and read in the page from disk
       */
      MmUnlockSectionSegment (Segment);
      MmUnlockSection (Section);
      MmUnlockAddressSpace (AddressSpace);

      if (Segment->Flags & MM_PAGEFILE_SECTION)
	{
	  Status = MmRequestPageMemoryConsumer (MC_USER, TRUE, &Page);
	  assertmsg (NT_SUCCESS (Status),
		     ("MmRequestPageMemoryConsumer() failed with status 0x%.08x\n",
		      Status));
	}
      else
	{
	  Status = MiReadPage (MemoryArea, &Offset, &Page);
	  assertmsg (NT_SUCCESS (Status)
		     || (Status == STATUS_END_OF_FILE),
		     ("MiReadPage() failed with status 0x%.08x\n", Status));
	}

      if (!NT_SUCCESS (Status) && (Status != STATUS_END_OF_FILE))
	{
	  /*
	   * * FIXME: What do we know in this case?
	   */
	  DPRINT ("IoPageRead failed (Status %x)\n", Status);

	  /*
	   * * Cleanup and release locks
	   */
	  PageOp->Status = Status;
	  KeSetEvent (&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
	  MmReleasePageOp (PageOp);
	  MmLockAddressSpace (AddressSpace);
	  return (Status);
	}

      /*
       * * Relock the address space, section and segment
       */
      MmLockAddressSpace (AddressSpace);
      MmLockSection (Section);
      MmLockSectionSegment (Segment);

      /*
       * Check the entry. No one should change the status of a page
       * that has a pending page-in.
       */
      Entry1 = MmGetPageEntrySectionSegment (Segment, Offset.QuadPart);
      if (Entry != Entry1)
	{
	  DbgPrint ("Someone changed ppte entry while we slept\n");
	  KeBugCheck (0);
	}

      /*
       * * Mark the offset within the section as having valid, in-memory
       * * data
       */
      Entry = (ULONG) Page;
      MmSetPageEntrySectionSegment (Segment, Offset.QuadPart, Entry);
      MmSharePageEntrySectionSegment (Segment, Offset.QuadPart);

      Status = MmCreateVirtualMapping (PsGetCurrentProcess (),
				       Address, Attributes, Page, FALSE);
      assertmsg (NT_SUCCESS (Status),
		 ("MmCreateVirtualMapping() failed with status 0x%.08x\n",
		  Status));
      if (!NT_SUCCESS (Status))
	{
	  MmUnlockSectionSegment (Segment);
	  MmUnlockSection (Section);
	  MmUnlockAddressSpace (AddressSpace);
	  while (!NT_SUCCESS (Status))
	    {
	      Status = MmCreateVirtualMapping
		(PsGetCurrentProcess (), Address, Attributes, Page, TRUE);
	    }
	  MmLockAddressSpace (AddressSpace);
	  MmLockSection (Section);
	  MmLockSectionSegment (Segment);
	}

      MmInsertRmap (Page, PsGetCurrentProcess (),
		    (PVOID) PAGE_ROUND_DOWN (Address),
		    MmSectionDeleteMapping, NULL);

      if (Locked)
	{
	  MmLockPage (MmGetPhysicalAddressForProcess (NULL, Address));
	}

      PageOp->Status = STATUS_SUCCESS;
      KeSetEvent (&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
      MmReleasePageOp (PageOp);
      MmUnlockSectionSegment (Segment);
      MmUnlockSection (Section);
      DPRINT ("MmNotPresentFaultSectionView succeeded\n");
      return (STATUS_SUCCESS);
    }
  else if (IS_SWAP_FROM_SSE (Entry))
    {
      SWAPENTRY SwapEntry;
      PMDL Mdl;

      SwapEntry = SWAPENTRY_FROM_SSE (Entry);

      /*
       * * Release all our locks and read in the page from disk
       */
      MmUnlockSectionSegment (Segment);
      MmUnlockSection (Section);
      MmUnlockAddressSpace (AddressSpace);

      Status = MmRequestPageMemoryConsumer (MC_USER, TRUE, &Page);
      assertmsg (NT_SUCCESS (Status),
		 ("MmRequestPageMemoryConsumer() failed with status 0x%.08x\n",
		  Status));
      if (!NT_SUCCESS (Status))
	{
	  PageOp->Status = Status;
	  KeSetEvent (&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
	  MmReleasePageOp (PageOp);
	  return Status;
	}

      Mdl = MmCreateMdl (NULL, NULL, PAGESIZE);
      MmBuildMdlFromPages (Mdl, (PULONG_PTR) & Page);
      Status = MmReadFromSwapPage (SwapEntry, Mdl);
      assertmsg (NT_SUCCESS (Status),
		 ("MmReadFromSwapPage() failed with status 0x%.08x\n",
		  Status));
      if (!NT_SUCCESS (Status))
	{
	  PageOp->Status = Status;
	  KeSetEvent (&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
	  MmReleasePageOp (PageOp);
	  return Status;
	}

      /*
       * * Relock the address space, section and segment
       */
      MmLockAddressSpace (AddressSpace);
      MmLockSection (Section);
      MmLockSectionSegment (Segment);

      /*
       * * Check the entry. No one should change the status of a page
       * * that has a pending page-in.
       */
      Entry1 = MmGetPageEntrySectionSegment (Segment, Offset.QuadPart);
      if (Entry != Entry1)
	{
	  DbgPrint ("Someone changed ppte entry while we slept\n");
	  KeBugCheck (0);
	}

      /*
       * Mark the offset within the section as having valid, in-memory
       * data
       */
      Entry = (ULONG) Page;
      MmSetPageEntrySectionSegment (Segment, Offset.QuadPart, Entry);
      MmSharePageEntrySectionSegment (Segment, Offset.QuadPart);

      /*
       * * Save the swap entry.
       */
      MmSetSavedSwapEntryPage (Page, SwapEntry);

      Status = MmCreateVirtualMapping (PsGetCurrentProcess (),
				       Address, Attributes, (ULONG) Page,
				       FALSE);
      assertmsg (NT_SUCCESS (Status),
		 ("MmCreateVirtualMapping() failed with status 0x%.08x\n",
		  Status));
      if (!NT_SUCCESS (Status))
	{
	  MmUnlockSectionSegment (Segment);
	  MmUnlockSection (Section);
	  MmUnlockAddressSpace (AddressSpace);
	  while (!NT_SUCCESS (Status))
	    {
	      Status = MmCreateVirtualMapping (PsGetCurrentProcess (),
					       Address, Attributes, Page,
					       TRUE);
	    }
	  MmLockAddressSpace (AddressSpace);
	  MmLockSection (Section);
	  MmLockSectionSegment (Segment);
	}

      MmInsertRmap (Page, PsGetCurrentProcess (),
		    (PVOID) PAGE_ROUND_DOWN (Address), MmSectionDeleteMapping,
		    NULL);

      if (Locked)
	{
	  MmLockPage (MmGetPhysicalAddressForProcess (NULL, Address));
	}

      PageOp->Status = STATUS_SUCCESS;
      KeSetEvent (&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
      MmReleasePageOp (PageOp);
      MmUnlockSectionSegment (Segment);
      MmUnlockSection (Section);
      DPRINT ("MmNotPresentFaultSectionView succeeded\n");
      return (STATUS_SUCCESS);
    }
  else
    {
      /*
       * If the section offset is already in-memory and valid then just
       * take another reference to the page 
       */

      Page = (ULONG_PTR) PAGE_FROM_SSE (Entry);
      MmReferencePage (Page);
      MmSharePageEntrySectionSegment (Segment, Offset.QuadPart);

      Status = MmCreateVirtualMapping (PsGetCurrentProcess (),
				       Address, Attributes, Page, FALSE);
      assertmsg (NT_SUCCESS (Status),
		 ("MmCreateVirtualMapping() failed with status 0x%.08x\n",
		  Status));
      if (!NT_SUCCESS (Status))
	{
	  MmUnlockSectionSegment (Segment);
	  MmUnlockSection (Section);
	  MmUnlockAddressSpace (AddressSpace);
	  while (!NT_SUCCESS (Status))
	    {
	      Status = MmCreateVirtualMapping (PsGetCurrentProcess (),
					       Address, Attributes, Page,
					       TRUE);
	    }
	  MmLockAddressSpace (AddressSpace);
	  MmLockSection (Section);
	  MmLockSectionSegment (Segment);
	}
    }

  MmInsertRmap (Page, PsGetCurrentProcess (),
		(PVOID) PAGE_ROUND_DOWN (Address), MmSectionDeleteMapping,
		NULL);

  if (Locked)
    {
      MmLockPage (MmGetPhysicalAddressForProcess (NULL, Address));
    }

  PageOp->Status = STATUS_SUCCESS;
  KeSetEvent (&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
  MmReleasePageOp (PageOp);
  MmUnlockSectionSegment (Segment);
  MmUnlockSection (Section);
  return (STATUS_SUCCESS);
}


NTSTATUS
MmAccessFaultSectionView (PMADDRESS_SPACE AddressSpace,
			  MEMORY_AREA * MemoryArea,
			  PVOID Address, BOOLEAN Locked)
{
  PMM_SECTION_SEGMENT Segment;
  PSECTION_OBJECT Section;
  ULONG_PTR OldPage;
  ULONG_PTR NewPage;
  PVOID NewAddress;
  NTSTATUS Status;
  PVOID PAddress;
  LARGE_INTEGER Offset;
  PMM_PAGEOP PageOp;

  DPRINT ("MmAccessFaultSectionView(Address 0x%.08x,  Locked %d)\n",
	  Address, Locked);

  /*
   * Check if the page has been paged out or has already been set readwrite
   */
  if (!MmIsPagePresent (NULL, Address) ||
      MmGetPageProtect (NULL, Address) & PAGE_READWRITE)
    {
      if (Locked)
	{
	  MmLockPage (MmGetPhysicalAddressForProcess (NULL, Address));
	}
      return (STATUS_SUCCESS);
    }

  /*
   * Find the offset of the page
   */
  PAddress = (PVOID) PAGE_ROUND_DOWN (((ULONG_PTR) Address));
  Offset.QuadPart =
    ((ULONG_PTR) PAddress - (ULONG_PTR) MemoryArea->BaseAddress) +
    MemoryArea->Data.SectionData.ViewOffset;

  /*
   * Lock the segment
   */
  Segment = MemoryArea->Data.SectionData.Segment;
  Section = MemoryArea->Data.SectionData.Section;
  MmLockSection (Section);
  MmLockSectionSegment (Segment);

  /*
   * Sanity check.
   */
  assertmsg (MmGetPageEntrySectionSegment (Segment, Offset.QuadPart) !=
	     0,
	     ("COW fault for page with MmGetPageEntrySectionSegment() = 0. Address was 0x%.8X\n"));

  /*
   * Check if we are doing copy-on-write (COW)
   */
  if (!MmIsCopyOnWriteMemoryArea (MemoryArea))
    {
      assertmsg (FALSE, ("Not doing COW for address 0x%.08x\n", Address));
      MmUnlockSection (Section);
      MmUnlockSectionSegment (Segment);
      return (STATUS_UNSUCCESSFUL);
    }

  /*
   * Get or create a pageop
   */
  PageOp = MmGetPageOp (MemoryArea, 0, 0, Segment, Offset.u.LowPart,
			MM_PAGEOP_ACCESSFAULT);
  assertmsg (PageOp != NULL, ("MmGetPageOp() failed\n"));

  /*
   * Wait for any other operations to complete
   */
  if (PageOp->Thread != PsGetCurrentThread ())
    {
      MmUnlockSectionSegment (Segment);
      MmUnlockSection (Section);
      MmUnlockAddressSpace (AddressSpace);
      DPRINT ("Waiting for page operation of type %d to finish\n",
	      PageOp->OpType);
      Status = KeWaitForSingleObject (&PageOp->CompletionEvent,
				      0, KernelMode, FALSE, NULL);
      /*
       * * Check for various strange conditions
       */
      assertmsg (Status == STATUS_SUCCESS, ("Failed to wait for pageop\n"));
      if (Status != STATUS_SUCCESS)
	{
	  MmReleasePageOp (PageOp);
	  MmLockAddressSpace (AddressSpace);
	  return Status;
	}

      assertmsg (PageOp->Status != STATUS_PENDING,
		 ("Woke for page op before completion\n"));
      if (PageOp->Status == STATUS_PENDING)
	{
	  MmReleasePageOp (PageOp);
	  MmLockAddressSpace (AddressSpace);
	  return Status;
	}

      /*
       * * Restart the operation
       */
      MmReleasePageOp (PageOp);
      MmLockAddressSpace (AddressSpace);
      return (STATUS_MM_RESTART_OPERATION);
    }

  /*
   * Release locks now we have the pageop
   */
  MmUnlockSectionSegment (Segment);
  MmUnlockSection (Section);
  MmUnlockAddressSpace (AddressSpace);

  /*
   * Allocate a page
   */
  Status = MmRequestPageMemoryConsumer (MC_USER, TRUE, &NewPage);
  assertmsg (NT_SUCCESS (Status),
	     ("MmRequestPageMemoryConsumer() failed with status 0x%.08x",
	      Status));
  if (!NT_SUCCESS (Status))
    {
      PageOp->Status = Status;
      KeSetEvent (&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
      MmReleasePageOp (PageOp);
      MmLockAddressSpace (AddressSpace);
      return Status;
    }

  /*
   * Copy the old page
   */
  OldPage = MmGetPhysicalAddressForProcess (NULL, Address);

  NewAddress = ExAllocatePageWithPhysPage (NewPage);
  assert (NewAddress);
  memcpy (NewAddress, (PVOID) PAGE_ROUND_DOWN (Address), PAGESIZE);
  ExUnmapPage (NewAddress);

  /*
   * Delete the old entry
   */
  MmDeleteVirtualMapping (PsGetCurrentProcess (), Address, FALSE, NULL, NULL);

  /*
   * Set the PTE to point to the new page
   */
  MmLockAddressSpace (AddressSpace);
  Status = MmCreateVirtualMapping (PsGetCurrentProcess (),
				   Address,
				   MemoryArea->Attributes, NewPage, FALSE);
  assertmsg (NT_SUCCESS (Status),
	     ("MmCreateVirtualMapping() failed with status 0x%.08x", Status));
  if (!NT_SUCCESS (Status))
    {
      MmUnlockAddressSpace (AddressSpace);
      while (!NT_SUCCESS (Status))
	{
	  Status = MmCreateVirtualMapping (PsGetCurrentProcess
					   (), Address,
					   MemoryArea->
					   Attributes, NewPage, TRUE);
	}
      MmLockAddressSpace (AddressSpace);
    }

  MmInsertRmap (NewPage, PsGetCurrentProcess (),
		(PVOID) PAGE_ROUND_DOWN (Address),
		MmSectionDeleteMapping, NULL);

  if (Locked)
    {
      MmLockPage (MmGetPhysicalAddressForProcess (NULL, Address));
    }

  /*
   * Unshare the old page. The page entry can be destroyed by this call.
   */
  MmUnsharePageEntrySectionSegment (Section, Segment, Offset.QuadPart, FALSE);
  MmDeleteRmap (OldPage, PsGetCurrentProcess (),
		(PVOID) PAGE_ROUND_DOWN (Address));
  MmDereferencePage (OldPage);

  DPRINT ("Made a private page for Virtual Page 0x%.08x\n",
	  PAGE_ROUND_DOWN (Address));

  PageOp->Status = STATUS_SUCCESS;
  KeSetEvent (&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
  MmReleasePageOp (PageOp);
  return (STATUS_SUCCESS);
}


/*
 * Flush pages used for data & image file sections
 */
NTSTATUS
MmpFlushSectionPage (IN PFILE_OBJECT FileObject,
		     IN PLARGE_INTEGER FileOffset,
		     IN PMDL Mdl, OUT PIO_STATUS_BLOCK IoStatus)
{
  NTSTATUS Status;

  assertmsg (FileObject != NULL
	     && FileObject->DeviceObject != NULL,
	     ("Bad file object 0x%.8X\n", FileObject));

  Status = IoPageWrite (FileObject, Mdl, FileOffset, IoStatus, FALSE);

  return (Status);
}


NTSTATUS
MmFlushSectionView (PMADDRESS_SPACE AddressSpace,
		    MEMORY_AREA * MemoryArea,
		    PVOID Address, PMM_PAGEOP PageOp)
{
  LARGE_INTEGER Offset;
  PSECTION_OBJECT Section;
  PMM_SECTION_SEGMENT Segment;
  ULONG_PTR PhysicalAddress;
  PMDL Mdl;
  ULONG Entry;
  BOOLEAN Private;
  NTSTATUS Status;
  PFILE_OBJECT FileObject;
  PREACTOS_COMMON_FCB_HEADER Fcb;
  BOOLEAN DirectMapped;
  BOOLEAN WasDirty;
  SWAPENTRY SwapEntry;

  DPRINT
    ("MmFlushSectionView(AddressSpace 0x%.08x, MemoryArea 0x%.08x, Address 0x%.08x, PageOp 0x%.08x\n",
     AddressSpace, MemoryArea, Address, PageOp);

  Address = (PVOID) PAGE_ROUND_DOWN (Address);

  Offset.QuadPart =
    (ULONG_PTR) ((ULONG_PTR) Address -
		 (ULONG_PTR) MemoryArea->BaseAddress) +
    MemoryArea->Data.SectionData.ViewOffset;

  FileObject = MemoryArea->Data.SectionData.Section->FileObject;
  DirectMapped = FALSE;
  if (FileObject != NULL)
    {
      Fcb = (PREACTOS_COMMON_FCB_HEADER) FileObject->FsContext;

      /*
       * If the file system is letting us go directly to the cache and the
       * memory area was mapped at an offset in the file which is page aligned
       * then note this is a direct mapped page.
       */
      if (FileObject->Flags & FO_DIRECT_CACHE_PAGING_READ &&
	  (Offset.QuadPart % PAGESIZE) == 0)
	{
	  DirectMapped = TRUE;
	  /*
	   * FIXME: Disabled for now 
	   */
	  DirectMapped = FALSE;
	}
    }

  /*
   * Get the segment and section.
   */
  Segment = MemoryArea->Data.SectionData.Segment;
  Section = MemoryArea->Data.SectionData.Section;

  /*
   * This should never happen since mappings of physical memory are never
   * placed in the rmap lists.
   */
  assertmsg (!(Section->Flags & SO_PHYSICAL_MEMORY),
	     ("Trying to flush from physical memory section address 0x%X "
	      "process %d\n", Address,
	      AddressSpace->Process->UniqueProcessId));

  /*
   * Get the section segment entry and the physical address.
   */
  Entry = MmGetPageEntrySectionSegment (Segment, Offset.QuadPart);
  PhysicalAddress =
    MmGetPhysicalAddressForProcess (AddressSpace->Process, Address);
  SwapEntry = MmGetSavedSwapEntryPage (PhysicalAddress);

  DPRINT ("PhysicalAddress 0x%.08x  SwapEntry 0x%.08x\n",
	  PhysicalAddress, SwapEntry);

  if (Segment->Characteristics & IMAGE_SECTION_CHAR_BSS ||
      IS_SWAP_FROM_SSE (Entry) ||
      (ULONG_PTR) (PAGE_FROM_SSE (Entry)) != PhysicalAddress)
    {
      Private = TRUE;
    }
  else
    {
      Private = FALSE;
    }

  assertmsg (!((!Private) && (Entry == 0)),
	     ("MmGetPageEntrySectionSegment() returned 0 for file offset 0x%.08x for non-private page\n",
	      Offset.u.LowPart));

  /*
   * Flushing data mapped read-only is easy.
   */
  if (MemoryArea->Attributes & PAGE_READONLY ||
      MemoryArea->Attributes & PAGE_EXECUTE_READ)
    {
      /*
       * Read-only data should never be COWed
       */
      assertmsg (!Private, ("Had private copy of read-only page.\n"));
      return (STATUS_SUCCESS);
    }

  /*
   * Disable all PTE's pointing to this physical address if not done
   * already 
   */
  MiDisableAllRmaps (PhysicalAddress, &WasDirty);

  /*
   * If the page wasn't dirty then we don't have to flush it. Since we
   * disabled all the mappings above we know it will not suddenly
   * become dirty.
   */
  if (!WasDirty)
    {
      if (Private)
	{
	  if (!(Segment->Characteristics & IMAGE_SECTION_CHAR_BSS)
	      && (SwapEntry == 0))
	    {
	      assertmsg (FALSE,
			 ("Private page, non-dirty but not swapped out "
			  "process %d address 0x%.8X\n",
			  AddressSpace->Process->UniqueProcessId, Address));
	      KeBugCheck (0);
	    }
	  else
	    {
	      Status = MmCreatePageFileMapping
		(AddressSpace->Process, Address, SwapEntry);
	      if (!NT_SUCCESS (Status))
		{
		  assert (FALSE);
		}
	    }
	}
      return (STATUS_SUCCESS);
    }

  if (FileObject != NULL)
    {
      IO_STATUS_BLOCK IoStatus;

      /*
       * This is an image or data file backed section 
       */

      /*
       * Write the page to the file
       */
      Mdl = MmCreateMdl (NULL, NULL, PAGESIZE);
      MmBuildMdlFromPages (Mdl, &PhysicalAddress);
      Status = MmpFlushSectionPage (FileObject, &Offset, Mdl, &IoStatus);
      if (!NT_SUCCESS (Status))
	{
	  DPRINT ("MM: Failed to write to file (Status was 0x%.8X)\n",
		  Status);
	}
      else
	{
	  /*
	   * Otherwise we have succeeded.
	   */
	  DPRINT ("MM: Wrote section page 0x%.8X to file!\n",
		  PhysicalAddress);
	}
    }
  else if (Segment->Flags & MM_PAGEFILE_SECTION)
    {
      /*
       * This is a page file backed section 
       */

      /*
       * If necessary, allocate an entry in the paging file for this page
       */
      SwapEntry = MmGetSavedSwapEntryPage (PhysicalAddress);
      if (SwapEntry == 0)
	{
	  SwapEntry = MmAllocSwapPage ();
	  if (SwapEntry == 0)
	    {
	      assertmsg (FALSE, ("MmAllocSwapPage() failed\n"));
	      return (STATUS_UNSUCCESSFUL);
	    }
	}

      /*
       * Write the page to the pagefile
       */
      Mdl = MmCreateMdl (NULL, NULL, PAGESIZE);
      MmBuildMdlFromPages (Mdl, &PhysicalAddress);
      Status = MmWriteToSwapPage (SwapEntry, Mdl);
      if (!NT_SUCCESS (Status))
	{
	  DPRINT
	    ("MM: Failed to write to swap page (Status was 0x%.8X)\n",
	     Status);
	  return (STATUS_UNSUCCESSFUL);
	}

      /*
       * Otherwise we have succeeded.
       */
      DPRINT ("MM: Wrote section page 0x%.8X to swap!\n", PhysicalAddress);

      if (Private)
	{
	  Status = MmCreatePageFileMapping (MemoryArea->Process,
					    Address, SwapEntry);
	  if (!NT_SUCCESS (Status))
	    {
	      assertmsg (FALSE,
			 ("MmCreatePageFileMapping() failed with status 0x%.08x\n",
			  Status));
	      return (Status);
	    }
	}
      else
	{
	  Entry = MAKE_SWAP_SSE (SwapEntry);
	  MmSetPageEntrySectionSegment (Segment, Offset.QuadPart, Entry);
	}
      MmSetSavedSwapEntryPage (PhysicalAddress, 0);
    }
  else
    {
      assertmsg (FALSE,
		 ("Unknown section object type (Segment->Flags 0x%.08x)\n",
		  Segment->Flags));
    }

  return (Status);
}


VOID STDCALL
MmpDeleteSection (PVOID ObjectBody)
{
  PSECTION_OBJECT Section = (PSECTION_OBJECT) ObjectBody;

  DPRINT ("MmpDeleteSection(ObjectBody %x)\n", ObjectBody);

  if (Section->Flags & MM_IMAGE_SECTION)
    {
      ULONG i;

      for (i = 0; i < Section->NrSegments; i++)
	{
	  InterlockedDecrement (&Section->Segments[i].ReferenceCount);
	}
    }
  else
    {
      InterlockedDecrement (&Section->Segments->ReferenceCount);
    }
  if (Section->FileObject != NULL)
    {
      ObDereferenceObject (Section->FileObject);
      Section->FileObject = NULL;
    }
}

VOID STDCALL
MmpCloseSection (PVOID ObjectBody, ULONG HandleCount)
{
  DPRINT ("MmpCloseSection(OB %x, HC %d) RC %d\n",
	  ObjectBody, HandleCount, ObGetObjectPointerCount (ObjectBody));
}

NTSTATUS STDCALL
MmpCreateSection (PVOID ObjectBody,
		  PVOID Parent,
		  PWSTR RemainingPath, POBJECT_ATTRIBUTES ObjectAttributes)
{
  DPRINT ("MmpCreateDevice(ObjectBody %x, Parent %x, RemainingPath %S)\n",
	  ObjectBody, Parent, RemainingPath);

  if (RemainingPath == NULL)
    {
      return (STATUS_SUCCESS);
    }

  if (wcschr (RemainingPath + 1, L'\\') != NULL)
    {
      return (STATUS_UNSUCCESSFUL);
    }

  return (STATUS_SUCCESS);
}

NTSTATUS
MmCreatePhysicalMemorySection (VOID)
{
  HANDLE PhysSectionH;
  PSECTION_OBJECT PhysSection;
  NTSTATUS Status;
  OBJECT_ATTRIBUTES Obj;
  UNICODE_STRING Name;
  LARGE_INTEGER SectionSize;

  /*
   * Create the section mapping physical memory 
   */
  SectionSize.QuadPart = 0xFFFFFFFF;
  RtlInitUnicodeString (&Name, L"\\Device\\PhysicalMemory");
  InitializeObjectAttributes (&Obj, &Name, 0, NULL, NULL);
  Status = NtCreateSection (&PhysSectionH,
			    SECTION_ALL_ACCESS,
			    &Obj,
			    &SectionSize, PAGE_EXECUTE_READWRITE, 0, NULL);
  if (!NT_SUCCESS (Status))
    {
      DbgPrint ("Failed to create PhysicalMemory section\n");
      KeBugCheck (0);
    }
  Status = ObReferenceObjectByHandle (PhysSectionH,
				      SECTION_ALL_ACCESS,
				      NULL,
				      KernelMode,
				      (PVOID *) & PhysSection, NULL);
  if (!NT_SUCCESS (Status))
    {
      DbgPrint ("Failed to reference PhysicalMemory section\n");
      KeBugCheck (0);
    }
  PhysSection->Flags = PhysSection->Flags | SO_PHYSICAL_MEMORY;
  ObDereferenceObject (PhysSection);

  return (STATUS_SUCCESS);
}

NTSTATUS
MmInitSectionImplementation (VOID)
{
  MmSectionObjectType = ExAllocatePool (NonPagedPool, sizeof (OBJECT_TYPE));

  RtlInitUnicodeString (&MmSectionObjectType->TypeName, L"Section");

  MmSectionObjectType->Tag = TAG ('S', 'E', 'C', 'T');
  MmSectionObjectType->TotalObjects = 0;
  MmSectionObjectType->TotalHandles = 0;
  MmSectionObjectType->MaxObjects = ULONG_MAX;
  MmSectionObjectType->MaxHandles = ULONG_MAX;
  MmSectionObjectType->PagedPoolCharge = 0;
  MmSectionObjectType->NonpagedPoolCharge = sizeof (SECTION_OBJECT);
  MmSectionObjectType->Mapping = &MmpSectionMapping;
  MmSectionObjectType->Dump = NULL;
  MmSectionObjectType->Open = NULL;
  MmSectionObjectType->Close = MmpCloseSection;
  MmSectionObjectType->Delete = MmpDeleteSection;
  MmSectionObjectType->Parse = NULL;
  MmSectionObjectType->Security = NULL;
  MmSectionObjectType->QueryName = NULL;
  MmSectionObjectType->OkayToClose = NULL;
  MmSectionObjectType->Create = MmpCreateSection;
  MmSectionObjectType->DuplicationNotify = NULL;

  return (STATUS_SUCCESS);
}

NTSTATUS
MmCreatePageFileSection (PHANDLE SectionHandle,
			 ACCESS_MASK DesiredAccess,
			 POBJECT_ATTRIBUTES ObjectAttributes,
			 PLARGE_INTEGER UMaximumSize,
			 ULONG SectionPageProtection,
			 ULONG AllocationAttributes)
     /*
      * Create a section which is backed by the pagefile
      */
{
  LARGE_INTEGER MaximumSize;
  PSECTION_OBJECT Section;
  PMM_SECTION_SEGMENT Segment;
  NTSTATUS Status;

  if (UMaximumSize == NULL)
    {
      return (STATUS_UNSUCCESSFUL);
    }
  MaximumSize = *UMaximumSize;

  /*
   * Check the protection
   */
  if ((SectionPageProtection & PAGE_FLAGS_VALID_FROM_USER_MODE) !=
      SectionPageProtection)
    {
      return (STATUS_INVALID_PAGE_PROTECTION);
    }

  /*
   * Create the section
   */
  Status = ObCreateObject (SectionHandle,
			   DesiredAccess,
			   ObjectAttributes,
			   MmSectionObjectType, (PVOID *) & Section);
  if (!NT_SUCCESS (Status))
    {
      return (Status);
    }

  /*
   * Initialize it
   */
  Section->SectionPageProtection = SectionPageProtection;
  Section->AllocateAttributes = AllocationAttributes;
  InitializeListHead (&Section->ViewListHead);
  KeInitializeSpinLock (&Section->ViewListLock);
  KeInitializeMutex (&Section->Lock, 0);
  Section->Flags = 0;
  Section->FileObject = NULL;
  Section->MaximumSize = MaximumSize;
  Segment =
    ExAllocatePoolWithTag (NonPagedPool,
			   sizeof (MM_SECTION_SEGMENT),
			   TAG_MM_SECTION_SEGMENT);
  if (Segment == NULL)
    {
      ZwClose (*SectionHandle);
      ObDereferenceObject (Section);
      return (STATUS_NO_MEMORY);
    }
  Section->Segments = Segment;
  Segment->ReferenceCount = 1;
  KeInitializeMutex (&Segment->Lock, 0);
  Segment->FileOffset = 0;
  Segment->Protection = SectionPageProtection;
  Segment->Attributes = AllocationAttributes;
  Segment->Length = MaximumSize.u.LowPart;
  Segment->Flags = MM_PAGEFILE_SECTION;
  Segment->WriteCopy = FALSE;
  return (STATUS_SUCCESS);
}


NTSTATUS
MmCreateDataFileSection (PHANDLE SectionHandle,
			 ACCESS_MASK DesiredAccess,
			 POBJECT_ATTRIBUTES ObjectAttributes,
			 PLARGE_INTEGER UMaximumSize,
			 ULONG SectionPageProtection,
			 ULONG AllocationAttributes, HANDLE FileHandle)
     /*
      * Create a section backed by a data file
      */
{
  PSECTION_OBJECT Section;
  NTSTATUS Status;
  LARGE_INTEGER MaximumSize;
  PFILE_OBJECT FileObject;
  PMM_SECTION_SEGMENT Segment;
  ULONG FileAccess;

  /*
   * Check the protection
   */
  if ((SectionPageProtection & PAGE_FLAGS_VALID_FROM_USER_MODE) !=
      SectionPageProtection)
    {
      return (STATUS_INVALID_PAGE_PROTECTION);
    }

  /*
   * Create the section
   */
  Status = ObCreateObject (SectionHandle,
			   DesiredAccess,
			   ObjectAttributes,
			   MmSectionObjectType, (PVOID *) & Section);
  if (!NT_SUCCESS (Status))
    {
      return (Status);
    }

  /*
   * Initialize it
   */
  Section->SectionPageProtection = SectionPageProtection;
  Section->AllocateAttributes = AllocationAttributes;
  InitializeListHead (&Section->ViewListHead);
  KeInitializeSpinLock (&Section->ViewListLock);
  KeInitializeMutex (&Section->Lock, 0);
  Section->Flags = 0;
  Section->NrSegments = 1;
  Section->ImageBase = NULL;
  Section->EntryPoint = NULL;
  Section->StackReserve = 0;
  Section->StackCommit = 0;
  Section->Subsystem = 0;
  Section->MinorSubsystemVersion = 0;
  Section->MajorSubsystemVersion = 0;
  Section->ImageCharacteristics = 0;
  Section->Machine = 0;
  Section->Executable = FALSE;

  /*
   * Check file access required
   */
  if (SectionPageProtection & PAGE_READWRITE ||
      SectionPageProtection & PAGE_EXECUTE_READWRITE)
    {
      FileAccess = FILE_READ_DATA | FILE_WRITE_DATA;
    }
  else
    {
      FileAccess = FILE_READ_DATA;
    }

  /*
   * Reference the file handle
   */
  Status = ObReferenceObjectByHandle (FileHandle,
				      FileAccess,
				      IoFileObjectType,
				      UserMode, (PVOID *) & FileObject, NULL);
  if (!NT_SUCCESS (Status))
    {
      ZwClose (*SectionHandle);
      ObDereferenceObject (Section);
      return (Status);
    }

  /*
   * We can't do memory mappings if the file system doesn't support the
   * standard FCB
   */
  if (!(FileObject->Flags & FO_FCB_IS_VALID))
    {
      ZwClose (*SectionHandle);
      ObDereferenceObject (Section);
      ObDereferenceObject (FileObject);
      return (STATUS_INVALID_FILE_FOR_SECTION);
    }

  /*
   * FIXME: Revise this once a locking order for file size changes is
   * decided
   */
  if (UMaximumSize != NULL)
    {
      MaximumSize = *UMaximumSize;
    }
  else
    {
      MaximumSize =
	((PREACTOS_COMMON_FCB_HEADER) FileObject->FsContext)->FileSize;
    }

  Section->MaximumSize.QuadPart = MaximumSize.QuadPart;

  /*
   * Lock the file
   */
  Status = KeWaitForSingleObject ((PVOID) & FileObject->Lock,
				  0, KernelMode, FALSE, NULL);
  if (Status != STATUS_SUCCESS)
    {
      ZwClose (*SectionHandle);
      ObDereferenceObject (Section);
      ObDereferenceObject (FileObject);
      return (Status);
    }

  /*
   * If this file hasn't been mapped as a data file before then allocate a
   * section segment to describe the data file mapping
   */
  if (FileObject->SectionObjectPointers->DataSectionObject == NULL)
    {
      Segment =
	ExAllocatePoolWithTag (NonPagedPool,
			       sizeof (MM_SECTION_SEGMENT),
			       TAG_MM_SECTION_SEGMENT);
      if (Segment == NULL)
	{
	  KeSetEvent ((PVOID) & FileObject->Lock, IO_NO_INCREMENT, FALSE);
	  ZwClose (*SectionHandle);
	  ObDereferenceObject (Section);
	  ObDereferenceObject (FileObject);
	  return (STATUS_NO_MEMORY);
	}
      Section->Segments = Segment;
      Segment->ReferenceCount = 1;
      KeInitializeMutex (&Segment->Lock, 0);

      /*
       * Set the lock before assigning the segment to the file object
       */
      Status = KeWaitForSingleObject ((PVOID) & Segment->Lock,
				      0, KernelMode, FALSE, NULL);
      if (Status != STATUS_SUCCESS)
	{
	  KeSetEvent ((PVOID) & FileObject->Lock, IO_NO_INCREMENT, FALSE);
	  ExFreePool (Segment);
	  ZwClose (*SectionHandle);
	  ObDereferenceObject (Section);
	  ObDereferenceObject (FileObject);
	  return (Status);
	}
      FileObject->SectionObjectPointers->DataSectionObject = (PVOID) Segment;

      Segment->FileOffset = 0;
      Segment->Protection = 0;
      Segment->Attributes = 0;
      Segment->Flags = 0;
      Segment->Characteristics = 0;
      Segment->WriteCopy = FALSE;
      if (AllocationAttributes & SEC_RESERVE)
	{
	  Segment->Length = 0;
	}
      else
	{
	  Segment->Length = MaximumSize.u.LowPart;
	}
      Segment->VirtualAddress = NULL;
    }
  else
    {
      /*
       * If the file is already mapped as a data file then we may need
       * to extend it
       */
      Segment =
	(PMM_SECTION_SEGMENT) FileObject->
	SectionObjectPointers->DataSectionObject;
      Section->Segments = Segment;
      InterlockedIncrement ((PLONG) & Segment->ReferenceCount);
      Status = KeWaitForSingleObject ((PVOID) & Section->Lock,
				      0, KernelMode, FALSE, NULL);
      if (Status != STATUS_SUCCESS)
	{
	  InterlockedDecrement ((PLONG) & Segment->ReferenceCount);
	  KeSetEvent ((PVOID) & FileObject->Lock, IO_NO_INCREMENT, FALSE);
	  ZwClose (*SectionHandle);
	  ObDereferenceObject (Section);
	  ObDereferenceObject (FileObject);
	  return (Status);
	}
      if (MaximumSize.u.LowPart > Segment->Length &&
	  !(AllocationAttributes & SEC_RESERVE))
	{
	  Segment->Length = MaximumSize.u.LowPart;
	}
    }
  KeSetEvent ((PVOID) & FileObject->Lock, IO_NO_INCREMENT, FALSE);
  Section->FileObject = FileObject;
  KeReleaseMutex (&Segment->Lock, FALSE);

  ObDereferenceObject (Section);
  return (STATUS_SUCCESS);
}

static ULONG SectionCharacteristicsToProtect[16] = {
  PAGE_NOACCESS,		// 0 = NONE
  PAGE_NOACCESS,		// 1 = SHARED
  PAGE_EXECUTE,			// 2 = EXECUTABLE
  PAGE_EXECUTE,			// 3 = EXECUTABLE, SHARED
  PAGE_READONLY,		// 4 = READABLE
  PAGE_READONLY,		// 5 = READABLE, SHARED
  PAGE_EXECUTE_READ,		// 6 = READABLE, EXECUTABLE
  PAGE_EXECUTE_READ,		// 7 = READABLE, EXECUTABLE, SHARED
  PAGE_READWRITE,		// 8 = WRITABLE
  PAGE_READWRITE,		// 9 = WRITABLE, SHARED
  PAGE_EXECUTE_READWRITE,	// 10 = WRITABLE, EXECUTABLE
  PAGE_EXECUTE_READWRITE,	// 11 = WRITABLE, EXECUTABLE, SHARED
  PAGE_READWRITE,		// 12 = WRITABLE, READABLE
  PAGE_READWRITE,		// 13 = WRITABLE, READABLE, SHARED
  PAGE_EXECUTE_READWRITE,	// 14 = WRITABLE, READABLE, EXECUTABLE,
  PAGE_EXECUTE_READWRITE,	// 15 = WRITABLE, READABLE, EXECUTABLE,
  // SHARED
};

NTSTATUS
MmCreateImageSection (PHANDLE SectionHandle,
		      ACCESS_MASK DesiredAccess,
		      POBJECT_ATTRIBUTES ObjectAttributes,
		      PLARGE_INTEGER UMaximumSize,
		      ULONG SectionPageProtection,
		      ULONG AllocationAttributes, HANDLE FileHandle)
{
  PSECTION_OBJECT Section;
  NTSTATUS Status;
  PFILE_OBJECT FileObject;
  ULONG FileAccess;
  IMAGE_DOS_HEADER DosHeader;
  IO_STATUS_BLOCK Iosb;
  LARGE_INTEGER Offset;
  IMAGE_NT_HEADERS PEHeader;
  PIMAGE_SECTION_HEADER ImageSections;
  PMM_SECTION_SEGMENT SectionSegments;
  ULONG NrSegments;
  PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
  LARGE_INTEGER MaximumSize;

  /*
   * Check the protection
   */
  if ((SectionPageProtection & PAGE_FLAGS_VALID_FROM_USER_MODE) !=
      SectionPageProtection)
    {
      return (STATUS_INVALID_PAGE_PROTECTION);
    }

  /*
   * Specifying a maximum size is meaningless for an image section
   */
  if (UMaximumSize != NULL)
    {
      return (STATUS_INVALID_PARAMETER_4);
    }

  /*
   * Read the dos header
   */
  Offset.QuadPart = 0;
  Status = ZwReadFile (FileHandle,
		       NULL,
		       NULL,
		       NULL,
		       &Iosb, &DosHeader, sizeof (DosHeader), &Offset, 0);
  if (!NT_SUCCESS (Status))
    {
      return (Status);
    }
  if (Iosb.Information != sizeof (DosHeader))
    {
      return (STATUS_INVALID_IMAGE_FORMAT);
    }

  /*
   * Check the DOS signature
   */
  if (DosHeader.e_magic != IMAGE_DOS_SIGNATURE)
    {
      return (STATUS_INVALID_IMAGE_FORMAT);
    }

  /*
   * Read the PE header
   */
  Offset.QuadPart = DosHeader.e_lfanew;
  Status = ZwReadFile (FileHandle,
		       NULL,
		       NULL,
		       NULL, &Iosb, &PEHeader, sizeof (PEHeader), &Offset, 0);
  if (!NT_SUCCESS (Status))
    {
      return (Status);
    }
  if (Iosb.Information != sizeof (PEHeader))
    {
      return (STATUS_INVALID_IMAGE_FORMAT);
    }

  /*
   * Check the signature
   */
  if (PEHeader.Signature != IMAGE_NT_SIGNATURE)
    {
      return (STATUS_INVALID_IMAGE_FORMAT);
    }

  /*
   * Read in the section headers
   */
  Offset.QuadPart = DosHeader.e_lfanew + sizeof (PEHeader);
  ImageSections =
    ExAllocatePool (NonPagedPool,
		    PEHeader.FileHeader.NumberOfSections *
		    sizeof (IMAGE_SECTION_HEADER));
  Status = ZwReadFile (FileHandle,
		       NULL,
		       NULL,
		       NULL,
		       &Iosb,
		       ImageSections,
		       PEHeader.FileHeader.NumberOfSections *
		       sizeof (IMAGE_SECTION_HEADER), &Offset, 0);
  if (!NT_SUCCESS (Status))
    {
      ExFreePool (ImageSections);
      return (Status);
    }
  if (Iosb.Information !=
      (PEHeader.FileHeader.NumberOfSections * sizeof (IMAGE_SECTION_HEADER)))
    {
      ExFreePool (ImageSections);
      return (STATUS_INVALID_IMAGE_FORMAT);
    }

  /*
   * Create the section
   */
  Status = ObCreateObject (SectionHandle,
			   DesiredAccess,
			   ObjectAttributes,
			   MmSectionObjectType, (PVOID *) & Section);
  if (!NT_SUCCESS (Status))
    {
      ExFreePool (ImageSections);
      return (Status);
    }

  /*
   * Initialize it
   */
  Section->SectionPageProtection = SectionPageProtection;
  Section->AllocateAttributes = AllocationAttributes;
  InitializeListHead (&Section->ViewListHead);
  KeInitializeSpinLock (&Section->ViewListLock);
  KeInitializeMutex (&Section->Lock, 0);
  Section->Flags = MM_IMAGE_SECTION;
  Section->NrSegments = PEHeader.FileHeader.NumberOfSections + 1;
  Section->ImageBase = (PVOID) PEHeader.OptionalHeader.ImageBase;
  Section->EntryPoint = (PVOID) PEHeader.OptionalHeader.AddressOfEntryPoint;
  Section->StackReserve = PEHeader.OptionalHeader.SizeOfStackReserve;
  Section->StackCommit = PEHeader.OptionalHeader.SizeOfStackCommit;
  Section->Subsystem = PEHeader.OptionalHeader.Subsystem;
  Section->MinorSubsystemVersion =
    PEHeader.OptionalHeader.MinorSubsystemVersion;
  Section->MajorSubsystemVersion =
    PEHeader.OptionalHeader.MajorSubsystemVersion;
  Section->ImageCharacteristics = PEHeader.FileHeader.Characteristics;
  Section->Machine = PEHeader.FileHeader.Machine;
  Section->Executable = (PEHeader.OptionalHeader.SizeOfCode != 0);

  /*
   * Check file access required
   */
  if (SectionPageProtection & PAGE_READWRITE ||
      SectionPageProtection & PAGE_EXECUTE_READWRITE)
    {
      FileAccess = FILE_READ_DATA | FILE_WRITE_DATA;
    }
  else
    {
      FileAccess = FILE_READ_DATA;
    }

  /*
   * Reference the file handle
   */
  Status = ObReferenceObjectByHandle (FileHandle,
				      FileAccess,
				      IoFileObjectType,
				      UserMode, (PVOID *) & FileObject, NULL);
  if (!NT_SUCCESS (Status))
    {
      ZwClose (*SectionHandle);
      ObDereferenceObject (Section);
      ExFreePool (ImageSections);
      return (Status);
    }

  /*
   * We can't do memory mappings if the file system doesn't support the
   * standard FCB
   */
  if (!(FileObject->Flags & FO_FCB_IS_VALID))
    {
      ZwClose (*SectionHandle);
      ObDereferenceObject (Section);
      ObDereferenceObject (FileObject);
      ExFreePool (ImageSections);
      return (STATUS_INVALID_FILE_FOR_SECTION);
    }

  /*
   * Lock the file
   */
  Status = KeWaitForSingleObject ((PVOID) & FileObject->Lock,
				  0, KernelMode, FALSE, NULL);
  if (Status != STATUS_SUCCESS)
    {
      ZwClose (*SectionHandle);
      ObDereferenceObject (Section);
      ObDereferenceObject (FileObject);
      ExFreePool (ImageSections);
      return (Status);
    }

  /*
   * If this file hasn't been mapped as a image file before then allocate the
   * section segments to describe the mapping
   */
  NrSegments = PEHeader.FileHeader.NumberOfSections + 1;
  if (FileObject->SectionObjectPointers->ImageSectionObject == NULL)
    {
      ULONG i;
      ULONG Size;

      Size = sizeof (MM_IMAGE_SECTION_OBJECT) +
	(sizeof (MM_SECTION_SEGMENT) * NrSegments);
      ImageSectionObject =
	ExAllocatePoolWithTag (NonPagedPool, Size, TAG_MM_SECTION_SEGMENT);
      if (ImageSectionObject == NULL)
	{
	  KeSetEvent ((PVOID) & FileObject->Lock, IO_NO_INCREMENT, FALSE);
	  ZwClose (*SectionHandle);
	  ObDereferenceObject (Section);
	  ObDereferenceObject (FileObject);
	  ExFreePool (ImageSections);
	  return (STATUS_NO_MEMORY);
	}
      ImageSectionObject->NrSegments = NrSegments;
      SectionSegments = ImageSectionObject->Segments;
      Section->Segments = SectionSegments;

      SectionSegments[0].FileOffset = 0;
      SectionSegments[0].Characteristics = IMAGE_SECTION_CHAR_DATA;
      SectionSegments[0].Protection = PAGE_READWRITE;
      SectionSegments[0].RawLength = PAGESIZE;
      SectionSegments[0].Length = PAGESIZE;
      SectionSegments[0].Flags = 0;
      SectionSegments[0].ReferenceCount = 1;
      SectionSegments[0].VirtualAddress = 0;
      SectionSegments[0].WriteCopy = TRUE;
      KeInitializeMutex (&SectionSegments[0].Lock, 0);

      MaximumSize.QuadPart = 0;

      for (i = 1; i < NrSegments; i++)
	{
	  SectionSegments[i].FileOffset =
	    ImageSections[i - 1].PointerToRawData;
	  SectionSegments[i].Characteristics =
	    ImageSections[i - 1].Characteristics;

	  /*
	   * Set up the protection and write copy variables.
	   */
	  if ((ImageSections[i - 1].
	       Characteristics & IMAGE_SECTION_CHAR_READABLE)
	      || (ImageSections[i - 1].
		  Characteristics & IMAGE_SECTION_CHAR_WRITABLE)
	      || (ImageSections[i - 1].
		  Characteristics & IMAGE_SECTION_CHAR_EXECUTABLE))
	    {
	      SectionSegments[i].Protection =
		SectionCharacteristicsToProtect
		[ImageSections[i - 1].Characteristics >> 28];
	      SectionSegments[i].WriteCopy =
		!(ImageSections[i - 1].
		  Characteristics & IMAGE_SECTION_CHAR_SHARED);
	    }
	  else if (ImageSections[i - 1].
		   Characteristics & IMAGE_SECTION_CHAR_CODE)
	    {
	      SectionSegments[i].Protection = PAGE_EXECUTE_READ;
	      SectionSegments[i].WriteCopy = TRUE;
	    }
	  else if (ImageSections[i - 1].Characteristics &
		   IMAGE_SECTION_CHAR_DATA)
	    {
	      SectionSegments[i].Protection = PAGE_READWRITE;
	      SectionSegments[i].WriteCopy = TRUE;
	    }
	  else if (ImageSections[i - 1].
		   Characteristics & IMAGE_SECTION_CHAR_BSS)
	    {
	      SectionSegments[i].Protection = PAGE_READWRITE;
	      SectionSegments[i].WriteCopy = TRUE;
	    }
	  else
	    {
	      SectionSegments[i].Protection = PAGE_NOACCESS;
	      SectionSegments[i].WriteCopy = TRUE;
	    }

	  /*
	   * Set up the attributes.
	   */
	  if (ImageSections[i - 1].Characteristics & IMAGE_SECTION_CHAR_CODE)
	    {
	      SectionSegments[i].Attributes = 0;
	    }
	  else if (ImageSections[i - 1].
		   Characteristics & IMAGE_SECTION_CHAR_DATA)
	    {
	      SectionSegments[i].Attributes = 0;
	    }
	  else if (ImageSections[i - 1].
		   Characteristics & IMAGE_SECTION_CHAR_BSS)
	    {
	      SectionSegments[i].Attributes = MM_SECTION_SEGMENT_BSS;
	    }
	  else
	    {
	      SectionSegments[i].Attributes = 0;
	    }

	  SectionSegments[i].RawLength = ImageSections[i - 1].SizeOfRawData;
	  SectionSegments[i].Length = ImageSections[i - 1].Misc.VirtualSize;
	  SectionSegments[i].Flags = 0;
	  SectionSegments[i].ReferenceCount = 1;
	  SectionSegments[i].VirtualAddress =
	    (PVOID) ImageSections[i - 1].VirtualAddress;
	  KeInitializeMutex (&SectionSegments[i].Lock, 0);
	  MaximumSize.QuadPart += SectionSegments[i].Length;
	}

      Section->MaximumSize.QuadPart = MaximumSize.QuadPart;

      FileObject->SectionObjectPointers->ImageSectionObject =
	(PVOID) ImageSectionObject;
    }
  else
    {
      ULONG i;

      ImageSectionObject = (PMM_IMAGE_SECTION_OBJECT)
	FileObject->SectionObjectPointers->ImageSectionObject;
      SectionSegments = ImageSectionObject->Segments;
      Section->Segments = SectionSegments;

      MaximumSize.QuadPart = 0;
      for (i = 1; i < NrSegments; i++)
	{
	  MaximumSize.QuadPart += SectionSegments[i].Length;
	}
      Section->MaximumSize.QuadPart = MaximumSize.QuadPart;

      /*
       * Otherwise just reference all the section segments
       */
      for (i = 0; i < NrSegments; i++)
	{
	  InterlockedIncrement (&SectionSegments[i].ReferenceCount);
	}

    }
  ExFreePool (ImageSections);
  KeSetEvent ((PVOID) & FileObject->Lock, IO_NO_INCREMENT, FALSE);
  Section->FileObject = FileObject;

  ObDereferenceObject (Section);
  return (STATUS_SUCCESS);
}

NTSTATUS STDCALL
NtCreateSection (OUT PHANDLE SectionHandle,
		 IN ACCESS_MASK DesiredAccess,
		 IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
		 IN PLARGE_INTEGER MaximumSize OPTIONAL,
		 IN ULONG SectionPageProtection OPTIONAL,
		 IN ULONG AllocationAttributes, IN HANDLE FileHandle OPTIONAL)
{
  if (AllocationAttributes & SEC_IMAGE)
    {
      return (MmCreateImageSection (SectionHandle,
				    DesiredAccess,
				    ObjectAttributes,
				    MaximumSize,
				    SectionPageProtection,
				    AllocationAttributes, FileHandle));
    }
  else if (FileHandle != NULL)
    {
      return (MmCreateDataFileSection (SectionHandle,
				       DesiredAccess,
				       ObjectAttributes,
				       MaximumSize,
				       SectionPageProtection,
				       AllocationAttributes, FileHandle));
    }
  else
    {
      return (MmCreatePageFileSection (SectionHandle,
				       DesiredAccess,
				       ObjectAttributes,
				       MaximumSize,
				       SectionPageProtection,
				       AllocationAttributes));
    }
}


/**********************************************************************
 * NAME
 * 	NtOpenSection
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 * 	SectionHandle
 *
 * 	DesiredAccess
 *
 * 	ObjectAttributes
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
NTSTATUS STDCALL
NtOpenSection (PHANDLE SectionHandle,
	       ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes)
{
  NTSTATUS Status;

  *SectionHandle = 0;

  Status = ObOpenObjectByName (ObjectAttributes,
			       MmSectionObjectType,
			       NULL,
			       UserMode, DesiredAccess, NULL, SectionHandle);

  return (Status);
}

NTSTATUS
MmMapViewOfSegment (PEPROCESS Process,
		    PMADDRESS_SPACE AddressSpace,
		    PSECTION_OBJECT Section,
		    PMM_SECTION_SEGMENT Segment,
		    PVOID * BaseAddress,
		    ULONG ViewSize, ULONG Protect, ULONG ViewOffset)
{
  PMEMORY_AREA MArea;
  NTSTATUS Status;
  KIRQL oldIrql;

  MmLockAddressSpace (&Process->AddressSpace);
  if (Protect == PAGE_NOACCESS || Protect == PAGE_GUARD)
    {
      assertmsg (FALSE,
		 ("Mapping inaccessible region between 0x%.8X and 0x%.8X\n",
		  (*BaseAddress), (*BaseAddress) + ViewSize));
    }
  if (ViewSize == 0)
    {
      DPRINT ("Mapping zero byte region between at 0x%.8X\n", (*BaseAddress));
    }

  Status = MmCreateMemoryArea (Process,
			       &Process->AddressSpace,
			       MEMORY_AREA_SECTION_VIEW_COMMIT,
			       BaseAddress, ViewSize, Protect, &MArea, FALSE);
  MmUnlockAddressSpace (&Process->AddressSpace);
  if (!NT_SUCCESS (Status))
    {
      assertmsg (FALSE,
		 ("Mapping between 0x%.8X and 0x%.8X failed.\n",
		  (*BaseAddress), (*BaseAddress) + ViewSize));
      return (Status);
    }

  KeAcquireSpinLock (&Section->ViewListLock, &oldIrql);
  InsertTailList (&Section->ViewListHead,
		  &MArea->Data.SectionData.ViewListEntry);
  KeReleaseSpinLock (&Section->ViewListLock, oldIrql);

  ObReferenceObjectByPointer ((PVOID) Section,
			      SECTION_MAP_READ, NULL, ExGetPreviousMode ());
  MArea->Data.SectionData.Segment = Segment;
  MArea->Data.SectionData.Section = Section;
  MArea->Data.SectionData.ViewOffset = ViewOffset;
  MArea->Data.SectionData.WriteCopyView = FALSE;

  return (STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtMapViewOfSection
 *
 * DESCRIPTION
 *	Maps a view of a section into the virtual address space of a 
 *	process.
 *	
 * ARGUMENTS
 *	SectionHandle
 *		Handle of the section.
 *		
 *	ProcessHandle
 *		Handle of the process.
 *		
 *	BaseAddress
 *		Desired base address (or NULL) on entry;
 *		Actual base address of the view on exit.
 *		
 *	ZeroBits
 *		Number of high order address bits that must be zero.
 *		
 *	CommitSize
 *		Size in bytes of the initially committed section of 
 *		the view.
 *		
 *	SectionOffset
 *		Offset in bytes from the beginning of the section
 *		to the beginning of the view.
 *		
 *	ViewSize
 *		Desired length of map (or zero to map all) on entry
 *		Actual length mapped on exit.
 *		
 *	InheritDisposition
 *		Specified how the view is to be shared with
 *		child processes.
 *		
 *	AllocateType
 *		Type of allocation for the pages.
 *		
 *	Protect
 *		Protection for the committed region of the view.
 *
 * RETURN VALUE
 * 	Status.
 */
NTSTATUS STDCALL
NtMapViewOfSection (HANDLE SectionHandle,
		    HANDLE ProcessHandle,
		    PVOID * BaseAddress,
		    ULONG ZeroBits,
		    ULONG CommitSize,
		    PLARGE_INTEGER SectionOffset,
		    PULONG ViewSize,
		    SECTION_INHERIT InheritDisposition,
		    ULONG AllocationType, ULONG Protect)
{
  PSECTION_OBJECT Section;
  PEPROCESS Process;
  NTSTATUS Status;
  PMADDRESS_SPACE AddressSpace;

  Status = ObReferenceObjectByHandle (ProcessHandle,
				      PROCESS_VM_OPERATION,
				      PsProcessType,
				      UserMode, (PVOID *) & Process, NULL);
  if (!NT_SUCCESS (Status))
    {
      return (Status);
    }

  AddressSpace = &Process->AddressSpace;

  Status = ObReferenceObjectByHandle (SectionHandle,
				      SECTION_MAP_READ,
				      MmSectionObjectType,
				      UserMode, (PVOID *) & Section, NULL);
  if (!(NT_SUCCESS (Status)))
    {
      DPRINT ("ObReference failed rc=%x\n", Status);
      ObDereferenceObject (Process);
      return (Status);
    }

  Status = MmMapViewOfSection (Section,
			       Process,
			       BaseAddress,
			       ZeroBits,
			       CommitSize,
			       SectionOffset,
			       ViewSize,
			       InheritDisposition, AllocationType, Protect);

  ObDereferenceObject (Section);
  ObDereferenceObject (Process);

  return (Status);
}


VOID
MmFreeSectionPage (IN BOOLEAN Before,
		   IN PVOID Context,
		   IN PMEMORY_AREA MemoryArea,
		   IN PVOID Address,
		   IN ULONG_PTR PhysicalAddress,
		   IN SWAPENTRY SwapEntry, IN BOOLEAN Dirty)
{
  PMEMORY_AREA MArea;
  ULONG Entry;

  DPRINT
    ("MmFreeSectionPage(Before %d  MArea 0x%.08x  Address 0x%.08x, SwapEntry 0x%.08x  Dirty %d)\n",
     Before, Context, Address, SwapEntry, Dirty);

  MArea = (PMEMORY_AREA) Context;

  if (Before)
    {
      PFILE_OBJECT FileObject;
      PSECTION_OBJECT Section;
      ULONG Offset;

      Section = MArea->Data.SectionData.Section;
      assert (Section);
      FileObject = Section->FileObject;

      Offset = ((ULONG) PAGE_ROUND_DOWN (Address) -
		(ULONG) MArea->BaseAddress) +
	(ULONG) PAGE_ROUND_DOWN (MArea->Data.SectionData.ViewOffset);

      Entry = MmGetPageEntrySectionSegment (MArea->Data.SectionData.
					    Segment, Offset);
      if (IS_SWAP_FROM_SSE (Entry))
	{
	  assert (FALSE);
	}
      else if (FileObject != NULL)
	{
	  LARGE_INTEGER FlushFileOffset;
	  NTSTATUS Status;

	  /*
	   * FIXME: Image sections can be flushed too 
	   */
	  if (!(Section->Flags & MM_IMAGE_SECTION))
	    {
	      /*
	       * FIXME: As an optimization, don't call MmFlushSection()
	       * if the page is not dirty 
	       */

	      FlushFileOffset.u.HighPart = 0;
	      FlushFileOffset.u.LowPart = Offset;

	      /*
	       * This section page is backed by a file so flush the
	       * section page 
	       */
	      Status = MmFlushSection (Section,
				       &FlushFileOffset, PAGESIZE, NULL);

	      if (!NT_SUCCESS (Status))
		{
		  DPRINT ("MmFlushSection() failed with status 0x%.08x\n",
			  Status);
		}
	    }
	}
    }
  else
    {				/* !Before */

      if (SwapEntry != 0)
	{
	  MmFreeSwapPage (SwapEntry);
	}
      else if (PhysicalAddress != 0)
	{
	  ULONG Offset;

	  Offset = ((ULONG) PAGE_ROUND_DOWN (Address) -
		    (ULONG) MArea->BaseAddress) +
	    MArea->Data.SectionData.ViewOffset;

	  Entry = MmGetPageEntrySectionSegment (MArea->Data.
						SectionData.Segment, Offset);
	  if (IS_SWAP_FROM_SSE (Entry))
	    {
	      assert (FALSE);
	    }
	  else if (PhysicalAddress != (PAGE_FROM_SSE (Entry)))
	    {
	      /*
	       * Just dereference private pages
	       */
	      MmDeleteRmap (PhysicalAddress, MArea->Process, Address);
	      MmDereferencePage (PhysicalAddress);
	    }
	  else
	    {
	      MmUnsharePageEntrySectionSegment (MArea->Data.
						SectionData.
						Section,
						MArea->Data.
						SectionData.
						Segment, Offset, Dirty);
	      MmDeleteRmap (PhysicalAddress, MArea->Process, Address);
	      MmDereferencePage (PhysicalAddress);
	    }
	}
    }
}

NTSTATUS STDCALL
MmUnmapViewOfSection (PEPROCESS Process, PVOID BaseAddress)
{
  NTSTATUS Status;
  PMEMORY_AREA MemoryArea;
  PMADDRESS_SPACE AddressSpace;
  PSECTION_OBJECT Section;
  PMM_SECTION_SEGMENT Segment;
  KIRQL oldIrql;

  AddressSpace = &Process->AddressSpace;

  DPRINT ("Opening memory area Process %x BaseAddress %x\n",
	  Process, BaseAddress);
  MmLockAddressSpace (AddressSpace);
  MemoryArea = MmOpenMemoryAreaByAddress (AddressSpace, BaseAddress);
  if (MemoryArea == NULL)
    {
      MmUnlockAddressSpace (AddressSpace);
      return (STATUS_UNSUCCESSFUL);
    }

  Section = MemoryArea->Data.SectionData.Section;
  Segment = MemoryArea->Data.SectionData.Segment;
  MmLockSection (Section);
  MmLockSectionSegment (Segment);
  KeAcquireSpinLock (&Section->ViewListLock, &oldIrql);
  RemoveEntryList (&MemoryArea->Data.SectionData.ViewListEntry);
  KeReleaseSpinLock (&Section->ViewListLock, oldIrql);
  MmCloseMemoryArea (MemoryArea);
  if (MemoryArea->Data.SectionData.Section->Flags & SO_PHYSICAL_MEMORY)
    {
      Status = MmFreeMemoryArea (&Process->AddressSpace,
				 BaseAddress, 0, NULL, NULL);
    }
  else
    {
      Status = MmFreeMemoryArea (&Process->AddressSpace,
				 BaseAddress,
				 0, MmFreeSectionPage, MemoryArea);
    }
  MmUnlockSectionSegment (Segment);
  MmUnlockSection (Section);
  ObDereferenceObject (Section);
  MmUnlockAddressSpace (AddressSpace);
  return (STATUS_SUCCESS);
}

/**********************************************************************
 * NAME							EXPORTED
 *	NtUnmapViewOfSection
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *	ProcessHandle
 *
 *	BaseAddress
 *
 * RETURN VALUE
 *	Status.
 *
 * REVISIONS
 *
 */
NTSTATUS STDCALL
NtUnmapViewOfSection (HANDLE ProcessHandle, PVOID BaseAddress)
{
  PEPROCESS Process;
  NTSTATUS Status;

  DPRINT ("NtUnmapViewOfSection(ProcessHandle %x, BaseAddress %x)\n",
	  ProcessHandle, BaseAddress);

  DPRINT ("Referencing process\n");
  Status = ObReferenceObjectByHandle (ProcessHandle,
				      PROCESS_VM_OPERATION,
				      PsProcessType,
				      UserMode, (PVOID *) & Process, NULL);
  if (!NT_SUCCESS (Status))
    {
      DPRINT ("ObReferenceObjectByHandle failed (Status %x)\n", Status);
      return (Status);
    }

  Status = MmUnmapViewOfSection (Process, BaseAddress);

  ObDereferenceObject (Process);

  return Status;
}


NTSTATUS STDCALL
NtQuerySection (IN HANDLE SectionHandle,
		IN CINT SectionInformationClass,
		OUT PVOID SectionInformation,
		IN ULONG Length, OUT PULONG ResultLength)
/*
 * FUNCTION: Queries the information of a section object.
 * ARGUMENTS: 
 *        SectionHandle = Handle to the section link object
 *        SectionInformationClass = Index to a certain information structure
 *        SectionInformation (OUT)= Caller supplies storage for resulting 
 *                                  information
 *        Length =  Size of the supplied storage 
 *        ResultLength = Data written
 * RETURNS: Status
 *
 */
{
  PSECTION_OBJECT Section;
  NTSTATUS Status;

  Status = ObReferenceObjectByHandle (SectionHandle,
				      SECTION_MAP_READ,
				      MmSectionObjectType,
				      UserMode, (PVOID *) & Section, NULL);
  if (!(NT_SUCCESS (Status)))
    {
      return (Status);
    }

  switch (SectionInformationClass)
    {
    case SectionBasicInformation:
      {
	PSECTION_BASIC_INFORMATION Sbi;

	if (Length != sizeof (SECTION_BASIC_INFORMATION))
	  {
	    ObDereferenceObject (Section);
	    return (STATUS_INFO_LENGTH_MISMATCH);
	  }

	Sbi = (PSECTION_BASIC_INFORMATION) SectionInformation;

	Sbi->BaseAddress = 0;
	Sbi->Attributes = 0;
	Sbi->Size.QuadPart = 0;

	Status = STATUS_SUCCESS;

	break;
      }

    case SectionImageInformation:
      {
	PSECTION_IMAGE_INFORMATION Sii;

	if (Length != sizeof (SECTION_IMAGE_INFORMATION))
	  {
	    ObDereferenceObject (Section);
	    return (STATUS_INFO_LENGTH_MISMATCH);
	  }
	Sii = (PSECTION_IMAGE_INFORMATION) SectionInformation;
	Sii->EntryPoint = Section->EntryPoint;
	Sii->Unknown1 = 0;
	Sii->StackReserve = Section->StackReserve;
	Sii->StackCommit = Section->StackCommit;
	Sii->Subsystem = Section->Subsystem;
	Sii->MinorSubsystemVersion = Section->MinorSubsystemVersion;
	Sii->MajorSubsystemVersion = Section->MajorSubsystemVersion;
	Sii->Unknown2 = 0;
	Sii->Characteristics = Section->ImageCharacteristics;
	Sii->ImageNumber = Section->Machine;
	Sii->Executable = Section->Executable;
	Sii->Unknown3 = 0;
	Sii->Unknown4[0] = 0;
	Sii->Unknown4[1] = 0;
	Sii->Unknown4[2] = 0;

	Status = STATUS_SUCCESS;
	break;
      }

    default:
      Status = STATUS_INVALID_INFO_CLASS;
    }
  ObDereferenceObject (Section);
  return (Status);
}


NTSTATUS STDCALL
NtExtendSection (IN HANDLE SectionHandle, IN ULONG NewMaximumSize)
{
  UNIMPLEMENTED;
}


/**********************************************************************
 * NAME							INTERNAL
 * 	MmFlushSection
 *
 * DESCRIPTION
 *   Flush modified pages in a range to secondary storage
 *
 * ARGUMENTS
 *   SectionObject - Pointer to a section object
 *   FileOffset    - Optional starting file offset where flushing starts
 * 	 Length        - Size of range to flush
 *   IoStatus      - Optional pointer to I/O status block
 *
 * NOTES
 *   If FileOffset is NULL, the entire section range is flushed.
 *
 * RETURN VALUE
 *   Status
 *
 */
NTSTATUS
MmFlushSection (IN PSECTION_OBJECT SectionObject,
		IN PLARGE_INTEGER FileOffset OPTIONAL,
		IN ULONG Length, OUT PIO_STATUS_BLOCK IoStatus OPTIONAL)
{
  LARGE_INTEGER FlushFileOffset;
  LARGE_INTEGER FlushFileLength;
  PMM_SECTION_SEGMENT Segment;
  NTSTATUS Status;

  DPRINT ("MmFlushSection(SectionObject 0x%.08x)\n", SectionObject);

  if (FileOffset)
    {
      DPRINT ("FileOffset 0x%.08x  Length 0x%.08x\n",
	      FileOffset->u.LowPart, Length);
      assertmsg ((FileOffset->QuadPart % PAGESIZE) == 0,
		 ("FileOffset 0x%.08x is not page aligned\n",
		  FileOffset->u.LowPart));
    }

  Segment = SectionObject->Segments;
  MmLockSection (SectionObject);
  MmLockSectionSegment (Segment);

  if (FileOffset)
    {
      if (FileOffset->QuadPart > SectionObject->MaximumSize.QuadPart)
	{
	  /*
	   * Don't flush outside maximum section size 
	   */
	  DPRINT
	    ("FileOffset 0x%.08x > SectionObject->MaximumSize 0x%.08x\n",
	     FileOffset->u.LowPart, SectionObject->MaximumSize.u.LowPart);
	  MmUnlockSectionSegment (Segment);
	  MmUnlockSection (SectionObject);
	  return STATUS_UNSUCCESSFUL;
	}

      if (Length == 0)
	{
	  /*
	   * Nothing to flush 
	   */
	  MmUnlockSectionSegment (Segment);
	  MmUnlockSection (SectionObject);
	  if (IoStatus)
	    {
	      IoStatus->Information = 0;
	      IoStatus->Status = STATUS_SUCCESS;
	    }
	  return STATUS_SUCCESS;
	}

      FlushFileOffset.QuadPart = FileOffset->QuadPart;
      FlushFileLength.QuadPart = Length;

      if (FlushFileOffset.QuadPart + FlushFileLength.QuadPart >
	  SectionObject->MaximumSize.QuadPart)
	{
	  /*
	   * Don't flush outside maximum section size 
	   */
	  FlushFileLength.QuadPart =
	    SectionObject->MaximumSize.QuadPart - FlushFileOffset.QuadPart;
	}
    }
  else
    {
      FlushFileOffset.QuadPart = 0;
      FlushFileLength.QuadPart = SectionObject->MaximumSize.QuadPart;
    }

  DPRINT ("FlushFileOffset 0x%.08x  FlushFileLength 0x%.08x\n",
	  FlushFileOffset.u.LowPart, FlushFileLength.u.LowPart);

  Status = STATUS_SUCCESS;
  if (!(Segment->Flags & MM_PAGEFILE_SECTION))
    {
      NTSTATUS NtStatus;
      ULONG_PTR Entry;
      ULONG i;

      for (i = 0;
	   i < (FlushFileLength.QuadPart + PAGESIZE - 1) / PAGESIZE; i++)
	{
	  /*
	   * FIXME: Investigate if MPW causes trouble if flushing the
	   * same page 
	   */

	  /*
	   * Get the entry corresponding to the offset within the section
	   */
	  Entry = MmGetPageEntrySectionSegment (Segment,
						FlushFileOffset.u.LowPart);
	  if (Entry != 0)
	    {
	      NtStatus = MmFlushPhysicalAddress (Entry);
	      if (!NT_SUCCESS (NtStatus))
		{
		  assertmsg (FALSE,
			     ("MmFlushPhysicalAddress() failed with status 0x%.08x\n",
			      NtStatus));
		}
	      if (NT_SUCCESS (Status))
		{
		  /*
		   * Flush as many pages as possible 
		   */
		  Status = NtStatus;
		}
	    }
	  else
	    {
	      /*
	       * CH: I'm not sure if this is a bug, but image section
	       * segments can be non-page aligned so this routine can be 
	       * called more than once from MmFreeSectionPage() for the
	       * same page when flushing right before the physical page
	       * is freed. The second time this routine is called the
	       * mapping does not exist anymore. 
	       */
	      DPRINT ("Section offset 0x%.08x does not exist!\n",
		      FlushFileOffset.u.LowPart);
	    }
	  FlushFileOffset.u.LowPart += PAGESIZE;
	}
    }

  MmUnlockSectionSegment (Segment);
  MmUnlockSection (SectionObject);

  if (IoStatus)
    {
      IoStatus->Information = FlushFileLength.u.LowPart;
      IoStatus->Status = Status;
    }
  return Status;
}


/**********************************************************************
 * NAME							INTERNAL
 * 	MmAllocateSection@4
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 * 	Length
 *
 * RETURN VALUE
 *
 * NOTE
 * 	Code taken from ntoskrnl/mm/special.c.
 *
 * REVISIONS
 *
 */
PVOID STDCALL
MmAllocateSection (IN ULONG Length)
{
  PVOID Result;
  PMEMORY_AREA MemoryArea;
  NTSTATUS Status;
  ULONG i;
  PMADDRESS_SPACE AddressSpace;

  DPRINT ("MmAllocateSection(Length %x)\n", Length);

  AddressSpace = MmGetKernelAddressSpace ();
  Result = NULL;
  MmLockAddressSpace (AddressSpace);
  Status = MmCreateMemoryArea (NULL,
			       AddressSpace,
			       MEMORY_AREA_SYSTEM,
			       &Result,
			       Length, PAGE_READWRITE, &MemoryArea, FALSE);
  if (!NT_SUCCESS (Status))
    {
      MmUnlockAddressSpace (AddressSpace);
      return (NULL);
    }
  MmUnlockAddressSpace (AddressSpace);
  DPRINT ("Result %p\n", Result);
  for (i = 0; (i <= (Length / PAGESIZE)); i++)
    {
      ULONG_PTR Page;

      Status = MmRequestPageMemoryConsumer (MC_NPPOOL, TRUE, &Page);
      if (!NT_SUCCESS (Status))
	{
	  DbgPrint ("Unable to allocate page\n");
	  KeBugCheck (0);
	}
      Status = MmCreateVirtualMapping (NULL,
				       (Result + (i * PAGESIZE)),
				       MemoryArea->Attributes,
				       (ULONG) Page, TRUE);
      if (!NT_SUCCESS (Status))
	{
	  DbgPrint ("Unable to create virtual mapping\n");
	  KeBugCheck (0);
	}
    }
  return ((PVOID) Result);
}


/**********************************************************************
 * NAME							EXPORTED
 *	MmMapViewOfSection
 *
 * DESCRIPTION
 *	Maps a view of a section into the virtual address space of a 
 *	process.
 *	
 * ARGUMENTS
 *	Section
 *		Pointer to the section object.
 *		
 *	ProcessHandle
 *		Pointer to the process.
 *		
 *	BaseAddress
 *		Desired base address (or NULL) on entry;
 *		Actual base address of the view on exit.
 *		
 *	ZeroBits
 *		Number of high order address bits that must be zero.
 *		
 *	CommitSize
 *		Size in bytes of the initially committed section of 
 *		the view.
 *		
 *	SectionOffset
 *		Offset in bytes from the beginning of the section
 *		to the beginning of the view.
 *		
 *	ViewSize
 *		Desired length of map (or zero to map all) on entry
 *		Actual length mapped on exit.
 *		
 *	InheritDisposition
 *		Specified how the view is to be shared with
 *		child processes.
 *		
 *	AllocationType
 *		Type of allocation for the pages.
 *		
 *	Protect
 *		Protection for the committed region of the view.
 *
 * RETURN VALUE
 *	Status.
 */
NTSTATUS STDCALL
MmMapViewOfSection (IN PVOID SectionObject,
		    IN PEPROCESS Process,
		    IN OUT PVOID * BaseAddress,
		    IN ULONG ZeroBits,
		    IN ULONG CommitSize,
		    IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
		    IN OUT PULONG ViewSize,
		    IN SECTION_INHERIT InheritDisposition,
		    IN ULONG AllocationType, IN ULONG Protect)
{
  PSECTION_OBJECT Section;
  PMADDRESS_SPACE AddressSpace;
  ULONG ViewOffset;
  NTSTATUS Status;

  Section = (PSECTION_OBJECT) SectionObject;
  AddressSpace = &Process->AddressSpace;

  MmLockAddressSpace (AddressSpace);
  MmLockSection (SectionObject);

  if (Section->Flags & MM_IMAGE_SECTION)
    {
      ULONG i;

      for (i = 0; i < Section->NrSegments; i++)
	{
	  PVOID SBaseAddress;

	  if (!(Section->Segments[i].Characteristics & IMAGE_SECTION_NOLOAD))
	    {
	      SBaseAddress = (PVOID)
		((ULONG) Section->ImageBase +
		 (ULONG) Section->Segments[i].VirtualAddress);

	      MmLockSectionSegment (&Section->Segments[i]);
	      Status = MmMapViewOfSegment (Process,
					   &Process->
					   AddressSpace,
					   Section,
					   &Section->
					   Segments[i],
					   &SBaseAddress,
					   Section->
					   Segments[i].
					   Length,
					   Section->
					   Segments[i].
					   Protection,
					   Section->Segments[i].FileOffset);
	      MmUnlockSectionSegment (&Section->Segments[i]);
	      if (!NT_SUCCESS (Status))
		{
		  MmUnlockSection (Section);
		  MmUnlockAddressSpace (AddressSpace);
		  return (Status);
		}
	    }
	}
      *BaseAddress = Section->ImageBase;
    }
  else
    {
      if (SectionOffset == NULL)
	{
	  ViewOffset = 0;
	}
      else
	{
	  ViewOffset = SectionOffset->u.LowPart;
	}

      if ((ViewOffset % PAGESIZE) != 0)
	{
	  MmUnlockSection (Section);
	  MmUnlockAddressSpace (AddressSpace);
	  return (STATUS_MAPPED_ALIGNMENT);
	}

      if (((*ViewSize) == 0)
	  || ((*ViewSize) + ViewOffset) > Section->MaximumSize.u.LowPart)
	{
	  (*ViewSize) = Section->MaximumSize.u.LowPart - ViewOffset;
	}

      MmLockSectionSegment (Section->Segments);
      Status = MmMapViewOfSegment (Process,
				   &Process->AddressSpace,
				   Section,
				   Section->Segments,
				   BaseAddress,
				   *ViewSize, Protect, ViewOffset);
      MmUnlockSectionSegment (Section->Segments);
      if (!NT_SUCCESS (Status))
	{
	  MmUnlockSection (Section);
	  MmUnlockAddressSpace (AddressSpace);
	  return (Status);
	}
    }

  MmUnlockSection (Section);
  MmUnlockAddressSpace (AddressSpace);

  return (STATUS_SUCCESS);
}


BOOLEAN STDCALL
MmCanFileBeTruncated (IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
		      IN PLARGE_INTEGER NewFileSize)
{
  UNIMPLEMENTED;
  return (FALSE);
}


BOOLEAN STDCALL
MmDisableModifiedWriteOfSection (DWORD Unknown0)
{
  UNIMPLEMENTED;
  return (FALSE);
}

BOOLEAN STDCALL
MmFlushImageSection (IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
		     IN MMFLUSH_TYPE FlushType)
{
  UNIMPLEMENTED;
  return (FALSE);
}

BOOLEAN STDCALL
MmForceSectionClosed (DWORD Unknown0, DWORD Unknown1)
{
  UNIMPLEMENTED;
  return (FALSE);
}


NTSTATUS STDCALL
MmMapViewInSystemSpace (IN PVOID Section,
			OUT PVOID * MappedBase, IN PULONG ViewSize)
{
  UNIMPLEMENTED;
  return (STATUS_NOT_IMPLEMENTED);
}

NTSTATUS STDCALL
MmUnmapViewInSystemSpace (DWORD Unknown0)
{
  UNIMPLEMENTED;
  return (STATUS_NOT_IMPLEMENTED);
}


NTSTATUS STDCALL
MmSetBankedSection (DWORD Unknown0,
		    DWORD Unknown1,
		    DWORD Unknown2,
		    DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  UNIMPLEMENTED;
  return (STATUS_NOT_IMPLEMENTED);
}


/**********************************************************************
 * NAME							EXPORTED
 * 	MmCreateSection@
 * 	
 * DESCRIPTION
 * 	Creates a section object.
 * 	
 * ARGUMENTS
 *	SectionObjiect (OUT)
 *		Caller supplied storage for the resulting pointer
 *		to a SECTION_BOJECT instance;
 *		
 *	DesiredAccess
 *		Specifies the desired access to the section can be a
 *		combination of:
 *			STANDARD_RIGHTS_REQUIRED	|
 *			SECTION_QUERY			|
 *			SECTION_MAP_WRITE		|
 *			SECTION_MAP_READ		|
 *			SECTION_MAP_EXECUTE
 *			
 *	ObjectAttributes [OPTIONAL]
 *		Initialized attributes for the object can be used 
 *		to create a named section;
 *
 *	MaximumSize
 *		Maximizes the size of the memory section. Must be 
 *		non-NULL for a page-file backed section. 
 *		If value specified for a mapped file and the file is 
 *		not large enough, file will be extended.
 *		
 *	SectionPageProtection
 *		Can be a combination of:
 *			PAGE_READONLY	| 
 *			PAGE_READWRITE	|
 *			PAGE_WRITEONLY	| 
 *			PAGE_WRITECOPY
 *			
 *	AllocationAttributes
 *		Can be a combination of:
 *			SEC_IMAGE	| 
 *			SEC_RESERVE
 *			
 *	FileHandle
 *		Handle to a file to create a section mapped to a file
 *		instead of a memory backed section;
 *
 *	File
 *		Unknown.
 *	
 * RETURN VALUE
 * 	Status.
 */
NTSTATUS STDCALL
MmCreateSection (OUT PSECTION_OBJECT * SectionObject,
		 IN ACCESS_MASK DesiredAccess,
		 IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
		 IN PLARGE_INTEGER MaximumSize,
		 IN ULONG SectionPageProtection,
		 IN ULONG AllocationAttributes,
		 IN HANDLE FileHandle OPTIONAL, IN PFILE_OBJECT File OPTIONAL)
{
  return (STATUS_NOT_IMPLEMENTED);
}

/*
 * EOF 
 */
