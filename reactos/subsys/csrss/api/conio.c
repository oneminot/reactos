/* $Id: conio.c,v 1.50 2003/08/18 07:20:23 jimtabor Exp $
 *
 * reactos/subsys/csrss/api/conio.c
 *
 * Console I/O functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>

#include <csrss/csrss.h>
#include "api.h"
#include <ntdll/rtl.h>
#include <ddk/ntddblue.h>

#define NDEBUG
#include <debug.h>

#define LOCK   RtlEnterCriticalSection(&ActiveConsoleLock)
#define UNLOCK RtlLeaveCriticalSection(&ActiveConsoleLock)

/* FIXME: Is there a way to create real aliasses with gcc? [CSH] */
#define ALIAS(Name, Target) typeof(Target) Name = Target
extern VOID CsrConsoleCtrlEvent(DWORD Event);

/* GLOBALS *******************************************************************/

static HANDLE ConsoleDeviceHandle;
static HANDLE KeyboardDeviceHandle;
static PCSRSS_CONSOLE ActiveConsole;
CRITICAL_SECTION ActiveConsoleLock;
static COORD PhysicalConsoleSize;
static BOOL KeyReadInhibit = FALSE;
static PCONTROLDISPATCHER CtrlDispatcher;

/* FUNCTIONS *****************************************************************/

VOID CsrConsoleCtrlEvent(DWORD Event)
{
HANDLE Process, hThread;
NTSTATUS Status;
CLIENT_ID ClientId, ClientId1;


	ClientId.UniqueProcess = (HANDLE) ActiveConsole->ProcessId;
	
	
	DPRINT1("CsrConsoleCtrlEvent Parent ProcessId = %x\n",	ClientId.UniqueProcess);
		
		
	Status = NtOpenProcess( &Process, PROCESS_DUP_HANDLE, 0, &ClientId );
	if( !NT_SUCCESS( Status ) )
	{
		DPRINT("CsrConsoleCtrlEvent: Failed for handle duplication\n");
		return;
	}

	DPRINT1("CsrConsoleCtrlEvent Process Handle = %x\n", Process);


	Status = RtlCreateUserThread(Process, NULL, FALSE, 0, NULL, NULL,
					(PTHREAD_START_ROUTINE)CtrlDispatcher,
					(PVOID) Event, hThread, &ClientId1);
	if( !NT_SUCCESS( Status ) )
	{
		DPRINT("CsrConsoleCtrlEvent: Failed Thread creation\n");
		NtClose(Process);
		return;
	}
	DPRINT1("CsrConsoleCtrlEvent Parent ProcessId = %x, ReturnPId = %x, hT = %x\n",
			ClientId.UniqueProcess, ClientId1.UniqueProcess, hThread);
	NtClose(Process);
}


CSR_API(CsrAllocConsole)
{
   PCSRSS_CONSOLE Console;
   HANDLE Process;
   NTSTATUS Status;
   CLIENT_ID ClientId;

   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
     sizeof(LPC_MESSAGE);

   if (ProcessData == NULL)
   {
     return(Reply->Status = STATUS_INVALID_PARAMETER);
   }

   if( ProcessData->Console )
      {
	 Reply->Status = STATUS_INVALID_PARAMETER;
	 return STATUS_INVALID_PARAMETER;
      }
   Reply->Status = STATUS_SUCCESS;
   Console = RtlAllocateHeap( CsrssApiHeap, 0, sizeof( CSRSS_CONSOLE ) );
   if( Console == 0 )
      {
	Reply->Status = STATUS_INSUFFICIENT_RESOURCES;
	return STATUS_INSUFFICIENT_RESOURCES;
      }
   Reply->Status = CsrInitConsole( Console );
   if( !NT_SUCCESS( Reply->Status ) )
     {
       RtlFreeHeap( CsrssApiHeap, 0, Console );
       return Reply->Status;
     }
   ProcessData->Console = Console;
   /* add a reference count because the process is tied to the console */
   Console->Header.ReferenceCount++;
   Status = CsrInsertObject( ProcessData, &Reply->Data.AllocConsoleReply.InputHandle, &Console->Header );
   if( !NT_SUCCESS( Status ) )
      {
	 CsrDeleteConsole( Console );
	 ProcessData->Console = 0;
	 return Reply->Status = Status;
      }
   Status = CsrInsertObject( ProcessData, &Reply->Data.AllocConsoleReply.OutputHandle, &Console->ActiveBuffer->Header );
   if( !NT_SUCCESS( Status ) )
      {
	 Console->Header.ReferenceCount--;
	 CsrReleaseObject( ProcessData, Reply->Data.AllocConsoleReply.InputHandle );
	 ProcessData->Console = 0;
	 return Reply->Status = Status;
      }

   CtrlDispatcher = Request->Data.AllocConsoleRequest.CtrlDispatcher;
   DPRINT1("CSRSS:CtrlDispatcher address: %x\n", CtrlDispatcher);      
   ProcessData->Console->ProcessId = ProcessData->ProcessId;

   ClientId.UniqueProcess = (HANDLE)ProcessData->ProcessId;
   Status = NtOpenProcess( &Process, PROCESS_DUP_HANDLE, 0, &ClientId );
   if( !NT_SUCCESS( Status ) )
     {
       DbgPrint( "CSR: NtOpenProcess() failed for handle duplication\n" );
       Console->Header.ReferenceCount--;
       ProcessData->Console = 0;
       CsrReleaseObject( ProcessData, Reply->Data.AllocConsoleReply.OutputHandle );
       CsrReleaseObject( ProcessData, Reply->Data.AllocConsoleReply.InputHandle );
       Reply->Status = Status;
       return Status;
     }
   Status = NtDuplicateObject( NtCurrentProcess(), ProcessData->Console->ActiveEvent, Process, &ProcessData->ConsoleEvent, SYNCHRONIZE, FALSE, 0 );
   if( !NT_SUCCESS( Status ) )
     {
       DbgPrint( "CSR: NtDuplicateObject() failed: %x\n", Status );
       NtClose( Process );
       Console->Header.ReferenceCount--;
       CsrReleaseObject( ProcessData, Reply->Data.AllocConsoleReply.OutputHandle );
       CsrReleaseObject( ProcessData, Reply->Data.AllocConsoleReply.InputHandle );
       ProcessData->Console = 0;
       Reply->Status = Status;
       return Status;
     }
   NtClose( Process );
   return STATUS_SUCCESS;
}

CSR_API(CsrFreeConsole)
{
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
     sizeof(LPC_MESSAGE);

   if (ProcessData == NULL)
   {
     return(Reply->Status = STATUS_INVALID_PARAMETER);
   }

   Reply->Status = STATUS_NOT_IMPLEMENTED;
   
   return(STATUS_NOT_IMPLEMENTED);
}

CSR_API(CsrReadConsole)
{
   PLIST_ENTRY CurrentEntry;
   ConsoleInput *Input;
   PCHAR Buffer;
   int   i = 0;
   ULONG nNumberOfCharsToRead;
   PCSRSS_CONSOLE Console;
   NTSTATUS Status;
   
   /* truncate length to CSRSS_MAX_READ_CONSOLE_REQUEST */
   nNumberOfCharsToRead = Request->Data.ReadConsoleRequest.NrCharactersToRead > CSRSS_MAX_READ_CONSOLE_REQUEST ? CSRSS_MAX_READ_CONSOLE_REQUEST : Request->Data.ReadConsoleRequest.NrCharactersToRead;
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = Reply->Header.MessageSize -
     sizeof(LPC_MESSAGE);

   Buffer = Reply->Data.ReadConsoleReply.Buffer;
   LOCK;   
   Status = CsrGetObject( ProcessData, Request->Data.ReadConsoleRequest.ConsoleHandle, (Object_t **)&Console );
   if( !NT_SUCCESS( Status ) )
      {
	 Reply->Status = Status;
	 UNLOCK;
	 return Status;
      }
   if( Console->Header.Type != CSRSS_CONSOLE_MAGIC )
      {
	 Reply->Status = STATUS_INVALID_HANDLE;
	 UNLOCK;
	 return STATUS_INVALID_HANDLE;
      }
   Reply->Data.ReadConsoleReply.EventHandle = ProcessData->ConsoleEvent;
   for (; i<nNumberOfCharsToRead && Console->InputEvents.Flink != &Console->InputEvents; i++ )     
      {
	 // remove input event from queue
        CurrentEntry = RemoveHeadList(&Console->InputEvents);
        Input = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);

	 // only pay attention to valid ascii chars, on key down
	 if( Input->InputEvent.EventType == KEY_EVENT &&
	     Input->InputEvent.Event.KeyEvent.bKeyDown == TRUE &&
	     Input->InputEvent.Event.KeyEvent.uChar.AsciiChar )
	    {
	       // backspace handling
	       if( Input->InputEvent.Event.KeyEvent.uChar.AsciiChar == '\b' )
		  {
		     // echo if it has not already been done, and either we or the client has chars to be deleted
		     if( !Input->Echoed && ( i || Request->Data.ReadConsoleRequest.nCharsCanBeDeleted ) )
			CsrpWriteConsole( Console->ActiveBuffer, &Input->InputEvent.Event.KeyEvent.uChar.AsciiChar, 1, TRUE );
		     if( i )
			i-=2;        // if we already have something to return, just back it up by 2
		     else
			{            // otherwise, return STATUS_NOTIFY_CLEANUP to tell client to back up its buffer
			   Reply->Data.ReadConsoleReply.NrCharactersRead = 0;
			   Reply->Status = STATUS_NOTIFY_CLEANUP;
			   Console->WaitingChars--;
			   RtlFreeHeap( CsrssApiHeap, 0, Input );
			   UNLOCK;
			   return STATUS_NOTIFY_CLEANUP;
			}
		     Request->Data.ReadConsoleRequest.nCharsCanBeDeleted--;
		     Input->Echoed = TRUE;   // mark as echoed so we don't echo it below
		  }
	       // do not copy backspace to buffer
	       else Buffer[i] = Input->InputEvent.Event.KeyEvent.uChar.AsciiChar;
	       // echo to screen if enabled and we did not already echo the char
	       if( Console->Mode & ENABLE_ECHO_INPUT &&
		   !Input->Echoed &&
		   Input->InputEvent.Event.KeyEvent.uChar.AsciiChar != '\r' )
		  CsrpWriteConsole( Console->ActiveBuffer, &Input->InputEvent.Event.KeyEvent.uChar.AsciiChar, 1, TRUE );
	    }
	 else i--;
	 Console->WaitingChars--;
	 RtlFreeHeap( CsrssApiHeap, 0, Input );
      }
   Reply->Data.ReadConsoleReply.NrCharactersRead = i;
   if( !i )
      Reply->Status = STATUS_PENDING;    // we didn't read anything
   else if( Console->Mode & ENABLE_LINE_INPUT )
      if( !Console->WaitingLines || Buffer[i-1] != '\n' )
	 {
	    Reply->Status = STATUS_PENDING; // line buffered, didn't get a complete line
	 }
      else {
	 Console->WaitingLines--;
	 Reply->Status = STATUS_SUCCESS; // line buffered, did get a complete line
      }
   else Reply->Status = STATUS_SUCCESS;  // not line buffered, did read something
   if( Reply->Status == STATUS_PENDING )
      {
	 Console->EchoCount = nNumberOfCharsToRead - i;
      }
   else {
      Console->EchoCount = 0;             // if the client is no longer waiting on input, do not echo
   }
   Reply->Header.MessageSize += i;
   UNLOCK;
   return Reply->Status;
}

#define GET_CELL_BUFFER(b,o)\
(b)->Buffer[(o)++];

#define SET_CELL_BUFFER(b,o,c,a)\
(b)->Buffer[(o)++]=(c);\
(b)->Buffer[(o)++]=(a);

static VOID FASTCALL
ClearLineBuffer (
	PCSRSS_SCREEN_BUFFER	Buff,
	DWORD			StartX
	)
{
	DWORD Offset   = 2 * ((Buff->CurrentY * Buff->MaxX) + StartX);
	
	for ( ; StartX < Buff->MaxX; StartX ++ )
	{
		/* Fill the cell: Offset is incremented by the macro */
		SET_CELL_BUFFER(Buff,Offset,' ',Buff->DefaultAttrib)
	}
}

