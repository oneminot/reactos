/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/synch/wait.c
 * PURPOSE:         Wrappers for the NT Wait Implementation
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x600
#include <k32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
DWORD
WINAPI
WaitForSingleObject(IN HANDLE hHandle,
                    IN DWORD dwMilliseconds)
{
    /* Call the extended API */
    return WaitForSingleObjectEx(hHandle, dwMilliseconds, FALSE);
}

/*
 * @implemented
 */
DWORD
WINAPI
WaitForSingleObjectEx(IN HANDLE hHandle,
                      IN DWORD dwMilliseconds,
                      IN BOOL bAlertable)
{
    PLARGE_INTEGER TimePtr;
    LARGE_INTEGER Time;
    NTSTATUS Status;

    /* Get real handle */
    hHandle = TranslateStdHandle(hHandle);

    /* Check for console handle */
    if ((IsConsoleHandle(hHandle)) && (VerifyConsoleIoHandle(hHandle)))
    {
        /* Get the real wait handle */
        hHandle = GetConsoleInputWaitHandle();
    }

    /* Check if this is an infinite wait */
    if (dwMilliseconds == INFINITE)
    {
        /* Under NT, this means no timer argument */
        TimePtr = NULL;
    }
    else
    {
        /* Otherwise, convert the time to NT Format */
        Time.QuadPart = UInt32x32To64(-10000, dwMilliseconds);
        TimePtr = &Time;
    }

    /* Start wait loop */
    do
    {
        /* Do the wait */
        Status = NtWaitForSingleObject(hHandle, (BOOLEAN)bAlertable, TimePtr);
        if (!NT_SUCCESS(Status))
        {
            /* The wait failed */
            SetLastErrorByStatus (Status);
            return WAIT_FAILED;
        }
    } while ((Status == STATUS_ALERTED) && (bAlertable));

    /* Return wait status */
    return Status;
}

/*
 * @implemented
 */
DWORD
WINAPI
WaitForMultipleObjects(IN DWORD nCount,
                       IN CONST HANDLE *lpHandles,
                       IN BOOL bWaitAll,
                       IN DWORD dwMilliseconds)
{
    /* Call the extended API */
    return WaitForMultipleObjectsEx(nCount,
                                    lpHandles,
                                    bWaitAll,
                                    dwMilliseconds,
                                    FALSE);
}

/*
 * @implemented
 */
DWORD
WINAPI
WaitForMultipleObjectsEx(IN DWORD nCount,
                         IN CONST HANDLE *lpHandles,
                         IN BOOL bWaitAll,
                         IN DWORD dwMilliseconds,
                         IN BOOL bAlertable)
{
    PLARGE_INTEGER TimePtr;
    LARGE_INTEGER Time;
    PHANDLE HandleBuffer;
    HANDLE Handle[8];
    DWORD i;
    NTSTATUS Status;

    /* Check if we have more handles then we locally optimize */
    if (nCount > 8)
    {
        /* Allocate a buffer for them */
        HandleBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                       0,
                                       nCount * sizeof(HANDLE));
        if (!HandleBuffer)
        {
            /* No buffer, fail the wait */
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return WAIT_FAILED;
        }
    }
    else
    {
        /* Otherwise, use our local buffer */
        HandleBuffer = Handle;
    }

    /* Copy the handles into our buffer and loop them all */
    RtlCopyMemory(HandleBuffer, (LPVOID)lpHandles, nCount * sizeof(HANDLE));
    for (i = 0; i < nCount; i++)
    {
        /* Check what kind of handle this is */
        HandleBuffer[i] = TranslateStdHandle(HandleBuffer[i]);

        /* Check for console handle */
        if ((IsConsoleHandle(HandleBuffer[i])) &&
            (VerifyConsoleIoHandle(HandleBuffer[i])))
        {
            /* Get the real wait handle */
            HandleBuffer[i] = GetConsoleInputWaitHandle();
        }
    }

    /* Check if this is an infinite wait */
    if (dwMilliseconds == INFINITE)
    {
        /* Under NT, this means no timer argument */
        TimePtr = NULL;
    }
    else
    {
        /* Otherwise, convert the time to NT Format */
        Time.QuadPart = UInt32x32To64(-10000, dwMilliseconds);
        TimePtr = &Time;
    }

    /* Start wait loop */
    do
    {
        /* Do the wait */
        Status = NtWaitForMultipleObjects(nCount,
                                          HandleBuffer,
                                          bWaitAll ? WaitAll : WaitAny,
                                          (BOOLEAN)bAlertable,
                                          TimePtr);
        if (!NT_SUCCESS(Status))
        {
            /* Wait failed */
            SetLastErrorByStatus (Status);
            return WAIT_FAILED;
        }
    } while ((Status == STATUS_ALERTED) && (bAlertable));

    /* Check if we didn't use our local buffer */
    if (HandleBuffer != Handle)
    {
        /* Free the allocated one */
        RtlFreeHeap(RtlGetProcessHeap(), 0, HandleBuffer);
    }

    /* Return wait status */
    return Status;
}

