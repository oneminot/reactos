/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
/* $Id: message.c,v 1.30 2003/10/09 06:13:04 gvg Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Messages
 * FILE:             subsys/win32k/ntuser/message.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <include/msgqueue.h>
#include <include/window.h>
#include <include/class.h>
#include <include/error.h>
#include <include/object.h>
#include <include/winsta.h>
#include <include/callback.h>
#include <include/painting.h>
#include <include/input.h>
#include <internal/safe.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS FASTCALL
IntInitMessageImpl(VOID)
{
  return STATUS_SUCCESS;
}

NTSTATUS FASTCALL
IntCleanupMessageImpl(VOID)
{
  return STATUS_SUCCESS;
}


LRESULT STDCALL
NtUserDispatchMessage(CONST MSG* UnsafeMsg)
{
  LRESULT Result;
  PWINDOW_OBJECT WindowObject;
  NTSTATUS Status;
  MSG Msg;

  Status = MmCopyFromCaller(&Msg, (PVOID) UnsafeMsg, sizeof(MSG));
  if (! NT_SUCCESS(Status))
    {
    SetLastNtError(Status);
    return 0;
    }

  /* Process timer messages. */
  if (Msg.message == WM_TIMER)
    {
      if (Msg.lParam)
	{
	  /* FIXME: Call hooks. */

	  /* FIXME: Check for continuing validity of timer. */

	  return IntCallWindowProc((WNDPROC)Msg.lParam,
				      Msg.hwnd,
				      Msg.message,
				      Msg.wParam,
				      0 /* GetTickCount() */);
	}
    }

  if( Msg.hwnd == 0 ) return 0;

  /* Get the window object. */
  Status = 
    ObmReferenceObjectByHandle(PsGetWin32Process()->WindowStation->HandleTable,
			       Msg.hwnd,
			       otWindow,
			       (PVOID*)&WindowObject);
  if (!NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return 0;
    }

  /* FIXME: Call hook procedures. */

  /* Call the window procedure. */
  if (WindowObject->Unicode == TRUE)
  {
	Result = IntCallWindowProc(WindowObject->WndProcW,
					Msg.hwnd,
					Msg.message,
					Msg.wParam,
					Msg.lParam);
  }
  else
  {
	Result = IntCallWindowProc(WindowObject->WndProcA,
					Msg.hwnd,
					Msg.message,
					Msg.wParam,
					Msg.lParam);
  }

  return Result;
}

/*
 * Internal version of PeekMessage() doing all the work
 */