NTSTATUS STDCALL CsrpWriteConsole( PCSRSS_SCREEN_BUFFER Buff, CHAR *Buffer, DWORD Length, BOOL Attrib )
{
   IO_STATUS_BLOCK Iosb;
   NTSTATUS Status;
   int i;
   DWORD Offset;
   
   for( i = 0; i < Length; i++ )
      {
	 switch( Buffer[ i ] )
	    {
	    /* --- LF --- */
	    case '\n':
	       Buff->CurrentX = 0;
	       /* slide the viewable screen */
	       if (((Buff->CurrentY-Buff->ShowY + Buff->MaxY) % Buff->MaxY) == PhysicalConsoleSize.Y - 1)
	         if (++Buff->ShowY == Buff->MaxY)
		   Buff->ShowY = 0;
	       if( ++Buff->CurrentY == Buff->MaxY )
		  {
		     Buff->CurrentY = 0;
		  }
	       ClearLineBuffer (Buff, 0);
	       break;
	    /* --- BS --- */
	    case '\b':
	      if( Buff->CurrentX == 0 )
		{
		  /* slide viewable screen up */
		  if( Buff->ShowY == Buff->CurrentY )
		    {
		      if( Buff->ShowY == 0 )
			Buff->ShowY = Buff->MaxY;
		      else
			Buff->ShowY--;
		    }
		  /* slide virtual position up */
		  Buff->CurrentX = Buff->MaxX;
		  if( Buff->CurrentY == 0 )
		    Buff->CurrentY = Buff->MaxY;
		  else
		    Buff->CurrentY--;
		}
	      else
		Buff->CurrentX--;
	      Offset = 2 * ((Buff->CurrentY * Buff->MaxX) + Buff->CurrentX);
	      SET_CELL_BUFFER(Buff,Offset,' ',Buff->DefaultAttrib);
	      break;
	    /* --- CR --- */
	    case '\r':
	      Buff->CurrentX = 0;
	      break;
	    /* --- TAB --- */
	    case '\t':
	      CsrpWriteConsole(Buff, "        ", (8 - (Buff->CurrentX % 8)), FALSE);
	      break;
	    /* --- */
	    default:
	       Offset = 2 * (((Buff->CurrentY * Buff->MaxX)) + Buff->CurrentX);
	       Buff->Buffer[Offset ++] = Buffer[ i ];
	       if( Attrib )
		  Buff->Buffer[Offset] = Buff->DefaultAttrib;
	       Buff->CurrentX++;
	       if( Buff->CurrentX == Buff->MaxX )
		  {
		     /* if end of line, go to next */
		     Buff->CurrentX = 0;
		     if( ++Buff->CurrentY == Buff->MaxY )
			{
			   /* if end of buffer, wrap back to beginning */
			   Buff->CurrentY = 0;
			}
		     /* clear new line */
		     ClearLineBuffer (Buff, 0);
		     /* slide the viewable screen */
	             if( (Buff->CurrentY - Buff->ShowY + Buff->MaxY - 1) % Buff->MaxY == PhysicalConsoleSize.Y -1)
		       if( ++Buff->ShowY == Buff->MaxY )
			 Buff->ShowY = 0;
		  }
	    }
      }
   if( Buff == ActiveConsole->ActiveBuffer )
     {    /* only write to screen if Console is Active, and not scrolled up */
	if( Attrib )
	   {
	      Status = NtWriteFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb, Buffer, Length, NULL, 0);
	      if (!NT_SUCCESS(Status))
		 DbgPrint("CSR: Write failed\n");
	   }
      }
   return(STATUS_SUCCESS);
}

#define CsrpInitRect(_Rect, _Top, _Left, _Bottom, _Right) \
{ \
  ((_Rect).Top) = _Top; \
  ((_Rect).Left) = _Left; \
  ((_Rect).Bottom) = _Bottom; \
  ((_Rect).Right) = _Right; \
}

#define CsrpRectHeight(Rect) \
    ((Rect.Top) > (Rect.Bottom) ? 0 : (Rect.Bottom) - (Rect.Top) + 1)

#define CsrpRectWidth(Rect) \
    ((Rect.Left) > (Rect.Right) ? 0 : (Rect.Right) - (Rect.Left) + 1)

#define CsrpIsRectEmpty(Rect) \
  ((Rect.Left > Rect.Right) || (Rect.Top > Rect.Bottom))


inline BOOLEAN CsrpIsEqualRect(
  SMALL_RECT Rect1,
  SMALL_RECT Rect2)
{
  return ((Rect1.Left == Rect2.Left) && (Rect1.Right == Rect2.Right) &&
	  (Rect1.Top == Rect2.Top) && (Rect1.Bottom == Rect2.Bottom));
}

inline BOOLEAN CsrpGetIntersection(
  PSMALL_RECT Intersection,
  SMALL_RECT Rect1,
  SMALL_RECT Rect2)
{
  if (CsrpIsRectEmpty(Rect1) ||
    (CsrpIsRectEmpty(Rect2)) ||
    (Rect1.Top > Rect2.Bottom) ||
    (Rect1.Left > Rect2.Right) ||
    (Rect1.Bottom < Rect2.Top) ||
    (Rect1.Right < Rect2.Left))
  {
    /* The rectangles do not intersect */
    CsrpInitRect(*Intersection, 0, -1, 0, -1)
    return FALSE;
  }

  CsrpInitRect(
    *Intersection,
    RtlMax(Rect1.Top, Rect2.Top),
    RtlMax(Rect1.Left, Rect2.Left),
    RtlMin(Rect1.Bottom, Rect2.Bottom),
    RtlMin(Rect1.Right, Rect2.Right));
  return TRUE;
}

inline BOOLEAN CsrpGetUnion(
  PSMALL_RECT Union,
  SMALL_RECT Rect1,
  SMALL_RECT Rect2)
{
  if (CsrpIsRectEmpty(Rect1))
    {
	    if (CsrpIsRectEmpty(Rect2))
	    {
	      CsrpInitRect(*Union, 0, -1, 0, -1);
	      return FALSE;
	    }
	  else
      *Union = Rect2;
    }
  else
    {
	    if (CsrpIsRectEmpty(Rect2))
        {
        *Union = Rect1;
        }
	    else
	      {
          CsrpInitRect(
            *Union,
            RtlMin(Rect1.Top, Rect2.Top),
            RtlMin(Rect1.Left, Rect2.Left),
            RtlMax(Rect1.Bottom, Rect2.Bottom),
            RtlMax(Rect1.Right, Rect2.Right));
	      }
    }
  return TRUE;
}

inline BOOLEAN CsrpSubtractRect(
  PSMALL_RECT Subtraction,
  SMALL_RECT Rect1,
  SMALL_RECT Rect2)
{
  SMALL_RECT tmp;

  if (CsrpIsRectEmpty(Rect1))
    {
	    CsrpInitRect(*Subtraction, 0, -1, 0, -1);
	    return FALSE;
    }
  *Subtraction = Rect1;
  if (CsrpGetIntersection(&tmp, Rect1, Rect2))
    {
	    if (CsrpIsEqualRect(tmp, *Subtraction))
	      {
	        CsrpInitRect(*Subtraction, 0, -1, 0, -1);
	        return FALSE;
	      }
	    if ((tmp.Top == Subtraction->Top) && (tmp.Bottom == Subtraction->Bottom))
	      {
	        if (tmp.Left == Subtraction->Left)
            Subtraction->Left = tmp.Right;
	        else if (tmp.Right == Subtraction->Right)
            Subtraction->Right = tmp.Left;
	      }
	    else if ((tmp.Left == Subtraction->Left) && (tmp.Right == Subtraction->Right))
	      {
	        if (tmp.Top == Subtraction->Top)
            Subtraction->Top = tmp.Bottom;
	        else if (tmp.Bottom == Subtraction->Bottom)
            Subtraction->Bottom = tmp.Top;
	      }
    }
  return TRUE;
}

/*
 * Screen buffer must be locked when this function is called
 */
static VOID CsrpCopyRegion(
  PCSRSS_SCREEN_BUFFER ScreenBuffer,
  SMALL_RECT SrcRegion,
  SMALL_RECT DstRegion)
{
  SHORT SrcY, DstY;
  DWORD SrcOffset;
  DWORD DstOffset;
  DWORD BytesPerLine;
  ULONG i;

  DstY = DstRegion.Top;
  BytesPerLine = CsrpRectWidth(DstRegion) * 2;

  SrcY = (SrcRegion.Top + ScreenBuffer->ShowY) % ScreenBuffer->MaxY;
  DstY = (DstRegion.Top + ScreenBuffer->ShowY) % ScreenBuffer->MaxY;
  SrcOffset = (SrcY * ScreenBuffer->MaxX + SrcRegion.Left + ScreenBuffer->ShowX) * 2;
  DstOffset = (DstY * ScreenBuffer->MaxX + DstRegion.Left + ScreenBuffer->ShowX) * 2;
  
  for (i = SrcRegion.Top; i <= SrcRegion.Bottom; i++)
  {
    RtlCopyMemory(
      &ScreenBuffer->Buffer[DstOffset],
      &ScreenBuffer->Buffer[SrcOffset],
      BytesPerLine);

    if (++DstY == ScreenBuffer->MaxY)
    {
      DstY = 0;
      DstOffset = (DstRegion.Left + ScreenBuffer->ShowX) * 2;
    }
    else
    {
      DstOffset += ScreenBuffer->MaxX * 2;
    }

    if (++SrcY == ScreenBuffer->MaxY)
    {
      SrcY = 0;
      SrcOffset = (SrcRegion.Left + ScreenBuffer->ShowX) * 2;
    }
    else
    {
      SrcOffset += ScreenBuffer->MaxX * 2;
    }
  }
}

/*
 * Screen buffer must be locked when this function is called
 */
static VOID CsrpFillRegion(
  PCSRSS_SCREEN_BUFFER ScreenBuffer,
  SMALL_RECT Region,
  CHAR_INFO CharInfo)
{
  SHORT X, Y;
  DWORD Offset;
  DWORD Delta;
  ULONG i;

  Y = (Region.Top + ScreenBuffer->ShowY) % ScreenBuffer->MaxY;
  Offset = (Y * ScreenBuffer->MaxX + Region.Left + ScreenBuffer->ShowX) * 2;
  Delta = (ScreenBuffer->MaxX - CsrpRectWidth(Region)) * 2;

  for (i = Region.Top; i <= Region.Bottom; i++)
  {
    for (X = Region.Left; X <= Region.Right; X++)
    {
      SET_CELL_BUFFER(ScreenBuffer, Offset, CharInfo.Char.AsciiChar, CharInfo.Attributes);
    }
    if (++Y == ScreenBuffer->MaxY)
    {
      Y = 0;
      Offset = (Region.Left + ScreenBuffer->ShowX) * 2;
    }
    else
    {
      Offset += Delta;
    }
  }
}

/*
 * Screen buffer must be locked when this function is called
 */
inline NTSTATUS CsrpSetConsoleDeviceCursor(PCSRSS_SCREEN_BUFFER ScreenBuffer, SHORT X, SHORT Y)
{
   CONSOLE_SCREEN_BUFFER_INFO ScrInfo;
   IO_STATUS_BLOCK Iosb;

   ScrInfo.dwCursorPosition.X = X;
   ScrInfo.dwCursorPosition.Y = Y;
   ScrInfo.wAttributes = ScreenBuffer->DefaultAttrib;

   return NtDeviceIoControlFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb,
     IOCTL_CONSOLE_SET_SCREEN_BUFFER_INFO, &ScrInfo, sizeof( ScrInfo ), 0, 0 );
}

/*
 * Region - Region of virtual screen buffer to draw onto the physical console
 * Screen buffer must be locked when this function is called
 */
static VOID CsrpDrawRegion(
  PCSRSS_SCREEN_BUFFER ScreenBuffer,
  SMALL_RECT Region)
{
   IO_STATUS_BLOCK Iosb;
   NTSTATUS Status;
   CONSOLE_SCREEN_BUFFER_INFO ScrInfo;
   CONSOLE_MODE Mode;
   int i, y;
   DWORD BytesPerLine;
   DWORD SrcOffset;
   DWORD SrcDelta;

   Mode.dwMode = 0; /* clear ENABLE_PROCESSED_OUTPUT mode */
   Status = NtDeviceIoControlFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb,
     IOCTL_CONSOLE_SET_MODE, &Mode, sizeof( Mode ), 0, 0 );
   if( !NT_SUCCESS( Status ) )
     {
       DbgPrint( "CSR: Failed to set console mode\n" );
       return;
     }

   /* blast out buffer */
   BytesPerLine = CsrpRectWidth(Region) * 2;
   SrcOffset = (((Region.Top + ScreenBuffer->ShowY) % ScreenBuffer->MaxY) * ScreenBuffer->MaxX + Region.Left + ScreenBuffer->ShowX) * 2;
   SrcDelta = ScreenBuffer->MaxX * 2;
   for( i = Region.Top, y = ScreenBuffer->ShowY; i <= Region.Bottom; i++ )
     {
        /* Position the cursor correctly */
        Status = CsrpSetConsoleDeviceCursor(ScreenBuffer, Region.Left, i);
        if( !NT_SUCCESS( Status ) )
          {
            DbgPrint( "CSR: Failed to set console info\n" );
            return;
          }

        Status = NtWriteFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb,
         &ScreenBuffer->Buffer[ SrcOffset ],
         BytesPerLine, 0, 0 );
       if( !NT_SUCCESS( Status ) )
	      {
	        DbgPrint( "CSR: Write to console failed\n" );
	        return;
	      }

       /* wrap back around the end of the buffer */
       if( ++y == ScreenBuffer->MaxY )
       {
	 y = 0;
         SrcOffset = (Region.Left + ScreenBuffer->ShowX) * 2;
       }
       else
       {
         SrcOffset += SrcDelta;
       }
     }
   Mode.dwMode = ENABLE_PROCESSED_OUTPUT;
   Status = NtDeviceIoControlFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb,
     IOCTL_CONSOLE_SET_MODE, &Mode, sizeof( Mode ), 0, 0 );
   if( !NT_SUCCESS( Status ) )
     {
       DbgPrint( "CSR: Failed to set console mode\n" );
       return;
     }
   Status = CsrpSetConsoleDeviceCursor(
     ScreenBuffer,
     ScreenBuffer->CurrentX - ScreenBuffer->ShowX,
     ((ScreenBuffer->CurrentY + ScreenBuffer->MaxY) - ScreenBuffer->ShowY) % ScreenBuffer->MaxY);
   if( !NT_SUCCESS( Status ) )
     {
       DbgPrint( "CSR: Failed to set console info\n" );
       return;
     }
   Status = NtDeviceIoControlFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb,
     IOCTL_CONSOLE_SET_CURSOR_INFO, &ScreenBuffer->CursorInfo,
     sizeof( ScreenBuffer->CursorInfo ), 0, 0 );
   if( !NT_SUCCESS( Status ) )
     {
       DbgPrint( "CSR: Failed to set cursor info\n" );
       return;
     }
}


