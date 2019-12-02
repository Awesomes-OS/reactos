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

#define COR_IS_32BIT_REQUIRED(_flags) \
    (((_flags) & (COMIMAGE_FLAGS_32BITREQUIRED|COMIMAGE_FLAGS_32BITPREFERRED)) == (COMIMAGE_FLAGS_32BITREQUIRED))

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

    LdrpCorExeMainRoutine = RtlEncodeSystemPointer(CorExeMainAddress);
    LdrpCorImageUnloadingRoutine = RtlEncodeSystemPointer(CorImageUnloadingAddress);
    LdrpCorValidateImageRoutine = RtlEncodeSystemPointer(CorValidateImageAddress);
    LdrpMscoreeDllHandle = BaseAddress;
    if (TargetEntry)
    {
#if 1
        DPRINT1("Highly unstable API, fix later");
#else
        *TargetEntry = DllEntry;
#endif
    }

    LdrpFreeUnicodeStringBundle(DllPathBundle);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
LdrpCorValidateImage(IN PVOID ImageBase, IN LPCWSTR FileName)
{
    // Vista+ calls _CorValidateImage, but 10+ doesn't!
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
    Context->Eax = (UINT_PTR)Pointer;
    return Pointer;
}

BOOL
NTAPI
LdrpIsILOnlyImage(PVOID BaseAddress)
{
    ULONG ComSectionSize;
    const PIMAGE_COR20_HEADER ManagedDataImageDirectory = (PIMAGE_COR20_HEADER)RtlImageDirectoryEntryToData(
        BaseAddress,
        TRUE,
        IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
        &ComSectionSize);
    return ManagedDataImageDirectory
        && (ComSectionSize >= sizeof(IMAGE_COR20_HEADER))
        && (ManagedDataImageDirectory->Flags & COMIMAGE_FLAGS_ILONLY);
}

NTSTATUS
NTAPI
LdrpCorProcessImports(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    LdrEntry->DdagNode->State = LdrModulesReadyToInit;
    return STATUS_SUCCESS;
}

/* EOF */
