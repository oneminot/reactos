/* INCLUDES ****************************************************************/

#define WIN32_NO_STATUS
#include <windows.h>
#include <ntsecapi.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#include "lsa_s.h"

#include <wine/debug.h>

typedef struct _LSA_DB_HANDLE
{
    ULONG Signature;
    ULONG Type;
    LONG RefCount;
    ACCESS_MASK AccessGranted;
} LSA_DB_HANDLE, *PLSA_DB_HANDLE;

#define LSAP_DB_SIGNATURE 0x12345678

static RTL_CRITICAL_SECTION PolicyHandleTableLock;

WINE_DEFAULT_DEBUG_CHANNEL(lsasrv);


/* FUNCTIONS ***************************************************************/

static LSAPR_HANDLE
LsapCreateDbHandle(ULONG Type)
{
    PLSA_DB_HANDLE DbHandle;

//    RtlEnterCriticalSection(&PolicyHandleTableLock);

    DbHandle = (PLSA_DB_HANDLE)RtlAllocateHeap(RtlGetProcessHeap(),
                                               0,
                                               sizeof(LSA_DB_HANDLE));
    if (DbHandle != NULL)
    {
        DbHandle->Signature = LSAP_DB_SIGNATURE;
        DbHandle->RefCount = 1;
        DbHandle->Type = Type;
    }

//    RtlLeaveCriticalSection(&PolicyHandleTableLock);

    return (LSAPR_HANDLE)DbHandle;
}


static BOOL
LsapValidateDbHandle(LSAPR_HANDLE Handle)
{
    PLSA_DB_HANDLE DbHandle = (PLSA_DB_HANDLE)Handle;
    BOOL bValid = FALSE;

    _SEH2_TRY
    {
        if (DbHandle->Signature == LSAP_DB_SIGNATURE)
            bValid = TRUE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        bValid = FALSE;
    }
    _SEH2_END;


    return bValid;
}




VOID
LsarStartRpcServer(VOID)
{
    RPC_STATUS Status;

    RtlInitializeCriticalSection(&PolicyHandleTableLock);

    TRACE("LsarStartRpcServer() called");

    Status = RpcServerUseProtseqEpW(L"ncacn_np",
                                    10,
                                    L"\\pipe\\lsarpc",
                                    NULL);
    if (Status != RPC_S_OK)
    {
        WARN("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
        return;
    }

    Status = RpcServerRegisterIf(lsarpc_v0_0_s_ifspec,
                                 NULL,
                                 NULL);
    if (Status != RPC_S_OK)
    {
        WARN("RpcServerRegisterIf() failed (Status %lx)\n", Status);
        return;
    }

    Status = RpcServerListen(1, 20, TRUE);
    if (Status != RPC_S_OK)
    {
        WARN("RpcServerListen() failed (Status %lx)\n", Status);
        return;
    }

    TRACE("LsarStartRpcServer() done\n");
}


void __RPC_USER LSAPR_HANDLE_rundown(LSAPR_HANDLE hHandle)
{

}


/* Function 0 */
NTSTATUS LsarClose(
    LSAPR_HANDLE *ObjectHandle)
{
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("0x%p\n", ObjectHandle);

//    RtlEnterCriticalSection(&PolicyHandleTableLock);

    if (LsapValidateDbHandle(*ObjectHandle))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, *ObjectHandle);
        *ObjectHandle = NULL;
    }
    else
        Status = STATUS_INVALID_HANDLE;

//    RtlLeaveCriticalSection(&PolicyHandleTableLock);

    return Status;
}


/* Function 1 */
NTSTATUS LsarDelete(
    LSAPR_HANDLE ObjectHandle)
{
    /* Deprecated */
    return STATUS_NOT_SUPPORTED;
}


