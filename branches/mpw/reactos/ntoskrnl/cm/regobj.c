/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             ntoskrnl/cm/regobj.c
 * PURPOSE:          Registry object manipulation routines.
 * UPDATE HISTORY:
*/

#include <ddk/ntddk.h>
#include <roscfg.h>
#include <internal/ob.h>
#include <limits.h>
#include <string.h>
#include <internal/pool.h>
#include <internal/registry.h>

#define NDEBUG
#include <internal/debug.h>

#include "cm.h"

extern POBJECT_TYPE  CmiKeyType;
extern KSPIN_LOCK  CmiKeyListLock;

NTSTATUS STDCALL
CmiObjectParse(PVOID ParsedObject,
	PVOID *NextObject,
	PUNICODE_STRING FullPath,
	PWSTR *Path,
	POBJECT_TYPE ObjectType,
	ULONG Attributes)
{
	BLOCK_OFFSET BlockOffset;
	PKEY_OBJECT FoundObject;
	PKEY_OBJECT ParsedKey;
	PKEY_CELL SubKeyCell;
	CHAR cPath[MAX_PATH];
	NTSTATUS Status;
	PWSTR end;

	ParsedKey = ParsedObject;

  VERIFY_KEY_OBJECT(ParsedKey);

	*NextObject = NULL;

	if ((*Path) == NULL)
		{
      DPRINT("*Path is NULL\n");
		  return STATUS_UNSUCCESSFUL;
		}
	
	if ((*Path[0]) == '\\')
		{
			end = wcschr((*Path) + 1, '\\');
			if (end != NULL)
			  *end = 0;
			wcstombs(cPath, (*Path) + 1, wcslen((*Path) + 1));
			cPath[wcslen((*Path) + 1)] = 0;
		}
	else
		{
			end = wcschr((*Path), '\\');
			if (end != NULL)
  			*end = 0;
			wcstombs(cPath, (*Path), wcslen((*Path)));
			cPath[wcslen((*Path))] = 0;
		}

	FoundObject = CmiScanKeyList(ParsedKey, cPath, Attributes);
	if (FoundObject == NULL)
		{
		  Status = CmiScanForSubKey(ParsedKey->RegistryHive,
	      ParsedKey->KeyCell,
	      &SubKeyCell,
	      &BlockOffset,
	      cPath,
	      0,
	      Attributes);

			if (!NT_SUCCESS(Status) || (SubKeyCell == NULL))
			{
				if (end != NULL)
					{
  					*end = '\\';
					}
				return STATUS_UNSUCCESSFUL;
			}
	
  		/* Create new key object and put into linked list */
			DPRINT("CmiObjectParse %s\n", cPath);
			Status = ObCreateObject(NULL,
				STANDARD_RIGHTS_REQUIRED,
				NULL,
				CmiKeyType,
				(PVOID*) &FoundObject);

			if (!NT_SUCCESS(Status))
				{
				  return Status;
				}

			FoundObject->Flags = 0;
			FoundObject->Name = SubKeyCell->Name;
			FoundObject->NameSize = SubKeyCell->NameSize;
			FoundObject->KeyCell = SubKeyCell;
			FoundObject->BlockOffset = BlockOffset;
			FoundObject->RegistryHive = ParsedKey->RegistryHive;
			CmiAddKeyToList(ParsedKey, FoundObject);
			DPRINT("Created object 0x%x\n", FoundObject);
		}
	else
    {
		  ObReferenceObjectByPointer(FoundObject,
		    STANDARD_RIGHTS_REQUIRED,
		    NULL,
		    UserMode);
    }

	DPRINT("CmiObjectParse %s\n", FoundObject->Name);

	if (end != NULL)
		{
			*end = '\\';
		  *Path = end;
		}
  else
		{
			*Path = NULL;
		}

VERIFY_KEY_OBJECT(FoundObject);
	
	*NextObject = FoundObject;
	
	return STATUS_SUCCESS;
}


NTSTATUS STDCALL
CmiObjectCreate(PVOID ObjectBody,
	PVOID Parent,
	PWSTR RemainingPath,
	struct _OBJECT_ATTRIBUTES* ObjectAttributes)
{
  PKEY_OBJECT pKey = ObjectBody;
	pKey->ParentKey = Parent;
	if (RemainingPath)
		{
			if(RemainingPath[0]== L'\\')
				{
					pKey->Name = (PCHAR) (&RemainingPath[1]);
					pKey->NameSize = wcslen(RemainingPath) - 1;
				}
			else
				{
					pKey->Name = (PCHAR) RemainingPath;
					pKey->NameSize = wcslen(RemainingPath);
				}
	   }
   else
    {
      pKey->NameSize = 0;
    }

  return STATUS_SUCCESS;
}