/*
 * @implemented
 */
DWORD
WINAPI
SignalObjectAndWait(IN HANDLE hObjectToSignal,
                    IN HANDLE hObjectToWaitOn,
                    IN DWORD dwMilliseconds,
                    IN BOOL bAlertable)
{
    PLARGE_INTEGER TimePtr;
    LARGE_INTEGER Time;
    NTSTATUS Status;

    /* Get real handle */
    hObjectToWaitOn = TranslateStdHandle(hObjectToWaitOn);

    /* Check for console handle */
    if ((IsConsoleHandle(hObjectToWaitOn)) &&
        (VerifyConsoleIoHandle(hObjectToWaitOn)))
    {
        /* Get the real wait handle */
        hObjectToWaitOn = GetConsoleInputWaitHandle();
    }

    /* Check if this is an infinite wait */
    if (dwMilliseconds == INFINITE)
    {
        /* Under NT, this means no timer argument */
        TimePtr = NULL;
    }
    else
    {
        /* Otherwise, convert the time to NT Format */
        Time.QuadPart = UInt32x32To64(-10000, dwMilliseconds);
        TimePtr = &Time;
    }

    /* Start wait loop */
    do
    {
        /* Do the wait */
        Status = NtSignalAndWaitForSingleObject(hObjectToSignal,
                                                hObjectToWaitOn,
                                                (BOOLEAN)bAlertable,
                                                TimePtr);
        if (!NT_SUCCESS(Status))
        {
            /* The wait failed */
            SetLastErrorByStatus (Status);
            return WAIT_FAILED;
        }
    } while ((Status == STATUS_ALERTED) && (bAlertable));

    /* Return wait status */
    return Status;
}

/*
 * @implemented
 */
HANDLE
WINAPI
CreateWaitableTimerW(IN LPSECURITY_ATTRIBUTES lpTimerAttributes OPTIONAL,
                     IN BOOL bManualReset,
                     IN LPCWSTR lpTimerName OPTIONAL)
{
    CreateNtObjectFromWin32Api(WaitableTimer, Timer, TIMER,
                               lpTimerAttributes,
                               lpTimerName,
                               bManualReset ? NotificationTimer : SynchronizationTimer);
}

/*
 * @implemented
 */
HANDLE
WINAPI
CreateWaitableTimerA(IN LPSECURITY_ATTRIBUTES lpTimerAttributes OPTIONAL,
                     IN BOOL bManualReset,
                     IN LPCSTR lpTimerName OPTIONAL)
{
    ConvertWin32AnsiObjectApiToUnicodeApi(WaitableTimer, lpTimerName, lpTimerAttributes, bManualReset);
}

/*
 * @implemented
 */
HANDLE
WINAPI
OpenWaitableTimerW(IN DWORD dwDesiredAccess,
                   IN BOOL bInheritHandle,
                   IN LPCWSTR lpTimerName)
{
    OpenNtObjectFromWin32Api(Timer, dwDesiredAccess, bInheritHandle, lpTimerName);
}

/*
 * @implemented
 */
