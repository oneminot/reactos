/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/ntos/ps.h
 * PURPOSE:      Process/thread declarations used by all the parts of the 
 *               system
 * PROGRAMMER:   David Welch <welch@cwcom.net>
 * UPDATE HISTORY: 
 *               27/06/00: Created
 */


#ifndef __INCLUDE_PS_H
#define __INCLUDE_PS_H

#define THREAD_ALL_ACCESS	(0x1f03ffL)
#define THREAD_DIRECT_IMPERSONATION	(512)
#define THREAD_GET_CONTEXT	(8)
#define THREAD_IMPERSONATE	(256)
#define THREAD_QUERY_INFORMATION	(64)
#define THREAD_SET_CONTEXT	(16)
#define THREAD_SET_INFORMATION	(32)
#define THREAD_SET_THREAD_TOKEN	(128)
#define THREAD_SUSPEND_RESUME	(2)
#define THREAD_TERMINATE	(1)

#define PROCESS_ALL_ACCESS	(0x1f0fffL)
#define PROCESS_CREATE_PROCESS	(128)
#define PROCESS_CREATE_THREAD	(2)
#define PROCESS_DUP_HANDLE	(64)
#define PROCESS_QUERY_INFORMATION	(1024)
#define PROCESS_SET_INFORMATION	(512)
#define PROCESS_TERMINATE	(1)
#define PROCESS_VM_OPERATION	(8)
#define PROCESS_VM_READ	(16)
#define PROCESS_VM_WRITE	(32)

#define THREAD_PRIORITY_ABOVE_NORMAL	(1)
#define THREAD_PRIORITY_BELOW_NORMAL	(-1)
#define THREAD_PRIORITY_HIGHEST	(2)
#define THREAD_PRIORITY_IDLE	(-15)
#define THREAD_PRIORITY_LOWEST	(-2)
#define THREAD_PRIORITY_NORMAL	(0)
#define THREAD_PRIORITY_TIME_CRITICAL	(15)
#define THREAD_PRIORITY_ERROR_RETURN	(2147483647)

/* CreateProcess */
#define CREATE_DEFAULT_ERROR_MODE	(67108864)
#define CREATE_NEW_CONSOLE	(16)
#define CREATE_NEW_PROCESS_GROUP	(512)
#define CREATE_SEPARATE_WOW_VDM	(2048)
#define CREATE_SUSPENDED	(4)
#define CREATE_UNICODE_ENVIRONMENT	(1024)
#define DEBUG_PROCESS	(1)
#define DEBUG_ONLY_THIS_PROCESS	(2)
#define DETACHED_PROCESS	(8)
#define HIGH_PRIORITY_CLASS	(128)
#define IDLE_PRIORITY_CLASS	(64)
#define NORMAL_PRIORITY_CLASS	(32)
#define REALTIME_PRIORITY_CLASS	(256)


#endif /* __INCLUDE_PS_H */