CSR_API(CsrWriteConsole)
{
   BYTE *Buffer = Request->Data.WriteConsoleRequest.Buffer;
   PCSRSS_SCREEN_BUFFER Buff;

   DPRINT("CsrWriteConsole\n");
   
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
     sizeof(LPC_MESSAGE);

   LOCK;
   if( !NT_SUCCESS( CsrGetObject( ProcessData, Request->Data.WriteConsoleRequest.ConsoleHandle,
     (Object_t **)&Buff ) ) || Buff->Header.Type != CSRSS_SCREEN_BUFFER_MAGIC )
      {
	 UNLOCK;
	 return Reply->Status = STATUS_INVALID_HANDLE;
      }
   CsrpWriteConsole( Buff, Buffer, Request->Data.WriteConsoleRequest.NrCharactersToWrite, TRUE );
   UNLOCK;
   return Reply->Status = STATUS_SUCCESS;
}


NTSTATUS STDCALL CsrInitConsoleScreenBuffer( PCSRSS_SCREEN_BUFFER Console )
{
  Console->Header.Type = CSRSS_SCREEN_BUFFER_MAGIC;
  Console->Header.ReferenceCount = 0;
  Console->MaxX = PhysicalConsoleSize.X;
  Console->MaxY = PhysicalConsoleSize.Y;
  Console->ShowX = 0;
  Console->ShowY = 0;
  Console->CurrentX = 0;
  Console->CurrentY = 0;
  Console->Buffer = RtlAllocateHeap( CsrssApiHeap, 0, Console->MaxX * Console->MaxY * 2 );
  if( Console->Buffer == 0 )
    return STATUS_INSUFFICIENT_RESOURCES;
  Console->DefaultAttrib = 0x17;
  /* initialize buffer to be empty with default attributes */
  for( ; Console->CurrentY < Console->MaxY; Console->CurrentY++ )
    {
	    ClearLineBuffer (Console, 0);
    }
  Console->CursorInfo.bVisible = TRUE;
  Console->CursorInfo.dwSize = 5;
  Console->Mode = ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
  Console->CurrentX = 0;
  Console->CurrentY = 0;
  return STATUS_SUCCESS;
}

VOID STDCALL CsrDeleteScreenBuffer( PCSRSS_SCREEN_BUFFER Buffer )
{
  RtlFreeHeap( CsrssApiHeap, 0, Buffer->Buffer );
  RtlFreeHeap( CsrssApiHeap, 0, Buffer );
}

NTSTATUS STDCALL CsrInitConsole(PCSRSS_CONSOLE Console)
{
  NTSTATUS Status;
  OBJECT_ATTRIBUTES ObjectAttributes;

  Console->Title.MaximumLength = Console->Title.Length = 0;
  Console->Title.Buffer = 0;
  
  RtlCreateUnicodeString( &Console->Title, L"Command Prompt" );
  
  Console->Header.ReferenceCount = 0;
  Console->WaitingChars = 0;
  Console->WaitingLines = 0;
  Console->EchoCount = 0;
  Console->Header.Type = CSRSS_CONSOLE_MAGIC;
  Console->Mode = ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT;
  Console->EarlyReturn = FALSE;
  InitializeListHead(&Console->InputEvents);

  InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_INHERIT, NULL, NULL);

  Status = NtCreateEvent( &Console->ActiveEvent, STANDARD_RIGHTS_ALL, &ObjectAttributes, FALSE, FALSE );
  if( !NT_SUCCESS( Status ) )
    {
      return Status;
    }
  Console->ActiveBuffer = RtlAllocateHeap( CsrssApiHeap, 0, sizeof( CSRSS_SCREEN_BUFFER ) );
  if( !Console->ActiveBuffer )
     {
	NtClose( Console->ActiveEvent );
	return STATUS_INSUFFICIENT_RESOURCES;
     }
  Status = CsrInitConsoleScreenBuffer( Console->ActiveBuffer );
  if( !NT_SUCCESS( Status ) )
     {
	NtClose( Console->ActiveEvent );
	RtlFreeHeap( CsrssApiHeap, 0, Console->ActiveBuffer );
	return Status;
     }
  /* Create the GDI Window to be used in windowed mode */
  /* FIXME: create window; now write NULL */
  Console->hWindow = (HWND) NULL;
  /* add a reference count because the buffer is tied to the console */
  Console->ActiveBuffer->Header.ReferenceCount++;
  /* make console active, and insert into console list */
  LOCK;
  if( ActiveConsole )
     {
	Console->Prev = ActiveConsole;
	Console->Next = ActiveConsole->Next;
	ActiveConsole->Next->Prev = Console;
	ActiveConsole->Next = Console;
     }
  else {
     Console->Prev = Console;
     Console->Next = Console;
  }
  ActiveConsole = Console;
  /* copy buffer contents to screen */
  CsrDrawConsole( Console->ActiveBuffer );
  UNLOCK;
  return STATUS_SUCCESS;
}

/***************************************************************
 *  CsrDrawConsole blasts the console buffer onto the screen   *
 *  must be called while holding the active console lock       *
 **************************************************************/
VOID STDCALL CsrDrawConsole( PCSRSS_SCREEN_BUFFER Buff )
{
   SMALL_RECT Region;

   CsrpInitRect(
     Region,
     0,
     0,
     PhysicalConsoleSize.Y - 1,
     PhysicalConsoleSize.X - 1);

   CsrpDrawRegion(Buff, Region);
}


VOID STDCALL CsrDeleteConsole( PCSRSS_CONSOLE Console )
{
   ConsoleInput *Event;
   DPRINT( "CsrDeleteConsole\n" );
   LOCK;
   /* Drain input event queue */
   while( Console->InputEvents.Flink != &Console->InputEvents )
      {
	 Event = (ConsoleInput *)Console->InputEvents.Flink;
	 Console->InputEvents.Flink = Console->InputEvents.Flink->Flink;
	 Console->InputEvents.Flink->Flink->Blink = &Console->InputEvents;
	 RtlFreeHeap( CsrssApiHeap, 0, Event );
      }
   /* Switch to next console */
   if( ActiveConsole == Console )
      {
	 if( Console->Next != Console )
	    {
	       ActiveConsole = Console->Next;
	    }
	 else ActiveConsole = 0;
      }
   if (Console->Next != Console)
   {
      Console->Prev->Next = Console->Next;
      Console->Next->Prev = Console->Prev;
   }
   
   if( ActiveConsole )
     CsrDrawConsole( ActiveConsole->ActiveBuffer );
   if( !--Console->ActiveBuffer->Header.ReferenceCount )
     CsrDeleteScreenBuffer( Console->ActiveBuffer );
   UNLOCK;
   NtClose( Console->ActiveEvent );
   RtlFreeUnicodeString( &Console->Title );
   RtlFreeHeap( CsrssApiHeap, 0, Console );
}

VOID STDCALL CsrInitConsoleSupport(VOID)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING DeviceName;
   NTSTATUS Status;
   IO_STATUS_BLOCK Iosb;
   CONSOLE_SCREEN_BUFFER_INFO ScrInfo;
   
   DPRINT("CSR: CsrInitConsoleSupport()\n");
   
   RtlInitUnicodeStringFromLiteral(&DeviceName, L"\\??\\BlueScreen");
   InitializeObjectAttributes(&ObjectAttributes,
			      &DeviceName,
			      0,
			      NULL,
			      NULL);
   Status = NtOpenFile(&ConsoleDeviceHandle,
		       FILE_ALL_ACCESS,
		       &ObjectAttributes,
		       &Iosb,
		       0,
		       FILE_SYNCHRONOUS_IO_ALERT);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("CSR: Failed to open console. Expect problems.\n");
     }

   RtlInitUnicodeStringFromLiteral(&DeviceName, L"\\??\\Keyboard");
   InitializeObjectAttributes(&ObjectAttributes,
			      &DeviceName,
			      0,
			      NULL,
			      NULL);
   Status = NtOpenFile(&KeyboardDeviceHandle,
		       FILE_ALL_ACCESS,
		       &ObjectAttributes,
		       &Iosb,
		       0,
		       0);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("CSR: Failed to open keyboard. Expect problems.\n");
     }
   
   ActiveConsole = 0;
   RtlInitializeCriticalSection( &ActiveConsoleLock );
   Status = NtDeviceIoControlFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb, IOCTL_CONSOLE_GET_SCREEN_BUFFER_INFO, 0, 0, &ScrInfo, sizeof( ScrInfo ) );
   if( !NT_SUCCESS( Status ) )
     {
       DbgPrint( "CSR: Failed to get console info, expect trouble\n" );
       return;
     }
   PhysicalConsoleSize = ScrInfo.dwSize;
}

