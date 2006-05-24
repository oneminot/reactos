/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ob/dirobj.c
 * PURPOSE:         Manages the Object Manager's Directory Implementation,
 *                  such as functions for addition, deletion and lookup into
 *                  the Object Manager's namespace. These routines are separate
 *                  from the Namespace Implementation because they are largely
 *                  independent and could be used for other namespaces.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Thomas Weidenmueller (w3seek@reactos.org)
 */

/* INCLUDES ***************************************************************/

#define NTDDI_VERSION NTDDI_WS03
#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

POBJECT_TYPE ObDirectoryType = NULL;

/* PRIVATE FUNCTIONS ******************************************************/

/*++
* @name ObpInsertEntryDirectory
*
*     The ObpInsertEntryDirectory routine <FILLMEIN>.
*
* @param Parent
*        <FILLMEIN>.
*
* @param Context
*        <FILLMEIN>.
*
* @param ObjectHeader
*        <FILLMEIN>.
*
* @return TRUE if the object was inserted, FALSE otherwise.
*
* @remarks None.
*
*--*/
BOOLEAN
NTAPI
ObpInsertEntryDirectory(IN POBJECT_DIRECTORY Parent,
                        IN POBP_LOOKUP_CONTEXT Context,
                        IN POBJECT_HEADER ObjectHeader)
{
    POBJECT_DIRECTORY_ENTRY *AllocatedEntry;
    POBJECT_DIRECTORY_ENTRY NewEntry;
    POBJECT_HEADER_NAME_INFO HeaderNameInfo;

    /* Make sure we have a name */
    ASSERT(ObjectHeader->NameInfoOffset != 0);

    /* Allocate a new Directory Entry */
    NewEntry = ExAllocatePoolWithTag(PagedPool,
                                     sizeof(OBJECT_DIRECTORY_ENTRY),
                                     OB_DIR_TAG);
    if (!NewEntry) return FALSE;

    /* Save the hash */
    NewEntry->HashValue = Context->HashValue;

    /* Get the Object Name Information */
    HeaderNameInfo = HEADER_TO_OBJECT_NAME(ObjectHeader);

    /* Get the Allocated entry */
    AllocatedEntry = &Parent->HashBuckets[Context->HashIndex];

    /* Set it */
    NewEntry->ChainLink = *AllocatedEntry;
    *AllocatedEntry = NewEntry;

    /* Associate the Object */
    NewEntry->Object = &ObjectHeader->Body;

    /* Associate the Directory */
    HeaderNameInfo->Directory = Parent;
    return TRUE;
}

