/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    ldrfuncs.h

Abstract:

    Functions definitions for the Loader.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _LDRFUNCS_H
#define _LDRFUNCS_H

//
// Dependencies
//
#include <ldrtypes.h>

PIMAGE_LOAD_CONFIG_DIRECTORY
NTAPI
LdrImageDirectoryEntryToLoadConfig(_In_ PVOID DllBase);

#ifdef NTOS_MODE_USER

//
// Loader Functions
//

NTSTATUS
NTAPI
LdrAddRefDll(
    _In_ ULONG Flags,
    _In_ PVOID BaseAddress
);

NTSTATUS
NTAPI
LdrDisableThreadCalloutsForDll(
    _In_ PVOID BaseAddress
);

NTSTATUS
NTAPI
LdrGetDllHandle(
    _In_opt_ PWSTR DllPath,
    _In_ PULONG DllCharacteristics,
    _In_ PUNICODE_STRING DllName,
    _Out_ PVOID* DllHandle
);

NTSTATUS
NTAPI
LdrGetDllHandleEx(
    _In_ ULONG Flags,
    _In_opt_ PWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PUNICODE_STRING DllName,
    _Out_opt_ PVOID* DllHandle);

NTSTATUS
NTAPI
LdrFindEntryForAddress(
    _In_ PVOID Address,
    _Out_ PLDR_DATA_TABLE_ENTRY* Module
);

NTSTATUS
NTAPI
LdrGetProcedureAddress(
    _In_ PVOID BaseAddress,
    _In_opt_ PANSI_STRING Name,
    _In_opt_ ULONG Ordinal,
    _Out_ PVOID* ProcedureAddress
);

NTSTATUS
NTAPI
LdrGetProcedureAddressEx(
    _In_ PVOID BaseAddress,
    _In_opt_ PANSI_STRING FunctionName,
    _In_opt_ ULONG Ordinal,
    _Out_ PVOID *ProcedureAddress,
    _In_ UINT8 Flags
);

NTSTATUS
NTAPI
LdrGetProcedureAddressForCaller(
    _In_ PVOID BaseAddress,
    _In_opt_ PANSI_STRING FunctionName,
    _In_opt_ ULONG Ordinal,
    _Out_ PVOID *ProcedureAddress,
    _In_ UINT8 Flags,
    _In_ PVOID *CallbackAddress
);

void
NTAPI
LdrInitializeThunk(
    ULONG Unknown1,
    ULONG Unknown2,
    ULONG Unknown3,
    ULONG Unknown4
);

NTSTATUS
NTAPI
LdrLoadDll(
    _In_opt_ PWSTR SearchPath,
    _In_opt_ PULONG LoadFlags,
    _In_ PUNICODE_STRING Name,
    _Out_opt_ PVOID* BaseAddress
);

PIMAGE_BASE_RELOCATION
NTAPI
LdrProcessRelocationBlock(
    _In_ ULONG_PTR Address,
    _In_ ULONG Count,
    _In_ PUSHORT TypeOffset,
    _In_ LONG_PTR Delta
);

NTSTATUS
NTAPI
LdrQueryImageFileExecutionOptions(
    _In_ PUNICODE_STRING SubKey,
    _In_ PCWSTR ValueName,
    _In_ ULONG ValueSize,
    _Out_ PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG ReturnedLength
);

void
NTAPI
LdrSetDllManifestProber(
    _In_ PLDR_MANIFEST_PROBER_ROUTINE Routine);

void
NTAPI
LdrShutdownProcess(
    void
);

void
NTAPI
LdrShutdownThread(
    void
);

NTSTATUS
NTAPI
LdrUnloadDll(
    _In_ PVOID BaseAddress
);

typedef void(NTAPI* PLDR_CALLBACK)(PVOID CallbackContext, PCHAR Name);
NTSTATUS
NTAPI
LdrVerifyImageMatchesChecksum(
    _In_ HANDLE FileHandle,
    _In_ PLDR_CALLBACK Callback,
    _In_ PVOID CallbackContext,
    _Out_ PUSHORT ImageCharacteristics
);

