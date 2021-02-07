/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS NT User-Mode Library
 * FILE:            dll/ntdll/ldr/ldrinit.c
 * PURPOSE:         User-Mode Process/Thread Startup
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ldrp.h>

/* GLOBALS *******************************************************************/

UNICODE_STRING MscoreeString = RTL_CONSTANT_STRING(L"mscoree.dll");
UNICODE_STRING CorInstallRootString = RTL_CONSTANT_STRING(L"COMPLUS_InstallRoot");
UNICODE_STRING CorVersionString = RTL_CONSTANT_STRING(L"COMPLUS_Version");
ANSI_STRING LdrpCorExeMainName = RTL_CONSTANT_STRING("_CorExeMain");
ANSI_STRING LdrpCorImageUnloadingName = RTL_CONSTANT_STRING("_CorImageUnloading");
ANSI_STRING LdrpCorValidateImageName = RTL_CONSTANT_STRING("_CorValidateImage");

typedef __int32 (STDMETHODCALLTYPE LDRP_COREXEMAIN_FUNC)();
typedef HRESULT (STDMETHODCALLTYPE LDRP_CORIMAGEUNLOADING_FUNC)(IN PVOID* ImageBase);
typedef HRESULT (STDMETHODCALLTYPE LDRP_CORVALIDATEIMAGE_FUNC)(IN PVOID* ImageBase, IN LPCWSTR FileName);

static LDRP_COREXEMAIN_FUNC* LdrpCorExeMainRoutine;
static LDRP_CORIMAGEUNLOADING_FUNC* LdrpCorImageUnloadingRoutine;
static LDRP_CORVALIDATEIMAGE_FUNC* LdrpCorValidateImageRoutine;
PVOID LdrpMscoreeDllHandle;
BOOLEAN UseCOR;

NTSTATUS LdrPerformRelocations(PIMAGE_NT_HEADERS NTHeaders, PVOID ImageBase);

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
LdrpCorInitialize(OUT PLDR_DATA_TABLE_ENTRY* TargetEntry)
{
    NTSTATUS Status;
    UNICODE_STRING EnvironmentValue;
    PVOID BaseAddress = NULL;
    PUNICODE_STRING DllPath = &MscoreeString;

    LDRP_UNICODE_STRING_BUNDLE DllPathBundle;

    LdrpCreateUnicodeStringBundle(DllPathBundle);

    RtlInitEmptyUnicodeString(&EnvironmentValue, NULL, 0);
    if (RtlQueryEnvironmentVariable_U(NULL, &CorInstallRootString, &EnvironmentValue) == STATUS_BUFFER_TOO_SMALL)
    {
        if (RtlQueryEnvironmentVariable_U(NULL, &CorVersionString, &EnvironmentValue) != STATUS_BUFFER_TOO_SMALL)
        {
            Status = LdrpBuildSystem32FileName(&DllPathBundle, &MscoreeString);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("LDR .NET: LdrpBuildSystem32FileName failed [0x%08lX]\n", Status);
                return Status;
            }

            DllPath = &DllPathBundle.String;
        }
    }

    // todo: RtlQueryEnvironmentVariable_U doesn't allocate anything, it seems. Envvars are always failing.
    // todo: fix DllPathBundle leak on failure paths

    if (!NT_SUCCESS(Status = LdrLoadDll(NULL, NULL, DllPath, &BaseAddress)))
    {
        DPRINT1("LDR .NET: MSCOREE failed to load [0x%08lX]\n", Status);
        return Status;
    }

    PVOID CorExeMainAddress = NULL, CorImageUnloadingAddress = NULL, CorValidateImageAddress = NULL;
    if (!NT_SUCCESS(Status = LdrGetProcedureAddress(BaseAddress, &LdrpCorExeMainName, 0, &CorExeMainAddress)))
    {
        DPRINT1("LDR .NET: _CorExeMain not found in MSCOREE [0x%08lX]\n", Status);
        LdrUnloadDll(BaseAddress);
        return Status;
    }
    if (!NT_SUCCESS(Status = LdrGetProcedureAddress(BaseAddress, &LdrpCorImageUnloadingName, 0, &CorImageUnloadingAddress)))
    {
        DPRINT1("LDR .NET: _CorImageUnloading not found in MSCOREE [0x%08lX]\n", Status);
        LdrUnloadDll(BaseAddress);
        return Status;
    }
    if (!NT_SUCCESS(Status = LdrGetProcedureAddress(BaseAddress, &LdrpCorValidateImageName, 0, &CorValidateImageAddress)))
    {
        DPRINT1("LDR .NET: _CorValidateImage not found in MSCOREE [0x%08lX]\n", Status);
        LdrUnloadDll(BaseAddress);
        return Status;
    }

    LdrpCorExeMainRoutine = static_cast<decltype(LdrpCorExeMainRoutine)>(RtlEncodeSystemPointer(CorExeMainAddress));
    LdrpCorImageUnloadingRoutine = static_cast<decltype(LdrpCorImageUnloadingRoutine)>(RtlEncodeSystemPointer(CorImageUnloadingAddress));
    LdrpCorValidateImageRoutine = static_cast<decltype(LdrpCorValidateImageRoutine)>(RtlEncodeSystemPointer(CorValidateImageAddress));
    LdrpMscoreeDllHandle = BaseAddress;

    ASSERT(TargetEntry);

    LDR_DDAG_STATE State = LdrModulesPlaceHolder;

    Status = LdrpFindLoadedDllByAddress(BaseAddress, TargetEntry, &State);

    DPRINT1("LDR .NET: MSCOREE found: [0x%08lX]:%d\n", Status, State);

    LdrpFreeUnicodeStringBundle(DllPathBundle);

    return Status;
}

