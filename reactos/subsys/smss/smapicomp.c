/* $Id: $
 *
 * smapicomp.c - SM_API_COMPLETE_SESSION
 *
 * Reactos Session Manager
 *
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 */
#include "smss.h"

#define NDEBUG
#include <debug.h>


/**********************************************************************
 * SmCompSes/1							API
 */
SMAPI(SmCompSes)
{
	NTSTATUS Status = STATUS_SUCCESS;

	DPRINT("SM: %s called from [%lx|%lx]\n",
		__FUNCTION__,
		Request->ClientId.UniqueProcessId,
		Request->ClientId.UniqueThreadId);
	
	Status = SmCompleteClientInitialization (Request->Header.ClientId.UniqueProcess);
	if(!NT_SUCCESS(Status))
	{
		Request->Status = STATUS_UNSUCCESSFUL;
	}
	return Status;
}


/* EOF */
