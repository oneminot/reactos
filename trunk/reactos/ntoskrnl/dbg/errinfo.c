/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/errinfo.c
 * PURPOSE:         Print information descriptions of error messages
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static PCHAR SeverityCodes[] = {"SUC", "INF", "ERR", "WRN"};

static struct _FACLIST
{
  ULONG Code;
  PCHAR Name;
} FacList[] = 
{
  {0, "KERNEL"},
  {0, NULL}
};

static struct _ERRLIST 
{
  NTSTATUS Code;
  PCHAR Name;
  PCHAR Text;
} ErrList[] = 
{
  {STATUS_SUCCESS, "SUCCESS", NULL},
  {STATUS_INSUFFICIENT_RESOURCES, "INSUFFICIENT_RESOURCES", NULL},
//  {STATUS_OBJECT_NAME_EXISTS, "OBJECT_NAME_EXISTS", NULL},
  {STATUS_OBJECT_NAME_COLLISION, "OBJECT_NAME_COLLISION", NULL},
  {STATUS_CTL_FILE_NOT_SUPPORTED, "CTL_FILE_NOT_SUPPORTED", NULL},
  {STATUS_PORT_ALREADY_SET, "PORT_ALREADY_SET", NULL},
  {STATUS_SECTION_NOT_IMAGE, "SECTION_NOT_IMAGE", NULL},
  {STATUS_BAD_WORKING_SET_LIMIT, "BAD_WORKING_SET_LIMIT", NULL},
  {STATUS_INCOMPATIBLE_FILE_MAP, "INCOMPATIBLE_FILE_MAP", NULL},
  {STATUS_HANDLE_NOT_WAITABLE, "HANDLE_NOT_WAITABLE", NULL},
  {STATUS_PORT_DISCONNECTED, "PORT_DISCONNECTED", NULL},
  {STATUS_NOT_LOCKED, "NOT_LOCKED", NULL},
  {STATUS_NOT_MAPPED_VIEW, "NOT_MAPPED_VIEW", NULL},
  {STATUS_UNABLE_TO_FREE_VM, "UNABLE_TO_FREE_VM", NULL},
  {STATUS_UNABLE_TO_DELETE_SECTION, "UNABLE_TO_DELETE_SECTION", NULL},
  {STATUS_MORE_PROCESSING_REQUIRED, "MORE_PROCESSING_REQUIRED", NULL},
  {STATUS_INVALID_CID, "INVALID_CID", NULL},
  {STATUS_BAD_INITIAL_STACK, "BAD_INITIAL_STACK", NULL},
  {STATUS_INVALID_VOLUME_LABEL, "INVALID_VOLUME_LABEL", NULL},
  {STATUS_SECTION_NOT_EXTENDED, "SECTION_NOT_EXTENDED", NULL},
  {STATUS_NOT_MAPPED_DATA, "NOT_MAPPED_DATA", NULL},
  {STATUS_INFO_LENGTH_MISMATCH, "INFO_LENGTH_MISMATCH", NULL},
  {STATUS_INVALID_INFO_CLASS, "INVALID_INFO_CLASS", NULL},
  {STATUS_SUSPEND_COUNT_EXCEEDED, "SUSPEND_COUNT_EXCEEDED", NULL},
  {STATUS_NOTIFY_ENUM_DIR, "NOTIFY_ENUM_DIR", NULL},
  {STATUS_REGISTRY_RECOVERED, "REGISTRY_RECOVERED", NULL},
  {STATUS_REGISTRY_IO_FAILED, "REGISTRY_IO_FAILED", NULL},
  {STATUS_KEY_DELETED, "KEY_DELETED", NULL},
  {STATUS_NO_LOG_SPACE, "NO_LOG_SPACE", NULL},
  {STATUS_KEY_HAS_CHILDREN, "KEY_HAS_CHILDREN", NULL},
  {STATUS_CHILD_MUST_BE_VOLATILE, "CHILD_MUST_BE_VOLATILE", NULL},
  {STATUS_REGISTRY_CORRUPT, "REGISTRY_CORRUPT", NULL},
  {STATUS_DLL_NOT_FOUND, "DLL_NOT_FOUND", NULL},
  {STATUS_DLL_INIT_FAILED, "DLL_INIT_FAILED", NULL},
  {STATUS_ORDINAL_NOT_FOUND, "ORDINAL_NOT_FOUND", NULL},
  {STATUS_ENTRYPOINT_NOT_FOUND, "ENTRYPOINT_NOT_FOUND", NULL},
  {STATUS_MORE_ENTRIES, "MORE_ENTRIES", NULL},
  {STATUS_BUFFER_OVERFLOW, "BUFFER_OVERFLOW", NULL},
  {STATUS_NO_MORE_FILES, "NO_MORE_FILES", NULL},
  {STATUS_NO_INHERITANCE, "NO_INHERITANCE", NULL},
  {STATUS_NO_MORE_EAS, "NO_MORE_EAS", NULL},
  {STATUS_NO_MORE_ENTRIES, "NO_MORE_ENTRIES", NULL},
  {STATUS_GUIDS_EXHAUSTED, "GUIDS_EXHAUSTED", NULL},
  {STATUS_AGENTS_EXHAUSTED, "AGENTS_EXHAUSTED", NULL},
  {STATUS_UNSUCCESSFUL, "UNSUCCESSFUL", NULL},
  {STATUS_NOT_IMPLEMENTED, "NOT_IMPLEMENTED", NULL},
  {STATUS_ILLEGAL_FUNCTION, "ILLEGAL_FUNCTION", NULL},
  {STATUS_PAGEFILE_QUOTA, "PAGEFILE_QUOTA", NULL},
  {STATUS_COMMITMENT_LIMIT, "COMMITMENT_LIMIT", NULL},
  {STATUS_SECTION_TOO_BIG, "SECTION_TOO_BIG", NULL},
  {RPC_NT_SS_IN_NULL_CONTEXT, "RPC_NT_SS_IN_NULL_CONTEXT", NULL},
  {RPC_NT_INVALID_BINDING, "RPC_NT_INVALID_BINDING", NULL},
  {STATUS_OBJECT_FILE_MISMATCH, "OBJECT_FILE_MISMATCH", NULL},
  {STATUS_FILE_CLOSED, "FILE_CLOSED", NULL},
  {STATUS_INVALID_PORT_HANDLE, "INVALID_PORT_HANDLE", NULL},
  {STATUS_NOT_COMMITTED, "NOT_COMMITTED", NULL},
  {STATUS_INVALID_PARAMETER, "INVALID_PARAMETER", NULL},
  {STATUS_INVALID_PARAMETER_1, "INVALID_PARAMETER_1", NULL},
  {STATUS_INVALID_PARAMETER_2, "INVALID_PARAMETER_2", NULL},
  {STATUS_INVALID_PARAMETER_3, "INVALID_PARAMETER_3", NULL},
  {STATUS_INVALID_PARAMETER_4, "INVALID_PARAMETER_4", NULL},
  {STATUS_INVALID_PARAMETER_5, "INVALID_PARAMETER_5", NULL},
  {STATUS_INVALID_PARAMETER_6, "INVALID_PARAMETER_6", NULL},
  {STATUS_INVALID_PARAMETER_7, "INVALID_PARAMETER_7", NULL},
  {STATUS_INVALID_PARAMETER_8, "INVALID_PARAMETER_8", NULL},
  {STATUS_INVALID_PARAMETER_9, "INVALID_PARAMETER_9", NULL},
  {STATUS_INVALID_PARAMETER_10, "INVALID_PARAMETER_10", NULL},
  {STATUS_INVALID_PARAMETER_11, "INVALID_PARAMETER_11", NULL},
  {STATUS_INVALID_PARAMETER_12, "INVALID_PARAMETER_12", NULL},
  {STATUS_INVALID_PARAMETER_MAX, "INVALID_PARAMETER_MAX", NULL},
  {STATUS_INVALID_PAGE_PROTECTION, "INVALID_PAGE_PROTECTION", NULL},
  {STATUS_RESOURCE_DATA_NOT_FOUND, "RESOURCE_DATA_NOT_FOUND", NULL},
  {STATUS_RESOURCE_TYPE_NOT_FOUND, "RESOURCE_TYPE_NOT_FOUND", NULL},
  {STATUS_RESOURCE_NAME_NOT_FOUND, "RESOURCE_NAME_NOT_FOUND", NULL},
  {STATUS_RESOURCE_LANG_NOT_FOUND, "RESOURCE_LANG_NOT_FOUND", NULL},
  {STATUS_NO_SUCH_DEVICE, "NO_SUCH_DEVICE", NULL},
  {STATUS_NO_SUCH_FILE, "NO_SUCH_FILE", NULL},
  {STATUS_INVALID_DEVICE_REQUEST, "INVALID_DEVICE_REQUEST", NULL},
  {STATUS_END_OF_FILE, "END_OF_FILE", NULL},
  {STATUS_FILE_FORCED_CLOSED, "FILE_FORCED_CLOSED", NULL},
  {STATUS_WRONG_VOLUME, "WRONG_VOLUME", NULL},
  {STATUS_NO_MEDIA, "NO_MEDIA", NULL},
  {STATUS_NO_MEDIA_IN_DEVICE, "NO_MEDIA_IN_DEVICE", NULL},
  {STATUS_NONEXISTENT_SECTOR, "NONEXISTENT_SECTOR", NULL},
  {STATUS_WORKING_SET_QUOTA, "WORKING_SET_QUOTA", NULL},
  {STATUS_CONFLICTING_ADDRESS, "CONFLICTING_ADDRESS", NULL},
  {STATUS_INVALID_SYSTEM_SERVICE, "INVALID_SYSTEM_SERVICE", NULL},
  {STATUS_THREAD_IS_TERMINATING, "THREAD_IS_TERMINATING", NULL},
  {STATUS_PROCESS_IS_TERMINATING, "PROCESS_IS_TERMINATING", NULL},
  {STATUS_INVALID_LOCK_SEQUENCE, "INVALID_LOCK_SEQUENCE", NULL},
  {STATUS_INVALID_VIEW_SIZE, "INVALID_VIEW_SIZE", NULL},
  {STATUS_ALREADY_COMMITTED, "ALREADY_COMMITTED", NULL},
  {STATUS_ACCESS_DENIED, "ACCESS_DENIED", NULL},
  {STATUS_FILE_IS_A_DIRECTORY, "FILE_IS_A_DIRECTORY", NULL},
  {STATUS_CANNOT_DELETE, "CANNOT_DELETE", NULL},
  {STATUS_INVALID_COMPUTER_NAME, "INVALID_COMPUTER_NAME", NULL},
  {STATUS_FILE_DELETED, "FILE_DELETED", NULL},
  {STATUS_DELETE_PENDING, "DELETE_PENDING", NULL},
  {STATUS_PORT_CONNECTION_REFUSED, "PORT_CONNECTION_REFUSED", NULL},
  {STATUS_NO_SUCH_PRIVILEGE, "NO_SUCH_PRIVILEGE", NULL},
  {STATUS_PRIVILEGE_NOT_HELD, "PRIVILEGE_NOT_HELD", NULL},
  {STATUS_CANNOT_IMPERSONATE, "CANNOT_IMPERSONATE", NULL},
  {STATUS_LOGON_FAILURE, "LOGON_FAILURE", NULL},
  {STATUS_ACCOUNT_RESTRICTION, "ACCOUNT_RESTRICTION", NULL},
  {STATUS_INVALID_LOGON_HOURS, "INVALID_LOGON_HOURS", NULL},
  {STATUS_INVALID_WORKSTATION, "INVALID_WORKSTATION", NULL},
  {STATUS_BUFFER_TOO_SMALL, "BUFFER_TOO_SMALL", NULL},
  {STATUS_UNABLE_TO_DECOMMIT_VM, "UNABLE_TO_DECOMMIT_VM", NULL},
  {STATUS_DISK_CORRUPT_ERROR, "DISK_CORRUPT_ERROR", NULL},
  {STATUS_OBJECT_NAME_INVALID, "OBJECT_NAME_INVALID", NULL},
  {STATUS_OBJECT_NAME_NOT_FOUND, "OBJECT_NAME_NOT_FOUND", NULL},
  {STATUS_OBJECT_PATH_INVALID, "OBJECT_PATH_INVALID", NULL},
  {STATUS_OBJECT_PATH_NOT_FOUND, "OBJECT_PATH_NOT_FOUND", NULL},
  {STATUS_DFS_EXIT_PATH_FOUND, "DFS_EXIT_PATH_FOUND", NULL},
  {STATUS_PATH_SYNTAX_BAD, "PATH_SYNTAX_BAD", NULL},
  {STATUS_DATA_OVERRUN, "DATA_OVERRUN", NULL},
  {STATUS_DATA_LATE_ERROR, "DATA_LATE_ERROR", NULL},
  {STATUS_DATA_ERROR, "DATA_ERROR", NULL},
  {STATUS_CRC_ERROR, "CRC_ERROR", NULL},
  {STATUS_SHARING_VIOLATION, "SHARING_VIOLATION", NULL},
  {STATUS_QUOTA_EXCEEDED, "QUOTA_EXCEEDED", NULL},
  {STATUS_MUTANT_NOT_OWNED, "MUTANT_NOT_OWNED", NULL},
  {STATUS_SEMAPHORE_LIMIT_EXCEEDED, "SEMAPHORE_LIMIT_EXCEEDED", NULL},
  {STATUS_DISK_FULL, "DISK_FULL", NULL},
  {STATUS_LOCK_NOT_GRANTED, "LOCK_NOT_GRANTED", NULL},
  {STATUS_DEVICE_NOT_READY, "DEVICE_NOT_READY", NULL},
  {STATUS_IO_TIMEOUT, "IO_TIMEOUT", NULL},
  {STATUS_MEDIA_WRITE_PROTECTED, "MEDIA_WRITE_PROTECTED", NULL},
  {STATUS_NO_MEDIA_IN_DRIVE, "NO_MEDIA_IN_DRIVE", NULL},
  {STATUS_VERIFY_REQUIRED, "VERIFY_REQUIRED", NULL},
  {STATUS_UNRECOGNIZED_MEDIA, "UNRECOGNIZED_MEDIA", NULL},
  {STATUS_UNRECOGNIZED_VOLUME, "UNRECOGNIZED_VOLUME", NULL},
  {STATUS_FS_DRIVER_REQUIRED, "FS_DRIVER_REQUIRED", NULL},
  {STATUS_NOT_SUPPORTED, "NOT_SUPPORTED", NULL},
  {STATUS_DISK_OPERATION_FAILED, "DISK_OPERATION_FAILED", NULL},
  {STATUS_FS_QUERY_REQUIRED, "FS_QUERY_REQUIRED", NULL},
  {0, NULL, NULL}
};

