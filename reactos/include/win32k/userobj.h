/* $Id: userobj.h,v 1.1 2001/07/06 00:05:05 rex Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          USER Object manager interface definition
 * FILE:             include/win32k/userobj.h
 * PROGRAMER:        Rex Jolliff (rex@lvcablemodem.com)
 *
 */

#ifndef  __WIN32K_USEROBJ_H
#define  __WIN32K_USEROBJ_H

#include <ddk/ntddk.h>

#define  UO_CLASS_MAGIC     0x5141
#define  UO_WINDOW_MAGIC    0x5142
#define  UO_MAGIC_DONTCARE  0xffff

#define USER_OBJECT_TAG (ULONG)(('W'<<0) + ('3'<<8) + ('2'<<16) + ('U'<<24))

typedef struct tag_USER_OBJECT_HEADER
{
  WORD  magic;
  DWORD  referenceCount;
  LIST_ENTRY  listEntry;
  FAST_MUTEX  mutex;
} USER_OBJECT_HEADER, *PUSER_OBJECT_HEADER;

typedef  PVOID  PUSER_OBJECT;
typedef  HANDLE  HUSEROBJ;

#define  UserObjectHeaderToBody(header) ((PUSER_OBJECT)(((PCHAR)header)+sizeof(USER_OBJECT_HEADER)))
#define  UserObjectBodyToHeader(body) ((PUSER_OBJECT_HEADER)(((PCHAR)body)-sizeof(USER_OBJECT_HEADER)))
#define  UserObjectHeaderToHandle(header) ((HUSEROBJ)header)
#define  UserObjectHandleToHeader(handle) ((HUSEROBJ)handle)

PUSER_OBJECT  USEROBJ_AllocObject (WORD  size, WORD  magic);
BOOL  USEROBJ_FreeObject (PUSER_OBJECT  object, WORD  magic);
HUSEROBJ  USEROBJ_PtrToHandle (PUSER_OBJECT  object, WORD  magic);
PUSER_OBJECT  USEROBJ_HandleToPtr (HUSEROBJ  object, WORD  magic);
BOOL  USEROBJ_LockObject (HUSEROBJ Obj);
BOOL  USEROBJ_UnlockObject (HUSEROBJ Obj);

#endif