VOID Console_Api( DWORD RefreshEvent )
{
  /* keep reading events from the keyboard and stuffing them into the current
     console's input queue */
  ConsoleInput *KeyEventRecord;
  ConsoleInput *TempInput;
  IO_STATUS_BLOCK Iosb;
  NTSTATUS Status;
  HANDLE Events[2];     // 0 = keyboard, 1 = refresh
  int c;
  int updown;
  PCSRSS_CONSOLE SwapConsole = 0; // console we are thinking about swapping with

  Events[0] = 0;
  Status = NtCreateEvent( &Events[0], STANDARD_RIGHTS_ALL, NULL, FALSE, FALSE );
  if( !NT_SUCCESS( Status ) )
    {
      DbgPrint( "CSR: NtCreateEvent failed: %x\n", Status );
      NtTerminateProcess( NtCurrentProcess(), Status );
    }
  Events[1] = (HANDLE)RefreshEvent;
  while( 1 )
    {
      KeyEventRecord = RtlAllocateHeap(CsrssApiHeap, 
				       0,
				       sizeof(ConsoleInput));
       if ( KeyEventRecord == 0 )
	{
	  DbgPrint( "CSR: Memory allocation failure!" );
	  continue;
	}
      KeyEventRecord->InputEvent.EventType = KEY_EVENT;
      if( !KeyReadInhibit ) {
        Status = NtReadFile( KeyboardDeviceHandle, Events[0], NULL, NULL, &Iosb,
                             &KeyEventRecord->InputEvent.Event.KeyEvent, sizeof( KEY_EVENT_RECORD ), NULL, 0 );
      } else {
	Status = STATUS_PENDING;
      }
      if( !NT_SUCCESS( Status ) )
	{
	  DbgPrint( "CSR: ReadFile on keyboard device failed\n" );
	  RtlFreeHeap( CsrssApiHeap, 0, KeyEventRecord );
	  continue;
	}
      if( Status == STATUS_PENDING )
	{
	  while( 1 )
	    {
	      Status = NtWaitForMultipleObjects( 2, Events, WaitAny, FALSE, NULL );
	      if( Status == STATUS_WAIT_0 + 1 )
		{
		  LOCK;
		  CsrDrawConsole( ActiveConsole->ActiveBuffer );
		  UNLOCK;
		  continue;
		}
	      else if( Status != STATUS_WAIT_0 )
		{
		  DbgPrint( "CSR: NtWaitForMultipleObjects failed: %x, exiting\n", Status );
		  NtTerminateProcess( NtCurrentProcess(), Status );
		}
	      else break;
	    }
	}
      if( KeyEventRecord->InputEvent.Event.KeyEvent.dwControlKeyState &
        ( RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED )&&
          KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualKeyCode == VK_TAB )
	 if( KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown == TRUE )
	    {
	      ANSI_STRING Title;
	      void * Buffer;
	      COORD *pos;
	      
	       /* alt-tab, swap consoles */
	       // move SwapConsole to next console, and print its title
	      LOCK;
	      if( !SwapConsole )
		SwapConsole = ActiveConsole;
	      
	      if( KeyEventRecord->InputEvent.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED )
		SwapConsole = SwapConsole->Prev;
	      else SwapConsole = SwapConsole->Next;
	      Title.MaximumLength = RtlUnicodeStringToAnsiSize( &SwapConsole->Title );
	      Title.Length = 0;
	      Buffer = RtlAllocateHeap( CsrssApiHeap,
					0,
					sizeof( COORD ) + Title.MaximumLength);
	      pos = (COORD *)Buffer;
	      Title.Buffer = Buffer + sizeof( COORD );

              RtlUnicodeStringToAnsiString(&Title, &SwapConsole->Title, FALSE);
	      pos->Y = PhysicalConsoleSize.Y / 2;
	      pos->X = ( PhysicalConsoleSize.X - Title.Length ) / 2;
	      // redraw the console to clear off old title
	      CsrDrawConsole( ActiveConsole->ActiveBuffer );
	      Status = NtDeviceIoControlFile( ConsoleDeviceHandle,
					      NULL,
					      NULL,
					      NULL,
					      &Iosb,
					      IOCTL_CONSOLE_WRITE_OUTPUT_CHARACTER,
					      Buffer,
					      sizeof (COORD) + Title.Length,
					      NULL,
					      0);
	      if( !NT_SUCCESS( Status ) )
		{
		  DPRINT1( "Error writing to console\n" );
		}
	      RtlFreeHeap( CsrssApiHeap, 0, Buffer );
	      
	      UNLOCK;
	      RtlFreeHeap( CsrssApiHeap, 0, KeyEventRecord );
	      continue;
	    }
	 else {
	    RtlFreeHeap( CsrssApiHeap, 0, KeyEventRecord );
	    continue;
	 }
      else if( SwapConsole &&
	       KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualKeyCode == VK_MENU &&
	       KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown == FALSE )
	{
	  // alt key released, swap consoles
	  PCSRSS_CONSOLE tmp;

	  LOCK;
	  if( SwapConsole != ActiveConsole )
	    {
	      // first remove swapconsole from the list
	      SwapConsole->Prev->Next = SwapConsole->Next;
	      SwapConsole->Next->Prev = SwapConsole->Prev;
	      // now insert before activeconsole
	      SwapConsole->Next = ActiveConsole;
	      SwapConsole->Prev = ActiveConsole->Prev;
	      ActiveConsole->Prev->Next = SwapConsole;
	      ActiveConsole->Prev = SwapConsole;
	    }
	  ActiveConsole = SwapConsole;
	  SwapConsole = 0;
	  CsrDrawConsole( ActiveConsole->ActiveBuffer );

	  UNLOCK;
	}
      if( KeyEventRecord->InputEvent.Event.KeyEvent.dwControlKeyState &
        ( RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED ) &&
        ( KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualKeyCode == VK_UP ||
          KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualKeyCode == VK_DOWN) )
	 {
	    if( KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown == TRUE )
	       {
		  /* scroll up or down */
		  LOCK;
		  if( ActiveConsole == 0 )
		     {
			DbgPrint( "CSR: No Active Console!\n" );
	    		UNLOCK;
			RtlFreeHeap( CsrssApiHeap, 0, KeyEventRecord );
			continue;
		     }
		  if( KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualKeyCode == VK_UP )
		     {
			/* only scroll up if there is room to scroll up into */
			if( ActiveConsole->ActiveBuffer->ShowY != ((ActiveConsole->ActiveBuffer->CurrentY + 1) %
        ActiveConsole->ActiveBuffer->MaxY) )
			   ActiveConsole->ActiveBuffer->ShowY = (ActiveConsole->ActiveBuffer->ShowY +
         ActiveConsole->ActiveBuffer->MaxY - 1) % ActiveConsole->ActiveBuffer->MaxY;
		     }
		  else if( ActiveConsole->ActiveBuffer->ShowY != ActiveConsole->ActiveBuffer->CurrentY )
		     /* only scroll down if there is room to scroll down into */
		     if( ActiveConsole->ActiveBuffer->ShowY % ActiveConsole->ActiveBuffer->MaxY != 
           ActiveConsole->ActiveBuffer->CurrentY )

			if( ((ActiveConsole->ActiveBuffer->CurrentY + 1) % ActiveConsole->ActiveBuffer->MaxY) != 
        (ActiveConsole->ActiveBuffer->ShowY + PhysicalConsoleSize.Y) % ActiveConsole->ActiveBuffer->MaxY )
			   ActiveConsole->ActiveBuffer->ShowY = (ActiveConsole->ActiveBuffer->ShowY + 1) %
         ActiveConsole->ActiveBuffer->MaxY;
		  CsrDrawConsole( ActiveConsole->ActiveBuffer );
		  UNLOCK;
	       }
	    RtlFreeHeap( CsrssApiHeap, 0, KeyEventRecord );
	    continue;
	}
      LOCK;
      if( ActiveConsole == 0 )
	 {
	    DbgPrint( "CSR: No Active Console!\n" );
	    UNLOCK;
	    RtlFreeHeap( CsrssApiHeap, 0, KeyEventRecord );
	    continue;
	 }
      // process special keys if enabled
      if( ActiveConsole->Mode & (ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT) )
	  switch( KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar )
	    {
	    case '\r':
	      // add a \n to the queue as well
	      // first add the \r
	      updown = KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown;
	      KeyEventRecord->Echoed = FALSE;
	      KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar = '\r';
        InsertTailList(&ActiveConsole->InputEvents, &KeyEventRecord->ListEntry);
	      ActiveConsole->WaitingChars++;
	      KeyEventRecord = RtlAllocateHeap( CsrssApiHeap, 0, sizeof( ConsoleInput ) );
	      if( !KeyEventRecord )
		{
		  DbgPrint( "CSR: Failed to allocate KeyEventRecord\n" );
		  UNLOCK;
		  continue;
		}
	      KeyEventRecord->InputEvent.EventType = KEY_EVENT;
	      KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown = updown;
	      KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualKeyCode = 0;
	      KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualScanCode = 0;
	      KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar = '\n';
	    }
      // add event to the queue
      InsertTailList(&ActiveConsole->InputEvents, &KeyEventRecord->ListEntry);
      // if line input mode is enabled, only wake the client on enter key down
      if( !(ActiveConsole->Mode & ENABLE_LINE_INPUT ) ||
	  ActiveConsole->EarlyReturn ||
	  ( KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar == '\n' &&
	    KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown == TRUE ) )
	{
	  NtSetEvent( ActiveConsole->ActiveEvent, 0 );
	  if( KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar == '\n' )
	     ActiveConsole->WaitingLines++;
	}
      KeyEventRecord->Echoed = FALSE;
      if( ActiveConsole->Mode & (ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT) &&
	  KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar == '\b' &&
	  KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown )
	 {
	    // walk the input queue looking for a char to backspace
	    for( TempInput = (ConsoleInput *)ActiveConsole->InputEvents.Blink;
		  TempInput != (ConsoleInput *)&ActiveConsole->InputEvents &&
		  (TempInput->InputEvent.EventType != KEY_EVENT ||
		  TempInput->InputEvent.Event.KeyEvent.bKeyDown == FALSE ||
		  TempInput->InputEvent.Event.KeyEvent.uChar.AsciiChar == '\b' );
		  TempInput = (ConsoleInput *)TempInput->ListEntry.Blink );
	    // if we found one, delete it, otherwise, wake the client
	    if( TempInput != (ConsoleInput *)&ActiveConsole->InputEvents )
	       {
		  // delete previous key in queue, maybe echo backspace to screen, and do not place backspace on queue
      RemoveEntryList(&TempInput->ListEntry);
		  if( TempInput->Echoed )
		     CsrpWriteConsole( ActiveConsole->ActiveBuffer, &KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar, 1, TRUE );
		  RtlFreeHeap( CsrssApiHeap, 0, TempInput );
      RemoveEntryList(&KeyEventRecord->ListEntry);
		  RtlFreeHeap( CsrssApiHeap, 0, KeyEventRecord );
		  ActiveConsole->WaitingChars -= 2;
	       }
	    else NtSetEvent( ActiveConsole->ActiveEvent, 0 );
   }
      else {
	 // echo chars if we are supposed to and client is waiting for some
	 if( ( ActiveConsole->Mode & ENABLE_ECHO_INPUT ) && ActiveConsole->EchoCount &&
	     KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar &&
	     KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown == TRUE &&
	     KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar != '\r' )
	    {
	       // mark the char as already echoed
	       CsrpWriteConsole( ActiveConsole->ActiveBuffer, &KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar, 1, TRUE );
	       ActiveConsole->EchoCount--;
	       KeyEventRecord->Echoed = TRUE;
	    }
      }

	/* After all the keys processed */
	if (((KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualKeyCode == VK_PAUSE) || 
		(KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualKeyCode == 'C')) &&
		(KeyEventRecord->InputEvent.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)))
	{
		DPRINT1("Console_Api Ctrl-C\n");
		CsrConsoleCtrlEvent((DWORD)CTRL_C_EVENT);
	}
      
      ActiveConsole->WaitingChars++;
      if( !(ActiveConsole->Mode & ENABLE_LINE_INPUT) )
	NtSetEvent( ActiveConsole->ActiveEvent, 0 );
      UNLOCK;
    }
}

CSR_API(CsrGetScreenBufferInfo)
{
   NTSTATUS Status;
   PCSRSS_SCREEN_BUFFER Buff;
   PCONSOLE_SCREEN_BUFFER_INFO pInfo;
   IO_STATUS_BLOCK Iosb;
   
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
     sizeof(LPC_MESSAGE);

   LOCK;
   if( !NT_SUCCESS( CsrGetObject( ProcessData, Request->Data.ScreenBufferInfoRequest.ConsoleHandle,
     (Object_t **)&Buff ) ) || Buff->Header.Type != CSRSS_SCREEN_BUFFER_MAGIC )
      {
	 UNLOCK;
	 return Reply->Status = STATUS_INVALID_HANDLE;
      }
   pInfo = &Reply->Data.ScreenBufferInfoReply.Info;
   if( Buff == ActiveConsole->ActiveBuffer )
     {
	Status = NtDeviceIoControlFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb,
    IOCTL_CONSOLE_GET_SCREEN_BUFFER_INFO, 0, 0, pInfo, sizeof( *pInfo ) );
	if( !NT_SUCCESS( Status ) )
	   DbgPrint( "CSR: Failed to get console info, expect trouble\n" );
	Reply->Status = Status;
     }
   else {
      pInfo->dwSize.X = PhysicalConsoleSize.X;
      pInfo->dwSize.Y = PhysicalConsoleSize.Y;
      pInfo->dwCursorPosition.X = Buff->CurrentX - Buff->ShowX;
      pInfo->dwCursorPosition.Y = (Buff->CurrentY + Buff->MaxY - Buff->ShowY) % Buff->MaxY;
      pInfo->wAttributes = Buff->DefaultAttrib;
      pInfo->srWindow.Left = 0;
      pInfo->srWindow.Right = PhysicalConsoleSize.X - 1;
      pInfo->srWindow.Top = 0;
      pInfo->srWindow.Bottom = PhysicalConsoleSize.Y - 1;
      Reply->Status = STATUS_SUCCESS;
   }
   UNLOCK;
   return Reply->Status;
}

CSR_API(CsrSetCursor)
{
   NTSTATUS Status;
   PCSRSS_SCREEN_BUFFER Buff;
   CONSOLE_SCREEN_BUFFER_INFO Info;
   IO_STATUS_BLOCK Iosb;
   
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
     sizeof(LPC_MESSAGE);

   LOCK;
   if( !NT_SUCCESS( CsrGetObject( ProcessData, Request->Data.SetCursorRequest.ConsoleHandle,
     (Object_t **)&Buff ) ) || Buff->Header.Type != CSRSS_SCREEN_BUFFER_MAGIC )
      {
	 UNLOCK;
	 return Reply->Status = STATUS_INVALID_HANDLE;
      }
   Info.dwCursorPosition = Request->Data.SetCursorRequest.Position;
   Info.wAttributes = Buff->DefaultAttrib;
   if( Buff == ActiveConsole->ActiveBuffer )
      {
	 Status = NtDeviceIoControlFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb,
     IOCTL_CONSOLE_SET_SCREEN_BUFFER_INFO, &Info, sizeof( Info ), 0, 0 );
	if( !NT_SUCCESS( Status ) )
	   DbgPrint( "CSR: Failed to set console info, expect trouble\n" );
      }

   Buff->CurrentX = Info.dwCursorPosition.X + Buff->ShowX;
   Buff->CurrentY = (Info.dwCursorPosition.Y + Buff->ShowY) % Buff->MaxY;
   UNLOCK;
   return Reply->Status = Status;
}

