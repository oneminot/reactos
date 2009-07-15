#include <user32.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(user32);

#define KEY_LENGTH 1024

static ULONG User32TlsIndex;
HINSTANCE User32Instance;

PPROCESSINFO g_ppi = NULL;
PUSER_HANDLE_TABLE gHandleTable = NULL;
PUSER_HANDLE_ENTRY gHandleEntries = NULL;
PSERVERINFO g_psi = NULL;
ULONG_PTR g_ulSharedDelta;

WCHAR szAppInit[KEY_LENGTH];

PUSER32_THREAD_DATA
User32GetThreadData()
{
   return ((PUSER32_THREAD_DATA)TlsGetValue(User32TlsIndex));
}


BOOL
GetDllList()
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Attributes;
    BOOL bRet = FALSE;
    BOOL bLoad;
    HANDLE hKey = NULL;
    DWORD dwSize;
    PKEY_VALUE_PARTIAL_INFORMATION kvpInfo = NULL;

    UNICODE_STRING szKeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows");
    UNICODE_STRING szLoadName = RTL_CONSTANT_STRING(L"LoadAppInit_DLLs");
    UNICODE_STRING szDllsName = RTL_CONSTANT_STRING(L"AppInit_DLLs");

    InitializeObjectAttributes(&Attributes, &szKeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenKey(&hKey, KEY_READ, &Attributes);
    if (NT_SUCCESS(Status))
    {
        dwSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD);
        kvpInfo = HeapAlloc(GetProcessHeap(), 0, dwSize);
        if (!kvpInfo)
            goto end;

        Status = NtQueryValueKey(hKey,
                                 &szLoadName,
                                 KeyValuePartialInformation,
                                 kvpInfo,
                                 dwSize,
                                 &dwSize);
        if (!NT_SUCCESS(Status))
            goto end;

        RtlMoveMemory(&bLoad,
                      kvpInfo->Data,
                      kvpInfo->DataLength);

        HeapFree(GetProcessHeap(), 0, kvpInfo);
        kvpInfo = NULL;

        if (bLoad)
        {
            Status = NtQueryValueKey(hKey,
                                     &szDllsName,
                                     KeyValuePartialInformation,
                                     NULL,
                                     0,
                                     &dwSize);
            if (Status != STATUS_BUFFER_TOO_SMALL)
                goto end;

            kvpInfo = HeapAlloc(GetProcessHeap(), 0, dwSize);
            if (!kvpInfo)
                goto end;

            Status = NtQueryValueKey(hKey,
                                     &szDllsName,
                                     KeyValuePartialInformation,
                                     kvpInfo,
                                     dwSize,
                                     &dwSize);
            if (NT_SUCCESS(Status))
            {
                LPWSTR lpBuffer = (LPWSTR)kvpInfo->Data;
                if (*lpBuffer != UNICODE_NULL)
                {
                    INT bytesToCopy, nullPos;

                    bytesToCopy = min(kvpInfo->DataLength, KEY_LENGTH * sizeof(WCHAR));

                    if (bytesToCopy != 0)
                    {
                        RtlMoveMemory(szAppInit,
                                      kvpInfo->Data,
                                      bytesToCopy);

                        nullPos = (bytesToCopy / sizeof(WCHAR)) - 1;

                        /* ensure string is terminated */
                        szAppInit[nullPos] = UNICODE_NULL;

                        bRet = TRUE;
                    }
                }
            }
        }
    }

end:
    if (hKey)
        NtClose(hKey);

    if (kvpInfo)
        HeapFree(GetProcessHeap(), 0, kvpInfo);

    return bRet;
}


VOID
LoadAppInitDlls()
{
    szAppInit[0] = UNICODE_NULL;

    if (GetDllList())
    {
        WCHAR buffer[KEY_LENGTH];
        LPWSTR ptr;
		size_t i;

        RtlCopyMemory(buffer, szAppInit, KEY_LENGTH);

		for (i = 0; i < KEY_LENGTH; ++ i)
		{
			if(buffer[i] == L' ' || buffer[i] == L',')
				buffer[i] = 0;
		}

		for (i = 0; i < KEY_LENGTH; )
		{
			if(buffer[i] == 0)
				++ i;
			else
			{
				ptr = buffer + i;
				i += wcslen(ptr);
				LoadLibraryW(ptr);
			}
		}
    }
}

VOID
UnloadAppInitDlls()
{
    if (szAppInit[0] != UNICODE_NULL)
    {
        WCHAR buffer[KEY_LENGTH];
        HMODULE hModule;
        LPWSTR ptr;
		size_t i;

        RtlCopyMemory(buffer, szAppInit, KEY_LENGTH);

		for (i = 0; i < KEY_LENGTH; ++ i)
		{
			if(buffer[i] == L' ' || buffer[i] == L',')
				buffer[i] = 0;
		}

		for (i = 0; i < KEY_LENGTH; )
		{
			if(buffer[i] == 0)
				++ i;
			else
			{
				ptr = buffer + i;
				i += wcslen(ptr);
				hModule = GetModuleHandleW(ptr);
				FreeLibrary(hModule);
			}
		}
    }
}

