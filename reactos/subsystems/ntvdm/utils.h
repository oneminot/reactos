/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            utils.h
 * PURPOSE:         Utility Functions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _UTILS_H_
#define _UTILS_H_

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

/* FUNCTIONS ******************************************************************/

VOID
FileClose(IN HANDLE FileHandle);

HANDLE
FileOpen(IN  PCSTR  FileName,
         OUT PULONG FileSize OPTIONAL);

BOOLEAN
FileLoadByHandle(IN  HANDLE FileHandle,
                 IN  PVOID  Location,
                 IN  ULONG  FileSize,
                 OUT PULONG BytesRead);

#endif // _UTILS_H_

/* EOF */
