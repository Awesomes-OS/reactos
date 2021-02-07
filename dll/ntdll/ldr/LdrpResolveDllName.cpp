#include <ldrp.h>
#include <reactos/ldrp.h>

NTSTATUS
NTAPI
LdrpResolveDllName(IN PUNICODE_STRING DosName,
                   IN OUT PLDRP_UNICODE_STRING_BUNDLE NtName,
                   OUT PUNICODE_STRING BaseDllName,
                   OUT PUNICODE_STRING FullDosName,
                   IN LDRP_LOAD_CONTEXT_FLAGS Flags)
{
    NTSTATUS Status;

    /* Display debug output if snaps are on */
    if (LdrpDebugFlags.LogTrace)
    {
        DPRINT1("LDR: %s(\"%wZ\", " LDR_HEXPTR_FMT ", " LDR_HEXPTR_FMT ", " LDR_HEXPTR_FMT ", 0x%X)\n",
                __FUNCTION__,
                DosName, NtName, BaseDllName, FullDosName, Flags.Flags);
    }

    ASSERT(DosName && NtName && BaseDllName && FullDosName);

    UNICODE_STRING LocalFullDosName;

    // Expand DOS name
    if (Flags.HasFullPath)
    {
        Status = STATUS_SUCCESS;
        LocalFullDosName = *DosName;
    }
    else
    {
        Status = LdrpGetFullPath(DosName, NtName);
        LocalFullDosName = NtName->String;
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LDR: %s(...): Win32 path expand failure [0x%08lX]\n", __FUNCTION__, Status);
        goto Quickie;
    }

    if (!Flags.HasFullPath && !LdrpIsUsingUnicodeStringBundleBufferPtr(NtName))
    {
        // Reset the pointer without freeing non-static buffer to preserve it inside LocalFullDosName
        LdrpCreateUnicodeStringBundlePtr(NtName);
    }
    else
    {
        const PWSTR OldBuffer = LocalFullDosName.Buffer;
        const USHORT OldLength = LocalFullDosName.Length;
        Status = LdrpAllocateUnicodeString(&LocalFullDosName, OldLength);
        if (NT_SUCCESS(Status))
        {
            RtlMoveMemory(LocalFullDosName.Buffer, OldBuffer, OldLength + sizeof(UNICODE_NULL));
            LocalFullDosName.Length = OldLength;
        }

        NtName->String.Length = 0;
    }

    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Convert LocalFullDosName to NT Name */
    Status = LdrpGetNtPathFromDosPath(&LocalFullDosName, NtName);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LDR: %s(...): LdrpGetNtPathFromDosPath -> 0x%08lX\n",
                __FUNCTION__, Status);

        switch (Status)
        {
            case STATUS_NO_SUCH_FILE:
            case STATUS_NO_MEDIA_IN_DEVICE:
            case STATUS_OBJECT_NAME_INVALID:
            case STATUS_OBJECT_NAME_NOT_FOUND:
            case STATUS_OBJECT_PATH_INVALID:
            case STATUS_OBJECT_PATH_NOT_FOUND:
            case STATUS_OBJECT_PATH_SYNTAX_BAD:
            case STATUS_DEVICE_NOT_READY:
            case STATUS_DEVICE_OFF_LINE:
                Status = STATUS_DLL_NOT_FOUND;
                break;
            default: ;
        }

        LdrpFreeUnicodeString(&LocalFullDosName);

        goto Quickie;
    }

    *FullDosName = LocalFullDosName;

    {
        WCHAR* pChar = LocalFullDosName.Buffer + LocalFullDosName.Length / sizeof(WCHAR) - 1;

        for (; pChar >= LocalFullDosName.Buffer; pChar--)
        {
            const WCHAR c = *pChar;
            if (c == '\\' || c == '/')
            {
                ++pChar;
                break;
            }
        }

        RtlInitUnicodeString(BaseDllName, pChar);
    }

Quickie:

    /* Display debug output if snaps are on */
    if (LdrpDebugFlags.LogTrace)
    {
        /* Check which output to use -- failure or success */
        if (NT_SUCCESS(Status))
        {
            DPRINT1("LDR: %s(...) -> SUCCESS\n", __FUNCTION__);
        }
        else
        {
            DPRINT1("LDR: %s(...) -> 0x%08lX\n", __FUNCTION__, Status);
        }
    }

    return Status;
}
