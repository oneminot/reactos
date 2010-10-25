/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window classes
 * FILE:             subsys/win32k/ntuser/wndproc.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                   Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 * NOTES:            Please use the Callback Memory Management functions for
 *                   callbacks to make sure, the memory is freed on thread
 *                   termination!
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/* CALLBACK MEMORY MANAGEMENT ************************************************/

typedef struct _INT_CALLBACK_HEADER
{
   /* list entry in the THREADINFO structure */
   LIST_ENTRY ListEntry;
}
INT_CALLBACK_HEADER, *PINT_CALLBACK_HEADER;

PVOID FASTCALL
IntCbAllocateMemory(ULONG Size)
{
   PINT_CALLBACK_HEADER Mem;
   PTHREADINFO W32Thread;

   if(!(Mem = ExAllocatePoolWithTag(PagedPool, Size + sizeof(INT_CALLBACK_HEADER),
                                    TAG_CALLBACK)))
   {
      return NULL;
   }

   W32Thread = PsGetCurrentThreadWin32Thread();
   ASSERT(W32Thread);

   /* insert the callback memory into the thread's callback list */

   InsertTailList(&W32Thread->W32CallbackListHead, &Mem->ListEntry);

   return (Mem + 1);
}

VOID FASTCALL
IntCbFreeMemory(PVOID Data)
{
   PINT_CALLBACK_HEADER Mem;
   PTHREADINFO W32Thread;

   ASSERT(Data);

   Mem = ((PINT_CALLBACK_HEADER)Data - 1);

   W32Thread = PsGetCurrentThreadWin32Thread();
   ASSERT(W32Thread);

   /* remove the memory block from the thread's callback list */
   RemoveEntryList(&Mem->ListEntry);

   /* free memory */
   ExFreePoolWithTag(Mem, TAG_CALLBACK);
}

VOID FASTCALL
IntCleanupThreadCallbacks(PTHREADINFO W32Thread)
{
   PLIST_ENTRY CurrentEntry;
   PINT_CALLBACK_HEADER Mem;

   while (!IsListEmpty(&W32Thread->W32CallbackListHead))
   {
      CurrentEntry = RemoveHeadList(&W32Thread->W32CallbackListHead);
      Mem = CONTAINING_RECORD(CurrentEntry, INT_CALLBACK_HEADER,
                              ListEntry);

      /* free memory */
      ExFreePool(Mem);
   }
}


//
// Pass the Current Window handle and pointer to the Client Callback.
// This will help user space programs speed up read access with the window object.
//
static VOID
IntSetTebWndCallback (HWND * hWnd, PWND * pWnd)
{
  HWND hWndS = *hWnd;
  PWND Window = UserGetWindowObject(*hWnd);
  PCLIENTINFO ClientInfo = GetWin32ClientInfo();

  *hWnd = ClientInfo->CallbackWnd.hWnd;
  *pWnd = ClientInfo->CallbackWnd.pWnd;

  ClientInfo->CallbackWnd.hWnd  = hWndS;
  ClientInfo->CallbackWnd.pWnd = DesktopHeapAddressToUser(Window);
}

static VOID
IntRestoreTebWndCallback (HWND hWnd, PWND pWnd)
{
  PCLIENTINFO ClientInfo = GetWin32ClientInfo();

  ClientInfo->CallbackWnd.hWnd = hWnd;
  ClientInfo->CallbackWnd.pWnd = pWnd;
}

/* FUNCTIONS *****************************************************************/

VOID APIENTRY
co_IntCallSentMessageCallback(SENDASYNCPROC CompletionCallback,
                              HWND hWnd,
                              UINT Msg,
                              ULONG_PTR CompletionCallbackContext,
                              LRESULT Result)
{
   SENDASYNCPROC_CALLBACK_ARGUMENTS Arguments;
   PVOID ResultPointer;
   PWND pWnd;
   ULONG ResultLength;
   NTSTATUS Status;

   Arguments.Callback = CompletionCallback;
   Arguments.Wnd = hWnd;
   Arguments.Msg = Msg;
   Arguments.Context = CompletionCallbackContext;
   Arguments.Result = Result;

   IntSetTebWndCallback (&hWnd, &pWnd);

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_SENDASYNCPROC,
                               &Arguments,
                               sizeof(SENDASYNCPROC_CALLBACK_ARGUMENTS),
                               &ResultPointer,
                               &ResultLength);

   UserEnterCo();

   IntRestoreTebWndCallback (hWnd, pWnd);

   if (!NT_SUCCESS(Status))
   {
      return;
   }
   return;
}

