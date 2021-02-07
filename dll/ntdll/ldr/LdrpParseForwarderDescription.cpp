#include <ldrp.h>
#include <ntintsafe.h>

NTSTATUS
NTAPI
LdrpParseForwarderDescription(IN PVOID ForwarderPointer,
                              IN OUT PANSI_STRING ForwardedDllName,
                              IN OUT PSTR* ForwardedFunctionName,
                              IN OUT PULONG ForwardedFunctionOrdinal)
{
    const PSTR ForwarderDescription = static_cast<PSTR>(ForwarderPointer);
    const PSTR Dot = strrchr(ForwarderDescription, '.');

    if (!Dot)
        return STATUS_INVALID_IMAGE_FORMAT;

    const ULONG_PTR Length = Dot - ForwarderDescription;

    if (Length > USHORT_MAX)
        return STATUS_INVALID_IMAGE_FORMAT;

    ForwardedDllName->Length = Length;
    ForwardedDllName->MaximumLength = Length;
    ForwardedDllName->Buffer = ForwarderDescription;

    const PSTR TargetDescription = Dot + 1;
    if (TargetDescription[0] != '#')
    {
        *ForwardedFunctionName = TargetDescription;
        *ForwardedFunctionOrdinal = 0;
        return STATUS_SUCCESS;
    }

    const PCSTR TargetOrdinalString = TargetDescription + 1;
    if (NT_SUCCESS(RtlCharToInteger(TargetOrdinalString, 0, ForwardedFunctionOrdinal)))
    {
        *ForwardedFunctionName = NULL;
        return STATUS_SUCCESS;
    }
    else
    {
        return STATUS_INVALID_IMAGE_FORMAT;
    }
}
