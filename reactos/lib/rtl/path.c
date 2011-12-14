/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/path.c
 * PURPOSE:         Path and current directory functions
 * PROGRAMMERS:     Wine team
 *                  Thomas Weidenmueller
 *                  Gunnar Dalsnes
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* DEFINITONS and MACROS ******************************************************/

#define MAX_PFX_SIZE       16

#define IS_PATH_SEPARATOR(x) (((x)==L'\\')||((x)==L'/'))


/* GLOBALS ********************************************************************/

static const WCHAR DeviceRootW[] = L"\\\\.\\";
const UNICODE_STRING DeviceRootString = RTL_CONSTANT_STRING(L"\\\\.\\");

const UNICODE_STRING RtlpDosDevicesUncPrefix = RTL_CONSTANT_STRING(L"\\??\\UNC\\");
const UNICODE_STRING RtlpWin32NtRootSlash =   RTL_CONSTANT_STRING(L"\\\\?\\");
const UNICODE_STRING RtlpDosSlashCONDevice =  RTL_CONSTANT_STRING(L"\\\\.\\CON");
const UNICODE_STRING RtlpDosDevicesPrefix  =  RTL_CONSTANT_STRING(L"\\??\\");

const UNICODE_STRING RtlpDosLPTDevice = RTL_CONSTANT_STRING(L"LPT");
const UNICODE_STRING RtlpDosCOMDevice = RTL_CONSTANT_STRING(L"COM");
const UNICODE_STRING RtlpDosPRNDevice = RTL_CONSTANT_STRING(L"PRN");
const UNICODE_STRING RtlpDosAUXDevice = RTL_CONSTANT_STRING(L"AUX");
const UNICODE_STRING RtlpDosCONDevice = RTL_CONSTANT_STRING(L"CON");
const UNICODE_STRING RtlpDosNULDevice = RTL_CONSTANT_STRING(L"NUL");

/* PRIVATE FUNCTIONS **********************************************************/

ULONG
NTAPI
RtlIsDosDeviceName_Ustr(IN PUNICODE_STRING PathString)
{
    UNICODE_STRING PathCopy;
    PWCHAR Start, End;
    USHORT PathChars, ColonCount = 0;
    USHORT ReturnOffset = 0, ReturnLength, OriginalLength;
    WCHAR c;

    /* Validate the input */
    if (!PathString) return 0;

    /* Check what type of path this is */
    switch (RtlDetermineDosPathNameType_Ustr(PathString))
    {
        /* Fail for UNC or unknown paths */
        case RtlPathTypeUnknown:
        case RtlPathTypeUncAbsolute:
            return 0;

        /* Make special check for the CON device */
        case RtlPathTypeLocalDevice:
            if (RtlEqualUnicodeString(PathString, &RtlpDosSlashCONDevice, TRUE))
            {
                /* This should return 0x80006 */
                return MAKELONG(RtlpDosCONDevice.Length, DeviceRootString.Length);
            }
            return 0;

        default:
            break;
    }

    /* Make a copy of the string */
    PathCopy = *PathString;
    OriginalLength = PathString->Length;

    /* Return if there's no characters */
    PathChars = PathCopy.Length / sizeof(WCHAR);
    if (!PathChars) return 0;

    /* Check for drive path and truncate */
    if (PathCopy.Buffer[PathChars - 1] == L':')
    {
        /* Fixup the lengths */
        PathCopy.Length -= sizeof(WCHAR);
        if (!--PathChars) return 0;

        /* Remember this for later */
        ColonCount = 1;
    }

    /* Check for extension or space, and truncate */
    do
    {
        /* Stop if we hit something else than a space or period */
        c = PathCopy.Buffer[PathChars - 1];
        if ((c != '.') && (c != ' ')) break;

        /* Fixup the lengths */
        PathCopy.Length -= sizeof(WCHAR);

        /* Remember this for later */
        ColonCount++;
    } while (--PathChars);

    /* Anything still left? */
    if (PathChars)
    {
        /* Loop from the end */
        for (End = &PathCopy.Buffer[PathChars - 1];
             End >= PathCopy.Buffer;
             --End)
        {
            /* Check if the character is a path or drive separator */
            c = *End;
            if ((c == '\\') || (c == '/') || ((c == ':') && (End == PathCopy.Buffer + 1)))
            {
                /* Get the next lower case character */
                End++;
                c = *End | ' '; // ' ' == ('z' - 'Z')

                /* Check if it's a DOS device (LPT, COM, PRN, AUX, or NUL) */
                if ((End < &PathCopy.Buffer[OriginalLength / sizeof(WCHAR)]) &&
                    ((c == 'l') || (c == 'c') || (c == 'p') || (c == 'a') || (c == 'n')))
                {
                    /* Calculate the offset */
                    ReturnOffset = (PCHAR)End - (PCHAR)PathCopy.Buffer;

                    /* Build the final string */
                    PathCopy.Length = OriginalLength - ReturnOffset - (ColonCount * sizeof(WCHAR));
                    PathCopy.Buffer = End;

                    /* Save new amount of chars in the path */
                    PathChars = PathCopy.Length / sizeof(WCHAR);

                    break;
                }
                else
                {
                    return 0;
                }
            }
        }

        /* Get the next lower case character and check if it's a DOS device */
        c = *PathCopy.Buffer | ' '; // ' ' == ('z' - 'Z')
        if ((c != 'l') && (c != 'c') && (c != 'p') && (c != 'a') && (c != 'n'))
        {
            /* Not LPT, COM, PRN, AUX, or NUL */
            return 0;
        }
    }

    /* Now skip past any extra extension or drive letter characters */
    Start = PathCopy.Buffer;
    End = &Start[PathChars];
    while (Start < End)
    {
        c = *Start;
        if ((c == '.') || (c == ':')) break;
        Start++;
    }

    /* And then go backwards to get rid of spaces */
    while ((Start > PathCopy.Buffer) && (Start[-1] == ' ')) --Start;

    /* Finally see how many characters are left, and that's our size */
    PathChars = Start - PathCopy.Buffer;
    PathCopy.Length = PathChars * sizeof(WCHAR);

    /* Check if this is a COM or LPT port, which has a digit after it */
    if ((PathChars == 4) &&
        (iswdigit(PathCopy.Buffer[3]) && (PathCopy.Buffer[3] != '0')))
    {
        /* Don't compare the number part, just check for LPT or COM */
        PathCopy.Length -= sizeof(WCHAR);
        if ((RtlEqualUnicodeString(&PathCopy, &RtlpDosLPTDevice, TRUE)) ||
            (RtlEqualUnicodeString(&PathCopy, &RtlpDosCOMDevice, TRUE)))
        {
            /* Found it */
            ReturnLength = sizeof(L"COM1") - sizeof(WCHAR);
            return MAKELONG(ReturnLength, ReturnOffset);
        }
    }
    else if ((PathChars == 3) &&
             ((RtlEqualUnicodeString(&PathCopy, &RtlpDosPRNDevice, TRUE)) ||
              (RtlEqualUnicodeString(&PathCopy, &RtlpDosAUXDevice, TRUE)) ||
              (RtlEqualUnicodeString(&PathCopy, &RtlpDosNULDevice, TRUE)) ||
              (RtlEqualUnicodeString(&PathCopy, &RtlpDosCONDevice, TRUE))))
    {
        /* Otherwise this was something like AUX, NUL, PRN, or CON */
        ReturnLength = sizeof(L"AUX") - sizeof(WCHAR);
        return MAKELONG(ReturnLength, ReturnOffset);
    }

    /* Otherwise, this is not a valid DOS device */
    return 0;
}

RTL_PATH_TYPE
NTAPI
RtlDetermineDosPathNameType_Ustr(IN PCUNICODE_STRING PathString)
{
    PWCHAR Path;
    ULONG Chars;

    /* Validate the input */
    if (!PathString) return RtlPathTypeUnknown;

    Path = PathString->Buffer;
    Chars = PathString->Length / sizeof(WCHAR);

    /* Return if there are no characters */
    if (!Chars) return RtlPathTypeUnknown;

    /*
     * The algorithm is similar to RtlDetermineDosPathNameType_U but here we
     * actually check for the path length before touching the characters
     */
    if ((Chars < 1) || (IS_PATH_SEPARATOR(Path[0])))
    {
        if ((Chars < 2) || !(IS_PATH_SEPARATOR(Path[1]))) return RtlPathTypeRooted;                /* \x             */
        if ((Chars < 3) || ((Path[2] != L'.') && (Path[2] != L'?'))) return RtlPathTypeUncAbsolute;/* \\x            */
        if ((Chars >= 4) && (IS_PATH_SEPARATOR(Path[3]))) return RtlPathTypeLocalDevice;           /* \\.\x or \\?\x */
        if (Chars != 3) return RtlPathTypeUncAbsolute;                                             /* \\.x or \\?x   */
        return RtlPathTypeRootLocalDevice;                                                         /* \\. or \\?     */
    }
    else
    {
        if ((Chars < 2) || (!(Path[0]) || (Path[1] != L':'))) return RtlPathTypeRelative;          /* x              */
        if ((Chars < 3) || (IS_PATH_SEPARATOR(Path[2]))) return RtlPathTypeDriveAbsolute;          /* x:\            */
        return RtlPathTypeDriveRelative;                                                           /* x:             */
    }
}

NTSTATUS
NTAPI
RtlpCheckDeviceName(IN PUNICODE_STRING FileName,
                    IN ULONG Length,
                    OUT PBOOLEAN NameInvalid)
{
    PWCHAR Buffer;
    NTSTATUS Status;

    /* Allocate a large enough buffer */
    Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, FileName->Length);
    if (Buffer)
    {
        /* Assume failure */
        *NameInvalid = TRUE;

        /* Copy the filename */
        RtlCopyMemory(Buffer, FileName->Buffer, FileName->Length);

        /* And add a dot at the end */
        Buffer[Length / sizeof(WCHAR)] = L'.';
        Buffer[(Length / sizeof(WCHAR)) + 1] = UNICODE_NULL;

        /* Check if the file exists or not */
        *NameInvalid = RtlDoesFileExists_U(Buffer) ? FALSE: TRUE;

        /* Get rid of the buffer now */
        Status = RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
    }
    else
    {
        /* Assume the name is ok, but fail the call */
        *NameInvalid = FALSE;
        Status = STATUS_NO_MEMORY;
    }

    /* Return the status */
    return Status;
}