BOOL STDCALL
IntPeekMessage(LPMSG Msg,
                HWND Wnd,
                UINT MsgFilterMin,
                UINT MsgFilterMax,
                UINT RemoveMsg)
{
  PUSER_MESSAGE_QUEUE ThreadQueue;
  BOOLEAN Present;
  PUSER_MESSAGE Message;
  BOOLEAN RemoveMessages;

  /* The queues and order in which they are checked are documented in the MSDN
     article on GetMessage() */

  ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;

  /* Inspect RemoveMsg flags */
  /* FIXME: The only flag we process is PM_REMOVE - processing of others must still be implemented */
  RemoveMessages = RemoveMsg & PM_REMOVE;

  /* Dispatch sent messages here. */
  while (MsqDispatchOneSentMessage(ThreadQueue))
    ;
      
  /* Now look for a quit message. */
  /* FIXME: WINE checks the message number filter here. */
  if (ThreadQueue->QuitPosted)
  {
    Msg->hwnd = Wnd;
    Msg->message = WM_QUIT;
    Msg->wParam = ThreadQueue->QuitExitCode;
    Msg->lParam = 0;
    if (RemoveMessages)
      {
        ThreadQueue->QuitPosted = FALSE;
      }
    return TRUE;
  }

  /* Now check for normal messages. */
  Present = MsqFindMessage(ThreadQueue,
                           FALSE,
                           RemoveMessages,
                           Wnd,
                           MsgFilterMin,
                           MsgFilterMax,
                           &Message);
  if (Present)
    {
      RtlCopyMemory(Msg, &Message->Msg, sizeof(MSG));
      if (RemoveMessages)
	{
	  MsqDestroyMessage(Message);
	}
      W32kKeyProcessMessage(Msg,PsGetWin32Thread()->KeyboardLayout);
      return TRUE;
    }

  /* Check for hardware events. */
  Present = MsqFindMessage(ThreadQueue,
                           TRUE,
                           RemoveMessages,
                           Wnd,
                           MsgFilterMin,
                           MsgFilterMax,
                           &Message);
  if (Present)
    {
      RtlCopyMemory(Msg, &Message->Msg, sizeof(MSG));
      if (RemoveMessages)
	{
	  MsqDestroyMessage(Message);
	}
      return TRUE;
    }

  /* Check for sent messages again. */
  while (MsqDispatchOneSentMessage(ThreadQueue))
    ;

  /* Check for paint messages. */
  if (ThreadQueue->PaintPosted)
    {
      PWINDOW_OBJECT WindowObject;

      Msg->hwnd = PaintingFindWinToRepaint(Wnd, PsGetWin32Thread());
      Msg->message = WM_PAINT;
      Msg->wParam = Msg->lParam = 0;

      WindowObject = IntGetWindowObject(Msg->hwnd);
      if (WindowObject != NULL)
	{
	  if (WindowObject->Style & WS_MINIMIZE &&
	      (HICON)NtUserGetClassLong(Msg->hwnd, GCL_HICON, FALSE) != NULL)
	    {
	      Msg->message = WM_PAINTICON;
	      Msg->wParam = 1;
	    }

	  if (Msg->hwnd == NULL || Msg->hwnd == Wnd ||
	      IntIsChildWindow(Wnd, Msg->hwnd))
	    {
	      if (WindowObject->Flags & WINDOWOBJECT_NEED_INTERNALPAINT &&
		  WindowObject->UpdateRegion == NULL)
		{
		  WindowObject->Flags &= ~WINDOWOBJECT_NEED_INTERNALPAINT;
		  if (RemoveMessages)
		    {
		      MsqDecPaintCountQueue(WindowObject->MessageQueue);
		    }
		}
	    }
	  IntReleaseWindowObject(WindowObject);
	}

      return TRUE;
    }

  return FALSE;
}

BOOL STDCALL
NtUserPeekMessage(LPMSG UnsafeMsg,
                  HWND Wnd,
                  UINT MsgFilterMin,
                  UINT MsgFilterMax,
                  UINT RemoveMsg)
{
  MSG SafeMsg;
  NTSTATUS Status;
  BOOL Present;
  PWINDOW_OBJECT Window;

  /* Validate input */
  if (NULL != Wnd)
    {
      Status = ObmReferenceObjectByHandle(PsGetWin32Process()->WindowStation->HandleTable,
                                          Wnd, otWindow, (PVOID*)&Window);
      if (!NT_SUCCESS(Status))
        {
	  Wnd = NULL;
        }
      else
	{
	  ObmDereferenceObject(Window);
	}
    }
  if (MsgFilterMax < MsgFilterMin)
    {
      MsgFilterMin = 0;
      MsgFilterMax = 0;
    }

  Present = IntPeekMessage(&SafeMsg, Wnd, MsgFilterMin, MsgFilterMax, RemoveMsg);
  if (Present)
    {
      Status = MmCopyToCaller(UnsafeMsg, &SafeMsg, sizeof(MSG));
      if (! NT_SUCCESS(Status))
	{
	  /* There is error return documented for PeekMessage().
             Do the best we can */
	  SetLastNtError(Status);
	  return FALSE;
	}
    }

  return Present;
}

static BOOL STDCALL
IntWaitMessage(HWND Wnd,
                UINT MsgFilterMin,
                UINT MsgFilterMax)
{
  PUSER_MESSAGE_QUEUE ThreadQueue;
  NTSTATUS Status;
  MSG Msg;

  ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;

  do
    {
      if (IntPeekMessage(&Msg, Wnd, MsgFilterMin, MsgFilterMax, PM_NOREMOVE))
	{
	  return TRUE;
	}

      /* Nothing found. Wait for new messages. */
      Status = MsqWaitForNewMessages(ThreadQueue);
    }
  while (STATUS_WAIT_0 <= STATUS_WAIT_0 && Status <= STATUS_WAIT_63);

  SetLastNtError(Status);

  return FALSE;
}