VOID STDCALL
CmiObjectDelete(PVOID DeletedObject)
{
  PKEY_OBJECT KeyObject;

  DPRINT("Delete object key\n");

  KeyObject = (PKEY_OBJECT) DeletedObject;

  if (!NT_SUCCESS(CmiRemoveKeyFromList(KeyObject)))
	  {
	    DPRINT1("Key not found in parent list ???\n");
	  }

  if (KeyObject->Flags & KO_MARKED_FOR_DELETE)
    {
      DPRINT("delete really key\n");
      CmiDestroyBlock(KeyObject->RegistryHive,
        KeyObject->KeyCell,
			  KeyObject->BlockOffset);
    }
  else
    {
      CmiReleaseBlock(KeyObject->RegistryHive, KeyObject->KeyCell);
    }
}

VOID
CmiAddKeyToList(PKEY_OBJECT ParentKey,
  PKEY_OBJECT NewKey)
{
  KIRQL OldIrql;

  DPRINT("ParentKey %.08x\n", ParentKey);
  
  KeAcquireSpinLock(&CmiKeyListLock, &OldIrql);

  if (ParentKey->SizeOfSubKeys <= ParentKey->NumberOfSubKeys)
	  {
	    PKEY_OBJECT *tmpSubKeys = ExAllocatePool(NonPagedPool,
        (ParentKey->NumberOfSubKeys + 1) * sizeof(DWORD));

	    if (ParentKey->NumberOfSubKeys > 0)
        {
	        memcpy(tmpSubKeys,
            ParentKey->SubKeys,
			      ParentKey->NumberOfSubKeys * sizeof(DWORD));
        }

	    if (ParentKey->SubKeys)
        ExFreePool(ParentKey->SubKeys);

	    ParentKey->SubKeys = tmpSubKeys;
	    ParentKey->SizeOfSubKeys = ParentKey->NumberOfSubKeys + 1;
	  }

  /* FIXME: Please maintain the list in alphabetic order */
  /*      to allow a dichotomic search */
  ParentKey->SubKeys[ParentKey->NumberOfSubKeys++] = NewKey;

DPRINT("Reference parent key: 0x%x\n", ParentKey);

  ObReferenceObjectByPointer(ParentKey,
		STANDARD_RIGHTS_REQUIRED,
		NULL,
		UserMode);
  NewKey->ParentKey = ParentKey;
  KeReleaseSpinLock(&CmiKeyListLock, OldIrql);
}


NTSTATUS
CmiRemoveKeyFromList(PKEY_OBJECT KeyToRemove)
{
  PKEY_OBJECT ParentKey;
  KIRQL OldIrql;
  DWORD Index;

  ParentKey = KeyToRemove->ParentKey;
  KeAcquireSpinLock(&CmiKeyListLock, &OldIrql);
  /* FIXME: If list maintained in alphabetic order, use dichotomic search */
  for (Index = 0; Index < ParentKey->NumberOfSubKeys; Index++)
	  {
	    if (ParentKey->SubKeys[Index] == KeyToRemove)
	    {
		      if (Index < ParentKey->NumberOfSubKeys-1)
		        RtlMoveMemory(&ParentKey->SubKeys[Index],
				      &ParentKey->SubKeys[Index + 1],
				      (ParentKey->NumberOfSubKeys - Index - 1) * sizeof(PKEY_OBJECT));
		      ParentKey->NumberOfSubKeys--;
		      KeReleaseSpinLock(&CmiKeyListLock, OldIrql);

DPRINT("Dereference parent key: 0x%x\n", ParentKey);
	
		      ObDereferenceObject(ParentKey);
		      return STATUS_SUCCESS;
		    }
	  }
  KeReleaseSpinLock(&CmiKeyListLock, OldIrql);
  return STATUS_UNSUCCESSFUL;
}


PKEY_OBJECT
CmiScanKeyList(PKEY_OBJECT Parent,
	PCHAR KeyName,
	ULONG Attributes)
{
	PKEY_OBJECT CurKey;
	KIRQL OldIrql;
	WORD NameSize;
	DWORD Index;

  DPRINT("Scanning key list for %s (Parent %s)\n",
    KeyName, Parent->Name);

  NameSize = strlen(KeyName);
  KeAcquireSpinLock(&CmiKeyListLock, &OldIrql);
  /* FIXME: if list maintained in alphabetic order, use dichotomic search */
  for (Index=0; Index < Parent->NumberOfSubKeys; Index++)
	  {
	    CurKey = Parent->SubKeys[Index];
	    if (Attributes & OBJ_CASE_INSENSITIVE)
		    {
		      if ((NameSize == CurKey->NameSize)
	          && (_strnicmp(KeyName, CurKey->Name, NameSize) == 0))
	          {
	            KeReleaseSpinLock(&CmiKeyListLock, OldIrql);
	            return CurKey;
	          }
		    }
	    else
		    {
		      if ((NameSize == CurKey->NameSize)
			      && (strncmp(KeyName,CurKey->Name,NameSize) == 0))
			      {
			         KeReleaseSpinLock(&CmiKeyListLock, OldIrql);
			         return CurKey;
			      }
		    }
	  }
  KeReleaseSpinLock(&CmiKeyListLock, OldIrql);
  
  return NULL;
}