ULONG
NTAPI
RtlGetFullPathName_Ustr(IN PUNICODE_STRING FileName,
                        IN ULONG Size,
                        IN PWSTR Buffer,
                        OUT PCWSTR *ShortName,
                        OUT PBOOLEAN InvalidName,
                        OUT RTL_PATH_TYPE *PathType)
{
    PWCHAR FileNameBuffer;
    ULONG FileNameLength, FileNameChars, DosLength, DosLengthOffset, FullLength;
    WCHAR c;
    NTSTATUS Status;

    /* For now, assume the name is valid */
    DPRINT("Filename: %wZ\n", FileName);
    DPRINT("Size and buffer: %lx %S\n", Size, Buffer);
    if (InvalidName) *InvalidName = FALSE;

    /* Handle initial path type and failure case */
    *PathType = RtlPathTypeUnknown;
    if (!(Size) || !(Buffer) || !(FileName) ||
        !(FileName->Length) || (FileName->Buffer[0] == UNICODE_NULL)) return 0;

    /* Break filename into component parts */
    FileNameBuffer = FileName->Buffer;
    FileNameLength = FileName->Length;
    FileNameChars = FileNameLength / sizeof(WCHAR);

    /* Kill trailing spaces */
    c = FileNameBuffer[FileNameChars - 1];
    while ((FileNameLength) && (c == L' '))
    {
        /* Keep going, ignoring the spaces */
        FileNameLength -= sizeof(WCHAR);
        if (FileNameLength) c = FileNameBuffer[FileNameLength / sizeof(WCHAR) - 1];
    }

    /* Check if anything is left */
    if (!FileNameLength) return 0;

    /* Check if this is a DOS name */
    DosLength = RtlIsDosDeviceName_Ustr(FileName);
    DPRINT("DOS length for filename: %lx %wZ\n", DosLength, FileName);
    if (DosLength)
    {
        /* Zero out the short name */
        if (ShortName) *ShortName = NULL;

        /* See comment for RtlIsDosDeviceName_Ustr if this is confusing... */
        DosLengthOffset = DosLength >> 16;
        DosLength = DosLength & 0xFFFF;

        /* Do we have a DOS length, and does the caller want validity? */
        if ((InvalidName) && (DosLengthOffset))
        {
            /* Do the check */
            Status = RtlpCheckDeviceName(FileName, DosLengthOffset, InvalidName);

            /* If the check failed, or the name is invalid, fail here */
            if (!NT_SUCCESS(Status)) return 0;
            if (*InvalidName) return 0;
        }

        /* Add the size of the device root and check if it fits in the size */
        FullLength = DosLength + DeviceRootString.Length;
        if (FullLength < Size)
        {
            /* Add the device string */
            RtlMoveMemory(Buffer, DeviceRootString.Buffer, DeviceRootString.Length);

            /* Now add the DOS device name */
            RtlMoveMemory((PCHAR)Buffer + DeviceRootString.Length,
                          (PCHAR)FileNameBuffer + DosLengthOffset,
                          DosLength);

            /* Null terminate */
            *(PWCHAR)((ULONG_PTR)Buffer + FullLength) = UNICODE_NULL;
            return FullLength;
        }

        /* Otherwise, there's no space, so return the buffer size needed */
        if ((FullLength + sizeof(UNICODE_NULL)) > UNICODE_STRING_MAX_BYTES) return 0;
        return FullLength + sizeof(UNICODE_NULL);
    }

    /* This should work well enough for our current needs */
    *PathType = RtlDetermineDosPathNameType_U(FileNameBuffer);
    DPRINT("Path type: %lx\n", *PathType);

    /* This is disgusting... but avoids re-writing everything */
    DPRINT("Calling old API with %s and %lx and %S\n", FileNameBuffer, Size, Buffer);
    return RtlGetFullPathName_U(FileNameBuffer, Size, Buffer, (PWSTR*)ShortName);
}

NTSTATUS
NTAPI
RtlpWin32NTNameToNtPathName_U(IN PUNICODE_STRING DosPath,
                              OUT PUNICODE_STRING NtPath,
                              OUT PCWSTR *PartName,
                              OUT PRTL_RELATIVE_NAME_U RelativeName)
{
    ULONG DosLength;
    PWSTR NewBuffer, p;

    /* Validate the input */
    if (!DosPath) return STATUS_OBJECT_NAME_INVALID;

    /* Validate the DOS length */
    DosLength = DosPath->Length;
    if (DosLength >= UNICODE_STRING_MAX_BYTES) return STATUS_NAME_TOO_LONG;

    /* Make space for the new path */
    NewBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                0,
                                DosLength + sizeof(UNICODE_NULL));
    if (!NewBuffer) return STATUS_NO_MEMORY;

    /* Copy the prefix, and then the rest of the DOS path, and NULL-terminate */
    RtlCopyMemory(NewBuffer, RtlpDosDevicesPrefix.Buffer, RtlpDosDevicesPrefix.Length);
    RtlCopyMemory((PCHAR)NewBuffer + RtlpDosDevicesPrefix.Length,
                  DosPath->Buffer + RtlpDosDevicesPrefix.Length / sizeof(WCHAR),
                  DosPath->Length - RtlpDosDevicesPrefix.Length);
    NewBuffer[DosLength / sizeof(WCHAR)] = UNICODE_NULL;

    /* Did the caller send a relative name? */
    if (RelativeName)
    {
        /* Zero initialize it */
        RtlInitEmptyUnicodeString(&RelativeName->RelativeName, NULL, 0);
        RelativeName->ContainingDirectory = NULL;
        RelativeName->CurDirRef = 0;
    }

    /* Did the caller request a partial name? */
    if (PartName)
    {
        /* Loop from the back until we find a path separator */
        p = &NewBuffer[(DosLength - 1) / sizeof (WCHAR)];
        while (p > NewBuffer) if (*p-- == '\\') break;

        /* Was one found? */
        if (p > NewBuffer)
        {
            /* Move past it -- anything left? */
            p++;
            if (!*p)
            {
                /* The path ends with a path separator, no part name */
                *PartName = NULL;
            }
            else
            {
                /* What follows the path separator is the part name */
                *PartName = p;
            }
        }
    }

    /* Build the final NT path string */
    NtPath->Length = (USHORT)DosLength;
    NtPath->Buffer = NewBuffer;
    NtPath->MaximumLength = (USHORT)DosLength + sizeof(UNICODE_NULL);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlpDosPathNameToRelativeNtPathName_Ustr(IN BOOLEAN HaveRelative,
                                         IN PCUNICODE_STRING DosName,
                                         OUT PUNICODE_STRING NtName,
                                         OUT PCWSTR *PartName,
                                         OUT PRTL_RELATIVE_NAME_U RelativeName)
{
    WCHAR BigBuffer[MAX_PATH + 1];
    PWCHAR PrefixBuffer, NewBuffer, Buffer;
    ULONG MaxLength, PathLength, PrefixLength, PrefixCut, LengthChars, Length;
    UNICODE_STRING CapturedDosName, PartNameString;
    BOOLEAN QuickPath;
    RTL_PATH_TYPE InputPathType, BufferPathType;
    NTSTATUS Status;
    BOOLEAN NameInvalid;

    /* Assume MAX_PATH for now */
    DPRINT("Relative: %lx DosName: %wZ NtName: %wZ, PartName: %p, RelativeName: %p\n",
            HaveRelative, DosName, NtName, PartName, RelativeName);
    MaxLength = sizeof(BigBuffer);

    /* Validate the input */
    if (!DosName) return STATUS_OBJECT_NAME_INVALID;

    /* Capture input string */
    CapturedDosName = *DosName;

    /* Check for \\?\\ form */
    if ((CapturedDosName.Length <= RtlpWin32NtRootSlash.Length) ||
        (CapturedDosName.Buffer[0] != RtlpWin32NtRootSlash.Buffer[0]) ||
        (CapturedDosName.Buffer[1] != RtlpWin32NtRootSlash.Buffer[1]) ||
        (CapturedDosName.Buffer[2] != RtlpWin32NtRootSlash.Buffer[2]) ||
        (CapturedDosName.Buffer[3] != RtlpWin32NtRootSlash.Buffer[3]))
    {
        /* Quick path won't be used */
        QuickPath = FALSE;

        /* Use the static buffer */
        Buffer = BigBuffer;
        MaxLength += RtlpDosDevicesUncPrefix.Length;

        /* Allocate a buffer to hold the path */
        NewBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, MaxLength);
        DPRINT("Length: %lx\n", MaxLength);
        if (!NewBuffer) return STATUS_NO_MEMORY;
    }
    else
    {
        /* Use the optimized path after acquiring the lock */
        QuickPath = TRUE;
        NewBuffer = NULL;
    }

    /* Lock the PEB and check if the quick path can be used */
    RtlAcquirePebLock();
    if (QuickPath)
    {
        /* Some simple fixups will get us the correct path */
        DPRINT("Quick path\n");
        Status = RtlpWin32NTNameToNtPathName_U(&CapturedDosName,
                                               NtName,
                                               PartName,
                                               RelativeName);

        /* Release the lock, we're done here */
        RtlReleasePebLock();
        return Status;
    }

    /* Call the main function to get the full path name and length */
    PathLength = RtlGetFullPathName_Ustr(&CapturedDosName,
                                         MAX_PATH * sizeof(WCHAR),
                                         Buffer,
                                         PartName,
                                         &NameInvalid,
                                         &InputPathType);
    if ((NameInvalid) || !(PathLength) || (PathLength > (MAX_PATH * sizeof(WCHAR))))
    {
        /* Invalid name, fail */
        DPRINT("Invalid name: %lx Path Length: %lx\n", NameInvalid, PathLength);
        RtlFreeHeap(RtlGetProcessHeap(), 0, NewBuffer);
        RtlReleasePebLock();
        return STATUS_OBJECT_NAME_INVALID;
    }

    /* Start by assuming the path starts with \??\ (DOS Devices Path) */
    PrefixLength = RtlpDosDevicesPrefix.Length;
    PrefixBuffer = RtlpDosDevicesPrefix.Buffer;
    PrefixCut = 0;

    /* Check where it really is */
    BufferPathType = RtlDetermineDosPathNameType_U(Buffer);
    DPRINT("Buffer: %S Type: %lx\n", Buffer, BufferPathType);
    switch (BufferPathType)
    {
        /* It's actually a UNC path in \??\UNC\ */
        case RtlPathTypeUncAbsolute:
            PrefixLength = RtlpDosDevicesUncPrefix.Length;
            PrefixBuffer = RtlpDosDevicesUncPrefix.Buffer;
            PrefixCut = 2;
            break;

        case RtlPathTypeLocalDevice:
            /* We made a good guess, go with it but skip the \??\ */
            PrefixCut = 4;
            break;

        case RtlPathTypeDriveAbsolute:
        case RtlPathTypeDriveRelative:
        case RtlPathTypeRooted:
        case RtlPathTypeRelative:
            /* Our guess was good, roll with it */
            break;

        /* Nothing else is expected */
        default:
            ASSERT(FALSE);

    }

    /* Now copy the prefix and the buffer */
    RtlCopyMemory(NewBuffer, PrefixBuffer, PrefixLength);
    RtlCopyMemory((PCHAR)NewBuffer + PrefixLength,
                  &Buffer[PrefixCut],
                  PathLength - (PrefixCut * sizeof(WCHAR)));

    /* Compute the length */
    Length = PathLength - PrefixCut * sizeof(WCHAR) + PrefixLength;
    LengthChars = Length / sizeof(WCHAR);

    /* Setup the actual NT path string and terminate it */
    NtName->Buffer = NewBuffer;
    NtName->Length = (USHORT)Length;
    NtName->MaximumLength = (USHORT)MaxLength;
    NewBuffer[LengthChars] = UNICODE_NULL;
    DPRINT("new buffer: %S\n", NewBuffer);
    DPRINT("NT Name: %wZ\n", NtName);

    /* Check if a partial name was requested */
    if ((PartName) && (*PartName))
    {
        /* Convert to Unicode */
        Status = RtlInitUnicodeStringEx(&PartNameString, *PartName);
        if (NT_SUCCESS(Status))
        {
            /* Set the partial name */
            *PartName = &NewBuffer[LengthChars - (PartNameString.Length / sizeof(WCHAR))];
        }
        else
        {
            /* Fail */
            RtlFreeHeap(RtlGetProcessHeap(), 0, NewBuffer);
            RtlReleasePebLock();
            return Status;
        }
    }

    /* Check if a relative name was asked for */
    if (RelativeName)
    {
        /* Setup the structure */
        RtlInitEmptyUnicodeString(&RelativeName->RelativeName, NULL, 0);
        RelativeName->ContainingDirectory = NULL;
        RelativeName->CurDirRef = 0;

        /* Check if the input path itself was relative */
        if (InputPathType == RtlPathTypeRelative)
        {
            /* FIXME: HACK: Old code */
            PCURDIR cd;
            UNICODE_STRING us;
            cd = (PCURDIR)&(NtCurrentPeb ()->ProcessParameters->CurrentDirectory.DosPath);
            if (cd->Handle)
            {
                RtlInitUnicodeString(&us, Buffer);
                us.Length = (cd->DosPath.Length < us.Length) ? cd->DosPath.Length : us.Length;
                if (RtlEqualUnicodeString(&us, &cd->DosPath, TRUE))
                {
                    Length = ((cd->DosPath.Length / sizeof(WCHAR)) - PrefixCut) + ((InputPathType == 1) ? 8 : 4);
                    RelativeName->RelativeName.Buffer = NewBuffer + Length;
                    RelativeName->RelativeName.Length = NtName->Length - (USHORT)(Length * sizeof(WCHAR));
                    RelativeName->RelativeName.MaximumLength = RelativeName->RelativeName.Length;
                    RelativeName->ContainingDirectory = cd->Handle;
                }
            }
        }
    }

    /* Done */
    RtlReleasePebLock();
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlpDosPathNameToRelativeNtPathName_U(IN BOOLEAN HaveRelative,
                                      IN PCWSTR DosName,
                                      OUT PUNICODE_STRING NtName,
                                      OUT PCWSTR *PartName,
                                      OUT PRTL_RELATIVE_NAME_U RelativeName)
{
    NTSTATUS Status;
    UNICODE_STRING NameString;

    /* Create the unicode name */
    Status = RtlInitUnicodeStringEx(&NameString, DosName);
    if (NT_SUCCESS(Status))
    {
        /* Call the unicode function */
        Status = RtlpDosPathNameToRelativeNtPathName_Ustr(HaveRelative,
                                                          &NameString,
                                                          NtName,
                                                          PartName,
                                                          RelativeName);
    }

    /* Return status */
    return Status;
}