BOOL STDCALL
NtUserGetMessage(LPMSG UnsafeMsg,
		 HWND Wnd,
		 UINT MsgFilterMin,
		 UINT MsgFilterMax)
/*
 * FUNCTION: Get a message from the calling thread's message queue.
 * ARGUMENTS:
 *      UnsafeMsg - Pointer to the structure which receives the returned message.
 *      Wnd - Window whose messages are to be retrieved.
 *      MsgFilterMin - Integer value of the lowest message value to be
 *                     retrieved.
 *      MsgFilterMax - Integer value of the highest message value to be
 *                     retrieved.
 */
{
  BOOL GotMessage;
  MSG SafeMsg;
  NTSTATUS Status;
  PWINDOW_OBJECT Window;

  /* Validate input */
  if (NULL != Wnd)
    {
      Status = ObmReferenceObjectByHandle(PsGetWin32Process()->WindowStation->HandleTable,
                                          Wnd, otWindow, (PVOID*)&Window);
      if (!NT_SUCCESS(Status))
        {
	  Wnd = NULL;
        }
      else
	{
	  ObmDereferenceObject(Window);
	}
    }
  if (MsgFilterMax < MsgFilterMin)
    {
      MsgFilterMin = 0;
      MsgFilterMax = 0;
    }

  do
    {
      GotMessage = IntPeekMessage(&SafeMsg, Wnd, MsgFilterMin, MsgFilterMax, PM_REMOVE);
      if (GotMessage)
	{
	  Status = MmCopyToCaller(UnsafeMsg, &SafeMsg, sizeof(MSG));
	  if (! NT_SUCCESS(Status))
	    {
	      SetLastNtError(Status);
	      return (BOOL) -1;
	    }
	}
      else
	{
	  IntWaitMessage(Wnd, MsgFilterMin, MsgFilterMax);
	}
    }
  while (! GotMessage);

  return WM_QUIT != SafeMsg.message;
}

DWORD
STDCALL
NtUserMessageCall(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6)
{
  UNIMPLEMENTED

  return 0;
}

BOOL STDCALL
NtUserPostMessage(HWND hWnd,
		  UINT Msg,
		  WPARAM wParam,
		  LPARAM lParam)
{
  PWINDOW_OBJECT Window;
  MSG Mesg;
  PUSER_MESSAGE Message;
  NTSTATUS Status;
  LARGE_INTEGER LargeTickCount;

  if (WM_QUIT == Msg)
    {
      MsqPostQuitMessage(PsGetWin32Thread()->MessageQueue, wParam);
    }
  else
    {
      Status = ObmReferenceObjectByHandle(PsGetWin32Process()->WindowStation->HandleTable,
                                          hWnd, otWindow, (PVOID*)&Window);
      if (!NT_SUCCESS(Status))
        {
	  SetLastNtError(Status);
          return FALSE;
        }
      Mesg.hwnd = hWnd;
      Mesg.message = Msg;
      Mesg.wParam = wParam;
      Mesg.lParam = lParam;
      Mesg.pt.x = PsGetWin32Process()->WindowStation->SystemCursor.x;
      Mesg.pt.y = PsGetWin32Process()->WindowStation->SystemCursor.y;
      KeQueryTickCount(&LargeTickCount);
      Mesg.time = LargeTickCount.u.LowPart;
      Message = MsqCreateMessage(&Mesg);
      MsqPostMessage(Window->MessageQueue, Message);
      ObmDereferenceObject(Window);
    }

  return TRUE;
}

BOOL STDCALL
NtUserPostThreadMessage(DWORD idThread,
			UINT Msg,
			WPARAM wParam,
			LPARAM lParam)
{
  MSG Mesg;

  PUSER_MESSAGE Message;
  PETHREAD peThread;
  PW32THREAD pThread;
  NTSTATUS Status;

  Status = PsLookupThreadByThreadId((void *)idThread,&peThread);
  
  if( Status == STATUS_SUCCESS ) {
    pThread = peThread->Win32Thread;
    if( !pThread || !pThread->MessageQueue )
      {
	ObDereferenceObject( peThread );
	return FALSE;
      }
    Mesg.hwnd = 0;
    Mesg.message = Msg;
    Mesg.wParam = wParam;
    Mesg.lParam = lParam;
    Message = MsqCreateMessage(&Mesg);
    MsqPostMessage(pThread->MessageQueue, Message);
    ObDereferenceObject( peThread );
    return TRUE;
  } else {
    SetLastNtError( Status );
    return FALSE;
  }
}

