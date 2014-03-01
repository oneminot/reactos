/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Base API Server DLL
 * FILE:            subsystems/win/basesrv/vdm.c
 * PURPOSE:         Virtual DOS Machines (VDM) Support
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *                  Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#include "basesrv.h"
#include "vdm.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

BOOLEAN FirstVDM = TRUE;
LIST_ENTRY VDMConsoleListHead;
RTL_CRITICAL_SECTION DosCriticalSection;
RTL_CRITICAL_SECTION WowCriticalSection;

/* FUNCTIONS ******************************************************************/

NTSTATUS NTAPI BaseSrvGetConsoleRecord(HANDLE ConsoleHandle, PVDM_CONSOLE_RECORD *Record)
{
    PLIST_ENTRY i;
    PVDM_CONSOLE_RECORD CurrentRecord = NULL;

    /* Search for a record that has the same console handle */
    for (i = VDMConsoleListHead.Flink; i != &VDMConsoleListHead; i = i->Flink)
    {
        CurrentRecord = CONTAINING_RECORD(i, VDM_CONSOLE_RECORD, Entry);
        if (CurrentRecord->ConsoleHandle == ConsoleHandle) break;
    }

    *Record = CurrentRecord;
    return CurrentRecord ? STATUS_SUCCESS : STATUS_NOT_FOUND;
}

ULONG NTAPI GetNextDosSesId(VOID)
{
    ULONG SessionId;
    PLIST_ENTRY i;
    PVDM_CONSOLE_RECORD CurrentRecord = NULL;
    BOOLEAN Found;

    /* Search for an available session ID */
    for (SessionId = 1; SessionId != 0; SessionId++)
    {
        Found = FALSE;

        /* Check if the ID is already in use */
        for (i = VDMConsoleListHead.Flink; i != &VDMConsoleListHead; i = i->Flink)
        {
            CurrentRecord = CONTAINING_RECORD(i, VDM_CONSOLE_RECORD, Entry);
            if (CurrentRecord->SessionId == SessionId) Found = TRUE;
        }

        /* If not, we found one */
        if (!Found) break;
    }

    ASSERT(SessionId != 0);

    /* Return the session ID */
    return SessionId;
}

BOOLEAN NTAPI BaseSrvIsVdmAllowed(VOID)
{
    NTSTATUS Status;
    BOOLEAN VdmAllowed = TRUE;
    HANDLE RootKey, KeyHandle;
    UNICODE_STRING KeyName, ValueName, MachineKeyName;
    OBJECT_ATTRIBUTES Attributes;
    UCHAR ValueBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
    ULONG ActualSize;

    /* Initialize the unicode strings */
    RtlInitUnicodeString(&MachineKeyName, L"\\Registry\\Machine");
    RtlInitUnicodeString(&KeyName, VDM_POLICY_KEY_NAME);
    RtlInitUnicodeString(&ValueName, VDM_DISALLOWED_VALUE_NAME);

    InitializeObjectAttributes(&Attributes,
                               &MachineKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Open the local machine key */
    Status = NtOpenKey(&RootKey, KEY_READ, &Attributes);
    if (!NT_SUCCESS(Status)) return FALSE;

    InitializeObjectAttributes(&Attributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               RootKey,
                               NULL);

    /* Open the policy key in the local machine hive, if it exists */
    if (NT_SUCCESS(NtOpenKey(&KeyHandle, KEY_READ, &Attributes)))
    {
        /* Read the value, if it's set */
        if (NT_SUCCESS(NtQueryValueKey(KeyHandle,
                                       &ValueName,
                                       KeyValuePartialInformation,
                                       ValueInfo,
                                       sizeof(ValueBuffer),
                                       &ActualSize)))
        {
            if (*((PULONG)ValueInfo->Data))
            {
                /* The VDM has been disabled in the registry */
                VdmAllowed = FALSE;
            }
        }

        NtClose(KeyHandle);
    }

    /* Close the local machine key */
    NtClose(RootKey);

    /* If it's disabled system-wide, there's no need to check the user key */
    if (!VdmAllowed) return FALSE;

    /* Open the current user key of the client */
    if (!CsrImpersonateClient(NULL)) return VdmAllowed;
    Status = RtlOpenCurrentUser(KEY_READ, &RootKey);
    CsrRevertToSelf();

    /* If that fails, return the system-wide setting */
    if (!NT_SUCCESS(Status)) return VdmAllowed;

    InitializeObjectAttributes(&Attributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               RootKey,
                               NULL);

    /* Open the policy key in the current user hive, if it exists */
    if (NT_SUCCESS(NtOpenKey(&KeyHandle, KEY_READ, &Attributes)))
    {
        /* Read the value, if it's set */
        if (NT_SUCCESS(NtQueryValueKey(KeyHandle,
                                       &ValueName,
                                       KeyValuePartialInformation,
                                       ValueInfo,
                                       sizeof(ValueBuffer),
                                       &ActualSize)))
        {
            if (*((PULONG)ValueInfo->Data))
            {
                /* The VDM has been disabled in the registry */
                VdmAllowed = FALSE;
            }
        }

        NtClose(KeyHandle);
    }

    return VdmAllowed;
}

NTSTATUS NTAPI BaseSrvCreatePairWaitHandles(PHANDLE ServerEvent, PHANDLE ClientEvent)
{
    NTSTATUS Status;

    /* Create the event */
    Status = NtCreateEvent(ServerEvent, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);
    if (!NT_SUCCESS(Status)) return Status;

    /* Duplicate the event into the client process */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               *ServerEvent,
                               CsrGetClientThread()->Process->ProcessHandle,
                               ClientEvent,
                               0,
                               0,
                               DUPLICATE_SAME_ATTRIBUTES | DUPLICATE_SAME_ACCESS);

    if (!NT_SUCCESS(Status)) NtClose(*ServerEvent);
    return Status;
}

