
list(APPEND SOURCE
    LdrEnumResources.c
    NtAllocateVirtualMemory.c
    NtContinue.c
    NtCreateFile.c
    NtCreateThread.c
    NtDeleteKey.c
    NtFreeVirtualMemory.c
    NtMapViewOfSection.c
    NtMutant.c
    NtProtectVirtualMemory.c
    NtQueryKey.c
    NtQuerySystemEnvironmentValue.c
    NtQueryVolumeInformationFile.c
    NtSaveKey.c
    RtlBitmap.c
    RtlDetermineDosPathNameType.c
    RtlDoesFileExists.c
    RtlDosPathNameToNtPathName_U.c
    RtlDosSearchPath_U.c
    RtlDosSearchPath_Ustr.c
    RtlGetFullPathName_U.c
    RtlGetFullPathName_Ustr.c
    RtlGetFullPathName_UstrEx.c
    RtlGetLengthWithoutTrailingPathSeperators.c
    RtlGetLongestNtPathLength.c
    RtlInitializeBitMap.c
    RtlMemoryStream.c
    RtlReAllocateHeap.c
    StackOverflow.c
    SystemInfo.c
    Timer.c
    testlist.c)

if(ARCH STREQUAL "i386")
    add_asm_files(ntdll_apitest_asm i386/NtContinue.S)
endif()

add_executable(ntdll_apitest ${SOURCE} ${ntdll_apitest_asm})
target_link_libraries(ntdll_apitest wine uuid ${PSEH_LIB})
set_module_type(ntdll_apitest win32cui)
add_importlibs(ntdll_apitest msvcrt advapi32 kernel32 ntdll)

if(NOT MSVC)
    set_source_files_properties(RtlGetFullPathName_UstrEx.c PROPERTIES COMPILE_FLAGS "-Wno-format")
endif()

add_cd_file(TARGET ntdll_apitest DESTINATION reactos/bin FOR all)
