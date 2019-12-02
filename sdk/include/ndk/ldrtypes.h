/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    ldrtypes.h

Abstract:

    Type definitions for the Loader.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _LDRTYPES_H
#define _LDRTYPES_H

//
// Dependencies
//
#include <umtypes.h>

#ifndef _NTIMAGE_
typedef struct _IMAGE_IMPORT_DESCRIPTOR IMAGE_IMPORT_DESCRIPTOR, *PIMAGE_IMPORT_DESCRIPTOR;
#endif

//
// Resource Type Levels
//
#define RESOURCE_TYPE_LEVEL                     0
#define RESOURCE_NAME_LEVEL                     1
#define RESOURCE_LANGUAGE_LEVEL                 2
#define RESOURCE_DATA_LEVEL                     3

//
// Dll Characteristics for LdrLoadDll
//
#define LDR_IGNORE_CODE_AUTHZ_LEVEL                 0x00001000

//
// LdrAddRef Flags
//
#define LDR_ADDREF_DLL_PIN                          0x00000001

//
// LdrLockLoaderLock Flags
//
#define LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS   0x00000001
#define LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY          0x00000002

//
// LdrUnlockLoaderLock Flags
//
#define LDR_UNLOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS 0x00000001

//
// LdrGetDllHandleEx Flags
//
#define LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT    0x00000001
#define LDR_GET_DLL_HANDLE_EX_PIN                   0x00000002


#define LDR_LOCK_LOADER_LOCK_DISPOSITION_INVALID           0
#define LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_ACQUIRED     1
#define LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_NOT_ACQUIRED 2

//
// FIXME: THIS SHOULD *NOT* BE USED!
//
#define IMAGE_SCN_TYPE_NOLOAD                   0x00000002

//
// Loader datafile/imagemapping macros
//
#define LDR_IS_DATAFILE(handle)     (((ULONG_PTR)(handle)) & (ULONG_PTR)1)
#define LDR_IS_IMAGEMAPPING(handle) (((ULONG_PTR)(handle)) & (ULONG_PTR)2)
#define LDR_IS_RESOURCE(handle)     (LDR_IS_IMAGEMAPPING(handle) || LDR_IS_DATAFILE(handle))

#define LDR_LOADCOUNT_MAX 0xFFFFFFFFul

//
// Activation Context
//
typedef PVOID PACTIVATION_CONTEXT;