DWORD STDCALL
NtUserQuerySendMessage(DWORD Unknown0)
{
  UNIMPLEMENTED;

  return 0;
}

LRESULT STDCALL
IntSendMessage(HWND hWnd,
		UINT Msg,
		WPARAM wParam,
		LPARAM lParam,
		BOOL KernelMessage)
{
  LRESULT Result;
  NTSTATUS Status;
  PWINDOW_OBJECT Window;

  /* FIXME: Check for a broadcast or topmost destination. */

  /* FIXME: Call hooks. */

  Status = 
    ObmReferenceObjectByHandle(PsGetWin32Process()->WindowStation->HandleTable,
			       hWnd,
			       otWindow,
			       (PVOID*)&Window);
  if (!NT_SUCCESS(Status))
    {
      return 0;
    }

  /* FIXME: Check for an exiting window. */

  if (NULL != PsGetWin32Thread() &&
      Window->MessageQueue == PsGetWin32Thread()->MessageQueue)
    {
      if (KernelMessage)
	{
	  Result = IntCallTrampolineWindowProc(NULL, hWnd, Msg, wParam,
						lParam);
	  return Result;
	}
      else
	{
	if (Window->Unicode == TRUE)
	{
	  Result = IntCallWindowProc(Window->WndProcW, hWnd, Msg, wParam, lParam);
	}
	else
	{
	  Result = IntCallWindowProc(Window->WndProcA, hWnd, Msg, wParam, lParam);
	}
	return Result;
	}
    }
  else
    {
      PUSER_SENT_MESSAGE Message;
      PKEVENT CompletionEvent;

      CompletionEvent = ExAllocatePool(NonPagedPool, sizeof(KEVENT));
      KeInitializeEvent(CompletionEvent, NotificationEvent, FALSE);

      Message = ExAllocatePool(NonPagedPool, sizeof(USER_SENT_MESSAGE));
      Message->Msg.hwnd = hWnd;
      Message->Msg.message = Msg;
      Message->Msg.wParam = wParam;
      Message->Msg.lParam = lParam;
      Message->CompletionEvent = CompletionEvent;
      Message->Result = &Result;
      Message->CompletionQueue = NULL;
      Message->CompletionCallback = NULL;
      MsqSendMessage(Window->MessageQueue, Message);

      ObmDereferenceObject(Window);
      Status = KeWaitForSingleObject(CompletionEvent,
				     UserRequest,
				     UserMode,
				     FALSE,
				     NULL);
      if (Status == STATUS_WAIT_0)
	{
	  return Result;
	}
      else
	{
	  return FALSE;
	}
    }
}

LRESULT STDCALL
NtUserSendMessage(HWND Wnd,
		  UINT Msg,
		  WPARAM wParam,
		  LPARAM lParam)
{
  return IntSendMessage(Wnd, Msg, wParam, lParam, FALSE);
}

BOOL STDCALL
NtUserSendMessageCallback(HWND hWnd,
			  UINT Msg,
			  WPARAM wParam,
			  LPARAM lParam,
			  SENDASYNCPROC lpCallBack,
			  ULONG_PTR dwData)
{
  UNIMPLEMENTED;

  return 0;
}

BOOL STDCALL
NtUserSendNotifyMessage(HWND hWnd,
			UINT Msg,
			WPARAM wParam,
			LPARAM lParam)
{
  UNIMPLEMENTED;

  return 0;
}

BOOL STDCALL
NtUserWaitMessage(VOID)
{

  return IntWaitMessage(NULL, 0, 0);
}

DWORD STDCALL
NtUserGetQueueStatus(BOOL ClearChanges)
{
   PUSER_MESSAGE_QUEUE Queue;
   DWORD Result;

   Queue = PsGetWin32Thread()->MessageQueue;

   ExAcquireFastMutex(&Queue->Lock);

   Result = MAKELONG(Queue->ChangedBits, Queue->WakeBits);
   if (ClearChanges)
   {
      Queue->ChangedBits = 0;
   }

   ExReleaseFastMutex(&Queue->Lock);

   return Result;
}

/* EOF */