NTSTATUS
NTAPI
LdrpCorValidateImage(IN PVOID ImageBase, IN LPCWSTR FileName)
{
    /*
     * _CorValidateImage does the following (according to Microsoft Docs):
     * 1. Confirms that the image is a valid managed module.
     * 2. Changes the entry point in the image to an entry point in the common language runtime (CLR).
     * 3. For 64-bit versions of Windows, modifies the image that is in memory by transforming it from PE32 to PE32+ format.
     * 4. Returns to the loader when the managed module images are loaded.
     *
     * Vista+ calls _CorValidateImage, but 10+ doesn't!
     * Windows 10+ LDR does all these itself, without help from MSCOREE:
     * 1. LdrpCorValidateImage
     * 2. LdrpCorReplaceStartContext
     * 3. LdrpCorFixupImage
     *
     */

#if 0
    LDRP_CORVALIDATEIMAGE_FUNC* Pointer = (LDRP_CORVALIDATEIMAGE_FUNC*) RtlDecodeSystemPointer(LdrpCorValidateImageRoutine);
    HRESULT Result = Pointer(ImageBase, FileName);
    if (FAILED(Result))
    {
        // todo: unload mscoreee dll and set LdrpMscoreeDllHandle to NULL?
#if 0
        LdrUnloadDll(LdrpMscoreeDllHandle);
        LdrpMscoreeDllHandle = NULL;
#endif
    }
    return SUCCEEDED(Result) ? STATUS_SUCCESS : (NTSTATUS)Result;
#else
    // Instead it checks that no TLS information is present in the image.
    ULONG Size;
    return RtlImageDirectoryEntryToData(ImageBase, TRUE, IMAGE_DIRECTORY_ENTRY_TLS, &Size)
               ? STATUS_INVALID_IMAGE_FORMAT
               : STATUS_SUCCESS;
#endif
}

LDRP_COREXEMAIN_FUNC*
NTAPI
LdrpCorReplaceStartContext(IN PCONTEXT Context)
{
    if (!LdrpMscoreeDllHandle)
        return NULL;

    LDRP_COREXEMAIN_FUNC* Pointer = (LDRP_COREXEMAIN_FUNC*)RtlDecodeSystemPointer(LdrpCorExeMainRoutine);

#if _M_IX86
    Context->Eax = (UINT_PTR)Pointer;
#else
#error Unknown processor, see KeSetContextReturnRegister
#endif

    return Pointer;
}

BOOL
NTAPI
LdrpIsILOnlyImage(PVOID BaseAddress)
{
    ULONG Cor20HeaderSize;

    const PIMAGE_COR20_HEADER Cor20Header = (PIMAGE_COR20_HEADER)RtlImageDirectoryEntryToData(
        BaseAddress,
        TRUE,
        IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
        &Cor20HeaderSize);

    return Cor20Header
        && Cor20HeaderSize >= sizeof(IMAGE_COR20_HEADER)
        && Cor20Header->Flags & COMIMAGE_FLAGS_ILONLY;
}

NTSTATUS
NTAPI
LdrpCorProcessImports(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    LdrEntry->DdagNode->State = LdrModulesReadyToInit;
    return STATUS_SUCCESS;
}

#if defined(_M_AMD64) || defined(_WIN64)
NTSTATUS
NTAPI
LdrpCorFixupImage(IN PVOID BaseAddress)
{
    ULONG Cor20HeaderSize;

    const PIMAGE_NT_HEADERS NtHeader = RtlImageNtHeader(BaseAddress);

    ASSERT(NtHeader);

    const WORD Machine = NtHeader->FileHeader.Machine;

    const PIMAGE_COR20_HEADER Cor20Header = (PIMAGE_COR20_HEADER)RtlImageDirectoryEntryToData(
        BaseAddress,
        TRUE,
        IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
        &Cor20HeaderSize);

    ASSERT(Cor20Header);
    ASSERT(Cor20HeaderSize >= sizeof(IMAGE_COR20_HEADER));
    ASSERT(Cor20Header->Flags & COMIMAGE_FLAGS_ILONLY);

    if (NtHeader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC || Machine != IMAGE_FILE_MACHINE_I386 ||
        Cor20Header->Flags & COMIMAGE_FLAGS_32BITREQUIRED)
    {
        return SharedUserData->ImageNumberLow <= Machine && Machine <= SharedUserData->ImageNumberHigh
                   ? STATUS_SUCCESS
                   : STATUS_INVALID_IMAGE_FORMAT;
    }

    UNIMPLEMENTED;

    return STATUS_INVALID_IMAGE_FORMAT;
}
#endif

/* EOF */