LRESULT APIENTRY
co_IntCallWindowProc(WNDPROC Proc,
                     BOOLEAN IsAnsiProc,
                     HWND Wnd,
                     UINT Message,
                     WPARAM wParam,
                     LPARAM lParam,
                     INT lParamBufferSize)
{
   WINDOWPROC_CALLBACK_ARGUMENTS StackArguments;
   PWINDOWPROC_CALLBACK_ARGUMENTS Arguments;
   NTSTATUS Status;
   PVOID ResultPointer;
   PWND pWnd;
   ULONG ResultLength;
   ULONG ArgumentLength;
   LRESULT Result;

   if (0 < lParamBufferSize)
   {
      ArgumentLength = sizeof(WINDOWPROC_CALLBACK_ARGUMENTS) + lParamBufferSize;
      Arguments = IntCbAllocateMemory(ArgumentLength);
      if (NULL == Arguments)
      {
         DPRINT1("Unable to allocate buffer for window proc callback\n");
         return -1;
      }
      RtlMoveMemory((PVOID) ((char *) Arguments + sizeof(WINDOWPROC_CALLBACK_ARGUMENTS)),
                    (PVOID) lParam, lParamBufferSize);
   }
   else
   {
      Arguments = &StackArguments;
      ArgumentLength = sizeof(WINDOWPROC_CALLBACK_ARGUMENTS);
   }
   Arguments->Proc = Proc;
   Arguments->IsAnsiProc = IsAnsiProc;
   Arguments->Wnd = Wnd;
   Arguments->Msg = Message;
   Arguments->wParam = wParam;
   Arguments->lParam = lParam;
   Arguments->lParamBufferSize = lParamBufferSize;
   ResultPointer = NULL;
   ResultLength = ArgumentLength;

   IntSetTebWndCallback (&Wnd, &pWnd);

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_WINDOWPROC,
                               Arguments,
                               ArgumentLength,
                               &ResultPointer,
                               &ResultLength);

   _SEH2_TRY
   {
      /* Simulate old behaviour: copy into our local buffer */
      RtlMoveMemory(Arguments, ResultPointer, ArgumentLength);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Status = _SEH2_GetExceptionCode();
   }
   _SEH2_END;

   UserEnterCo();

   IntRestoreTebWndCallback (Wnd, pWnd);

   if (!NT_SUCCESS(Status))
   {
      if (0 < lParamBufferSize)
      {
         IntCbFreeMemory(Arguments);
      }
      return -1;
   }
   Result = Arguments->Result;

   if (0 < lParamBufferSize)
   {
      RtlMoveMemory((PVOID) lParam,
                    (PVOID) ((char *) Arguments + sizeof(WINDOWPROC_CALLBACK_ARGUMENTS)),
                    lParamBufferSize);
      IntCbFreeMemory(Arguments);
   }

   return Result;
}

HMENU APIENTRY
co_IntLoadSysMenuTemplate()
{
   LRESULT Result = 0;
   NTSTATUS Status;
   PVOID ResultPointer;
   ULONG ResultLength;

   ResultPointer = NULL;
   ResultLength = sizeof(LRESULT);

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_LOADSYSMENUTEMPLATE,
                               NULL,
                               0,
                               &ResultPointer,
                               &ResultLength);
   if (NT_SUCCESS(Status))
   {
      /* Simulate old behaviour: copy into our local buffer */
      _SEH2_TRY
      {
        ProbeForRead(ResultPointer, sizeof(LRESULT), 1);
        Result = *(LRESULT*)ResultPointer;
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
        Result = 0;
      }
      _SEH2_END
   }

   UserEnterCo();

   return (HMENU)Result;
}

BOOL APIENTRY
co_IntLoadDefaultCursors(VOID)
{
   NTSTATUS Status;
   PVOID ResultPointer;
   ULONG ResultLength;
   BOOL DefaultCursor = TRUE;

   ResultPointer = NULL;
   ResultLength = sizeof(LRESULT);

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_LOADDEFAULTCURSORS,
                               &DefaultCursor,
                               sizeof(BOOL),
                               &ResultPointer,
                               &ResultLength);

   UserEnterCo();

   if (!NT_SUCCESS(Status))
   {
      return FALSE;
   }
   return TRUE;
}

