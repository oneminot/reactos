/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/msv1_0/msv1_0.c
 * PURPOSE:     Main file
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */

/* INCLUDES ****************************************************************/

#include "msv1_0.h"

WINE_DEFAULT_DEBUG_CHANNEL(msv1_0);


/* GLOBALS *****************************************************************/

LSA_DISPATCH_TABLE DispatchTable;


/* FUNCTIONS ***************************************************************/

static
NTSTATUS
GetDomainSid(PRPC_SID *Sid)
{
    LSAPR_HANDLE PolicyHandle = NULL;
    PLSAPR_POLICY_INFORMATION PolicyInfo = NULL;
    ULONG Length = 0;
    NTSTATUS Status;

    Status = LsaIOpenPolicyTrusted(&PolicyHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("LsaIOpenPolicyTrusted() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    Status = LsarQueryInformationPolicy(PolicyHandle,
                                        PolicyAccountDomainInformation,
                                        &PolicyInfo);
    if (!NT_SUCCESS(Status))
    {
        TRACE("LsarQueryInformationPolicy() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    Length = RtlLengthSid(PolicyInfo->PolicyAccountDomainInfo.Sid);

    *Sid = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length);
    if (*Sid == NULL)
    {
        ERR("Failed to allocate SID\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    memcpy(*Sid, PolicyInfo->PolicyAccountDomainInfo.Sid, Length);

done:
    if (PolicyInfo != NULL)
        LsaIFree_LSAPR_POLICY_INFORMATION(PolicyAccountDomainInformation,
                                          PolicyInfo);

    if (PolicyHandle != NULL)
        LsarClose(&PolicyHandle);

    return Status;
}


static
NTSTATUS
BuildInteractiveProfileBuffer(IN PLSA_CLIENT_REQUEST ClientRequest,
                              IN PSAMPR_USER_INFO_BUFFER UserInfo,
                              IN PUNICODE_STRING LogonServer,
                              OUT PMSV1_0_INTERACTIVE_PROFILE *ProfileBuffer,
                              OUT PULONG ProfileBufferLength)
{
    PMSV1_0_INTERACTIVE_PROFILE LocalBuffer = NULL;
    PVOID ClientBaseAddress = NULL;
    LPWSTR Ptr;
    ULONG BufferLength;
    NTSTATUS Status;

    *ProfileBuffer = NULL;
    *ProfileBufferLength = 0;

    BufferLength = sizeof(MSV1_0_INTERACTIVE_PROFILE) +
                   UserInfo->All.FullName.Length + sizeof(WCHAR) +
                   UserInfo->All.HomeDirectory.Length + sizeof(WCHAR) +
                   UserInfo->All.HomeDirectoryDrive.Length + sizeof(WCHAR) +
                   UserInfo->All.ScriptPath.Length + sizeof(WCHAR) +
                   UserInfo->All.ProfilePath.Length + sizeof(WCHAR) +
                   LogonServer->Length + sizeof(WCHAR);

    LocalBuffer = DispatchTable.AllocateLsaHeap(BufferLength);
    if (LocalBuffer == NULL)
    {
        TRACE("Failed to allocate the local buffer!\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    Status = DispatchTable.AllocateClientBuffer(ClientRequest,
                                                BufferLength,
                                                &ClientBaseAddress);
    if (!NT_SUCCESS(Status))
    {
        TRACE("DispatchTable.AllocateClientBuffer failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    TRACE("ClientBaseAddress: %p\n", ClientBaseAddress);

    Ptr = (LPWSTR)((ULONG_PTR)LocalBuffer + sizeof(MSV1_0_INTERACTIVE_PROFILE));

    LocalBuffer->MessageType = MsV1_0InteractiveProfile;
    LocalBuffer->LogonCount = UserInfo->All.LogonCount;
    LocalBuffer->BadPasswordCount = UserInfo->All.BadPasswordCount;
//  LARGE_INTEGER              LogonTime;
//  LARGE_INTEGER              LogoffTime;
//  LARGE_INTEGER              KickOffTime;
//  LARGE_INTEGER              PasswordLastSet;
//  LARGE_INTEGER              PasswordCanChange;
//  LARGE_INTEGER              PasswordMustChange;

    LocalBuffer->LogonScript.Length = UserInfo->All.ScriptPath.Length;
    LocalBuffer->LogonScript.MaximumLength = UserInfo->All.ScriptPath.Length + sizeof(WCHAR);
    LocalBuffer->LogonScript.Buffer = (LPWSTR)((ULONG_PTR)ClientBaseAddress + (ULONG_PTR)Ptr - (ULONG_PTR)LocalBuffer);
    memcpy(Ptr,
           UserInfo->All.ScriptPath.Buffer,
           UserInfo->All.ScriptPath.Length);

    Ptr = (LPWSTR)((ULONG_PTR)Ptr + LocalBuffer->LogonScript.MaximumLength);

    LocalBuffer->HomeDirectory.Length = UserInfo->All.HomeDirectory.Length;
    LocalBuffer->HomeDirectory.MaximumLength = UserInfo->All.HomeDirectory.Length + sizeof(WCHAR);
    LocalBuffer->HomeDirectory.Buffer = (LPWSTR)((ULONG_PTR)ClientBaseAddress + (ULONG_PTR)Ptr - (ULONG_PTR)LocalBuffer);
    memcpy(Ptr,
           UserInfo->All.HomeDirectory.Buffer,
           UserInfo->All.HomeDirectory.Length);

    Ptr = (LPWSTR)((ULONG_PTR)Ptr + LocalBuffer->HomeDirectory.MaximumLength);

    LocalBuffer->FullName.Length = UserInfo->All.FullName.Length;
    LocalBuffer->FullName.MaximumLength = UserInfo->All.FullName.Length + sizeof(WCHAR);
    LocalBuffer->FullName.Buffer = (LPWSTR)((ULONG_PTR)ClientBaseAddress + (ULONG_PTR)Ptr - (ULONG_PTR)LocalBuffer);
    memcpy(Ptr,
           UserInfo->All.FullName.Buffer,
           UserInfo->All.FullName.Length);
    TRACE("FullName.Buffer: %p\n", LocalBuffer->FullName.Buffer);

    Ptr = (LPWSTR)((ULONG_PTR)Ptr + LocalBuffer->FullName.MaximumLength);

    LocalBuffer->ProfilePath.Length = UserInfo->All.ProfilePath.Length;
    LocalBuffer->ProfilePath.MaximumLength = UserInfo->All.ProfilePath.Length + sizeof(WCHAR);
    LocalBuffer->ProfilePath.Buffer = (LPWSTR)((ULONG_PTR)ClientBaseAddress + (ULONG_PTR)Ptr - (ULONG_PTR)LocalBuffer);
    memcpy(Ptr,
           UserInfo->All.ProfilePath.Buffer,
           UserInfo->All.ProfilePath.Length);

    Ptr = (LPWSTR)((ULONG_PTR)Ptr + LocalBuffer->ProfilePath.MaximumLength);

    LocalBuffer->HomeDirectoryDrive.Length = UserInfo->All.HomeDirectoryDrive.Length;
    LocalBuffer->HomeDirectoryDrive.MaximumLength = UserInfo->All.HomeDirectoryDrive.Length + sizeof(WCHAR);
    LocalBuffer->HomeDirectoryDrive.Buffer = (LPWSTR)((ULONG_PTR)ClientBaseAddress + (ULONG_PTR)Ptr - (ULONG_PTR)LocalBuffer);
    memcpy(Ptr,
           UserInfo->All.HomeDirectoryDrive.Buffer,
           UserInfo->All.HomeDirectoryDrive.Length);

    Ptr = (LPWSTR)((ULONG_PTR)Ptr + LocalBuffer->HomeDirectoryDrive.MaximumLength);

    LocalBuffer->LogonServer.Length = LogonServer->Length;
    LocalBuffer->LogonServer.MaximumLength = LogonServer->Length + sizeof(WCHAR);
    LocalBuffer->LogonServer.Buffer = (LPWSTR)((ULONG_PTR)ClientBaseAddress + (ULONG_PTR)Ptr - (ULONG_PTR)LocalBuffer);;
    memcpy(Ptr,
           LogonServer->Buffer,
           LogonServer->Length);

    LocalBuffer->UserFlags = 0;

    Status = DispatchTable.CopyToClientBuffer(ClientRequest,
                                              BufferLength,
                                              ClientBaseAddress,
                                              LocalBuffer);
    if (!NT_SUCCESS(Status))
    {
        TRACE("DispatchTable.CopyToClientBuffer failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    *ProfileBuffer = (PMSV1_0_INTERACTIVE_PROFILE)ClientBaseAddress;
    *ProfileBufferLength = BufferLength;

done:
    if (LocalBuffer != NULL)
        DispatchTable.FreeLsaHeap(LocalBuffer);

    if (!NT_SUCCESS(Status))
    {
        if (ClientBaseAddress != NULL)
            DispatchTable.FreeClientBuffer(ClientRequest,
                                           ClientBaseAddress);
    }

    return Status;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaApCallPackage(IN PLSA_CLIENT_REQUEST ClientRequest,
                 IN PVOID ProtocolSubmitBuffer,
                 IN PVOID ClientBufferBase,
                 IN ULONG SubmitBufferLength,
                 OUT PVOID *ProtocolReturnBuffer,
                 OUT PULONG ReturnBufferLength,
                 OUT PNTSTATUS ProtocolStatus)
{
    TRACE("()\n");
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaApCallPackagePassthrough(IN PLSA_CLIENT_REQUEST ClientRequest,
                            IN PVOID ProtocolSubmitBuffer,
                            IN PVOID ClientBufferBase,
                            IN ULONG SubmitBufferLength,
                            OUT PVOID *ProtocolReturnBuffer,
                            OUT PULONG ReturnBufferLength,
                            OUT PNTSTATUS ProtocolStatus)
{
    TRACE("()\n");
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaApCallPackageUntrusted(IN PLSA_CLIENT_REQUEST ClientRequest,
                          IN PVOID ProtocolSubmitBuffer,
                          IN PVOID ClientBufferBase,
                          IN ULONG SubmitBufferLength,
                          OUT PVOID *ProtocolReturnBuffer,
                          OUT PULONG ReturnBufferLength,
                          OUT PNTSTATUS ProtocolStatus)
{
    TRACE("()\n");
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaApInitializePackage(IN ULONG AuthenticationPackageId,
                       IN PLSA_DISPATCH_TABLE LsaDispatchTable,
                       IN PLSA_STRING Database OPTIONAL,
                       IN PLSA_STRING Confidentiality OPTIONAL,
                       OUT PLSA_STRING *AuthenticationPackageName)
{
    PANSI_STRING NameString;
    PCHAR NameBuffer;

    TRACE("(%lu %p %p %p %p)\n",
          AuthenticationPackageId, LsaDispatchTable, Database,
          Confidentiality, AuthenticationPackageName);

    /* Get the dispatch table entries */
    DispatchTable.AllocateLsaHeap = LsaDispatchTable->AllocateLsaHeap;
    DispatchTable.FreeLsaHeap = LsaDispatchTable->FreeLsaHeap;
    DispatchTable.AllocateClientBuffer = LsaDispatchTable->AllocateClientBuffer;
    DispatchTable.FreeClientBuffer = LsaDispatchTable->FreeClientBuffer;
    DispatchTable.CopyToClientBuffer = LsaDispatchTable->CopyToClientBuffer;
    DispatchTable.CopyFromClientBuffer = LsaDispatchTable->CopyFromClientBuffer;


    /* Return the package name */
    NameString = DispatchTable.AllocateLsaHeap(sizeof(LSA_STRING));
    if (NameString == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    NameBuffer = DispatchTable.AllocateLsaHeap(sizeof(MSV1_0_PACKAGE_NAME));
    if (NameBuffer == NULL)
    {
        DispatchTable.FreeLsaHeap(NameString);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    strcpy(NameBuffer, MSV1_0_PACKAGE_NAME);

    RtlInitAnsiString(NameString, NameBuffer);

    *AuthenticationPackageName = (PLSA_STRING)NameString;

    return STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
VOID
NTAPI
LsaApLogonTerminated(IN PLUID LogonId)
{
    TRACE("()\n");
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaApLogonUser(IN PLSA_CLIENT_REQUEST ClientRequest,
               IN SECURITY_LOGON_TYPE LogonType,
               IN PVOID AuthenticationInformation,
               IN PVOID ClientAuthenticationBase,
               IN ULONG AuthenticationInformationLength,
               OUT PVOID *ProfileBuffer,
               OUT PULONG ProfileBufferLength,
               OUT PLUID LogonId,
               OUT PNTSTATUS SubStatus,
               OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
               OUT PVOID *TokenInformation,
               OUT PLSA_UNICODE_STRING *AccountName,
               OUT PLSA_UNICODE_STRING *AuthenticatingAuthority)
{
    PMSV1_0_INTERACTIVE_LOGON LogonInfo;

    SAMPR_HANDLE ServerHandle = NULL;
    SAMPR_HANDLE DomainHandle = NULL;
    SAMPR_HANDLE UserHandle = NULL;
    PRPC_SID AccountDomainSid = NULL;
    RPC_UNICODE_STRING Names[1];
    SAMPR_ULONG_ARRAY RelativeIds = {0, NULL};
    SAMPR_ULONG_ARRAY Use = {0, NULL};
    PSAMPR_USER_INFO_BUFFER UserInfo = NULL;
    UNICODE_STRING LogonServer;
    NTSTATUS Status;

    TRACE("()\n");

    TRACE("LogonType: %lu\n", LogonType);
    TRACE("AuthenticationInformation: %p\n", AuthenticationInformation);
    TRACE("AuthenticationInformationLength: %lu\n", AuthenticationInformationLength);


    *ProfileBuffer = NULL;
    *ProfileBufferLength = 0;
    *SubStatus = STATUS_SUCCESS;

    if (LogonType == Interactive ||
        LogonType == Batch ||
        LogonType == Service)
    {
        ULONG_PTR PtrOffset;

        LogonInfo = (PMSV1_0_INTERACTIVE_LOGON)AuthenticationInformation;

        /* Fix-up pointers in the authentication info */
        PtrOffset = (ULONG_PTR)AuthenticationInformation - (ULONG_PTR)ClientAuthenticationBase;

        LogonInfo->LogonDomainName.Buffer = (PWSTR)((ULONG_PTR)LogonInfo->LogonDomainName.Buffer + PtrOffset);
        LogonInfo->UserName.Buffer = (PWSTR)((ULONG_PTR)LogonInfo->UserName.Buffer + PtrOffset);
        LogonInfo->Password.Buffer = (PWSTR)((ULONG_PTR)LogonInfo->Password.Buffer + PtrOffset);

        TRACE("Domain: %S\n", LogonInfo->LogonDomainName.Buffer);
        TRACE("User: %S\n", LogonInfo->UserName.Buffer);
        TRACE("Password: %S\n", LogonInfo->Password.Buffer);

        RtlInitUnicodeString(&LogonServer, L"Testserver");
    }
    else
    {
        FIXME("LogonType %lu is not supported yet!\n", LogonType);
        return STATUS_NOT_IMPLEMENTED;
    }

    Status = GetDomainSid(&AccountDomainSid);
    if (!NT_SUCCESS(Status))
    {
        TRACE("GetDomainSid() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Connect to the SAM server */
    Status = SamIConnect(NULL,
                         &ServerHandle,
                         SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                         TRUE);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamIConnect() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Open the account domain */
    Status = SamrOpenDomain(ServerHandle,
                            DOMAIN_LOOKUP,
                            AccountDomainSid,
                            &DomainHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamrOpenDomain failed (Status %08lx)\n", Status);
        goto done;
    }

    Names[0].Length = LogonInfo->UserName.Length;
    Names[0].MaximumLength = LogonInfo->UserName.MaximumLength;
    Names[0].Buffer = LogonInfo->UserName.Buffer;

    /* Try to get the RID for the user name */
    Status = SamrLookupNamesInDomain(DomainHandle,
                                     1,
                                     Names,
                                     &RelativeIds,
                                     &Use);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamrLookupNamesInDomain failed (Status %08lx)\n", Status);
        Status = STATUS_NO_SUCH_USER;
        goto done;
    }

    /* Fail, if it is not a user account */
    if (Use.Element[0] != SidTypeUser)
    {
        TRACE("Account is not a user account!\n");
        Status = STATUS_NO_SUCH_USER;
        goto done;
    }

    /* Open the user object */
    Status = SamrOpenUser(DomainHandle,
                          USER_READ_GENERAL | USER_READ_LOGON |
                          USER_READ_ACCOUNT | USER_READ_PREFERENCES, /* FIXME */
                          RelativeIds.Element[0],
                          &UserHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamrOpenUser failed (Status %08lx)\n", Status);
        goto done;
    }

    Status = SamrQueryInformationUser(UserHandle,
                                      UserAllInformation,
                                      &UserInfo);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamrQueryInformationUser failed (Status %08lx)\n", Status);
        goto done;
    }


    TRACE("UserName: %S\n", UserInfo->All.UserName.Buffer);

    /* FIXME: Check restrictions */

    /* FIXME: Check the password */
    if ((UserInfo->All.UserAccountControl & USER_PASSWORD_NOT_REQUIRED) == 0)
    {
        FIXME("Must check the password!\n");

    }

    /* Return logon information */

    /* Create and return a new logon id */
    Status = NtAllocateLocallyUniqueId(LogonId);
    if (!NT_SUCCESS(Status))
    {
        TRACE("NtAllocateLocallyUniqueId failed (Status %08lx)\n", Status);
        goto done;
    }

    /* Build and fill the interactve profile buffer */
    Status = BuildInteractiveProfileBuffer(ClientRequest,
                                           UserInfo,
                                           &LogonServer,
                                           (PMSV1_0_INTERACTIVE_PROFILE*)ProfileBuffer,
                                           ProfileBufferLength);
    if (!NT_SUCCESS(Status))
    {
        TRACE("BuildInteractiveProfileBuffer failed (Status %08lx)\n", Status);
        goto done;
    }

    /* Return the token information type */
    *TokenInformationType = LsaTokenInformationV1;

done:
    if (!NT_SUCCESS(Status))
    {
        if (*ProfileBuffer != NULL)
        {
            DispatchTable.FreeClientBuffer(ClientRequest,
                                           *ProfileBuffer);
            *ProfileBuffer = NULL;
        }
    }

    if (UserHandle != NULL)
        SamrCloseHandle(&UserHandle);

    SamIFree_SAMPR_USER_INFO_BUFFER(UserInfo,
                                    UserAllInformation);
    SamIFree_SAMPR_ULONG_ARRAY(&RelativeIds);
    SamIFree_SAMPR_ULONG_ARRAY(&Use);

    if (DomainHandle != NULL)
        SamrCloseHandle(&DomainHandle);

    if (ServerHandle != NULL)
        SamrCloseHandle(&ServerHandle);

    if (AccountDomainSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AccountDomainSid);

    TRACE("LsaApLogonUser done (Status %08lx)\n", Status);

    return Status;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaApLogonUserEx(IN PLSA_CLIENT_REQUEST ClientRequest,
                 IN SECURITY_LOGON_TYPE LogonType,
                 IN PVOID AuthenticationInformation,
                 IN PVOID ClientAuthenticationBase,
                 IN ULONG AuthenticationInformationLength,
                 OUT PVOID *ProfileBuffer,
                 OUT PULONG ProfileBufferLength,
                 OUT PLUID LogonId,
                 OUT PNTSTATUS SubStatus,
                 OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
                 OUT PVOID *TokenInformation,
                 OUT PUNICODE_STRING *AccountName,
                 OUT PUNICODE_STRING *AuthenticatingAuthority,
                 OUT PUNICODE_STRING *MachineName)
{
    TRACE("()\n");

    TRACE("LogonType: %lu\n", LogonType);
    TRACE("AuthenticationInformation: %p\n", AuthenticationInformation);
    TRACE("AuthenticationInformationLength: %lu\n", AuthenticationInformationLength);

    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaApLogonUserEx2(IN PLSA_CLIENT_REQUEST ClientRequest,
                  IN SECURITY_LOGON_TYPE LogonType,
                  IN PVOID ProtocolSubmitBuffer,
                  IN PVOID ClientBufferBase,
                  IN ULONG SubmitBufferSize,
                  OUT PVOID *ProfileBuffer,
                  OUT PULONG ProfileBufferSize,
                  OUT PLUID LogonId,
                  OUT PNTSTATUS SubStatus,
                  OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
                  OUT PVOID *TokenInformation,
                  OUT PUNICODE_STRING *AccountName,
                  OUT PUNICODE_STRING *AuthenticatingAuthority,
                  OUT PUNICODE_STRING *MachineName,
                  OUT PSECPKG_PRIMARY_CRED PrimaryCredentials,
                  OUT PSECPKG_SUPPLEMENTAL_CRED_ARRAY *SupplementalCredentials)
{
    TRACE("()\n");

    TRACE("LogonType: %lu\n", LogonType);
    TRACE("ProtocolSubmitBuffer: %p\n", ProtocolSubmitBuffer);
    TRACE("SubmitBufferSize: %lu\n", SubmitBufferSize);


    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