//
// Loader Data stored in the PEB
//
typedef struct _PEB_LDR_DATA
{
    ULONG Length;
    BOOLEAN Initialized;
    PVOID SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
    PVOID EntryInProgress;
#if (NTDDI_VERSION >= NTDDI_WIN7)
    UCHAR ShutdownInProgress;
    PVOID ShutdownThreadId;
#endif
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef enum _LDR_DDAG_STATE
{
    LdrModulesMerged = -5,
    LdrModulesInitError = -4,
    LdrModulesSnapError = -3,
    LdrModulesUnloaded = -2,
    LdrModulesUnloading = -1,
    LdrModulesPlaceHolder = 0,
    LdrModulesMapping = 1,
    LdrModulesMapped = 2,
    LdrModulesWaitingForDependencies = 3,
    LdrModulesSnapping = 4,
    LdrModulesSnapped = 5,
    LdrModulesCondensed = 6,
    LdrModulesReadyToInit = 7,
    LdrModulesInitializing = 8,
    LdrModulesReadyToRun = 9
} LDR_DDAG_STATE, *PLDR_DDAG_STATE;

typedef enum _LDR_DLL_LOAD_REASON
{
    LoadReasonStaticDependency = 0,
    LoadReasonStaticForwarderDependency = 1,
    LoadReasonDynamicForwarderDependency = 2,
    LoadReasonDelayloadDependency = 3,
    LoadReasonDynamicLoad = 4,
    LoadReasonAsImageLoad = 5,
    LoadReasonAsDataLoad = 6,
    LoadReasonEnclavePrimary = 7,
    LoadReasonEnclaveDependency = 8,
    LoadReasonUnknown = -1
} LDR_DLL_LOAD_REASON, *PLDR_DLL_LOAD_REASON;

typedef struct _LDRP_CSLIST
{
    PSINGLE_LIST_ENTRY Tail;
} LDRP_CSLIST, *PLDRP_CSLIST;

typedef struct _LDR_SERVICE_TAG_RECORD
{
    struct _LDR_SERVICE_TAG_RECORD* Next;
    ULONG32 ServiceTag;
} LDR_SERVICE_TAG_RECORD, *PLDR_SERVICE_TAG_RECORD;

typedef union _LDRP_PATH_SEARCH_OPTIONS
{
    ULONG32 Flags;

    struct
    {
        ULONG32 Unknown;
    };
} LDRP_PATH_SEARCH_OPTIONS, *PLDRP_PATH_SEARCH_OPTIONS;

typedef struct _LDRP_LOAD_CONTEXT LDRP_LOAD_CONTEXT, *PLDRP_LOAD_CONTEXT;

// Loader Distributed Dependency Graph Node (as in Windows Internals)
// DDAG likely stands for Distributed Dependency Acyclic Graph
typedef struct _LDR_DDAG_NODE
{
    LIST_ENTRY Modules;
    LDR_SERVICE_TAG_RECORD* ServiceTagList;
    ULONG32 LoadCount;
    ULONG32 LoadWhileUnloadingCount;
    ULONG32 LowestLink;
    LDRP_CSLIST Dependencies;
    LDRP_CSLIST IncomingDependencies;
    LDR_DDAG_STATE State;
    SINGLE_LIST_ENTRY CondenseLink;
    ULONG32 PreorderNumber;
} LDR_DDAG_NODE, *PLDR_DDAG_NODE;

//
// Loader Data Table Entry
//
typedef struct _LDR_DATA_TABLE_ENTRY
{
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;

    union
    {
        ULONG32 Flags;

        struct
        {
            ULONG32 PackagedBinary : 1; // 0
            ULONG32 MarkedForRemoval : 1; // 1
            ULONG32 ImageDll : 1; // 2
            ULONG32 LoadNotificationsSent : 1; // 3
            ULONG32 TelemetryEntryProcessed : 1; // 4
            ULONG32 ProcessStaticImport : 1; // 5
            ULONG32 InLegacyLists : 1; // 6
            ULONG32 InIndexes : 1; // 7
            ULONG32 ShimDll : 1; // 8
            ULONG32 InExceptionTable : 1; // 9
            ULONG32 ReservedFlags1 : 1; // 10
            ULONG32 ReactOSSystemMapped : 1; // 11
            ULONG32 LoadInProgress : 1; // 12
            ULONG32 LoadConfigProcessed : 1; // 13
            ULONG32 EntryProcessed : 1; // 14
            ULONG32 ProtectDelayLoad : 1; // 15
            ULONG32 ReactOSLdrSymbolsLoaded : 1; // 16
            ULONG32 ReactOSDriverDependency : 1; // 17
            ULONG32 DontCallForThreads : 1; // 18
            ULONG32 ProcessAttachCalled : 1; // 19
            ULONG32 ProcessAttachFailed : 1; // 20
            ULONG32 CorDeferredValidate : 1; // 21
            ULONG32 CorImage : 1; // 22
            ULONG32 DontRelocate : 1; // 23
            ULONG32 CorILOnly : 1; // 24
            ULONG32 ChpeImage : 1; // 25; CHPE = Compiled Hybrid Portable Executable
            ULONG32 ReactOSDriverVerifying : 1; // 26
            ULONG32 ReactOSNativeMapped : 1; // 27
            ULONG32 Redirected : 1; // 28
            ULONG32 ReactOSShimSuppress : 1; // 29
            ULONG32 ReactOSKernelLoaded : 1; // 30
            ULONG32 CompatDatabaseProcessed : 1; // 31
    };
    };

    UINT16 ObsoleteLoadCount;
    UINT16 TlsIndex;
    LIST_ENTRY HashLinks;
    ULONG32 TimeDateStamp;
    PACTIVATION_CONTEXT EntryPointActivationContext;
    PVOID PatchInformation;
    PLDR_DDAG_NODE DdagNode;
    LIST_ENTRY NodeModuleLink; // LDR_DDAG_NODE.Modules
    PLDRP_LOAD_CONTEXT LoadContext;
    PVOID ParentDllBase;
    UINT_PTR OriginalBase;
    LARGE_INTEGER LoadTime;
    ULONG32 BaseNameHashValue;
    LDR_DLL_LOAD_REASON LoadReason;
    LDRP_PATH_SEARCH_OPTIONS ImplicitPathOptions;
    ULONG32 ReferenceCount;
    ULONG32 DependentLoadFlags;
    PVOID ReactOSLoadedImports; // ARM3 sysldr
    PVOID ReactOSSectionPointer; // ARM3 sysldr
    ULONG ReactOSCheckSum; // ARM3 sysldr
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

//
// Loaded Imports Reference Counting in Kernel
//
typedef struct _LOAD_IMPORTS
{
    SIZE_T Count;
    PLDR_DATA_TABLE_ENTRY Entry[1];
} LOAD_IMPORTS, *PLOAD_IMPORTS;

//
// Loader Resource Information
//
typedef struct _LDR_RESOURCE_INFO
{
    ULONG_PTR Type;
    ULONG_PTR Name;
    ULONG_PTR Language;
} LDR_RESOURCE_INFO, *PLDR_RESOURCE_INFO;

typedef struct _LDR_ENUM_RESOURCE_INFO
{
    ULONG_PTR Type;
    ULONG_PTR Name;
    ULONG_PTR Language;
    PVOID Data;
    SIZE_T Size;
    ULONG_PTR Reserved;
} LDR_ENUM_RESOURCE_INFO, *PLDR_ENUM_RESOURCE_INFO;

//
// DLL Notifications
//
typedef struct _LDR_DLL_LOADED_NOTIFICATION_DATA
{
    ULONG Flags;
    PUNICODE_STRING FullDllName;
    PUNICODE_STRING BaseDllName;
    PVOID DllBase;
    ULONG SizeOfImage;
} LDR_DLL_LOADED_NOTIFICATION_DATA, *PLDR_DLL_LOADED_NOTIFICATION_DATA;

typedef VOID
(NTAPI *PLDR_DLL_LOADED_NOTIFICATION_CALLBACK)(
    _In_ BOOLEAN Type,
    _In_ struct _LDR_DLL_LOADED_NOTIFICATION_DATA *Data
);

typedef struct _LDR_DLL_LOADED_NOTIFICATION_ENTRY
{
    LIST_ENTRY NotificationListEntry;
    PLDR_DLL_LOADED_NOTIFICATION_CALLBACK Callback;
} LDR_DLL_LOADED_NOTIFICATION_ENTRY, *PLDR_DLL_LOADED_NOTIFICATION_ENTRY;

//
// Alternate Resources Support
//
typedef struct _ALT_RESOURCE_MODULE
{
    LANGID LangId;
    PVOID ModuleBase;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PVOID ModuleManifest;
#endif
    PVOID AlternateModule;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    HANDLE AlternateFileHandle;
    ULONG ModuleCheckSum;
    ULONG ErrorCode;
#endif
} ALT_RESOURCE_MODULE, *PALT_RESOURCE_MODULE;

//
// Callback function for LdrEnumerateLoadedModules
//
typedef VOID (NTAPI LDR_ENUM_CALLBACK)(_In_ PLDR_DATA_TABLE_ENTRY ModuleInformation, _In_ PVOID Parameter, _Out_ BOOLEAN *Stop);
typedef LDR_ENUM_CALLBACK *PLDR_ENUM_CALLBACK;

//
// Manifest prober routine set via LdrSetDllManifestProber
//
typedef NTSTATUS (NTAPI LDR_MANIFEST_PROBER_ROUTINE)(_In_ PVOID DllHandle, _In_ PCWSTR FullDllName, _Out_ PVOID *ActCtx);
typedef LDR_MANIFEST_PROBER_ROUTINE *PLDR_MANIFEST_PROBER_ROUTINE;

//
// DLL Main Routine
//
typedef BOOLEAN
(NTAPI *PDLL_INIT_ROUTINE)(
    _In_ PVOID DllHandle,
    _In_ ULONG Reason,
    _In_opt_ PCONTEXT Context
);

#endif