CSR_API(CsrWriteConsoleOutputChar)
{
   PBYTE String = Request->Data.WriteConsoleOutputCharRequest.String;
   PBYTE Buffer;
   PCSRSS_SCREEN_BUFFER Buff;
   DWORD X, Y, Length;
   NTSTATUS Status;
   IO_STATUS_BLOCK Iosb;

   DPRINT("CsrWriteConsoleOutputChar\n");
   
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
     sizeof(LPC_MESSAGE);
   LOCK;
   if( !NT_SUCCESS( CsrGetObject( ProcessData, Request->Data.WriteConsoleOutputCharRequest.ConsoleHandle, (Object_t **)&Buff ) ) || Buff->Header.Type != CSRSS_SCREEN_BUFFER_MAGIC )
      {
	 UNLOCK;
	 return Reply->Status = STATUS_INVALID_HANDLE;
      }


   X = Request->Data.WriteConsoleOutputCharRequest.Coord.X + Buff->ShowX;
   Y = (Request->Data.WriteConsoleOutputCharRequest.Coord.Y + Buff->ShowY) % Buff->MaxY;
   Length = Request->Data.WriteConsoleOutputCharRequest.Length;
   Buffer = &Buff->Buffer[2 * (Y * Buff->MaxX + X)];
   while(Length--)
   {
      *Buffer = *String++;
      Buffer += 2;
      if (++X == Buff->MaxX)
      {
         if (++Y == Buff->MaxY)
	 {
	    Y = 0;
	    Buffer = Buff->Buffer;
	 }
	 X = 0;
      }
   }
   if( ActiveConsole->ActiveBuffer == Buff )
     {
       Status = NtDeviceIoControlFile( ConsoleDeviceHandle,
				       NULL,
				       NULL,
				       NULL,
				       &Iosb,
				       IOCTL_CONSOLE_WRITE_OUTPUT_CHARACTER,
				       &Request->Data.WriteConsoleOutputCharRequest.Coord,
				       sizeof (COORD) + Request->Data.WriteConsoleOutputCharRequest.Length,
				       NULL,
				       0);
       if( !NT_SUCCESS( Status ) )
	 DPRINT1( "Failed to write output chars: %x\n", Status );
     }
   Reply->Data.WriteConsoleOutputCharReply.EndCoord.X = X - Buff->ShowX;
   Reply->Data.WriteConsoleOutputCharReply.EndCoord.Y = (Y + Buff->MaxY - Buff->ShowY) % Buff->MaxY;
   UNLOCK;
   return Reply->Status = STATUS_SUCCESS;
}

CSR_API(CsrFillOutputChar)
{
   PCSRSS_SCREEN_BUFFER Buff;
   DWORD X, Y, Length;
   BYTE Char;
   OUTPUT_CHARACTER Character;
   PBYTE Buffer;
   NTSTATUS Status;
   IO_STATUS_BLOCK Iosb;

   DPRINT("CsrFillOutputChar\n");
   
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
     sizeof(LPC_MESSAGE);

   LOCK;
   if( !NT_SUCCESS( CsrGetObject( ProcessData, Request->Data.FillOutputRequest.ConsoleHandle, (Object_t **)&Buff ) ) || Buff->Header.Type != CSRSS_SCREEN_BUFFER_MAGIC )
      {
	 UNLOCK;
	 return Reply->Status = STATUS_INVALID_HANDLE;
      }
   X = Request->Data.FillOutputRequest.Position.X + Buff->ShowX;
   Y = (Request->Data.FillOutputRequest.Position.Y + Buff->ShowY) % Buff->MaxY;
   Buffer = &Buff->Buffer[2 * (Y * Buff->MaxX + X)];
   Char = Request->Data.FillOutputRequest.Char;
   Length = Request->Data.FillOutputRequest.Length;
   while (Length--)
   {
      *Buffer = Char;
      Buffer += 2;
      if( ++X == Buff->MaxX )
      {
         if( ++Y == Buff->MaxY )
	 {
	    Y = 0;
	    Buffer = Buff->Buffer;
	 }
	 X = 0;
      }
   }
   if( Buff == ActiveConsole->ActiveBuffer )
   {
      Character.dwCoord = Request->Data.FillOutputRequest.Position;
      Character.cCharacter = Char;
      Character.nLength = Request->Data.FillOutputRequest.Length;
      Status = NtDeviceIoControlFile( ConsoleDeviceHandle, 
	                              NULL, 
				      NULL, 
				      NULL, 
				      &Iosb,
	                              IOCTL_CONSOLE_FILL_OUTPUT_CHARACTER,
				      &Character,
				      sizeof(Character),
				      NULL,
				      0);
      if (!NT_SUCCESS(Status))
	  DPRINT1( "Failed to write output characters to console\n" );
   }
   UNLOCK;
   return Reply->Status;
}

CSR_API(CsrReadInputEvent)
{
   PLIST_ENTRY CurrentEntry;
   PCSRSS_CONSOLE Console;
   NTSTATUS Status;
   ConsoleInput *Input;
   
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
      sizeof(LPC_MESSAGE);
   Reply->Data.ReadInputReply.Event = ProcessData->ConsoleEvent;
   
   LOCK;
   Status = CsrGetObject( ProcessData, Request->Data.ReadInputRequest.ConsoleHandle, (Object_t **)&Console );
   if( !NT_SUCCESS( Status ) || (Status = Console->Header.Type == CSRSS_CONSOLE_MAGIC ? 0 : STATUS_INVALID_HANDLE))
      {
         Reply->Status = Status;
         UNLOCK;
         return Status;
      }

   // only get input if there is any
   if( Console->InputEvents.Flink != &Console->InputEvents )
     {
       CurrentEntry = RemoveHeadList(&Console->InputEvents);
       Input = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);
       Reply->Data.ReadInputReply.Input = Input->InputEvent;

       if( Input->InputEvent.EventType == KEY_EVENT )
	 {
	   if( Console->Mode & ENABLE_LINE_INPUT &&
	       Input->InputEvent.Event.KeyEvent.bKeyDown == FALSE &&
	       Input->InputEvent.Event.KeyEvent.uChar.AsciiChar == '\n' )
	     Console->WaitingLines--;
	   Console->WaitingChars--;
	 }
       RtlFreeHeap( CsrssApiHeap, 0, Input );

       if (Console->InputEvents.Flink != &Console->InputEvents &&
	   Reply->Data.ReadInputReply.Input.EventType == KEY_EVENT &&
	   Reply->Data.ReadInputReply.Input.Event.KeyEvent.uChar.AsciiChar == '\r')
       {
          Input = CONTAINING_RECORD(Console->InputEvents.Flink, ConsoleInput, ListEntry);
	  if (Input->InputEvent.EventType == KEY_EVENT &&
              Input->InputEvent.Event.KeyEvent.uChar.AsciiChar == '\n' && 
              ((Input->InputEvent.Event.KeyEvent.bKeyDown && Reply->Data.ReadInputReply.Input.Event.KeyEvent.bKeyDown) ||
              (Input->InputEvent.Event.KeyEvent.bKeyDown==FALSE && Reply->Data.ReadInputReply.Input.Event.KeyEvent.bKeyDown==FALSE)))
	  {
	    if(Console->Mode & ENABLE_LINE_INPUT &&
	       Input->InputEvent.Event.KeyEvent.bKeyDown == FALSE &&
	       Input->InputEvent.Event.KeyEvent.uChar.AsciiChar == '\n' )
	     Console->WaitingLines--;
	    Console->WaitingChars--;
	    RemoveHeadList(&Console->InputEvents);
            RtlFreeHeap( CsrssApiHeap, 0, Input );
	  }
       }

       Reply->Data.ReadInputReply.MoreEvents = (Console->InputEvents.Flink != &Console->InputEvents) ? TRUE : FALSE;
       Status = STATUS_SUCCESS;
       Console->EarlyReturn = FALSE; // clear early return
     }
   else {
      Status = STATUS_PENDING;
      Console->EarlyReturn = TRUE;  // mark for early return
   }
   UNLOCK;
   return Reply->Status = Status;
}

CSR_API(CsrWriteConsoleOutputAttrib)
{
   PCSRSS_SCREEN_BUFFER Buff;
   PUCHAR Buffer, Attribute;
   NTSTATUS Status;
   int X, Y, Length;
   IO_STATUS_BLOCK Iosb;

   DPRINT("CsrWriteConsoleOutputAttrib\n");
   
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
      sizeof(LPC_MESSAGE);
   LOCK;
   Status = CsrGetObject( ProcessData, Request->Data.WriteConsoleOutputAttribRequest.ConsoleHandle, (Object_t **)&Buff );
   if( !NT_SUCCESS( Status ) || (Status = Buff->Header.Type == CSRSS_SCREEN_BUFFER_MAGIC ? 0 : STATUS_INVALID_HANDLE ))
      {
	 Reply->Status = Status;
	 UNLOCK;
	 return Status;
      }
   X = Request->Data.WriteConsoleOutputAttribRequest.Coord.X + Buff->ShowX;
   Y = (Request->Data.WriteConsoleOutputAttribRequest.Coord.Y + Buff->ShowY) % Buff->MaxY;
   Length = Request->Data.WriteConsoleOutputAttribRequest.Length;
   Buffer = &Buff->Buffer[2 * (Y * Buff->MaxX + X) + 1];
   Attribute = Request->Data.WriteConsoleOutputAttribRequest.String;
   while (Length--)
   {
      *Buffer = *Attribute++;
      Buffer += 2;
      if( ++X == Buff->MaxX )
      {
         X = 0;
	 if( ++Y == Buff->MaxY )
	 {
	    Y = 0;
	    Buffer = Buff->Buffer + 1;
	 }
      }
   }
   if( Buff == ActiveConsole->ActiveBuffer )
      {
	Status = NtDeviceIoControlFile( ConsoleDeviceHandle,
					NULL,
					NULL,
					NULL,
					&Iosb,
					IOCTL_CONSOLE_WRITE_OUTPUT_ATTRIBUTE,
					&Request->Data.WriteConsoleOutputAttribRequest.Coord,
					Request->Data.WriteConsoleOutputAttribRequest.Length +
					sizeof (COORD),
					NULL,
					0 );
	if( !NT_SUCCESS( Status ) )
	  DPRINT1( "Failed to write output attributes to console\n" );
      }
   Reply->Data.WriteConsoleOutputAttribReply.EndCoord.X = Buff->CurrentX - Buff->ShowX;
   Reply->Data.WriteConsoleOutputAttribReply.EndCoord.Y = ( Buff->CurrentY + Buff->MaxY - Buff->ShowY ) % Buff->MaxY;
   UNLOCK;
   return Reply->Status = STATUS_SUCCESS;
}

CSR_API(CsrFillOutputAttrib)
{
   OUTPUT_ATTRIBUTE Attribute;
   PCSRSS_SCREEN_BUFFER Buff;
   PCHAR Buffer;
   NTSTATUS Status;
   int X, Y, Length;
   IO_STATUS_BLOCK Iosb;
   UCHAR Attr;

   DPRINT("CsrFillOutputAttrib\n");
   
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
      sizeof(LPC_MESSAGE);
   LOCK;
   Status = CsrGetObject( ProcessData, Request->Data.FillOutputAttribRequest.ConsoleHandle, (Object_t **)&Buff );
   if( !NT_SUCCESS( Status ) || (Status = Buff->Header.Type == CSRSS_SCREEN_BUFFER_MAGIC ? 0 : STATUS_INVALID_HANDLE ))
      {
	 Reply->Status = Status;
	 UNLOCK;
	 return Status;
      }
   X = Request->Data.FillOutputAttribRequest.Coord.X + Buff->ShowX;
   Y = (Request->Data.FillOutputAttribRequest.Coord.Y + Buff->ShowY) % Buff->MaxY;
   Length = Request->Data.FillOutputAttribRequest.Length;
   Attr = Request->Data.FillOutputAttribRequest.Attribute;
   Buffer = &Buff->Buffer[(Y * Buff->MaxX * 2) + (X * 2) + 1];
   while(Length--)
   {
      *Buffer = Attr;
      Buffer += 2;
      if( ++X == Buff->MaxX )
      {
         X = 0;
	 if( ++Y == Buff->MaxY )
	 {
	    Y = 0;
	    Buffer = Buff->Buffer + 1;
	 }
      }
   }
   if( Buff == ActiveConsole->ActiveBuffer )
   {
      Attribute.wAttribute = Attr;
      Attribute.nLength = Request->Data.FillOutputAttribRequest.Length;
      Attribute.dwCoord = Request->Data.FillOutputAttribRequest.Coord;
      Status = NtDeviceIoControlFile( ConsoleDeviceHandle, 
	                              NULL, 
				      NULL, 
				      NULL, 
				      &Iosb,
				      IOCTL_CONSOLE_FILL_OUTPUT_ATTRIBUTE, 
				      &Attribute,
				      sizeof(OUTPUT_ATTRIBUTE), 
				      NULL, 
				      0);
      if( !NT_SUCCESS( Status ) )
         DPRINT1( "Failed to fill output attributes to console\n" );
   }
   UNLOCK;
   return Reply->Status = STATUS_SUCCESS;
}