BOOLEAN
NTAPI
RtlDosPathNameToRelativeNtPathName_Ustr(IN PCUNICODE_STRING DosName,
                                        OUT PUNICODE_STRING NtName,
                                        OUT PCWSTR *PartName,
                                        OUT PRTL_RELATIVE_NAME_U RelativeName)
{
    /* Call the internal function */
    ASSERT(RelativeName);
    return NT_SUCCESS(RtlpDosPathNameToRelativeNtPathName_Ustr(TRUE,
                                                               DosName,
                                                               NtName,
                                                               PartName,
                                                               RelativeName));
}

BOOLEAN
NTAPI
RtlDoesFileExists_UstrEx(IN PCUNICODE_STRING FileName,
                         IN BOOLEAN SucceedIfBusy)
{
    BOOLEAN Result;
    RTL_RELATIVE_NAME_U RelativeName;
    UNICODE_STRING NtPathName;
    PVOID Buffer;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    FILE_BASIC_INFORMATION BasicInformation;

    /* Validate the input */
    if (!FileName) return FALSE;

    /* Get the NT Path */
    Result = RtlDosPathNameToRelativeNtPathName_Ustr(FileName,
                                                     &NtPathName,
                                                     NULL,
                                                     &RelativeName);
    if (!Result) return FALSE;

    /* Save the buffer */
    Buffer = NtPathName.Buffer;

    /* Check if we have a relative name */
    if (RelativeName.RelativeName.Length)
    {
        /* Use it */
        NtPathName = RelativeName.RelativeName;
    }
    else
    {
        /* Otherwise ignore it */
        RelativeName.ContainingDirectory = NULL;
    }

    /* Initialize the object attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               &NtPathName,
                               OBJ_CASE_INSENSITIVE,
                               RelativeName.ContainingDirectory,
                               NULL);

    /* Query the attributes and free the buffer now */
    Status = ZwQueryAttributesFile(&ObjectAttributes, &BasicInformation);
    RtlReleaseRelativeName(&RelativeName);
    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);

    /* Check if we failed */
    if (!NT_SUCCESS(Status))
    {
        /* Check if we failed because the file is in use */
        if ((Status == STATUS_SHARING_VIOLATION) ||
            (Status == STATUS_ACCESS_DENIED))
        {
            /* Check if the caller wants this to be considered OK */
            Result = SucceedIfBusy ? TRUE : FALSE;
        }
        else
        {
            /* A failure because the file didn't exist */
            Result = FALSE;
        }
    }
    else
    {
        /* The file exists */
        Result = TRUE;
    }

    /* Return the result */
    return Result;
}

BOOLEAN
NTAPI
RtlDoesFileExists_UStr(IN PUNICODE_STRING FileName)
{
    /* Call the updated API */
    return RtlDoesFileExists_UstrEx(FileName, TRUE);
}

