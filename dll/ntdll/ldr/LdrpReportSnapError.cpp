#include <ldrp.h>
#include <ndk/exfuncs.h>

void
NTAPI
LdrpReportSnapError(IN PVOID BaseAddress OPTIONAL,
                    IN PUNICODE_STRING ModuleName OPTIONAL,
                    IN PANSI_STRING FunctionName OPTIONAL,
                    IN ULONG Ordinal OPTIONAL,
                    NTSTATUS Status,
                    BOOLEAN Static)
{
    switch (Status)
    {
        case STATUS_ORDINAL_NOT_FOUND:
        {
            DPRINT1("Locating export at ordinal %lu failed with status: 0x%08lX. [BaseAddress=" LDR_HEXPTR_FMT ", ModuleName=%wZ]\n",
                    Ordinal,
                    Status,
                    BaseAddress,
                    ModuleName);

            break;
        }

        case STATUS_ENTRYPOINT_NOT_FOUND:
        {
            DPRINT1("Locating export \"%Z\" failed with status: 0x%08lX. [BaseAddress=" LDR_HEXPTR_FMT ", ModuleName=%wZ]\n",
                    FunctionName,
                    Status,
                    BaseAddress,
                    ModuleName);

            break;
        }
    }

    if (LdrpDebugFlags.BreakInDebugger)
        __debugbreak();

    // Is this a static snap?
    if (Static)
    {
        ULONG_PTR HardErrorParameters[MAXIMUM_HARDERROR_PARAMETERS];
        ULONG HardErrorParameterCount = 0;
        UNICODE_STRING FunctionNameUnicode;
        ULONG HardErrorMask = 0;

        if (BaseAddress)
        {
            HardErrorParameters[HardErrorParameterCount] = (ULONG_PTR) BaseAddress;

            HardErrorParameterCount++;
        }

        if (ModuleName)
        {
            HardErrorParameters[HardErrorParameterCount] = (ULONG_PTR) ModuleName;

            HardErrorMask |= 1u << HardErrorParameterCount;

            HardErrorParameterCount++;
        }

        if (FunctionName)
        {
            RtlAnsiStringToUnicodeString(&FunctionNameUnicode, FunctionName, TRUE);

            HardErrorParameters[HardErrorParameterCount] = (ULONG_PTR) &FunctionNameUnicode;

            HardErrorMask |= 1u << HardErrorParameterCount;

            HardErrorParameterCount++;
        }

        if (Ordinal)
        {
            HardErrorParameters[HardErrorParameterCount] = (ULONG_PTR) Ordinal;

            HardErrorParameterCount++;
        }

        ULONG HardErrorResponse = 0;

        NTSTATUS ErrorStatus = NtRaiseHardError(Status,
                                                HardErrorParameterCount,
                                                HardErrorMask,
                                                HardErrorParameters,
                                                OptionOk,
                                                &HardErrorResponse);

        if (NT_SUCCESS(ErrorStatus) && LdrInitState != 3)
            LdrpFatalHardErrorCount++;

        if (Status == STATUS_ORDINAL_NOT_FOUND || Status == STATUS_ENTRYPOINT_NOT_FOUND)
            RtlRaiseStatus(Status);
    }
}
