/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/dbgbuffer.c
 * PROGRAMER:       James Tabor
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
PDEBUG_BUFFER STDCALL
RtlCreateQueryDebugBuffer(IN ULONG Size,
                          IN BOOLEAN EventPair)
{
   NTSTATUS Status;
   PDEBUG_BUFFER Buf = NULL;
   ULONG SectionSize  = 100 * PAGE_SIZE;

   Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                    (PVOID)&Buf,
                                     0,
                                    &SectionSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
        return NULL;
     }

   Buf->SectionBase = Buf;
   Buf->SectionSize = SectionSize;

   DPRINT("RtlCQDB: BA: %x BS: %d\n", Buf->SectionBase, Buf->SectionSize);

   return Buf;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlDestroyQueryDebugBuffer(IN PDEBUG_BUFFER Buf)
{
   NTSTATUS Status = STATUS_SUCCESS;

   if (NULL != Buf)
     {
     Status = NtFreeVirtualMemory( NtCurrentProcess(),
                                  (PVOID)&Buf,
                                  &Buf->SectionSize,
                                   MEM_RELEASE);
     }
   if (!NT_SUCCESS(Status))
     {
        DPRINT1("RtlDQDB: Failed to free VM!\n");
     }
   return Status;
}

/*
 *	Based on lib/epsapi/enum/modules.c by KJK::Hyperion.
 *
 */