BOOLEAN
NTAPI
RtlDoesFileExists_UEx(IN PCWSTR FileName,
                      IN BOOLEAN SucceedIfBusy)
{
    UNICODE_STRING NameString;

    /* Create the unicode name*/
    if (NT_SUCCESS(RtlInitUnicodeStringEx(&NameString, FileName)))
    {
        /* Call the unicode function */
        return RtlDoesFileExists_UstrEx(&NameString, SucceedIfBusy);
    }

    /* Fail */
    return FALSE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
VOID
NTAPI
RtlReleaseRelativeName(IN PRTL_RELATIVE_NAME_U RelativeName)
{
    /* Check if a directory reference was grabbed */
    if (RelativeName->CurDirRef)
    {
        /* FIXME: Not yet supported */
        UNIMPLEMENTED;
        RelativeName->CurDirRef = NULL;
    }
}

/*
 * @implemented
 */
ULONG
NTAPI
RtlGetLongestNtPathLength(VOID)
{
    /*
     * The longest NT path is a DOS path that actually sits on a UNC path (ie:
     * a mapped network drive), which is accessed through the DOS Global?? path.
     * This is, and has always been equal to, 269 characters, except in Wine
     * which claims this is 277. Go figure.
     */
    return (MAX_PATH + RtlpDosDevicesUncPrefix.Length + sizeof(ANSI_NULL));
}

/*
 * @implemented
 */
ULONG
NTAPI
RtlDetermineDosPathNameType_U(IN PCWSTR Path)
{
    DPRINT("RtlDetermineDosPathNameType_U %S\n", Path);

    /* Validate the input */
    if (!Path) return RtlPathTypeUnknown;

    /* Unlike the newer RtlDetermineDosPathNameType_U we assume 4 characters */
    if (IS_PATH_SEPARATOR(Path[0]))
    {
        if (!IS_PATH_SEPARATOR(Path[1])) return RtlPathTypeRooted;                /* \x             */
        if ((Path[2] != L'.') && (Path[2] != L'?')) return RtlPathTypeUncAbsolute;/* \\x            */
        if (IS_PATH_SEPARATOR(Path[3])) return RtlPathTypeLocalDevice;            /* \\.\x or \\?\x */
        if (Path[3]) return RtlPathTypeUncAbsolute;                               /* \\.x or \\?x   */
        return RtlPathTypeRootLocalDevice;                                        /* \\. or \\?     */
    }
    else
    {
        if (!(Path[0]) || (Path[1] != L':')) return RtlPathTypeRelative;          /* x              */
        if (IS_PATH_SEPARATOR(Path[2])) return RtlPathTypeDriveAbsolute;          /* x:\            */
        return RtlPathTypeDriveRelative;                                          /* x:             */
    }
}

/*
 * @implemented
 */
ULONG
NTAPI
RtlIsDosDeviceName_U(IN PWSTR Path)
{
    UNICODE_STRING PathString;
    NTSTATUS Status;

    /* Build the string */
    Status = RtlInitUnicodeStringEx(&PathString, Path);
    if (!NT_SUCCESS(Status)) return 0;

    /*
     * Returns 0 if name is not valid DOS device name, or DWORD with
     * offset in bytes to DOS device name from beginning of buffer in high word
     * and size in bytes of DOS device name in low word
     */
     return RtlIsDosDeviceName_Ustr(&PathString);
}

/*
 * @implemented
 */
ULONG
NTAPI
RtlGetCurrentDirectory_U(IN ULONG MaximumLength,
                         IN PWSTR Buffer)
{
    ULONG Length, Bytes;
    PCURDIR CurDir;
    PWSTR CurDirName;
    DPRINT("RtlGetCurrentDirectory %lu %p\n", MaximumLength, Buffer);

    /* Lock the PEB to get the current directory */
    RtlAcquirePebLock();
    CurDir = &NtCurrentPeb()->ProcessParameters->CurrentDirectory;

    /* Get the buffer and character length */
    CurDirName = CurDir->DosPath.Buffer;
    Length = CurDir->DosPath.Length / sizeof(WCHAR);
    ASSERT((CurDirName != NULL) && (Length > 0));

    /*
     * DosPath.Buffer should always have a trailing slash. There is an assert
     * below which checks for this.
     *
     * This function either returns x:\ for a root (keeping the original buffer)
     * or it returns x:\path\foo for a directory (replacing the trailing slash
     * with a NULL.
     */
    Bytes = Length * sizeof(WCHAR);
    if ((Length <= 1) || (CurDirName[Length - 2] == L':'))
    {
        /* Check if caller does not have enough space */
        if (MaximumLength <= Bytes)
        {
            /* Call has no space for it, fail, add the trailing slash */
            RtlReleasePebLock();
            return Bytes + sizeof(OBJ_NAME_PATH_SEPARATOR);
        }
    }
    else
    {
        /* Check if caller does not have enough space */
        if (MaximumLength < Bytes)
        {
            /* Call has no space for it, fail */
            RtlReleasePebLock();
            return Bytes;
        }
    }

    /* Copy the buffer since we seem to have space */
    RtlCopyMemory(Buffer, CurDirName, Bytes);

    /* The buffer should end with a path separator */
    ASSERT(Buffer[Length - 1] == OBJ_NAME_PATH_SEPARATOR);

    /* Again check for our two cases (drive root vs path) */
    if ((Length <= 1) || (Buffer[Length - 2] != L':'))
    {
        /* Replace the trailing slash with a null */
        Buffer[Length - 1] = UNICODE_NULL;
        --Length;
    }
    else
    {
        /* Append the null char since there's no trailing slash */
        Buffer[Length] = UNICODE_NULL;
    }

    /* Release PEB lock */
    RtlReleasePebLock();
    DPRINT("CurrentDirectory %S\n", Buffer);
    return Length * sizeof(WCHAR);
}

/*
 * @implemented
 */
NTSTATUS NTAPI
RtlSetCurrentDirectory_U(PUNICODE_STRING dir)
{
   UNICODE_STRING full;
   FILE_FS_DEVICE_INFORMATION device_info;
   OBJECT_ATTRIBUTES Attr;
   IO_STATUS_BLOCK iosb;
   PCURDIR cd;
   NTSTATUS Status;
   USHORT size;
   HANDLE handle = NULL;
   PWSTR ptr;

   DPRINT("RtlSetCurrentDirectory %wZ\n", dir);

   full.Buffer = NULL;

   RtlAcquirePebLock ();

   cd = (PCURDIR)&NtCurrentPeb ()->ProcessParameters->CurrentDirectory.DosPath;

   if (!RtlDosPathNameToNtPathName_U (dir->Buffer, &full, 0, 0))
   {
      RtlReleasePebLock ();
      return STATUS_OBJECT_NAME_INVALID;
   }

   DPRINT("RtlSetCurrentDirectory: full %wZ\n",&full);

   InitializeObjectAttributes (&Attr,
			       &full,
			       OBJ_CASE_INSENSITIVE | OBJ_INHERIT,
			       NULL,
			       NULL);

   Status = ZwOpenFile (&handle,
			SYNCHRONIZE | FILE_TRAVERSE,
			&Attr,
			&iosb,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);

   if (!NT_SUCCESS(Status))
   {
      RtlFreeUnicodeString( &full);
      RtlReleasePebLock ();
      return Status;
   }

   /* don't keep the directory handle open on removable media */
   if (NT_SUCCESS(ZwQueryVolumeInformationFile( handle, &iosb, &device_info,
                                                sizeof(device_info), FileFsDeviceInformation )) &&
     (device_info.Characteristics & FILE_REMOVABLE_MEDIA))
   {
      DPRINT1("don't keep the directory handle open on removable media\n");
      ZwClose( handle );
      handle = 0;
   }

   if (cd->Handle)
      ZwClose(cd->Handle);
   cd->Handle = handle;

   /* append trailing \ if missing */
   size = full.Length / sizeof(WCHAR);
   ptr = full.Buffer;
   ptr += 4;  /* skip \??\ prefix */
   size -= 4;

   /* This is ok because RtlDosPathNameToNtPathName_U returns a nullterminated string.
    * So the nullterm is replaced with \
    * -Gunnar
    */
   if (size && ptr[size - 1] != '\\') ptr[size++] = '\\';

   memcpy( cd->DosPath.Buffer, ptr, size * sizeof(WCHAR));
   cd->DosPath.Buffer[size] = 0;
   cd->DosPath.Length = size * sizeof(WCHAR);

   RtlFreeUnicodeString( &full);
   RtlReleasePebLock();

   return STATUS_SUCCESS;
}


/******************************************************************
 *		collapse_path
 *
 * Helper for RtlGetFullPathName_U.
 * Get rid of . and .. components in the path.
 */
void FORCEINLINE collapse_path( WCHAR *path, UINT mark )
{
    WCHAR *p, *next;

    /* convert every / into a \ */
    for (p = path; *p; p++) if (*p == '/') *p = '\\';

    /* collapse duplicate backslashes */
    next = path + max( 1, mark );
    for (p = next; *p; p++) if (*p != '\\' || next[-1] != '\\') *next++ = *p;
    *next = 0;

    p = path + mark;
    while (*p)
    {
        if (*p == '.')
        {
            switch(p[1])
            {
            case '\\': /* .\ component */
                next = p + 2;
                memmove( p, next, (wcslen(next) + 1) * sizeof(WCHAR) );
                continue;
            case 0:  /* final . */
                if (p > path + mark) p--;
                *p = 0;
                continue;
            case '.':
                if (p[2] == '\\')  /* ..\ component */
                {
                    next = p + 3;
                    if (p > path + mark)
                    {
                        p--;
                        while (p > path + mark && p[-1] != '\\') p--;
                    }
                    memmove( p, next, (wcslen(next) + 1) * sizeof(WCHAR) );
                    continue;
                }
                else if (!p[2])  /* final .. */
                {
                    if (p > path + mark)
                    {
                        p--;
                        while (p > path + mark && p[-1] != '\\') p--;
                        if (p > path + mark) p--;
                    }
                    *p = 0;
                    continue;
                }
                break;
            }
        }
        /* skip to the next component */
        while (*p && *p != '\\') p++;
        if (*p == '\\')
        {
            /* remove last dot in previous dir name */
            if (p > path + mark && p[-1] == '.') memmove( p-1, p, (wcslen(p) + 1) * sizeof(WCHAR) );
            else p++;
        }
    }

    /* remove trailing spaces and dots (yes, Windows really does that, don't ask) */
    while (p > path + mark && (p[-1] == ' ' || p[-1] == '.')) p--;
    *p = 0;
}



/******************************************************************
 *    skip_unc_prefix
 *
 * Skip the \\share\dir\ part of a file name. Helper for RtlGetFullPathName_U.
 */
static const WCHAR *skip_unc_prefix( const WCHAR *ptr )
{
    ptr += 2;
    while (*ptr && !IS_PATH_SEPARATOR(*ptr)) ptr++;  /* share name */
    while (IS_PATH_SEPARATOR(*ptr)) ptr++;
    while (*ptr && !IS_PATH_SEPARATOR(*ptr)) ptr++;  /* dir name */
    while (IS_PATH_SEPARATOR(*ptr)) ptr++;
    return ptr;
}


/******************************************************************
 *    get_full_path_helper
 *
 * Helper for RtlGetFullPathName_U
 * Note: name and buffer are allowed to point to the same memory spot
 */
static ULONG get_full_path_helper(
   LPCWSTR name,
   LPWSTR buffer,
   ULONG size)
{
    SIZE_T                      reqsize = 0, mark = 0, dep = 0, deplen;
    LPWSTR                      ins_str = NULL;
    LPCWSTR                     ptr;
    const UNICODE_STRING*       cd;
    WCHAR                       tmp[4];

    /* return error if name only consists of spaces */
    for (ptr = name; *ptr; ptr++) if (*ptr != ' ') break;
    if (!*ptr) return 0;

    RtlAcquirePebLock();

    //cd = &((PCURDIR)&NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->CurrentDirectory.DosPath)->DosPath;
    cd = &NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->CurrentDirectory.DosPath;

    switch (RtlDetermineDosPathNameType_U(name))
    {
    case RtlPathTypeUncAbsolute:              /* \\foo   */
        ptr = skip_unc_prefix( name );
        mark = (ptr - name);
        break;

    case RtlPathTypeLocalDevice:           /* \\.\foo */
        mark = 4;
        break;

    case RtlPathTypeDriveAbsolute:   /* c:\foo  */
        reqsize = sizeof(WCHAR);
        tmp[0] = towupper(name[0]);
        ins_str = tmp;
        dep = 1;
        mark = 3;
        break;

    case RtlPathTypeDriveRelative:   /* c:foo   */
        dep = 2;
        if (towupper(name[0]) != towupper(cd->Buffer[0]) || cd->Buffer[1] != ':')
        {
            UNICODE_STRING      var, val;

            tmp[0] = '=';
            tmp[1] = name[0];
            tmp[2] = ':';
            tmp[3] = '\0';
            var.Length = 3 * sizeof(WCHAR);
            var.MaximumLength = 4 * sizeof(WCHAR);
            var.Buffer = tmp;
            val.Length = 0;
            val.MaximumLength = (USHORT)size;
            val.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, size);
            if (val.Buffer == NULL)
            {
                reqsize = 0;
                goto done;
            }

            switch (RtlQueryEnvironmentVariable_U(NULL, &var, &val))
            {
            case STATUS_SUCCESS:
                /* FIXME: Win2k seems to check that the environment variable actually points
                 * to an existing directory. If not, root of the drive is used
                 * (this seems also to be the only spot in RtlGetFullPathName that the
                 * existence of a part of a path is checked)
                 */
                /* fall thru */
            case STATUS_BUFFER_TOO_SMALL:
                reqsize = val.Length + sizeof(WCHAR); /* append trailing '\\' */
                val.Buffer[val.Length / sizeof(WCHAR)] = '\\';
                ins_str = val.Buffer;
                break;
            case STATUS_VARIABLE_NOT_FOUND:
                reqsize = 3 * sizeof(WCHAR);
                tmp[0] = name[0];
                tmp[1] = ':';
                tmp[2] = '\\';
                ins_str = tmp;
                RtlFreeHeap(RtlGetProcessHeap(), 0, val.Buffer);
                break;
            default:
                DPRINT1("Unsupported status code\n");
                RtlFreeHeap(RtlGetProcessHeap(), 0, val.Buffer);
                break;
            }
            mark = 3;
            break;
        }
        /* fall through */

    case RtlPathTypeRelative:         /* foo     */
        reqsize = cd->Length;
        ins_str = cd->Buffer;
        if (cd->Buffer[1] != ':')
        {
            ptr = skip_unc_prefix( cd->Buffer );
            mark = ptr - cd->Buffer;
        }
        else mark = 3;
        break;

    case RtlPathTypeRooted:         /* \xxx    */
#ifdef __WINE__
        if (name[0] == '/')  /* may be a Unix path */
        {
            const WCHAR *ptr = name;
            int drive = find_drive_root( &ptr );
            if (drive != -1)
            {
                reqsize = 3 * sizeof(WCHAR);
                tmp[0] = 'A' + drive;
                tmp[1] = ':';
                tmp[2] = '\\';
                ins_str = tmp;
                mark = 3;
                dep = ptr - name;
                break;
            }
        }
#endif
        if (cd->Buffer[1] == ':')
        {
            reqsize = 2 * sizeof(WCHAR);
            tmp[0] = cd->Buffer[0];
            tmp[1] = ':';
            ins_str = tmp;
            mark = 3;
        }
        else
        {
            ptr = skip_unc_prefix( cd->Buffer );
            reqsize = (ptr - cd->Buffer) * sizeof(WCHAR);
            mark = reqsize / sizeof(WCHAR);
            ins_str = cd->Buffer;
        }
        break;

    case RtlPathTypeRootLocalDevice:         /* \\.     */
        reqsize = 4 * sizeof(WCHAR);
        dep = 3;
        tmp[0] = '\\';
        tmp[1] = '\\';
        tmp[2] = '.';
        tmp[3] = '\\';
        ins_str = tmp;
        mark = 4;
        break;

    case RtlPathTypeUnknown:
        goto done;
    }

    /* enough space ? */
    deplen = wcslen(name + dep) * sizeof(WCHAR);
    if (reqsize + deplen + sizeof(WCHAR) > size)
    {
        /* not enough space, return need size (including terminating '\0') */
        reqsize += deplen + sizeof(WCHAR);
        goto done;
    }

    memmove(buffer + reqsize / sizeof(WCHAR), name + dep, deplen + sizeof(WCHAR));
    if (reqsize) memcpy(buffer, ins_str, reqsize);
    reqsize += deplen;

    if (ins_str != tmp && ins_str != cd->Buffer)
        RtlFreeHeap(RtlGetProcessHeap(), 0, ins_str);

    collapse_path( buffer, (ULONG)mark );
    reqsize = wcslen(buffer) * sizeof(WCHAR);

done:
    RtlReleasePebLock();
    return (ULONG)reqsize;
}