BOOL
InitThread(VOID)
{
   PUSER32_THREAD_DATA ThreadData;

   ThreadData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                          sizeof(USER32_THREAD_DATA));
   if (ThreadData == NULL)
      return FALSE;
   if (!TlsSetValue(User32TlsIndex, ThreadData))
      return FALSE;
   return TRUE;
}

VOID
CleanupThread(VOID)
{
   PUSER32_THREAD_DATA ThreadData;

   ThreadData = (PUSER32_THREAD_DATA)TlsGetValue(User32TlsIndex);
   HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, ThreadData);
   TlsSetValue(User32TlsIndex, 0);
}

BOOL
Init(VOID)
{
   USERCONNECT UserCon;

   /* Set up the kernel callbacks. */
   NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_WINDOWPROC] =
      (PVOID)User32CallWindowProcFromKernel;
   NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_SENDASYNCPROC] =
      (PVOID)User32CallSendAsyncProcForKernel;
   NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_LOADSYSMENUTEMPLATE] =
      (PVOID)User32LoadSysMenuTemplateForKernel;
   NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_LOADDEFAULTCURSORS] =
      (PVOID)User32SetupDefaultCursors;
   NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_HOOKPROC] =
      (PVOID)User32CallHookProcFromKernel;
   NtCurrentPeb()->KernelCallbackTable[USER32_CALLBACK_EVENTPROC] =
      (PVOID)User32CallEventProcFromKernel;

   NtUserProcessConnect( NtCurrentProcess(),
                         &UserCon,
                         sizeof(USERCONNECT));

   g_ppi = GetWin32ClientInfo()->ppi; // Snapshot PI, used as pointer only!
   g_ulSharedDelta = UserCon.siClient.ulSharedDelta;
   g_psi = SharedPtrToUser(UserCon.siClient.psi);
   gHandleTable = SharedPtrToUser(UserCon.siClient.aheList);
   gHandleEntries = SharedPtrToUser(gHandleTable->handles);
   //ERR("1 SI 0x%x : HT 0x%x : D 0x%x\n", UserCon.siClient.psi, UserCon.siClient.aheList,  g_ulSharedDelta);

   RegisterClientPFN();
   /* Allocate an index for user32 thread local data. */
   User32TlsIndex = TlsAlloc();
   if (User32TlsIndex != TLS_OUT_OF_INDEXES)
   {
      if (MessageInit())
      {
         if (MenuInit())
         {
            InitializeCriticalSection(&U32AccelCacheLock);
            GdiDllInitialize(NULL, DLL_PROCESS_ATTACH, NULL);
            InitStockObjects();
            LoadAppInitDlls();

            return TRUE;
         }
         MessageCleanup();
      }
      TlsFree(User32TlsIndex);
   }

   return FALSE;
}

VOID
Cleanup(VOID)
{
   DeleteCriticalSection(&U32AccelCacheLock);
   MenuCleanup();
   MessageCleanup();
   DeleteFrameBrushes();
   UnloadAppInitDlls();
   GdiDllInitialize(NULL, DLL_PROCESS_DETACH, NULL);
   TlsFree(User32TlsIndex);
}

INT WINAPI
DllMain(
   IN PVOID hInstanceDll,
   IN ULONG dwReason,
   IN PVOID reserved)
{
   switch (dwReason)
   {
      case DLL_PROCESS_ATTACH:
         User32Instance = hInstanceDll;
         if (!NtUserRegisterUserModule(hInstanceDll) ||
             !RegisterSystemControls())
         {
             return FALSE;
         }

         if (!Init())
            return FALSE;
         if (!InitThread())
         {
            Cleanup();
            return FALSE;
         }

         /* Initialize message spying */
        if (!SPY_Init()) return FALSE;

         break;

      case DLL_THREAD_ATTACH:
         if (!InitThread())
            return FALSE;
         break;

      case DLL_THREAD_DETACH:
         CleanupThread();
         break;

      case DLL_PROCESS_DETACH:
         if (hImmInstance) FreeLibrary(hImmInstance);
         CleanupThread();
         Cleanup();
         break;
   }

   return TRUE;
}


VOID
FASTCALL
GetConnected(VOID)
{
  USERCONNECT UserCon;
//  ERR("GetConnected\n");

  if ((PW32THREADINFO)NtCurrentTeb()->Win32ThreadInfo == NULL)
     NtUserGetThreadState(THREADSTATE_GETTHREADINFO);

  if (g_psi && g_ppi) return;
// FIXME HAX: Due to the "Dll Initialization Bug" we have to call this too.
  GdiDllInitialize(NULL, DLL_PROCESS_ATTACH, NULL);

  NtUserProcessConnect( NtCurrentProcess(),
                         &UserCon,
                         sizeof(USERCONNECT));

  g_ppi = GetWin32ClientInfo()->ppi;
  g_ulSharedDelta = UserCon.siClient.ulSharedDelta;
  g_psi = SharedPtrToUser(UserCon.siClient.psi);
  gHandleTable = SharedPtrToUser(UserCon.siClient.aheList);
  gHandleEntries = SharedPtrToUser(gHandleTable->handles);

  RegisterClientPFN();  
  
}
