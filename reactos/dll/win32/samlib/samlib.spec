@ stub SamAddMemberToAlias
@ stub SamAddMemberToGroup
@ stub SamAddMultipleMembersToAlias
@ stub SamChangePasswordUser2
@ stub SamChangePasswordUser3
@ stub SamChangePasswordUser
@ stdcall SamCloseHandle(ptr)
@ stdcall SamConnect(ptr ptr long ptr)
@ stub SamConnectWithCreds
@ stub SamCreateAliasInDomain
@ stub SamCreateGroupInDomain
@ stub SamCreateUser2InDomain
@ stub SamCreateUserInDomain
@ stub SamDeleteAlias
@ stub SamDeleteGroup
@ stub SamDeleteUser
@ stub SamEnumerateAliasesInDomain
@ stub SamEnumerateDomainsInSamServer
@ stub SamEnumerateGroupsInDomain
@ stub SamEnumerateUsersInDomain
@ stub SamFreeMemory
@ stub SamGetAliasMembership
@ stub SamGetCompatibilityMode
@ stub SamGetDisplayEnumerationIndex
@ stub SamGetGroupsForUser
@ stub SamGetMembersInAlias
@ stub SamGetMembersInGroup
@ stub SamLookupDomainInSamServer
@ stub SamLookupIdsInDomain
@ stub SamLookupNamesInDomain
@ stub SamOpenAlias
@ stub SamOpenDomain
@ stub SamOpenGroup
@ stub SamOpenUser
@ stub SamQueryDisplayInformation
@ stub SamQueryInformationAlias
@ stub SamQueryInformationDomain
@ stub SamQueryInformationGroup
@ stub SamQueryInformationUser
@ stub SamQuerySecurityObject
@ stub SamRemoveMemberFromAlias
@ stub SamRemoveMemberFromForeignDomain
@ stub SamRemoveMemberFromGroup
@ stub SamRemoveMultipleMembersFromAlias
@ stub SamRidToSid
@ stub SamSetInformationAlias
@ stub SamSetInformationDomain
@ stub SamSetInformationGroup
@ stub SamSetInformationUser
@ stub SamSetMemberAttributesOfGroup
@ stub SamSetSecurityObject
@ stdcall SamShutdownSamServer(ptr)
@ stub SamTestPrivateFunctionsDomain
@ stub SamTestPrivateFunctionsUser
@ stub SamiChangeKeys
@ stub SamiChangePasswordUser2
@ stub SamiChangePasswordUser3
@ stub SamiChangePasswordUser
@ stub SamiEncryptPasswords
@ stub SamiGetBootKeyInformation
@ stub SamiLmChangePasswordUser
@ stub SamiOemChangePasswordUser2
@ stub SamiSetBootKeyInformation
@ stub SamiSetDSRMPassword
@ stub SamiSetDSRMPasswordOWF

@ stdcall SamCreateUser(wstr wstr ptr)
@ stdcall SamCheckUserPassword(wstr wstr)
@ stdcall SamGetUserSid(wstr ptr)