CSR_API(CsrGetCursorInfo)
{
   PCSRSS_SCREEN_BUFFER Buff;
   NTSTATUS Status;
   
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
      sizeof(LPC_MESSAGE);
   LOCK;
   Status = CsrGetObject( ProcessData, Request->Data.GetCursorInfoRequest.ConsoleHandle, (Object_t **)&Buff );
   if( !NT_SUCCESS( Status ) || (Status = Buff->Header.Type == CSRSS_SCREEN_BUFFER_MAGIC ? 0 : STATUS_INVALID_HANDLE ))
      {
	 Reply->Status = Status;
	 UNLOCK;
	 return Status;
      }
   Reply->Data.GetCursorInfoReply.Info = Buff->CursorInfo;
   UNLOCK;
   return Reply->Status = STATUS_SUCCESS;
}

CSR_API(CsrSetCursorInfo)
{
   PCSRSS_SCREEN_BUFFER Buff;
   NTSTATUS Status;
   IO_STATUS_BLOCK Iosb;
   
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
      sizeof(LPC_MESSAGE);
   LOCK;
   Status = CsrGetObject( ProcessData,
     Request->Data.SetCursorInfoRequest.ConsoleHandle, (Object_t **)&Buff );

   if( !NT_SUCCESS( Status ) || (Status = Buff->Header.Type == CSRSS_SCREEN_BUFFER_MAGIC ? 0 : STATUS_INVALID_HANDLE ))
      {
	 Reply->Status = Status;
	 UNLOCK;
	 return Status;
      }
   Buff->CursorInfo = Request->Data.SetCursorInfoRequest.Info;
   if( Buff == ActiveConsole->ActiveBuffer )
      {
	 Status = NtDeviceIoControlFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb, IOCTL_CONSOLE_SET_CURSOR_INFO, &Buff->CursorInfo, sizeof( Buff->CursorInfo ), 0, 0 );
	 if( !NT_SUCCESS( Status ) )
	    {
	       DbgPrint( "CSR: Failed to set cursor info\n" );
	       return Reply->Status = Status;
	    }
      }
   UNLOCK;
   return Reply->Status = STATUS_SUCCESS;
}

CSR_API(CsrSetTextAttrib)
{
   NTSTATUS Status;
   CONSOLE_SCREEN_BUFFER_INFO ScrInfo;
   IO_STATUS_BLOCK Iosb;
   PCSRSS_SCREEN_BUFFER Buff;

   DPRINT("CsrSetTextAttrib\n");
   
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
      sizeof(LPC_MESSAGE);
   LOCK;
   Status = CsrGetObject( ProcessData, Request->Data.SetAttribRequest.ConsoleHandle, (Object_t **)&Buff );
   if( !NT_SUCCESS( Status ) || (Status = Buff->Header.Type == CSRSS_SCREEN_BUFFER_MAGIC ? 0 : STATUS_INVALID_HANDLE ))
      {
	 Reply->Status = Status;
	 UNLOCK;
	 return Status;
      }
   Buff->DefaultAttrib = Request->Data.SetAttribRequest.Attrib;
   if( Buff == ActiveConsole->ActiveBuffer )
      {
	 ScrInfo.wAttributes = Buff->DefaultAttrib;
	 ScrInfo.dwCursorPosition.X = Buff->CurrentX - Buff->ShowX;   
	 ScrInfo.dwCursorPosition.Y = ((Buff->CurrentY + Buff->MaxY) - Buff->ShowY) % Buff->MaxY;
	 Status = NtDeviceIoControlFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb, IOCTL_CONSOLE_SET_SCREEN_BUFFER_INFO, &ScrInfo, sizeof( ScrInfo ), 0, 0 );
	 if( !NT_SUCCESS( Status ) )
	    {
	       DbgPrint( "CSR: Failed to set console info\n" );
	       UNLOCK;
	       return Reply->Status = Status;
	    }
      }
   UNLOCK;
   return Reply->Status = STATUS_SUCCESS;
}

CSR_API(CsrSetConsoleMode)
{
   NTSTATUS Status;
   PCSRSS_CONSOLE Console;
   PCSRSS_SCREEN_BUFFER Buff;

   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);
   LOCK;
   Status = CsrGetObject( ProcessData,
     Request->Data.SetConsoleModeRequest.ConsoleHandle,
     (Object_t **)&Console );
   if( !NT_SUCCESS( Status ) )
      {
	 Reply->Status = Status;
	 UNLOCK;
	 return Status;
      }

   Buff = (PCSRSS_SCREEN_BUFFER)Console;
   if( Console->Header.Type == CSRSS_CONSOLE_MAGIC )
      Console->Mode = Request->Data.SetConsoleModeRequest.Mode & CONSOLE_INPUT_MODE_VALID;
   else if( Console->Header.Type == CSRSS_SCREEN_BUFFER_MAGIC )
      Buff->Mode = Request->Data.SetConsoleModeRequest.Mode & CONSOLE_OUTPUT_MODE_VALID;
   else {
      Reply->Status = STATUS_INVALID_HANDLE;
      UNLOCK;
      return Status;
   }
   UNLOCK;
   Reply->Status = STATUS_SUCCESS;
   return Reply->Status;
}

CSR_API(CsrGetConsoleMode)
{
   NTSTATUS Status;
   PCSRSS_CONSOLE Console;
   PCSRSS_SCREEN_BUFFER Buff;   /* gee, I really wish I could use an anonymous union here */

   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);
   LOCK;
   Status = CsrGetObject( ProcessData,
     Request->Data.GetConsoleModeRequest.ConsoleHandle,
     (Object_t **)&Console );
   if( !NT_SUCCESS( Status ) )
      {
	 Reply->Status = Status;
	 UNLOCK;
	 return Status;
      }
   Reply->Status = STATUS_SUCCESS;
   Buff = (PCSRSS_SCREEN_BUFFER)Console;
   if( Console->Header.Type == CSRSS_CONSOLE_MAGIC )
      Reply->Data.GetConsoleModeReply.ConsoleMode = Console->Mode;
   else if( Buff->Header.Type == CSRSS_SCREEN_BUFFER_MAGIC )
      Reply->Data.GetConsoleModeReply.ConsoleMode = Buff->Mode;
   else Status = STATUS_INVALID_HANDLE;
   UNLOCK;
   return Reply->Status;
}

CSR_API(CsrCreateScreenBuffer)
{
   PCSRSS_SCREEN_BUFFER Buff;
   NTSTATUS Status;
   
   Reply->Header.MessageSize = sizeof( CSRSS_API_REPLY );
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);

   if (ProcessData == NULL)
   {
     return(Reply->Status = STATUS_INVALID_PARAMETER);
   }

   Buff = RtlAllocateHeap( CsrssApiHeap, 0, sizeof( CSRSS_SCREEN_BUFFER ) );
   if( !Buff )
      Reply->Status = STATUS_INSUFFICIENT_RESOURCES;
   LOCK;
   Status = CsrInitConsoleScreenBuffer( Buff );
   if( !NT_SUCCESS( Status ) )
      Reply->Status = Status;
   else {
      Status = CsrInsertObject( ProcessData, &Reply->Data.CreateScreenBufferReply.OutputHandle, &Buff->Header );
      if( !NT_SUCCESS( Status ) )
	 Reply->Status = Status;
      else Reply->Status = STATUS_SUCCESS;
   }
   UNLOCK;
   return Reply->Status;
}

CSR_API(CsrSetScreenBuffer)
{
   NTSTATUS Status;
   PCSRSS_SCREEN_BUFFER Buff;
   
   Reply->Header.MessageSize = sizeof( CSRSS_API_REPLY );
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);
   LOCK;
   Status = CsrGetObject( ProcessData, Request->Data.SetActiveScreenBufferRequest.OutputHandle, (Object_t **)&Buff );
   if( !NT_SUCCESS( Status ) )
      Reply->Status = Status;
   else {
      // drop reference to old buffer, maybe delete
      if( !InterlockedDecrement( &ProcessData->Console->ActiveBuffer->Header.ReferenceCount ) )
	 CsrDeleteScreenBuffer( ProcessData->Console->ActiveBuffer );
      // tie console to new buffer
      ProcessData->Console->ActiveBuffer = Buff;
      // inc ref count on new buffer
      InterlockedIncrement( &Buff->Header.ReferenceCount );
      // if the console is active, redraw it
      if( ActiveConsole == ProcessData->Console )
	 CsrDrawConsole( Buff );
      Reply->Status = STATUS_SUCCESS;
   }
   UNLOCK;
   return Reply->Status;
}

CSR_API(CsrSetTitle)
{
  NTSTATUS Status;
  PCSRSS_CONSOLE Console;
  
  Reply->Header.MessageSize = sizeof( CSRSS_API_REPLY );
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);
  LOCK;
  Status = CsrGetObject( ProcessData, Request->Data.SetTitleRequest.Console, (Object_t **)&Console );
  if( !NT_SUCCESS( Status ) )
    Reply->Status = Status;  
  else {
    // copy title to console
    RtlFreeUnicodeString( &Console->Title );
    RtlCreateUnicodeString( &Console->Title, Request->Data.SetTitleRequest.Title );
    Reply->Status = STATUS_SUCCESS;
  }
  UNLOCK;
  return Reply->Status;
}

CSR_API(CsrGetTitle)
{
	NTSTATUS	Status;
	PCSRSS_CONSOLE	Console;
  
	Reply->Header.MessageSize = sizeof (CSRSS_API_REPLY);
	Reply->Header.DataSize =
		sizeof (CSRSS_API_REPLY)
		- sizeof(LPC_MESSAGE);
	LOCK;
	Status = CsrGetObject (
			ProcessData,
			Request->Data.GetTitleRequest.ConsoleHandle,
			(Object_t **) & Console
			);
   if ( !NT_SUCCESS( Status ) )
   {
      Reply->Status = Status;
   }
	else
	{
		HANDLE ConsoleHandle = Request->Data.GetTitleRequest.ConsoleHandle;
		
		/* Copy title of the console to the user title buffer */
		RtlZeroMemory (
			& Reply->Data.GetTitleReply,
			sizeof (CSRSS_GET_TITLE_REPLY)
			);
		Reply->Data.GetTitleReply.ConsoleHandle = ConsoleHandle;
		Reply->Data.GetTitleReply.Length = Console->Title.Length;
		wcscpy (Reply->Data.GetTitleReply.Title, Console->Title.Buffer);
		Reply->Header.MessageSize += Console->Title.Length;
		Reply->Header.DataSize += Console->Title.Length;
		Reply->Status = STATUS_SUCCESS;
	}
	UNLOCK;
	return Reply->Status;
}

