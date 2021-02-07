#include <ldrp.h>
#include <reactos/ldrp.h>

NTSTATUS
NTAPI
LdrpGetFullPath(IN PUNICODE_STRING OriginalName,
                IN OUT PLDRP_UNICODE_STRING_BUNDLE ExpandedName)
{
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(OriginalName);
    ASSERT(ExpandedName);

    /* Display debug output if snaps are on */
    if (LdrpDebugFlags.LogTrace)
    {
        DPRINT1("LDR: %s(\"%wZ\", ...)\n", __FUNCTION__, OriginalName);
    }

    while (TRUE)
    {
        RTLP_PATH_INFO PathType;
        BOOLEAN InvalidName;

        /* Attempt to get the path name */
        ULONG Length = RtlGetFullPathName_Ustr(OriginalName,
                                               ExpandedName->String.MaximumLength,
                                               ExpandedName->String.Buffer,
                                               NULL,
                                               &InvalidName,
                                               &PathType);
        if (InvalidName)
        {
            /* Fail */
            Status = STATUS_DLL_NOT_FOUND;
            break;
        }

        if (!(Length) || (Length > UNICODE_STRING_MAX_BYTES))
        {
            /* Fail */
            Status = STATUS_NAME_TOO_LONG;
            break;
        }

        /* Check if the length fits */
        if (Length < ExpandedName->String.MaximumLength)
        {
            /* Successful return */
            ExpandedName->String.Length = (USHORT)Length;
            break;
        }

        /* Sanity check */
        ASSERT(Length >= sizeof(WCHAR));

        /* Allocate a string */
        Status = LdrpAllocateFileNameBufferIfNeeded(ExpandedName, Length);
        if (!NT_SUCCESS(Status))
            break;

        ExpandedName->String.Length = 0;
        ExpandedName->String.Buffer[0] = UNICODE_NULL;
    }

    /* Display debug output if snaps are on */
    if (LdrpDebugFlags.LogTrace)
    {
        /* Check which output to use -- failure or success */
        if (NT_SUCCESS(Status))
        {
            DPRINT1("LDR: %s(...) -> \"%wZ\"\n", __FUNCTION__, ExpandedName);
        }
        else
        {
            DPRINT1("LDR: %s(...) -> 0x%08lX\n", __FUNCTION__, Status);
        }
    }

    /* Return status */
    return Status;
}