LRESULT APIENTRY
co_IntCallHookProc(INT HookId,
                   INT Code,
                   WPARAM wParam,
                   LPARAM lParam,
                   HOOKPROC Proc,
                   BOOLEAN Ansi,
                   PUNICODE_STRING ModuleName)
{
   ULONG ArgumentLength;
   PVOID Argument = NULL;
   LRESULT Result = 0;
   NTSTATUS Status;
   PVOID ResultPointer;
   ULONG ResultLength;
   PHOOKPROC_CALLBACK_ARGUMENTS Common;
   CBT_CREATEWNDW *CbtCreateWnd = NULL;
   PCHAR Extra;
   PHOOKPROC_CBT_CREATEWND_EXTRA_ARGUMENTS CbtCreatewndExtra = NULL;
   UNICODE_STRING WindowName, ClassName;
   ANSI_STRING asWindowName, asClassName;
   PTHREADINFO pti;
   PWND pWnd;
   BOOL Hit = FALSE;

   ASSERT(Proc);

   pti = PsGetCurrentThreadWin32Thread();
   if (pti->TIF_flags & TIF_INCLEANUP)
   {
      DPRINT1("Thread is in cleanup and trying to call hook %d\n", Code);
      return 0;
   }

   ArgumentLength = sizeof(HOOKPROC_CALLBACK_ARGUMENTS) - sizeof(WCHAR)
                    + ModuleName->Length;
   switch(HookId)
   {
      case WH_CBT:
         switch(Code)
         {
            case HCBT_CREATEWND:
               pWnd = UserGetWindowObject((HWND) wParam);
               if (!pWnd)
               {
                  DPRINT1("WH_CBT HCBT_CREATEWND wParam bad hWnd!\n");
                  goto Fault_Exit;
               }

               CbtCreateWnd = (CBT_CREATEWNDW *) lParam;
               ArgumentLength += sizeof(HOOKPROC_CBT_CREATEWND_EXTRA_ARGUMENTS);

               if (Ansi)
               {
                  RtlInitAnsiString(&asWindowName, NULL);
                  _SEH2_TRY
                  {
                     ProbeForRead(CbtCreateWnd->lpcs->lpszName, sizeof(CHAR), 1);
                  }
                  _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                  {
                     Hit = TRUE;
                  }
                  _SEH2_END;
                  if (Hit) // Client is at deaths door.
                     goto Fault_Exit;
                  if (CbtCreateWnd->lpcs->lpszName)
                     RtlInitAnsiString(&asWindowName, (PCSZ)CbtCreateWnd->lpcs->lpszName);
                  ArgumentLength += asWindowName.Length + sizeof(CHAR);
               }
               else
               {
                  RtlInitUnicodeString(&WindowName, NULL);
                  _SEH2_TRY
                  {
                     ProbeForRead(CbtCreateWnd->lpcs->lpszName, sizeof(WCHAR), 1);
                  }
                  _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                  {
                     Hit = TRUE;
                  }
                  _SEH2_END;
                  if (Hit)
                     goto Fault_Exit;
                  if (CbtCreateWnd->lpcs->lpszName)
                     RtlInitUnicodeString(&WindowName, CbtCreateWnd->lpcs->lpszName);
                  ArgumentLength += WindowName.Length + sizeof(WCHAR);
               }

               if (!IS_ATOM(CbtCreateWnd->lpcs->lpszClass))
               {
                  if (Ansi)
                  {
                     RtlInitAnsiString(&asClassName, NULL);
                     if (CbtCreateWnd->lpcs->lpszClass)
                        RtlInitAnsiString(&asClassName, (PCSZ)CbtCreateWnd->lpcs->lpszClass);
                     ArgumentLength += asClassName.Length + sizeof(CHAR);
                  }
                  else
                  {
                     RtlInitUnicodeString(&ClassName, NULL);
                     if (CbtCreateWnd->lpcs->lpszClass)
                        RtlInitUnicodeString(&ClassName, CbtCreateWnd->lpcs->lpszClass);
                     ArgumentLength += ClassName.Length + sizeof(WCHAR);
                  }
               }
               break;

            case HCBT_MOVESIZE:
               ArgumentLength += sizeof(RECTL);
               break;
            case HCBT_ACTIVATE:
               ArgumentLength += sizeof(CBTACTIVATESTRUCT);
               break;
            case HCBT_CLICKSKIPPED:
               ArgumentLength += sizeof(MOUSEHOOKSTRUCT);
               break;
/*   ATM pass on */
            case HCBT_KEYSKIPPED:
            case HCBT_MINMAX:
            case HCBT_SETFOCUS:
            case HCBT_SYSCOMMAND:
/*   These types pass through. */
            case HCBT_DESTROYWND:
            case HCBT_QS:
               break;
            default:
               DPRINT1("Trying to call unsupported CBT hook %d\n", Code);
               goto Fault_Exit;
         }
         break;
      case WH_KEYBOARD_LL:
         ArgumentLength += sizeof(KBDLLHOOKSTRUCT);
         break;
      case WH_MOUSE_LL:
         ArgumentLength += sizeof(MSLLHOOKSTRUCT);
         break;
      case WH_MOUSE:
         ArgumentLength += sizeof(MOUSEHOOKSTRUCT);
         break;
     case WH_CALLWNDPROC:
         ArgumentLength += sizeof(CWPSTRUCT);
         break;
      case WH_CALLWNDPROCRET:
         ArgumentLength += sizeof(CWPRETSTRUCT);
         break;
      case WH_MSGFILTER:
      case WH_SYSMSGFILTER:
      case WH_GETMESSAGE:
         ArgumentLength += sizeof(MSG);
         break;
      case WH_FOREGROUNDIDLE:
      case WH_KEYBOARD:
      case WH_SHELL:
         break;
      default:
         DPRINT1("Trying to call unsupported window hook %d\n", HookId);
         goto Fault_Exit;
   }

   Argument = IntCbAllocateMemory(ArgumentLength);
   if (NULL == Argument)
   {
      DPRINT1("HookProc callback failed: out of memory\n");
      goto Fault_Exit;
   }
   Common = (PHOOKPROC_CALLBACK_ARGUMENTS) Argument;
   Common->HookId = HookId;
   Common->Code = Code;
   Common->wParam = wParam;
   Common->lParam = lParam;
   Common->Proc = Proc;
   Common->Ansi = Ansi;
   Common->ModuleNameLength = ModuleName->Length;
   if (ModuleName->Buffer)
      RtlCopyMemory(Common->ModuleName, ModuleName->Buffer, ModuleName->Length);
   Extra = (PCHAR) Common->ModuleName + Common->ModuleNameLength;

   switch(HookId)
   {
      case WH_CBT:
         switch(Code)
         {
            case HCBT_CREATEWND:
               Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
               CbtCreatewndExtra = (PHOOKPROC_CBT_CREATEWND_EXTRA_ARGUMENTS) Extra;
               RtlCopyMemory( &CbtCreatewndExtra->Cs, CbtCreateWnd->lpcs, sizeof(CREATESTRUCTW) );
               CbtCreatewndExtra->WndInsertAfter = CbtCreateWnd->hwndInsertAfter;
               Extra = (PCHAR) (CbtCreatewndExtra + 1);
               CbtCreatewndExtra->Cs.lpszClass = CbtCreateWnd->lpcs->lpszClass; // if Atom

               if (Ansi)
               {
                 if (asWindowName.Buffer)
                    RtlCopyMemory(Extra, asWindowName.Buffer, asWindowName.Length);
                 CbtCreatewndExtra->Cs.lpszName = (LPCWSTR) (Extra - (PCHAR) CbtCreatewndExtra);
                 Extra += asWindowName.Length;

                 *((CHAR *) Extra) = '\0';
                 Extra += sizeof(CHAR);
               }
               else
               {
                 if (WindowName.Buffer)
                    RtlCopyMemory(Extra, WindowName.Buffer, WindowName.Length);
                 CbtCreatewndExtra->Cs.lpszName = (LPCWSTR) (Extra - (PCHAR) CbtCreatewndExtra);
                 Extra += WindowName.Length;

                 *((WCHAR *) Extra) = L'\0';
                 Extra += sizeof(WCHAR);
               }

               if (!IS_ATOM(CbtCreateWnd->lpcs->lpszClass))
               {
                  if (Ansi)
                  {
                     if (asClassName.Buffer)
                        RtlCopyMemory(Extra, asClassName.Buffer, asClassName.Length);
                     CbtCreatewndExtra->Cs.lpszClass =
                        (LPCWSTR)(ULONG_PTR) MAKELONG(Extra - (PCHAR) CbtCreatewndExtra, 1);
                     Extra += asClassName.Length;

                     *((CHAR *) Extra) = '\0';
                     Extra += sizeof(CHAR);
                  }
                  else
                  {
                     if (ClassName.Buffer)
                        RtlCopyMemory(Extra, ClassName.Buffer, ClassName.Length);
                     CbtCreatewndExtra->Cs.lpszClass =
                        (LPCWSTR)(ULONG_PTR) MAKELONG(Extra - (PCHAR) CbtCreatewndExtra, 1);
                     Extra += ClassName.Length;

                     *((WCHAR *) Extra) = L'\0';
                     Extra += sizeof(WCHAR);
                  }
               }
               break;
            case HCBT_CLICKSKIPPED:
               RtlCopyMemory(Extra, (PVOID) lParam, sizeof(MOUSEHOOKSTRUCT));
               Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
               break;
            case HCBT_MOVESIZE:
               RtlCopyMemory(Extra, (PVOID) lParam, sizeof(RECTL));
               Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
               break;
            case HCBT_ACTIVATE:
               RtlCopyMemory(Extra, (PVOID) lParam, sizeof(CBTACTIVATESTRUCT));
               Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
               break;
         }
         break;
      case WH_KEYBOARD_LL:
         RtlCopyMemory(Extra, (PVOID) lParam, sizeof(KBDLLHOOKSTRUCT));
         Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
         break;
      case WH_MOUSE_LL:
         RtlCopyMemory(Extra, (PVOID) lParam, sizeof(MSLLHOOKSTRUCT));
         Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
         break;
      case WH_MOUSE:
         RtlCopyMemory(Extra, (PVOID) lParam, sizeof(MOUSEHOOKSTRUCT));
         Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
         break;         
      case WH_CALLWNDPROC:
         RtlCopyMemory(Extra, (PVOID) lParam, sizeof(CWPSTRUCT));
         Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
         break;         
      case WH_CALLWNDPROCRET:
         RtlCopyMemory(Extra, (PVOID) lParam, sizeof(CWPRETSTRUCT));
         Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
         break;         
      case WH_MSGFILTER:
      case WH_SYSMSGFILTER:
      case WH_GETMESSAGE:
         RtlCopyMemory(Extra, (PVOID) lParam, sizeof(MSG));
         Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
         break;
      case WH_FOREGROUNDIDLE:
      case WH_KEYBOARD:
      case WH_SHELL:
         break;         
   }

   ResultPointer = NULL;
   ResultLength = sizeof(LRESULT);

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_HOOKPROC,
                               Argument,
                               ArgumentLength,
                               &ResultPointer,
                               &ResultLength);

   UserEnterCo();

   _SEH2_TRY
   {
      ProbeForRead(ResultPointer, sizeof(LRESULT), 1);
      /* Simulate old behaviour: copy into our local buffer */
      Result = *(LRESULT*)ResultPointer;
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Result = 0;
      Hit = TRUE;
   }
   _SEH2_END;

   if (!NT_SUCCESS(Status))
   {
      goto Fault_Exit;
   }
   /* Support write backs... SEH is in UserCallNextHookEx. */
   switch (HookId)
   {
      case WH_CBT:
         if (Code == HCBT_CREATEWND)
         {
            if (CbtCreatewndExtra)
            {/*
               The parameters could have been changed, include the coordinates
               and dimensions of the window. We copy it back.
              */
               CbtCreateWnd->hwndInsertAfter = CbtCreatewndExtra->WndInsertAfter;
               CbtCreateWnd->lpcs->x  = CbtCreatewndExtra->Cs.x;
               CbtCreateWnd->lpcs->y  = CbtCreatewndExtra->Cs.y;
               CbtCreateWnd->lpcs->cx = CbtCreatewndExtra->Cs.cx;
               CbtCreateWnd->lpcs->cy = CbtCreatewndExtra->Cs.cy;
            }
         }
         break;
      // "The GetMsgProc hook procedure can examine or modify the message."
      case WH_GETMESSAGE:
         if (lParam)
         {
            RtlCopyMemory((PVOID) lParam, Extra, sizeof(MSG));
         }
         break;
   }

