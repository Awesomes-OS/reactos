#include <ldrp.h>

#include <ntintsafe.h>

NTSTATUS
NTAPI
LdrpAllocateFileNameBufferIfNeeded(PLDRP_UNICODE_STRING_BUNDLE Destination, ULONG Length)
{
    PWSTR NewBuffer;

    if (Length <= Destination->String.MaximumLength)
        return STATUS_SUCCESS;

    if (Length > UNICODE_STRING_MAX_BYTES)
        return STATUS_NAME_TOO_LONG;

    if (Length & 1u)
        DPRINT1("LDR: %s(\"%wZ\", %lu): misalignment", __FUNCTION__, Destination, Length);

    if (!LdrpIsUsingUnicodeStringBundleBufferPtr(Destination))
    {
        NewBuffer = (PWSTR)LdrpHeapReAlloc(HEAP_ZERO_MEMORY, Destination->String.Buffer, Length);
    }
    else
    {
        NewBuffer = (PWSTR)LdrpHeapAlloc(HEAP_ZERO_MEMORY, Length);

        if (NewBuffer)
        {
            if (Destination->String.Length)
                memcpy(NewBuffer, Destination->String.Buffer, Destination->String.Length);
        }
    }

    if (NewBuffer)
    {
        Destination->String.Buffer = NewBuffer;
        Destination->String.MaximumLength = Length;
        return STATUS_SUCCESS;
    }

    return STATUS_NO_MEMORY;
}

NTSTATUS
NTAPI
LdrpAppendUnicodeStringToFilenameBuffer(PLDRP_UNICODE_STRING_BUNDLE Destination, UNICODE_STRING *Source)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (Source->Length)
    {
        Status = LdrpAllocateFileNameBufferIfNeeded(Destination, Destination->String.Length + Source->Length + sizeof(WCHAR));

        if (NT_SUCCESS(Status))
        {
            memcpy(PTR_ADD_OFFSET(Destination->String.Buffer, Destination->String.Length), Source->Buffer, Source->Length);
            Destination->String.Length += Source->Length;
            Destination->String.Buffer[Destination->String.Length / sizeof(WCHAR)] = 0;
        }
    }

    return Status;
}

NTSTATUS
NTAPI
LdrpAppendAnsiStringToFilenameBuffer(PLDRP_UNICODE_STRING_BUNDLE Destination, ANSI_STRING *Source)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (Source->Length)
    {
        const ULONG Size = RtlOemStringToUnicodeSize(Source);
        Status = LdrpAllocateFileNameBufferIfNeeded(Destination, Destination->String.Length + Size);

        if (NT_SUCCESS(Status))
        {
            UNICODE_STRING TempUnicode;
            TempUnicode.Length = 0;
            TempUnicode.MaximumLength = Destination->String.MaximumLength - Destination->String.Length;
            TempUnicode.Buffer = Destination->String.Buffer + Destination->String.Length / sizeof(WCHAR);

            RtlAnsiStringToUnicodeString(&TempUnicode, Source, FALSE);

            Destination->String.Length += TempUnicode.Length;
        }
    }

    return Status;
}