HANDLE
WINAPI
OpenWaitableTimerA(IN DWORD dwDesiredAccess,
                   IN BOOL bInheritHandle,
                   IN LPCSTR lpTimerName)
{
    ConvertOpenWin32AnsiObjectApiToUnicodeApi(WaitableTimer, dwDesiredAccess, bInheritHandle, lpTimerName);
}

/*
 * @implemented
 */
BOOL
WINAPI
SetWaitableTimer(IN HANDLE hTimer,
                 IN const LARGE_INTEGER *pDueTime,
                 IN LONG lPeriod,
                 IN PTIMERAPCROUTINE pfnCompletionRoutine OPTIONAL,
                 IN OPTIONAL LPVOID lpArgToCompletionRoutine,
                 IN BOOL fResume)
{
    NTSTATUS Status;

    /* Set the timer */
    Status = NtSetTimer(hTimer,
                        (PLARGE_INTEGER)pDueTime,
                        (PTIMER_APC_ROUTINE)pfnCompletionRoutine,
                        lpArgToCompletionRoutine,
                        (BOOLEAN)fResume,
                        lPeriod,
                        NULL);
    if (NT_SUCCESS(Status)) return TRUE;

    /* If we got here, then we failed */
    SetLastErrorByStatus(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
CancelWaitableTimer(IN HANDLE hTimer)
{
    NTSTATUS Status;

    /* Cancel the timer */
    Status = NtCancelTimer(hTimer, NULL);
    if (NT_SUCCESS(Status)) return TRUE;

    /* If we got here, then we failed */
    SetLastErrorByStatus(Status);
    return FALSE;
}

/*
 * @implemented
 */
HANDLE
WINAPI
CreateSemaphoreA(IN LPSECURITY_ATTRIBUTES lpSemaphoreAttributes  OPTIONAL,
                 IN LONG lInitialCount,
                 IN LONG lMaximumCount,
                 IN LPCSTR lpName  OPTIONAL)
{
    ConvertWin32AnsiObjectApiToUnicodeApi(Semaphore, lpName, lpSemaphoreAttributes, lInitialCount, lMaximumCount);
}

/*
 * @implemented
 */
HANDLE
WINAPI
CreateSemaphoreW(IN LPSECURITY_ATTRIBUTES lpSemaphoreAttributes  OPTIONAL,
                 IN LONG lInitialCount,
                 IN LONG lMaximumCount,
                 IN LPCWSTR lpName  OPTIONAL)
{
    CreateNtObjectFromWin32Api(Semaphore, Semaphore, SEMAPHORE,
                               lpSemaphoreAttributes,
                               lpName,
                               lInitialCount,
                               lMaximumCount);
}

/*
 * @implemented
 */
HANDLE
WINAPI
OpenSemaphoreA(IN DWORD dwDesiredAccess,
               IN BOOL bInheritHandle,
               IN LPCSTR lpName)
{
    ConvertOpenWin32AnsiObjectApiToUnicodeApi(Semaphore, dwDesiredAccess, bInheritHandle, lpName);
}

/*
 * @implemented
 */
HANDLE
WINAPI
OpenSemaphoreW(IN DWORD dwDesiredAccess,
               IN BOOL bInheritHandle,
               IN LPCWSTR lpName)
{
    OpenNtObjectFromWin32Api(Semaphore, dwDesiredAccess, bInheritHandle, lpName);
}

/*
 * @implemented
 */
BOOL
WINAPI
ReleaseSemaphore(IN HANDLE hSemaphore,
                 IN LONG lReleaseCount,
                 IN LPLONG lpPreviousCount)
{
    NTSTATUS Status;

    /* Release the semaphore */
    Status = NtReleaseSemaphore(hSemaphore, lReleaseCount, lpPreviousCount);
    if (NT_SUCCESS(Status)) return TRUE;

    /* If we got here, then we failed */
    SetLastErrorByStatus(Status);
    return FALSE;
}

/*
 * @implemented
 */
HANDLE
WINAPI
CreateMutexA(IN LPSECURITY_ATTRIBUTES lpMutexAttributes  OPTIONAL,
             IN BOOL bInitialOwner,
             IN LPCSTR lpName  OPTIONAL)
{
    ConvertWin32AnsiObjectApiToUnicodeApi(Mutex, lpName, lpMutexAttributes, bInitialOwner);
}

/*
 * @implemented
 */
HANDLE
WINAPI
CreateMutexW(IN LPSECURITY_ATTRIBUTES lpMutexAttributes  OPTIONAL,
             IN BOOL bInitialOwner,
             IN LPCWSTR lpName  OPTIONAL)
{
    CreateNtObjectFromWin32Api(Mutex, Mutant, MUTEX,
                               lpMutexAttributes,
                               lpName,
                               bInitialOwner);
}

/*
 * @implemented
 */
HANDLE
WINAPI
OpenMutexA(IN DWORD dwDesiredAccess,
           IN BOOL bInheritHandle,
           IN LPCSTR lpName)
{
    ConvertOpenWin32AnsiObjectApiToUnicodeApi(Mutex, dwDesiredAccess, bInheritHandle, lpName);
}

/*
 * @implemented
 */
HANDLE
WINAPI
OpenMutexW(IN DWORD dwDesiredAccess,
           IN BOOL bInheritHandle,
           IN LPCWSTR lpName)
{
    OpenNtObjectFromWin32Api(Mutant, dwDesiredAccess, bInheritHandle, lpName);
}

/*
 * @implemented
 */
BOOL
WINAPI
ReleaseMutex(IN HANDLE hMutex)
{
    NTSTATUS Status;

    /* Release the mutant */
    Status = NtReleaseMutant(hMutex, NULL);
    if (NT_SUCCESS(Status)) return TRUE;

    /* If we got here, then we failed */
    SetLastErrorByStatus(Status);
    return FALSE;
}

/*
 * @implemented
 */
HANDLE
WINAPI
CreateEventA(IN LPSECURITY_ATTRIBUTES lpEventAttributes  OPTIONAL,
             IN BOOL bManualReset,
             IN BOOL bInitialState,
             IN LPCSTR lpName OPTIONAL)
{
    ConvertWin32AnsiObjectApiToUnicodeApi(Event, lpName, lpEventAttributes, bManualReset, bInitialState);
}

/*
 * @implemented
 */
HANDLE
WINAPI
CreateEventW(IN LPSECURITY_ATTRIBUTES lpEventAttributes  OPTIONAL,
             IN BOOL bManualReset,
             IN BOOL bInitialState,
             IN LPCWSTR lpName  OPTIONAL)
{
    CreateNtObjectFromWin32Api(Event, Event, EVENT,
                               lpEventAttributes,
                               lpName,
                               bManualReset ? NotificationTimer : SynchronizationTimer,
                               bInitialState);
}

/*
 * @implemented
 */
HANDLE
WINAPI
OpenEventA(IN DWORD dwDesiredAccess,
           IN BOOL bInheritHandle,
           IN LPCSTR lpName)
{
    ConvertOpenWin32AnsiObjectApiToUnicodeApi(Event, dwDesiredAccess, bInheritHandle, lpName);
}

/*
 * @implemented
 */
HANDLE
WINAPI
OpenEventW(IN DWORD dwDesiredAccess,
           IN BOOL bInheritHandle,
           IN LPCWSTR lpName)
{
    OpenNtObjectFromWin32Api(Event, dwDesiredAccess, bInheritHandle, lpName);
}

/*
 * @implemented
 */
BOOL
WINAPI
PulseEvent(IN HANDLE hEvent)
{
    NTSTATUS Status;

    /* Pulse the event */
    Status = NtPulseEvent(hEvent, NULL);
    if (NT_SUCCESS(Status)) return TRUE;

    /* If we got here, then we failed */
    SetLastErrorByStatus(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
ResetEvent(IN HANDLE hEvent)
{
    NTSTATUS Status;

    /* Clear the event */
    Status = NtResetEvent(hEvent, NULL);
    if (NT_SUCCESS(Status)) return TRUE;

    /* If we got here, then we failed */
    SetLastErrorByStatus(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetEvent(IN HANDLE hEvent)
{
    NTSTATUS Status;

    /* Set the event */
    Status = NtSetEvent(hEvent, NULL);
    if (NT_SUCCESS(Status)) return TRUE;

    /* If we got here, then we failed */
    SetLastErrorByStatus(Status);
    return FALSE;
}

/*
 * @implemented
 */
VOID
WINAPI
InitializeCriticalSection(OUT LPCRITICAL_SECTION lpCriticalSection)
{
    NTSTATUS Status;

    /* Initialize the critical section and raise an exception if we failed */
    Status = RtlInitializeCriticalSection((PVOID)lpCriticalSection);
    if (!NT_SUCCESS(Status)) RtlRaiseStatus(Status);
}

/*
 * @implemented
 */
BOOL
WINAPI
InitializeCriticalSectionAndSpinCount(OUT LPCRITICAL_SECTION lpCriticalSection,
                                      IN DWORD dwSpinCount)
{
    NTSTATUS Status;

    /* Initialize the critical section */
    Status = RtlInitializeCriticalSectionAndSpinCount((PVOID)lpCriticalSection,
                                                      dwSpinCount);
    if (!NT_SUCCESS(Status))
    {
        /* Set failure code */
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    /* Success */
    return TRUE;
}


/*
 * @implemented
 */
VOID WINAPI
Sleep(DWORD dwMilliseconds)
{
  SleepEx(dwMilliseconds, FALSE);
  return;
}


/*
 * @implemented
 */
DWORD WINAPI
SleepEx(DWORD dwMilliseconds,
	BOOL bAlertable)
{
  LARGE_INTEGER Interval;
  NTSTATUS errCode;

  if (dwMilliseconds != INFINITE)
    {
      /*
       * System time units are 100 nanoseconds (a nanosecond is a billionth of
       * a second).
       */
      Interval.QuadPart = -((LONGLONG)dwMilliseconds * 10000);
    }
  else
    {
      /* Approximately 292000 years hence */
      Interval.QuadPart = -0x7FFFFFFFFFFFFFFFLL;
    }

dowait:
  errCode = NtDelayExecution ((BOOLEAN)bAlertable, &Interval);
  if ((bAlertable) && (errCode == STATUS_ALERTED)) goto dowait;
  return (errCode == STATUS_USER_APC) ? WAIT_IO_COMPLETION : 0;
}


/*
 * @implemented
 */
BOOL
WINAPI
RegisterWaitForSingleObject(
    PHANDLE phNewWaitObject,
    HANDLE hObject,
    WAITORTIMERCALLBACK Callback,
    PVOID Context,
    ULONG dwMilliseconds,
    ULONG dwFlags
    )
{
    NTSTATUS Status = RtlRegisterWait(phNewWaitObject,
                                      hObject,
                                      Callback,
                                      Context,
                                      dwMilliseconds,
                                      dwFlags);

    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }
    return TRUE;
}


/*
 * @implemented
 */
HANDLE
WINAPI
RegisterWaitForSingleObjectEx(
    HANDLE hObject,
    WAITORTIMERCALLBACK Callback,
    PVOID Context,
    ULONG dwMilliseconds,
    ULONG dwFlags
    )
{
    NTSTATUS Status;
    HANDLE hNewWaitObject;

    Status = RtlRegisterWait(&hNewWaitObject,
                             hObject,
                             Callback,
                             Context,
                             dwMilliseconds,
                             dwFlags);

    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return NULL;
    }

    return hNewWaitObject;
}


/*
 * @implemented
 */
BOOL
WINAPI
UnregisterWait(
    HANDLE WaitHandle
    )
{
    NTSTATUS Status = RtlDeregisterWaitEx(WaitHandle, NULL);

    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
UnregisterWaitEx(
    HANDLE WaitHandle,
    HANDLE CompletionEvent
    )
{
    NTSTATUS Status = RtlDeregisterWaitEx(WaitHandle, CompletionEvent);

    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    return TRUE;
}

/* EOF */