CSR_API(CsrWriteConsoleOutput)
{
   SHORT i, X, Y, SizeX, SizeY;
   PCSRSS_SCREEN_BUFFER Buff;
   SMALL_RECT ScreenBuffer;
   CHAR_INFO* CurCharInfo;
   SMALL_RECT WriteRegion;
   CHAR_INFO* CharInfo;
   COORD BufferCoord;
   COORD BufferSize;
   NTSTATUS Status;
   DWORD Offset;
   DWORD PSize;

   DPRINT("CsrWriteConsoleOutput\n");

   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);
   LOCK;
   Status = CsrGetObject( ProcessData, Request->Data.WriteConsoleOutputRequest.ConsoleHandle, (Object_t **)&Buff );
   if( !NT_SUCCESS( Status ) || (Status = Buff->Header.Type == CSRSS_SCREEN_BUFFER_MAGIC ? STATUS_SUCCESS : STATUS_INVALID_HANDLE ))
      {
	 Reply->Status = Status;
	 UNLOCK;
	 return Status;
      }

   BufferSize = Request->Data.WriteConsoleOutputRequest.BufferSize;
   PSize = BufferSize.X * BufferSize.Y * sizeof(CHAR_INFO);
   BufferCoord = Request->Data.WriteConsoleOutputRequest.BufferCoord;
   CharInfo = Request->Data.WriteConsoleOutputRequest.CharInfo;
   if (((PVOID)CharInfo < ProcessData->CsrSectionViewBase) ||
       (((PVOID)CharInfo + PSize) > 
	(ProcessData->CsrSectionViewBase + ProcessData->CsrSectionViewSize)))
     {
       UNLOCK;
       Reply->Status = STATUS_ACCESS_VIOLATION;
       return(Reply->Status);
     }
   WriteRegion = Request->Data.WriteConsoleOutputRequest.WriteRegion;

   SizeY = RtlMin(BufferSize.Y - BufferCoord.Y, CsrpRectHeight(WriteRegion));
   SizeX = RtlMin(BufferSize.X - BufferCoord.X, CsrpRectWidth(WriteRegion));
   WriteRegion.Bottom = WriteRegion.Top + SizeY - 1;
   WriteRegion.Right = WriteRegion.Left + SizeX - 1;

   /* Make sure WriteRegion is inside the screen buffer */
   CsrpInitRect(ScreenBuffer, 0, 0, Buff->MaxY - 1, Buff->MaxX - 1);
   if (!CsrpGetIntersection(&WriteRegion, ScreenBuffer, WriteRegion))
      {
         UNLOCK;
         /* It is okay to have a WriteRegion completely outside the screen buffer.
            No data is written then. */
         return (Reply->Status = STATUS_SUCCESS);
      }

   for ( i = 0, Y = WriteRegion.Top; Y <= WriteRegion.Bottom; i++, Y++ )
   {
     CurCharInfo = CharInfo + (i + BufferCoord.Y) * BufferSize.X + BufferCoord.X;
     Offset = (((Y + Buff->ShowY) % Buff->MaxY) * Buff->MaxX + WriteRegion.Left) * 2;
     for ( X = WriteRegion.Left; X <= WriteRegion.Right; X++ )
      {
        SET_CELL_BUFFER(Buff, Offset, CurCharInfo->Char.AsciiChar, CurCharInfo->Attributes);
        CurCharInfo++;
      }
   }

   if( Buff == ActiveConsole->ActiveBuffer )
     {
        CsrpDrawRegion( ActiveConsole->ActiveBuffer, WriteRegion );
     }

   UNLOCK;
   Reply->Data.WriteConsoleOutputReply.WriteRegion.Right = WriteRegion.Left + SizeX - 1;
   Reply->Data.WriteConsoleOutputReply.WriteRegion.Bottom = WriteRegion.Top + SizeY - 1;
   Reply->Data.WriteConsoleOutputReply.WriteRegion.Left = WriteRegion.Left;
   Reply->Data.WriteConsoleOutputReply.WriteRegion.Top = WriteRegion.Top;
   return (Reply->Status = STATUS_SUCCESS);
}

CSR_API(CsrFlushInputBuffer)
{
  PLIST_ENTRY CurrentEntry;
  PLIST_ENTRY NextEntry;
  PCSRSS_CONSOLE Console;
  ConsoleInput* Input;
  NTSTATUS Status;

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);
  LOCK;
  Status = CsrGetObject( ProcessData, Request->Data.FlushInputBufferRequest.ConsoleInput, (Object_t **)&Console );
  if( !NT_SUCCESS( Status ) || (Status = Console->Header.Type == CSRSS_CONSOLE_MAGIC ? STATUS_SUCCESS : STATUS_INVALID_HANDLE ))
    {
	Reply->Status = Status;
	UNLOCK;
	return Status;
    }

  /* Discard all entries in the input event queue */
  while (!IsListEmpty(&Console->InputEvents))
    {
      CurrentEntry = RemoveHeadList(&Console->InputEvents);
      Input = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);
      /* Destroy the event */
      RtlFreeHeap( CsrssApiHeap, 0, Input );
    }
  Console->WaitingChars=0;
  UNLOCK;

  return (Reply->Status = STATUS_SUCCESS);
}

CSR_API(CsrScrollConsoleScreenBuffer)
{
  SHORT i, X, Y, SizeX, SizeY;
  PCSRSS_SCREEN_BUFFER Buff;
  SMALL_RECT ScreenBuffer;
  SMALL_RECT SrcRegion;
  SMALL_RECT DstRegion;
  SMALL_RECT FillRegion;
  IO_STATUS_BLOCK Iosb;
  CHAR_INFO* CharInfo;
  NTSTATUS Status;
  DWORD SrcOffset;
  DWORD DstOffset;
  BOOLEAN DoFill;

  ALIAS(ConsoleHandle,Request->Data.ScrollConsoleScreenBufferRequest.ConsoleHandle);
  ALIAS(ScrollRectangle,Request->Data.ScrollConsoleScreenBufferRequest.ScrollRectangle);
  ALIAS(UseClipRectangle,Request->Data.ScrollConsoleScreenBufferRequest.UseClipRectangle);
  ALIAS(ClipRectangle,Request->Data.ScrollConsoleScreenBufferRequest.ClipRectangle);
  ALIAS(DestinationOrigin,Request->Data.ScrollConsoleScreenBufferRequest.DestinationOrigin);
  ALIAS(Fill,Request->Data.ScrollConsoleScreenBufferRequest.Fill);

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);
  LOCK;
  Status = CsrGetObject( ProcessData, ConsoleHandle, (Object_t **)&Buff );
  if( !NT_SUCCESS( Status ) || (Status = Buff->Header.Type == CSRSS_SCREEN_BUFFER_MAGIC ? STATUS_SUCCESS : STATUS_INVALID_HANDLE ))
  {
    Reply->Status = Status;
    UNLOCK;
    return Status;
  }

  /* Make sure source rectangle is inside the screen buffer */
  CsrpInitRect(ScreenBuffer, 0, 0, Buff->MaxY - 1, Buff->MaxX - 1);
  if (!CsrpGetIntersection(&SrcRegion, ScreenBuffer, ScrollRectangle))
    {
      UNLOCK;
      return (Reply->Status = STATUS_INVALID_PARAMETER);
    }

  if (UseClipRectangle)
    {
      if (!CsrpGetIntersection(&SrcRegion, SrcRegion, ClipRectangle))
        {
          UNLOCK;
          return (Reply->Status = STATUS_SUCCESS);
        }
    }


  CsrpInitRect(
    DstRegion,
    DestinationOrigin.Y,
    DestinationOrigin.X,
    DestinationOrigin.Y + CsrpRectHeight(ScrollRectangle) - 1,
    DestinationOrigin.X + CsrpRectWidth(ScrollRectangle) - 1)

  /* Make sure destination rectangle is inside the screen buffer */
  if (!CsrpGetIntersection(&DstRegion, DstRegion, ScreenBuffer))
    {
      UNLOCK;
      return (Reply->Status = STATUS_INVALID_PARAMETER);
    }

  CsrpCopyRegion(Buff, SrcRegion, DstRegion);


  /* Get the region that should be filled with the specified character and attributes */

  DoFill = FALSE;

  CsrpGetUnion(&FillRegion, SrcRegion, DstRegion);

  if (CsrpSubtractRect(&FillRegion, FillRegion, DstRegion))
    {
      /* FIXME: The subtracted rectangle is off by one line */
      FillRegion.Top += 1;

      CsrpFillRegion(Buff, FillRegion, Fill);
      DoFill = TRUE;
    }

  if (Buff == ActiveConsole->ActiveBuffer)
    {
      /* Draw destination region */
      CsrpDrawRegion(ActiveConsole->ActiveBuffer, DstRegion);

      if (DoFill)
        {
          /* Draw filled region */
          CsrpDrawRegion(ActiveConsole->ActiveBuffer, FillRegion);
        }
    }

  UNLOCK;
  return(Reply->Status = STATUS_SUCCESS);
}


CSR_API(CsrReadConsoleOutputChar)
{
  NTSTATUS Status;
  PCSRSS_SCREEN_BUFFER ScreenBuffer;
  DWORD Xpos, Ypos;
  BYTE* ReadBuffer;
  DWORD i;

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = Reply->Header.MessageSize - sizeof(LPC_MESSAGE);
  ReadBuffer = Reply->Data.ReadConsoleOutputCharReply.String;

  LOCK;

  Status = CsrGetObject(ProcessData, Request->Data.ReadConsoleOutputCharRequest.ConsoleHandle, (Object_t**)&ScreenBuffer);
  if (!NT_SUCCESS(Status))
    {
      Reply->Status = Status;
      UNLOCK;
      return(Reply->Status);
    }

  if (ScreenBuffer->Header.Type != CSRSS_SCREEN_BUFFER_MAGIC)
    {
      Reply->Status = STATUS_INVALID_HANDLE;
      UNLOCK;
      return(Reply->Status);
    }

  Xpos = Request->Data.ReadConsoleOutputCharRequest.ReadCoord.X + ScreenBuffer->ShowX;
  Ypos = (Request->Data.ReadConsoleOutputCharRequest.ReadCoord.Y + ScreenBuffer->ShowY) % ScreenBuffer->MaxY;

  for (i = 0; i < Request->Data.ReadConsoleOutputCharRequest.NumCharsToRead; ++i)
    {
      *ReadBuffer = ScreenBuffer->Buffer[(Xpos * 2) + (Ypos * 2 * ScreenBuffer->MaxX)];

      ReadBuffer++;
      Xpos++;

      if (Xpos == ScreenBuffer->MaxX)
	{
	  Xpos = 0;
	  Ypos++;

	  if (Ypos == ScreenBuffer->MaxY)
	    Ypos = 0;
	}
    }

  *ReadBuffer = 0;

  Reply->Status = STATUS_SUCCESS;
  Reply->Data.ReadConsoleOutputCharReply.EndCoord.X = Xpos - ScreenBuffer->ShowX;
  Reply->Data.ReadConsoleOutputCharReply.EndCoord.Y = (Ypos - ScreenBuffer->ShowY + ScreenBuffer->MaxY) % ScreenBuffer->MaxY;
  Reply->Header.MessageSize += Request->Data.ReadConsoleOutputCharRequest.NumCharsToRead;
  Reply->Header.DataSize += Request->Data.ReadConsoleOutputCharRequest.NumCharsToRead;

  UNLOCK;

  return(Reply->Status);
}


CSR_API(CsrReadConsoleOutputAttrib)
{
  NTSTATUS Status;
  PCSRSS_SCREEN_BUFFER ScreenBuffer;
  DWORD Xpos, Ypos;
  CHAR* ReadBuffer;
  DWORD i;

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = Reply->Header.MessageSize - sizeof(LPC_MESSAGE);
  ReadBuffer = Reply->Data.ReadConsoleOutputAttribReply.String;

  LOCK;

  Status = CsrGetObject(ProcessData, Request->Data.ReadConsoleOutputAttribRequest.ConsoleHandle, (Object_t**)&ScreenBuffer);
  if (!NT_SUCCESS(Status))
    {
      Reply->Status = Status;
      UNLOCK;
      return(Reply->Status);
    }

  if (ScreenBuffer->Header.Type != CSRSS_SCREEN_BUFFER_MAGIC)
    {
      Reply->Status = STATUS_INVALID_HANDLE;
      UNLOCK;
      return(Reply->Status);
    }

  Xpos = Request->Data.ReadConsoleOutputAttribRequest.ReadCoord.X + ScreenBuffer->ShowX;
  Ypos = (Request->Data.ReadConsoleOutputAttribRequest.ReadCoord.Y + ScreenBuffer->ShowY) % ScreenBuffer->MaxY;

  for (i = 0; i < Request->Data.ReadConsoleOutputAttribRequest.NumAttrsToRead; ++i)
    {
      *ReadBuffer = ScreenBuffer->Buffer[(Xpos * 2) + (Ypos * 2 * ScreenBuffer->MaxX) + 1];

      ReadBuffer++;
      Xpos++;

      if (Xpos == ScreenBuffer->MaxX)
	{
	  Xpos = 0;
	  Ypos++;

	  if (Ypos == ScreenBuffer->MaxY)
	    Ypos = 0;
	}
    }

  *ReadBuffer = 0;

  Reply->Status = STATUS_SUCCESS;
  Reply->Data.ReadConsoleOutputAttribReply.EndCoord.X = Xpos - ScreenBuffer->ShowX;
  Reply->Data.ReadConsoleOutputAttribReply.EndCoord.Y = (Ypos - ScreenBuffer->ShowY + ScreenBuffer->MaxY) % ScreenBuffer->MaxY;
  Reply->Header.MessageSize += Request->Data.ReadConsoleOutputAttribRequest.NumAttrsToRead;
  Reply->Header.DataSize += Request->Data.ReadConsoleOutputAttribRequest.NumAttrsToRead;

  UNLOCK;

  return(Reply->Status);
}