VOID NTAPI BaseInitializeVDM(VOID)
{
    /* Initialize the list head */
    InitializeListHead(&VDMConsoleListHead);

    /* Initialize the critical section */
    RtlInitializeCriticalSection(&DosCriticalSection);
    RtlInitializeCriticalSection(&WowCriticalSection);
}

/* PUBLIC SERVER APIS *********************************************************/

CSR_API(BaseSrvCheckVDM)
{
    NTSTATUS Status;
    PBASE_CHECK_VDM CheckVdmRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.CheckVDMRequest;
    PRTL_CRITICAL_SECTION CriticalSection = NULL;
    PVDM_CONSOLE_RECORD ConsoleRecord = NULL;
    PVDM_DOS_RECORD DosRecord = NULL;
    BOOLEAN NewConsoleRecord = FALSE;

    /* Don't do anything if the VDM has been disabled in the registry */
    if (!BaseSrvIsVdmAllowed()) return STATUS_ACCESS_DENIED;

    /* Validate the message buffers */
    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&CheckVdmRequest->CmdLine,
                                  CheckVdmRequest->CmdLen,
                                  sizeof(*CheckVdmRequest->CmdLine))
        || !CsrValidateMessageBuffer(ApiMessage,
                                     (PVOID*)&CheckVdmRequest->AppName,
                                     CheckVdmRequest->AppLen,
                                     sizeof(*CheckVdmRequest->AppName))
        || !CsrValidateMessageBuffer(ApiMessage,
                                     (PVOID*)&CheckVdmRequest->PifFile,
                                     CheckVdmRequest->PifLen,
                                     sizeof(*CheckVdmRequest->PifFile))
        || !CsrValidateMessageBuffer(ApiMessage,
                                     (PVOID*)&CheckVdmRequest->CurDirectory,
                                     CheckVdmRequest->CurDirectoryLen,
                                     sizeof(*CheckVdmRequest->CurDirectory))
        || !CsrValidateMessageBuffer(ApiMessage,
                                     (PVOID*)&CheckVdmRequest->Desktop,
                                     CheckVdmRequest->DesktopLen,
                                     sizeof(*CheckVdmRequest->Desktop))
        || !CsrValidateMessageBuffer(ApiMessage,
                                     (PVOID*)&CheckVdmRequest->Title,
                                     CheckVdmRequest->TitleLen,
                                     sizeof(*CheckVdmRequest->Title))
        || !CsrValidateMessageBuffer(ApiMessage,
                                     (PVOID*)&CheckVdmRequest->Reserved,
                                     CheckVdmRequest->ReservedLen,
                                     sizeof(*CheckVdmRequest->Reserved)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    CriticalSection = (CheckVdmRequest->BinaryType != BINARY_TYPE_SEPARATE_WOW)
                      ? &DosCriticalSection
                      : &WowCriticalSection;

    /* Enter the critical section */
    RtlEnterCriticalSection(CriticalSection);

    /* Check if this is a DOS or WOW VDM */
    if (CheckVdmRequest->BinaryType != BINARY_TYPE_SEPARATE_WOW)
    {
        /* Get the console record */
        Status = BaseSrvGetConsoleRecord(CheckVdmRequest->ConsoleHandle,
                                         &ConsoleRecord);

        if (!NT_SUCCESS(Status))
        {
            /* Allocate a new console record */
            ConsoleRecord = (PVDM_CONSOLE_RECORD)RtlAllocateHeap(BaseSrvHeap,
                                                                 HEAP_ZERO_MEMORY,
                                                                 sizeof(VDM_CONSOLE_RECORD));
            if (ConsoleRecord == NULL)
            {
                Status = STATUS_NO_MEMORY;
                goto Cleanup;
            }

            /* Initialize the console record */
            ConsoleRecord->ConsoleHandle = CheckVdmRequest->ConsoleHandle;
            ConsoleRecord->CurrentDirs = NULL;
            ConsoleRecord->CurDirsLength = 0;
            ConsoleRecord->SessionId = GetNextDosSesId();
            InitializeListHead(&ConsoleRecord->DosListHead);
            // TODO: The console record structure is incomplete

            /* Remember that the console record was allocated here */
            NewConsoleRecord = TRUE;
        }

        /* Allocate a new DOS record */
        DosRecord = (PVDM_DOS_RECORD)RtlAllocateHeap(BaseSrvHeap,
                                                     HEAP_ZERO_MEMORY,
                                                     sizeof(VDM_DOS_RECORD));
        if (DosRecord == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        /* Initialize the DOS record */
        DosRecord->State = NewConsoleRecord ? VDM_NOT_LOADED : VDM_READY;
        DosRecord->ExitCode = 0;
        // TODO: The DOS record structure is incomplete

        Status = BaseSrvCreatePairWaitHandles(&DosRecord->ServerEvent, &DosRecord->ClientEvent);
        if (!NT_SUCCESS(Status)) goto Cleanup;

        /* Add the DOS record */
        InsertHeadList(&ConsoleRecord->DosListHead, &DosRecord->Entry);

        if (NewConsoleRecord)
        {
            /* Add the console record */
            InsertTailList(&VDMConsoleListHead, &ConsoleRecord->Entry);
        }

        CheckVdmRequest->VDMState = DosRecord->State;
        Status = STATUS_SUCCESS;
    }
    else
    {
        // TODO: NOT IMPLEMENTED
        UNIMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    }

Cleanup:
    /* Check if it failed */
    if (!NT_SUCCESS(Status))
    {
        /* Free the DOS record */
        if (DosRecord != NULL)
        {
            RtlFreeHeap(BaseSrvHeap, 0, DosRecord);
            DosRecord = NULL;
        }

        /* Free the console record if it was allocated here */
        if (NewConsoleRecord)
        {
            RtlFreeHeap(BaseSrvHeap, 0, ConsoleRecord);
            ConsoleRecord = NULL;
        }
    }

    /* Leave the critical section */
    RtlLeaveCriticalSection(CriticalSection);

    return Status;
}

CSR_API(BaseSrvUpdateVDMEntry)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(BaseSrvGetNextVDMCommand)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(BaseSrvExitVDM)
{
    NTSTATUS Status;
    PBASE_EXIT_VDM ExitVdmRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.ExitVDMRequest;
    PRTL_CRITICAL_SECTION CriticalSection = NULL;
    PVDM_CONSOLE_RECORD ConsoleRecord = NULL;
    PVDM_DOS_RECORD DosRecord;

    CriticalSection = (ExitVdmRequest->iWowTask == 0)
                      ? &DosCriticalSection
                      : &WowCriticalSection;

    /* Enter the critical section */
    RtlEnterCriticalSection(CriticalSection);

    if (ExitVdmRequest->iWowTask == 0)
    {
        /* Get the console record */
        Status = BaseSrvGetConsoleRecord(ExitVdmRequest->ConsoleHandle, &ConsoleRecord);
        if (!NT_SUCCESS(Status)) goto Cleanup;

        /* Cleanup the DOS records */
        while (ConsoleRecord->DosListHead.Flink != &ConsoleRecord->DosListHead)
        {
            DosRecord = CONTAINING_RECORD(ConsoleRecord->DosListHead.Flink,
                                          VDM_DOS_RECORD,
                                          Entry);

            /* Remove the DOS entry */
            RemoveEntryList(&DosRecord->Entry);
            RtlFreeHeap(BaseSrvHeap, 0, DosRecord);
        }

        if (ConsoleRecord->CurrentDirs != NULL)
        {
            /* Free the current directories */
            RtlFreeHeap(BaseSrvHeap, 0, ConsoleRecord->CurrentDirs);
            ConsoleRecord->CurrentDirs = NULL;
            ConsoleRecord->CurDirsLength = 0;
        }

        /* Remove the console record */
        RemoveEntryList(&ConsoleRecord->Entry);
        RtlFreeHeap(BaseSrvHeap, 0, ConsoleRecord);
    }
    else
    {
        // TODO: NOT IMPLEMENTED
        UNIMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    }

Cleanup:
    /* Leave the critical section */
    RtlLeaveCriticalSection(CriticalSection);

    return Status;
}

CSR_API(BaseSrvIsFirstVDM)
{
    PBASE_IS_FIRST_VDM IsFirstVDMRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.IsFirstVDMRequest;

    /* Return the result */
    IsFirstVDMRequest->FirstVDM = FirstVDM;

    /* Clear the first VDM flag */
    FirstVDM = FALSE;

    return STATUS_SUCCESS;
}

CSR_API(BaseSrvGetVDMExitCode)
{
    NTSTATUS Status;
    PBASE_GET_VDM_EXIT_CODE GetVDMExitCodeRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.GetVDMExitCodeRequest;
    PLIST_ENTRY i = NULL;
    PVDM_CONSOLE_RECORD ConsoleRecord = NULL;
    PVDM_DOS_RECORD DosRecord = NULL;

    /* Enter the critical section */
    RtlEnterCriticalSection(&DosCriticalSection);

    /* Get the console record */
    Status = BaseSrvGetConsoleRecord(GetVDMExitCodeRequest->ConsoleHandle, &ConsoleRecord);
    if (!NT_SUCCESS(Status)) goto Cleanup;

    /* Search for a DOS record that has the same parent process handle */
    for (i = ConsoleRecord->DosListHead.Flink; i != &ConsoleRecord->DosListHead; i = i->Flink)
    {
        DosRecord = CONTAINING_RECORD(i, VDM_DOS_RECORD, Entry);
        if (DosRecord->ClientEvent == GetVDMExitCodeRequest->hParent) break;
    }

    /* Check if no DOS record was found */
    if (i == &ConsoleRecord->DosListHead)
    {
        Status = STATUS_NOT_FOUND;
        goto Cleanup;
    }

    /* Check if this task is still running */
    if (DosRecord->State == VDM_READY)
    {
        GetVDMExitCodeRequest->ExitCode = STATUS_PENDING;
        goto Cleanup;
    }

    /* Return the exit code */
    GetVDMExitCodeRequest->ExitCode = DosRecord->ExitCode;

    /* Since this is a zombie task record, remove it */
    RemoveEntryList(&DosRecord->Entry);
    RtlFreeHeap(BaseSrvHeap, 0, DosRecord);

Cleanup:
    /* Leave the critical section */
    RtlLeaveCriticalSection(&DosCriticalSection);

    return Status;
}

CSR_API(BaseSrvSetReenterCount)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(BaseSrvSetVDMCurDirs)
{
    NTSTATUS Status;
    PBASE_GETSET_VDM_CURDIRS VDMCurrentDirsRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.VDMCurrentDirsRequest;
    PVDM_CONSOLE_RECORD ConsoleRecord;
    PCHAR Buffer = NULL;

    /* Validate the input buffer */
    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&VDMCurrentDirsRequest->lpszzCurDirs,
                                  VDMCurrentDirsRequest->cchCurDirs,
                                  sizeof(*VDMCurrentDirsRequest->lpszzCurDirs)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Enter the critical section */
    RtlEnterCriticalSection(&DosCriticalSection);

    /* Find the console record */
    Status = BaseSrvGetConsoleRecord(VDMCurrentDirsRequest->ConsoleHandle, &ConsoleRecord);
    if (!NT_SUCCESS(Status)) goto Cleanup;

    if (ConsoleRecord->CurrentDirs == NULL)
    {
        /* Allocate memory for the current directory information */
        Buffer = RtlAllocateHeap(BaseSrvHeap,
                                 HEAP_ZERO_MEMORY,
                                 VDMCurrentDirsRequest->cchCurDirs);
    }
    else
    {
        /* Resize the amount of allocated memory */
        Buffer = RtlReAllocateHeap(BaseSrvHeap,
                                   HEAP_ZERO_MEMORY,
                                   ConsoleRecord->CurrentDirs,
                                   VDMCurrentDirsRequest->cchCurDirs);
    }

    if (Buffer == NULL)
    {
        /* Allocation failed */
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    /* Update the console record */
    ConsoleRecord->CurrentDirs = Buffer;
    ConsoleRecord->CurDirsLength = VDMCurrentDirsRequest->cchCurDirs;

    /* Copy the data */
    RtlMoveMemory(ConsoleRecord->CurrentDirs,
                  VDMCurrentDirsRequest->lpszzCurDirs,
                  VDMCurrentDirsRequest->cchCurDirs);

Cleanup:
    /* Leave the critical section */
    RtlLeaveCriticalSection(&DosCriticalSection);

    return Status;
}

CSR_API(BaseSrvGetVDMCurDirs)
{
    NTSTATUS Status;
    PBASE_GETSET_VDM_CURDIRS VDMCurrentDirsRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.VDMCurrentDirsRequest;
    PVDM_CONSOLE_RECORD ConsoleRecord;

    /* Validate the output buffer */
    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&VDMCurrentDirsRequest->lpszzCurDirs,
                                  VDMCurrentDirsRequest->cchCurDirs,
                                  sizeof(*VDMCurrentDirsRequest->lpszzCurDirs)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Enter the critical section */
    RtlEnterCriticalSection(&DosCriticalSection);

    /* Find the console record */
    Status = BaseSrvGetConsoleRecord(VDMCurrentDirsRequest->ConsoleHandle, &ConsoleRecord);
    if (!NT_SUCCESS(Status)) goto Cleanup;

    /* Return the actual size of the current directory information */
    VDMCurrentDirsRequest->cchCurDirs = ConsoleRecord->CurDirsLength;

    /* Check if the buffer is large enough */
    if (VDMCurrentDirsRequest->cchCurDirs < ConsoleRecord->CurDirsLength)
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto Cleanup;
    }

    /* Copy the data */
    RtlMoveMemory(VDMCurrentDirsRequest->lpszzCurDirs,
                  ConsoleRecord->CurrentDirs,
                  ConsoleRecord->CurDirsLength);

Cleanup:
    /* Leave the critical section */
    RtlLeaveCriticalSection(&DosCriticalSection);

    return Status;
}

CSR_API(BaseSrvBatNotification)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(BaseSrvRegisterWowExec)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(BaseSrvRefreshIniFileMapping)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