/******************************************************************
 *    RtlGetFullPathName_U  (NTDLL.@)
 *
 * Returns the number of bytes written to buffer (not including the
 * terminating NULL) if the function succeeds, or the required number of bytes
 * (including the terminating NULL) if the buffer is too small.
 *
 * file_part will point to the filename part inside buffer (except if we use
 * DOS device name, in which case file_in_buf is NULL)
 *
 * @implemented
 */
ULONG NTAPI RtlGetFullPathName_U(
   const WCHAR* name,
   ULONG size,
   WCHAR* buffer,
   WCHAR** file_part)
{
    WCHAR*      ptr;
    ULONG       dosdev;
    ULONG       reqsize;

    DPRINT("RtlGetFullPathName_U(%S %lu %p %p)\n", name, size, buffer, file_part);

    if (!name || !*name) return 0;

    if (file_part) *file_part = NULL;

    /* check for DOS device name */
    dosdev = RtlIsDosDeviceName_U((WCHAR*)name);
    if (dosdev)
    {
        DWORD   offset = HIWORD(dosdev) / sizeof(WCHAR); /* get it in WCHARs, not bytes */
        DWORD   sz = LOWORD(dosdev); /* in bytes */

        if (8 + sz + 2 > size) return sz + 10;
        wcscpy(buffer, DeviceRootW);
        memmove(buffer + 4, name + offset, sz);
        buffer[4 + sz / sizeof(WCHAR)] = '\0';
        /* file_part isn't set in this case */
        return sz + 8;
    }

    reqsize = get_full_path_helper(name, buffer, size);
    if (!reqsize) return 0;
    if (reqsize > size)
    {
        LPWSTR tmp = RtlAllocateHeap(RtlGetProcessHeap(), 0, reqsize);
        if (tmp == NULL) return 0;
        reqsize = get_full_path_helper(name, tmp, reqsize);
        if (reqsize + sizeof(WCHAR) > size)  /* it may have worked the second time */
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, tmp);
            return reqsize + sizeof(WCHAR);
        }
        memcpy( buffer, tmp, reqsize + sizeof(WCHAR) );
        RtlFreeHeap(RtlGetProcessHeap(), 0, tmp);
    }

    /* find file part */
    if (file_part && (ptr = wcsrchr(buffer, '\\')) != NULL && ptr >= buffer + 2 && *++ptr)
        *file_part = ptr;
    return reqsize;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlDosPathNameToNtPathName_U(IN PCWSTR DosName,
                             OUT PUNICODE_STRING NtName,
                             OUT PCWSTR *PartName,
                             OUT PRTL_RELATIVE_NAME_U RelativeName)
{
    /* Call the internal function */
    return NT_SUCCESS(RtlpDosPathNameToRelativeNtPathName_U(FALSE,
                                                            DosName,
                                                            NtName,
                                                            PartName,
                                                            RelativeName));
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlDosPathNameToNtPathName_U_WithStatus(IN PCWSTR DosName,
                                        OUT PUNICODE_STRING NtName,
                                        OUT PCWSTR *PartName,
                                        OUT PRTL_RELATIVE_NAME_U RelativeName)
{
    /* Call the internal function */
    return RtlpDosPathNameToRelativeNtPathName_U(FALSE,
                                                 DosName,
                                                 NtName,
                                                 PartName,
                                                 RelativeName);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlDosPathNameToRelativeNtPathName_U(IN PWSTR DosName,
                                     OUT PUNICODE_STRING NtName,
                                     OUT PCWSTR *PartName,
                                     OUT PRTL_RELATIVE_NAME_U RelativeName)
{
    /* Call the internal function */
    ASSERT(RelativeName);
    return NT_SUCCESS(RtlpDosPathNameToRelativeNtPathName_U(TRUE,
                                                            DosName,
                                                            NtName,
                                                            PartName,
                                                            RelativeName));
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlDosPathNameToRelativeNtPathName_U_WithStatus(IN PWSTR DosName,
                                                OUT PUNICODE_STRING NtName,
                                                OUT PCWSTR *PartName,
                                                OUT PRTL_RELATIVE_NAME_U RelativeName)
{
    /* Call the internal function */
    ASSERT(RelativeName);
    return RtlpDosPathNameToRelativeNtPathName_U(TRUE,
                                                 DosName,
                                                 NtName,
                                                 PartName,
                                                 RelativeName);
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlNtPathNameToDosPathName(ULONG Unknown1, ULONG Unknown2, ULONG Unknown3, ULONG Unknown4)
{
    DPRINT1("RtlNtPathNameToDosPathName: stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
ULONG
NTAPI
RtlDosSearchPath_U(IN PCWSTR Path,
                   IN PCWSTR FileName,
                   IN PCWSTR Extension,
                   IN ULONG Size,
                   IN PWSTR Buffer,
                   OUT PWSTR *PartName)
{
    NTSTATUS Status;
    ULONG ExtensionLength, Length, FileNameLength, PathLength;
    UNICODE_STRING TempString;
    PWCHAR NewBuffer, BufferStart;
    PCWSTR p;

    /* Validate the input */
    if (!(Path) || !(FileName)) return 0;

    /* Check if this is an absolute path */
    if (RtlDetermineDosPathNameType_U(FileName) != RtlPathTypeRelative)
    {
        /* Check if the file exists */
        if (RtlDoesFileExists_UEx(FileName, TRUE))
        {
            /* Get the full name, which does the DOS lookup */
            return RtlGetFullPathName_U(FileName, Size, Buffer, PartName);
        }

        /* Doesn't exist, so fail */
        return 0;
    }

    /* Scan the filename */
    p = FileName;
    while (*p)
    {
        /* Looking for an extension */
        if (*p == '.')
        {
            /* No extension string needed -- it's part of the filename */
            Extension = NULL;
            break;
        }

        /* Next character */
        p++;
    }

    /* Do we have an extension? */
    if (!Extension)
    {
        /* Nope, don't worry about one */
        ExtensionLength = 0;
    }
    else
    {
        /* Build a temporary string to get the extension length */
        Status = RtlInitUnicodeStringEx(&TempString, Extension);
        if (!NT_SUCCESS(Status)) return 0;
        ExtensionLength = TempString.Length;
    }

    /* Build a temporary string to get the path length */
    Status = RtlInitUnicodeStringEx(&TempString, Path);
    if (!NT_SUCCESS(Status)) return 0;
    PathLength = TempString.Length;

    /* Build a temporary string to get the filename length */
    Status = RtlInitUnicodeStringEx(&TempString, FileName);
    if (!NT_SUCCESS(Status)) return 0;
    FileNameLength = TempString.Length;

    /* Allocate the buffer for the new string name */
    NewBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                0,
                                FileNameLength +
                                ExtensionLength +
                                PathLength +
                                3 * sizeof(WCHAR));
    if (!NewBuffer)
    {
        /* Fail the call */
        DbgPrint("%s: Failing due to out of memory (RtlAllocateHeap failure)\n",
                 __FUNCTION__);
        return 0;
    }

    /* Final loop to build the path */
    while (TRUE)
    {
        /* Check if we have a valid character */
        BufferStart = NewBuffer;
        if (*Path)
        {
            /* Loop as long as there's no semicolon */
            while (*Path != ';')
            {
                /* Copy the next character */
                *BufferStart++ = *Path++;
                if (!*Path) break;
            }

            /* We found a semi-colon, to stop path processing on this loop */
            if (*Path == ';') ++Path;
        }

        /* Add a terminating slash if needed */
        if ((BufferStart != NewBuffer) && (BufferStart[-1] != '\\'))
        {
            *BufferStart++ = '\\';
        }

        /* Bail out if we reached the end */
        if (!*Path) Path = NULL;

        /* Copy the file name and check if an extension is needed */
        RtlCopyMemory(BufferStart, FileName, FileNameLength);
        if (ExtensionLength)
        {
            /* Copy the extension too */
            RtlCopyMemory((PCHAR)BufferStart + FileNameLength,
                          Extension,
                          ExtensionLength + sizeof(WCHAR));
        }
        else
        {
            /* Just NULL-terminate */
            *(PWCHAR)((PCHAR)BufferStart + FileNameLength) = UNICODE_NULL;
        }

        /* Now, does this file exist? */
        if (RtlDoesFileExists_UEx(NewBuffer, FALSE))
        {
            /* Call the full-path API to get the length */
            Length = RtlGetFullPathName_U(NewBuffer, Size, Buffer, PartName);
            break;
        }

        /* If we got here, path doesn't exist, so fail the call */
        Length = 0;
        if (!Path) break;
    }

    /* Free the allocation and return the length */
    RtlFreeHeap(RtlGetProcessHeap(), 0, NewBuffer);
    return Length;
}

/*
 * @implemented
 */
ULONG
NTAPI
RtlGetFullPathName_UstrEx(IN PUNICODE_STRING FileName,
                          IN PUNICODE_STRING StaticString,
                          IN PUNICODE_STRING DynamicString,
                          IN PUNICODE_STRING *StringUsed,
                          IN PSIZE_T FilePartSize,
                          OUT PBOOLEAN NameInvalid,
                          OUT RTL_PATH_TYPE* PathType,
                          OUT PULONG LengthNeeded)
{
    NTSTATUS Status;
    PWCHAR StaticBuffer;
    PCWCH ShortName;
    ULONG Length;
    USHORT StaticLength;
    UNICODE_STRING TempDynamicString;

    /* Initialize all our locals */
    ShortName = NULL;
    StaticBuffer = NULL;
    TempDynamicString.Buffer = NULL;

    /* Initialize the input parameters */
    if (StringUsed) *StringUsed = NULL;
    if (LengthNeeded) *LengthNeeded = 0;
    if (FilePartSize) *FilePartSize = 0;

    /* Check for invalid parameters */
    if ((DynamicString) && !(StringUsed) && (StaticString))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if we did not get an input string */
    if (!StaticString)
    {
        /* Allocate one */
        StaticLength = MAX_PATH * sizeof(WCHAR);
        StaticBuffer = RtlpAllocateStringMemory(MAX_PATH * sizeof(WCHAR), TAG_USTR);
        if (!StaticBuffer) return STATUS_NO_MEMORY;
    }
    else
    {
        /* Use the one we received */
        StaticBuffer = StaticString->Buffer;
        StaticLength = StaticString->MaximumLength;
    }

    /* Call the lower-level function */
    Length = RtlGetFullPathName_Ustr(FileName,
                                     StaticLength,
                                     StaticBuffer,
                                     &ShortName,
                                     NameInvalid,
                                     PathType);
    DPRINT("Length: %d StaticBuffer: %S\n", Length, StaticBuffer);
    if (!Length)
    {
        /* Fail if it failed */
        DbgPrint("%s(%d) - RtlGetFullPathName_Ustr() returned 0\n",
                 __FUNCTION__,
                 __LINE__);
        Status = STATUS_OBJECT_NAME_INVALID;
        goto Quickie;
    }

    /* Check if it fits inside our static string */
    if ((StaticString) && (Length < StaticLength))
    {
        /* Set the final length */
        StaticString->Length = Length;

        /* Set the file part size */
        if (FilePartSize) *FilePartSize = ShortName ? (ShortName - StaticString->Buffer) : 0;

        /* Return the static string if requested */
        if (StringUsed) *StringUsed = StaticString;

        /* We are done with success */
        Status = STATUS_SUCCESS;
        goto Quickie;
    }

    /* Did we not have an input dynamic string ?*/
    if (!DynamicString)
    {
        /* Return the length we need */
        if (LengthNeeded) *LengthNeeded = Length;

        /* And fail such that the caller can try again */
        Status = STATUS_BUFFER_TOO_SMALL;
        goto Quickie;
    }

    /* Check if it fits in our static buffer */
    if ((StaticBuffer) && (Length < StaticLength))
    {
        /* NULL-terminate it */
        StaticBuffer[Length / sizeof(WCHAR)] = UNICODE_NULL;

        /* Set the settings for the dynamic string the caller sent */
        DynamicString->MaximumLength = StaticLength;
        DynamicString->Length = Length;
        DynamicString->Buffer = StaticBuffer;

        /* Set the part size */
        if (FilePartSize) *FilePartSize = ShortName ? (ShortName - StaticBuffer) : 0;

        /* Return the dynamic string if requested */
        if (StringUsed) *StringUsed = DynamicString;

        /* Do not free the static buffer on exit, and return success */
        StaticBuffer = NULL;
        Status = STATUS_SUCCESS;
        goto Quickie;
    }

    /* Now try again under the PEB lock */
    RtlAcquirePebLock();
    Length = RtlGetFullPathName_Ustr(FileName,
                                     StaticLength,
                                     StaticBuffer,
                                     &ShortName,
                                     NameInvalid,
                                     PathType);
    if (!Length)
    {
        /* It failed */
        DbgPrint("%s line %d: RtlGetFullPathName_Ustr() returned 0\n",
                 __FUNCTION__, __LINE__);
        Status = STATUS_OBJECT_NAME_INVALID;
        goto Release;
    }

    /* Check if it fits inside our static string now */
    if ((StaticString) && (Length < StaticLength))
    {
        /* Set the final length */
        StaticString->Length = Length;

        /* Set the file part size */
        if (FilePartSize) *FilePartSize = ShortName ? (ShortName - StaticString->Buffer) : 0;

        /* Return the static string if requested */
        if (StringUsed) *StringUsed = StaticString;

        /* We are done with success */
        Status = STATUS_SUCCESS;
        goto Release;
    }

    /* Check if the path won't even fit in a real string */
    if ((Length + sizeof(WCHAR)) > UNICODE_STRING_MAX_BYTES)
    {
        /* Name is way too long, fail */
        Status = STATUS_NAME_TOO_LONG;
        goto Release;
    }

    /* Allocate the string to hold the path name now */
    TempDynamicString.Buffer = RtlpAllocateStringMemory(Length + sizeof(WCHAR),
                                                        TAG_USTR);
    if (!TempDynamicString.Buffer)
    {
        /* Out of memory, fail */
        Status = STATUS_NO_MEMORY;
        goto Release;
    }

    /* Add space for a NULL terminator, and now check the full path */
    TempDynamicString.MaximumLength = Length + sizeof(UNICODE_NULL);
    Length = RtlGetFullPathName_Ustr(FileName,
                                     Length,
                                     TempDynamicString.Buffer,
                                     &ShortName,
                                     NameInvalid,
                                     PathType);
    if (!Length)
    {
        /* Some path error, so fail out */
        DbgPrint("%s line %d: RtlGetFullPathName_Ustr() returned 0\n",
                 __FUNCTION__, __LINE__);
        Status = STATUS_OBJECT_NAME_INVALID;
        goto Release;
    }

    /* It should fit in the string we just allocated */
    ASSERT(Length < (TempDynamicString.MaximumLength - sizeof(WCHAR)));
    if (Length > TempDynamicString.MaximumLength)
    {
        /* This is really weird and would mean some kind of race */
        Status = STATUS_INTERNAL_ERROR;
        goto Release;
    }

    /* Return the file part size */
    if (FilePartSize) *FilePartSize = ShortName ? (ShortName - TempDynamicString.Buffer) : 0;

    /* Terminate the whole string now */
    TempDynamicString.Buffer[Length / sizeof(WCHAR)] = UNICODE_NULL;

    /* Finalize the string and return it to the user */
    DynamicString->Buffer = TempDynamicString.Buffer;
    DynamicString->Length = Length;
    DynamicString->MaximumLength = TempDynamicString.MaximumLength;
    if (StringUsed) *StringUsed = DynamicString;

    /* Return success and make sure we don't free the buffer on exit */
    TempDynamicString.Buffer = NULL;
    Status = STATUS_SUCCESS;

Release:
    /* Release the PEB lock */
    RtlReleasePebLock();

Quickie:
    /* Free any buffers we should be freeing */
    DPRINT("Status: %lx %S %S\n", Status, StaticBuffer, TempDynamicString.Buffer);
    if ((StaticBuffer) && (StaticBuffer != StaticString->Buffer))
    {
        RtlpFreeMemory(StaticBuffer, TAG_USTR);
    }
    if (TempDynamicString.Buffer)
    {
        RtlpFreeMemory(TempDynamicString.Buffer, TAG_USTR);
    }

    /* Print out any unusual errors */
    if ((NT_ERROR(Status)) &&
        (Status != STATUS_NO_SUCH_FILE) && (Status != STATUS_BUFFER_TOO_SMALL))
    {
        DbgPrint("RTL: %s - failing on filename %wZ with status %08lx\n",
                __FUNCTION__, FileName, Status);
    }

    /* Return, we're all done */
    return Status;
}

/*
 * @implemented
 */
ULONG
NTAPI
RtlDosSearchPath_Ustr(IN ULONG Flags,
                      IN PUNICODE_STRING PathString,
                      IN PUNICODE_STRING FileNameString,
                      IN PUNICODE_STRING ExtensionString,
                      IN PUNICODE_STRING CallerBuffer,
                      IN OUT PUNICODE_STRING DynamicString OPTIONAL,
                      OUT PUNICODE_STRING* FullNameOut OPTIONAL,
                      OUT PULONG FilePartSize OPTIONAL,
                      OUT PULONG LengthNeeded OPTIONAL)
{
    WCHAR StaticCandidateBuffer[MAX_PATH];
    UNICODE_STRING StaticCandidateString;
    NTSTATUS Status;
    RTL_PATH_TYPE PathType;
    PWCHAR p, End, CandidateEnd, SegmentEnd;
    ULONG SegmentSize, NamePlusExtLength, PathSize, MaxPathSize = 0, WorstCaseLength, ByteCount;
    USHORT ExtensionLength = 0;
    PUNICODE_STRING FullIsolatedPath;
    DPRINT("DOS Path Search: %lx %wZ %wZ %wZ %wZ %wZ\n",
            Flags, PathString, FileNameString, ExtensionString, CallerBuffer, DynamicString);

    /* Initialize the input string */
    RtlInitEmptyUnicodeString(&StaticCandidateString,
                              StaticCandidateBuffer,
                              sizeof(StaticCandidateBuffer));

    /* Initialize optional arguments */
    if (FullNameOut) *FullNameOut = NULL;
    if (FilePartSize) *FilePartSize = 0;
    if (DynamicString)
    {
        DynamicString->Length = DynamicString->MaximumLength = 0;
        DynamicString->Buffer = NULL;
    }

    /* Check for invalid parameters */
    if ((Flags & ~7) ||
        !(PathString) ||
        !(FileNameString) ||
        ((CallerBuffer) && (DynamicString) && !(FullNameOut)))
    {
        /* Fail */
        DbgPrint("%s: Invalid parameters passed\n", __FUNCTION__);
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* First check what kind of path this is */
    PathType = RtlDetermineDosPathNameType_Ustr(FileNameString);

    /* Check if the caller wants to prevent relative .\ and ..\ paths */
    if ((Flags & 2) &&
         (PathType == RtlPathTypeRelative) &&
         (FileNameString->Length >= (2 * sizeof(WCHAR))) &&
         (FileNameString->Buffer[0] == L'.') &&
         ((IS_PATH_SEPARATOR(FileNameString->Buffer[1])) ||
          ((FileNameString->Buffer[1] == L'.') &&
           ((FileNameString->Length >= (3 * sizeof(WCHAR))) &&
           (IS_PATH_SEPARATOR(FileNameString->Buffer[2]))))))
    {
        /* Yes, and this path is like that, so make it seem unknown */
        PathType = RtlPathTypeUnknown;
    }

    /* Now check relative vs non-relative paths */
    if (PathType == RtlPathTypeRelative)
    {
        /* Does the caller want SxS? */
        if (Flags & 1)
        {
            /* Apply the SxS magic */
            FullIsolatedPath = NULL;
            Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                              FileNameString,
                                                              ExtensionString,
                                                              CallerBuffer,
                                                              DynamicString,
                                                              &FullIsolatedPath,
                                                              NULL,
                                                              FilePartSize,
                                                              LengthNeeded);
            if (NT_SUCCESS(Status))
            {
                /* We found the SxS path, return it */
                if (FullNameOut) *FullNameOut = FullIsolatedPath;
                goto Quickie;
            }
            else if (Status != STATUS_SXS_KEY_NOT_FOUND)
            {
                /* Critical SxS error, fail */
                DbgPrint("%s: Failing because call to "
                         "RtlDosApplyIsolationRedirection_Ustr(%wZ) failed with "
                          "status 0x%08lx\n",
                         __FUNCTION__,
                         FileNameString,
                         Status);
                goto Quickie;
            }
        }

        /* No SxS key found, or not requested, check if there's an extension */
        if (ExtensionString)
        {
            /* Save the extension length, and check if there's a file name */
            ExtensionLength = ExtensionString->Length;
            if (FileNameString->Length)
            {
                /* Start parsing the file name */
                End = &FileNameString->Buffer[FileNameString->Length / sizeof(WCHAR)];
                while (End > FileNameString->Buffer)
                {
                    /* If we find a path separator, there's no extension */
                    if (IS_PATH_SEPARATOR(*--End)) break;

                    /* Otherwise, did we find an extension dot? */
                    if (*End == L'.')
                    {
                        /* Ignore what the caller sent it, use the filename's */
                        ExtensionString = NULL;
                        ExtensionLength = 0;
                        break;
                    }
                }
            }
        }

        /* Check if we got a path */
        if (PathString->Length)
        {
            /* Start parsing the path name, looking for path separators */
            End = &PathString->Buffer[PathString->Length / sizeof(WCHAR)];
            p = End;
            while ((p > PathString->Buffer) && (*--p == L';'))
            {
                /* This is the size of the path -- handle a trailing slash */
                PathSize = End - p - 1;
                if ((PathSize) && !(IS_PATH_SEPARATOR(*(End - 1)))) PathSize++;

                /* Check if we found a bigger path than before */
                if (PathSize > MaxPathSize) MaxPathSize = PathSize;

                /* Keep going with the path after this path separator */
                End = p;
            }

            /* This is the trailing path, run the same code as above */
            PathSize = End - p;
            if ((PathSize) && !(IS_PATH_SEPARATOR(*(End - 1)))) PathSize++;
            if (PathSize > MaxPathSize) MaxPathSize = PathSize;

            /* Finally, convert the largest path size into WCHAR */
            MaxPathSize *= sizeof(WCHAR);
        }

        /* Use the extension, the file name, and the largest path as the size */
        WorstCaseLength = ExtensionLength +
                          FileNameString->Length +
                          MaxPathSize +
                          sizeof(UNICODE_NULL);
        if (WorstCaseLength > UNICODE_STRING_MAX_BYTES)
        {
            /* It has to fit in a registry string, if not, fail here */
            DbgPrint("%s returning STATUS_NAME_TOO_LONG because the computed "
                     "worst case file name length is %Iu bytes\n",
                     __FUNCTION__,
                     WorstCaseLength);
            Status = STATUS_NAME_TOO_LONG;
            goto Quickie;
        }

        /* Scan the path now, to see if we can find the file */
        p = PathString->Buffer;
        End = &p[PathString->Length / sizeof(WCHAR)];
        while (p < End)
        {
            /* Find out where this path ends */
            for (SegmentEnd = p;
                 ((SegmentEnd != End) && (*SegmentEnd != L';'));
                 SegmentEnd++);

            /* Compute the size of this path */
            ByteCount = SegmentSize = (SegmentEnd - p) * sizeof(WCHAR);

            /* Handle trailing slash if there isn't one */
            if ((SegmentSize) && !(IS_PATH_SEPARATOR(*(SegmentEnd - 1))))
            {
                /* Add space for one */
                SegmentSize += sizeof(OBJ_NAME_PATH_SEPARATOR);
            }

            /* Now check if our initial static buffer is too small */
            if (StaticCandidateString.MaximumLength <
                (SegmentSize + ExtensionLength + FileNameString->Length))
            {
                /* At this point we should've been using our static buffer */
                ASSERT(StaticCandidateString.Buffer == StaticCandidateBuffer);
                if (StaticCandidateString.Buffer != StaticCandidateBuffer)
                {
                    /* Something is really messed up if this was the dynamic string */
                    DbgPrint("%s: internal error #1; "
                             "CandidateString.Buffer = %p; "
                             "StaticCandidateBuffer = %p\n",
                            __FUNCTION__,
                            StaticCandidateString.Buffer,
                            StaticCandidateBuffer);
                    Status = STATUS_INTERNAL_ERROR;
                    goto Quickie;
                }

                /* We checked before that the maximum possible size shoudl fit! */
                ASSERT((SegmentSize + FileNameString->Length + ExtensionLength) <
                        UNICODE_STRING_MAX_BYTES);
                if ((SegmentSize + ExtensionLength + FileNameString->Length) >
                    (UNICODE_STRING_MAX_BYTES - sizeof(WCHAR)))
                {
                    /* For some reason it's not fitting anymore. Something messed up */
                    DbgPrint("%s: internal error #2; SegmentSize = %u, "
                             "FileName->Length = %u, DefaultExtensionLength = %u\n",
                             __FUNCTION__,
                             SegmentSize,
                             FileNameString->Length,
                             ExtensionLength);
                    Status = STATUS_INTERNAL_ERROR;
                    goto Quickie;
                }

                /* Now allocate the dynamic string */
                StaticCandidateString.MaximumLength = FileNameString->Length +
                                                      WorstCaseLength;
                StaticCandidateString.Buffer = RtlpAllocateStringMemory(WorstCaseLength,
                                                                        TAG_USTR);
                if (!StaticCandidateString.Buffer)
                {
                    /* Out of memory, fail */
                    DbgPrint("%s: Unable to allocate %u byte buffer for path candidate\n",
                             __FUNCTION__,
                             StaticCandidateString.MaximumLength);
                    Status = STATUS_NO_MEMORY;
                    goto Quickie;
                }
            }

            /* Copy the path in the string */
            RtlCopyMemory(StaticCandidateString.Buffer, p, ByteCount);

            /* Get to the end of the string, and add the trailing slash if missing */
            CandidateEnd = &StaticCandidateString.Buffer[ByteCount / sizeof(WCHAR)];
            if ((SegmentSize) && (SegmentSize != ByteCount))
            {
                *CandidateEnd++ = OBJ_NAME_PATH_SEPARATOR;
            }

            /* Copy the filename now */
            RtlCopyMemory(CandidateEnd,
                          FileNameString->Buffer,
                          FileNameString->Length);
            CandidateEnd += (FileNameString->Length / sizeof(WCHAR));

            /* Check if there was an extension */
            if (ExtensionString)
            {
                /* Copy the extension too */
                RtlCopyMemory(CandidateEnd,
                              ExtensionString->Buffer,
                              ExtensionString->Length);
                          CandidateEnd += (ExtensionString->Length / sizeof(WCHAR));
            }

            /* We are done, terminate it */
            *CandidateEnd = UNICODE_NULL;

            /* Now set the final length of the string so it becomes valid */
            StaticCandidateString.Length = (CandidateEnd -
                                            StaticCandidateString.Buffer) *
                                           sizeof(WCHAR);

            /* Check if this file exists */
            DPRINT("BUFFER: %S\n", StaticCandidateString.Buffer);
            if (RtlDoesFileExists_UEx(StaticCandidateString.Buffer, FALSE))
            {
                /* Awesome, it does, now get the full path */
                Status = RtlGetFullPathName_UstrEx(&StaticCandidateString,
                                                   CallerBuffer,
                                                   DynamicString,
                                                   (PUNICODE_STRING*)FullNameOut,
                                                   FilePartSize,
                                                   NULL,
                                                   &PathType,
                                                   LengthNeeded);
                if (!(NT_SUCCESS(Status)) &&
                    ((Status != STATUS_NO_SUCH_FILE) &&
                     (Status != STATUS_BUFFER_TOO_SMALL)))
                {
                    DbgPrint("%s: Failing because we thought we found %wZ on "
                             "the search path, but RtlGetfullPathNameUStrEx() "
                             "returned %08lx\n",
                             __FUNCTION__,
                             Status);
                }
                DPRINT("STatus: %lx BUFFER: %S\n", Status, CallerBuffer->Buffer);
                goto Quickie;
            }
            else
            {
                /* Otherwise, move to the next path */
                if (SegmentEnd != End)
                {
                    /* Handle the case of the path separator trailing */
                    p = SegmentEnd + 1;
                }
                else
                {
                    p = SegmentEnd;
                }
            }
        }

        /* Loop finished and we didn't break out -- fail */
        Status = STATUS_NO_SUCH_FILE;
    }
    else
    {
        /* We have a full path, so check if it does exist */
        DPRINT("%wZ\n", FileNameString);
        if (!RtlDoesFileExists_UstrEx(FileNameString, TRUE))
        {
            /* It doesn't exist, did we have an extension? */
            if (!(ExtensionString) || !(ExtensionString->Length))
            {
                /* No extension, so just fail */
                Status = STATUS_NO_SUCH_FILE;
                goto Quickie;
            }

            /* There was an extension, check if the filename already had one */
            if (!(Flags & 4) && (FileNameString->Length))
            {
                /* Parse the filename */
                p = FileNameString->Buffer;
                End = &p[FileNameString->Length / sizeof(WCHAR)];
                while (End > p)
                {
                    /* If there's a path separator, there's no extension */
                    if (IS_PATH_SEPARATOR(*--End)) break;

                    /* Othwerwise, did we find an extension dot? */
                    if (*End == L'.')
                    {
                        /* File already had an extension, so fail */
                        Status = STATUS_NO_SUCH_FILE;
                        goto Quickie;
                    }
                }
            }

            /* So there is an extension, we'll try again by adding it */
            NamePlusExtLength = FileNameString->Length +
                                ExtensionString->Length +
                                sizeof(UNICODE_NULL);
            if (NamePlusExtLength > UNICODE_STRING_MAX_BYTES)
            {
                /* It won't fit in any kind of valid string, so fail */
                DbgPrint("%s: Failing because filename plus extension (%Iu bytes) is too big\n",
                         __FUNCTION__,
                         NamePlusExtLength);
                Status = STATUS_NAME_TOO_LONG;
                goto Quickie;
            }

            /* Fill it fit in our temporary string? */
            if (NamePlusExtLength > StaticCandidateString.MaximumLength)
            {
                /* It won't fit anymore, allocate a dynamic string for it */
                StaticCandidateString.MaximumLength = NamePlusExtLength;
                StaticCandidateString.Buffer = RtlpAllocateStringMemory(NamePlusExtLength,
                                                                        TAG_USTR);
                if (!StaticCandidateString.Buffer)
                {
                    /* Ran out of memory, so fail */
                    DbgPrint("%s: Failing because allocating the dynamic filename buffer failed\n",
                             __FUNCTION__);
                    Status = STATUS_NO_MEMORY;
                    goto Quickie;
                }
            }

            /* Copy the filename */
            RtlCopyMemory(StaticCandidateString.Buffer,
                          FileNameString->Buffer,
                          FileNameString->Length);

            /* Copy the extension */
            RtlCopyMemory(&StaticCandidateString.Buffer[FileNameString->Length / sizeof(WCHAR)],
                          ExtensionString->Buffer,
                          ExtensionString->Length);

            /* Now NULL-terminate */
            StaticCandidateString.Buffer[StaticCandidateString.Length / sizeof(WCHAR)] = UNICODE_NULL;

            /* Finalize the length of the string to make it valid */
            StaticCandidateString.Length = FileNameString->Length + ExtensionString->Length;
            DPRINT("SB: %wZ\n", &StaticCandidateString);

            /* And check if this file now exists */
            if (!RtlDoesFileExists_UstrEx(&StaticCandidateString, TRUE))
            {
                /* Still no joy, fail out */
                Status = STATUS_NO_SUCH_FILE;
                goto Quickie;
            }

            /* File was found, get the final full path */
            Status = RtlGetFullPathName_UstrEx(&StaticCandidateString,
                                               CallerBuffer,
                                               DynamicString,
                                               (PUNICODE_STRING*)FullNameOut,
                                               FilePartSize,
                                               NULL,
                                               &PathType,
                                               LengthNeeded);
            if (!(NT_SUCCESS(Status)) && (Status != STATUS_NO_SUCH_FILE))
            {
                DbgPrint("%s: Failing on \"%wZ\" because RtlGetfullPathNameUStrEx() "
                         "failed with status %08lx\n",
                         __FUNCTION__,
                         &StaticCandidateString,
                         Status);
            }
            DPRINT("STatus: %lx BUFFER: %S\n", Status, CallerBuffer->Buffer);
        }
        else
        {
            /* File was found on the first try, get the final full path */
            Status = RtlGetFullPathName_UstrEx(FileNameString,
                                               CallerBuffer,
                                               DynamicString,
                                               (PUNICODE_STRING*)FullNameOut,
                                               FilePartSize,
                                               NULL,
                                               &PathType,
                                               LengthNeeded);
            if (!(NT_SUCCESS(Status)) &&
                ((Status != STATUS_NO_SUCH_FILE) &&
                (Status != STATUS_BUFFER_TOO_SMALL)))
            {
                DbgPrint("%s: Failing because RtlGetfullPathNameUStrEx() on %wZ "
                         "failed with status %08lx\n",
                         __FUNCTION__,
                         FileNameString,
                         Status);
            }
            DPRINT("STatus: %lx BUFFER: %S\n", Status, CallerBuffer->Buffer);
        }
    }

Quickie:
    /* Anything that was not an error, turn into STATUS_SUCCESS */
    if (NT_SUCCESS(Status)) Status = STATUS_SUCCESS;

    /* Check if we had a dynamic string */
    if ((StaticCandidateString.Buffer) &&
        (StaticCandidateString.Buffer != StaticCandidateBuffer))
    {
        /* Free it */
        RtlFreeUnicodeString(&StaticCandidateString);
    }

    /* Return the status */
    return Status;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlDoesFileExists_U(IN PCWSTR FileName)
{
    /* Call the new function */
    return RtlDoesFileExists_UEx(FileName, TRUE);
}

/* EOF */
