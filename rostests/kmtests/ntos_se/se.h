/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Se helper declarations
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#ifndef _KMTEST_SE_H_
#define _KMTEST_SE_H_

/* FIXME: belongs in ntifs.h or something */
#define SEF_DACL_AUTO_INHERIT 1
#define SEF_SACL_AUTO_INHERIT 2
#define SEF_DEFAULT_DESCRIPTOR_FOR_OBJECT 4
#define SEF_AVOID_PRIVILEGE_CHECK 8
#define SEF_AVOID_OWNER_CHECK 16
#define SEF_DEFAULT_OWNER_FROM_PARENT 32
#define SEF_DEFAULT_GROUP_FROM_PARENT 64
#define SEF_MACL_NO_WRITE_UP 256
#define SEF_MACL_NO_READ_UP 512
#define SEF_MACL_NO_EXECUTE_UP 1024
#define SEF_AI_USE_EXTRA_PARAMS 2048
#define SEF_AVOID_OWNER_RESTRICTION 4096
#define SEF_MACL_VALID_FLAGS (SEF_MACL_NO_WRITE_UP | SEF_MACL_NO_READ_UP | SEF_MACL_NO_EXECUTE_UP)

NTSTATUS
RtlxAddAuditAccessAceEx(
    _Inout_ PACL Acl,
    _In_ ULONG Revision,
    _In_ ULONG Flags,
    _In_ ACCESS_MASK AccessMask,
    _In_ PSID Sid,
    _In_ BOOLEAN Success,
    _In_ BOOLEAN Failure);

#define NO_SIZE ((ULONG)-1)

#define CheckSid(Sid, SidSize, ExpectedSid) CheckSid_(Sid, SidSize, ExpectedSid, __FILE__, __LINE__)
#define CheckSid_(Sid, SidSize, ExpectedSid, file, line) CheckSid__(Sid, SidSize, ExpectedSid, file ":" KMT_STRINGIZE(line))
VOID
CheckSid__(
    _In_ PSID Sid,
    _In_ ULONG SidSize,
    _In_ PISID ExpectedSid,
    _In_ PCSTR FileAndLine);

VOID
VCheckAcl__(
    _In_ PACL Acl,
    _In_ ULONG AceCount,
    _In_ PCSTR FileAndLine,
    _In_ va_list Arguments);

#define CheckAcl(Acl, AceCount, ...) CheckAcl_(Acl, AceCount, __FILE__, __LINE__, ##__VA_ARGS__)
#define CheckAcl_(Acl, AceCount, file, line, ...) CheckAcl__(Acl, AceCount, file ":" KMT_STRINGIZE(line), ##__VA_ARGS__)
VOID
CheckAcl__(
    _In_ PACL Acl,
    _In_ ULONG AceCount,
    _In_ PCSTR FileAndLine,
    ...);

#endif /* !defined _KMTEST_SE_H_ */