NTSTATUS STDCALL
RtlpQueryRemoteProcessModules(HANDLE ProcessHandle,
                              IN PMODULE_INFORMATION ModuleInformation OPTIONAL,
                              IN ULONG Size OPTIONAL,
                              OUT PULONG ReturnedSize)
{
  PROCESS_BASIC_INFORMATION pbiInfo;
  PPEB_LDR_DATA ppldLdrData;
  LDR_DATA_TABLE_ENTRY lmModule;
  PLIST_ENTRY pleListHead;
  PLIST_ENTRY pleCurEntry;

  PDEBUG_MODULE_INFORMATION ModulePtr = NULL;
  NTSTATUS Status = STATUS_SUCCESS;
  ULONG UsedSize = sizeof(ULONG);
  ANSI_STRING AnsiString;
  PCHAR p;

  DPRINT("RtlpQueryRemoteProcessModules Start\n");

  /* query the process basic information (includes the PEB address) */
  Status = NtQueryInformationProcess ( ProcessHandle,
                                       ProcessBasicInformation,
                                       &pbiInfo,
                                       sizeof(PROCESS_BASIC_INFORMATION),
                                       NULL);

  if (!NT_SUCCESS(Status))
    {
       /* failure */
       DPRINT("NtQueryInformationProcess 1 &x \n", Status);
       return Status;
    }

  if (ModuleInformation == NULL || Size == 0)
    {
      Status = STATUS_INFO_LENGTH_MISMATCH;
    }
  else
    {
      ModuleInformation->ModuleCount = 0;
      ModulePtr = &ModuleInformation->ModuleEntry[0];
      Status = STATUS_SUCCESS;
    }

  /* get the address of the PE Loader data */
  Status = NtReadVirtualMemory ( ProcessHandle,
                                &(pbiInfo.PebBaseAddress->Ldr),
                                &ppldLdrData,
                                sizeof(ppldLdrData),
                                NULL );

  if (!NT_SUCCESS(Status))
    {
       /* failure */
       DPRINT("NtReadVirtualMemory 1 %x \n", Status);
       return Status;
    }


  /* head of the module list: the last element in the list will point to this */
  pleListHead = &ppldLdrData->InLoadOrderModuleList;

  /* get the address of the first element in the list */
  Status = NtReadVirtualMemory ( ProcessHandle,
               &(ppldLdrData->InLoadOrderModuleList.Flink),
                                &pleCurEntry,
                                 sizeof(pleCurEntry),
                                 NULL );

   if (!NT_SUCCESS(Status))
     {
        /* failure */
        DPRINT("NtReadVirtualMemory 2 %x \n", Status);
        return Status;
     }

  while(pleCurEntry != pleListHead)
  {

   UNICODE_STRING Unicode;
   WCHAR  Buffer[256 * sizeof(WCHAR)];

   /* read the current module */
   Status = NtReadVirtualMemory ( ProcessHandle,
            CONTAINING_RECORD(pleCurEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderModuleList),
                                 &lmModule,
                                 sizeof(LDR_DATA_TABLE_ENTRY),
                                 NULL );

   /* Import module name from remote Process user space. */
   Unicode.Length = lmModule.FullDllName.Length;
   Unicode.MaximumLength = lmModule.FullDllName.MaximumLength;
   Unicode.Buffer = Buffer;

   Status = NtReadVirtualMemory ( ProcessHandle,
                                  lmModule.FullDllName.Buffer,
                                  Unicode.Buffer,
                                  Unicode.Length,
                                  NULL );

   if (!NT_SUCCESS(Status))
     {
        /* failure */
        DPRINT( "NtReadVirtualMemory 3 %x \n", Status);
        return Status;
     }

      DPRINT("  Module %wZ\n", &Unicode);

      if (UsedSize > Size)
        {
          Status = STATUS_INFO_LENGTH_MISMATCH;
        }
      else if (ModuleInformation != NULL)
        {
          ModulePtr->Reserved[0] = ModulePtr->Reserved[1] = 0;      // FIXME: ??
          ModulePtr->Base        = lmModule.DllBase;
          ModulePtr->Size        = lmModule.SizeOfImage;
          ModulePtr->Flags       = lmModule.Flags;
          ModulePtr->Index       = 0;      // FIXME:  ??
          ModulePtr->Unknown     = 0;      // FIXME: ??
          ModulePtr->LoadCount   = lmModule.LoadCount;

          AnsiString.Length        = 0;
          AnsiString.MaximumLength = 256;
          AnsiString.Buffer        = ModulePtr->ImageName;
          RtlUnicodeStringToAnsiString(&AnsiString,
                                       &Unicode,
                                       FALSE);

          p = strrchr(ModulePtr->ImageName, '\\');
          if (p != NULL)
            ModulePtr->ModuleNameOffset = p - ModulePtr->ImageName + 1;
          else
            ModulePtr->ModuleNameOffset = 0;

          ModulePtr++;
          ModuleInformation->ModuleCount++;
        }
      UsedSize += sizeof(DEBUG_MODULE_INFORMATION);

      /* address of the next module in the list */
      pleCurEntry = lmModule.InLoadOrderModuleList.Flink;
  }

  if (ReturnedSize != 0)
       *ReturnedSize = UsedSize;

  DPRINT("RtlpQueryRemoteProcessModules End\n");

    /* success */
  return (STATUS_SUCCESS);
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlQueryProcessDebugInformation(IN ULONG ProcessId,
                                IN ULONG DebugInfoMask,
                                IN OUT PDEBUG_BUFFER Buf)
{
   NTSTATUS Status = STATUS_SUCCESS;
   ULONG Pid = (ULONG) NtCurrentTeb()->Cid.UniqueProcess;

   Buf->InfoClassMask = DebugInfoMask;
   Buf->SizeOfInfo = sizeof(DEBUG_BUFFER);

   DPRINT("QueryProcessDebugInformation Start\n");

/*
      Currently ROS can not read-only from kenrel space, and doesn't
      check for boundaries inside kernel space that are page protected
      from every one but the kernel. aka page 0 - 2
 */
if (ProcessId <= 1)
  {
	Status = STATUS_ACCESS_VIOLATION;
  }
else
if (Pid == ProcessId)
  {
   if (DebugInfoMask & PDI_MODULES)
     {
    PMODULE_INFORMATION Mp;
    ULONG ReturnSize = 0;
    ULONG MSize;

    Mp = (PMODULE_INFORMATION)(Buf + Buf->SizeOfInfo);

    /* I like this better than the do & while loop. */
    Status = LdrQueryProcessModuleInformation( NULL,
                                               0 ,
                                              &ReturnSize);
    Status = LdrQueryProcessModuleInformation( Mp,
                                               ReturnSize ,
                                              &ReturnSize);
   if (!NT_SUCCESS(Status))
     {
         return Status;
     }

    MSize = Mp->ModuleCount * (sizeof(MODULE_INFORMATION) + 8);
    Buf->ModuleInformation = Mp;
    Buf->SizeOfInfo = Buf->SizeOfInfo + MSize;
     }

   if (DebugInfoMask & PDI_HEAPS)
     {
   PHEAP_INFORMATION Hp;
   ULONG HSize;

   Hp = (PHEAP_INFORMATION)(Buf + Buf->SizeOfInfo);
   HSize = sizeof(HEAP_INFORMATION);
        if (DebugInfoMask & PDI_HEAP_TAGS)
          {
          }
        if (DebugInfoMask & PDI_HEAP_BLOCKS)
          {
          }
   Buf->HeapInformation = Hp;
   Buf->SizeOfInfo = Buf->SizeOfInfo + HSize;

     }

   if (DebugInfoMask & PDI_LOCKS)
     {
   PLOCK_INFORMATION Lp;
   ULONG LSize;

   Lp = (PLOCK_INFORMATION)(Buf + Buf->SizeOfInfo);
   LSize = sizeof(LOCK_INFORMATION);
   Buf->LockInformation = Lp;
   Buf->SizeOfInfo = Buf->SizeOfInfo + LSize;
    }

   DPRINT("QueryProcessDebugInformation end \n");
   DPRINT("QueryDebugInfo : %d\n", Buf->SizeOfInfo);
}
else
{
  HANDLE hProcess;
  CLIENT_ID ClientId;
  OBJECT_ATTRIBUTES ObjectAttributes;

       Buf->Unknown[0] = (ULONG)NtCurrentProcess();

       ClientId.UniqueThread = 0;
       ClientId.UniqueProcess = (HANDLE)ProcessId;
       InitializeObjectAttributes(&ObjectAttributes,
                                  NULL,
                                  0,
                                  NULL,
                                  NULL);

       Status = NtOpenProcess( &hProcess,
                                (PROCESS_ALL_ACCESS),
                               &ObjectAttributes,
                               &ClientId );
       if (!NT_SUCCESS(Status))
         {
           return Status;
         }

   if (DebugInfoMask & PDI_MODULES)
     {
    PMODULE_INFORMATION Mp;
    ULONG ReturnSize = 0;
    ULONG MSize;

    Mp = (PMODULE_INFORMATION)(Buf + Buf->SizeOfInfo);

    Status = RtlpQueryRemoteProcessModules( hProcess,
                                            NULL,
                                            0,
                                           &ReturnSize);

    Status = RtlpQueryRemoteProcessModules( hProcess,
                                            Mp,
                                            ReturnSize ,
                                           &ReturnSize);
   if (!NT_SUCCESS(Status))
     {
         return Status;
     }

    MSize = Mp->ModuleCount * (sizeof(MODULE_INFORMATION) + 8);
    Buf->ModuleInformation = Mp;
    Buf->SizeOfInfo = Buf->SizeOfInfo + MSize;
     }

   if (DebugInfoMask & PDI_HEAPS)
     {
   PHEAP_INFORMATION Hp;
   ULONG HSize;

   Hp = (PHEAP_INFORMATION)(Buf + Buf->SizeOfInfo);
   HSize = sizeof(HEAP_INFORMATION);
        if (DebugInfoMask & PDI_HEAP_TAGS)
          {
          }
        if (DebugInfoMask & PDI_HEAP_BLOCKS)
          {
          }
   Buf->HeapInformation = Hp;
   Buf->SizeOfInfo = Buf->SizeOfInfo + HSize;

     }

   if (DebugInfoMask & PDI_LOCKS)
     {
   PLOCK_INFORMATION Lp;
   ULONG LSize;

   Lp = (PLOCK_INFORMATION)(Buf + Buf->SizeOfInfo);
   LSize = sizeof(LOCK_INFORMATION);
   Buf->LockInformation = Lp;
   Buf->SizeOfInfo = Buf->SizeOfInfo + LSize;
    }

   DPRINT("QueryProcessDebugInformation end \n");
   DPRINT("QueryDebugInfo : %d\n", Buf->SizeOfInfo);
}
   return Status;

}

/* EOL */
