/* $Id: close.c,v 1.4 2001/03/13 16:25:54 dwelch Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/close.c
 * PURPOSE:         Communication mechanism
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/port.h>
#include <internal/dbg.h>

#define NDEBUG
#include <internal/debug.h>


/**********************************************************************
 * NAME
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
VOID
NiClosePort (PVOID	ObjectBody, ULONG	HandleCount)
{
  PEPORT Port = (PEPORT)ObjectBody;
  LPC_MESSAGE Message;
  
  /*
   * If the client has just closed its handle then tell the server what
   * happened and disconnect this port.
   */
  if (HandleCount == 0 && Port->State == EPORT_CONNECTED_CLIENT && 
      ObGetReferenceCount(Port) == 2)
    {
      Message.MessageSize = sizeof(LPC_MESSAGE);
      Message.DataSize = 0;
      
      EiReplyOrRequestPort (Port->OtherPort,
			    &Message,
			    LPC_PORT_CLOSED,
			    Port);
      KeSetEvent (&Port->OtherPort->Event,
		  IO_NO_INCREMENT,
		  FALSE);
      
      Port->OtherPort->OtherPort = NULL;
      Port->OtherPort->State = EPORT_DISCONNECTED;
      ObDereferenceObject (Port);
    }

  /*
   * If the server has closed all of its handles then disconnect the port,
   * don't actually notify the client until it attempts an operation.
   */
  if (HandleCount == 0 && Port->State == EPORT_CONNECTED_SERVER && 
      ObGetReferenceCount(Port) == 2)
    {
	Port->OtherPort->OtherPort = NULL;
	Port->OtherPort->State = EPORT_DISCONNECTED;
	ObDereferenceObject(Port->OtherPort);
     }
}


/**********************************************************************
 * NAME
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
VOID
NiDeletePort (PVOID	ObjectBody)
{
   //   PEPORT Port = (PEPORT)ObjectBody;
   
   //   DPRINT1("Deleting port %x\n", Port);
}


/* EOF */