/* Function 2 */
NTSTATUS LsarEnumeratePrivileges(
    LSAPR_HANDLE PolicyHandle,
    DWORD *EnumerationContext,
    PLSAPR_PRIVILEGE_ENUM_BUFFER EnumerationBuffer,
    DWORD PreferedMaximumLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 3 */
NTSTATUS LsarQuerySecurityObject(
    LSAPR_HANDLE ObjectHandle,
    SECURITY_INFORMATION SecurityInformation,
    PLSAPR_SR_SECURITY_DESCRIPTOR *SecurityDescriptor)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 4 */
NTSTATUS LsarSetSecurityObject(
    LSAPR_HANDLE ObjectHandle,
    SECURITY_INFORMATION SecurityInformation,
    PLSAPR_SR_SECURITY_DESCRIPTOR SecurityDescriptor)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 5 */
NTSTATUS LsarChangePassword(
    handle_t IDL_handle,
    PRPC_UNICODE_STRING String1,
    PRPC_UNICODE_STRING String2,
    PRPC_UNICODE_STRING String3,
    PRPC_UNICODE_STRING String4,
    PRPC_UNICODE_STRING String5)
{
    /* Deprecated */
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 6 */
NTSTATUS LsarOpenPolicy(
    LPWSTR SystemName,
    PLSAPR_OBJECT_ATTRIBUTES ObjectAttributes,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *PolicyHandle)
{
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("LsarOpenPolicy called!\n");

    RtlEnterCriticalSection(&PolicyHandleTableLock);

    *PolicyHandle = LsapCreateDbHandle(0);
    if (*PolicyHandle == NULL)
        Status = STATUS_INSUFFICIENT_RESOURCES;

    RtlLeaveCriticalSection(&PolicyHandleTableLock);

    TRACE("LsarOpenPolicy done!\n");

    return Status;
}


/* Function 7 */
NTSTATUS LsarQueryInformationPolicy(
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    PLSAPR_POLICY_INFORMATION *PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 8 */
NTSTATUS LsarSetInformationPolicy(
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    PLSAPR_POLICY_INFORMATION PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 9 */
NTSTATUS LsarClearAuditLog(
    LSAPR_HANDLE ObjectHandle)
{
    /* Deprecated */
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 10 */
NTSTATUS LsarCreateAccount(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID AccountSid,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *AccountHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 11 */
NTSTATUS LsarEnumerateAccounts(
    LSAPR_HANDLE PolicyHandle,
    DWORD *EnumerationContext,
    PLSAPR_ACCOUNT_ENUM_BUFFER EnumerationBuffer,
    DWORD PreferedMaximumLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 12 */
NTSTATUS LsarCreateTrustedDomain(
    LSAPR_HANDLE PolicyHandle,
    PLSAPR_TRUST_INFORMATION TrustedDomainInformation,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *TrustedDomainHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 13 */
NTSTATUS LsarEnumerateTrustedDomains(
    LSAPR_HANDLE PolicyHandle,
    DWORD *EnumerationContext,
    PLSAPR_TRUSTED_ENUM_BUFFER EnumerationBuffer,
    DWORD PreferedMaximumLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 14 */
NTSTATUS LsarLookupNames(
    LSAPR_HANDLE PolicyHandle,
    DWORD Count,
    PRPC_UNICODE_STRING Names,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_SIDS TranslatedSids,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 15 */
NTSTATUS LsarLookupSids(
    LSAPR_HANDLE PolicyHandle,
    PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_NAMES TranslatedNames,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 16 */
NTSTATUS LsarCreateSecret(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING SecretName,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *SecretHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 17 */
NTSTATUS LsarOpenAccount(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID AccountSid,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *AccountHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 18 */
NTSTATUS LsarEnumeratePrivilegesAccount(
    LSAPR_HANDLE AccountHandle,
    PLSAPR_PRIVILEGE_SET *Privileges)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 19 */
NTSTATUS LsarAddPrivilegesToAccount(
    LSAPR_HANDLE AccountHandle,
    PLSAPR_PRIVILEGE_SET Privileges)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 20 */
NTSTATUS LsarRemovePrivilegesFromAccount(
    LSAPR_HANDLE AccountHandle,
    BOOL AllPrivileges,
    PLSAPR_PRIVILEGE_SET Privileges)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 21 */
NTSTATUS LsarGetQuotasForAccount(
    LSAPR_HANDLE AccountHandle,
    PQUOTA_LIMITS QuotaLimits)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 22 */
NTSTATUS LsarSetQuotasForAccount(
    LSAPR_HANDLE AccountHandle,
    PQUOTA_LIMITS QuotaLimits)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 23 */
NTSTATUS LsarGetSystemAccessAccount(
    LSAPR_HANDLE AccountHandle,
    ACCESS_MASK *SystemAccess)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 24 */
NTSTATUS LsarSetSystemAccessAccount(
    LSAPR_HANDLE AccountHandle,
    ACCESS_MASK SystemAccess)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 25 */
NTSTATUS LsarOpenTrustedDomain(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID TrustedDomainSid,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *TrustedDomainHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 26 */
NTSTATUS LsarQueryInfoTrustedDomain(
    LSAPR_HANDLE TrustedDomainHandle,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PLSAPR_TRUSTED_DOMAIN_INFO *TrustedDomainInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 27 */
NTSTATUS LsarSetInformationTrustedDomain(
    LSAPR_HANDLE TrustedDomainHandle,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 28 */
NTSTATUS LsarOpenSecret(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING SecretName,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *SecretHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 29 */
NTSTATUS LsarSetSecret(
    LSAPR_HANDLE *SecretHandle,
    PLSAPR_CR_CIPHER_VALUE EncryptedCurrentValue,
    PLSAPR_CR_CIPHER_VALUE EncryptedOldValue)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 30 */
NTSTATUS LsarQuerySecret(
    LSAPR_HANDLE SecretHandle,
    PLSAPR_CR_CIPHER_VALUE *EncryptedCurrentValue,
    PLARGE_INTEGER CurrentValueSetTime,
    PLSAPR_CR_CIPHER_VALUE *EncryptedOldValue,
    PLARGE_INTEGER OldValueSetTime)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 31 */
NTSTATUS LsarLookupPrivilegeValue(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING Name,
    PLUID Value)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 32 */
NTSTATUS LsarLookupPrivilegeName(
    LSAPR_HANDLE PolicyHandle,
    PLUID Value,
    PRPC_UNICODE_STRING *Name)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 33 */
NTSTATUS LsarLookupPrivilegeDisplayName(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING Name,
    USHORT ClientLanguage,
    USHORT ClientSystemDefaultLanguage,
    PRPC_UNICODE_STRING *DisplayName,
    USHORT *LanguageReturned)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 34 */
NTSTATUS LsarDeleteObject(
    LSAPR_HANDLE *ObjectHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 35 */
NTSTATUS LsarEnumerateAccountsWithUserRight(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING UserRight,
    PLSAPR_ACCOUNT_ENUM_BUFFER EnumerationBuffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 36 */
NTSTATUS LsarEnmuerateAccountRights(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID AccountSid,
    PLSAPR_USER_RIGHT_SET UserRights)
{
    FIXME("(%p,%p,%p) stub\n", PolicyHandle, AccountSid, UserRights);

    if (!LsapValidateDbHandle(PolicyHandle))
        return STATUS_INVALID_HANDLE;

    UserRights->Entries = 0;
    UserRights->UserRights = NULL;
    return STATUS_OBJECT_NAME_NOT_FOUND;
}


/* Function 37 */
NTSTATUS LsarAddAccountRights(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID AccountSid,
    PLSAPR_USER_RIGHT_SET UserRights)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 38 */
NTSTATUS LsarRemoveAccountRights(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID AccountSid,
    BOOL AllRights,
    PLSAPR_USER_RIGHT_SET UserRights)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 39 */
NTSTATUS LsarQueryTrustedDomainInfo(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID TrustedDomainSid,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PLSAPR_TRUSTED_DOMAIN_INFO *TrustedDomainInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 40 */
NTSTATUS LsarSetTrustedDomainInfo(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID TrustedDomainSid,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 41 */
NTSTATUS LsarDeleteTrustedDomain(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID TrustedDomainSid)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 42 */
NTSTATUS LsarStorePrivateData(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING KeyName,
    PLSAPR_CR_CIPHER_VALUE EncryptedData)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 43 */
NTSTATUS LsarRetrievePrivateData(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING KeyName,
    PLSAPR_CR_CIPHER_VALUE *EncryptedData)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 44 */
NTSTATUS LsarOpenPolicy2(
    LPWSTR SystemName,
    PLSAPR_OBJECT_ATTRIBUTES ObjectAttributes,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *PolicyHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 45 */
NTSTATUS LsarGetUserName(
    LPWSTR SystemName,
    PRPC_UNICODE_STRING *UserName,
    PRPC_UNICODE_STRING *DomainName)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 46 */
NTSTATUS LsarQueryInformationPolicy2(
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    unsigned long *PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 47 */
NTSTATUS LsarSetInformationPolicy2(
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    unsigned long PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 48 */
NTSTATUS LsarQueryTrustedDomainInfoByName(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING TrustedDomainName,
    POLICY_INFORMATION_CLASS InformationClass,
    unsigned long *PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 49 */
NTSTATUS LsarSetTrustedDomainInfoByName(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING TrustedDomainName,
    POLICY_INFORMATION_CLASS InformationClass,
    unsigned long PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 50 */
NTSTATUS LsarEnumerateTrustedDomainsEx(
    LSAPR_HANDLE PolicyHandle,
    DWORD *EnumerationContext,
    PLSAPR_TRUSTED_ENUM_BUFFER_EX EnumerationBuffer,
    DWORD PreferedMaximumLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 51 */
NTSTATUS LsarCreateTrustedDomainEx(
    LSAPR_HANDLE PolicyHandle,
    PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX TrustedDomainInformation,
    PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION AuthentificationInformation,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *TrustedDomainHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 52 */
NTSTATUS LsarSetPolicyReplicationHandle(
    PLSAPR_HANDLE PolicyHandle)
{
    /* Deprecated */
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 53 */
NTSTATUS LsarQueryDomainInformationPolicy(
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    unsigned long *PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 54 */
NTSTATUS LsarSetDomainInformationPolicy(
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    unsigned long PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 55 */
NTSTATUS LsarOpenTrustedDomainByName(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING TrustedDomainName,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *TrustedDomainHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 56 */
NTSTATUS LsarTestCall(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 57 */
NTSTATUS LsarLookupSids2(
    LSAPR_HANDLE PolicyHandle,
    PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount,
    DWORD LookupOptions,
    DWORD ClientRevision)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 58 */
NTSTATUS LsarLookupNames2(
    LSAPR_HANDLE PolicyHandle,
    DWORD Count,
    PRPC_UNICODE_STRING Names,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_SID_EX TranslatedSids,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount,
    DWORD LookupOptions,
    DWORD ClientRevision)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 59 */
NTSTATUS LsarCreateTrustedDomainEx2(
    LSAPR_HANDLE PolicyHandle,
    PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX TrustedDomainInformation,
    PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL AuthentificationInformation,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *TrustedDomainHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 60 */
NTSTATUS CredrWrite(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 61 */
NTSTATUS CredrRead(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 62 */
NTSTATUS CredrEnumerate(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 63 */
NTSTATUS CredrWriteDomainCredentials(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 64 */
NTSTATUS CredrReadDomainCredentials(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 65 */
NTSTATUS CredrDelete(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 66 */
NTSTATUS CredrGetTargetInfo(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 67 */
NTSTATUS CredrProfileLoaded(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 68 */
NTSTATUS LsarLookupNames3(
    LSAPR_HANDLE PolicyHandle,
    DWORD Count,
    PRPC_UNICODE_STRING Names,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_SID_EX2 TranslatedSids,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount,
    DWORD LookupOptions,
    DWORD ClientRevision)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 69 */
NTSTATUS CredrGetSessionTypes(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 70 */
NTSTATUS LsarRegisterAuditEvent(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 71 */
NTSTATUS LsarGenAuditEvent(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 72 */
NTSTATUS LsarUnregisterAuditEvent(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 73 */
NTSTATUS LsarQueryForestTrustInformation(
    LSAPR_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING TrustedDomainName,
    LSA_FOREST_TRUST_RECORD_TYPE HighestRecordType,
    PLSA_FOREST_TRUST_INFORMATION *ForestTrustInfo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 74 */
NTSTATUS LsarSetForestTrustInformation(
    LSAPR_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING TrustedDomainName,
    LSA_FOREST_TRUST_RECORD_TYPE HighestRecordType,
    PLSA_FOREST_TRUST_INFORMATION ForestTrustInfo,
    BOOL CheckOnly,
    PLSA_FOREST_TRUST_COLLISION_INFORMATION *CollisionInfo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 75 */
NTSTATUS CredrRename(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 76 */
NTSTATUS LsarLookupSids3(
    LSAPR_HANDLE PolicyHandle,
    PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount,
    DWORD LookupOptions,
    DWORD ClientRevision)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 77 */
NTSTATUS LsarLookupNames4(
    handle_t RpcHandle,
    DWORD Count,
    PRPC_UNICODE_STRING Names,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_SID_EX2 TranslatedSids,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount,
    DWORD LookupOptions,
    DWORD ClientRevision)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 78 */
NTSTATUS LsarOpenPolicySce(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 79 */
NTSTATUS LsarAdtRegisterSecurityEventSource(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 80 */
NTSTATUS LsarAdtUnregisterSecurityEventSource(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 81 */
NTSTATUS LsarAdtReportSecurityEvent(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 82 */
NTSTATUS CredrFindBestCredential(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 83 */
NTSTATUS LsarSetAuditPolicy(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 84 */
NTSTATUS LsarQueryAuditPolicy(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 85 */
NTSTATUS LsarEnumerateAuditPolicy(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 86 */
NTSTATUS LsarEnumerateAuditCategories(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 87 */
NTSTATUS LsarEnumerateAuditSubCategories(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 88 */
NTSTATUS LsarLookupAuditCategoryName(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 89 */
NTSTATUS LsarLookupAuditSubCategoryName(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 90 */
NTSTATUS LsarSetAuditSecurity(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 91 */
NTSTATUS LsarQueryAuditSecurity(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 92 */
NTSTATUS CredReadByTokenHandle(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 93 */
NTSTATUS CredrRestoreCredentials(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 94 */
NTSTATUS CredrBackupCredentials(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