CSR_API(CsrGetNumberOfConsoleInputEvents)
{
  NTSTATUS Status;
  PCSRSS_CONSOLE Console;
  PLIST_ENTRY CurrentItem;
  DWORD NumEvents;
  
  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = Reply->Header.MessageSize - sizeof(LPC_MESSAGE);

  LOCK;

  Status = CsrGetObject(ProcessData, Request->Data.GetNumInputEventsRequest.ConsoleHandle, (Object_t**)&Console);
  if (!NT_SUCCESS(Status))
    {
      Reply->Status = Status;
      UNLOCK;
      return(Reply->Status);
    }

  if (Console->Header.Type != CSRSS_CONSOLE_MAGIC)
    {
      Reply->Status = STATUS_INVALID_HANDLE;
      UNLOCK;
      return(Reply->Status);
    }
  
  CurrentItem = &Console->InputEvents;
  NumEvents = 0;
  
  // If there are any events ...
  if(CurrentItem->Flink != CurrentItem)
  {
    do
    {
      CurrentItem = CurrentItem->Flink;
      ++NumEvents;
    }while(CurrentItem != &Console->InputEvents);
  }
  
  UNLOCK;
  
  Reply->Status = STATUS_SUCCESS;
  Reply->Data.GetNumInputEventsReply.NumInputEvents = NumEvents;
   
  return Reply->Status;
}


CSR_API(CsrPeekConsoleInput)
{
   NTSTATUS Status;
   PCSRSS_CONSOLE Console;
   DWORD Size;
   DWORD Length;
   PLIST_ENTRY CurrentItem;
   PLIST_ENTRY NextItem;
   PINPUT_RECORD InputRecord;
   ConsoleInput* Item;
   UINT NumItems;
   
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);
   
   LOCK;
   
   Status = CsrGetObject(ProcessData, Request->Data.GetNumInputEventsRequest.ConsoleHandle, (Object_t**)&Console);
   if(!NT_SUCCESS(Status))
   {
      Reply->Status = Status;
      UNLOCK;
      return Reply->Status;
   }

   if(Console->Header.Type != CSRSS_CONSOLE_MAGIC)
   {
      Reply->Status = STATUS_INVALID_HANDLE;
      UNLOCK;
      return Reply->Status;
   }
   
   InputRecord = Request->Data.PeekConsoleInputRequest.InputRecord;
   Length = Request->Data.PeekConsoleInputRequest.Length;
   Size = Length * sizeof(INPUT_RECORD);
   
    if(((PVOID)InputRecord < ProcessData->CsrSectionViewBase)
         || (((PVOID)InputRecord + Size) > (ProcessData->CsrSectionViewBase + ProcessData->CsrSectionViewSize)))
   {
      UNLOCK;
      Reply->Status = STATUS_ACCESS_VIOLATION;
      return Reply->Status ;
   }
   
   NumItems = 0;
   
   if(!IsListEmpty(&Console->InputEvents))
   {
      CurrentItem = &Console->InputEvents;
   
      while(NumItems < Length)
      {
         ++NumItems;
         Item = CONTAINING_RECORD(CurrentItem, ConsoleInput, ListEntry);
         *InputRecord++ = Item->InputEvent;
         
         if(CurrentItem->Flink == &Console->InputEvents)
            break;
         else
            CurrentItem = CurrentItem->Flink;
      }
   }

   UNLOCK;
   
   Reply->Status = STATUS_SUCCESS;
   Reply->Data.PeekConsoleInputReply.Length = NumItems;
   return Reply->Status;
}


CSR_API(CsrReadConsoleOutput)
{
   PCHAR_INFO CharInfo;
   PCHAR_INFO CurCharInfo;
   PCSRSS_SCREEN_BUFFER ScreenBuffer;
   DWORD Size;
   DWORD Length;
   DWORD SizeX, SizeY;
   NTSTATUS Status;
   COORD BufferSize;
   COORD BufferCoord;
   SMALL_RECT ReadRegion;
   SMALL_RECT ScreenRect;
   DWORD i, Y, X, Offset;
      
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);
  
   LOCK;
  
   Status = CsrGetObject(ProcessData, Request->Data.ReadConsoleOutputRequest.ConsoleHandle, (Object_t**)&ScreenBuffer);
   if(!NT_SUCCESS(Status))
   {
      Reply->Status = Status;
      UNLOCK;
      return Reply->Status;
   }

   if(ScreenBuffer->Header.Type != CSRSS_SCREEN_BUFFER_MAGIC)
   {
      Reply->Status = STATUS_INVALID_HANDLE;
      UNLOCK;
      return Reply->Status;
   }
   
   CharInfo = Request->Data.ReadConsoleOutputRequest.CharInfo;
   ReadRegion = Request->Data.ReadConsoleOutputRequest.ReadRegion;
   BufferSize = Request->Data.ReadConsoleOutputRequest.BufferSize;
   BufferCoord = Request->Data.ReadConsoleOutputRequest.BufferCoord;
   Length = BufferSize.X * BufferSize.Y;
   Size = Length * sizeof(INPUT_RECORD);
   
    if(((PVOID)CharInfo < ProcessData->CsrSectionViewBase)
         || (((PVOID)CharInfo + Size) > (ProcessData->CsrSectionViewBase + ProcessData->CsrSectionViewSize)))
   {
      UNLOCK;
      Reply->Status = STATUS_ACCESS_VIOLATION;
      return Reply->Status ;
   }
   
   SizeY = RtlMin(BufferSize.Y - BufferCoord.Y, CsrpRectHeight(ReadRegion));
   SizeX = RtlMin(BufferSize.X - BufferCoord.X, CsrpRectWidth(ReadRegion));
   ReadRegion.Bottom = ReadRegion.Top + SizeY;
   ReadRegion.Right = ReadRegion.Left + SizeX;

   CsrpInitRect(ScreenRect, 0, 0, ScreenBuffer->MaxY - 1, ScreenBuffer->MaxX - 1);
   if (!CsrpGetIntersection(&ReadRegion, ScreenRect, ReadRegion))
   {
      UNLOCK;
      Reply->Status = STATUS_SUCCESS;
      return Reply->Status;
   }
   
   for(i = 0, Y = ReadRegion.Top; Y < ReadRegion.Bottom; ++i, ++Y)
   {
     CurCharInfo = CharInfo + (i * BufferSize.Y);
     
     Offset = (((Y + ScreenBuffer->ShowY) % ScreenBuffer->MaxY) * ScreenBuffer->MaxX + ReadRegion.Left) * 2;
     for(X = ReadRegion.Left; X < ReadRegion.Right; ++X)
     {
        CurCharInfo->Char.AsciiChar = GET_CELL_BUFFER(ScreenBuffer, Offset);
        CurCharInfo->Attributes = GET_CELL_BUFFER(ScreenBuffer, Offset);
        ++CurCharInfo;
     }
  }
  
   UNLOCK;
   
   Reply->Status = STATUS_SUCCESS;
   Reply->Data.ReadConsoleOutputReply.ReadRegion.Right = ReadRegion.Left + SizeX - 1;
   Reply->Data.ReadConsoleOutputReply.ReadRegion.Bottom = ReadRegion.Top + SizeY - 1;
   Reply->Data.ReadConsoleOutputReply.ReadRegion.Left = ReadRegion.Left;
   Reply->Data.ReadConsoleOutputReply.ReadRegion.Top = ReadRegion.Top;
   
   return Reply->Status;
}


CSR_API(CsrWriteConsoleInput)
{
   PINPUT_RECORD InputRecord;
   PCSRSS_CONSOLE Console;
   NTSTATUS Status;
   DWORD Length;
   DWORD Size;
   DWORD i;
   PLIST_ENTRY NextItem;
   ConsoleInput* Record;
   
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);
   
   LOCK;
   
   Status = CsrGetObject(ProcessData, Request->Data.WriteConsoleInputRequest.ConsoleHandle, (Object_t**)&Console);
   if(!NT_SUCCESS(Status))
   {
      Reply->Status = Status;
      UNLOCK;
      return Reply->Status;
   }

   if(Console->Header.Type != CSRSS_CONSOLE_MAGIC)
   {
      Reply->Status = STATUS_INVALID_HANDLE;
      UNLOCK;
      return Reply->Status;
   }
   
   InputRecord = Request->Data.WriteConsoleInputRequest.InputRecord;
   Length = Request->Data.WriteConsoleInputRequest.Length;
   Size = Length * sizeof(INPUT_RECORD);
   
    if(((PVOID)InputRecord < ProcessData->CsrSectionViewBase)
         || (((PVOID)InputRecord + Size) > (ProcessData->CsrSectionViewBase + ProcessData->CsrSectionViewSize)))
   {
      UNLOCK;
      Reply->Status = STATUS_ACCESS_VIOLATION;
      return Reply->Status ;
   }
   
   for(i = 0; i < Length; ++i)
   {
      Record = RtlAllocateHeap(CsrssApiHeap, 0, sizeof(ConsoleInput));
      if(Record == NULL)
      {
         UNLOCK;
         Reply->Status = STATUS_INSUFFICIENT_RESOURCES;
         return Reply->Status;
      }
      
      Record->InputEvent = *InputRecord++;
      InsertTailList(&Console->InputEvents, &Record->ListEntry);
   }
      
   UNLOCK;
   
   Reply->Status = STATUS_SUCCESS;
   Reply->Data.WriteConsoleInputReply.Length = i;
   return Reply->Status;
}

/**********************************************************************
 *	HardwareStateProperty
 *
 *	DESCRIPTION
 *		Set/Get the value of the HardwareState and switch
 *		between direct video buffer ouput and GDI windowed
 *		output.
 *	ARGUMENTS
 *		Client hands us a CSRSS_CONSOLE_HARDWARE_STATE
 *		object. We use the same object to reply.
 *	NOTE
 *		ConsoleHwState has the correct size to be compatible
 *		with NT's, but values are not.
 */
static NTSTATUS FASTCALL SetConsoleHardwareState (PCSRSS_CONSOLE Console, DWORD ConsoleHwState)
{
   DbgPrint( "Console Hardware State: %d\n", ConsoleHwState );

   if ( (CONSOLE_HARDWARE_STATE_GDI_MANAGED == ConsoleHwState)
      ||(CONSOLE_HARDWARE_STATE_DIRECT == ConsoleHwState))
   {
      /* Inhibit keyboard input when hardware state ==
       * CONSOLE_HARDWARE_STATE_GDI_MANAGED */
      if (CONSOLE_HARDWARE_STATE_GDI_MANAGED == ConsoleHwState) {
         DbgPrint( "Keyboard Inhibited.\n" );
         KeyReadInhibit = TRUE;
      } else {
         DbgPrint( "Keyboard Enabled.\n" );
         KeyReadInhibit = FALSE;
      }
      if (Console->HardwareState != ConsoleHwState)
      {
	 /* TODO: implement switching from full screen to windowed mode */
	 /* TODO: or back; now simply store the hardware state */
         Console->HardwareState = ConsoleHwState;
      }
      return STATUS_SUCCESS;	
   }
   return STATUS_INVALID_PARAMETER_3; // Client: (handle, set_get, [mode])
}

CSR_API(CsrHardwareStateProperty)
{
   PCSRSS_CONSOLE Console;
   NTSTATUS       Status;
 
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);

   LOCK;
   
   Status = CsrGetObject (
		   ProcessData,
		   Request->Data.ConsoleHardwareStateRequest.ConsoleHandle,
		   (Object_t**) & Console
		   );
   if (!NT_SUCCESS(Status))
   {
      DbgPrint( "Failed to get console handle in SetConsoleHardwareState\n" );
      Reply->Status = Status;
   }
   else
   {
      if(Console->Header.Type != CSRSS_CONSOLE_MAGIC)
      {
	DbgPrint( "Bad magic on Console: %08x\n", Console->Header.Type );
        Reply->Status = STATUS_INVALID_HANDLE;
      }
      else
      {
         switch (Request->Data.ConsoleHardwareStateRequest.SetGet)
         {
         case CONSOLE_HARDWARE_STATE_GET:
            Reply->Data.ConsoleHardwareStateReply.State = Console->HardwareState;
            break;
      
         case CONSOLE_HARDWARE_STATE_SET:
	    DbgPrint( "Setting console hardware state.\n" );
            Reply->Status = SetConsoleHardwareState (Console, Request->Data.ConsoleHardwareStateRequest.State);
            break;

         default:
            Reply->Status = STATUS_INVALID_PARAMETER_2; // Client: (handle, [set_get], mode)
            break;
         }
      }
   }

   UNLOCK;

   return Reply->Status;
}

CSR_API(CsrGetConsoleWindow)
{
   PCSRSS_CONSOLE Console;
   NTSTATUS       Status;
 
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);

   LOCK;
   
   Status = CsrGetObject (
		   ProcessData,
		   Request->Data.ConsoleWindowRequest.ConsoleHandle,
		   (Object_t**) & Console
		   );
   if (!NT_SUCCESS(Status))
   {
      Reply->Status = Status;
   }
   else
   {
      if(Console->Header.Type != CSRSS_CONSOLE_MAGIC)
      {
         Reply->Status = STATUS_INVALID_HANDLE;
      }
      else
      {
	 // Is this GDI handle valid in the client's context?
	 Reply->Data.ConsoleWindowReply.WindowHandle = Console->hWindow;
      }
   }

   UNLOCK;

   return Reply->Status;
}

/* EOF */
