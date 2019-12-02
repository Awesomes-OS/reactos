#include <ldrp.h>
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/exfuncs.h>
#include <ndk/ntndk.h>

NTSTATUS
NTAPI
LdrpProcessMachineMismatch(PLDRP_LOAD_CONTEXT LoadContext)
{
    PPEB Peb = NtCurrentPeb();
    ULONG Response = ResponseCancel;
    ULONG_PTR HardErrorParameters[2];

    /* Load our header */
    PIMAGE_NT_HEADERS ImageNtHeader = RtlImageNtHeader(Peb->ImageBaseAddress);

    /* Are we an NT 3.0 image? [Do these still exist? LOL -- IAI] */
    if (ImageNtHeader->OptionalHeader.MajorSubsystemVersion <= 3)
    {
        /* Assume defaults if we don't have to run the Hard Error path */
        NTSTATUS HardErrorStatus;

        /* Reset the entrypoint, save our Dll Name */
        LoadContext->Entry->EntryPoint = NULL;
        HardErrorParameters[0] = (ULONG_PTR) &LoadContext->Entry->FullDllName;

        /* Raise the error */
        HardErrorStatus = ZwRaiseHardError(STATUS_IMAGE_MACHINE_TYPE_MISMATCH,
                                           1,
                                           1,
                                           HardErrorParameters,
                                           OptionOkCancel,
                                           &Response);

        /* Check if the user pressed cancel */
        if (NT_SUCCESS(HardErrorStatus))
        {
            if (Response == ResponseCancel)
            {
                /* Yup, so increase fatal error count if we are initializing */
                if (LdrpInLdrInit)
                    LdrpFatalHardErrorCount++;

                /* Return failure */
                return STATUS_INVALID_IMAGE_FORMAT;
            }
            LoadContext->Entry->ImageDll = FALSE;
        }
    }

    return STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
}

NTSTATUS
NTAPI
LdrpMinimalMapModule(PLDRP_LOAD_CONTEXT LoadContext, HANDLE SectionHandle, PSIZE_T ViewSize)
{
    const PTEB Teb = NtCurrentTeb();
    PVOID *ViewBase = &LoadContext->Entry->DllBase;
    NTSTATUS Status;

    /* Stuff the image name in the TIB, for the debugger */
    const PVOID ArbitraryUserPointer = Teb->NtTib.ArbitraryUserPointer;
    Teb->NtTib.ArbitraryUserPointer = LoadContext->Entry->FullDllName.Buffer;

    const ULONG AllocationType = 0;

    // AllocationType |= MEM_DIFFERENT_IMAGE_BASE_OK;
    // AllocationType |= MEM_MAPPED; // if enclave

    /* Map the DLL */
    Status = NtMapViewOfSection(SectionHandle,
                                NtCurrentProcess(),
                                ViewBase,
                                0,
                                0,
                                NULL,
                                ViewSize,
                                ViewShare,
                                AllocationType,
                                PAGE_READONLY);

    /* Restore */
    Teb->NtTib.ArbitraryUserPointer = ArbitraryUserPointer;

    /* Fail if we couldn't map it */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LDR: %s([%wZ], %p, ...): NtMapViewOfSection -> 0x%08lX\n",
            __FUNCTION__,
            &LoadContext->DllName,
            SectionHandle,
            Status);

        return Status;
    }

    /* Show debug message */
    if (ShowSnaps)
    {
        DPRINT1("LDR: %s(%wZ[full:%wZ | base:%wZ], %p, ...)\n",
                __FUNCTION__,
                &LoadContext->DllName,
                &LoadContext->Entry->FullDllName,
                &LoadContext->Entry->BaseDllName,
                SectionHandle);
    }

    /* Check for invalid CPU Image */
    if (Status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH)
    {
        Status = LdrpProcessMachineMismatch(LoadContext);
    }

    if (*ViewBase && (!NT_SUCCESS(Status) || Status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH))
    {
        NtUnmapViewOfSection(NtCurrentProcess(), *ViewBase);
        LoadContext->Entry->DllBase = NULL;
    }

    return Status;
}