NTSTATUS
NTAPI
LdrOpenImageFileOptionsKey(
    _In_ PUNICODE_STRING SubKey,
    _In_ BOOLEAN Wow64,
    _Out_ PHANDLE NewKeyHandle
);

NTSTATUS
NTAPI
LdrQueryImageFileKeyOption(
    _In_ HANDLE KeyHandle,
    _In_ PCWSTR ValueName,
    _In_ ULONG Type,
    _Out_ PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG ReturnedLength
);

#endif


//
// Resource Functions
//
NTSTATUS
NTAPI
LdrAccessResource(
    _In_ PVOID BaseAddress,
    _In_ PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry,
    _Out_opt_ PVOID *Resource,
    _Out_opt_ PULONG Size
);

NTSTATUS
NTAPI
LdrFindResource_U(
    _In_ PVOID BaseAddress,
    _In_ PLDR_RESOURCE_INFO ResourceInfo,
    _In_ ULONG Level,
    _Out_ PIMAGE_RESOURCE_DATA_ENTRY *ResourceDataEntry
);

NTSTATUS
NTAPI
LdrEnumResources(
    _In_ PVOID BaseAddress,
    _In_ PLDR_RESOURCE_INFO ResourceInfo,
    _In_ ULONG Level,
    _Inout_ ULONG *ResourceCount,
    _Out_writes_to_(*ResourceCount,*ResourceCount) LDR_ENUM_RESOURCE_INFO *Resources
);


NTSTATUS
NTAPI
LdrFindResourceDirectory_U(
    _In_ PVOID BaseAddress,
    _In_ PLDR_RESOURCE_INFO ResourceInfo,
    _In_ ULONG Level,
    _Out_ PIMAGE_RESOURCE_DIRECTORY *ResourceDirectory
);

NTSTATUS
NTAPI
LdrLoadAlternateResourceModule(
    _In_ PVOID Module,
    _In_ PWSTR Buffer
);

BOOLEAN
NTAPI
LdrUnloadAlternateResourceModule(
    _In_ PVOID BaseAddress
);
//
// Misc. Functions
//

#ifdef NTOS_MODE_USER

NTSTATUS
NTAPI
LdrLockLoaderLock(
    _In_ ULONG Flags,
    _Out_opt_ PULONG Disposition,
    _Out_opt_ PULONG_PTR Cookie
);

NTSTATUS
NTAPI
LdrUnlockLoaderLock(
    _In_ ULONG Flags,
    _In_opt_ ULONG_PTR Cookie
);

#endif

BOOLEAN
NTAPI
LdrVerifyMappedImageMatchesChecksum(
    _In_ PVOID BaseAddress,
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG FileLength
);

PIMAGE_BASE_RELOCATION
NTAPI
LdrProcessRelocationBlockLongLong(
    _In_ ULONG_PTR Address,
    _In_ ULONG Count,
    _In_ PUSHORT TypeOffset,
    _In_ LONGLONG Delta
);

#ifdef NTOS_MODE_USER

NTSTATUS
NTAPI
LdrEnumerateLoadedModules(
    _In_ BOOLEAN ReservedFlag,
    _In_ PLDR_ENUM_CALLBACK EnumProc,
    _In_ PVOID Context
);

#endif

#ifndef NTOS_MODE_USER
typedef struct _SYSTEM_MODULE_INFORMATION LDR_PROCESS_MODULES, *PLDR_PROCESS_MODULES;
#else
typedef struct _RTL_PROCESS_MODULES LDR_PROCESS_MODULES, *PLDR_PROCESS_MODULES;
#endif

NTSTATUS
NTAPI
LdrQueryProcessModuleInformation(
    _In_opt_ PLDR_PROCESS_MODULES ModuleInformation,
    _In_opt_ ULONG Size,
    _Out_ PULONG ReturnedSize
);

#endif