Fault_Exit:
   if (Hit)
   {
      DPRINT1("Exception CallHookProc HookId %d Code %d\n",HookId,Code);
   }
   if (Argument) IntCbFreeMemory(Argument);

   return Result;
}

//
// Events are notifications w/o results.
//
LRESULT
APIENTRY
co_IntCallEventProc(HWINEVENTHOOK hook,
                           DWORD event,
                             HWND hWnd,
                         LONG idObject,
                          LONG idChild,
                   DWORD dwEventThread,
                   DWORD dwmsEventTime,
                     WINEVENTPROC Proc)
{
   LRESULT Result = 0;
   NTSTATUS Status;
   PEVENTPROC_CALLBACK_ARGUMENTS Common;
   ULONG ArgumentLength, ResultLength;
   PVOID Argument, ResultPointer;

   ArgumentLength = sizeof(EVENTPROC_CALLBACK_ARGUMENTS);

   Argument = IntCbAllocateMemory(ArgumentLength);
   if (NULL == Argument)
   {
      DPRINT1("EventProc callback failed: out of memory\n");
      return 0;
   }
   Common = (PEVENTPROC_CALLBACK_ARGUMENTS) Argument;
   Common->hook = hook;
   Common->event = event;
   Common->hwnd = hWnd;
   Common->idObject = idObject;
   Common->idChild = idChild;
   Common->dwEventThread = dwEventThread;
   Common->dwmsEventTime = dwmsEventTime;
   Common->Proc = Proc;

   ResultPointer = NULL;
   ResultLength = sizeof(LRESULT);

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_EVENTPROC,
                               Argument,
                               ArgumentLength,
                               &ResultPointer,
                               &ResultLength);

   UserEnterCo();

   IntCbFreeMemory(Argument);
  
   if (!NT_SUCCESS(Status))
   {
      return 0;
   }

   return Result;
}