/*++
* @name ObpLookupEntryDirectory
*
*     The ObpLookupEntryDirectory routine <FILLMEIN>.
*
* @param Directory
*        <FILLMEIN>.
*
* @param Name
*        <FILLMEIN>.
*
* @param Attributes
*        <FILLMEIN>.
*
* @param SearchShadow
*        <FILLMEIN>.
*
* @param Context
*        <FILLMEIN>.
*
* @return Pointer to the object which was found, or NULL otherwise.
*
* @remarks None.
*
*--*/
PVOID
NTAPI
ObpLookupEntryDirectory(IN POBJECT_DIRECTORY Directory,
                        IN PUNICODE_STRING Name,
                        IN ULONG Attributes,
                        IN UCHAR SearchShadow,
                        IN POBP_LOOKUP_CONTEXT Context)
{
    BOOLEAN CaseInsensitive = FALSE;
    POBJECT_HEADER_NAME_INFO HeaderNameInfo;
    ULONG HashValue;
    ULONG HashIndex;
    LONG TotalChars;
    WCHAR CurrentChar;
    POBJECT_DIRECTORY_ENTRY *AllocatedEntry;
    POBJECT_DIRECTORY_ENTRY *LookupBucket;
    POBJECT_DIRECTORY_ENTRY CurrentEntry;
    PVOID FoundObject = NULL;
    PWSTR Buffer;
    PAGED_CODE();

    /* Always disable this until we have DOS Device Maps */
    SearchShadow = FALSE;

    /* Fail if we don't have a directory or name */
    if (!(Directory) || !(Name)) goto Quickie;

    /* Get name information */
    TotalChars = Name->Length / sizeof(WCHAR);
    Buffer = Name->Buffer;

    /* Fail if the name is empty */
    if (!(Buffer) || !(TotalChars)) goto Quickie;

    /* Set up case-sensitivity */
    if (Attributes & OBJ_CASE_INSENSITIVE) CaseInsensitive = TRUE;

    /* Create the Hash */
    for (HashValue = 0; TotalChars; TotalChars--)
    {
        /* Go to the next Character */
        CurrentChar = *Buffer++;

        /* Prepare the Hash */
        HashValue += (HashValue << 1) + (HashValue >> 1);

        /* Create the rest based on the name */
        if (CurrentChar < 'a') HashValue += CurrentChar;
        else if (CurrentChar > 'z') HashValue += RtlUpcaseUnicodeChar(CurrentChar);
        else HashValue += (CurrentChar - ('a'-'A'));
    }

    /* Merge it with our number of hash buckets */
    HashIndex = HashValue % 37;

    /* Save the result */
    Context->HashValue = HashValue;
    Context->HashIndex = HashIndex;

    /* Get the root entry and set it as our lookup bucket */
    AllocatedEntry = &Directory->HashBuckets[HashIndex];
    LookupBucket = AllocatedEntry;

    /* Check if the directory is already locked */
    if (!Context->DirectoryLocked)
    {
        /* Lock it */
        KeEnterCriticalRegion();
        ExAcquireResourceSharedLite(&Directory->Lock, TRUE);
    }

    /* Start looping */
    while ((CurrentEntry = *AllocatedEntry))
    {
        /* Do the hashes match? */
        if (CurrentEntry->HashValue == HashValue)
        {
            /* Make sure that it has a name */
            ASSERT(BODY_TO_HEADER(CurrentEntry->Object)->NameInfoOffset != 0);

            /* Get the name information */
            HeaderNameInfo = HEADER_TO_OBJECT_NAME(BODY_TO_HEADER(CurrentEntry->Object));

            /* Do the names match? */
            if ((Name->Length == HeaderNameInfo->Name.Length) &&
                (RtlEqualUnicodeString(Name, &HeaderNameInfo->Name, CaseInsensitive)))
            {
                break;
            }
        }

        /* Move to the next entry */
        AllocatedEntry = &CurrentEntry->ChainLink;
    }

    /* Check if we still have an entry */
    if (CurrentEntry)
    {
        /* Set this entry as the first, to speed up incoming insertion */
        if (AllocatedEntry != LookupBucket)
        {
            /* Set the Current Entry */
            *AllocatedEntry = CurrentEntry->ChainLink;

            /* Link to the old Hash Entry */
            CurrentEntry->ChainLink = *LookupBucket;

            /* Set the new Hash Entry */
            *LookupBucket = CurrentEntry;
        }

        /* Save the found object */
        FoundObject = CurrentEntry->Object;
        if (!FoundObject) goto Quickie;

        /* Add a reference to the object */
        ObReferenceObject(FoundObject);
    }

    /* Check if the directory was unlocked (which means we locked it) */
    if (!Context->DirectoryLocked)
    {
        /* Lock it */
        ExReleaseResourceLite(&Directory->Lock);
        KeLeaveCriticalRegion();
    }

Quickie:
    /* Return the object we found */
    Context->Object = FoundObject;
    return FoundObject;
}

/*++
* @name ObpDeleteEntryDirectory
*
*     The ObpDeleteEntryDirectory routine <FILLMEIN>.
*
* @param Context
*        <FILLMEIN>.
*
* @return TRUE if the object was deleted, FALSE otherwise.
*
* @remarks None.
*
*--*/
BOOLEAN
NTAPI
ObpDeleteEntryDirectory(POBP_LOOKUP_CONTEXT Context)
{
    POBJECT_DIRECTORY Directory;
    POBJECT_DIRECTORY_ENTRY *AllocatedEntry;
    POBJECT_DIRECTORY_ENTRY CurrentEntry;

    /* Get the Directory */
    Directory = Context->Directory;
    if (!Directory) return FALSE;

    /* Get the Entry */
    AllocatedEntry = &Directory->HashBuckets[Context->HashIndex];
    CurrentEntry = *AllocatedEntry;

    /* Unlink the Entry */
    *AllocatedEntry = CurrentEntry->ChainLink;
    CurrentEntry->ChainLink = NULL;

    /* Free it */
    ExFreePool(CurrentEntry);

    /* Return */
    return TRUE;
}

/* FUNCTIONS **************************************************************/