/* FUNCTIONS *****************************************************************/

VOID 
DbgGetErrorText(NTSTATUS ErrorCode, PUNICODE_STRING ErrorText, ULONG Flags)
{
  int i;
  char TempBuf[255], NumBuf[32];
  ANSI_STRING AnsiString;

  TempBuf[0] = '\0';
  if (Flags & DBG_GET_SHOW_FACILITY)
    {
      if (NT_CUSTOMER(ErrorCode))
        {
          sprintf(TempBuf, 
                  "%%CUST-%s-", 
                  SeverityCodes[NT_SEVERITY(ErrorCode)]);
        }
      else 
        {
          for (i = 0; FacList[i].Name != NULL; i++)
            {
              if (FacList[i].Code == NT_FACILITY(ErrorCode))
                {
                  break;
                }
            }
          if (FacList[i].Name != NULL)
            {
              sprintf(TempBuf, "%%%s-%s-", 
                      FacList[i].Name, 
                      SeverityCodes[NT_SEVERITY(ErrorCode)]);
            }
          else
            {
              sprintf(TempBuf, "%%UNKNOWN-%s-", 
                      SeverityCodes[NT_SEVERITY(ErrorCode)]);
            }
        }
    }
  for (i = 0; ErrList[i].Name != NULL; i++)
    {
      if (ErrorCode == ErrList[i].Code)
        {
          break;
        }
    }
  if (ErrList[i].Name != NULL)
    {
      if (Flags & DBG_GET_SHOW_FACILITY)
        {
          strcat(TempBuf, ErrList[i].Name);
          strcat(TempBuf, " ");
        }
      if (ErrList[i].Text != NULL)
        {
          strcat(TempBuf, ErrList[i].Text);
        }
      else
        {
          strcat(TempBuf, ErrList[i].Name);
        }
    }
  else
    {
      if (Flags & DBG_GET_SHOW_FACILITY)
        {
          sprintf(NumBuf, "%08lx", ErrorCode);
          strcat(TempBuf, NumBuf);
          strcat(TempBuf, " ");
        }
      sprintf(NumBuf, "Unknown Message #%08lx", ErrorCode);
      strcat(TempBuf, NumBuf);
    }
  RtlInitAnsiString(&AnsiString, TempBuf);
  RtlAnsiStringToUnicodeString(ErrorText, &AnsiString, TRUE);
}

VOID 
DbgPrintErrorMessage(NTSTATUS ErrorCode)
{
  UNICODE_STRING ErrorText;

  DbgGetErrorText(ErrorCode, &ErrorText, 0xf);
  DbgPrint("%wZ\n", &ErrorText);
  RtlFreeUnicodeString(&ErrorText);
}


