/*
 * ntlsa.h
 *
 * This file is part of the ReactOS PSDK package.
 *
 * Contributors:
 *   Created by Eric Kohl.
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef _NTLSA_
#define _NTLSA_

#ifdef __cplusplus
extern "C" {
#endif

#define ACCOUNT_VIEW 1
#define ACCOUNT_ADJUST_PRIVILEGES 2
#define ACCOUNT_ADJUST_QUOTAS 4
#define ACCOUNT_ADJUST_SYSTEM_ACCESS 8

#define SECRET_SET_VALUE 1
#define SECRET_QUERY_VALUE 2

/* System Access Flags */
#define SECURITY_ACCESS_INTERACTIVE_LOGON             0x00000001
#define SECURITY_ACCESS_NETWORK_LOGON                 0x00000002
#define SECURITY_ACCESS_BATCH_LOGON                   0x00000004
#define SECURITY_ACCESS_SERVICE_LOGON                 0x00000010
#define SECURITY_ACCESS_PROXY_LOGON                   0x00000020
#define SECURITY_ACCESS_DENY_INTERACTIVE_LOGON        0x00000040
#define SECURITY_ACCESS_DENY_NETWORK_LOGON            0x00000080
#define SECURITY_ACCESS_DENY_BATCH_LOGON              0x00000100
#define SECURITY_ACCESS_DENY_SERVICE_LOGON            0x00000200
#define SECURITY_ACCESS_REMOTE_INTERACTIVE_LOGON      0x00000400
#define SECURITY_ACCESS_DENY_REMOTE_INTERACTIVE_LOGON 0x00000800


#ifdef __cplusplus
}
#endif


#endif /* _NTLSA_ */