/*++
* @name NtOpenDirectoryObject
* @implemented NT4
*
*     The NtOpenDirectoryObject routine opens a namespace directory object.
*
* @param DirectoryHandle
*        Variable which receives the directory handle.
*
* @param DesiredAccess
*        Desired access to the directory.
*
* @param ObjectAttributes
*        Structure describing the directory.
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtOpenDirectoryObject (OUT PHANDLE DirectoryHandle,
                       IN ACCESS_MASK DesiredAccess,
                       IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    HANDLE hDirectory;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check if we need to do any probing */
    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            /* Probe the return handle */
            ProbeForWriteHandle(DirectoryHandle);
        }
        _SEH_HANDLE
        {
            /* Get the error code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* If we failed, return the error */
        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Open the directory object */
    Status = ObOpenObjectByName(ObjectAttributes,
                                ObDirectoryType,
                                NULL,
                                PreviousMode,
                                DesiredAccess,
                                NULL,
                                &hDirectory);
    if(NT_SUCCESS(Status))
    {
        _SEH_TRY
        {
            /* Write back the handle to the caller */
            *DirectoryHandle = hDirectory;
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }

    /* Return the status to the caller */
    return Status;
}

/*++
* @name NtQueryDirectoryObject
* @implemented NT4
*
*     The NtQueryDirectoryObject routine reads information from a directory in
*     the system namespace.
*
* @param DirectoryHandle
*        Handle obtained with NtOpenDirectoryObject which
*        must grant DIRECTORY_QUERY access to the directory object.
*
* @param Buffer
*        Buffer to hold the data read.
*
* @param BufferLength
*        Size of the buffer in bytes.
*
* @param ReturnSingleEntry
*        When TRUE, only 1 entry is written in DirObjInformation;
*        otherwise as many as will fit in the buffer.
*
* @param RestartScan
*        If TRUE start reading at index 0.
*        If FALSE start reading at the index specified by *ObjectIndex.
*
* @param Context
*        Zero based index into the directory, interpretation
*        depends on RestartScan.
*
* @param ReturnLength
*        Caller supplied storage for the number of bytes
*        written (or NULL).
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks Although you can iterate over the directory by calling this
*          function multiple times, the directory is unlocked between
*          calls. This means that another thread can change the directory
*          and so iterating doesn't guarantee a consistent picture of the
*          directory. Best thing is to retrieve all directory entries in
*          one call.
*
*--*/
NTSTATUS
NTAPI
NtQueryDirectoryObject(IN HANDLE DirectoryHandle,
                       OUT PVOID Buffer,
                       IN ULONG BufferLength,
                       IN BOOLEAN ReturnSingleEntry,
                       IN BOOLEAN RestartScan,
                       IN OUT PULONG Context,
                       OUT PULONG ReturnLength OPTIONAL)
{
    POBJECT_DIRECTORY Directory;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    ULONG SkipEntries = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check if we need to do any probing */
    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            /* Probe the buffer (assuming it will hold Unicode characters) */
            ProbeForWrite(Buffer, BufferLength, sizeof(WCHAR));

            /* Probe the context and copy it unless scan-restart was requested */
            ProbeForWriteUlong(Context);
            if (!RestartScan) SkipEntries = *Context;

            /* Probe the return length if the caller specified one */
            if(ReturnLength) ProbeForWriteUlong(ReturnLength);
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Return the exception to caller if we failed */
        if(!NT_SUCCESS(Status)) return Status;
    }
    else if (!RestartScan)
    {
        /* This is kernel mode, save the context without probing, if needed */
        SkipEntries = *Context;
    }

    /* Get a reference to directory */
    Status = ObReferenceObjectByHandle(DirectoryHandle,
                                       DIRECTORY_QUERY,
                                       ObDirectoryType,
                                       PreviousMode,
                                       (PVOID*)&Directory,
                                       NULL);
    if(NT_SUCCESS(Status))
    {
        /* FIXME: TODO. UNIMPLEMENTED */
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Return status to caller */
    return Status;
}

/*++
* @name NtCreateDirectoryObject
* @implemented NT4
*
*     The NtOpenDirectoryObject routine creates or opens a directory object.
*
* @param DirectoryHandle
*        Variable which receives the directory handle.
*
* @param DesiredAccess
*        Desired access to the directory.
*
* @param ObjectAttributes
*        Structure describing the directory.
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtCreateDirectoryObject(OUT PHANDLE DirectoryHandle,
                        IN ACCESS_MASK DesiredAccess,
                        IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    POBJECT_DIRECTORY Directory;
    HANDLE hDirectory;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    DPRINT("NtCreateDirectoryObject(DirectoryHandle %x, "
            "DesiredAccess %x, ObjectAttributes %x\n",
            DirectoryHandle, DesiredAccess, ObjectAttributes);

    /* Check if we need to do any probing */
    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            /* Probe the return handle */
            ProbeForWriteHandle(DirectoryHandle);
        }
        _SEH_HANDLE
        {
            /* Get the error code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* If we failed, return the error */
        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Create the object */
    Status = ObCreateObject(PreviousMode,
                            ObDirectoryType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(OBJECT_DIRECTORY),
                            0,
                            0,
                            (PVOID*)&Directory);
    if(NT_SUCCESS(Status))
    {
        /* Insert it into the handle table */
        Status = ObInsertObject((PVOID)Directory,
                                NULL,
                                DesiredAccess,
                                0,
                                NULL,
                                &hDirectory);
        if(NT_SUCCESS(Status))
        {
            _SEH_TRY
            {
                /* Return the handle back to the caller */
                *DirectoryHandle = hDirectory;
            }
            _SEH_HANDLE
            {
                /* Get the exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
        }

        /* ReactOS HACK: ObInsertObject double-references */
        ObDereferenceObject(Directory);
    }

    /* Return status to caller */
    return Status;
}

/* EOF */
