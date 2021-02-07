#include <ldrp.h>

EXTERN_C
{

#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/exfuncs.h>
#include <ndk/ntndk.h>

}

NTSTATUS
NTAPI
LdrpProcessMachineMismatch(PLDRP_LOAD_CONTEXT LoadContext)
{
    ULONG Cor20HeaderSize;

    PIMAGE_COR20_HEADER Cor20Header = (PIMAGE_COR20_HEADER)RtlImageDirectoryEntryToData(
        LoadContext->Module->DllBase,
        TRUE,
        IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
        &Cor20HeaderSize);

    if (Cor20Header && Cor20Header->Flags & COMIMAGE_FLAGS_ILONLY)
    {
        LoadContext->Flags.CorImage = TRUE;
        return STATUS_SUCCESS;
    }

    /* Are we an NT 3.0 image? */
    if (LdrpAppHeaders->OptionalHeader.MajorSubsystemVersion <= 3)
    {
        /* Assume defaults if we don't have to run the Hard Error path */
        ULONG Response = ResponseCancel;
        ULONG_PTR HardErrorParameters[2];
        NTSTATUS HardErrorStatus;

        /* Reset the entrypoint, save our Dll Name */
        LoadContext->Module->EntryPoint = NULL;
        HardErrorParameters[0] = (ULONG_PTR) &LoadContext->Module->FullDllName;

        /* Raise the error */
        HardErrorStatus = NtRaiseHardError(STATUS_IMAGE_MACHINE_TYPE_MISMATCH,
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
                if (LdrInitState != 3)
                    LdrpFatalHardErrorCount++;

                /* Return failure */
                return STATUS_INVALID_IMAGE_FORMAT;
            }

            LoadContext->Module->ImageDll = FALSE;
        }

        return HardErrorStatus;
    }

    return STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
}

NTSTATUS
NTAPI
LdrpMinimalMapModule(PLDRP_LOAD_CONTEXT LoadContext, HANDLE SectionHandle, PSIZE_T ViewSize)
{
    const PTEB Teb = NtCurrentTeb();
    const PLDR_DATA_TABLE_ENTRY Entry = LoadContext->Module;
    PVOID* ViewBase = &Entry->DllBase;
    NTSTATUS Status;

    /* Stuff the image name in the TIB, for the debugger */
    const PVOID ArbitraryUserPointer = Teb->NtTib.ArbitraryUserPointer;
    Teb->NtTib.ArbitraryUserPointer = Entry->FullDllName.Buffer;

    ULONG AllocationType = 0;

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

    /* Check for invalid CPU Image */
    if (Status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH)
    {
        Status = LdrpProcessMachineMismatch(LoadContext);
    }

    if (*ViewBase && (!NT_SUCCESS(Status) || Status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH))
    {
        NtUnmapViewOfSection(NtCurrentProcess(), *ViewBase);
        *ViewBase = NULL;
    }

    if (LdrpDebugFlags.LogTrace)
    {
        DPRINT1("LDR: %s(%wZ[full:%wZ | base:%wZ], " LDR_HEXPTR_FMT ", ...) -> 0x%08lX\n",
                __FUNCTION__,
                &LoadContext->DllName,
                &Entry->FullDllName,
                &Entry->BaseDllName,
                SectionHandle,
                Status);
    }

    return Status;
}
