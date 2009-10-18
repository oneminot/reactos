/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/mem/virtual.c
 * PURPOSE:         Handles virtual memory APIs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
LPVOID
NTAPI
VirtualAllocEx(IN HANDLE hProcess,
               IN LPVOID lpAddress,
               IN SIZE_T dwSize,
               IN DWORD flAllocationType,
               IN DWORD flProtect)
{
    NTSTATUS Status;

    /* Allocate the memory */
    Status = NtAllocateVirtualMemory(hProcess,
                                     (PVOID *)&lpAddress,
                                     0,
                                     &dwSize,
                                     flAllocationType,
                                     flProtect);
    if (!NT_SUCCESS(Status))
    {
        /* We failed */
        SetLastErrorByStatus(Status);
        return NULL;
    }

    /* Return the allocated address */
    return lpAddress;
}

/*
 * @implemented
 */
LPVOID
NTAPI
VirtualAlloc(IN LPVOID lpAddress,
             IN SIZE_T dwSize,
             IN DWORD flAllocationType,
             IN DWORD flProtect)
{
    /* Call the extended API */
    return VirtualAllocEx(GetCurrentProcess(),
                          lpAddress,
                          dwSize,
                          flAllocationType,
                          flProtect);
}

/*
 * @implemented
 */
BOOL
NTAPI
VirtualFreeEx(IN HANDLE hProcess,
              IN LPVOID lpAddress,
              IN SIZE_T dwSize,
              IN DWORD dwFreeType)
{
    NTSTATUS Status;

    /* Free the memory */
    Status = NtFreeVirtualMemory(hProcess,
                                 (PVOID *)&lpAddress,
                                 (PULONG)&dwSize,
                                 dwFreeType);
    if (!NT_SUCCESS(Status))
    {
        /* We failed */
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    /* Return success */
    return TRUE;
}

/*
 * @implemented
 */
BOOL
NTAPI
VirtualFree(IN LPVOID lpAddress,
            IN SIZE_T dwSize,
            IN DWORD dwFreeType)
{
    /* Call the extended API */
    return VirtualFreeEx(GetCurrentProcess(),
                         lpAddress,
                         dwSize,
                         dwFreeType);
}

/*
 * @implemented
 */
BOOL
NTAPI
VirtualProtect(IN LPVOID lpAddress,
               IN SIZE_T dwSize,
               IN DWORD flNewProtect,
               OUT PDWORD lpflOldProtect)
{
    /* Call the extended API */
    return VirtualProtectEx(GetCurrentProcess(),
                            lpAddress,
                            dwSize,
                            flNewProtect,
                            lpflOldProtect);
}

/*
 * @implemented
 */
BOOL
NTAPI
VirtualProtectEx(IN HANDLE hProcess,
                 IN LPVOID lpAddress,
                 IN SIZE_T dwSize,
                 IN DWORD flNewProtect,
                 OUT PDWORD lpflOldProtect)
{
    NTSTATUS Status;

    /* Change the protection */
    Status = NtProtectVirtualMemory(hProcess,
                                    &lpAddress,
                                    &dwSize,
                                    flNewProtect,
                                    (PULONG)lpflOldProtect);
    if (!NT_SUCCESS(Status))
    {
        /* We failed */
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    /* Return success */
    return TRUE;
}

/*
 * @implemented
 */
BOOL
NTAPI
VirtualLock(IN LPVOID lpAddress,
            IN SIZE_T dwSize)
{
    ULONG BytesLocked;
    NTSTATUS Status;

    /* Lock the memory */
    Status = NtLockVirtualMemory(NtCurrentProcess(),
                                 lpAddress,
                                 dwSize,
                                 &BytesLocked);
    if (!NT_SUCCESS(Status))
    {
        /* We failed */
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    /* Return success */
    return TRUE;
}

/*
 * @implemented
 */
DWORD
NTAPI
VirtualQuery(IN LPCVOID lpAddress,
             OUT PMEMORY_BASIC_INFORMATION lpBuffer,
             IN SIZE_T dwLength)
{
    /* Call the extended API */
    return VirtualQueryEx(NtCurrentProcess(),
                          lpAddress,
                          lpBuffer,
                          dwLength);
}

/*
 * @implemented
 */
DWORD
NTAPI
VirtualQueryEx(IN HANDLE hProcess,
               IN LPCVOID lpAddress,
               OUT PMEMORY_BASIC_INFORMATION lpBuffer,
               IN SIZE_T dwLength)
{
    NTSTATUS Status;
    ULONG ResultLength;

    /* Query basic information */
    Status = NtQueryVirtualMemory(hProcess,
                                  (LPVOID)lpAddress,
                                  MemoryBasicInformation,
                                  lpBuffer,
                                  sizeof(MEMORY_BASIC_INFORMATION),
                                  &ResultLength);
    if (!NT_SUCCESS(Status))
    {
        /* We failed */
        SetLastErrorByStatus(Status);
        return 0;
    }

    /* Return the length returned */
    return ResultLength;
}

/*
 * @implemented
 */
BOOL
NTAPI
VirtualUnlock(IN LPVOID lpAddress,
              IN SIZE_T dwSize)
{
    ULONG BytesLocked;
    NTSTATUS Status;

    /* Unlock the memory */
    Status = NtUnlockVirtualMemory(NtCurrentProcess(),
                                   lpAddress,
                                   dwSize,
                                   &BytesLocked);
    if (!NT_SUCCESS(Status))
    {
        /* We failed */
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    /* Return success */
    return TRUE;
}

/* EOF */
