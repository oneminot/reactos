#ifndef __INCLUDE_DDK_STATUS_H
#define __INCLUDE_DDK_STATUS_H

#define NT_SUCCESS(StatCode)  ((NTSTATUS)(StatCode) >= 0)

/*
 * Possible status codes
 * FIXME: These may not be the actual values used by NT
 */
enum
{
   STATUS_SUCCESS = 0x0,
   STATUS_INSUFFICIENT_RESOURCES = 0x80000000,
   STATUS_OBJECT_NAME_EXISTS,
   STATUS_OBJECT_NAME_COLLISION,
//   STATUS_DATATYPE_MISALIGNMENT,
   STATUS_CTL_FILE_NOT_SUPPORTED,
//   STATUS_ACCESS_VIOLATION,
   STATUS_PORT_ALREADY_SET,
   STATUS_SECTION_NOT_IMAGE,
   STATUS_BAD_WORKING_SET_LIMIT,
   STATUS_INCOMPATIBLE_FILE_MAP,
   STATUS_HANDLE_NOT_WAITABLE,
   STATUS_PORT_DISCONNECTED,
   STATUS_NOT_LOCKED,
   STATUS_NOT_MAPPED_VIEW,
   STATUS_UNABLE_TO_FREE_VM,
   STATUS_UNABLE_TO_DELETE_SECTION,
   STATUS_MORE_PROCESSING_REQUIRED,
   STATUS_INVALID_CID,
   STATUS_BAD_INITIAL_STACK,
   STATUS_INVALID_VOLUME_LABEL,
   STATUS_SECTION_NOT_EXTENDED,
   STATUS_NOT_MAPPED_DATA,
   STATUS_INFO_LENGTH_MISMATCH,
   STATUS_INVALID_INFO_CLASS,
   STATUS_SUSPEND_COUNT_EXCEEDED,
   STATUS_NOTIFY_ENUM_DIR,
   STATUS_REGISTRY_RECOVERED,
   STATUS_REGISTRY_IO_FAILED,
   STATUS_KEY_DELETED,
   STATUS_NO_LOG_SPACE,
   STATUS_KEY_HAS_CHILDREN,
   STATUS_CHILD_MUST_BE_VOLATILE,
   STATUS_REGISTRY_CORRUPT,
   STATUS_DLL_NOT_FOUND,
   STATUS_DLL_INIT_FAILED,
   STATUS_ORDINAL_NOT_FOUND,
   STATUS_ENTRYPOINT_NOT_FOUND,
//   STATUS_PENDING,
   STATUS_MORE_ENTRIES,
//   STATUS_INTEGER_OVERFLOW,
   STATUS_BUFFER_OVERFLOW,
   STATUS_NO_MORE_FILES,
   STATUS_NO_INHERITANCE,
   STATUS_NO_MORE_EAS,
   STATUS_NO_MORE_ENTRIES,
   STATUS_GUIDS_EXHAUSTED,
   STATUS_AGENTS_EXHAUSTED,
   STATUS_UNSUCCESSFUL,
   STATUS_NOT_IMPLEMENTED,
   STATUS_ILLEGAL_FUNCTION,
//   STATUS_IN_PAGE_ERROR,
   STATUS_PAGEFILE_QUOTA,
   STATUS_COMMITMENT_LIMIT,
   STATUS_SECTION_TOO_BIG,
   RPC_NT_SS_IN_NULL_CONTEXT,
   RPC_NT_INVALID_BINDING,
//   STATUS_INVALID_HANDLE,
   STATUS_OBJECT_FILE_MISMATCH,
   STATUS_FILE_CLOSED,
   STATUS_INVALID_PORT_HANDLE,
   STATUS_NOT_COMMITTED,
   STATUS_INVALID_PARAMETER,
   STATUS_INVALID_PARAMETER_1,
   STATUS_INVALID_PARAMETER_2,
   STATUS_INVALID_PARAMETER_3,
   STATUS_INVALID_PARAMETER_4,
   STATUS_INVALID_PARAMETER_5,
   STATUS_INVALID_PARAMETER_6,
   STATUS_INVALID_PARAMETER_7,
   STATUS_INVALID_PARAMETER_8,
   STATUS_INVALID_PARAMETER_9,
   STATUS_INVALID_PARAMETER_10,
   STATUS_INVALID_PARAMETER_11,
   STATUS_INVALID_PARAMETER_12,
   STATUS_INVALID_PARAMETER_MAX,
   STATUS_INVALID_PAGE_PROTECTION,
   STATUS_RESOURCE_DATA_NOT_FOUND,
   STATUS_RESOURCE_TYPE_NOT_FOUND,
   STATUS_RESOURCE_NAME_NOT_FOUND,
   STATUS_RESOURCE_LANG_NOT_FOUND,
   STATUS_NO_SUCH_DEVICE,
   STATUS_NO_SUCH_FILE,
   STATUS_INVALID_DEVICE_REQUEST,
   STATUS_END_OF_FILE,
   STATUS_FILE_FORCED_CLOSED,
   STATUS_WRONG_VOLUME,
   STATUS_NO_MEDIA,
   STATUS_NO_MEDIA_IN_DEVICE,
   STATUS_NONEXISTENT_SECTOR,
   STATUS_WORKING_SET_QUOTA,
//   STATUS_NO_MEMORY,
   STATUS_CONFLICTING_ADDRESS,
   STATUS_INVALID_SYSTEM_SERVICE,
   STATUS_THREAD_IS_TERMINATING,
   STATUS_PROCESS_IS_TERMINATING,
   STATUS_INVALID_LOCK_SEQUENCE,
   STATUS_INVALID_VIEW_SIZE,
   STATUS_ALREADY_COMMITTED,
   STATUS_ACCESS_DENIED,
   STATUS_FILE_IS_A_DIRECTORY,
   STATUS_CANNOT_DELETE,
   STATUS_INVALID_COMPUTER_NAME,
   STATUS_FILE_DELETED,
   STATUS_DELETE_PENDING,
   STATUS_PORT_CONNECTION_REFUSED,
   STATUS_NO_SUCH_PRIVILEGE,
   STATUS_PRIVILEGE_NOT_HELD,
   STATUS_CANNOT_IMPERSONATE,
   STATUS_LOGON_FAILURE,
   STATUS_ACCOUNT_RESTRICTION,
   STATUS_INVALID_LOGON_HOURS,
   STATUS_INVALID_WORKSTATION,
   STATUS_BUFFER_TOO_SMALL,
   STATUS_UNABLE_TO_DECOMMIT_VM,
   STATUS_DISK_CORRUPT_ERROR,
   STATUS_OBJECT_NAME_INVALID,
   STATUS_OBJECT_NAME_NOT_FOUND,
//   STATUS_OBJECT_NAME_COLLISION,
   STATUS_OBJECT_PATH_INVALID,
   STATUS_OBJECT_PATH_NOT_FOUND,
   STATUS_DFS_EXIT_PATH_FOUND,
   STATUS_OBJECT_PATH_SYNTAX_BAD,
   STATUS_DATA_OVERRUN,
   STATUS_DATA_LATE_ERROR,
   STATUS_DATA_ERROR,
   STATUS_CRC_ERROR,
   STATUS_SHARING_VIOLATION,
   STATUS_QUOTA_EXCEEDED,
   STATUS_MUTANT_NOT_OWNED,
   STATUS_SEMAPHORE_LIMIT_EXCEEDED,
   STATUS_DISK_FULL,
   STATUS_LOCK_NOT_GRANTED,
     
   STATUS_DEVICE_NOT_READY,
   STATUS_IO_TIMEOUT,
   STATUS_MEDIA_WRITE_PROTECTED,
   STATUS_NO_MEDIA_IN_DRIVE,
   STATUS_VERIFY_REQUIRED,
   STATUS_UNRECOGNIZED_MEDIA,
   STATUS_UNRECOGNIZED_VOLUME,
//   STATUS_WRONG_VOLUME,
   STATUS_FS_DRIVER_REQUIRED,
   STATUS_NOT_SUPPORTED = 9999,
   STATUS_DISK_OPERATION_FAILED,
     
     /*
      * Reactos codes
      */
     STATUS_FS_QUERY_REQUIRED,
};

#endif /* __INCLUDE_DDK_STATUS_H */