//
// Callback Load Menu and results.
//
HMENU
APIENTRY
co_IntCallLoadMenu( HINSTANCE hModule,
                    PUNICODE_STRING pMenuName )
{
   LRESULT Result = 0;
   NTSTATUS Status;
   PLOADMENU_CALLBACK_ARGUMENTS Common;
   ULONG ArgumentLength, ResultLength;
   PVOID Argument, ResultPointer;

   ArgumentLength = sizeof(LOADMENU_CALLBACK_ARGUMENTS);

   ArgumentLength += pMenuName->Length + sizeof(WCHAR);

   Argument = IntCbAllocateMemory(ArgumentLength);
   if (NULL == Argument)
   {
      DPRINT1("LoadMenu callback failed: out of memory\n");
      return 0;
   }
   Common = (PLOADMENU_CALLBACK_ARGUMENTS) Argument;

   // Help Intersource check and MenuName is now 4 bytes + so zero it.
   RtlZeroMemory(Common, ArgumentLength); 

   Common->hModule = hModule;
   if (pMenuName->Length)
      RtlCopyMemory(&Common->MenuName, pMenuName->Buffer, pMenuName->Length);
   else
      RtlCopyMemory(&Common->MenuName, &pMenuName->Buffer, sizeof(WCHAR));

   ResultPointer = NULL;
   ResultLength = sizeof(LRESULT);

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_LOADMENU,
                               Argument,
                               ArgumentLength,
                               &ResultPointer,
                               &ResultLength);

   UserEnterCo();

   Result = *(LRESULT*)ResultPointer;

   IntCbFreeMemory(Argument);

   if (!NT_SUCCESS(Status))
   {
      return 0;
   }

   return (HMENU)Result;
}

NTSTATUS
APIENTRY
co_IntClientThreadSetup(VOID)
{
   NTSTATUS Status;
   ULONG ArgumentLength, ResultLength;
   PVOID Argument, ResultPointer;

   ArgumentLength = ResultLength = 0;
   Argument = ResultPointer = NULL;

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_CLIENTTHREADSTARTUP,
                               Argument,
                               ArgumentLength,
                               &ResultPointer,
                               &ResultLength);

   UserEnterCo();

   return Status;
}


/* EOF */
